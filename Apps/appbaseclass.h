#ifndef APPBASECLASS_H
#define APPBASECLASS_H

#include <QObject>
#include <QGraphicsObject>
#include <QThread>
#include <QRunnable>
#include <QThreadPool>

class AppBaseClass : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit    AppBaseClass(QObject *parent = nullptr);
    ~AppBaseClass();

    virtual     void    ImageInfer(const QImage& frame) = 0;

signals:

    void sendAppResultImage(const QImage& frame, const QList<QGraphicsItem*> &overlayItems);


    // QRunnable interface
public:
    virtual void run() = 0;

    void setTerminate(bool newTerminate);

protected:
    bool            m_Terminate = false;
    QList<QImage>   *m_pImageInferQueue = nullptr;

};

#endif // APPBASECLASS_H
