#pragma once

#include <QPixmap>
#include <QWidget>

class QLabel;

class HomeTabWidget : public QWidget {
  Q_OBJECT
public:
  explicit HomeTabWidget(QWidget *parent = nullptr);

protected:
  void resizeEvent(QResizeEvent *event) override;

private:
  QPixmap m_logoPixmap; // original pixmap
  QLabel *m_logoLabel;  // will show the scaled pixmap
};
