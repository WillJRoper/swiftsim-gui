#include "DataWatcher.h"
#include <QDebug>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

/**************************************************************************************************/
/*                                    Constructor / Destructor */
/**************************************************************************************************/
DataWatcher::DataWatcher(const QString &filePath, QObject *parent)
    : QObject(parent), m_filePath(filePath),
      m_watcher(new QFileSystemWatcher(this)), m_timer(new QTimer(this)) {
  // 1) Configure one-shot debounce timer
  m_timer->setSingleShot(true);
  m_timer->setInterval(1000); // 1000 ms debounce
  connect(m_timer, &QTimer::timeout, this, &DataWatcher::updateData);

  // 2) Watch the file; onFileChanged will re-add if it's replaced
  m_watcher->addPath(m_filePath);
  connect(m_watcher, &QFileSystemWatcher::fileChanged, this,
          &DataWatcher::onFileChanged);

  // 3) No initial load on the UI thread—will be triggered when thread starts
}

/**************************************************************************************************/
/*                                 Debounce and Re-watch slot */
/**************************************************************************************************/
void DataWatcher::onFileChanged(const QString &path) {
  // 1) Restart debounce timer
  m_timer->start();

  // 2) If file was replaced, re-add so we keep watching
  if (!m_watcher->files().contains(path)) {
    m_watcher->addPath(path);
  }
}

/**************************************************************************************************/
/*                               Read & parse on timer timeout */
/**************************************************************************************************/
void DataWatcher::updateData() {
  QFile file(m_filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qWarning() << "DataWatcher: failed to open" << m_filePath;
    return;
  }

  // 1) Read all lines, split on newline, skip empties
  QStringList lines = QTextStream(&file).readAll().split(
      QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
  file.close();

  // 2) Need at least header + one data line
  if (lines.size() <= 1)
    return;

  // 3) Drop header
  lines.removeFirst();
  const QString &lastLine = lines.last();

  // 4) Split into columns
  QStringList parts =
      lastLine.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
  if (parts.size() < 13) {
    qWarning() << "DataWatcher: malformed line (expected ≥13 cols):"
               << lastLine;
    return;
  }

  // 5) Always emit stepChanged and percentRunChanged
  bool okStep = false, okPct = false, okStarMass = false, okRedshift = false;
  int step = parts[0].toInt(&okStep);
  double pct = parts[12].toDouble(&okPct);
  double starMass = parts[14].toDouble(&okStarMass) * 1e10; // Convert to Msun
  double redshift = parts[2].toDouble(&okRedshift);
  if (okStep)
    emit stepChanged(step);
  if (okPct)
    emit percentRunChanged(pct);
  if (okStarMass)
    emit starMassChanged(starMass);
  if (okRedshift)
    emit redshiftChanged(redshift);

  // 6) Decide if we “emit heavy” (≥ half g-parts updated)
  bool emitHeavy = true;
  qint64 gUpd = parts[4].toLongLong(&emitHeavy);
  if (emitHeavy && m_numGParts > 0 && gUpd < m_numGParts / 2)
    emitHeavy = false;

  // 7) Emit the heavier signals only if allowed
  if (emitHeavy) {
    bool okSF = false, okRZ = false, okWT = false, okBH = false;
    double sf = parts[1].toDouble(&okSF);
    double rz = parts[2].toDouble(&okRZ);
    double wt = parts[11].toDouble(&okWT);
    int bh = parts[6].toInt(&okBH);
    if (okSF)
      emit scaleFactorChanged(sf);
    if (okRZ)
      emit redshiftChanged(rz);
    if (okWT)
      emit wallClockTimeForStepChanged(wt);
    emit numberOfGPartsChanged(gUpd);
    if (okBH)
      emit numberofBHChanged(bh);

    // Sum up the wall-clock time
    double totalTime = 0.0;
    long long totalUpdates = 0;
    for (const QString &line : lines) {
      QStringList cols =
          line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
      if (cols.size() >= 13) {
        bool ok = false;
        double time = cols[11].toDouble(&ok);
        if (ok)
          totalTime += time;

        // GParts are all parts so we just need to accumulate these
        int gupdates = cols[8].toInt(&ok);
        if (ok)
          totalUpdates += gupdates;
      }
    }
    emit totalWallClockTimeChanged(totalTime);
    emit totalPartUpdatesChanged(totalUpdates);
  }

  // 8) Capture total g-parts once
  if (gUpd > m_numGParts)
    m_numGParts = int(gUpd);
}
