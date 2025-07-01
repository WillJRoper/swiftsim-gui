#include "PlotWidget.h"
#include <QDebug>
#include <QPixmap>
#include <QVBoxLayout>

PlotWidget::PlotWidget(const QString &scriptPath, const QString &csvPath,
                       const QString &outputPng, QWidget *parent)
    : QWidget(parent), m_scriptPath(scriptPath), m_csvPath(csvPath),
      m_outputPng(outputPng), m_imageLabel(new ScaledPixmapLabel(this)) {

  // Ensure the script and CSV paths are valid
  if (m_scriptPath.isEmpty() || m_csvPath.isEmpty() || m_outputPng.isEmpty()) {
    qCritical("PlotWidget: script, CSV, or output PNG path is empty.");
    return;
  }

  // Set up the image label
  auto *lay = new QVBoxLayout(this);
  lay->addWidget(m_imageLabel);
  setLayout(lay);

  // Initial draw
  refresh(0);
}

void PlotWidget::refresh(int step) {
  // 1) Call the Python script synchronously
  QProcess proc;
  QStringList args{m_scriptPath, m_csvPath, m_outputPng};
  qInfo() << "Running plot script: python" << qPrintable(m_scriptPath)
          << " with args: " << args.join(" ");
  proc.start("python", args);
  if (!proc.waitForFinished(30000)) { // wait up to 30s
    qWarning("Plot process timed out.");
    return;
  }
  if (proc.exitCode() != 0) {
    qWarning("Plot process error: %s", proc.readAllStandardError().constData());
    return;
  }

  // 2) Load and display the PNG
  QPixmap pix(m_outputPng);
  if (pix.isNull()) {
    qWarning("Failed to load plot image '%s'.", qPrintable(m_outputPng));
    return;
  }
  m_imageLabel->setPixmapKeepingAspect(pix);
}
