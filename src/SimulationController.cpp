// SimulationController.cpp
#include "SimulationController.h"
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QStandardPaths>
#include <QTextStream>

SimulationController::SimulationController(QObject *parent,
                                           const QString &simDir,
                                           const QString &swiftDir)
    : QObject(parent), m_logText("Ready.\n"), m_simDir(simDir),
      m_swiftDir(swiftDir), m_process(), m_logFile() {
  m_process.setProcessChannelMode(QProcess::MergedChannels);
  connect(&m_process, &QProcess::readyRead, this,
          &SimulationController::onProcessReadyRead);
  connect(&m_process,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          &SimulationController::onProcessFinished);
}

SimulationController::~SimulationController() {
  // 1) If the process is still running, stop it cleanly
  if (m_process.state() != QProcess::NotRunning) {
    m_process.kill();
    // give it up to one second to shut down
    m_process.waitForFinished(1000);
  }

  // 2) Close the log file if open
  if (m_logFile.isOpen()) {
    m_logFile.close();
  }
}

QString SimulationController::logText() const { return m_logText; }
QStringList SimulationController::visualizationFiles() const {
  return m_visuals;
}
QString SimulationController::simulationDirectory() const { return m_simDir; }
QString SimulationController::swiftDirectory() const { return m_swiftDir; }

void SimulationController::newSimulation() {
  m_logText = "Started new simulation.\n";
  emit logTextChanged();
  m_visuals.clear();
  emit visualsChanged();
}

void SimulationController::openSimulation() {
  QString dir = QFileDialog::getExistingDirectory(
      nullptr, tr("Select Simulation Directory"), m_simDir,
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if (!dir.isEmpty() && dir != m_simDir) {
    m_simDir = dir;
    emit simulationDirectoryChanged(m_simDir);
    m_logText += tr("Simulation directory set to: %1\n").arg(dir);
    emit logTextChanged();
  }
}

void SimulationController::configure() {
  m_logText += "Configured simulation parameters.\n";
  emit logTextChanged();
}

void SimulationController::compile() {
  m_logText += "Compiled simulation code.\n";
  emit logTextChanged();
}

void SimulationController::dryRun() {
  m_logText += "Performed dry run.\n";
  emit logTextChanged();
}

void SimulationController::run() {
  emit runStarted();

  if (m_process.state() != QProcess::NotRunning) {
    m_process.kill();
    m_process.waitForFinished();
  }

  QString outPath = QDir(m_simDir).filePath("log.txt");
  m_logFile.setFileName(outPath);
  if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Truncate |
                      QIODevice::Text)) {
    qWarning() << "Cannot open log for writing:" << outPath;
    return;
  }

  QString stdbufPath = QStandardPaths::findExecutable("stdbuf");
  QString program;
  QStringList args;

  if (!stdbufPath.isEmpty()) {
    program = stdbufPath;
    args << "-oL" << QDir(m_swiftDir).filePath("swift");
  } else {
    program = QDir(m_swiftDir).filePath("swift");
  }
  args << "--eagle" << "--cosmology" << "--threads" << "8" << "eagle_6.yml";

  m_process.setWorkingDirectory(m_simDir);
  m_process.start(program, args);
  if (!m_process.waitForStarted(3000)) {
    qWarning() << "Failed to start process:" << program << args;
  }
}

void SimulationController::onProcessReadyRead() {
  if (!m_logFile.isOpen())
    return;
  QByteArray chunk = m_process.readAll();
  if (chunk.isEmpty())
    return;

  QTextStream out(&m_logFile);
  out << QString::fromLocal8Bit(chunk);
  m_logFile.flush();

  emit newLogLinesWritten();
}

void SimulationController::onProcessFinished(int /*exitCode*/,
                                             QProcess::ExitStatus /*status*/) {
  m_logFile.close();
  QDir d(m_simDir);
  m_visuals = d.entryList({"*.png"}, QDir::Files, QDir::Name);
  emit visualsChanged();
}
