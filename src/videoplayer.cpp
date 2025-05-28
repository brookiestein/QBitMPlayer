#include "videoplayer.hpp"

VideoPlayer::VideoPlayer(QWidget *parent)
    : QVideoWidget {parent}
    , m_firstTime {true}
{
}

void VideoPlayer::mouseDoubleClickEvent(QMouseEvent *event)
{
    setFullScreen(not isFullScreen());
}

void VideoPlayer::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton)
        emit pause();
    event->ignore();
}

void VideoPlayer::showEvent(QShowEvent *event) {
    if (m_firstTime) {
        m_firstTime = false;
        setVisible(false);
        event->accept();
    } else {
        event->ignore();
    }
}
