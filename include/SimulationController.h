
#pragma once
#include <QObject>
#include <QStringList>

class SimulationController : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString logText READ logText NOTIFY logTextChanged)
  Q_PROPERTY(QStringList visualizationFiles READ visualizationFiles NOTIFY
                 visualsChanged)
public:
  explicit SimulationController(QObject *parent = nullptr);

  Q_INVOKABLE void startLogWatcher(const QString &path, int maxLines = 200);
  QString logText() const;
  QStringList visualizationFiles() const;

signals:
  void logTextChanged();
  void visualsChanged();

private:
  // …
};
