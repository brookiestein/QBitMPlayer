#include "settings.hpp"
#include "ui_settings.h"
#include "config.hpp"

#include <QDir>
#include <QIntValidator>
#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>

Settings::Settings(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::Settings)
    , m_modified {false}
    , m_changesApplied {false}
    , m_quitShortcut {new QShortcut(QKeySequence(Qt::Key_Escape), this)}
{
    m_ui->setupUi(this);
    m_ui->tabWidget->setTabText(0, tr("Window"));
    m_ui->tabWidget->setTabText(1, tr("Audio"));
    m_ui->tabWidget->setTabText(2, tr("Playlist"));
    m_ui->tabWidget->setCurrentIndex(0);
    setWindowTitle(tr("%1 - Settings").arg(PROJECT_NAME));

    auto *widthValidator = new QIntValidator(0, screen()->geometry().width(), this);
    auto *heightValidator = new QIntValidator(0, screen()->geometry().height(), this);
    auto *volumeValidator = new QIntValidator(0, 100, this);

    m_ui->widthEdit->setValidator(widthValidator);
    m_ui->heightEdit->setValidator(heightValidator);
    m_ui->volumeLevelEdit->setValidator(volumeValidator);
    m_ui->applyWindowSettingsButton->setEnabled(false);
    m_ui->applyAudioSettingsButton->setEnabled(false);
    m_ui->applyPlaylistSettingsButton->setEnabled(false);

    m_ui->centeredCheckBox->setToolTip(
        tr("If checked, floating window will always be opened at the center of the screen.")
    );

    m_ui->alwaysMaximizedCheckBox->setToolTip(
        tr("If checked, window will always be opened maximized.")
    );

    m_ui->minimizeToSystrayCheckBox->setToolTip(
        tr("If checked, the player will be minimized to the system tray "
           "when asking to close, for example, by pressing the close button.")
    );

    m_playlistSettings = new QSettings(
        createEnvironment(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)),
        QSettings::IniFormat,
        this
    );

    m_settings = new QSettings(createEnvironment(), QSettings::IniFormat, this);
    m_settings->beginGroup("WindowSettings");

    auto state = m_settings->value("RememberWindowSize", false).toBool() ? Qt::Checked : Qt::Unchecked;
    m_ui->rememberWindowSizeCheckBox->setCheckState(state);

    state = m_settings->value("Centered", false).toBool() ? Qt::Checked : Qt::Unchecked;
    m_ui->centeredCheckBox->setCheckState(state);

    state = m_settings->value("AlwaysMaximized", false).toBool() ? Qt::Checked : Qt::Unchecked;
    m_ui->alwaysMaximizedCheckBox->setCheckState(state);

    state = m_settings->value("MinimizeToSystray", false).toBool() ? Qt::Checked : Qt::Unchecked;
    m_ui->minimizeToSystrayCheckBox->setCheckState(state);

    /* AlwaysMaximized state overwrites Centered */
    if (m_ui->alwaysMaximizedCheckBox->isChecked()) {
        m_ui->centeredCheckBox->setEnabled(false);
        m_ui->centeredCheckBox->setCheckState(Qt::Unchecked);
    }

    int width = m_settings->value("Width", geometry().width()).toInt();
    int height = m_settings->value("Height", geometry().height()).toInt();
    m_settings->endGroup();

    m_ui->widthEdit->setText(QString::number(width));
    m_ui->heightEdit->setText(QString::number(height));

    m_settings->beginGroup("AudioSettings");
    state = m_settings->value("RememberVolumeLevel", false).toBool() ? Qt::Checked : Qt::Unchecked;
    m_ui->rememberVolumeLevelCheckBox->setCheckState(state);
    m_ui->volumeLevelEdit->setText(m_settings->value("VolumeLevel", "50").toString());
    m_settings->endGroup();

    if (m_ui->rememberVolumeLevelCheckBox->isChecked()) {
        m_ui->volumeLevelEdit->setEnabled(false);
    }

    bool checked = m_ui->rememberWindowSizeCheckBox->isChecked();
    m_ui->widthEdit->setEnabled(not checked);
    m_ui->heightEdit->setEnabled(not checked);

    auto stateText = tr("Changes will apply on reboot.");
    m_ui->windowStateLabel->setText(stateText);
    m_ui->audioStateLabel->setText(stateText);
    m_ui->playlistStateLabel->setText(stateText);

    loadPlaylists();
    setInitialValues();

    connect(m_ui->tabWidget, &QTabWidget::currentChanged, this, &Settings::onCurrentChanged);

    connect(
        m_ui->rememberWindowSizeCheckBox,
        &QCheckBox::checkStateChanged,
        this,
        &Settings::onRememberWindowSizeChecked
    );

    connect(
        m_ui->centeredCheckBox,
        &QCheckBox::checkStateChanged,
        this,
        &Settings::checkForChange
    );

    connect(
        m_ui->alwaysMaximizedCheckBox,
        &QCheckBox::checkStateChanged,
        this,
        &Settings::onAlwaysMaximizedChecked
    );

    connect(
        m_ui->minimizeToSystrayCheckBox,
        &QCheckBox::checkStateChanged,
        this,
        &Settings::onMinimizeToSystrayChecked
    );

    connect(m_ui->widthEdit, &QLineEdit::textChanged, this, &Settings::checkForChange);
    connect(m_ui->heightEdit, &QLineEdit::textChanged, this, &Settings::checkForChange);
    connect(m_ui->applyWindowSettingsButton, &QPushButton::clicked, this, &Settings::applyChanges);
    connect(m_ui->applyAudioSettingsButton, &QPushButton::clicked, this, &Settings::applyChanges);
    connect(m_ui->applyPlaylistSettingsButton, &QPushButton::clicked, this, &Settings::applyChanges);

    connect(
        m_ui->rememberVolumeLevelCheckBox,
        &QCheckBox::checkStateChanged,
        this,
        &Settings::onRememberVolumeLevelChecked
    );

    connect(m_ui->volumeLevelEdit, &QLineEdit::textChanged, this, &Settings::checkForChange);

    connect(
        m_ui->defaultPlaylistComboBox,
        &QComboBox::currentIndexChanged,
        this,
        &Settings::checkForChange
    );

    connect(m_quitShortcut, &QShortcut::activated, this, &QWidget::close);
}

Settings::~Settings()
{
    delete m_ui;
}

void Settings::closeEvent(QCloseEvent *event)
{
    if (m_modified) {
        auto reply = QMessageBox::question(this,
                                           tr("Unsaved changes"),
                                           tr("There are unsaved changes. Would you like to save them?"));
        if (reply == QMessageBox::Yes) {
            applyChanges();
        }
    }

    if (m_changesApplied) {
        auto reply = QMessageBox::question(this,
                                           tr("Settings Saved"),
                                           tr("New settings have been saved. "
                                              "Would you like to reboot to apply them?"));
        if (reply != QMessageBox::Yes) {
            QMessageBox::information(this,
                                     tr("Information"),
                                     tr("Changes will take effect on next start."));
        } else {
            auto *app = QApplication::instance();
            app->quit();
            QProcess::startDetached(app->arguments()[0], app->arguments().mid(1));
        }
    }

    emit closed();
    QWidget::closeEvent(event);
}

void Settings::onCurrentChanged(int index)
{
    if (not windowTitle().contains('*')) {
        return;
    }

    switch (index)
    {
    case 0:
        m_ui->applyWindowSettingsButton->setEnabled(true);
        break;
    case 1:
        m_ui->applyAudioSettingsButton->setEnabled(true);
        break;
    case 2:
        m_ui->applyPlaylistSettingsButton->setEnabled(true);
        break;
    }
}

QString Settings::createEnvironment(const QString &location)
{
    QString configLocation;

    if (not location.endsWith(PROJECT_NAME)) {
        configLocation = QString("%1%2%3").arg(location, QDir::separator(), PROJECT_NAME);
    } else {
        configLocation = location;
    }

    QDir d;
    if (not d.exists(configLocation)) {
        d.mkpath(configLocation);
    }

    auto configFile = QString("%1%2%3").arg(configLocation, QDir::separator(), PROJECT_NAME".conf");
    return configFile;
}

void Settings::loadPlaylists()
{
    m_playlistSettings->beginGroup("Playlists");

    m_ui->defaultPlaylistComboBox->addItem(tr("None"));
    for (const auto &playlist : m_playlistSettings->childGroups()) {
        m_ui->defaultPlaylistComboBox->addItem(playlist);
    }

    m_playlistSettings->endGroup();

    m_settings->beginGroup("PlaylistSettings");
    auto playlistName = m_settings->value("DefaultPlaylist", "").toString();
    m_settings->endGroup();

    if (not playlistName.isEmpty()) {
        m_ui->defaultPlaylistComboBox->setCurrentText(playlistName);
    }
}

void Settings::setInitialValues()
{
    m_initialCheckBoxesValues[m_ui->rememberWindowSizeCheckBox] = m_ui->rememberWindowSizeCheckBox->isChecked();
    m_initialCheckBoxesValues[m_ui->centeredCheckBox] = m_ui->centeredCheckBox->isChecked();
    m_initialCheckBoxesValues[m_ui->alwaysMaximizedCheckBox] = m_ui->alwaysMaximizedCheckBox->isChecked();
    m_initialCheckBoxesValues[m_ui->minimizeToSystrayCheckBox] = m_ui->minimizeToSystrayCheckBox->isChecked();
    m_initialCheckBoxesValues[m_ui->rememberVolumeLevelCheckBox] = m_ui->rememberVolumeLevelCheckBox->isChecked();

    m_initialFieldValues[m_ui->widthEdit] = m_ui->widthEdit->text();
    m_initialFieldValues[m_ui->heightEdit] = m_ui->heightEdit->text();
    m_initialFieldValues[m_ui->volumeLevelEdit] = m_ui->volumeLevelEdit->text();

    m_initialComboBoxValues[m_ui->defaultPlaylistComboBox] = m_ui->defaultPlaylistComboBox->currentIndex();
}

/* Works, but is it the best way to check for a change? */
void Settings::checkForChange()
{
    bool changed {false};
    if (m_initialCheckBoxesValues[m_ui->rememberWindowSizeCheckBox] != m_ui->rememberWindowSizeCheckBox->isChecked()) {
        m_ui->applyWindowSettingsButton->setEnabled(true);
        changed = true;
        goto exit;
    }

    if (m_initialCheckBoxesValues[m_ui->centeredCheckBox] != m_ui->centeredCheckBox->isChecked()) {
        m_ui->applyWindowSettingsButton->setEnabled(true);
        changed = true;
        goto exit;
    }

    if (m_initialCheckBoxesValues[m_ui->alwaysMaximizedCheckBox] != m_ui->alwaysMaximizedCheckBox->isChecked()) {
        m_ui->applyWindowSettingsButton->setEnabled(true);
        changed = true;
        goto exit;
    }

    if (m_initialCheckBoxesValues[m_ui->minimizeToSystrayCheckBox] != m_ui->minimizeToSystrayCheckBox->isChecked()) {
        m_ui->applyWindowSettingsButton->setEnabled(true);
        changed = true;
        goto exit;
    }

    if (m_initialCheckBoxesValues[m_ui->rememberVolumeLevelCheckBox] != m_ui->rememberVolumeLevelCheckBox->isChecked()) {
        m_ui->applyAudioSettingsButton->setEnabled(true);
        changed = true;
        goto exit;
    }

    if (m_initialFieldValues[m_ui->widthEdit] != m_ui->widthEdit->text()) {
        m_ui->applyWindowSettingsButton->setEnabled(true);
        changed = true;
        goto exit;
    }

    if (m_initialFieldValues[m_ui->heightEdit] != m_ui->heightEdit->text()) {
        m_ui->applyWindowSettingsButton->setEnabled(true);
        changed = true;
        goto exit;
    }

    if (m_initialFieldValues[m_ui->volumeLevelEdit] != m_ui->volumeLevelEdit->text()) {
        m_ui->applyAudioSettingsButton->setEnabled(true);
        changed = true;
        goto exit;
    }

    if (m_initialComboBoxValues[m_ui->defaultPlaylistComboBox] != m_ui->defaultPlaylistComboBox->currentIndex()) {
        m_ui->applyPlaylistSettingsButton->setEnabled(true);
        changed = true;
        goto exit;
    }

exit:
    m_modified = changed;
    auto title = windowTitle();
    if (m_modified) {
        if (not title.contains("*")) {
            setWindowTitle(QString("%1*").arg(title));
        }
    } else {
        if (title.contains("*")) {
            title = title.mid(0, title.indexOf('*'));
            setWindowTitle(title);
        }

        m_ui->applyWindowSettingsButton->setEnabled(false);
        m_ui->applyAudioSettingsButton->setEnabled(false);
        m_ui->applyPlaylistSettingsButton->setEnabled(false);
    }
}

void Settings::onRememberWindowSizeChecked(Qt::CheckState state)
{
    bool alwaysMaximized = m_ui->alwaysMaximizedCheckBox->isChecked();
    switch (state)
    {
    case Qt::Unchecked:
        m_ui->widthEdit->setEnabled(not alwaysMaximized);
        m_ui->heightEdit->setEnabled(not alwaysMaximized);
        break;
    case Qt::PartiallyChecked: [[fallthrough]];
    case Qt::Checked:
        m_ui->alwaysMaximizedCheckBox->setChecked(false);
        m_ui->widthEdit->setEnabled(false);
        m_ui->heightEdit->setEnabled(false);
        break;
    }

    checkForChange();
}

void Settings::onAlwaysMaximizedChecked(Qt::CheckState state)
{
    switch (state)
    {
    case Qt::Unchecked:
        if (not m_ui->rememberWindowSizeCheckBox->isChecked()) {
            m_ui->widthEdit->setEnabled(true);
            m_ui->heightEdit->setEnabled(true);
        }

        m_ui->centeredCheckBox->setEnabled(true);
        break;
    case Qt::PartiallyChecked: [[fallthrough]];
    case Qt::Checked:
        m_ui->rememberWindowSizeCheckBox->setChecked(false);
        m_ui->centeredCheckBox->setChecked(false);
        m_ui->centeredCheckBox->setEnabled(false);
        m_ui->widthEdit->setEnabled(false);
        m_ui->heightEdit->setEnabled(false);
        break;
    }

    checkForChange();
}

void Settings::onMinimizeToSystrayChecked(Qt::CheckState state)
{
    checkForChange();
}

void Settings::onRememberVolumeLevelChecked(Qt::CheckState state)
{
    switch (state)
    {
    case Qt::Unchecked:
        m_ui->volumeLevelEdit->setEnabled(true);
        break;
    case Qt::PartiallyChecked: [[fallthrough]];
    case Qt::Checked:
        m_ui->volumeLevelEdit->setEnabled(false);
        break;
    }

    checkForChange();
}

void Settings::applyChanges()
{
    if (not m_modified) {
        return;
    }

    m_settings->beginGroup("WindowSettings");
    bool rememberWindowSize = m_ui->rememberWindowSizeCheckBox->isChecked();
    bool centered = m_ui->centeredCheckBox->isChecked();
    bool alwaysMaximized = m_ui->alwaysMaximizedCheckBox->isChecked();
    bool minimizeToSystray = m_ui->minimizeToSystrayCheckBox->isChecked();
    bool rememberVolumeLevel = m_ui->rememberVolumeLevelCheckBox->isChecked();
    int width {-1};
    int height {-1};
    int volumeLevel {-1};

    if (not rememberWindowSize) {
        const auto &widthText = m_ui->widthEdit->text();
        const auto &heightText = m_ui->heightEdit->text();
        if (widthText.isEmpty()) {
            m_settings->endGroup();
            QMessageBox::warning(this, tr("Width Required"), tr("Width is required."));
            return;
        }

        if (heightText.isEmpty()) {
            m_settings->endGroup();
            QMessageBox::warning(this, tr("Height Required"), tr("Height is required."));
            return;
        }

        width = widthText.toInt();
        height = heightText.toInt();

        if (width > screen()->geometry().width()) {
            m_settings->endGroup();
            QMessageBox::warning(this, tr("Too High"), tr("Width is too high."));
            return;
        }

        if (height > screen()->geometry().height()) {
            m_settings->endGroup();
            QMessageBox::warning(this, tr("Too High"), tr("Height is too high."));
            return;
        }
    }

    if (not rememberVolumeLevel) {
        const auto &volumeLevelText = m_ui->volumeLevelEdit->text();
        if (volumeLevelText.isEmpty()) {
            m_settings->endGroup();
            QMessageBox::warning(this,
                                 tr("Volume Level Required"),
                                 tr("The volume level is required.")
            );
            return;
        }

        volumeLevel = volumeLevelText.toInt();
        if (volumeLevel > 100) {
            m_settings->endGroup();
            QMessageBox::warning(this, tr("Too High"), tr("Volume level is too high."));
            return;
        }
    }

    m_settings->setValue("RememberWindowSize", rememberWindowSize);
    m_settings->setValue("Centered", centered);
    m_settings->setValue("AlwaysMaximized", alwaysMaximized);
    m_settings->setValue("MinimizeToSystray", minimizeToSystray);
    if (width >= 0)
        m_settings->setValue("Width", width);
    if (height >= 0)
        m_settings->setValue("Height", height);
    m_settings->endGroup();

    m_settings->beginGroup("AudioSettings");
    m_settings->setValue("RememberVolumeLevel", rememberVolumeLevel);
    if (volumeLevel >= 0)
        m_settings->setValue("VolumeLevel", volumeLevel);
    m_settings->endGroup();

    if (m_initialComboBoxValues[m_ui->defaultPlaylistComboBox] != m_ui->defaultPlaylistComboBox->currentIndex()) {
        m_settings->beginGroup("PlaylistSettings");

        /* Don't translate this value for setting. */
        auto name = m_ui->defaultPlaylistComboBox->currentText();
        if (name == tr("None")) {
            name = "None";
        }

        m_settings->setValue("DefaultPlaylist", name);
        m_settings->endGroup();
    }

    setWindowTitle(windowTitle().mid(0, windowTitle().indexOf('*')));
    setInitialValues();
    m_modified = false;
    m_changesApplied = true;
    m_ui->applyWindowSettingsButton->setEnabled(m_modified);
    m_ui->applyAudioSettingsButton->setEnabled(m_modified);
    m_ui->applyPlaylistSettingsButton->setEnabled(m_modified);
}
