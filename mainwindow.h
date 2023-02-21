#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QGridLayout>
#include <QString>
#include <QFileDialog>
#include <QStandardPaths>
#include <QInputDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QThread>
#include <QDebug>
#include "streamcontainer.h"

#define MAX_NUM_SUPPORTED_STREAM_VIEW (16)

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

enum class eStreamFPS {FPS_DEFAULT, FPS_20, FPS_15, FPS_10 ,FPS_5};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void PlacementStreamViewRefreshAll();
    StreamView*  PlacementStreamViewAddNew(QString ID, eStreamViewType StreamType);

    void SetGlobalStreamFPSMenu(eStreamFPS FPS);

protected:

    int  GetSuggestedTotalRowCol(int totalIndex);

private slots:

    void PlacementStreamViewRemove(QString ID);
    void PlacementStreamViewAddFromJsonCfg(QJsonValue StreamCfg);

    void handleContainerViewUpdateRequest(QString ID, eStreamViewUpdate Request);


    StreamView* on_actionCamera_triggered();

    StreamView* on_actionVideo_triggered();

    void on_actionStreamFPS_Default_triggered();

    void on_actionStreamFPS_15_triggered();

    void on_actionStreamFPS_10_triggered();

    void on_actionStreamFPS_5_triggered();

    void on_actionStreamFPS_20_triggered();

    void on_actionSave_Stream_Config_triggered();

    void on_actionLoad_Stream_Config_triggered();

    void StreamCfgLoadingDone();

private:
    Ui::MainWindow  *ui;
    StreamContainer m_streamContainer;
    QGridLayout     m_gridlayout;
    int             m_uniqueID = 0;
    int             m_GlobalTargetFPS = 0; //0 is the default depending on the stream fps.
};


class StreamLoadingProgress : public QThread
{
    Q_OBJECT
    void run() override {

        int delaysInMs = 50;
        int progressStepPerStream = 0;

        QProgressDialog progress("Removing Stream Views...", "Abort", 0, 100);
        progress.setWindowModality(Qt::WindowModal);
        progress.setMinimumDuration(0);

        /* First remove all open stream */

        if (StreamViewIDList.count())
            progressStepPerStream = 50 / StreamViewIDList.count();

        for (QString ID : StreamViewIDList) {
            emit RemoveStreamViewByID(ID);

            usleep(delaysInMs*1000);
            progress.setValue(progress.value() + progressStepPerStream);
        }

        /* Now we add streams */

        progress.setLabelText("Loading Stream Views...");
        usleep(500*1000);

        if (StreamCfgJsonArray.count())
            progressStepPerStream = 50 / StreamCfgJsonArray.count();

        for (auto StreamJsonValue : StreamCfgJsonArray) {

            progress.setValue(progress.value() + progressStepPerStream);

            emit AddStreamFromJsonCfg(StreamJsonValue);

            usleep(delaysInMs*1000);
        }

        progress.setValue(100);
        emit AllDone();
    }

public:
    QJsonArray     StreamCfgJsonArray;
    QList<QString> StreamViewIDList;

signals:
    void RemoveStreamViewByID(QString ID);
    void AddStreamFromJsonCfg(QJsonValue StreamCfg);
    void AllDone();
};


#endif // MAINWINDOW_H
