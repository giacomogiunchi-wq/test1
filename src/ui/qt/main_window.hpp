#pragma once
#include <QMainWindow>
class QLabel;
class MainWindow final : public QMainWindow { Q_OBJECT public: explicit MainWindow(QWidget* parent=nullptr); private: QLabel* selection_; };
