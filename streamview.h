#ifndef STREAMVIEW_H
#define STREAMVIEW_H

#include <QObject>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QGraphicsPixmapItem>

class StreamView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit    StreamView(QString NewID, QWidget *parent = nullptr, int viewSizeMinW = 160, int viewSizeMinH = 160);
    ~StreamView();
    bool        getHidden() const;
    void        setHidden(bool newHidden);
    void        ClearMainImageGroupOverlay();
    void        setMainImageGroupOverlay(const QList<QGraphicsItem*> &overlayItems);
    void        drawStreamFramePixmap(const QPixmap& frame);
    void        setHelperDescription(QString newHelperDescription);

signals:
    Q_INVOKABLE void stremViewWantsDeleteEvent(QString ID);

protected:
    void mousePressEvent(QMouseEvent * e);


public:
    QString         ID;


protected:
    QGraphicsScene          m_scene;
    QGraphicsPixmapItem     m_videoPixelmapItem;
    QGraphicsTextItem*      m_pHelperDescription = nullptr;  //auto release by parent
    QGraphicsItemGroup*     m_pMainImageItemGroup = nullptr; //auto release by parent
    bool Hidden;

};


#endif // STREAMVIEW_H
