#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <QAudioOutput>
#include <QMediaPlayer>
#include <QObject>
#ifdef ENABLE_VIDEO_PLAYER
    #include <QVideoWidget>
#endif

class Player : public QObject
{
    Q_OBJECT

    bool hasNext();
    void setCurrent(const QString &musicFile);

public:
    explicit Player(const QStringList &playlist = QStringList(), QObject *parent = nullptr);
    void setPlaylistName(const QString &playlistName);
    void setPlayList(const QStringList &playlist);
    void setCurrent(qint64 index);
    /* Useful when in the command line. */
    void setAutoPlay(bool autoPlay);
    void setAudioDevice(QAudioDevice device);
#ifdef ENABLE_VIDEO_PLAYER
    void setVideoOutput(QVideoWidget *videoOutput);
#endif
    QString playlistName() const;
    QString currentMusicFilename() const;
    qint64 currentPosition() const;
    qint64 currentIndex() const;
    qint64 currentDuration() const;
    bool isPlaying() const;
    enum class MEDIA_TYPE { AUDIO = 0, VIDEO };

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
    void mediaType(MEDIA_TYPE type);
    void error(const QString &message);
    void warning(const QString &message);
    void durationChanged(qint64 duration);
    void positionChanged(qint64 position);
    void finished();
    void nowPlaying(const QString &filename);

private:
    QString m_playlistName;
    QStringList m_playlist;
    qint64 m_currentMusicIndex;
    QString m_currentMusicFilename;
    qint64 m_currentMusicDuration;
    QAudioOutput *m_audioOutput;
    QMediaPlayer *m_mediaPlayer;
    bool m_autoplay;
    bool m_currentChanged;
};

#endif // PLAYER_HPP
