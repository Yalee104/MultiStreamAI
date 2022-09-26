#include "mainwindow.h"
#include <QThread>
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QThread::currentThread()->setObjectName("Main Thread");

    MainWindow w;
    w.show();
    return a.exec();
}
