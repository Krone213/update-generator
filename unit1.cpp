#include "unit1.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QFileDialog> // Убедитесь, что включен
#include "crcunit.h"   // Убедитесь, что включен и содержит calcCrc32/calcCrc32_intermediate
#include <QDebug>      // Для qDebug/qWarning
#include <QtEndian>    // Для qToLittleEndian и работы с байтами
#include <cstring>     // Для memcpy/memset
#include <QRegularExpression> // Для очистки имени файла


Unit1::Unit1(Ui::MainWindow *ui, QObject *parent)
    : QObject(parent), ui(ui)
{
    // Инициализация таймера
    statusTimer = new QTimer(this);
    statusTimer->setSingleShot(true);
    connect(statusTimer, &QTimer::timeout, this, &Unit1::hideConnectionStatus);

    // Скрываем метку по умолчанию
    ui->lblConnectionStatus->setVisible(false);
    loadConfig("config.xml"); // Unit1 сам грузит конфиг
    // Unit1 сам подключается к своему комбобоксу
    connect(ui->cmbRevision, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Unit1::onRevisionChanged);

    // Инициализация UI после загрузки конфига
    ui->cmbRevision->setCurrentIndex(-1); // Начать без выбора
    onRevisionChanged(-1); // Обновить UI для состояния "нет выбора"
}

void Unit1::onBtnConnectClicked()
{
    // Реализация логики из оригинала
    bool success = true;  // Успешное подключение (placeholder)
    // bool success = false;  // Ошибка подключения (placeholder)
    ui->lblConnectionStatus->setText(success ? "<font color='green'><b>✓</b></font>" : "<font color='red'><b>✗</b></font>");
    ui->lblConnectionStatus->setVisible(true);
    statusTimer->start(2000);  // Показать метку на 2 секунды
}

void Unit1::onBtnUploadClicked()
{
    qDebug() << "Upload button clicked (Unit1)";
    // Здесь логика для btnUpload (placeholder)
    QMessageBox::information(ui->btnUpload->window(), "Загрузка", "Логика загрузки прошивки еще не реализована.");
}

void Unit1::hideConnectionStatus()
{
    ui->lblConnectionStatus->setVisible(false);
}

// Функция-слот для кнопки "Инфо" из оригинала
void Unit1::onBtnShowInfoClicked()
{
    QString title = tr("Инфо");
    QString message = tr("Создание файлов первоначальной прошивки для устройств серий КР, МД, ИН.\n(Максимальный размер файла прошивки 1024 Кб)");
    QMessageBox::information(ui->btnShowInfo->window(), title, message);
}

// Функция загрузки конфига из оригинала
void Unit1::loadConfig(const QString &filePath)
{
    ui->cmbRevision->blockSignals(true);
    ui->cmbRevision->clear();
    revisionsMap.clear();
    autoSavePaths.clear(); // Очищаем сет путей при загрузке
    SaveFirmware.clear();
    ui->cmbRevision->blockSignals(false);


    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(ui->cmbRevision->window(), "Ошибка", "Не удалось открыть config.xml: " + file.errorString());
        return;
    }

    QDomDocument doc;
    QString errorMsg;
    int errorLine, errorColumn;
    if (!doc.setContent(&file, true, &errorMsg, &errorLine, &errorColumn)) {
        QMessageBox::critical(ui->cmbRevision->window(), "Ошибка разбора XML",
                              QString("Невозможно разобрать config.xml\n%1\nСтрока: %2, Колонка: %3")
                                  .arg(errorMsg).arg(errorLine).arg(errorColumn));
        file.close();
        return;
    }
    file.close();

    QDomElement root = doc.documentElement();
    QStringList categories; // Собираем категории для добавления

    QDomNodeList deviceModels = root.elementsByTagName("DeviceModel");
    for (int i = 0; i < deviceModels.count(); ++i) {
        QDomElement model = deviceModels.at(i).toElement();
        QDomNodeList revisions = model.elementsByTagName("revision");
        for (int j = 0; j < revisions.count(); ++j) {
            QDomElement rev = revisions.at(j).toElement();
            if (!rev.hasAttribute("category")) continue;

            QString category = rev.attribute("category");
            QString bootloader = rev.firstChildElement("BootLoader_File").text().trimmed();
            QString mainProgram = rev.firstChildElement("MainProgram_File").text().trimmed();
            // --- Собираем путь автосохранения ---
            QString savePathFragment = rev.firstChildElement("SaveFirmware").text().trimmed();
            if (!savePathFragment.isEmpty()) {
                autoSavePaths.insert(savePathFragment); // Добавляем уникальный путь в сет
            }
            // -------------------------------------

            if (!category.isEmpty() && !bootloader.isEmpty() && !mainProgram.isEmpty()) {
                // Используем оригинальную структуру RevisionInfo
                RevisionInfo info{ bootloader, mainProgram, savePathFragment};
                revisionsMap[category] = info;
                if (!categories.contains(category)) categories.append(category);
            } else {
                qWarning() << "Пропущена неполная ревизия:" << category;
            }
        }
    }

    // Добавляем найденные категории в комбобокс
    ui->cmbRevision->blockSignals(true);
    // categories.sort(); // Опционально сортируем
    ui->cmbRevision->addItems(categories);

    // Добавляем "OthDev"
    if (!revisionsMap.contains("OthDev")) {
        ui->cmbRevision->addItem("OthDev");
        revisionsMap["OthDev"] = RevisionInfo{ "", "", "" }; // Пустая информация для OthDev
    }
    ui->cmbRevision->blockSignals(false);

    ui->cmbRevision->setCurrentIndex(-1); // Сбросить выбор по умолчанию
    qInfo() << "Unit1 loaded" << revisionsMap.count() << "revisions.";
}


// Функция onBtnClearRevisionClicked из оригинала
void Unit1::onBtnClearRevisionClicked()
{
    if (autoSavePaths.isEmpty()) {
        QMessageBox::information(ui->btnClearRevision->window(), tr("Очистка"),
                                 tr("Нет информации о путях для автоматического сохранения (проверьте config.xml)."));
        return;
    }
    QStringList pathsToDeleteDisplay;
    QString currentDir = QDir::currentPath();
    for (const QString &pathFragment : qAsConst(autoSavePaths)) {
        pathsToDeleteDisplay << QDir::toNativeSeparators(pathFragment);
    }
    QString confirmationMessage = tr("Будут рекурсивно удалены все файлы и подпапки в следующих директориях (относительно папки программы):\n\n%1\n\nВы уверены, что хотите продолжить?")
                                      .arg(pathsToDeleteDisplay.join("\n"));
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(ui->btnClearRevision->window(), tr("Подтверждение очистки"),
                                  confirmationMessage, QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply != QMessageBox::Yes) { qInfo() << "Очистка отменена пользователем."; return; }

    qInfo() << "Начало очистки автоматически сохраненных файлов...";
    int successCount = 0; int failCount = 0; QStringList errorDetails;
    for (const QString &pathFragment : qAsConst(autoSavePaths)) {
        QString fullPathBase = QDir(currentDir).filePath(pathFragment);
        QDir baseDir(fullPathBase);
        if (baseDir.exists()) {
            qInfo() << "Удаление базовой папки:" << QDir::toNativeSeparators(fullPathBase);
            if (baseDir.removeRecursively()) { qInfo() << "Успешно."; successCount++; }
            else { qWarning() << "Не удалось удалить:" << QDir::toNativeSeparators(fullPathBase); failCount++; errorDetails << QDir::toNativeSeparators(pathFragment) + " (базовая)"; }
        } else { qInfo() << "Базовая папка не найдена, пропуск:" << QDir::toNativeSeparators(fullPathBase); }
    }
    QString resultMessage;
    if (failCount == 0 && successCount > 0) { resultMessage = tr("Автоматически сохраненные файлы и папки (%1) успешно удалены.").arg(successCount); QMessageBox::information(ui->btnClearRevision->window(), tr("Очистка завершена"), resultMessage); }
    else if (failCount > 0) { resultMessage = tr("Не удалось удалить %1 папок:\n%2").arg(failCount).arg(errorDetails.join("\n")); if (successCount > 0) resultMessage += tr("\n\nУспешно удалено: %1 папок.").arg(successCount); QMessageBox::warning(ui->btnClearRevision->window(), tr("Ошибка очистки"), resultMessage); }
    else { resultMessage = tr("Не найдено папок для удаления."); QMessageBox::information(ui->btnClearRevision->window(), tr("Очистка"), resultMessage); }
    qInfo() << "Очистка завершена.";
}
void Unit1::updateProgramDataFileSize(const QString &filePath)
{
    if (filePath.isEmpty() || filePath.startsWith("Файл:") || filePath == "-") {
        ui->lblProgramDataFileSize->setText("Размер: -"); return;
    }
    // Проверяем абсолютный путь
    QString absPath = QDir::isRelativePath(filePath) ? QDir(QDir::currentPath()).filePath(filePath) : filePath;
    QFileInfo fileInfo(absPath);
    if (fileInfo.exists() && fileInfo.isFile()) {
        qint64 size = fileInfo.size();
        // Обновляем правильную метку
        ui->lblProgramDataFileSize->setText("Размер: " + QString::number(size) + tr(" байт"));
    } else {
        // Обновляем правильную метку
        ui->lblProgramDataFileSize->setText(tr("Файл не найден"));
    }
}
// Функция onRevisionChanged из оригинала
// Функция onRevisionChanged из оригинала (ИСПРАВЛЕНО: добавлено объявление selected)
void Unit1::onRevisionChanged(int index)
{
    QString selected; // <--- ОБЪЯВЛЕНИЕ ПЕРЕМЕННОЙ

    // Проверка индекса
    if (index < 0 || index >= ui->cmbRevision->count()) {
        selected = "OthDev"; // Считаем, что выбор сброшен или индекс невалиден
    } else {
        selected = ui->cmbRevision->itemText(index); // Получаем текст выбранного элемента
    }
    qDebug() << "Unit1::onRevisionChanged - selected:" << selected;

    // Дальнейшая логика использует объявленную переменную 'selected'
    if (selected == "OthDev") {
        // Очистка полей для ручного выбора
        ui->lblLoaderFileName->setText("Файл: -");
        ui->lblProgramDataFileName->setText("Файл: -");
        ui->lblLoaderFileSize->setText("Размер: -");
        ui->lblProgramDataFileSize->setText("Размер: -");
        ui->lblTotalFirmwareSize->setText("Размер: -");
        SaveFirmware.clear();
    } else if (revisionsMap.contains(selected)) {
        RevisionInfo info = revisionsMap[selected];
        ui->lblLoaderFileName->setText(info.bootloaderFile.isEmpty() ? "Файл: -" : QDir::toNativeSeparators(info.bootloaderFile));
        ui->lblProgramDataFileName->setText(info.mainProgramFile.isEmpty() ? "Файл: -" : QDir::toNativeSeparators(info.mainProgramFile));
        SaveFirmware = info.SaveFirmware;
        updateLoaderFileSize(info.bootloaderFile);
        updateProgramDataFileSize(info.mainProgramFile);
        updateTotalFirmwareSize();
    } else {
        qWarning() << "Selected revision" << selected << "not found in Unit1's revisionsMap!";
        ui->lblLoaderFileName->setText("Файл: ОШИБКА");
        ui->lblProgramDataFileName->setText("Файл: ОШИБКА");
        ui->lblLoaderFileSize->setText("Размер: -");
        ui->lblProgramDataFileSize->setText("Размер: -");
        ui->lblTotalFirmwareSize->setText("Размер: -");
        SaveFirmware.clear();
    }
}

// Функция updateLoaderFileSize из оригинала
void Unit1::updateLoaderFileSize(const QString &filePath)
{
    if (filePath.isEmpty() || filePath.startsWith("Файл:") || filePath == "-") {
        ui->lblLoaderFileSize->setText("Размер: -"); return;
    }
    // Проверяем абсолютный путь
    QString absPath = QDir::isRelativePath(filePath) ? QDir(QDir::currentPath()).filePath(filePath) : filePath;
    QFileInfo fileInfo(absPath);
    if (fileInfo.exists() && fileInfo.isFile()) {
        qint64 size = fileInfo.size();
        ui->lblLoaderFileSize->setText("Размер: " +QString::number(size) + tr(" байт"));
    } else {
        ui->lblLoaderFileSize->setText(tr("Файл не найден"));
    }
}

// Функция updateTotalFirmwareSize из оригинала - ИЗМЕНЕНА
void Unit1::updateTotalFirmwareSize()
{
    qint64 totalSize = 0;
    QString currentDir = QDir::currentPath();
    QString selectedCategory = ui->cmbRevision->currentText(); // Получаем текущую категорию

    QString loaderFilePath = "";
    QString programDataFilePath = "";

    // Получаем пути к файлам из сохраненных данных, а не из UI меток
    if (!selectedCategory.isEmpty() && selectedCategory != "OthDev" && revisionsMap.contains(selectedCategory))
    {
        const RevisionInfo& info = revisionsMap.value(selectedCategory);
        loaderFilePath = info.bootloaderFile;      // Путь к загрузчику из карты
        programDataFilePath = info.mainProgramFile; // Путь к программе из карты
    }
    else if (selectedCategory == "OthDev" || selectedCategory.isEmpty())
    {
        // Если выбрано OthDev или ничего не выбрано, или выбраны файлы вручную (в экспертном режиме)
        // Пытаемся получить пути из меток (для совместимости с ручным выбором в экспертном режиме)
        QString loaderFilePathRel = ui->lblLoaderFileName->text();
        if (!loaderFilePathRel.startsWith("Файл:") && !loaderFilePathRel.isEmpty() && loaderFilePathRel != "-") {
            loaderFilePath = loaderFilePathRel; // Используем путь из метки
        }

        QString programDataFilePathRel = ui->lblProgramDataFileName->text();
        if (!programDataFilePathRel.startsWith("Файл:") && !programDataFilePathRel.isEmpty() && programDataFilePathRel != "-") {
            programDataFilePath = programDataFilePathRel; // Используем путь из метки
        }
    }


    // Расчет размера на основе полученных путей (loaderFilePath и programDataFilePath)
    if (!loaderFilePath.isEmpty()) {
        // Преобразуем в абсолютный путь, если он относительный
        QString absPath = QDir::isRelativePath(loaderFilePath) ? QDir(currentDir).filePath(loaderFilePath) : loaderFilePath;
        QFileInfo loaderFileInfo(absPath);
        if (loaderFileInfo.exists() && loaderFileInfo.isFile()) {
            // В общем размере учитываем не более 16Кб загрузчика
            totalSize += qMin(loaderFileInfo.size(), (qint64)(16 * 1024));
        } else {
            qWarning() << "updateTotalFirmwareSize: Loader file not found at" << absPath;
        }
    }

    if (!programDataFilePath.isEmpty()) {
        // Преобразуем в абсолютный путь, если он относительный
        QString absPath = QDir::isRelativePath(programDataFilePath) ? QDir(currentDir).filePath(programDataFilePath) : programDataFilePath;
        QFileInfo programDataFileInfo(absPath);
        if (programDataFileInfo.exists() && programDataFileInfo.isFile()) {
            totalSize += programDataFileInfo.size();
        } else {
            qWarning() << "updateTotalFirmwareSize: Program data file not found at" << absPath;
        }
    }

    // Обновление метки общего размера
    if (totalSize > 0) {
        double totalSizeKB = static_cast<double>(totalSize) / 1024.0;
        // Используем tr для возможного перевода и форматирование как в прошлый раз
        ui->lblTotalFirmwareSize->setText(tr("Суммарный размер: %1 КБ (%2 байт)")
                                              .arg(QString::number(totalSizeKB, 'f', 2))
                                              .arg(totalSize));
    } else {
        // Ставим тире, если файлы не выбраны или не найдены
        ui->lblTotalFirmwareSize->setText(tr("Размер: -"));
    }
}

// Функция onBtnChooseProgramDataFileClicked из оригинала
void Unit1::onBtnChooseProgramDataFileClicked()
{
    QString initialDir = QDir::currentPath();
    // Логика определения начальной папки как в оригинале
    QString currentFilePath = ui->lblProgramDataFileName->text();
    if (!currentFilePath.startsWith("Файл:") && !currentFilePath.isEmpty() && QFileInfo(currentFilePath).exists()) {
        initialDir = QFileInfo(currentFilePath).absolutePath();
    } else {
        initialDir = QDir(initialDir).filePath("MainProgram_File");
        QDir().mkpath(initialDir); // Создать, если нет
    }


    QString filePath = QFileDialog::getOpenFileName(
        ui->cmbRevision->window(),
        tr("Выберите файл программы данных"),
        initialDir,
        tr("Файлы прошивки (*.bin *.hex);;Все файлы (*)")
        );

    if (filePath.isEmpty()) return; // Отмена

    QFileInfo fileInfo(filePath); // Информация о выбранном файле
    // Проверки из оригинала
    if (!fileInfo.exists() || !fileInfo.isFile()) { QMessageBox::critical(ui->cmbRevision->window(), tr("Ошибка"), tr("Файл не найден или недоступен.")); return; }
    qint64 size = fileInfo.size();
    if (size > ((1024 - 16) * 1024)) { QMessageBox::critical(ui->cmbRevision->window(), tr("Ошибка"), tr("Файл больше максимально допустимого размера (1008 Кб)")); return; }

    // Чтение файла (если было в оригинале, иначе можно убрать)
    /* QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) { QMessageBox::critical(ui->cmbRevision->window(), tr("Ошибка"), tr("Ошибка при открытии файла")); return; }
    QByteArray data = file.readAll(); file.close();
    if (data.size() != size) { QMessageBox::critical(ui->cmbRevision->window(), tr("Ошибка"), tr("Ошибка при чтении данных файла")); return; }
    */

    // Обновление UI
    ui->lblProgramDataFileName->setText( QDir::toNativeSeparators(filePath) );
    // Обновляем размер через нашу функцию
    updateProgramDataFileSize(filePath);
    // Обновляем общий размер
    updateTotalFirmwareSize();
}

// Функция onBtnChooseLoaderFileClicked из оригинала
void Unit1::onBtnChooseLoaderFileClicked()
{
    QString initialDir = QDir::currentPath();
    // Логика определения начальной папки как в оригинале
    QString currentFilePath = ui->lblLoaderFileName->text();
    if (!currentFilePath.startsWith("Файл:") && !currentFilePath.isEmpty() && QFileInfo(currentFilePath).exists()) {
        initialDir = QFileInfo(currentFilePath).absolutePath();
    } else {
        initialDir = QDir(initialDir).filePath("BootLoader_File");
        QDir().mkpath(initialDir); // Создать, если нет
    }

    QString filePath = QFileDialog::getOpenFileName(
        ui->cmbRevision->window(),
        tr("Выберите файл загрузчика"),
        initialDir,
        tr("Файлы загрузчика (*.bin *.hex);;Все файлы (*)")
        );

    if (filePath.isEmpty()) return; // Отмена

    QFileInfo fileInfo(filePath); // Информация о выбранном файле
        // Проверки из оригинала
    if (!fileInfo.exists() || !fileInfo.isFile()) { QMessageBox::critical(ui->cmbRevision->window(), tr("Ошибка"), tr("Файл не найден или недоступен.")); return; }
    qint64 size = fileInfo.size();
    if (size > (16 * 1024)) { QMessageBox::critical(ui->cmbRevision->window(), tr("Ошибка"), tr("Файл больше максимально допустимого размера (16 Кб)")); return; }

    // Чтение файла (если было в оригинале, иначе можно убрать)
    /* QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) { QMessageBox::critical(ui->cmbRevision->window(), tr("Ошибка"), tr("Ошибка при открытии файла")); return; }
    QByteArray data = file.readAll(); file.close();
    if (data.size() != size) { QMessageBox::critical(ui->cmbRevision->window(), tr("Ошибка"), tr("Ошибка при чтении данных файла")); return; }
    */

    // Обновление UI
    ui->lblLoaderFileName->setText( QDir::toNativeSeparators(filePath) );
    // Обновляем размер через нашу функцию
    updateLoaderFileSize(filePath);
    // Обновляем общий размер
    updateTotalFirmwareSize();
}


// --- Определение ProgInfo ПЕРЕД использованием ---
#pragma pack(push, 1)
struct ProgInfo_Original { // Назовем чуть иначе, чтобы избежать конфликта имен если ProgInfo уже где-то есть
    quint32 tableID;
    quint32 programSize;
    quint32 programCRC; // Это должен быть финальный CRC программы (с ~)
    quint32 tableCRC;   // Это должен быть финальный CRC заголовка (с ~)
};
#pragma pack(pop)

// --- !!! ФУНКЦИЯ createFirmwareFiles ИЗ ВАШЕГО САМОГО ПЕРВОГО СООБЩЕНИЯ !!! ---
// ВАЖНО: Используем ProgInfo_Original и оригинальную логику CRC
void Unit1::createFirmwareFiles(const QString &outputDir)
{
    // --- 1. Проверка входных данных ---
    QString serialBeginStr = ui->editInitialSerialNumber->text().trimmed();
    bool ok;
    int serialBegin = serialBeginStr.toInt(&ok);
    if (serialBeginStr.isEmpty() || !ok || serialBegin <= 0) { QMessageBox::critical(ui->cmbRevision->window(), "Ошибка", "Неверный или не указан начальный серийный номер."); return; }
    QString serialCountStr = ui->editNumberOfSerials->text().trimmed();
    int serialCount = serialCountStr.toInt(&ok);
    if (serialCountStr.isEmpty() || !ok || serialCount <= 0) { QMessageBox::critical(ui->cmbRevision->window(), "Ошибка", "Неверное или не указано количество номеров."); return; }

    // Получаем пути из UI (могут быть относительными)
    QString programFilePathRel = ui->lblProgramDataFileName->text();
    QString loaderFilePathRel = ui->lblLoaderFileName->text();
    if (programFilePathRel.startsWith("Файл:") || programFilePathRel.isEmpty() || programFilePathRel == "-") { QMessageBox::critical(ui->cmbRevision->window(), "Ошибка", "Файл программы не выбран."); return; }
    if (loaderFilePathRel.startsWith("Файл:") || loaderFilePathRel.isEmpty() || loaderFilePathRel == "-") { QMessageBox::critical(ui->cmbRevision->window(), "Ошибка", "Файл загрузчика не выбран."); return; }

    // Преобразуем в абсолютные пути
    QString currentDir = QDir::currentPath();
    QString programFilePathAbs = QDir(currentDir).filePath(programFilePathRel);
    QString loaderFilePathAbs = QDir(currentDir).filePath(loaderFilePathRel);

    QFileInfo programFile(programFilePathAbs);
    QFileInfo loaderFile(loaderFilePathAbs);
    const qint64 minFileSize = 4 * 1024; // Оригинальная проверка
    if (!programFile.exists() || !programFile.isFile() || programFile.size() < minFileSize) { QMessageBox::critical(ui->cmbRevision->window(), "Ошибка", QString("Ошибка файла программы '%1' (%2 Кб).").arg(programFile.fileName()).arg(minFileSize / 1024)); return; }
    if (!loaderFile.exists() || !loaderFile.isFile() || loaderFile.size() < minFileSize) { QMessageBox::critical(ui->cmbRevision->window(), "Ошибка", QString("Ошибка файла загрузчика '%1' (%2 Кб).").arg(loaderFile.fileName()).arg(minFileSize / 1024)); return; }
    // Проверки макс. размера
    if (programFile.size() > ((1024 - 16) * 1024)) { QMessageBox::critical(ui->cmbRevision->window(), "Ошибка", "Файл программы больше максимально допустимого размера (1008 Кб)."); return; }
    if (loaderFile.size() > (16 * 1024)) { QMessageBox::critical(ui->cmbRevision->window(), "Ошибка", "Файл загрузчика больше максимально допустимого размера (16 Кб)."); return; }


    // --- 2. Загрузка данных файлов ---
    QByteArray loaderData = loadFile(loaderFile.absoluteFilePath());
    QByteArray programData = loadFile(programFile.absoluteFilePath());
    if (loaderData.isEmpty()) { QMessageBox::critical(ui->cmbRevision->window(), "Ошибка", "Не удалось прочитать файл загрузчика: " + loaderFile.absoluteFilePath()); return; }
    if (programData.isEmpty()) { QMessageBox::critical(ui->cmbRevision->window(), "Ошибка", "Не удалось прочитать файл программы: " + programFile.absoluteFilePath()); return; }

    // --- 3. Подготовка базового буфера прошивки (Большого) ---
    const qsizetype firmwareBufferSize = 1024 * 1024; // 1MB
    const qsizetype loaderMaxSize = 16 * 1024;
    const qsizetype programOffset = loaderMaxSize;
    const qsizetype serialNumberOffset = 0x4900;
    const qsizetype serialNumberClearSize = 63;
    const qsizetype serialNumberBlockSize = 64;
    const qsizetype progInfoOffset = 0x4FF0;
    // ВАЖНО: actualDataSize здесь - это РЕАЛЬНЫЙ размер loader + program БЕЗ учета патча CRC
    const qsizetype actualDataSize = loaderMaxSize + qsizetype(programData.size());

    // Проверки смещений
    if (actualDataSize > firmwareBufferSize) { QMessageBox::critical(ui->cmbRevision->window(), "Ошибка", QString("Общий размер прошивки (%1 байт) превышает максимальный размер буфера (%2 байт).").arg(actualDataSize).arg(firmwareBufferSize)); return; }
    if (progInfoOffset + qsizetype(sizeof(ProgInfo_Original)) > firmwareBufferSize) { QMessageBox::critical(ui->cmbRevision->window(), "Ошибка", "Смещение таблицы ProgInfo выходит за пределы буфера."); return; }
    if (serialNumberOffset + serialNumberBlockSize > firmwareBufferSize) { QMessageBox::critical(ui->cmbRevision->window(), "Ошибка", "Смещение серийного номера выходит за пределы буфера."); return; }

    QByteArray baseFirmwareBuffer(firmwareBufferSize, '\xFF'); // Заполняем FF
    qsizetype loaderSizeToCopy = qMin((qsizetype)loaderData.size(), loaderMaxSize);
    memcpy(baseFirmwareBuffer.data(), loaderData.constData(), loaderSizeToCopy);
    memcpy(baseFirmwareBuffer.data() + programOffset, programData.constData(), programData.size());

    // --- 4. Настройка CRC и ГЕНЕРАЦИЯ ТАБЛИЦ (Оригинальная логика) ---
    bool updateCRC = ui->chkUpdateCrc32->isChecked();
    quint32 requiredCRC = 0;
    quint32 inline_fwdTable[256];
    quint32 inline_revTable[256];

    if (updateCRC) {
        QString crcText = ui->editNewCrc32->text().trimmed();
        bool crcOk;
        requiredCRC = crcText.toUInt(&crcOk, 16);
        if (!crcOk) { QMessageBox::critical(ui->cmbRevision->window(), "Ошибка", "Неверный формат требуемого CRC32 (ожидается HEX число)."); return; }

        // Встроенная генерация таблиц (как в Delphi/оригинале)
        // ИСПОЛЬЗУЕМ ЛОКАЛЬНУЮ ФУНКЦИЮ ИЗ ЭТОГО ФАЙЛА
        generateCRCTables(inline_fwdTable, inline_revTable);
    }

    // --- 5. Создание папки ---
    QDir dir(outputDir); // outputDir - абсолютный путь
    if (!dir.exists()) { if (!dir.mkpath(".")) { QMessageBox::critical(ui->cmbRevision->window(), "Ошибка", "Не удалось создать папку для сохранения: " + outputDir); return; } }

    // --- 6. Цикл генерации файлов (Оригинальная логика) ---
    int filesCreated = 0;
    ProgInfo_Original progInfo; // Используем локально определенную структуру

    for (int i = 0; i < serialCount; ++i) {
        QByteArray currentFirmware = baseFirmwareBuffer; // Копируем базовый буфер

        // 6a. Серийный номер (Оригинальная логика)
        int currentSerialInt = serialBegin + i; QString serialStr = QString::number(currentSerialInt);
        QByteArray serialBytesRaw = serialStr.toLatin1();
        memset(currentFirmware.data() + serialNumberOffset, '\0', serialNumberClearSize); // Очистка 63 байт
        qsizetype bytesToCopy = qMin((qsizetype)serialBytesRaw.size(), serialNumberClearSize); // Копируем не более 63
        memcpy(currentFirmware.data() + serialNumberOffset, serialBytesRaw.constData(), bytesToCopy);

        // 6b. ProgInfo (Оригинальная логика)
        progInfo.tableID = qToLittleEndian(0x52444C42); // "BLDR"
        progInfo.programSize = qToLittleEndian(static_cast<quint32>(programData.size()));
        QByteArray programDataInBuff = QByteArray::fromRawData(currentFirmware.constData() + programOffset, programData.size());
        progInfo.programCRC = qToLittleEndian(CrcUnit::calcCrc32(programDataInBuff)); // Финальный CRC программы (с ~)
        QByteArray progInfoHeadForCRC = QByteArray::fromRawData((const char*)&progInfo, sizeof(quint32) * 3);
        progInfo.tableCRC = qToLittleEndian(CrcUnit::calcCrc32(progInfoHeadForCRC)); // Финальный CRC заголовка (с ~)
        memcpy(currentFirmware.data() + progInfoOffset, (const char*)&progInfo, sizeof(ProgInfo_Original));

        // --- ВАЖНО: Определяем РЕАЛЬНЫЙ размер данных ДО патча CRC ---
        qsizetype currentFileSize = actualDataSize; // loaderMaxSize + programData.size()

        // 6c. Корректируем финальный CRC32 (Оригинальная логика)
        if (updateCRC) {
            // Вычисляем CRC БЕЗ финального инвертирования от РЕАЛЬНЫХ данных
            QByteArray dataForIntermediateCRC = QByteArray::fromRawData(currentFirmware.constData(), currentFileSize);
            quint32 intermediateCRC = CrcUnit::calcCrc32_intermediate(dataForIntermediateCRC);

            // Вычисляем значение патча (цикл реверса как в Delphi/оригинале)
            quint32 patchValue = requiredCRC ^ 0xFFFFFFFF;
            // Читаем байты ПРОМЕЖУТОЧНОГО CRC для реверсивного расчета
            unsigned char intermediateCRCBytesLE[4];
            intermediateCRCBytesLE[0] = (intermediateCRC >> 0)  & 0xFF;
            intermediateCRCBytesLE[1] = (intermediateCRC >> 8)  & 0xFF;
            intermediateCRCBytesLE[2] = (intermediateCRC >> 16) & 0xFF;
            intermediateCRCBytesLE[3] = (intermediateCRC >> 24) & 0xFF;

            // --- ТОТ САМЫЙ ЦИКЛ РЕВЕРСА ---
            // В оригинале цикл шел по байтам intermediateCRC, записанным в конец буфера.
            // Эмулируем это, используя массив байт intermediateCRCBytesLE
            for (int k = 3; k >= 0; --k) { // Цикл по 4 байтам CRC
                patchValue = (patchValue << 8) ^ inline_revTable[patchValue >> 24] ^ static_cast<quint32>(intermediateCRCBytesLE[k]);
            }
            //-------------------------------

            // Ручная запись патча (patchValue) В КОНЕЦ БУФЕРА
            // Увеличиваем размер буфера на 4 байта
            currentFirmware.resize(currentFileSize + 4);
            unsigned char* patchDest = reinterpret_cast<unsigned char*>(currentFirmware.data() + currentFileSize); // Указатель на место для патча
            // Записываем патч в Little Endian
            patchDest[0] = (patchValue >> 0)  & 0xFF;
            patchDest[1] = (patchValue >> 8)  & 0xFF;
            patchDest[2] = (patchValue >> 16) & 0xFF;
            patchDest[3] = (patchValue >> 24) & 0xFF;

            // Обновляем размер файла
            currentFileSize += 4;

#ifdef QT_DEBUG // Проверка CRC
            QByteArray finalDataView = QByteArray::fromRawData(currentFirmware.constData(), currentFileSize);
            quint32 finalCRC = CrcUnit::calcCrc32(finalDataView);
            if (finalCRC != requiredCRC) {
                qWarning() << "CRC MISMATCH S/N" << serialStr << "! Req:" << QString::number(requiredCRC, 16).toUpper().rightJustified(8, '0')
                << " Calc:" << QString::number(finalCRC, 16).toUpper().rightJustified(8, '0')
                << " Patch:" << QString::number(patchValue, 16).toUpper().rightJustified(8, '0');
            } else {
                qDebug() << "CRC OK S/N" << serialStr << "Target:" << QString::number(requiredCRC, 16).toUpper().rightJustified(8,'0');
            }
#endif
        } else {
            // Если CRC не обновляем, ОБРЕЗАЕМ буфер до РЕАЛЬНОГО размера данных
            currentFirmware.resize(currentFileSize);
        }

        // 6d. Сохраняем файл
        QString baseName = ui->cmbRevision->currentText();
        if (baseName.isEmpty() || ui->cmbRevision->currentIndex() < 0 || baseName == "OthDev") { baseName = "Firmware"; }
        baseName.replace(QRegularExpression(R"([\\/:*?"<>|])"), "_"); // Очистка имени
        QString fileName = QString("%1-Ldr+Prog-sn-%2.bin").arg(baseName).arg(serialStr);
        QString fullOutPath = dir.filePath(fileName);

        QFile outFile(fullOutPath);
        if (!outFile.open(QIODevice::WriteOnly)) { QMessageBox::critical(ui->cmbRevision->window(), "Ошибка", "Ошибка открытия файла для записи: " + outFile.fileName() + "\nПричина: " + outFile.errorString()); return; }
        // Записываем ТОЛЬКО РЕАЛЬНЫЙ РАЗМЕР ДАННЫХ (currentFileSize)
        qint64 bytesWritten = outFile.write(currentFirmware.constData(), currentFileSize);
        outFile.close();
        if (bytesWritten != currentFileSize) { QMessageBox::critical(ui->cmbRevision->window(), "Ошибка", "Ошибка записи в файл (не все байты записаны): " + outFile.fileName()); outFile.remove(); return; }
        filesCreated++;

    } // Конец цикла for

    // --- 7. Сообщение о завершении (Оригинальное) ---
    if (filesCreated > 0) { QMessageBox::information(ui->cmbRevision->window(), "Готово", QStringLiteral("%1 файл(ов) прошивки успешно создан(о) в папке:\n%2").arg(filesCreated).arg(QDir::toNativeSeparators(outputDir))); }
    else if (serialCount > 0) { QMessageBox::warning(ui->cmbRevision->window(), "Завершено", "Не было создано ни одного файла (возможно, из-за ошибок)."); }
}
// --- Конец функции createFirmwareFiles ---


// Функция generateCRCTables из оригинала
void Unit1::generateCRCTables(quint32* fwdTable, quint32* revTable)
{
    const quint32 poly = 0xEDB88320; // Отраженный полином
    for(int i = 0; i < 256; ++i)
    {
        quint32 crcFwd = static_cast<quint32>(i);
        quint32 crcRev = static_cast<quint32>(i) << 24;
        for(int j = 0; j < 8; ++j) // Или for(int j = 8; j > 0; --j)
        {
            crcFwd = (crcFwd >> 1) ^ ((crcFwd & 1) ? poly : 0); // Прямой расчет
            // Обратный расчет - ТОЧНОЕ СООТВЕТСТВИЕ DELPHI
            if ((crcRev & 0x80000000) != 0) { crcRev = ((crcRev ^ poly) << 1) | 1; }
            else { crcRev <<= 1; }
        }
        fwdTable[i] = crcFwd;
        revTable[i] = crcRev; // Сохраняем правильно сгенерированное значение
    }
}


// Функция calculateReverseCRC из оригинала (если она нужна для чего-то еще)
// Если она использовалась ТОЛЬКО в createFirmwareFiles, то она больше не нужна,
// т.к. логика патча встроена в createFirmwareFiles.
// quint32 Unit1::calculateReverseCRC(const QByteArray &data_with_placeholders, quint32 targetCRC, const quint32* revTable)
// {
//     quint32 desired = targetCRC ^ 0xFFFFFFFF;
//     const unsigned char* bufferPtr = reinterpret_cast<const unsigned char*>(data_with_placeholders.constData());
//     int size = data_with_placeholders.size(); // data + 4 placeholders
//     for(int i = size - 1; i >= 0; --i) {
//         desired = (desired << 8) ^ revTable[(desired >> 24) ^ bufferPtr[i]];
//     }
//     return desired;
// }


// Функция loadFile из оригинала
QByteArray Unit1::loadFile(const QString &filePath) // Ожидает абсолютный путь
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Error opening file for reading:" << filePath << file.errorString();
        return QByteArray();
    }
    QByteArray data = file.readAll();
    file.close();
    // Добавим проверку на случай ошибки чтения
    if (data.isEmpty() && QFileInfo(filePath).size() > 0) {
        qWarning() << "Read 0 bytes from non-empty file:" << filePath;
    }
    return data;
}

// Функция onBtnCreateFileManualClicked из оригинала
void Unit1::onBtnCreateFileManualClicked()
{
    QString dir = QFileDialog::getExistingDirectory(ui->cmbRevision->window(), tr("Выберите папку для сохранения прошивок"), QDir::currentPath());
    if (dir.isEmpty()) return;
    createFirmwareFiles(dir); // Передаем выбранный АБСОЛЮТНЫЙ путь
}

// Функция onBtnCreateFileAutoClicked из оригинала
void Unit1::onBtnCreateFileAutoClicked()
{
    // Используем SaveFirmware, сохраненный в onRevisionChanged
    if (SaveFirmware.isEmpty()) {
        QMessageBox::warning(ui->cmbRevision->window(), "Внимание", "Путь для авто-сохранения не определен для текущей ревизии.");
        return;
    }
    QString currentDir = QDir::currentPath();
    // Собираем АБСОЛЮТНЫЙ путь
    QString outDirAbs = QDir(currentDir).filePath(SaveFirmware);

    // Создаем папку если ее нет
    QDir dir(outDirAbs);
    if (!dir.exists() && !dir.mkpath(".")) {
        QMessageBox::critical(ui->cmbRevision->window(), "Ошибка", "Не удалось создать папку для авто-сохранения:\n" + QDir::toNativeSeparators(outDirAbs));
        return;
    }

    createFirmwareFiles(outDirAbs); // Передаем АБСОЛЮТНЫЙ путь
}
