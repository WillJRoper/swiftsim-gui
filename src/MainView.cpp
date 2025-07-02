#include <complex>
#include <iostream>

#include "CommandLineParser.h"
#include "DataWatcher.h"
#include "DiagTabWidget.h"
#include "HomeTabWidget.h"
#include "ImageProgressWidget.h"
#include "LogTabWidget.h"
#include "MainView.h"
#include "PlotWidget.h"
#include "SimulationController.h"
#include "StepCounter.h"
#include "StyledSplitter.h"
#include "VizTabWidget.h"

#include <QAction>
#include <QCoreApplication>
#include <QCursor>
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

  // Create the data watcher that will gather the data from gui_data.txt
  m_dataWatcher =
      new DataWatcher(cmdParser->simulationDirectory() + "/gui_data.txt");

  // Create the step counter widget
  m_stepCounter = new StepCounterWidget(this);

  // Set up each of the UI elements
  createSplitterAndLayouts();
  createTabs(cmdParser);
  createProgressBar();
  createPlots();
  createVisualisations();
  createActions();
}

/**
 * @brief Creates all QAction shortcuts and wires up DataWatcher signals.
 *
 * Shortcuts:
 *   - H/L/V/D : switch tabs (Home/Log/Visualise/Diagnostics)
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
  connect(vizTabAct, &QAction::triggered, this, [this] { switchToTab(3); });
  addAction(vizTabAct);
  QAction *diagTabAct = new QAction(tr("Diagnostics"), this);
  diagTabAct->setShortcut(QKeySequence(Qt::Key_D));
  diagTabAct->setShortcutContext(Qt::ApplicationShortcut);
  connect(diagTabAct, &QAction::triggered, this, [this] { switchToTab(2); });
  addAction(diagTabAct);

  // ─── Switch to dark matter visualisation (1) ────────────────
  QAction *showDarkMatterViz =
      new QAction(tr("Dark Matter Visualisation"), this);
  showDarkMatterViz->setShortcut(QKeySequence(Qt::Key_1));
  showDarkMatterViz->setShortcutContext(Qt::ApplicationShortcut);
  connect(showDarkMatterViz, &QAction::triggered, this, [this] {
    m_tabs->setCurrentIndex(3);
    m_vizTab->setDatasetKey("dark_matter");
  });
  addAction(showDarkMatterViz);

  // ─── Switch to gas visualisation (2) ────────────────────────
  QAction *showGasViz = new QAction(tr("Gas Visualisation"), this);
  showGasViz->setShortcut(QKeySequence(Qt::Key_2));
  showGasViz->setShortcutContext(Qt::ApplicationShortcut);
  connect(showGasViz, &QAction::triggered, this, [this] {
    m_tabs->setCurrentIndex(3);
    m_vizTab->setDatasetKey("gas");
  });
  addAction(showGasViz);

  // ─── Switch to stars visualisation (3) ──────────────────────
  QAction *showStarsViz = new QAction(tr("Stars Visualisation"), this);
  showStarsViz->setShortcut(QKeySequence(Qt::Key_3));
  showStarsViz->setShortcutContext(Qt::ApplicationShortcut);
  connect(showStarsViz, &QAction::triggered, this, [this] {
    m_tabs->setCurrentIndex(3);
    m_vizTab->setDatasetKey("stars");
  });
  addAction(showStarsViz);

  // ─── Switch to gas temperature visualisation (4) ─────────────
  QAction *showGasTempViz =
      new QAction(tr("Gas Temperature Visualisation"), this);
  showGasTempViz->setShortcut(QKeySequence(Qt::Key_4));
  showGasTempViz->setShortcutContext(Qt::ApplicationShortcut);
  connect(showGasTempViz, &QAction::triggered, this, [this] {
    m_tabs->setCurrentIndex(3);
    m_vizTab->setDatasetKey("gas_temperature");
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

  // ─── Dashboard updates from DataWatcher ───────────────────────
  connect(m_dataWatcher, &DataWatcher::percentRunChanged, this,
          &MainWindow::updateProgressBar);
  connect(m_dataWatcher, &DataWatcher::stepChanged, this,
          &MainWindow::updateStepCounter);

  // ─── Plot updates from DataWatcher ──────────────────────────
  connect(m_dataWatcher, &DataWatcher::stepChanged, m_wallTimePlot,
          &PlotWidget::refresh);
  connect(m_dataWatcher, &DataWatcher::stepChanged, m_particlePlot,
          &PlotWidget::refresh);

  // ─── Update the top box top widget on a Timer ─────────────────────
  m_topRotateTimer = new QTimer(this);
  m_topRotateTimer->setInterval(10000);
  connect(m_topRotateTimer, &QTimer::timeout, this, &MainWindow::rotateTopPage);
  m_topRotateTimer->start();
}

/**
 * @brief Creates the main splitter and top/bottom sections of the window.
 *
 * Sets up a vertical splitter with:
 *  - Page 0: dashboard (counter + progress bar)
 *  - Pages 1–3: reserved for plots (added later in createPlots())
 *  - A middle expandable spacer
 *  - A bottom QTabWidget for Home/Log/Visualise/Diagnostics
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

  // ─── Bottom: tab widget ─────────────────────────────────────
  m_tabs = new QTabWidget(this);
  m_tabs->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  m_splitter->addWidget(m_tabs);

  // Set initial splitter sizes (approx. 27% / 25% / 48%)
  int totalH = height();
  int topH = int(0.2735 * totalH + 0.5);
  int midH = int(0.2564 * totalH + 0.5);
  int botH = totalH - topH - midH;
  m_splitter->setSizes({topH, midH, botH});

  setCentralWidget(m_splitter);
}

void MainWindow::createTabs(CommandLineParser *cmdParser) {
  // Create the tab widget and its pages
  m_tabs->tabBar()->setExpanding(false);
  m_tabs->setDocumentMode(true);

  m_tabs->addTab(new HomeTabWidget, tr("Home"));
  m_logTab = new LogTabWidget(cmdParser->logFilePath(), this);
  m_tabs->addTab(m_logTab, tr("Log"));
  m_tabs->addTab(new DiagTabWidget, tr("Diagnostics"));
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

  // Make another widget for the step counter
  m_topStack->addWidget(m_stepCounter);
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
  // First make the visualisation tab widget
  m_vizTab = new VizTabWidget();
  m_tabs->addTab(m_vizTab, tr("Visualise"));
  QString imagesDir = m_simCtrl->simulationDirectory() + "/images";

  // Start watching the images directory
  m_vizTab->watchImageDirectory(imagesDir);
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

void MainWindow::updateStepCounter(int step) { m_stepCounter->setStep(step); }

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
  if (index >= 0 && index < m_tabs->count()) {
    m_tabs->setCurrentIndex(index);
  }
}
