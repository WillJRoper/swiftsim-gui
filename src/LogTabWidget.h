#pragma once

#include <QString>
#include <QWidget>

class QPlainTextEdit;

class LogTabWidget : public QWidget {
  Q_OBJECT
public:
  explicit LogTabWidget(QWidget *parent = nullptr);

  // Call this when the user picks a new font size
  void setFontSize(int pointSize);

  // Set the file path to read from
  void setFilePath(const QString &filePath);

  // Call this to update the view
  void updateLogView();

protected:
  void resizeEvent(QResizeEvent *event) override;

signals:
  void currentTimeChanged(double t);

private:
  QPlainTextEdit *m_textEdit;
  QString m_filePath;
  QFont m_font;

  double parseTimeFromLine(const QString &line);
};
