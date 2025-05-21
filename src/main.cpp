
#include "CommandLineParser.h"
#include "MainView.h"

#include "SimulationController.h"
#include <QApplication>
#include <QDebug>

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  app.setApplicationName("swift_gui");
  app.setApplicationVersion("0.1");

  // 1) Load the global QSS
  QFile qssFile(":/styles/app.qss");
  // If not using resources: QFile qssFile("styles/app.qss");
  if (qssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QTextStream ts(&qssFile);
    app.setStyleSheet(ts.readAll());
  }

  // 1) Parse command line
  CommandLineParser cli(app);
  cli.process(app);
  QString simDir = cli.simulationDirectory();
  QString swiftDir = cli.swiftDirectory();

  qDebug() << "Simulation directory:" << simDir;
  qDebug() << "Swift directory:" << swiftDir;

  // 2) Pass the simDir into your controller (add a ctor or setter)
  SimulationController simCtrl(nullptr, simDir, swiftDir);

  // 3) Launch main window
  MainWindow win(&simCtrl);
  win.show();
  return app.exec();
}
