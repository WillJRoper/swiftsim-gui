
#include "SimulationController.h"
#include <QDebug>

SimulationController::SimulationController(QObject *parent) : QObject(parent) {
  // nothing yet
}

void SimulationController::startLogWatcher(const QString &path, int maxLines) {
  Q_UNUSED(path);
  Q_UNUSED(maxLines);
  // stub: you’ll later start a timer or watcher here
}

QString SimulationController::logText() const {
  // stub: return the tail of your log once you wire it up
  return QString{};
}

QStringList SimulationController::visualizationFiles() const {
  // stub: return your list of image paths
  return QStringList{};
}
