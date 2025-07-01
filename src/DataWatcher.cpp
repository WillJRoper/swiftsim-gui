#include "DataWatcher.h"
#include <QDebug>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

// 1) Construct: initialize watcher, timer, and load initial data
DataWatcher::DataWatcher(const QString &filePath, QObject *parent)
    : QObject(parent), m_filePath(filePath),
      m_watcher(new QFileSystemWatcher(this)), m_timer(new QTimer(this)) {
  // Configure one-shot debounce timer
  m_timer->setSingleShot(true);
  connect(m_timer, &QTimer::timeout, this, &DataWatcher::updateData);

  // Watch the file; if it’s replaced, onFileChanged will re-add it
  m_watcher->addPath(m_filePath);
  connect(m_watcher, &QFileSystemWatcher::fileChanged, this,
          &DataWatcher::onFileChanged);

  // Initial load
  updateData();
}

// 2) Debounce file-changed notifications and ensure watch persists
void DataWatcher::onFileChanged(const QString &path) {
  // Restart debounce timer (100 ms)
  m_timer->start(100);

  // If file was rotated/replaced, re-add the path so we keep watching
  if (!m_watcher->files().contains(path)) {
    m_watcher->addPath(path);
  }
}

// 3) Read the file, extract the last non-empty line, and parse
void DataWatcher::updateData() {
  QFile file(m_filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qWarning() << "DataWatcher: failed to open" << m_filePath;
    return;
  }

  // Read all lines, split on newlines
  QStringList lines = QTextStream(&file).readAll().split(
      QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
  file.close();

  // Need at least one header + one data line
  if (lines.size() <= 1) {
    return;
  }

  // Remove the header
  lines.removeFirst();

  // Parse the last data line
  parseLine(lines.last());
}

// 4) Parse a single whitespace-separated line into columns and emit signals
void DataWatcher::parseLine(const QString &line) {
  const QStringList parts =
      line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
  if (parts.size() < 9) {
    qWarning() << "DataWatcher: malformed line (expected 9 cols):" << line;
    return;
  }

  bool ok = false;
  int idx = 0;

  // 1) Step
  int step = parts[idx++].toInt(&ok);
  if (ok)
    emit stepChanged(step);

  // 2) Scale factor
  double scaleFactor = parts[idx++].toDouble(&ok);
  if (ok)
    emit scaleFactorChanged(scaleFactor);

  // 3) Redshift
  double redshift = parts[idx++].toDouble(&ok);
  if (ok)
    emit redshiftChanged(redshift);

  // 4) Number of parts
  qint64 numParts = parts[idx++].toLongLong(&ok);
  if (ok) {
    emit numberOfPartsChanged(numParts);
    m_totalNumParts += numParts;
    emit totalNumberOfPartsChanged(m_totalNumParts);
  }

  // 5) Number of gparts
  qint64 numGParts = parts[idx++].toLongLong(&ok);
  if (ok) {
    emit numberOfGPartsChanged(numGParts);
    m_totalGParts += numGParts;
    emit totalNumberOfGPartsChanged(m_totalGParts);
  }

  // 6) Number of sparts
  qint64 numSParts = parts[idx++].toLongLong(&ok);
  if (ok) {
    emit numberOfSPartsChanged(numSParts);
    m_totalSParts += numSParts;
    emit totalNumberOfSPartsChanged(m_totalSParts);
  }

  // 7) Number of black holes
  qint64 numBH = parts[idx++].toLongLong(&ok);
  if (ok) {
    emit numberOfBlackHolesChanged(numBH);
    m_totalBlackHoles += numBH;
    emit totalNumberOfBlackHolesChanged(m_totalBlackHoles);
  }

  // 8) Wall-clock time for this step
  double wallTime = parts[idx++].toDouble(&ok);
  if (ok) {
    emit wallClockTimeForStepChanged(wallTime);
    m_totalWallClockTime += wallTime;
    emit totalWallClockTimeChanged(m_totalWallClockTime);
  }

  // 9) Percentage of full run
  double percentRun = parts[idx++].toDouble(&ok);
  if (ok)
    emit percentRunChanged(percentRun);
}
