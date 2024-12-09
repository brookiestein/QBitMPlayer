#ifndef PLAYLISTCHOOSER_HPP
#define PLAYLISTCHOOSER_HPP

#include <QCloseEvent>
#include <QSettings>
#include <QShowEvent>
#include <QWidget>

namespace Ui {
class PlaylistChooser;
}

class PlaylistChooser : public QWidget
{
    Q_OBJECT

    void configureTable();
    void loadPlaylists();

public:
    explicit PlaylistChooser(QSettings *settings, QWidget *parent = nullptr);
    ~PlaylistChooser();
    QString playlist() const;

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void onItemDoubleClicked(int row, int column);

signals:
    void closed();

private:
    Ui::PlaylistChooser *m_ui;
    QSettings *m_settings;
    QString m_chosenPlaylist;
};

#endif // PLAYLISTCHOOSER_HPP
