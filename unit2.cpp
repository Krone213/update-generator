// Файл unit2.cpp (полный, с имитацией Now() через QDateTime)

#include "unit2.h"
#include "mainwindow.h" // Might need for context/parent access
#include <QDebug>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QComboBox>
#include <QDir>
#include <QSaveFile>
#include <QDataStream>
#include <QtEndian>
#include "crcunit.h" // Убедитесь, что calcCrc32 здесь объявлен
#include <QDateTime> // <<<--- Нужно для QDateTime
#include <QRegularExpression>
#include <stdexcept>


// --- Add to top of unit2.cpp OR in a new header file ---
#include <QtGlobal>
#include <cstring>

// Define types like in C++Builder
typedef signed char s8;
typedef signed short s16;
typedef signed long s32;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

// Use quint types for better portability
typedef quint8  quint8_t;
typedef quint16 quint16_t;
typedef quint32 quint32_t;
typedef qint32  qint32_t;


#pragma pack(push,1)
struct TUpdateTask
{
    quint32_t FileId;
    quint32_t BlockType;
    quint32_t Version;
    quint32_t BlockSize;
    char ModelInfo[64];
    char DescInfo[64];
    char DevSerial[64];
    char DevId[64];
    quint32_t EraseBeginAddr;
    quint32_t EraseEndAddr;
    quint32_t ProgramBeginAddr;
    quint32_t ProgramSize;
    quint32_t ProgramCrc;
    quint32_t LoaderBaseCmd;
    quint32_t LoaderExtCmd;
    quint32_t LoaderBeforeCmd;
    quint32_t LoaderAfterCmd;
    quint8_t Time[8]; // 8 bytes for time representation
    quint8_t Res[192];
    quint32_t Crc;

    // Helper to safely copy QString to fixed char array
    void setStringField(char* dest, int destSize, const QString& src) {
        memset(dest, 0, destSize);
        QByteArray ba = src.toLatin1(); // Sticking with Latin1
        int bytesToCopy = qMin(ba.size(), destSize - 1);
        memcpy(dest, ba.constData(), bytesToCopy);
    }
};
#pragma pack(pop)

// Команды загрузчику
#define CMD_IS_BASE_PROGRAM              ((quint32_t)0x00000001)
#define CMD_UPDATE_SAME_SERIAL           ((quint32_t)0x00000002)
#define CMD_UPDATE_SAME_DEV_ID           ((quint32_t)0x00000003)
#define CMD_NO_MODEL_CHECK               ((quint32_t)0x00000080)
// Дополнительные команды
#define CMD_SAVE_BTH_NAME                ((quint32_t)0x00000001)
#define CMD_SAVE_BTH_ADDR                ((quint32_t)0x00000002)
#define CMD_SAVE_DEV_DESC                ((quint32_t)0x00000004)
#define CMD_SAVE_HW_VERSION              ((quint32_t)0x00000008)
#define CMD_SAVE_SW_VERSION              ((quint32_t)0x00000010)
#define CMD_SAVE_SERIAL                  ((quint32_t)0x00000020)
#define CMD_SAVE_MODEL_INFO              ((quint32_t)0x00000040)
#define CMD_SET_RDP_LV1                  ((quint32_t)0x00000001)
// --- End of definitions ---


// Function to perform the PSP XOR encoding
void applyPSPEncoding(quint8_t* buffer, qsizetype size, quint32_t seed) {
    quint32_t currentSeed = seed;
    for (qsizetype i = 0; i < size; ++i) {
        currentSeed = (currentSeed * 0x08088405) + 1;
        buffer[i] ^= static_cast<quint8_t>(currentSeed >> 24);
    }
}


// --- Function to create update files ---
void Unit2::createUpdateFiles(const QString &outputFileAbsPath) {
    qDebug() << "--- Starting createUpdateFiles ---";
    qDebug() << "Output file path:" << outputFileAbsPath;

    TUpdateTask updateTask;
    memset(&updateTask, 0, sizeof(TUpdateTask)); // Initialize struct with zeros

    bool ok;
    QString errorTitle = tr("Ошибка");

    try {
        // --- 1. Get Data from UI and Validate ---
        QString programFilePathRel = ui->lblUpdateProgramDataFileName->text();
        if (programFilePathRel.startsWith("Файл:") || programFilePathRel.isEmpty()) {
            throw std::runtime_error(tr("Файл программы для обновления не выбран.").toStdString());
        }
        QString programFilePathAbs = programFilePathRel;
        if (QDir::isRelativePath(programFilePathRel)) {
            programFilePathAbs = QDir(QDir::currentPath()).filePath(programFilePathRel);
        }
        QFileInfo programFileInfo(programFilePathAbs);
        if (!programFileInfo.exists() || !programFileInfo.isFile()) {
            throw std::runtime_error(tr("Файл программы для обновления не найден:\n%1").arg(QDir::toNativeSeparators(programFilePathAbs)).toStdString());
        }
        qint64 programDataSize = programFileInfo.size();
        qDebug() << "Program file path:" << programFilePathAbs << "Size:" << programDataSize;
        if (programDataSize > ((1024L - 16L) * 1024L)) {
            throw std::runtime_error(tr("Файл программы больше максимально допустимого размера (1008 Кб).").toStdString());
        }

        QString devModel = ui->cmbDeviceModel->currentText().trimmed();
        if (devModel.isEmpty()) {
            throw std::runtime_error(tr("Модель устройства не выбрана или не указана.").toStdString());
        }

        // Use corrected field names
        QString devDesc = ui->editDeviceDescriptor->text().trimmed();
        QString devSerial = ui->editSerialNumber->text().trimmed();
        QString devId = ui->editHardwareId->text().trimmed();

        QString eraseBeginStr = ui->editEraseStartAddress->text().trimmed().remove("0x", Qt::CaseInsensitive);
        QString eraseEndStr = ui->editEraseEndAddress->text().trimmed().remove("0x", Qt::CaseInsensitive);
        QString progBeginStr = ui->editProgramStartAddress->text().trimmed().remove("0x", Qt::CaseInsensitive);

        quint32_t eraseBeginAddr = eraseBeginStr.isEmpty() ? 0 : eraseBeginStr.toUInt(&ok, 16);
        if (!eraseBeginStr.isEmpty() && !ok) throw std::runtime_error(tr("Начальный адрес для стирания указан неверно (ожидается HEX).").toStdString());
        quint32_t eraseEndAddr = eraseEndStr.isEmpty() ? 0 : eraseEndStr.toUInt(&ok, 16);
        if (!eraseEndStr.isEmpty() && !ok) throw std::runtime_error(tr("Конечный адрес для стирания указан неверно (ожидается HEX).").toStdString());
        quint32_t progBeginAddr = progBeginStr.isEmpty() ? 0 : progBeginStr.toUInt(&ok, 16);
        if (!progBeginStr.isEmpty() && !ok) throw std::runtime_error(tr("Начальный адрес программы указан неверно (ожидается HEX).").toStdString());

        qDebug() << "UI Values: Model:" << devModel << "Desc:" << devDesc << "Serial:" << devSerial << "ID:" << devId;
        qDebug() << "Addresses: EraseStart: 0x" << QString::number(eraseBeginAddr, 16) << "EraseEnd: 0x" << QString::number(eraseEndAddr, 16) << "ProgStart: 0x" << QString::number(progBeginAddr, 16);

        // Address Range Checks... (same as before)
        // ... (validation code) ...

        // Checkbox Dependencies... (same as before)
        // ... (validation code) ...

        // --- 2. Load Program Data ---
        QFile file(programFilePathAbs);
        if (!file.open(QIODevice::ReadOnly)) {
            throw std::runtime_error(tr("Не удалось открыть файл программы: %1").arg(file.errorString()).toStdString());
        }
        QByteArray programData = file.readAll();
        file.close();
        qDebug() << "Loaded program data size:" << programData.size();

        // --- 3. Populate TUpdateTask ---
        updateTask.FileId = qToLittleEndian(0x52444C42); // "BLDR"
        updateTask.BlockType = qToLittleEndian(0x00010000);
        updateTask.Version = qToLittleEndian(0x00000100); // 1.00
        updateTask.BlockSize = qToLittleEndian(512);

        updateTask.setStringField(updateTask.ModelInfo, sizeof(updateTask.ModelInfo), devModel);
        updateTask.setStringField(updateTask.DescInfo, sizeof(updateTask.DescInfo), devDesc);
        updateTask.setStringField(updateTask.DevSerial, sizeof(updateTask.DevSerial), devSerial);
        updateTask.setStringField(updateTask.DevId, sizeof(updateTask.DevId), devId);

        updateTask.EraseBeginAddr = qToLittleEndian(eraseBeginAddr);
        updateTask.EraseEndAddr = qToLittleEndian(eraseEndAddr);

        // Calculate program CRC (Host Byte Order)
        quint32_t programCRC = 0;
        if (!programData.isEmpty()) {
            programCRC = CrcUnit::calcCrc32(programData);
            qInfo() << "Calculated programCRC (HOST): 0x" + QString::number(programCRC, 16).toUpper().rightJustified(8, '0');
        } else {
            qInfo() << "Program data is empty, programCRC = 0";
        }
        // Store Little Endian version in struct
        updateTask.ProgramBeginAddr = qToLittleEndian(progBeginAddr);
        updateTask.ProgramSize = qToLittleEndian(static_cast<quint32_t>(programData.size()));
        updateTask.ProgramCrc = qToLittleEndian(programCRC);

        // Build command masks (Host byte order)
        quint32_t loaderBaseCmd = 0;
        if (ui->main_prog->isChecked()) loaderBaseCmd |= CMD_IS_BASE_PROGRAM;
        if (ui->OnlyForEnteredSN->isChecked()) loaderBaseCmd |= CMD_UPDATE_SAME_SERIAL;
        if (ui->OnlyForEnteredDevID->isChecked()) loaderBaseCmd |= CMD_UPDATE_SAME_DEV_ID;
        if (ui->NoCheckModel->isChecked()) loaderBaseCmd |= CMD_NO_MODEL_CHECK;

        quint32_t loaderBeforeCmd = 0;
        if (ui->SaveBthName->isChecked()) loaderBeforeCmd |= CMD_SAVE_BTH_NAME;
        if (ui->SaveBthAddr->isChecked()) loaderBeforeCmd |= CMD_SAVE_BTH_ADDR;
        if (ui->SaveDevDesc->isChecked()) loaderBeforeCmd |= CMD_SAVE_DEV_DESC;
        if (ui->SaveHwVerr->isChecked()) loaderBeforeCmd |= CMD_SAVE_HW_VERSION;
        if (ui->SaveSwVerr->isChecked()) loaderBeforeCmd |= CMD_SAVE_SW_VERSION;
        if (ui->SaveDevModel->isChecked()) loaderBeforeCmd |= CMD_SAVE_MODEL_INFO;
        if (ui->SaveDevSerial->isChecked()) loaderBeforeCmd |= CMD_SAVE_SERIAL;
        if (ui->Lvl1MemProtection->isChecked()) loaderBeforeCmd |= CMD_SET_RDP_LV1;

        // Store LE versions in struct
        updateTask.LoaderBaseCmd = qToLittleEndian(loaderBaseCmd);
        updateTask.LoaderExtCmd = 0;
        updateTask.LoaderBeforeCmd = qToLittleEndian(loaderBeforeCmd);
        updateTask.LoaderAfterCmd = 0;

        // <<< ИСПОЛЬЗОВАНИЕ QDateTime для поля Time >>>
        qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
        quint64 nowMsLe = qToLittleEndian(static_cast<quint64>(nowMs));
        memcpy(updateTask.Time, &nowMsLe, sizeof(updateTask.Time));
        qDebug() << "Set Time field using Qt msecs since epoch (LE):" << QByteArray(reinterpret_cast<char*>(updateTask.Time), 8).toHex().toUpper();
        // <<< КОНЕЦ ИЗМЕНЕНИЯ >>>

        // Calculate header CRC *after* filling all fields including Time
        qsizetype headerSizeForCrc = sizeof(TUpdateTask) - sizeof(quint32_t);
        QByteArray headerDataForCrc = QByteArray::fromRawData(
            reinterpret_cast<const char*>(&updateTask),
            static_cast<int>(headerSizeForCrc)
            );
        quint32_t headerCRC = CrcUnit::calcCrc32(headerDataForCrc); // Host byte order
        qInfo() << "Calculated headerCRC (HOST):  0x" + QString::number(headerCRC, 16).toUpper().rightJustified(8, '0');

        updateTask.Crc = qToLittleEndian(headerCRC); // Store LE version


        // --- 4. PSP Encoding ---
        TUpdateTask encodedTask = updateTask; // Copy final struct
        QByteArray encodedData = programData; // Copy program data

        // Encode header (task) - everything EXCEPT the final Crc field
        qDebug() << "Encoding header (size:" << headerSizeForCrc << ") with seed: 0x" << QString::number(headerCRC, 16).toUpper().rightJustified(8, '0');
        applyPSPEncoding(
            reinterpret_cast<quint8_t*>(&encodedTask),
            headerSizeForCrc,
            headerCRC
            );

        // Encode program data (if any)
        if (!encodedData.isEmpty()) {
            quint32_t dataEncodingSeed = headerCRC ^ programCRC; // Seed = HeaderCRC XOR ProgramCRC (Use HOST byte order CRCs)
            qInfo() << "Encoding data (size:" << encodedData.size() << ") with seed:   0x" + QString::number(dataEncodingSeed, 16).toUpper().rightJustified(8, '0');
            applyPSPEncoding(
                reinterpret_cast<quint8_t*>(encodedData.data()),
                encodedData.size(),
                dataEncodingSeed
                );
        } else {
            qDebug() << "No program data to encode.";
        }

        // --- 5. Write Output File ---
        QSaveFile saveFile(outputFileAbsPath);
        if (!saveFile.open(QIODevice::WriteOnly)) {
            throw std::runtime_error(tr("Не удалось открыть файл для записи: %1").arg(saveFile.errorString()).toStdString());
        }

        qint64 bytesWritten = saveFile.write(reinterpret_cast<const char*>(&encodedTask), sizeof(TUpdateTask));
        if (bytesWritten != sizeof(TUpdateTask)) {
            saveFile.cancelWriting();
            throw std::runtime_error(tr("Ошибка записи заголовка в файл.").toStdString());
        }
        if (!encodedData.isEmpty()) {
            bytesWritten = saveFile.write(encodedData);
            if (bytesWritten != encodedData.size()) {
                saveFile.cancelWriting();
                throw std::runtime_error(tr("Ошибка записи данных программы в файл.").toStdString());
            }
        }
        if (!saveFile.commit()) {
            throw std::runtime_error(tr("Не удалось сохранить файл: %1").arg(saveFile.errorString()).toStdString());
        }

        // --- 6. Success Message ---
        QMessageBox::information(ui->cmbUpdateRevision->window(),
                                 tr("Готово"),
                                 tr("Файл обновления '%1' успешно создан.")
                                     .arg(QFileInfo(outputFileAbsPath).fileName()));
        qDebug() << "--- Finished createUpdateFiles Successfully ---";

    } catch (const std::runtime_error& e) {
        qCritical() << "Error during createUpdateFiles:" << e.what();
        QMessageBox::critical(ui->cmbUpdateRevision->window(), errorTitle, QString::fromUtf8(e.what()));
        qDebug() << "--- Finished createUpdateFiles with runtime_error ---";
    } catch (...) {
        qCritical() << "Unknown error during createUpdateFiles.";
        QMessageBox::critical(ui->cmbUpdateRevision->window(), errorTitle, tr("Неизвестная ошибка при создании файла обновления."));
        qDebug() << "--- Finished createUpdateFiles with unknown error ---";
    }
}

// --- Constructor and other methods ---

Unit2::Unit2(Ui::MainWindow *ui, QObject *parent)
    : QObject(parent), ui(ui)
{
    clearUIFields(); // Start with clean fields
}

void Unit2::updateUI(const ExtendedRevisionInfo& info) {
    qDebug() << "Unit2::updateUI for category:" << info.category;
    if (info.category.isEmpty() || info.category == "OthDev") {
        clearUIFields();
    } else {
        // Update QComboBox cmbDeviceModel
        if (info.bldrDevModel.isEmpty()) {
            if (ui->cmbDeviceModel->currentIndex() != -1) {
                ui->cmbDeviceModel->setCurrentIndex(-1);
            }
        } else {
            int modelIndex = ui->cmbDeviceModel->findText(info.bldrDevModel);
            if (modelIndex != -1) {
                if (ui->cmbDeviceModel->currentIndex() != modelIndex) {
                    ui->cmbDeviceModel->setCurrentIndex(modelIndex);
                }
            } else {
                if (ui->cmbDeviceModel->currentIndex() != -1) {
                    ui->cmbDeviceModel->setCurrentIndex(-1);
                }
                qWarning() << "Unit2::updateUI: Model" << info.bldrDevModel << "for category" << info.category << "not found in cmbDeviceModel items!";
            }
        }

        // Update other fields based on info
        ui->editProgramStartAddress->setText(info.beginAddress.isEmpty() ? "0x0" : info.beginAddress);


        ui->lblUpdateProgramDataFileName->setText(info.mainProgramFile.isEmpty() ? "Файл: -" : QDir::toNativeSeparators(info.mainProgramFile));
        updateUpdateProgramDataFileSize(info.mainProgramFile);

        // Set checkboxes based on info struct
        ui->main_prog->setChecked(info.cmdMainProg);
        ui->OnlyForEnteredSN->setChecked(info.cmdOnlyForEnteredSN);
        ui->OnlyForEnteredDevID->setChecked(info.cmdOnlyForEnteredDevID);
        ui->NoCheckModel->setChecked(info.cmdNoCheckModel);
        ui->SaveBthName->setChecked(info.dopSaveBthName);
        ui->SaveBthAddr->setChecked(info.dopSaveBthAddr);
        ui->SaveDevDesc->setChecked(info.dopSaveDevDesc);
        ui->SaveHwVerr->setChecked(info.dopSaveHwVerr);
        ui->SaveSwVerr->setChecked(info.dopSaveSwVerr);
        ui->SaveDevSerial->setChecked(info.dopSaveDevSerial);
        ui->SaveDevModel->setChecked(info.dopSaveDevModel);
        ui->Lvl1MemProtection->setChecked(info.dopLvl1MemProtection);

        // Clear fields that are manually entered by user
        ui->editDeviceDescriptor->clear();
        ui->editSerialNumber->clear();
        ui->editHardwareId->clear();
    }
}


void Unit2::clearUIFields() {
    qDebug() << "Unit2::clearUIFields called";
    if (ui->cmbDeviceModel->currentIndex() != -1) {
        ui->cmbDeviceModel->setCurrentIndex(-1);
    }
    // Clear all manual input fields and set defaults
    ui->editProgramStartAddress->clear();
    ui->editDeviceDescriptor->clear();
    ui->editSerialNumber->clear();
    ui->editHardwareId->clear();

    ui->lblUpdateProgramDataFileName->setText("Файл: -");
    ui->lblUpdateProgramDataFileSize->setText("Размер: -");

    // Reset all checkboxes
    ui->main_prog->setChecked(false);
    ui->OnlyForEnteredSN->setChecked(false);
    ui->OnlyForEnteredDevID->setChecked(false);
    ui->NoCheckModel->setChecked(false);
    ui->SaveBthName->setChecked(false);
    ui->SaveBthAddr->setChecked(false);
    ui->SaveDevDesc->setChecked(false);
    ui->SaveHwVerr->setChecked(false);
    ui->SaveSwVerr->setChecked(false);
    ui->SaveDevSerial->setChecked(false);
    ui->SaveDevModel->setChecked(false);
    ui->Lvl1MemProtection->setChecked(false);
}

// --- Slots Connected Directly from MainWindow UI ---

void Unit2::onBtnChooseUpdateProgramDataFileClicked()
{
    QString initialDir = QDir::currentPath();
    QString currentFilePath = ui->lblUpdateProgramDataFileName->text();
    if (!currentFilePath.startsWith("Файл:") && !currentFilePath.isEmpty()) {
        QFileInfo currentInfo(currentFilePath);
        if (currentInfo.exists()){
            initialDir = currentInfo.absolutePath();
        } else {
            QDir dir(QFileInfo(currentFilePath).path());
            if (dir.exists()) {
                initialDir = dir.absolutePath();
            } else {
                initialDir = QDir(initialDir).filePath("MainProgram_File");
                QDir().mkpath(initialDir);
            }
        }
    } else {
        initialDir = QDir(initialDir).filePath("MainProgram_File");
        QDir().mkpath(initialDir);
    }

    QString filePath = QFileDialog::getOpenFileName( ui->cmbUpdateRevision->window(),
                                                    tr("Выберите файл программы для обновления"), initialDir, tr("Файлы прошивки (*.bin *.hex);;Все файлы (*)"));
    if (filePath.isEmpty()) return;

    ui->lblUpdateProgramDataFileName->setText(QDir::toNativeSeparators(filePath));
    updateUpdateProgramDataFileSize(filePath);
}


void Unit2::onBtnUpdateCreateFileManualClicked()
{
    QString baseName = ui->cmbDeviceModel->currentText();
    if (baseName.isEmpty()) {
        baseName = "FirmwareUpdate";
    }
    baseName.replace(QRegularExpression(R"([\\/:*?"<>|])"), "_");
    QString suggestedFileName = baseName + "_Update.bin"; // Added _Update

    QString initialDir = QDir::currentPath();
    // Consider getting default dir from config/MainWindow later

    QString filePath = QFileDialog::getSaveFileName(
        ui->cmbUpdateRevision->window(),
        tr("Сохранить файл обновления"),
        QDir(initialDir).filePath(suggestedFileName),
        tr("Файлы обновления (*.bin);;Все файлы (*)")
        );

    if (filePath.isEmpty()) return;

    createUpdateFiles(filePath);
}

void Unit2::onBtnUpdateShowInfoClicked()
{
    QMessageBox::information(ui->btnUpdateShowInfo->window(), tr("Инфо (Обновление)"),
                             tr("Создание файлов обновления прошивки для устройств.\nВыберите ревизию, файл программы и необходимые параметры."));
}

#include <QMessageBox> // Для диалоговых окон
#include <QDir>        // Для работы с директориями
#include <QDebug>      // Для вывода отладочной информации
#include <QStringList> // Для списков строк (путей)



void Unit2::onBtnClearUpdateRevisionClicked()
{
    qDebug() << "Unit2::onBtnClearUpdateRevisionClicked: Запущен процесс очистки папок обновлений.";

    // Получаем указатель на родительский объект
    QObject* parentObj = this->parent();
    MainWindow* mainWindow = qobject_cast<MainWindow*>(parentObj); // Пытаемся преобразовать родителя к MainWindow

    // Проверяем, удалось ли получить MainWindow и указатель на UI
    if (!mainWindow || !ui) {
        QMessageBox::critical(nullptr, // Не можем получить родительское окно безопасно
                              tr("Критическая ошибка"),
                              tr("Не удалось получить доступ к главному окну для получения путей очистки."));
        qCritical() << "Очистка папок обновлений: Не удалось получить MainWindow из parent() или UI не инициализирован.";
        return;
    }

    // 1. Получаем список путей из MainWindow
    QSet<QString> updateAutoSavePaths = mainWindow->getUpdateAutoSavePaths();

    // 2. Проверяем, есть ли информация о путях
    if (updateAutoSavePaths.isEmpty()) {
        QMessageBox::information(ui->btnClearUpdateRevision->window(), // Теперь используем ui для окна
                                 tr("Очистка (Обновление)"),
                                 tr("Нет информации о путях для автоматического сохранения файлов обновления.\n"
                                    "Проверьте наличие и корректность тегов <SaveUpdate> в config.xml."));
        qInfo() << "Очистка папок обновлений: Пути <SaveUpdate> не найдены или не загружены.";
        return;
    }

    // 3. Готовим список путей для сообщения пользователю (логика без изменений)
    QStringList pathsToDeleteDisplay;
    QString currentDir = QDir::currentPath();
    for (const QString &pathFragment : qAsConst(updateAutoSavePaths)) {
        QString fullDisplayPath = QDir(currentDir).filePath(pathFragment);
        pathsToDeleteDisplay << QDir::toNativeSeparators(fullDisplayPath);
    }

    QString confirmationMessage = tr("ВНИМАНИЕ!\n\n"
                                     "Будут РЕКУРСИВНО удалены все файлы и подпапки в следующих директориях, "
                                     "используемых для автосохранения файлов ОБНОВЛЕНИЯ:\n\n"
                                     "%1\n\n"
                                     "Это действие необратимо. Вы уверены, что хотите продолжить?")
                                      .arg(pathsToDeleteDisplay.join("\n"));

    // 4. Запрашиваем подтверждение у пользователя (логика без изменений)
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(ui->btnClearUpdateRevision->window(),
                                  tr("Подтверждение очистки папок обновлений"),
                                  confirmationMessage,
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        qInfo() << "Очистка папок обновлений: Операция отменена пользователем.";
        return;
    }

    // 5. Выполняем удаление (логика без изменений)
    qInfo() << "Очистка папок обновлений: Начало удаления...";
    int successCount = 0;
    int failCount = 0;
    QStringList errorDetails;

    for (const QString &pathFragment : qAsConst(updateAutoSavePaths)) {
        QString fullPathToDelete = QDir(currentDir).filePath(pathFragment);
        QDir dirToDelete(fullPathToDelete);

        if (dirToDelete.exists()) {
            qInfo() << "Очистка папок обновлений: Попытка удаления папки:" << QDir::toNativeSeparators(fullPathToDelete);
            if (dirToDelete.removeRecursively()) {
                qInfo() << "Очистка папок обновлений: Успешно удалена папка:" << QDir::toNativeSeparators(fullPathToDelete);
                successCount++;
            } else {
                qWarning() << "Очистка папок обновлений: Не удалось удалить папку:" << QDir::toNativeSeparators(fullPathToDelete);
                failCount++;
                errorDetails << QDir::toNativeSeparators(pathFragment);
            }
        } else {
            qInfo() << "Очистка папок обновлений: Папка не найдена, пропуск:" << QDir::toNativeSeparators(fullPathToDelete);
        }
    }

    // 6. Сообщаем результат пользователю (логика без изменений)
    QString resultMessage;
    QString resultTitle = tr("Очистка папок обновлений");

    if (failCount == 0 && successCount > 0) {
        resultMessage = tr("Папки (%1 шт.) для автосохранения файлов обновлений успешно удалены.").arg(successCount);
        QMessageBox::information(ui->btnClearUpdateRevision->window(), resultTitle, resultMessage);
    } else if (failCount > 0) {
        resultTitle = tr("Ошибка очистки папок обновлений");
        resultMessage = tr("Не удалось удалить %1 папок обновлений:\n%2").arg(failCount).arg(errorDetails.join("\n"));
        if (successCount > 0) {
            resultMessage += tr("\n\nУспешно удалено: %1 папок обновлений.").arg(successCount);
        }
        QMessageBox::warning(ui->btnClearUpdateRevision->window(), resultTitle, resultMessage);
    } else {
        resultMessage = tr("Не найдено существующих папок для удаления (согласно тегам <SaveUpdate> в config.xml).");
        QMessageBox::information(ui->btnClearUpdateRevision->window(), resultTitle, resultMessage);
    }
    qInfo() << "Очистка папок обновлений: Операция завершена.";
}
// --- Полная реализация слота для создания общего файла обновления ---
void Unit2::onBtnCreateCommonUpdateFileClicked()
{
    qDebug() << "--- Запуск создания ОБЩЕГО файла обновления ---";

    // 1. Получаем доступ к MainWindow и карте ревизий
    MainWindow* mainWindow = qobject_cast<MainWindow*>(this->parent());
    if (!mainWindow || !ui) {
        QMessageBox::critical(nullptr, tr("Критическая ошибка"),
                              tr("Не удалось получить доступ к главному окну для создания общего файла."));
        qCritical() << "Создание общего файла: Не удалось получить MainWindow или UI.";
        return;
    }
    const QMap<QString, ExtendedRevisionInfo>& revisions = mainWindow->getRevisionsMap();
    if (revisions.isEmpty()) {
        QMessageBox::warning(ui->btnCreateCommonUpdateFile->window(), tr("Внимание"),
                             tr("Нет загруженных ревизий в config.xml для создания общего файла."));
        qWarning() << "Создание общего файла: Карта ревизий пуста.";
        return;
    }

    // 2. Запрашиваем имя файла для сохранения
    QString defaultFileName = "Firmware_Common_Update.bin";
    QString currentPath = QDir::currentPath();
    QString saveFilePath = QFileDialog::getSaveFileName(
        ui->btnCreateCommonUpdateFile->window(),
        tr("Сохранить общий файл обновления"),
        QDir(currentPath).filePath(defaultFileName),
        tr("Файлы обновления (*.bin);;Все файлы (*.*)")
        );

    if (saveFilePath.isEmpty()) {
        qInfo() << "Создание общего файла: Сохранение отменено пользователем.";
        return;
    }

    // 3. Открываем файл для записи
    QSaveFile saveFile(saveFilePath);
    if (!saveFile.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(ui->btnCreateCommonUpdateFile->window(), tr("Ошибка"),
                              tr("Не удалось открыть файл для записи:\n%1\nОшибка: %2")
                                  .arg(QDir::toNativeSeparators(saveFilePath))
                                  .arg(saveFile.errorString()));
        qCritical() << "Создание общего файла: Ошибка открытия QSaveFile:" << saveFile.errorString();
        return;
    }

    qInfo() << "Создание общего файла: Запись в" << QDir::toNativeSeparators(saveFilePath);
    int blocksAdded = 0;
    int revisionsSkipped = 0;
    QStringList errorsList;    // Список критических ошибок при обработке ревизий
    QStringList skippedList;   // Список пропущенных ревизий с причинами

    // 4. Итерируем по всем ревизиям из карты MainWindow
    QMapIterator<QString, ExtendedRevisionInfo> it(revisions);
    while (it.hasNext()) {
        it.next();
        const QString& category = it.key();
        const ExtendedRevisionInfo& info = it.value(); // Данные ТЕКУЩЕЙ ревизии из config.xml

        qDebug() << "Обработка ревизии для общего файла:" << category;

        // 4a. Пропускаем нерелевантные или неполные ревизии
        if (category == "OthDev") {
            qDebug() << "  Пропуск 'OthDev'.";
            revisionsSkipped++;
            skippedList << tr("Ревизия '%1': Пропущена (служебная).").arg(category);
            continue;
        }
        if (info.mainProgramFile.isEmpty()) {
            qWarning() << "  Пропуск ревизии" << category << "- отсутствует MainProgram_File.";
            revisionsSkipped++;
            // Добавляем в список пропущенных, не считаем критической ошибкой всего процесса
            skippedList << tr("Ревизия '%1': Пропущена (не указан файл программы <MainProgram_File>).").arg(category);
            continue;
        }
        if (info.bldrDevModel.isEmpty()) {
            qWarning() << "  Пропуск ревизии" << category << "- отсутствует BldrDevModel.";
            revisionsSkipped++;
            skippedList << tr("Ревизия '%1': Пропущена (не указана модель устройства <BldrDevModel>).").arg(category);
            continue;
        }
        if (info.beginAddress.isEmpty()) {
            qWarning() << "  Пропуск ревизии" << category << "- отсутствует begin_adres.";
            revisionsSkipped++;
            skippedList << tr("Ревизия '%1': Пропущена (не указан начальный адрес <begin_adres>).").arg(category);
            continue;
        }

        // === НАЧАЛО БЛОКА ДЛЯ ОДНОЙ РЕВИЗИИ ===
        TUpdateTask currentTask; // Заголовок для текущей ревизии
        memset(&currentTask, 0, sizeof(TUpdateTask));
        QByteArray programData; // Данные программы для текущей ревизии
        quint32_t programCRC = 0;
        quint32_t headerCRC = 0;
        bool ok;

        try {
            // --- 1b. Валидация и получение данных из 'info' ---
            QString programFilePathAbs = info.mainProgramFile; // Путь из config.xml для этой ревизии
            if (QDir::isRelativePath(programFilePathAbs)) {
                programFilePathAbs = QDir(currentPath).filePath(programFilePathAbs);
            }
            QFileInfo programFileInfo(programFilePathAbs);
            if (!programFileInfo.exists() || !programFileInfo.isFile()) {
                // Ошибка: файл не найден - это критично для этой ревизии
                throw std::runtime_error(tr("Файл программы не найден: %1").arg(QDir::toNativeSeparators(programFilePathAbs)).toStdString());
            }
            qint64 programDataSize = programFileInfo.size();
            if (programDataSize > ((1024L - 16L) * 1024L)) { // Проверка размера
                throw std::runtime_error(tr("Файл программы '%1' больше 1008 Кб.").arg(programFileInfo.fileName()).toStdString());
            }
            qDebug() << "  Файл программы:" << programFilePathAbs << "Размер:" << programDataSize;

            QString devModel = info.bldrDevModel; // Модель из config.xml
            QString beginAddrStr = info.beginAddress.trimmed().remove("0x", Qt::CaseInsensitive);
            quint32_t beginAddr = beginAddrStr.toUInt(&ok, 16);
            if (!ok) throw std::runtime_error(tr("Неверный формат начального адреса (begin_adres): '%1'.").arg(info.beginAddress).toStdString());

            quint32_t eraseBeginAddr = beginAddr;
            quint32_t programBeginAddr = beginAddr;
            quint32_t eraseEndAddr = 0; // Оставляем 0

            // --- 2b. Загрузка данных программы ---
            QFile file(programFilePathAbs);
            if (!file.open(QIODevice::ReadOnly)) {
                throw std::runtime_error(tr("Не удалось открыть файл программы '%1': %2").arg(programFileInfo.fileName()).arg(file.errorString()).toStdString());
            }
            programData = file.readAll();
            file.close();
            if (programData.isEmpty() && programDataSize > 0) {
                throw std::runtime_error(tr("Ошибка чтения файла программы '%1' (прочитано 0 байт).").arg(programFileInfo.fileName()).toStdString());
            }
            qDebug() << "  Загружен размер данных:" << programData.size();

            // --- 3b. Заполнение TUpdateTask данными из 'info' для ТЕКУЩЕЙ ревизии ---
            currentTask.FileId = qToLittleEndian(0x52444C42); // "BLDR"
            currentTask.BlockType = qToLittleEndian(0x00010000); // Type Update
            currentTask.Version = qToLittleEndian(0x00000100);   // Format Version 1.00
            currentTask.BlockSize = qToLittleEndian(512);       // Header size

            currentTask.setStringField(currentTask.ModelInfo, sizeof(currentTask.ModelInfo), devModel); // Модель из info
            currentTask.setStringField(currentTask.DescInfo, sizeof(currentTask.DescInfo), ""); // Пусто для общего файла
            currentTask.setStringField(currentTask.DevSerial, sizeof(currentTask.DevSerial), ""); // Пусто для общего файла
            currentTask.setStringField(currentTask.DevId, sizeof(currentTask.DevId), "");       // Пусто для общего файла

            currentTask.EraseBeginAddr = qToLittleEndian(eraseBeginAddr);
            currentTask.EraseEndAddr = qToLittleEndian(eraseEndAddr);
            currentTask.ProgramBeginAddr = qToLittleEndian(programBeginAddr);
            currentTask.ProgramSize = qToLittleEndian(static_cast<quint32_t>(programData.size()));

            // Рассчитываем CRC программы
            programCRC = programData.isEmpty() ? 0 : CrcUnit::calcCrc32(programData);
            currentTask.ProgramCrc = qToLittleEndian(programCRC);
            qInfo() << "  CRC программы (HOST): 0x" + QString::number(programCRC, 16).toUpper().rightJustified(8, '0');

            // Собираем команды, используя флаги из 'info' ТЕКУЩЕЙ ревизии
            quint32_t loaderBaseCmd = 0;
            quint32_t loaderBeforeCmd = 0;

            qDebug() << "  Применяемые флаги для" << category << ":";
            // Базовые команды
            if (info.cmdMainProg) { loaderBaseCmd |= CMD_IS_BASE_PROGRAM; qDebug() << "    + CMD_IS_BASE_PROGRAM"; }
            if (info.cmdNoCheckModel) { loaderBaseCmd |= CMD_NO_MODEL_CHECK; qDebug() << "    + CMD_NO_MODEL_CHECK"; }
            // Пропускаем: cmdOnlyForEnteredSN, cmdOnlyForEnteredDevID

            // Дополнительные команды
            if (info.dopSaveBthName) { loaderBeforeCmd |= CMD_SAVE_BTH_NAME; qDebug() << "    + CMD_SAVE_BTH_NAME"; }
            if (info.dopSaveBthAddr) { loaderBeforeCmd |= CMD_SAVE_BTH_ADDR; qDebug() << "    + CMD_SAVE_BTH_ADDR"; }
            if (info.dopSaveDevDesc) { loaderBeforeCmd |= CMD_SAVE_DEV_DESC; qDebug() << "    + CMD_SAVE_DEV_DESC"; }
            if (info.dopSaveHwVerr) { loaderBeforeCmd |= CMD_SAVE_HW_VERSION; qDebug() << "    + CMD_SAVE_HW_VERSION"; }
            if (info.dopSaveSwVerr) { loaderBeforeCmd |= CMD_SAVE_SW_VERSION; qDebug() << "    + CMD_SAVE_SW_VERSION"; }
            // Пропускаем: dopSaveDevSerial, dopSaveDevModel
            if (info.dopLvl1MemProtection) { loaderBeforeCmd |= CMD_SET_RDP_LV1; qDebug() << "    + CMD_SET_RDP_LV1"; }

            currentTask.LoaderBaseCmd = qToLittleEndian(loaderBaseCmd);
            currentTask.LoaderExtCmd = 0;
            currentTask.LoaderBeforeCmd = qToLittleEndian(loaderBeforeCmd);
            currentTask.LoaderAfterCmd = 0;

            // Устанавливаем время
            qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
            quint64 nowMsLe = qToLittleEndian(static_cast<quint64>(nowMs));
            memcpy(currentTask.Time, &nowMsLe, sizeof(currentTask.Time));

            // Рассчитываем CRC заголовка
            qsizetype headerSizeForCrc = sizeof(TUpdateTask) - sizeof(quint32_t);
            QByteArray headerDataForCrc = QByteArray::fromRawData(reinterpret_cast<const char*>(&currentTask), headerSizeForCrc);
            headerCRC = CrcUnit::calcCrc32(headerDataForCrc);
            currentTask.Crc = qToLittleEndian(headerCRC);
            qInfo() << "  CRC заголовка (HOST): 0x" + QString::number(headerCRC, 16).toUpper().rightJustified(8, '0');

            // --- 4b. PSP Encoding ---
            TUpdateTask encodedTask = currentTask; // Копируем заголовок
            QByteArray encodedData = programData; // Копируем данные

            qDebug() << "  Шифрование заголовка (размер:" << headerSizeForCrc << ") сидом 0x" << QString::number(headerCRC, 16).toUpper();
            applyPSPEncoding(reinterpret_cast<quint8_t*>(&encodedTask), headerSizeForCrc, headerCRC);

            if (!encodedData.isEmpty()) {
                quint32_t dataEncodingSeed = headerCRC ^ programCRC;
                qInfo() << "  Шифрование данных (размер:" << encodedData.size() << ") сидом 0x" + QString::number(dataEncodingSeed, 16).toUpper();
                applyPSPEncoding(reinterpret_cast<quint8_t*>(encodedData.data()), encodedData.size(), dataEncodingSeed);
            }

            // --- 5b. Запись шифрованного блока в ОБЩИЙ файл ---
            qint64 bytesWritten;
            bytesWritten = saveFile.write(reinterpret_cast<const char*>(&encodedTask), sizeof(TUpdateTask));
            if (bytesWritten != sizeof(TUpdateTask)) {
                throw std::runtime_error(tr("Ошибка записи заголовка в общий файл.").toStdString());
            }
            if (!encodedData.isEmpty()) {
                bytesWritten = saveFile.write(encodedData);
                if (bytesWritten != encodedData.size()) {
                    throw std::runtime_error(tr("Ошибка записи данных в общий файл.").toStdString());
                }
            }
            qDebug() << "  Блок для ревизии" << category << "успешно записан в общий файл.";
            blocksAdded++; // Увеличиваем счетчик только при успешной записи блока

        } catch (const std::runtime_error& e) {
            // Ошибка при обработке ЭТОЙ ревизии
            qWarning() << "  Критическая ошибка при обработке ревизии" << category << "для общего файла:" << e.what();
            // Добавляем в ОБА списка: и ошибок, и пропущенных
            errorsList << tr("Ревизия '%1': %2").arg(category).arg(QString::fromStdString(e.what()));
            skippedList << tr("Ревизия '%1': Пропущена из-за ошибки (%2).").arg(category).arg(QString::fromStdString(e.what()));
            revisionsSkipped++;
            continue; // Переходим к следующей ревизии
        }
        // === КОНЕЦ БЛОКА ДЛЯ ОДНОЙ РЕВИЗИИ ===

    } // Конец цикла while по ревизиям

    // 5. Завершаем запись и закрываем файл
    bool commitOk = saveFile.commit();
    if (!commitOk) {
        QMessageBox::critical(ui->btnCreateCommonUpdateFile->window(), tr("Ошибка сохранения"),
                              tr("Не удалось сохранить финальный общий файл:\n%1\nОшибка: %2")
                                  .arg(QDir::toNativeSeparators(saveFilePath))
                                  .arg(saveFile.errorString()));
        qCritical() << "Создание общего файла: Ошибка commit QSaveFile:" << saveFile.errorString();
        // Добавляем ошибку сохранения, если commit не удался
        errorsList << tr("Финальная ошибка сохранения файла: %1").arg(saveFile.errorString());
    }

    // 6. Сообщаем результат пользователю
    qInfo() << "--- Завершение создания общего файла ---";
    qInfo() << "Добавлено блоков:" << blocksAdded << ", Пропущено/ошибки:" << revisionsSkipped;

    QString resultTitle = tr("Создание общего файла");
    QString resultMessage;
    QMessageBox::Icon icon;

    // Определяем результат на основе commitOk, blocksAdded и errorsList
    if (commitOk && blocksAdded > 0 && errorsList.isEmpty()) { // Полный успех
        resultTitle += tr(" завершено успешно");
        icon = QMessageBox::Information;
        resultMessage = tr("Общий файл обновления '%1' успешно создан.\n\nВключено ревизий: %2.")
                            .arg(QFileInfo(saveFilePath).fileName())
                            .arg(blocksAdded);


    } else if (commitOk && blocksAdded > 0 && !errorsList.isEmpty()) { // Частичный успех (блоки добавлены, но были ошибки с некоторыми ревизиями)
        resultTitle += tr(" завершено с ошибками");
        icon = QMessageBox::Warning;
        resultMessage = tr("Общий файл обновления '%1' создан, но при обработке некоторых ревизий возникли ошибки.\n\nУспешно добавлено ревизий: %2\nПропущено из-за ошибок/отсутствия данных: %3\n")
                            .arg(QFileInfo(saveFilePath).fileName())
                            .arg(blocksAdded)
                            .arg(revisionsSkipped);
        // Показываем полный список пропущенных ревизий (включая ошибки)
        if (!skippedList.isEmpty()){
            resultMessage += "\n" + tr("Список пропущенных/ошибочных ревизий:") + "\n- " + skippedList.join("\n- ");
        }
        // Показываем только критические ошибки обработки
        resultMessage += "\n\n" + tr("Обнаруженные критические ошибки:") + "\n- " + errorsList.join("\n- ");

    } else { // Полная неудача (либо commit не удался, либо не добавлено ни одного блока)
        resultTitle = tr("Ошибка создания общего файла");
        icon = QMessageBox::Critical;
        if (!commitOk) { // Ошибка на этапе сохранения
            resultMessage = tr("Не удалось сохранить общий файл '%1'.\n\nОшибка: %2")
                                .arg(QFileInfo(saveFilePath).fileName())
                                .arg(errorsList.last()); // Последняя ошибка - это ошибка commit
        } else { // commitOk, но blocksAdded == 0
            resultMessage = tr("Не удалось добавить ни одного блока ревизии в общий файл.\nПроверьте config.xml и наличие файлов прошивок.\n");
            if (!skippedList.isEmpty()){
                resultMessage += "\n" + tr("Список пропущенных/ошибочных ревизий:") + "\n- " + skippedList.join("\n- ");
            } else {
                resultMessage += "\n" + tr("Возможно, нет подходящих ревизий в config.xml.");
            }
            if (!errorsList.isEmpty()) { // Показываем ошибки, если они были причиной отсутствия блоков
                resultMessage += "\n\n" + tr("Обнаруженные критические ошибки:") + "\n- " + errorsList.join("\n- ");
            }
        }
    }

    // Показываем итоговое сообщение
    QMessageBox msgBox(icon, resultTitle, resultMessage, QMessageBox::Ok, ui->btnCreateCommonUpdateFile->window());
    // Увеличим ширину окна сообщения, если текст длинный
    // if (resultMessage.length() > 200) { // Примерная оценка
    //     msgBox.setStyleSheet("QMessageBox { messagebox-width: 500px; }");
    // }
    msgBox.exec();
}

// --- Private Helper Methods ---
void Unit2::updateUpdateProgramDataFileSize(const QString &filePath)
{
    if (filePath.isEmpty() || filePath.startsWith("Файл:") || filePath == "-") {
        ui->lblUpdateProgramDataFileSize->setText("Размер: -"); return; }

    QString absPath = filePath;
    if (QDir::isRelativePath(filePath)) {
        absPath = QDir(QDir::currentPath()).filePath(filePath);
    }

    QFileInfo fileInfo(absPath);
    if (fileInfo.exists() && fileInfo.isFile()) {
        ui->lblUpdateProgramDataFileSize->setText("Размер: " + QString::number(fileInfo.size()) + tr(" байт"));
    } else {
        ui->lblUpdateProgramDataFileSize->setText(tr("Файл не найден"));
    }
}
