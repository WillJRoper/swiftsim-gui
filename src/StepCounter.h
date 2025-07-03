#pragma once

#include <QWidget>
class QLCDNumber;
class QLabel;
class DataWatcher; // if you wire this up elsewhere

/**
 * @class StepCounterWidget
 * @brief Displays a titled step counter with a QLCDNumber.
 *
 * The title is centered above the LCD; the LCD keeps your glow effect.
 */
class StepCounterWidget : public QWidget {
  Q_OBJECT

public:
  /**
   * @brief Create a counter display.
   * @param title   Text to show above the LCD display, centered.
   * @param parent  Parent widget (optional).
   */
  explicit StepCounterWidget(const QString &title, QWidget *parent = nullptr,
                             int digitCount = 7);

  /// @brief Update the displayed step (with leading zeros).
  void setStep(int step);

  /**
   * @brief Expand the number of digits in the step counter.
   * @param newDigitCount  New number of digits to display
   */
  void expandDigits(int newDigitCount);

private:
  QLabel *m_titleLabel; ///< Centered title
  QLCDNumber *m_lcd;    ///< Segmented‐style display
  int m_digitCount = 7; ///< Number of digits in the step counter
};
