#ifndef UNIT2_H
#define UNIT2_H

#include "ui_mainwindow.h"
#include "config_data.h"

#include <QObject>
#include <QDebug>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QComboBox>
#include <QDir>
#include <QSaveFile>
#include <QDataStream>
#include <QtEndian>
#include <QDateTime>
#include <QStringList>
#include <QRegularExpression>
#include <QtGlobal>

#include <cstring>

class Unit2 : public QObject
{
    Q_OBJECT

public:
    explicit Unit2(Ui::MainWindow *ui, QObject *parent = nullptr);

    void updateUI(const ExtendedRevisionInfo& info);
    void clearUIFields();
    void createUpdateFiles(const QString &outputDirAbsPath);

private:
    Ui::MainWindow *ui;

    void updateUpdateProgramDataFileSize(const QString &filePath);


public slots:
    void onBtnChooseUpdateProgramDataFileClicked();
    void onBtnUpdateCreateFileManualClicked();
    void onBtnUpdateShowInfoClicked();
    void onBtnClearUpdateRevisionClicked();
};

#endif // UNIT2_H
