#ifndef NOTIFIER_HPP
#define NOTIFIER_HPP

#include <exception>
#include <QObject>
#ifdef __linux__
extern "C" {
    #undef signals
        #include <libnotify/notify.h>
        #include <libnotify/notification.h>
    #define signals Q_SIGNALS
}
#elif defined(WIN32) || defined(_WIN32)
    #include "wintoastlib.h"
    #include <string>
#endif

class NotificationException : public std::exception
{
    const char *m_reason;
public:
    NotificationException(const char *reason);
    const char *what();
};

#ifdef Q_OS_WIN
class WinToastHandler : public WinToastLib::IWinToastHandler
{
public:
    WinToastHandler();
    void toastActivated() const override;
    void toastActivated(int actionIndex) const override;
    void toastActivated(std::wstring response) const override;
    void toastDismissed(WinToastLib::IWinToastHandler::WinToastDismissalReason reason) const override;
    void toastFailed() const override;
};
#endif

class Notifier : public QObject
{
    Q_OBJECT
public:
#ifdef Q_OS_LINUX
    NotifyNotification *m_notification;
    const char *m_summary;
    const char *m_body;
    const char *m_icon;
    explicit Notifier(const char *summary,
                      const char *body,
                      const char *icon = nullptr,
                      QObject *parent = nullptr);
    ~Notifier();
#elif defined(Q_OS_WIN)
    WinToastLib::WinToast *m_winToast = WinToastLib::WinToast::instance();
    std::wstring m_title;
    std::wstring m_body;
    explicit Notifier(const std::wstring &title, const std::wstring &body, QObject *parent = nullptr);
#endif
    void sendNotification();
signals:
    void errorOccurred(const QString &reason);
};

#endif // NOTIFIER_HPP
