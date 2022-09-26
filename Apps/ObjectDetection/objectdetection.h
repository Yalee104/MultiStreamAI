#ifndef OBJECTDETECTION_H
#define OBJECTDETECTION_H

#include <QObject>
#include <QMutex>
#include "Apps/appbaseclass.h"

class ObjectDetection : public AppBaseClass
{
    Q_OBJECT
public:
    Q_INVOKABLE             ObjectDetection(QObject *parent = nullptr);
    static const QString    Name();


    void ImageInfer(const QImage& frame) override;
    void run() Q_DECL_OVERRIDE;

    void chas(int(*func)(const QImage &image), const QImage &image);

signals:


protected:
    QMutex  ResourceLock;

};

#endif // OBJECTDETECTION_H
