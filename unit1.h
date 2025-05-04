#ifndef UNIT1_H
#define UNIT1_H

#include "ui_mainwindow.h"

#include <QObject>
#include <QString>
#include <QTimer>
#include <QDomDocument>
#include <QMap>
#include <QSet>
#include <QProcess>
#include <QDir>
#include <QStringList>
#include <QTemporaryFile>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>
#include <QtEndian>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QDateTime>

#include <cstring>

    struct RevisionInfo {
        QString bootloaderFile;
        QString mainProgramFile;
        QString SaveFirmware;
    };

    class Unit1 : public QObject
    {
        Q_OBJECT

    signals:
        void logToInterface(const QString &message, bool isError);

    public:
        explicit Unit1(Ui::MainWindow *ui, QObject *parent = nullptr);
        ~Unit1();

    private:
        Ui::MainWindow *ui;
        QTimer *statusTimer;

        QTimer *m_animationTimer;
        int m_animationFrame;

        // --- Члены класса из оригинала ---
        QMap<QString, RevisionInfo> revisionsMap; // Карта для хранения данных ревизий Unit1
        QSet<QString> autoSavePaths;  // Множество всех путей автосохранения из <SaveFirmware>
        QString SaveFirmware; // Текущий путь автосохранения для выбранной ревизии

        // --- Приватные методы из оригинала ---
        void loadConfig(const QString &filePath);
        QByteArray loadFile(const QString &filePath);
        void generateCRCTables(quint32* fwdTable, quint32* revTable);

        // Вернем calculateReverseCRC, т.к. он был в вашем первом коде
        quint32 calculateReverseCRC(const QByteArray &data_with_placeholders, quint32 targetCRC, const quint32* revTable);
        void updateLoaderFileSize(const QString &filePath);
        void updateProgramDataFileSize(const QString &filePath);
        void updateTotalFirmwareSize();
        void createFirmwareFiles(const QString &outputDir);

        // Функция подключения ST-Link
        QString cliSubfolder = "ST-LINK";
        QString cliExecutable = "ST-LINK_CLI.exe";

        #ifdef Q_OS_WIN
        QString m_stLinkCliPath = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + QDir::separator() + cliSubfolder + QDir::separator() + cliExecutable);
        #else
        QString m_stLinkCliPath = QCoreApplication::applicationDirPath() + QDir::separator() + cliSubfolder + QDir::separator() + cliExecutable;
        #endif

        QProcess *m_stLinkProcess = nullptr;
        QString m_firmwareFilePath; // Путь к выбранной прошивке
        QStringList m_currentCommandArgs;

        QTimer *m_retryTimer = nullptr;
        const int m_maxProgramAttempts = 3;
        int m_programAttemptsLeft = 0;

        void executeProgramAttempt();


    public slots:
        void onBtnChooseProgramDataFileClicked();
        void onBtnChooseLoaderFileClicked();
        void onBtnShowInfoClicked();
        void onBtnClearRevisionClicked();
        void onBtnConnectAndUploadClicked();
        void hideConnectionStatus();
        void onBtnCreateFileManualClicked();
        void onBtnCreateFileAutoClicked();
        void onRevisionChanged(int index);

    private slots:
        void handleStLinkFinished(int exitCode, QProcess::ExitStatus exitStatus);
        void handleStLinkError(QProcess::ProcessError error);
        void handleStLinkStdOut();
        void handleStLinkStdErr();
        void updateLoadingAnimation();

        void retryProgramAttempt();
        void cleanupTemporaryFile();
    };

    #endif // UNIT1_H
