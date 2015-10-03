QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MarkovChain
TEMPLATE = app

SOURCES += main.cpp \
           markovchaingui.cpp \
           libmarkov/libmarkov.cpp

HEADERS  += markovchaingui.h \
            libmarkov/libmarkov.h

FORMS    += markovchaingui.ui

INCLUDEPATH += libmarkov

CONFIG += c++11
