#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setStyle(QStyleFactory::create("Fusion"));

    QPalette lightPalette;

    lightPalette.setColor(QPalette::Window, QColor(236, 239, 241));
    lightPalette.setColor(QPalette::WindowText, QColor(33, 33, 33));
    lightPalette.setColor(QPalette::Base, QColor(255, 255, 255));
    lightPalette.setColor(QPalette::AlternateBase, QColor(245, 245, 245));
    lightPalette.setColor(QPalette::ToolTipBase, QColor(255, 255, 220));
    lightPalette.setColor(QPalette::ToolTipText, QColor(0, 0, 0));
    lightPalette.setColor(QPalette::Text, QColor(33, 33, 33));
    lightPalette.setColor(QPalette::Button, QColor(236, 239, 241));
    lightPalette.setColor(QPalette::ButtonText, QColor(33, 33, 33));
    lightPalette.setColor(QPalette::BrightText, QColor(255, 0, 0));
    lightPalette.setColor(QPalette::Link, QColor(0, 0, 255));
    lightPalette.setColor(QPalette::Highlight, QColor(66, 165, 245));
    lightPalette.setColor(QPalette::HighlightedText, Qt::white);

    lightPalette.setColor(QPalette::Disabled, QPalette::Text, Qt::darkGray);
    lightPalette.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::darkGray);
    lightPalette.setColor(QPalette::Disabled, QPalette::WindowText, Qt::darkGray);
    lightPalette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(210, 210, 210));
    lightPalette.setColor(QPalette::Disabled, QPalette::HighlightedText, Qt::darkGray);

    a.setPalette(lightPalette);

    MainWindow w;
    w.show();
    return a.exec();
}
