#pragma once

// ─── Work around moc choking on QT_IGNORE_DEPRECATIONS pragmas ────
#ifdef Q_MOC_RUN
// moc defines Q_MOC_RUN; redefine the macro to a no-op so
// moc never sees the embedded pragmas in Qt headers.
#undef QT_IGNORE_DEPRECATIONS
#define QT_IGNORE_DEPRECATIONS(X) X
#endif

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
  Q_PROPERTY(double startTime READ startTime NOTIFY progressChanged)
  Q_PROPERTY(double endTime READ endTime NOTIFY progressChanged)

public:
  explicit SimulationController(
      QObject *parent = nullptr,
      const QString &simDir = QStringLiteral("mock_sim"),
      const QString &swiftDir = QString(),
      const std::string &paramsPath = std::string("params.yaml"));

  ~SimulationController();

  // property getters
  QString logText() const;
  QStringList visualizationFiles() const;
  QString simulationDirectory() const;
  QString swiftDirectory() const;
  double startTime() const;
  double endTime() const;

  void readTimeIntegrationParams(const std::string &path = "params.yaml");

public slots:
  void newSimulation();
  void openSimulation();
  void configure();
  void compile();

signals:
  void logTextChanged();
  void visualsChanged();
  void simulationDirectoryChanged(QString dir);
  void newLogLinesWritten();
  void runStarted();
  void progressChanged(int percent);

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

  // Progress tracking members
  double m_tStart = 0.0;
  double m_aStart = 0.0;
  double m_tEnd = 1.0;
  double m_aEnd = 1.0;
  double m_tCurrent = 0.0;
};
