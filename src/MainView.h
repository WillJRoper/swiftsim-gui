#pragma once

#include <QAction>
#include <QActionGroup>
#include <QLabel>
#include <QMainWindow>
#include <QProgressBar>
#include <QPropertyAnimation>

class SimulationController;
class LogTabWidget;
class QMenu;
class QTabWidget;
class RuntimeOptionsDialog;
class CommandLineParser;
class StyledSplitter;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(SimulationController *simCtrl,
                      CommandLineParser *cmdParser, QWidget *parent = nullptr);

private:
  // setup routines
  void createSplitterAndLayouts();
  void createActions();
  void createMenus();
  void createTabs(CommandLineParser *cmdParser);
  void createProgressBar();

  void switchToTab(int index);

  // slots
  void changeLogFontSize();

  // data
  SimulationController *m_simCtrl;
  LogTabWidget *m_logTab;
  QTabWidget *m_tabs;

  // NEW: sections for splitter layout
  QWidget *m_topBox = nullptr;
  QWidget *m_middleGap = nullptr;
  StyledSplitter *m_splitter = nullptr;

  // progress bar
  QProgressBar *m_progressBar;
  QPropertyAnimation *m_progressAnim;

  // Current time label
  QLabel *m_currentLabel;
  int m_currentLabelWidth = 12;
  int m_currentLabelPrecision = 3;

  // Runtime options dialog
  RuntimeOptionsDialog *m_runtimeOptsDlg = nullptr;

  // actions
  QAction *m_newSimAct;
  QAction *m_openSimAct;
  QAction *m_configureAct;
  QAction *m_compileAct;
  QAction *m_dryRunAct;
  QAction *m_runAct;
  QAction *m_img_RunAct;
  QAction *m_logFontSizeAct;
  QAction *m_scaleLinearAct;
  QAction *m_scaleLogAct;
  QAction *m_scaleAutoAct;
  QActionGroup *m_scaleGroup;
  QAction *m_runtimeOptsAct;

  // menus
  QMenu *m_fileMenu;
  QMenu *m_swiftMenu;
  QMenu *m_logMenu;
  QMenu *m_vizMenu;
  QMenu *m_imagesMenu;
};
