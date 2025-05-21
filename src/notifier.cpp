#include "notifier.hpp"
#include "config.hpp"

NotificationException::NotificationException(const char *reason)
    : m_reason {reason}
{

}

const char *NotificationException::what()
{
    return m_reason;
}

Notifier::Notifier(const char *summary, const char *body, const char *icon, QObject *parent)
    : QObject{parent}
    , m_summary {summary}
    , m_body {body}
    , m_icon {icon}
{
    if (not notify_init(PROJECT_NAME))
        throw NotificationException("Notification library could not be initialized.");

    m_notification = notify_notification_new(m_summary, m_body, m_icon);

    notify_notification_set_urgency(m_notification, NOTIFY_URGENCY_NORMAL);
}

Notifier::~Notifier()
{
    notify_uninit();
}

void Notifier::sendNotification()
{
    GError *error = nullptr;
    if (!notify_notification_show(m_notification, &error)) {
        emit errorOccurred(
            tr("An error occurred while sending the desktop notification.\n%1").arg(error->message)
        );
    }
}
