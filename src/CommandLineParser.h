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

private:
  QCommandLineParser *m_parser;
  QString m_simDir;
  QString m_swiftDir;
};
