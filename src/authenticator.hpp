#ifndef AUTHENTICATOR_HPP
#define AUTHENTICATOR_HPP

#include <QObject>
#include <QTcpServer>

class Authenticator : public QObject
{
    Q_OBJECT

    QTcpServer *m_server;
    qint64 m_port;
public:
    enum class CodeType { CODE = 0, TOKEN };
    explicit Authenticator(CodeType type, QObject *parent = nullptr);
    ~Authenticator();
    bool startServer();
signals:
    void errorOccurred(const QString &reason);
    void tokenReceived(const QString &token);
    void codeReceived(const QString &code);
private slots:
    void handleConnection();
private:
    CodeType m_type;
};

#endif // AUTHENTICATOR_HPP
