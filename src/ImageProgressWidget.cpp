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

  // Destination rectangle: fill the widget
  QRect dst = rect();

  // Keep aspect ratio: compute a scaled pixmap
  QPixmap col =
      m_color.scaled(dst.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
  QPixmap gry =
      m_gray.scaled(dst.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

  // Center both in the widget
  QPoint topLeft((width() - col.width()) / 2, (height() - col.height()) / 2);
  QRect drawRect(topLeft, col.size());

  // Compute split line in widget coords
  int splitX = drawRect.left() + int(m_progress * drawRect.width());

  // 1) Draw grayscale full image
  p.drawPixmap(drawRect, gry);

  // 2) Draw colored left portion
  if (splitX > drawRect.left()) {
    QRect srcLeft(0, 0, int(col.width() * m_progress), col.height());
    QRect dstLeft(drawRect.left(), drawRect.top(), splitX - drawRect.left(),
                  drawRect.height());
    p.drawPixmap(dstLeft, col, srcLeft);
  }
}
