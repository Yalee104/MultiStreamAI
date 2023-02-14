#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QtMath>
#include <QMenu>
#include <QCameraInfo>
#include "videoview.h"
#include "Apps/appcommon.h"

#define STREAM_FPS_DEFAULT "Default"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->menubar->setNativeMenuBar(false);
    connect(&m_streamContainer, SIGNAL(ContainerViewUpdateRequest(QString, eStreamViewUpdate)), this, SLOT(handleContainerViewUpdateRequest(QString, eStreamViewUpdate)));
    QWidget *widget = new QWidget;
    widget->setLayout(&m_gridlayout);

    SetGlobalStreamFPSMenu(eStreamFPS::FPS_DEFAULT);

    setCentralWidget(widget);

}

MainWindow::~MainWindow()
{
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    pHailoPipeline->ReleaseAllResource();

    delete ui;
}

void MainWindow::SetGlobalStreamFPSMenu(eStreamFPS FPS)
{
    QString FPSstringSel = STREAM_FPS_DEFAULT;

    switch (FPS) {
    case eStreamFPS::FPS_20:
        FPSstringSel = "20";
        break;

    case eStreamFPS::FPS_15:
        FPSstringSel = "15";
        break;

    case eStreamFPS::FPS_10:
        FPSstringSel = "10";
        break;

    case eStreamFPS::FPS_5:
        FPSstringSel = "5";
        break;

    default:
        break;
    }

    for (QAction* action : ui->menubar->actions()){
        if (action->menu() && action->text().contains("FPS")) {
            for (QAction* FPSaction : action->menu()->actions()){
                //qDebug () << FPSaction->text();
                if (FPSaction->text() == FPSstringSel) {
                    FPSaction->setChecked(true);
                    if (FPSstringSel == STREAM_FPS_DEFAULT)
                        m_streamContainer.UpdateTargetFPSToAllStream(0);
                    else
                        m_streamContainer.UpdateTargetFPSToAllStream(FPSstringSel.toInt());
                }
                else {
                    FPSaction->setChecked(false);
                }
            }
        }
    }
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

void MainWindow::PlacementStreamViewRemoveAll()
{
    QList<QString> StreamViewIDList = m_streamContainer.GetAllStreamViewID();
    for (QString ID : StreamViewIDList) {
        PlacementStreamViewRemove(ID);
    }
}

void MainWindow::PlacementStreamViewRemove(QString ID)
{
    StreamView* pStreamView = m_streamContainer.GetStreamViewByID(ID);
    m_gridlayout.removeWidget(pStreamView);
    m_streamContainer.DeleteStream(ID);
    PlacementStreamViewRefreshAll();
}

StreamView*  MainWindow::PlacementStreamViewAddNew(QString ID, eStreamViewType StreamType)
{
    StreamView* pStreamView = nullptr;
    if (m_streamContainer.StreamList.size() < MAX_NUM_SUPPORTED_STREAM_VIEW) {
        pStreamView = m_streamContainer.CreateNewStream(ID, StreamType);
        connect(pStreamView, SIGNAL(stremViewWantsDeleteEvent(QString)), this, SLOT(PlacementStreamViewRemove(QString)));
        PlacementStreamViewRefreshAll();
        m_streamContainer.UpdateTargetFPSToAllStream(m_GlobalTargetFPS);
    }

    return pStreamView;
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


StreamView* MainWindow::on_actionCamera_triggered()
{
    StreamView* pStreamView = nullptr;
    pStreamView = PlacementStreamViewAddNew(QString::number(m_uniqueID), eStreamViewType::CAMERA);
    m_uniqueID++;
    return pStreamView;
}


StreamView* MainWindow::on_actionVideo_triggered()
{
    StreamView* pStreamView = nullptr;
    pStreamView = PlacementStreamViewAddNew(QString::number(m_uniqueID), eStreamViewType::VIDEO);
    m_uniqueID++;
    return pStreamView;
}


void MainWindow::on_actionStreamFPS_Default_triggered()
{
    SetGlobalStreamFPSMenu(eStreamFPS::FPS_DEFAULT);
    m_GlobalTargetFPS = 0;
}

void MainWindow::on_actionStreamFPS_20_triggered()
{
    SetGlobalStreamFPSMenu(eStreamFPS::FPS_20);
    m_GlobalTargetFPS = 20;
}

void MainWindow::on_actionStreamFPS_15_triggered()
{
    SetGlobalStreamFPSMenu(eStreamFPS::FPS_15);
    m_GlobalTargetFPS = 15;
}


void MainWindow::on_actionStreamFPS_10_triggered()
{
    SetGlobalStreamFPSMenu(eStreamFPS::FPS_10);
    m_GlobalTargetFPS = 10;
}


void MainWindow::on_actionStreamFPS_5_triggered()
{
    SetGlobalStreamFPSMenu(eStreamFPS::FPS_5);
    m_GlobalTargetFPS = 5;
}


void MainWindow::on_actionSave_Stream_Config_triggered()
{
    bool ok;
    QString ConfigFileName = QInputDialog::getText(this, tr(""),
                                         tr("Config Name to Save"), QLineEdit::Normal,
                                         tr(""), &ok);
    if (ok) {

        if (!ConfigFileName.isEmpty()) {
            ConfigFileName.append(".json");
            m_streamContainer.SaveStreamInfoToFile(ConfigFileName);
        }
        else {
            QMessageBox msgBox;
            msgBox.setText("File name cannot be empty");
            msgBox.exec();
        }

    }
}

void MainWindow::on_actionLoad_Stream_Config_triggered()
{

    QFileDialog fileDialog(this);
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setWindowTitle(tr("Open Config File"));
    fileDialog.setNameFilter("*.json");

    if (fileDialog.exec() == QDialog::Accepted) {

        PlacementStreamViewRemoveAll();

        QString FilePath = fileDialog.selectedUrls().constFirst().path();
        QFile loadFile(FilePath);

        if (loadFile.open(QIODevice::ReadOnly)) {

            QByteArray saveData = loadFile.readAll();

            QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));

            QJsonObject Streams = loadDoc.object();

            if (Streams.contains("StreamDescriptor") && Streams["StreamDescriptor"].isArray()) {

                QJsonArray StreamDescriptorList = Streams["StreamDescriptor"].toArray();
                for (int i = 0; i < StreamDescriptorList.size(); ++i) {
                    QJsonObject StreamObj = StreamDescriptorList[i].toObject();
                    if (StreamObj["Type"].toString().compare("Video") == 0) {
                        StreamView* pStreamView = on_actionVideo_triggered();
                        m_streamContainer.ConfigStream(pStreamView, StreamObj);
                    }

                    if (StreamObj["Type"].toString().compare("Camera") == 0) {
                        StreamView* pStreamView = on_actionCamera_triggered();
                        m_streamContainer.ConfigStream(pStreamView, StreamObj);
                    }
                }
            }
        }
        else {
            qWarning("Couldn't open save file.");
        }
    }
}

