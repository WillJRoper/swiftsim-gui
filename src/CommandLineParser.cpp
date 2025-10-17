#include "CommandLineParser.h"
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>

CommandLineParser::CommandLineParser(QCoreApplication & /*app*/)
    : m_parser(new QCommandLineParser) {
  // You could also set the description here, but we do it in process()
  m_parser->addHelpOption();
  m_parser->addVersionOption();

  QCommandLineOption simDirOpt(QStringList{"d", "sim-dir"},
                               "Path to the simulation directory.", "path");
  m_parser->addOption(simDirOpt);

  QCommandLineOption swiftDirOpt(QStringList{"s", "swift-dir"},
                                 "Path to the Swift installation.", "path");
  m_parser->addOption(swiftDirOpt);

  QCommandLineOption imagesPathOpt(QStringList{"i", "images-path"},
                                   "Path to the directory for storing images.",
                                   "path");
  m_parser->addOption(imagesPathOpt);
  QCommandLineOption logFilePathOpt(QStringList{"l", "log-file"},
                                    "Path to the log file.", "path");
  m_parser->addOption(logFilePathOpt);
  QCommandLineOption paramFilePathOpt(QStringList{"p", "params-file"},
                                      "Path to the parameters file.", "path");
  m_parser->addOption(paramFilePathOpt);
}

void CommandLineParser::process(QCoreApplication &app) {
  // Optionally set application metadata for help/version display
  m_parser->setApplicationDescription(app.applicationName());
  m_parser->process(app);

  m_simDir = m_parser->value("sim-dir");
  m_swiftDir = m_parser->value("swift-dir");
  m_imagesPath = m_parser->value("images-path");
  m_logFilePath = m_parser->value("log-file");
  m_paramFilePath = m_parser->value("params-file");

  // If you want to handle --help or --version, you can do it here
  if (m_parser->isSet("help")) {
    m_parser->showHelp();
    return;
  } else if (m_parser->isSet("version")) {
    m_parser->showVersion();
    return;
  }

  // Error if we haven't been handed everything we need
  if (m_simDir.isEmpty()) {
    qCritical()
        << "Error: Simulation directory must be specified with --sim-dir.";
    m_parser->showHelp();
    return;
  }
  if (m_swiftDir.isEmpty()) {
    qCritical() << "Error: Swift directory must be specified with --swift-dir.";
    m_parser->showHelp();
    return;
  }
  if (m_imagesPath.isEmpty()) {
    qCritical() << "Error: Images path must be specified with --images-path.";
    m_parser->showHelp();
    return;
  }
  if (m_logFilePath.isEmpty()) {
    qCritical() << "Error: Log file path must be specified with --log-file.";
    m_parser->showHelp();
    return;
  }
  if (m_paramFilePath.isEmpty()) {
    qCritical()
        << "Error: Parameters file path must be specified with --params-file.";
    m_parser->showHelp();
    return;
  }

  // Report what we have found
  qDebug() << "Simulation directory:" << m_simDir;
  qDebug() << "Swift directory:" << m_swiftDir;
  qDebug() << "Images path:" << m_imagesPath;
  qDebug() << "Log file path:" << m_logFilePath;
  qDebug() << "Parameters file path:" << m_paramFilePath;
}

QString CommandLineParser::simulationDirectory() const { return m_simDir; }

QString CommandLineParser::swiftDirectory() const {
  return m_parser->value("swift-dir");
}

QString CommandLineParser::imagesPath() const {
  return m_parser->value("images-path");
}

QString CommandLineParser::logFilePath() const {
  return m_parser->value("log-file");
}

QString CommandLineParser::paramFilePath() const {
  return m_parser->value("params-file");
}
