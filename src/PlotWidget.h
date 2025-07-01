// PlotWidget.h
#pragma once

#include "ScaledPixmapLabel.h"

#include <QLabel>
#include <QProcess>
#include <QWidget>

class PlotWidget : public QWidget {
  Q_OBJECT
public:
  explicit PlotWidget(const QString &scriptPath, const QString &csvPath,
                      const QString &outputPng, QWidget *parent = nullptr);

  /** Re-run the Python script and update the display. */
  void refresh(int step);

private:
  QString m_scriptPath;
  QString m_csvPath;
  QString m_outputPng;
  ScaledPixmapLabel *m_imageLabel;
};
