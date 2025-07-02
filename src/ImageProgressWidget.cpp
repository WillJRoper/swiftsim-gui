#include "ImageProgressWidget.h"
#include <QImage>
#include <QPainter>

/*----------------------------------------------------------------------------*/
/* Constructors and Setup                                                    */
/*----------------------------------------------------------------------------*/

ImageProgressWidget::ImageProgressWidget(const QString &imagePath,
                                         QWidget *parent)
    : QWidget(parent), m_color(imagePath), m_gray(toGrayscale(m_color)),
      m_progress(0.0) {
  // Ensure we can scale in layouts
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

/*----------------------------------------------------------------------------*/
/* Public API                                                                 */
/*----------------------------------------------------------------------------*/

void ImageProgressWidget::setProgress(double fraction) {
  // Clamp between 0 and 1
  if (fraction < 0.0)
    fraction = 0.0;
  if (fraction > 1.0)
    fraction = 1.0;
  if (!qFuzzyCompare(fraction, m_progress)) {
    m_progress = fraction;
    update(); // schedule a repaint
  }
}

/*----------------------------------------------------------------------------*/
/* Painting */
/*----------------------------------------------------------------------------*/

void ImageProgressWidget::paintEvent(QPaintEvent *) {
  if (m_color.isNull())
    return;

  QPainter p(this);
  p.setRenderHint(QPainter::SmoothPixmapTransform, true);

  // Destination: full widget rect
  QRect dst = rect();

  // Keep aspect ratio
  QPixmap col =
      m_color.scaled(dst.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
  QPixmap gry =
      m_gray.scaled(dst.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

  // Center both
  QPoint topLeft((width() - col.width()) / 2, (height() - col.height()) / 2);
  QRect drawRect(topLeft, col.size());

  // Define starting offset fraction (e.g., 20% in)
  const double startOffset = 0.275;
  // Remap progress [0..1] to [startOffset..1]
  double effFrac = startOffset + m_progress * (1.0 - startOffset);

  // Compute split X based on remapped fraction
  int splitX = drawRect.left() + int(effFrac * drawRect.width());

  // 1) Draw full grayscale
  p.drawPixmap(drawRect, gry);

  // 2) Draw color portion up to splitX
  if (splitX > drawRect.left()) {
    QRect srcLeft(0, 0, int(col.width() * effFrac), col.height());
    QRect dstLeft(drawRect.left(), drawRect.top(), splitX - drawRect.left(),
                  drawRect.height());
    p.drawPixmap(dstLeft, col, srcLeft);
  }
}

QPixmap ImageProgressWidget::toGrayscale(const QPixmap &src) {
  // Convert QPixmap to QImage, then to grayscale and back to QPixmap
  QImage img = src.toImage();
  QImage gray = img.convertToFormat(QImage::Format_Grayscale8);
  return QPixmap::fromImage(gray);
}
