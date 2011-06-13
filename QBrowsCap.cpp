#include "QBrowsCap.h"

QBrowsCap::QBrowsCap() {
    this->init();
}

QBrowsCap::QBrowsCap(const QString & csvFile) {
    this->init();
    this->setCsvFile(csvFile);
}

QBrowsCap::QBrowsCap(const QString & csvFile, const QString & indexFile) {
    this->init();
    this->setCsvFile(csvFile);
    this->setIndexFile(indexFile);
}

void QBrowsCap::setCsvFile(const QString & csvFile) {
    this->csvFile = csvFile;
}

void QBrowsCap::setIndexFile(const QString &indexFile) {
    this->indexFile = indexFile;
}

void QBrowsCap::init() {
    connect(&this->manager, SIGNAL(finished(QNetworkReply*)), SLOT(downloadFinished(QNetworkReply*)));
}

/**
 * On the condition that a browscap.csv file and an index file have been set,
 * let QBrowsCap automatically update itself if necessary, and rebuild the
 * index if necessary.
 *
 * @return
 *   Returns true if it already is
 */
bool QBrowsCap::selfUpdate() {
    if (this->csvFile.isEmpty() || this->indexFile.isEmpty()) {
        qWarning("Cannot perform a self update because either no browscap.csv file or no index file have been set.");
        return false;
    }

    if (!this->isUpToDate()) {
        return this->downloadUpdate(this->csvFile) && this->buildIndex();
    }
    else
        return this->buildIndex();
}

bool QBrowsCap::isUpToDate() {
    int cvsVersion = this->getCsvVersion();
    int latestVersion = this->getLatestVersion();

    // Continue with the current version in case of network problems.
    if (latestVersion == -1)
        return true;
    else
        return cvsVersion == latestVersion;
}

/**
 * Get the version of the browscap.csv file.
 *
 * @return
 *   The version number, or -1 in case of error.
 */
int QBrowsCap::getCsvVersion() const {
    int csvVersion;

    if (!QFile::exists(this->csvFile))
        return -1;

    QFile csv(this->csvFile);
    if (!csv.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical("Could not open '%s' file for reading: %s.", qPrintable(this->csvFile), qPrintable(csv.errorString()));
        return -1;
    }
    else {
        QTextStream in(&csv);
        QString line;
        in.readLine();
        csvVersion = in.readLine()              // The second line of the .csv file contains the version number.
                     .mid(1, line.length() - 2) // Strip the leading & trailing double quotes.
                     .split("\",\"")            // Split according to the columns.
                     .at(0)                     // Get the first column.
                     .toInt();                  // Typecast to an integer.
    }

    return csvVersion;
}

/**
 * Get the latest version of the browscap.csv file from the internet. But,
 * only do this at most once per day.
 *
 * @return
 *   The version number, or -1 in case of error or previous check in the
 *   last 24 hours.
 */
int QBrowsCap::getLatestVersion() {
    int lastUpdateTimestamp = 0;

    // Get the last version check time stamp, if any. Similarly, get the
    // latest version from the last version check.
    QFileInfo info(this->indexFile);
    if (info.size() > 1) {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "last-version-check-read");
        db.setDatabaseName(this->indexFile);
        if (db.open()) {
            QSqlQuery query(db);

            // Get the last version check time stamp.
            query.prepare("SELECT browser_version FROM browscap WHERE pattern = ?;");
            query.addBindValue(QBROWSCAP_INDEX_DB_LAST_VERSION_CHECK_PATTERN);
            if (query.exec()) {
                if (query.next())
                    lastUpdateTimestamp = query.value(0).toInt();
                else
                    lastUpdateTimestamp = 0;
            }
            else
                qCritical("Could not query '%s' for the last version check timestamp.", qPrintable(this->indexFile));

            // Get the latest version (as found by the last version check).
            query.prepare("SELECT browser_version FROM browscap WHERE pattern = ?;");
            query.addBindValue(QBROWSCAP_INDEX_DB_LAST_VERSION_PATTERN);
            if (query.exec()) {
                if (query.next())
                    this->latestVersion = query.value(0).toInt();
                else
                    this->latestVersion = -1;
            }
            else
                qCritical("Could not query '%s' for the latest version (as found by the last version check).", qPrintable(this->indexFile));

        }
    }
    QSqlDatabase::removeDatabase("last-version-check-read");

    // Decide whether to request the latest version or not.
    if (lastUpdateTimestamp > QDateTime::currentMSecsSinceEpoch() / 1000 - QBROWSCAP_MIN_UPDATE_INTERVAL) {
        // We already checked in the last 24 hours, simply return -1.
        return this->latestVersion;
    }
    else {
        this->manager.get(QNetworkRequest(QUrl(QBROWSCAP_VERSION_URL)));

        // Run our own event loop to make this download synchronous.
        QEventLoop eventLoop;
        connect(&this->manager, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));
        eventLoop.exec();

        // If there were problems downloading the latest version number, set it
        // to -1.
        if (!this->versionDownloadResult)
            this->latestVersion = -1;

        // Store the current time as the latest update check time.
        QFileInfo info(this->indexFile);
        if (info.size() > 1) {
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "last-version-check-update");
            db.setDatabaseName(this->indexFile);
            if (db.open()) {
                QSqlQuery query(db);

                query.prepare("DELETE FROM browscap WHERE pattern = ?;");
                query.addBindValue(QBROWSCAP_INDEX_DB_LAST_VERSION_CHECK_PATTERN);
                query.exec();
                query.addBindValue(QBROWSCAP_INDEX_DB_LAST_VERSION_PATTERN);
                query.exec();

                query.prepare("INSERT INTO browscap (pattern, browser_version) VALUES(?, ?);");
                query.addBindValue(QBROWSCAP_INDEX_DB_LAST_VERSION_CHECK_PATTERN);
                query.addBindValue(QDateTime::currentMSecsSinceEpoch() / 1000);
                if (!query.exec()) {
                    qCritical("Could not store the time of the latest version check! Reason: %s.", qPrintable(query.lastError().text()));
                }
                query.addBindValue(QBROWSCAP_INDEX_DB_LAST_VERSION_PATTERN);
                query.addBindValue(this->latestVersion);
                if (!query.exec()) {
                    qCritical("Could not store the latest version check! Reason: %s.", qPrintable(query.lastError().text()));
                }
            }
        }
        QSqlDatabase::removeDatabase("last-version-check-update");
    }

    return this->latestVersion;
}

/**
 * Get the version of the index corresponding to a browscap.csv file.
 *
 * @return
 *   The version number, or -1 in case of error.
 */
int QBrowsCap::getIndexVersion() const {
    int indexVersion = -1;

    QFileInfo info(this->indexFile);
    if (info.size() > 1) {
        {
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "version-check");
            db.setDatabaseName(this->indexFile);
            if (db.open()) {
                QSqlQuery query(db);
                query.prepare("SELECT browser_version FROM browscap WHERE pattern = ?;");
                query.addBindValue(QBROWSCAP_INDEX_DB_VERSION_PATTERN);
                if (query.exec()) {
                    query.next();
                    indexVersion = query.value(0).toInt();
                }
                else
                    qCritical("Could not query '%s' for the version number.", qPrintable(this->indexFile));
            }
        }
        QSqlDatabase::removeDatabase("version-check");
    }

    return indexVersion;
}

/**
 * Compare the browscap.csv version with the index version. If they match,
 * the index is up-to-date.
 */
bool QBrowsCap::indexIsUpToDate() const {
    if (!QFile(this->indexFile).exists()) {
        return false;
    }

    return this->getCsvVersion() == this->getIndexVersion();
}

/**
 * Open a connection to the index DB, if it doesn't exist yet.
 */
bool QBrowsCap::connectIndexDB() {
    // Ensure we only maintain one "index" DB connection.
    if (QSqlDatabase::connectionNames().contains("index"))
        return true;

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "index");
    db.setDatabaseName(this->indexFile);
    if (!db.open()) {
        qCritical("Could not open the database: %s.", qPrintable(db.lastError().text()));
        return false;
    }
    else
        return true;
}

/**
 * We use a simple SQLite database for the index. Since browscap.csv uses
 * *NIX' globbing functionality to match patterns, and SQLite also has a
 * GLOB operator, this is a perfect match.
 */
bool QBrowsCap::buildIndex(bool force, bool ignoreCrawlers, bool ignoreFeedReaders, bool ignoreBanned, bool ignoreNoJS) {
    // We need a browscap.csv file to build an index.
    if (this->csvFile.isEmpty()) {
        return false;
    }

    // If the index is up-to-date and we're not rebuilding the index with
    // force, then we don't have to do anything.
    if (!force && this->indexIsUpToDate()) {
        this->connectIndexDB();
        return true;
    }

    // Delete existing index, possibly with force, or bail out.
    if (QFile::exists(this->indexFile)) {
        if (!force && this->indexIsUpToDate()) {
            qWarning("Index already exists.");
            return false;
        }
        else if (!QFile::remove(this->indexFile)) {
            qCritical("Existing index could not be deleted");
            return false;
        }
    }

    // Open the database in which the index will be stored.
    if (!this->connectIndexDB()) {
        qCritical("Existing index could not be opened");
        return false;
    }

    // Create the schema.
    QSqlDatabase index = QSqlDatabase::database("index");
    QSqlQuery query(index);
    if (!query.exec("CREATE TABLE browscap(pattern TEXT PRIMARY KEY, \
                                           platform TEXT, \
                                           browser_name TEXT, \
                                           browser_version TEXT, \
                                           browser_version_major INTEGER, \
                                           browser_version_minor INTEGER, \
                                           is_mobile INTEGER \
                                           );")) {
        qCritical("Failed to create table: %s.", qPrintable(query.lastError().text()));
        return false;
    }

    // Parse the browscap.csv file.
    QFile csv(this->csvFile);
    quint64 numLines = 0;
    if (!csv.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical("Could not open '%s' file for reading: %s.", qPrintable(this->csvFile), qPrintable(csv.errorString()));
        return false;
    }
    else {
        QTextStream in(&csv);
        QString line;
        QString pattern, browser, version, platform;
        QString parentBrowser, parentVersion, parentPlatform;
        int majorVersion = 0, minorVersion = 0;
        int parentMajorVersion = 0, parentMinorVersion = 0;
        bool hasJS, isBanned, isMobile, isCrawler, isFeedReader;
        bool parentHasJS = false, parentIsBanned = false, parentIsMobile = false, parentIsCrawler = false, parentIsFeedReader = false;
        QStringList parts;

        int rows = 0;
        query.prepare("INSERT INTO browscap VALUES(?, ?, ?, ?, ?, ?, ?);");

        while (!in.atEnd()) {
            line = in.readLine();
            parts = line.mid(1, line.length() - 2).split("\",\"");
            numLines++;

            if (numLines <= 3) {
                if (numLines == 2) {
                    // Store the version info.
                    query.addBindValue(QBROWSCAP_INDEX_DB_VERSION_PATTERN);
                    query.addBindValue("");
                    query.addBindValue("");
                    query.addBindValue(parts[0]);
                    query.addBindValue(0);
                    query.addBindValue(0);
                    query.addBindValue(false);
                    query.exec();
                }
                // Lines 1 and 3 don't contain anything useful. Line 2 is
                // parsed above.
                continue;
            }

            pattern = parts[1].remove('[').remove(']');
            browser      = (!parts[2].isEmpty()) ? parts[2]         : parentBrowser;
            version      = (!parts[3].isEmpty()) ? parts[3]         : parentVersion;
            majorVersion = (!parts[4].isEmpty()) ? parts[4].toInt() : parentMajorVersion;
            minorVersion = (!parts[5].isEmpty()) ? parts[5].toInt() : parentMinorVersion;
            platform     = (!parts[6].isEmpty()) ? parts[6]         : parentPlatform;

            // hasJS
            if (QString::compare(parts[18], "default", Qt::CaseInsensitive) != 0)
                hasJS = QString::compare(parts[18], "true", Qt::CaseInsensitive) == 0;
            else
                hasJS = parentHasJS;
            // isBanned
            if (QString::compare(parts[21], "default", Qt::CaseInsensitive) != 0)
                isBanned = QString::compare(parts[21], "true", Qt::CaseInsensitive) == 0;
            else
                isBanned = parentIsBanned;
            // isMobile
            if (QString::compare(parts[22], "default", Qt::CaseInsensitive) != 0)
                isMobile = QString::compare(parts[22], "true", Qt::CaseInsensitive) == 0;
            else
                isMobile = parentIsMobile;
            // isFeedReader
            if (QString::compare(parts[23], "default", Qt::CaseInsensitive) != 0)
                isFeedReader = QString::compare(parts[23], "true", Qt::CaseInsensitive) == 0;
            else
                isFeedReader = parentIsFeedReader;
            // isCrawler
            if (QString::compare(parts[24], "default", Qt::CaseInsensitive) != 0)
                isCrawler = QString::compare(parts[24], "true", Qt::CaseInsensitive) == 0;
            else
                isCrawler = parentIsCrawler;

            // Ignore abstract parents.
            if (pattern.compare(parts[0]) == 0) {
                parentBrowser = browser;
                parentVersion = version;
                parentMajorVersion= majorVersion;
                parentMinorVersion = minorVersion;
                parentPlatform = platform;
                parentHasJS = hasJS;
                parentIsMobile = isMobile;
                parentIsBanned = isBanned;
                parentIsCrawler = isCrawler;
                parentIsFeedReader = isFeedReader;
                continue;
            }

            if (ignoreBanned && isBanned)
                continue;
            if (ignoreCrawlers && isCrawler)
                continue;
            if (ignoreFeedReaders && isFeedReader)
                continue;
            if (ignoreNoJS && !hasJS)
                continue;

            rows++;

            query.addBindValue(pattern);
            query.addBindValue(platform);
            query.addBindValue(browser);
            query.addBindValue(version);
            query.addBindValue(majorVersion);
            query.addBindValue(minorVersion);
            query.addBindValue(isMobile);
            query.exec();
        }
    }

    return true;
}

/**
 * Download an update of the browscap.csv file. This is entirely optional
 * and is the only
 *
 * @see http://browsers.garykeith.com/terms.asp
 */
bool QBrowsCap::downloadUpdate(const QString & targetPath) {
    this->csvTargetPath = targetPath;
    this->manager.get(QNetworkRequest(QUrl(QBROWSCAP_CSV_URL)));

    // Run our own event loop to make this download synchronous.
    QEventLoop eventLoop;
    connect(&this->manager, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));
    eventLoop.exec();

    return csvDownloadResult;
}

void QBrowsCap::downloadFinished(QNetworkReply * reply) {
    if (reply->error()) {
        qDebug() << reply->url() << "download failed:" << reply->errorString();
        if (reply->url().toString().compare(QBROWSCAP_CSV_URL) == 0) {
            this->csvDownloadResult = false;
            emit downloadedUpdate(false, reply->errorString());
        }
        else {
            this->versionDownloadResult = false;
            emit versionChecked(false, -1, reply->errorString());
        }
    }
    else {
        if (reply->url().toString().compare(QBROWSCAP_CSV_URL) == 0) {
            QFile file(this->csvTargetPath);
            if (!file.open(QIODevice::WriteOnly)) {
                qDebug() << "browscap.csv download succeded, write failed:" << file.errorString();
                this->csvDownloadResult = false;
                emit downloadedUpdate(false, QString("Could not open %1 for writing: %2").arg(this->csvTargetPath, file.errorString()));
            }
            file.write(reply->readAll());
            file.close();

            this->csvDownloadResult = true;
            emit downloadedUpdate(true);
        }
        else {
            QString line = reply->readLine();
            QString failureReason = QString::null;
            if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200) {
                failureReason = "The BrowsCap site is blocking our requests, because we've requested updates >10 times in 24 hours.";
                qWarning("%s.", qPrintable(failureReason));
                this->versionDownloadResult = false;
                this->latestVersion = -1;
            }
            else {
                this->versionDownloadResult = true;
                this->latestVersion = line.toInt();
            }
            emit versionChecked(this->versionDownloadResult, this->latestVersion, failureReason);
        }
    }
}

/**
 * Match the user agent string
 */
QPair<bool, QBrowsCapRecord> QBrowsCap::matchUserAgent(const QString & userAgent) {
    QPair<bool, QBrowsCapRecord> answer;
    static bool indexConnected = false;

    if (!indexConnected)
        this->connectIndexDB();

    this->cacheMutex.lock();
    if (!this->cache.contains(userAgent)) {
        this->cacheMutex.unlock();

        QSqlDatabase index = QSqlDatabase::database("index");
        QSqlQuery query(index);
        query.prepare("SELECT platform, \
                              browser_name, browser_version, \
                              browser_version_major, browser_version_minor, \
                              is_mobile \
                       FROM browscap \
                       WHERE ? GLOB pattern \
                       ORDER BY LENGTH(pattern) \
                       DESC LIMIT 1");
        query.addBindValue(userAgent);
        query.exec();
        if (query.next()) {
            answer.first = true;
            answer.second = QBrowsCapRecord(query.value(0).toString(),
                                            query.value(1).toString(),
                                            query.value(2).toString(),
                                            query.value(3).toInt(),
                                            query.value(4).toInt(),
                                            query.value(5).toBool());
        }
        else {
            // No match: unidentifiable user agent.
            answer.first = false;
        }

        this->cacheMutex.lock();
        this->cache.insert(userAgent, answer);
        this->cacheMutex.unlock();
    }
    else {
        this->cacheMutex.unlock();

        answer = this->cache.value(userAgent);
    }

    return answer;
}

#ifdef DEBUG
QDebug operator<<(QDebug dbg, const QBrowsCapRecord & record) {
    dbg.nospace() << record.browser_name.toStdString().c_str() << " " << record.browser_version.toStdString().c_str()
                  << " (" << record.browser_version_major << ", " << record.browser_version_minor << ")"
                  << " on " << record.platform.toStdString().c_str();
    return dbg.nospace();
}
#endif
