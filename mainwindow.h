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

#include "unit1.h"
#include "unit2.h"

#include "config_data.h" // Используем расширенную структуру

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class Unit1;
class Unit2;
class QComboBox;
class QTextEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QSet<QString> getUpdateAutoSavePaths() const;
    const QMap<QString, ExtendedRevisionInfo>& getRevisionsMap() const;

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onExpertModeToggled(bool checked);

    // --- Слоты для обработки изменений в комбобоксах ---
    void handleRevisionComboBoxChanged(int index);
    void handleUpdateRevisionComboBoxChanged(int index);
    void handleDeviceModelComboBoxChanged(int index);

    // --- Слот для кнопки Unit2, требующей центральных данных ---
    void onAutoCreateUpdateTriggered();

    void appendToLog(const QString &message, bool isError);


private:
    Ui::MainWindow *ui;
    QSize expertSize;
    QSize simpleSize;

    Unit1 *unit1;
    Unit2 *unit2;

    QMap<QString, ExtendedRevisionInfo> revisionsMap;

    // --- Приватные методы ---
    void loadConfigAndPopulate(const QString &filePath);
    void updateUnit2UI(const QString& category);
    void synchronizeComboBoxes(QObject* senderComboBox);
    QString findCategoryForModel(const QString& modelName);
};

#endif // MAINWINDOW_H
