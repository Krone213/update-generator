#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , unit1(nullptr)
    , unit2(nullptr)
    , expertModeCheckbox(nullptr)
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

    expertModeCheckbox = new QCheckBox("Экспертный режим", ui->tabWidget);
    ui->tabWidget->setCornerWidget(expertModeCheckbox, Qt::TopRightCorner);
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int /*index*/) {
        QTimer::singleShot(0, this, [this]() {
            QWidget *cornerWidget = ui->tabWidget->cornerWidget(Qt::TopRightCorner);
            if (cornerWidget) cornerWidget->move(cornerWidget->pos().x(), cornerWidget->pos().y() - 8);
        });
    });
    connect(expertModeCheckbox, &QCheckBox::toggled, this, &MainWindow::onExpertModeToggled);

    expertModeCheckbox->installEventFilter(this);

    expertSize = size();
    simpleSize = QSize(800, 470);
    onExpertModeToggled(expertModeCheckbox->isChecked());

    unit1 = new Unit1(ui, this);
    unit2 = new Unit2(ui, this);

    loadConfigAndPopulate("config.xml");

    connect(ui->cmbRevision, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::handleRevisionComboBoxChanged);
    connect(ui->cmbUpdateRevision, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::handleUpdateRevisionComboBoxChanged);
    connect(ui->cmbDevModel, QOverload<int>::of(&QComboBox::currentIndexChanged), // DeviceModel/@name for Tab 1
            this, &MainWindow::handleDevModelNameComboBoxChanged);
    connect(ui->cmbUpdateDevModel, QOverload<int>::of(&QComboBox::currentIndexChanged), // DeviceModel/@name for Tab 2
            this, &MainWindow::handleUpdateDevModelNameComboBoxChanged);

    // Unit1 connections ...
    connect(ui->btnChooseProgramDataFile, &QPushButton::clicked, unit1, &Unit1::onBtnChooseProgramDataFileClicked);
    connect(ui->btnChooseLoaderFile, &QPushButton::clicked, unit1, &Unit1::onBtnChooseLoaderFileClicked);
    connect(ui->btnCreateFileManual, &QPushButton::clicked, unit1, &Unit1::onBtnCreateFileManualClicked);
    connect(ui->btnCreateFileAuto, &QPushButton::clicked, unit1, &Unit1::onBtnCreateFileAutoClicked);
    connect(ui->btnShowInfo, &QPushButton::clicked, unit1, &Unit1::onBtnShowInfoClicked);
    connect(ui->btnClearRevision, &QPushButton::clicked, unit1, &Unit1::onBtnClearRevisionClicked);
    connect(ui->btnConnect, &QPushButton::clicked, unit1, &Unit1::onBtnConnectClicked);
    connect(ui->btnUploadCPU1, &QPushButton::clicked, unit1, &Unit1::onbtnUploadCPU1Clicked);
    connect(ui->btnUploadCPU2, &QPushButton::clicked, unit1, &Unit1::onbtnUploadCPU2Clicked);
    connect(ui->btnEraseChip, &QPushButton::clicked, unit1, &Unit1::onBtnEraseChipClicked);

    // Unit2 connections ...
    connect(ui->btnChooseUpdateProgramDataFile, &QPushButton::clicked, unit2, &Unit2::onBtnChooseUpdateProgramDataFileClicked);
    connect(ui->btnUpdateCreateFileManual, &QPushButton::clicked, unit2, &Unit2::onBtnUpdateCreateFileManualClicked);
    connect(ui->btnUpdateShowInfo, &QPushButton::clicked, unit2, &Unit2::onBtnUpdateShowInfoClicked);
    connect(ui->btnClearUpdateRevision, &QPushButton::clicked, unit2, &Unit2::onBtnClearUpdateRevisionClicked);
    connect(ui->btnUpdateCreateFileAuto, &QPushButton::clicked, this, &MainWindow::onAutoCreateUpdateTriggered);

    ui->cmbDevModel->setCurrentIndex(-1);
    ui->cmbUpdateDevModel->setCurrentIndex(-1);

    ui->cmbRevision->setEnabled(false);
    ui->cmbUpdateRevision->setEnabled(false);
    filterAndPopulateRevisionComboBoxes("", nullptr);

    ui->cmbRevision->setCurrentIndex(-1);
    ui->cmbUpdateRevision->setCurrentIndex(-1);

    ui->cmbDeviceModel->clear(); // Очищаем его от возможных элементов из Designer
    ui->cmbDeviceModel->setEnabled(false); // Изначально неактивен, т.к. ревизия не выбрана

    updateUnit2UI("");
}


MainWindow::~MainWindow()
{
    if(unit1) unit1->stopOpenOcd();
    delete ui;
}

void MainWindow::onExpertModeToggled(bool checked)
{
    ui->grpProgramDataFile->setVisible(checked);
    ui->grpLoaderFile->setVisible(checked);
    ui->lblNewCrc32->setVisible(checked);
    ui->editNewCrc32->setVisible(checked);
    ui->lblNumberOfSerials->setVisible(checked);
    ui->editNumberOfSerials->setVisible(checked);
    ui->chkUpdateCrc32->setVisible(checked);
    ui->grpUpdateProgramDataFile->setVisible(checked);
    ui->grpUpdateParameters->setVisible(checked);
    ui->cmbDeviceModel->setVisible(checked);
    ui->cmbTargetMCU->setVisible(checked);

    if (checked) {
        setFixedSize(expertSize);
        ui->btnShowInfo->move(10, 700);
        ui->btnCreateFileAuto->move(460, 700);
        ui->btnCreateFileManual->move(650, 700);
        ui->grpParameters->move(10, 390);
        ui->grpParameters->setFixedSize(761, 301);
        ui->grpSerialNumbers->setFixedSize(341, 224);
        ui->lblTotalFirmwareSize->move(10, 260);
        ui->btnUpdateShowInfo->move(10, 700);
        ui->btnUpdateCreateFileAuto->move(460, 700);
        ui->btnUpdateCreateFileManual->move(650, 700);

    } else {
        setFixedSize(simpleSize);
        ui->btnShowInfo->move(10, 370);
        ui->btnCreateFileAuto->move(460, 370);
        ui->btnCreateFileManual->move(650, 370);
        ui->grpParameters->move(10, 100);
        ui->grpParameters->setFixedSize(761, 260);
        ui->grpSerialNumbers->setFixedSize(341, 89);
        ui->lblTotalFirmwareSize->move(10, 220);
        ui->btnUpdateShowInfo->move(10, 370);
        ui->btnUpdateCreateFileAuto->move(460, 370);
        ui->btnUpdateCreateFileManual->move(650, 370);
    }

    QTimer::singleShot(0, this, [this]() {
        QWidget *cornerWidget = ui->tabWidget->cornerWidget(Qt::TopRightCorner);
        if (cornerWidget) {
            QPoint pos = cornerWidget->pos();
            cornerWidget->move(pos.x(), pos.y() - 8);
        }
    });
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == expertModeCheckbox) {
        bool ctrlPressed = (QApplication::keyboardModifiers() & Qt::ControlModifier);
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                if (!ctrlPressed) return true;
            }
        }
        else if (event->type() == QEvent::MouseButtonDblClick) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                if (!ctrlPressed) return true;
            }
        }
        else if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Space) {
                if (!ctrlPressed) return true;
            }
        }
        else if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {};
        }
    }
    return QObject::eventFilter(watched, event);
}

void MainWindow::loadConfigAndPopulate(const QString &filePath)
{
    QSet<QString> deviceModelXmlNamesSet;
    revisionsMap.clear();
    allCategoriesList.clear();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось открыть config.xml: " + file.errorString());
        return;
    }
    QString fileContent = QString::fromUtf8(file.readAll());
    file.close();

#ifndef Q_OS_WIN
    fileContent.replace("\\", "/");
#endif

    QDomDocument doc;
    QString errorMsg; int errorLine, errorColumn;
    if (!doc.setContent(fileContent, true, &errorMsg, &errorLine, &errorColumn)) {
        QMessageBox::critical(this, "Ошибка разбора XML",
                              QString("Невозможно разобрать config.xml\n%1\nСтрока: %2, Колонка: %3")
                                  .arg(errorMsg).arg(errorLine).arg(errorColumn));
        return;
    }

    QDomElement root = doc.documentElement();
    if (root.tagName() != "UpdateGenerator") {
        QMessageBox::critical(this, "Ошибка", "Неверный корневой элемент...");
        return;
    }

    if(unit1) {
        connect(unit1, &Unit1::logToInterface, this, &MainWindow::appendToLog);
    }

    QDomNodeList deviceModelNodes = root.elementsByTagName("DeviceModel");
    for (int i = 0; i < deviceModelNodes.count(); ++i) {
        QDomElement modelElement = deviceModelNodes.at(i).toElement();
        QString currentDeviceModelXmlName = modelElement.attribute("name");
        if (!currentDeviceModelXmlName.isEmpty()) {
            deviceModelXmlNamesSet.insert(currentDeviceModelXmlName);
        }

        QDomNodeList revisions = modelElement.elementsByTagName("revision");
        for (int j = 0; j < revisions.count(); ++j) {
            QDomElement rev = revisions.at(j).toElement();
            if (!rev.hasAttribute("category")) continue;
            ExtendedRevisionInfo info;
            info.category = rev.attribute("category");
            info.deviceModelXmlName = currentDeviceModelXmlName;
            info.bldrDevModel = rev.attribute("BldrDevModel"); // Это значение пойдет в cmbDeviceModel
            info.bootloaderFile = rev.firstChildElement("BootLoader_File").text().trimmed();
            info.mainProgramFile = rev.firstChildElement("MainProgram_File").text().trimmed();
            info.beginAddress = rev.firstChildElement("begin_adres").text().trimmed();
            info.saveFirmwarePath = rev.firstChildElement("SaveFirmware").text().trimmed();
            info.saveUpdatePath = rev.firstChildElement("SaveUpdate").text().trimmed();
            QDomElement commandsElem = rev.firstChildElement("comands");
            if (!commandsElem.isNull()) {
                info.cmdMainProg = getBoolAttr(commandsElem, "main_prog");
                info.cmdOnlyForEnteredSN = getBoolAttr(commandsElem, "OnlyForEnteredSN");
                info.cmdOnlyForEnteredDevID = getBoolAttr(commandsElem, "OnlyForEnteredDevID");
                info.cmdNoCheckModel = getBoolAttr(commandsElem, "NoCheckModel");
            }
            QDomElement dopElem = rev.firstChildElement("dop");
            if (!dopElem.isNull()) {
                info.dopSaveBthName = getBoolAttr(dopElem, "SaveBthName");
                info.dopSaveBthAddr = getBoolAttr(dopElem, "SaveBthAddr");
                info.dopSaveDevDesc = getBoolAttr(dopElem, "SaveDevDesc");
                info.dopSaveHwVerr = getBoolAttr(dopElem, "SaveHwVerr");
                info.dopSaveSwVerr = getBoolAttr(dopElem, "SaveSwVerr");
                info.dopSaveDevSerial = getBoolAttr(dopElem, "SaveDevSerial");
                info.dopSaveDevModel = getBoolAttr(dopElem, "SaveDevModel");
                info.dopLvl1MemProtection = getBoolAttr(dopElem, "Lvl1MemProtection");
            }

            if (!info.category.isEmpty()) {
                revisionsMap[info.category] = info;
                if (!allCategoriesList.contains(info.category)) {
                    allCategoriesList.append(info.category);
                }
            }
        }
    }

    if (!allCategoriesList.contains("OthDev")) {
        allCategoriesList.append("OthDev");
    }
    if (!revisionsMap.contains("OthDev")) {
        ExtendedRevisionInfo othDevInfo;
        othDevInfo.category = "OthDev";
        othDevInfo.deviceModelXmlName = "";
        othDevInfo.bldrDevModel = ""; // OthDev может не иметь BldrDevModel
        revisionsMap["OthDev"] = othDevInfo;
    }

    qInfo() << "MainWindow loaded" << revisionsMap.count() << "revisions into central map.";
    qInfo() << "Loaded DeviceModel/@names:" << deviceModelXmlNamesSet.values();
    qInfo() << "All loaded categories:" << allCategoriesList;

    ui->cmbRevision->blockSignals(true);
    ui->cmbUpdateRevision->blockSignals(true);
    ui->cmbDevModel->blockSignals(true);
    ui->cmbUpdateDevModel->blockSignals(true);
    ui->cmbDeviceModel->blockSignals(true); // Для BldrDevModel

    ui->cmbRevision->clear();
    ui->cmbUpdateRevision->clear();
    ui->cmbDevModel->clear();
    ui->cmbUpdateDevModel->clear();
    ui->cmbDeviceModel->clear(); // Очищаем от элементов из Designer

    QStringList deviceModelXmlNameList = deviceModelXmlNamesSet.values();
    ui->cmbDevModel->addItems(deviceModelXmlNameList);
    ui->cmbUpdateDevModel->addItems(deviceModelXmlNameList);

    // cmbDeviceModel (BldrDevModel) не заполняется списком здесь

    ui->cmbRevision->blockSignals(false);
    ui->cmbUpdateRevision->blockSignals(false);
    ui->cmbDevModel->blockSignals(false);
    ui->cmbUpdateDevModel->blockSignals(false);
    ui->cmbDeviceModel->blockSignals(false);
}

void MainWindow::filterAndPopulateRevisionComboBoxes(const QString& deviceModelXmlName, QComboBox* /*sourceDevModelComboBox*/) {
    qDebug() << Q_FUNC_INFO << "Filtering revisions for DeviceModelXmlName:" << deviceModelXmlName;

    QStringList filteredCategories;
    QString previouslySelectedRevision = ui->cmbRevision->currentText();
    QString previouslySelectedUpdateRevision = ui->cmbUpdateRevision->currentText();

    bool revisionCombosEnabled = !deviceModelXmlName.isEmpty();
    ui->cmbRevision->setEnabled(revisionCombosEnabled);
    ui->cmbUpdateRevision->setEnabled(revisionCombosEnabled);

    if (revisionCombosEnabled) {
        for (const QString& category : allCategoriesList) {
            if (category == "OthDev") {
                if (!filteredCategories.contains("OthDev")) filteredCategories.append("OthDev");
                continue;
            }
            if (revisionsMap.contains(category) && revisionsMap.value(category).deviceModelXmlName == deviceModelXmlName) {
                filteredCategories.append(category);
            }
        }
    }

    qDebug() << Q_FUNC_INFO << "Filtered categories:" << filteredCategories;

    bool revBlocked = ui->cmbRevision->signalsBlocked();
    bool updRevBlocked = ui->cmbUpdateRevision->signalsBlocked();
    ui->cmbRevision->blockSignals(true);
    ui->cmbUpdateRevision->blockSignals(true);

    ui->cmbRevision->clear();
    ui->cmbRevision->addItems(filteredCategories);
    ui->cmbUpdateRevision->clear();
    ui->cmbUpdateRevision->addItems(filteredCategories);

    int newRevIdx = -1; // По умолчанию нет выбора
    if (revisionCombosEnabled && filteredCategories.contains(previouslySelectedRevision) &&
        (revisionsMap.value(previouslySelectedRevision).deviceModelXmlName == deviceModelXmlName || previouslySelectedRevision == "OthDev")) {
        newRevIdx = ui->cmbRevision->findText(previouslySelectedRevision); // Восстанавливаем, если подходит
    }
    ui->cmbRevision->setCurrentIndex(newRevIdx);

    int newUpdRevIdx = -1; // По умолчанию нет выбора
    if (revisionCombosEnabled && filteredCategories.contains(previouslySelectedUpdateRevision) &&
        (revisionsMap.value(previouslySelectedUpdateRevision).deviceModelXmlName == deviceModelXmlName || previouslySelectedUpdateRevision == "OthDev")) {
        newUpdRevIdx = ui->cmbUpdateRevision->findText(previouslySelectedUpdateRevision);
    }
    ui->cmbUpdateRevision->setCurrentIndex(newUpdRevIdx);

    qDebug() << Q_FUNC_INFO << "cmbRevision new index:" << newRevIdx << "Text:" << ui->cmbRevision->currentText();
    qDebug() << Q_FUNC_INFO << "cmbUpdateRevision new index:" << newUpdRevIdx << "Text:" << ui->cmbUpdateRevision->currentText();

    if (!revBlocked) ui->cmbRevision->blockSignals(false);
    if (!updRevBlocked) ui->cmbUpdateRevision->blockSignals(false);
}

void MainWindow::updateBldrDevModelDisplay(const QString& category) {
    QString bldrDevModelToShow;
    bool enableBldrDevModelCombo = false;

    if (!category.isEmpty() && revisionsMap.contains(category)) {
        bldrDevModelToShow = revisionsMap.value(category).bldrDevModel;
        enableBldrDevModelCombo = !bldrDevModelToShow.isEmpty(); // Активируем, только если есть что показать
    }

    ui->cmbDeviceModel->blockSignals(true);
    ui->cmbDeviceModel->clear();
    if (enableBldrDevModelCombo) {
        ui->cmbDeviceModel->addItem(bldrDevModelToShow);
        ui->cmbDeviceModel->setCurrentIndex(0);
    } else {
        ui->cmbDeviceModel->setCurrentIndex(-1); // Нет выбора, если пусто
    }
    ui->cmbDeviceModel->setEnabled(enableBldrDevModelCombo); // Управляем активностью
    ui->cmbDeviceModel->blockSignals(false);
}


void MainWindow::synchronizeComboBoxes(QObject* senderComboBoxObj) {
    QComboBox* senderComboBox = qobject_cast<QComboBox*>(senderComboBoxObj);
    if (!senderComboBox || senderComboBox->signalsBlocked()) {
        qDebug() << Q_FUNC_INFO << "Skipped: sender null, blocked, or not QComboBox.";
        return;
    }

    qDebug() << Q_FUNC_INFO << "Start. Triggered by" << senderComboBox->objectName()
             << "Index:" << senderComboBox->currentIndex()
             << "Text:" << senderComboBox->currentText();

    QString determinedCategory = "";
    QString determinedDeviceModelXmlName = ""; // DeviceModel/@name

    bool modelXmlNameChanged = false;

    if (senderComboBox == ui->cmbRevision || senderComboBox == ui->cmbUpdateRevision) {
        determinedCategory = senderComboBox->currentText();
        if (determinedCategory.isEmpty() || !revisionsMap.contains(determinedCategory)) {
            if (senderComboBox->currentIndex() == -1 && senderComboBox->isEnabled()){ // Явный сброс пользователем
                determinedCategory = ""; // Означает "нет выбора"
                determinedDeviceModelXmlName = ui->cmbDevModel->currentText(); // Модель остается той же, ревизия сброшена
            } else if (senderComboBox->count() > 0 && senderComboBox->isEnabled()) { // Если список не пуст и активен
                determinedCategory = senderComboBox->itemText(0); // Берем первую доступную (может быть OthDev)
                if (!revisionsMap.contains(determinedCategory)) determinedCategory = "OthDev";
            } else { // Список пуст или неактивен
                determinedCategory = ""; // Нет выбора
                determinedDeviceModelXmlName = ""; // И модель тоже не выбрана
            }
        }

        if (!determinedCategory.isEmpty() && revisionsMap.contains(determinedCategory)) {
            if (determinedCategory == "OthDev") {
                determinedDeviceModelXmlName = revisionsMap.value("OthDev").deviceModelXmlName; // Может быть ""
                if (determinedDeviceModelXmlName.isEmpty() && !ui->cmbDevModel->currentText().isEmpty()) {
                    // Если OthDev общий, а модель была выбрана, не сбрасываем модель здесь.
                    // Пусть модель останется, а ревизия будет OthDev.
                    determinedDeviceModelXmlName = ui->cmbDevModel->currentText();
                }
            } else {
                determinedDeviceModelXmlName = revisionsMap.value(determinedCategory).deviceModelXmlName;
            }
        } else { // determinedCategory пуст (нет выбора ревизии)
            // Модель берется из соответствующего комбобокса модели, если она там выбрана
            if (senderComboBox == ui->cmbRevision) determinedDeviceModelXmlName = ui->cmbDevModel->currentText();
            else if (senderComboBox == ui->cmbUpdateRevision) determinedDeviceModelXmlName = ui->cmbUpdateDevModel->currentText();
        }


    } else if (senderComboBox == ui->cmbDevModel || senderComboBox == ui->cmbUpdateDevModel) {
        determinedDeviceModelXmlName = senderComboBox->currentText(); // Может быть "" если индекс -1
        modelXmlNameChanged = true;

        bool revBlocked = ui->cmbRevision->signalsBlocked();
        bool updRevBlocked = ui->cmbUpdateRevision->signalsBlocked();
        ui->cmbRevision->blockSignals(true);
        ui->cmbUpdateRevision->blockSignals(true);
        filterAndPopulateRevisionComboBoxes(determinedDeviceModelXmlName, senderComboBox);
        if (!revBlocked) ui->cmbRevision->blockSignals(false);
        if (!updRevBlocked) ui->cmbUpdateRevision->blockSignals(false);

        // Определяем категорию на основе нового состояния комбобоксов ревизий
        if (determinedDeviceModelXmlName.isEmpty()) { // Модель сброшена
            determinedCategory = ""; // Нет выбора категории
        } else { // Модель выбрана
            // Проверяем, какая ревизия теперь выбрана (может быть -1)
            // Приоритет cmbRevision, если он связан с senderComboBox (cmbDevModel)
            // или cmbUpdateRevision, если он связан с senderComboBox (cmbUpdateDevModel)
            QComboBox* relevantRevisionCombo = (senderComboBox == ui->cmbDevModel) ? ui->cmbRevision : ui->cmbUpdateRevision;
            if (relevantRevisionCombo->currentIndex() != -1) {
                determinedCategory = relevantRevisionCombo->currentText();
            } else { // Если после фильтрации нет выбора, то и категория не выбрана
                determinedCategory = "";
            }
        }
        if (!determinedCategory.isEmpty() && !revisionsMap.contains(determinedCategory)) {
            determinedCategory = ""; // Сбрасываем, если категория стала невалидной
        }


    } else {
        qWarning() << Q_FUNC_INFO << "Unknown sender:" << senderComboBox->objectName();
        return;
    }

    qDebug() << Q_FUNC_INFO << "Determined state - Category:" << determinedCategory << "DeviceModelXmlName:" << determinedDeviceModelXmlName;

    QList<QComboBox*> allUserComboBoxes = {ui->cmbRevision, ui->cmbUpdateRevision, ui->cmbDevModel, ui->cmbUpdateDevModel};
    for(QComboBox* cb : allUserComboBoxes) {
        if(cb != senderComboBox) cb->blockSignals(true);
    }

    // Обновляем cmbRevision
    if (senderComboBox != ui->cmbRevision) {
        if (ui->cmbRevision->currentText() != determinedCategory || (determinedCategory.isEmpty() && ui->cmbRevision->currentIndex() != -1) ) {
            int idx = ui->cmbRevision->findText(determinedCategory); // "" найдет -1
            qDebug() << Q_FUNC_INFO << "Updating cmbRevision to" << determinedCategory << "(idx:" << idx << ")";
            ui->cmbRevision->setCurrentIndex(idx);
        }
    }

    // Обновляем cmbUpdateRevision
    if (senderComboBox != ui->cmbUpdateRevision) {
        if (ui->cmbUpdateRevision->currentText() != determinedCategory || (determinedCategory.isEmpty() && ui->cmbUpdateRevision->currentIndex() != -1) ) {
            int idx = ui->cmbUpdateRevision->findText(determinedCategory);
            qDebug() << Q_FUNC_INFO << "Updating cmbUpdateRevision to" << determinedCategory << "(idx:" << idx << ")";
            ui->cmbUpdateRevision->setCurrentIndex(idx);
        }
    }

    // Обновляем cmbDevModel (DeviceModel/@name)
    if (senderComboBox != ui->cmbDevModel) {
        if (ui->cmbDevModel->currentText() != determinedDeviceModelXmlName || (determinedDeviceModelXmlName.isEmpty() && ui->cmbDevModel->currentIndex() != -1)) {
            int idx = ui->cmbDevModel->findText(determinedDeviceModelXmlName);
            qDebug() << Q_FUNC_INFO << "Updating cmbDevModel to" << determinedDeviceModelXmlName << "(idx:" << idx << ")";
            ui->cmbDevModel->setCurrentIndex(idx);
            if (modelXmlNameChanged && senderComboBox != ui->cmbDevModel) { // Если модель изменилась не этим комбобоксом
                filterAndPopulateRevisionComboBoxes(determinedDeviceModelXmlName, ui->cmbDevModel);
            }
        }
    }

    // Обновляем cmbUpdateDevModel (DeviceModel/@name)
    if (senderComboBox != ui->cmbUpdateDevModel) {
        if (ui->cmbUpdateDevModel->currentText() != determinedDeviceModelXmlName || (determinedDeviceModelXmlName.isEmpty() && ui->cmbUpdateDevModel->currentIndex() != -1)) {
            int idx = ui->cmbUpdateDevModel->findText(determinedDeviceModelXmlName);
            qDebug() << Q_FUNC_INFO << "Updating cmbUpdateDevModel to" << determinedDeviceModelXmlName << "(idx:" << idx << ")";
            ui->cmbUpdateDevModel->setCurrentIndex(idx);
            if (modelXmlNameChanged && senderComboBox != ui->cmbUpdateDevModel) {
                filterAndPopulateRevisionComboBoxes(determinedDeviceModelXmlName, ui->cmbUpdateDevModel);
            }
        }
    }

    for(QComboBox* cb : allUserComboBoxes) {
        if(cb != senderComboBox) cb->blockSignals(false);
    }

    // Обновляем отображение BldrDevModel в cmbDeviceModel
    updateBldrDevModelDisplay(determinedCategory);

    // Обновляем UI Unit2
    if (!determinedCategory.isEmpty() && revisionsMap.contains(determinedCategory) && ui->cmbRevision->isEnabled()) {
        updateUnit2UI(determinedCategory);
    } else {
        qDebug() << Q_FUNC_INFO << "Category" << determinedCategory << "not valid for Unit2 UI update. Clearing Unit2 UI.";
        updateUnit2UI("");
    }
    qDebug() << Q_FUNC_INFO << "End.";
}


// ... (appendToLog, handle*ComboBoxChanged, findFirstCategoryForDeviceModelXmlName, updateUnit2UI, onExpertModeToggled, etc.)
// onExpertModeToggled должен также управлять видимостью cmbDeviceModel (BldrDevModel)
void MainWindow::appendToLog(const QString &message, bool isError)
{
    if (!ui || !ui->logTextEdit) return;

    QString colorName;
    bool effectivelyAnError = isError;

    if (isError && message.contains("The remote host closed the connection") && unit1 && unit1->wasShutdownCommandSent()) {
        colorName = "gray"; // Или другой "не ошибочный" цвет
        effectivelyAnError = false;
    } else if (isError) {
        colorName = "red";
    } else {
        if (message.contains("Verified OK") || message.contains("успешно завершены") || message.contains("Programming Finished") || message.contains("Telnet соединение установлено") || message.contains("Examination succeed")) {
            colorName = "darkGreen";
        } else if (message.contains("[Telnet TX]")) {
            colorName = "purple";
        } else if (message.contains("Warn :")) {
            colorName = "orange";
        } else if (message.startsWith("[OOCD ERR] Info :") || message.startsWith("[OOCD] Info :")) {
            colorName = "gray";
        } else {
            colorName = "navy";
        }
    }

    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");

    QString escapedMessage = message;
    escapedMessage.replace('&', "&");
    escapedMessage.replace('<', "<");
    escapedMessage.replace('>', ">");

    QString htmlMessage = QString("<font color='%1'>[%2] %3</font>")
                              .arg(colorName)
                              .arg(timestamp)
                              .arg(escapedMessage);

    ui->logTextEdit->append(htmlMessage);
    ui->logTextEdit->moveCursor(QTextCursor::End);
    ui->logTextEdit->ensureCursorVisible();
}


void MainWindow::handleRevisionComboBoxChanged(int /*index*/) {
    synchronizeComboBoxes(sender());
}
void MainWindow::handleUpdateRevisionComboBoxChanged(int /*index*/) {
    synchronizeComboBoxes(sender());
}
void MainWindow::handleDevModelNameComboBoxChanged(int /*index*/) {
    synchronizeComboBoxes(sender());
}
void MainWindow::handleUpdateDevModelNameComboBoxChanged(int /*index*/) {
    synchronizeComboBoxes(sender());
}

QString MainWindow::findFirstCategoryForDeviceModelXmlName(const QString& deviceModelXmlName) {
    if (deviceModelXmlName.isEmpty()) return ""; // Если модель не выбрана, то и категория не выбрана

    for (const QString& category : allCategoriesList) {
        if (category == "OthDev") continue;
        if (revisionsMap.contains(category) && revisionsMap.value(category).deviceModelXmlName == deviceModelXmlName) {
            return category;
        }
    }
    // Если специфичных не найдено, проверяем OthDev (если он подходит для этой модели или общий)
    if (allCategoriesList.contains("OthDev")) {
        const auto& othDevInfo = revisionsMap.value("OthDev");
        if (othDevInfo.deviceModelXmlName.isEmpty() || othDevInfo.deviceModelXmlName == deviceModelXmlName) {
            return "OthDev";
        }
    }
    return ""; // Не найдено подходящих категорий
}

void MainWindow::updateUnit2UI(const QString& category) {
    if (!unit2) return;
    qDebug() << Q_FUNC_INFO << "Updating Unit2 UI for category:" << category;

    if (category.isEmpty() || !revisionsMap.contains(category)) {
        qDebug() << Q_FUNC_INFO << "Category is empty or not in map. Clearing Unit2 fields.";
        unit2->clearUIFields();
    } else if (category == "OthDev") {
        const ExtendedRevisionInfo& othInfo = revisionsMap.value("OthDev");
        bool othDevHasMeaningfulDataForUnit2 = !othInfo.bldrDevModel.isEmpty() ||
                                               !othInfo.beginAddress.isEmpty() ||
                                               !othInfo.saveUpdatePath.isEmpty();
        if (othDevHasMeaningfulDataForUnit2) {
            qDebug() << Q_FUNC_INFO << "Updating Unit2 UI with OthDev info.";
            unit2->updateUI(othInfo);
        } else {
            qDebug() << Q_FUNC_INFO << "OthDev category is 'empty' for Unit2. Clearing Unit2 fields.";
            unit2->clearUIFields();
        }
    }
    else {
        qDebug() << Q_FUNC_INFO << "Updating Unit2 UI with info for category:" << category;
        unit2->updateUI(revisionsMap.value(category));
    }
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

    if (!ui->cmbUpdateRevision->isEnabled() || currentCategory.isEmpty() || !revisionsMap.contains(currentCategory) || currentCategory == "OthDev") {
        QMessageBox::warning(this, "Внимание", "Сначала выберите модель, а затем конкретную ревизию (кроме OthDev) для авто-создания файла обновления.");
        return;
    }
    const ExtendedRevisionInfo& info = revisionsMap.value(currentCategory);
    if (info.saveUpdatePath.isEmpty()) {
        QMessageBox::warning(this, "Внимание",
                             "Путь для авто-сохранения обновления (<SaveUpdate>) не определен в config.xml для ревизии: " + currentCategory);
        return;
    }
    QString currentDir = QDir::currentPath();
    QString outDirAbs = QDir(currentDir).filePath(info.saveUpdatePath);
    QDir dir(outDirAbs);
    if (!dir.exists() && !dir.mkpath(".")) {
        QMessageBox::critical(this, "Ошибка", "Не удалось создать папку для авто-сохранения обновления:\n" + QDir::toNativeSeparators(outDirAbs));
        return;
    }
    QString baseName = currentCategory;
    baseName.replace(QRegularExpression(R"([\\/:*?"<>|])"), "_");
    QString outFilePathAbs = dir.filePath(baseName + "_Update.bin");
    qInfo() << "Triggering auto-create update in Unit2 for file:" << outFilePathAbs;
    if(unit2) unit2->createUpdateFiles(outFilePathAbs);
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
