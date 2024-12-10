#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <QCloseEvent>
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
    void onRememberWindowSizeChecked(Qt::CheckState state);
    void onAlwaysMaximizedChecked(Qt::CheckState state);
    void onTextChanged(const QString &text);
    void onRememberVolumeLevelChecked(Qt::CheckState state);
    void applyChanges();

signals:
    void closed();

private:
    Ui::Settings *m_ui;
    QSettings *m_settings;
    bool m_modified;
};

#endif // SETTINGS_HPP
