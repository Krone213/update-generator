    #ifndef UNIT1_H
    #define UNIT1_H

    #include "ui_mainwindow.h" // Доступ к UI
    #include <QObject>
    #include <QTimer>
    #include <QDomDocument>     // Для загрузки XML
    #include <QMap>             // Для revisionsMap
    #include <QSet>             // Для autoSavePaths
    #include <QMessageBox>      // Для сообщений
    #include <QProcess>
    #include <QFileDialog>      // <--- Добавлено для диалога выбора файла
    #include <QDir>             // <--- Добавлено для работы с путями
    #include <QStringList>
    #include <QTemporaryFile>

    // --- Оригинальная структура для Unit1 ---
    struct RevisionInfo {
        QString bootloaderFile;
        QString mainProgramFile;
        QString SaveFirmware; // Путь автосохранения для этой ревизии
    };

    class Unit1 : public QObject
    {
        Q_OBJECT

    public:
        explicit Unit1(Ui::MainWindow *ui, QObject *parent = nullptr);
        ~Unit1();

    private:
        Ui::MainWindow *ui;
        QTimer *statusTimer;

        // --- Члены класса из оригинала ---
        QMap<QString, RevisionInfo> revisionsMap; // Карта для хранения данных ревизий Unit1
        QSet<QString> autoSavePaths;           // Множество всех путей автосохранения из <SaveFirmware>
        QString SaveFirmware;                  // Текущий путь автосохранения для выбранной ревизии

        // --- Приватные методы из оригинала ---
        void loadConfig(const QString &filePath); // Загрузка конфига для Unit1
        QByteArray loadFile(const QString &filePath);
        void generateCRCTables(quint32* fwdTable, quint32* revTable);
        // Вернем calculateReverseCRC, т.к. он был в вашем первом коде
        quint32 calculateReverseCRC(const QByteArray &data_with_placeholders, quint32 targetCRC, const quint32* revTable);
        void updateLoaderFileSize(const QString &filePath);
        void updateProgramDataFileSize(const QString &filePath);
        void updateTotalFirmwareSize();
        void createFirmwareFiles(const QString &outputDir); // Использует внутреннее состояние

        // Функция подключения ST-Link

        QString cliSubfolder = "ST-LINK";
        QString cliExecutable = "ST-LINK_CLI.exe";

        #ifdef Q_OS_WIN
        // Для Windows путь собирается просто
        QString m_stLinkCliPath = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + QDir::separator() + cliSubfolder + QDir::separator() + cliExecutable);
        #else
        QString m_stLinkCliPath = QCoreApplication::applicationDirPath() + QDir::separator() + cliSubfolder + QDir::separator() + cliExecutable;
        #endif

        QProcess *m_stLinkProcess = nullptr;
        QString m_firmwareFilePath; // <--- Добавлено: путь к выбранной прошивке
        QStringList m_currentCommandArgs;

        // --- Для повторных попыток подключения ---
        QTimer *m_retryTimer = nullptr;         // Таймер задержки между попытками
        const int m_maxProgramAttempts = 3;     // Максимальное количество попыток
        int m_programAttemptsLeft = 0;          // Счетчик оставшихся попыток

        void executeProgramAttempt();


    public slots:
        // --- Слоты из оригинала ---
        void onBtnChooseProgramDataFileClicked();
        void onBtnChooseLoaderFileClicked();
        void onBtnShowInfoClicked();
        void onBtnClearRevisionClicked(); // Обрабатывается внутри Unit1
        void onBtnConnectAndUploadClicked();
        void hideConnectionStatus();
        void onBtnCreateFileManualClicked();
        void onBtnCreateFileAutoClicked(); // Обрабатывается внутри Unit1
        void onRevisionChanged(int index);

    private slots:
        void handleStLinkFinished(int exitCode, QProcess::ExitStatus exitStatus);
        void handleStLinkError(QProcess::ProcessError error);
        void handleStLinkStdOut();
        void handleStLinkStdErr();

        void retryProgramAttempt();
        void cleanupTemporaryFile();
    };

    #endif // UNIT1_H
