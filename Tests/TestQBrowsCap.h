#ifndef TESTQBROWSCAP_H
#define TESTQBROWSCAP_H

#include <QtTest/QtTest>
#include <QDir>
#include <QFile>
#include <QTemporaryFile>
#include <QDebug>
#include <QTime>
#include "../QBrowsCap.h"

#define TESTQBROWSCAP_CSV_VERSION 4594

class TestQBrowsCap: public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void getCsvVersion();
    void getIndexVersion();
    void indexIsUpToDate();
    void matchUserAgent();
    void matchUserAgent_data();

private:
    QBrowsCap browsCap;
    QTemporaryFile tmp;
};

#endif // TESTQBROWSCAP_H
