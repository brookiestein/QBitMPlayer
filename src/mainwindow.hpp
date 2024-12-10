#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QAction>
#include <QCloseEvent>
#include <QDir>
#include <QListWidgetItem>
#include <QMainWindow>
#include <QSettings>
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
    QString getMusicName(const QString &filename);

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QString createEnvironment(
        const QString &location = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                                  + QDir::separator()
                                  + PROJECT_NAME
        );

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MainWindow *m_ui;
    QAction *m_removeSongAction;
    QSettings *m_settings;
    QSettings *m_playlistSettings;
    Player m_player;
    QStringList m_playlist;
    QString m_currentPlaylistName;
    qint8 m_hours;
    qint8 m_minutes;
    qint8 m_seconds;
    enum class AUTOREPEAT_TYPE { DONT_AUTOREPEAT = 0, ONCE, ALL };
    AUTOREPEAT_TYPE m_autoRepeatType = AUTOREPEAT_TYPE::DONT_AUTOREPEAT;
    bool hasRepeated;
    bool m_canModifySlider;

private slots:
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
    void onPlayButtonClicked();
    void onStopButtonClicked();
    void onAutoRepeatButtonClicked();
    void onSeekSliderPressed();
    void onSeekSliderReleased();
    void onVolumeSliderValueChanged(int value);
    void about();
};
#endif // MAINWINDOW_HPP
