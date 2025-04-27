#ifndef CONFIG_DATA_H
#define CONFIG_DATA_H

#include <QString>
#include <QDomElement>
#include <QDebug>

// Helper function to safely get boolean attributes from XML
inline bool getBoolAttr(const QDomElement& element, const QString& attrName, bool defaultValue = false) {
    if (!element.hasAttribute(attrName)) {
        return defaultValue;
    }
    QString val = element.attribute(attrName).trimmed();
    return (val == "1" || val.compare("true", Qt::CaseInsensitive) == 0 || val.compare("yes", Qt::CaseInsensitive) == 0);
}

// Расширенная структура - используется MainWindow для чтения XML и передачи данных в Unit2
struct ExtendedRevisionInfo {
    // --- Поля для Unit1 (MainWindow читает, но Unit1 использует свои) ---
    QString category;
    QString bootloaderFile;
    QString mainProgramFile;
    QString saveFirmwarePath; // Для Unit1

    // --- Поля для Unit2 ---
    QString bldrDevModel;
    QString beginAddress;
    QString saveUpdatePath;   // Для Unit2

    // Флаги <comands>
    bool cmdMainProg = false;
    bool cmdOnlyForEnteredSN = false;
    bool cmdOnlyForEnteredDevID = false;
    bool cmdNoCheckModel = false;

    // Флаги <dop>
    bool dopSaveBthName = false;
    bool dopSaveBthAddr = false;
    bool dopSaveDevDesc = false;
    bool dopSaveHwVerr = false;
    bool dopSaveSwVerr = false;
    bool dopSaveDevSerial = false;
    bool dopSaveDevModel = false;
    bool dopLvl1MemProtection = false;

    ExtendedRevisionInfo() = default;
};

#endif // CONFIG_DATA_H
