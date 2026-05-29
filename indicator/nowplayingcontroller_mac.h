/*
 * SPDX-FileCopyrightText: 2026 LM. Garret <lm@codingarret.dev>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QString>

class DevicesModel;
class MprisDbusInterface;
class NowPlayingBridge;

/**
 * Bridges the daemon's mprisremote D-Bus interface into the macOS Now Playing UI
 * (Control Center, lock screen, media keys via MPRemoteCommandCenter).
 *
 * Watches the indicator's DevicesModel for connected devices with the kdeconnect_mprisremote
 * plugin loaded, picks one as the active source, and feeds its state into a NowPlayingBridge
 * each time the daemon's propertiesChangedProxy signal fires. Remote command callbacks fired
 * by macOS are routed back to the active device's MprisDbusInterface (sendAction / setPosition).
 */
class NowPlayingController : public QObject
{
    Q_OBJECT
public:
    explicit NowPlayingController(DevicesModel *model, QObject *parent = nullptr);
    ~NowPlayingController() override;

private Q_SLOTS:
    void rebuildDeviceSet();
    void refreshActive();

    void onPlay();
    void onPause();
    void onTogglePlayPause();
    void onNext();
    void onPrevious();
    void onSeek(qint64 positionMs);

private:
    MprisDbusInterface *activeInterface() const;

    QPointer<DevicesModel> m_model;
    NowPlayingBridge *m_bridge;
    QHash<QString, MprisDbusInterface *> m_interfaces;
    QString m_activeDeviceId;

    // Last pushed state, to avoid spamming MPNowPlayingInfoCenter.
    QString m_lastTitle;
    QString m_lastArtist;
    QString m_lastAlbum;
    QString m_lastPlayerName;
    QString m_lastArtworkPath;
    int m_lastDurationMs = -1;
    int m_lastPositionMs = -1;
    bool m_lastIsPlaying = false;
    bool m_haveLastState = false;
};
