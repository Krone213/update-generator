#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "unit1.h"
#include "unit2.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Настройки окна
    setFixedSize(size());
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);

    // Получаем основной экран и его размеры
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int screenWidth = screenGeometry.width();
    int screenHeight = screenGeometry.height();

    // Получаем размеры окна
    int windowWidth = this->width();
    int windowHeight = this->height();

    // Вычисляем координаты для положения окна
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2 - 50;

    this->move(x, y);

    // Инициализация тумблера Простой/Экспертный режим
    QCheckBox *expertModeCheckbox = new QCheckBox("Экспертный режим", ui->tabWidget);
    ui->tabWidget->setCornerWidget(expertModeCheckbox, Qt::TopRightCorner);
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int /*index*/) {
        QTimer::singleShot(0, this, [this]() {
            QWidget *cornerWidget = ui->tabWidget->cornerWidget(Qt::TopRightCorner);
            if (cornerWidget) {
                QPoint pos = cornerWidget->pos();
                cornerWidget->move(pos.x(), pos.y() - 8);  // сдвиг на 2 пикселя вверх
            }
        });
    });

    // Подключение сигнала toggled к слоту
    connect(expertModeCheckbox, &QCheckBox::toggled, this, &MainWindow::onExpertModeToggled);

    // Установка размеров окна
    expertSize = size();             // Текущий размер окна для экспертного режима
    simpleSize = QSize(800, 360);    // Уменьшенный размер для простого режима

    // Инициализация Unit1 и Unit2
    unit1 = new Unit1(ui, this);
    unit2 = new Unit2(ui, this);

    // Подключение сигналов кнопок к слотам Unit1 (StartFirmwareMenu)
    connect(ui->btnChooseProgramDataFile, &QPushButton::clicked, unit1, &Unit1::onBtnChooseProgramDataFileClicked);
    connect(ui->btnChooseLoaderFile, &QPushButton::clicked, unit1, &Unit1::onBtnChooseLoaderFileClicked);
    connect(ui->btnCreateFileManual, &QPushButton::clicked, unit1, &Unit1::onBtnCreateFileManualClicked);
    connect(ui->btnCreateFileAuto, &QPushButton::clicked, unit1, &Unit1::onBtnCreateFileAutoClicked);
    connect(ui->btnShowInfo, &QPushButton::clicked, unit1, &Unit1::onBtnShowInfoClicked);
    connect(ui->btnClearRevision, &QPushButton::clicked, unit1, &Unit1::onBtnClearRevisionClicked);
    connect(ui->btnConnect, &QPushButton::clicked, unit1, &Unit1::onBtnConnectClicked);
    connect(ui->btnUpload, &QPushButton::clicked, unit1, &Unit1::onBtnUploadClicked);

    // Подключение сигналов кнопок к слотам Unit2 (UpdateFirmwareMenu)
    connect(ui->btnChooseUpdateProgramDataFile, &QPushButton::clicked, unit2, &Unit2::onBtnChooseUpdateProgramDataFileClicked);
    connect(ui->btnClearUpdateProgramDataFile, &QPushButton::clicked, unit2, &Unit2::onBtnClearUpdateProgramDataFileClicked);
    connect(ui->btnUpdateCreateFileAuto, &QPushButton::clicked, unit2, &Unit2::onBtnUpdateCreateFileAutoClicked);
    connect(ui->btnUpdateCreateFileManual, &QPushButton::clicked, unit2, &Unit2::onBtnUpdateCreateFileManualClicked);
    connect(ui->btnUpdateShowInfo, &QPushButton::clicked, unit2, &Unit2::onBtnUpdateShowInfoClicked);
    connect(ui->btnClearUpdateRevision, &QPushButton::clicked, unit2, &Unit2::onBtnClearUpdateRevisionClicked);
    connect(ui->btnCreateCommonUpdateFile, &QPushButton::clicked, unit2, &Unit2::onBtnCreateCommonUpdateFileClicked);

    // Простой режим по умолчанию
    onExpertModeToggled(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onExpertModeToggled(bool checked)
{
    // Управление видимостью групп в меню "Начальная прошивка"
    ui->grpProgramDataFile->setVisible(checked);
    ui->grpLoaderFile->setVisible(checked);
    ui->lblNewCrc32->setVisible(checked);
    ui->editNewCrc32->setVisible(checked);
    ui->chkUpdateCrc32->setVisible(checked);

    // Управление видимостью групп в меню "Обновление прошивки"
    ui->grpUpdateProgramDataFile->setVisible(checked);
    ui->grpUpdateParameters->setVisible(checked);

    // Изменение размера окна
    if (checked) {
        setFixedSize(expertSize);  // Полный размер для экспертного режима

        // Меню "Начальная прошивка"
        ui->btnShowInfo->move(10, 620);
        ui->btnCreateFileAuto->move(470, 620);
        ui->btnCreateFileManual->move(650, 620);
        ui->grpParameters->move(10, 350);
        ui->grpParameters->setFixedSize(761, 251);
        ui->grpSerialNumbers->setFixedSize(501, 161);
        ui->lblTotalFirmwareSize->move(10, 200);

        // Меню "Обновление прошивки"
        ui->btnUpdateShowInfo->move(10, 620);
        ui->btnUpdateCreateFileAuto->move(470, 620);
        ui->btnUpdateCreateFileManual->move(650, 620);
        ui->btnCreateCommonUpdateFile->move(210, 620);
    } else {
        setFixedSize(simpleSize);  // Уменьшенный размер для простого режима

        // Меню "Начальная прошивка"
        ui->btnShowInfo->move(10, 260);
        ui->btnCreateFileAuto->move(470, 260);
        ui->btnCreateFileManual->move(650, 260);
        ui->grpParameters->move(10, 65);
        ui->grpParameters->setFixedSize(761, 185);
        ui->grpSerialNumbers->setFixedSize(350, 117);
        ui->lblTotalFirmwareSize->move(10, 146);

        // Меню "Обновление прошивки"
        ui->btnUpdateShowInfo->move(10, 260);
        ui->btnUpdateCreateFileAuto->move(470, 260);
        ui->btnUpdateCreateFileManual->move(650, 260);
        ui->btnCreateCommonUpdateFile->move(210, 260);
    }

    QTimer::singleShot(0, this, [this]() {
        QWidget *cornerWidget = ui->tabWidget->cornerWidget(Qt::TopRightCorner);
        if (cornerWidget) {
            QPoint pos = cornerWidget->pos();
            cornerWidget->move(pos.x(), pos.y() - 8);
        }
    });
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    QTimer::singleShot(0, this, [this]() {
        QWidget *cornerWidget = ui->tabWidget->cornerWidget(Qt::TopRightCorner);
        if (cornerWidget) {
            QPoint pos = cornerWidget->pos();
            cornerWidget->move(pos.x(), pos.y() - 8);
        }
    });
}
