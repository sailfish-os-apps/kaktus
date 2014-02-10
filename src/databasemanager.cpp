/*
  Copyright (C) 2014 Michal Kosciesza <michal@mkiol.net>

  This file is part of Kaktus.

  Kaktus is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Kaktus is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Kaktus.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "databasemanager.h"

const QString DatabaseManager::version = QString("0.2");

DatabaseManager::DatabaseManager(QObject *parent) :
    QObject(parent)
{}

void DatabaseManager::init()
{
    if (!openDB()) {
        qWarning() << "DB can not be opened!";
        emit error();
        return;
    }
    if (!checkParameters()) {
        qWarning() << "Check DB parameters failed!";
        emit error();
        return;
    }
}

bool DatabaseManager::openDB()
{
    _db = QSqlDatabase::addDatabase("QSQLITE","qt_sql_nreader_connection");
    Settings *s = Settings::instance();

    dbFilePath = s->getSettingsDir();
    dbFilePath.append(QDir::separator()).append("settings.db");
    dbFilePath = QDir::toNativeSeparators(dbFilePath);
    _db.setDatabaseName(dbFilePath);
    //_db.setConnectOptions("QSQLITE_ENABLE_SHARED_CACHE");

    return _db.open();
}

QSqlError DatabaseManager::lastError()
{
    return _db.lastError();
}

bool DatabaseManager::deleteDB()
{
    _db.close();
    return QFile::remove(dbFilePath);
}

bool DatabaseManager::isTableExists(const QString &name)
{
    if (_db.isOpen()) {
        QSqlQuery query(_db);
        if (query.exec(QString("SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name='%1';").arg(name))) {
            while(query.next()) {
                //qDebug() << query.value(0).toInt();
                if(query.value(0).toInt() == 1) {
                    return true;
                } else {
                    return false;
                }
            }
        } else {
            qWarning() << "SQL error!";
            return false;
        }
    } else {
        qWarning() << "DB is not opened!";
        return false;
    }

    /// @todo fix it!
}

bool DatabaseManager::checkParameters()
{
    bool createDB = false;

    if (_db.isOpen()) {
        QSqlQuery query(_db);

        query.exec("PRAGMA journal_mode = MEMORY");
        query.exec("PRAGMA synchronous = OFF");

        if (isTableExists("parameters")) {
            // Table parameters exists
            //qDebug() << "DB parameters table exists!";
            query.exec("SELECT value FROM parameters WHERE name='version';");
            if (query.first()) {
                //qDebug() << "DB version=" << query.value(0).toString();
                if (query.value(0).toString() != version) {
                    qWarning() << "DB version mismatch!";
                    createDB = true;
                } else {
                    qDebug() << "DB version ok!";
                    // Check is Dashboard exists
                    if (!isDashborardExists()) {
                        emit empty();
                    } else {
                        emit notEmpty();
                    }

                    return true;
                }
            }
            else {
                createDB = true;
            }
        } else {
            //qDebug() << "Parameters table not exists!";
            createDB = true;
        }

        if (createDB) {
            if (!deleteDB()) {
                qWarning() << "DB can not be deleted!";
                return false;
            }
            if (!openDB()) {
                qWarning() << "DB can not be opened!";
                return false;
            }
            if (!createStructure()) {
                qWarning() << "Create DB structure faild!";
                return false;
            }
            if (!createActionsStructure()) {
                qWarning() << "Create Actions structure faild!";
                return false;
            }
            // New empty DB created
            qDebug() << "New empty DB created!";
            emit empty();
        } else {
            emit notEmpty();
        }

        return true;

    } else {
        qWarning() << "DB is not opened!";
        return false;
    }
}

bool DatabaseManager::createStructure()
{
    bool ret = true;
    if (_db.isOpen()) {
        QSqlQuery query(_db);

        query.exec("PRAGMA journal_mode = MEMORY");
        query.exec("PRAGMA synchronous = OFF");

        ret = query.exec("DROP TABLE IF EXISTS parameters;");
        ret = query.exec("CREATE TABLE IF NOT EXISTS parameters ("
                         "name CHARACTER(10) PRIMARY KEY, "
                         "value VARCHAR(10), "
                         "description TEXT "
                         ");");
        ret = query.exec(QString("INSERT INTO parameters VALUES('%1','%2','%3');")
                         .arg("version").arg(DatabaseManager::version).arg("Data structure version"));
    } else {
        qWarning() << "DB is not opened!";
        return false;
    }

    return ret;
}

bool DatabaseManager::createActionsStructure()
{
    bool ret = true;
    if (_db.isOpen()) {
        QSqlQuery query(_db);

        query.exec("PRAGMA journal_mode = MEMORY");
        query.exec("PRAGMA synchronous = OFF");

        ret = query.exec("DROP TABLE IF EXISTS actions;");
        ret = query.exec("CREATE TABLE IF NOT EXISTS actions ("
                         "type INTEGER, "
                         "feed_id VARCHAR(50), "
                         "entry_id VARCHAR(50), "
                         "older_date TIMESTAMP, "
                         "date TIMESTAMP"
                         ");");
    } else {
        qWarning() << "DB is not opened!";
        return false;
    }

    return ret;
}

bool DatabaseManager::createTabsStructure()
{
    bool ret = true;
    if (_db.isOpen()) {
        QSqlQuery query(_db);

        query.exec("PRAGMA journal_mode = MEMORY");
        query.exec("PRAGMA synchronous = OFF");

        ret = query.exec("DROP TABLE IF EXISTS tabs;");
        ret = query.exec("CREATE TABLE tabs ("
                         "id VARCHAR(50) PRIMARY KEY, "
                         "dashboard_id VARCHAR(50), "
                         "title VARCHAR(100), "
                         "icon VARCHAR(100) "
                         ");");
    } else {
        qWarning() << "DB is not opened!";
        return false;
    }

    return ret;
}

bool DatabaseManager::createCacheStructure()
{
    bool ret = true;
    if (_db.isOpen()) {
        QSqlQuery query(_db);

        query.exec("PRAGMA journal_mode = MEMORY");
        query.exec("PRAGMA synchronous = OFF");

        ret = query.exec("DROP TABLE IF EXISTS cache;");
        ret = query.exec("CREATE TABLE cache ("
                         "id CHAR(32) PRIMARY KEY, "
                         "orig_url CHAR(32), "
                         "final_url CHAR(32), "
                         "type VARCHAR(50), "
                         "content_type TEXT, "
                         "entry_id VARCHAR(50), "
                         "feed_id VARCHAR(50), "
                         "cached INTEGER DEFAULT 0, "
                         "cached_date TIMESTAMP "
                         ");");
    } else {
        qWarning() << "DB is not opened!";
        return false;
    }

    return ret;
}

bool DatabaseManager::createDashboardsStructure()
{
    bool ret = true;
    if (_db.isOpen()) {
        QSqlQuery query(_db);

        query.exec("PRAGMA journal_mode = MEMORY");
        query.exec("PRAGMA synchronous = OFF");

        ret = query.exec("DROP TABLE IF EXISTS dashboards;");
        ret = query.exec("CREATE TABLE dashboards ("
                         "id VARCHAR(50) PRIMARY KEY, "
                         "name TEXT, "
                         "title TEXT, "
                         "description TEXT "
                         ");");
    } else {
        qWarning() << "DB is not opened!";
        return false;
    }

    return ret;
}

bool DatabaseManager::createFeedsStructure()
{
    bool ret = true;
    if (_db.isOpen()) {
        QSqlQuery query(_db);

        query.exec("PRAGMA journal_mode = MEMORY");
        query.exec("PRAGMA synchronous = OFF");

        ret = query.exec("DROP TABLE IF EXISTS feeds;");
        ret = query.exec("CREATE TABLE feeds ("
                         "id VARCHAR(50) PRIMARY KEY, "
                         "tab_id VARCHAR(50), "
                         "title TEXT, "
                         "content TEXT, "
                         "link TEXT, "
                         "url TEXT, "
                         "stream_id VARCHAR(50), "
                         "unread INTEGER DEFAULT 0, "
                         "read INTEGER DEFAULT 0, "
                         "readlater INTEGER DEFAULT 0, "
                         "item_count INTEGER, "
                         "new_item_count INTEGER DEFAULT 0, "
                         "error_code INTEGER DEFAULT 0, "
                         "last_parsed TIMESTAMP, "
                         "next_update TIMESTAMP, "
                         "last_update TIMESTAMP "
                         ");");
    } else {
        qWarning() << "DB is not opened!";
        return false;
    }

    return ret;
}

bool DatabaseManager::createEntriesStructure()
{
    bool ret = true;
    if (_db.isOpen()) {
        QSqlQuery query(_db);

        query.exec("PRAGMA journal_mode = MEMORY");
        query.exec("PRAGMA synchronous = OFF");

        ret = query.exec("DROP TABLE IF EXISTS entries;");
        ret = query.exec("CREATE TABLE entries ("
                         "id VARCHAR(50) PRIMARY KEY, "
                         "feed_id VARCHAR(50), "
                         "title TEXT, "
                         "content TEXT, "
                         "link TEXT, "
                         "read INTEGER DEFAULT 0, "
                         "readlater INTEGER DEFAULT 0, "
                         "created_at TIMESTAMP, "
                         "updated_at TIMESTAMP, "
                         "date TIMESTAMP, "
                         "cached INTEGER DEFAULT 0, "
                         "cached_date TIMESTAMP "
                         ");");
    } else {
        qWarning() << "DB is not opened!";
        return false;
    }

    return ret;
}


bool DatabaseManager::writeDashboard(const Dashboard &dashboard)
{
    bool ret = false;
    if (_db.isOpen()) {
        QSqlQuery query(_db);
        ret = query.exec(QString("INSERT INTO dashboards VALUES('%1','%2','%3','%4');")
                         .arg(dashboard.id).arg(dashboard.name)
                         .arg(QString(dashboard.title.toUtf8().toBase64()))
                         .arg(QString(dashboard.description.toUtf8().toBase64())));
    }

    return ret;
}

bool DatabaseManager::writeCache(const CacheItem &item, int cacheDate, int flag)
{
    bool ret = false;
    if (_db.isOpen()) {
        QSqlQuery query(_db);
        //qDebug() << "item.contentType=" << item.contentType << QString(item.contentType.toUtf8().toBase64());
        ret = query.exec(QString("INSERT INTO cache (id, orig_url, final_url, type, content_type, entry_id, feed_id, cached, cached_date) VALUES('%1','%2','%3','%4','%5','%6','%7',%8,'%9');")
                         .arg(item.id)
                         .arg(item.origUrl).arg(item.finalUrl)
                         .arg(item.type)
                         .arg(QString(item.contentType.toUtf8().toBase64()))
                         .arg(item.entryId).arg(item.feedId)
                         .arg(flag)
                         .arg(cacheDate)
                         );
    }

    return ret;
}

bool DatabaseManager::writeTab(const QString &dashboardId, const Tab &tab)
{
    bool ret = false;
    if (_db.isOpen()) {
        QSqlQuery query(_db);
        ret = query.exec(QString("INSERT INTO tabs VALUES('%1','%2','%3','%4');")
                         .arg(tab.id).arg(dashboardId)
                         .arg(QString(tab.title.toUtf8().toBase64()))
                         .arg(tab.icon));
    }

    return ret;
}

bool DatabaseManager::writeAction(const Action &action, int date)
{
    bool ret = false;
    if (_db.isOpen()) {

        // finding reverse action type
        DatabaseManager::ActionsTypes rtype, ftype;
        switch (action.type) {
        case DatabaseManager::SetRead:
            rtype = DatabaseManager::UnSetRead;
            break;
        case DatabaseManager::UnSetRead:
            rtype = DatabaseManager::SetRead;
            break;
        case DatabaseManager::UnSetReadlater:
            rtype = DatabaseManager::SetReadlater;
            break;
        case DatabaseManager::SetReadlater:
            rtype = DatabaseManager::UnSetReadlater;
            break;
        }

        bool empty = true; int rowid;
        QSqlQuery query(QString("SELECT rowid, type FROM actions WHERE entry_id='%1' AND (type=%2 OR type=%3) ORDER BY date;")
                        .arg(action.entryId)
                        .arg(static_cast<int>(action.type))
                        .arg(static_cast<int>(rtype)),_db);
        while(query.next()) {
            empty = false;
            ftype = static_cast<ActionsTypes>(query.value(1).toInt());
            rowid = query.value(0).toInt();
        }

        qDebug() << "type=" << action.type << "rtype=" << rtype << "empty=" << empty << "rowid=" << rowid;

        if (!empty && ftype!=action.type) {
            ret = query.exec(QString("UPDATE actions SET type=%1, date=%2 WHERE rowid=%3;")
                             .arg(static_cast<int>(action.type))
                             .arg(date)
                             .arg(rowid));
            qDebug() << "!empty && ftype!=action.type, ret=" << ret;
        }

        if (empty) {
            ret = query.exec(QString("INSERT INTO actions VALUES(%1,'%2','%3',%4,%5);")
                             .arg(static_cast<int>(action.type))
                             .arg(action.feedId)
                             .arg(action.entryId)
                             .arg(action.olderDate)
                             .arg(date));

            qDebug() << "empty, ret=" << ret;
        }

        qDebug() << "writeAction, ret=" << ret;

    } else {
        qWarning() << "DB is not open!";
    }

    return ret;
}

bool DatabaseManager::writeFeed(const QString &tabId, const Feed &feed)
{
    bool ret = false;
    if (_db.isOpen()) {
        QSqlQuery query(_db);
        //qDebug() << "writeFeed, " << feed.id << tabId << feed.title;
        ret = query.exec(QString("INSERT INTO feeds (id, tab_id, title, content, link, url, stream_id, unread, readlater, last_update) VALUES('%1','%2','%3','%4','%5','%6','%7',%8,%9,'%10');")
                         .arg(feed.id)
                         .arg(tabId)
                         .arg(QString(feed.title.toUtf8().toBase64()))
                         .arg(QString(feed.content.toUtf8().toBase64()))
                         .arg(feed.link)
                         .arg(feed.url)
                         .arg(feed.streamId)
                         .arg(feed.unread)
                         .arg(feed.readlater)
                         .arg(feed.lastUpdate));
        if(!ret) {
            ret = query.exec(QString("UPDATE feeds SET last_update='%1', unread=%2, readlater=%3 WHERE id='%4';")
                             .arg(feed.lastUpdate)
                             .arg(feed.unread)
                             .arg(feed.readlater)
                             .arg(feed.id));
        }
    }

    return ret;
}

bool DatabaseManager::removeFeed(const QString &feedId)
{
    bool ret = false;
    if (_db.isOpen()) {
        QSqlQuery query(_db);
        ret = query.exec(QString("DELETE FROM cache WHERE entry_id IN (SELECT id FROM entries WHERE feed_id='%1');").arg(feedId));
        ret = query.exec(QString("DELETE FROM entries WHERE feed_id='%1'").arg(feedId));
        ret = query.exec(QString("DELETE FROM feeds WHERE id='%1'").arg(feedId));

    }

    return ret;
}

/*bool DatabaseManager::removeCacheItems(const QString &entryId)
{
    bool ret = false;
    if (_db.isOpen()) {
        QSqlQuery query(_db);
        ret = query.exec(QString("DELETE FROM cache WHERE entry_id='%1'").arg(entryId));
    }

    return ret;
}*/

bool DatabaseManager::writeEntry(const QString &feedId, const Entry &entry)
{
    bool ret = false;
    if (_db.isOpen()) {
        QSqlQuery query(_db);
        //qDebug() << "writeEntry, feedId=" << feedId << "title=" << entry.title;
        ret = query.exec(QString("INSERT INTO entries (id, feed_id, title, content, link, read, readlater, date) VALUES('%1','%2','%3','%4','%5',%6,%7,'%8');")
                         .arg(entry.id)
                         .arg(feedId)
                         .arg(QString(entry.title.toUtf8().toBase64()))
                         .arg(QString(entry.content.toUtf8().toBase64()))
                         .arg(entry.link)
                         .arg(entry.read).arg(entry.readlater)
                         .arg(entry.date));
        if(!ret) {
            //qDebug() << entry.title << entry.read;
            ret = query.exec(QString("UPDATE entries SET read=%1, readlater=%2 WHERE id='%3';")
                             .arg(entry.read)
                             .arg(entry.readlater)
                             .arg(entry.id));
        }
    }

    return ret;
}

bool DatabaseManager::updateEntryCache(const QString &entryId, int cacheDate, int flag)
{
    bool ret = false;
    if (_db.isOpen()) {
        QSqlQuery query(_db);
        ret = query.exec(QString("UPDATE entries SET cached=%1, cached_date='%2' WHERE id='%3';")
                         .arg(flag)
                         .arg(cacheDate)
                         .arg(entryId));
    }

    return ret;
}

//bool updateEntryReadFlag(const QString &entryId, int read);
//bool updateEntryReadlaterFlag(const QString &entryId, int readlater);

bool DatabaseManager::updateEntryReadFlag(const QString &entryId, int read)
{
    bool ret = false;
    if (_db.isOpen()) {
        QSqlQuery query(_db);
        ret = query.exec(QString("UPDATE entries SET read=%1 WHERE id='%2';")
                         .arg(read)
                         .arg(entryId));
    }

    return ret;
}

bool DatabaseManager::updateEntryReadlaterFlag(const QString &entryId, int readlater)
{
    bool ret = false;
    if (_db.isOpen()) {
        QSqlQuery query(_db);
        ret = query.exec(QString("UPDATE entries SET readlater=%1 WHERE id='%2';")
                         .arg(readlater)
                         .arg(entryId));
    }

    return ret;
}



bool DatabaseManager::updateFeedUnreadFlag(const QString &feedId, int unread)
{
    bool ret = false;
    if (_db.isOpen()) {
        QSqlQuery query(_db);
        ret = query.exec(QString("UPDATE feeds SET unread=%1 WHERE id='%2';")
                         .arg(unread)
                         .arg(feedId));
    }

    return ret;
}

bool DatabaseManager::updateFeedReadlaterFlag(const QString &feedId, int readlater)
{
    bool ret = false;
    if (_db.isOpen()) {
        QSqlQuery query(_db);
        ret = query.exec(QString("UPDATE feeds SET readlater=%1 WHERE id='%2';")
                         .arg(readlater)
                         .arg(feedId));
    }

    return ret;
}


QList<DatabaseManager::Dashboard> DatabaseManager::readDashboards()
{
    QList<DatabaseManager::Dashboard> list;

    if (_db.isOpen()) {
        QSqlQuery query("SELECT id, name, title, description FROM dashboards;",_db);
        while(query.next()) {
            Dashboard d;
            d.id = query.value(0).toString();
            d.name = query.value(1).toString();
            d.title = QString(QByteArray::fromBase64(query.value(2).toByteArray()));
            d.description = QString(QByteArray::fromBase64(query.value(3).toByteArray()));
            list.append(d);
        }
    } else {
        qWarning() << "DB is not open!";
    }

    return list;
}

QList<DatabaseManager::Tab> DatabaseManager::readTabs(const QString &dashboardId)
{
    QList<DatabaseManager::Tab> list;

    if (_db.isOpen()) {
        QSqlQuery query(QString("SELECT id, title, icon FROM tabs WHERE dashboard_id='%1';")
                        .arg(dashboardId),_db);
        while(query.next()) {
            //qDebug() << "readTabs, " << query.value(1).toString();
            Tab t;
            t.id = query.value(0).toString();
            t.title = QString(QByteArray::fromBase64(query.value(1).toByteArray()));
            t.icon = query.value(2).toString();
            list.append(t);
        }
    } else {
        qWarning() << "DB is not open!";
    }

    return list;
}

QList<DatabaseManager::Feed> DatabaseManager::readFeeds(const QString &tabId)
{
    QList<DatabaseManager::Feed> list;

    if (_db.isOpen()) {
        QSqlQuery query(QString("SELECT id, title, content, link, url, stream_id, unread, readlater, last_update FROM feeds WHERE tab_id='%1';")
                        .arg(tabId),_db);
        while(query.next()) {
            //qDebug() << "readFeeds, " << query.value(1).toString();
            Feed f;
            f.id = query.value(0).toString();
            f.title = QString(QByteArray::fromBase64(query.value(1).toByteArray()));
            f.content = QString(QByteArray::fromBase64(query.value(2).toByteArray()));
            f.link = query.value(3).toString();
            f.url = query.value(4).toString();
            f.streamId = query.value(5).toString();
            f.unread = query.value(6).toInt();
            f.readlater = query.value(7).toInt();
            f.lastUpdate = query.value(8).toInt();
            list.append(f);
        }
    } else {
        qWarning() << "DB is not open!";
    }

    return list;
}

QString DatabaseManager::readFeedId(const QString &entryId)
{
    if (_db.isOpen()) {
        QSqlQuery query(QString("SELECT feed_id FROM entries WHERE id='%1';").arg(entryId),_db);
        while(query.next()) {
            return query.value(0).toString();
        }
    } else {
        qWarning() << "DB is not open!";
    }

    return "";
}

/*QList<DatabaseManager::CacheItem> DatabaseManager::readCacheItems()
{
    QList<DatabaseManager::CacheItem> list;

    if (_db.isOpen()) {
        QSqlQuery query("SELECT id, orig_url, final_url, type, content_type, entry_id, feed_id FROM cache WHERE cached>0;",_db);
        while(query.next()) {
            CacheItem item;
            item.id = query.value(0).toString();
            item.origUrl = query.value(1).toString();
            item.finalUrl = query.value(2).toString();
            item.type = query.value(3).toString();
            item.contentType = QString(QByteArray::fromBase64(query.value(4).toByteArray()));
            item.entryId = query.value(5).toString();
            item.feedId = query.value(6).toString();
            list.append(item);
        }
    } else {
        qWarning() << "DB is not open!";
    }

    return list;
}*/

QList<QString> DatabaseManager::readCacheIdsOlderThan(int cacheDate, int limit)
{
    QList<QString> list;

    if (_db.isOpen()) {

        QSqlQuery query(QString("SELECT id FROM cache WHERE entry_id IN (SELECT id FROM entries WHERE cached_date<%1 AND feed_id IN (SELECT feed_id FROM entries GROUP BY feed_id HAVING count(*)>%2));")
                        .arg(cacheDate).arg(limit), _db);
        while(query.next()) {
            list.append(query.value(0).toString());
        }
    } else {
        qWarning() << "DB is not open!";
    }

    return list;
}

/*bool DatabaseManager::removeCacheItemsOlderThan(int cacheDate, int limit)
{
    bool ret = false;
    if (_db.isOpen()) {
        QSqlQuery query(_db);
        ret = query.exec(QString("DELETE FROM cache WHERE entry_id IN (SELECT id FROM entries WHERE cached_date<%1 AND feed_id IN (SELECT feed_id FROM entries GROUP BY feed_id HAVING count(*)>%2));")
                        .arg(cacheDate).arg(limit));
    } else {
        qWarning() << "DB is not open!";
    }

    return ret;
}*/


DatabaseManager::CacheItem DatabaseManager::readCacheItemFromOrigUrl(const QString &origUrl)
{
    CacheItem item;
    if (_db.isOpen()) {
        QSqlQuery query(QString("SELECT id, orig_url, final_url, type, content_type, entry_id, feed_id FROM cache WHERE orig_url='%1' AND cached=1;")
                        .arg(origUrl),_db);
        qDebug() << "size=" << query.size() << query.lastError() << origUrl;
        while(query.next()) {
            item.id = query.value(0).toString();
            item.origUrl = query.value(1).toString();
            item.finalUrl = query.value(2).toString();
            item.type = query.value(3).toString();
            item.contentType = QString(QByteArray::fromBase64(query.value(4).toByteArray()));
            item.entryId = query.value(5).toString();
            item.feedId = query.value(6).toString();
            //qDebug() << "item.contentType=" << item.contentType;
        }
    } else {
        qWarning() << "DB is not open!";
    }

    return item;
}

DatabaseManager::CacheItem DatabaseManager::readCacheItemFromEntryId(const QString &entryId)
{
    CacheItem item;
    if (_db.isOpen()) {
        QSqlQuery query(QString("SELECT id, orig_url, final_url, type, content_type, entry_id, feed_id FROM cache WHERE entry_id='%1' AND cached=1;")
                        .arg(entryId),_db);
        while(query.next()) {
            item.id = query.value(0).toString();
            item.origUrl = query.value(1).toString();
            item.finalUrl = query.value(2).toString();
            item.type = query.value(3).toString();
            item.contentType = QString(QByteArray::fromBase64(query.value(4).toByteArray()));
            item.entryId = query.value(5).toString();
            item.feedId = query.value(6).toString();
            //qDebug() << "item.contentType=" << item.contentType;
        }
    } else {
        qWarning() << "DB is not open!";
    }

    return item;
}

DatabaseManager::CacheItem DatabaseManager::readCacheItem(const QString &cacheId)
{
    CacheItem item;
    if (_db.isOpen()) {
        QSqlQuery query(QString("SELECT id, orig_url, final_url, type, content_type, entry_id, feed_id FROM cache WHERE id='%1';")
                        .arg(cacheId),_db);
        while(query.next()) {
            item.id = query.value(0).toString();
            item.origUrl = query.value(1).toString();
            item.finalUrl = query.value(2).toString();
            item.type = query.value(3).toString();
            item.contentType = QString(QByteArray::fromBase64(query.value(4).toByteArray()));
            item.entryId = query.value(5).toString();
            item.feedId = query.value(6).toString();
        }
    } else {
        qWarning() << "DB is not open!";
    }

    return item;
}

bool DatabaseManager::isCacheItemExists(const QString &cacheId)
{
    if (_db.isOpen()) {
        QSqlQuery query(QString("SELECT COUNT(*) FROM cache WHERE id='%1';")
                        .arg(cacheId),_db);
        while(query.next()) {
            if (query.value(0).toInt() > 0) {
                return true;
            }
        }
    } else {
        qWarning() << "DB is not open!";
    }

    return false;
}

bool DatabaseManager::isDashborardExists()
{
    if (_db.isOpen()) {
        QSqlQuery query("SELECT COUNT(*) FROM dashboards;",_db);
        while(query.next()) {
            if (query.value(0).toInt() > 0) {
                return true;
            }
        }
    } else {
        qWarning() << "DB is not open!";
    }

    return false;
}

QMap<QString,int> DatabaseManager::readFeedsLastUpdate()
{
    QMap<QString,int> list;

    if (_db.isOpen()) {
        QSqlQuery query("SELECT id, last_update FROM feeds;",_db);
        while(query.next()) {
            list.insert(query.value(0).toString(), query.value(1).toInt());
            //qDebug() << "is: " << query.value(0).toString() << "timestamp: " << query.value(1).toInt();
        }
    } else {
        qWarning() << "DB is not open!";
    }

    return list;
}

QMap<QString,int> DatabaseManager::readFeedsFirstUpdate()
{
    QMap<QString,int> list;

    if (_db.isOpen()) {
        QSqlQuery query("SELECT feed_id, min(date) FROM entries GROUP BY feed_id;",_db);
        while(query.next()) {
            list.insert(query.value(0).toString(), query.value(1).toInt());
            //qDebug() << "is: " << query.value(0).toString() << "timestamp: " << query.value(1).toInt();
        }
    } else {
        qWarning() << "DB is not open!";
    }

    return list;
}

QList<DatabaseManager::Entry> DatabaseManager::readEntries(const QString &feedId)
{
    QList<DatabaseManager::Entry> list;

    if (_db.isOpen()) {
        QSqlQuery query(QString("SELECT id, title, content, link, read, readlater, date FROM entries WHERE feed_id='%1' ORDER BY date DESC;")
                        .arg(feedId),_db);
        while(query.next()) {
            //qDebug() << "readEntries, " << query.value(1).toString();
            Entry e;
            e.id = query.value(0).toString();
            e.title = QString(QByteArray::fromBase64(query.value(1).toByteArray()));
            e.content = QString(QByteArray::fromBase64(query.value(2).toByteArray()));
            e.link = query.value(3).toString();
            e.read = query.value(4).toInt();
            e.readlater= query.value(5).toInt();
            e.date = query.value(6).toInt();
            list.append(e);
        }
    } else {
        qWarning() << "DB is not open!";
    }

    return list;
}

QList<DatabaseManager::Entry> DatabaseManager::readEntries()
{
    QList<DatabaseManager::Entry> list;

    if (_db.isOpen()) {
        QSqlQuery query("SELECT id, title, content, link, read, readlater, date FROM entries ORDER BY date DESC;",_db);
        while(query.next()) {
            Entry e;
            e.id = query.value(0).toString();
            e.title = QString(QByteArray::fromBase64(query.value(1).toByteArray()));
            e.content = QString(QByteArray::fromBase64(query.value(2).toByteArray()));
            e.link = query.value(3).toString();
            e.read = query.value(4).toInt();
            e.readlater= query.value(5).toInt();
            e.date = query.value(6).toInt();
            list.append(e);
        }
    } else {
        qWarning() << "DB is not open!";
    }

    return list;
}

QList<DatabaseManager::Action> DatabaseManager::readActions()
{
    QList<DatabaseManager::Action> list;

    if (_db.isOpen()) {
        QSqlQuery query("SELECT type, feed_id, entry_id, older_date FROM actions ORDER BY date DESC;",_db);
        while(query.next()) {
            Action a;
            a.type = static_cast<ActionsTypes>(query.value(0).toInt());
            a.feedId = query.value(1).toString();
            a.entryId = query.value(2).toString();
            a.olderDate = query.value(3).toInt();
            list.append(a);
        }
    } else {
        qWarning() << "DB is not open!";
    }

    return list;
}

QList<DatabaseManager::Entry> DatabaseManager::readEntriesCachedOlderThan(int cacheDate, int limit)
{
    QList<DatabaseManager::Entry> list;

    if (_db.isOpen()) {
        QSqlQuery query(QString("SELECT id, title, content, link, read, readlater, date FROM entries WHERE cached_date<%1 AND feed_id IN (SELECT feed_id FROM entries GROUP BY feed_id HAVING count(*)>%2);")
                        .arg(cacheDate).arg(limit), _db);
        while(query.next()) {
            Entry e;
            e.id = query.value(0).toString();
            e.title = QString(QByteArray::fromBase64(query.value(1).toByteArray()));
            e.content = QString(QByteArray::fromBase64(query.value(2).toByteArray()));
            e.link = query.value(3).toString();
            e.read = query.value(4).toInt();
            e.readlater= query.value(5).toInt();
            e.date = query.value(6).toInt();
            list.append(e);
        }
    } else {
        qWarning() << "DB is not open!";
    }

    return list;
}

bool DatabaseManager::removeEntriesOlderThan(int cacheDate, int limit)
{
    bool ret = false;
    if (_db.isOpen()) {
        QSqlQuery query(_db);
        ret = query.exec(QString("DELETE FROM cache WHERE entry_id IN (SELECT id FROM entries WHERE readlater!=1 AND cached_date<%1 AND feed_id IN (SELECT feed_id FROM entries GROUP BY feed_id HAVING count(*)>%2));")
                         .arg(cacheDate).arg(limit));
        ret = query.exec(QString("DELETE FROM entries WHERE readlater!=1 AND cached_date<%1 AND feed_id IN (SELECT feed_id FROM entries GROUP BY feed_id HAVING count(*)>%2);")
                         .arg(cacheDate).arg(limit));
    }

    return ret;
}

bool DatabaseManager::removeAction(const QString &entryId)
{
    /// @todo fix it!
    bool ret = false;
    if (_db.isOpen()) {
        QSqlQuery query(_db);
        ret = query.exec(QString("DELETE FROM actions WHERE entry_id='%1';")
                         .arg(entryId));
    }

    return ret;
}

QMap<QString,QString> DatabaseManager::readNotCachedEntries()
{
    QMap<QString,QString> list;

    if (_db.isOpen()) {
        QSqlQuery query("SELECT id, link FROM entries WHERE cached=0;",_db);
        while(query.next()) {
            list.insert(query.value(0).toString(), query.value(1).toString());
        }
    } else {
        qWarning() << "DB is not open!";
    }

    return list;
}

int DatabaseManager::readNotCachedEntriesCount()
{
    int count = 0;

    if (_db.isOpen()) {
        QSqlQuery query("SELECT count(*) FROM entries WHERE cached=0;",_db);
        while(query.next()) {
            count = query.value(0).toInt();
        }
    } else {
        qWarning() << "DB is not open!";
    }

    return count;
}

bool DatabaseManager::cleanDashboards()
{
    return createDashboardsStructure();
}

bool DatabaseManager::cleanTabs()
{
    return createTabsStructure();
}

bool DatabaseManager::cleanFeeds()
{
    return createFeedsStructure();
}

bool DatabaseManager::cleanEntries()
{
    return createEntriesStructure();
}

bool DatabaseManager::cleanCache()
{
    return createCacheStructure();
}

int DatabaseManager::readEntriesCount()
{
    int count = 0;

    if (_db.isOpen()) {
        QSqlQuery query("SELECT count(*) FROM entries;",_db);
        while(query.next()) {
            count = query.value(0).toInt();
        }
    } else {
        qWarning() << "DB is not open!";
    }

    return count;
}

int DatabaseManager::readFeedsCount()
{
    int count = 0;

    if (_db.isOpen()) {
        QSqlQuery query("SELECT count(*) FROM feeds;",_db);
        while(query.next()) {
            count = query.value(0).toInt();
        }
    } else {
        qWarning() << "DB is not open!";
    }

    return count;
}