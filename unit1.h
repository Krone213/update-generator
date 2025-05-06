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
#include <QTcpSocket>
#include <QHostAddress>
#include <QStandardPaths>

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

        bool m_shutdownCommandSent;

        // Функция подключения ST-Link
        /*QString cliSubfolder = "ST-LINK";
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

        void executeProgramAttempt();*/

        // --- OpenOCD Integration ---
        QString m_openOcdDir;                 // Base directory for OpenOCD relative to app
        QString m_openOcdExecutablePath;      // Full path to openocd.exe
        QString m_openOcdScriptsPath;         // Full path to the scripts directory
        QString m_interfaceScript = "interface/stlink.cfg"; // Fixed for ST-Link v2
        // Target script will be selected via UI (cmbTargetMCU)
        QProcess *m_openOcdProcess = nullptr;
        QTcpSocket *m_telnetSocket = nullptr;
        QString m_openOcdHost = "localhost";
        quint16 m_openOcdTelnetPort = 4444;
        bool m_isOpenOcdRunning = false;      // Is OpenOCD process supposed to be running?
        bool m_isConnected = false;           // Is Telnet socket connected?
        bool m_isConnecting = false;          // Is connection in progress?
        bool m_isProgramming = false;         // Is programming in progress?
        QString m_firmwareFilePathForUpload;
        QString m_originalFirmwarePathForLog;
        QString m_firmwareAddress = "0x08000000";
        QByteArray m_receivedTelnetData;


    public slots:
        void onBtnChooseProgramDataFileClicked();
        void onBtnChooseLoaderFileClicked();
        void onBtnShowInfoClicked();
        void onBtnClearRevisionClicked();
        //void onBtnConnectAndUploadClicked();
        void hideConnectionStatus();
        void onBtnCreateFileManualClicked();
        void onBtnCreateFileAutoClicked();
        void onBtnConnectClicked();
        void onBtnUploadClicked();

    private slots:

        // --- OpenOCD Process Handlers ---
        void handleOpenOcdStarted();
        void handleOpenOcdFinished(int exitCode, QProcess::ExitStatus exitStatus);
        void handleOpenOcdError(QProcess::ProcessError error);
        void handleOpenOcdStdOut();
        void handleOpenOcdStdErr();
        // --- Telnet Socket Handlers ---
        void handleTelnetConnected();
        void handleTelnetDisconnected();
        void handleTelnetReadyRead();
        void handleTelnetError(QAbstractSocket::SocketError socketError);

       /* void handleStLinkFinished(int exitCode, QProcess::ExitStatus exitStatus);
        void handleStLinkError(QProcess::ProcessError error);
        void handleStLinkStdOut();
        void handleStLinkStdErr();*/

        void updateLoadingAnimation();
        void sendOpenOcdCommand(const QString &command);
        void processTelnetBuffer();
        void stopOpenOcd();
        void cleanupTemporaryFile();
        bool checkOpenOcdPrerequisites();
        void onRevisionChanged(int index);
    };

    #endif // UNIT1_H
