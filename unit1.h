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
#include <QQueue>

#include <cstring>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

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
    bool wasShutdownCommandSent() const { return m_shutdownCommandSent; }
    ~Unit1();

private:
    Ui::MainWindow *ui;
    QTimer *statusTimer;

    QTimer *m_animationTimer;
    int m_animationFrame;

    QMap<QString, RevisionInfo> revisionsMap; // Карта для хранения данных ревизий Unit1
    QSet<QString> autoSavePaths;  // Множество всех путей автосохранения из <SaveFirmware>
    QString SaveFirmware; // Текущий путь автосохранения для выбранной ревизии

    void loadConfig(const QString &filePath);
    QByteArray loadFile(const QString &filePath);
    void generateCRCTables(quint32* fwdTable, quint32* revTable);

    // Вернем calculateReverseCRC, т.к. он был в вашем первом коде
    quint32 calculateReverseCRC(const QByteArray &data_with_placeholders, quint32 targetCRC, const quint32* revTable);
    void updateLoaderFileSize(const QString &filePath);
    void updateProgramDataFileSize(const QString &filePath);
    void updateTotalFirmwareSize();
    void createFirmwareFiles(const QString &outputDir);

    // Автодетектор OpenOCD
    bool m_isAttemptingAutoDetect;
    QString m_detectedTargetScript;
    QTimer* m_autoDetectTimeoutTimer;
    void startOpenOcdForAutoDetect();
    void processOpenOcdOutputForDetection(const QString& output);
    void proceedWithConnection(const QString& targetScript);

    // Пути OpenOCD
    QString m_openOcdDir;
    QString m_openOcdExecutablePath;
    QString m_openOcdScriptsPath;
    QString m_interfaceScript = "interface/stlink.cfg";

    // ---------- Управление OpenOCD
    QProcess *m_openOcdProcess = nullptr;
    QTcpSocket *m_telnetSocket = nullptr;
    QString m_openOcdHost = "localhost";
    quint16 m_openOcdTelnetPort = 4444;
    // Флаги состояния OpenOCD
    bool m_isOpenOcdRunning = false;
    bool m_isConnected = false;
    bool m_isConnecting = false;
    bool m_isProgramming = false;
    bool m_shutdownCommandSent;
    // Логика загрузки прошивки
    QString m_firmwareFilePathForUpload;
    QString m_originalFirmwarePathForLog;
    QString m_currentSafeTempSubdirPath;
    QString m_firmwareAddress = "0x08000000";
    QByteArray m_receivedTelnetData;

    static const QString TEMP_SUBDIR_NAME_OPENOCD;

    // ---------- Управление безопасными путями
#ifdef Q_OS_WIN
    QString winGetShortPathName(const QString& longPath);
#endif
    QString getSafeTemporaryDirectoryBasePath();
    bool ensureDirectoryExists(const QString& path);

public slots:
    void onBtnChooseProgramDataFileClicked();
    void onBtnChooseLoaderFileClicked();
    void onBtnShowInfoClicked();
    void onBtnClearRevisionClicked();
    void onBtnCreateFileManualClicked();
    void onBtnCreateFileAutoClicked();
    void hideConnectionStatus();

    void onBtnConnectClicked();
    void onbtnUploadCPU1Clicked();
    void onbtnUploadCPU2Clicked();
    void onBtnEraseChipClicked();
    void stopOpenOcd();

private slots:
    void onRevisionChanged(int index);

    void handleOpenOcdStarted();
    void handleOpenOcdFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleOpenOcdError(QProcess::ProcessError error);
    void handleOpenOcdStdOut();
    void handleOpenOcdStdErr();
    void handleTelnetConnected();
    void handleTelnetDisconnected();
    void handleTelnetReadyRead();
    void handleTelnetError(QAbstractSocket::SocketError socketError);

    bool checkOpenOcdPrerequisites(const QString& targetScriptPath);
    void sendOpenOcdCommand(const QString &command);
    void processTelnetBuffer();
    void cleanupTemporaryFile();
    void updateLoadingAnimation();
    void updateUploadButtonsState();
};

#endif // UNIT1_H
