#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QString>
#include <QTimer>

/**
 * @brief Watches a whitespace-separated table file (gui_data.txt) and emits
 * per-step and cumulative values via Qt signals.
 *
 * The file is expected to have exactly 13 columns per line:
 *   1) step
 *   2) scale factor
 *   3) redshift
 *   4) (unused)
 *   5) number of gparts
 *   6) (unused)
 *   7) (unused)
 *   8) (unused)
 *   9) (unused)
 *  10) (unused)
 *  11) (unused)
 *  12) wallclock time for the step
 *  13) percentage of full run
 *
 * Emits two “always” signals for UI counters,
 * and only emits the heavier-weight signals when at least half of the g-parts
 * have updated.
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

public slots:
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

signals:
  // ─── Always-emitted, lightweight ───────────────────────────────────────
  void stepChanged(int step);
  void percentRunChanged(double percent);

  // ─── Heavier-weight, rate-limited by “half g-parts” rule ──────────────
  void scaleFactorChanged(double scaleFactor);
  void redshiftChanged(double redshift);
  void numberOfGPartsChanged(qint64 numGParts);
  void numberofBHChanged(int numBH);
  void starMassChanged(double totalMass);
  void wallClockTimeForStepChanged(double time);

  // ─── Totals from sums we do ────────────────────────────────
  void totalWallClockTimeChanged(double totalTime);
  void totalPartUpdatesChanged(int totalUpdates);

private:
  QString m_filePath;            ///< Path to gui_data.txt
  QFileSystemWatcher *m_watcher; ///< Watches file for changes
  QTimer *m_timer;               ///< Debounce timer

  int m_numGParts = 0; ///< Total g-parts in the simulation
};
