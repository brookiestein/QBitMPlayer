#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <QAudioOutput>
#include <QMediaPlayer>
#include <QObject>

class Player : public QObject
{
    Q_OBJECT

    bool hasNext();
    void setCurrent(const QString &musicFile);

public:
    explicit Player(const QStringList &playlist = QStringList(), QObject *parent = nullptr);
    void setPlayList(const QStringList &playlist);
    void setCurrent(qint64 index);
    /* Useful when in the command line. */
    void setAutoPlay(bool autoPlay);
    const QString &currentMusicFilename() const;
    qint64 currentPosition() const;
    qint64 currentIndex() const;
    bool isPlaying() const;

public slots:
    void setVolume(float volume);
    bool pause();
    bool play();
    bool playPrevious();
    bool playNext();
    void stop();
    void seek(qint64 position);
    void clearSource();

private slots:
    void errorOcurred(QMediaPlayer::Error err, const QString &errorString);
    void onDurationChanged(qint64 duration);
    void mediaStatusChanged(QMediaPlayer::MediaStatus status);
    void positionChangedSlot(qint64 position);

signals:
    void error(const QString &message);
    void warning(const QString &message);
    void durationChanged(qint64 duration);
    void positionChanged(qint64 position);
    void finished();
#ifdef USE_NOTIFICATIONS
    void nowPlaying(const QString &filename);
#endif // USE_NOTIFICATIONS

private:
    QStringList m_playlist;
    qint64 m_currentMusicIndex;
    QString m_currentMusicFilename;
    QAudioOutput *m_audioOutput;
    QMediaPlayer *m_mediaPlayer;
    bool m_autoplay;
#ifdef USE_NOTIFICATIONS
    bool m_currentChanged;
#endif // USE_NOTIFICATIONS
};

#endif // PLAYER_HPP
