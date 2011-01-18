DEPENDPATH += ..
INCLUDEPATH += ..
include("../QBrowsCap.pri")

SOURCES += sample.cpp

OTHER_FILES += ../Tests/browscap.csv

CONFIG -= debug
CONFIG += release
macx {
  CONFIG -= app_bundle
}

# Copy all OTHER_FILES to the build directory when using shadow builds.
!equals($${PWD}, $${OUT_PWD}) {
    unix:COPY  = cp -f
    win32:COPY = copy /y
    for(other_file, OTHER_FILES) {
          QMAKE_PRE_LINK += $${COPY} $${PWD}/$${other_file} $${OUT_PWD}/$$basename(other_file);
    }
}
