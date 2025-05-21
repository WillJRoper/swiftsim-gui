#include "CommandLineParser.h"
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>

CommandLineParser::CommandLineParser(QCoreApplication & /*app*/)
    : m_parser(new QCommandLineParser), m_simDir("mock_sim") {
  // You could also set the description here, but we do it in process()
  m_parser->addHelpOption();
  m_parser->addVersionOption();

  QCommandLineOption simDirOpt(
      QStringList{"d", "sim-dir"},
      "Path to the simulation directory (default: mock_sim).", "path",
      m_simDir);
  m_parser->addOption(simDirOpt);

  QCommandLineOption swiftDirOpt(
      QStringList{"s", "swift-dir"},
      "Path to the Swift installation (default: /usr/local/bin).", "path",
      "/usr/local/bin");
  m_parser->addOption(swiftDirOpt);
}

void CommandLineParser::process(QCoreApplication &app) {
  // Optionally set application metadata for help/version display
  m_parser->setApplicationDescription(app.applicationName());
  m_parser->process(app);

  m_simDir = m_parser->value("sim-dir");
  m_swiftDir = m_parser->value("swift-dir");
}

QString CommandLineParser::simulationDirectory() const { return m_simDir; }

QString CommandLineParser::swiftDirectory() const {
  return m_parser->value("swift-dir");
}
