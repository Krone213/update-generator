#include "unit1.h"
#include "crcunit.h"
#include <vector>

const char TELNET_CMD_TERMINATOR = '\x1a';
const QString Unit1::TEMP_SUBDIR_NAME_OPENOCD = "openocd_fw_temp";

Unit1::Unit1(Ui::MainWindow *ui, QObject *parent)
    : QObject(parent), ui(ui)
{
    statusTimer = new QTimer(this);
    statusTimer->setSingleShot(true);
    connect(statusTimer, &QTimer::timeout, this, &Unit1::hideConnectionStatus);

    QString appDir = QCoreApplication::applicationDirPath();
    QString openOcdBaseDirName;
    QString openOcdExecutableName;

#ifdef Q_OS_WIN
    openOcdBaseDirName = "openocd_win";
    openOcdExecutableName = "openocd.exe";
#else
    openOcdBaseDirName = "openocd_linux";
    openOcdExecutableName = "openocd";
#endif

    m_openOcdDir = QDir(appDir).filePath(openOcdBaseDirName);
    m_openOcdExecutablePath = QDir(m_openOcdDir).filePath(QDir::toNativeSeparators("bin/" + openOcdExecutableName));
    m_openOcdScriptsPath = QDir(m_openOcdDir).filePath("openocd/scripts");

    m_shutdownCommandSent = false;

    m_openOcdProcess = new QProcess(this);
    connect(m_openOcdProcess, &QProcess::started, this, &Unit1::handleOpenOcdStarted);
    connect(m_openOcdProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Unit1::handleOpenOcdFinished);
    connect(m_openOcdProcess, &QProcess::errorOccurred, this, &Unit1::handleOpenOcdError);
    connect(m_openOcdProcess, &QProcess::readyReadStandardOutput, this, &Unit1::handleOpenOcdStdOut);
    connect(m_openOcdProcess, &QProcess::readyReadStandardError, this, &Unit1::handleOpenOcdStdErr);

    m_telnetSocket = new QTcpSocket(this);
    connect(m_telnetSocket, &QTcpSocket::connected, this, &Unit1::handleTelnetConnected);
    connect(m_telnetSocket, &QTcpSocket::disconnected, this, &Unit1::handleTelnetDisconnected);
    connect(m_telnetSocket, &QTcpSocket::readyRead, this, &Unit1::handleTelnetReadyRead);
    connect(m_telnetSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &Unit1::handleTelnetError);

    m_animationTimer = new QTimer(this);
    m_animationTimer->setInterval(250);
    connect(m_animationTimer, &QTimer::timeout, this, &Unit1::updateLoadingAnimation);

    ui->lblConnectionStatus->setVisible(false);
    loadConfig("config.xml");

    connect(ui->cmbRevision, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Unit1::onRevisionChanged);

    ui->cmbRevision->setCurrentIndex(-1);
    onRevisionChanged(-1);

    ui->btnConnect->setEnabled(true);

    ui->cmbTargetMCU->clear();
    ui->cmbTargetMCU->addItem("STM32L4x Series", "target/stm32l4x.cfg");
    ui->cmbTargetMCU->addItem("STM32F1x Series", "target/stm32f1x.cfg");
    ui->cmbTargetMCU->addItem("STM32F3x Series", "target/stm32f3x.cfg");
    ui->cmbTargetMCU->setCurrentIndex(-1);

    m_isOpenOcdRunning = false;
    m_isConnected = false;
    m_isConnecting = false;
    m_isProgramming = false;

    m_isAttemptingAutoDetect = false;
    m_detectedTargetScript.clear();

    m_autoDetectTimeoutTimer = new QTimer(this);
    m_autoDetectTimeoutTimer->setSingleShot(true);
    connect(m_autoDetectTimeoutTimer, &QTimer::timeout, this, [&, this]() {
        if (m_isAttemptingAutoDetect) {
            emit logToInterface("Тайм-аут автоопределения MCU. Остановка OpenOCD.", true);
            if (m_openOcdProcess && m_openOcdProcess->state() != QProcess::NotRunning) {
                m_openOcdProcess->blockSignals(true);
                m_openOcdProcess->terminate();
                m_openOcdProcess->waitForFinished(500);
                m_openOcdProcess->kill();
                m_openOcdProcess->waitForFinished(200);
                m_openOcdProcess->blockSignals(false);
            }
            m_isAttemptingAutoDetect = false;
            m_isConnecting = false;
            m_detectedTargetScript.clear();
            ui->lblConnectionStatus->setText("<font color='red'>Тайм-аут\nавтоопр.</font>");
            statusTimer->start(3000);

            ui->btnConnect->setEnabled(true);
            ui->cmbTargetMCU->setEnabled(true);
            updateUploadButtonsState();
        }
    });

    updateUploadButtonsState();
}

Unit1::~Unit1()
{
    stopOpenOcd();
    cleanupTemporaryFile();
}

// ----------------------- Интеграция OpenOCD

bool Unit1::checkOpenOcdPrerequisites(const QString& targetScriptPath) {
    if (!QFile::exists(m_openOcdExecutablePath)) {
        QString expectedParentFolder = QDir(m_openOcdDir).dirName();
        emit logToInterface("Критическая ошибка: Исполняемый файл OpenOCD не найден по пути: " + m_openOcdExecutablePath, true);
        emit logToInterface("Убедитесь, что папка '" + expectedParentFolder + "' со всеми необходимыми файлами OpenOCD "
                                                                              "(включая подпапку 'bin') находится рядом с программой.", true);
        return false;
    }

    if (!QDir(m_openOcdScriptsPath).exists()) {
        emit logToInterface("Критическая ошибка: Папка со скриптами OpenOCD не найдена: " + m_openOcdScriptsPath, true);
        emit logToInterface("Ожидаемая структура: папка_программы/" + QDir(m_openOcdDir).dirName() + "/scripts/", true);
        return false;
    }

    QString fullInterfaceScriptPath = QDir(m_openOcdScriptsPath).filePath(m_interfaceScript);
    if (m_interfaceScript.isEmpty() || !QFile::exists(fullInterfaceScriptPath)) {
        emit logToInterface("Критическая ошибка: Файл скрипта интерфейса не найден: " + fullInterfaceScriptPath, true);
        return false;
    }

    if (targetScriptPath.isEmpty()) {
        emit logToInterface("Ошибка: Путь к скрипту цели не предоставлен для проверки.", true);
        return false;
    }

    QString fullTargetScriptPath = QDir(m_openOcdScriptsPath).filePath(targetScriptPath);
    if (!QFile::exists(fullTargetScriptPath)) {
        emit logToInterface("Критическая ошибка: Выбранный файл скрипта цели не найден: " + fullTargetScriptPath, true);
        return false;
    }

    return true;
}

#ifdef Q_OS_WIN
QString Unit1::winGetShortPathName(const QString& longPath) {
    std::wstring longPathW = longPath.toStdWString();
    DWORD bufferLength = GetShortPathNameW(longPathW.c_str(), nullptr, 0);

    if (bufferLength == 0) return longPath;

    std::vector<wchar_t> shortPathBuffer(bufferLength);
    if (GetShortPathNameW(longPathW.c_str(), shortPathBuffer.data(), bufferLength) == 0) {
        return longPath;
    }
    return QString::fromWCharArray(shortPathBuffer.data());
}
#endif

QString Unit1::getSafeTemporaryDirectoryBasePath() {
    QString basePath;
    bool isAsciiPath = false;

#ifdef Q_OS_WIN
    basePath = QDir::tempPath();
    emit logToInterface("Windows: Проверка системного TEMP пути: " + basePath, false);
    basePath = winGetShortPathName(basePath);
    emit logToInterface("Windows: Короткая версия TEMP пути: " + basePath, false);

    isAsciiPath = true;
    for (const QChar &ch : basePath) {
        if (ch.unicode() > 127) {
            isAsciiPath = false;
            break;
        }
    }
    if (!isAsciiPath) {
        emit logToInterface("Windows: Короткий TEMP путь все еще содержит не-ASCII: " + basePath + ". Попытка других вариантов.", true);
        basePath.clear();
    } else {
        emit logToInterface("Windows: Используется TEMP путь (короткий/ASCII): " + basePath, false);
    }

    if (basePath.isEmpty()) {
        QString cacheLocation = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        if (!cacheLocation.isEmpty()) {
            emit logToInterface("Windows: Проверка CacheLocation пути: " + cacheLocation, false);
            basePath = winGetShortPathName(cacheLocation);
            emit logToInterface("Windows: Короткая версия CacheLocation: " + basePath, false);
            isAsciiPath = true;
            for (const QChar &ch : basePath) {
                if (ch.unicode() > 127) {
                    isAsciiPath = false;
                    break;
                }
            }
            if (!isAsciiPath) {
                emit logToInterface("Windows: Короткий CacheLocation путь содержит не-ASCII: " + basePath + ". Попытка других вариантов.", true);
                basePath.clear();
            } else {
                emit logToInterface("Windows: Используется CacheLocation (короткий/ASCII): " + basePath, false);
            }
        }
    }
#else
    basePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    emit logToInterface("Linux/Other: Проверка CacheLocation пути: " + basePath, false);
    if (basePath.isEmpty()) {
        emit logToInterface("Linux/Other: CacheLocation не найден, используем системный temp.", true);
        basePath = QDir::tempPath();
        emit logToInterface("Linux/Other: Используется системный temp путь: " + basePath, false);
    }
    isAsciiPath = true;
#endif

    if (basePath.isEmpty()) {
        emit logToInterface("КРИТИЧЕСКАЯ ОШИБКА: Не удалось определить подходящий базовый каталог для временных файлов.", true);
        return QString();
    }

    if (!ensureDirectoryExists(basePath)) {
        return QString();
    }

#ifdef Q_OS_WIN
    if (!isAsciiPath) {
        emit logToInterface("КРИТИЧЕСКАЯ ОШИБКА: Не удалось найти ASCII-совместимый базовый путь для временных файлов на Windows.", true);
        return QString();
    }
#endif

    return basePath;
}

bool Unit1::ensureDirectoryExists(const QString& path) {
    QDir dir(path);
    if (dir.exists()) {
        return true;
    }
    if (!dir.mkpath(".")) {
        emit logToInterface("КРИТИЧЕСКАЯ ОШИБКА: Не удалось создать каталог: " + path, true);
        return false;
    }
    emit logToInterface("Каталог успешно создан: " + path, false);
    return true;
}

// ----------------------------------- Автоопределение

void Unit1::startOpenOcdForAutoDetect() {
    if (!QFile::exists(m_openOcdExecutablePath) || !QDir(m_openOcdScriptsPath).exists() || m_interfaceScript.isEmpty()) {
        emit logToInterface("Критическая ошибка: Отсутствуют базовые файлы OpenOCD или скрипт интерфейса для автоопределения.", true);
        m_isAttemptingAutoDetect = false;
        m_isConnecting = false;
        m_animationTimer->stop(); // Остановить анимацию
        ui->lblConnectionStatus->setText("<font color='red'>Ошибка\nфайлов</font>");
        statusTimer->start(3000); // Показать ошибку на время
        ui->btnConnect->setEnabled(true);
        ui->cmbTargetMCU->setEnabled(true);
        updateUploadButtonsState();
        return;
    }
    QString fullInterfaceScriptPath = QDir(m_openOcdScriptsPath).filePath(m_interfaceScript);
    if (!QFile::exists(fullInterfaceScriptPath)) {
        emit logToInterface("Критическая ошибка: Файл скрипта интерфейса '"+fullInterfaceScriptPath+"' не найден.", true);
        return;
    }

    if (!m_detectedTargetScript.isEmpty()) {
        qWarning() << "CRITICAL: m_detectedTargetScript was NOT empty at the start of startOpenOcdForAutoDetect!";
        m_detectedTargetScript.clear();
    }

    emit logToInterface("!! Запуск OpenOCD для автоопределения MCU (с stm32l4x.cfg как базой и чтением IDCODE)...", false);

    QStringList arguments;
    arguments << "-s" << QDir::toNativeSeparators(m_openOcdScriptsPath);
    arguments << "-f" << m_interfaceScript;

    arguments << "-f" << "target/stm32l4x.cfg";
    arguments << "-c" << "adapter speed 1000";
    arguments << "-c" << "init";
    arguments << "-c" << "echo \"Reading IDCODE...\"";
    arguments << "-c" << "set MY_IDCODE [mrw 0xE0042000]";
    arguments << "-c" << "echo \"IDCODE_VALUE: $MY_IDCODE\"";
    arguments << "-c" << "shutdown";

    emit logToInterface("Команда запуска для автоопределения: " + m_openOcdExecutablePath + " " + arguments.join(" "), false);

    m_openOcdProcess->setWorkingDirectory(m_openOcdDir);
    m_autoDetectTimeoutTimer->start(10000);
    m_openOcdProcess->start(m_openOcdExecutablePath, arguments);
}

void Unit1::proceedWithConnection(const QString& targetScript) {
    m_isAttemptingAutoDetect = false;
    m_autoDetectTimeoutTimer->stop();

    ui->logTextEdit->clear();
    m_isConnected = false;
    m_isProgramming = false;
    m_receivedTelnetData.clear();

    emit logToInterface("!! Запуск OpenOCD и попытка подключения к " + targetScript, false);

    QString mcuNameForStatus = QFileInfo(targetScript).baseName(); // stm32l4x или stm32f3x
    ui->lblConnectionStatus->setText(QString("<font color='blue'><b>Подключение\n%1...</b></font>").arg(mcuNameForStatus));
    ui->lblConnectionStatus->setVisible(true);
    m_animationFrame = -1;
    if (!m_animationTimer->isActive()) m_animationTimer->start();
    updateLoadingAnimation();
    statusTimer->stop();
    ui->btnConnect->setEnabled(false);
    ui->cmbTargetMCU->setEnabled(false);

    // Формируем аргументы для OpenOCD
    QStringList arguments;
    arguments << "-s" << QDir::toNativeSeparators(m_openOcdScriptsPath);
    arguments << "-f" << m_interfaceScript;
    arguments << "-f" << targetScript;
    arguments << "-c" << "adapter speed 1000";

    emit logToInterface("Команда запуска: " + m_openOcdExecutablePath + " " + arguments.join(" "), false);

    m_openOcdProcess->setWorkingDirectory(m_openOcdDir);
    m_openOcdProcess->start(m_openOcdExecutablePath, arguments);
}

void Unit1::processOpenOcdOutputForDetection(const QString& output) {
    if (!m_isAttemptingAutoDetect || !m_detectedTargetScript.isEmpty()) return;

    QRegularExpression regex("IDCODE_VALUE:\\s*(0x[0-9a-fA-F]+|[0-9]+)");
    QRegularExpressionMatch match = regex.match(output);

    if (match.hasMatch()) {
        QString idcodeHexStr = match.captured(1);

        bool ok;
        uint rawIdcode = idcodeHexStr.toUInt(&ok, 0);

        if (ok) {
            uint dev_id = rawIdcode & 0x00000FFF; // Извлекаем DEV_ID (младшие 12 бит)

            emit logToInterface(QString("Автоопределение: Прочитан DBGMCU_IDCODE=0x%1, DEV_ID=0x%2")
                                    .arg(rawIdcode, 8, 16, QChar('0'))
                                    .arg(dev_id, 3, 16, QChar('0')), false);
            qDebug() << "Parsed DEV_ID:" << QString("0x%1").arg(dev_id, 0, 16);

            // ----- CПИСКИ УСТРОЙСТВ -----
            const QList<uint> l4_dev_ids = {
                0x415, // STM32L471xx, STM32L475xx, STM32L476xx, STM32L486xx
                0x435, // STM32L431xx, STM32L432xx, STM32L433xx, STM32L442xx, STM32L443xx
                0x462, // STM32L412xx, STM32L422xx
                0x470, // STM32L4R5xx, STM32L4R7xx, STM32L4R9xx, STM32L4S5xx, STM32L4S7xx, STM32L4S9xx
                0x472  // STM32L4P5xx, STM32L4Q5xx
            };

            const QList<uint> f3_dev_ids = {
                0x414, // STM32F303xB/C (старые?), STM32F302xB/C?
                0x422, // STM32F303x6/8, STM32F303xB/C, STM32F303xD/E, STM32F398xx, STM32F328xx, STM32F358xx
                0x431, // STM32F302xDxE? - проверьте документацию
                0x432, // STM32F373xx, STM32F378xx  <--- Этот ID был у вас в логе для F3
                0x438, // STM32F301x6/8, STM32F302x6/8, STM32F318xx
                0x446  // STM32F334x4/6/8
            };
            // ----- КОНЕЦ СПИСКОВ -----

            qDebug() << "L4 DEV_IDs List:" << l4_dev_ids;
            qDebug() << "F3 DEV_IDs List:" << f3_dev_ids;
            qDebug() << "Current DEV_ID is in L4 list?" << l4_dev_ids.contains(dev_id);
            qDebug() << "Current DEV_ID is in F3 list?" << f3_dev_ids.contains(dev_id);

            if (l4_dev_ids.contains(dev_id)) {
                m_detectedTargetScript = "target/stm32l4x.cfg";
                emit logToInterface("Автоопределение: Определен STM32L4x по DEV_ID.", false);
                qDebug() << "Detected as L4";
                m_autoDetectTimeoutTimer->stop();
            } else if (f3_dev_ids.contains(dev_id)) {
                m_detectedTargetScript = "target/stm32f3x.cfg";
                emit logToInterface("Автоопределение: Определен STM32F3x по DEV_ID.", false);
                qDebug() << "Detected as F3";
                m_autoDetectTimeoutTimer->stop();
            } else {
                emit logToInterface(QString("Автоопределение: Неизвестный DEV_ID 0x%1. Не удалось определить тип MCU.").arg(dev_id, 3, 16, QChar('0')), true);
                qDebug() << "Unknown DEV_ID";
            }
        } else {
            emit logToInterface("Автоопределение: Ошибка преобразования IDCODE в число: " + idcodeHexStr, true);
        }
    } else if (output.contains("Can't read memory address", Qt::CaseInsensitive) || output.contains("failed to read memory", Qt::CaseInsensitive)) {
        emit logToInterface("Автоопределение: Ошибка чтения IDCODE регистра. Убедитесь, что MCU подключен и отвечает.", true);
    }
}

// ----------------------------------- Автоопределение

void Unit1::onBtnConnectClicked() {
    m_detectedTargetScript.clear();

    if (m_isConnected) {
        ui->btnConnect->setEnabled(false);
        emit logToInterface("Нажата кнопка 'Отключить'. Остановка OpenOCD...", false);
        ui->cmbTargetMCU->setCurrentIndex(-1);
        stopOpenOcd();
        return;
    }

    if (m_isOpenOcdRunning || m_isConnecting || m_isAttemptingAutoDetect) {
        emit logToInterface("Процесс OpenOCD, подключения или автоопределения уже активен. Пожалуйста, подождите.", true);
        return;
    }

    ui->logTextEdit->clear();
    m_isConnecting = true;
    m_isAttemptingAutoDetect = false;
    m_receivedTelnetData.clear();
    statusTimer->stop();
    m_animationFrame = -1;

    QString targetScriptFromCmb = ui->cmbTargetMCU->currentData().toString();

    if (targetScriptFromCmb.isEmpty()) {
        emit logToInterface("MCU не выбран. Попытка автоопределения...", false);
        m_isAttemptingAutoDetect = true;

        ui->lblConnectionStatus->setText("<font color='blue'><b>Определение\nMCU...</b></font>");
        ui->lblConnectionStatus->setVisible(true);
        if (!m_animationTimer->isActive()) m_animationTimer->start();
        updateLoadingAnimation();

        ui->btnConnect->setEnabled(false);
        updateUploadButtonsState();
        ui->cmbTargetMCU->setEnabled(false);

        startOpenOcdForAutoDetect();
    } else { // MCU выбран пользователем, обычное подключение
        if (!checkOpenOcdPrerequisites(targetScriptFromCmb)) {
            m_isConnecting = false;
            return;
        }
        proceedWithConnection(targetScriptFromCmb);
    }
}

void Unit1::updateUploadButtonsState() {
    bool cpu1Enabled = false;
    bool cpu2Enabled = false;

    if (m_isConnected && !m_isConnecting && !m_isProgramming) {
        QString targetScript = ui->cmbTargetMCU->currentData().toString();

        if (targetScript.contains("stm32l4x", Qt::CaseInsensitive)) {
            cpu1Enabled = true;
            cpu2Enabled = false;
        } else if (targetScript.contains("stm32f3x", Qt::CaseInsensitive)) {
            cpu1Enabled = false;
            cpu2Enabled = true;
        } else if (targetScript.contains("stm32f1x", Qt::CaseInsensitive)) {
            cpu1Enabled = true;
            cpu2Enabled = true;
        } else {
            cpu1Enabled = false;
            cpu2Enabled = false;
            if (!targetScript.isEmpty()){
                emit logToInterface("Предупреждение: Неизвестный тип MCU для правил кнопок: " + targetScript, true);
            }
        }
    }

    if(ui->btnUploadCPU1) ui->btnUploadCPU1->setEnabled(cpu1Enabled);
    if(ui->btnUploadCPU2) ui->btnUploadCPU2->setEnabled(cpu2Enabled);
}

void Unit1::onbtnUploadCPU1Clicked() {
    if (!m_isConnected) {
        emit logToInterface("Ошибка: Устройство не подключено. Сначала нажмите 'Подключить'.", true);
        return;
    }
    if (m_isProgramming) {
        emit logToInterface("Программирование уже выполняется.", true);
        return;
    }

#ifdef Q_OS_WIN
    QString CPU1dir = "\\Файл прошивки CPU1";
#else
    QString CPU1dir = "/Файл прошивки CPU1";
#endif

    QString originalFirmwarePath;
    QString defaultDir = QCoreApplication::applicationDirPath() + CPU1dir;

    originalFirmwarePath = QFileDialog::getOpenFileName(
        nullptr,
        tr("Выберите файл прошивки CPU1"),
        defaultDir,
        tr("Файлы прошивки (*.bin *.hex *.elf);;Все файлы (*.*)")
        );

    if (originalFirmwarePath.isEmpty()) {
        emit logToInterface("Выбор файла отменен.", false);
        return;
    }

    m_originalFirmwarePathForLog = originalFirmwarePath;
    emit logToInterface("Выбран файл: " + originalFirmwarePath, false);

    QString safeBaseDirPath = getSafeTemporaryDirectoryBasePath();
    if (safeBaseDirPath.isEmpty()) {
        return;
    }
    m_currentSafeTempSubdirPath = QDir(safeBaseDirPath).filePath(TEMP_SUBDIR_NAME_OPENOCD);
    if (!ensureDirectoryExists(m_currentSafeTempSubdirPath)) {
        m_currentSafeTempSubdirPath.clear();
        return;
    }
    emit logToInterface("Временная подпапка для прошивок: " + m_currentSafeTempSubdirPath, false);

    QFileInfo originalFileInfo(originalFirmwarePath);
    QString suffix = originalFileInfo.suffix().toLower();
    bool isAsciiSafeSuffix = true;
    if (!suffix.isEmpty()) {
        for (const QChar& c : suffix) {
            if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) {
                isAsciiSafeSuffix = false;
                break;
            }
        }
    }

    if (suffix.isEmpty() || !isAsciiSafeSuffix || suffix.length() > 4) {
        suffix = "bin";
        emit logToInterface("Предупреждение: Расширение оригинального файла некорректно или не ASCII-совместимо. Используется '.bin' для временного файла.", false);
    }

    QString simpleFileName = QString("fw_upload_%1.%2")
                                 .arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmsszzz"))
                                 .arg(suffix);

    QString temporaryFirmwarePath = QDir(m_currentSafeTempSubdirPath).filePath(simpleFileName);

    if (!m_firmwareFilePathForUpload.isEmpty() && m_firmwareFilePathForUpload != temporaryFirmwarePath && QFile::exists(m_firmwareFilePathForUpload)) {
        emit logToInterface("Удаление предыдущего временного файла: " + m_firmwareFilePathForUpload, false);
        QFile::remove(m_firmwareFilePathForUpload);
    }
    if (QFile::exists(temporaryFirmwarePath)) {
        if(!QFile::remove(temporaryFirmwarePath)){
            emit logToInterface("Предупреждение: Не удалось удалить существующий одноименный временный файл перед копированием: " + temporaryFirmwarePath, true);
        }
    }

    if (!QFile::copy(originalFirmwarePath, temporaryFirmwarePath)) {
        QFile sourceFile(originalFirmwarePath);
        QFile destFile(temporaryFirmwarePath);
        QString errorDetails = "Ошибка исходного файла: " + sourceFile.errorString() +
                               ", Ошибка файла назначения: " + destFile.errorString();
        emit logToInterface("Критическая ошибка: Не удалось скопировать файл прошивки из '" + originalFirmwarePath + "' в '" +
                                temporaryFirmwarePath + "'. " + errorDetails, true);
        return;
    }
    emit logToInterface("Файл скопирован во временный: " + temporaryFirmwarePath, false);
    m_firmwareFilePathForUpload = temporaryFirmwarePath;

    QString firmwarePathForOcd = QDir::fromNativeSeparators(m_firmwareFilePathForUpload);
    firmwarePathForOcd.replace('\\', '/');

    bool finalPathIsAscii = true;
    for (const QChar &ch : firmwarePathForOcd) {
        if (ch.unicode() > 127) {
            finalPathIsAscii = false;
            break;
        }
    }
    if (!finalPathIsAscii) {
        emit logToInterface("КРИТИЧЕСКОЕ ПРЕДУПРЕЖДЕНИЕ: Финальный путь для OpenOCD "
                            "("+firmwarePathForOcd+") содержит не-ASCII символы! Программирование может не удасться.", true);
    }
    emit logToInterface("Путь для OpenOCD (проверен на ASCII): " + firmwarePathForOcd, false);

    m_shutdownCommandSent = false;
    m_isProgramming = true;
    emit logToInterface("!! Начало программирования (файл: " + originalFileInfo.fileName() + ")", false);

    ui->lblConnectionStatus->setText("<font color='blue'><b>Прошивка\n...</b></font>");
    ui->lblConnectionStatus->setVisible(true);
    m_animationFrame = -1;
    if (!m_animationTimer->isActive()) m_animationTimer->start();
    updateLoadingAnimation();
    statusTimer->stop();
    if(ui->btnUploadCPU1) ui->btnUploadCPU1->setEnabled(false);

    m_receivedTelnetData.clear();
    sendOpenOcdCommand("reset halt");
    QTimer::singleShot(200, this, [this, firmwarePathForOcd]() {
        if (m_isProgramming) {
            QString programCmd = QString("program \"%1\" %2 verify reset").arg(firmwarePathForOcd).arg(m_firmwareAddress);
            sendOpenOcdCommand(programCmd);
        }
    });
}

void Unit1::onbtnUploadCPU2Clicked() {
    if (!m_isConnected) {
        emit logToInterface("Ошибка: Устройство не подключено. Сначала нажмите 'Подключить'.", true);
        return;
    }
    if (m_isProgramming) {
        emit logToInterface("Программирование уже выполняется.", true);
        return;
    }

#ifdef Q_OS_WIN
    QString CPU2dir = "\\Файл прошивки CPU2";
#else
    QString CPU2dir = "/Файл прошивки CPU2";
#endif

    QString originalFirmwarePath;
    QString defaultDir = QCoreApplication::applicationDirPath() + CPU2dir;

    originalFirmwarePath = QFileDialog::getOpenFileName(
        nullptr,
        tr("Выберите файл прошивки CPU2"),
        defaultDir,
        tr("Файлы прошивки (*.bin *.hex *.elf);;Все файлы (*.*)")
        );

    if (originalFirmwarePath.isEmpty()) {
        emit logToInterface("Выбор файла отменен.", false);
        return;
    }

    m_originalFirmwarePathForLog = originalFirmwarePath;
    emit logToInterface("Выбран файл: " + originalFirmwarePath, false);

    QString safeBaseDirPath = getSafeTemporaryDirectoryBasePath();
    if (safeBaseDirPath.isEmpty()) {
        return;
    }
    m_currentSafeTempSubdirPath = QDir(safeBaseDirPath).filePath(TEMP_SUBDIR_NAME_OPENOCD);
    if (!ensureDirectoryExists(m_currentSafeTempSubdirPath)) {
        m_currentSafeTempSubdirPath.clear();
        return;
    }
    emit logToInterface("Временная подпапка для прошивок: " + m_currentSafeTempSubdirPath, false);

    QFileInfo originalFileInfo(originalFirmwarePath);
    QString suffix = originalFileInfo.suffix().toLower();
    bool isAsciiSafeSuffix = true;
    if (!suffix.isEmpty()) {
        for (const QChar& c : suffix) {
            if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) {
                isAsciiSafeSuffix = false;
                break;
            }
        }
    }

    if (suffix.isEmpty() || !isAsciiSafeSuffix || suffix.length() > 4) {
        suffix = "bin";
        emit logToInterface("Предупреждение: Расширение оригинального файла некорректно или не ASCII-совместимо. Используется '.bin' для временного файла.", false);
    }

    QString simpleFileName = QString("fw_upload_%1.%2")
                                 .arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmsszzz"))
                                 .arg(suffix);

    QString temporaryFirmwarePath = QDir(m_currentSafeTempSubdirPath).filePath(simpleFileName);

    if (!m_firmwareFilePathForUpload.isEmpty() && m_firmwareFilePathForUpload != temporaryFirmwarePath && QFile::exists(m_firmwareFilePathForUpload)) {
        emit logToInterface("Удаление предыдущего временного файла: " + m_firmwareFilePathForUpload, false);
        QFile::remove(m_firmwareFilePathForUpload);
    }
    if (QFile::exists(temporaryFirmwarePath)) {
        if(!QFile::remove(temporaryFirmwarePath)){
            emit logToInterface("Предупреждение: Не удалось удалить существующий одноименный временный файл перед копированием: " + temporaryFirmwarePath, true);
        }
    }

    if (!QFile::copy(originalFirmwarePath, temporaryFirmwarePath)) {
        QFile sourceFile(originalFirmwarePath);
        QFile destFile(temporaryFirmwarePath);
        QString errorDetails = "Ошибка исходного файла: " + sourceFile.errorString() +
                               ", Ошибка файла назначения: " + destFile.errorString();
        emit logToInterface("Критическая ошибка: Не удалось скопировать файл прошивки из '" + originalFirmwarePath + "' в '" +
                                temporaryFirmwarePath + "'. " + errorDetails, true);
        return;
    }
    emit logToInterface("Файл скопирован во временный: " + temporaryFirmwarePath, false);
    m_firmwareFilePathForUpload = temporaryFirmwarePath;

    QString firmwarePathForOcd = QDir::fromNativeSeparators(m_firmwareFilePathForUpload);
    firmwarePathForOcd.replace('\\', '/');

    bool finalPathIsAscii = true;
    for (const QChar &ch : firmwarePathForOcd) {
        if (ch.unicode() > 127) {
            finalPathIsAscii = false;
            break;
        }
    }
    if (!finalPathIsAscii) {
        emit logToInterface("КРИТИЧЕСКОЕ ПРЕДУПРЕЖДЕНИЕ: Финальный путь для OpenOCD ("+firmwarePathForOcd+") содержит не-ASCII символы! Программирование может не удасться.", true);
    }
    emit logToInterface("Путь для OpenOCD (проверен на ASCII): " + firmwarePathForOcd, false);

    m_shutdownCommandSent = false;
    m_isProgramming = true;
    emit logToInterface("!! Начало программирования (файл: " + originalFileInfo.fileName() + ")", false);

    ui->lblConnectionStatus->setText("<font color='blue'><b>Прошивка\n...</b></font>");
    ui->lblConnectionStatus->setVisible(true);
    m_animationFrame = -1;
    if (!m_animationTimer->isActive()) m_animationTimer->start();
    updateLoadingAnimation();
    statusTimer->stop();

    m_receivedTelnetData.clear();
    sendOpenOcdCommand("reset halt");
    QTimer::singleShot(200, this, [this, firmwarePathForOcd]() {
        if (m_isProgramming) {
            QString programCmd = QString("program \"%1\" %2 verify reset").arg(firmwarePathForOcd).arg(m_firmwareAddress);
            sendOpenOcdCommand(programCmd);
        }
    });
}


void Unit1::stopOpenOcd() {
    bool wasConnected = m_isConnected;
    bool wasActive = m_isOpenOcdRunning || m_isConnecting || m_isProgramming || m_isConnected;

    m_autoDetectTimeoutTimer->stop();
    m_isAttemptingAutoDetect = false;
    m_detectedTargetScript.clear();

    // Сначала сбрасываем флаги состояния
    m_isOpenOcdRunning = false;
    m_isConnected = false;
    m_isConnecting = false;
    m_isProgramming = false;

    if (m_telnetSocket && m_telnetSocket->state() != QAbstractSocket::UnconnectedState) {
        emit logToInterface("Закрытие Telnet сокета...", false);
        m_telnetSocket->abort();
    }

    if (m_openOcdProcess && m_openOcdProcess->state() != QProcess::NotRunning) {
        emit logToInterface("Остановка процесса OpenOCD...", false);
        m_openOcdProcess->blockSignals(true);
        m_openOcdProcess->terminate();
        if (!m_openOcdProcess->waitForFinished(1000)) {
            emit logToInterface("Принудительное завершение OpenOCD (kill)...", true);
            m_openOcdProcess->kill();
            m_openOcdProcess->waitForFinished(500);
        }
        m_openOcdProcess->blockSignals(false);
        emit logToInterface("Процесс OpenOCD остановлен.", false);
    }

    m_animationTimer->stop();

    if (wasConnected && !m_shutdownCommandSent) {
        ui->lblConnectionStatus->setText("<font color='gray'>Отключено</font>");
    } else if (!m_shutdownCommandSent) {
        if (ui->lblConnectionStatus->text().isEmpty() || ui->lblConnectionStatus->text().contains("...")) {
            ui->lblConnectionStatus->setVisible(false);
        }
    }

    if (ui->lblConnectionStatus->isVisible() && ui->lblConnectionStatus->text().contains("Отключено")) {
        statusTimer->start(2000);
    }

    ui->btnConnect->setText("Подключить");
    ui->btnConnect->setEnabled(true);
    ui->cmbTargetMCU->setEnabled(true);

    cleanupTemporaryFile(); // Очистка временных файлов

    if (wasActive && !m_shutdownCommandSent) {
        emit logToInterface("!! OpenOCD и все связанные операции остановлены (возможно, неожиданно).", false);
    } else if (m_shutdownCommandSent) {
        emit logToInterface("!! OpenOCD штатно остановлен после команды shutdown.", false);
    }
    m_shutdownCommandSent = false;

    ui->cmbTargetMCU->setCurrentIndex(-1);
    updateUploadButtonsState();
}

void Unit1::sendOpenOcdCommand(const QString &command) {
    if (!m_telnetSocket || m_telnetSocket->state() != QAbstractSocket::ConnectedState) {
        emit logToInterface("Ошибка: Невозможно отправить команду '" + command + "', нет активного Telnet соединения.", true);

        if(m_isProgramming && (command.startsWith("program") || command.startsWith("reset halt") || command == "shutdown")) {
            emit logToInterface("Критическая ошибка Telnet во время программирования. Остановка.", true);
            m_isProgramming = false;
            QTimer::singleShot(100, this, &Unit1::stopOpenOcd);
        }
        return;
    }

    emit logToInterface("[Telnet TX] " + command, false);
    QByteArray commandData = command.toUtf8() + "\n"; // OpenOCD ждет \n

    if (m_telnetSocket->write(commandData) == -1) {
        emit logToInterface("Ошибка записи в Telnet сокет для команды: " + command, true);
        if(m_isProgramming && (command.startsWith("program") || command.startsWith("reset halt") || command == "shutdown")) {
            emit logToInterface("Критическая ошибка записи в Telnet во время программирования. Остановка.", true);
            m_isProgramming = false;
            QTimer::singleShot(100, this, &Unit1::stopOpenOcd);
        }
        return;
    }
    m_telnetSocket->flush();
}

void Unit1::handleOpenOcdStarted() {
    m_isOpenOcdRunning = true;
    emit logToInterface("Процесс OpenOCD запущен.", false);

    if (!m_isAttemptingAutoDetect) {
        QTimer::singleShot(750, this, [this]() {
            if (m_isConnecting && m_telnetSocket->state() == QAbstractSocket::UnconnectedState) {
                emit logToInterface(QString("Подключение к Telnet %1:%2...").arg(m_openOcdHost).arg(m_openOcdTelnetPort), false);
                if (!m_animationTimer->isActive() || !ui->lblConnectionStatus->text().contains("Подключение")) {
                    ui->lblConnectionStatus->setText("<font color='blue'><b>Подключение\nTelnet...</b></font>");
                    ui->lblConnectionStatus->setVisible(true);
                    if (!m_animationTimer->isActive()) m_animationTimer->start();
                    updateLoadingAnimation();
                }
                m_telnetSocket->connectToHost(m_openOcdHost, m_openOcdTelnetPort);
            }
        });
    } else { }
}

void Unit1::handleTelnetReadyRead() {
    m_receivedTelnetData.append(m_telnetSocket->readAll());
    processTelnetBuffer();
}

void Unit1::handleOpenOcdFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    QString statusMsg = (exitStatus == QProcess::NormalExit) ? "нормально" : "с ошибкой";
    m_autoDetectTimeoutTimer->stop();

    if (m_isAttemptingAutoDetect) {
        m_isAttemptingAutoDetect = false;
        m_isOpenOcdRunning = false;

        if (!m_detectedTargetScript.isEmpty()) {
            emit logToInterface(QString("Автоопределение завершено. Обнаружен MCU: %1. Переход к основному подключению.")
                                    .arg(m_detectedTargetScript), false);
            qDebug() << "handleOpenOcdFinished (autodetect success): Proceeding with script:" << m_detectedTargetScript;
            int idx = ui->cmbTargetMCU->findData(m_detectedTargetScript);
            if (idx != -1) {
                ui->cmbTargetMCU->setCurrentIndex(idx);
            } else {
                emit logToInterface("Предупреждение: Определенный скрипт " + m_detectedTargetScript + " не найден в списке MCU.", true);
            }
            proceedWithConnection(m_detectedTargetScript);
        } else {
            qDebug() << "handleOpenOcdFinished (autodetect FAILED): m_detectedTargetScript is EMPTY.";
            emit logToInterface("Автоопределение MCU не удалось. OpenOCD завершился без определения типа.", true);
            m_isConnecting = false;
            m_animationTimer->stop();
            ui->lblConnectionStatus->setText("<font color='orange'><b>MCU<br>не опр.</br></b></font>");
            statusTimer->start(3000);
            ui->btnConnect->setEnabled(true);
            ui->cmbTargetMCU->setEnabled(true);
            updateUploadButtonsState();
        }
        return;
    }

    m_isOpenOcdRunning = false;
    bool isUnexpectedError = (exitCode != 0 || exitStatus != QProcess::NormalExit);
    emit logToInterface(QString("Процесс OpenOCD завершен (Код: %1, Статус: %2).")
                            .arg(exitCode).arg(statusMsg), isUnexpectedError && !m_shutdownCommandSent);

    bool wasConnectingBeforeTelnet = m_isConnecting && !m_isConnected;

    if (m_shutdownCommandSent) {
        emit logToInterface("OpenOCD корректно завершил работу после команды shutdown.", false);
        m_isConnecting = false;
        m_isConnected = false;
        m_animationTimer->stop();
        ui->btnConnect->setText("Подключить");
        ui->btnConnect->setEnabled(true);
        ui->cmbTargetMCU->setEnabled(true);
        ui->cmbTargetMCU->setCurrentIndex(-1);
        cleanupTemporaryFile();
        m_shutdownCommandSent = false;
    } else if (wasConnectingBeforeTelnet) {
        emit logToInterface("OpenOCD неожиданно завершился или не смог остаться запущенным до Telnet соединения.", true);
        m_isConnecting = false;
        m_animationTimer->stop();
        ui->lblConnectionStatus->setText("<font color='red'><b>Ошибка<br>OpenOCD</b></font>"); // Более информативно
        statusTimer->start(4000);
        ui->btnConnect->setEnabled(true);
        ui->cmbTargetMCU->setEnabled(true);
    }

    updateUploadButtonsState();
}

void Unit1::handleOpenOcdError(QProcess::ProcessError error) {
    emit logToInterface("Критическая ошибка процесса OpenOCD: " + m_openOcdProcess->errorString() + QString(" (Код: %1)").arg(error), true);

    m_autoDetectTimeoutTimer->stop();
    m_isAttemptingAutoDetect = false;
    m_detectedTargetScript.clear();

    m_animationTimer->stop();
    ui->lblConnectionStatus->setText("<font color='red'><b>Крит.<br>ошибка<br>OpenOCD</b></font>");
    ui->lblConnectionStatus->setVisible(true);
    statusTimer->start(5000);

    stopOpenOcd();
}

void Unit1::handleOpenOcdStdOut() {
    QByteArray data = m_openOcdProcess->readAllStandardOutput();
    QString message = QString::fromLocal8Bit(data).trimmed();
    if (!message.isEmpty()) {
        if (m_isAttemptingAutoDetect && m_detectedTargetScript.isEmpty()) {
            processOpenOcdOutputForDetection(message);
        }
        emit logToInterface("[OOCD] " + message, false);
    }
}

void Unit1::handleOpenOcdStdErr() {
    QByteArray data = m_openOcdProcess->readAllStandardError();
    QString message = QString::fromLocal8Bit(data).trimmed();
    if (!message.isEmpty()) {
        if (m_isAttemptingAutoDetect && m_detectedTargetScript.isEmpty()) {
            processOpenOcdOutputForDetection(message);
        }
        bool isError = false;
        if (message.startsWith("Error:")) { isError = true; }
        else if (message.startsWith("Warn :")) { isError = false; }
        else if (message.contains("failed", Qt::CaseInsensitive) ||
                 message.contains("Can't find", Qt::CaseInsensitive) ||
                 message.contains("couldn't open", Qt::CaseInsensitive)) {
            isError = true;
        }
        if (message.contains("Unable to match requested speed")) { isError = false; }
        if (message.contains("shutdown command invoked")) { isError = false; }

        emit logToInterface("[OOCD ERR] " + message, isError);
    }
}

void Unit1::handleTelnetConnected() {
    emit logToInterface("Telnet соединение установлено.", false);

    m_isConnected = true;
    m_isConnecting = false;

    m_animationTimer->stop();
    ui->lblConnectionStatus->setText("<font color='green'><b>Успешно<br>✓</br></b></font>");
    statusTimer->start(2000);

    ui->btnConnect->setText("Отключить");
    ui->btnConnect->setEnabled(true);
    ui->cmbTargetMCU->setEnabled(false);

    updateUploadButtonsState();
}

void Unit1::handleTelnetDisconnected() {
    if (m_shutdownCommandSent) {
        emit logToInterface("Telnet соединение закрыто OpenOCD.", false);
        m_isConnected = false;
    }
    if (m_isConnected) {
        emit logToInterface("Telnet соединение с OpenOCD неожиданно разорвано.", true);
        m_animationTimer->stop();
        ui->lblConnectionStatus->setText("<font color='orange'><b>Разрыв<br>связи ?</b></font>");
        ui->lblConnectionStatus->setVisible(true);
        statusTimer->start(3000);
        stopOpenOcd();
    } else if (m_isConnecting) {
        emit logToInterface("Telnet отключился во время попытки соединения.", true);
        m_animationTimer->stop();
        ui->lblConnectionStatus->setText("<font color='red'><b>Ошибка<br>Telnet ✗</b></font>");
        ui->lblConnectionStatus->setVisible(true);
        statusTimer->start(3000);
        stopOpenOcd();
    }
}

void Unit1::handleTelnetError(QAbstractSocket::SocketError socketError) {
    if (m_shutdownCommandSent && socketError == QAbstractSocket::RemoteHostClosedError) {
        emit logToInterface("Telnet соединение закрыто OpenOCD после команды shutdown.", false);
        return;
    }

    emit logToInterface("Ошибка Telnet сокета: " + m_telnetSocket->errorString() + QString(" (Код: %1)").arg(socketError), true);
    m_animationTimer->stop();

    if (m_isConnecting) {
        ui->lblConnectionStatus->setText("<font color='red'><b>Ошибка<br>Telnet ✗</b></font>");
    } else if (m_isConnected) {
        ui->lblConnectionStatus->setText("<font color='red'><b>Ошибка<br>Telnet ✗</b></font>");
    }
    statusTimer->start(3000);
    stopOpenOcd();
}

void Unit1::processTelnetBuffer() {
    if (m_telnetSocket->bytesAvailable() > 0) {
        m_receivedTelnetData.append(m_telnetSocket->readAll());
    }

    QString fullBuffer = QString::fromUtf8(m_receivedTelnetData);

    if (m_isProgramming && !m_shutdownCommandSent) {
        bool outcomeDetected = false;
        bool verifyOk = fullBuffer.contains("Verified OK", Qt::CaseInsensitive) ||
                        fullBuffer.contains("verification succeded", Qt::CaseInsensitive) ||
                        fullBuffer.contains("verified", Qt::CaseInsensitive);
        bool programFailedGeneral = fullBuffer.contains("failed", Qt::CaseInsensitive) ||
                                    fullBuffer.contains("error:", Qt::CaseInsensitive);
        bool programFailedSpecific = fullBuffer.contains("** Programming Failed **", Qt::CaseInsensitive) ||
                                     fullBuffer.contains("Error: couldn't open", Qt::CaseInsensitive) ||
                                     fullBuffer.contains("timed out while waiting for target halted", Qt::CaseInsensitive);

        if (programFailedSpecific || (programFailedGeneral && fullBuffer.contains("program", Qt::CaseInsensitive))) {
            if (fullBuffer.contains("TARGET: stm32") && fullBuffer.contains("- Not halted") && !programFailedSpecific) {
            } else {
                emit logToInterface("Ошибка программирования/верификации!", true);
                m_animationTimer->stop();
                ui->lblConnectionStatus->setText("<font color='red'><b>Прошивка<br>✗</b></font>");
                outcomeDetected = true;
            }
        } else if (verifyOk) {
            if (fullBuffer.contains("error", Qt::CaseInsensitive) && fullBuffer.contains("verified", Qt::CaseInsensitive)) {
            } else {
                emit logToInterface("Программирование и верификация успешно завершены!", false);
                m_animationTimer->stop();
                ui->lblConnectionStatus->setText("<font color='green'><b>Прошивка<br>✓</b></font>");
                outcomeDetected = true;
            }
        }


        if (outcomeDetected) {
            m_isProgramming = false;
            statusTimer->start(5000);

            emit logToInterface("Отправка команды shutdown в OpenOCD...", false);
            sendOpenOcdCommand("shutdown");
            m_shutdownCommandSent = true;
        }
    }

    int promptPos;
    while ((promptPos = m_receivedTelnetData.indexOf("\n> ")) != -1) {
        QByteArray messageBytes = m_receivedTelnetData.left(promptPos);
        QString message = QString::fromUtf8(messageBytes).trimmed();

        m_receivedTelnetData.remove(0, promptPos + 3);

        if (!message.isEmpty()) {
            bool isError = message.startsWith("Error:", Qt::CaseInsensitive) ||
                           message.contains("failed", Qt::CaseInsensitive) ||
                           message.contains("timed out", Qt::CaseInsensitive) ||
                           message.contains("Can't find", Qt::CaseInsensitive);
            bool isWarning = message.startsWith("Warn :", Qt::CaseInsensitive);
            if (message.contains("clearing lockup after double fault")) isError = true;
            if (message.contains("xPSR: 0x01000003")) isError = true;

            emit logToInterface("[Telnet] " + message, isError || isWarning);
        }
    }
}

void Unit1::cleanupTemporaryFile() {
    if (!m_firmwareFilePathForUpload.isEmpty()) {
        QFile tempFile(m_firmwareFilePathForUpload);
        if (tempFile.exists()) {
            emit logToInterface("Удаление временного файла прошивки: " + m_firmwareFilePathForUpload, false);
            if (!tempFile.remove()) {
                emit logToInterface("Предупреждение: Не удалось удалить временный файл: " + m_firmwareFilePathForUpload + ". Ошибка: "
                                        + tempFile.errorString(), true);
            }
        }
        m_firmwareFilePathForUpload.clear();
    }

    if (!m_currentSafeTempSubdirPath.isEmpty()) {
        QDir tempSubDir(m_currentSafeTempSubdirPath);
        if (tempSubDir.exists() && tempSubDir.dirName() == TEMP_SUBDIR_NAME_OPENOCD) {
            tempSubDir.setFilter(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);
            if (tempSubDir.entryInfoList().isEmpty()) {
                QDir parentOfTempSubDir = tempSubDir;
                if (parentOfTempSubDir.cdUp()) {
                    emit logToInterface("Попытка удаления пустой временной подпапки: " + tempSubDir.path(), false);
                    if (parentOfTempSubDir.rmdir(tempSubDir.dirName())) {
                        emit logToInterface("Временная подпапка успешно удалена: " + tempSubDir.path(), false);
                        m_currentSafeTempSubdirPath.clear();
                    } else {
                        emit logToInterface("Предупреждение: Не удалось удалить пустую временную подпапку: " + tempSubDir.path() +
                                                ". Возможно, используется другим процессом или ошибка прав.", true);
                    }
                } else {
                    emit logToInterface("Предупреждение: Не удалось перейти к родительскому каталогу для удаления подпапки: "
                                            + tempSubDir.path(), true);
                }
            } else {
                emit logToInterface("Временная подпапка не пуста, не удаляем: " + tempSubDir.path(), false);
            }
        }
    }

    if (!m_isProgramming) {
        m_originalFirmwarePathForLog.clear();
    }
}

void Unit1::hideConnectionStatus()
{
    if (m_animationTimer->isActive()) {
        m_animationTimer->stop();
    }

    ui->lblConnectionStatus->setVisible(false);
    ui->lblConnectionStatus->setText("");
}

void Unit1::updateLoadingAnimation() {
    m_animationFrame = (m_animationFrame + 1) % 4;
    QString text = "<font color='blue'><b>...";

    for (int i = 0; i < m_animationFrame; ++i) {
        text += ".";
    }
    text += "<b></font>";

    ui->lblConnectionStatus->setText(text);
}

// ----------------------- Unit1 Перенесённый

void Unit1::onBtnShowInfoClicked()
{
    QString title = tr("Инфо");
    QString message = tr("Создание файлов первоначальной прошивки для устройств серий КР, МД, ИН.\n(Максимальный размер файла прошивки 1024 Кб)");
    QMessageBox::information(ui->btnShowInfo->window(), title, message);
}

void Unit1::loadConfig(const QString &filePath)
{
    ui->cmbRevision->blockSignals(true);
    ui->cmbRevision->clear();
    revisionsMap.clear();
    autoSavePaths.clear();
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
    QStringList categories;

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

    ui->cmbRevision->blockSignals(true);
    ui->cmbRevision->addItems(categories);

    if (!revisionsMap.contains("OthDev")) {
        ui->cmbRevision->addItem("OthDev");
        revisionsMap["OthDev"] = RevisionInfo{ "", "", "" };
    }

    ui->cmbRevision->blockSignals(false);
    ui->cmbRevision->setCurrentIndex(-1);
    qInfo() << "Unit1 loaded" << revisionsMap.count() << "revisions.";
}

void Unit1::onBtnClearRevisionClicked()
{
    if (autoSavePaths.isEmpty()) {
        QMessageBox::information(ui->btnClearRevision->window(), tr("Очистка"),
                                 tr("Нет информации о путях для автоматического сохранения (проверьте config.xml)."));
        return;
    }
    QStringList pathsToDeleteDisplay;
    QString currentDir = QDir::currentPath();
    for (const QString &pathFragment : std::as_const(autoSavePaths)) {
        pathsToDeleteDisplay << QDir::toNativeSeparators(pathFragment);
    }

    QString confirmationMessage = tr("Будут рекурсивно удалены все файлы и подпапки в следующих директориях (относительно папки программы):"
                                     "\n\n%1\n\nВы уверены, что хотите продолжить?")
                                      .arg(pathsToDeleteDisplay.join("\n"));

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(ui->btnClearRevision->window(), tr("Подтверждение очистки"),
                                  confirmationMessage, QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply != QMessageBox::Yes) { qInfo() << "Очистка отменена пользователем."; return; }

    qInfo() << "Начало очистки автоматически сохраненных файлов...";

    int successCount = 0; int failCount = 0; QStringList errorDetails;
    for (const QString &pathFragment : std::as_const(autoSavePaths)) {
        QString fullPathBase = QDir(currentDir).filePath(pathFragment);
        QDir baseDir(fullPathBase);
        if (baseDir.exists()) {
            qInfo() << "Удаление базовой папки:" << QDir::toNativeSeparators(fullPathBase);
            if (baseDir.removeRecursively()) { qInfo() << "Успешно."; successCount++; }
            else { qWarning() << "Не удалось удалить:" << QDir::toNativeSeparators(fullPathBase); failCount++; errorDetails <<
                    QDir::toNativeSeparators(pathFragment) + " (базовая)"; }
        } else { qInfo() << "Базовая папка не найдена, пропуск:" << QDir::toNativeSeparators(fullPathBase); }
    }

    QString resultMessage;
    if (failCount == 0 && successCount > 0) { resultMessage = tr("Автоматически сохраненные файлы и папки (%1) успешно удалены.").arg(successCount);
        QMessageBox::information(ui->btnClearRevision->window(), tr("Очистка завершена"), resultMessage); }
    else if (failCount > 0) { resultMessage = tr("Не удалось удалить %1 папок:\n%2").arg(failCount).arg(errorDetails.join("\n"));
        if (successCount > 0) resultMessage += tr("\n\nУспешно удалено: %1 папок.").arg(successCount);
        QMessageBox::warning(ui->btnClearRevision->window(), tr("Ошибка очистки"), resultMessage); }
    else { resultMessage = tr("Не найдено папок для удаления."); QMessageBox::information(ui->btnClearRevision->window(), tr("Очистка"), resultMessage); }
    qInfo() << "Очистка завершена.";
}

void Unit1::updateProgramDataFileSize(const QString &filePath)
{
    if (filePath.isEmpty() || filePath.startsWith("Файл:") || filePath == "-") {
        ui->lblProgramDataFileSize->setText("Размер: -"); return;
    }

    QString absPath = QDir::isRelativePath(filePath) ? QDir(QDir::currentPath()).filePath(filePath) : filePath;
    QFileInfo fileInfo(absPath);

    if (fileInfo.exists() && fileInfo.isFile()) {
        qint64 size = fileInfo.size();
        ui->lblProgramDataFileSize->setText("Размер: " + QString::number(size) + tr(" байт"));
    } else {
        ui->lblProgramDataFileSize->setText(tr("Файл не найден"));
    }
}

void Unit1::onRevisionChanged(int index)
{
    QString selected;

    if (index < 0 || index >= ui->cmbRevision->count()) {
        selected = "OthDev"; // Считаем, что выбор сброшен или индекс невалиден
    } else {
        selected = ui->cmbRevision->itemText(index); // Получаем текст выбранного элемента
    }
    qDebug() << "Unit1::onRevisionChanged - selected:" << selected;

    if (selected == "OthDev") {
        ui->lblLoaderFileName->setText("Файл: -");
        ui->lblProgramDataFileName->setText("Файл: -");
        ui->lblLoaderFileSize->setText("Размер: -");
        ui->lblProgramDataFileSize->setText("Размер: -");
        ui->lblTotalFirmwareSize->setText("Суммарный размер файла прошивки: -");
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
        ui->lblTotalFirmwareSize->setText("Суммарный размер файла прошивки: -");
        SaveFirmware.clear();
    }
}

void Unit1::updateLoaderFileSize(const QString &filePath)
{
    if (filePath.isEmpty() || filePath.startsWith("Файл:") || filePath == "-") {
        ui->lblLoaderFileSize->setText("Размер: -"); return;
    }

    QString absPath = QDir::isRelativePath(filePath) ? QDir(QDir::currentPath()).filePath(filePath) : filePath;
    QFileInfo fileInfo(absPath);

    if (fileInfo.exists() && fileInfo.isFile()) {
        qint64 size = fileInfo.size();
        ui->lblLoaderFileSize->setText("Размер: " +QString::number(size) + tr(" байт"));
    } else {
        ui->lblLoaderFileSize->setText(tr("Файл не найден"));
    }
}

void Unit1::updateTotalFirmwareSize()
{
    qint64 totalSize = 0;
    QString currentDir = QDir::currentPath();
    QString selectedCategory = ui->cmbRevision->currentText();

    QString loaderFilePath = "";
    QString programDataFilePath = "";

    if (!selectedCategory.isEmpty() && selectedCategory != "OthDev" && revisionsMap.contains(selectedCategory))
    {
        const RevisionInfo& info = revisionsMap.value(selectedCategory);
        loaderFilePath = info.bootloaderFile;
        programDataFilePath = info.mainProgramFile;
    }
    else if (selectedCategory == "OthDev" || selectedCategory.isEmpty())
    {
        QString loaderFilePathRel = ui->lblLoaderFileName->text();
        if (!loaderFilePathRel.startsWith("Файл:") && !loaderFilePathRel.isEmpty() && loaderFilePathRel != "-") {
            loaderFilePath = loaderFilePathRel; // Используем путь из метки
        }

        QString programDataFilePathRel = ui->lblProgramDataFileName->text();
        if (!programDataFilePathRel.startsWith("Файл:") && !programDataFilePathRel.isEmpty() && programDataFilePathRel != "-") {
            programDataFilePath = programDataFilePathRel; // Используем путь из метки
        }
    }

    if (!loaderFilePath.isEmpty()) {
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
        QString absPath = QDir::isRelativePath(programDataFilePath) ? QDir(currentDir).filePath(programDataFilePath) : programDataFilePath;
        QFileInfo programDataFileInfo(absPath);
        if (programDataFileInfo.exists() && programDataFileInfo.isFile()) {
            totalSize += programDataFileInfo.size();
        } else {
            qWarning() << "updateTotalFirmwareSize: Program data file not found at" << absPath;
        }
    }

    if (totalSize > 0) {
        double totalSizeKB = static_cast<double>(totalSize) / 1024.0;
        ui->lblTotalFirmwareSize->setText(tr("Суммарный размер файла прошивки: %1 КБ (%2 байт)")
                                              .arg(QString::number(totalSizeKB, 'f', 2))
                                              .arg(totalSize));
    } else {
        ui->lblTotalFirmwareSize->setText(tr("Суммарный размер файла прошивки: -"));
    }
}

void Unit1::onBtnChooseProgramDataFileClicked()
{
    QString initialDir = QDir::currentPath();
    QString currentFilePath = ui->lblProgramDataFileName->text();

    if (!currentFilePath.startsWith("Файл:") && !currentFilePath.isEmpty() && QFileInfo(currentFilePath).exists()) {
        initialDir = QFileInfo(currentFilePath).absolutePath();
    } else {
        initialDir = QDir(initialDir).filePath("MainProgram_File");
        QDir().mkpath(initialDir);
    }

    QString filePath = QFileDialog::getOpenFileName(
        ui->cmbRevision->window(),
        tr("Выберите файл программы данных"),
        initialDir,
        tr("Файлы прошивки (*.bin *.hex);;Все файлы (*)")
        );

    if (filePath.isEmpty()) return;
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) { QMessageBox::critical(ui->cmbRevision->window(), tr("Ошибка"), tr("Файл не найден или недоступен.")); return; }
    qint64 size = fileInfo.size();
    if (size > ((1024 - 16) * 1024)) { QMessageBox::critical(ui->cmbRevision->window(), tr("Ошибка"), tr("Файл больше максимально допустимого размера (1008 Кб)")); return; }

    ui->lblProgramDataFileName->setText( QDir::toNativeSeparators(filePath) );

    updateProgramDataFileSize(filePath);
    updateTotalFirmwareSize();
}

void Unit1::onBtnChooseLoaderFileClicked()
{
    QString initialDir = QDir::currentPath();
    QString currentFilePath = ui->lblLoaderFileName->text();

    if (!currentFilePath.startsWith("Файл:") && !currentFilePath.isEmpty() && QFileInfo::exists(currentFilePath)) {
        initialDir = QFileInfo(currentFilePath).absolutePath();
    } else {
        initialDir = QDir(initialDir).filePath("BootLoader_File");
        QDir().mkpath(initialDir);
    }

    QString filePath = QFileDialog::getOpenFileName(
        ui->cmbRevision->window(),
        tr("Выберите файл загрузчика"),
        initialDir,
        tr("Файлы загрузчика (*.bin *.hex);;Все файлы (*)")
        );

    if (filePath.isEmpty()) return;

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) { QMessageBox::critical(ui->cmbRevision->window(), tr("Ошибка"), tr("Файл не найден или недоступен.")); return; }
    qint64 size = fileInfo.size();
    if (size > (16 * 1024)) { QMessageBox::critical(ui->cmbRevision->window(), tr("Ошибка"), tr("Файл больше максимально допустимого размера (16 Кб)")); return; }

    ui->lblLoaderFileName->setText( QDir::toNativeSeparators(filePath) );

    updateLoaderFileSize(filePath);
    updateTotalFirmwareSize();
}

#pragma pack(push, 1)
struct ProgInfo_Original { // Назовем чуть иначе, чтобы избежать конфликта имен если ProgInfo уже где-то есть
    quint32 tableID;
    quint32 programSize;
    quint32 programCRC; // Это должен быть финальный CRC программы (с ~)
    quint32 tableCRC;   // Это должен быть финальный CRC заголовка (с ~)
};
#pragma pack(pop)

// ВАЖНО: Используем ProgInfo_Original и оригинальную логику CRC
void Unit1::createFirmwareFiles(const QString &outputDir)
{
    QString serialBeginStr = ui->editInitialSerialNumber->text().trimmed();
    bool ok;
    int serialBegin = serialBeginStr.toInt(&ok);
    if (serialBeginStr.isEmpty() || !ok || serialBegin <= 0) { QMessageBox::critical(ui->cmbRevision->window(),
                              "Ошибка", "Неверный или не указан начальный серийный номер."); return; }
    QString serialCountStr = ui->editNumberOfSerials->text().trimmed();
    int serialCount = serialCountStr.toInt(&ok);
    if (serialCountStr.isEmpty() || !ok || serialCount <= 0) { QMessageBox::critical(ui->cmbRevision->window(),
                              "Ошибка", "Неверное или не указано количество номеров."); return; }

    // Получаем пути из UI (могут быть относительными)
    QString programFilePathRel = ui->lblProgramDataFileName->text();
    QString loaderFilePathRel = ui->lblLoaderFileName->text();
    if (programFilePathRel.startsWith("Файл:") || programFilePathRel.isEmpty() || programFilePathRel == "-") { QMessageBox::critical(ui->cmbRevision->window(),
                              "Ошибка", "Файл программы не выбран."); return; }
    if (loaderFilePathRel.startsWith("Файл:") || loaderFilePathRel.isEmpty() || loaderFilePathRel == "-") { QMessageBox::critical(ui->cmbRevision->window(),
                              "Ошибка", "Файл загрузчика не выбран."); return; }

    QString currentDir = QDir::currentPath();
    QString programFilePathAbs = QDir(currentDir).filePath(programFilePathRel);
    QString loaderFilePathAbs = QDir(currentDir).filePath(loaderFilePathRel);

    QFileInfo programFile(programFilePathAbs);
    QFileInfo loaderFile(loaderFilePathAbs);
    const qint64 minFileSize = 4 * 1024; // Оригинальная проверка

    if (!programFile.exists() || !programFile.isFile() || programFile.size() < minFileSize) { QMessageBox::critical(ui->cmbRevision->window(),
                              "Ошибка", QString("Ошибка файла программы '%1' (%2 Кб).").arg(programFile.fileName()).arg(minFileSize / 1024)); return; }
    if (!loaderFile.exists() || !loaderFile.isFile() || loaderFile.size() < minFileSize) { QMessageBox::critical(ui->cmbRevision->window(),
                              "Ошибка", QString("Ошибка файла загрузчика '%1' (%2 Кб).").arg(loaderFile.fileName()).arg(minFileSize / 1024)); return; }

    // Проверки макс. размера
    if (programFile.size() > ((1024 - 16) * 1024)) { QMessageBox::critical(ui->cmbRevision->window(),
                              "Ошибка", "Файл программы больше максимально допустимого размера (1008 Кб)."); return; }
    if (loaderFile.size() > (16 * 1024)) { QMessageBox::critical(ui->cmbRevision->window(),
                              "Ошибка", "Файл загрузчика больше максимально допустимого размера (16 Кб)."); return; }

    QByteArray loaderData = loadFile(loaderFile.absoluteFilePath());
    QByteArray programData = loadFile(programFile.absoluteFilePath());
    if (loaderData.isEmpty()) { QMessageBox::critical(ui->cmbRevision->window(),
                              "Ошибка", "Не удалось прочитать файл загрузчика: " + loaderFile.absoluteFilePath()); return; }
    if (programData.isEmpty()) { QMessageBox::critical(ui->cmbRevision->window(),
                              "Ошибка", "Не удалось прочитать файл программы: " + programFile.absoluteFilePath()); return; }

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
    if (actualDataSize > firmwareBufferSize) { QMessageBox::critical(ui->cmbRevision->window(),
                              "Ошибка", QString("Общий размер прошивки (%1 байт) превышает максимальный размер буфера (%2 байт).").arg(actualDataSize).arg(firmwareBufferSize)); return; }
    if (progInfoOffset + qsizetype(sizeof(ProgInfo_Original)) > firmwareBufferSize) { QMessageBox::critical(ui->cmbRevision->window(),
                              "Ошибка", "Смещение таблицы ProgInfo выходит за пределы буфера."); return; }
    if (serialNumberOffset + serialNumberBlockSize > firmwareBufferSize) { QMessageBox::critical(ui->cmbRevision->window(),
                              "Ошибка", "Смещение серийного номера выходит за пределы буфера."); return; }

    QByteArray baseFirmwareBuffer(firmwareBufferSize, '\xFF'); // Заполняем FF
    qsizetype loaderSizeToCopy = qMin((qsizetype)loaderData.size(), loaderMaxSize);
    memcpy(baseFirmwareBuffer.data(), loaderData.constData(), loaderSizeToCopy);
    memcpy(baseFirmwareBuffer.data() + programOffset, programData.constData(), programData.size());

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

    QDir dir(outputDir); // outputDir - абсолютный путь
    if (!dir.exists()) { if (!dir.mkpath(".")) { QMessageBox::critical(ui->cmbRevision->window(), "Ошибка", "Не удалось создать папку для сохранения: " + outputDir); return; } }

    int filesCreated = 0;
    ProgInfo_Original progInfo; // Используем локально определенную структуру

    for (int i = 0; i < serialCount; ++i) {
        QByteArray currentFirmware = baseFirmwareBuffer; // Копируем базовый буфер

        int currentSerialInt = serialBegin + i; QString serialStr = QString::number(currentSerialInt);
        QByteArray serialBytesRaw = serialStr.toLatin1();
        memset(currentFirmware.data() + serialNumberOffset, '\0', serialNumberClearSize); // Очистка 63 байт
        qsizetype bytesToCopy = qMin((qsizetype)serialBytesRaw.size(), serialNumberClearSize); // Копируем не более 63
        memcpy(currentFirmware.data() + serialNumberOffset, serialBytesRaw.constData(), bytesToCopy);

        progInfo.tableID = qToLittleEndian(0x52444C42); // "BLDR"
        progInfo.programSize = qToLittleEndian(static_cast<quint32>(programData.size()));
        QByteArray programDataInBuff = QByteArray::fromRawData(currentFirmware.constData() + programOffset, programData.size());
        progInfo.programCRC = qToLittleEndian(CrcUnit::calcCrc32(programDataInBuff)); // Финальный CRC программы (с ~)
        QByteArray progInfoHeadForCRC = QByteArray::fromRawData((const char*)&progInfo, sizeof(quint32) * 3);
        progInfo.tableCRC = qToLittleEndian(CrcUnit::calcCrc32(progInfoHeadForCRC)); // Финальный CRC заголовка (с ~)
        memcpy(currentFirmware.data() + progInfoOffset, (const char*)&progInfo, sizeof(ProgInfo_Original));

        qsizetype currentFileSize = actualDataSize; // loaderMaxSize + programData.size()

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

            // В оригинале цикл шел по байтам intermediateCRC, записанным в конец буфера.
            // Эмулируем это, используя массив байт intermediateCRCBytesLE
            for (int k = 3; k >= 0; --k) { // Цикл по 4 байтам CRC
                patchValue = (patchValue << 8) ^ inline_revTable[patchValue >> 24] ^ static_cast<quint32>(intermediateCRCBytesLE[k]);
            }

            // Ручная запись патча (patchValue) В КОНЕЦ БУФЕРА
            // Увеличиваем размер буфера на 4 байта
            currentFirmware.resize(currentFileSize + 4);
            unsigned char* patchDest = reinterpret_cast<unsigned char*>(currentFirmware.data() + currentFileSize); // Указатель на место для патча
            // Записываем патч в Little Endian
            patchDest[0] = (patchValue >> 0)  & 0xFF;
            patchDest[1] = (patchValue >> 8)  & 0xFF;
            patchDest[2] = (patchValue >> 16) & 0xFF;
            patchDest[3] = (patchValue >> 24) & 0xFF;

            currentFileSize += 4;

#ifdef QT_DEBUG
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
            currentFirmware.resize(currentFileSize);
        }

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

    }

    if (filesCreated > 0) { QMessageBox::information(ui->cmbRevision->window(), "Готово", QStringLiteral("%1 файл(ов) прошивки успешно создан(о) в папке:\n%2").arg(filesCreated).arg(QDir::toNativeSeparators(outputDir))); }
    else if (serialCount > 0) { QMessageBox::warning(ui->cmbRevision->window(), "Завершено", "Не было создано ни одного файла (возможно, из-за ошибок)."); }
}

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

/* Функция calculateReverseCRC из оригинала (если она нужна для чего-то еще)
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
} */

QByteArray Unit1::loadFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Error opening file for reading:" << filePath << file.errorString();
        return QByteArray();
    }
    QByteArray data = file.readAll();
    file.close();
    if (data.isEmpty() && QFileInfo(filePath).size() > 0) {
        qWarning() << "Read 0 bytes from non-empty file:" << filePath;
    }
    return data;
}

void Unit1::onBtnCreateFileManualClicked()
{
    QString dir = QFileDialog::getExistingDirectory(ui->cmbRevision->window(), tr("Выберите папку для сохранения прошивок"), QDir::currentPath());
    if (dir.isEmpty()) return;
    createFirmwareFiles(dir);
}

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

    createFirmwareFiles(outDirAbs);
}
