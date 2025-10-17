#pragma once

#include "ScaledPixmapLabel.h"

#include <QLabel>
#include <QProcess>
#include <QTimer>
#include <QWidget>

/**
 * @class PlotWidget
 * @brief Runs an external Python script to generate a plot PNG and displays it.
 *
 * Debounces multiple refresh() calls, queues at most one extra request,
 * and invokes the Python script asynchronously via QProcess.
 */
class PlotWidget : public QWidget {
  Q_OBJECT

public:
  /**
   * @brief Construct a new PlotWidget.
   * @param scriptPath  Path to the Python plotting script.
   * @param csvPath     Path to the input CSV/GUI-data file.
   * @param outputPng   Path for the generated PNG.
   * @param parent      Parent QWidget (optional).
   */
  explicit PlotWidget(const QString &scriptPath, const QString &csvPath,
                      const QString &outputPng, QWidget *parent = nullptr);

public slots:
  /**
   * @brief Request regeneration of the plot for the given step.
   *
   * These calls are debounced: rapid successive calls will only trigger
   * one actual script run plus at most one queued rerun.
   */
  void refresh(int step);

private slots:
  /// Fired after the debounce interval; actually starts the QProcess.
  void onDebounceTimeout();
  /// Fired when the Python script QProcess finishes; updates the display.
  void onPlotProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
  void startPlotProcess(int step);

  QString m_scriptPath;            ///< The Python script to run
  QString m_csvPath;               ///< The CSV or GUI-data input
  QString m_outputPng;             ///< Where to write the PNG
  ScaledPixmapLabel *m_imageLabel; ///< Displays the resulting QPixmap

  QTimer *m_debounceTimer; ///< Debounce timer
  int m_pendingStep = 0;   ///< Last requested step

  QProcess *m_proc = nullptr; ///< Current process, or nullptr
  bool m_queued = false;      ///< True if a new request came in while busy
};
