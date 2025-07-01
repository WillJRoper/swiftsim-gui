#include <iostream>

#include "DiagTabWidget.h"
#include "HomeTabWidget.h"
#include "LogTabWidget.h"
#include "MainView.h"
#include "RuntimeOptions.h"
#include "SimulationController.h"
#include "StyledSplitter.h"
#include "VizTabWidget.h"

#include <QAction>
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

  std::cout << "MainWindow initialized." << std::endl;

  // Set up each of the UI elements
  createSplitterAndLayouts();
  std::cout << "Created the splitter and layouts." << std::endl;
  createTabs(cmdParser);
  std::cout << "Created the tabs." << std::endl;
  createProgressBar();
  std::cout << "Created the progress bar." << std::endl;
  createActions();
  createMenus();
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

  connect(m_simCtrl, &SimulationController::newLogLinesWritten, m_logTab,
          [this]() { m_logTab->updateLogView(); });

  connect(m_simCtrl, &SimulationController::runStarted, this, [this]() {
    this->switchToTab(1); // Switch to the log tab
  });

  // Tab switching actions
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

  m_runtimeOptsAct = new QAction(tr("Runtime Options…"), this);
  connect(m_runtimeOptsAct, &QAction::triggered, this, [this]() {
    if (m_simCtrl->m_runtimeOpts->exec() == QDialog::Accepted) {
      // React to OK if needed
    }
  });
}

void MainWindow::createMenus() {
  m_fileMenu = menuBar()->addMenu(tr("&File"));
  m_fileMenu->addAction(m_newSimAct);
  m_fileMenu->addAction(m_openSimAct);

  m_swiftMenu = menuBar()->addMenu(tr("&Swiftsim"));
  m_swiftMenu->addAction(m_configureAct);
  m_swiftMenu->addAction(m_compileAct);
  m_swiftMenu->addAction(m_runtimeOptsAct);
  m_swiftMenu->addAction(m_dryRunAct);
  m_swiftMenu->addAction(m_runAct);

  m_logMenu = menuBar()->addMenu(tr("&Log"));
  m_logMenu->addAction(m_logFontSizeAct);

  QMenu *vizMenu = menuBar()->addMenu(tr("&Visualisation"));
  vizMenu->addAction(m_scaleLinearAct);
  vizMenu->addAction(m_scaleLogAct);
  vizMenu->addAction(m_scaleAutoAct);

  m_imagesMenu = vizMenu->addMenu(tr("&Images"));
  connect(m_imagesMenu, &QMenu::aboutToShow, this, [this]() {
    m_imagesMenu->clear();

    QDir imagesRoot(m_simCtrl->simulationDirectory() + "/images");
    QStringList entries =
        imagesRoot.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    for (int i = 0; i < entries.size(); ++i) {
      const QString name = entries.at(i);
      QAction *act = m_imagesMenu->addAction(name);
      act->setData(name);

      if (i < 9) {
        QKeySequence key(Qt::Key_1 + i);
        act->setShortcut(key);
        act->setShortcutContext(Qt::ApplicationShortcut);
      }

      connect(act, &QAction::triggered, this, [this, name]() {
        auto *viz = qobject_cast<VizTabWidget *>(m_tabs->widget(2));
        if (viz) {
          viz->setImageDirectory("images/" + name);
          switchToTab(2);
        }
      });
    }

    if (entries.isEmpty()) {
      m_imagesMenu->addAction(tr("<no sets found>"))->setEnabled(false);
    }
  });
}

void MainWindow::createSplitterAndLayouts() {
  // Create a splitter to allow resizing between sections
  m_splitter = new StyledSplitter(Qt::Vertical, this);
  m_splitter->setChildrenCollapsible(false);

  m_splitter->setStyleSheet(R"(
  QSplitter::handle {
    background-color: #444;
    height: 4px;
  }
)");

  // ─── Top: progress bar container ──────────────────────────────
  m_topBox = new QWidget;
  m_topBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  m_splitter->addWidget(m_topBox);

  // ─── Middle: resizable blank spacer ───────────────────────────
  m_middleGap = new QWidget;
  m_middleGap->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  m_splitter->addWidget(m_middleGap);

  // ─── Bottom: tabs ─────────────────────────────────────────────
  m_tabs = new QTabWidget(this);
  m_tabs->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  m_splitter->addWidget(m_tabs);

  // ─── Set initial proportions (roughly 27/25/47 percent) ───────
  const int totalH = height();
  const int topH = int(0.2735042735 * totalH + 0.5);
  const int midH = int(0.2564102564 * totalH + 0.5);
  const int botH = totalH - topH - midH;
  m_splitter->setSizes({topH, midH, botH});

  setCentralWidget(m_splitter);
  u

      void
      MainWindow::createTabs(CommandLineParser * cmdParser) {
    // Create the tab widget and its pages
    m_tabs->tabBar()->setExpanding(false);
    m_tabs->setDocumentMode(true);

    m_tabs->addTab(new HomeTabWidget, tr("Home"));
    m_logTab = new LogTabWidget(this);
    m_logTab->setFilePath(cmdParser->logFilePath());
    m_tabs->addTab(m_logTab, tr("Log"));
    m_tabs->addTab(new VizTabWidget(m_simCtrl), tr("Visualise"));
    m_tabs->addTab(new DiagTabWidget, tr("Diagnostics"));
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
    m_progressBar->setSizePolicy(QSizePolicy::Expanding,
                                 QSizePolicy::Preferred);
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

    // Now drive the bar from the LogTab’s current‐time signal:
    connect(
        m_logTab, &LogTabWidget::currentTimeChanged, this, [this](double t) {
          // Update the current time label
          QString txt = QString("%1").arg(t, m_currentLabelWidth, 'e',
                                          m_currentLabelPrecision, QChar(' '));
          m_currentLabel->setText(txt);
          std::cout << "Current time updated to: " << t << std::endl;

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

    // Add these to the splitter layout
    m_topBox->setLayout(tlLayout);
  }

  void MainWindow::changeLogFontSize() {
    bool ok;
    int current = m_logTab->font().pointSize();
    int size = QInputDialog::getInt(this, tr("Log Font Size"),
                                    tr("Point size:"), current, 6, 48, 1, &ok);
    if (ok) {
      m_logTab->setFontSize(size);
    }
  }

  void MainWindow::switchToTab(int index) {
    if (index >= 0 && index < m_tabs->count()) {
      m_tabs->setCurrentIndex(index);
    }
  }
