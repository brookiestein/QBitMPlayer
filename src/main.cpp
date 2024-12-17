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

#include "config.hpp"
#include "player.hpp"

const QMap<QString, QString> availableLanguages {
    { "es", ":/resources/i18n/QBitMPlayer_es_MX.qm" },
    /* In order to handle several user inputs. */
    { "sp", ":/resources/i18n/QBitMPlayer_es_MX.qm" },
    { "español", ":/resources/i18n/QBitMPlayer_es_MX.qm" }
};

QList<QCommandLineOption> commandLineOptions();
/* Prompts user for a reply with a QMessageBox or in the command line. */
QMessageBox::StandardButton showMessage(int argc, const QString &message, bool question);

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setApplicationName(PROJECT_NAME);
    QApplication::setApplicationVersion(PROJECT_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription(PROJECT_DESCRIPTION);
    parser.addHelpOption();
    parser.addOptions(commandLineOptions());
    parser.addVersionOption();
    parser.process(a);

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
        const QStringList uiLanguages = QLocale::system().uiLanguages();
        for (const QString &locale : uiLanguages) {
            const QString baseName = "QBitMPlayer_" + QLocale(locale).name();
            if (translator.load(":/i18n/" + baseName)) {
                a.installTranslator(&translator);
                break;
            }
        }
    }

    /* If user didn't provide any command line option or just set language, show the GUI. */
    if (argc == 1 or (parser.isSet("language") and argc == 3)) {
        MainWindow w;
        bool maximized {false};

        {
            QSettings settings(w.createEnvironment(), QSettings::IniFormat);
            settings.beginGroup("WindowSettings");
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

    return options;
}

QMessageBox::StandardButton showMessage(int argc, const QString &message, bool question)
{
    if (argc == 3) {
        if (question) {
            return QMessageBox::question(nullptr, QObject::tr("Question"), message);
        } else {
            return QMessageBox::critical(nullptr, QObject::tr("Error"), message);
        }
    } else {
        if (question) {
            std::string input;
            std::cout << message.toStdString() << " [Y/N]: ";
            std::getline(std::cin, input);
            if (QString(input.c_str()).contains("Y", Qt::CaseInsensitive)) {
                return QMessageBox::Yes;
            } else {
                return QMessageBox::No;
            }
        } else {
            qInfo().noquote() << message;
        }
    }

    return QMessageBox::Yes;
}
