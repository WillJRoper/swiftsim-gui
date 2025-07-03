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
}

void RotationFrameLoader::startLoading(
    const QString &imageDirectory, int fileNumber, const QString &datasetKey,
    float percentileLow, float percentileHigh, int colormapIdx, int fps) {
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

  // pre-compute percentiles
  computePercentiles();

  // timer for subsequent frames
  m_timer = new QTimer(this);
  m_timer->setInterval(1000 / fps);
  connect(m_timer, &QTimer::timeout, this, &RotationFrameLoader::loadNextFrame);
  m_timer->start();
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

  m_currentRotationFrame = (m_currentRotationFrame + 1) % m_nFrames;
}

/**************************************************************************************************/
/*                                        VizTabWidget */
/**************************************************************************************************/
VizTabWidget::VizTabWidget(QWidget *parent)
    : QWidget(parent), m_imageLabel(new ScaledPixmapLabel(this)),
      m_logoLabel(new QLabel(this)), m_flamingoLabel(new QLabel(this)),
      m_esaLabel(new QLabel(this)), m_sussexLabel(new QLabel(this)),
      m_loader(new RotationFrameLoader), m_loaderThread(new QThread(this)) {
  // --- Title label ---
  m_titleLabel = new QLabel(tr("Dark Matter"), this);
  m_titleLabel->setObjectName("vizTitleLabel");
  m_titleLabel->setAlignment(Qt::AlignCenter);
  m_titleLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

  // Layout
  auto *lay = new QVBoxLayout(this);
  lay->setContentsMargins(0, 0, 0, 0);

  // Put the title on top with no stretch
  lay->addWidget(m_titleLabel, /*stretch=*/0);

  // Then the image expands to fill
  lay->addWidget(m_imageLabel, /*stretch=*/1);

  // --- bottom-right SWIFT logo ---
  m_logoOrig = QPixmap(":/images/swift-logo-white.png");
  m_logoLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
  m_logoLabel->setStyleSheet("background:transparent;");
  m_logoLabel->show();
  auto *swiftOpacity = new QGraphicsOpacityEffect(this);
  swiftOpacity->setOpacity(0.75);
  m_logoLabel->setGraphicsEffect(swiftOpacity);

  // --- top-left Flamingo logo ---
  m_flamingoOrig = QPixmap(":/images/flamingo-logo.png");
  m_flamingoLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
  m_flamingoLabel->setStyleSheet("background:transparent;");
  m_flamingoLabel->show();
  auto *flamingoOpacity = new QGraphicsOpacityEffect(this);
  flamingoOpacity->setOpacity(0.75);
  m_flamingoLabel->setGraphicsEffect(flamingoOpacity);

  // --- bottom-left Sussex logo ---
  m_sussexOrig = QPixmap(":/images/sussex-logo.png");
  m_sussexLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
  m_sussexLabel->setStyleSheet("background:transparent;");
  m_sussexLabel->show();
  auto *sussexOpacity = new QGraphicsOpacityEffect(this);
  sussexOpacity->setOpacity(0.75);
  m_sussexLabel->setGraphicsEffect(sussexOpacity);

  // --- top-right ESA logo ---
  m_esaOrig = QPixmap(":/images/ESA.png");
  m_esaLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
  m_esaLabel->setStyleSheet("background:transparent;");
  m_esaLabel->show();
  auto *esaOpacity = new QGraphicsOpacityEffect(this);
  esaOpacity->setOpacity(0.75);
  m_esaLabel->setGraphicsEffect(esaOpacity);

  // directory watcher
  connect(&m_dirWatcher, &QFileSystemWatcher::directoryChanged, this,
          &VizTabWidget::onImageDirectoryChanged);

  // loader/thread setup
  m_loader->moveToThread(m_loaderThread);
  connect(this, &VizTabWidget::startLoader, m_loader,
          &RotationFrameLoader::startLoading, Qt::QueuedConnection);
  connect(m_loader, &RotationFrameLoader::frameReady, this,
          &VizTabWidget::handleFrameReady, Qt::QueuedConnection);
  m_loaderThread->start();
}

VizTabWidget::~VizTabWidget() {
  // stop loader thread
  m_loaderThread->quit();
  m_loaderThread->wait();
}

void VizTabWidget::watchImageDirectory(const QString &dir) {
  m_imageDirectory = dir;
  if (!m_imageDirectory.endsWith('/'))
    m_imageDirectory += '/';
  m_dirWatcher.removePaths(m_dirWatcher.directories());
  if (QDir(m_imageDirectory).exists()) {
    m_dirWatcher.addPath(m_imageDirectory);
    scanImageDirectory();
  }
}

void VizTabWidget::scanImageDirectory() {
  if (m_imageDirectory.isEmpty())
    return;
  QDir d(m_imageDirectory);
  QStringList files = d.entryList({"*.hdf5"}, QDir::Files, QDir::Name);
  static const QRegularExpression re(R"(_(\d+)\.hdf5$)");
  int maxIdx = m_latestFileNumber;
  for (auto &fn : files) {
    auto m = re.match(fn);
    if (m.hasMatch()) {
      int idx = m.captured(1).toInt();
      maxIdx = std::max(maxIdx, idx);
    }
  }
  if (maxIdx > m_latestFileNumber) {
    m_latestFileNumber = maxIdx;
  }
  setCurrentFileNumber(m_latestFileNumber);
}

void VizTabWidget::setCurrentFileNumber(int idx) {
  idx = std::clamp(idx, 0, m_latestFileNumber);
  if (idx == m_currentFileNumber)
    return;
  m_currentFileNumber = idx;
  // restart loader
  emit startLoader(m_imageDirectory, m_currentFileNumber, m_currentDatasetKey,
                   m_percentileLow, m_percentileHigh, int(m_colormap), m_fps);
}

void VizTabWidget::setDatasetKey(const QString &key) {
  if (key == m_currentDatasetKey)
    return;
  m_currentDatasetKey = key;
  if (key == "dark_matter") {
    m_colormap = Colormap::Plasma;
    setTitle(tr("Dark Matter"));
  } else if (key == "gas") {
    m_colormap = Colormap::Magma;
    setTitle(tr("Gas"));
  } else if (key == "stars") {
    m_colormap = Colormap::Greyscale;
    setTitle(tr("Stars"));
  } else if (key == "gas_temperature") {
    m_colormap = Colormap::Inferno;
    setTitle(tr("Temperature"));
  } else {
    m_colormap = Colormap::Viridis;
    setTitle(tr("Unknown Dataset"));
  }

  // restart loader with new dataset
  if (m_currentFileNumber >= 0) {
    emit startLoader(m_imageDirectory, m_currentFileNumber, m_currentDatasetKey,
                     m_percentileLow, m_percentileHigh, int(m_colormap), m_fps);
  }
}

void VizTabWidget::setPercentileRange(float low, float high) {
  m_percentileLow = std::clamp(low, 0.0f, 100.0f);
  m_percentileHigh = std::clamp(high, 0.0f, 100.0f);
  if (m_currentFileNumber >= 0) {
    emit startLoader(m_imageDirectory, m_currentFileNumber, m_currentDatasetKey,
                     m_percentileLow, m_percentileHigh, int(m_colormap), m_fps);
  }
}

void VizTabWidget::percentileRange(float &low, float &high) const {
  low = m_percentileLow;
  high = m_percentileHigh;
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
    break;
  }
}

void VizTabWidget::handleFrameReady(const QImage &img, int fileNumber,
                                    int frameIndex, int totalFrames) {
  // paint
  m_imageLabel->setPixmapKeepingAspect(QPixmap::fromImage(img));
}

// In VizTabWidget.cpp
void VizTabWidget::resizeEvent(QResizeEvent *ev) {
  QWidget::resizeEvent(ev);

  // --- bottom-right Swift logo (~15% height) ---
  {
    int maxH = height() * m_swiftSizePercent / 100;
    QSize tgt(maxH * m_logoOrig.width() / m_logoOrig.height(), maxH);
    QPixmap scaled =
        m_logoOrig.scaled(tgt, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_logoLabel->setPixmap(scaled);
    m_logoLabel->setFixedSize(scaled.size());
    int x = width() - scaled.width() - m_swiftXMargin;
    int y = height() - scaled.height() - m_swiftYMargin;
    m_logoLabel->move(x, y);
    m_logoLabel->raise();
  }

  // --- top-left Flamingo logo (~20% height) ---
  {
    int maxH = height() * m_flamingoSizePercent / 100;
    QSize tgt(maxH * m_flamingoOrig.width() / m_flamingoOrig.height(), maxH);
    QPixmap scaled = m_flamingoOrig.scaled(tgt, Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation);
    m_flamingoLabel->setPixmap(scaled);
    m_flamingoLabel->setFixedSize(scaled.size());
    int x = m_flamingoXMargin;
    int y = m_flamingoYMargin;
    m_flamingoLabel->move(x, y);
    m_flamingoLabel->raise();
  }

  // --- bottom-left Sussex logo (~10% height) ---
  {
    int maxH = height() * m_sussexSizePercent / 100;
    QSize tgt(maxH * m_sussexOrig.width() / m_sussexOrig.height(), maxH);
    QPixmap scaled =
        m_sussexOrig.scaled(tgt, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_sussexLabel->setPixmap(scaled);
    m_sussexLabel->setFixedSize(scaled.size());
    int x = m_sussexXMargin;
    int y = height() - scaled.height() - m_sussexYMargin;
    m_sussexLabel->move(x, y);
    m_sussexLabel->raise();
  }

  // --- top-right ESA logo (~15% height) ---
  {
    int maxH = height() * m_esaSizePercent / 100;
    QSize tgt(maxH * m_esaOrig.width() / m_esaOrig.height(), maxH);
    QPixmap scaled =
        m_esaOrig.scaled(tgt, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_esaLabel->setPixmap(scaled);
    m_esaLabel->setFixedSize(scaled.size());
    int x = width() - scaled.width() - m_esaXMargin;
    int y = m_esaYMargin;
    m_esaLabel->move(x, y);
    m_esaLabel->raise();
  }
}

void VizTabWidget::onImageDirectoryChanged(const QString &) {
  scanImageDirectory();
}

void VizTabWidget::setTitle(const QString &text) {
  m_titleLabel->setText(text);
}
