TEMPLATE = app
CONFIG += QtTest meegotouch MImServer mimframework
DEPENDPATH += .
INCLUDEPATH += . \
        ../../m-keyboard/ \
        ../stubs/


LIBS += -Wl,-rpath=/usr/lib/m-im-plugins/ -L../../m-keyboard/ -lmkeyboard

# Input
HEADERS += ut_mhardwarekeyboard.h \
           ../stubs/mgconfitem_stub.h \
           ../stubs/fakegconf.h

SOURCES += ut_mhardwarekeyboard.cpp \
           ../stubs/fakegconf.cpp

include(../common_check.pri)