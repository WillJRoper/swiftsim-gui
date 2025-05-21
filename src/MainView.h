#pragma once

#include <QAction>
#include <QMainWindow>

class SimulationController;
class LogTabWidget;
class QMenu;
class QTabWidget;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(SimulationController *simCtrl, QWidget *parent = nullptr);

private:
  // setup routines
  void createActions();
  void createMenus();
  void createTabs();

  void switchToTab(int index);

  // slots
  void changeLogFontSize();

  // data
  SimulationController *m_simCtrl;
  LogTabWidget *m_logTab;
  QTabWidget *m_tabs;

  // actions
  QAction *m_newSimAct;
  QAction *m_openSimAct;
  QAction *m_configureAct;
  QAction *m_compileAct;
  QAction *m_dryRunAct;
  QAction *m_runAct;
  QAction *m_logFontSizeAct;

  // menus
  QMenu *m_fileMenu;
  QMenu *m_swiftMenu;
  QMenu *m_logMenu;
};
