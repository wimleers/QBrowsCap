DEPENDPATH += ..
INCLUDEPATH += ..
include("../QBrowsCap.pri")

SOURCES += sample.cpp

OTHER_FILES += Tests/browscap.csv

CONFIG -= debug
CONFIG += release
macx {
  CONFIG -= app_bundle
}
