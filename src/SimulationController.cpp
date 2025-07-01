// SimulationController.cpp
#include "SimulationController.h"
#include "RuntimeOptions.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QString>
#include <QTextStream>
#include <string>
#include <yaml-cpp/yaml.h>

SimulationController::SimulationController(QObject *parent,
                                           const QString &simDir,
                                           const QString &swiftDir,
                                           const std::string &paramsPath)
    : QObject(parent), m_logText("Ready.\n"), m_simDir(simDir),
      m_swiftDir(swiftDir), m_process(), m_logFile() {
  m_process.setProcessChannelMode(QProcess::MergedChannels);
  connect(&m_process, &QProcess::readyRead, this,
          &SimulationController::onProcessReadyRead);
  connect(&m_process,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          &SimulationController::onProcessFinished);

  // Read the time integration parameters from params.yaml
  readTimeIntegrationParams(paramsPath);

  // Set up and attach the runtime options dialog
  m_runtimeOpts = new RuntimeOptionsDialog(this);

  qDebug() << "SimulationController initialized with directory:" << m_simDir
           << "and Swift directory:" << m_swiftDir;
  qDebug() << "Start time:" << startTime() << "End time:" << endTime();
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

void SimulationController::runDryRun() {
  emit runStarted();
  if (m_process.state() != QProcess::NotRunning) {
    m_process.kill();
    m_process.waitForFinished();
  }
  // reset the log
  QString outPath = QDir(m_simDir).filePath("log.txt");
  m_logFile.setFileName(outPath);
  if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Truncate |
                      QIODevice::Text)) {
    qWarning() << "Cannot open log for writing:" << outPath;
    return;
  }

  // assemble your simulator‐side arguments with “--dry-run” baked in:
  QStringList simArgs = m_runtimeOpts->runtimeArgs(QStringLiteral("--dry-run"));
  qDebug() << "Dry‐run args:" << simArgs;

  QString program;
  QStringList args;
  QString stdbufPath = QStandardPaths::findExecutable("stdbuf");
  QString swiftPath = QDir(m_swiftDir).filePath("swift");

  if (!stdbufPath.isEmpty()) {
    program = stdbufPath;
    // stdbuf options first, then your swift binary, then its flags
    args << "-oL" << swiftPath;
    args += simArgs;
  } else {
    program = swiftPath;
    args = simArgs;
  }

  m_process.setWorkingDirectory(m_simDir);
  m_process.setProcessChannelMode(QProcess::MergedChannels);
  m_process.start(program, args);
  if (!m_process.waitForStarted(3000)) {
    qWarning() << "Failed to start process:" << program << args;
  }
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

  QStringList simArgs = m_runtimeOpts->runtimeArgs(); // no extras
  qDebug() << "Run args:" << simArgs;

  QString program;
  QStringList args;
  QString stdbufPath = QStandardPaths::findExecutable("stdbuf");
  QString swiftPath = QDir(m_swiftDir).filePath("swift");

  if (!stdbufPath.isEmpty()) {
    program = stdbufPath;
    args << "-oL" << swiftPath;
    args += simArgs;
  } else {
    program = swiftPath;
    args = simArgs;
  }

  m_process.setWorkingDirectory(m_simDir);
  m_process.setProcessChannelMode(QProcess::MergedChannels);
  m_process.start(program, args);
  if (!m_process.waitForStarted(3000)) {
    qWarning() << "Failed to start process:" << program << args;
  }
}

void SimulationController::onProcessReadyRead() {
  if (!m_logFile.isOpen()) {
    qWarning() << "Log file is not open, cannot write process output.";
    return;
  }
  QByteArray chunk = m_process.readAll();
  if (chunk.isEmpty()) {
    qDebug() << "No new output from process.";
    return;
  }

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

void SimulationController::readTimeIntegrationParams(const std::string &path) {
  try {
    YAML::Node doc = YAML::LoadFile(path);

    // Defaults
    m_tStart = 0.0;
    m_tEnd = 1.0;

    if (doc["TimeIntegration"]) {
      auto ti = doc["TimeIntegration"];
      m_tStart = ti["time_begin"] ? ti["time_begin"].as<double>() : 0.0;
      m_tEnd = ti["time_end"] ? ti["time_end"].as<double>() : 1.0;
    }

    // (Optional) cosmology block
    m_aStart = 0.0;
    m_aEnd = 1.0;
    if (doc["Cosmology"]) {
      auto cos = doc["Cosmology"];
      m_aStart = cos["age_begin"] ? cos["age_begin"].as<double>() : 0.0;
      m_aEnd = cos["age_end"] ? cos["age_end"].as<double>() : 1.0;
    }
  } catch (const YAML::Exception &e) {
    qWarning() << "YAML parse error in" << path << ":" << e.what();
    // keep defaults
  }
}

double SimulationController::startTime() const {
  if (m_runtimeOpts->withCosmology()) {
    return m_aStart;
  } else {
    return m_tStart;
  }
}

double SimulationController::endTime() const {
  if (m_runtimeOpts->withCosmology()) {
    return m_aEnd;
  } else {
    return m_tEnd;
  }
}
