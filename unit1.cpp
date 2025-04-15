#include "unit1.h"

Unit1::Unit1(Ui::MainWindow *ui, QObject *parent)
    : QObject(parent), ui(ui)
{
    // Инициализация таймера
    statusTimer = new QTimer(this);
    statusTimer->setSingleShot(true);
    connect(statusTimer, &QTimer::timeout, this, &Unit1::hideConnectionStatus);

    // Скрываем метку по умолчанию
    ui->lblConnectionStatus->setVisible(false);
}

void Unit1::onBtnConnectClicked()
{
    // Реализация логики
    bool success = true;  // Успешное подключение
    // bool success = false;  // Ошибка подключения

    if (success) {
        ui->lblConnectionStatus->setText("✓");
    } else {
        ui->lblConnectionStatus->setText("✗");
    }

    ui->lblConnectionStatus->setVisible(true);
    statusTimer->start(2000);  // Показать метку на 2 секунды
}

void Unit1::onBtnUploadClicked()
{
    // Здесь логика для btnUpload
}

void Unit1::hideConnectionStatus()
{
    ui->lblConnectionStatus->setVisible(false);
}

void Unit1::onBtnChooseProgramDataFileClicked()
{
    // Добавьте свою логику для выбора файла программы/данных
}

void Unit1::onBtnChooseLoaderFileClicked()
{
    // Добавьте свою логику для выбора файла загрузчика
}

void Unit1::onBtnCreateFileManualClicked()
{
    // Добавьте свою логику для создания файла вручную
}

void Unit1::onBtnCreateFileAutoClicked()
{
    // Добавьте свою логику для создания файла автоматически
}

void Unit1::onBtnShowInfoClicked()
{
    // Добавьте свою логику для показа информации
}

void Unit1::onBtnClearRevisionClicked()
{
    // Добавьте свою логику для очистки ревизии
}
