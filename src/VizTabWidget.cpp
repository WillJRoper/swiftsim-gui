// VizTabWidget.cpp
#include "VizTabWidget.h"

#include <QApplication>
#include <QDir>
#include <QKeyEvent>
#include <QLabel>
#include <QMetaObject>
#include <QRegularExpression>
#include <QVBoxLayout>

#include <algorithm>
#include <limits>
#include <vector>

/// ---------------------------------------------------------
///  RotationFrameLoader implementation
/// ---------------------------------------------------------
VizTabWidget::RotationFrameLoader::RotationFrameLoader() = default;
VizTabWidget::RotationFrameLoader::~RotationFrameLoader() {
  if (m_fileId >= 0)
    H5Fclose(m_fileId);
}

bool VizTabWidget::RotationFrameLoader::openFile(const QString &path,
                                                 const QString &datasetKey,
                                                 float lowPct, float highPct) {
  // Close previous
  if (m_fileId >= 0) {
    H5Fclose(m_fileId);
    m_fileId = -1;
  }

  // Open
  m_fileId = H5Fopen(path.toUtf8().constData(), H5F_ACC_RDONLY, H5P_DEFAULT);
  if (m_fileId < 0)
    return false;

  // Open dataset & query dims
  hid_t dset = H5Dopen2(m_fileId, datasetKey.toUtf8().constData(), H5P_DEFAULT);
  hid_t space = H5Dget_space(dset);
  hsize_t dims[3];
  H5Sget_simple_extent_dims(space, dims, nullptr);
  m_nFrames = int(dims[0]);
  m_xres = int(dims[1]);
  m_yres = int(dims[2]);
  H5Sclose(space);
  H5Dclose(dset);

  // Resize buffer
  m_buf.assign(size_t(m_xres) * m_yres, 0.0f);
  m_datasetKey = datasetKey;

  // Compute percentile cut-points
  computePercentiles(lowPct, highPct);
  return true;
}

void VizTabWidget::RotationFrameLoader::computePercentiles(float lowPct,
                                                           float highPct) {
  // Read only frame 0 for speed
  hid_t dset =
      H5Dopen2(m_fileId, m_datasetKey.toUtf8().constData(), H5P_DEFAULT);
  hid_t space = H5Dget_space(dset);
  hsize_t off[3] = {0, 0, 0}, cnt[3] = {1, (hsize_t)m_xres, (hsize_t)m_yres};
  H5Sselect_hyperslab(space, H5S_SELECT_SET, off, nullptr, cnt, nullptr);
  hid_t mem = H5Screate_simple(3, cnt, nullptr);
  H5Dread(dset, H5T_NATIVE_FLOAT, mem, space, H5P_DEFAULT, m_buf.data());
  H5Sclose(mem);
  H5Sclose(space);
  H5Dclose(dset);

  // Build copy, nth_element
  std::vector<float> tmp = m_buf;
  auto N = tmp.size();
  if (N == 0) {
    m_lowerValue = 0;
    m_upperValue = 1;
    return;
  }
  size_t lo = size_t((lowPct / 100.0f) * (N - 1) + 0.5f),
         hi = size_t((highPct / 100.0f) * (N - 1) + 0.5f);
  lo = std::clamp(lo, size_t(0), N - 1);
  hi = std::clamp(hi, size_t(0), N - 1);
  std::nth_element(tmp.begin(), tmp.begin() + lo, tmp.end());
  m_lowerValue = tmp[lo];
  std::nth_element(tmp.begin(), tmp.begin() + hi, tmp.end());
  m_upperValue = tmp[hi];
  if (m_lowerValue == m_upperValue) {
    m_lowerValue = *std::min_element(tmp.begin(), tmp.end());
    m_upperValue = *std::max_element(tmp.begin(), tmp.end());
  }
}

bool VizTabWidget::RotationFrameLoader::fetchFrame(int idx) {
  if (m_fileId < 0 || idx < 0 || idx >= m_nFrames)
    return false;
  // Read hyperslab idx
  hid_t dset =
      H5Dopen2(m_fileId, m_datasetKey.toUtf8().constData(), H5P_DEFAULT);
  hid_t space = H5Dget_space(dset);
  hsize_t off[3] = {(hsize_t)idx, 0, 0},
          cnt[3] = {1, (hsize_t)m_xres, (hsize_t)m_yres};
  H5Sselect_hyperslab(space, H5S_SELECT_SET, off, nullptr, cnt, nullptr);
  hid_t mem = H5Screate_simple(3, cnt, nullptr);
  H5Dread(dset, H5T_NATIVE_FLOAT, mem, space, H5P_DEFAULT, m_buf.data());
  H5Sclose(mem);
  H5Sclose(space);
  H5Dclose(dset);
  return true;
}

/// ---------------------------------------------------------
///  VizTabWidget implementation
/// ---------------------------------------------------------
VizTabWidget::VizTabWidget(QWidget *parent)
    : QOpenGLWidget(parent), m_overlayLabel(new QLabel(this)),
      m_logoLabel(new QLabel(this)), m_timer(new QTimer(this)) {
  // Overlay styling
  m_overlayLabel->setStyleSheet(
      "QLabel{ background:rgba(0,0,0,128); color:white; padding:4px; }");
  m_overlayLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
  m_overlayLabel->show();

  // Watermark logo
  QPixmap logo(":/images/swift-logo-white.png");
  m_logoLabel->setPixmap(logo);
  m_logoLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
  m_logoLabel->setStyleSheet("background:transparent");
  m_logoLabel->show();

  // Timer connects to advance
  connect(m_timer, &QTimer::timeout, this, &VizTabWidget::advanceRotationFrame);
  m_timer->start(1000 / m_fps);

  // Directory watcher
  connect(&m_dirWatcher, &QFileSystemWatcher::directoryChanged, this,
          &VizTabWidget::onImageDirectoryChanged);

  // Default colormap
  setColormap(Colormap::Plasma);
}

VizTabWidget::~VizTabWidget() {
  makeCurrent();
  glDeleteTextures(1, &m_texture);
  glDeleteProgram(m_program);
  doneCurrent();
}

void VizTabWidget::initializeGL() {
  initializeOpenGLFunctions();
  initShader();

  // Create texture
  glGenTextures(1, &m_texture);
  glBindTexture(GL_TEXTURE_2D, m_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void VizTabWidget::resizeGL(int w, int h) { glViewport(0, 0, w, h); }

void VizTabWidget::paintGL() {
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(m_program);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_texture);
  glUniform1i(m_texUnit, 0);
  // draw full-screen quad (assume VAO/setup done in shader init)
  glDrawArrays(GL_TRIANGLES, 0, 6);
}

void VizTabWidget::initShader() {
  // Compile a minimal vertex+fragment that samples float texture,
  // applies colormap lookup via a uniform array (passed as a texture or UBO),
  // and writes out RGB.  (Omitted here for brevity—assume you have your
  // standard colormap shader loaded into m_program.)
}

void VizTabWidget::resizeEvent(QResizeEvent *ev) {
  QOpenGLWidget::resizeEvent(ev);
  m_overlayLabel->move(10, 10);
  // watermark bottom-right
  int margin = 8, lw = m_logoLabel->pixmap()->width(),
      lh = m_logoLabel->pixmap()->height();
  m_logoLabel->move(width() - lw - margin, height() - lh - margin);
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
    QOpenGLWidget::keyPressEvent(evt);
    break;
  }
}

void VizTabWidget::advanceRotationFrame() {
  m_currentRotationFrame = (m_currentRotationFrame + 1) % m_loader.m_nFrames;
  loadCurrentFrame();
}

void VizTabWidget::watchImageDirectory(const QString &dir) {
  m_imageDirectory = dir;
  if (!m_imageDirectory.endsWith('/'))
    m_imageDirectory += '/';
  m_dirWatcher.removePaths(m_dirWatcher.directories());
  if (QDir(dir).exists()) {
    m_dirWatcher.addPath(dir);
    scanImageDirectory();
  }
}

void VizTabWidget::scanImageDirectory() {
  if (m_imageDirectory.isEmpty())
    return;
  QDir d(m_imageDirectory);
  auto files = d.entryList({"*.hdf5"}, QDir::Files, QDir::Name);
  static QRegularExpression re(R"(_(\d+)\.hdf5$)");
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
    m_currentFileNumber = maxIdx;
    QString path =
        m_imageDirectory + "image_" + QString::number(maxIdx) + ".hdf5";
    if (m_loader.openFile(path, m_datasetKey, m_percentileLow,
                          m_percentileHigh)) {
      loadCurrentFrame();
    }
  }
}

void VizTabWidget::setDatasetKey(const QString &key) {
  if (key == m_datasetKey)
    return;
  m_datasetKey = key;
  // pick colormap
  if (key == "dark_matter")
    setColormap(Colormap::Plasma);
  else if (key == "gas")
    setColormap(Colormap::Magma);
  else if (key == "stars")
    setColormap(Colormap::Greyscale);
  else if (key == "gas_temperature")
    setColormap(Colormap::Inferno);
  // reopen file to re-compute percentiles
  if (m_currentFileNumber >= 0) {
    QString path = m_imageDirectory + "image_" +
                   QString::number(m_currentFileNumber) + ".hdf5";
    m_loader.openFile(path, m_datasetKey, m_percentileLow, m_percentileHigh);
    loadCurrentFrame();
  }
}

void VizTabWidget::setPercentileRange(float low, float high) {
  m_percentileLow = qBound(0.0f, low, 100.0f);
  m_percentileHigh = qBound(0.0f, high, 100.0f);
  // recompute / reload
  if (m_currentFileNumber >= 0) {
    QString path = m_imageDirectory + "image_" +
                   QString::number(m_currentFileNumber) + ".hdf5";
    m_loader.openFile(path, m_datasetKey, m_percentileLow, m_percentileHigh);
    loadCurrentFrame();
  }
}

void VizTabWidget::percentileRange(float &low, float &high) const {
  low = m_percentileLow;
  high = m_percentileHigh;
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

void VizTabWidget::loadCurrentFrame() {
  // 1) Fetch floats
  if (!m_loader.fetchFrame(m_currentRotationFrame))
    return;

  // 2) Upload as single‐channel floating‐point texture
  glBindTexture(GL_TEXTURE_2D, m_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, m_loader.width(), m_loader.height(),
               0, GL_RED, GL_FLOAT, m_loader.data());

  // 3) Set uniforms: [lower,upper] and colormap LUT
  glUseProgram(m_program);
  glUniform2f(m_locRange, m_loader.m_lowerValue, m_loader.m_upperValue);

  // TODO upload colormap LUT (e.g. as a 1D texture or UBO)

  // 4) Trigger redraw
  update();

  // 5) Overlay text
  m_overlayLabel->setText(QString("File %1/%2  Rot %3/%4  [%5]")
                              .arg(m_currentFileNumber)
                              .arg(m_latestFileNumber)
                              .arg(m_currentRotationFrame)
                              .arg(m_loader.m_nFrames)
                              .arg(m_datasetKey));
  m_overlayLabel->adjustSize();
}
