#pragma once

#include <QPixmap>
#include <QWidget>
class ScaledPixmapLabel;

class HomeTabWidget : public QWidget {
  Q_OBJECT
public:
  explicit HomeTabWidget(QWidget *parent = nullptr);

private:
  ScaledPixmapLabel *m_logoLabel;
};
