#include "notifier.hpp"
#include "config.hpp"

#include <QDebug>

#ifdef Q_OS_WIN
    #include "wintoastlib.h"
#endif

NotificationException::NotificationException(const char *reason)
    : m_reason {reason}
{

}

const char *NotificationException::what()
{
    return m_reason;
}

#ifdef Q_OS_LINUX
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
#elif defined(Q_OS_WIN)
Notifier::Notifier(const std::wstring &title, const std::wstring &body, QObject *parent)
    : QObject {parent}
    , m_title {title}
    , m_body {body}
{
    WinToastLib::WinToast::instance()->setAppName(L"" PROJECT_NAME);
    WinToastLib::WinToast::instance()->setAppUserModelId(
        WinToastLib::WinToast::configureAUMI(
            L"Brayan M. Salazar", L"" PROJECT_NAME, L"", L"" PROJECT_VERSION)
    );

    if (!WinToastLib::WinToast::instance()->initialize())
        throw NotificationException("Notification library could not be initialized.");
}

void Notifier::sendNotification()
{
    auto *handler = new WinToastHandler;
    WinToastLib::WinToast::WinToastError error;
    WinToastLib::WinToastTemplate templ = WinToastLib::WinToastTemplate(WinToastLib::WinToastTemplate::Text02);
    templ.setTextField(m_title, WinToastLib::WinToastTemplate::FirstLine);
    templ.setTextField(m_body, WinToastLib::WinToastTemplate::SecondLine);

    const auto toastId = WinToastLib::WinToast::instance()->showToast(templ, handler, &error);
    if (toastId < 0) {
        auto message = tr("An error occurred while sending the desktop notification.");
        switch (error)
        {
        }
    }
    delete handler;
}

WinToastHandler::WinToastHandler()
{
}

void WinToastHandler::toastActivated() const
{
}

void WinToastHandler::toastActivated([[maybe_unused]] int actionIndex) const
{
}

void WinToastHandler::toastActivated([[maybe_unused]] std::wstring response) const
{
}

void WinToastHandler::toastDismissed([[maybe_unused]] WinToastLib::IWinToastHandler::WinToastDismissalReason reason) const
{
}

void WinToastHandler::toastFailed() const
{
    qCritical() << "Desktop notification failed to be sent.";
}
#endif
