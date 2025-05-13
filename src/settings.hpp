#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QLineEdit>
#include <QSettings>
#include <QShortcut>
#include <QStandardPaths>
#include <QWidget>

namespace Ui {
class Settings;
}

class Settings : public QWidget
{
    Q_OBJECT

    QString createEnvironment(
        const QString &location = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
    );
    void loadPlaylists();
    void setInitialValues();

public:
    explicit Settings(QWidget *parent = nullptr);
    ~Settings();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onCurrentChanged(int index);
    void checkForChange();
    void onRememberWindowSizeChecked(Qt::CheckState state);
    void onAlwaysMaximizedChecked(Qt::CheckState state);
    void onMinimizeToSystrayChecked(Qt::CheckState state);
    void onRememberVolumeLevelChecked(Qt::CheckState state);
    void applyChanges();

signals:
    void closed();

private:
    Ui::Settings *m_ui;
    QSettings *m_settings;
    QSettings *m_playlistSettings;
    bool m_modified;
    bool m_changesApplied;
    /* In order to check whether has had a change. */
    QMap<const QCheckBox *, bool> m_initialCheckBoxesValues;
    QMap<const QLineEdit *, QString> m_initialFieldValues;
    QMap<const QComboBox *, int> m_initialComboBoxValues;
    QShortcut *m_quitShortcut; /* Quit on Escape pressed */
};

#endif // SETTINGS_HPP
