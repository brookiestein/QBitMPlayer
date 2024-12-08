#include "mainwindow.hpp"
#include "./ui_mainwindow.h"

#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QStandardPaths>

#include "config.hpp"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
    , m_canModifySlider(true)
{
    m_ui->setupUi(this);
    m_ui->durationLabel->setText("0/0");
    m_ui->volumeSlider->setRange(0, 100);
    m_ui->volumeSlider->setValue(50);
    m_ui->volumeSlider->setToolTip(tr("Current volume level: %1%").arg(m_ui->volumeSlider->value()));
    m_player.setVolume(m_ui->volumeSlider->value());

    m_settings = new QSettings(createEnvironment(), QSettings::IniFormat);

    connect(&m_player, &Player::error, this, &MainWindow::error);
    connect(&m_player, &Player::warning, this, &MainWindow::warning);
    connect(&m_player, &Player::durationChanged, this, &MainWindow::durationChanged);
    connect(&m_player, &Player::positionChanged, this, &MainWindow::positionChanged);
    connect(&m_player, &Player::finished, this, &MainWindow::finished);
    connect(m_ui->actionOpenFiles, &QAction::triggered, this, &MainWindow::onOpenFilesActionRequested);
    connect(m_ui->actionQuit, &QAction::triggered, this, &QApplication::quit, Qt::QueuedConnection);
    connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
    connect(m_ui->actionAboutQt, &QAction::triggered, this, &QApplication::aboutQt);
    connect(m_ui->playlistWidget, &QListWidget::itemDoubleClicked, this, &MainWindow::onPlaylistItemDoubleClicked);
    connect(m_ui->openFilesButton, &QPushButton::clicked, this, &MainWindow::onOpenFilesActionRequested);
    connect(m_ui->actionSavePlaylist, &QAction::triggered, this, &MainWindow::onSavePlayListActionRequested);
    connect(m_ui->savePlaylistButton, &QPushButton::clicked, this, &MainWindow::onSavePlayListActionRequested);
    connect(m_ui->seekMusicSlider, &QSlider::sliderPressed, this, &MainWindow::onSeekSliderPressed);
    connect(m_ui->seekMusicSlider, &QSlider::sliderReleased, this, &MainWindow::onSeekSliderReleased);
    connect(m_ui->playButton, &QPushButton::clicked, this, &MainWindow::onPlayButtonClicked);
    connect(m_ui->stopButton, &QPushButton::clicked, this, &MainWindow::onStopButtonClicked);
    connect(m_ui->autoRepeatButton, &QPushButton::clicked, this, &MainWindow::onAutoRepeatButtonClicked);
    connect(m_ui->volumeSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeSliderValueChanged);
}

MainWindow::~MainWindow()
{
    delete m_ui;
}

QString MainWindow::createEnvironment()
{
    auto appDataLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir d;
    if (not d.exists(appDataLocation)) {
        d.mkpath(appDataLocation);
    }

    auto filename = QString("%1%2%3")
                        .arg(appDataLocation, QDir::separator(), PROJECT_NAME".conf");
    return filename;
}

void MainWindow::resetControls()
{
    m_ui->playButton->setText(tr("Play"));
    m_ui->seekMusicSlider->setValue(0);
    m_ui->durationLabel->setText(QString("0/%1:%2:%3")
                                     .arg(QString::number(m_hours),
                                          QString::number(m_minutes),
                                          QString::number(m_seconds))
                                 );
}

QString MainWindow::getMusicName(const QString &filename)
{
    auto musicName = filename.mid(filename.lastIndexOf('/') + 1, filename.size());
    musicName = musicName.mid(0, musicName.lastIndexOf('.'));
    return musicName;
}

void MainWindow::error(const QString &message)
{
    QMessageBox::critical(this, tr("Error"), message);
}

void MainWindow::warning(const QString &message)
{
    QMessageBox::warning(this, tr("Warning"), message);
}

void MainWindow::durationChanged(qint64 duration)
{
    m_ui->seekMusicSlider->setRange(0, duration);
    m_hours = 0;
    m_minutes = 0;
    m_seconds = 0;

    while (duration > 0) {
        ++m_seconds;

        if (m_seconds == 60) {
            ++m_minutes;
            m_seconds = 0;
        }

        if (m_minutes == 60) {
            ++m_hours;
            m_minutes = 0;
        }

        --duration;
    }

    m_ui->durationLabel->setText(QString("0/%1:%2:%3")
                                     .arg(QString::number(m_hours),
                                          QString::number(m_minutes),
                                          QString::number(m_seconds))
                                 );
}

void MainWindow::positionChanged(qint64 position)
{
    if (m_canModifySlider) {
        m_ui->seekMusicSlider->setValue(position);
    }

    qint8 hours = 0;
    qint8 minutes = 0;
    qint8 seconds = 0;

    while (position > 0) {
        ++seconds;

        if (seconds == 60) {
            ++minutes;
            seconds = 0;
        }

        if (minutes == 60) {
            ++hours;
            minutes = 0;
        }

        --position;
    }

    QRegularExpression regex("*/");
    auto time = QString("%1:%2:%3/%4:%5:%6")
                    .arg(QString::number(hours),
                         QString::number(minutes),
                         QString::number(seconds),
                         QString::number(m_hours),
                         QString::number(m_minutes),
                         QString::number(m_seconds)
                         );
    m_ui->durationLabel->setText(time);
}

void MainWindow::finished()
{
    switch (m_autoRepeatType)
    {
    case AUTOREPEAT_TYPE::DONT_AUTOREPEAT:
        resetControls();
        break;
    case AUTOREPEAT_TYPE::ONCE:
        if (hasRepeated) {
            resetControls();
        } else {
            m_player.play();
            hasRepeated = true;
        }
        break;
    case AUTOREPEAT_TYPE::ALL:
        /* This call will play next music if there's any, if not returns false,
         * allowing us to reset the widgets. */
        if (not m_player.playNext()) {
            /* There's just one music in the playlist, so repeat this one. */
            m_player.playPrevious();
        }
        break;
    }
}

void MainWindow::onPlaylistItemDoubleClicked(QListWidgetItem *item)
{
    /* Items in playlistWidget are added in the same order as those in m_playlist. */
    auto index = m_ui->playlistWidget->indexFromItem(item).row();

    m_player.stop();
    resetControls();
    m_player.setCurrent(index);
    m_player.play();

    m_ui->playingEdit->setText(getMusicName(m_playlist[index]));
    m_ui->playButton->setText(tr("Pause"));
}

void MainWindow::onOpenFilesActionRequested()
{
    auto dir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    auto filters = tr("Audio files (*.aac *.avi *.mp3 *.wav)");

    auto playlist = QFileDialog::getOpenFileNames(this,
                                               tr("Open Audio Files"),
                                               dir,
                                               filters
                                               );

    if (playlist.isEmpty()) {
        return;
    }

    m_playlist << playlist;

    m_player.setPlayList(m_playlist);
    hasRepeated = false;

    for (const auto &filename : m_playlist) {
        auto name = getMusicName(filename);
        m_ui->playlistWidget->addItem(name);
    }
}

void MainWindow::onOpenPlayListActionRequested()
{
    // TODO
}

void MainWindow::onSavePlayListActionRequested()
{
    if (m_playlist.isEmpty()) {
        QMessageBox::warning(this,
                             tr("Warning"),
                             tr("You must first load some music files."));
        return;
    }

    auto name = QInputDialog::getText(this,
                                      tr("Give it a name"),
                                      tr("How should we call this awesome playlist?"));
    if (name.isEmpty()) {
        return;
    }

    m_settings->beginGroup("Playlists");
    if (m_settings->childGroups().contains(name, Qt::CaseInsensitive)) {
        auto reply = QMessageBox::question(this,
                                           tr("Oops"),
                                           tr("It seems that this playlist already exists. "
                                              "Would you like to replace it?")
                                           );
        if (reply != QMessageBox::Yes) {
            m_settings->endGroup();
            return;
        }
    }

    m_settings->beginGroup(name);
    for (const auto &filename : m_playlist) {
        m_settings->setValue(filename, getMusicName(filename));
    }
    m_settings->endGroup(); /* name */
    m_settings->endGroup(); /* Playlists */

    QMessageBox::information(this,
                             tr("Yay!"),
                             tr("Now you can play all the awesome music %1 has!").arg(name));
}

void MainWindow::onPlayButtonClicked()
{
    auto *snder = qobject_cast<QPushButton *>(sender());
    /* tr() seems to inserts an & somewhere in the text.
     * I find easier just to check if text is not &Pause than
     * looking where the & is. */
    if (snder->text() == tr("&Play")) {
        if (m_ui->playingEdit->text().isEmpty()) {
            QMessageBox::warning(this,
                                 tr("Warning"),
                                 tr("You must first load a music or choose one from the playlist.")
                                 );
            return;
        }

        m_player.play();
        snder->setText(tr("Pause"));
    } else if (snder->text() != tr("&Pause")) {
        m_player.play();
        snder->setText(tr("Pause"));
    } else {
        m_player.pause();
        snder->setText(tr("Continue"));
    }
}

void MainWindow::onStopButtonClicked()
{
    m_player.stop();
    if (m_ui->playButton->text() != tr("&Play")) {
        m_ui->playButton->setText(tr("Play"));
    }

    hasRepeated = false;
}

void MainWindow::onAutoRepeatButtonClicked()
{
    auto *snder = qobject_cast<QPushButton *>(sender());
    auto text = snder->text();

    if (text == tr("&Auto Repeat")) {
        m_autoRepeatType = AUTOREPEAT_TYPE::ONCE;
        snder->setText(tr("Auto Repeat Once"));
        hasRepeated = false;
    } else if (text.contains("Once")) {
        m_autoRepeatType = AUTOREPEAT_TYPE::ALL;
        snder->setText(tr("Auto Repeat All"));
    } else {
        m_autoRepeatType = AUTOREPEAT_TYPE::DONT_AUTOREPEAT;
        snder->setText(tr("Auto Repeat"));
    }
}

void MainWindow::onSeekSliderPressed()
{
    m_canModifySlider = false;
}

void MainWindow::onSeekSliderReleased()
{
    qint64 milliseconds = m_ui->seekMusicSlider->value() * 1'000;
    m_player.seek(milliseconds);
    m_canModifySlider = true;
}

void MainWindow::onVolumeSliderValueChanged(int value)
{
    float volumeLevel {value / 100.00f};
    m_player.setVolume(volumeLevel);
    m_ui->volumeSlider->setToolTip(
        tr("Current volume level: %1%").arg(m_ui->volumeSlider->value())
    );
}

void MainWindow::about()
{
    auto text = QString("\
<h1>%1 %2</h1>\
<p>License: %3<br>\
Authors: %4<br>\
Build Date: %5</p>").arg(PROJECT_NAME, PROJECT_VERSION, PROJECT_LICENSE, AUTHORS, BUILD_DATETIME);
    QMessageBox messageBox;
    messageBox.setWindowTitle(tr("About - %1").arg(PROJECT_NAME));
    messageBox.setTextFormat(Qt::RichText);
    messageBox.setText(text);
    messageBox.exec();
}
