
#include "LogTabWidget.h"
#include <QLabel>
#include <QVBoxLayout>

LogTabWidget::LogTabWidget(QWidget *parent) : QWidget(parent) {
  auto *label = new QLabel(tr("Log"), this);
  label->setAlignment(Qt::AlignCenter);

  auto *layout = new QVBoxLayout(this);
  layout->addWidget(label);
  setLayout(layout);
}
