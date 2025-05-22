#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QAction>
#include <QCloseEvent>
#include <QDir>
#include <QListWidgetItem>
#include <QMainWindow>
#include <QSettings>
#include <QShortcut>
#include <QShowEvent>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#ifdef USE_IPC
    #include <QDBusConnection>
#endif

#include "config.hpp"
#include "player.hpp"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
#ifdef USE_IPC
    Q_CLASSINFO("D-Bus Interface", SERVICE_NAME)

    QDBusConnection m_dbusConnection;
#endif

    void resetControls();
    QString musicName(const QString &filename);

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QString createEnvironment(
        const QString &location = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                                  + QDir::separator()
                                  + PROJECT_NAME
    );
    void loadPlaylist(const QString &playlistName);
    void setVolumeIcon();

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    Ui::MainWindow *m_ui;
    QSystemTrayIcon m_systray;
    QAction *m_showHideAction;
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

private slots:
    void onQuit();
    void error(const QString &message);
    void warning(const QString &message);
    void durationChanged(qint64 duration);
    void positionChanged(qint64 position);
    void finished();
    void onPlaylistItemDoubleClicked(QListWidgetItem *item);
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
    void onVolumeSliderValueChanged(int value);
    void onVolumeIconButtonClicked();
    void onVolumeIncrease();
    void onVolumeDecrease();
    void about();
#ifdef USE_IPC
public slots:
    Q_SCRIPTABLE void togglePlay();
    Q_SCRIPTABLE void playPrevious();
    Q_SCRIPTABLE void playNext();
    Q_SCRIPTABLE void stop();
#endif // USE_IPC
#ifdef USE_NOTIFICATIONS
    void sendNotification(const QString &name);
#endif // USE_NOTIFICATIONS
};
#endif // MAINWINDOW_HPP
