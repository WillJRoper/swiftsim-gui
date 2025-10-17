// RotationFrameLoader.cpp
#include "RotationFrameLoader.h"
#include "VizTabWidget.h"
#include <QFile>
#include <algorithm>
#include <limits>

RotationFrameLoader::RotationFrameLoader(QObject *parent)
    : QObject(parent), m_timer(new QTimer(this)) {
  // Always-on rotation timer
  connect(m_timer, &QTimer::timeout, this,
          &RotationFrameLoader::nextRotationFrame);
  m_timer->start(1000 / m_fps);
}

RotationFrameLoader::~RotationFrameLoader() {
  m_timer->stop();
  // tear down HDF5 in reverse order
  if (m_memSpace >= 0)
    H5Sclose(m_memSpace);
  if (m_fileSpace >= 0)
    H5Sclose(m_fileSpace);
  if (m_dsetId >= 0)
    H5Dclose(m_dsetId);
  if (m_fileId >= 0)
    H5Fclose(m_fileId);
}

void RotationFrameLoader::startLoading(const QString &imageDirectory,
                                       int fileNumber,
                                       const QString &datasetKey,
                                       int colormapIdx, int fps,
                                       bool keepPercentiles) {
  // stash parameters
  m_imageDirectory = imageDirectory;
  m_currentFileNumber = fileNumber;
  m_currentDatasetKey = datasetKey;
  m_fps = fps;
  m_colormapIdx = colormapIdx;
  setColormap(colormapIdx);

  // close previous HDF5 handles
  if (m_memSpace >= 0) {
    H5Sclose(m_memSpace);
    m_memSpace = -1;
  }
  if (m_fileSpace >= 0) {
    H5Sclose(m_fileSpace);
    m_fileSpace = -1;
  }
  if (m_dsetId >= 0) {
    H5Dclose(m_dsetId);
    m_dsetId = -1;
  }
  if (m_fileId >= 0) {
    H5Fclose(m_fileId);
    m_fileId = -1;
  }

  // Open file with a larger chunk/cache (32 MiB) for better throughput
  hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
  H5Pset_cache(fapl,
               /*mdc_nelmts=*/0,    // 0 = default # of metadata entries
               /*rdcc_nslots=*/521, // raw-data chunk cache slots
               /*rdcc_nbytes=*/32 * 1024 * 1024, // 32 MiB chunk cache
               /*rdcc_w0=*/0.75);                // preemption policy
  QString path =
      m_imageDirectory + QString("image_%1.hdf5").arg(m_currentFileNumber);
  m_fileId = H5Fopen(path.toUtf8().constData(), H5F_ACC_RDONLY, fapl);
  H5Pclose(fapl);

  // open dataset + get its dataspace
  m_dsetId =
      H5Dopen2(m_fileId, m_currentDatasetKey.toUtf8().constData(), H5P_DEFAULT);
  m_fileSpace = H5Dget_space(m_dsetId);

  // Read the step and age attributes from the root group
  hid_t rootGroup = H5Gopen2(m_fileId, "/", H5P_DEFAULT);
  if (rootGroup >= 0) {
    // Unused currently!
    // hid_t stepAttr = H5Aopen_name(rootGroup, "step");
    // if (stepAttr >= 0) {
    //   H5Aread(stepAttr, H5T_NATIVE_INT, &m_currentStep);
    //   H5Aclose(stepAttr);
    // } else {
    //   qWarning() << "Failed to read 'step' attribute.";
    // }

    hid_t ageAttr = H5Aopen_name(rootGroup, "age");
    if (ageAttr >= 0) {
      H5Aread(ageAttr, H5T_NATIVE_DOUBLE, &m_currentAge);
      H5Aclose(ageAttr);
      emit ageChanged(static_cast<long long>(m_currentAge * 1e9));

      // Emit percent clamped to [0, 100]
      int intPercent = static_cast<int>(m_currentAge / 13.81 * 100);
      emit percentChanged(std::clamp(intPercent, 0, 100));
    } else {
      qWarning() << "Failed to read 'age' attribute.";
    }

    H5Gclose(rootGroup);
  } else {
    qWarning() << "Failed to open root group.";
  }

  // query full dims [frames, x, y]
  hsize_t fullDims[3];
  H5Sget_simple_extent_dims(m_fileSpace, fullDims, nullptr);
  m_nFrames = int(fullDims[0]);
  m_xres = int(fullDims[1]);
  m_yres = int(fullDims[2]);

  // create one‐slice memspace
  hsize_t count[3] = {1, fullDims[1], fullDims[2]};
  m_memSpace = H5Screate_simple(3, count, nullptr);

  // allocate reusable buffers
  m_buf.assign(size_t(m_xres) * m_yres, 0.0f);
  m_img = QImage(m_xres, m_yres, QImage::Format_RGB888);

  // percentile compute
  if (!keepPercentiles)
    computePercentiles();
}

void RotationFrameLoader::jumpToFile(int fileNumber, bool keepPercentiles) {
  // under the same rotation clock, just reopen
  startLoading(m_imageDirectory, fileNumber, m_currentDatasetKey, m_colormapIdx,
               m_fps, keepPercentiles);
}

/**
 * @brief Computes the lower and upper percentiles for the current latest file.
 *
 * The minimum and maximum values computed here will be used to normalize all
 * preceding frames and get updates when there is a new file.
 */
void RotationFrameLoader::computePercentiles() {
  if (m_dsetId < 0 || m_nFrames <= 0)
    return;

  // Open the latest file, we always scale the latest file
  QString path =
      m_imageDirectory + QString("image_%1.hdf5").arg(m_latestFileNumber);
  hid_t fileId =
      H5Fopen(path.toUtf8().constData(), H5F_ACC_RDONLY, H5P_DEFAULT);

  // First do the dark matter dataset

  // Open the dark matter dataset
  hid_t dsetId = H5Dopen2(fileId, "dark_matter", H5P_DEFAULT);
  hid_t fileSpace = H5Dget_space(m_dsetId);
  hsize_t fullDims[3];
  H5Sget_simple_extent_dims(fileSpace, fullDims, nullptr);
  hsize_t count[3] = {1, fullDims[1], fullDims[2]};
  hid_t memSpace = H5Screate_simple(3, count, nullptr);

  // Allocate buffer for reading
  std::vector<float> buf(size_t(m_xres) * m_yres, 0.0f);

  // read slice 0
  hsize_t offset[3] = {0, 0, 0};
  H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, offset, nullptr,
                      (hsize_t[]){1, (hsize_t)m_xres, (hsize_t)m_yres},
                      nullptr);
  H5Dread(dsetId, H5T_NATIVE_FLOAT, memSpace, fileSpace, H5P_DEFAULT,
          buf.data());
  size_t N = m_buf.size();
  if (N == 0) {
    m_lowerValueDM = 0;
    m_upperValueDM = 1;
    return;
  }
  size_t lo = size_t((m_percentileLowDM / 100.f) * (N - 1) + .5f);
  size_t hi = size_t((m_percentileHighDM / 100.f) * (N - 1) + .5f);
  lo = std::clamp(lo, size_t(0), N - 1);
  hi = std::clamp(hi, size_t(0), N - 1);
  std::nth_element(buf.begin(), buf.begin() + lo, buf.end());
  m_lowerValueDM = buf[lo];
  std::nth_element(buf.begin(), buf.begin() + hi, buf.end());
  m_upperValueDM = buf[hi];

  // If these are the same, fall back to min/max
  if (m_lowerValueDM == m_upperValueDM) {
    m_lowerValueDM = std::numeric_limits<float>::max();
    m_upperValueDM = std::numeric_limits<float>::lowest();
    for (float v : buf) {
      m_lowerValueDM = std::min(m_lowerValueDM, v);
      m_upperValueDM = std::max(m_upperValueDM, v);
    }
  }

  // Now do the gas dataset

  dsetId = H5Dopen2(fileId, "gas", H5P_DEFAULT);
  fileSpace = H5Dget_space(dsetId);
  H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, offset, nullptr,
                      (hsize_t[]){1, (hsize_t)m_xres, (hsize_t)m_yres},
                      nullptr);
  H5Dread(dsetId, H5T_NATIVE_FLOAT, memSpace, fileSpace, H5P_DEFAULT,
          buf.data());
  N = m_buf.size();
  if (N == 0) {
    m_lowerValueGas = 0;
    m_upperValueGas = 1;
    return;
  }
  lo = size_t((m_percentileLowGas / 100.f) * (N - 1) + .5f);
  hi = size_t((m_percentileHighGas / 100.f) * (N - 1) + .5f);
  lo = std::clamp(lo, size_t(0), N - 1);
  hi = std::clamp(hi, size_t(0), N - 1);
  std::nth_element(buf.begin(), buf.begin() + lo, buf.end());
  m_lowerValueGas = buf[lo];
  std::nth_element(buf.begin(), buf.begin() + hi, buf.end());
  m_upperValueGas = buf[hi];

  // If these are the same, fall back to min/max
  if (m_lowerValueGas == m_upperValueGas) {
    m_lowerValueGas = std::numeric_limits<float>::max();
    m_upperValueGas = std::numeric_limits<float>::lowest();
    for (float v : buf) {
      m_lowerValueGas = std::min(m_lowerValueGas, v);
      m_upperValueGas = std::max(m_upperValueGas, v);
    }
  }

  // Now do the stars dataset
  dsetId = H5Dopen2(fileId, "stars", H5P_DEFAULT);
  fileSpace = H5Dget_space(dsetId);
  H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, offset, nullptr,
                      (hsize_t[]){1, (hsize_t)m_xres, (hsize_t)m_yres},
                      nullptr);
  H5Dread(dsetId, H5T_NATIVE_FLOAT, memSpace, fileSpace, H5P_DEFAULT,
          buf.data());
  N = m_buf.size();
  if (N == 0) {
    m_lowerValueStars = 0;
    m_upperValueStars = 1;
    return;
  }
  lo = size_t((m_percentileLowStars / 100.f) * (N - 1) + .5f);
  hi = size_t((m_percentileHighStars / 100.f) * (N - 1) + .5f);
  lo = std::clamp(lo, size_t(0), N - 1);
  hi = std::clamp(hi, size_t(0), N - 1);
  std::nth_element(buf.begin(), buf.begin() + lo, buf.end());
  m_lowerValueStars = buf[lo];
  std::nth_element(buf.begin(), buf.begin() + hi, buf.end());
  m_upperValueStars = buf[hi];

  // If these are the same, fall back to min/max
  if (m_lowerValueStars == m_upperValueStars) {
    m_lowerValueStars = std::numeric_limits<float>::max();
    m_upperValueStars = std::numeric_limits<float>::lowest();
    for (float v : buf) {
      m_lowerValueStars = std::min(m_lowerValueStars, v);
      m_upperValueStars = std::max(m_upperValueStars, v);
    }
  }

  // Now do the gas temperature dataset
  dsetId = H5Dopen2(fileId, "gas_temperature", H5P_DEFAULT);
  fileSpace = H5Dget_space(dsetId);
  H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, offset, nullptr,
                      (hsize_t[]){1, (hsize_t)m_xres, (hsize_t)m_yres},
                      nullptr);
  H5Dread(dsetId, H5T_NATIVE_FLOAT, memSpace, fileSpace, H5P_DEFAULT,
          buf.data());
  N = m_buf.size();
  if (N == 0) {
    m_lowerValueTemp = 0;
    m_upperValueTemp = 1;
    return;
  }
  lo = size_t((m_percentileLowTemp / 100.f) * (N - 1) + .5f);
  hi = size_t((m_percentileHighTemp / 100.f) * (N - 1) + .5f);
  lo = std::clamp(lo, size_t(0), N - 1);
  hi = std::clamp(hi, size_t(0), N - 1);
  std::nth_element(buf.begin(), buf.begin() + lo, buf.end());
  m_lowerValueTemp = buf[lo];
  std::nth_element(buf.begin(), buf.begin() + hi, buf.end());
  m_upperValueTemp = buf[hi];

  // If these are the same, fall back to min/max
  if (m_lowerValueTemp == m_upperValueTemp) {
    m_lowerValueTemp = std::numeric_limits<float>::max();
    m_upperValueTemp = std::numeric_limits<float>::lowest();
    for (float v : buf) {
      m_lowerValueTemp = std::min(m_lowerValueTemp, v);
      m_upperValueTemp = std::max(m_upperValueTemp, v);
    }
  }

  // Close all HDF5 handles
  H5Sclose(memSpace);
  H5Sclose(fileSpace);
  H5Dclose(dsetId);
  H5Fclose(fileId);
}

void RotationFrameLoader::setColormap(int colormapIdx) {
  switch (colormapIdx) {
  case int(VizTabWidget::Colormap::Plasma):
    m_cmap = plasma_colormap;
    m_cmap_size = plasma_colormap_size;
    break;
  case int(VizTabWidget::Colormap::Magma):
    m_cmap = magma_colormap_colormap;
    m_cmap_size = magma_colormap_colormap_size;
    break;
  case int(VizTabWidget::Colormap::Viridis):
    m_cmap = viridis_colormap_colormap;
    m_cmap_size = viridis_colormap_colormap_size;
    break;
  case int(VizTabWidget::Colormap::Jet):
    m_cmap = jet_colormap_colormap;
    m_cmap_size = jet_colormap_colormap_size;
    break;
  case int(VizTabWidget::Colormap::Inferno):
    m_cmap = inferno_colormap_colormap;
    m_cmap_size = inferno_colormap_colormap_size;
    break;
  case int(VizTabWidget::Colormap::SpeakNow):
    m_cmap = speaknow_colormap;
    m_cmap_size = speaknow_colormap_size;
    break;
  case int(VizTabWidget::Colormap::Copper):
    m_cmap = copper_colormap;
    m_cmap_size = copper_colormap_size;
    break;
  case int(VizTabWidget::Colormap::Cosmic):
    m_cmap = cosmic_colormap;
    m_cmap_size = cosmic_colormap_size;
    break;
  case int(VizTabWidget::Colormap::Greyscale):
  default:
    m_cmap = greyscale_colormap_colormap;
    m_cmap_size = greyscale_colormap_colormap_size;
    break;
  }
}

double RotationFrameLoader::minValue() const {
  if (m_currentDatasetKey == "dark_matter")
    return m_lowerValueDM;
  else if (m_currentDatasetKey == "gas")
    return m_lowerValueGas;
  else if (m_currentDatasetKey == "stars")
    return m_lowerValueStars;
  else if (m_currentDatasetKey == "gas_temperature")
    return m_lowerValueTemp;
  else
    return 0.0; // fallback
}

double RotationFrameLoader::maxValue() const {
  if (m_currentDatasetKey == "dark_matter")
    return m_upperValueDM;
  else if (m_currentDatasetKey == "gas")
    return m_upperValueGas;
  else if (m_currentDatasetKey == "stars")
    return m_upperValueStars;
  else if (m_currentDatasetKey == "gas_temperature")
    return m_upperValueTemp;
  else
    return 1.0; // fallback
}

void RotationFrameLoader::nextRotationFrame() {
  if (m_dsetId < 0 || m_nFrames <= 0)
    return;

  m_currentRotationFrame = (m_currentRotationFrame + 1) % m_nFrames;
  loadNextFrame();
}

void RotationFrameLoader::loadNextFrame() {
  if (m_dsetId < 0 || m_nFrames <= 0)
    return;

  // read current slice
  hsize_t offset[3] = {(hsize_t)m_currentRotationFrame, 0, 0};
  H5Sselect_hyperslab(m_fileSpace, H5S_SELECT_SET, offset, nullptr,
                      (hsize_t[]){1, (hsize_t)m_xres, (hsize_t)m_yres},
                      nullptr);
  H5Dread(m_dsetId, H5T_NATIVE_FLOAT, m_memSpace, m_fileSpace, H5P_DEFAULT,
          m_buf.data());

  // apply colormap into m_img
  float min = minValue();
  float max = maxValue();
  float range = max - min;
  if (range <= 0)
    range = 1.0f;

  // pick your stretch strength (higher → more pop in the shadows)
  constexpr float asinhStretchFactor = 9.0f;
  // precompute denominator so we only call asinh() once
  const float asinhNormalizationDenominator = std::asinh(asinhStretchFactor);

  // compute int upper bound for clamp exactly once
  int maxColorMapIndex = static_cast<int>(m_cmap_size) - 1;

  for (int y = 0; y < m_yres; ++y) {
    uchar *scanLine = m_img.scanLine(y);

    for (int x = 0; x < m_xres; ++x) {
      float rawBufferValue = m_buf[y * m_xres + x];

      if (rawBufferValue <= 0.0f) {
        // background
        scanLine[3 * x + 0] = 0;
        scanLine[3 * x + 1] = 0;
        scanLine[3 * x + 2] = 0;
      } else {
        // 1) linear normalize
        float normalizedValue = (rawBufferValue - min) / range;
        normalizedValue = std::clamp(normalizedValue, 0.0f, 1.0f);

        // 2) arcsinh stretch
        float stretchedValue =
            std::asinh(asinhStretchFactor * normalizedValue) /
            asinhNormalizationDenominator;
        stretchedValue = std::clamp(stretchedValue, 0.0f, 1.0f);

        // 3) map into colormap
        int colorMapIndex = int(stretchedValue * maxColorMapIndex + 0.5f);
        colorMapIndex = std::clamp(colorMapIndex, 0, maxColorMapIndex);

        const uint8_t *rgbTriplet = m_cmap[colorMapIndex];
        scanLine[3 * x + 0] = rgbTriplet[0];
        scanLine[3 * x + 1] = rgbTriplet[1];
        scanLine[3 * x + 2] = rgbTriplet[2];
      }
    }
  }

  emit frameReady(m_img, m_currentFileNumber, m_currentRotationFrame,
                  m_nFrames);
}
