#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <QCheckBox>
#include <QCloseEvent>
#include <QLineEdit>
#include <QSettings>
#include <QWidget>

namespace Ui {
class Settings;
}

class Settings : public QWidget
{
    Q_OBJECT

    QString createEnvironment();

public:
    explicit Settings(QWidget *parent = nullptr);
    ~Settings();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void checkForChange();
    void onRememberWindowSizeChecked(Qt::CheckState state);
    void onAlwaysMaximizedChecked(Qt::CheckState state);
    void onRememberVolumeLevelChecked(Qt::CheckState state);
    void applyChanges();

signals:
    void closed();

private:
    Ui::Settings *m_ui;
    QSettings *m_settings;
    bool m_modified;
    /* In order to check whether has had a change. */
    QMap<const QCheckBox *, bool> m_initialCheckBoxesValues;
    QMap<const QLineEdit *, QString> m_initialFieldValues;
};

#endif // SETTINGS_HPP
