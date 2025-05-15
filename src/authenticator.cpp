#include "authenticator.hpp"

#include <QRegularExpression>
#include <QTcpSocket>

Authenticator::Authenticator(CodeType type, QObject *parent)
    : QObject{parent}
    , m_type(type)
    , m_server (new QTcpServer(this))
    , m_port(5173)
{
    connect(m_server, &QTcpServer::newConnection, this, &Authenticator::handleConnection);
}

Authenticator::~Authenticator()
{
    m_server->close();
}

bool Authenticator::startServer()
{
    if (not m_server->listen(QHostAddress::LocalHost, m_port)) {
        emit errorOccurred(tr("Failed to start authentication server."));
        return false;
    }

    return true;
}

void Authenticator::handleConnection()
{
    auto *socket = m_server->nextPendingConnection();
    if (!socket)
        return;

    QString code;
    QString token;
    connect(socket, &QTcpSocket::readyRead, this, [this, &socket, &code, &token] () {
        auto request = QString::fromUtf8(socket->readAll());

        if (m_type == CodeType::TOKEN) {
            token = request;
            return;
        }

        if (not request.contains("code"))
            return;

        code = request.mid(request.indexOf("code="));
        code = code.mid(code.indexOf('=') + 1, code.indexOf("HTTP") - 6);
        emit codeReceived(code);

        QByteArray response("HTTP/1.1 200 OK\r\n\
Content-Type: text/plain\r\n\r\n\
Authenticated, thanks! You may now close this tab.");
        socket->write(response);
        socket->disconnectFromHost();
    });

    socket->waitForDisconnected();
    if (not code.isEmpty())
        emit codeReceived(code);
    else if (not token.isEmpty())
        emit tokenReceived(token);
    else
        emit errorOccurred(tr("An error occurred while getting the authentication code."));

    socket->deleteLater();
}
