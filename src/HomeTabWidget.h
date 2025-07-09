// HomeTabWidget.h
#pragma once

#include <QLabel>
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
  QLabel *m_creditsLabel;
  QPixmap m_flamingoOrig; // original flamingo logo
  QLabel *m_flamingoLabel;
  QPixmap m_sussexOrig;  // original Sussex logo
  QLabel *m_sussexLabel; // label for Sussex m_logoLabel

  int m_flamingoXMargin = 15;
  int m_flamingoYMargin = 15;
  int m_sussexXMargin = 15;
  int m_sussexYMargin = 15;

  double m_flamingoSizePercent = 17.0;
  double m_sussexSizePercent = 15;
};
