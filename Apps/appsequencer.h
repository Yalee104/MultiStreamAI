#ifndef APPSEQUENCER_H
#define APPSEQUENCER_H

#include <QtConcurrent>
#include <QImage>

enum class eAppSequencerState { INIT, READY, STARTED, INIT_FUNCTION_ERROR };
enum class eSequencerState { INIT, READY, STARTED, DONE };

enum class eAppSeqErrorCode { NOT_READY, OK, SOME_DONE, ERROR_INIT_FUNCTION_FAILED };

template <class T, class I>
class AppSequencer
{
    typedef int (*InitFunc)(I* pInitData);
    typedef int (*SequenceFunc)(I* pInitData, T* pShareData, const QImage &image);
    typedef int (*ShareDataCleanupFunc)(T* pShareData);

    struct  SequencerData {
        T*                      pShareData;
        QImage                  OriginalImage;
        eSequencerState         State;
        QList<QFuture<void>>    Futures;
        int                     NextSequenceIndex;
    };

    struct  SequenceFunctionInfo {
        SequenceFunc        func;
        bool                bParallelExecution;
    };

public:
    struct  TaskSequenceInfo {
        bool                bAvailable;
        T*                  pShareData;
        QImage              OriginalImage;
        int                 TaskSequenceIndex;
    };

public:
    AppSequencer(I* pInitData) {
        MainState = eAppSequencerState::INIT;
        pWorkInitData = pInitData;
    };

    void        AddInitializer(InitFunc func) {
        InitFunctionList.push_back(func);
    };

    void        AddSequencer(SequenceFunc func, bool ParallelExecution) {

        SequenceFunctionInfo FunctionInfo;
        FunctionInfo.func = func;
        FunctionInfo.bParallelExecution = ParallelExecution;
        SequenceFunctionList.push_back(FunctionInfo);

        if (ShareDataDeleteFunc != nullptr)
            MainState = eAppSequencerState::READY;
    };

    void        setShareDataCleanupFunc(ShareDataCleanupFunc func) {
        ShareDataDeleteFunc = func;
        if (SequenceFunctionList.count())
             MainState = eAppSequencerState::READY;
    };

    eAppSeqErrorCode    SendSequenceTask(T *pShareData, const QImage &OriginalImage) {

        if ((MainState != eAppSequencerState::READY) && (MainState != eAppSequencerState::STARTED))
            return eAppSeqErrorCode::NOT_READY;

        SequencerData NewSequenceData;
        NewSequenceData.pShareData = pShareData;
        NewSequenceData.OriginalImage = OriginalImage;
        NewSequenceData.State = eSequencerState::READY;
        NewSequenceData.Futures.clear();
        NewSequenceData.NextSequenceIndex = 0;
        SequencerTask.push_back(NewSequenceData);

        return eAppSeqErrorCode::OK;
    };

    TaskSequenceInfo    RetrieveSequenceTaskDoneInfo() {

        TaskSequenceInfo SequenceTaskDoneInfo;
        SequenceTaskDoneInfo.bAvailable = false;
        if (SequencerTask.count()) {
            for (int i = 0; i < SequencerTask.count(); i++) {
                if (SequencerTask[i].State == eSequencerState::DONE) {
                    SequenceTaskDoneInfo.bAvailable = true;
                    SequenceTaskDoneInfo.TaskSequenceIndex = i;
                    SequenceTaskDoneInfo.OriginalImage = SequencerTask[i].OriginalImage;
                    SequenceTaskDoneInfo.pShareData = SequencerTask[i].pShareData;
                    break;
                }
            }
        }
        return SequenceTaskDoneInfo;
    };

    void                RemoveSequenceTask(TaskSequenceInfo &TaskSequenceToRemove) {
        if (TaskSequenceToRemove.TaskSequenceIndex <  SequencerTask.count()) {
            if (ShareDataDeleteFunc)
                ShareDataDeleteFunc(SequencerTask[TaskSequenceToRemove.TaskSequenceIndex].pShareData);
            SequencerTask.removeAt(TaskSequenceToRemove.TaskSequenceIndex);
        }
    };

    eAppSeqErrorCode     SequencerStepRun() {

        eAppSeqErrorCode ErrorCode = eAppSeqErrorCode::OK;

        if (MainState == eAppSequencerState::INIT)
            return ErrorCode = eAppSeqErrorCode::NOT_READY;

        //We will only call initialization function once.
        if (MainState == eAppSequencerState::READY) {
            if (InitFunctionList.count()) {
                MainState = eAppSequencerState::STARTED;
                QFutureSynchronizer<int> synchronizer;
                for (int i = 0; i < InitFunctionList.count(); i++) {
                    synchronizer.addFuture(QtConcurrent::run(InitFunctionList[i], pWorkInitData));
                }
                synchronizer.waitForFinished();
                const QList<QFuture<int>> FutureResults = synchronizer.futures();
                for (const QFuture<int> &result : FutureResults) {
                    if (result.result() != 0) {
                        ErrorCode = eAppSeqErrorCode::ERROR_INIT_FUNCTION_FAILED;
                        MainState = eAppSequencerState::INIT_FUNCTION_ERROR;
                        break;
                    }
                }
            }
        }

        if (MainState == eAppSequencerState::STARTED) {
            if (SequencerTask.count()) {

                for (int i = 0; i < SequencerTask.count(); i++) {
                    if (SequencerTask[i].State == eSequencerState::READY) {
                        if (SequencerTask[i].Futures.count()) {
                            //We have running task taking in place
                            bool bAllDone = true;

                            for (QFuture<void> TaskFuture : SequencerTask[i].Futures) {
                                if (!TaskFuture.isFinished()) {
                                    bAllDone = false;
                                    break;
                                }
                            }

                            if (bAllDone) {
                                SequencerTask[i].Futures.clear();
                                SequencerTask[i].NextSequenceIndex++;
                            }
                        }
                        else {
                            //No task
                            int NextSequenceIndex = SequencerTask[i].NextSequenceIndex;
                            if (NextSequenceIndex >= SequenceFunctionList.count()) {
                                //We are all done
                                SequencerTask[i].State = eSequencerState::DONE;
                                ErrorCode = eAppSeqErrorCode::SOME_DONE;
                            }
                            else {
                                //Let run task
                                //TODO: Implement parallel task
                                QFuture<void> TaskFuture = QtConcurrent::run(SequenceFunctionList[NextSequenceIndex].func, pWorkInitData, SequencerTask[i].pShareData, SequencerTask[i].OriginalImage);
                                SequencerTask[i].Futures.push_back(TaskFuture);
                            }
                        }
                    }
                }
            }
        }
        return ErrorCode;
    };

protected:
    eAppSequencerState              MainState;
    I*                              pWorkInitData = nullptr;
    QList<SequencerData>            SequencerTask;
    QList<SequenceFunctionInfo>     SequenceFunctionList;
    QList<InitFunc>                 InitFunctionList;
    ShareDataCleanupFunc            ShareDataDeleteFunc = nullptr;
};




#endif // APPSEQUENCER_H
