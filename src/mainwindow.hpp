#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QAction>
#include <QCloseEvent>
#include <QDir>
#include <QMainWindow>
#include <QMediaDevices>
#include <QMouseEvent>
#include <QSettings>
#include <QShortcut>
#include <QShowEvent>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QTreeWidgetItem>
#ifdef ENABLE_VIDEO_PLAYER
    #include <QVideoWidget>
#endif // ENABLE_VIDEO_PLAYER
#ifdef ENABLE_IPC
    #include <QDBusConnection>
#endif // ENABLE_IPC

#include "config.hpp"
#include "player.hpp"
#ifdef ENABLE_VIDEO_PLAYER
    #include "videoplayer.hpp"
#endif

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
#ifdef ENABLE_IPC
    Q_CLASSINFO("D-Bus Interface", SERVICE_NAME)

    QDBusConnection m_dbusConnection;
#endif

    void clearAudioOutputs();
    void setAudioOutputs();
    void resetControls();
    QString musicName(const QString &filename);

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void loadPlaylist(const QString &playlistName);
    void setVolumeIcon();

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    Ui::MainWindow *m_ui;
    QMediaDevices m_mediaDevices;
#ifdef ENABLE_VIDEO_PLAYER
    VideoPlayer m_videoPlayer;
#endif
    QSystemTrayIcon m_systray;
    QAction *m_clearRecentSongs;
    QAction *m_clearRecentPlaylists;
    QAction *m_showHideSystrayAction;
    QAction *m_playPauseSystrayAction;
    QAction *m_stopSystrayAction;
    QAction *m_showHideControlsTreeWidgetAction;
    QAction *m_addSongToPlaylist;
    QAction *m_removeSongAction;

    QSettings *m_settings;
    QSettings *m_playlistSettings;

    Player m_player;
    QStringList m_playlistInitState;
    QStringList m_playlist;
    QString m_currentPlaylistName;
    bool m_canModifySlider;

    qint8 m_hours;
    qint8 m_minutes;
    qint8 m_seconds;

    // Pausing seems not to actually pause, let's save the current position before pausing
    // in order to seek the player there when the user click continue.
    qint64 m_currentPosition;

    enum class AUTOREPEAT { NONE = 0, ONE, ALL };
    AUTOREPEAT m_autorepeat = AUTOREPEAT::NONE;

    QShortcut *m_quitShortcut; /* Ctrl + Q */
    QShortcut *m_openFilesShortcut; /* Ctrl + O */
    QShortcut *m_openDirectoryShortcut; /* Ctrl + D */
    QShortcut *m_openPlaylistShortcut; /* Ctrl + Shift + O */
    QShortcut *m_closePlaylistShortcut; /* Ctrl + C */
    QShortcut *m_savePlaylistShortcut; /* Ctrl + S */
    QShortcut *m_removePlaylistShortcut; /* Ctrl + R */
    QShortcut *m_settingsShortcut; /* Ctrl + Shift + S */
    QShortcut *m_playShortcut; /* P */
    QShortcut *m_playShortcut2; /* Space */
    QShortcut *m_stopShortcut; /* Escape */
    QShortcut *m_previousShortcut; /* Left arrow */
    QShortcut *m_nextShortcut; /* Right arrow */
    QShortcut *m_autorepeatShortcut; /* R */
    QShortcut *m_increaseVolumeBy5Shortcut; /* Up arrow */
    QShortcut *m_increaseVolumeBy10Shortcut; /* Shift + Up arrow */
    QShortcut *m_decreaseVolumeBy5Shortcut; /* Down arrow */
    QShortcut *m_decreaseVolumeBy10Shortcut; /* Shift + Down arrow */

    bool m_controlsHidden;

private slots:
    void onQuit();
    void error(const QString &message);
    void warning(const QString &message);
    void clearRecents();
    void onHideShowControls([[maybe_unused]] bool triggered);
    void onOpenSongActionTriggered([[maybe_unused]] bool triggered);
    void durationChanged(qint64 duration);
    void positionChanged(qint64 position);
    void finished();
    void onChangeAudioDevice([[maybe_unused]] bool checked);
    void onPlaylistItemDoubleClicked(QTreeWidgetItem *item);
    void onRemoveSongActionTriggered([[maybe_unused]] bool triggered);
    QStringList findFiles(const QString &dir, const QStringList &filters);
    QStringList openFiles(bool justFiles = true);
    void onOpenFilesActionRequested();
    void onOpenPlayListActionRequested();
    void onClosePlayListActionRequested();
    void onSavePlayListActionRequested();
    void onRemovePlayListActionRequested();
    void onOpenSettings();
    void playPauseHelper();
    void onPlayButtonClicked();
    void onStopPlayer();
    void onPlayPrevious();
    void onPlayNext();
    void onAutoRepeatButtonClicked();
    void onSeekSliderPressed();
    void onSeekSliderReleased();
    void onSeekBackwardButtonClicked();
    void onSeekForwardButtonClicked();
    void onVolumeSliderValueChanged(int value);
    void onVolumeIconButtonClicked();
    void onVolumeIncrease();
    void onVolumeDecrease();
    void about();
    void sendNotification(const QString &name);
#ifdef ENABLE_IPC
public slots:
    Q_SCRIPTABLE void togglePlay();
    Q_SCRIPTABLE void playPrevious();
    Q_SCRIPTABLE void playNext();
    Q_SCRIPTABLE void stop();
#ifdef SINGLE_INSTANCE
    Q_SCRIPTABLE void show();
#endif // SINGLE_INSTANCE
#endif // ENABLE_IPC
};
#endif // MAINWINDOW_HPP
