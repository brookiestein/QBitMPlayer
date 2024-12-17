#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QAction>
#include <QCloseEvent>
#include <QDir>
#include <QListWidgetItem>
#include <QMainWindow>
#include <QSettings>
#include <QShortcut>
#include <QStandardPaths>

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

private:
    Ui::MainWindow *m_ui;
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

    enum class AUTOREPEAT { NONE = 0, ONE, ALL };
    AUTOREPEAT m_autorepeat = AUTOREPEAT::NONE;

    QShortcut *m_quitShortcut; /* Ctrl + Q */
    QShortcut *m_openFilesShortcut; /* Ctrl + O */
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
    void onOpenFilesActionRequested();
    void onOpenPlayListActionRequested();
    void onClosePlayListActionRequested();
    void onSavePlayListActionRequested();
    void onRemovePlayListActionRequested();
    void onOpenSettings();
    void playShortcutHelper();
    void onPlayButtonClicked();
    void onStopActionRequested();
    void onPreviousButtonClicked();
    void onNextButtonClicked();
    void onAutoRepeatButtonClicked();
    void onSeekSliderPressed();
    void onSeekSliderReleased();
    void onVolumeSliderValueChanged(int value);
    void onVolumeIconButtonClicked();
    void onVolumeIncrease();
    void onVolumeDecrease();
    void about();
};
#endif // MAINWINDOW_HPP
