#include "settings.hpp"
#include "ui_settings.h"
#include "config.hpp"

#include <QAudioDevice>
#include <QDir>
#include <QIntValidator>
#include <QMediaDevices>
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
    setWindowTitle(tr("%1 - Settings").arg(PROJECT_NAME));

    auto *widthValidator = new QIntValidator(0, screen()->geometry().width(), this);
    auto *heightValidator = new QIntValidator(0, screen()->geometry().height(), this);
    auto *volumeValidator = new QIntValidator(0, 100, this);

    m_ui->widthEdit->setValidator(widthValidator);
    m_ui->heightEdit->setValidator(heightValidator);
    m_ui->volumeLevelEdit->setValidator(volumeValidator);
    m_ui->applySettingsButton->setEnabled(false);

    m_ui->centeredCheckBox->setToolTip(
        tr("If checked, floating window will always be opened "
           "at the center of the screen.")
    );

    m_ui->alwaysMaximizedCheckBox->setToolTip(
        tr("If checked, window will always be opened maximized.")
    );

    m_ui->minimizeToSystrayCheckBox->setToolTip(
        tr("If checked, the player will be minimized to the system tray "
           "when asking to close, for example, by pressing the close button.")
    );

    m_ui->hideControlsAtStartupCheckBox->setToolTip(
        tr("Hide playlist and buttons controlling it by default "
           "which can be shown again from the menu bar.")
    );

    m_playlistSettings = new QSettings(
        createEnvironment(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)),
        QSettings::IniFormat,
        this
    );

    m_settings = new QSettings(createEnvironment(), QSettings::IniFormat, this);
    m_settings->beginGroup("WindowSettings");

#ifdef ENABLE_VIDEO_PLAYER
    m_ui->hideControlsOnVideoCheckBox->setToolTip(
        tr("Hide playlist and buttons controlling it when opening a video "
           "which can be shown again from the menu bar.")
    );
    m_ui->hideControlsOnVideoCheckBox->setChecked(
        m_settings->value("HideControlsOnVideo", false).toBool()
    );
#else
    m_ui->hideControlsOnVideoCheckBox->setVisible(false);
#endif

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

    m_ui->hideControlsAtStartupCheckBox->setChecked(m_settings->value("HideControlsAtStartup", false).toBool());

    m_ui->defaultLanguageComboBox->addItems({
        "",
        tr("English"),
        tr("Español"),
    });

    auto defaultLanguage = m_settings->value("DefaultLanguage", "").toString();
    m_ui->defaultLanguageComboBox->setCurrentText(defaultLanguage);

    m_settings->endGroup();

    if (not defaultLanguage.isEmpty() and m_ui->defaultLanguageComboBox->currentText().isEmpty()) {
        qCritical().noquote()
            << tr("Language: %1 found in settings doesn't "
                  "correspond to any supported at the moment.").arg(defaultLanguage);
    }

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

    m_settings->beginGroup("PlaylistSettings");
    bool rememberLastSong = m_settings->value("RememberLastSong", false).toBool();
    m_settings->endGroup();
    m_ui->rememberLastSongCheckBox->setChecked(rememberLastSong);

    loadPlaylists();
    setAudioOutputs();
    setInitialValues();

    if (m_ui->defaultPlaylistComboBox->currentIndex() == 0)
        m_ui->rememberLastSongCheckBox->setEnabled(false);

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

    connect(
        m_ui->hideControlsAtStartupCheckBox,
        &QCheckBox::checkStateChanged,
        this,
        &Settings::checkForChange
    );

    connect(
        m_ui->hideControlsOnVideoCheckBox,
        &QCheckBox::checkStateChanged,
        this,
        &Settings::checkForChange
    );

    connect(
        m_ui->defaultLanguageComboBox,
        &QComboBox::currentIndexChanged,
        this,
        &Settings::checkForChange
    );

    connect(m_ui->widthEdit, &QLineEdit::textChanged, this, &Settings::checkForChange);
    connect(m_ui->heightEdit, &QLineEdit::textChanged, this, &Settings::checkForChange);
    connect(m_ui->audioOutputsCombo, &QComboBox::currentIndexChanged, this, &Settings::checkForChange);
    connect(m_ui->applySettingsButton, &QPushButton::clicked, this, &Settings::applyChanges);

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
        [this] (int index) {
            if (index == 0)
                m_ui->rememberLastSongCheckBox->setEnabled(false);
            else
                m_ui->rememberLastSongCheckBox->setEnabled(true);
            checkForChange();
        }
    );

    connect(
        m_ui->rememberLastSongCheckBox,
        &QCheckBox::checkStateChanged,
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

void Settings::setAudioOutputs()
{
    m_ui->audioOutputsCombo->addItem("");
    m_settings->beginGroup("AudioSettings");
    auto defaultAudioOutput = m_settings->value("DefaultAudioOutput", "").toString();
    m_settings->endGroup();

    int i = 0;
    int currentIndex = 0;
    for (const auto &device : QMediaDevices::audioOutputs()) {
        ++i;
        m_ui->audioOutputsCombo->addItem(device.description());
        if (device.id() == defaultAudioOutput)
            currentIndex = i;
    }

    m_ui->audioOutputsCombo->setCurrentIndex(currentIndex);
}

void Settings::setInitialValues()
{
    m_initialCheckBoxesValues[m_ui->rememberWindowSizeCheckBox] = m_ui->rememberWindowSizeCheckBox->isChecked();
    m_initialCheckBoxesValues[m_ui->centeredCheckBox] = m_ui->centeredCheckBox->isChecked();
    m_initialCheckBoxesValues[m_ui->alwaysMaximizedCheckBox] = m_ui->alwaysMaximizedCheckBox->isChecked();
    m_initialCheckBoxesValues[m_ui->minimizeToSystrayCheckBox] = m_ui->minimizeToSystrayCheckBox->isChecked();
    m_initialCheckBoxesValues[m_ui->hideControlsAtStartupCheckBox] = m_ui->hideControlsAtStartupCheckBox->isChecked();
#ifdef ENABLE_VIDEO_PLAYER
    m_initialCheckBoxesValues[m_ui->hideControlsOnVideoCheckBox] = m_ui->hideControlsOnVideoCheckBox->isChecked();
#endif
    m_initialCheckBoxesValues[m_ui->rememberVolumeLevelCheckBox] = m_ui->rememberVolumeLevelCheckBox->isChecked();
    m_initialCheckBoxesValues[m_ui->rememberLastSongCheckBox] = m_ui->rememberLastSongCheckBox->isChecked();

    m_initialFieldValues[m_ui->widthEdit] = m_ui->widthEdit->text();
    m_initialFieldValues[m_ui->heightEdit] = m_ui->heightEdit->text();
    m_initialFieldValues[m_ui->volumeLevelEdit] = m_ui->volumeLevelEdit->text();

    m_initialComboBoxValues[m_ui->defaultPlaylistComboBox] = m_ui->defaultPlaylistComboBox->currentIndex();
    m_initialComboBoxValues[m_ui->audioOutputsCombo] = m_ui->audioOutputsCombo->currentIndex();
    m_initialComboBoxValues[m_ui->defaultLanguageComboBox] = m_ui->defaultLanguageComboBox->currentIndex();
}

/* Works, but is it the best way to check for a change? */
void Settings::checkForChange()
{
    bool changed {false};
    if (m_initialCheckBoxesValues[m_ui->rememberWindowSizeCheckBox] != m_ui->rememberWindowSizeCheckBox->isChecked()) {
        m_ui->applySettingsButton->setEnabled(true);
        changed = true;
        goto exit;
    }

    if (m_initialCheckBoxesValues[m_ui->centeredCheckBox] != m_ui->centeredCheckBox->isChecked()) {
        m_ui->applySettingsButton->setEnabled(true);
        changed = true;
        goto exit;
    }

    if (m_initialCheckBoxesValues[m_ui->alwaysMaximizedCheckBox] != m_ui->alwaysMaximizedCheckBox->isChecked()) {
        m_ui->applySettingsButton->setEnabled(true);
        changed = true;
        goto exit;
    }

    if (m_initialCheckBoxesValues[m_ui->minimizeToSystrayCheckBox] != m_ui->minimizeToSystrayCheckBox->isChecked()) {
        m_ui->applySettingsButton->setEnabled(true);
        changed = true;
        goto exit;
    }

    if (m_initialCheckBoxesValues[m_ui->hideControlsAtStartupCheckBox] != m_ui->hideControlsAtStartupCheckBox->isChecked()) {
        m_ui->applySettingsButton->setEnabled(true);
        changed = true;
        goto exit;
    }

#ifdef ENABLE_VIDEO_PLAYER
    if (m_initialCheckBoxesValues[m_ui->hideControlsOnVideoCheckBox] != m_ui->hideControlsOnVideoCheckBox->isChecked()) {
        m_ui->applySettingsButton->setEnabled(true);
        changed = true;
        goto exit;
    }
#endif

    if (m_initialCheckBoxesValues[m_ui->rememberVolumeLevelCheckBox] != m_ui->rememberVolumeLevelCheckBox->isChecked()) {
        m_ui->applySettingsButton->setEnabled(true);
        changed = true;
        goto exit;
    }

    if (m_initialCheckBoxesValues[m_ui->rememberLastSongCheckBox] != m_ui->rememberLastSongCheckBox->isChecked()) {
        m_ui->applySettingsButton->setEnabled(true);
        changed = true;
        goto exit;
    }

    if (m_initialFieldValues[m_ui->widthEdit] != m_ui->widthEdit->text()) {
        m_ui->applySettingsButton->setEnabled(true);
        changed = true;
        goto exit;
    }

    if (m_initialFieldValues[m_ui->heightEdit] != m_ui->heightEdit->text()) {
        m_ui->applySettingsButton->setEnabled(true);
        changed = true;
        goto exit;
    }

    if (m_initialFieldValues[m_ui->volumeLevelEdit] != m_ui->volumeLevelEdit->text()) {
        m_ui->applySettingsButton->setEnabled(true);
        changed = true;
        goto exit;
    }

    if (m_initialComboBoxValues[m_ui->defaultPlaylistComboBox] != m_ui->defaultPlaylistComboBox->currentIndex()) {
        m_ui->applySettingsButton->setEnabled(true);
        changed = true;
        goto exit;
    }

    if (m_initialComboBoxValues[m_ui->audioOutputsCombo] != m_ui->audioOutputsCombo->currentIndex()) {
        m_ui->applySettingsButton->setEnabled(true);
        changed = true;
        goto exit;
    }

    if (m_initialComboBoxValues[m_ui->defaultLanguageComboBox] != m_ui->defaultLanguageComboBox->currentIndex()) {
        m_ui->applySettingsButton->setEnabled(true);
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

        m_ui->applySettingsButton->setEnabled(false);
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
    bool hideControlsAtStartup = m_ui->hideControlsAtStartupCheckBox->isChecked();
#ifdef ENABLE_VIDEO_PLAYER
    bool hideControlsOnVideo = m_ui->hideControlsOnVideoCheckBox->isChecked();
#endif
    QString defaultLanguage = m_ui->defaultLanguageComboBox->currentText();
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
    m_settings->setValue("HideControlsAtStartup", hideControlsAtStartup);
#ifdef ENABLE_VIDEO_PLAYER
    m_settings->setValue("HideControlsOnVideo", hideControlsOnVideo);
#endif

    if (width >= 0)
        m_settings->setValue("Width", width);
    if (height >= 0)
        m_settings->setValue("Height", height);

    m_settings->setValue("DefaultLanguage", defaultLanguage);

    m_settings->endGroup();

    m_settings->beginGroup("AudioSettings");
    m_settings->setValue("RememberVolumeLevel", rememberVolumeLevel);
    if (volumeLevel >= 0)
        m_settings->setValue("VolumeLevel", volumeLevel);

    QString id {};
    for (const auto &device : QMediaDevices::audioOutputs())
        if (device.description() == m_ui->audioOutputsCombo->currentText())
            id = device.id();
    m_settings->setValue("DefaultAudioOutput", id);
    m_settings->endGroup();

    m_settings->beginGroup("PlaylistSettings");
    if (m_initialComboBoxValues[m_ui->defaultPlaylistComboBox] != m_ui->defaultPlaylistComboBox->currentIndex()) {

        /* Don't translate this value for setting. */
        auto name = m_ui->defaultPlaylistComboBox->currentText();
        if (name == tr("None")) {
            name = "None";
        }

        m_settings->setValue("DefaultPlaylist", name);
    }

    m_settings->setValue("RememberLastSong", m_ui->rememberLastSongCheckBox->isChecked());
    m_settings->endGroup();

    setWindowTitle(windowTitle().mid(0, windowTitle().indexOf('*')));
    setInitialValues();
    m_modified = false;
    m_changesApplied = true;
    m_ui->applySettingsButton->setEnabled(m_modified);
}
