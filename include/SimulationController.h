#pragma once

#include <QFile> // for m_logFile
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

class SimulationController : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString logText READ logText NOTIFY logTextChanged)
  Q_PROPERTY(QStringList visualizationFiles READ visualizationFiles NOTIFY
                 visualsChanged)
  Q_PROPERTY(QString simulationDirectory READ simulationDirectory NOTIFY
                 simulationDirectoryChanged)

public:
  explicit SimulationController(
      QObject *parent = nullptr,
      const QString &simDir = QStringLiteral("mock_sim"),
      const QString &swiftDir = QString());

  ~SimulationController();

  // property getters
  QString logText() const;
  QStringList visualizationFiles() const;
  QString simulationDirectory() const;
  QString swiftDirectory() const;

public slots:
  void newSimulation();
  void openSimulation();
  void configure();
  void compile();
  void dryRun();
  void run();

signals:
  void logTextChanged();
  void visualsChanged();
  void simulationDirectoryChanged(QString dir);
  void newLogLinesWritten();
  void runStarted();

private slots:
  void onProcessReadyRead();
  void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
  QString m_logText;
  QString m_simDir;
  QString m_swiftDir;
  QStringList m_visuals;

  QProcess m_process;
  QFile m_logFile;
};
