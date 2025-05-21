#ifndef NOTIFIER_HPP
#define NOTIFIER_HPP

#include <exception>
#include <libnotify/notification.h>
#include <libnotify/notify.h>
#include <QObject>

class NotificationException : public std::exception
{
    const char *m_reason;
public:
    NotificationException(const char *reason);
    const char *what();
};

class Notifier : public QObject
{
    Q_OBJECT
    NotifyNotification *m_notification;
    const char *m_summary;
    const char *m_body;
    const char *m_icon;
public:
    explicit Notifier(const char *summary,
                      const char *body,
                      const char *icon = nullptr,
                      QObject *parent = nullptr);
    ~Notifier();
    void sendNotification();
signals:
    void errorOccurred(const QString &reason);
};

#endif // NOTIFIER_HPP
