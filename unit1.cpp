#include "unit1.h"
#include "crcunit.h"

const char TELNET_CMD_TERMINATOR = '\x1a';

Unit1::Unit1(Ui::MainWindow *ui, QObject *parent)
    : QObject(parent), ui(ui)
{

    // Инициализация таймера
    statusTimer = new QTimer(this);
    statusTimer->setSingleShot(true);
    connect(statusTimer, &QTimer::timeout, this, &Unit1::hideConnectionStatus);

    // --- Determine OpenOCD paths relative to application directory ---
    QString appDir = QCoreApplication::applicationDirPath();
    m_openOcdDir = QDir(appDir).filePath("openocd"); // Expect 'openocd' folder next to exe
    m_openOcdExecutablePath = QDir(m_openOcdDir).filePath("bin/openocd.exe");
    m_openOcdScriptsPath = QDir(m_openOcdDir).filePath("openocd/scripts");
    m_shutdownCommandSent = false;

    qDebug() << "Application directory:" << appDir;
    qDebug() << "Expected OpenOCD executable:" << m_openOcdExecutablePath;
    qDebug() << "Expected OpenOCD scripts:" << m_openOcdScriptsPath;

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

    connect(ui->btnConnect, &QPushButton::clicked, this, &Unit1::onBtnConnectClicked);
    ui->btnConnect->setEnabled(true);
    connect(ui->btnUpload, &QPushButton::clicked, this, &Unit1::onBtnUploadClicked);
    ui->btnUpload->setEnabled(false); // Disabled initially

    ui->cmbTargetMCU->clear();
    ui->cmbTargetMCU->addItem("STM32L4x Series", "target/stm32l4x.cfg");
    ui->cmbTargetMCU->addItem("STM32F1x Series", "target/stm32f1x.cfg");
    ui->cmbTargetMCU->addItem("STM32F4x Series", "target/stm32f4x.cfg");
    ui->cmbTargetMCU->addItem("STM32G4x Series", "target/stm32g4x.cfg");
    ui->cmbTargetMCU->addItem("STM32H7x Series", "target/stm32h7x.cfg");
    ui->cmbTargetMCU->setCurrentIndex(0); // Select placeholder initially
}

Unit1::~Unit1()
{
    stopOpenOcd();
    cleanupTemporaryFile();
}

bool Unit1::checkOpenOcdPrerequisites() {
    // 1. Check if OpenOCD executable exists
    if (!QFile::exists(m_openOcdExecutablePath)) {
        emit logToInterface("Критическая ошибка: Файл openocd.exe не найден по пути: " + m_openOcdExecutablePath, true);
        emit logToInterface("Убедитесь, что папка 'openocd' со всеми файлами находится рядом с программой.", true);
        QMessageBox::critical(nullptr, "Ошибка OpenOCD", "Не найден исполняемый файл OpenOCD.\n" + m_openOcdExecutablePath + "\n\nПоместите папку 'openocd' рядом с программой.");
        return false;
    }
    // 2. Check if base scripts directory exists
    if (!QDir(m_openOcdScriptsPath).exists()) {
        emit logToInterface("Критическая ошибка: Папка со скриптами OpenOCD не найдена: " + m_openOcdScriptsPath, true);
        QMessageBox::critical(nullptr, "Ошибка OpenOCD", "Не найдена папка со скриптами OpenOCD.\n" + m_openOcdScriptsPath);
        return false;
    }
    // 3. Check if interface script exists
    QString fullInterfaceScriptPath = QDir(m_openOcdScriptsPath).filePath(m_interfaceScript);
    if (!QFile::exists(fullInterfaceScriptPath)) {
        emit logToInterface("Критическая ошибка: Файл скрипта интерфейса не найден: " + fullInterfaceScriptPath, true);
        QMessageBox::critical(nullptr, "Ошибка OpenOCD", "Не найден файл скрипта интерфейса ST-Link:\n" + fullInterfaceScriptPath);
        return false;
    }

    // 4. Check if a target MCU is selected in the ComboBox
    if (!ui->cmbTargetMCU || ui->cmbTargetMCU->currentIndex() <= 0) { // Index 0 is placeholder
        emit logToInterface("Ошибка: Не выбран целевой микроконтроллер (MCU).", true);
        QMessageBox::warning(nullptr, "Требуется выбор MCU", "Пожалуйста, выберите тип целевого STM32 из списка.");
        return false;
    }

    // 5. Check if the selected target script exists
    QString targetScript = ui->cmbTargetMCU->currentData().toString();
    QString fullTargetScriptPath = QDir(m_openOcdScriptsPath).filePath(targetScript);
    if (!QFile::exists(fullTargetScriptPath)) {
        emit logToInterface("Критическая ошибка: Выбранный файл скрипта цели не найден: " + fullTargetScriptPath, true);
        QMessageBox::critical(nullptr, "Ошибка OpenOCD", "Не найден файл скрипта для выбранного MCU:\n" + fullTargetScriptPath);
        return false;
    }


    return true; // All checks passed
}

void Unit1::onBtnConnectClicked() {
    if (m_isOpenOcdRunning || m_isConnecting) {
        emit logToInterface("OpenOCD уже запущен или идет подключение.", true);
        return;
    }

    // --- Check prerequisites before starting ---
    if (!checkOpenOcdPrerequisites()) {
        return; // Stop if checks fail
    }

    // --- Get selected target script ---
    QString targetScript = ui->cmbTargetMCU->currentData().toString();


    // --- Clear previous state/logs ---
    ui->logTextEdit->clear();
    m_isOpenOcdRunning = true; // Mark that we INTEND to run OpenOCD
    m_isConnected = false;
    m_isConnecting = true;
    m_isProgramming = false;
    m_receivedTelnetData.clear();
    emit logToInterface("--- Запуск OpenOCD и попытка подключения ---", false);

    // --- Update UI ---
    ui->lblConnectionStatus->setText("<font color='blue'><b>...</b></font>");
    ui->lblConnectionStatus->setVisible(true);
    m_animationFrame = -1;
    updateLoadingAnimation();
    m_animationTimer->start();
    statusTimer->stop();
    if(ui->btnConnect) ui->btnConnect->setEnabled(false); // Disable connect while connecting/connected
    if(ui->btnUpload) ui->btnUpload->setEnabled(false);
    if(ui->cmbTargetMCU) ui->cmbTargetMCU->setEnabled(false); // Disable target change while connected


    // --- Prepare OpenOCD arguments ---
    QStringList arguments;
    // --- Specify search path for scripts first! ---
    arguments << "-s" << QDir::toNativeSeparators(m_openOcdScriptsPath);
    // --- Add interface and target scripts ---
    arguments << "-f" << m_interfaceScript;
    arguments << "-f" << targetScript;

    emit logToInterface("Запуск: " + m_openOcdExecutablePath + " " + arguments.join(" "), false);

    // --- Start OpenOCD process ---
    m_openOcdProcess->setWorkingDirectory(m_openOcdDir); // Set working directory to openocd folder
    m_openOcdProcess->start(m_openOcdExecutablePath, arguments);
}

void Unit1::stopOpenOcd() {
    qDebug() << ">>> stopOpenOcd CALLED. Current state: m_isOpenOcdRunning=" << m_isOpenOcdRunning
             << " Process state:" << (m_openOcdProcess ? m_openOcdProcess->state() : QProcess::NotRunning)
             << " Socket state:" << (m_telnetSocket ? m_telnetSocket->state() : QAbstractSocket::UnconnectedState); // DEBUG

    // Если уже не запущен или процесс уже завершен, просто выходим
    if (!m_isOpenOcdRunning && (!m_openOcdProcess || m_openOcdProcess->state() == QProcess::NotRunning)) {
        qDebug() << ">>> stopOpenOcd: Nothing to stop or already stopped."; // DEBUG
        // Убедимся, что UI сброшен на всякий случай
        m_animationTimer->stop();
        ui->lblConnectionStatus->setVisible(false);
        if(ui->btnConnect) ui->btnConnect->setEnabled(true);
        if(ui->btnUpload) ui->btnUpload->setEnabled(false);
        if(ui->cmbTargetMCU) ui->cmbTargetMCU->setEnabled(true);
        return;
    }

    // Логируем начало остановки только если действительно есть что останавливать
    if (m_isOpenOcdRunning || (m_openOcdProcess && m_openOcdProcess->state() != QProcess::NotRunning)) {
        emit logToInterface("--- Остановка OpenOCD ---", false);
    }

    // Сбрасываем флаги намерения и состояния
    m_isOpenOcdRunning = false;
    m_isConnected = false;
    m_isConnecting = false;
    m_isProgramming = false;

    // --- Сначала закрываем сокет (если еще не закрыт) ---
    // Это важно, чтобы он не пытался переподключиться или что-то еще
    if (m_telnetSocket && m_telnetSocket->state() != QAbstractSocket::UnconnectedState) {
        qDebug() << ">>> stopOpenOcd: Aborting Telnet socket."; // DEBUG
        m_telnetSocket->abort(); // Жестко разрываем соединение
    }

    // --- Затем останавливаем процесс ---
    if (m_openOcdProcess && m_openOcdProcess->state() != QProcess::NotRunning) {
        qDebug() << ">>> stopOpenOcd: Attempting to terminate OpenOCD process..."; // DEBUG
        m_openOcdProcess->terminate(); // Просим завершиться
        // Даем немного времени на завершение
        if (!m_openOcdProcess->waitForFinished(1000)) { // Ждем 1 секунду
            qDebug() << ">>> stopOpenOcd: Terminate failed or timed out. Killing process..."; // DEBUG
            emit logToInterface("Принудительное завершение OpenOCD...", true);
            m_openOcdProcess->kill(); // Убиваем, если не завершился сам
            m_openOcdProcess->waitForFinished(500); // Короткое ожидание после kill
        } else {
            qDebug() << ">>> stopOpenOcd: Terminate succeeded."; // DEBUG
        }
        qDebug() << ">>> stopOpenOcd: Final process state after stop attempt:" << m_openOcdProcess->state(); // DEBUG
    } else {
        qDebug() << ">>> stopOpenOcd: OpenOCD process was not running or pointer invalid."; // DEBUG
    }

    // --- Окончательно обновляем UI ---
    qDebug() << ">>> stopOpenOcd: Finalizing UI state."; // DEBUG
    m_animationTimer->stop(); // Останавливаем анимацию
    ui->lblConnectionStatus->setVisible(false); // Скрываем статус
    if(ui->btnConnect) ui->btnConnect->setEnabled(true); // Разрешаем новое подключение
    if(ui->btnUpload) ui->btnUpload->setEnabled(false); // Запрещаем загрузку (нужно сначала подключиться)
    if(ui->cmbTargetMCU) ui->cmbTargetMCU->setEnabled(true); // Разрешаем выбор MCU

    // Очищаем временный файл в самом конце
    cleanupTemporaryFile();
}

void Unit1::onBtnUploadClicked() {
    if (!m_isConnected) {
        emit logToInterface("Ошибка: Устройство не подключено. Сначала нажмите 'Подключить'.", true);
        QMessageBox::warning(nullptr, "Нет подключения", "Необходимо сначала подключиться к устройству.");
        return;
    }
    if (m_isProgramming) {
        emit logToInterface("Программирование уже выполняется.", true);
        return;
    }

    // --- Select Firmware File ---
    QString originalFirmwarePath;
    QString defaultDir = QDir::currentPath() + "/Файл Прошивки CPU1/"; // Or use QStandardPaths
    QDir testDir(defaultDir);
    if (!testDir.exists()) {
        defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation); // Safer default
        if (defaultDir.isEmpty()) defaultDir = QDir::currentPath();
    }
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

    // --- Copy to Temporary Location ---
    QString appDir = QCoreApplication::applicationDirPath();
    QString tempSubDir = "temp_firmware";
    QDir tempDir(appDir);
    if (!tempDir.exists(tempSubDir)) { if (!tempDir.mkdir(tempSubDir)) { /*...*/ return; } } // Simplified

    QFileInfo originalFileInfo(originalFirmwarePath);
    QString suffix = originalFileInfo.suffix().isEmpty() ? "tmp" : originalFileInfo.suffix();
    QString simpleFileName = QString("upload_%1.%2").arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz")).arg(suffix);
    QString temporaryFirmwarePath = tempDir.filePath(tempSubDir + "/" + simpleFileName);

    cleanupTemporaryFile();
    QFile::remove(temporaryFirmwarePath);

    if (!QFile::copy(originalFirmwarePath, temporaryFirmwarePath)) {
        emit logToInterface("Критическая ошибка: Не удалось скопировать файл прошивки из " + originalFirmwarePath + " в " +
                                temporaryFirmwarePath + ". Ошибка: " + QFile(originalFirmwarePath).errorString(), true);
        QMessageBox::warning(nullptr, "Ошибка копирования", "Абывгда");
        return;
    }

    emit logToInterface("Файл скопирован во временный: " + temporaryFirmwarePath, false);
    m_firmwareFilePathForUpload = QDir::toNativeSeparators(temporaryFirmwarePath);

    QString firmwarePathForOcd = m_firmwareFilePathForUpload.replace('\\', '/');
    emit logToInterface("Путь для OpenOCD: " + firmwarePathForOcd, false);

    m_shutdownCommandSent = false;

    // --- Start Programming Sequence ---
    m_isProgramming = true;
    emit logToInterface("--- Начало программирования (файл: " + originalFileInfo.fileName() + ") ---", false);

    // --- Update UI ---
    ui->lblConnectionStatus->setText("<font color='blue'><b>...</b></font>");
    ui->lblConnectionStatus->setVisible(true);
    m_animationFrame = -1;
    updateLoadingAnimation();
    m_animationTimer->start();
    statusTimer->stop();
    // Keep Connect disabled
    if(ui->btnUpload) ui->btnUpload->setEnabled(false); // Disable upload during programming
    // Keep Target disabled


    // --- Send OpenOCD Commands ---
    m_receivedTelnetData.clear();
    sendOpenOcdCommand("reset halt");
    // --- Wait a bit for halt to complete before sending program ---
    // --- A better way is to parse the ">" prompt after halt ---
    QTimer::singleShot(200, this, [this, firmwarePathForOcd]() { // Захватываем исправленный путь
        if (m_isProgramming) {
            // Используем firmwarePathForOcd вместо m_firmwareFilePathForUpload
            QString programCmd = QString("program \"%1\" %2 verify reset").arg(firmwarePathForOcd).arg(m_firmwareAddress);
            sendOpenOcdCommand(programCmd);
        }
    });
}

void Unit1::handleOpenOcdStarted() {
    emit logToInterface("Процесс OpenOCD запущен.", false);
    QTimer::singleShot(750, this, [this](){ // Increased delay slightly
        if (m_isConnecting && m_telnetSocket->state() == QAbstractSocket::UnconnectedState) {
            emit logToInterface(QString("Подключение к Telnet %1:%2...").arg(m_openOcdHost).arg(m_openOcdTelnetPort), false);
            m_telnetSocket->connectToHost(m_openOcdHost, m_openOcdTelnetPort);
        }
    });
}

void Unit1::handleTelnetReadyRead() {
    m_receivedTelnetData.append(m_telnetSocket->readAll());
    processTelnetBuffer();
}

void Unit1::handleOpenOcdFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    QString statusMsg = (exitStatus == QProcess::NormalExit) ? "нормально" : "с ошибкой";
    emit logToInterface(QString("Процесс OpenOCD завершен (Код: %1, Статус: %2).")
                            .arg(exitCode).arg(statusMsg), exitCode != 0 && m_isOpenOcdRunning); // Log as error only if unexpected
    qDebug() << "OpenOCD process finished. ExitCode:" << exitCode << "ExitStatus:" << static_cast<int>(exitStatus);

    // --- If OpenOCD stops unexpectedly while we thought it should be running ---
    if (m_isOpenOcdRunning) {
        m_isOpenOcdRunning = false; // It's not running anymore
        m_isConnected = false;
        m_isConnecting = false;
        m_isProgramming = false;
        m_animationTimer->stop();

        if (m_telnetSocket->state() != QAbstractSocket::UnconnectedState) {
            m_telnetSocket->abort();
        }

        emit logToInterface("Соединение с OpenOCD неожиданно разорвано.", true);
        ui->lblConnectionStatus->setText("<font color='red'><b>✗ OOCD</b></font>");
        ui->lblConnectionStatus->setVisible(true);
        statusTimer->start(3000);

        // --- Reset UI ---
        if(ui->btnConnect) ui->btnConnect->setEnabled(true);
        if(ui->btnUpload) ui->btnUpload->setEnabled(false);
        if(ui->cmbTargetMCU) ui->cmbTargetMCU->setEnabled(true);
    }
}

void Unit1::handleOpenOcdError(QProcess::ProcessError error) {
    emit logToInterface("Ошибка запуска/работы процесса OpenOCD: " + m_openOcdProcess->errorString(), true);
    qDebug() << "OpenOCD QProcess Error:" << static_cast<int>(error) << m_openOcdProcess->errorString();

    // --- Reset state fully on process error ---
    m_isOpenOcdRunning = false;
    m_isConnected = false;
    m_isConnecting = false;
    m_isProgramming = false;
    m_animationTimer->stop();

    ui->lblConnectionStatus->setText("<font color='red'><b>✗ Процесс</b></font>");
    ui->lblConnectionStatus->setVisible(true);
    statusTimer->start(3000);

    // --- Reset UI ---
    if(ui->btnConnect) ui->btnConnect->setEnabled(true);
    if(ui->btnUpload) ui->btnUpload->setEnabled(false);
    if(ui->cmbTargetMCU) ui->cmbTargetMCU->setEnabled(true);

    cleanupTemporaryFile();
}

void Unit1::handleOpenOcdStdOut() {
    QByteArray data = m_openOcdProcess->readAllStandardOutput();
    QString message = QString::fromLocal8Bit(data).trimmed();
    if (!message.isEmpty()) {
        emit logToInterface("[OOCD] " + message, false);
        qDebug() << "[OOCD stdout]" << message;
    }
}

void Unit1::handleOpenOcdStdErr() {
    QByteArray data = m_openOcdProcess->readAllStandardError();
    QString message = QString::fromLocal8Bit(data).trimmed();
    if (!message.isEmpty()) {
        // --- Умное определение ошибки ---
        bool isError = false; // По умолчанию - не ошибка
        if (message.startsWith("Error:")) {
            isError = true;
        } else if (message.startsWith("Warn :")) {
            // Предупреждения можно тоже красить красным или другим цветом
            // Пока оставим как НЕ ошибку для простоты, но можно и isError = true;
            isError = false; // Или isError = true, если хотите их видеть красными
        } else if (message.contains("failed") || message.contains("Can't find") || message.contains("couldn't open")) {
            // Некоторые строки без Error: тоже могут быть ошибками
            isError = true;
        }
        // Дополнительно: Можно игнорировать частое сообщение про скорость
        if (message.contains("Unable to match requested speed")) {
            isError = false; // Это не критическая ошибка
        }

        // Отправляем лог с правильным флагом isError
        emit logToInterface("[OOCD ERR] " + message, isError);
        qDebug() << "[OOCD stderr]" << message;

    }
}

void Unit1::handleTelnetConnected() {
    emit logToInterface("Telnet соединение установлено.", false);
    m_isConnected = true;
    m_isConnecting = false;
    m_animationTimer->stop();

    // --- Update UI ---
    ui->lblConnectionStatus->setText("<font color='green'><b>✓</b></font>");
    ui->lblConnectionStatus->setVisible(true);
    statusTimer->start(2000);
    // Connect button remains disabled
    if(ui->btnUpload) ui->btnUpload->setEnabled(true); // Enable upload now
    // Target MCU combo remains disabled
}

void Unit1::handleTelnetDisconnected() {
    qDebug() << ">>> handleTelnetDisconnected CALLED. m_isOpenOcdRunning=" << m_isOpenOcdRunning << "m_isConnecting=" << m_isConnecting; // DEBUG
    // --- Only log as error if the disconnection was unexpected ---
    if (m_isOpenOcdRunning) { // Эта проверка важна, чтобы не реагировать на штатный вызов stopOpenOcd()
        qDebug() << ">>> Telnet disconnected while OpenOCD was supposed to be running (likely after shutdown cmd or error)."; // DEBUG
        emit logToInterface("Telnet соединение разорвано (ожидаемо после shutdown или из-за ошибки).", true); // Изменим текст лога

        // Сбрасываем флаги состояния СРАЗУ
        m_isConnected = false;
        m_isConnecting = false; // На всякий случай
        m_isProgramming = false; // На всякий случай
        m_animationTimer->stop(); // Останавливаем любую анимацию

        // Обновляем UI, показывая состояние "не подключено"
        ui->lblConnectionStatus->setText("<font color='orange'><b>! Disconnected</b></font>"); // Оранжевый - разорвано
        ui->lblConnectionStatus->setVisible(true);
        statusTimer->start(3000);

        // --- Вызываем stopOpenOcd для завершения процесса и финализации UI ---
        qDebug() << ">>> Calling stopOpenOcd() from handleTelnetDisconnected."; // DEBUG
        stopOpenOcd(); // Вызываем в любом случае, если соединение разорвано при m_isOpenOcdRunning=true

    } else {
        qDebug() << ">>> Telnet socket disconnected normally (m_isOpenOcdRunning is false)."; // DEBUG
        // Если !m_isOpenOcdRunning, значит stopOpenOcd() уже был вызван ранее и инициировал отключение.
        // Ничего больше делать не нужно.
    }
    // cleanupTemporaryFile() вызывается внутри stopOpenOcd, не дублируем
}

void Unit1::handleTelnetError(QAbstractSocket::SocketError socketError) {
    emit logToInterface("Ошибка Telnet сокета: " + m_telnetSocket->errorString(), true);
    qDebug() << "Telnet Socket Error:" << static_cast<int>(socketError) << m_telnetSocket->errorString();

    m_animationTimer->stop();
    m_isConnected = false;
    m_isProgramming = false;

    if (m_isConnecting) {
        m_isConnecting = false;
        emit logToInterface("Не удалось подключиться к Telnet OpenOCD.", true);
        ui->lblConnectionStatus->setText("<font color='red'><b>✗ Telnet</b></font>");
        ui->lblConnectionStatus->setVisible(true);
        statusTimer->start(3000);
        stopOpenOcd(); // Stop the process if Telnet connection fails
    } else if (m_isOpenOcdRunning) { // Error while connected
        ui->lblConnectionStatus->setText("<font color='red'><b>✗ Telnet Err</b></font>");
        ui->lblConnectionStatus->setVisible(true);
        statusTimer->start(3000);
        stopOpenOcd(); // Stop the process on Telnet error
    }

    // UI is reset by stopOpenOcd()
    cleanupTemporaryFile();
}

void Unit1::sendOpenOcdCommand(const QString &command) {
    // Проверяем, есть ли активное Telnet-соединение
    if (!m_isConnected || !m_telnetSocket || m_telnetSocket->state() != QAbstractSocket::ConnectedState) {
        // Если нет соединения, логируем ошибку и ничего не отправляем
        qDebug() << "--- Cannot send command, Telnet not connected.";
        emit logToInterface("Ошибка: Невозможно отправить команду '" + command + "', нет Telnet соединения.", true);

        // Если мы пытались отправить команду во время программирования и не смогли,
        // то нужно сбросить состояние программирования, чтобы не зависнуть
        if(m_isProgramming && (command.startsWith("program") || command == "shutdown")) {
            m_isProgramming = false; // Сбрасываем флаг
            m_animationTimer->stop(); // Останавливаем анимацию
            ui->lblConnectionStatus->setText("<font color='red'><b>✗ Telnet</b></font>"); // Показываем ошибку
            if(ui->btnUpload) ui->btnUpload->setEnabled(m_isConnected); // Кнопка Upload зависит от m_isConnected
            statusTimer->start(3000); // Показать статус на время
            cleanupTemporaryFile(); // Очистить временный файл, если он был
            // Возможно, стоит также остановить OpenOCD, раз Telnet потерян
            // QTimer::singleShot(100, this, &Unit1::stopOpenOcd);
        }
        return; // Выходим из метода
    }

    // Если соединение есть, логируем отправляемую команду (синим цветом)
    emit logToInterface("[Telnet TX] " + command, false);
    qDebug() << "[Telnet TX]" << command;

    // Преобразуем команду в QByteArray (UTF-8) и добавляем символ новой строки (\n),
    // который OpenOCD ожидает в конце Telnet-команды
    QByteArray commandData = command.toUtf8() + "\n";

    qDebug() << "--- Writing command to Telnet:" << commandData.trimmed(); // DEBUG
    m_telnetSocket->write(commandData);
    m_telnetSocket->flush();
}

// --- processTelnetBuffer: Needs careful parsing ---
void Unit1::processTelnetBuffer() {
    // 1. Добавляем новые данные в буфер
    m_receivedTelnetData.append(m_telnetSocket->readAll());
    QString fullBuffer = QString::fromUtf8(m_receivedTelnetData); // Работаем с полной копией

    qDebug() << "--- processTelnetBuffer ENTRY --- Buffer size:" << fullBuffer.length(); // DEBUG

    // 2. ПРИОРИТЕТ: Проверяем исход программирования, если оно идет и shutdown еще не отправлен
    if (m_isProgramming && !m_shutdownCommandSent) {
        qDebug() << "--- Checking for programming outcome (isProgramming=true, shutdownSent=false)"; // DEBUG
        bool outcomeDetected = false;

        // Ищем ключевые фразы в ВЕСЬ накопленный буфер
        bool verifyOk = fullBuffer.contains("Verified OK") || fullBuffer.contains("verification succeded");
        bool programFailed = fullBuffer.contains("** Programming Failed **") || fullBuffer.contains("Error: couldn't open");

        if (programFailed) {
            qDebug() << "!!! Programming FAILED detected in buffer."; // DEBUG
            emit logToInterface("Ошибка программирования/верификации!", true);
            ui->lblConnectionStatus->setText("<font color='red'><b>✗ Прог</b></font>");
            outcomeDetected = true;
        } else if (verifyOk) {
            qDebug() << "!!! Programming SUCCESS (Verified OK) detected in buffer."; // DEBUG
            emit logToInterface("Программирование и верификация успешно завершены!", false);
            ui->lblConnectionStatus->setText("<font color='green'><b>✓ Прог</b></font>");
            outcomeDetected = true;
        }

        // Если исход определен (успех или провал)
        if (outcomeDetected) {
            m_isProgramming = false;      // Больше не программируем
            m_animationTimer->stop();     // Останавливаем анимацию
            statusTimer->start(5000);     // Показываем финальный статус

            qDebug() << "!!! Outcome detected. Attempting to send shutdown command."; // DEBUG
            emit logToInterface("Отправка команды shutdown в OpenOCD...", false);
            sendOpenOcdCommand("shutdown"); // Отправляем команду завершения
            m_shutdownCommandSent = true; // Устанавливаем флаг, чтобы не отправлять снова
            // НЕ ОЧИЩАЕМ БУФЕР ЗДЕСЬ. Пусть disconnect обработает остальное.
        }
    }

    // 3. Обрабатываем буфер для ЛОГИРОВАНИЯ строк до промпта '>'
    // Этот блок теперь отвечает только за вывод строк в лог, а не за определение исхода прошивки.
    int promptPos;
    // Используем локальную копию буфера для разбора логов,
    // а m_receivedTelnetData оставляем для накопления на случай разрыва строк.
    QString bufferForLogging = fullBuffer;

    while ((promptPos = bufferForLogging.indexOf("\n> ")) != -1) {
        QString message = bufferForLogging.left(promptPos).trimmed(); // Сообщение до промпта
        QString blockToRemove = bufferForLogging.left(promptPos + 3); // Блок с промптом

        if (!message.isEmpty()) {
            // --- Раскраска логов (как в функции appendToLog, но можно упростить) ---
            bool isError = message.startsWith("Error:") || message.contains("failed") || message.contains("timed out");
            bool isWarning = message.startsWith("Warn :");
            // Отправляем в UI для раскраски
            emit logToInterface("[Telnet] " + message, isError || isWarning); // Пока красим Warn как Error
            qDebug() << "[Telnet Parsed Line]" << message;
        }

        // Удаляем обработанный блок из оригинального накопленного буфера
        // Чтобы избежать повторного логирования и корректно сохранить остаток
        m_receivedTelnetData.remove(0, blockToRemove.length());
        // Обновляем локальную копию для следующей итерации цикла
        bufferForLogging = QString::fromUtf8(m_receivedTelnetData);

        qDebug() << "--- Logged block, remaining buffer size:" << m_receivedTelnetData.length(); // DEBUG
    }
    // Оставшаяся часть буфера (без промпта) сохраняется в m_receivedTelnetData для следующего вызова
    qDebug() << "--- processTelnetBuffer EXIT --- Remaining buffer size:" << m_receivedTelnetData.length(); // DEBUG
}

void Unit1::cleanupTemporaryFile() {
    if (!m_firmwareFilePathForUpload.isEmpty() && m_firmwareFilePathForUpload.contains("temp_firmware")) {
        qDebug() << "Удаление временного файла:" << m_firmwareFilePathForUpload;
        QFile tempFile(m_firmwareFilePathForUpload);
        if (tempFile.exists()) {
            if (!tempFile.remove()) {
                qWarning() << "Не удалось удалить временный файл:" << m_firmwareFilePathForUpload << tempFile.errorString();
                emit logToInterface("Предупреждение: Не удалось удалить временный файл: " + m_firmwareFilePathForUpload, true);
            } else {
                qDebug() << "Временный файл удален:" << m_firmwareFilePathForUpload;
            }
        }

        QFileInfo tempFileInfo(m_firmwareFilePathForUpload);
        QDir tempDir = tempFileInfo.dir();
        if (tempDir.exists() && tempDir.dirName() == "temp_firmware") {
            tempDir.setFilter(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);
            if (tempDir.entryInfoList().isEmpty()) {
                QDir parentDir = tempDir;
                parentDir.cdUp();
                if (parentDir.rmdir(tempDir.dirName())) {
                    qDebug() << "Временная папка удалена:" << tempDir.path();
                } else {
                    qWarning() << "Не удалось удалить временную папку (возможно, не пуста):" << tempDir.path();
                }
            } else {
                qDebug() << "Временная папка не пуста, не удаляется:" << tempDir.path();
            }
        }
        m_firmwareFilePathForUpload.clear();
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
    // --- 1. Проверка входных данных ---
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

    // Преобразуем в абсолютные пути
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

    // --- 2. Загрузка данных файлов ---
    QByteArray loaderData = loadFile(loaderFile.absoluteFilePath());
    QByteArray programData = loadFile(programFile.absoluteFilePath());
    if (loaderData.isEmpty()) { QMessageBox::critical(ui->cmbRevision->window(),
                              "Ошибка", "Не удалось прочитать файл загрузчика: " + loaderFile.absoluteFilePath()); return; }
    if (programData.isEmpty()) { QMessageBox::critical(ui->cmbRevision->window(),
                              "Ошибка", "Не удалось прочитать файл программы: " + programFile.absoluteFilePath()); return; }

    // --- 3. Подготовка базового буфера прошивки ---
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

    }

    // --- 7. Сообщение о завершении (Оригинальное) ---
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

void Unit1::onBtnCreateFileManualClicked()
{
    QString dir = QFileDialog::getExistingDirectory(ui->cmbRevision->window(), tr("Выберите папку для сохранения прошивок"), QDir::currentPath());
    if (dir.isEmpty()) return;
    createFirmwareFiles(dir); // Передаем выбранный АБСОЛЮТНЫЙ путь
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

    createFirmwareFiles(outDirAbs); // Передаем АБСОЛЮТНЫЙ путь
}
