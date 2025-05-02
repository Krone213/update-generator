#ifndef UNIT2_H
#define UNIT2_H

#include "ui_mainwindow.h" // Access to UI elements
#include <QObject>
#include "config_data.h" // Include the common data structure
// --- Add to top of unit2.cpp OR in a new header file ---
#include <QtGlobal> // For qint types
#include <cstring>  // For memset/memcpy
#include <QDateTime> // For time
#include <QtEndian> // For byte order potentially


class Unit2 : public QObject
{
    Q_OBJECT

public:
    explicit Unit2(Ui::MainWindow *ui, QObject *parent = nullptr);

    // --- Methods Called by MainWindow ---
    void updateUI(const ExtendedRevisionInfo& info); // Обновление UI из MainWindow
    void clearUIFields();                           // Очистка UI из MainWindow
    // Worker function for creating update files (called by MainWindow)
    void createUpdateFiles(const QString &outputDirAbsPath);


private:
    Ui::MainWindow *ui; // Pointer to UI elements

    // --- Private Helper Methods ---
    void updateUpdateProgramDataFileSize(const QString &filePath); // Specific to Unit2's label


public slots:
    // --- Slots Connected Directly from MainWindow UI ---
    void onBtnChooseUpdateProgramDataFileClicked();
    void onBtnUpdateCreateFileManualClicked(); // Manual create uses this directly
    void onBtnUpdateShowInfoClicked();
    void onBtnClearUpdateRevisionClicked(); // Clears Unit2 UI or specific files?
    void onBtnCreateCommonUpdateFileClicked(); // Specific Unit2 action

private slots:
               // Internal slots if needed
};

#endif // UNIT2_H
