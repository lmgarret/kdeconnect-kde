/*
 * SPDX-FileCopyrightText: 2026 LM. Garret <lm@codingarret.dev>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "nowplayingcontroller.h"

#include <QSet>

#include "dbusinterfaces/dbusinterfaces.h"
#include "models/devicesmodel.h"
#include "nowplayingbridge.h"

namespace
{
constexpr int kPositionDriftToleranceMs = 1500;
}

NowPlayingController::NowPlayingController(DevicesModel *model, QObject *parent)
    : QObject(parent)
    , m_model(model)
    , m_bridge(new NowPlayingBridge(this))
{
    connect(m_bridge, &NowPlayingBridge::playRequested, this, &NowPlayingController::onPlay);
    connect(m_bridge, &NowPlayingBridge::pauseRequested, this, &NowPlayingController::onPause);
    connect(m_bridge, &NowPlayingBridge::togglePlayPauseRequested, this, &NowPlayingController::onTogglePlayPause);
    connect(m_bridge, &NowPlayingBridge::nextRequested, this, &NowPlayingController::onNext);
    connect(m_bridge, &NowPlayingBridge::previousRequested, this, &NowPlayingController::onPrevious);
    connect(m_bridge, &NowPlayingBridge::seekRequested, this, &NowPlayingController::onSeek);

    connect(m_model, &DevicesModel::rowsInserted, this, &NowPlayingController::rebuildDeviceSet);
    connect(m_model, &DevicesModel::rowsRemoved, this, &NowPlayingController::rebuildDeviceSet);
    connect(m_model, &DevicesModel::rowsChanged, this, &NowPlayingController::rebuildDeviceSet);

    rebuildDeviceSet();
}

NowPlayingController::~NowPlayingController()
{
    qDeleteAll(m_interfaces);
    m_interfaces.clear();
}

void NowPlayingController::rebuildDeviceSet()
{
    if (!m_model)
        return;

    QSet<QString> seen;
    for (int i = 0, count = m_model->rowCount(); i < count; ++i) {
        QObject *obj = m_model->data(m_model->index(i, 0), DevicesModel::DeviceRole).value<QObject *>();
        auto *device = qobject_cast<DeviceDbusInterface *>(obj);
        if (!device)
            continue;
        const QString id = device->id();
        seen.insert(id);
        if (!m_interfaces.contains(id)) {
            auto *iface = new MprisDbusInterface(id, this);
            // The wrapper relays the daemon's propertiesChanged signal (in
            // dbusinterfaces.cpp) to propertiesChangedProxy. That gives us
            // signal-driven Now Playing updates whenever the phone pushes new
            // state to the daemon — no polling required.
            connect(iface, &MprisDbusInterface::propertiesChangedProxy, this, &NowPlayingController::refreshActive);
            m_interfaces.insert(id, iface);
        }
    }

    for (auto it = m_interfaces.begin(); it != m_interfaces.end();) {
        if (!seen.contains(it.key())) {
            delete it.value();
            it = m_interfaces.erase(it);
        } else {
            ++it;
        }
    }

    if (!seen.contains(m_activeDeviceId)) {
        m_activeDeviceId.clear();
    }

    refreshActive();
}

void NowPlayingController::refreshActive()
{
    // Pick the active device: prefer the current one if still has a player,
    // otherwise the first one with a non-empty playerList.
    auto candidate = [this]() -> QString {
        if (!m_activeDeviceId.isEmpty()) {
            auto *iface = m_interfaces.value(m_activeDeviceId);
            if (iface && !iface->playerList().isEmpty())
                return m_activeDeviceId;
        }
        for (auto it = m_interfaces.constBegin(); it != m_interfaces.constEnd(); ++it) {
            if (!it.value()->playerList().isEmpty())
                return it.key();
        }
        return QString();
    }();

    if (candidate != m_activeDeviceId) {
        m_activeDeviceId = candidate;
        m_haveLastState = false;
    }

    if (m_activeDeviceId.isEmpty()) {
        if (m_haveLastState) {
            m_bridge->clear();
            m_haveLastState = false;
        }
        return;
    }

    MprisDbusInterface *iface = m_interfaces.value(m_activeDeviceId);
    if (!iface)
        return;

    const QString title = iface->title();
    const QString artist = iface->artist();
    const QString album = iface->album();
    const QString playerName = iface->player();
    const int duration = iface->length();
    const int position = iface->position();
    const bool isPlaying = iface->isPlaying();
    const QUrl artworkUrl(iface->localAlbumArtUrl());
    const QString artworkPath = artworkUrl.isLocalFile() ? artworkUrl.toLocalFile() : QString();

    // Skip transient empty-metadata states (e.g. between rebuildDeviceSet and
    // the first packet arriving).
    if (!m_haveLastState && title.isEmpty() && artist.isEmpty())
        return;

    const bool metadataChanged = !m_haveLastState //
        || title != m_lastTitle //
        || artist != m_lastArtist //
        || album != m_lastAlbum //
        || playerName != m_lastPlayerName //
        || artworkPath != m_lastArtworkPath //
        || duration != m_lastDurationMs //
        || isPlaying != m_lastIsPlaying //
        || qAbs(position - m_lastPositionMs) > kPositionDriftToleranceMs;

    if (metadataChanged) {
        m_bridge->updateInfo(title, artist, album, playerName, duration, position, isPlaying, artworkPath);
        m_lastTitle = title;
        m_lastArtist = artist;
        m_lastAlbum = album;
        m_lastPlayerName = playerName;
        m_lastArtworkPath = artworkPath;
        m_lastDurationMs = duration;
        m_lastPositionMs = position;
        m_lastIsPlaying = isPlaying;
        m_haveLastState = true;
    }
}

MprisDbusInterface *NowPlayingController::activeInterface() const
{
    if (m_activeDeviceId.isEmpty())
        return nullptr;
    return m_interfaces.value(m_activeDeviceId);
}

void NowPlayingController::onPlay()
{
    if (auto *iface = activeInterface()) {
        if (!iface->isPlaying())
            iface->sendAction(QStringLiteral("PlayPause"));
    }
}

void NowPlayingController::onPause()
{
    if (auto *iface = activeInterface()) {
        if (iface->isPlaying())
            iface->sendAction(QStringLiteral("PlayPause"));
    }
}

void NowPlayingController::onTogglePlayPause()
{
    if (auto *iface = activeInterface())
        iface->sendAction(QStringLiteral("PlayPause"));
}

void NowPlayingController::onNext()
{
    if (auto *iface = activeInterface())
        iface->sendAction(QStringLiteral("Next"));
}

void NowPlayingController::onPrevious()
{
    if (auto *iface = activeInterface())
        iface->sendAction(QStringLiteral("Previous"));
}

void NowPlayingController::onSeek(qint64 positionMs)
{
    if (auto *iface = activeInterface())
        iface->setPosition(static_cast<int>(positionMs));
}

#include "moc_nowplayingcontroller.cpp"
