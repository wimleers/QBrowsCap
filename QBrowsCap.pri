QT += core network sql
QT -= gui

# Disable qDebug() output when in release mode.
CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

# Add a DEBUG define when in debug mode.
CONFIG(debug, debug|release):DEFINES += DEBUG

HEADERS += QBrowsCap.h
SOURCES += QBrowsCap.cpp
