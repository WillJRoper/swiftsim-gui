#include "PlotWidget.h"
#include <QDebug>
#include <QVBoxLayout>

PlotWidget::PlotWidget(const QString &scriptPath, const QString &csvPath,
                       const QString &outputPng, QWidget *parent)
    : QWidget(parent), m_scriptPath(scriptPath), m_csvPath(csvPath),
      m_outputPng(outputPng), m_imageLabel(new ScaledPixmapLabel(this)),
      m_debounceTimer(new QTimer(this)) {

  // Validate inputs once
  if (m_scriptPath.isEmpty() || m_csvPath.isEmpty() || m_outputPng.isEmpty()) {
    qCritical("PlotWidget: script, CSV, or output PNG path is empty.");
    return;
  }

  // Lay out the image label
  auto *lay = new QVBoxLayout(this);
  lay->setContentsMargins(0, 0, 0, 0);
  lay->addWidget(m_imageLabel);
  setLayout(lay);

  // Configure debounce timer
  m_debounceTimer->setSingleShot(true);
  connect(m_debounceTimer, &QTimer::timeout, this,
          &PlotWidget::onDebounceTimeout);

  // Initial draw at step 0 (debounced)
  refresh(0);
}

void PlotWidget::refresh(int step) {
  // Remember the latest request
  m_pendingStep = step;

  // If a process is running, queue for immediately after it finishes
  if (m_proc) {
    m_queued = true;
    return;
  }

  // (Re)start debounce timer
  m_debounceTimer->start(5000); // 5 s debounce interval
}

void PlotWidget::onDebounceTimeout() {
  // If already running, queue and return
  if (m_proc) {
    m_queued = true;
    return;
  }
  // Launch the plot process
  startPlotProcess(m_pendingStep);
}

void PlotWidget::startPlotProcess(int step) {
  // Spawn new QProcess
  m_proc = new QProcess(this);
  // Clean up when done
  connect(m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          this, &PlotWidget::onPlotProcessFinished);

  // Kick off Python script
  QStringList args{m_scriptPath, m_csvPath, m_outputPng};
  // (If your script takes step as a CLI arg; drop the last if not.)
  m_proc->start("python", args);
}

void PlotWidget::onPlotProcessFinished(int exitCode,
                                       QProcess::ExitStatus status) {
  // Ensure we have our QProcess
  QProcess *proc = m_proc;
  m_proc = nullptr;

  if (exitCode != 0 || status != QProcess::NormalExit) {
    qWarning() << "Plot process error:" << proc->readAllStandardError();
    proc->deleteLater();
  } else {
    // Load and display the new PNG
    QPixmap pix(m_outputPng);
    if (pix.isNull()) {
      qWarning() << "Failed to load plot image" << m_outputPng;
    } else {
      m_imageLabel->setPixmapKeepingAspect(pix);
    }
    proc->deleteLater();
  }

  // If a new request came in while we were busy, immediately trigger it
  if (m_queued) {
    m_queued = false;
    m_debounceTimer->start(0);
  }
}
