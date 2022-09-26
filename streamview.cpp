#include "streamview.h"
#include <QSizePolicy>
#include <QDebug>
#include <QColor>
#include <QTextDocument>

StreamView::StreamView(QString NewID, QWidget *parent, int viewSizeMinW, int viewSizeMinH) :
    QGraphicsView(parent),
    ID(NewID)
{

    setScene(&m_scene);
    m_scene.addItem(&m_videoPixelmapItem);

    m_pHelperDescription = m_scene.addText("Right Click For\n More Options");
    m_pHelperDescription->setDefaultTextColor(Qt::white);
    QFont HelperFont;
    HelperFont.setBold(true);
    HelperFont.setPointSize(12);
    m_pHelperDescription->setFont(HelperFont);

    //NOTE: Can turn on Smooth transformation so that thin line drawn does not dissapear when scaled
    //m_videoPixelmapItem.setTransformationMode(Qt::SmoothTransformation);

    //Create main image item group with m_videoPixelmapItem as the parent (first one)
    //NOTE: All added item to this group will automatically scale and position with respect to the m_videoPixelmapItem
    m_pMainImageItemGroup = m_scene.createItemGroup(QList<QGraphicsItem*>({&m_videoPixelmapItem}));

    //TEST Sample - To be removed
    //QGraphicsRectItem* myRect =  m_scene.addRect(0,0,100,100);
    //m_pMainImageItemGroup->addToGroup(myRect);


    this->setMinimumSize(viewSizeMinW, viewSizeMinH);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setBackgroundBrush(Qt::black);
}

StreamView::~StreamView()
{

}

void StreamView::mousePressEvent(QMouseEvent * e)
{
    qDebug() << "StreamView mousePressEvent not implemented " << e->pos();
}

void StreamView::setHelperDescription(QString newHelperDescription)
{
    m_pHelperDescription->setPlainText(newHelperDescription);
}


bool StreamView::getHidden() const
{
    return Hidden;
}

void StreamView::setHidden(bool newHidden)
{
    if (Hidden == newHidden)
        return;

    if (newHidden == true)
        hide();
    else
        show();

    Hidden = newHidden;
}

void StreamView::ClearMainImageGroupOverlay()
{
    QList<QGraphicsItem *> CurrentList = m_pMainImageItemGroup->childItems();
    if (CurrentList.size() > 1) {
        for (int i = 1; i < CurrentList.size(); i++) {
            QGraphicsItem * itemToRemove = CurrentList[i];
            m_pMainImageItemGroup->removeFromGroup(itemToRemove);
            delete itemToRemove;
        }
    }
}

void StreamView::setMainImageGroupOverlay(const QList<QGraphicsItem *> &overlayItems)
{
    //WARNING: The main image item group st transform will not take imediate effect
    //         if Calling this function right before drawStreamFramePixmap.
    //         it will only take effect in the next graphics run loop
    //         therefore, removing and adding item "every" frame is not recommended.
    for (QGraphicsItem* OverlayItem : overlayItems) {
        m_pMainImageItemGroup->addToGroup(OverlayItem);
    }
}

void StreamView::drawStreamFramePixmap(const QPixmap &frame)
{
    m_pHelperDescription->hide();

    //Set the scen rect to same size as the view
    m_scene.setSceneRect(-1,-1,width(),height());

    //Get the scale aspect ratio fit for the main image item
    QSize MainImageItemSize = mapFromScene(m_videoPixelmapItem.sceneBoundingRect()).boundingRect().size();
    QSize MainImageItemScaled = MainImageItemSize.scaled(viewport()->size(), Qt::KeepAspectRatio);
    double ratio = qMin((double)MainImageItemScaled.width()/MainImageItemSize.width(), (double)MainImageItemScaled.height()/MainImageItemSize.height());

    //Apply the ratio to the item group transform and offset to be centered on the view
    //NOTE: this will "automatically" position and scale all drawm item with respect to the MainImageItem (which is m_videoPixelmapItem)
    m_pMainImageItemGroup->setTransform(QTransform::fromScale(ratio,ratio), true);
    int offsetX = (width() - m_videoPixelmapItem.sceneBoundingRect().width()) / 2;
    int offsetY = (height() - m_videoPixelmapItem.sceneBoundingRect().height()) / 2;
    m_pMainImageItemGroup->setPos(offsetX,offsetY);

    m_videoPixelmapItem.setPixmap(frame);

}
