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
  // Flag that we are generating a new plot, skip if already generating
  if (m_isGenerating) {
    qWarning() << "Plot generation already in progress, skipping refresh.";
    return;
  }
  m_isGenerating = true;

  // 1) Kick off the Python script *asynchronously*
  QProcess *proc = new QProcess(this);

  // Clean up when done
  proc->setParent(this);

  // Connect the finished signal
  connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          this, &PlotWidget::onPlotProcessFinished);

  QStringList args{m_scriptPath, m_csvPath, m_outputPng};
  proc->start("python", args);
}

void PlotWidget::onPlotProcessFinished(int exitCode,
                                       QProcess::ExitStatus status) {
  // Sender is our QProcess*
  QProcess *proc = qobject_cast<QProcess *>(sender());
  if (!proc)
    return;

  if (exitCode != 0 || status != QProcess::NormalExit) {
    qWarning() << "Plot process error:" << proc->readAllStandardError();
    proc->deleteLater();
    return;
  }

  // 2) Load and display the PNG
  QPixmap pix(m_outputPng);
  if (pix.isNull()) {
    qWarning() << "Failed to load plot image" << m_outputPng;
  } else {
    m_imageLabel->setPixmapKeepingAspect(pix);
  }

  proc->deleteLater();

  m_isGenerating = false; // Reset the flag
}
