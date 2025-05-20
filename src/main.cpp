// main.cpp
#include <QAction>
#include <QApplication>
#include <QFont>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

// Forward-declare your custom page widgets:
#include "DiagTabWidget.h"
#include "HomeTabWidget.h"
#include "LogTabWidget.h"
#include "MainView.h"
#include "SimulationController.h"
#include "VizTabWidget.h"

int main(int argc, char **argv) {
  QApplication app(argc, argv);

  SimulationController simCtrl;
  MainWindow win(&simCtrl);
  win.show();

  return app.exec();
}
