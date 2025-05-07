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
    bool wasShutdownCommandSent() const { return m_shutdownCommandSent; }
    ~Unit1();

private:
    Ui::MainWindow *ui;
    QTimer *statusTimer;

    QTimer *m_animationTimer;
    int m_animationFrame;

    // Члены класса из оригинала ---
    QMap<QString, RevisionInfo> revisionsMap; // Карта для хранения данных ревизий Unit1
    QSet<QString> autoSavePaths;  // Множество всех путей автосохранения из <SaveFirmware>
    QString SaveFirmware; // Текущий путь автосохранения для выбранной ревизии

    // Приватные методы из оригинала ---
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


    QString m_openOcdDir;
    QString m_openOcdExecutablePath;
    QString m_openOcdScriptsPath;
    QString m_interfaceScript = "interface/stlink.cfg";

    QProcess *m_openOcdProcess = nullptr;
    QTcpSocket *m_telnetSocket = nullptr;
    QString m_openOcdHost = "localhost";
    quint16 m_openOcdTelnetPort = 4444;
    bool m_isOpenOcdRunning = false;
    bool m_isConnected = false;
    bool m_isConnecting = false;
    bool m_isProgramming = false;
    QString m_firmwareFilePathForUpload;
    QString m_originalFirmwarePathForLog;
    QString m_firmwareAddress = "0x08000000";
    QByteArray m_receivedTelnetData;

public slots:
    void onBtnChooseProgramDataFileClicked();
    void onBtnChooseLoaderFileClicked();
    void onBtnShowInfoClicked();
    void onBtnClearRevisionClicked();
    void hideConnectionStatus();
    void onBtnCreateFileManualClicked();
    void onBtnCreateFileAutoClicked();
    void onBtnConnectClicked();
    void onBtnUploadClicked();
    void stopOpenOcd();

private slots:
    void handleOpenOcdStarted();
    void handleOpenOcdFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleOpenOcdError(QProcess::ProcessError error);
    void handleOpenOcdStdOut();
    void handleOpenOcdStdErr();
    void handleTelnetConnected();
    void handleTelnetDisconnected();
    void handleTelnetReadyRead();
    void handleTelnetError(QAbstractSocket::SocketError socketError);

    void updateLoadingAnimation();
    void sendOpenOcdCommand(const QString &command);
    void processTelnetBuffer();
    void cleanupTemporaryFile();
    bool checkOpenOcdPrerequisites();
    void onRevisionChanged(int index);
};

#endif // UNIT1_H
