/*
 * SPDX-FileCopyrightText: 2026 LM. Garret <lm@codingarret.dev>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "nowplayingbridge.h"

#import <AppKit/AppKit.h>
#import <MediaPlayer/MediaPlayer.h>

class NowPlayingBridgePrivate
{
public:
    bool commandsInstalled = false;
};

NowPlayingBridge::NowPlayingBridge(QObject *parent)
    : QObject(parent)
    , d(new NowPlayingBridgePrivate)
{
    MPRemoteCommandCenter *cc = [MPRemoteCommandCenter sharedCommandCenter];

    [cc.playCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *event) {
        Q_UNUSED(event);
        Q_EMIT this->playRequested();
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    [cc.pauseCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *event) {
        Q_UNUSED(event);
        Q_EMIT this->pauseRequested();
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    [cc.togglePlayPauseCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *event) {
        Q_UNUSED(event);
        Q_EMIT this->togglePlayPauseRequested();
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    [cc.nextTrackCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *event) {
        Q_UNUSED(event);
        Q_EMIT this->nextRequested();
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    [cc.previousTrackCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *event) {
        Q_UNUSED(event);
        Q_EMIT this->previousRequested();
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    [cc.changePlaybackPositionCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *event) {
        MPChangePlaybackPositionCommandEvent *ev = (MPChangePlaybackPositionCommandEvent *)event;
        Q_EMIT this->seekRequested(static_cast<qint64>(ev.positionTime * 1000.0));
        return MPRemoteCommandHandlerStatusSuccess;
    }];

    cc.playCommand.enabled = YES;
    cc.pauseCommand.enabled = YES;
    cc.togglePlayPauseCommand.enabled = YES;
    cc.nextTrackCommand.enabled = YES;
    cc.previousTrackCommand.enabled = YES;
    cc.changePlaybackPositionCommand.enabled = YES;

    d->commandsInstalled = true;
}

NowPlayingBridge::~NowPlayingBridge()
{
    clear();
    if (d->commandsInstalled) {
        MPRemoteCommandCenter *cc = [MPRemoteCommandCenter sharedCommandCenter];
        [cc.playCommand removeTarget:nil];
        [cc.pauseCommand removeTarget:nil];
        [cc.togglePlayPauseCommand removeTarget:nil];
        [cc.nextTrackCommand removeTarget:nil];
        [cc.previousTrackCommand removeTarget:nil];
        [cc.changePlaybackPositionCommand removeTarget:nil];
    }
    delete d;
}

void NowPlayingBridge::updateInfo(const QString &title,
                                  const QString &artist,
                                  const QString &album,
                                  const QString &playerName,
                                  int durationMs,
                                  int positionMs,
                                  bool isPlaying,
                                  const QString &artworkPath)
{
    // Compose the album line so the source app (e.g. "Deezer", "YouTube Music") is
    // visible alongside the album when both are known, and stands alone otherwise.
    QString albumLine;
    if (!album.isEmpty() && !playerName.isEmpty()) {
        albumLine = album + QStringLiteral(" • ") + playerName;
    } else if (!album.isEmpty()) {
        albumLine = album;
    } else {
        albumLine = playerName;
    }

    NSMutableDictionary *info = [NSMutableDictionary dictionary];
    info[MPMediaItemPropertyTitle] = title.toNSString();
    info[MPMediaItemPropertyArtist] = artist.toNSString();
    info[MPMediaItemPropertyAlbumTitle] = albumLine.toNSString();
    info[MPMediaItemPropertyPlaybackDuration] = @(durationMs / 1000.0);
    info[MPNowPlayingInfoPropertyElapsedPlaybackTime] = @(positionMs / 1000.0);
    info[MPNowPlayingInfoPropertyPlaybackRate] = @(isPlaying ? 1.0 : 0.0);
    info[MPNowPlayingInfoPropertyMediaType] = @(MPNowPlayingInfoMediaTypeAudio);

    if (!artworkPath.isEmpty()) {
        NSImage *image = [[NSImage alloc] initWithContentsOfFile:artworkPath.toNSString()];
        if (image && image.isValid) {
            MPMediaItemArtwork *artwork = [[MPMediaItemArtwork alloc] initWithBoundsSize:image.size
                                                                          requestHandler:^NSImage *(CGSize size) {
                                                                              Q_UNUSED(size);
                                                                              return image;
                                                                          }];
            info[MPMediaItemPropertyArtwork] = artwork;
        }
    }

    MPNowPlayingInfoCenter *center = [MPNowPlayingInfoCenter defaultCenter];
    center.nowPlayingInfo = info;
    center.playbackState = isPlaying ? MPNowPlayingPlaybackStatePlaying : MPNowPlayingPlaybackStatePaused;
}

void NowPlayingBridge::clear()
{
    MPNowPlayingInfoCenter *center = [MPNowPlayingInfoCenter defaultCenter];
    center.nowPlayingInfo = nil;
    center.playbackState = MPNowPlayingPlaybackStateStopped;
}
