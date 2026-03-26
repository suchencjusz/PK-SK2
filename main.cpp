// Uruchamianie testow
//#define TESTY

#ifdef TESTY
#include "Testy_Wlasne.h"
#include <QCoreApplication>
#else
#include "MainWindow.h"
#include <QApplication>
#include <QPalette>
#include <QColor>
#endif

// Start testow
#ifdef TESTY
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    TestyWlasne::uruchom();
    return 0;
}

// Start symulatora
#else
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle("Fusion");

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(31, 31, 31));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Base, QColor(43, 43, 43));
    palette.setColor(QPalette::AlternateBase, QColor(42, 42, 42));
    palette.setColor(QPalette::ToolTipBase, Qt::white);
    palette.setColor(QPalette::ToolTipText, Qt::white);
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Button, QColor(42, 42, 42));
    palette.setColor(QPalette::ButtonText, Qt::white);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Highlight, QColor(58, 110, 165));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    a.setPalette(palette);
    a.setStyleSheet(
        "QLabel { color: #E0E0E0; }"
        "QGroupBox { color: #E0E0E0; }"
        "QCheckBox { color: #E0E0E0; }"
        "QComboBox, QSpinBox, QDoubleSpinBox, QLineEdit {"
            "  background-color: #2B2B2B;"
            "  color: #EDEDED;"
            "  border: 1px solid #555;"
            "  border-radius: 4px;"
            "  padding: 2px 22px 2px 6px;"
        "}"
        "QSpinBox::up-button, QDoubleSpinBox::up-button {"
            "  subcontrol-origin: border;"
            "  subcontrol-position: top right;"
            "  width: 16px;"
        "}"
        "QSpinBox::down-button, QDoubleSpinBox::down-button {"
            "  subcontrol-origin: border;"
            "  subcontrol-position: bottom right;"
            "  width: 16px;"
        "}"
        "QComboBox::drop-down {"
            "  subcontrol-origin: padding;"
            "  subcontrol-position: top right;"
            "  width: 18px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: #2B2B2B;"
        "  color: #EDEDED;"
        "  selection-background-color: #3A6EA5;"
        "}"
    );

    MainWindow w;
    w.show();
    return a.exec();
}
#endif

