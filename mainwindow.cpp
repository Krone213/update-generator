#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , unit1(nullptr)
    , unit2(nullptr)
{
    ui->setupUi(this);

    setFixedSize(size());
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        int screenWidth = screenGeometry.width();
        int screenHeight = screenGeometry.height();
        int windowWidth = this->width();
        int windowHeight = this->height();
        int x = (screenWidth - windowWidth) / 2;
        int y = (screenHeight - windowHeight) / 2 - 50;
        this->move(x, y);
    }

    QCheckBox *expertModeCheckbox = new QCheckBox("Экспертный режим", ui->tabWidget);
    ui->tabWidget->setCornerWidget(expertModeCheckbox, Qt::TopRightCorner);
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int /*index*/) {
        QTimer::singleShot(0, this, [this]() {
            QWidget *cornerWidget = ui->tabWidget->cornerWidget(Qt::TopRightCorner);
            if (cornerWidget) cornerWidget->move(cornerWidget->pos().x(), cornerWidget->pos().y() - 8);
        });
    });
    connect(expertModeCheckbox, &QCheckBox::toggled, this, &MainWindow::onExpertModeToggled);

    expertSize = size();
    simpleSize = QSize(800, 360);

    unit1 = new Unit1(ui, this);
    unit2 = new Unit2(ui, this);

    // --- Загрузка конфига в MainWindow и ЗАПОЛНЕНИЕ ВСЕХ комбобоксов ---
    loadConfigAndPopulate("config.xml");

    // --- Подключения для СИНХРОНИЗАЦИИ комбобоксов ---
    connect(ui->cmbRevision, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::handleRevisionComboBoxChanged);
    connect(ui->cmbUpdateRevision, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::handleUpdateRevisionComboBoxChanged);

    // Подключение нового комбобокса
    connect(ui->cmbDeviceModel, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::handleDeviceModelComboBoxChanged);

    // Unit1
    connect(ui->btnChooseProgramDataFile, &QPushButton::clicked, unit1, &Unit1::onBtnChooseProgramDataFileClicked);
    connect(ui->btnChooseLoaderFile, &QPushButton::clicked, unit1, &Unit1::onBtnChooseLoaderFileClicked);
    connect(ui->btnCreateFileManual, &QPushButton::clicked, unit1, &Unit1::onBtnCreateFileManualClicked);
    connect(ui->btnCreateFileAuto, &QPushButton::clicked, unit1, &Unit1::onBtnCreateFileAutoClicked);
    connect(ui->btnShowInfo, &QPushButton::clicked, unit1, &Unit1::onBtnShowInfoClicked);
    connect(ui->btnClearRevision, &QPushButton::clicked, unit1, &Unit1::onBtnClearRevisionClicked);
    connect(ui->btnConnectAndUpload, &QPushButton::clicked, unit1, &Unit1::onBtnConnectAndUploadClicked);

    // Unit2
    connect(ui->btnChooseUpdateProgramDataFile, &QPushButton::clicked, unit2, &Unit2::onBtnChooseUpdateProgramDataFileClicked);
    connect(ui->btnUpdateCreateFileManual, &QPushButton::clicked, unit2, &Unit2::onBtnUpdateCreateFileManualClicked);
    connect(ui->btnUpdateShowInfo, &QPushButton::clicked, unit2, &Unit2::onBtnUpdateShowInfoClicked);
    connect(ui->btnClearUpdateRevision, &QPushButton::clicked, unit2, &Unit2::onBtnClearUpdateRevisionClicked);
    connect(ui->btnCreateCommonUpdateFile, &QPushButton::clicked, unit2, &Unit2::onBtnCreateCommonUpdateFileClicked);
    connect(ui->btnUpdateCreateFileAuto, &QPushButton::clicked, this, &MainWindow::onAutoCreateUpdateTriggered);

    onExpertModeToggled(false);

    // Сбросить выбор в комбобоксах после заполнения
    ui->cmbRevision->setCurrentIndex(-1);
    ui->cmbUpdateRevision->setCurrentIndex(-1);
    ui->cmbDeviceModel->setCurrentIndex(-1);
    updateUnit2UI("");
}

MainWindow::~MainWindow()
{
    delete ui;
}

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

    connect(unit1, &Unit1::logToInterface, this, &MainWindow::appendToLog);

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

    ui->cmbRevision->blockSignals(true);
    ui->cmbUpdateRevision->blockSignals(true);
    ui->cmbDeviceModel->blockSignals(true);

    ui->cmbRevision->clear();
    ui->cmbUpdateRevision->clear();
    ui->cmbDeviceModel->clear();

    ui->cmbRevision->addItems(categories);
    ui->cmbUpdateRevision->addItems(categories);

    QStringList modelList = deviceModelsSet.values();
    modelList.sort();
    ui->cmbDeviceModel->addItems(modelList);

    ui->cmbRevision->setCurrentIndex(-1);
    ui->cmbUpdateRevision->setCurrentIndex(-1);
    ui->cmbDeviceModel->setCurrentIndex(-1);

    ui->cmbRevision->blockSignals(false);
    ui->cmbUpdateRevision->blockSignals(false);
    ui->cmbDeviceModel->blockSignals(false);
}

void MainWindow::appendToLog(const QString &message, bool isError)
{
    if (!ui || !ui->logTextEdit) return;

    QString color = isError ? "red" : "blue";
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");

    QString escapedMessage = message;
    escapedMessage.replace('&', "&");
    escapedMessage.replace('<', "<");
    escapedMessage.replace('>', ">");

    QString htmlMessage = QString("<font color='%1'>[%2] %3</font>")
                              .arg(color)
                              .arg(timestamp)
                              .arg(escapedMessage);

    ui->logTextEdit->append(htmlMessage); // append добавляет текст и переводит строку
}

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
void MainWindow::synchronizeComboBoxes(QObject* senderComboBox) {
    QComboBox* changedComboBox = qobject_cast<QComboBox*>(senderComboBox);

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
        return;
    }

    qDebug() << "Sync: Determined target category:" << selectedCategory << "model:" << selectedModel << "cleared:" << selectionCleared;

    int targetRevisionIndex = selectionCleared ? othDevIndexRevision : ui->cmbRevision->findText(selectedCategory);
    int targetUpdateRevisionIndex = selectionCleared ? othDevIndexUpdate : ui->cmbUpdateRevision->findText(selectedCategory);
    int targetModelIndex = (selectionCleared || selectedModel.isEmpty()) ? -1 : ui->cmbDeviceModel->findText(selectedModel);

    bool needUpdateRevision = (ui->cmbRevision->currentIndex() != targetRevisionIndex);
    bool needUpdateUpdateRevision = (ui->cmbUpdateRevision->currentIndex() != targetUpdateRevisionIndex);
    bool needUpdateDeviceModel = (ui->cmbDeviceModel->currentIndex() != targetModelIndex);

    ui->cmbRevision->blockSignals(true);
    ui->cmbUpdateRevision->blockSignals(true);
    ui->cmbDeviceModel->blockSignals(true);

    // Обновляем cmbRevision, если нужно и если он не был источником
    if (changedComboBox != ui->cmbRevision && needUpdateRevision) {
        qDebug() << "Sync: Updating cmbRevision programmatically to index" << targetRevisionIndex;
        ui->cmbRevision->blockSignals(false); // <<< РАЗБЛОКИРОВАТЬ ПЕРЕД УСТАНОВКОЙ
        ui->cmbRevision->setCurrentIndex(targetRevisionIndex);
        ui->cmbRevision->blockSignals(true);
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

    qDebug() << "Sync: Updating Unit2 UI for category:" << selectedCategory;
    updateUnit2UI(selectedCategory);

    ui->cmbRevision->blockSignals(false);
    ui->cmbUpdateRevision->blockSignals(false);
    ui->cmbDeviceModel->blockSignals(false);
}

void MainWindow::updateUnit2UI(const QString& category) {
    if (!unit2) return;
    // Используем "OthDev" как индикатор очистки
    if (category.isEmpty() || category == "OthDev" || !revisionsMap.contains(category)) {
        unit2->clearUIFields();
    } else {
        unit2->updateUI(revisionsMap.value(category));
    }
}

QString MainWindow::findCategoryForModel(const QString& modelName) {
    if (modelName.isEmpty()) return "";
    // Итерация по значениям карты (ExtendedRevisionInfo)
    for (const ExtendedRevisionInfo &info : revisionsMap.values()) {
        // Ищем точное совпадение модели, исключая "OthDev"
        if (info.bldrDevModel == modelName && info.category != "OthDev") {
            return info.category;
        }
    }
    return "";
}

void MainWindow::onExpertModeToggled(bool checked)
{
    ui->grpProgramDataFile->setVisible(checked);
    ui->grpLoaderFile->setVisible(checked);
    ui->lblNewCrc32->setVisible(checked);
    ui->editNewCrc32->setVisible(checked);
    ui->chkUpdateCrc32->setVisible(checked);

    ui->grpUpdateProgramDataFile->setVisible(checked);
    ui->grpUpdateParameters->setVisible(checked);
    ui->cmbDeviceModel->setVisible(checked);

    if (checked) {
        setFixedSize(expertSize);

        // Меню "Начальная прошивка" - позиции из вашего кода
        ui->btnShowInfo->move(10, 620);
        ui->btnCreateFileAuto->move(470, 620);
        ui->btnCreateFileManual->move(650, 620);
        ui->grpParameters->move(10, 350);
        ui->grpParameters->setFixedSize(761, 251);
        ui->grpSerialNumbers->setFixedSize(471, 181);
        ui->lblTotalFirmwareSize->move(10, 210);
        ui->editNumberOfSerials->move(160, ui->editNumberOfSerials->pos().y() + 10);
        ui->lblNumberOfSerials->move(10, ui->lblNumberOfSerials->pos().y() + 10);
        ui->logTextEdit->move(490, 85);
        ui->logTextEdit->setFixedSize(261, 120);

        // Меню "Обновление прошивки" - позиции из вашего кода
        ui->btnUpdateShowInfo->move(10, 620);
        ui->btnUpdateCreateFileAuto->move(470, 620);
        ui->btnUpdateCreateFileManual->move(650, 620);
        ui->btnCreateCommonUpdateFile->move(210, 620);
    } else {
        setFixedSize(simpleSize);

        // Меню "Начальная прошивка"
        ui->btnShowInfo->move(10, 260);
        ui->btnCreateFileAuto->move(470, 260);
        ui->btnCreateFileManual->move(650, 260);
        ui->grpParameters->move(10, 65);
        ui->grpParameters->setFixedSize(761, 185);
        ui->grpSerialNumbers->setFixedSize(350, 117);
        ui->lblTotalFirmwareSize->move(10, 146);
        ui->editNumberOfSerials->move(160, ui->editNumberOfSerials->pos().y() - 10);
        ui->lblNumberOfSerials->move(10, ui->lblNumberOfSerials->pos().y() - 10);
        ui->logTextEdit->move(365, 77);
        ui->logTextEdit->setFixedSize(387, 64);

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

void MainWindow::onAutoCreateUpdateTriggered() {
    if (!unit2) return;
    QString currentCategory = ui->cmbUpdateRevision->currentText();
    if (currentCategory.isEmpty() || currentCategory == "OthDev" || !revisionsMap.contains(currentCategory)) {
        QMessageBox::warning(this, "Внимание", "Выберите ревизию (кроме OthDev) для авто-создания файла обновления."); return; }

    const ExtendedRevisionInfo& info = revisionsMap.value(currentCategory);
    if (info.saveUpdatePath.isEmpty()) { QMessageBox::warning(this, "Внимание",
                             "Путь для авто-сохранения обновления (<SaveUpdate>) не определен в config.xml для ревизии: " + currentCategory); return; }

    QString currentDir = QDir::currentPath();
    QString outDirAbs = QDir(currentDir).filePath(info.saveUpdatePath);

    QDir dir(outDirAbs);
    if (!dir.exists() && !dir.mkpath(".")) {
        QMessageBox::critical(this, "Ошибка", "Не удалось создать папку для авто-сохранения обновления:\n" + QDir::toNativeSeparators(outDirAbs)); return;
    }

    QString baseName = currentCategory;
    baseName.replace(QRegularExpression(R"([\\/:*?"<>|])"), "_");
    QString outFilePathAbs = dir.filePath(baseName + "_Update.bin");

    qInfo() << "Triggering auto-create update in Unit2 for file:" << outFilePathAbs;

    unit2->createUpdateFiles(outFilePathAbs);
}

QSet<QString> MainWindow::getUpdateAutoSavePaths() const
{
    QSet<QString> updatePaths;
        QMapIterator<QString, ExtendedRevisionInfo> i(revisionsMap);

    while (i.hasNext()) {
        i.next();
        const ExtendedRevisionInfo& info = i.value();
        if (!info.saveUpdatePath.isEmpty()) {
            updatePaths.insert(info.saveUpdatePath);
        }
    }
    qDebug() << "MainWindow::getUpdateAutoSavePaths - Collected" << updatePaths.count() << "unique paths on request.";
    return updatePaths;
}

const QMap<QString, ExtendedRevisionInfo>& MainWindow::getRevisionsMap() const {
    return revisionsMap;
}
