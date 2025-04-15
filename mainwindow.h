#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QScreen>
#include <QApplication>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class Unit1;
class Unit2;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onExpertModeToggled(bool checked);

private:
    Ui::MainWindow *ui;
    QSize expertSize;  // Размер окна в экспертном режиме
    QSize simpleSize;  // Размер окна в простом режиме

    Unit1 *unit1;  // Для логики StartFirmwareMenu
    Unit2 *unit2;  // Для логики UpdateFirmwareMenu
};

#endif // MAINWINDOW_H
