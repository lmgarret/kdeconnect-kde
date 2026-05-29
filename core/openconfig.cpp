/**
 * SPDX-FileCopyrightText: 2022 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "openconfig.h"

#include <QCoreApplication>
#include <QDebug>
#include <QProcess>

#ifdef HAVE_KIO
#include <KIO/CommandLauncherJob>
#endif

void OpenConfig::setXdgActivationToken(const QString &token)
{
    m_currentToken = token;
}

void OpenConfig::openConfiguration(const QString &deviceId, const QString &pluginId)
{
    QStringList args;

    QString argument;

    if (!deviceId.isEmpty()) {
        args << QStringLiteral("--device");
        args << deviceId;

        if (!pluginId.isEmpty()) {
            args << QStringLiteral("--plugin-config");
            args << pluginId;
        }
    }

#if defined(Q_OS_WIN)
    QProcess::startDetached(QStringLiteral("kdeconnect-app.exe"), args);
#elif defined(Q_OS_MAC)
    QProcess::startDetached(QCoreApplication::applicationDirPath() + QLatin1String("/kdeconnect-app"), args);
#elif defined(HAVE_KIO)
    auto job = new KIO::CommandLauncherJob(QStringLiteral("kdeconnect-app"), args);
    job->setDesktopName(QStringLiteral("org.kde.kdeconnect.app"));
    job->setStartupId(m_currentToken.toUtf8());
    job->start();

    m_currentToken = QString();
#else
    QProcess::startDetached(QStringLiteral("kdeconnect-app"), args);
#endif
}

#include "moc_openconfig.cpp"
