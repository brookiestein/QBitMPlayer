#include "mainwindow.hpp"
#include "./ui_mainwindow.h"

#include <QDir>
#include <QEventLoop>
#include <QFileDialog>
#include <QInputDialog>
#include <QMediaFormat>
#include <QMessageBox>
#include <QStandardPaths>

#include "config.hpp"
#include "playlistchooser.hpp"
#include "settings.hpp"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
    , m_canModifySlider(true)
{
    m_ui->setupUi(this);
    m_ui->playlistLabel->setText(tr("Playlist: Unnamed"));
    m_ui->durationLabel->setText("0/0");
    m_ui->autoRepeatButton->setToolTip(
        tr("Neither current music nor current playlist repeats.")
    );

    m_removeSongAction = new QAction(tr("Remove song from playlist"), this);
    m_ui->playlistWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_ui->playlistWidget->addAction(m_removeSongAction);

    m_settings = new QSettings(createEnvironment(), QSettings::IniFormat, this);

    m_playlistSettings = new QSettings(
        createEnvironment(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)),
        QSettings::IniFormat,
        this);

    m_settings->beginGroup("AudioSettings");
    m_ui->volumeSlider->setRange(0, 100);
    m_ui->volumeSlider->setValue(m_settings->value("VolumeLevel", 50).toInt());
    m_ui->volumeSlider->setToolTip(tr("Current volume level: %1%").arg(m_ui->volumeSlider->value()));
    m_player.setVolume(m_ui->volumeSlider->value());
    m_settings->endGroup();

    connect(&m_player, &Player::error, this, &MainWindow::error);
    connect(&m_player, &Player::warning, this, &MainWindow::warning);
    connect(&m_player, &Player::durationChanged, this, &MainWindow::durationChanged);
    connect(&m_player, &Player::positionChanged, this, &MainWindow::positionChanged);
    connect(&m_player, &Player::finished, this, &MainWindow::finished);
    connect(m_removeSongAction, &QAction::triggered, this, &MainWindow::onRemoveSongActionTriggered);
    connect(m_ui->actionOpenFiles, &QAction::triggered, this, &MainWindow::onOpenFilesActionRequested);
    connect(m_ui->actionQuit, &QAction::triggered, this, &QApplication::quit, Qt::QueuedConnection);
    connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
    connect(m_ui->actionAboutQt, &QAction::triggered, this, &QApplication::aboutQt);
    connect(m_ui->playlistWidget, &QListWidget::itemDoubleClicked, this, &MainWindow::onPlaylistItemDoubleClicked);
    connect(m_ui->openFilesButton, &QPushButton::clicked, this, &MainWindow::onOpenFilesActionRequested);
    connect(m_ui->actionOpenPlaylist, &QAction::triggered, this, &MainWindow::onOpenPlayListActionRequested);
    connect(m_ui->actionClosePlaylist, &QAction::triggered, this, &MainWindow::onClosePlayListActionRequested);
    connect(m_ui->actionSavePlaylist, &QAction::triggered, this, &MainWindow::onSavePlayListActionRequested);
    connect(m_ui->actionRemovePlaylist, &QAction::triggered, this, &MainWindow::onRemovePlayListActionRequested);
    connect(m_ui->openPlaylistButton, &QPushButton::clicked, this, &MainWindow::onOpenPlayListActionRequested);
    connect(m_ui->closePlayListButton, &QPushButton::clicked, this, &MainWindow::onClosePlayListActionRequested);
    connect(m_ui->savePlaylistButton, &QPushButton::clicked, this, &MainWindow::onSavePlayListActionRequested);
    connect(m_ui->removePlaylistButton, &QPushButton::clicked, this, &MainWindow::onRemovePlayListActionRequested);
    connect(m_ui->actionSettings, &QAction::triggered, this, &MainWindow::onOpenSettings);
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

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_settings->beginGroup("WindowSettings");
    if (m_settings->value("RememberWindowSize", false).toBool()) {
        m_settings->setValue("Width", geometry().width());
        m_settings->setValue("Height", geometry().height());
        m_settings->setValue("Maximized", isMaximized());
    }
    m_settings->endGroup();

    m_settings->beginGroup("AudioSettings");
    if (m_settings->value("RememberVolumeLevel", false).toBool()) {
        m_settings->setValue("VolumeLevel", m_ui->volumeSlider->value());
    }
    m_settings->endGroup();

    QMainWindow::closeEvent(event);
}

QString MainWindow::createEnvironment(const QString &location)
{
    QDir d;
    if (not d.exists(location)) {
        d.mkpath(location);
    }

    auto filename = QString("%1%2%3")
                        .arg(location, QDir::separator(), PROJECT_NAME".conf");
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

void MainWindow::onRemoveSongActionTriggered(bool triggered)
{
    auto selectedItems = m_ui->playlistWidget->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    /* Remember that playlistWidget has its items added in the same order as m_playlist */
    auto index = m_ui->playlistWidget->indexFromItem(selectedItems[0]).row();
    auto *item = m_ui->playlistWidget->takeItem(index);
    delete item;

    auto filename = m_playlist[index];
    m_playlist.removeAt(index);

    if (filename == m_player.currentMusicFilename()) {
        onClosePlayListActionRequested();
    }

    m_playlistSettings->beginGroup("Playlists");
    for (const auto &playlist : m_playlistSettings->childGroups()) {
        m_playlistSettings->beginGroup(playlist);
        if (m_playlistSettings->contains(filename)) {
            m_playlistSettings->remove(filename);
            m_playlistSettings->endGroup();
            break;
        }
        m_playlistSettings->endGroup();
    }
    m_playlistSettings->endGroup();
}

void MainWindow::onOpenFilesActionRequested()
{
    auto dir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    QString filters = tr("Audio files (");
    QMediaFormat mediaFormat;
    auto supportedFormats = mediaFormat.supportedFileFormats(QMediaFormat::Decode);
    for (const auto &format : supportedFormats) {
        switch (format)
        {
        case QMediaFormat::UnspecifiedFormat:
            break;
        case QMediaFormat::WMV:
            filters += "*.wmv ";
            break;
        case QMediaFormat::AVI:
            filters += "*.avi ";
            break;
        case QMediaFormat::Matroska:
            filters += "*.mkv *.mk3d *.mka *.mks ";
            break;
        case QMediaFormat::MPEG4:
            filters += "*.mp4 ";
            break;
        case QMediaFormat::Ogg:
            filters += "*.ogg *.ogv *.oga *.ogx *.ogm *.spx *.opus ";
            break;
        case QMediaFormat::QuickTime:
            break;
        case QMediaFormat::WebM:
            filters += "*.webm ";
            break;
        case QMediaFormat::Mpeg4Audio:
            filters += "*.m4a ";
            break;
        case QMediaFormat::AAC:
            filters += "*.aac *.3gp ";
            break;
        case QMediaFormat::WMA:
            filters += "*.wma ";
            break;
        case QMediaFormat::MP3:
            filters += "*.mp3 ";
            break;
        case QMediaFormat::FLAC:
            filters += "*.flac ";
            break;
        case QMediaFormat::Wave:
            filters += "*.wav *.wave ";
            break;
        }
    }

    filters = QString("%1)").arg(filters.trimmed());

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
    QEventLoop loop;
    PlaylistChooser chooser(m_playlistSettings);
    connect(&chooser, &PlaylistChooser::closed, &loop, &QEventLoop::quit);
    chooser.show();
    loop.exec();

    auto playlist = chooser.playlist();
    if (playlist.isEmpty()) {
        return;
    }

    /* First close the current playlist if there's one. */
    onClosePlayListActionRequested();

    m_playlistSettings->beginGroup("Playlists");
    m_playlistSettings->beginGroup(playlist);

    for (auto filename : m_playlistSettings->allKeys()) {
#ifdef Q_OS_LINUX
        /* For some reason QSettings removes the first slash.
         * Test is required for a Windows machine */
        filename.prepend("/");
#endif
        m_playlist << filename;
        auto musicName = getMusicName(m_playlistSettings->value(filename).toString());
        m_ui->playlistWidget->addItem(musicName);
    }

    m_player.setPlayList(m_playlist);
    hasRepeated = false;
    m_playlistSettings->endGroup(); /* Playlists */
    m_playlistSettings->endGroup(); /* playlist */
    m_ui->playlistLabel->setText(tr("Playlist: %1").arg(playlist));
    m_currentPlaylistName = playlist;
}

void MainWindow::onClosePlayListActionRequested()
{
    m_player.clearSource();
    m_playlist.clear();
    m_ui->playlistLabel->setText(tr("Playlist: Unnamed"));
    m_currentPlaylistName.clear();
    m_ui->playlistWidget->clear();
    m_ui->playingEdit->setText("");
    m_ui->playButton->setText(tr("Play"));
    m_ui->durationLabel->setText("0/0");
    m_ui->seekMusicSlider->setValue(0);
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

    m_playlistSettings->beginGroup("Playlists");
    if (m_playlistSettings->childGroups().contains(name, Qt::CaseInsensitive)) {
        auto reply = QMessageBox::question(this,
                                           tr("Oops"),
                                           tr("It seems that this playlist already exists. "
                                              "Would you like to replace it?")
                                           );
        if (reply != QMessageBox::Yes) {
            m_playlistSettings->endGroup();
            return;
        }
    }

    m_playlistSettings->beginGroup(name);
    for (const auto &filename : m_playlist) {
        m_playlistSettings->setValue(filename, getMusicName(filename));
    }
    m_playlistSettings->endGroup(); /* name */
    m_playlistSettings->endGroup(); /* Playlists */

    m_ui->playlistLabel->setText(tr("Playlist: %1").arg(name));
    m_currentPlaylistName = name;

    QMessageBox::information(this,
                             tr("Yay!"),
                             tr("Now you can play all the awesome music %1 has!").arg(name));
}

void MainWindow::onRemovePlayListActionRequested()
{
    QEventLoop loop;
    PlaylistChooser chooser(m_playlistSettings);
    connect(&chooser, &PlaylistChooser::closed, &loop, &QEventLoop::quit);
    chooser.show();
    loop.exec();

    auto playlist = chooser.playlist();
    if (playlist.isEmpty()) {
        return;
    }

    m_playlistSettings->beginGroup("Playlists");
    m_playlistSettings->remove(playlist);
    m_playlistSettings->endGroup();

    if (playlist == m_currentPlaylistName) {
        onClosePlayListActionRequested();
    }

    QMessageBox::information(this,
                             tr("Playlist Removed"),
                             tr("The playlist %1 was successfully removed!").arg(playlist)
                             );
}

void MainWindow::onOpenSettings()
{
    QEventLoop loop;
    Settings settings;
    connect(&settings, &Settings::closed, &loop, &QEventLoop::quit);
    settings.show();
    loop.exec();
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
        snder->setToolTip(tr("Current music will repeat just once."));
        hasRepeated = false;
    } else if (text.contains("Once")) {
        m_autoRepeatType = AUTOREPEAT_TYPE::ALL;
        snder->setText(tr("Auto Repeat All"));
        snder->setToolTip(tr("Current playlist will repeat forever."));
    } else {
        m_autoRepeatType = AUTOREPEAT_TYPE::DONT_AUTOREPEAT;
        snder->setText(tr("Auto Repeat"));
        snder->setToolTip(tr("Neither current music nor current playlist repeats."));
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
