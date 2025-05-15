#ifndef PLAYLISTCHOOSER_HPP
#define PLAYLISTCHOOSER_HPP

#include <QCloseEvent>
#include <QSettings>
#include <QShortcut>
#include <QShowEvent>
#include <QWidget>

#ifdef USE_SPOTIFY
#include "spotifymanager.hpp"
#endif

namespace Ui {
class PlaylistChooser;
}

class PlaylistChooser : public QWidget
{
    Q_OBJECT

public:
    explicit PlaylistChooser(QSettings *settings, QWidget *parent = nullptr);
    enum class TYPE { LOCAL = 0, SPOTIFY };
#ifdef USE_SPOTIFY
    explicit PlaylistChooser(SpotifyManager *spotifyManager, QWidget *parent = nullptr);
#endif
    ~PlaylistChooser();
    QString playlist() const;

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void onItemDoubleClicked(int row, int column);
#ifdef USE_SPOTIFY
public slots:
    void playlistsFetched();
#endif

signals:
    void closed();
    void errorOccurred(const QString &reason);

private:
    Ui::PlaylistChooser *m_ui;
    QSettings *m_settings;
    QString m_chosenPlaylist;
    QShortcut *m_quitShortcut; /* Quit on Espace pressed */
#ifdef USE_SPOTIFY
    SpotifyManager *m_spotifyManager;
#endif

    void configureTable();
    void loadPlaylists(TYPE type = TYPE::LOCAL);
};

#endif // PLAYLISTCHOOSER_HPP
