
#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QString>
#include <QTimer>

/**
 * @brief Watches a whitespace-separated table file (gui_data.txt) and emits
 * per-step and cumulative values via Qt signals.
 *
 * The file is expected to have exactly 9 columns per line:
 *   1) step
 *   2) scale factor
 *   3) redshift
 *   4) number of parts
 *   5) number of gparts
 *   6) number of sparts
 *   7) number of black holes
 *   8) wallclock time for the step
 *   9) percentage of full run
 */
class DataWatcher : public QObject {
  Q_OBJECT

public:
  /**
   * @brief Constructs a new DataWatcher.
   * @param filePath  Absolute or relative path to gui_data.txt
   * @param parent    Optional QObject parent for ownership
   */
  explicit DataWatcher(const QString &filePath, QObject *parent = nullptr);

private slots:
  /**
   * @brief Slot invoked when the watched file is modified or replaced.
   *        Debounces rapid changes via a QTimer.
   * @param path  The file path that changed (should match filePath)
   */
  void onFileChanged(const QString &path);

  /**
   * @brief Reads gui_data.txt, extracts the last non-empty line, and passes it
   *        to parseLine().
   */
  void updateData();

private:
  /**
   * @brief Parses a single whitespace-separated line and emits all
   * corresponding signals. Also updates and emits cumulative totals.
   * @param line  One full line from the file (must contain 9 columns)
   */
  void parseLine(const QString &line);

  QString m_filePath;            ///< Path to gui_data.txt
  QFileSystemWatcher *m_watcher; ///< Watches file for changes
  QTimer *m_timer;               ///< Debounce timer

  // Cumulative totals (sums over all parsed steps)
  qint64 m_totalNumParts = 0;
  qint64 m_totalGParts = 0;
  qint64 m_totalSParts = 0;
  qint64 m_totalBlackHoles = 0;
  double m_totalWallClockTime = 0;

signals:
  // Per-step values
  void stepChanged(int step);
  void scaleFactorChanged(double scaleFactor);
  void redshiftChanged(double redshift);
  void numberOfPartsChanged(qint64 numParts);
  void numberOfGPartsChanged(qint64 numGParts);
  void numberOfSPartsChanged(qint64 numSParts);
  void numberOfBlackHolesChanged(qint64 numBlackHoles);
  void wallClockTimeForStepChanged(double time);
  void percentRunChanged(double percent);

  // Cumulative totals
  void totalNumberOfPartsChanged(qint64 totalParts);
  void totalNumberOfGPartsChanged(qint64 totalGParts);
  void totalNumberOfSPartsChanged(qint64 totalSParts);
  void totalNumberOfBlackHolesChanged(qint64 totalBlackHoles);
  void totalWallClockTimeChanged(double totalTime);
};
