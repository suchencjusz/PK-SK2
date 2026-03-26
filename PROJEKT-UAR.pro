QT       += core gui
QT       += printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

TARGET = PROJEKT-UAR
TEMPLATE = app

SOURCES += \
    MainWindow.cpp \
    main.cpp \
    ARXDialog.cpp \
    GeneratorWartosciZadanej.cpp \
    KonfiguracjaUAR.cpp \
    ModelARX.cpp \
    ProstyUAR.cpp \
    qcustomplot.cpp \
    RegulatorPID.cpp \
    SymulacjaUAR.cpp \
    Testy_Wlasne.cpp \
    UslugiUAR.cpp


HEADERS += \
    ARXDialog.h \
    GeneratorWartosciZadanej.h \
    Json.hpp \
    KonfiguracjaUAR.h \
    MainWindow.h \
    ModelARX.h \
    ProstyUAR.h \
    qcustomplot.h \
    RegulatorPID.h \
    SymulacjaUAR.h \
    Testy_Wlasne.h \
    UslugiUAR.h\


FORMS += \
    ARXDialog.ui \
    MainWindow.ui


qnx: target.path = /tmp/$$TARGET/bin
else: unix:!android: target.path = /opt/$$TARGET/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES +=
