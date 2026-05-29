/*
 * SPDX-FileCopyrightText: 2026 LM. Garret <lm@codingarret.dev>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>
#include <QString>

class NowPlayingBridgePrivate;

class NowPlayingBridge : public QObject
{
    Q_OBJECT
public:
    explicit NowPlayingBridge(QObject *parent = nullptr);
    ~NowPlayingBridge() override;

    void updateInfo(const QString &title,
                    const QString &artist,
                    const QString &album,
                    const QString &playerName,
                    int durationMs,
                    int positionMs,
                    bool isPlaying,
                    const QString &artworkPath);
    void clear();

Q_SIGNALS:
    void playRequested();
    void pauseRequested();
    void togglePlayPauseRequested();
    void nextRequested();
    void previousRequested();
    void seekRequested(qint64 positionMs);

private:
    NowPlayingBridgePrivate *d;
};
