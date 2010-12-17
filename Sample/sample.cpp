#include "QBrowsCap.h"
#include <QCoreApplication>
#include <QDebug>
#include <QTime>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QTextStream cout(stdout);
    QTime timer;
    int timePassed;

    QBrowsCap browsCap;
    browsCap.setCsvFile("./browscap.csv");
    browsCap.setIndexFile("./index.db");
    browsCap.selfUpdate();
    browsCap.buildIndex();

    timer.start();
    qDebug() << browsCap.matchUserAgent("Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10_6_5; en-US) AppleWebKit/534.10 (KHTML, like Gecko) Chrome/8.0.552.215 Safari/534.10");
    timePassed = timer.elapsed();

    cout << QString("Duration: %1 ms.")
            .arg(timePassed)
            << endl;
}
