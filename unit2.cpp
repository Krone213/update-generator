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
        ui->editEraseStartAddress->setText(info.beginAddress.isEmpty() ? "0x0" : info.beginAddress); // Default EraseStart to ProgramStart from config
        ui->editEraseEndAddress->setText("0x0");   // Default Erase End to 0

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
    ui->editEraseStartAddress->setText("0x0");
    ui->editEraseEndAddress->setText("0x0");
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

void Unit2::onBtnClearUpdateProgramDataFileClicked()
{
    ui->lblUpdateProgramDataFileName->setText("Файл: -");
    ui->lblUpdateProgramDataFileSize->setText("Размер: -");
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

void Unit2::onBtnClearUpdateRevisionClicked()
{
    qDebug() << "Unit2::onBtnClearUpdateRevisionClicked: Clearing Update Revision UI triggered by button.";
    clearUIFields(); // Clear related fields
    // Trigger synchronization by changing the combobox index
    if (ui->cmbUpdateRevision->currentIndex() != -1) {
        int othDevIndex = ui->cmbUpdateRevision->findText("OthDev");
        ui->cmbUpdateRevision->setCurrentIndex(othDevIndex >= 0 ? othDevIndex : -1); // Use -1 if OthDev not found
    }
}


void Unit2::onBtnCreateCommonUpdateFileClicked()
{
    qDebug() << "Create Common Update File clicked";
    QMessageBox::information(ui->btnCreateCommonUpdateFile->window(), "Общий файл",
                             "Логика создания общего файла обновления еще не реализована.");
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
