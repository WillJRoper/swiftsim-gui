// VizTabWidget.cpp
#include "VizTabWidget.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfoList>
#include <QKeyEvent>
#include <QMetaObject>
#include <QRegularExpression>
#include <QVBoxLayout>
#include <algorithm>
#include <thread>
#include <vector>

/**************************************************************************************************/
/*                                        Constructor / Destructor */
/**************************************************************************************************/
VizTabWidget::VizTabWidget(QWidget *parent)
    : QWidget(parent), m_imageLabel(new ScaledPixmapLabel(this)),
      m_overlayLabel(new QLabel(this)), m_timer(new QTimer(this)),
      m_logoLabel(new QLabel(this)) {
  // Layout
  auto *lay = new QVBoxLayout(this);
  lay->setContentsMargins(0, 0, 0, 0);
  lay->addWidget(m_imageLabel);
  setLayout(lay);

  // Overlay styling
  m_overlayLabel->setStyleSheet(
      "QLabel { background: rgba(0,0,0,128); color: white; "
      "padding:4px; border-radius:3px; font:10pt 'Arial'; }");
  m_overlayLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
  m_overlayLabel->move(10, 10);
  m_overlayLabel->show();

  // Timer drives the rotation
  connect(m_timer, &QTimer::timeout, this, &VizTabWidget::advanceRotationFrame);
  m_timer->start(1000 / m_fps);

  // Set up our default colormap:
  setColormap(Colormap::Plasma);

  // Create the logo label (bottom‐right watermark)
  m_logoOrig = QPixmap(QStringLiteral(":/images/swift-logo-white.png"));
  Q_ASSERT(!m_logoOrig.isNull());
  m_logoLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
  m_logoLabel->setStyleSheet("background:transparent;");
  m_logoLabel->raise();
  m_logoLabel->show();

  // Directory watcher → slot
  connect(&m_dirWatcher, &QFileSystemWatcher::directoryChanged, this,
          &VizTabWidget::onImageDirectoryChanged);
}

VizTabWidget::~VizTabWidget() {
  if (m_fileId >= 0) {
    H5Fclose(m_fileId);
    m_fileId = -1;
  }
}

QString VizTabWidget::currentFilePath() {
  // Return the full path to the current file
  return m_imageDirectory + "image_" + QString::number(m_currentFileNumber) +
         m_hdf5Ext;
}

void VizTabWidget::setPercentileRange(float low, float high) {
  m_percentileLow = qBound(0.0f, low, 100.0f);
  m_percentileHigh = qBound(0.0f, high, 100.0f);
  if (m_fileId >= 0) {
    computePercentiles();
    loadCurrentFrame();
  }
}

void VizTabWidget::percentileRange(float &low, float &high) const {
  low = m_percentileLow;
  high = m_percentileHigh;
}

/**************************************************************************************************/
/*                                     Public slots */
/**************************************************************************************************/
void VizTabWidget::openHDF5File() {
  // 1) Build the full path
  QString path = currentFilePath();
  if (!QFile::exists(path)) {
    qWarning() << "VizTabWidget: HDF5 file" << path << "does not exist.";
    return;
  }

  // 2) Give UI feedback & pause rotation
  QApplication::setOverrideCursor(Qt::BusyCursor);
  m_timer->stop();

  // 3) Wait until it’s a valid HDF5 superblock (up to 5s)
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (std::chrono::steady_clock::now() < deadline) {
    if (H5Fis_hdf5(path.toUtf8().constData()) > 0)
      break;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  if (!H5Fis_hdf5(path.toUtf8().constData())) {
    qWarning() << "VizTabWidget: timeout waiting for valid HDF5:" << path;
    QApplication::restoreOverrideCursor();
    m_timer->start(1000 / m_fps);
    return;
  }

  // 4) Close any previously open file
  if (m_fileId >= 0) {
    H5Fclose(m_fileId);
    m_fileId = -1;
  }

  // 5) Open the new file
  m_fileId = H5Fopen(path.toUtf8().constData(), H5F_ACC_RDONLY, H5P_DEFAULT);
  if (m_fileId < 0) {
    qWarning() << "VizTabWidget: failed to open HDF5 file" << path;
    QApplication::restoreOverrideCursor();
    m_timer->start(1000 / m_fps);
    return;
  }

  // 6) Query dimensions from the current dataset
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

  // 7) Compute percentiles (now fast, single-frame)
  computePercentiles();

  // 8) Restore UI and show frame 0
  QApplication::restoreOverrideCursor();
  m_timer->start(1000 / m_fps);

  // 9) Load & display the very first frame
  loadCurrentFrame();
}

void VizTabWidget::computePercentiles() {
  // Nothing to do if we don’t have a file or no frames
  if (m_fileId < 0 || m_nFrames <= 0)
    return;

  // 1) Open the dataset for the current key
  hid_t dset =
      H5Dopen2(m_fileId, m_currentDatasetKey.toUtf8().constData(), H5P_DEFAULT);
  if (dset < 0) {
    qWarning() << "computePercentiles(): failed to open dataset"
               << m_currentDatasetKey;
    return;
  }

  // 2) Read just frame 0 into a flat buffer
  std::vector<float> all(size_t(m_xres) * m_yres);
  {
    // select hyperslab [frame=0, full xres×yres]
    hid_t space = H5Dget_space(dset);
    hsize_t offset[3] = {0, 0, 0};
    hsize_t count[3] = {1, (hsize_t)m_xres, (hsize_t)m_yres};
    H5Sselect_hyperslab(space, H5S_SELECT_SET, offset, nullptr, count, nullptr);
    hid_t memsp = H5Screate_simple(3, count, nullptr);

    herr_t err =
        H5Dread(dset, H5T_NATIVE_FLOAT, memsp, space, H5P_DEFAULT, all.data());
    if (err < 0) {
      qWarning("computePercentiles(): H5Dread failed");
    }

    H5Sclose(memsp);
    H5Sclose(space);
  }

  // 3) Close the dataset
  H5Dclose(dset);

  // 4) Compute percentile‐indices
  size_t N = all.size();
  if (N == 0) {
    m_lowerValue = 0.0f;
    m_upperValue = 1.0f;
    return;
  }
  size_t loIdx = size_t((m_percentileLow / 100.0f) * (N - 1) + 0.5f);
  size_t hiIdx = size_t((m_percentileHigh / 100.0f) * (N - 1) + 0.5f);
  loIdx = std::clamp(loIdx, size_t(0), N - 1);
  hiIdx = std::clamp(hiIdx, size_t(0), N - 1);

  // 5) Extract the two cut‐points via nth_element
  std::nth_element(all.begin(), all.begin() + loIdx, all.end());
  m_lowerValue = all[loIdx];

  std::nth_element(all.begin(), all.begin() + hiIdx, all.end());
  m_upperValue = all[hiIdx];

  // It's possible that when there are few populate pixels that both upper and
  // lower values are the same, in which case we need can fall back to a clean
  // minimum and maximum.
  if (m_lowerValue == m_upperValue) {
    m_lowerValue = std::numeric_limits<float>::max();
    m_upperValue = 0.0f;
    for (float val : all) {
      if (val < m_lowerValue)
        m_lowerValue = val;
      if (val > m_upperValue)
        m_upperValue = val;
    }
  }
}

void VizTabWidget::setColormap(Colormap map) {
  m_colormap = map;
  switch (map) {
  case Colormap::Plasma:
    m_cmap = plasma_colormap;
    m_cmap_size = plasma_colormap_size;
    break;
  case Colormap::Magma:
    m_cmap = magma_colormap_colormap;
    m_cmap_size = magma_colormap_colormap_size;
    break;
  case Colormap::Viridis:
    m_cmap = viridis_colormap_colormap;
    m_cmap_size = viridis_colormap_colormap_size;
    break;
  case Colormap::Jet:
    m_cmap = jet_colormap_colormap;
    m_cmap_size = jet_colormap_colormap_size;
    break;
  case Colormap::Inferno:
    m_cmap = inferno_colormap_colormap;
    m_cmap_size = inferno_colormap_colormap_size;
    break;
  case Colormap::Greyscale:
    m_cmap = greyscale_colormap_colormap;
    m_cmap_size = greyscale_colormap_colormap_size;
    break;
  }
}

void VizTabWidget::setDatasetKey(const QString &key) {
  if (key == m_currentDatasetKey)
    return;
  m_currentDatasetKey = key;

  // Set the appropriate colormap
  if (key == "dark_matter") {
    setColormap(Colormap::Plasma);
  } else if (key == "gas") {
    setColormap(Colormap::Magma);
  } else if (key == "stars") {
    setColormap(Colormap::Greyscale);
  } else if (key == "gas_temperature") {
    setColormap(Colormap::Inferno);
  } else {
    qWarning("VizTabWidget: Unknown dataset key '%s'. Using default colormap.",
             qPrintable(key));
    setColormap(Colormap::Greyscale);
  }

  // Compute the percentiles for the new dataset
  computePercentiles();

  // if a file is already open, re-open it to re-query dims
  if (m_fileId >= 0)
    loadCurrentFrame();
}

void VizTabWidget::addNewFile(int fileNumber) {
  m_latestFileNumber = std::max(m_latestFileNumber, fileNumber);
}

void VizTabWidget::setCurrentFileNumber(int idx) {
  // If the index is the same as the current, do nothing
  if (idx == m_currentFileNumber)
    return;

  // Otherwise, make sure it’s within bounds
  m_currentFileNumber = std::clamp(idx, 0, m_latestFileNumber);

  // And update everything for this new file
  openHDF5File();
  loadCurrentFrame();
}

void VizTabWidget::watchImageDirectory(const QString &directory) {
  m_imageDirectory = directory;

  // Ensure the directory ends with a slash
  if (!m_imageDirectory.endsWith('/')) {
    m_imageDirectory += '/';
  }

  // clear any old paths:
  m_dirWatcher.removePaths(m_dirWatcher.directories());

  if (QDir(directory).exists()) {
    m_dirWatcher.addPath(directory);
    scanImageDirectory();
  } else {
    qWarning() << "VizTabWidget: cannot watch, directory does not exist:"
               << directory;
  }
}

void VizTabWidget::onImageDirectoryChanged(const QString & /*path*/) {
  scanImageDirectory();
}

/**************************************************************************************************/
/*                                     Event handlers */
/**************************************************************************************************/
void VizTabWidget::resizeEvent(QResizeEvent *ev) {
  QWidget::resizeEvent(ev);

  // 1) Top-left overlay
  m_overlayLabel->move(10, 10);
  m_overlayLabel->raise();

  // 2) Scale the logo to ~15% of widget height
  const int margin = 10;
  int maxH = height() * 20 / 100; // 15% of total height
  QSize target(maxH * m_logoOrig.width() / m_logoOrig.height(), maxH);
  QPixmap scaled =
      m_logoOrig.scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation);

  // 3) Apply pixmap + resize label
  m_logoLabel->setPixmap(scaled);
  m_logoLabel->setFixedSize(scaled.size());

  // 4) Reposition bottom-right
  int x = width() - scaled.width() - margin;
  int y = height() - scaled.height() - margin;
  m_logoLabel->move(x, y);
  m_logoLabel->raise();

  // 5) Finally redraw your frame
  loadCurrentFrame();
}

void VizTabWidget::keyPressEvent(QKeyEvent *evt) {
  switch (evt->key()) {
  case Qt::Key_1:
    setDatasetKey("dark_matter");
    break;
  case Qt::Key_2:
    setDatasetKey("gas");
    break;
  case Qt::Key_3:
    setDatasetKey("stars");
    break;
  case Qt::Key_4:
    setDatasetKey("gas_temperature");
    break;
  default:
    QWidget::keyPressEvent(evt);
  }
}

/**************************************************************************************************/
/*                                     Timer slot */
/**************************************************************************************************/
void VizTabWidget::advanceRotationFrame() {
  if (m_fileId < 0 || m_nFrames == 0)
    return;
  m_currentRotationFrame = (m_currentRotationFrame + 1) % m_nFrames;
  loadCurrentFrame();
}

/**************************************************************************************************/
/*                                 Frame loading & display */
/**************************************************************************************************/
void VizTabWidget::loadCurrentFrame() {
  if (m_fileId < 0 || m_cmap == nullptr)
    return;

  // 1) Read one rotation‐slice of floats
  hid_t dset =
      H5Dopen2(m_fileId, m_currentDatasetKey.toUtf8().constData(), H5P_DEFAULT);
  hid_t space = H5Dget_space(dset);

  hsize_t offset[3] = {static_cast<hsize_t>(m_currentRotationFrame), 0, 0};
  hsize_t count[3] = {1, static_cast<hsize_t>(m_xres),
                      static_cast<hsize_t>(m_yres)};
  H5Sselect_hyperslab(space, H5S_SELECT_SET, offset, nullptr, count, nullptr);
  hid_t memsp = H5Screate_simple(3, count, nullptr);

  std::vector<float> buf(static_cast<size_t>(m_xres) * m_yres);
  H5Dread(dset, H5T_NATIVE_FLOAT, memsp, space, H5P_DEFAULT, buf.data());

  H5Sclose(memsp);
  H5Sclose(space);
  H5Dclose(dset);

  // 2) Normalize using global min/max
  float range = m_upperValue - m_lowerValue;
  if (range <= 0.0f)
    range = 1.0f;

  // 3) Build an RGB image via the colormap
  QImage img(m_xres, m_yres, QImage::Format_RGB888);
  for (int y = 0; y < m_yres; ++y) {
    uchar *line = img.scanLine(y);
    for (int x = 0; x < m_xres; ++x) {
      float val = buf[y * m_xres + x];

      // **Force true zeros to black**
      if (val == 0.0f) {
        int base = x * 3;
        line[base] = 0;
        line[base + 1] = 0;
        line[base + 2] = 0;
        continue;
      }

      // normalize using global min/max
      float norm = (val - m_lowerValue) / range;
      norm = qBound(0.0f, norm, 1.0f);

      // lookup in colormap
      int idx = int(norm * (m_cmap_size - 1) + 0.5f);
      idx = qBound(0, idx, int(m_cmap_size - 1));

      const uint8_t *rgb = m_cmap[idx];
      int base = x * 3;
      line[base] = rgb[0];
      line[base + 1] = rgb[1];
      line[base + 2] = rgb[2];
    }
  }

  // 4) Display it
  m_imageLabel->setPixmapKeepingAspect(QPixmap::fromImage(img));

  // 5) Update overlay
  m_overlayLabel->setText(QString("File %1/%2  Rot %3/%4  [%5]")
                              .arg(m_currentFileNumber)
                              .arg(m_latestFileNumber)
                              .arg(m_currentRotationFrame)
                              .arg(m_nFrames)
                              .arg(m_currentDatasetKey));
  m_overlayLabel->adjustSize();
}

/**************************************************************************************************/
/*                                 Directory scanning */
/**************************************************************************************************/
void VizTabWidget::scanImageDirectory() {
  if (m_imageDirectory.isEmpty())
    return;

  QDir dir(m_imageDirectory);
  // look for all .hdf5 files
  QStringList files = dir.entryList(QStringList() << QStringLiteral("*.hdf5"),
                                    QDir::Files, QDir::Name);

  // regex to extract the trailing integer before ".hdf5"
  static const QRegularExpression re(R"(_(\d+)\.hdf5$)");

  int maxIdx = m_latestFileNumber;
  for (const QString &fn : files) {
    auto match = re.match(fn);
    if (match.hasMatch()) {
      bool ok = false;
      int idx = match.captured(1).toInt(&ok);
      if (ok && idx > maxIdx) {
        maxIdx = idx;
      }
    }
  }

  // if we found new files, call addNewFile() for each one in turn
  addNewFile(maxIdx);

  // Ok, lets set the current to this maxIdx so it gets visualised
  setCurrentFileNumber(maxIdx);
}
