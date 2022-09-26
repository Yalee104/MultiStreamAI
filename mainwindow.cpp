#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QtMath>
#include <QMenu>
#include <QCameraInfo>
#include "videoview.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(&m_streamContainer, SIGNAL(ContainerViewUpdateRequest(QString, eStreamViewUpdate)), this, SLOT(handleContainerViewUpdateRequest(QString, eStreamViewUpdate)));
    QWidget *widget = new QWidget;
    widget->setLayout(&m_gridlayout);

    setCentralWidget(widget);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::PlacementStreamViewRefreshAll()
{
    int TotalStreamCount = m_streamContainer.GetVisibleStreamViewCount();
    int TotalRowCol = GetSuggestedTotalRowCol(TotalStreamCount);
    int VisibleViewIndex = 0;

    //Here we need to set all existing row and column stretch to 0 (auto) first
    //Otherwise when we reduce view it wll not automatically fit to man view.
    for(int c=0; c < m_gridlayout.columnCount(); c++) m_gridlayout.setColumnStretch(c,0);
    for(int r=0; r < m_gridlayout.rowCount(); r++)  m_gridlayout.setRowStretch(r,0);

    //TODO: Maybe we should add a loop here to remove all widget first.
    //NOTE: From experimentation it looks like without removing widget looks fine but not sure if
    //      there is memory leak or not. Grid layout add widget behaviour as follow
    //      1. If the grid already contain widget then adding new widget to this grid will not override it.
    //      2. You cannot add same widget on more than one grid

    for (int i = 0; i < m_streamContainer.StreamList.size(); ++i) {
        if (!m_streamContainer.StreamList.at(i)->getHidden()) {
            int atRow = VisibleViewIndex / TotalRowCol;
            int atCol = VisibleViewIndex % TotalRowCol;

            m_gridlayout.addWidget(m_streamContainer.StreamList.at(i),atRow,atCol);
            m_gridlayout.setColumnStretch(atCol,1);
            m_gridlayout.setRowStretch(atRow,1);
            VisibleViewIndex++;
        }
    }
}

void MainWindow::PlacementStreamViewRemove(QString ID)
{
    StreamView* pStreamView = m_streamContainer.GetStreamViewByID(ID);
    m_gridlayout.removeWidget(pStreamView);
    m_streamContainer.DeleteStream(ID);
    PlacementStreamViewRefreshAll();
}

void MainWindow::PlacementStreamViewAddNew(QString ID, eStreamViewType StreamType)
{
    if (m_streamContainer.StreamList.size() < MAX_NUM_SUPPORTED_STREAM_VIEW) {
        StreamView* pStreamView = m_streamContainer.CreateNewStream(ID, StreamType);
        connect(pStreamView, SIGNAL(stremViewWantsDeleteEvent(QString)), this, SLOT(PlacementStreamViewRemove(QString)));
        PlacementStreamViewRefreshAll();
    }
}

int MainWindow::GetSuggestedTotalRowCol(int totalIndex)
{
    if (totalIndex <= 4)
        return 2;

    if (totalIndex <= 9)
        return 3;

    if (totalIndex <= MAX_NUM_SUPPORTED_STREAM_VIEW)
        return qSqrt(MAX_NUM_SUPPORTED_STREAM_VIEW);

    return 0;
}


void MainWindow::handleContainerViewUpdateRequest(QString ID, eStreamViewUpdate Request)
{
    switch(Request){
        case(eStreamViewUpdate::HIDDEN):
        {
            StreamView*   pStreamView = m_streamContainer.GetStreamViewByID(ID);
            pStreamView->setHidden(true);
            PlacementStreamViewRefreshAll();
            qDebug() << ID << " is now Hidden";
            break;
        }
        case(eStreamViewUpdate::REMOVE):
        {
            qDebug() << ID << " wants to remove (TO BE IMPLEMENTED)";
            break;
        }
        default:
           break;
    }
}


void MainWindow::on_actionCamera_triggered()
{
    PlacementStreamViewAddNew(QString::number(m_uniqueID), eStreamViewType::CAMERA);
    m_uniqueID++;
}


void MainWindow::on_actionVideo_triggered()
{
    PlacementStreamViewAddNew(QString::number(m_uniqueID), eStreamViewType::VIDEO);
    m_uniqueID++;
}

