#include "TestQBrowsCap.h"

QTEST_APPLESS_MAIN(TestQBrowsCap);

void TestQBrowsCap::initTestCase() {
    this->browsCap.setCsvFile(QDir::currentPath() + "/browscap.csv");

    QVERIFY2(tmp.open() == true, "A temporary file could not be created.");
    this->browsCap.setIndexFile(tmp.fileName());

    QWARN("About to build a SQLite-based index of the browscap.csv file. This will take a couple of seconds.");
    QVERIFY2(this->browsCap.buildIndex() == true, "The index could not be built.");

    QVERIFY2(this->browsCap.indexIsUpToDate() == true, "The index was built, but is not up to date.");
}

void TestQBrowsCap::getCsvVersion() {
    QVERIFY(this->browsCap.getCsvVersion() == TESTQBROWSCAP_CSV_VERSION);
}

void TestQBrowsCap::getIndexVersion() {
    QVERIFY(this->browsCap.getIndexVersion() == TESTQBROWSCAP_CSV_VERSION);
}

void TestQBrowsCap::indexIsUpToDate() {
    QVERIFY(this->browsCap.indexIsUpToDate() == true);
}

void TestQBrowsCap::matchUserAgent_data() {
    QTest::addColumn<QString>("userAgent");
    QTest::addColumn<bool>("success");
    QTest::addColumn<QString>("platform");
    QTest::addColumn<QString>("browser_name");
    QTest::addColumn<QString>("browser_version");
    QTest::addColumn<quint16>("browser_version_major");
    QTest::addColumn<quint16>("browser_version_minor");
    QTest::addColumn<bool>("is_mobile");

    QTest::newRow("Chrome 8.0 on Mac OS X")
            << "Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10_6_5; en-US) AppleWebKit/534.10 (KHTML, like Gecko) Chrome/8.0.552.231 Safari/534.10"
            << true
            << "MacOSX"
            << "Chrome"
            << "8.0"
            << (quint16) 8
            << (quint16) 0
            << false;
    QTest::newRow("Internet Explorer 8.0 on Windows XP")
            << "Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 5.1; Trident/4.0; WinTSI 05.11.2009)"
            << true
            << "WinXP"
            << "IE"
            << "8.0"
            << (quint16) 8
            << (quint16) 0
            << false;
    QTest::newRow("Firefox 3.5 on Mac OS X")
            << "Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10.6; en-US; rv:1.9.1.13) Gecko/20100914 Firefox/3.5.13"
            << true
            << "MacOSX"
            << "Firefox"
            << "3.5"
            << (quint16) 3
            << (quint16) 5
            << false;
    QTest::newRow("Firefox 3.6 on Windows 7")
            << "Mozilla/5.0 (Windows; U; Windows NT 6.1; en-US; rv:1.9.2.10) Gecko/20100914 Firefox/3.6.10"
            << true
            << "Win7"
            << "Firefox"
            << "3.6"
            << (quint16) 3
            << (quint16) 6
            << false;
    QTest::newRow("Netscape 8.1 on Windows XP")
            << "Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.7.5) Gecko/20060127 Netscape/8.1"
            << true
            << "WinXP"
            << "Netscape"
            << "8.1"
            << (quint16) 8
            << (quint16) 1
            << false;
    QTest::newRow("Chrome 6.0 on Windows 7")
            << "Mozilla/5.0 (Windows; U; Windows NT 6.1; en-US) AppleWebKit/534.3 (KHTML, like Gecko) Chrome/6.0.472.63 Safari/534.3"
            << true
            << "Win7"
            << "Chrome"
            << "6.0"
            << (quint16) 6
            << (quint16) 0
            << false;

    QTest::newRow("Safari on iPhone")
            << "Mozilla/5.0 (iPhone; U; CPU iPhone OS 4_2_1 like Mac OS X; en-us) AppleWebKit/533.17.9 (KHTML, like Gecko) Version/5.0.2 Mobile/8C148a Safari/6533.18.5"
            << true
            << "iPhone OSX"
            << "iPhone"
            << "4.2"
            << (quint16) 4
            << (quint16) 2
            << true;

    QTest::newRow("Non-existing browser")
            << "Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10_6_5; en-US) AppleWebKit/534.10 (KHTML, like Gecko) NON/10.0"
            << false
            << "" << "" << "" << (quint16) -1 << (quint16) -1; // Dummy values.

}

void TestQBrowsCap::matchUserAgent() {
    static QPair<bool, QBrowsCapRecord> result;
    static QBrowsCapRecord details;

    QFETCH(QString, userAgent);
    QFETCH(bool, success);

    result = this->browsCap.matchUserAgent(userAgent);
    details = result.second;

    QCOMPARE(result.first, success);
    if (success) {
        QTEST(details.platform, "platform");
        QTEST(details.browser_name, "browser_name");
        QTEST(details.browser_version, "browser_version");
        QTEST(details.browser_version_major, "browser_version_major");
        QTEST(details.browser_version_minor, "browser_version_minor");
        QTEST(details.is_mobile, "is_mobile");
    }
}
