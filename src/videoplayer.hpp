#ifndef VIDEOPLAYER_HPP
#define VIDEOPLAYER_HPP

#include <QMouseEvent>
#include <QVideoWidget>

class VideoPlayer : public QVideoWidget
{
    Q_OBJECT
    bool m_firstTime;
public:
    explicit VideoPlayer(QWidget *parent = nullptr);
protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void showEvent(QShowEvent *event) override;
signals:
    // Ask to pause the video when the user right clicks the video area.
    void pause();
};

#endif // VIDEOPLAYER_HPP
