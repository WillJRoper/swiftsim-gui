#pragma once

#include <QCursor>
#include <QSplitter>
#include <QSplitterHandle>
#include <QStyle>

class StyledSplitter : public QSplitter {
public:
  using QSplitter::QSplitter;

protected:
  QSplitterHandle *createHandle() override {
    auto *handle = new QSplitterHandle(orientation(), this);
    handle->setCursor(Qt::SplitVCursor); // vertical resize cursor
    return handle;
  }
};
