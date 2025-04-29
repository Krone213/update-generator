#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QScreen>
#include <QApplication>
#include <QTimer>
#include <QShowEvent>
#include <QMap>
#include <QSet>
#include <QFile>
#include <QDomDocument>
#include <QMessageBox>
#include <QDebug>
#include <QComboBox>
#include <QCheckBox>
#include <QDir>
#include <QFileDialog>

#include "config_data.h" // Используем расширенную структуру

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class Unit1; // Forward declaration
class Unit2; // Forward declaration
class QComboBox; // Forward declaration

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onExpertModeToggled(bool checked);

    // --- Слоты для обработки изменений в комбобоксах (теперь вызывают единый обработчик) ---
    void handleRevisionComboBoxChanged(int index);
    void handleUpdateRevisionComboBoxChanged(int index);
    void handleDeviceModelComboBoxChanged(int index); // <-- НОВЫЙ СЛОТ

    // --- Слот для кнопки Unit2, требующей центральных данных ---
    void onAutoCreateUpdateTriggered(); // Для кнопки "Авто" в Unit2

private:
    Ui::MainWindow *ui;
    QSize expertSize;
    QSize simpleSize;

    Unit1 *unit1;
    Unit2 *unit2;

    QMap<QString, ExtendedRevisionInfo> revisionsMap; // Карта с расширенными данными

    // --- Приватные методы ---
    void loadConfigAndPopulate(const QString &filePath); // Загружает конфиг и заполняет ВСЕ комбобоксы
    void updateUnit2UI(const QString& category);         // Обновляет ТОЛЬКО UI для Unit2
    void synchronizeComboBoxes(QObject* senderComboBox); // <-- НОВЫЙ МЕТОД СИНХРОНИЗАЦИИ
    QString findCategoryForModel(const QString& modelName); // <-- Вспомогательный метод
};

#endif // MAINWINDOW_H
