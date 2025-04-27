    #ifndef UNIT1_H
    #define UNIT1_H

    #include "ui_mainwindow.h" // Доступ к UI
    #include <QObject>
    #include <QTimer>
    #include <QDomDocument>     // Для загрузки XML
    #include <QMap>             // Для revisionsMap
    #include <QSet>             // Для autoSavePaths
    #include <QMessageBox>      // Для сообщений

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


    public slots:
        // --- Слоты из оригинала ---
        void onBtnChooseProgramDataFileClicked();
        void onBtnChooseLoaderFileClicked();
        void onBtnShowInfoClicked();
        void onBtnClearRevisionClicked(); // Обрабатывается внутри Unit1
        void onBtnConnectClicked();
        void onBtnUploadClicked();
        void hideConnectionStatus();
        void onBtnCreateFileManualClicked();
        void onBtnCreateFileAutoClicked(); // Обрабатывается внутри Unit1
        void onRevisionChanged(int index);

    private slots:
        // --- Слот из оригинала ---


    };

    #endif // UNIT1_H
