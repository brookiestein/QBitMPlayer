#include "player.hpp"

#include <QAudioDevice>
#include <QDebug>
#include <QUrl>

Player::Player(const QStringList &playlist, QObject *parent)
    : QObject{parent}
    , m_audioOutput(new QAudioOutput(this))
    , m_mediaPlayer(new QMediaPlayer(this))
    , m_playlist(playlist)
    , m_currentMusicIndex(-1)
    , m_autoplay(false)
#ifdef USE_NOTIFICATIONS
    , m_currentChanged {false}
#endif
{
    m_mediaPlayer->setAudioOutput(m_audioOutput);

    connect(m_mediaPlayer, &QMediaPlayer::durationChanged, this, &Player::onDurationChanged);
    connect(m_mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &Player::mediaStatusChanged);
    connect(m_mediaPlayer, &QMediaPlayer::positionChanged, this, &Player::positionChangedSlot);
    connect(m_mediaPlayer, &QMediaPlayer::hasVideoChanged, this, [this] (bool videoAvailable) {
        if (videoAvailable)
            emit mediaType(MEDIA_TYPE::VIDEO);
        else
            emit mediaType(MEDIA_TYPE::AUDIO);
    });
}

void Player::setPlaylistName(const QString &playlistName)
{
    m_playlistName = playlistName;
}

bool Player::hasNext()
{
    return m_currentMusicIndex < m_playlist.size();
}

void Player::setCurrent(const QString &musicFile)
{
    m_mediaPlayer->setSource(QUrl::fromLocalFile(musicFile));
    m_currentMusicFilename = musicFile;
    if (m_playlist.contains(musicFile)) {
        m_currentMusicIndex = m_playlist.indexOf(musicFile);
    }

#ifdef USE_NOTIFICATIONS
    m_currentChanged = true;
#endif
}

void Player::setCurrent(qint64 index)
{
    if (index < 0 or index >= m_playlist.size()) {
        emit error(tr("There's no such music at index: %1.").arg(index));
        return;
    }

    m_currentMusicIndex = index;
    m_mediaPlayer->setSource(QUrl::fromLocalFile(m_playlist[index]));
    m_currentMusicFilename = m_playlist[index];

#ifdef USE_NOTIFICATIONS
    m_currentChanged = true;
#endif
}

void Player::setAutoPlay(bool autoPlay)
{
    m_autoplay = autoPlay;
}

void Player::setAudioDevice(QAudioDevice device)
{
    m_mediaPlayer->audioOutput()->setDevice(device);
}

void Player::setVideoOutput(QVideoWidget *videoOutput)
{
    m_mediaPlayer->setVideoOutput(videoOutput);
}

QString Player::playlistName() const
{
    return m_playlistName;
}

QString Player::currentMusicFilename() const
{
    return m_currentMusicFilename;
}

qint64 Player::currentPosition() const
{
    return m_mediaPlayer->position();
}

qint64 Player::currentIndex() const
{
    return m_currentMusicIndex;
}

qint64 Player::currentDuration() const
{
    return m_currentMusicDuration;
}

bool Player::isPlaying() const
{
    return m_mediaPlayer->isPlaying();
}

void Player::setPlayList(const QStringList &playlist)
{
    m_playlist = playlist;
}

void Player::setVolume(float volume)
{
    m_audioOutput->setVolume(volume);
}

bool Player::pause()
{
    if (not m_mediaPlayer->isPlaying()) {
        emit warning(tr("There isn't any music playing."));
        return false;
    }

    m_mediaPlayer->pause();
    return true;
}

bool Player::play()
{
    if (m_mediaPlayer->isPlaying()) {
        emit warning(tr("There's already a music playing."));
        return false;
    }

    if (m_mediaPlayer->source().isEmpty()) {
        emit warning(tr("You must first open a music file."));
        return false;
    }

    m_mediaPlayer->play();
    m_currentMusicDuration = m_mediaPlayer->duration();

#ifdef USE_NOTIFICATIONS
    if (m_currentChanged) {
        m_currentChanged = false;
        emit nowPlaying(m_currentMusicFilename);
    }
#endif

    return true;
}

bool Player::playPrevious()
{
    if (m_currentMusicIndex <= 0) {
        emit warning(tr("There's no previous music to play."));
        return false;
    }

    --m_currentMusicIndex;
    auto music = m_playlist[m_currentMusicIndex];
    setCurrent(music);
    play();
    return true;
}

bool Player::playNext()
{
    ++m_currentMusicIndex;
    if (not hasNext()) {
        --m_currentMusicIndex;
        return false;
    }

    auto music = m_playlist[m_currentMusicIndex];
    setCurrent(music);
    play();
    return true;
}

void Player::stop()
{
    m_mediaPlayer->stop();
}

void Player::seek(qint64 position)
{
    if (position < 0 or position > m_mediaPlayer->duration())
        return;
    m_mediaPlayer->setPosition(position);
}

void Player::clearSource()
{
    m_mediaPlayer->setSource(QUrl());
    m_playlistName.clear();
}

void Player::errorOcurred(QMediaPlayer::Error err, const QString &errorString)
{
    if (err == QMediaPlayer::NoError) {
        return;
    }

    emit error(errorString);
}

void Player::onDurationChanged(qint64 duration)
{
    emit durationChanged(duration / 1'000); /* Emit just seconds */
}

void Player::mediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia) {
        if (m_autoplay) {
            playNext();
        } else {
            emit finished();
        }
    }
}

void Player::positionChangedSlot(qint64 position)
{
    emit positionChanged(position / 1'000); /* Emit just seconds */
}
