#include "main_window.hpp"
#include "occt_viewport.hpp"
#include <QLabel>
#include <QStatusBar>
MainWindow::MainWindow(QWidget* parent):QMainWindow(parent),selection_(new QLabel("Selected topology: None",this)) { auto* viewport=new OcctViewport(this); setCentralWidget(viewport); statusBar()->addPermanentWidget(selection_); connect(viewport,&OcctViewport::topologySelected,this,[this](const QString& type){selection_->setText("Selected topology: "+type);}); setWindowTitle("Duomec Platform — Milestone 0"); resize(1100,750); }
