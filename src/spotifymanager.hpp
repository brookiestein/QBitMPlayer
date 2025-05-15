#ifndef SPOTIFYMANAGER_HPP
#define SPOTIFYMANAGER_HPP

#include <QObject>
#include <QNetworkReply>
#include <QSettings>
#include <map>

class SpotifyManager : public QObject
{
    Q_OBJECT
    QSettings *m_settings;
    QString m_verifier;
    QString m_challenge;
    QString m_code;
    QString m_accessToken;
    const QString m_clientID = "542f7b4160444d2684bdb3e84590dda0";
    const QString m_redirect_uri = "http://127.0.0.1:5173/callback";
    bool m_needsAuthentication;
    QString m_userID;
    QString m_displayName;
    QString m_accountType;
    std::map<QString, QString> m_playlists;

    void generateCodeVerifier(int length);
    void generateCodeChallenge();
    void getAccessToken();
public:
    explicit SpotifyManager(QSettings *settings, QObject *parent = nullptr);
    void authenticate();
    bool isAuthenticationNeeded() const;
    void fetchProfile();
    void fetchPlaylist();
    QString clientID() const;
    QString redirectURI() const;
    QString challenge() const;
    QString displayName() const;
    QString accountType() const;
    std::map<QString, QString> playlists() const;
    std::map<QString, QString> playlist(const QString &playlistName);
signals:
    void information(const QString &message);
    void errorOccurred(const QString &reason);
    void hideAuthenticationButton();
    void needsAuthentication(const QString &message);
private slots:
    void handleNetworkError(QNetworkReply::NetworkError error);
};

#endif // SPOTIFYMANAGER_HPP
