
#include "DiagTabWidget.h"
#include <QLabel>
#include <QVBoxLayout>

DiagTabWidget::DiagTabWidget(QWidget *parent) : QWidget(parent) {
  auto *label = new QLabel(tr("Diagnostics"), this);
  label->setAlignment(Qt::AlignCenter);

  auto *layout = new QVBoxLayout(this);
  layout->addWidget(label);
  setLayout(layout);
}
