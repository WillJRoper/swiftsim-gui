#pragma once

#include <QLabel>
#include <QPixmap>

class ScaledPixmapLabel : public QLabel {
  Q_OBJECT

public:
  explicit ScaledPixmapLabel(QWidget *parent = nullptr);

  // Call this to set the image; it will be repainted at the right size
  void setPixmapKeepingAspect(const QPixmap &pixmap);

protected:
  // Override paintEvent to draw the scaled pixmap centered
  void paintEvent(QPaintEvent *ev) override;

private:
  QPixmap m_original;
};
