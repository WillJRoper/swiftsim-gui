#include <complex>
#include <iostream>

#include "CommandLineParser.h"
#include "DataWatcher.h"
#include "HomeTabWidget.h"
#include "ImageProgressWidget.h"
#include "LogTabWidget.h"
#include "MainView.h"
#include "PlotWidget.h"
#include "SerialHandler.h"
#include "SimulationController.h"
#include "StepCounter.h"
#include "StyledSplitter.h"
#include "VizTabWidget.h"

#include <QAction>
#include <QCoreApplication>
#include <QCursor>
#include <QDebug>
#include <QDir>
#include <QInputDialog>
#include <QMenu>
#include <QMenuBar>
#include <QSplitter>
#include <QSplitterHandle>
#include <QStatusBar>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>

MainWindow::MainWindow(SimulationController *simCtrl,
                       CommandLineParser *cmdParser, QWidget *parent)
    : QMainWindow(parent), m_simCtrl(simCtrl) {

  // Set the window title and size to fill the screen
  setWindowTitle(tr("Swift GUI"));
  setMinimumSize(540, 960); // reasonable minimum size
  setWindowState(Qt::WindowMaximized);
  setWindowIcon(QIcon(":/images/swift-logo-white.png")); // set the icon

  // Set up each of the UI elements
  createSplitterAndLayouts();
  createsBottom(cmdParser);
  createProgressBar();
  createPlots();
  createDataWatcher();
  createVisualisations();
  createSerialHandler("/dev/cu.usbmodem2101");
  createCounters();

  // Connection all the signals and slots
  createActions();
}

/**
 * @brief Creates all QAction shortcuts and wires up DataWatcher signals.
 *
 * Shortcuts:
 *   - H/L/V/D : switch tabs (Home/Log/Visualise)
 *   - 0       : Dashboard (counter + progress bar)
 *   - 7       : Wall-Clock Time plot (page 1)
 *   - 8       : Percent-Complete plot (page 2)
 *   - 9       : Particle-Counts plot (page 3)
 *
 * Signals → slots:
 *   - percentRunChanged    → updateProgressBar()
 *   - scaleFactorChanged   → updateCurrentTimeLabel()
 *   - stepChanged          → updateStepCounter()
 *
 */
void MainWindow::createActions() {
  // ─── Tab-switching shortcuts ───────────────────────────────────
  QAction *homeTabAct = new QAction(tr("Home"), this);
  homeTabAct->setShortcut(QKeySequence(Qt::Key_H));
  homeTabAct->setShortcutContext(Qt::ApplicationShortcut);
  connect(homeTabAct, &QAction::triggered, this, [this] { switchToTab(0); });
  addAction(homeTabAct);
  QAction *logTabAct = new QAction(tr("Log"), this);
  logTabAct->setShortcut(QKeySequence(Qt::Key_L));
  logTabAct->setShortcutContext(Qt::ApplicationShortcut);
  connect(logTabAct, &QAction::triggered, this, [this] { switchToTab(1); });
  addAction(logTabAct);
  QAction *vizTabAct = new QAction(tr("Visualise"), this);
  vizTabAct->setShortcut(QKeySequence(Qt::Key_V));
  vizTabAct->setShortcutContext(Qt::ApplicationShortcut);
  connect(vizTabAct, &QAction::triggered, this, [this] { switchToTab(2); });
  addAction(vizTabAct);

  // ─── Switch to dark matter visualisation (1) ────────────────
  QAction *showDarkMatterViz =
      new QAction(tr("Dark Matter Visualisation"), this);
  showDarkMatterViz->setShortcut(QKeySequence(Qt::Key_1));
  showDarkMatterViz->setShortcutContext(Qt::ApplicationShortcut);
  connect(showDarkMatterViz, &QAction::triggered, this, [this] {
    m_vizTab->setDatasetKey("dark_matter");
    m_bottomWidget->setCurrentIndex(2);
  });
  addAction(showDarkMatterViz);

  // ─── Switch to gas visualisation (2) ────────────────────────
  QAction *showGasViz = new QAction(tr("Gas Visualisation"), this);
  showGasViz->setShortcut(QKeySequence(Qt::Key_2));
  showGasViz->setShortcutContext(Qt::ApplicationShortcut);
  connect(showGasViz, &QAction::triggered, this, [this] {
    m_vizTab->setDatasetKey("gas");
    m_bottomWidget->setCurrentIndex(2);
  });
  addAction(showGasViz);

  // ─── Switch to stars visualisation (3) ──────────────────────
  QAction *showStarsViz = new QAction(tr("Stars Visualisation"), this);
  showStarsViz->setShortcut(QKeySequence(Qt::Key_3));
  showStarsViz->setShortcutContext(Qt::ApplicationShortcut);
  connect(showStarsViz, &QAction::triggered, this, [this] {
    m_vizTab->setDatasetKey("stars");
    m_bottomWidget->setCurrentIndex(2);
  });
  addAction(showStarsViz);

  // ─── Switch to gas temperature visualisation (4) ─────────────
  QAction *showGasTempViz =
      new QAction(tr("Gas Temperature Visualisation"), this);
  showGasTempViz->setShortcut(QKeySequence(Qt::Key_4));
  showGasTempViz->setShortcutContext(Qt::ApplicationShortcut);
  connect(showGasTempViz, &QAction::triggered, this, [this] {
    m_vizTab->setDatasetKey("gas_temperature");
    m_bottomWidget->setCurrentIndex(2);
  });
  addAction(showGasTempViz);

  // ─── Dashboard view shortcut (0) ─────────────────────────────
  QAction *showDashboard = new QAction(tr("Dashboard"), this);
  showDashboard->setShortcut(QKeySequence(Qt::Key_0));
  showDashboard->setShortcutContext(Qt::ApplicationShortcut);
  connect(showDashboard, &QAction::triggered, this,
          [this] { m_topStack->setCurrentIndex(0); });
  addAction(showDashboard);

  // ─── Step counter shortcut ──────────────────────────────
  QAction *showStepCounter = new QAction(tr("Step Counter"), this);
  showStepCounter->setShortcut(QKeySequence(Qt::Key_7));
  showStepCounter->setShortcutContext(Qt::ApplicationShortcut);
  connect(showStepCounter, &QAction::triggered, this,
          [this] { m_topStack->setCurrentIndex(1); });
  addAction(showStepCounter);

  // ─── Plot view shortcuts ──────────────────────────────────────
  QAction *showWallTime = new QAction(tr("Wall-Clock Plot"), this);
  showWallTime->setShortcut(QKeySequence(Qt::Key_8));
  showWallTime->setShortcutContext(Qt::ApplicationShortcut);
  connect(showWallTime, &QAction::triggered, this,
          [this] { m_topStack->setCurrentIndex(2); });
  addAction(showWallTime);

  QAction *showParticles = new QAction(tr("Particles Plot"), this);
  showParticles->setShortcut(QKeySequence(Qt::Key_9));
  showParticles->setShortcutContext(Qt::ApplicationShortcut);
  connect(showParticles, &QAction::triggered, this,
          [this] { m_topStack->setCurrentIndex(3); });
  addAction(showParticles);

  QAction *showCSFRD = new QAction(tr("CSFRD Plot"), this);
  showCSFRD->setShortcut(QKeySequence(Qt::Key_6));
  showCSFRD->setShortcutContext(Qt::ApplicationShortcut);
  connect(showCSFRD, &QAction::triggered, this,
          [this] { m_topStack->setCurrentIndex(5); });
  addAction(showCSFRD);

  // ─── DataWatcher → MainWindow ───────────────────────────
  connect(m_dataWatcher, &DataWatcher::stepChanged, this,
          &MainWindow::updateStepCounter, Qt::QueuedConnection);
  connect(m_dataWatcher, &DataWatcher::totalWallClockTimeChanged, this,
          &MainWindow::updateWallClockCounter, Qt::QueuedConnection);
  connect(m_dataWatcher, &DataWatcher::starMassChanged, this,
          &MainWindow::updateStarsFormedCounter, Qt::QueuedConnection);
  connect(m_dataWatcher, &DataWatcher::numberofBHChanged, this,
          &MainWindow::updateBlackHolesFormedCounter, Qt::QueuedConnection);
  connect(m_dataWatcher, &DataWatcher::totalPartUpdatesChanged, this,
          &MainWindow::updateParticleUpdateCounter, Qt::QueuedConnection);
  connect(m_dataWatcher, &DataWatcher::percentRunChanged, this,
          &MainWindow::updateProgressBar, Qt::QueuedConnection);

  // ─── DataWatcher → PlotWidgets ──────────────────────────
  connect(m_dataWatcher, &DataWatcher::wallClockTimeForStepChanged,
          m_wallTimePlot, &PlotWidget::refresh, Qt::QueuedConnection);

  connect(m_dataWatcher, &DataWatcher::numberOfGPartsChanged, m_particlePlot,
          &PlotWidget::refresh, Qt::QueuedConnection);

  // ─── Update the top box top widget on a Timer ─────────────────────
  m_topRotateTimer = new QTimer(this);
  m_topRotateTimer->setInterval(5000);
  connect(m_topRotateTimer, &QTimer::timeout, this, &MainWindow::rotateTopPage);
  m_topRotateTimer->start();

  // ─── Map control buttons GUI updates ────────────────────
  connect(m_serialHandler, &SerialHandler::buttonPressed, this,
          &MainWindow::buttonUpdateUI);

  // // ─── Map encoder rotation to time control ────────────────────
  // connect(m_serialHandler, &SerialHandler::rotatedCW, m_vizTab,
  //         &VizTabWidget::fastForwardTime);
  // connect(m_serialHandler, &SerialHandler::rotatedCCW, m_vizTab,
  //         &VizTabWidget::rewindTime);

  // Reset idle timer on any button press:
  connect(m_serialHandler, &SerialHandler::buttonPressed, m_vizTab,
          &VizTabWidget::resetIdleTimer);

  // Reset idle timer on any knob turn:
  connect(m_serialHandler, &SerialHandler::rotatedCW, m_vizTab,
          &VizTabWidget::resetIdleTimer);
  connect(m_serialHandler, &SerialHandler::rotatedCCW, m_vizTab,
          &VizTabWidget::resetIdleTimer);

  // ─── Edit low‐percentile (“[”) ────────────────────────────────
  QAction *editLowPct = new QAction(tr("Edit Low Percentile"), this);
  editLowPct->setShortcut(QKeySequence(Qt::Key_BracketLeft));
  editLowPct->setShortcutContext(Qt::ApplicationShortcut);
  connect(editLowPct, &QAction::triggered, this, [this] {
    // switch to Visualise tab if not already there:
    m_bottomWidget->setCurrentIndex(2);
    m_vizTab->promptLowPercentile();
  });
  addAction(editLowPct);

  // ─── Edit high‐percentile (“]”) ───────────────────────────────
  QAction *editHighPct = new QAction(tr("Edit High Percentile"), this);
  editHighPct->setShortcut(QKeySequence(Qt::Key_BracketRight));
  editHighPct->setShortcutContext(Qt::ApplicationShortcut);
  connect(editHighPct, &QAction::triggered, this, [this] {
    m_bottomWidget->setCurrentIndex(2);
    m_vizTab->promptHighPercentile();
  });
  addAction(editHighPct);
}

/**
 * @brief Creates the main splitter and top/bottom sections of the window.
 *
 * Sets up a vertical splitter with:
 *  - Page 0: dashboard (counter + progress bar)
 *  - Pages 1–3: reserved for plots (added later in createPlots())
 *  - A middle expandable spacer
 *  - A bottom QTabWidget for Home/Log/Visualise
 */
void MainWindow::createSplitterAndLayouts() {
  // Create a vertical splitter; children non‐collapsible
  m_splitter = new StyledSplitter(Qt::Vertical, this);
  m_splitter->setChildrenCollapsible(false);
  m_splitter->setStyleSheet(R"(
        QSplitter::handle { background-color: #444; height: 4px; }
    )");

  // ─── Top: a stacked widget (0: dashboard; 1–3: plots) ───────
  m_topStack = new QStackedWidget(this);
  m_topStack->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  m_splitter->addWidget(m_topStack);

  // Page 0: dashboard container
  auto *dashPage = new QWidget(this);
  auto *dashLayout = new QVBoxLayout(dashPage);
  dashLayout->setContentsMargins(0, 0, 0, 0);
  dashLayout->setSpacing(4);
  m_topStack->addWidget(dashPage);

  // ─── Middle: blank expandable spacer ─────────────────────────
  m_middleGap = new QWidget(this);
  m_middleGap->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  m_splitter->addWidget(m_middleGap);

  // ─── Bottom: stacked widget ─────────────────────────────────
  m_bottomWidget = new QStackedWidget(this);
  m_bottomWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  m_splitter->addWidget(m_bottomWidget);

  // Set initial splitter sizes (approx. 27% / 25% / 48%)
  int totalH = height();
  int topH = int(0.2735 * totalH + 0.5);
  int midH = int(0.2564 * totalH + 0.5);
  int botH = totalH - topH - midH;
  m_splitter->setSizes({topH, midH, botH});

  setCentralWidget(m_splitter);
}

void MainWindow::createsBottom(CommandLineParser *cmdParser) {
  m_bottomWidget->addWidget(new HomeTabWidget);
  m_logTab = new LogTabWidget(cmdParser->logFilePath(), this);
  m_bottomWidget->addWidget(m_logTab);
}

/**
 * @brief Populates the dashboard page (index 0) with the step counter and
 * timeline.
 *
 * Builds a timeline widget containing a QProgressBar and QLabel, then
 * adds it below the existing m_stepCounter in the dashboard.
 */
void MainWindow::createProgressBar() {
  // Build timeline container
  auto *timeline = new QWidget(this);
  auto *tlLayout = new QHBoxLayout(timeline);
  tlLayout->setContentsMargins(0, 2, 0, 2);
  tlLayout->setSpacing(8);

  // Image‐based progress “bar”
  m_progressWidget =
      new ImageProgressWidget(":/images/evolution.jpg", timeline);
  // start empty
  m_progressWidget->setProgress(0.0);
  m_progressWidget->setSizePolicy(QSizePolicy::Expanding,
                                  QSizePolicy::Preferred);
  tlLayout->addWidget(m_progressWidget);

  // Add to dashboard page
  auto *dashPage = m_topStack->widget(0);
  auto *vbox = static_cast<QVBoxLayout *>(dashPage->layout());
  vbox->addWidget(timeline);
}

/**
 * @brief Creates the plot pages (indexes 1–3) in the topStack.
 *
 * Page 1: Wall‐Clock Time vs Scale Factor
 * Page 2: Percent Complete vs Scale Factor
 * Page 3: Combined Particle Counts vs Scale Factor
 */
void MainWindow::createPlots() {
  // 1) Repo root is wherever the EXE lives
  QDir repoRoot(QCoreApplication::applicationDirPath());

  // 2) Compute absolute script & plot dirs
  const QString scriptsDir = repoRoot.filePath("scripts");
  const QString plotsDir = repoRoot.filePath("plots");
  QDir().mkpath(plotsDir);

  // Page 1: Wall‐Clock Time
  const QString wallScript = QDir(scriptsDir).filePath("plot_wall_time.py");
  const QString wallPng = QDir(plotsDir).filePath("wall_time_plot.png");
  m_wallTimePlot = new PlotWidget(
      wallScript, m_simCtrl->simulationDirectory() + "/gui_data.txt", wallPng,
      this);
  m_topStack->addWidget(m_wallTimePlot);

  // Page 2: Combined Particle Counts
  const QString partScript = QDir(scriptsDir).filePath("plot_particles.py");
  const QString partPng = QDir(plotsDir).filePath("particles_plot.png");
  m_particlePlot = new PlotWidget(
      partScript, m_simCtrl->simulationDirectory() + "/gui_data.txt", partPng,
      this);
  m_topStack->addWidget(m_particlePlot);

  // Page 3: CSFRD
  const QString csfrdScript = QDir(scriptsDir).filePath("plot_csfrd.py");
  const QString csfrdPng = QDir(plotsDir).filePath("csfrd_plot.png");
  m_csfrdPlot = new PlotWidget(
      csfrdScript, m_simCtrl->simulationDirectory() + "/gui_data.txt", csfrdPng,
      this);
  m_topStack->addWidget(m_csfrdPlot);

  // Page 4: Particle Update Counts
  const QString updatesScript = QDir(scriptsDir).filePath("plot_updates.py");
  const QString updatesPng = QDir(plotsDir).filePath("update_plot.png");
  m_updatesPlot = new PlotWidget(
      updatesScript, m_simCtrl->simulationDirectory() + "/gui_data.txt",
      updatesPng, this);
  m_topStack->addWidget(m_updatesPlot);
}

void MainWindow::updateProgressBar(double pcent) {
  // Ensure pcent is in [0, 100]
  pcent = qBound(0.0, pcent, 100.0);

  // Drive our image widget (0.0–1.0)
  m_progressWidget->setProgress(pcent / 100.0);
}

void MainWindow::updateCurrentTimeLabel(double t) {
  QString txt = QString("%1").arg(t, m_currentLabelWidth, 'e',
                                  m_currentLabelPrecision, QChar(' '));
  m_currentLabel->setText(txt);
}

void MainWindow::createVisualisations() {
  m_vizTab = new VizTabWidget(this);
  m_bottomWidget->addWidget(m_vizTab);
  QString imagesDir = m_simCtrl->simulationDirectory() + "/images";
  m_vizTab->watchImageDirectory(imagesDir);
}

void MainWindow::createDataWatcher() {
  // 1) Instantiate (no parent—lives in its own thread)
  m_dataWatcher =
      new DataWatcher(m_simCtrl->simulationDirectory() + "/gui_data.txt",
                      /*parent=*/nullptr);

  // 2) Move it to its own thread
  m_dwThread = new QThread(this);
  m_dataWatcher->moveToThread(m_dwThread);

  // 3) Kick off its initial load when the thread starts
  connect(m_dwThread, &QThread::started, m_dataWatcher,
          &DataWatcher::updateData);

  // 4) Clean up watcher when thread finishes
  connect(m_dwThread, &QThread::finished, m_dataWatcher, &QObject::deleteLater);

  m_dwThread->start();
}

/**
 * @brief Create counters and link them into the top stacked widget.
 */
void MainWindow::createCounters() {
  // Create the step counter widget
  m_stepCounter = new StepCounterWidget(tr("SIMULATION STEPS"), this, 4);
  m_topStack->addWidget(m_stepCounter);

  // Create the current time counter
  m_wallClockCounter = new StepCounterWidget(tr("RUNTIME (HRS)"), this, 4);
  m_topStack->addWidget(m_wallClockCounter);

  // Create the stellar mass formed counter
  m_starsFormedCounter =
      new StepCounterWidget(tr("SOLAR MASSES \nFORMED"), this, 4);
  m_topStack->addWidget(m_starsFormedCounter);

  // Create the black holes formed counter
  m_blackHolesFormedCounter =
      new StepCounterWidget(tr("BLACK HOLES \nFORMED"), this, 4);
  m_topStack->addWidget(m_blackHolesFormedCounter);

  // Create the updated particle counter
  m_ParticleUpdateCounter =
      new StepCounterWidget(tr("PARTICLES \nUPDATED"), this, 6);
  m_topStack->addWidget(m_ParticleUpdateCounter);
}

void MainWindow::createSerialHandler(const QString &portPath) {
  // Instantiate and let MainWindow own it
  m_serialHandler =
      new SerialHandler(portPath, /*baud=*/115200, /*parent=*/nullptr);

  // Attach the serial handler to the other classes that need it
  m_logTab->setSerialHandler(m_serialHandler);
  m_vizTab->setSerialHandler(m_serialHandler);

  // Button presses → debug
  connect(m_serialHandler, &SerialHandler::buttonPressed, this,
          [](int id) { qDebug() << "[Serial] Button" << id << "pressed"; });

  // Clockwise rotation → debug
  connect(m_serialHandler, &SerialHandler::rotatedCW, this, [](int steps) {
    qDebug() << "[Serial] Rotated CW by" << steps << "steps";
  });

  // Anti-clockwise rotation → debug
  connect(m_serialHandler, &SerialHandler::rotatedCCW, this, [](int steps) {
    qDebug() << "[Serial] Rotated CCW by" << steps << "steps";
  });

  // Absolute position updates → debug
  connect(m_serialHandler, &SerialHandler::positionChanged, this,
          [](int pos) { qDebug() << "[Serial] Position changed to" << pos; });

  // Error reporting → debug
  connect(m_serialHandler, &SerialHandler::errorOccurred, this,
          [](const QString &msg) { qDebug() << "[Serial] Error:" << msg; });
}

void MainWindow::rotateTopPage() {
  if (!m_topStack)
    return;
  int count = m_topStack->count();
  if (count < 2)
    return;

  int next = (m_topStack->currentIndex() + 1) % count;
  m_topStack->setCurrentIndex(next);
}

void MainWindow::updateStepCounter(long long step) {
  m_stepCounter->setStep(step);
}

void MainWindow::updateWallClockCounter(double t) {
  // Convert seconds to hours
  int hours = t / 1000.0 / 60 / 60;
  m_wallClockCounter->setStep(hours);
}

void MainWindow::updateStarsFormedCounter(double mass) {
  // Convert mass to integer count (assuming 1 solar mass per star)
  int count = static_cast<int>(mass);
  m_starsFormedCounter->setStep(count);
}

void MainWindow::updateBlackHolesFormedCounter(long long count) {
  m_blackHolesFormedCounter->setStep(count);
}

void MainWindow::updateParticleUpdateCounter(long long count) {
  m_ParticleUpdateCounter->setStep(count);
}

void MainWindow::changeLogFontSize() {
  bool ok;
  int current = m_logTab->font().pointSize();
  int size = QInputDialog::getInt(this, tr("Log Font Size"), tr("Point size:"),
                                  current, 6, 48, 1, &ok);
  if (ok) {
    m_logTab->setFontSize(size);
  }
}

void MainWindow::switchToTab(int index) {
  if (index >= 0 && index < m_bottomWidget->count()) {
    m_bottomWidget->setCurrentIndex(index);
  }
}

void MainWindow::buttonUpdateUI(int id) {

  // Perform the right operation based on the button ID
  switch (id) {
  case 1:
    // Dark Matter Visualisation
    m_vizTab->setDatasetKey("dark_matter");
    m_bottomWidget->setCurrentIndex(2);
    break;
  case 5:
    // Gas Visualisation
    m_vizTab->setDatasetKey("gas");
    m_bottomWidget->setCurrentIndex(2);
    break;
  case 6:
    // Stars Visualisation
    m_vizTab->setDatasetKey("stars");
    m_bottomWidget->setCurrentIndex(2);
    break;
  case 2:
    // Gas Temperature Visualisation
    m_vizTab->setDatasetKey("gas_temperature");
    m_bottomWidget->setCurrentIndex(2);
    break;
  case 3:
    m_bottomWidget->setCurrentIndex(1); // Log widget
    break;
  case 4:
    // Next on top widget
    rotateTopPage();
    break;
  default:
    qDebug() << "Unknown button ID:" << id;
    break;
  }
}
