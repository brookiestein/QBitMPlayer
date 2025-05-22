#include "notifier.hpp"
#include "config.hpp"

#include <QDebug>

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
using namespace WinToastLib;

Notifier::Notifier(const std::wstring &title, const std::wstring &body, QObject *parent)
    : QObject {parent}
    , m_title {title}
    , m_body {body}
{
    m_winToast->setAppName(L"" PROJECT_NAME);
    m_winToast->setAppUserModelId(
        WinToast::configureAUMI(
            L"Brayan M. Salazar", L"" PROJECT_NAME, L"", L"" PROJECT_VERSION
        )
    );

    if (!m_winToast->initialize())
        throw NotificationException("Notification library could not be initialized.");
}

void Notifier::sendNotification()
{
    auto *handler = new WinToastHandler;
    WinToast::WinToastError error;
    WinToastTemplate templ = WinToastTemplate(WinToastLib::WinToastTemplate::Text02);
    templ.setTextField(m_title, WinToastTemplate::FirstLine);
    templ.setTextField(m_body, WinToastTemplate::SecondLine);

    const auto toastId = m_winToast->showToast(templ, handler, &error);
    if (toastId < 0) {
        auto message = tr("An error occurred while sending the desktop notification.\n%1").arg(
            QString::fromStdWString(WinToast::strerror(error))
        );
        emit errorOccurred(message);
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

void WinToastHandler::toastDismissed([[maybe_unused]] IWinToastHandler::WinToastDismissalReason reason) const
{
}

void WinToastHandler::toastFailed() const
{
    emit errorOccurred(tr("Desktop notification failed to be sent."));
}
#endif
