#include "mainwindow.hpp"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QLocale>
#include <QSettings>
#include <QTranslator>

#include "config.hpp"
#include "player.hpp"

QList<QCommandLineOption> commandLineOptions();

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
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "QBitMPlayer_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    /* If user didn't provide any command line option, show the GUI. */
    if (argc == 1) {
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

    return options;
}
