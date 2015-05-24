/*
  Copyright (C) 2015 Michal Kosciesza <michal@mkiol.net>

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

#include <QNetworkReply>
#include <QRegExp>
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <QStringList>
#include <QDateTime>
#else
#include "qjson.h"
#endif

#include "oldreaderfetcher.h"
#include "settings.h"
#include "downloadmanager.h"
#include "utils.h"

OldReaderFetcher::OldReaderFetcher(QObject *parent) :
    Fetcher(parent),
    currentJob(Idle)
{
}

void OldReaderFetcher::signIn()
{
    data.clear();

    Settings *s = Settings::instance();

    // Check is already have cookie
    if (s->getCookie() != "") {
        prepareUploadActions();
        return;
    }

    QString password = s->getPassword();
    QString username = s->getUsername();
    int type = s->getSigninType();

    if (currentReply != NULL) {
        currentReply->disconnect();
        currentReply->deleteLater();
        currentReply = NULL;
    }

    QString body;
    QNetworkRequest request;

    switch (type) {
    case 10:
        if (password == "" || username == "") {
            qWarning() << "Username & password do not match!";
            if (busyType == Fetcher::CheckingCredentials)
                emit errorCheckingCredentials(400);
            else
                emit error(400);
            setBusy(false);
            return;
        }

        request.setUrl(QUrl("https://theoldreader.com/accounts/ClientLogin"));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
        body = "output=json&client=Kaktus&accountType=HOSTED_OR_GOOGLE&service=reader&Email="+
                QUrl::toPercentEncoding(username)+"&Passwd="+
                QUrl::toPercentEncoding(password);
        currentReply = nam.post(request,body.toUtf8());
        break;
    default:
        qWarning() << "Invalid sign in type!";
        emit error(500);
        setBusy(false);
        return;
    }

#ifndef QT_NO_SSL
    connect(currentReply, SIGNAL(sslErrors(QList<QSslError>)), SLOT(sslErrors(QList<QSslError>)));
#endif

    if (busyType == Fetcher::CheckingCredentials)
        connect(currentReply, SIGNAL(finished()), this, SLOT(finishedSignInOnlyCheck()));
    else
        connect(currentReply, SIGNAL(finished()), this, SLOT(finishedSignIn()));

    connect(currentReply, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(currentReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(networkError(QNetworkReply::NetworkError)));
}

void OldReaderFetcher::setAction()
{
    data.clear();

    DatabaseManager::Action action = actionsList.first();

    Settings *s = Settings::instance();

    //qDebug() << "########### setAction";
    //qDebug() << action.type << action.id1 << action.id2 << action.date1 << action.date2;

    if (currentReply != NULL) {
        currentReply->disconnect();
        currentReply->deleteLater();
        currentReply = NULL;
    }

    QUrl url;
    QString body;
    switch (action.type) {
    case DatabaseManager::SetRead:
        url.setUrl("https://theoldreader.com/reader/api/0/edit-tag");
        body = QString("a=user/-/state/com.google/read&i=%1").arg(action.id1);
        break;
    case DatabaseManager::UnSetRead:
        url.setUrl("https://theoldreader.com/reader/api/0/edit-tag");
        body = QString("r=user/-/state/com.google/read&i=%1").arg(action.id1);
        break;
    case DatabaseManager::SetSaved:
        url.setUrl("https://theoldreader.com/reader/api/0/edit-tag");
        body = QString("a=user/-/state/com.google/starred&i=%1").arg(action.id1);
        break;
    case DatabaseManager::UnSetSaved:
        url.setUrl("https://theoldreader.com/reader/api/0/edit-tag");
        body = QString("r=user/-/state/com.google/starred&i=%1").arg(action.id1);
        break;
    case DatabaseManager::SetStreamReadAll:
        url.setUrl("https://theoldreader.com/reader/api/0/mark-all-as-read");
        body = QString("s=%1&ts=%2").arg(action.id1).arg(
                    QString::number(s->db->readLastLastUpdateByStream(action.id1))+"000000"
                    );
        break;
    case DatabaseManager::SetTabReadAll:
        if (action.id1 == "subscriptions") {
            // Adding SetStreamReadAll action for every stream in substriptions folder
            QStringList list = s->db->readStreamIdsByTab("subscriptions");
            QStringList::iterator it = list.begin();
            while (it != list.end()) {
                DatabaseManager::Action action;
                action.type = DatabaseManager::SetStreamReadAll;
                action.id1 = *it;
                actionsList.insert(1,action);
                ++it;
            }

            finishedSetAction();
            return;
        }

        url.setUrl("https://theoldreader.com/reader/api/0/mark-all-as-read");
        body = QString("s=%1&ts=%2").arg(action.id1).arg(
                    QString::number(s->db->readLastLastUpdateByTab(action.id1))+"000000"
                    );
        break;
    case DatabaseManager::SetAllRead:
        url.setUrl("https://theoldreader.com/reader/api/0/mark-all-as-read");
        body = QString("s=user/-/state/com.google/reading-list&ts=%1").arg(
                    QString::number(s->db->readLastLastUpdateByDashboard(s->getDashboardInUse()))+"000000"
                    );
        break;
    default:
        // Unknown action -> skiping
        qWarning("Unknown action!");
        finishedSetAction();
        return;
    }

    QNetworkRequest request(url);

    // Headers
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    request.setRawHeader("Authorization",QString("GoogleLogin auth=%1").arg(s->getCookie()).toLatin1());

    //qDebug() << body;

    currentReply = nam.post(request,body.toUtf8());

    connect(currentReply, SIGNAL(finished()), this, SLOT(finishedSetAction()));
    connect(currentReply, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(currentReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(networkError(QNetworkReply::NetworkError)));
}

void OldReaderFetcher::fetchTabs()
{
    //qDebug() << "fetchTabs";
    data.clear();

    Settings *s = Settings::instance();

    if (currentReply != NULL) {
        currentReply->disconnect();
        currentReply->deleteLater();
        currentReply = NULL;
    }

    QUrl url("https://theoldreader.com/reader/api/0/tag/list?output=json");
    QNetworkRequest request(url);

    // Authorization header
    request.setRawHeader("Authorization",QString("GoogleLogin auth=%1").arg(s->getCookie()).toLatin1());

    currentReply = nam.get(request);

    connect(currentReply, SIGNAL(finished()), this, SLOT(finishedTabs()));
    connect(currentReply, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(currentReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(networkError(QNetworkReply::NetworkError)));
}

void OldReaderFetcher::fetchFeeds()
{
    //qDebug() << "fetchFeeds";
    data.clear();

    Settings *s = Settings::instance();

    if (currentReply != NULL) {
        currentReply->disconnect();
        currentReply->deleteLater();
        currentReply = NULL;
    }

    QUrl url("https://theoldreader.com/reader/api/0/subscription/list?output=json");
    QNetworkRequest request(url);

    // Authorization header
    request.setRawHeader("Authorization",QString("GoogleLogin auth=%1").arg(s->getCookie()).toLatin1());

    currentReply = nam.get(request);

    connect(currentReply, SIGNAL(finished()), this, SLOT(finishedFeeds()));
    connect(currentReply, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(currentReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(networkError(QNetworkReply::NetworkError)));
}

void OldReaderFetcher::fetchStreamUpdate()
{
    QString feedId = feedUpdateList.first().streamId;
    //qDebug() << "fetchStreamUpdate, feedId=" << feedId;

    data.clear();

    Settings *s = Settings::instance();

    if (currentReply != NULL) {
        currentReply->disconnect();
        currentReply->deleteLater();
        currentReply = NULL;
    }

    QUrl url;
    if (lastContinuation == "")
        url.setUrl(QString("https://theoldreader.com/reader/api/0/stream/contents?output=json&n=%1&s=%2")
                   .arg(limitAtOnceForUpdate).arg(feedId));
    else
        url.setUrl(QString("https://theoldreader.com/reader/api/0/stream/contents?output=json&n=%1&s=%2&c=%3")
                   .arg(limitAtOnceForUpdate).arg(feedId).arg(lastContinuation));
    QNetworkRequest request(url);

    // Authorization header
    request.setRawHeader("Authorization",QString("GoogleLogin auth=%1").arg(s->getCookie()).toLatin1());

    currentReply = nam.get(request);

    connect(currentReply, SIGNAL(finished()), this, SLOT(finishedStreamUpdate()));
    connect(currentReply, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(currentReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(networkError(QNetworkReply::NetworkError)));
}

void OldReaderFetcher::fetchStream()
{
    QString feedId;
    if (busyType == Fetcher::Updating)
        feedId = feedList.first().streamId;
    else
        feedId = feedUpdateList.first().streamId;
    //qDebug() << "fetchStream, feedId=" << feedId;

    data.clear();

    Settings *s = Settings::instance();

    if (currentReply != NULL) {
        currentReply->disconnect();
        currentReply->deleteLater();
        currentReply = NULL;
    }

    QUrl url;
    if (lastContinuation == "")
        url.setUrl(QString("https://theoldreader.com/reader/api/0/stream/contents?output=json&n=%1&s=%2")
                   .arg(limitAtOnce).arg(feedId));
    else
        url.setUrl(QString("https://theoldreader.com/reader/api/0/stream/contents?output=json&n=%1&s=%2&c=%3")
                   .arg(limitAtOnce).arg(feedId).arg(lastContinuation));
    QNetworkRequest request(url);

    // Authorization header
    request.setRawHeader("Authorization",QString("GoogleLogin auth=%1").arg(s->getCookie()).toLatin1());

    currentReply = nam.get(request);

    connect(currentReply, SIGNAL(finished()), this, SLOT(finishedStream()));
    connect(currentReply, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(currentReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(networkError(QNetworkReply::NetworkError)));
}

void OldReaderFetcher::fetchStarredStream()
{
    //qDebug() << "fetchStarredStream, lastContinuation=" << lastContinuation << "continuationCount=" << continuationCount;
    data.clear();

    Settings *s = Settings::instance();

    if (currentReply != NULL) {
        currentReply->disconnect();
        currentReply->deleteLater();
        currentReply = NULL;
    }

    QUrl url;
    if (lastContinuation == "")
        url.setUrl(QString("https://theoldreader.com/reader/api/0/stream/contents?output=json&n=%1&s=user/-/state/com.google/starred")
                   .arg(limitAtOnceForStarred));
    else
        url.setUrl(QString("https://theoldreader.com/reader/api/0/stream/contents?output=json&n=%1&c=%2&s=user/-/state/com.google/starred")
                   .arg(limitAtOnceForStarred).arg(lastContinuation));
    QNetworkRequest request(url);

    // Authorization header
    request.setRawHeader("Authorization",QString("GoogleLogin auth=%1").arg(s->getCookie()).toLatin1());

    currentReply = nam.get(request);

    connect(currentReply, SIGNAL(finished()), this, SLOT(finishedStarredStream()));
    connect(currentReply, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(currentReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(networkError(QNetworkReply::NetworkError)));
}

void OldReaderFetcher::finishedSignInOnlyCheck()
{
    //qDebug() << data;
    Settings *s = Settings::instance();

    if (currentReply->error() &&
        currentReply->error()!=QNetworkReply::OperationCanceledError) {
        int code = currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // 403 Forbidden = Invalid username or password
        if (code == 403) {
            emit errorCheckingCredentials(402);
            setBusy(false);
            qWarning() << "Sign in check failed! Invalid username or password.";
            return;
        }

        qWarning() << "Sign in failed!";
        emit errorCheckingCredentials(501);
        setBusy(false);
        return;
    }

    switch (s->getSigninType()) {
    case 10:
        if (parse()) {
            QString auth = jsonObj["Auth"].toString();
            if (auth != "") {
                s->setSignedIn(true);
                s->setCookie(auth);
                emit credentialsValid();
                setBusy(false);
            } else {
                s->setSignedIn(false);
                qWarning() << "Sign in check failed! Can not find Auth param.";
                emit errorCheckingCredentials(501);
                setBusy(false);
            }
        } else {
            s->setSignedIn(false);
            qWarning() << "Sign in check failed! Error while parsing JSON";
            emit errorCheckingCredentials(501);
            setBusy(false);
        }
        break;
    default:
        qWarning() << "Invalid sign in type!";
        emit errorCheckingCredentials(502);
        setBusy(false);
        s->setSignedIn(false);
    }
}

void OldReaderFetcher::finishedSignIn()
{
    //qDebug() << data;
    Settings *s = Settings::instance();

    if (currentReply->error() &&
        currentReply->error()!=QNetworkReply::OperationCanceledError) {
        int code = currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        // 403 Forbidden = Invalid username or password
        if (code == 403) {
            emit error(402);
            setBusy(false);
            qWarning() << "Sign in failed! Invalid username or password.";
            return;
        }

        qWarning() << "Sign in failed!";
        emit error(501);
        setBusy(false);
        return;
    }

    switch (s->getSigninType()) {
    case 10:
        if (parse()) {
            QString auth = jsonObj["Auth"].toString();
            if (auth != "") {
                s->setSignedIn(true);
                s->setCookie(auth);
                prepareUploadActions();
            } else {
                s->setSignedIn(false);
                qWarning() << "Sign in failed! Can not find Auth param.";
                emit error(501);
                setBusy(false);
            }
        } else {
            s->setSignedIn(false);
            qWarning() << "Sign in failed! Error while parsing JSON";
            emit error(501);
            setBusy(false);
        }
        break;
    default:
        qWarning() << "Invalid sign in type!";
        emit error(502);
        setBusy(false);
        s->setSignedIn(false);
    }
}

void OldReaderFetcher::finishedTabs()
{
    //qDebug() << data;
    if (currentReply->error()) {
        emit error(500);
        setBusy(false);
        return;
    }

    startJob(StoreTabs);
}

void OldReaderFetcher::finishedTabs2()
{
    Settings *s = Settings::instance();

    lastContinuation = "";
    continuationCount = 0;

    if (busyType == Fetcher::Updating) {
        s->db->updateEntriesFreshFlag(0); // Set current entries as not fresh
        storedFeedList = s->db->readStreamModuleTabList();
    }

    s->db->cleanStreams();
    s->db->cleanModules();

    if (tabList.isEmpty()) {
        qWarning() << "No Tabs to download!";
        if (busyType == Fetcher::Initiating)
            s->db->cleanEntries();

        // Proggres initiating
        proggressTotal = 2;
        proggress = 1;
        emit progress(proggress, proggressTotal);

        fetchStarredStream();
        return;
    }

    fetchFeeds();
}

void OldReaderFetcher::finishedFeeds()
{
    //qDebug() << data;
    if (currentReply->error()) {
        emit error(500);
        setBusy(false);
        return;
    }

    startJob(StoreFeeds);
}

void OldReaderFetcher::finishedFeeds2()
{
    Settings *s = Settings::instance();

    // Proggres initiating
    proggressTotal = feedUpdateList.count() + 2;
    proggress = 1;
    emit progress(proggress, proggressTotal);

    if (busyType == Fetcher::Updating) {
        prepareFeedLists();
        removeDeletedFeeds();
        //qDebug() << "New feeds:" << feedList.count() << "Feeds to update:" << feedUpdateList.count();
        fetchStreamUpdate();
        return;
    }

    if (busyType == Fetcher::Initiating) {
        s->db->cleanEntries();
        //feedUpdateList.clear();
        //qDebug() << "New feeds:" << feedUpdateList.count();

        if (feedUpdateList.isEmpty()) {
            qWarning() << "No Feeds to download!";

            // Proggres initiating
            proggressTotal = 2;
            proggress = 1;
            emit progress(proggress, proggressTotal);

            fetchStarredStream();
        } else {
            fetchStream();
        }
    }
}

void OldReaderFetcher::finishedStreamUpdate()
{
    //qDebug() << data;
    if (currentReply->error()) {
        emit error(500);
        setBusy(false);
        return;
    }

    startJob(StoreStreamUpdate);
}

void OldReaderFetcher::finishedStreamUpdate2()
{
    if (lastContinuation == "" ||
        continuationCount > continuationLimitForUpdate) {

        ++proggress;
        emit progress(proggress, proggressTotal);

        lastContinuation = "";
        continuationCount = 0;

        feedUpdateList.removeFirst();
        if (feedUpdateList.isEmpty()) {
            //qDebug() << "Streams update done. Starting New Streams download.";
            if (feedList.isEmpty()) {
                fetchStarredStream();
            } else {
                fetchStream();
            }
            return;
        }
    }

    fetchStreamUpdate();
}

void OldReaderFetcher::finishedStream()
{
    //qDebug() << data;
    if (currentReply->error()) {
        emit error(500);
        setBusy(false);
        return;
    }

    startJob(StoreStream);
}

void OldReaderFetcher::finishedStream2()
{
    if (lastContinuation == "" ||
        continuationCount > continuationLimit) {

        ++proggress;
        emit progress(proggress, proggressTotal);

        lastContinuation = "";
        continuationCount = 0;

        if (busyType == Fetcher::Initiating) {
            feedUpdateList.removeFirst();
            if (feedUpdateList.isEmpty()) {
                //qDebug() << "New Streams download done.";
                fetchStarredStream();
                return;
            }
        }

        if (busyType == Fetcher::Updating) {
            feedList.removeFirst();
            if (feedList.isEmpty()) {
                //qDebug() << "New Streams download done.";
                fetchStarredStream();
                return;
            }
        }
    }

    fetchStream();
}

void OldReaderFetcher::finishedStarredStream()
{
    //qDebug() << data;
    if (currentReply->error()) {
        emit error(500);
        setBusy(false);
        return;
    }

    startJob(StoreStarredStream);
}

void OldReaderFetcher::finishedStarredStream2()
{
    if (lastContinuation == "" ||
        continuationCount > continuationLimitForStarred) {

        ++proggress;
        emit progress(proggress, proggressTotal);

        //taskEnd();
        startJob(MarkSlow);
        return;
    }

    fetchStarredStream();
}

void OldReaderFetcher::finishedSetAction()
{
    //qDebug() << data;
    if (currentReply != NULL && currentReply->error()) {
        emit error(500);
        setBusy(false);
        return;
    }

    Settings *s = Settings::instance();

    // Deleting action
    DatabaseManager::Action action = actionsList.takeFirst();
    s->db->removeActionsById(action.id1);

    if (actionsList.isEmpty()) {
        //qDebug() << "All action uploaded.";
        s->db->cleanDashboards();
        startFetching();
        return;
    }

    setAction();
}

void OldReaderFetcher::finishedMarkSlow()
{
    taskEnd();
    return;
}

bool OldReaderFetcher::setConnectUrl(const QString &url)
{
    Q_UNUSED(url);
    return false;
}

void OldReaderFetcher::getConnectUrl(int type)
{
    Q_UNUSED(type)
    if (busy) {
        qWarning() << "Fetcher is busy!";
        return;
    }

    //TODO ...
}

void OldReaderFetcher::startFetching()
{
    Settings *s = Settings::instance();

    s->db->cleanDashboards();

    // Old Reader API doesnt have Dashboards
    // Manually adding dummy Dashboard
    DatabaseManager::Dashboard d;
    d.id = "oldreader";
    d.name = "Default";
    d.title = "Default";
    d.description = "Old Reader dafault dashboard";
    s->db->writeDashboard(d);
    s->setDashboardInUse(d.id);

    s->db->cleanTabs();
    //s->db->cleanModules();

    // Create Modules and Cache structure
    if(busyType == Fetcher::Initiating) {
        s->db->cleanCache();
        s->db->cleanModules();
    }

    //qDebug() << "Fetching tabs...";
    fetchTabs();
}

void OldReaderFetcher::startJob(Job job)
{
    if (isRunning()) {
        qWarning() << "Job is running";
        return;
    }

    disconnect(this, SIGNAL(finished()), 0, 0);
    currentJob = job;

    if (parse()) {
        /*qWarning() << "Cookie expires!";
        Settings *s = Settings::instance();
        s->setCookie("");
        setBusy(false);

        // If credentials other than OldReader, prompting for re-auth
        if (s->getSigninType()>10) {
            emit error(403);
            return;
        }

        update();
        return;*/
    } else {
        qWarning() << "Error parsing Json!";
        emit error(600);
        setBusy(false);
        return;
    }

    switch (job) {
    case StoreTabs:
        connect(this, SIGNAL(finished()), this, SLOT(finishedTabs2()));
        break;
    case StoreFeeds:
        connect(this, SIGNAL(finished()), this, SLOT(finishedFeeds2()));
        break;
    case StoreStreamUpdate:
        connect(this, SIGNAL(finished()), this, SLOT(finishedStreamUpdate2()));
        break;
    case StoreStream:
        connect(this, SIGNAL(finished()), this, SLOT(finishedStream2()));
        break;
    case StoreStarredStream:
        connect(this, SIGNAL(finished()), this, SLOT(finishedStarredStream2()));
        break;
    case MarkSlow:
        connect(this, SIGNAL(finished()), this, SLOT(finishedMarkSlow()));
        break;
    default:
        qWarning() << "Unknown Job!";
        emit error(502);
        setBusy(false);
        return;
    }

    start(QThread::LowPriority);
}

void OldReaderFetcher::run()
{
    switch (currentJob) {
    case StoreTabs:
        storeTabs();
        break;
    case StoreFeeds:
        storeFeeds();
        break;
    case StoreStreamUpdate:
        storeStream();
        break;
    case StoreStream:
    case StoreStarredStream:
        storeStream();
        break;
    case MarkSlow:
        markSlowFeeds();
        break;
    default:
        qWarning() << "Unknown Job!";
        break;
    }
}

void OldReaderFetcher::storeTabs()
{
    Settings *s = Settings::instance();
    QString dashboardId = "oldreader";
    tabList.clear();

    // Adding Subscriptions folder
    DatabaseManager::Tab t;
    t.id = "subscriptions";
    t.dashboardId = dashboardId;
    t.title = tr("Subscriptions");
    s->db->writeTab(t);
    tabList.append(t.id);

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    if (jsonObj["tags"].isArray()) {
        QJsonArray::const_iterator i = jsonObj["tags"].toArray().constBegin();
        QJsonArray::const_iterator end = jsonObj["tags"].toArray().constEnd();
#else
    if (jsonObj["tags"].type()==QVariant::List) {
        QVariantList::const_iterator i = jsonObj["tags"].toList().constBegin();
        QVariantList::const_iterator end = jsonObj["tags"].toList().constEnd();
#endif
        while (i != end) {
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
            QJsonObject obj = (*i).toObject();
#else
            QVariantMap obj = (*i).toMap();
#endif
            // Checking if folder (label)
            QStringList id = obj["id"].toString().split('/');
            //qDebug() << id;
            if (id.at(2)=="label") {
                // Tab
                DatabaseManager::Tab t;
                t.id = obj["id"].toString();
                t.dashboardId = dashboardId;
                t.title = id.at(3);
                s->db->writeTab(t);
                tabList.append(t.id);
            }

            ++i;
        }

    }  else {
        qWarning() << "No \"tabs\" element found!";
    }
}

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
void OldReaderFetcher::getFolderFromCategories(const QJsonArray &categories, QString &tabId, QString &tabName)
{
    QJsonArray::const_iterator i = categories.constBegin();
    QJsonArray::const_iterator end = categories.constEnd();
#else
void OldReaderFetcher::getFolderFromCategories(const QVariantList &categories, QString &tabId, QString &tabName)
{
    QVariantList::const_iterator i = categories.constBegin();
    QVariantList::const_iterator end = categories.constEnd();
#endif
    while (i != end) {
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
        QJsonObject obj = (*i).toObject();
#else
        QVariantMap obj = (*i).toMap();
#endif
        QStringList id = obj["id"].toString().split('/');
        //qDebug() << id;
        if (id.at(2)=="label") {
            // Subscription can be only in one folder
            // Always true?
            tabId = obj["id"].toString();
            tabName = id.at(3);
            return;
        }

        ++i;
    }

    tabId = "";
    tabName = "";
}

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
void OldReaderFetcher::getFromCategories(const QJsonArray &categories, QVariantMap &result)
{
    QJsonArray::const_iterator i = categories.constBegin();
    QJsonArray::const_iterator end = categories.constEnd();
#else
void OldReaderFetcher::getFolderFromCategories(const QVariantList &categories, QVariantMap &result)
{
    QVariantList::const_iterator i = categories.constBegin();
    QVariantList::const_iterator end = categories.constEnd();
#endif
    bool read = false;
    bool starred = false;
    bool fresh = false;
    bool liked = false;
    while (i != end) {
        QStringList id = (*i).toString().split('/');
        //qDebug() << id;
        if (id.at(2)=="label") {
            result.insert("tabId",QVariant((*i).toString()));
            result.insert("tabName",QVariant(id.at(3)));
        } else if ((*i).toString() == "user/-/state/com.google/read") {
            read = true;
        } else if ((*i).toString() == "user/-/state/com.google/starred") {
            starred = true;
        } else if ((*i).toString() == "user/-/state/com.google/like") {
            liked = true;
        } else if ((*i).toString() == "user/-/state/com.google/fresh") {
            fresh = true;
        }

        ++i;
    }

    if (read)
        result.insert("read",QVariant(1));
    else
        result.insert("read",QVariant(0));
    if (starred)
        result.insert("starred",QVariant(1));
    else
        result.insert("starred",QVariant(0));
    if (liked)
        result.insert("liked",QVariant(1));
    else
        result.insert("liked",QVariant(0));
    if (fresh)
        result.insert("fresh",QVariant(1));
    else
        result.insert("fresh",QVariant(0));
}

void OldReaderFetcher::storeFeeds()
{
    //feedList.clear();
    feedUpdateList.clear();
    Settings *s = Settings::instance();

    bool subscriptionsFolderFeed = false;

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    if (jsonObj["subscriptions"].isArray()) {
        QJsonArray::const_iterator i = jsonObj["subscriptions"].toArray().constBegin();
        QJsonArray::const_iterator end = jsonObj["subscriptions"].toArray().constEnd();
#else
    if (jsonObj["subscriptions"].type()==QVariant::List) {
        QVariantList::const_iterator i = jsonObj["subscriptions"].toList().constBegin();
        QVariantList::const_iterator end = jsonObj["subscriptions"].toList().constEnd();
#endif
        while (i != end) {
            QString tabId, tabName;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

            // Checking tab (folder)
            QJsonObject obj = (*i).toObject();
            if (obj["categories"].isArray()) {
                getFolderFromCategories(obj["categories"].toArray(), tabId, tabName);
            }
#else
            QVariantMap obj = (*i).toMap();
            if (obj["categories"].type()==QVariant::List) {
                getFolderFromCategories(obj["categories"].toList(), tabId, tabName);
            }
#endif
            QStringList id = obj["id"].toString().split('/');
            if (tabId == "" && !id.isEmpty() && id.at(0) == "feed") {
                // Feed without label -> Subscriptions folder
                tabId = "subscriptions";
                subscriptionsFolderFeed = true;
            }

            if (!id.isEmpty() && id.at(0) == "feed") {
                // Stream
                DatabaseManager::Stream st;
                st.id = obj["id"].toString();
                st.title = obj["title"].toString().remove(QRegExp("<[^>]*>"));
                st.link = obj["htmlUrl"].toString();
                st.query = obj["url"].toString();
                st.content = "";
                st.type = "";
                st.unread = 0;
                st.read = 0;
                st.slow = 0;
                st.newestItemAddedAt = (int) obj["firstitemmsec"].toDouble();
                st.updateAt = (int) obj["firstitemmsec"].toDouble();
                st.lastUpdate = QDateTime::currentDateTimeUtc().toTime_t();
                if (obj["iconUrl"].toString() != "") {
                    st.icon = "http:"+obj["iconUrl"].toString();
                    // Downloading fav icon file
                    DatabaseManager::CacheItem item;
                    item.origUrl = st.icon;
                    item.finalUrl = st.icon;
                    item.type = "icon";
                    emit addDownload(item);
                }
                s->db->writeStream(st);

                // Module
                DatabaseManager::Module m;
                m.id = st.id;
                m.name = st.title;
                m.title = st.title;
                m.status = "";
                m.widgetId = "";
                m.pageId = "";
                m.tabId = tabId;
                m.streamList.append(st.id);
                s->db->writeModule(m);

                DatabaseManager::StreamModuleTab smt;
                smt.streamId = st.id;
                smt.moduleId = st.id;
                smt.tabId = tabId;

                //feedList.append(smt);
                feedUpdateList.append(smt);
            }

            ++i;
        }

    }  else {
        qWarning() << "No \"tabs\" element found!";
    }

    if (!subscriptionsFolderFeed) {
        // Removing Subscriptions folder
        s->db->removeTabById("subscriptions");
    }
}

void OldReaderFetcher::storeStream()
{
    Settings *s = Settings::instance();

    //qDebug() << jsonObj;

    double updated = 0;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    if (jsonObj["updated"].isDouble()) {
        updated = jsonObj["updated"].toDouble();
    } else {
        qWarning() << "No updated param in stream!";
    }
    if (jsonObj["items"].isArray()) {
        QJsonArray::const_iterator i = jsonObj["items"].toArray().constBegin();
        QJsonArray::const_iterator end = jsonObj["items"].toArray().constEnd();
#else
    if (jsonObj["updated"].type()==QVariant::Double) {
        updated = jsonObj["updated"].toDouble();
    } else {
        qWarning() << "No updated param in stream!";
    }
    if (jsonObj["items"].type()==QVariant::List) {
        QVariantList::const_iterator i = jsonObj["items"].toList().constBegin();
        QVariantList::const_iterator end = jsonObj["items"].toList().constEnd();
#endif
        //qDebug() << "Updated:" << updated;
        while (i != end) {
            QString feedId;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
            QJsonObject obj = (*i).toObject();
            if (obj["origin"].isObject()) {
                feedId = obj["origin"].toObject()["streamId"].toString();
            }
#else
            QVariantMap obj = (*i).toMap();
            if (obj["origin"].type() == QVariant::Map) {
                feedId = obj["origin"].toMap()["streamId"].toString();
            }
#endif
            //qDebug() << feedId;

            DatabaseManager::Entry e;
            e.id = obj["id"].toString();
            e.streamId = feedId;
            e.title = obj["title"].toString();
            e.author = obj["author"].toString();
            QVariantMap categories;

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
            if (obj["summary"].isObject())
                e.content = obj["summary"].toObject()["content"].toString();
            if (obj["canonical"].isArray() && !obj["canonical"].toArray().isEmpty() && obj["canonical"].toArray()[0].isObject())
                e.link = obj["canonical"].toArray()[0].toObject()["href"].toString();
            if (obj["categories"].isArray())
                getFromCategories(obj["categories"].toArray(), categories);
#else
            if (obj["summary"].type() == QVariant::Map)
                e.content = obj["summary"].toMap()["content"].toString();
            if (obj["canonical"].type() == QVariant::List && obj["canonical"].toList().isEmpty() && obj["canonical"].toList()[0].type() == QVariant::Map)
                e.link = obj["canonical"].toList()[0].toMap()["href"].toString();
            if (obj["categories"].type() == QVariant::List)
                getFromCategories(obj["categories"].toList(), categories);
#endif
            e.read = categories.value("read").toInt();
            e.saved = categories.value("starred").toInt();
            e.liked = categories.value("liked").toInt();
            e.freshOR = categories.value("fresh").toInt();
            e.cached = 0;
            e.publishedAt = obj["published"].toDouble();
            e.createdAt = obj["updated"].toDouble();
            e.fresh = 1;

            QString crawlTime = obj["crawlTimeMsec"].toString();
            crawlTime.chop(3); // converting Msec to sec
            e.crawlTime = crawlTime.toDouble();
            e.crawlTime = updated;
            QString timestamp = obj["timestampUsec"].toString();
            timestamp.chop(6); // converting Usec to sec

            //e.timestamp = timestamp.toDouble();

            /*qDebug() << ">>>>>>>>>>>>>>>";
            qDebug() << e.title << e.streamId;
            qDebug() << "crawlTime" << e.crawlTime;
            qDebug() << "timestampUsec" << e.timestamp;
            qDebug() << "publishedAt"<< e.publishedAt;
            qDebug() << "createdAt" << e.createdAt;
            qDebug() << "<<<<<<<<<<<<<<<";*/

            // Downloading image file
            // Checking if content contains image
            QRegExp rx("<img\\s[^>]*src\\s*=\\s*(\"[^\"]*\"|'[^']*')", Qt::CaseInsensitive);
            if (rx.indexIn(e.content)!=-1) {
                QString imgSrc = rx.cap(1); imgSrc = imgSrc.mid(1,imgSrc.length()-2);
                if (imgSrc!="") {
                    if (s->getCachingMode() == 2 || (s->getCachingMode() == 1 && s->dm->isWLANConnected())) {
                        if (!s->db->isCacheExistsByFinalUrl(Utils::hash(imgSrc))) {
                            DatabaseManager::CacheItem item;
                            item.origUrl = imgSrc;
                            item.finalUrl = imgSrc;
                            item.type = "entry-image";
                            emit addDownload(item);
                        }
                    }
                    e.image = imgSrc;
                }
            }

            s->db->writeEntry(e);

            ++i;
        }
    }

    QString continuation = jsonObj["continuation"].toString();
    lastContinuation = continuation;
    ++continuationCount;
}

void OldReaderFetcher::prepareFeedLists()
{
    // Removing all new feeds form feedUpdateList
    QList<DatabaseManager::StreamModuleTab>::iterator ui = feedUpdateList.begin();

    while (ui != feedUpdateList.end()) {
        bool newFeed = true;
        QList<DatabaseManager::StreamModuleTab>::iterator si = storedFeedList.begin();
        while (si != storedFeedList.end()) {
            if ((*si).streamId == (*ui).streamId) {
                newFeed = false;
                break;
            }
            ++si;
        }

        if (newFeed) {
            feedList.append((*ui)); // adding as new Feed
            ui = feedUpdateList.erase(ui);
        } else {
            ++ui;
        }

    }
}

void OldReaderFetcher::removeDeletedFeeds()
{
    // Removing all existing feeds form feedList
    QList<DatabaseManager::StreamModuleTab>::iterator si = storedFeedList.begin();

    while (si != storedFeedList.end()) {
        bool newFeed = true;
        //qDebug() << ">>>>>>>> si:" << (*si).streamId;
        QList<DatabaseManager::StreamModuleTab>::iterator ui = feedUpdateList.begin();
        while (ui != feedUpdateList.end()) {
            //qDebug() << "ui:" << (*ui).streamId;
            if ((*ui).streamId == (*si).streamId) {
                //qDebug() << "  matched!";
                newFeed = false;
                break;
            }
            ++ui;
        }

        if (newFeed) {
            //qDebug() << "Removing feed:" << (*si).streamId;
            Settings *s = Settings::instance();
            s->db->removeStreamsByStream((*si).streamId);
        }

        ++si;
    }
}

void OldReaderFetcher::uploadActions()
{
    if (!actionsList.isEmpty()) {
        emit uploading();
        //qDebug() << "Uploading actions...";
        setAction();
    }
}

void OldReaderFetcher::markSlowFeeds()
{
    // A feed is considered "slow" when it publishes
    // less than 5 articles in a month.

    Settings *s = Settings::instance();
    QStringList list = s->db->readStreamIds();
    QStringList::iterator it = list.begin();
    while (it != list.end()) {
        int n = s->db->countEntriesNewerThanByStream(*it, QDateTime::currentDateTime().addDays(-30));
        /*if (busyType == Fetcher::Updating)
            n = s->db->countEntriesNewerThanByStream(*it, QDateTime::fromTime_t(s->getLastUpdateDate()).addDays(-30));
        else
            n = s->db->countEntriesNewerThanByStream(*it, QDateTime::currentDateTime().addDays(-30));*/
        //qDebug() << "n:" << *it << n;
        if (n<5) {
            // Slow detected
            s->db->updateStreamSlowFlagById(*it, 1);
        }
        ++it;
    }
}