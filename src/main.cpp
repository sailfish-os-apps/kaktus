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

#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif

#include <QObject>
#include <QtDebug>
#include <QGuiApplication>
#include <QScopedPointer>
#include <sailfishapp.h>

#include "databasemanager.h"
#include "downloadmanager.h"
#include "cacheserver.h"
#include "netvibesfetcher.h"
#include "utils.h"

static const char *APP_NAME = "Kaktus";
static const char *VERSION = "0.0.1";
static const char *AUTHOR = "Michał Kościesza <michal@mkiol.net>";
static const char *PAGE = "https://github/mkiol/kaktus";

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    QScopedPointer<QQuickView> view(SailfishApp::createView());

    view->rootContext()->setContextProperty("APP_NAME", APP_NAME);
    view->rootContext()->setContextProperty("VERSION", VERSION);
    view->rootContext()->setContextProperty("AUTHOR", AUTHOR);
    view->rootContext()->setContextProperty("PAGE", PAGE);

    app->setOrganizationName("mkiol");

    DatabaseManager db;
    DownloadManager dm(&db);
    CacheServer cache(&db);
    NetvibesFetcher fetcher(&db, &dm);
    Utils utils(&db, view.data());

    QObject::connect(&fetcher, SIGNAL(ready()), &utils, SLOT(updateModels()));
    //QObject::connect(view->engine(), SIGNAL(quit()), QCoreApplication::instance(), SLOT(quit()));

    view->rootContext()->setContextProperty("db", &db);
    view->rootContext()->setContextProperty("fetcher", &fetcher);
    view->rootContext()->setContextProperty("utils", &utils);
    view->rootContext()->setContextProperty("dm", &dm);
    view->rootContext()->setContextProperty("cache", &cache);
    view->rootContext()->setContextProperty("settings", Settings::instance());

    view->setSource(SailfishApp::pathTo("qml/main.qml"));
    view->show();

    return app->exec();
}