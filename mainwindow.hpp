#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>

#include "player.hpp"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *m_ui;
    Player m_player;
    QString m_currentMusic;
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
    void onOpenSingleFileButtonClicked();
    void onPlayButtonClicked();
    void onStopButtonClicked();
    void onAutoRepeatButtonClicked();
    void onSeekSliderPressed();
    void onSeekSliderReleased();
    void onVolumeSliderValueChanged(int value);
};
#endif // MAINWINDOW_HPP
