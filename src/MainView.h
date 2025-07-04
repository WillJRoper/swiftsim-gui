#pragma once

#include "PlotWidget.h"
#include "SerialHandler.h"
#include <QAction>
#include <QActionGroup>
#include <QLabel>
#include <QMainWindow>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QStackedWidget>
#include <QThread>

class SimulationController;
class LogTabWidget;
class QMenu;
class QTabWidget;
class CommandLineParser;
class StyledSplitter;
class DataWatcher;
class StepCounterWidget;
class PlotWidget;
class ImageProgressWidget;
class VizTabWidget;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(SimulationController *simCtrl,
                      CommandLineParser *cmdParser, QWidget *parent = nullptr);

private slots:
  /// Control for swapping the top page in the stacked widget at the top
  /// of the UI
  void rotateTopPage();

private:
  // setup routines
  void createSplitterAndLayouts();
  void createActions();
  void createsBottom(CommandLineParser *cmdParser);
  void createProgressBar();
  void createPlots();
  void createVisualisations();
  void createDataWatcher();
  void createCounters();

  void switchToTab(int index);

  // slots
  void changeLogFontSize();

  SimulationController *m_simCtrl;
  DataWatcher *m_dataWatcher = nullptr;
  QThread *m_dwThread = nullptr;
  LogTabWidget *m_logTab;
  QStackedWidget *m_bottomWidget;

  // Sections for splitter layout
  QStackedWidget *m_topStack = nullptr;
  QWidget *m_middleGap = nullptr;
  StyledSplitter *m_splitter = nullptr;

  // Progress bar
  ImageProgressWidget *m_progressWidget;

  // Counters
  StepCounterWidget *m_stepCounter;
  StepCounterWidget *m_wallClockCounter;
  StepCounterWidget *m_starsFormedCounter;
  StepCounterWidget *m_blackHolesFormedCounter;
  StepCounterWidget *m_ParticleUpdateCounter;

  // Functions for updating UI elements
  void updateProgressBar(double percent);
  void updateCurrentTimeLabel(double t);
  void updateStepCounter(int step);
  void updateWallClockCounter(double t);
  void updateStarsFormedCounter(double mass);
  void updateBlackHolesFormedCounter(int count);
  void updateParticleUpdateCounter(int count);
  void buttonUpdateUI(int id);

  // Plots
  PlotWidget *m_wallTimePlot;
  PlotWidget *m_particlePlot;
  PlotWidget *m_csfrdPlot;
  PlotWidget *m_updatesPlot;

  // Visualization tab (4 rotating‐cube datasets)
  VizTabWidget *m_vizTab;

  // Current time label
  QLabel *m_currentLabel;
  int m_currentLabelWidth = 12;
  int m_currentLabelPrecision = 3;

  // Actions for the menu bar
  QTimer *m_topRotateTimer = nullptr;

  /// Our serial handler for interfacing with the controls
  SerialHandler *m_serialHandler = nullptr;

  /// Sets up serial I/O on the given port and hooks up debug slots
  void createSerialHandler(const QString &portPath);
};
