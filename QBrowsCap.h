#ifndef QBROWSCAP_H
#define QBROWSCAP_H


#include <QMap>
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
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>
#include <QMetaType>
#include <QDebug>


#define QBROWSCAP_CSV_URL "http://browsers.garykeith.com/stream.asp?BrowsCapCSV"
#define QBROWSCAP_VERSION_URL "http://browsers.garykeith.com/versions/version-number.asp"
#define QBROWSCAP_INDEX_DB_VERSION_PATTERN "___QBROWSCAP_VERSION___"
#define QBROWSCAP_INDEX_DB_LAST_VERSION_PATTERN "___QBROWSCAP_LAST_VERSION___"
#define QBROWSCAP_INDEX_DB_LAST_VERSION_CHECK_PATTERN "___QBROWSCAP_LAST_VERSION_CHECK___"
#define QBROWSCAP_MIN_UPDATE_INTERVAL 86400 // Allow only daily updates.


struct QBrowsCapRecord {
    QBrowsCapRecord() {}
    ~QBrowsCapRecord() {}
    QBrowsCapRecord(QString platform, QString browser_name,
                    QString browser_version, quint16 browser_version_major,
                    quint16 browser_version_minor, bool is_mobile)
    {
        this->platform              = platform;
        this->browser_name          = browser_name;;
        this->browser_version       = browser_version;
        this->browser_version_major = browser_version_major;
        this->browser_version_minor = browser_version_minor;
        this->is_mobile             = is_mobile;
    }

    QString platform;
    QString browser_name;
    QString browser_version;
    quint16 browser_version_major;
    quint16 browser_version_minor;
    bool    is_mobile;
};

// Register metatype to allow these types to be streamed in QTests.
Q_DECLARE_METATYPE(QBrowsCapRecord)

#ifdef DEBUG
// QDebug() streaming output operators.
QDebug operator<<(QDebug dbg, const QBrowsCapRecord & record);
#endif

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

    int getCacheSize() { QMutexLocker(&this->cacheMutex); return this->cache.size(); }

    bool isUpToDate();
    bool downloadUpdate(const QString & targetPath);
    bool indexIsUpToDate() const;
    bool buildIndex(bool force = false, bool ignoreCrawlers = true, bool ignorFeedReaders = true, bool ignoreBanned = true, bool ignoreNoJS = true);

    QPair<bool, QBrowsCapRecord> matchUserAgent(const QString & userAgent);

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
    QMap<QString, QPair<bool, QBrowsCapRecord> > cache;

    // The browscap.csv file.
    QString csvFile;

    // The corresponding index (a SQLite DB).
    QString indexFile;

    // Mutexes are necessary to make the SQLite DB queries and the cache
    // lookups/insertions thread-safe (all calls are serialized).
    QMutex queryMutex;
    QMutex cacheMutex;

    void init();
    bool connectIndexDB();
};

#endif // QBROWSCAP_H
