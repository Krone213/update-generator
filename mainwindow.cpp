#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "unit1.h" // Include Unit1 header
#include "unit2.h" // Include Unit2 header
#include <QFile>
#include <QDomDocument>
#include <QMessageBox>
#include <QDebug>
#include <QComboBox>   // Include QComboBox header
#include <QCheckBox>   // Include QCheckBox header
#include <QDir>        // Include QDir for path manipulation
#include <QFileDialog> // Include QFileDialog
#include <QSet>        // Include QSet for collecting unique models

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , unit1(nullptr)
    , unit2(nullptr)
{
    ui->setupUi(this);

    // --- Настройки окна (ИЗ ВАШЕГО КОДА) ---
    setFixedSize(size()); // Изначально фиксируем размер
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    // Центрирование окна (ИЗ ВАШЕГО КОДА)
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        int screenWidth = screenGeometry.width();
        int screenHeight = screenGeometry.height();
        int windowWidth = this->width();
        int windowHeight = this->height();
        int x = (screenWidth - windowWidth) / 2;
        int y = (screenHeight - windowHeight) / 2 - 50; // с вашим смещением -50
        this->move(x, y);
    }

    // --- Экспертный режим (ИЗ ВАШЕГО КОДА) ---
    QCheckBox *expertModeCheckbox = new QCheckBox("Экспертный режим", ui->tabWidget);
    ui->tabWidget->setCornerWidget(expertModeCheckbox, Qt::TopRightCorner);
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int /*index*/) {
        QTimer::singleShot(0, this, [this]() {
            QWidget *cornerWidget = ui->tabWidget->cornerWidget(Qt::TopRightCorner);
            if (cornerWidget) cornerWidget->move(cornerWidget->pos().x(), cornerWidget->pos().y() - 8);
        });
    });
    connect(expertModeCheckbox, &QCheckBox::toggled, this, &MainWindow::onExpertModeToggled);
    // Запоминаем размеры для режимов (ИЗ ВАШЕГО КОДА)
    expertSize = size();             // Текущий размер окна для экспертного режима
    simpleSize = QSize(800, 360);    // Уменьшенный размер для простого режима


    // --- Инициализация Unit1 и Unit2 ---
    unit1 = new Unit1(ui, this); // Unit1 инициализируется и сам загрузит конфиг
    unit2 = new Unit2(ui, this); // Unit2 инициализируется, данные получит позже

    // --- Загрузка конфига в MainWindow и ЗАПОЛНЕНИЕ ВСЕХ комбобоксов ---
    loadConfigAndPopulate("config.xml");

    // --- Подключения для СИНХРОНИЗАЦИИ комбобоксов (теперь через общие обработчики) ---
    connect(ui->cmbRevision, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::handleRevisionComboBoxChanged);
    connect(ui->cmbUpdateRevision, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::handleUpdateRevisionComboBoxChanged);
    // Подключение нового комбобокса
    connect(ui->cmbDeviceModel, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::handleDeviceModelComboBoxChanged);


    // --- Подключения кнопок Unit1 НАПРЯМУЮ к слотам unit1 (ИЗ ВАШЕГО КОДА) ---
    connect(ui->btnChooseProgramDataFile, &QPushButton::clicked, unit1, &Unit1::onBtnChooseProgramDataFileClicked);
    connect(ui->btnChooseLoaderFile, &QPushButton::clicked, unit1, &Unit1::onBtnChooseLoaderFileClicked);
    connect(ui->btnCreateFileManual, &QPushButton::clicked, unit1, &Unit1::onBtnCreateFileManualClicked);
    connect(ui->btnCreateFileAuto, &QPushButton::clicked, unit1, &Unit1::onBtnCreateFileAutoClicked);
    connect(ui->btnShowInfo, &QPushButton::clicked, unit1, &Unit1::onBtnShowInfoClicked);
    connect(ui->btnClearRevision, &QPushButton::clicked, unit1, &Unit1::onBtnClearRevisionClicked);
    connect(ui->btnConnect, &QPushButton::clicked, unit1, &Unit1::onBtnConnectClicked);
    connect(ui->btnUpload, &QPushButton::clicked, unit1, &Unit1::onBtnUploadClicked);

    // --- Подключения кнопок Unit2 (ИЗ ВАШЕГО КОДА + АВТО) ---
    connect(ui->btnChooseUpdateProgramDataFile, &QPushButton::clicked, unit2, &Unit2::onBtnChooseUpdateProgramDataFileClicked);
    connect(ui->btnClearUpdateProgramDataFile, &QPushButton::clicked, unit2, &Unit2::onBtnClearUpdateProgramDataFileClicked);
    connect(ui->btnUpdateCreateFileManual, &QPushButton::clicked, unit2, &Unit2::onBtnUpdateCreateFileManualClicked);
    connect(ui->btnUpdateShowInfo, &QPushButton::clicked, unit2, &Unit2::onBtnUpdateShowInfoClicked);
    connect(ui->btnClearUpdateRevision, &QPushButton::clicked, unit2, &Unit2::onBtnClearUpdateRevisionClicked);
    connect(ui->btnCreateCommonUpdateFile, &QPushButton::clicked, unit2, &Unit2::onBtnCreateCommonUpdateFileClicked);
    // Кнопка "Авто" для Unit2 подключается к MainWindow
    connect(ui->btnUpdateCreateFileAuto, &QPushButton::clicked, this, &MainWindow::onAutoCreateUpdateTriggered);


    // --- Инициализация UI (ИЗ ВАШЕГО КОДА) ---
    onExpertModeToggled(false);  // Начать в простом режиме

    // Сбросить выбор в комбобоксах после заполнения
    ui->cmbRevision->setCurrentIndex(-1);
    ui->cmbUpdateRevision->setCurrentIndex(-1);
    ui->cmbDeviceModel->setCurrentIndex(-1); // Сбрасываем новый
    updateUnit2UI(""); // Очистить UI для Unit2
}

MainWindow::~MainWindow()
{
    delete ui;
}

// --- Загрузка конфига и заполнение комбобоксов ---
void MainWindow::loadConfigAndPopulate(const QString &filePath)
{
    QSet<QString> deviceModelsSet; // Собираем уникальные модели BldrDevModel
    revisionsMap.clear();
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) { QMessageBox::critical(this, "Ошибка", "Не удалось открыть config.xml: " + file.errorString()); return; }
    QDomDocument doc;
    QString errorMsg; int errorLine, errorColumn;
    if (!doc.setContent(&file, true, &errorMsg, &errorLine, &errorColumn)) { QMessageBox::critical(this, "Ошибка разбора XML", QString("Невозможно разобрать config.xml\n%1\nСтрока: %2, Колонка: %3").arg(errorMsg).arg(errorLine).arg(errorColumn)); file.close(); return; }
    file.close();
    QDomElement root = doc.documentElement();
    if (root.tagName() != "UpdateGenerator") { QMessageBox::critical(this, "Ошибка", "Неверный корневой элемент..."); return; }

    QStringList categories;
    QDomNodeList deviceModels = root.elementsByTagName("DeviceModel");
    for (int i = 0; i < deviceModels.count(); ++i) {
        QDomElement model = deviceModels.at(i).toElement();
        QDomNodeList revisions = model.elementsByTagName("revision");
        for (int j = 0; j < revisions.count(); ++j) {
            QDomElement rev = revisions.at(j).toElement();
            if (!rev.hasAttribute("category")) continue;
            ExtendedRevisionInfo info; info.category = rev.attribute("category");
            info.bldrDevModel = rev.attribute("BldrDevModel"); info.bootloaderFile = rev.firstChildElement("BootLoader_File").text().trimmed(); info.mainProgramFile = rev.firstChildElement("MainProgram_File").text().trimmed(); info.beginAddress = rev.firstChildElement("begin_adres").text().trimmed(); info.saveFirmwarePath = rev.firstChildElement("SaveFirmware").text().trimmed(); info.saveUpdatePath = rev.firstChildElement("SaveUpdate").text().trimmed();
            QDomElement commandsElem = rev.firstChildElement("comands"); if (!commandsElem.isNull()) { info.cmdMainProg = getBoolAttr(commandsElem, "main_prog"); info.cmdOnlyForEnteredSN = getBoolAttr(commandsElem, "OnlyForEnteredSN"); info.cmdOnlyForEnteredDevID = getBoolAttr(commandsElem, "OnlyForEnteredDevID"); info.cmdNoCheckModel = getBoolAttr(commandsElem, "NoCheckModel");}
            QDomElement dopElem = rev.firstChildElement("dop"); if (!dopElem.isNull()) { info.dopSaveBthName = getBoolAttr(dopElem, "SaveBthName"); info.dopSaveBthAddr = getBoolAttr(dopElem, "SaveBthAddr"); info.dopSaveDevDesc = getBoolAttr(dopElem, "SaveDevDesc"); info.dopSaveHwVerr = getBoolAttr(dopElem, "SaveHwVerr"); info.dopSaveSwVerr = getBoolAttr(dopElem, "SaveSwVerr"); info.dopSaveDevSerial = getBoolAttr(dopElem, "SaveDevSerial"); info.dopSaveDevModel = getBoolAttr(dopElem, "SaveDevModel"); info.dopLvl1MemProtection = getBoolAttr(dopElem, "Lvl1MemProtection");}
            if (!info.category.isEmpty()) { revisionsMap[info.category] = info; if (!categories.contains(info.category)) categories.append(info.category); }
            // Собираем уникальные непустые модели (кроме OthDev)
            if (!info.category.isEmpty() && info.category != "OthDev" && !info.bldrDevModel.isEmpty()) {
                deviceModelsSet.insert(info.bldrDevModel);
            }
        }
    }
    if (!revisionsMap.contains("OthDev")) { ExtendedRevisionInfo othDevInfo; othDevInfo.category = "OthDev"; revisionsMap["OthDev"] = othDevInfo; categories.append("OthDev"); }
    qInfo() << "MainWindow loaded" << revisionsMap.count() << "revisions into central map.";

    // Заполнение ВСЕХ трех комбобоксов
    ui->cmbRevision->blockSignals(true);
    ui->cmbUpdateRevision->blockSignals(true);
    ui->cmbDeviceModel->blockSignals(true); // Блокируем новый комбобокс

    ui->cmbRevision->clear();
    ui->cmbUpdateRevision->clear();
    ui->cmbDeviceModel->clear();

    // categories.sort(); // Опционально сортируем категории
    ui->cmbRevision->addItems(categories);
    ui->cmbUpdateRevision->addItems(categories);

    // Заполнение cmbDeviceModel уникальными моделями
    QStringList modelList = deviceModelsSet.values();
    modelList.sort(); // Опционально сортируем модели
    ui->cmbDeviceModel->addItems(modelList);

    ui->cmbRevision->setCurrentIndex(-1);
    ui->cmbUpdateRevision->setCurrentIndex(-1);
    ui->cmbDeviceModel->setCurrentIndex(-1);

    ui->cmbRevision->blockSignals(false);
    ui->cmbUpdateRevision->blockSignals(false);
    ui->cmbDeviceModel->blockSignals(false);
}

// --- Слоты-обработчики изменений комбобоксов ---
void MainWindow::handleRevisionComboBoxChanged(int /*index*/) {
    synchronizeComboBoxes(sender());
}

void MainWindow::handleUpdateRevisionComboBoxChanged(int /*index*/) {
    synchronizeComboBoxes(sender());
}

void MainWindow::handleDeviceModelComboBoxChanged(int /*index*/) {
    synchronizeComboBoxes(sender());
}

// --- Централизованная синхронизация ---
// --- Централизованная синхронизация ---
void MainWindow::synchronizeComboBoxes(QObject* senderComboBox) {
    QComboBox* changedComboBox = qobject_cast<QComboBox*>(senderComboBox);
    // Добавим проверку на валидность отправителя и на то, не заблокированы ли у него сигналы
    // (чтобы избежать лишних срабатываний при программной установке)
    if (!changedComboBox || changedComboBox->signalsBlocked()) {
        qDebug() << "Sync: Skipping because sender is null, blocked, or not a QComboBox";
        return;
    }
    qDebug() << "Sync: Triggered by" << changedComboBox->objectName() << "with index" << changedComboBox->currentIndex();

    QString selectedCategory = "";
    QString selectedModel = "";
    bool selectionCleared = (changedComboBox->currentIndex() == -1);
    int othDevIndexRevision = ui->cmbRevision->findText("OthDev");
    int othDevIndexUpdate = ui->cmbUpdateRevision->findText("OthDev");

    // 1. Определяем целевую категорию и модель на основе изменившегося комбобокса
    if (changedComboBox == ui->cmbRevision) {
        selectedCategory = changedComboBox->currentText();
        if (selectionCleared || selectedCategory == "OthDev") {
            selectedCategory = "OthDev"; // Нормализация для сброса
            selectionCleared = true;
        } else if (revisionsMap.contains(selectedCategory)) {
            selectedModel = revisionsMap.value(selectedCategory).bldrDevModel;
        } else { // Ошибка: категория не найдена
            qWarning() << "Sync: Category from cmbRevision not found:" << selectedCategory;
            selectedCategory = "OthDev";
            selectionCleared = true;
        }
    } else if (changedComboBox == ui->cmbUpdateRevision) {
        selectedCategory = changedComboBox->currentText();
        if (selectionCleared || selectedCategory == "OthDev") {
            selectedCategory = "OthDev";
            selectionCleared = true;
        } else if (revisionsMap.contains(selectedCategory)) {
            selectedModel = revisionsMap.value(selectedCategory).bldrDevModel;
        } else {
            qWarning() << "Sync: Category from cmbUpdateRevision not found:" << selectedCategory;
            selectedCategory = "OthDev";
            selectionCleared = true;
        }
    } else if (changedComboBox == ui->cmbDeviceModel) {
        selectedModel = changedComboBox->currentText();
        if (selectionCleared) {
            selectedCategory = "OthDev";
        } else {
            selectedCategory = findCategoryForModel(selectedModel);
            if (selectedCategory.isEmpty()) { // Не нашли категорию для модели
                qWarning() << "Sync: Could not find category for model:" << selectedModel;
                selectedCategory = "OthDev";
                selectionCleared = true;
            }
        }
    } else {
        qWarning() << "Sync: Unknown sender:" << senderComboBox;
        return; // Неизвестный отправитель
    }

    qDebug() << "Sync: Determined target category:" << selectedCategory << "model:" << selectedModel << "cleared:" << selectionCleared;

    // --- 2. Обновление других комбобоксов и UI ---

    // Определяем целевые индексы
    int targetRevisionIndex = selectionCleared ? othDevIndexRevision : ui->cmbRevision->findText(selectedCategory);
    int targetUpdateRevisionIndex = selectionCleared ? othDevIndexUpdate : ui->cmbUpdateRevision->findText(selectedCategory);
    int targetModelIndex = (selectionCleared || selectedModel.isEmpty()) ? -1 : ui->cmbDeviceModel->findText(selectedModel);

    // Проверяем, нужно ли реально менять индексы
    bool needUpdateRevision = (ui->cmbRevision->currentIndex() != targetRevisionIndex);
    bool needUpdateUpdateRevision = (ui->cmbUpdateRevision->currentIndex() != targetUpdateRevisionIndex);
    bool needUpdateDeviceModel = (ui->cmbDeviceModel->currentIndex() != targetModelIndex);

    // Блокируем все сигналы перед началом изменений, чтобы избежать каскадных вызовов
    ui->cmbRevision->blockSignals(true);
    ui->cmbUpdateRevision->blockSignals(true);
    ui->cmbDeviceModel->blockSignals(true);

    // Обновляем cmbRevision, если нужно и если он не был источником
    if (changedComboBox != ui->cmbRevision && needUpdateRevision) {
        qDebug() << "Sync: Updating cmbRevision programmatically to index" << targetRevisionIndex;
        ui->cmbRevision->blockSignals(false); // <<< РАЗБЛОКИРОВАТЬ ПЕРЕД УСТАНОВКОЙ
        ui->cmbRevision->setCurrentIndex(targetRevisionIndex); // Сигнал должен сработать здесь
        ui->cmbRevision->blockSignals(true);  // <<< ЗАБЛОКИРОВАТЬ СРАЗУ ПОСЛЕ
    } else {
        qDebug() << "Sync: Skipping programmatic update for cmbRevision (either sender or index already matches)";
    }

    // Обновляем cmbUpdateRevision, если нужно и если он не был источником
    if (changedComboBox != ui->cmbUpdateRevision && needUpdateUpdateRevision) {
        qDebug() << "Sync: Updating cmbUpdateRevision programmatically to index" << targetUpdateRevisionIndex;
        // Сигналы уже заблокированы и остаются заблокированными
        ui->cmbUpdateRevision->setCurrentIndex(targetUpdateRevisionIndex);
    } else {
        qDebug() << "Sync: Skipping programmatic update for cmbUpdateRevision (either sender or index already matches)";
    }

    // Обновляем cmbDeviceModel, если нужно и если он не был источником
    if (changedComboBox != ui->cmbDeviceModel && needUpdateDeviceModel) {
        qDebug() << "Sync: Updating cmbDeviceModel programmatically to index" << targetModelIndex;
            // Сигналы уже заблокированы и остаются заблокированными
        ui->cmbDeviceModel->setCurrentIndex(targetModelIndex);
        if (targetModelIndex == -1 && !selectedModel.isEmpty() && !selectionCleared) {
            qWarning() << "Sync: Model" << selectedModel << "not found in cmbDeviceModel list during update.";
        }
    } else {
        qDebug() << "Sync: Skipping programmatic update for cmbDeviceModel (either sender or index already matches)";
    }

    // --- 3. Обновляем UI для Unit2 ---
    // Это нужно делать всегда, чтобы отразить текущее состояние, определенное категорией
    qDebug() << "Sync: Updating Unit2 UI for category:" << selectedCategory;
    updateUnit2UI(selectedCategory);

    // --- 4. Разблокируем все сигналы в конце ---
    // Важно разблокировать, чтобы пользовательские изменения снова работали
    ui->cmbRevision->blockSignals(false);
    ui->cmbUpdateRevision->blockSignals(false);
    ui->cmbDeviceModel->blockSignals(false);
    qDebug() << "Sync: All signals unblocked.";

    // Ручной вызов Unit1::onRevisionChanged больше не нужен, так как мы полагаемся на сигнал
}

// --- Обновление UI ТОЛЬКО для Unit2 ---
void MainWindow::updateUnit2UI(const QString& category) {
    if (!unit2) return;
    // Используем "OthDev" как индикатор очистки
    if (category.isEmpty() || category == "OthDev" || !revisionsMap.contains(category)) {
        unit2->clearUIFields();
    } else {
        unit2->updateUI(revisionsMap.value(category));
    }
}



// --- Вспомогательная функция поиска категории по модели ---
// Возвращает ПЕРВУЮ найденную категорию или пустую строку
QString MainWindow::findCategoryForModel(const QString& modelName) {
    if (modelName.isEmpty()) return "";
    // Итерация по значениям карты (ExtendedRevisionInfo)
    for (const ExtendedRevisionInfo &info : revisionsMap.values()) {
        // Ищем точное совпадение модели, исключая "OthDev"
        if (info.bldrDevModel == modelName && info.category != "OthDev") {
            return info.category; // Нашли первое совпадение
        }
    }
    return ""; // Не найдено подходящей категории
}


// --- ВОССТАНОВЛЕННАЯ ЛОГИКА ИЗМЕНЕНИЯ РАЗМЕРА/ПОЗИЦИЙ (ИЗ ВАШЕГО КОДА) ---
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
    // В экспертном режиме показываем комбобокс модели, в простом - нет
    ui->cmbDeviceModel->setVisible(checked);      // Сам комбобокс

    // Изменение размера и позиций (ТОЧНО КАК В ВАШЕМ КОДЕ, но учтём cmbDeviceModel)
    if (checked) {
        setFixedSize(expertSize);  // Полный размер для экспертного режима

        // Меню "Начальная прошивка" - позиции из вашего кода
        ui->btnShowInfo->move(10, 620);
        ui->btnCreateFileAuto->move(470, 620);
        ui->btnCreateFileManual->move(650, 620);
        ui->grpParameters->move(10, 350);
        ui->grpParameters->setFixedSize(761, 251);
        ui->grpSerialNumbers->setFixedSize(501, 161);
        // --- ВАЖНО: Позиция lblTotalFirmwareSize в экспертном режиме ИЗ ВАШЕГО КОДА ---
        ui->lblTotalFirmwareSize->move(10, 200); // <--- ВАШЕ ЗНАЧЕНИЕ

        // Меню "Обновление прошивки" - позиции из вашего кода
        ui->btnUpdateShowInfo->move(10, 620);
        ui->btnUpdateCreateFileAuto->move(470, 620);
        ui->btnUpdateCreateFileManual->move(650, 620);
        ui->btnCreateCommonUpdateFile->move(210, 620);
        // Позиции для модели устройства (можно настроить)
        // ui->lblStaticDeviceModel->move(?, ?);
        // ui->cmbDeviceModel->move(?, ?);
    } else {
        setFixedSize(simpleSize);  // Уменьшенный размер для простого режима

        // Меню "Начальная прошивка" - позиции из вашего кода
        ui->btnShowInfo->move(10, 260);
        ui->btnCreateFileAuto->move(470, 260);
        ui->btnCreateFileManual->move(650, 260);
        ui->grpParameters->move(10, 65);
        ui->grpParameters->setFixedSize(761, 185);
        ui->grpSerialNumbers->setFixedSize(350, 117); // Ваша строка
        // --- ВАЖНО: Позиция lblTotalFirmwareSize в простом режиме ИЗ ВАШЕГО КОДА ---
        ui->lblTotalFirmwareSize->move(10, 146); // <--- ВАШЕ ЗНАЧЕНИЕ

        // Меню "Обновление прошивки" - позиции из вашего кода
        ui->btnUpdateShowInfo->move(10, 260);
        ui->btnUpdateCreateFileAuto->move(470, 260);
        ui->btnUpdateCreateFileManual->move(650, 260);
        ui->btnCreateCommonUpdateFile->move(210, 260);
    }

    // Корректировка чекбокса (ИЗ ВАШЕГО КОДА)
    QTimer::singleShot(0, this, [this]() {
        QWidget *cornerWidget = ui->tabWidget->cornerWidget(Qt::TopRightCorner);
        if (cornerWidget) {
            QPoint pos = cornerWidget->pos();
            cornerWidget->move(pos.x(), pos.y() - 8);
        }
    });
}

// --- ВОССТАНОВЛЕН showEvent (ИЗ ВАШЕГО КОДА) ---
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
// --- Слот для кнопки "Авто" в Unit2 ---
void MainWindow::onAutoCreateUpdateTriggered() {
    if (!unit2) return;
    QString currentCategory = ui->cmbUpdateRevision->currentText();
    if (currentCategory.isEmpty() || currentCategory == "OthDev" || !revisionsMap.contains(currentCategory)) { QMessageBox::warning(this, "Внимание", "Выберите ревизию (кроме OthDev) для авто-создания файла обновления."); return; }

    const ExtendedRevisionInfo& info = revisionsMap.value(currentCategory);
    if (info.saveUpdatePath.isEmpty()) { QMessageBox::warning(this, "Внимание", "Путь для авто-сохранения обновления (<SaveUpdate>) не определен в config.xml для ревизии: " + currentCategory); return; }

    QString currentDir = QDir::currentPath();
    // Путь к ПАПКЕ автосохранения
    QString outDirAbs = QDir(currentDir).filePath(info.saveUpdatePath);

    // Создаем папку, если ее нет
    QDir dir(outDirAbs);
    if (!dir.exists() && !dir.mkpath(".")) {
        QMessageBox::critical(this, "Ошибка", "Не удалось создать папку для авто-сохранения обновления:\n" + QDir::toNativeSeparators(outDirAbs)); return;
    }

    // Формируем ПОЛНЫЙ ПУТЬ К ФАЙЛУ (имя файла по умолчанию)
    QString baseName = currentCategory; // Используем категорию как базовое имя
    baseName.replace(QRegularExpression(R"([\\/:*?"<>|])"), "_"); // Очистка имени
    QString outFilePathAbs = dir.filePath(baseName + "_Update.bin"); // Или просто "FirmwareUpdate.bin"

    qInfo() << "Triggering auto-create update in Unit2 for file:" << outFilePathAbs;
    // Передаем ПОЛНЫЙ ПУТЬ К ФАЙЛУ в createUpdateFiles
    unit2->createUpdateFiles(outFilePathAbs);
}
