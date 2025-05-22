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
#include <QKeyEvent>

#include "unit1.h"
#include "unit2.h"

#include "config_data.h"

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
    QStringList allCategoriesList;

    QString findFirstCategoryForDeviceModelXmlName(const QString& );

protected:
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onExpertModeToggled(bool checked);

    // Слоты для обработки изменений в комбобоксах
    void handleRevisionComboBoxChanged(int index);
    void handleUpdateRevisionComboBoxChanged(int index);
    void handleDevModelNameComboBoxChanged(int index);
    void handleUpdateDevModelNameComboBoxChanged(int index);

    // Слот для кнопки Unit2, требующей центральных данных
    void onAutoCreateUpdateTriggered();
    void appendToLog(const QString &message, bool isError);


private:
    Ui::MainWindow *ui;
    QSize expertSize;
    QSize simpleSize;

    Unit1 *unit1;
    Unit2 *unit2;

    QMap<QString, ExtendedRevisionInfo> revisionsMap;
    QCheckBox *expertModeCheckbox;

    QString findCategoryForModel(const QString& modelName);
    void loadConfigAndPopulate(const QString &filePath);
    void updateUnit2UI(const QString& category);
    void synchronizeComboBoxes(QObject* senderComboBox);
    void filterAndPopulateRevisionComboBoxes(const QString& deviceModelXmlName, QComboBox* sourceDevModelComboBox = nullptr);
    void updateBldrDevModelDisplay(const QString& category);
};

#endif // MAINWINDOW_H
