#ifndef QBROWSCAP_H
#define QBROWSCAP_H


#include <QHash>
#include <QStringList>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QObject>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QEventLoop>
#include <QPair>
#include <QDateTime>

#ifdef DEBUG
#include <QDebug>
#endif


#define QBROWSCAP_CSV_URL "http://localhost/browscap.csv" //http://browsers.garykeith.com/stream.asp?BrowsCapCSV"
#define QBROWSCAP_VERSION_URL "http://localhost/version" //http://browsers.garykeith.com/versions/version-number.asp"
#define QBROWSCAP_INDEX_DB_VERSION_PATTERN "___QBROWSCAP_VERSION___"
#define QBROWSCAP_INDEX_DB_LAST_VERSION_PATTERN "___QBROWSCAP_LAST_VERSION___"
#define QBROWSCAP_INDEX_DB_LAST_VERSION_CHECK_PATTERN "___QBROWSCAP_LAST_VERSION_CHECK___"
#define QBROWSCAP_MIN_UPDATE_INTERVAL 86400 // Allow only daily updates.


class QBrowsCap : public QObject {
    Q_OBJECT

public:
    QBrowsCap();
    QBrowsCap(const QString & csvFile);
    QBrowsCap(const QString & csvFile, const QString & indexFile);

    void setCsvFile(const QString & csvFile);
    void setIndexFile(const QString & indexFile);

    bool selfUpdate();

    int getCsvVersion() const;
    int getLatestVersion();
    int getIndexVersion() const;

    bool isUpToDate();
    bool downloadUpdate(const QString & targetPath);
    bool indexIsUpToDate() const;
    bool buildIndex(bool force = false, bool ignoreCrawlers = true, bool ignorFeedReaders = true, bool ignoreBanned = true, bool ignoreNoJS = true);

    QPair<bool, QStringList> matchUserAgent(const QString & userAgent);

protected slots:
    void downloadFinished(QNetworkReply * reply);

signals:
    void downloadedUpdate(bool ok, const QString & failureReason = QString::null);
    void versionChecked(bool ok, int version, const QString & failureReason = QString::null);

protected:
    // Download-related variables.
    QNetworkAccessManager manager;
    QString csvTargetPath;
    bool csvDownloadResult, versionDownloadResult;
    int latestVersion;

    // The two speed-up layers: the index is persistent, the cache is not.
    QSqlDatabase index;
    QHash<QString, QPair<bool, QStringList> > cache;

    // The browscap.csv file.
    QString csvFile;

    // The corresponding index (a SQLite DB).
    QString indexFile;

    void init();
    bool connectIndexDB();
};

#endif // QBROWSCAP_H
