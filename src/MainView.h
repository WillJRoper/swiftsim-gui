#pragma once

#include "PlotWidget.h"
#include <QAction>
#include <QActionGroup>
#include <QLabel>
#include <QMainWindow>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QStackedWidget>

class SimulationController;
class LogTabWidget;
class QMenu;
class QTabWidget;
class CommandLineParser;
class StyledSplitter;
class DataWatcher;
class StepCounterWidget;
class PlotWidget;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(SimulationController *simCtrl,
                      CommandLineParser *cmdParser, QWidget *parent = nullptr);

private:
  // setup routines
  void createSplitterAndLayouts();
  void createActions();
  void createTabs(CommandLineParser *cmdParser);
  void createProgressBar();
  void createPlots();

  void switchToTab(int index);

  // slots
  void changeLogFontSize();

  // data
  SimulationController *m_simCtrl;
  DataWatcher *m_dataWatcher;
  LogTabWidget *m_logTab;
  QTabWidget *m_tabs;

  // NEW: sections for splitter layout
  QStackedWidget *m_topStack = nullptr;
  QWidget *m_middleGap = nullptr;
  StyledSplitter *m_splitter = nullptr;

  // progress bar
  QProgressBar *m_progressBar;
  QPropertyAnimation *m_progressAnim;

  // Step Counter
  StepCounterWidget *m_stepCounter;

  // Functions for updating UI elements
  void updateProgressBar(double percent);
  void updateCurrentTimeLabel(double t);
  void updateStepCounter(int step);

  // Plots
  QStackedWidget *m_plotStack;
  PlotWidget *m_wallTimePlot;
  PlotWidget *m_percentPlot;
  PlotWidget *m_particlePlot;

  // Current time label
  QLabel *m_currentLabel;
  int m_currentLabelWidth = 12;
  int m_currentLabelPrecision = 3;
};
