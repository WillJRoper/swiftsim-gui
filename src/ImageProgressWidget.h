#pragma once

#include <QPixmap>
#include <QWidget>

/**
 * @brief A widget that displays an image whose left portion is shown in color
 *        up to a set progress fraction, and the remainder in grayscale.
 */
class ImageProgressWidget : public QWidget {
  Q_OBJECT

public:
  /**
   * @brief Constructs the widget and loads the image at imagePath.
   * @param imagePath  Path to the source color image.
   * @param parent     Optional parent widget.
   */
  explicit ImageProgressWidget(const QString &imagePath,
                               QWidget *parent = nullptr);

  /**
   * @brief Set the current progress (0.0 to 1.0) and repaint.
   */
  void setProgress(double fraction);

  /**
   * @brief Get the current progress fraction.
   */
  double progress() const { return m_progress; }

protected:
  void paintEvent(QPaintEvent * /*event*/) override;

private:
  QPixmap m_color;   ///< The original color pixmap.
  QPixmap m_gray;    ///< A grayscale version of the pixmap.
  double m_progress; ///< Fraction [0..1] of image to show in color.

  /**
   * @brief Convert a QPixmap to grayscale.
   */
  static QPixmap toGrayscale(const QPixmap &src);
};
