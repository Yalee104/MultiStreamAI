/* This file implements the lprnet pre and postprocess in cpp to improve the runtime
 * This is an intial implemntation further optimization are required.
*/

#include "lprnet_process.h"

// WARNING: This MUST match exactly from the trainned model
char DUMMY = '-';
char CLASS[] = {
         '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
         DUMMY  // This is the dummy padding character at the end of license plate for max character alignment
         };


// implementation of tf.reduce_mean(x,axis=2)
void reduce_mean(float *in, float *out, int feature_map_row, int feature_map_col, int channel_map_size) {
    // we assume H=W so only a single feature_map is used
    float mean;
    for (int col = 0; col < feature_map_col; ++col) {
        for (int channel = 0; channel < channel_map_size; ++channel) {
            mean = 0;
            for (int row = 0; row < feature_map_row; ++row) {
                mean += in[feature_map_col * channel_map_size * row + channel_map_size * col + channel];
            }

            out[channel_map_size * col + channel] = mean / feature_map_row;
        }
    }
}



size_t LprNet_PostProcessing(std::vector<std::vector<float32_t>>& inferOutResult, std::vector<float32_t>& detectionsResult)
{
    detectionsResult.resize(19 * sizeof(CLASS));

    reduce_mean(inferOutResult[0].data(), detectionsResult.data(), 5, 19, sizeof(CLASS));

    return 1;
}


stLprResult LprNet_Decode(std::vector<float32_t> &detectionsResult, bool bprintoutput)
{
    // Prepare Inputs
    const int max_time      =  19;
    const int batch_size    =  1;
    const int num_classes   =  sizeof(CLASS);
    const int top_paths     = 1;    //WARNING: Should fix to 1 as we only output 1
    stLprResult prediction_result;

    //Simply set 10 times of paths or half of num_class
    //Note that high beam width will increase search time substantially 
    //(https://towardsdatascience.com/beam-search-decoding-in-ctc-trained-neural-networks-5a889a3d85a7)
    const int beam_width    = 10;   

    float32_t *pinputs = detectionsResult.data();

    int sequence_length[batch_size] = {max_time};

    // Prepare Outputs
    int decoded[top_paths][batch_size][max_time];
    float log_probabilities[batch_size][top_paths] = {{0.0f}};

    int status = -1;
    status = ctc_beam_search_decoder(   pinputs, 
                                        sequence_length,
                                        beam_width,
                                        top_paths,
                                        max_time,
                                        batch_size,
                                        num_classes,
                                        &decoded[0][0][0],
                                        &log_probabilities[0][0],
                                        false);

    if(status != 0)
        return prediction_result;

    for (int path = 0; path < top_paths; ++path) {
        for (int b = 0; b < batch_size; ++b) {
            prediction_result.confidence = std::exp(log_probabilities[b][path]);              

            if (bprintoutput)
                printf("Path_%d (prob = %.7f) = ",path,prediction_result.confidence);  

            for (int i = 0; i < max_time; ++i) {
                int decoded_index = decoded[path][b][i];
                if (decoded_index >= 0)
                {
                    if (bprintoutput)
                        printf("%2d,",decoded_index);
                    if (DUMMY != CLASS[decoded_index])
                        prediction_result.licensePlate.push_back(CLASS[decoded_index]);
                }
            }
        }
        if (bprintoutput)
            printf("\n");
    }

    if (bprintoutput)
    {
        printf("\n");
        std::cout << "Prediction Result of License Plate Number is: " << prediction_result.licensePlate << std::endl;
    }

    return prediction_result;
}


int LprNet_Initialize(NetworkInferenceBasedObjInfo* pLicensePlateRecognitionInfo, std::string AppID)
{
    //qDebug() << "Yolov4TinyLicensePlate_Initialize";

    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    stNetworkModelInfo Network;
    Network.id_name = std::string("LprNet").append(AppID);
    Network.hef_path = "lprnet_hailo.hef";

    Network.batch_size = 1;
    Network.in_quantized = false;
    Network.in_format = HAILO_FORMAT_TYPE_UINT8;
    Network.out_quantized = false;
    Network.out_format = HAILO_FORMAT_TYPE_FLOAT32;
    pHailoPipeline->AddNetwork(0, Network, AppID);

    pLicensePlateRecognitionInfo->ModelID = Network.id_name;
    pLicensePlateRecognitionInfo->AppID = AppID;
    pLicensePlateRecognitionInfo->OutputFormat = Network.out_format;
    pLicensePlateRecognitionInfo->NetworkInputHeight = LPRNET_IMAGE_HEIGHT;
    pLicensePlateRecognitionInfo->NetworkInputWidth = LPRNET_IMAGE_WIDTH;
    pLicensePlateRecognitionInfo->NetworkInputSize = pLicensePlateRecognitionInfo->NetworkInputHeight*pLicensePlateRecognitionInfo->NetworkInputWidth*3; //RGB channel

    pHailoPipeline->GetNetworkQuantizationInfo(pLicensePlateRecognitionInfo->ModelID, pLicensePlateRecognitionInfo->QuantizationInfo);

    pHailoPipeline->InitializeOutputBuffer<float32_t>(pLicensePlateRecognitionInfo->ModelID, pLicensePlateRecognitionInfo->OutputBufferFloat32, AppID);

    return 0;
}

void LprNet_InferWorker(NetworkInferenceBasedObjInfo* pLicensePlateRecognitionInfo, 
                        AppImageData* pImageData)
{

    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();

    for (QImage &image : pImageData->ImageWorkerList) {

        const uchar* pImageVehicle = image.bits();

        pLicensePlateRecognitionInfo->ImageInputRaw.assign(pImageVehicle, pImageVehicle+image.sizeInBytes());
        //pLicensePlateRecognitionInfo->ImageInputRaw.resize(pLicensePlateRecognitionInfo->NetworkInputSize, 0);
        //qDebug() << "pLicensePlateRecognitionInfo->ImageInputRaw size: " << pLicensePlateRecognitionInfo->ImageInputRaw.size();
        pHailoPipeline->Infer(pLicensePlateRecognitionInfo->ModelID, pLicensePlateRecognitionInfo->ImageInputRaw, pLicensePlateRecognitionInfo->AppID); 

    }  
}


stLprResult LprNet_ReadOutputWorker(NetworkInferenceBasedObjInfo* pLicensePlateRecognitionInfo)
{

    MnpReturnCode ReadOutRet = MnpReturnCode::NO_DATA_AVAILABLE;
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();

    //Read the inference result of license plate recognition
    ReadOutRet = pHailoPipeline->ReadOutputById(pLicensePlateRecognitionInfo->ModelID, 
                                                pLicensePlateRecognitionInfo->OutputBufferFloat32, 
                                                pLicensePlateRecognitionInfo->AppID);

    if (ReadOutRet == MnpReturnCode::SUCCESS) {
        
        std::vector<float32_t> detectionsResult(0);
        LprNet_PostProcessing(pLicensePlateRecognitionInfo->OutputBufferFloat32, detectionsResult);

        return LprNet_Decode(detectionsResult, false);
    }

    return stLprResult();
}

