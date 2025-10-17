#pragma once

#include <QString>

class QCoreApplication;
class QCommandLineParser;

class CommandLineParser {
public:
  explicit CommandLineParser(QCoreApplication &app);

  /// Parse the arguments (handles --help/--version) and store values.
  void process(QCoreApplication &app);

  /// Returns the path passed via --sim-dir (or the default).
  QString simulationDirectory() const;

  /// Returns the path passed via --swift-dir (or the default).
  QString swiftDirectory() const;

  /// Returns the path passed via --images-path (or the default).
  QString imagesPath() const;

  /// Returns the path passed via --log-file (or the default).
  QString logFilePath() const;

  /// Returns the path passed via --params-file (or the default).
  QString paramFilePath() const;

private:
  QCommandLineParser *m_parser;
  QString m_simDir;
  QString m_swiftDir;
  QString m_imagesPath;
  QString m_logFilePath;
  QString m_paramFilePath;
};
