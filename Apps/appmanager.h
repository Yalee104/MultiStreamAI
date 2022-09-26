#ifndef APPMANAGER_H
#define APPMANAGER_H

#include <QObject>
#include <QMenu>
#include <QMutex>
#include "AppsFactory.h"
#include "Apps/appbaseclass.h"

class AppManager : public QObject
{
    Q_OBJECT
public:
    explicit AppManager(QObject *parent = nullptr);
    ~AppManager();

    QMenu *getAppMenu() const;
    bool  AppSelected();

    void  AppImageInfer(const QImage& frame);

private:
    void  ReleaseCurrentApp();

signals:

    void sendImage(const QImage& frame, const QList<QGraphicsItem*> &overlayItems);

public slots:
    void AppSelectionTrigger(QAction *action);

protected:

    QMenu*          m_pAppMenu = nullptr;
    AppBaseClass*   m_pAppRunnableObject = nullptr;
    AppsFactory     m_AppsFactory;
    QMutex          m_AppAccessMutex;
};

#endif // APPMANAGER_H
