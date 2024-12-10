#include "settings.hpp"
#include "ui_settings.h"
#include "config.hpp"

#include <QDir>
#include <QIntValidator>
#include <QMessageBox>
#include <QStandardPaths>

Settings::Settings(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::Settings)
    , m_modified(false)
{
    m_ui->setupUi(this);
    m_ui->tabWidget->setTabText(0, tr("Window"));
    m_ui->tabWidget->setTabText(1, tr("Audio"));
    m_ui->tabWidget->setCurrentIndex(0);
    setWindowTitle(tr("%1 - Settings").arg(PROJECT_NAME));

    auto *widthValidator = new QIntValidator(0, screen()->geometry().width(), this);
    auto *heightValidator = new QIntValidator(0, screen()->geometry().height(), this);
    auto *volumeValidator = new QIntValidator(0, 100, this);

    m_ui->widthEdit->setValidator(widthValidator);
    m_ui->heightEdit->setValidator(heightValidator);
    m_ui->volumeLevelEdit->setValidator(volumeValidator);

    m_settings = new QSettings(createEnvironment(), QSettings::IniFormat, this);
    m_settings->beginGroup("WindowSettings");

    auto state = m_settings->value("RememberWindowSize", false).toBool() ? Qt::Checked : Qt::Unchecked;
    m_ui->rememberWindowSizeCheckBox->setCheckState(state);
    state = m_settings->value("AlwaysMaximized", false).toBool() ? Qt::Checked : Qt::Unchecked;
    m_ui->alwaysMaximizedCheckBox->setCheckState(state);

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

    connect(
        m_ui->rememberWindowSizeCheckBox,
        &QCheckBox::checkStateChanged,
        this,
        &Settings::onRememberWindowSizeChecked
    );

    connect(
        m_ui->alwaysMaximizedCheckBox,
        &QCheckBox::checkStateChanged,
        this,
        &Settings::onAlwaysMaximizedChecked
    );

    connect(m_ui->widthEdit, &QLineEdit::textChanged, this, &Settings::onTextChanged);
    connect(m_ui->heightEdit, &QLineEdit::textChanged, this, &Settings::onTextChanged);
    connect(m_ui->applyWindowSettingsButton, &QPushButton::clicked, this, &Settings::applyChanges);
    connect(m_ui->applyAudioSettingsButton, &QPushButton::clicked, this, &Settings::applyChanges);

    connect(
        m_ui->rememberVolumeLevelCheckBox,
        &QCheckBox::checkStateChanged,
        this,
        &Settings::onRememberVolumeLevelChecked
    );

    connect(m_ui->volumeLevelEdit, &QLineEdit::textChanged, this, &Settings::onTextChanged);
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

    emit closed();
    QWidget::closeEvent(event);
}

QString Settings::createEnvironment()
{
    auto configLocation = QString("%1%2%3")
                              .arg(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation),
                                   QDir::separator(), PROJECT_NAME);
    QDir d;
    if (not d.exists(configLocation)) {
        d.mkpath(configLocation);
    }

    auto configFile = QString("%1%2%3").arg(configLocation, QDir::separator(), PROJECT_NAME".conf");
    return configFile;
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

    if (not m_modified) {
        setWindowTitle(windowTitle() + "*");
        m_modified = true;
    }
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
        break;
    case Qt::PartiallyChecked: [[fallthrough]];
    case Qt::Checked:
        m_ui->rememberWindowSizeCheckBox->setChecked(false);
        m_ui->widthEdit->setEnabled(false);
        m_ui->heightEdit->setEnabled(false);
        break;
    }

    if (not m_modified) {
        setWindowTitle(windowTitle() + "*");
        m_modified = true;
    }
}

void Settings::onTextChanged(const QString &text)
{
    Q_UNUSED(text);
    m_modified = true;
    if (not windowTitle().contains('*')) {
        setWindowTitle(windowTitle() + "*");
    }
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

    if (not m_modified) {
        setWindowTitle(windowTitle() + "*");
        m_modified = true;
    }
}

void Settings::applyChanges()
{
    if (not m_modified) {
        return;
    }

    m_settings->beginGroup("WindowSettings");
    bool rememberWindowSize = m_ui->rememberWindowSizeCheckBox->isChecked();
    bool alwaysMaximized = m_ui->alwaysMaximizedCheckBox->isChecked();
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
    m_settings->setValue("AlwaysMaximized", alwaysMaximized);
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

    setWindowTitle(windowTitle().mid(0, windowTitle().indexOf('*')));
    m_modified = false;
}
