#include "mainwindow.hpp"
#include "./ui_mainwindow.h"

#include <QDir>
#include <QDirIterator>
#include <QEventLoop>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMediaFormat>
#include <QMessageBox>
#include <QStandardPaths>
#include <QShortcut>

#include "config.hpp"
#include "playlistchooser.hpp"
#include "settings.hpp"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_ui {new Ui::MainWindow}
    , m_canModifySlider {true}
    , m_quitShortcut {new QShortcut(QKeySequence(Qt::Modifier::CTRL | Qt::Key_Q), this)}
    , m_openFilesShortcut {new QShortcut(QKeySequence(Qt::Modifier::CTRL | Qt::Key_O), this)}
    , m_openDirectoryShortcut {new QShortcut(QKeySequence(Qt::Modifier::CTRL | Qt::Key_D), this)}
    , m_openPlaylistShortcut {new QShortcut(QKeySequence(Qt::Modifier::CTRL | Qt::Modifier::SHIFT | Qt::Key_O), this)}
    , m_closePlaylistShortcut {new QShortcut(QKeySequence(Qt::Modifier::CTRL | Qt::Key_C), this)}
    , m_savePlaylistShortcut {new QShortcut(QKeySequence(Qt::Modifier::CTRL | Qt::Key_S), this)}
    , m_removePlaylistShortcut {new QShortcut(QKeySequence(Qt::Modifier::CTRL | Qt::Key_R), this)}
    , m_settingsShortcut {new QShortcut(QKeySequence(Qt::Modifier::CTRL | Qt::Modifier::SHIFT | Qt::Key_S), this)}
    , m_playShortcut {new QShortcut(QKeySequence(Qt::Key_P), this)}
    , m_playShortcut2 {new QShortcut(QKeySequence(Qt::Key_Space), this)}
    , m_stopShortcut {new QShortcut(QKeySequence(Qt::Key_Escape), this)}
    , m_previousShortcut {new QShortcut(QKeySequence(Qt::Key_Left), this)}
    , m_nextShortcut {new QShortcut(QKeySequence(Qt::Key_Right), this)}
    , m_autorepeatShortcut {new QShortcut(QKeySequence(Qt::Key_R), this)}
    , m_increaseVolumeBy5Shortcut {new QShortcut(QKeySequence(Qt::Key_Up), this)}
    , m_increaseVolumeBy10Shortcut {new QShortcut(QKeySequence(Qt::Modifier::SHIFT | Qt::Key_Up), this)}
    , m_decreaseVolumeBy5Shortcut {new QShortcut(QKeySequence(Qt::Key_Down), this)}
    , m_decreaseVolumeBy10Shortcut {new QShortcut(QKeySequence(Qt::Modifier::SHIFT | Qt::Key_Down), this)}
    , m_currentPosition(0)
{
    m_ui->setupUi(this);

    m_ui->playlistLabel->setText(tr("Playlist: Unnamed"));
    m_ui->durationLabel->setText("00:00:00/00:00:00");
    m_ui->autoRepeatButton->setText("");
    m_ui->autoRepeatButton->setToolTip(
        tr("Neither current music nor current playlist repeats.")
    );
    m_ui->volumeIconButton->setToolTip(tr("Click to increase volume by 25%."));

    m_addSongToPlaylist = new QAction(tr("Add song to playlist"), this);
    m_removeSongAction = new QAction(tr("Remove song from playlist"), this);
    m_ui->playlistWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_ui->playlistWidget->addAction(m_addSongToPlaylist);
    m_ui->playlistWidget->addAction(m_removeSongAction);

    m_settings = new QSettings(createEnvironment(), QSettings::IniFormat, this);

    m_playlistSettings = new QSettings(
        createEnvironment(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)),
        QSettings::IniFormat,
        this);

    m_settings->beginGroup("WindowSettings");
    if (m_settings->value("Centered", false).toBool()) {
        if (not m_settings->value("AlwaysMaximized", false).toBool()) {
            move(screen()->geometry().center() - frameGeometry().center());
        }
    }
    m_settings->endGroup();

    m_settings->beginGroup("AudioSettings");
    m_ui->volumeSlider->setRange(0, 100);
    m_ui->volumeSlider->setValue(m_settings->value("VolumeLevel", 50).toInt());
    m_ui->volumeSlider->setToolTip(tr("Current volume level: %1%").arg(m_ui->volumeSlider->value()));
    m_player.setVolume(m_ui->volumeSlider->value());
    setVolumeIcon();
    m_settings->endGroup();

    m_settings->beginGroup("PlaylistSettings");
    auto playlistName = m_settings->value("DefaultPlaylist", "").toString();
    if (not playlistName.isEmpty() and playlistName != "None") {
        loadPlaylist(playlistName);
    }
    m_settings->endGroup();

    connect(&m_player, &Player::error, this, &MainWindow::error);
    connect(&m_player, &Player::warning, this, &MainWindow::warning);
    connect(&m_player, &Player::durationChanged, this, &MainWindow::durationChanged);
    connect(&m_player, &Player::positionChanged, this, &MainWindow::positionChanged);
    connect(&m_player, &Player::finished, this, &MainWindow::finished);
    connect(m_addSongToPlaylist, &QAction::triggered, this, &MainWindow::onOpenFilesActionRequested);
    connect(m_removeSongAction, &QAction::triggered, this, &MainWindow::onRemoveSongActionTriggered);
    connect(m_ui->actionOpenFiles, &QAction::triggered, this, &MainWindow::onOpenFilesActionRequested);
    connect(m_ui->actionOpen_Directory, &QAction::triggered, this, &MainWindow::onOpenFilesActionRequested);
    connect(m_ui->actionQuit, &QAction::triggered, this, &MainWindow::onQuit);
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
    connect(m_ui->stopButton, &QPushButton::clicked, this, &MainWindow::onStopActionRequested);
    connect(m_ui->previousButton, &QPushButton::clicked, this, &MainWindow::onPreviousButtonClicked);
    connect(m_ui->nextButton, &QPushButton::clicked, this, &MainWindow::onNextButtonClicked);
    connect(m_ui->autoRepeatButton, &QPushButton::clicked, this, &MainWindow::onAutoRepeatButtonClicked);
    connect(m_ui->volumeSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeSliderValueChanged);
    connect(m_ui->volumeIconButton, &QPushButton::clicked, this, &MainWindow::onVolumeIconButtonClicked);

    /* Keyboard shortcuts */
    connect(m_quitShortcut, &QShortcut::activated, this, &MainWindow::onQuit);
    connect(m_openFilesShortcut, &QShortcut::activated, this, &MainWindow::onOpenFilesActionRequested);
    connect(m_openDirectoryShortcut, &QShortcut::activated, this, &MainWindow::onOpenFilesActionRequested);
    connect(m_openPlaylistShortcut, &QShortcut::activated, this, &MainWindow::onOpenPlayListActionRequested);
    connect(m_closePlaylistShortcut, &QShortcut::activated, this, &MainWindow::onClosePlayListActionRequested);
    connect(m_savePlaylistShortcut, &QShortcut::activated, this, &MainWindow::onSavePlayListActionRequested);
    connect(m_removePlaylistShortcut, &QShortcut::activated, this, &MainWindow::onRemovePlayListActionRequested);
    connect(m_settingsShortcut, &QShortcut::activated, this, &MainWindow::onOpenSettings);
    connect(m_playShortcut, &QShortcut::activated, this, &MainWindow::playShortcutHelper);
    connect(m_playShortcut2, &QShortcut::activated, this, &MainWindow::playShortcutHelper);
    connect(m_stopShortcut, &QShortcut::activated, this, &MainWindow::onStopActionRequested);
    connect(m_previousShortcut, &QShortcut::activated, this, &MainWindow::onPreviousButtonClicked);
    connect(m_nextShortcut, &QShortcut::activated, this, &MainWindow::onNextButtonClicked);
    connect(m_autorepeatShortcut, &QShortcut::activated, this, &MainWindow::onAutoRepeatButtonClicked);
    connect(m_increaseVolumeBy5Shortcut, &QShortcut::activated, this, &MainWindow::onVolumeIncrease);
    connect(m_increaseVolumeBy10Shortcut, &QShortcut::activated, this, &MainWindow::onVolumeIncrease);
    connect(m_decreaseVolumeBy5Shortcut, &QShortcut::activated, this, &MainWindow::onVolumeDecrease);
    connect(m_decreaseVolumeBy10Shortcut, &QShortcut::activated, this, &MainWindow::onVolumeDecrease);
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

void MainWindow::onQuit()
{
    auto reply = QMessageBox::question(
        this,
        tr("Quit"),
        tr("Are you sure you want to exit?")
    );

    if (reply == QMessageBox::Yes) {
        auto *app = QApplication::instance();
        app->quit();
    }
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
    m_ui->playButton->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackStart));
    m_ui->seekMusicSlider->setValue(0);
    m_ui->durationLabel->setText(QString("00:00:00/%1:%2:%3")
                                     .arg(QString::number(m_hours),
                                          QString::number(m_minutes),
                                          QString::number(m_seconds))
                                 );
}

QString MainWindow::musicName(const QString &filename)
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

    m_ui->durationLabel->setText(
        QString("00:00:00/%1%2:%3%4:%5%6")
            .arg(m_hours < 10 ? "0" : "",
                 QString::number(m_hours),
                 m_minutes < 10 ? "0" : "",
                 QString::number(m_minutes),
                 m_seconds < 10 ? "0" : "",
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

    auto time = QString("%1%2:%3%4:%5%6/%7%8:%9%10:%11%12")
                    .arg(hours < 10 ? "0" : "", QString::number(hours),
                         minutes < 10 ? "0" : "", QString::number(minutes),
                         seconds < 10 ? "0" : "", QString::number(seconds),
                         m_hours < 10 ? "0" : "", QString::number(m_hours),
                         m_minutes < 10 ? "0" : "", QString::number(m_minutes),
                         m_seconds < 10 ? "0" : "", QString::number(m_seconds)
                         );
    m_ui->durationLabel->setText(time);
}

void MainWindow::finished()
{
    switch (m_autorepeat)
    {
    case AUTOREPEAT::NONE:
        if (m_player.playNext()) {
            int index = m_ui->playlistWidget->currentIndex().row();
            m_ui->playlistWidget->setCurrentRow(index + 1);
        } else {
            resetControls();
        }
        break;
    case AUTOREPEAT::ONE:
        m_player.play();
        break;
    case AUTOREPEAT::ALL:
        if (m_playlist.size() == 1) {
            m_player.play();
        } else {
            if (m_player.playNext()) {
                int index = m_ui->playlistWidget->currentIndex().row();
                m_ui->playlistWidget->setCurrentRow(index + 1);
            } else {
                /* We've reached the end of the playlist, let's start again. */
                m_player.setCurrent(0);
                m_player.play();
                m_ui->playlistWidget->setCurrentRow(0);
            }
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

    m_ui->playingEdit->setText(musicName(m_playlist[index]));
    m_ui->playButton->setText(tr("Pause"));
    m_ui->playButton->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackPause));
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

            if (m_playlistSettings->allKeys().isEmpty()) {
                m_playlistSettings->remove(playlist);
                m_ui->playlistLabel->setText(tr("Playlist: Unnamed"));

                m_settings->beginGroup("PlaylistSettings");
                if (m_settings->value("DefaultPlaylist").toString() == playlist) {
                    m_settings->setValue("DefaultPlaylist", "None");
                }
                m_settings->endGroup();
            }

            m_playlistSettings->endGroup();

            break;
        }
        m_playlistSettings->endGroup();
    }
    m_playlistSettings->endGroup();
}

QStringList MainWindow::findFiles(const QString &dir, const QStringList &filters)
{
    QStringList files;
    for (const auto &entry : QDir(dir).entryList(QDir::Filter::AllEntries | QDir::Filter::NoDotAndDotDot)) {
        auto filepath = QString("%1%2%3").arg(dir, QDir::separator(), entry);

        if (QFileInfo info(filepath); info.isFile()) {
            auto fileExtension = entry.mid(entry.lastIndexOf('.'));
            if (filters.contains(fileExtension))
                files << filepath;
        } else {
            qInfo().noquote() << tr("Found directory: %1 in: %2. Looking into it...").arg(entry, dir);
            files << findFiles(filepath, filters);
        }
    }

    if (!files.isEmpty()) {
        qInfo().noquote() << tr("Loaded all music files from directory: %1.").arg(dir);
    }

    return files;
}

QStringList MainWindow::openFiles(bool justFiles)
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

    if (justFiles)
        return QFileDialog::getOpenFileNames(this,
                                             tr("Open Audio Files"),
                                             dir,
                                             filters
                                             );

    dir = QFileDialog::getExistingDirectory(this, tr("Open Music Directory"), dir);
    if (dir.isEmpty())
        return {};

    filters = filters
                  .mid(filters.indexOf('(') + 1)
                  .replace(")", "")
                  .replace("*", "");

    auto files = findFiles(dir, filters.split(' '));

    if (not files.isEmpty() and m_ui->playlistLabel->text().contains(tr("Unnamed"))) {
        auto playlistName = dir.mid(dir.lastIndexOf('/') + 1);
        m_ui->playlistLabel->setText(tr("Playlist: %1*").arg(playlistName));
        m_ui->playlistLabel->setToolTip(tr("Playlist is currently not saved."));
    }

    return files;
}

void MainWindow::onOpenFilesActionRequested()
{
    bool wasPlaylistEmpty = m_playlist.isEmpty();
    QStringList playlist = openFiles(sender() == m_ui->actionOpenFiles);

    if (playlist.isEmpty()) {
        return;
    }

    m_playlist << playlist;

    m_player.setPlayList(m_playlist);

    for (const auto &filename : playlist) {
        auto name = musicName(filename);
        m_ui->playlistWidget->addItem(name);
    }

    if (wasPlaylistEmpty) {
        m_player.setCurrent(0);
        m_ui->playingEdit->setText(musicName(m_playlist[0]));
        m_ui->playlistWidget->setCurrentRow(0);
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

    loadPlaylist(playlist);
}

void MainWindow::loadPlaylist(const QString &playlistName)
{
    /* First close the current playlist if there's one. */
    onClosePlayListActionRequested();

    if (playlistName == "None") {
        return;
    }

    m_playlistSettings->beginGroup("Playlists");
    m_playlistSettings->beginGroup(playlistName);

    for (auto filename : m_playlistSettings->allKeys()) {
#ifdef Q_OS_LINUX
        /* For some reason QSettings removes the first slash.
         * Test is required for a Windows machine */
        filename.prepend("/");
#endif
        m_playlist << filename;
        auto name = musicName(m_playlistSettings->value(filename).toString());
        m_ui->playlistWidget->addItem(name);
    }

    m_playlistInitState = m_playlist;
    m_player.setPlayList(m_playlist);
    m_player.setCurrent(0);

    m_playlistSettings->endGroup(); /* Playlists */
    m_playlistSettings->endGroup(); /* playlist */

    m_ui->playlistLabel->setText(tr("Playlist: %1").arg(playlistName));
    m_ui->playlistLabel->setToolTip("");
    m_ui->playlistWidget->setCurrentRow(0);
    m_ui->playingEdit->setText(musicName(m_playlist[0]));

    m_currentPlaylistName = playlistName;
}

void MainWindow::setVolumeIcon()
{
    int level = m_ui->volumeSlider->value();
    if (level < 25) {
        m_ui->volumeIconButton->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::AudioVolumeLow));
    } else if (level < 75) {
        m_ui->volumeIconButton->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::AudioVolumeMedium));
    } else {
        m_ui->volumeIconButton->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::AudioVolumeHigh));
    }
}

void MainWindow::onClosePlayListActionRequested()
{
    m_player.clearSource();
    m_playlist.clear();
    m_ui->playlistLabel->setText(tr("Playlist: Unnamed"));
    m_ui->playlistLabel->setToolTip("");
    m_currentPlaylistName.clear();
    m_ui->playlistWidget->clear();
    m_ui->playingEdit->setText("");
    m_ui->playButton->setText(tr("Play"));
    m_ui->playButton->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackStart));
    m_ui->durationLabel->setText("00:00:00");
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

    bool updated {false};
    QString name;
    QStringList songs;

    if (m_ui->playlistLabel->text() == tr("Playlist: Unnamed")) {
        name = QInputDialog::getText(this,
                                     tr("Give it a name"),
                                     tr("How should we call this awesome playlist?"));
        if (name.isEmpty()) {
            return;
        }

        songs = m_playlist;
    } else {
        for (const auto &song : m_playlist) {
            if (not m_playlistInitState.contains(song)) {
                songs << song;
            }
        }

        if (songs.isEmpty()) {
            return;
        }

        name = m_ui->playlistLabel->text();
        name = name.mid(name.indexOf(':') + 2);
        if (name.endsWith('*'))
            name = name.mid(0, name.lastIndexOf('*'));
        updated = true;
    }

    m_playlistSettings->beginGroup("Playlists");
    if (m_playlistSettings->childGroups().contains(name, Qt::CaseInsensitive) and not updated) {
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
    for (const auto &filename : songs) {
        m_playlistSettings->setValue(filename, musicName(filename));
    }
    m_playlistSettings->endGroup(); /* name */
    m_playlistSettings->endGroup(); /* Playlists */

    m_ui->playlistLabel->setText(tr("Playlist: %1").arg(name));
    m_currentPlaylistName = name;

    auto message = updated
                       ? tr("Playlist %1 has been updated!").arg(name)
                       : tr("Now you can play all the awesome music %1 has!").arg(name);

    m_ui->playlistLabel->setToolTip("");
    QMessageBox::information(this, tr("Yay!"), message);
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

void MainWindow::playShortcutHelper()
{
    if (m_ui->playingEdit->text().isEmpty()) {
        return;
    }

    m_ui->playButton->click();
}

void MainWindow::onPlayButtonClicked()
{
    auto *snder = qobject_cast<QPushButton *>(sender());
    /* tr() seems to inserts an & somewhere in the text.
     * I find easier just to check if text is not &Pause than
     * looking where the & is. */
    if (snder->text().contains(tr("Play"))) {
        if (m_ui->playingEdit->text().isEmpty()) {
            QMessageBox::warning(this,
                                 tr("Warning"),
                                 tr("You must first load a music or choose one from the playlist.")
                                 );
            return;
        }

        m_player.seek(m_currentPosition);
        m_player.play();
        snder->setText(tr("Pause"));
        m_ui->playButton->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackPause));
        m_currentPosition = 0;
    } else if (not snder->text().contains(tr("Pause"))) {
        m_player.seek(m_currentPosition);
        m_player.play();
        snder->setText(tr("Pause"));
        m_ui->playButton->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackPause));
        m_currentPosition = 0;
    } else {
        m_currentPosition = m_player.currentPosition();
        m_player.pause();
        snder->setText(tr("Continue"));
        m_ui->playButton->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackStart));
    }
}

void MainWindow::onStopActionRequested()
{
    m_player.stop();
    if (m_ui->playButton->text() != tr("&Play")) {
        m_ui->playButton->setText(tr("Play"));
        m_ui->playButton->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackStart));
    }
}

void MainWindow::onPreviousButtonClicked()
{
    if (m_player.playPrevious()) {
        int index = m_ui->playlistWidget->currentRow() - 1;
        m_ui->playlistWidget->setCurrentRow(index);
        m_ui->playingEdit->setText(musicName(m_playlist[index]));
    } /* No need to warn because player emits a warning signal and it's caught by this class. */
}

void MainWindow::onNextButtonClicked()
{
    if (m_player.playNext()) {
        int index = m_ui->playlistWidget->currentRow() + 1;
        m_ui->playlistWidget->setCurrentRow(index);
        m_ui->playingEdit->setText(musicName(m_playlist[index]));
        m_ui->playButton->setText(tr("Pause"));
        m_ui->playButton->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackPause));
    } else {
        QMessageBox::warning(this, tr("Warning"), tr("There's no next music to play."));
    }
}

void MainWindow::onAutoRepeatButtonClicked()
{
    auto text = m_ui->autoRepeatButton->text();

    if (text.isEmpty()) {
        m_autorepeat = AUTOREPEAT::ONE;
        m_ui->autoRepeatButton->setText(tr("1"));
        m_ui->autoRepeatButton->setToolTip(tr("Current music will repeat forever."));
    } else if (text.contains(tr("1"))) {
        m_autorepeat = AUTOREPEAT::ALL;
        m_ui->autoRepeatButton->setText(tr("All"));
        m_ui->autoRepeatButton->setToolTip(tr("Current playlist will repeat forever."));
    } else {
        m_autorepeat = AUTOREPEAT::NONE;
        m_ui->autoRepeatButton->setText("");
        m_ui->autoRepeatButton->setToolTip(tr("Neither current music nor current playlist repeats."));
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

    setVolumeIcon();
}

void MainWindow::onVolumeIconButtonClicked()
{
    int level = m_ui->volumeSlider->value();
    /* Second condition is to allow to cycle on press */
    if (level < 25 or level == 100) {
        level = 25;
    } else if (level < 50) {
        level = 50;
    } else if (level < 75) {
        level = 75;
    } else {
        level = 100;
    }

    m_ui->volumeSlider->setValue(level);
}

void MainWindow::onVolumeIncrease()
{
    auto *snder = qobject_cast<QShortcut *>(sender());
    if (snder == nullptr) {
        return;
    }

    int level = m_ui->volumeSlider->value();
    level += snder == m_increaseVolumeBy5Shortcut ? 5 : 10;

    if (level > 100) {
        level = 100;
    }

    m_ui->volumeSlider->setValue(level);
}

void MainWindow::onVolumeDecrease()
{
    auto *snder = qobject_cast<QShortcut *>(sender());
    if (snder == nullptr) {
        return;
    }

    int level = m_ui->volumeSlider->value();
    level -= snder == m_decreaseVolumeBy5Shortcut ? 5 : 10;

    if (level < 0) {
        level = 0;
    }

    m_ui->volumeSlider->setValue(level);
}

void MainWindow::about()
{
    auto text = QString("\
<h1>%1 %2</h1>\
<p>License: %3<br>\
Authors: %4<br>\
<a href=\"%5\">Main Page</a><br>\
Build Date: %6</p>").arg(PROJECT_NAME, PROJECT_VERSION, PROJECT_LICENSE, AUTHORS, PROJECT_WEBPAGE, BUILD_DATETIME);
    QMessageBox messageBox;
    messageBox.setWindowTitle(tr("About - %1").arg(PROJECT_NAME));
    messageBox.setTextFormat(Qt::RichText);
    messageBox.setText(text);
    messageBox.exec();
}
