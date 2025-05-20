#pragma once

// main.cpp
#include <QAction>
#include <QApplication>
#include <QFont>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

// Forward-declare your custom page widgets:
#include "DiagTabWidget.h"
#include "HomeTabWidget.h"
#include "LogTabWidget.h"
#include "SimulationController.h"
#include "VizTabWidget.h"

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  MainWindow(SimulationController *simCtrl, QWidget *parent = nullptr)
      : QMainWindow(parent) {
    // // 1) Menus
    // auto *fileMenu = menuBar()->addMenu(tr("&File"));
    // fileMenu->addAction(tr("&New Simulation"), simCtrl,
    //                     &SimulationController::newSimulation);
    // fileMenu->addAction(tr("&Open Simulation…"), simCtrl,
    //                     &SimulationController::openSimulation);
    //
    // auto *swiftMenu = menuBar()->addMenu(tr("&Swiftsim"));
    // swiftMenu->addAction(tr("&Configure…"), simCtrl,
    //                      &SimulationController::configure);
    // swiftMenu->addAction(tr("&Compile"), simCtrl,
    //                      &SimulationController::compile);
    // swiftMenu->addAction(tr("&Dry Run"), simCtrl,
    //                      &SimulationController::dryRun);
    // swiftMenu->addAction(tr("&Run"), simCtrl, &SimulationController::run);

    // 2) Central widget with tabs
    auto *tabs = new QTabWidget;
    tabs->addTab(new HomeTabWidget(), tr("Home"));
    tabs->addTab(new LogTabWidget(), tr("Log"));
    tabs->addTab(new VizTabWidget(), tr("Visualise"));
    tabs->addTab(new DiagTabWidget(), tr("Diagnostics"));

    // 3) Font and theme (dark + red accent)
    QFont mono("Hack Nerd Font Mono", 12);
    QApplication::setFont(mono);

    // Optional: dark palette + red accent
    QPalette dark;
    dark.setColor(QPalette::Window, QColor("#121212"));
    dark.setColor(QPalette::WindowText, Qt::white);
    dark.setColor(QPalette::Highlight, QColor("#E50000")); // HAL red
    dark.setColor(QPalette::HighlightedText, Qt::black);
    QApplication::setPalette(dark);

    // 4) Layout
    auto *container = new QWidget;
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(tabs);
    setCentralWidget(container);

    resize(800, 600);
  }
};
