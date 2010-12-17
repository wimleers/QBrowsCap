DEPENDPATH += ..
INCLUDEPATH += ..
include (../QBrowsCap.pri)

CONFIG += qtestlib
OBJECTS_DIR = .obj
MOC_DIR = .moc
RCC_DIR = .rcc

macx {
  CONFIG -= app_bundle
}
TARGET = TestQBrowsCap

HEADERS += TestQBrowsCap.h
SOURCES += TestQBrowsCap.cpp

OTHER_FILES += browscap.csv

# Copy browscap.csv to the build directory when shadow builds are being used.
!equals($${PWD}, $${OUT_PWD}) {
    unix {
        COPY = cp
    }
    win32 {
        COPY = copy /y
    }

    QMAKE_PRE_LINK = $$COPY $$PWD/browscap.csv $$OUT_PWD/browscap.csv
}
