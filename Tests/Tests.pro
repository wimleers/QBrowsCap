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

# Copy all OTHER_FILES to the build directory when using shadow builds.
!equals($${PWD}, $${OUT_PWD}) {
    unix:COPY  = cp
    win32:COPY = copy /y
    for(other_file, OTHER_FILES) {
          QMAKE_PRE_LINK += $${COPY} $${PWD}/$${other_file} $${OUT_PWD}/$${other_file};
    }
}
