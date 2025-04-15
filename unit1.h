#ifndef UNIT1_H
#define UNIT1_H

#include "ui_mainwindow.h"
#include <QObject>
#include <QTimer>

class Unit1 : public QObject
{
    Q_OBJECT

public:
    explicit Unit1(Ui::MainWindow *ui, QObject *parent = nullptr);

private:
    Ui::MainWindow *ui;
    QTimer *statusTimer;

public slots:
    void onBtnChooseProgramDataFileClicked();
    void onBtnChooseLoaderFileClicked();
    void onBtnCreateFileManualClicked();
    void onBtnCreateFileAutoClicked();
    void onBtnShowInfoClicked();
    void onBtnClearRevisionClicked();
    void onBtnConnectClicked();
    void onBtnUploadClicked();
    void hideConnectionStatus();
};

#endif // UNIT1_H
