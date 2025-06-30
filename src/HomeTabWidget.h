// HomeTabWidget.h
#pragma once

#include <QPixmap>
#include <QWidget>

class ScaledPixmapLabel;

class HomeTabWidget : public QWidget {
  Q_OBJECT
public:
  explicit HomeTabWidget(QWidget *parent = nullptr);

protected:
  void resizeEvent(QResizeEvent *event) override;
  void paintEvent(QPaintEvent *event) override;

private:
  ScaledPixmapLabel *m_logoLabel;
  QPixmap m_background;       // original square
  QPixmap m_scaledBackground; // covers the widget
};
