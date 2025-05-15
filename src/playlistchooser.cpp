#include "playlistchooser.hpp"
#include "ui_playlistchooser.h"

PlaylistChooser::PlaylistChooser(QSettings *settings, QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::PlaylistChooser)
    , m_settings {settings}
    , m_quitShortcut {new QShortcut(QKeySequence(Qt::Key_Escape), this)}
{
    m_ui->setupUi(this);

    configureTable();
    loadPlaylists();

    connect(m_ui->tableWidget, &QTableWidget::cellDoubleClicked, this, &PlaylistChooser::onItemDoubleClicked);
    connect(m_quitShortcut, &QShortcut::activated, this, &QWidget::close);
}

#ifdef USE_SPOTIFY
PlaylistChooser::PlaylistChooser(SpotifyManager *spotifyManager, QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::PlaylistChooser)
    , m_spotifyManager(spotifyManager)
    , m_quitShortcut {new QShortcut(QKeySequence(Qt::Key_Escape), this)}
{
    m_ui->setupUi(this);

    configureTable();
    loadPlaylists(TYPE::SPOTIFY);

    connect(m_ui->tableWidget, &QTableWidget::cellDoubleClicked, this, &PlaylistChooser::onItemDoubleClicked);
    connect(m_quitShortcut, &QShortcut::activated, this, &QWidget::close);
    m_spotifyManager->fetchPlaylist();
}
#endif

PlaylistChooser::~PlaylistChooser()
{
    delete m_ui;
}

QString PlaylistChooser::playlist() const
{
    return m_chosenPlaylist;
}

void PlaylistChooser::closeEvent(QCloseEvent *event)
{
    emit closed();
    QWidget::closeEvent(event);
}

void PlaylistChooser::showEvent(QShowEvent *event)
{
    m_ui->tableWidget->resizeColumnsToContents();
    m_ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    QWidget::showEvent(event);
}

void PlaylistChooser::onItemDoubleClicked(int row, int column)
{
    m_chosenPlaylist = m_ui->tableWidget->item(row, column)->text();
    close();
}

void PlaylistChooser::configureTable()
{
    QStringList headers;
    headers << tr("Playlist Name");

    m_ui->tableWidget->setColumnCount(headers.size());
    m_ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_ui->tableWidget->setEditTriggers(QTableWidget::NoEditTriggers);
    m_ui->tableWidget->setHorizontalHeaderLabels(headers);
}

void PlaylistChooser::loadPlaylists(TYPE type)
{
    if (type == TYPE::SPOTIFY) {
#ifndef USE_SPOTIFY
        emit errorOccurred(tr(PROJECT_NAME " wasn't build with Spotify support."));
        return;
#endif

        auto playlists = m_spotifyManager->playlists();
        for (const auto &[id, name] : playlists) {
            auto *item = new QTableWidgetItem(name);
            item->setTextAlignment(Qt::AlignCenter);

            int rowCount = m_ui->tableWidget->rowCount();
            m_ui->tableWidget->insertRow(rowCount);
            m_ui->tableWidget->setItem(rowCount, 0, item);
        }
        return;
    }

    m_settings->beginGroup("Playlists");

    for (const auto &playlistName : m_settings->childGroups()) {
        auto *item = new QTableWidgetItem(playlistName);
        item->setTextAlignment(Qt::AlignCenter);

        int rowCount = m_ui->tableWidget->rowCount();
        m_ui->tableWidget->insertRow(rowCount);
        m_ui->tableWidget->setItem(rowCount, 0, item);
    }

    m_settings->endGroup();
}
