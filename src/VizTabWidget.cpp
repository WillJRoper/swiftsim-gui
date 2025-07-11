#include "VizTabWidget.h"
#include "RotationFrameLoader.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfoList>
#include <QGraphicsOpacityEffect>
#include <QHideEvent>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMetaObject>
#include <QRegularExpression>
#include <QShortcut>
#include <QShowEvent>
#include <QVBoxLayout>
#include <algorithm>

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
  m_titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
  m_titleLabel->setStyleSheet(
      "background: rgba(0,0,0,0);"); // or semi‐opaque if you like

  // Layout
  auto *lay = new QVBoxLayout(this);
  lay->setContentsMargins(0, 0, 0, 0);

  // Then the image expands to fill
  lay->addWidget(m_imageLabel, /*stretch=*/1);

  // These are turned off for now, in favour of logos on walls around the
  // exhibit

  // // --- bottom-right SWIFT logo ---
  // m_logoOrig = QPixmap(":/images/swift-logo-white.png");
  // m_logoLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
  // m_logoLabel->setStyleSheet("background:transparent;");
  // m_logoLabel->show();
  // auto *swiftOpacity = new QGraphicsOpacityEffect(this);
  // swiftOpacity->setOpacity(0.75);
  // m_logoLabel->setGraphicsEffect(swiftOpacity);
  //
  // // --- top-left Flamingo logo ---
  // m_flamingoOrig = QPixmap(":/images/flamingo-logo.png");
  // m_flamingoLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
  // m_flamingoLabel->setStyleSheet("background:transparent;");
  // m_flamingoLabel->show();
  // auto *flamingoOpacity = new QGraphicsOpacityEffect(this);
  // flamingoOpacity->setOpacity(0.75);
  // m_flamingoLabel->setGraphicsEffect(flamingoOpacity);
  //
  // // --- bottom-left Sussex logo ---
  // m_sussexOrig = QPixmap(":/images/sussex-logo.png");
  // m_sussexLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
  // m_sussexLabel->setStyleSheet("background:transparent;");
  // m_sussexLabel->show();
  // auto *sussexOpacity = new QGraphicsOpacityEffect(this);
  // sussexOpacity->setOpacity(0.75);
  // m_sussexLabel->setGraphicsEffect(sussexOpacity);
  //
  // // --- top-right ESA logo ---
  // m_esaOrig = QPixmap(":/images/ESA.png");
  // m_esaLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
  // m_esaLabel->setStyleSheet("background:transparent;");
  // m_esaLabel->show();
  // auto *esaOpacity = new QGraphicsOpacityEffect(this);
  // esaOpacity->setOpacity(0.75);
  // m_esaLabel->setGraphicsEffect(esaOpacity);

  // Step counters
  m_counterBR = new StepCounterWidget(tr("PERCENTAGE RUN"), this, 2, 16, true);
  m_counterBR->setAttribute(Qt::WA_TransparentForMouseEvents);
  m_counterBR->setStyleSheet("background:transparent;");
  m_counterBR->show();
  connect(m_loader, &RotationFrameLoader::percentChanged, m_counterBR,
          &StepCounterWidget::setStep, Qt::QueuedConnection);

  m_counterBL = new StepCounterWidget(tr("AGE (YRS)"), this, 4, 16, true);
  m_counterBL->setAttribute(Qt::WA_TransparentForMouseEvents);
  m_counterBL->setStyleSheet("background:transparent;");
  m_counterBL->show();
  connect(m_loader, &RotationFrameLoader::ageChanged, m_counterBL,
          &StepCounterWidget::setStep, Qt::QueuedConnection);

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

  // Debounce interval for knob manipulating the file number
  constexpr int DEBOUNCE_MS = 10;
  m_debounceTimer.setSingleShot(true);
  m_debounceTimer.setInterval(DEBOUNCE_MS);
  connect(&m_debounceTimer, &QTimer::timeout, this,
          &VizTabWidget::applyPendingDelta);

  // Idle timer setup (reset to latest after inactivity)
  constexpr int IDLE_MS = 60 * 1000;
  m_idleTimer.setSingleShot(true);
  m_idleTimer.setInterval(IDLE_MS);
  connect(&m_idleTimer, &QTimer::timeout, this, &VizTabWidget::resetToLatest);

  // Start idle countdown immediately
  m_idleTimer.start();

  // Make sure this widget gets focus and key events
  setFocusPolicy(Qt::StrongFocus);

  // Scan the image directory initially
  scanImageDirectory();

  // If we have a file number, start loading it
  if (m_latestFileNumber >= 0) {
    emit startLoader(m_imageDirectory, m_currentFileNumber, m_currentDatasetKey,
                     m_percentileLow, m_percentileHigh, int(m_colormap), m_fps,
                     false);
  }
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
}

void VizTabWidget::setCurrentFileNumber(int idx) {
  idx = std::clamp(idx, 0, m_latestFileNumber);
  if (idx == m_currentFileNumber)
    return;
  m_currentFileNumber = idx;
  // restart loader
  emit startLoader(m_imageDirectory, m_currentFileNumber, m_currentDatasetKey,
                   m_percentileLow, m_percentileHigh, int(m_colormap), m_fps,
                   false);
}

void VizTabWidget::setCurrentFileNumberKnob(int idx) {
  idx = std::clamp(idx, 0, m_latestFileNumber);
  if (idx == m_currentFileNumber)
    return;

  m_currentFileNumber = idx;

  // Simply swap files under the continuing rotation clock:
  QMetaObject::invokeMethod(m_loader, "jumpToFile", Qt::QueuedConnection,
                            Q_ARG(int, m_currentFileNumber),
                            Q_ARG(bool, true) // keepPercentiles
  );
}

void VizTabWidget::setDatasetKey(const QString &key) {
  if (key == m_currentDatasetKey)
    return;
  m_currentDatasetKey = key;
  if (key == "dark_matter") {
    m_colormap = Colormap::Copper;
    setTitle(tr("Dark Matter"));
  } else if (key == "gas") {
    m_colormap = Colormap::Cosmic;
    setTitle(tr("Gas"));
  } else if (key == "stars") {
    m_colormap = Colormap::SpeakNow;
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
                     m_percentileLow, m_percentileHigh, int(m_colormap), m_fps,
                     false);
  }
}

void VizTabWidget::setPercentileRange(float low, float high) {
  m_percentileLow = std::clamp(low, 0.0f, 100.0f);
  m_percentileHigh = std::clamp(high, 0.0f, 100.0f);
  if (m_currentFileNumber >= 0) {
    emit startLoader(m_imageDirectory, m_currentFileNumber, m_currentDatasetKey,
                     m_percentileLow, m_percentileHigh, int(m_colormap), m_fps,
                     false);
  }
}

void VizTabWidget::percentileRange(float &low, float &high) const {
  low = m_percentileLow;
  high = m_percentileHigh;
}

void VizTabWidget::keyPressEvent(QKeyEvent *evt) {
  switch (evt->key()) {
  case Qt::Key_Up:
    fastForwardTime(1);
    return; // eat the event
  case Qt::Key_Down:
    rewindTime(1);
    return;
  default:
    QWidget::keyPressEvent(evt);
    return;
  }
  // if we hit one of the number keys, call your existing handler:
  QWidget::keyPressEvent(evt);
}

void VizTabWidget::handleFrameReady(const QImage &img, int fileNumber,
                                    int frameIndex, int totalFrames) {
  // paint
  m_imageLabel->setPixmapKeepingAspect(QPixmap::fromImage(img));
}

// In VizTabWidget.cpp
void VizTabWidget::resizeEvent(QResizeEvent *ev) {
  QWidget::resizeEvent(ev);

  // position the title:
  const int topMargin = 20;
  int w = width();
  int h = m_titleLabel->sizeHint().height();     // height based on font
  m_titleLabel->setGeometry(0, topMargin, w, h); // full‐width
  // (you can inset left/right margins if you like:
  //  m_titleLabel->setGeometry(leftMargin, topMargin,
  //                            w - leftMargin - rightMargin, h);
  m_titleLabel->raise();

  // These are turned off for now, in favour of logos on walls around the
  // exhibit

  // // --- bottom-right Swift logo (~15% height) ---
  // {
  //   int maxH = height() * m_swiftSizePercent / 100;
  //   QSize tgt(maxH * m_logoOrig.width() / m_logoOrig.height(), maxH);
  //   QPixmap scaled =
  //       m_logoOrig.scaled(tgt, Qt::KeepAspectRatio,
  //       Qt::SmoothTransformation);
  //   m_logoLabel->setPixmap(scaled);
  //   m_logoLabel->setFixedSize(scaled.size());
  //   int x = width() - scaled.width() - m_swiftXMargin;
  //   int y = height() - scaled.height() - m_swiftYMargin;
  //   m_logoLabel->move(x, y);
  //   m_logoLabel->raise();
  // }
  //
  // // --- top-left Flamingo logo (~20% height) ---
  // {
  //   int maxH = height() * m_flamingoSizePercent / 100;
  //   QSize tgt(maxH * m_flamingoOrig.width() / m_flamingoOrig.height(), maxH);
  //   QPixmap scaled = m_flamingoOrig.scaled(tgt, Qt::KeepAspectRatio,
  //                                          Qt::SmoothTransformation);
  //   m_flamingoLabel->setPixmap(scaled);
  //   m_flamingoLabel->setFixedSize(scaled.size());
  //   int x = m_flamingoXMargin;
  //   int y = m_flamingoYMargin;
  //   m_flamingoLabel->move(x, y);
  //   m_flamingoLabel->raise();
  // }
  //
  // // --- bottom-left Sussex logo (~10% height) ---
  // {
  //   int maxH = height() * m_sussexSizePercent / 100;
  //   QSize tgt(maxH * m_sussexOrig.width() / m_sussexOrig.height(), maxH);
  //   QPixmap scaled =
  //       m_sussexOrig.scaled(tgt, Qt::KeepAspectRatio,
  //       Qt::SmoothTransformation);
  //   m_sussexLabel->setPixmap(scaled);
  //   m_sussexLabel->setFixedSize(scaled.size());
  //   int x = m_sussexXMargin;
  //   int y = height() - scaled.height() - m_sussexYMargin;
  //   m_sussexLabel->move(x, y);
  //   m_sussexLabel->raise();
  // }
  //
  // // --- top-right ESA logo (~15% height) ---
  // {
  //   int maxH = height() * m_esaSizePercent / 100;
  //   QSize tgt(maxH * m_esaOrig.width() / m_esaOrig.height(), maxH);
  //   QPixmap scaled =
  //       m_esaOrig.scaled(tgt, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  //   m_esaLabel->setPixmap(scaled);
  //   m_esaLabel->setFixedSize(scaled.size());
  //   int x = width() - scaled.width() - m_esaXMargin;
  //   int y = m_esaYMargin;
  //   m_esaLabel->move(x, y);
  //   m_esaLabel->raise();
  // }

  // Position bottom-left counter
  h = int(height() * m_counterSizePercent / 100);
  w = int(width() * 20 / 100); // 20% of width
  m_counterBL->setFixedHeight(h);
  m_counterBL->setFixedWidth(w);
  m_counterBL->adjustSize();
  int xBL = m_counterMargin;
  int yBL = height() - m_counterBL->height() - m_counterMargin;
  m_counterBL->move(xBL, yBL);

  // Position bottom-right counter
  m_counterBR->setFixedHeight(h);
  m_counterBR->setFixedWidth(w);
  m_counterBR->adjustSize();
  int xBR = width() - m_counterBR->width() - m_counterMargin;
  int yBR = yBL;
  m_counterBR->move(xBR, yBR);
}

void VizTabWidget::onImageDirectoryChanged(const QString &) {
  scanImageDirectory();
}

void VizTabWidget::setTitle(const QString &text) {
  m_titleLabel->setText(text);
}

// Instead of immediately changing file number, add to pending and restart timer
void VizTabWidget::rewindTime(int delta) {
  if (!isVisible()) // <-- bail out if the tab isn’t showing
    return;

  m_pendingDelta -= delta / m_deltaScaler;
  m_debounceTimer.start();
}

void VizTabWidget::fastForwardTime(int delta) {
  if (!isVisible()) // <-- bail out if the tab isn’t showing
    return;

  m_pendingDelta += delta / m_deltaScaler;
  m_debounceTimer.start();
}

// When pulses calm down, apply the net change
void VizTabWidget::applyPendingDelta() {
  if (m_pendingDelta == 0)
    return;

  // Combine any previous remaining pending delta
  m_pendingDelta += m_tickRemainder;
  m_tickRemainder = 0;

  // Convert pending delta to integer and remember the remainder
  int deltaInt = static_cast<int>(m_pendingDelta);
  m_tickRemainder = m_pendingDelta - deltaInt;

  // compute new clamped file number
  int newFileNumber = m_currentFileNumber + deltaInt;

  // apply and reset
  setCurrentFileNumberKnob(newFileNumber);
  m_pendingDelta = 0;
}

void VizTabWidget::resetIdleTimer() { m_idleTimer.start(); }

void VizTabWidget::resetToLatest() {
  // Jump to the latest frame
  setCurrentFileNumber(m_latestFileNumber);

  // Restart idle timer for next reset
  resetIdleTimer();
}

void VizTabWidget::promptLowPercentile() {
  bool ok = false;
  qInfo() << "Current low percentile:" << m_percentileLow;
  double newLow =
      QInputDialog::getDouble(this, tr("Set Low Percentile"), tr("Low (%):"),
                              m_percentileLow, // current value
                              0.0,             // min
                              100.0,           // max
                              5,               // decimals
                              &ok);
  if (ok) {
    setPercentileRange(static_cast<float>(newLow), m_percentileHigh);
  }
  qInfo() << "New low percentile set to:" << newLow;
}

void VizTabWidget::promptHighPercentile() {
  bool ok = false;
  qInfo() << "Current high percentile:" << m_percentileHigh;
  double newHigh =
      QInputDialog::getDouble(this, tr("Set High Percentile"), tr("High (%):"),
                              m_percentileHigh, 0.0, 100.0, 5, &ok);
  if (ok) {
    setPercentileRange(m_percentileLow, static_cast<float>(newHigh));
  }
  qInfo() << "New high percentile set to:" << newHigh;
}

void VizTabWidget::showEvent(QShowEvent *ev) {
  QWidget::showEvent(ev);
  if (m_serialHandler) {
    connect(m_serialHandler, &SerialHandler::rotatedCW, this,
            &VizTabWidget::fastForwardTime);
    connect(m_serialHandler, &SerialHandler::rotatedCCW, this,
            &VizTabWidget::rewindTime);
  }
}

void VizTabWidget::hideEvent(QHideEvent *ev) {
  QWidget::hideEvent(ev);
  if (m_serialHandler) {
    disconnect(m_serialHandler, &SerialHandler::rotatedCW, this,
               &VizTabWidget::fastForwardTime);
    disconnect(m_serialHandler, &SerialHandler::rotatedCCW, this,
               &VizTabWidget::rewindTime);
  }
}
