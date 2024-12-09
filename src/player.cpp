#include "player.hpp"

#include <QUrl>

Player::Player(const QStringList &playlist, QObject *parent)
    : QObject{parent}
    , m_audioOutput(new QAudioOutput(this))
    , m_mediaPlayer(new QMediaPlayer(this))
    , m_playlist(playlist)
    , m_currentMusicIndex(-1)
{
    m_mediaPlayer->setAudioOutput(m_audioOutput);

    connect(m_mediaPlayer, &QMediaPlayer::durationChanged, this, &Player::onDurationChanged);
    connect(m_mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &Player::mediaStatusChanged);
    connect(m_mediaPlayer, &QMediaPlayer::positionChanged, this, &Player::positionChangedSlot);
}

bool Player::hasNext()
{
    return m_currentMusicIndex < m_playlist.size();
}

void Player::setCurrent(const QString &musicFile)
{
    m_mediaPlayer->setSource(QUrl::fromLocalFile(musicFile));
    if (m_playlist.contains(musicFile)) {
        m_currentMusicIndex = m_playlist.indexOf(musicFile);
    }
}

void Player::setCurrent(qint64 index)
{
    if (index < 0 or index >= m_playlist.size()) {
        emit error(tr("There's no such music at index: %1.").arg(index));
        return;
    }

    m_currentMusicIndex = index;
    m_mediaPlayer->setSource(QUrl::fromLocalFile(m_playlist[index]));
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
    return true;
}

bool Player::playPrevious()
{
    if (m_currentMusicIndex == 0) {
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
        qDebug() << "There's no next music to play.";
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
    m_mediaPlayer->setPosition(position);
}

void Player::clearSource()
{
    m_mediaPlayer->setSource(QUrl());
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
    emit durationChanged(m_mediaPlayer->duration() / 1'000); /* Emit just seconds */
}

void Player::mediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia) {
        emit finished();
    }
}

void Player::positionChangedSlot(qint64 position)
{
    emit positionChanged(position / 1'000); /* Emit just seconds */
}
