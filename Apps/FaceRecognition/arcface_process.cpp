#include "arcface_process.h"
#include "yolov5_faceDet_process.h"
#include <opencv2/opencv.hpp>

#include <QThread>
#include <QElapsedTimer>
#include <QDir>

//TODO: Revise the use of define and use more structurized design pattern
#define TRACKER_USE
#define OPENCV_RESIZE


#ifdef TRACKER_USE

#include "hailo_tracker.hpp"

std::vector<int> face_tracked_ids;

#endif

#define FACE_TRACKER_NEED_PREDICT   (0)
#define FACE_TRACKER_IDENTIFIED     (1)
#define FACE_TRACKER_UNIDENTIFIED   (2)

std::vector<std::pair<std::string,std::vector<float32_t>>> gFacesDB;

//http://mlwiki.org/index.php/Cosine_Similarity
//https://www.machinelearningplus.com/nlp/cosine-similarity/
float32_t cosine_similarity(std::vector<float32_t> a, std::vector<float32_t> b)
{
    size_t len = a.size();
    float32_t dot = 0;
    float32_t denom_a = 0;
    float32_t denom_b = 0;
    for (size_t i = 0; i < len; i++)
    {
        dot += a[i] * b[i];
        denom_a += a[i] * a[i];
        denom_b += b[i] * b[i];
    }

    return dot / (sqrt(denom_a) * sqrt(denom_b));
}

template <typename T>
QImage paddedImage(const QImage & source, int targetWidth, int TargetHeight, T padValue)
{
    int padWidth = (targetWidth - source.width())/2;
    int padHeight = (TargetHeight - source.height())/2;

    QImage padded{targetWidth, TargetHeight, source.format()};
    padded.fill(padValue);
    QPainter p{&padded};
    p.drawImage(QPoint(padWidth, padHeight), source);
    return padded;
}

cv::Mat ResizeAspectRatioFit( const cv::Mat& img, int target_width = 640, int target_height = 640, int color = 114 )
{
    int width = img.cols,
        height = img.rows;

    cv::Mat aspectfit( target_height, target_width, img.type(), cv::Scalar(color, color, color) );

    float scaleHeight = (float)target_height / (float)height;
    float scaleWidth = (float)target_width / (float)width;
    float scale = MIN(scaleHeight, scaleWidth);

    cv::Rect roi;
    if (width > height) {
        roi.width = target_width;
        roi.x = 0;
        roi.height = height * scale;
        roi.y = ( target_height - roi.height ) / 2;
    } else {
        roi.y = 0;
        roi.height = target_height;
        roi.width = width * scale;
        roi.x = ( target_width - roi.width ) / 2;
    }

    cv::resize( img, aspectfit( roi ), roi.size() );

    return aspectfit;
}


void Arface_BuildFaceDB(const QString &FaceDBPath, FaceRecognitionInfo* pInitData)
{
    Timer   TimerCheck;
    MnpReturnCode ReadOutRet = MnpReturnCode::NO_DATA_AVAILABLE;
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();

    int TotalInfer = 0;
    QList<int> InferedIndex;
    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.jpeg";
    QFileInfoList fileInfoList = QDir(FaceDBPath).entryInfoList(filters, QDir::Files|QDir::NoDotAndDotDot);
    for (int i = 0; i < fileInfoList.size(); ++i) {
        QFileInfo fileInfo = fileInfoList.at(i);
        qDebug() << "DB File name: " << fileInfo.fileName();

#ifdef OPENCV_RESIZE

        cv::Mat out_frame;
        cv::Mat org_frame = cv::imread(fileInfo.absoluteFilePath().toStdString());

        if (!org_frame.empty()) {
            cv::cvtColor(org_frame, org_frame, cv::COLOR_BGR2RGB);
            out_frame = ResizeAspectRatioFit( org_frame, 112, 112);

            int totalsz = out_frame.dataend - out_frame.datastart;
            pInitData->ImageInputRaw.assign(out_frame.datastart, out_frame.datastart+totalsz);

            pHailoPipeline->Infer(pInitData->ModelID, pInitData->ImageInputRaw, pInitData->AppID);
            TotalInfer++;

            InferedIndex.push_back(i);
        }
        else {
            qDebug() << "WARNING: Face DB image is not supported: " << fileInfo.absoluteFilePath();
        }

#else //Use native QT QImage but with less performance although best for cross platform compatibility

        QImage scaledImage = QImage(fileInfo.absoluteFilePath()).scaled(pInitData->NetworkInputWidth, pInitData->NetworkInputHeight, Qt::KeepAspectRatio);
        scaledImage.convertTo(QImage::Format_RGB888);

        if (!scaledImage.isNull()) {

            auto padded = paddedImage(scaledImage, pInitData->NetworkInputWidth, pInitData->NetworkInputHeight, Qt::gray);
            //padded.save("SavedImage.jpg");
            const uchar* pImagedata = padded.bits();

            pInitData->ImageInputRaw.assign(pImagedata, pImagedata+padded.sizeInBytes());
            //pInitData->ImageInputRaw.resize(pInitData->NetworkInputSize, 0);

            pHailoPipeline->Infer(pInitData->ModelID, pInitData->ImageInputRaw, pInitData->AppID);
            TotalInfer++;

            InferedIndex.push_back(i);
        }
        else {
            qDebug() << "WARNING: Face DB image is not supported: " << fileInfo.absoluteFilePath();
        }
#endif
    }

    while (TotalInfer)
    {
        if (pInitData->OutputFormat == HAILO_FORMAT_TYPE_UINT8)
            ReadOutRet = pHailoPipeline->ReadOutputById(pInitData->ModelID, pInitData->OutputBufferUint8, pInitData->AppID);
        else //Must be float 32
            ReadOutRet = pHailoPipeline->ReadOutputById(pInitData->ModelID, pInitData->OutputBufferFloat32, pInitData->AppID);

        if (ReadOutRet == MnpReturnCode::SUCCESS) {

            TotalInfer--;
            int ImageDbIndex = InferedIndex.front();

            gFacesDB.push_back(std::make_pair(fileInfoList[ImageDbIndex].baseName().toStdString(), pInitData->OutputBufferFloat32[0]));

            InferedIndex.pop_front();
        }
        else {
            if (TimerCheck.isTimePastSec(2.0)) {
                break;
            }

            QThread::currentThread()->usleep(100);
        }
    }
}

#if 1
void Arcface_Test(const QString &TestImagePath, const QString &ImageFileName, FaceRecognitionInfo* pInitData)
{
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    MnpReturnCode ReadOutRet = MnpReturnCode::NO_DATA_AVAILABLE;
    if (QDir(TestImagePath).exists()) {

        if (QDir(TestImagePath).exists(ImageFileName)) {

            QElapsedTimer timer;
            timer.start();

#ifdef OPENCV_RESIZE

        cv::Mat out_frame;
        cv::Mat org_frame = cv::imread(QDir(TestImagePath).absoluteFilePath(ImageFileName).toStdString());

        if (!org_frame.empty()) {
            cv::cvtColor(org_frame, org_frame, cv::COLOR_BGR2RGB);
            out_frame = ResizeAspectRatioFit( org_frame, 112, 112);

            int totalsz = out_frame.dataend - out_frame.datastart;
            pInitData->ImageInputRaw.assign(out_frame.datastart, out_frame.datastart+totalsz);

            pHailoPipeline->Infer(pInitData->ModelID, pInitData->ImageInputRaw, pInitData->AppID);
        }
        else {
            qDebug() << "WARNING: Face DB image is not supported: " << QDir(TestImagePath).absoluteFilePath(ImageFileName);
        }

#else //Use native QT QImage but with less performance although best for cross platform compatibility

        QImage scaledImage = QImage(QDir(TestImagePath).absoluteFilePath(ImageFileName)).scaled(pInitData->NetworkInputWidth, pInitData->NetworkInputHeight, Qt::KeepAspectRatio);

        if (!scaledImage.isNull()) {

            scaledImage.convertTo(QImage::Format_RGB888);

            auto padded = paddedImage(scaledImage, pInitData->NetworkInputWidth, pInitData->NetworkInputHeight, Qt::gray);
            //padded.save("SavedImage_Test.jpg");
            const uchar* pImagedata = padded.bits();

            pInitData->ImageInputRaw.assign(pImagedata, pImagedata+padded.sizeInBytes());

            qDebug() << "output readed at " << timer.nsecsElapsed();

            pHailoPipeline->Infer(pInitData->ModelID, pInitData->ImageInputRaw, pInitData->AppID);

        }
        else {
            qDebug() << "WARNING: Face DB image is not supported: " << fileInfo.absoluteFilePath();
        }
#endif


            while(1) {

                ReadOutRet = pHailoPipeline->ReadOutputById(pInitData->ModelID, pInitData->OutputBufferFloat32, pInitData->AppID);

                if (ReadOutRet == MnpReturnCode::SUCCESS) {

                    //Here we compare the face database and identify the face.
                    float32_t   best_score = 0.0f;
                    size_t      best_match_index = -1;
                    qDebug() << "Face DB size: " << gFacesDB.size();
                    for (size_t k=0; k < gFacesDB.size(); k++)
                    {
                        float32_t score = cosine_similarity(pInitData->OutputBufferFloat32[0], gFacesDB[k].second);

                        if (score > best_score){
                            best_score = score;
                            best_match_index = k;
                            qDebug() << "Face Best = " << gFacesDB[k].first.c_str() << ", score = " << best_score;
                        }
                    }

                    if (best_match_index >= 0 && best_score > 0.7)
                    {
                        qDebug() << "Face Found = " << gFacesDB[best_match_index].first.c_str() << ", score = " << best_score;
                    }
                    else
                    {
                        qDebug() << "Face Best but not reaching target score = " << gFacesDB[best_match_index].first.c_str() << ", score = " << best_score;
                    }

                    break;
                }
            }
        }
    }
    else {
        qDebug() << "Image Path does not exist";
    }
}
#else


void Arcface_Test(const QString &TestImagePath, const QString &ImageFileName, FaceRecognitionInfo* pInitData)
{
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    MnpReturnCode ReadOutRet = MnpReturnCode::NO_DATA_AVAILABLE;
    if (QDir(TestImagePath).exists()) {

        if (QDir(TestImagePath).exists(ImageFileName)) {

            QElapsedTimer timer;
            timer.start();

            cv::Mat out_frame;
            cv::Mat org_frame = cv::imread(QDir(TestImagePath).absoluteFilePath(ImageFileName).toStdString());
            cv::cvtColor(org_frame, org_frame, cv::COLOR_BGR2RGB);
            out_frame = ResizeAspectRatioFit( org_frame, 112, 112);
            //cv::imwrite("cv_out_frame.jpg", out_frame);

            int totalsz = out_frame.dataend - out_frame.datastart;

            pInitData->ImageInputRaw.assign(out_frame.datastart, out_frame.datastart+totalsz);

            qDebug() << "output readed at " << timer.nsecsElapsed();

            pHailoPipeline->Infer(pInitData->ModelID, pInitData->ImageInputRaw, pInitData->AppID);


            while(1) {

                ReadOutRet = pHailoPipeline->ReadOutputById(pInitData->ModelID, pInitData->OutputBufferFloat32, pInitData->AppID);

                if (ReadOutRet == MnpReturnCode::SUCCESS) {

                    //Here we compare the face database and identify the face.
                    float32_t   best_score = 0.0f;
                    size_t      best_match_index = -1;
                    qDebug() << "Face DB size: " << gFacesDB.size();
                    for (size_t k=0; k < gFacesDB.size(); k++)
                    {
                        float32_t score = cosine_similarity(pInitData->OutputBufferFloat32[0], gFacesDB[k].second);

                        if (score > best_score){
                            best_score = score;
                            best_match_index = k;
                            qDebug() << "Face Best = " << gFacesDB[k].first.c_str() << ", score = " << best_score;
                        }
                    }

                    if (best_match_index >= 0 && best_score > 0.7)
                    {
                        qDebug() << "Face Found = " << gFacesDB[best_match_index].first.c_str() << ", score = " << best_score;
                    }
                    else
                    {
                        qDebug() << "Face Best but not reaching target score = " << gFacesDB[best_match_index].first.c_str() << ", score = " << best_score;
                    }

                    break;
                }
            }
        }
        else {
            qDebug() << "WARNING: Face image is not supported: " << ImageFileName;
        }
    }
    else {
        qDebug() << "Image Path does not exist";
    }
}

#endif


int Arcface_Initialize(FaceRecognitionInfo* pFaceInfo, std::string AppID) {
    //qDebug() << "Arcface_Initialize";

    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    stNetworkModelInfo Network;
    Network.id_name = "ArcfaceMobile";
    Network.hef_path = "arcface_mobilefacenet.hef";
    //Network.hef_path = "arcface_r50.hef";

    Network.batch_size = 1;
    Network.in_quantized = false;
    Network.in_format = HAILO_FORMAT_TYPE_UINT8;
    Network.out_quantized = false;
    Network.out_format = HAILO_FORMAT_TYPE_FLOAT32;
    pHailoPipeline->AddNetwork(0, Network);

    pFaceInfo->ModelID = Network.id_name;
    pFaceInfo->AppID = AppID;
    pFaceInfo->OutputFormat = Network.out_format;
    pFaceInfo->NetworkInputHeight = 112;
    pFaceInfo->NetworkInputWidth = 112;
    pFaceInfo->NetworkInputSize = pFaceInfo->NetworkInputHeight*pFaceInfo->NetworkInputWidth*3; //RGB channel

    pHailoPipeline->GetNetworkQuantizationInfo(pFaceInfo->ModelID, pFaceInfo->QuantizationInfo);

#ifdef TRACKER_USE
    HailoTracker::GetInstance().add_jde_tracker(pFaceInfo->AppID);
    //We want to keep past metadata since we will be adding user specific meta data
    HailoTracker::GetInstance().set_keep_past_metadata(pFaceInfo->AppID, true);
#endif

    return 0;
}

void ArcFace_InferWorker(FaceRecognitionInfo* pFaceInfo, ObjectDetectionInfo* pDetectionInfo, ObjectDetectionData* pDetectionData)
{
    //qDebug() << "ArcFace_InferWorker";
    bool NewTrackObjFound = false;
    static Timer   TimerCheck;
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();

#ifdef TRACKER_USE

    //TODO: We should filter out class that is not face so that it does not go into tracker to save time especially when
    //      there is multi-stream.
    std::vector<HailoDetectionPtr> FaceTracker = HailoTracker::GetInstance().update(pFaceInfo->AppID, pDetectionData->DecodedResult);


    for (HailoDetectionPtr  &tracker : FaceTracker) {

        //Here we only check for face class
        if (tracker->get_class_id() != YOLO_PERSONFACE_FACE_CLASS_ID)
            continue;

        auto unique_ids = hailo_common::get_hailo_unique_id(tracker);
        if (std::find(face_tracked_ids.begin(), face_tracked_ids.end(), unique_ids[0]->get_id()) != face_tracked_ids.end())
        {
            // this track id was already updated
            continue;
        }
        else
        {
            NewTrackObjFound = true;
            face_tracked_ids.emplace_back(unique_ids[0]->get_id());

            HailoUserMetaPtr usermeta = std::make_shared<HailoUserMeta>(HailoUserMeta(FACE_TRACKER_NEED_PREDICT,"Stranger",0));
            HailoTracker::GetInstance().add_object_to_track(pFaceInfo->AppID, unique_ids[0]->get_id(), usermeta);

            //NOTE: this list will keep accumulating, during actual implementation need to clean up.
            //qDebug() << "list id size: " << face_tracked_ids.size();
            //qDebug() << "New Tracked ID: " << unique_ids[0]->get_id();

        }
    }

    pDetectionData->DecodedResult = FaceTracker; //Replace with updated one

#else

    //Without tracker we will simply have to pass all face object for inference
    NewTrackObjFound = true;
    for (HailoDetectionPtr  &tracker : pDetectionData->DecodedResult) {
        //Here we only check for face class
        if (tracker->get_class_id() != YOLO_PERSONFACE_FACE_CLASS_ID)
            continue;

        HailoUserMetaPtr usermeta = std::make_shared<HailoUserMeta>(HailoUserMeta(FACE_TRACKER_NEED_PREDICT,"Stranger",0));
        tracker->add_object(usermeta);
    }

#endif

    //Get the scale between image and object detection network input size
    float widthScale = pDetectionInfo->NetworkInputWidth * pDetectionInfo->scaledRatioWidth;
    float heightScale = pDetectionInfo->NetworkInputHeight * pDetectionInfo->scaledRatioHeight;

    //For all face class we cropp the face and infer
    bool bDataSentForInfer = false;
    for (HailoDetectionPtr  &tracker : pDetectionData->DecodedResult) {

        if (tracker->get_class_id() == YOLO_PERSONFACE_FACE_CLASS_ID) {

            //Skip for face that does not have metadata
            std::vector<HailoObjectPtr> ObjPtr = tracker->get_objects_typed(HAILO_USER_META);
            if (ObjPtr.size() == 0)
                continue;

            //Skip for face that are already identified
            HailoUserMetaPtr UserMetaPtr = std::dynamic_pointer_cast<HailoUserMeta>(ObjPtr[0]);
            if (UserMetaPtr->get_user_int() == FACE_TRACKER_IDENTIFIED)
                continue;


            //For face that are unidentified we will do a routine prediction every seconds since last prediction
            if (UserMetaPtr->get_user_int() == FACE_TRACKER_UNIDENTIFIED) {

                if (NewTrackObjFound ||
                    (!NewTrackObjFound && TimerCheck.isTimePastSec(1.0))) {
                    UserMetaPtr->set_user_int(FACE_TRACKER_NEED_PREDICT);
                }
                else {
                    continue;
                }               
            }


            bDataSentForInfer = true;

            QRect rect(tracker->get_bbox().xmin()*widthScale,
                       tracker->get_bbox().ymin()*heightScale,
                       tracker->get_bbox().width()*widthScale,
                       tracker->get_bbox().height()*heightScale);
            QImage cropped = pDetectionData->VisualizedImage.copy(rect);
            QImage scaledImage = cropped.scaled(pFaceInfo->NetworkInputWidth, pFaceInfo->NetworkInputHeight, Qt::KeepAspectRatio);
            scaledImage.convertTo(QImage::Format_RGB888);

            QImage padded = paddedImage(scaledImage, pFaceInfo->NetworkInputWidth, pFaceInfo->NetworkInputHeight, Qt::gray);
            //padded.save("SavedImage_Infer.jpg");
            const uchar* pImagedata = padded.bits();
            //const uchar* pImagedata = scaledImage.bits();

            pFaceInfo->ImageInputRaw.assign(pImagedata, pImagedata+padded.sizeInBytes());
            //pFaceInfo->ImageInputRaw.resize(pFaceInfo->NetworkInputSize, 0);

            pHailoPipeline->Infer(pFaceInfo->ModelID, pFaceInfo->ImageInputRaw, pFaceInfo->AppID);
        }
    }

    if (bDataSentForInfer)
        TimerCheck.reset();
}

void ArcFace_ReadOutputWorker(FaceRecognitionInfo* pFaceInfo, ObjectDetectionData* pData) {

    //qDebug() << "ArcFace_ReadOutputWorker";
    Timer   TimerCheck;
    MnpReturnCode ReadOutRet = MnpReturnCode::NO_DATA_AVAILABLE;
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();

    //for (int k= 0; k < pData->DecodedResult.size(); k++) {
    //    HailoDetectionPtr tracker = pData->DecodedResult[k];
    for (HailoDetectionPtr  &tracker : pData->DecodedResult) {

        if (tracker->get_class_id() == YOLO_PERSONFACE_FACE_CLASS_ID) {

            //Skip for face that does not have metadata
            std::vector<HailoObjectPtr> ObjPtr = tracker->get_objects_typed(HAILO_USER_META);
            if (!ObjPtr.size())
                continue;

            //Skip for face that are already identified
            HailoUserMetaPtr UserMetaPtr = std::dynamic_pointer_cast<HailoUserMeta>(ObjPtr[0]);
            if (UserMetaPtr->get_user_int() == FACE_TRACKER_NEED_PREDICT) {

                TimerCheck.reset();
                while(1) {

                    ReadOutRet = pHailoPipeline->ReadOutputById(pFaceInfo->ModelID, pFaceInfo->OutputBufferFloat32, pFaceInfo->AppID);

                    if (ReadOutRet == MnpReturnCode::SUCCESS) {

                        //Here we compare the face database and identify the face.
                        float32_t   best_score = 0.0f;
                        int         best_match_index = -1;
                        //qDebug() << "Face DB size: " << gFacesDB.size();
                        for (size_t k=0; k < gFacesDB.size(); k++)
                        {
                            float32_t score = cosine_similarity(pFaceInfo->OutputBufferFloat32[0], gFacesDB[k].second);

                            if (score > best_score){
                                best_score = score;
                                best_match_index = k;
                                //qDebug() << "Face Best = " << gFacesDB[k].first.c_str() << ", score = " << best_score;
                            }
                        }

                        if (best_match_index >= 0 && best_score > 0.4)
                        {
                            //qDebug() << "Face Found = " << gFacesDB[best_match_index].first.c_str() << ", score = " << best_score;
                            UserMetaPtr->set_user_string(gFacesDB[best_match_index].first);
                            UserMetaPtr->set_user_int(FACE_TRACKER_IDENTIFIED);
                        }
                        else
                        {
                            //Warning: Do not use best_match_index here as its -1
                            UserMetaPtr->set_user_int(FACE_TRACKER_UNIDENTIFIED);
                        }


                        break;
                    }
                    else {
                        if (TimerCheck.isTimePastSec(2.0)) {
                            break;
                        }

                        QThread::currentThread()->usleep(100);
                    }
                }
            }
        }
    }
}

void ArcFace_VisualizeWorker(ObjectDetectionInfo* pInfo, ObjectDetectionData* pData) {

    //qDebug() << "Yolov5mVisualize";

    int totalDetections = pData->DecodedResult.size();
    QPainter qPainter(&pData->VisualizedImage);
    qPainter.setPen(QPen(Qt::red, 2));

    QFont font;
    font.setPixelSize(24);
    qPainter.setFont(font);

    float widthScale = pInfo->NetworkInputWidth * pInfo->scaledRatioWidth;
    float heightScale = pInfo->NetworkInputHeight * pInfo->scaledRatioHeight;

    for (int k = 0; k < totalDetections; k++){
        //We ignore all prediction is provability smaller than 50%
        if (pData->DecodedResult[k]->get_confidence() < 0.4)
            continue;

        if (pData->DecodedResult[k]->get_class_id() != YOLO_PERSONFACE_FACE_CLASS_ID)
            continue;

        qPainter.drawRect(  pData->DecodedResult[k]->get_bbox().xmin()*widthScale,
                            pData->DecodedResult[k]->get_bbox().ymin()*heightScale,
                            pData->DecodedResult[k]->get_bbox().width()*widthScale,
                            pData->DecodedResult[k]->get_bbox().height()*heightScale);

        std::vector<HailoObjectPtr> ObjPtr = pData->DecodedResult[k]->get_objects_typed(HAILO_USER_META);
        if (ObjPtr.size()) {
            HailoUserMetaPtr UserMetaPtr = std::dynamic_pointer_cast<HailoUserMeta>(ObjPtr[0]);

            qPainter.drawText(pData->DecodedResult[k]->get_bbox().xmin()*widthScale,
                              pData->DecodedResult[k]->get_bbox().ymin()*heightScale,
                              QString::fromStdString(UserMetaPtr->get_user_string()));
        }

        qPainter.drawText(5,25, QString("FPS: ") + QString::number(pInfo->PerformaceFPS, 'g', 4));
    }

    qPainter.end();

}
