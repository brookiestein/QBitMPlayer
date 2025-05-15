#include "spotifymanager.hpp"
#include "authenticator.hpp"

#include <QCryptographicHash>
#include <QDesktopServices>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QUrl>
#include <QUrlQuery>

SpotifyManager::SpotifyManager(QSettings *settings, QObject *parent)
    : QObject{parent}
    , m_settings(settings)
    , m_needsAuthentication(true)
{
    m_settings->beginGroup("Spotify");
    m_accessToken = m_settings->value("AccessToken", "").toString();
    m_userID = m_settings->value("UserID", "").toString();
    m_settings->endGroup();

    if (not m_accessToken.isEmpty())
        m_needsAuthentication = false;
}

bool SpotifyManager::isAuthenticationNeeded() const
{
    return m_needsAuthentication;
}

void SpotifyManager::generateCodeVerifier(int length)
{
    m_verifier.clear();
    QString possible = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~";

    for (int i {}; i < length; ++i)
        m_verifier += possible.at(
            std::floor(QRandomGenerator::global()->bounded(possible.size()))
    );
}

void SpotifyManager::generateCodeChallenge()
{
    auto digest = QCryptographicHash::hash(
        m_verifier.toUtf8(),
        QCryptographicHash::Sha256
    );

    auto base64url = digest.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

    m_challenge = QString::fromUtf8(base64url);
}

void SpotifyManager::authenticate()
{
    generateCodeVerifier(128);
    generateCodeChallenge();
    QUrlQuery query;
    query.addQueryItem("client_id", m_clientID);
    query.addQueryItem("response_type", "code");
    query.addQueryItem("redirect_uri", m_redirect_uri);

    query.addQueryItem(
        "scope",
        "user-read-private user-read-email "
        "playlist-read-private playlist-read-collaborative"
    );

    query.addQueryItem("code_challenge_method", "S256");
    query.addQueryItem("code_challenge", m_challenge);
    QUrl url(QString("https://accounts.spotify.com/authorize?%1").arg(query.toString()));

    QEventLoop loop;
    Authenticator authenticator(Authenticator::CodeType::CODE);
    connect(&authenticator, &Authenticator::errorOccurred, this, [this] (const QString &reason) {
        emit errorOccurred(reason);
    });

    connect(&authenticator, &Authenticator::codeReceived, this, [this] (const QString &code) {
        m_code = code;
    });

    connect(&authenticator, &Authenticator::codeReceived, &loop, &QEventLoop::quit);

    if (not authenticator.startServer())
        return;

    QDesktopServices::openUrl(url);
    loop.exec();

    if (not m_code.isEmpty()) {
        m_needsAuthentication = false;
        emit hideAuthenticationButton();
    }

    getAccessToken();
}

void SpotifyManager::getAccessToken()
{
    QUrlQuery query;
    query.addQueryItem("client_id", m_clientID);
    query.addQueryItem("grant_type", "authorization_code");
    query.addQueryItem("code", m_code);
    query.addQueryItem("redirect_uri", m_redirect_uri);
    query.addQueryItem("code_verifier", m_verifier);

    QUrl url("https://accounts.spotify.com/api/token");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QEventLoop loop;
    QNetworkAccessManager manager;
    auto *reply = manager.post(request, query.toString().toUtf8());

    connect(reply, &QNetworkReply::readyRead, this, [this, &reply] () {
        auto response = QJsonDocument::fromJson(reply->readAll());
        m_accessToken = response["access_token"].toString();
        reply->deleteLater();
    });

    connect(reply, &QNetworkReply::errorOccurred, this, &SpotifyManager::handleNetworkError);
    connect(reply, &QNetworkReply::errorOccurred, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::readyRead, &loop, &QEventLoop::quit);

    loop.exec();

    if (m_accessToken.isEmpty()) {
        emit errorOccurred(tr("An error occurred while getting the Spotify access token."));
    } else {
        m_settings->beginGroup("Spotify");
        m_settings->setValue("AccessToken", m_accessToken);
        m_settings->endGroup();
    }
}

void SpotifyManager::fetchProfile()
{
    QNetworkRequest request(QUrl("https://api.spotify.com/v1/me"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_accessToken.toUtf8());
    QNetworkAccessManager manager;
    QEventLoop loop;
    auto *reply = manager.get(request);

    connect(reply, &QNetworkReply::readyRead, this, [this, &reply] () {
        auto response = QJsonDocument::fromJson(reply->readAll());

        m_userID = response["id"].toString();
        m_displayName = response["display_name"].toString();
        m_accountType = response["product"].toString();

        m_settings->beginGroup("Spotify");
        m_settings->setValue("UserID", m_userID);
        m_settings->endGroup();

        reply->deleteLater();
    });

    connect(reply, &QNetworkReply::readyRead, &loop, &QEventLoop::quit);
    loop.exec();
}

void SpotifyManager::fetchPlaylist()
{
    QNetworkRequest request(
        QUrl(
            QString("https://api.spotify.com/v1/users/%1/playlists").arg(m_userID)
        )
    );

    request.setRawHeader("Authorization", "Bearer " + m_accessToken.toUtf8());

    QEventLoop loop;
    QNetworkAccessManager manager;
    auto *reply = manager.get(request);

    connect(reply, &QNetworkReply::readyRead, this, [this, &reply] () {
        auto response = QJsonDocument::fromJson(reply->readAll());
        auto playlists = response["items"].toArray();

        for (qsizetype i {}, size = playlists.size(); i < size; ++i) {
            auto object = playlists.at(i);
            m_playlists[object["id"].toString()] = object["name"].toString();
        }
    });

    connect(reply, &QNetworkReply::readyRead, &loop, &QEventLoop::quit);

    loop.exec();
}

QString SpotifyManager::displayName() const
{
    return m_displayName;
}

QString SpotifyManager::accountType() const
{
    return m_accountType;
}

std::map<QString, QString> SpotifyManager::playlists() const
{
    return m_playlists;
}

std::map<QString, QString> SpotifyManager::playlist(const QString &playlistName)
{
    if (playlistName.isEmpty())
        return {};
    if (m_playlists.empty()) {
        emit errorOccurred(tr("SpotifyManager::fetchPlaylist() must first be called."));
        return {};
    }

    std::map<QString, QString> playlist;
    QString playlistID {};
    for (const auto &[id, name] : m_playlists) {
        if (playlistName == name) {
            playlistID = id;
            break;
        }
    }

    QNetworkRequest request(
        QUrl(
            QString("https://api.spotify.com/v1/playlists/%1/tracks").arg(playlistID)
        )
    );

    request.setRawHeader("Authorization", "Bearer " + m_accessToken.toUtf8());

    QEventLoop loop;
    QNetworkAccessManager manager;
    auto *reply = manager.get(request);

    connect(reply, &QNetworkReply::readyRead, this, [this, &reply, &playlist] () {
        auto response = QJsonDocument::fromJson(reply->readAll());
        auto items = response["items"].toArray();
        qDebug().noquote() << response.toJson();

        for (qsizetype i {}, size = items.size(); i < size; ++i) {
            auto object = items.at(i);

            playlist[object["id"].toString()] = object["name"].toString();
        }
    });

    return playlist;
}

void SpotifyManager::handleNetworkError(QNetworkReply::NetworkError error)
{
    switch (error)
    {
    case QNetworkReply::NoError:
        break;
    case QNetworkReply::ConnectionRefusedError:
        emit errorOccurred(tr("Connection to spotify.com failed."));
        break;
    case QNetworkReply::RemoteHostClosedError:
        emit errorOccurred(tr("spotify.com closed the connection."));
        break;
    case QNetworkReply::HostNotFoundError: [[fallthrough]];
    case QNetworkReply::TimeoutError:
        emit errorOccurred(tr("Could not connect to spotify.com. Please make sure you have internet connection."));
        break;
    case QNetworkReply::OperationCanceledError:
    case QNetworkReply::SslHandshakeFailedError:
    case QNetworkReply::TemporaryNetworkFailureError:
    case QNetworkReply::NetworkSessionFailedError:
    case QNetworkReply::BackgroundRequestNotAllowedError:
    case QNetworkReply::TooManyRedirectsError:
    case QNetworkReply::InsecureRedirectError:
    case QNetworkReply::UnknownNetworkError:
    case QNetworkReply::ProxyConnectionRefusedError:
    case QNetworkReply::ProxyConnectionClosedError:
    case QNetworkReply::ProxyNotFoundError:
    case QNetworkReply::ProxyTimeoutError:
    case QNetworkReply::ProxyAuthenticationRequiredError:
    case QNetworkReply::UnknownProxyError:
    case QNetworkReply::ContentAccessDenied:
    case QNetworkReply::ContentOperationNotPermittedError:
    case QNetworkReply::ContentNotFoundError:
    case QNetworkReply::AuthenticationRequiredError:
        emit needsAuthentication(
            tr("Since this apps it's started, it's not possible to get a token for a longer time. "
               "This will be fixed in the future. We're so sorry for this, just authorize the app again "
               "in the link that will be opened after this message is closed.")
        );
        break;
    case QNetworkReply::ContentReSendError:
    case QNetworkReply::ContentConflictError:
    case QNetworkReply::ContentGoneError:
    case QNetworkReply::UnknownContentError:
    case QNetworkReply::ProtocolUnknownError:
    case QNetworkReply::ProtocolInvalidOperationError:
    case QNetworkReply::ProtocolFailure:
    case QNetworkReply::InternalServerError:
    case QNetworkReply::OperationNotImplementedError:
    case QNetworkReply::ServiceUnavailableError:
    case QNetworkReply::UnknownServerError:
        break;
    }
}
