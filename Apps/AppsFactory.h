#ifndef APPSFACTORY_H
#define APPSFACTORY_H

#include <QByteArray>
#include <QHash>
#include <QObject>

class AppsFactory
{
private:
   QHash<QByteArray,const QMetaObject*> metaObjects;

public:
   template<class T>
   QByteArray registerObject()
   {
        metaObjects.insert( T::staticMetaObject.className(), &(T::staticMetaObject) );
        return QByteArray(T::staticMetaObject.className());
   }

   bool    isObjectRegistered(const QByteArray &type)
   {
       const QMetaObject *meta = metaObjects.value( type );
       return meta ? true : false;
   }

   QObject *createObject( const QByteArray &type )
   {
        const QMetaObject *meta = metaObjects.value( type );
        return meta ? meta->newInstance() : 0;
   }
};


#endif // APPSFACTORY_H
