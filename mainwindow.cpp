#include "mainwindow.hpp"
#include "./ui_mainwindow.h"

#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>

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

    connect(&m_player, &Player::error, this, &MainWindow::error);
    connect(&m_player, &Player::warning, this, &MainWindow::warning);
    connect(&m_player, &Player::durationChanged, this, &MainWindow::durationChanged);
    connect(&m_player, &Player::positionChanged, this, &MainWindow::positionChanged);
    connect(&m_player, &Player::finished, this, &MainWindow::finished);
    connect(m_ui->openSingleFileButton, &QPushButton::clicked, this, &MainWindow::onOpenSingleFileButtonClicked);
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
    if (m_autoRepeatType != AUTOREPEAT_TYPE::ONCE) {
        /* This call will play next music if there's any, if not returns false,
         * allowing us to reset the widgets. */
        if (not m_player.playNext()) {
            m_ui->playButton->setText(tr("Play"));
            m_ui->seekMusicSlider->setValue(0);
            m_ui->durationLabel->setText(QString("0/%1:%2:%3")
                                             .arg(QString::number(m_hours),
                                                  QString::number(m_minutes),
                                                  QString::number(m_seconds))
                                         );
        }

        return;
    }

    if (not hasRepeated) {
        m_player.play();
        hasRepeated = true;
    }
}

void MainWindow::onOpenSingleFileButtonClicked()
{
    auto dir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    m_currentMusic = QFileDialog::getOpenFileName(this,
                                                  tr("Open Audio File"),
                                                  dir,
                                                  tr("Audio files (*.mp3)")
                                                  );

    if (m_currentMusic.isEmpty()) {
        return;
    }

    m_ui->playingEdit->setText(m_currentMusic);
    m_player.setPlayList({ m_currentMusic });
    qDebug() << "Loaded music file:" << m_currentMusic;
    hasRepeated = false;
}

void MainWindow::onPlayButtonClicked()
{
    auto *snder = qobject_cast<QPushButton *>(sender());
    /* tr() seems to inserts an & somewhere in the text.
     * I find easier just to check if text is not &Pause than
     * looking where the & is. */
    if (snder->text() == tr("&Play") or snder->text() != tr("&Pause")) {
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
