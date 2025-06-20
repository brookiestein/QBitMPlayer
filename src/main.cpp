#include "mainwindow.hpp"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QLocale>
#include <QMessageBox>
#include <QSettings>
#include <QTranslator>
#include <iostream>
#include <string>
#ifdef ENABLE_IPC
    #include <QDBusConnection>
    #include <QDBusMessage>
#endif // ENABLE_IPC
#ifdef SINGLE_INSTANCE
    #include <QTimer>
    #include <QTcpServer>
    #include <QTcpSocket>

    #define SERVER_PORT 20640
#endif // SINGLE_INSTANCE

#include "config.hpp"
#include "player.hpp"
#include "settings.hpp"

const QMap<QString, QString> availableLanguages {
    { "es", ":/resources/i18n/QBitMPlayer_es_MX.qm" },
    /* In order to handle several user inputs. */
    { "sp", ":/resources/i18n/QBitMPlayer_es_MX.qm" },
    { "español", ":/resources/i18n/QBitMPlayer_es_MX.qm" }
};

QList<QCommandLineOption> commandLineOptions();
/* Prompts user for a reply with a QMessageBox or in the command line. */
QMessageBox::StandardButton showMessage(int argc, const QString &message, bool question);

#ifdef SINGLE_INSTANCE
    bool isAnotherInstanceRunning();
#endif

#ifdef ENABLE_IPC
    void registerDBusService(MainWindow &m);
#endif // ENABLE_IPC

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setApplicationName(PROJECT_NAME);
    QApplication::setApplicationVersion(PROJECT_VERSION);
    QApplication::setWindowIcon(QIcon::fromTheme(QIcon::ThemeIcon::MultimediaPlayer));

    QCommandLineParser parser;
    parser.setApplicationDescription(PROJECT_DESCRIPTION);
    parser.addHelpOption();
    parser.addOptions(commandLineOptions());
    parser.addVersionOption();
    parser.process(a);

    QSettings settings(Settings::createEnvironment(), QSettings::IniFormat);
    QTranslator translator;

    if (parser.isSet("language")) {
        auto lang = parser.value("language").toLower();
        if (not availableLanguages.contains(lang)) {
            showMessage(
                argc,
                QObject::tr("Language %1 is not yet supported.").arg(lang),
                false
            );
            return 1;
        }

        if (translator.load(availableLanguages[lang])) {
            a.installTranslator(&translator);
        } else {
            auto reply = showMessage(
                argc,
                QObject::tr("Language: %1 couldn't be loaded. "
                            "Would you like to continue with English?").arg(lang),
                true
            );

            if (reply != QMessageBox::Yes) {
                return 1;
            }
        }
    } else {
        settings.beginGroup("WindowSettings");
        auto defaultLanguage = settings.value("DefaultLanguage", "").toString();
        settings.endGroup();

        if (defaultLanguage.isEmpty()) {
            const QStringList uiLanguages = QLocale::system().uiLanguages();
            for (const QString &locale : uiLanguages) {
                const QString baseName = "QBitMPlayer_" + QLocale(locale).name();
                if (translator.load(":/i18n/" + baseName)) {
                    a.installTranslator(&translator);
                    break;
                }
            }
        } else if (defaultLanguage == "English") {
            // No need to set because it's the default one.
        } else if (defaultLanguage == "Español") {
            if (translator.load(availableLanguages["es"])) {
                a.installTranslator(&translator);
            } else {
                auto reply = showMessage(
                    argc,
                    QObject::tr("Language: Español couldn't be loaded. "
                                "Would you like to continue with English?"),
                    true
                );

                if (reply != QMessageBox::Yes) {
                    return 1;
                }
            }

            qDebug().noquote() << QObject::tr("Interface language set to "
                                              "Español because it was set "
                                              "in the configurations.");
        } else {
            showMessage(
                argc,
                QObject::tr("Language: %1 found in settings doesn't "
                            "correspond to any supported at the moment.")
                    .arg(defaultLanguage),
                false
            );
        }
    }

#ifdef ENABLE_IPC
    if (parser.isSet("next")) {
        QDBusConnection::sessionBus().send(
            QDBusMessage::createMethodCall(SERVICE_NAME, "/Listen", "", "playNext")
        );
        return EXIT_SUCCESS;
    } else if (parser.isSet("previous")) {
        QDBusConnection::sessionBus().send(
            QDBusMessage::createMethodCall(SERVICE_NAME, "/Listen", "", "playPrevious")
        );
        return EXIT_SUCCESS;
    } else if (parser.isSet("toggle-play")) {
        QDBusConnection::sessionBus().send(
            QDBusMessage::createMethodCall(SERVICE_NAME, "/Listen", "", "togglePlay")
        );
        return EXIT_SUCCESS;
    } else if (parser.isSet("stop")) {
        QDBusConnection::sessionBus().send(
            QDBusMessage::createMethodCall(SERVICE_NAME, "/Listen", "", "stop")
        );
        return EXIT_SUCCESS;
    }
#endif // ENABLE_IPC

#ifdef SINGLE_INSTANCE
#ifndef ENABLE_IPC
#error SINGLE_INSTANCE must be in combination with ENABLE_IPC.
#endif // ENABLE_IPC
    QTcpServer server;
    QObject::connect(&server, &QTcpServer::newConnection, [&server] () {
        QDBusConnection::sessionBus().send(
            QDBusMessage::createMethodCall(SERVICE_NAME, "/Listen", "", "show")
        );

        server.nextPendingConnection()->deleteLater();
    });

    if (isAnotherInstanceRunning()) {
        QDBusConnection::sessionBus().send(
            QDBusMessage::createMethodCall(SERVICE_NAME, "/Listen", "", "show")
        );
        return EXIT_SUCCESS;
    } else {
        server.listen(QHostAddress::LocalHost, SERVER_PORT);
    }
#else
    qInfo().noquote() << QObject::tr("Not built with support for detecting other running instances.");
#endif // SINGLE_INSTANCE

    /* If user didn't provide any command line option or just set language, show the GUI. */
    if (argc == 1 or (parser.isSet("language") and argc == 3)) {
        MainWindow w;
        bool maximized {false};
#ifdef ENABLE_IPC
        registerDBusService(w);
#endif // ENABLE_IPC

        {
            settings.beginGroup("WindowSettings");

            // If MinimizeToSystray == false, then do close on last window closed.
            QApplication::setQuitOnLastWindowClosed(
                not settings.value("MinimizeToSystray", false).toBool()
            );

            int width = settings.value("Width", 1024).toInt();
            int height = settings.value("Height", 530).toInt();
            /* In order to show the window maximized both
             * Maximized as RememberWindowSize have to be set to true
             * or alwaysMaximized. */
            maximized = settings.value("AlwaysMaximized", false).toBool()
                        or (settings.value("Maximized", false).toBool()
                            and settings.value("RememberWindowSize", false).toBool());

            /* Center window by default. User can overwrite this in the settings page. */
            if (not settings.contains("Centered")) {
                settings.setValue("Centered", true);
            }

            settings.endGroup();

            w.setGeometry(w.x(), w.y(), width, height);

        }

        if (maximized) {
            w.showMaximized();
        } else {
            w.show();
        }

        return a.exec();
    }

    /* Go with command line options */
    auto playlist = parser.value("files").trimmed().split(',');

    for (auto &filename : playlist) {
        filename = filename.trimmed();
    }

    Player player;
    QObject::connect(&player, &Player::finished, &a, &QApplication::quit, Qt::QueuedConnection);
    QObject::connect(&player, &Player::error, [] (const QString &message) {
        qCritical() << message;
    });
    QObject::connect(&player, &Player::warning, [] (const QString &message) {
        qDebug() << message;
    });
    QObject::connect(&player, &Player::error, &a, &QApplication::quit, Qt::QueuedConnection);

    player.setPlayList(playlist);
    player.setAutoPlay(true);
    player.playNext();
    return a.exec();
}

QList<QCommandLineOption> commandLineOptions()
{
    QList<QCommandLineOption> options;

    options << QCommandLineOption(
        QStringList() << "f" << "files",
        QObject::tr("Comma-separated list of music files to be played. "
                    "Please note that when using quotes to provide the filepaths "
                    "your shell may not expand characters like '~', for example, "
                    "thus your songs may not be played at all."),
        "files"
    );

    options << QCommandLineOption(
        QStringList() << "l" << "language",
        QObject::tr("Which language to display the app in other than English. "
                    "Available: Spanish."),
        "language"
    );

#ifdef ENABLE_IPC
    options << QCommandLineOption(
        QStringList() << "n" << "next",
        QObject::tr("Tell an existing %1 instance to play the next song if any.").arg(PROJECT_NAME)
    );

    options << QCommandLineOption(
        QStringList() << "P" << "previous",
        QObject::tr("Tell an existing %1 instance to play the previous song if any.").arg(PROJECT_NAME)
    );

    options << QCommandLineOption(
        QStringList() << "t" << "toggle-play",
        QObject::tr("Tell an existing %1 instance to resume or pause the player.").arg(PROJECT_NAME)
    );

    options << QCommandLineOption(
        QStringList() << "s" << "stop",
        QObject::tr("Tell an existing %1 instance to stop the player.").arg(PROJECT_NAME)
    );
#endif // ENABLE_IPC

    return options;
}

QMessageBox::StandardButton showMessage(int argc, const QString &message, bool question)
{
    if (argc == 3) {
        if (question)
            return QMessageBox::question(nullptr, QObject::tr("Question"), message);
        else
            return QMessageBox::critical(nullptr, QObject::tr("Error"), message);
    } else {
        if (question) {
            std::string input;
            std::cout << message.toStdString() << " [Y/N]: ";
            std::getline(std::cin, input);

            if (QString(input.c_str()).contains("Y", Qt::CaseInsensitive))
                return QMessageBox::Yes;
            else
                return QMessageBox::No;
        } else {
            qInfo().noquote() << message;
        }
    }

    return QMessageBox::Yes;
}

#ifdef SINGLE_INSTANCE
bool isAnotherInstanceRunning()
{
    bool running {};
    QEventLoop loop;

    QTimer timer;
    timer.setInterval(3'000);

    QObject::connect(&timer, &QTimer::timeout, [&loop] () {
        loop.quit();
    });

    QTcpSocket socket;
    QObject::connect(&socket, &QTcpSocket::connected, [&running, &loop] () {
        QMessageBox::warning(
            nullptr,
            QObject::tr("Another Instance is Running"),
            QObject::tr("Detected another instance running, showing that one. "
                        "Please take in account that this will only work if you "
                        "first started %1's window.").arg(PROJECT_NAME)
        );

        running = true;
        loop.quit();
    });

    QObject::connect(&socket, &QTcpSocket::errorOccurred, [&timer, &loop] () {
        timer.stop();
        loop.quit();
    });

    socket.connectToHost(QHostAddress::LocalHost, SERVER_PORT);

    // waitForConnected
    timer.start();

    loop.exec();

    return running;
}
#endif // SINGLE_INSTANCE

#ifdef ENABLE_IPC
void registerDBusService(MainWindow &m)
{
    auto connection = QDBusConnection::sessionBus();
    if (not connection.registerService(SERVICE_NAME)) {
        QMessageBox::critical(
            nullptr,
            QObject::tr("Error"),
            QObject::tr("Unable to register DBus service. Play won't respond to IPC commands.")
        );

        return;
    }

    if (not connection.registerObject("/Listen", &m, QDBusConnection::ExportScriptableSlots)) {
        QMessageBox::critical(
            nullptr,
            QObject::tr("Error"),
            QObject::tr("Unable to register listener object. Play won't respond to IPC commands.")
        );

        return;
    }
}
#endif // ENABLE_IPC
