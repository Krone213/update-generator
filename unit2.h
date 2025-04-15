#ifndef UNIT2_H
#define UNIT2_H

#include "ui_mainwindow.h"
#include <QObject>

class Unit2 : public QObject
{
    Q_OBJECT

public:
    explicit Unit2(Ui::MainWindow *ui, QObject *parent = nullptr);

private:
    Ui::MainWindow *ui;

public slots:
    void onBtnChooseUpdateProgramDataFileClicked();
    void onBtnClearUpdateProgramDataFileClicked();
    void onBtnUpdateCreateFileAutoClicked();
    void onBtnUpdateCreateFileManualClicked();
    void onBtnUpdateShowInfoClicked();
    void onBtnClearUpdateRevisionClicked();
    void onBtnCreateCommonUpdateFileClicked();
};

#endif // UNIT2_H
