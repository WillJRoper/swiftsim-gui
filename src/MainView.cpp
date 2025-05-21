
#include "MainView.h"
#include "DiagTabWidget.h"
#include "HomeTabWidget.h"
#include "LogTabWidget.h"
#include "SimulationController.h"
#include "VizTabWidget.h"

#include <QAction>
#include <QInputDialog>
#include <QMenu>
#include <QMenuBar>
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

  createTabs();
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
  m_runAct = new QAction(tr("&Run"), this);
  m_runAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
  connect(m_configureAct, &QAction::triggered, m_simCtrl,
          &SimulationController::configure);
  connect(m_compileAct, &QAction::triggered, m_simCtrl,
          &SimulationController::compile);
  connect(m_dryRunAct, &QAction::triggered, m_simCtrl,
          &SimulationController::dryRun);
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
}

void MainWindow::createMenus() {
  m_fileMenu = menuBar()->addMenu(tr("&File"));
  m_fileMenu->addAction(m_newSimAct);
  m_fileMenu->addAction(m_openSimAct);

  m_swiftMenu = menuBar()->addMenu(tr("&Swiftsim"));
  m_swiftMenu->addAction(m_configureAct);
  m_swiftMenu->addAction(m_compileAct);
  m_swiftMenu->addAction(m_dryRunAct);
  m_swiftMenu->addAction(m_runAct);

  m_logMenu = menuBar()->addMenu(tr("&Log"));
  m_logMenu->addAction(m_logFontSizeAct);
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
  m_tabs->addTab(new VizTabWidget, tr("Visualise"));
  m_tabs->addTab(new DiagTabWidget, tr("Diagnostics"));

  auto *container = new QWidget(this);
  auto *layout = new QVBoxLayout(container);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(m_tabs);
  setCentralWidget(container);
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
