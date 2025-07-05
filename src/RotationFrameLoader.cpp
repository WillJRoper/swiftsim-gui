#include "RotationFrameLoader.h"
#include "VizTabWidget.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfoList>
#include <QGraphicsOpacityEffect>
#include <QKeyEvent>
#include <QMetaObject>
#include <QRegularExpression>
#include <QVBoxLayout>
#include <algorithm>

/**************************************************************************************************/
/*                                  RotationFrameLoader */
/**************************************************************************************************/
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
  case int(VizTabWidget::Colormap::Greyscale):
  default:
    m_cmap = greyscale_colormap_colormap;
    m_cmap_size = greyscale_colormap_colormap_size;
    break;
  }
}

void RotationFrameLoader::computePercentiles() {
  if (m_fileId < 0 || m_nFrames <= 0)
    return;

  // read only frame 0
  std::vector<float> all(size_t(m_xres) * m_yres);
  hid_t dset =
      H5Dopen2(m_fileId, m_currentDatasetKey.toUtf8().constData(), H5P_DEFAULT);
  hid_t space = H5Dget_space(dset);
  hsize_t offset[3] = {0, 0, 0};
  hsize_t count[3] = {1, (hsize_t)m_xres, (hsize_t)m_yres};
  H5Sselect_hyperslab(space, H5S_SELECT_SET, offset, nullptr, count, nullptr);
  hid_t memsp = H5Screate_simple(3, count, nullptr);
  H5Dread(dset, H5T_NATIVE_FLOAT, memsp, space, H5P_DEFAULT, all.data());
  H5Sclose(memsp);
  H5Sclose(space);
  H5Dclose(dset);

  size_t N = all.size();
  if (N == 0) {
    m_lowerValue = 0.0f;
    m_upperValue = 1.0f;
    return;
  }

  size_t lo = size_t((m_percentileLow / 100.0f) * (N - 1) + 0.5f);
  size_t hi = size_t((m_percentileHigh / 100.0f) * (N - 1) + 0.5f);
  lo = std::clamp(lo, size_t(0), N - 1);
  hi = std::clamp(hi, size_t(0), N - 1);

  std::nth_element(all.begin(), all.begin() + lo, all.end());
  m_lowerValue = all[lo];
  std::nth_element(all.begin(), all.begin() + hi, all.end());
  m_upperValue = all[hi];

  if (m_lowerValue == m_upperValue) {
    m_lowerValue = std::numeric_limits<float>::max();
    m_upperValue = 0.0f;
    for (float v : all) {
      m_lowerValue = std::min(m_lowerValue, v);
      m_upperValue = std::max(m_upperValue, v);
    }
  }

  qInfo() << "Computed percentiles for" << m_currentDatasetKey << ": ["
          << m_lowerValue << ", " << m_upperValue << "] from" << m_percentileLow
          << "% to" << m_percentileHigh << "%";
}

void RotationFrameLoader::startLoading(const QString &imageDirectory,
                                       int fileNumber,
                                       const QString &datasetKey,
                                       float percentileLow,
                                       float percentileHigh, int colormapIdx,
                                       int fps, bool keepPercentiles) {
  // stop old timer & close old file
  if (m_timer) {
    m_timer->stop();
    m_timer->deleteLater();
    m_timer = nullptr;
  }
  if (m_fileId >= 0) {
    H5Fclose(m_fileId);
    m_fileId = -1;
  }

  // stash parameters
  m_imageDirectory = imageDirectory;
  m_currentFileNumber = fileNumber;
  m_currentDatasetKey = datasetKey;
  m_percentileLow = percentileLow;
  m_percentileHigh = percentileHigh;
  setColormap(colormapIdx);

  // build full path
  QString path = m_imageDirectory + "image_" +
                 QString::number(m_currentFileNumber) + ".hdf5";
  if (!QFile::exists(path))
    return;
  if (H5Fis_hdf5(path.toUtf8().constData()) <= 0)
    return;

  // open HDF5
  m_fileId = H5Fopen(path.toUtf8().constData(), H5F_ACC_RDONLY, H5P_DEFAULT);
  if (m_fileId < 0)
    return;

  // query dims
  hid_t dset =
      H5Dopen2(m_fileId, m_currentDatasetKey.toUtf8().constData(), H5P_DEFAULT);
  hid_t space = H5Dget_space(dset);
  hsize_t dims[3];
  H5Sget_simple_extent_dims(space, dims, nullptr);
  m_nFrames = int(dims[0]);
  m_xres = int(dims[1]);
  m_yres = int(dims[2]);
  H5Sclose(space);
  H5Dclose(dset);

  // pre-compute percentiles but only if we are not keeping the last ones (i.e.
  // if the knob is being turned)
  if (!keepPercentiles) {
    computePercentiles();
  }

  // timer for subsequent frames
  m_timer = new QTimer(this);
  m_timer->setInterval(1000 / fps);
  connect(m_timer, &QTimer::timeout, this,
          &RotationFrameLoader::nextRotationFrame);
  m_timer->start();
}

void RotationFrameLoader::nextRotationFrame() {
  if (m_fileId < 0 || m_nFrames <= 0)
    return;

  // increment frame index
  m_currentRotationFrame = (m_currentRotationFrame + 1) % m_nFrames;

  // load the next frame
  loadNextFrame();
}

void RotationFrameLoader::loadNextFrame() {
  if (m_fileId < 0 || m_nFrames <= 0)
    return;

  // read one slice
  hid_t dset =
      H5Dopen2(m_fileId, m_currentDatasetKey.toUtf8().constData(), H5P_DEFAULT);
  hid_t space = H5Dget_space(dset);

  hsize_t offset[3] = {(hsize_t)m_currentRotationFrame, 0, 0};
  hsize_t count[3] = {1, (hsize_t)m_xres, (hsize_t)m_yres};
  H5Sselect_hyperslab(space, H5S_SELECT_SET, offset, nullptr, count, nullptr);
  hid_t memsp = H5Screate_simple(3, count, nullptr);

  std::vector<float> buf(size_t(m_xres) * m_yres);
  H5Dread(dset, H5T_NATIVE_FLOAT, memsp, space, H5P_DEFAULT, buf.data());

  H5Sclose(memsp);
  H5Sclose(space);
  H5Dclose(dset);

  // build RGB image
  QImage img(m_xres, m_yres, QImage::Format_RGB888);
  float range = m_upperValue - m_lowerValue;
  if (range <= 0.0f)
    range = 1.0f;

  for (int y = 0; y < m_yres; ++y) {
    uchar *line = img.scanLine(y);
    for (int x = 0; x < m_xres; ++x) {
      float v = buf[y * m_xres + x];
      if (v == 0.0f) {
        line[x * 3 + 0] = 0;
        line[x * 3 + 1] = 0;
        line[x * 3 + 2] = 0;
      } else {
        float norm = (v - m_lowerValue) / range;
        norm = std::clamp(norm, 0.0f, 1.0f);
        int idx = int(norm * (m_cmap_size - 1) + 0.5f);
        idx = std::clamp(idx, 0, int(m_cmap_size - 1));
        const uint8_t *rgb = m_cmap[idx];
        line[x * 3 + 0] = rgb[0];
        line[x * 3 + 1] = rgb[1];
        line[x * 3 + 2] = rgb[2];
      }
    }
  }

  emit frameReady(img, m_currentFileNumber, m_currentRotationFrame, m_nFrames);
}
