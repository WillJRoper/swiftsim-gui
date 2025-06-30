
#include "MainView.h"
#include "DiagTabWidget.h"
#include "HomeTabWidget.h"
#include "LogTabWidget.h"
#include "RuntimeOptions.h"
#include "SimulationController.h"
#include "VizTabWidget.h"

#include <QAction>
#include <QDir>
#include <QInputDialog>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>

MainWindow::MainWindow(SimulationController *simCtrl, QWidget *parent)
    : QMainWindow(parent), m_simCtrl(simCtrl) {

  // Set the window title and size to fill the screen
  setWindowTitle(tr("Swift GUI"));
  setMinimumSize(800, 600);
  setWindowState(Qt::WindowMaximized);
  setWindowIcon(QIcon(":/images/swift-logo-white.png")); // set the icon

  // Set up each of the UI elements
  createTabs();
  createActions();
  createMenus();
  createProgressBar();
}

void MainWindow::createActions() {
  // File actions
  m_newSimAct = new QAction(tr("&New Simulation"), this);
  m_newSimAct->setShortcut(QKeySequence::New);
  m_newSimAct->setShortcutContext(Qt::ApplicationShortcut);
  m_openSimAct = new QAction(tr("&Open Simulation…"), this);
  m_openSimAct->setShortcut(QKeySequence::Open);
  m_openSimAct->setShortcutContext(Qt::ApplicationShortcut);
  connect(m_newSimAct, &QAction::triggered, m_simCtrl,
          &SimulationController::newSimulation);
  connect(m_openSimAct, &QAction::triggered, m_simCtrl,
          &SimulationController::openSimulation);

  // Swiftsim actions
  m_configureAct = new QAction(tr("&Configure…"), this);
  m_compileAct = new QAction(tr("&Compile"), this);
  m_dryRunAct = new QAction(tr("&Dry Run"), this);
  m_dryRunAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
  m_runAct = new QAction(tr("&Run"), this);
  m_runAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
  m_img_RunAct = new QAction(tr("&Run with Imaging"), this);
  m_img_RunAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
  connect(m_configureAct, &QAction::triggered, m_simCtrl,
          &SimulationController::configure);
  connect(m_compileAct, &QAction::triggered, m_simCtrl,
          &SimulationController::compile);
  connect(m_dryRunAct, &QAction::triggered, m_simCtrl,
          &SimulationController::runDryRun);
  connect(m_runAct, &QAction::triggered, m_simCtrl, &SimulationController::run);

  // Log menu “Font Size…” action
  m_logFontSizeAct = new QAction(tr("Font Size…"), this);
  connect(m_logFontSizeAct, &QAction::triggered, this,
          &MainWindow::changeLogFontSize);

  // Connect the simulation controller to the log tab
  connect(m_simCtrl, &SimulationController::simulationDirectoryChanged,
          m_logTab, [this]() {
            const QString path = m_simCtrl->simulationDirectory() + "/log.txt";
            m_logTab->setFilePath(path);
          });

  // Connect the simulation controller to the log tab
  connect(m_simCtrl, &SimulationController::newLogLinesWritten, m_logTab,
          [this]() { m_logTab->updateLogView(); });

  // Connect the start of the run to the log tab
  connect(m_simCtrl, &SimulationController::runStarted, this, [this]() {
    this->switchToTab(1); // Switch to the log tab
  });

  // Tab swtiching actions
  QAction *homeTabAct = new QAction(tr("Home"), this);
  homeTabAct->setShortcut(QKeySequence(Qt::Key_H));
  homeTabAct->setShortcutContext(Qt::ApplicationShortcut);
  connect(homeTabAct, &QAction::triggered, this, [this]() { switchToTab(0); });
  addAction(homeTabAct);
  QAction *logTabAct = new QAction(tr("Log"), this);
  logTabAct->setShortcut(QKeySequence(Qt::Key_L));
  logTabAct->setShortcutContext(Qt::ApplicationShortcut);
  connect(logTabAct, &QAction::triggered, this, [this]() { switchToTab(1); });
  addAction(logTabAct);
  QAction *vizTabAct = new QAction(tr("Visualise"), this);
  vizTabAct->setShortcut(QKeySequence(Qt::Key_V));
  vizTabAct->setShortcutContext(Qt::ApplicationShortcut);
  connect(vizTabAct, &QAction::triggered, this, [this]() { switchToTab(2); });
  addAction(vizTabAct);
  QAction *diagTabAct = new QAction(tr("Diagnostics"), this);
  diagTabAct->setShortcut(QKeySequence(Qt::Key_D));
  diagTabAct->setShortcutContext(Qt::ApplicationShortcut);
  connect(diagTabAct, &QAction::triggered, this, [this]() { switchToTab(3); });
  addAction(diagTabAct);

  // Visualisation → Scale Mode
  m_scaleGroup = new QActionGroup(this);
  m_scaleGroup->setExclusive(true);

  m_scaleLinearAct = new QAction(tr("Linear Scale"), this);
  m_scaleLinearAct->setCheckable(true);
  m_scaleLinearAct->setChecked(true);
  m_scaleGroup->addAction(m_scaleLinearAct);

  m_scaleLogAct = new QAction(tr("Logarithmic Scale"), this);
  m_scaleLogAct->setCheckable(true);
  m_scaleGroup->addAction(m_scaleLogAct);

  m_scaleAutoAct = new QAction(tr("Auto Min/Max"), this);
  m_scaleAutoAct->setCheckable(true);
  m_scaleGroup->addAction(m_scaleAutoAct);

  // Connect each to the VizTabWidget slot
  connect(m_scaleLinearAct, &QAction::triggered, this, [this]() {
    if (auto *w = qobject_cast<VizTabWidget *>(m_tabs->widget(2)))
      w->setScaleMode(VizTabWidget::ScaleMode::Linear);
  });
  connect(m_scaleLogAct, &QAction::triggered, this, [this]() {
    if (auto *w = qobject_cast<VizTabWidget *>(m_tabs->widget(2)))
      w->setScaleMode(VizTabWidget::ScaleMode::Logarithmic);
  });
  connect(m_scaleAutoAct, &QAction::triggered, this, [this]() {
    if (auto *w = qobject_cast<VizTabWidget *>(m_tabs->widget(2)))
      w->setScaleMode(VizTabWidget::ScaleMode::AutoMinMax);
  });

  // Runtime options action for bringing up the dialog
  m_runtimeOptsAct = new QAction(tr("Runtime Options…"), this);
  connect(m_runtimeOptsAct, &QAction::triggered, this, [this]() {
    // Create a fresh dialog each time and run it modally:
    if (m_simCtrl->m_runtimeOpts->exec() == QDialog::Accepted) {
      // optional: react to OK here if you need to do something immediately
    }
  });
}

void MainWindow::createMenus() {
  // File menu: add the actions for creating/opening simulations and working
  // with the file system
  m_fileMenu = menuBar()->addMenu(tr("&File"));
  m_fileMenu->addAction(m_newSimAct);
  m_fileMenu->addAction(m_openSimAct);

  // SWIFT menu: add the actions for interacting with the simulation
  m_swiftMenu = menuBar()->addMenu(tr("&Swiftsim"));
  m_swiftMenu->addAction(m_configureAct);
  m_swiftMenu->addAction(m_compileAct);
  m_swiftMenu->addAction(m_runtimeOptsAct);
  m_swiftMenu->addAction(m_dryRunAct);
  m_swiftMenu->addAction(m_runAct);

  // Log menu: add the font size action
  m_logMenu = menuBar()->addMenu(tr("&Log"));
  m_logMenu->addAction(m_logFontSizeAct);

  // Visualisation menu: just add the pre-built actions
  QMenu *vizMenu = menuBar()->addMenu(tr("&Visualisation"));
  vizMenu->addAction(m_scaleLinearAct);
  vizMenu->addAction(m_scaleLogAct);
  vizMenu->addAction(m_scaleAutoAct);

  // Image selectiom menu: add the actions for selecting the image
  m_imagesMenu = vizMenu->addMenu(tr("&Images"));

  // Check for the what images are available
  connect(m_imagesMenu, &QMenu::aboutToShow, this, [this]() {
    m_imagesMenu->clear();

    // Find all sub-directories under <simDir>/images
    QDir imagesRoot(m_simCtrl->simulationDirectory() + "/images");
    QStringList entries =
        imagesRoot.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    // For each entry, add an action
    for (int i = 0; i < entries.size(); ++i) {
      const QString name = entries.at(i);
      QAction *act = m_imagesMenu->addAction(name);
      act->setData(name);

      // First nine get digit shortcuts, context=app-wide
      if (i < 9) {
        QKeySequence key(Qt::Key_1 + i);
        act->setShortcut(key);
        act->setShortcutContext(Qt::ApplicationShortcut);
      }

      // When selected, tell the VizTabWidget to load that set
      connect(act, &QAction::triggered, this, [this, name]() {
        auto *viz = qobject_cast<VizTabWidget *>(m_tabs->widget(2));
        if (viz) {
          viz->setImageDirectory("images/" + name);
          switchToTab(2); // Switch to the Viz tab
        }
      });
    }

    // If no entries, show a placeholder
    if (entries.isEmpty()) {
      m_imagesMenu->addAction(tr("<no sets found>"))->setEnabled(false);
    }
  });
}

void MainWindow::createTabs() {

  // Create the tabs
  m_tabs = new QTabWidget(this);

  // Make the tabs left aligned and "nice" looking
  m_tabs->tabBar()->setExpanding(false);
  m_tabs->setDocumentMode(true);

  m_tabs->addTab(new HomeTabWidget, tr("Home"));
  m_logTab = new LogTabWidget(this);
  m_logTab->setFilePath(m_simCtrl->simulationDirectory() + "/log.txt");
  m_tabs->addTab(m_logTab, tr("Log"));
  m_tabs->addTab(new VizTabWidget(m_simCtrl), tr("Visualise"));
  m_tabs->addTab(new DiagTabWidget, tr("Diagnostics"));

  auto *container = new QWidget(this);
  auto *layout = new QVBoxLayout(container);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(m_tabs);
  setCentralWidget(container);
}

void MainWindow::createProgressBar() {
  // ─── Build a custom “timeline” widget ─────────────────────────
  auto *timeline = new QWidget(this);
  auto *tlLayout = new QHBoxLayout(timeline);
  tlLayout->setContentsMargins(0, 2, 0, 2);
  tlLayout->setSpacing(8);

  // The progress bar
  m_progressBar = new QProgressBar(timeline);
  m_progressBar->setRange(0, 100);
  m_progressBar->setTextVisible(false);
  m_progressBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  tlLayout->addWidget(m_progressBar);

  // Smooth animation of the bar value
  m_progressAnim = new QPropertyAnimation(m_progressBar, "value", this);
  m_progressAnim->setDuration(300);
  m_progressAnim->setEasingCurve(QEasingCurve::OutCubic);

  // Current time label (starting at the start time)
  double t0 = m_simCtrl->startTime();
  QString txt = QString("%1").arg(t0, m_currentLabelWidth, 'e',
                                  m_currentLabelPrecision, QChar(' '));
  m_currentLabel = new QLabel(txt, timeline);
  m_currentLabel->setStyleSheet(
      "QLabel { color: #E50000; font-weight: bold; }");
  tlLayout->addWidget(m_currentLabel);

  // Put it in the status bar
  statusBar()->addPermanentWidget(timeline, /*stretch=*/1);

  // Whenever we open or change simulation, re-read start/end
  connect(m_simCtrl, &SimulationController::simulationDirectoryChanged, this,
          [this](const QString &) {
            m_simCtrl->readTimeIntegrationParams();
            m_progressBar->setValue(0.0);
          });

  // Now drive the bar from the LogTab’s current‐time signal:
  connect(m_logTab, &LogTabWidget::currentTimeChanged, this, [this](double t) {
    // Update the current time label
    QString txt = QString("%1").arg(t, m_currentLabelWidth, 'e',
                                    m_currentLabelPrecision, QChar(' '));
    m_currentLabel->setText(txt);

    // get start/end from the controller
    double t0 = m_simCtrl->startTime();
    double t1 = m_simCtrl->endTime();
    double frac = (t - t0) / (t1 - t0);
    int pct = qBound(0, int(frac * 100.0 + 0.5), 100);
    m_progressAnim->stop();
    m_progressAnim->setStartValue(m_progressBar->value());
    m_progressAnim->setEndValue(pct);
    m_progressAnim->start();
  });
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
  if (index >= 0 && index < m_tabs->count()) {
    m_tabs->setCurrentIndex(index);
  }
}
