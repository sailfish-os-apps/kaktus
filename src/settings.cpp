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

#include "settings.h"

Settings* Settings::inst = 0;

Settings::Settings(QObject *parent) : QObject(parent), settings()
{
}

Settings* Settings::instance()
{
    if (Settings::inst == NULL) {
        Settings::inst = new Settings();
    }

    return Settings::inst;
}

void Settings::setOfflineMode(bool value)
{
    settings.setValue("offlinemode", value);
    emit settingsChanged();
}

bool Settings::getOfflineMode()
{
    return settings.value("offlinemode", false).toBool();
}

void Settings::setSignedIn(bool value)
{
    settings.setValue("signedin", value);
}

bool Settings::getSignedIn()
{
    return settings.value("signedin", false).toBool();
}

void Settings::setAutoDownloadOnUpdate(bool value)
{
    settings.setValue("autodownloadonupdate", value);
}

bool Settings::getAutoDownloadOnUpdate()
{
    return settings.value("autodownloadonupdate", true).toBool();
}

void Settings::setNetvibesUsername(const QString &value)
{
    settings.setValue("username", value);
}

QString Settings::getNetvibesUsername()
{
    return settings.value("username", "").toString();
}

void Settings::setNetvibesPassword(const QString &value)
{
    settings.setValue("password", value);
}

QString Settings::getNetvibesPassword()
{
    return settings.value("password", "").toString();
}

void Settings::setNetvibesDefaultDashboard(const QString &value)
{
    settings.setValue("dafaultdashboard", value);
}

QString Settings::getNetvibesDefaultDashboard()
{
    return settings.value("dafaultdashboard", "").toString();
}

void Settings::setNetvibesFeedLimit(int value)
{
    settings.setValue("limit", value);
}

int Settings::getNetvibesFeedLimit()
{
    return settings.value("limit", 15).toInt();
}

void Settings::setNetvibesFeedUpdateAtOnce(int value)
{
    settings.setValue("feedupdateatonce", value);
}

int Settings::getNetvibesFeedUpdateAtOnce()
{
    return settings.value("feedupdateatonce", 20).toInt();
}

void  Settings::setDmCacheRetencyFeedLimit(int value)
{
    settings.setValue("dm_limit", value);
}

int  Settings::getDmCacheRetencyFeedLimit()
{
    return settings.value("dm_limit", 20).toInt();
}

void Settings::setSettingsDir(const QString &value)
{
    settings.setValue("settingsdir", value);
}

QString Settings::getSettingsDir()
{
    QString value = QDir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation))
            .filePath(QCoreApplication::applicationName());
    value = settings.value("settingsdir", value).toString();

    if (!QDir(value).exists()) {
        if (!QDir::root().mkpath(value)) {
            qWarning() << "Unable to create settings dir!";
            /// @todo handle 'Unable to create settings dir'
        }
    }

    return value;
}

void Settings::setDmConnections(int value)
{
    settings.setValue("connections", value);
}

int Settings::getDmConnections()
{
    return settings.value("connections", 20).toInt();
}

void Settings::setDmTimeOut(int value)
{
    settings.setValue("timeout", value);
}

int Settings::getDmTimeOut()
{
    return settings.value("timeout", 20000).toInt();
}

void Settings::setDmMaxSize(int value)
{
    settings.setValue("maxsize", value);
}

int Settings::getDmMaxSize()
{
    return settings.value("maxsize", 1000000).toInt();
}

void Settings::setDmCacheDir(const QString &value)
{
    settings.setValue("cachedir", value);
}

QString Settings::getDmCacheDir()
{
    QString value = QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation))
            .filePath("cached_files");
    value = settings.value("cachedir", value).toString();

    if (!QDir(value).exists()) {
        if (!QDir::root().mkpath(value)) {
            qWarning() << "Unable to create cache dir!";
            /// @todo handle 'Unable to create cache dir'
        }
    }

    return value;
}

void Settings::setDmUserAgent(const QString &value)
{
    settings.setValue("useragent", value);
}

QString Settings::getDmUserAgent()
{
    QString value = "Mozilla/5.0 (Linux; Android 4.2.1; Nexus 4 Build/JOP40D) AppleWebKit/535.19 (KHTML, like Gecko) Chrome/18.0.1025.166 Mobile Safari/535.19";
    return settings.value("cachedir", value).toString();
}

void Settings::setDmMaxCacheRetency(int value)
{
    settings.setValue("cacheretency", value);
}

int Settings::getDmMaxCacheRetency()
{
    // 1 day = 86400
    // 1 week = 604800
    return settings.value("cacheretency", 604800).toInt();
}

void Settings::setCsPost(int value)
{
    settings.setValue("port", value);
}

int Settings::getCsPort()
{
    return settings.value("port", 9999).toInt();
}