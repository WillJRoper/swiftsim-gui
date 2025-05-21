#include "ScaledPixmapLabel.h"
#include <QPaintEvent>
#include <QPainter>

ScaledPixmapLabel::ScaledPixmapLabel(QWidget *parent) : QLabel(parent) {
  // Let layouts expand this to fill available space
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  setAlignment(Qt::AlignCenter);
}

void ScaledPixmapLabel::setPixmapKeepingAspect(const QPixmap &pixmap) {
  m_original = pixmap;
  update(); // schedule a repaint
}

void ScaledPixmapLabel::paintEvent(QPaintEvent *ev) {
  if (m_original.isNull()) {
    // No pixmap—fall back to default label painting (e.g. text)
    QLabel::paintEvent(ev);
    return;
  }

  QPainter painter(this);
  // Scale to this widget’s size, keeping aspect ratio
  QPixmap scaled =
      m_original.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

  // Center the pixmap inside the widget
  QPoint center =
      rect().center() - QPoint(scaled.width() / 2, scaled.height() / 2);
  painter.drawPixmap(center, scaled);
}
