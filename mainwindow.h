#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QGridLayout>
#include <QString>
#include "streamcontainer.h"

#define MAX_NUM_SUPPORTED_STREAM_VIEW (16)

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void PlacementStreamViewRefreshAll();
    void PlacementStreamViewAddNew(QString ID, eStreamViewType StreamType);

protected:

    int GetSuggestedTotalRowCol(int totalIndex);

private slots:

    void PlacementStreamViewRemove(QString ID);

    void handleContainerViewUpdateRequest(QString ID, eStreamViewUpdate Request);


    void on_actionCamera_triggered();

    void on_actionVideo_triggered();

private:
    Ui::MainWindow  *ui;
    StreamContainer m_streamContainer;
    QGridLayout     m_gridlayout;
    int             m_uniqueID = 0;
};
#endif // MAINWINDOW_H
