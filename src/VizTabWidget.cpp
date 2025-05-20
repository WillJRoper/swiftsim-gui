
#include "VizTabWidget.h"
#include <QLabel>
#include <QVBoxLayout>

VizTabWidget::VizTabWidget(QWidget *parent) : QWidget(parent) {
  auto *label = new QLabel(tr("Visualise"), this);
  label->setAlignment(Qt::AlignCenter);

  auto *layout = new QVBoxLayout(this);
  layout->addWidget(label);
  setLayout(layout);
}
