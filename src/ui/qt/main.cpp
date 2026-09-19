#include "duomec/core/log.hpp"
#include "main_window.hpp"
#include <QApplication>
int main(int argc,char** argv) { QApplication app(argc,argv); duomec::core::log(duomec::core::LogLevel::info,"Starting Duomec Platform"); MainWindow window; window.show(); return app.exec(); }
