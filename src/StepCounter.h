#pragma once

#include <QWidget>
class QLCDNumber;
class DataWatcher;

class StepCounterWidget : public QWidget {
  Q_OBJECT

public:
  /// @brief Create a counter display, wired to the given DataWatcher.
  explicit StepCounterWidget(QWidget *parent = nullptr);

  /// @brief Set the current step number to display.
  void setStep(int step);

private:
  QLCDNumber *m_lcd; ///< Segmented‐style display
};
