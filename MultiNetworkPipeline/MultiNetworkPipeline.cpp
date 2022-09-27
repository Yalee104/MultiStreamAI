/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   MultiNetworkPipeline.cpp
 * Author: Aaron
 * 
 * Created on July 5, 2021, 11:32 AM
 */

/* Hailo API
 * 1. hailo_create_pcie_device()
 * 2. hailo_create_hef_file()
 * 3. hailo_configure_device()
 * 4. hailo_hef_get_all_stream_infos()
 * 5. hailo_make_input_stream_params()
 * 6. hailo_make_output_stream_params()
 * 7. hailo_activate_network_group()
 * 8. hailo_get_input_stream_by_name()
 * 9. hailo_get_output_stream_by_name()
 * 10. hailo_release_hef() 
 * 11. hailo_deactivate_network_group()
 * 12. hailo_release_device()
 */
 
 /* Network Switch Workflow
 * 1. hailo_create_pcie_device()
 * 2. hailo_create_hef(hef1)
 *      a. hailo_configure_device(hef1, network1_out)
 *      b. make_streams_params, get streams_infos from network1
 * 3. hailo_create_hef(hef2)
 *      a. hailo_configure_device(hef2, network2_out)
 *      b. make_streams_params, get streams_infos from network2
 * 4. hailo_create_hef(hef3)
 *      a. hailo_configure_device(hef2, network3_out)
 *      b. make_streams_params, get streams_infos from network3
 * 5. hailo_activate_network(network1)
 *      a. get streams, infer
 * 6. hailo_deactivate_network(network1)
 * 7. hailo_activate_network(network2)
 *      a. get streams, infer
 * 8. hailo_deactivate_network(network2)
 * 9. hailo_activate_network(network3)
 *      a. get streams, infer
 * 10.hailo_deactivate_network(network3)
 * 11.hailo_release_hef() for all hef
 * 12.hailo_release_device()
 * 
 * NOTE1: steps 2-4 can be performed at any order
 * NOTE2: hailo_configure_device() does not loads the network to the device, but prepared it 
 *        (and some configurations resources) for fast loading in the activation time
 * NOTE3: step k can be performed after you create the network_groups instance (after hailo_configure_device())
 * NOTE4: notice that at each time, maximum of 1 network_group can be activated 
 *        (a proper error will raise if you'll try to activate more than 1 in a given point)
 * NOTE4: The new API will get the stream_params in the hailo_configure_device() 
 *        step [instead of in the activation time], and will allow access to streans 
 *        objects with the configured_network scope [before the network activation]
 */

/*  Feature
 *  1. Singleton (Thread safe) 
 *  2. Allow layer output ordering by name
 *  3. Automatic merge layer output by given order
 *  4. Automatic network switch on inference while allowing batch size > 1
 *  5. Adding or removing network on the fly 
 */

#include "MultiNetworkPipeline.hpp"

#define THREAD_EXIT_MAGIC_WORD   "MNP_TERMINATE"


/**
 * Static methods should be defined outside the class.
 */

MultiNetworkPipeline* MultiNetworkPipeline::pinstance_{nullptr};
std::mutex MultiNetworkPipeline::mutex_class_protection;


/**
 * Internal Helper Functions
 */

bool NetworkIsStillDoingInference(stNetworkObjInfo  &NetworkObj)
{
    bool InferenceInProgress = false;
    for (size_t i = 0; i < NUMBER_OF_INPUT_STREAMS; i++)
    {
        if (NetworkObj.pdata_waiting_for_inference_count[i]->isZeroValue() == false)
        {
            InferenceInProgress = true;
            break;
        }
    }

    return InferenceInProgress;
}

void SplitStringBy(std::string str, std::string splitBy, std::vector<std::string>& tokens)
{
    /* Store the original string in the array, so we can loop the rest
     * of the algorithm. */
    tokens.push_back(str);

    // Store the split index in a 'size_t' (unsigned integer) type.
    size_t splitAt;
    // Store the size of what we're splicing out.
    size_t splitLen = splitBy.size();
    // Create a string for temporarily storing the fragment we're processing.
    std::string frag;
    // Loop infinitely - break is internal.
    while(true)
    {
        /* Store the last string in the vector, which is the only logical
         * candidate for processing. */
        frag = tokens.back();
        /* The index where the split is. */
        splitAt = frag.find(splitBy);
        // If we didn't find a new split point...
        if(splitAt == std::string::npos)
        {
            // Break the loop and (implicitly) return.
            break;
        }
        /* Put everything from the left side of the split where the string
         * being processed used to be. */
        tokens.back() = frag.substr(0, splitAt);
        /* Push everything from the right side of the split to the next empty
         * index in the vector. */
        tokens.push_back(frag.substr(splitAt+splitLen, frag.size()-(splitAt+splitLen)));
    }
}


size_t FindInputStreamIndexFromOutputStreams(stNetworkObjInfo &NetworkObject, size_t outputStreamIndex)
{        
    size_t inputStreamGroupIndex = 0;
    hailo_vstream_info_t out_vstream_info;
    hailo_get_output_vstream_info(NetworkObject.output_streams[outputStreamIndex], &out_vstream_info);
    for (size_t i = 0; i < NetworkObject.input_stream_group_names.size(); i++)
    {
        std::string output_stream_name = out_vstream_info.name;
        if (output_stream_name.find(NetworkObject.input_stream_group_names[i]) != std::string::npos)
        {
            inputStreamGroupIndex = i;
            break;
        }
    }

    return inputStreamGroupIndex;
}


template<typename T> void SendToCollectedOutputStream(  stOutputstreamInfo<T>       &OutputStreamInfo, 
                                                        stNetworkObjInfo            &NetworkObjInfo,
                                                        TypeOutStreamCollection<T>  &collected_output_stream)
{

    OutputStreamInfo.data_id = NetworkObjInfo.data_id_afterInfer_queue.front();
    auto iter = collected_output_stream.find(NetworkObjInfo.id_name);
    if(iter != collected_output_stream.end())
    {
        iter->second.push(std::move(OutputStreamInfo));
    }
    else
    {                    
        std::pair<std::string, TypeOutStreamDataQueue<T>> OutStreamPair;
        OutStreamPair.first = NetworkObjInfo.id_name;
        OutStreamPair.second.push(std::move(OutputStreamInfo));
        collected_output_stream.insert(std::move(OutStreamPair));                    
        
        /*
        collected_output_stream.insert(std::pair<std::string, TypeOutStreamDataQueue>(NetworkObjInfo.id_name,
                                                                                      TypeOutStreamDataQueue({OutputStreamInfo})));
        */
    }
}

template<typename T> bool GetOutputDataAndSendToStreamArray(SharedQueue<stStreamDataInfo<T>>  *OutputDataQueue,
                                                            std::vector<T>                    &StreamArray)
{        
    bool bDataSent = false;

    if (!(OutputDataQueue->empty())) 
    {
        stStreamDataInfo<T> OutputData = std::move(OutputDataQueue->front());                        
        StreamArray = std::move(OutputData.data);

        OutputDataQueue->pop_front();

        bDataSent = true;
    }

    return bDataSent;
}

/*
 * THREAD METHOD
 */


void MultiNetworkPipeline::ProcessInferDataInQueue()
{
    Timer   TimerCheck;
    TimerMs FlowControlTimer;

    DBG_INFO("Started ProcessInferDataInQueue thread");
    
    while (1)
    {
        bool        data_process_done = false;  
        bool        terminate_process = false;
        size_t      earliest_requested_time = std::numeric_limits<size_t>::max();  
        std::string queue_networkname_to_infer;  

        ProtectMutex_MapQueueInferData.lock();
        for (auto &inferData : MapQueueInferData)
        {
            DBG_DEBUG("ProcessInferDataInQueue received from : " << inferData.first);
            if (inferData.first.compare(THREAD_EXIT_MAGIC_WORD) == 0)
            {
                terminate_process = true;
                break;
            }
            else if (!inferData.second.empty())
            {
                stInfererenceDataInfo<uint8_t> &InferDataInfo = inferData.second.front();
                if (InferDataInfo.requested_time.getStartTime() < earliest_requested_time)
                {
                    earliest_requested_time = InferDataInfo.requested_time.getStartTime();
                    queue_networkname_to_infer = inferData.first;
                    //std::cout << "Aaron Name: " << queue_networkname_to_infer << "time elapse: " << earliest_requested_time << std::endl;
                }
            }
        }   
        ProtectMutex_MapQueueInferData.unlock();

        if (terminate_process)
            break;

        bool                            waitandcontinue = true;
        std::map<std::string, std::queue<stInfererenceDataInfo<uint8_t>>>::iterator iter;
        if (!queue_networkname_to_infer.empty())
        {
            ProtectMutex_MapQueueInferData.lock();
            iter = MapQueueInferData.find(queue_networkname_to_infer);
            
            stInfererenceDataInfo<uint8_t> &InferDataInfo = iter->second.front();
            if (InferDataInfo.requested_time.isTimePastMs(StreamingFlowControlConfig.MinInferWaitMs) ||
                (iter->second.size() >= StreamingFlowControlConfig.MinInferFrame)) {
                //std::cout << "Flow ctrl total frame in queue: " << iter->second.size() << " from network: " <<  iter->first << std::endl;
                waitandcontinue = false;
            }            
            
            ProtectMutex_MapQueueInferData.unlock();
        }
        
        if (waitandcontinue)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(500));
            continue;
        }

        std::vector<stInfererenceDataInfo<uint8_t>> InferDataInfoList;
        std::string                                 InferDataInfo_NetworkIDName;
        ProtectMutex_MapQueueInferData.lock();
        size_t totalinfer = (StreamingFlowControlConfig.MinInferFrame == 1) ? 1 : iter->second.size();                    
        for(size_t k = 0; k < totalinfer; k++) {
            stInfererenceDataInfo<uint8_t> &InferDataInfo = iter->second.front();
            if(k==0)
                InferDataInfo_NetworkIDName = InferDataInfo.network_id_name;
            InferDataInfoList.push_back(std::move(InferDataInfo));
            iter->second.pop(); 
        }
        
        //std::cout << "Aaron Network is: " << InferDataInfo_NetworkIDName << std::endl;


        ProtectMutex_MapQueueInferData.unlock();

        TimerCheck.reset();
        
        /*
         * 1. Get current active network
         * 2. Compare if same as network_id_name
         * 3. if same simply call infer
         * 4. if not the same check if pdata_waiting_for_inference_count is zero
         * 5.   if so, we switch the network
         * 6.   if not, we will have to wait until inference is finished before network switch + infer      
         */        
        while (!data_process_done)
        {
            //Protect resource
            mutex_class_protection.lock();                       
            
            int index = GetNetworkObjIdexByName(InferDataInfo_NetworkIDName);

            if (index >= 0)
            {
                if (NetworkObjList[index].network_is_active)
                {
                    for (stInfererenceDataInfo<uint8_t> &InferDataInfo : InferDataInfoList) {
                        HailoInfer(NetworkObjList[index], InferDataInfo.data, InferDataInfo.input_stream_index, InferDataInfo.data_id);
                    }

                    data_process_done = true;
                }
                else
                {
                    bool bSwitchNetworkAndInfer = true;
                    int activeNetworkIndex = GetActiveNetworkIndex();
                    if (activeNetworkIndex >= 0)
                    {    
                        //If current active network is still busy running inference then we skip
                        //network switch for now
                        if (NetworkIsStillDoingInference(NetworkObjList[activeNetworkIndex]) == true)                        
                        {
                            bSwitchNetworkAndInfer = false;
                        }   
                    }
                    
                    if (bSwitchNetworkAndInfer)
                    {                        
                        
                        MnpReturnCode RetCode = SwitchNetwork(InferDataInfo_NetworkIDName);                        

                        if (RetCode == MnpReturnCode::SUCCESS)
                        {
                            for (stInfererenceDataInfo<uint8_t> &InferDataInfo : InferDataInfoList) {
                                HailoInfer(NetworkObjList[index], InferDataInfo.data, InferDataInfo.input_stream_index, InferDataInfo.data_id);
                            }                            
                        }
                        else
                        {
                            DBG_WARN("Unable to switch network for " << InferDataInfo_NetworkIDName << " with return code = " << (int)RetCode);
                        }
                        data_process_done = true;                        
                    }
                }
            }
            else
            {
                DBG_WARN("ProcessInferDataInQueue for network " << InferDataInfo_NetworkIDName << " not found");
                data_process_done = true; 
            }
            
            //Release resource before sleep
            mutex_class_protection.unlock();
            
            if (data_process_done == false)
            {
                if (TimerCheck.isTimePastSec(5.0))
                {
                    DBG_WARN("Current Inference taking too long (5s), data will be dropped for " << InferDataInfo_NetworkIDName)
                    data_process_done = true;
                    break;
                }
                
                //rest a bit before continue while all running inference is done
                //for current network
                std::this_thread::sleep_for(std::chrono::microseconds(500));
            }
        }
                                    
    }
    
    DBG_INFO("Exiting ProcessInferDataInQueue thread");
}


void MultiNetworkPipeline::ProcessOutputDataFromQueue()
{    
    stOutputstreamInfo<float32_t> OutputStreamInfo32f;
    stOutputstreamInfo<uint8_t> OutputStreamInfoUint8;

    std::string last_active_network_id("N/A");
    
    DBG_INFO("Started ProcessOutputDataFromQueue thread");
    

    while (1)
    {        
        if (!msg_to_out_data_process_thread.empty())
        {
            if (msg_to_out_data_process_thread.front().compare(THREAD_EXIT_MAGIC_WORD) == 0)
                break;
        }
        
        /*
         * 1. Get current active network, if no current active then sleep 1ms
         * 2. if not running inference in process then sleep for 0.5ms
         * 3. get output stream result from hailo in sequence
         * 4. save to collected output list for later retrieval by application.
         */
        mutex_class_protection.lock();
        
        int     activeNetworkIndex = GetActiveNetworkIndex();
        int     time_to_sleep_micro_s = 0;
        bool    get_output_data = false;
        if (activeNetworkIndex >= 0)
        {
            if (NetworkIsStillDoingInference(NetworkObjList[activeNetworkIndex]) == true)
            {
                get_output_data = true;
            }
            else
            {
                time_to_sleep_micro_s = 500;
            }
        }
        else
        {
            time_to_sleep_micro_s = 1000;
        }
        
        mutex_class_protection.unlock();
        
        if (get_output_data)
        {
            
            size_t OutputStreamCnt = NetworkObjList[activeNetworkIndex].output_stream_cnt;
            
            if (NetworkModelInfoList[activeNetworkIndex].out_format == HAILO_FORMAT_TYPE_UINT8) {
                OutputStreamInfoUint8.recv_array.clear();
                OutputStreamInfoUint8.recv_array.resize(OutputStreamCnt);
            }
            else //Must be Float 32
            {
                OutputStreamInfo32f.recv_array.clear();
                OutputStreamInfo32f.recv_array.resize(OutputStreamCnt);
            }
            
            int     group_index = -1;
            bool    CollectionReady = false;
            bool    bTimeout = false;
            Timer   TimerCheck;
            while (!CollectionReady && !bTimeout)
            {
                std::vector<std::vector<size_t>> workgroup_list;

                for (size_t k = 0; k < NetworkObjList[activeNetworkIndex].output_stream_group.size(); k++)
                {
#ifdef JOIN_NETWORK_SINGLE_CONTEXT_INDEPENDENT_INFER 
                    workgroup_list.push_back(NetworkObjList[activeNetworkIndex].output_stream_group[k]);
#else
                    if (k == 0) {
                        workgroup_list.push_back(NetworkObjList[activeNetworkIndex].output_stream_group[k]);
                    }
                    else {
                        workgroup_list[0].insert(   workgroup_list[0].begin(), 
                                                    NetworkObjList[activeNetworkIndex].output_stream_group[k].begin(),
                                                    NetworkObjList[activeNetworkIndex].output_stream_group[k].end());
                    }
#endif
                }

                for (size_t k = 0; k < workgroup_list.size(); k++)
                {
                    std::vector<size_t> workgroup = workgroup_list[k];
                    bool group_element_moved = false;
                    while (workgroup.size())
                    {                    
                        for (size_t i = 0; i < workgroup.size(); i++)
                        {
                            bool bDataMoved = false;
                            size_t stream_index = workgroup[i];
                            if (NetworkModelInfoList[activeNetworkIndex].out_format == HAILO_FORMAT_TYPE_UINT8) {
                                
                                bDataMoved = GetOutputDataAndSendToStreamArray<uint8_t>(NetworkObjList[activeNetworkIndex].outputstream_queue_uint8[stream_index],
                                                                                        OutputStreamInfoUint8.recv_array[stream_index]);
                            }
                            else //Must be Float 32
                            {
                                bDataMoved = GetOutputDataAndSendToStreamArray<float32_t>(  NetworkObjList[activeNetworkIndex].outputstream_queue_32f[stream_index],
                                                                                            OutputStreamInfo32f.recv_array[stream_index]);
                            }

                            if (bDataMoved)
                            {
                                TimerCheck.reset();
                                group_element_moved = true;
                                group_index = k;
                                workgroup.erase (workgroup.begin()+i);
                                break;
                            }

                            std::this_thread::sleep_for(std::chrono::microseconds(500));

                            if (TimerCheck.isTimePastSec(3.0))
                            {
                                DBG_WARN("Waiting for output stream timeout for " << NetworkObjList[activeNetworkIndex].id_name);
                                bTimeout = true;
                                break;
                            }                            
                        }

                        if (group_element_moved == false || bTimeout)
                            break;  //no output found on any stream of this group, let's give turn for next group
                    }

                    if (group_element_moved)
                    {
                        CollectionReady = true;
                        break;
                    }

                    if (bTimeout)
                        break;
                }
            }
            
            mutex_class_protection.lock();
                            
            if (CollectionReady)
            {                
                collected_output_stream_mutex.lock();

                if (NetworkModelInfoList[activeNetworkIndex].out_format == HAILO_FORMAT_TYPE_UINT8) {

                    SendToCollectedOutputStream<uint8_t>(   OutputStreamInfoUint8, 
                                                            NetworkObjList[activeNetworkIndex], 
                                                            collected_output_stream_uint8);
                }
                else //Must be Float 32
                {
                    SendToCollectedOutputStream<float32_t>( OutputStreamInfo32f, 
                                                            NetworkObjList[activeNetworkIndex], 
                                                            collected_output_stream_32f);
                }
                
                collected_output_stream_mutex.unlock();
            } 

#ifdef JOIN_NETWORK_SINGLE_CONTEXT_INDEPENDENT_INFER 
            (*NetworkObjList[activeNetworkIndex].pdata_waiting_for_inference_count[group_index])--;    
            NetworkObjList[activeNetworkIndex].data_id_afterInfer_queue.erase(NetworkObjList[activeNetworkIndex].data_id_afterInfer_queue.begin());
#else
            for (size_t kk = 0; kk < NUMBER_OF_INPUT_STREAMS; kk++) {
                if ((*NetworkObjList[activeNetworkIndex].pdata_waiting_for_inference_count[kk]).isZeroValue() == false)
                {
                    (*NetworkObjList[activeNetworkIndex].pdata_waiting_for_inference_count[kk])--; 
                    NetworkObjList[activeNetworkIndex].data_id_afterInfer_queue.erase(NetworkObjList[activeNetworkIndex].data_id_afterInfer_queue.begin());
                }
            }
#endif
            mutex_class_protection.unlock();
            
        }
                
        if (time_to_sleep_micro_s)
            std::this_thread::sleep_for(std::chrono::microseconds(time_to_sleep_micro_s));
        
        
    }
    
    DBG_INFO("Exiting ProcessOutputDataFromQueue thread");
}

template<typename T> bool GetPredictionOutputAndSendToQueue(std::vector<T>                   &recv_array,
                                                            stRecvThreadArgs                 *recv_args,
                                                            SharedQueue<stStreamDataInfo<T>> *Queue,
                                                            const Counter32                  &CurrentTotalOutputCount)
{
    bool bPredictionReceived = false;
    hailo_status status = HAILO_SUCCESS;

    //Set the size of the data buffer
    //This will only run once until recv_array is being reset.
    if (recv_array.size() == 0)
    {

        //NOTE: actual array size will depend on the defined type of output
        //      For example, if output frame type is HAILO_FORMAT_TYPE_FLOAT32
        //      the size of host_output_frame_size would be 4 times 
        //      since hailo specifies in "byte". Therefore, we will
        //      have to divide by the type size otherwise the returned array back
        //      to application will be 4 times larger in this case.
        size_t frameSize = recv_args->host_output_frame_size/sizeof(decltype(recv_array.back()));
        recv_array.resize(frameSize);
        
        /*
        DBG_INFO("Set layer output buffer size (" <<
                recv_args->host_output_frame_size <<
                ") for network " <<
                recv_args->id_name);
        */
    }
    
    //TODO: Note that inference with batch size greater than 1 may encounter issue
    //      where hailo read raw buffer will be called and never exit if
    //      application does not infer number that is multiple of batch size.
    //      find a way to overcome this issue.
    if (recv_args->pdata_waiting_for_inference_count->accumulated > CurrentTotalOutputCount) {
        
        //Timer CheckTest;

        status = hailo_vstream_read_raw_buffer(recv_args->output_stream, recv_array.data(), recv_args->host_output_frame_size);

        //std::cout << "A1- " << CheckTest.getElapsedInSec() * 1000.0 
        //          << ", size = " << recv_args->host_output_frame_size << std::endl;

        if (status != HAILO_SUCCESS)
            DBG_WARN("Hailo Code: " << status << " Msg: Failed at hailo_stream_sync_read_all_raw_buffer");
                    
        Queue->push_back(stStreamDataInfo<T>{recv_args->id_name, recv_array});
        
        bPredictionReceived = true;  
    }
    else
    {
        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }

    return bPredictionReceived;

}

void MultiNetworkPipeline::ProcessOutputstream(void *args)
{
    stRecvThreadArgs *recv_args = (stRecvThreadArgs *)args;
    Counter32 total_output_count = 0;
    std::vector<float32_t>  recv_array_32f(0, 0);        
    std::vector<uint8_t>    recv_array_uint8(0, 0);        
    
    while (recv_args->status != STOP)
    {
        while (recv_args->status == HOLD) {
            recv_args->last_change_status = HOLD;
            std::this_thread::sleep_for(std::chrono::microseconds(1000));                
        }

        recv_args->last_change_status = RUN;

        bool bPredictionReceived = false;

        if (recv_args->output_format == HAILO_FORMAT_TYPE_UINT8)
        {    

            bPredictionReceived =  GetPredictionOutputAndSendToQueue<uint8_t>(  recv_array_uint8,
                                                                                recv_args,
                                                                                recv_args->output_stream_queue_uint8,
                                                                                total_output_count);
        }
        else // Must be Float 32
        {       
            bPredictionReceived =  GetPredictionOutputAndSendToQueue<float32_t>(    recv_array_32f,
                                                                                    recv_args,
                                                                                    recv_args->output_stream_queue_32f,
                                                                                    total_output_count);
        }

        if (bPredictionReceived)
            total_output_count++;
    }
    recv_args->last_change_status = STOP;

    DBG_INFO("Exiting ProcessOutputstream thread " << recv_args->id_name << "-" << recv_args->index);
}


/*
 * PRIVATE METHOD
 */


MnpReturnCode MultiNetworkPipeline::HailoInfer(stNetworkObjInfo &NetworkObj, const std::vector<uint8_t> &data, size_t input_stream_index /*=0*/, std::string data_id /*= "N/A"*/)
{
    MnpReturnCode RetCode = MnpReturnCode::FAILED;
    
    hailo_status status = HAILO_SUCCESS;
    status = hailo_vstream_write_raw_buffer(NetworkObj.input_stream[input_stream_index], (void*)data.data(), NetworkObj.host_input_frame_size[input_stream_index]);
    REQUIRE_SUCCESS_CHECK(status, l_exit, "hailo_stream_sync_write_all_raw_buffer failed");
        
    //Notify data is inferred and we will need to get the result before any network switch
    //can take place.
    (*NetworkObj.pdata_waiting_for_inference_count[input_stream_index])++;

    //We now move the data id to after infer queue
    NetworkObj.data_id_afterInfer_queue.push_back(data_id);

    RetCode = MnpReturnCode::SUCCESS;
    
l_exit:    
    return RetCode;
}


int MultiNetworkPipeline::GetNetworkObjIdexByName(const std::string &id_name)
{
    int index = -1;
    
    if (!NetworkObjList.empty())
    {
        for(unsigned int i=0; i < NetworkObjList.size(); i++)
        {
            if (NetworkObjList[i].id_name.compare(id_name) == 0)
            {
                index = i;
                break;
            }
        }
    }
    
    return index;
}

int MultiNetworkPipeline::GetActiveNetworkIndex()
{
    int index = -1;
    
    if (!NetworkObjList.empty())
    {
        for(unsigned int i=0; i < NetworkObjList.size(); i++)
        {
            if (NetworkObjList[i].network_is_active)
            {
                index = i;
                break;
            }
        }
    }
    
    return index;
}


int MultiNetworkPipeline::GetNetworkModelInfoIndexByName(const std::string &id_name)
{
    int index = -1;

    if (!NetworkModelInfoList.empty())
    {    
        for(unsigned int i=0; i < NetworkModelInfoList.size(); i++)
        {
            if (NetworkModelInfoList[i].id_name.compare(id_name) == 0)
            {
                index = i;
                break;
            }
        }
    }
    
    return index;    
}


MnpReturnCode MultiNetworkPipeline::DeactivateCurrentNetwork()
{
    MnpReturnCode RetCode = MnpReturnCode::FAILED;
    
    int activeNetworkIndex = GetActiveNetworkIndex();
    if (activeNetworkIndex >= 0)
    {
        /**
         * 1. Set the status of all thread to HOLD for the current active network
         *    output stream thread. (NOTE: How can we be sure it is on HOLD?)
         * 2. Deactivate current network
         * 
         */
            
        //iterate and we don't care the order.
        for (stRecvThreadArgs& threadArg : NetworkObjList[activeNetworkIndex].outputstream_recv_args)
        {
            threadArg.status = HOLD;
        }
        
        //Aaron
        //TODO: Find out why we need to add 0.5 to 1ms of delay before we deactivate the network
        //      otherwise the inference output of next network might be wrong.
        //      Note that this is not related to output stream thread HOLD status above, the delay can
        //      be move to before setting the status to HOLD and will still fix the problem. 
        std::this_thread::sleep_for(std::chrono::microseconds(1000));


        //TODO: Should we somehow find a way to make sure all thread is in HOLD state before we
        //      deactivate network??

        //deactivate network
        hailo_deactivate_network_group(NetworkObjList[activeNetworkIndex].active_net_g);        
        NetworkObjList[activeNetworkIndex].network_is_active = false;

    }
    
    return RetCode;    
}

    
MnpReturnCode MultiNetworkPipeline::ActivateNetworkById(const std::string &id_name)
{
    MnpReturnCode RetCode = MnpReturnCode::FAILED;
    hailo_status status = HAILO_SUCCESS;
    int NetIndexToAct = GetNetworkObjIdexByName(id_name);
    if (NetIndexToAct >= 0)
    {
                    
        status = hailo_activate_network_group(NetworkObjList[NetIndexToAct].network_group, 
                                              NULL, 
                                              &NetworkObjList[NetIndexToAct].active_net_g);
     
        REQUIRE_SUCCESS_CHECK(status, l_exit, "Failed activate_network_group");
        
        NetworkObjList[NetIndexToAct].network_is_active = true;     
        
    }

    RetCode = MnpReturnCode::SUCCESS;
    
l_exit:
    return RetCode;
}
  

MnpReturnCode MultiNetworkPipeline::SwitchNetwork(const std::string &id_name)
{
    MnpReturnCode RetCode = MnpReturnCode::FAILED;
    
    
    /**
     * 1. Find activated network (if any)
     * 2. Update all output stream thread and send it to suspend mode
     * 3. Deactivate the network
     * 4. Activate the desired network
     * 5. get output streams
     * 6. create thread (if not created) and update the output stream + other related info 
     *  
     */
    
    /* Deactivate Network */
    DeactivateCurrentNetwork();

    /* Activate Network */
    RetCode = ActivateNetworkById(id_name);
    
    /* Prepare output stream thread */
    if (RetCode == MnpReturnCode::SUCCESS)
        PrepareOutputStreamThreadById(id_name);
    
    return RetCode;
}


void MultiNetworkPipeline::PrepareOutputStreamThreadById(const std::string &id_name)
{
        
    const stNetworkModelInfo* pNetworkModelIndo = GetNetworkInfoByName_Unprotect(id_name);

    int NetIndexToAct = GetNetworkObjIdexByName(id_name);
    if (NetIndexToAct >= 0)
    {    
        size_t OutputStreamCount = NetworkObjList[NetIndexToAct].output_stream_cnt;
                
        if (NetworkObjList[NetIndexToAct].outputstream_recv_pthreads.empty())
        {
            DBG_INFO("Creating thread and stream for network " << id_name);
            if (pNetworkModelIndo->out_format == HAILO_FORMAT_TYPE_UINT8) {
                NetworkObjList[NetIndexToAct].outputstream_queue_uint8.resize(OutputStreamCount);
                for (size_t i = 0; i < OutputStreamCount; i++) {
                    NetworkObjList[NetIndexToAct].outputstream_queue_uint8[i] = new SharedQueue<stStreamDataInfo<uint8_t>>;                    
                }
            }
            else {//Must be Float 32
                NetworkObjList[NetIndexToAct].outputstream_queue_32f.resize(OutputStreamCount);
                for (size_t i = 0; i < OutputStreamCount; i++) {
                    NetworkObjList[NetIndexToAct].outputstream_queue_32f[i] = new SharedQueue<stStreamDataInfo<float32_t>>;                    
                }                
            }

            NetworkObjList[NetIndexToAct].outputstream_recv_args.resize(OutputStreamCount);
            NetworkObjList[NetIndexToAct].outputstream_recv_pthreads.resize(OutputStreamCount);
            
            for (size_t s=0; s < OutputStreamCount; s++) {
                
                NetworkObjList[NetIndexToAct].outputstream_recv_pthreads[s] =
                         std::thread(&MultiNetworkPipeline::ProcessOutputstream, 
                                     this, 
                                     &NetworkObjList[NetIndexToAct].outputstream_recv_args[s]);
                                 
            }
        }
                
        for (size_t s=0; s < OutputStreamCount; s++) {           

            NetworkObjList[NetIndexToAct].outputstream_recv_args[s].index = s;

            size_t inputStreamGroupIndex = FindInputStreamIndexFromOutputStreams(NetworkObjList[NetIndexToAct], s);

            NetworkObjList[NetIndexToAct].outputstream_recv_args[s].pdata_waiting_for_inference_count = NetworkObjList[NetIndexToAct].pdata_waiting_for_inference_count[inputStreamGroupIndex];
                    
            if (pNetworkModelIndo->out_format == HAILO_FORMAT_TYPE_UINT8)        
                NetworkObjList[NetIndexToAct].outputstream_recv_args[s].output_stream_queue_uint8 = NetworkObjList[NetIndexToAct].outputstream_queue_uint8[s];
            else //Must be Float 32
                NetworkObjList[NetIndexToAct].outputstream_recv_args[s].output_stream_queue_32f = NetworkObjList[NetIndexToAct].outputstream_queue_32f[s];
            
            NetworkObjList[NetIndexToAct].outputstream_recv_args[s].output_stream = NetworkObjList[NetIndexToAct].output_streams[s];

            NetworkObjList[NetIndexToAct].outputstream_recv_args[s].host_output_frame_size = NetworkObjList[NetIndexToAct].host_output_frame_size[s];            

            NetworkObjList[NetIndexToAct].outputstream_recv_args[s].id_name = NetworkObjList[NetIndexToAct].id_name;
            
            NetworkObjList[NetIndexToAct].outputstream_recv_args[s].output_format = pNetworkModelIndo->out_format;

            //This MUST be last so that thread will get all latest value update before entering run state
            NetworkObjList[NetIndexToAct].outputstream_recv_args[s].status = RUN;            
        }
        
    }
}


const stNetworkModelInfo* MultiNetworkPipeline::GetNetworkInfoByName_Unprotect(const std::string &id_name)
{
    stNetworkModelInfo* pNetwork = nullptr;
    
    if (!NetworkModelInfoList.empty())
    {
        for(unsigned int i=0; i < NetworkModelInfoList.size(); i++)
        {
            if (NetworkModelInfoList[i].id_name.compare(id_name) == 0)
            {
                pNetwork = &NetworkModelInfoList[i];
                break;
            }
        }
    }
    
    return pNetwork;
}

MultiNetworkPipeline::MultiNetworkPipeline()
{
    //TODO: Get all PCIe Hailo interfaces to support multiple hailo module on same system

    hailo_device_found = 0;
    NetworkObjList.clear();
    NetworkModelInfoList.clear();
            
    if (InitializeHailo() == 0) {
        DBG_WARN("No Hailo Device Found");
    }
    
}

MultiNetworkPipeline::~MultiNetworkPipeline()
{
    
}

/* PUBLIC METHOD */


/**
 * The first time we call GetInstance we will lock the storage location
 *      and then we make sure again that the variable is null and then we
 *      set the value. RU:
 */
MultiNetworkPipeline *MultiNetworkPipeline::GetInstance()
{
    // mutex_class_protection is automatically released when lock
    // goes out of scope
    std::lock_guard<std::mutex> lock(mutex_class_protection);
    if (pinstance_ == nullptr)
    {
        pinstance_ = new MultiNetworkPipeline();
    }
    return pinstance_;
}


MnpReturnCode MultiNetworkPipeline::ReleaseAllResource()
{

    std::queue<std::string> empty;
    MnpReturnCode RetCode = MnpReturnCode::SUCCESS;    
    TimerMs terminate_time;
    std::pair<std::string, std::queue<stInfererenceDataInfo<uint8_t>>> MapQueueData;

    stInfererenceDataInfo<uint8_t> Data = {THREAD_EXIT_MAGIC_WORD, 
                                           std::vector<uint8_t>{0}, 
                                           0,
                                           "", 
                                           terminate_time};    

    if (!hailo_device_found)
        return MnpReturnCode::HAILO_NOT_INITIALIZED;     
    
    int activeNetworkIndex = GetActiveNetworkIndex();
    
    if (activeNetworkIndex >= 0)
    {                
        if (NetworkIsStillDoingInference(NetworkObjList[activeNetworkIndex]) == true)
        {
            RetCode = MnpReturnCode::RUNNING_INFERENCE;
            goto l_exit;
        }
    }
    
    if (!NetworkModelInfoList.empty())
    {
        std::vector<std::string> list_to_remove;
        for(unsigned int i=0; i < NetworkModelInfoList.size(); i++)
            list_to_remove.push_back(NetworkModelInfoList[i].id_name);
        
        for(auto x : list_to_remove)
            RemoveNetwork(x);

    }
    
    
    /* Terminate all class generated thread */
    MapQueueData.first = THREAD_EXIT_MAGIC_WORD;
    MapQueueData.second.push(std::move(Data));
    
    ProtectMutex_MapQueueInferData.lock();
    MapQueueInferData.insert(std::move(MapQueueData));   
    ProtectMutex_MapQueueInferData.unlock();

    /*
    MapQueueInferData.insert(   std::pair<std::string, 
                                std::queue<stInfererenceDataInfo<uint8_t>>>(   THREAD_EXIT_MAGIC_WORD,
                                                                                stInfererenceDataInfo<uint8_t>{ THREAD_EXIT_MAGIC_WORD, 
                                                                                                                std::vector<uint8_t>{0}, 
                                                                                                                0,
                                                                                                                "", 
                                                                                                                terminate_time}));     
    */
    infer_data_queue_process_thread.join();

    msg_to_out_data_process_thread.push(THREAD_EXIT_MAGIC_WORD);
    output_data_queue_process_thread.join();

        
    /* Release all other queue or vector data after all thread termination 
     * if not already done so.
     */ 
    NetworkObjList.clear();
    NetworkModelInfoList.clear();    
    collected_output_stream_32f.clear();
    collected_output_stream_uint8.clear();

    /* Clear queue */
    ProtectMutex_MapQueueInferData.lock();
    MapQueueInferData.clear();    
    ProtectMutex_MapQueueInferData.unlock();

    std::swap( msg_to_out_data_process_thread, empty );

    for (size_t i = 0; i < hailo_device_found; i++){
        if (devices[i] != NULL)
            hailo_release_device(devices[i]);
    }

    hailo_device_found = 0;
    
l_exit:
    return RetCode;
}

size_t MultiNetworkPipeline::InitializeHailo()
{    
    if (hailo_device_found)
        goto init_exit;
    
    for (int i = 0; i < NUMBER_OF_DEV_SUPPORTED; i++)
    {
        devices[i] = NULL;
    }

    try {

        hailo_scan_pcie_devices(pcie_device_info, NUMBER_OF_DEV_SUPPORTED, &hailo_device_found);
        
        if (hailo_device_found > NUMBER_OF_DEV_SUPPORTED)
        {
            DBG_ERROR("-E- found " << hailo_device_found << " device(s) but exceeding max device support of " << NUMBER_OF_DEV_SUPPORTED);
            hailo_device_found = 0;
        }


        /*
        std::cout << "WARNING: Dissabling throttling for test purposes only" << std::endl;
        status = hailo_set_throttling_state(devices, false);
        if (status != HAILO_SUCCESS)
            hailo_device_found = 0;
        */
       
    } catch (std::exception const& e) {
        DBG_ERROR("-E- create device failed" << e.what());
    }

    //Start processing thread if havent started
    if (hailo_device_found)
    {
        if (!infer_data_queue_process_thread.joinable())
            infer_data_queue_process_thread = std::thread(&MultiNetworkPipeline::ProcessInferDataInQueue, this);
        if (!output_data_queue_process_thread.joinable())
            output_data_queue_process_thread = std::thread(&MultiNetworkPipeline::ProcessOutputDataFromQueue, this);        
    }
    
init_exit:
    
    return hailo_device_found;
}


MnpReturnCode MultiNetworkPipeline::AddNetwork(uint32_t device_id, const stNetworkModelInfo &NewNetworkInfo )
{
    
    size_t number_of_infos = MAX_NUM_GROUP_INFO;
    hailo_network_group_info_t group_info[MAX_NUM_GROUP_INFO];
    
    MnpReturnCode RetCode = MnpReturnCode::FAILED;
    
    if (!hailo_device_found)
        return MnpReturnCode::HAILO_NOT_INITIALIZED;

    if (device_id >= hailo_device_found)
    {
        DBG_WARN("device id exceeded current maximum hailodevice found (" << hailo_device_found << ")");
        return MnpReturnCode::INVALID_PARAMETER;        
    }

    if ((NewNetworkInfo.out_format != HAILO_FORMAT_TYPE_UINT8) && 
        (NewNetworkInfo.out_format != HAILO_FORMAT_TYPE_FLOAT32))
    {
        DBG_WARN("Configured output format not yet supported.");
        return MnpReturnCode::INVALID_PARAMETER;
    }

    //Any manipulation method we lock the resources first
    std::lock_guard<std::mutex> lock(mutex_class_protection);        

    //NOTE: device_id not used at the moment, reserve for future use    
    if (GetNetworkInfoByName_Unprotect(NewNetworkInfo.id_name))
       return MnpReturnCode::DUPLICATE;

    //Create device if not already created
    if (devices[device_id] == NULL)
    {
        if (hailo_create_pcie_device(&pcie_device_info[device_id], &devices[device_id]) != HAILO_SUCCESS)
        {
            DBG_ERROR("-E- failed to create hailo_create_pcie_device");
            return MnpReturnCode::FAILED;
        }
    }

    /* Not require at the moment, but maybe will in the future.
    if (NewNetworkInfo.id_name.find_first_not_of("abcdefghijklmnopqrstuvwxyz01234567890") != std::string::npos)
    {
        DBG_ERROR("Network ID name must be lower case and alpha numeric only");
        return MnpReturnCode::INVALID_PARAMETER;
    }
    */ 
    
    //Since we are going to manipulate class resources such as NetworkObjList
    //we will need to make sure no network is running inference to prevent
    //thread from accessing resources while we manipulate it.    
    int activeNetworkIndex = GetActiveNetworkIndex();
    if (activeNetworkIndex >= 0)
    {       
        if (NetworkIsStillDoingInference(NetworkObjList[activeNetworkIndex]) == true)         
        {
            return MnpReturnCode::RUNNING_INFERENCE;
        }
    }    
    

    stNetworkObjInfo NewNetworkObj;    
    NewNetworkObj.id_name = NewNetworkInfo.id_name;
    NewNetworkObj.network_is_active = false;
    NewNetworkObj.outputstream_recv_pthreads.clear();
    NewNetworkObj.outputstream_recv_args.clear();

    for (size_t i = 0; i < NewNetworkObj.outputstream_queue_32f.size(); i++) {
        delete NewNetworkObj.outputstream_queue_32f[i];
    }

    for (size_t i = 0; i < NewNetworkObj.outputstream_queue_uint8.size(); i++) {
        delete NewNetworkObj.outputstream_queue_uint8[i];
    }

    NewNetworkObj.outputstream_queue_32f.clear();    
    NewNetworkObj.outputstream_queue_uint8.clear();

    NewNetworkObj.data_id_afterInfer_queue.clear();

    bool bUseDefaultInputOrder = true;
    bool bUseDefaultOutputOrder = true;
    hailo_status status = HAILO_SUCCESS;
    size_t number_of_network_groups = 1;
    hailo_configure_params_t    configure_params = {0};
    size_t                      total_output_stream_in_group = 0;

    
    /* Create HEF and Configure Devices*/
    
    status = hailo_create_hef_file(&NewNetworkObj.hef, NewNetworkInfo.hef_path.c_str());
    REQUIRE_SUCCESS_CHECK(status, l_exit, "Failed to create hef file");
    
    //In order to configure the parameter properly we will need to provide the network 
    //exact name which we will have to get it from hailo_get_network_groups_infos
    //API. Note that in here we are only expecting 1 network in a HEF file.
    //Currently this module does NOT support multiple network on HEF file as yet.
    hailo_get_network_groups_infos( NewNetworkObj.hef, group_info, &number_of_infos);    

    status = hailo_init_configure_params(NewNetworkObj.hef, HAILO_STREAM_INTERFACE_PCIE, &configure_params);
    REQUIRE_SUCCESS_CHECK(status, l_exit, "Failed init configure params");

    configure_params.network_group_params[0].batch_size = NewNetworkInfo.batch_size;
    
    status = hailo_configure_device(devices[device_id], NewNetworkObj.hef, &configure_params, &NewNetworkObj.network_group, &number_of_network_groups);
    REQUIRE_SUCCESS_CHECK(status, l_exit, "Failed to configure device from hef");
    
    if (number_of_network_groups > 1) {
        status = HAILO_UNINITIALIZED;
        REQUIRE_SUCCESS_CHECK(status, l_exit, "Failed to extract network group, larger than 1(" << number_of_network_groups << ")");
    }

    
    /* Make Stream Param */

    
    status = hailo_make_input_vstream_params(NewNetworkObj.network_group, 
                                             NewNetworkInfo.in_quantized, 
                                             NewNetworkInfo.in_format,
                                             NewNetworkObj.input_stream_params, 
                                             &NewNetworkObj.input_stream_cnt);
    REQUIRE_SUCCESS_CHECK(status, l_exit, "Failed making input virtual stream params");


    status = hailo_make_output_vstream_params(  NewNetworkObj.network_group, 
                                                NewNetworkInfo.out_quantized, 
                                                NewNetworkInfo.out_format,
                                                NewNetworkObj.output_stream_params, 
                                                &NewNetworkObj.output_stream_cnt);
    
    REQUIRE_SUCCESS_CHECK(status, l_exit, "Failed making output virtual stream params");
    
    /* Create Virtual Input Stream */

    if (!NewNetworkInfo.input_order_by_name.empty())
        bUseDefaultInputOrder = false;

    NewNetworkObj.input_quantization_info.clear();
    NewNetworkObj.input_quantization_info.resize(NewNetworkObj.input_stream_cnt);

    for (size_t i = 0; i < NewNetworkObj.input_stream_cnt; i++)
    {

        if (bUseDefaultInputOrder)
        {
            status = hailo_create_input_vstreams(   NewNetworkObj.network_group, 
                                                    &NewNetworkObj.input_stream_params[i], 
                                                    1, 
                                                    &NewNetworkObj.input_stream[i]);
            REQUIRE_SUCCESS_CHECK(status, l_exit, "Failed creating virtual input stream");

            DBG_INFO( "inut_stream_by_name " << NewNetworkObj.input_stream_params[i].name);
        }
        else 
        {
            bool bFound = false;
            for (size_t j = 0; j < NewNetworkObj.input_stream_cnt; j++)
            {
                if (strcmp(NewNetworkObj.input_stream_params[j].name, NewNetworkInfo.input_order_by_name[i].c_str()) == 0)
                {
                    status = hailo_create_input_vstreams(   NewNetworkObj.network_group, 
                                                            &NewNetworkObj.input_stream_params[j], 
                                                            1, 
                                                            &NewNetworkObj.input_stream[i]);

                    REQUIRE_SUCCESS_CHECK(status, l_exit, "Failed creating virtual input stream");

                    DBG_INFO( "inut_stream_by_name " << NewNetworkObj.input_stream_params[i].name);                    

                    bFound = true;
                    break;       
                }
            }

            if (bFound == false)
            {
                DBG_WARN("Unable to find given input layer name " << NewNetworkInfo.input_order_by_name[i]);
                goto l_exit;
            }             
        }

        status = hailo_get_input_vstream_frame_size(NewNetworkObj.input_stream[i], &NewNetworkObj.host_input_frame_size[i]);
        REQUIRE_SUCCESS_CHECK(status, l_exit, "Failed getting input virtual stream frame size");

        //Get input stream info
        hailo_vstream_info_t in_vstream_info;
        status = hailo_get_input_vstream_info(NewNetworkObj.input_stream[i], &in_vstream_info);
        REQUIRE_SUCCESS_CHECK(status, l_exit, "Failed hailo_get_input_vstream_info");
        
        //If this network is a join network, get all its network names.
        std::string input_stream_name = in_vstream_info.name;
        std::vector<std::string> inputNetowrkName;
        SplitStringBy(input_stream_name, "/", inputNetowrkName);
        if (inputNetowrkName.size() > 1)
        {
            NewNetworkObj.input_stream_group_names.push_back(inputNetowrkName[0]);
        }

        /* Get the quantization info */
        qp_zp_scale_t transformScale = {in_vstream_info.quant_info.qp_zp,
                                        in_vstream_info.quant_info.qp_scale};
        NewNetworkObj.input_quantization_info[i] = transformScale;
        //std::cout << "input(" <<  inputNetowrkName[0] << ")stream format type: " << in_vstream_info.format.type << ", quant info scale = " << transformScale.qp_scale << " zero point = " << transformScale.qp_zp << std::endl;


    }

    if (NewNetworkObj.input_stream_group_names.size())
    {
        if (NewNetworkObj.input_stream_group_names.size() != NewNetworkObj.input_stream_cnt)
        {
            DBG_ERROR("Multiple input stream of same network (not a join network) is not yet supported.");
            goto l_exit;
        }

        if (NewNetworkObj.input_stream_cnt > 1) {
            DBG_WARN("- [" << NewNetworkInfo.id_name << "] This is a multiple input stream of a joined network, data_id (if given) from infer should be same for infering any input network (input_stream_index)");
        }
        NewNetworkObj.output_stream_group.resize(NewNetworkObj.input_stream_group_names.size());
    }
    else
    {
        NewNetworkObj.output_stream_group.resize(1);
    }

    /* Create Virtual Output Stream and obtain vstream info*/

    NewNetworkObj.output_quantization_info.clear();
    NewNetworkObj.output_quantization_info.resize(NewNetworkObj.output_stream_cnt);

    if (!NewNetworkInfo.output_order_by_name.empty())
        bUseDefaultOutputOrder = false;
    
    //Get all output vstream at once, this is the recommended way
    hailo_output_vstream    output_streams_temp[NUMBER_OF_OUTPUT_STREAMS];
    status = hailo_create_output_vstreams(  NewNetworkObj.network_group, 
                                            NewNetworkObj.output_stream_params,
                                            NewNetworkObj.output_stream_cnt,
                                            output_streams_temp);

    //Now we sort it (if needed) and obtain information of each stream
    for (size_t i = 0; i < NewNetworkObj.output_stream_cnt; ++i) {                

        if (!bUseDefaultOutputOrder)
        {                    
            bool bFound = false;
            for (size_t j = 0; j < NewNetworkObj.output_stream_cnt; j++)
            {
                hailo_vstream_info_t    vstream_info;
                hailo_get_output_vstream_info(output_streams_temp[j], &vstream_info);

                if (strcmp(vstream_info.name, NewNetworkInfo.output_order_by_name[i].c_str()) == 0)
                {
                    NewNetworkObj.output_streams[i] = output_streams_temp[j];
                    bFound = true;
                    break;       
                }
            }

            if (bFound == false)
            {
                DBG_WARN("Unable to find given output layer name " << NewNetworkInfo.output_order_by_name[i]);
                goto l_exit;
            }            
        }
        else
        {
            NewNetworkObj.output_streams[i] = output_streams_temp[i];
        }
        
        REQUIRE_SUCCESS_CHECK(status, l_exit, "Failed get_output_stream_by_name");
        
        /* Get the quantization info */
        hailo_vstream_info_t out_vstream_info;
        status = hailo_get_output_vstream_info(NewNetworkObj.output_streams[i], &out_vstream_info);
        qp_zp_scale_t transformScale = {out_vstream_info.quant_info.qp_zp,
                                        out_vstream_info.quant_info.qp_scale};
        NewNetworkObj.output_quantization_info[i] = transformScale;
        //std::cout << "output stream format type: " << out_vstream_info.format.type << ", quant info scale = " << transformScale.qp_scale << " zero point = " << transformScale.qp_zp << std::endl;
        DBG_INFO("output_stream_by_name: " << out_vstream_info.name);

        /* Get the output sequence group */
        
        if (NewNetworkObj.input_stream_group_names.size())
        {
            for (size_t k=0; k < NewNetworkObj.input_stream_group_names.size(); k++)
            {
                std::string output_stream_name = out_vstream_info.name;
                if (output_stream_name.find(NewNetworkObj.input_stream_group_names[k]) != std::string::npos)
                {
                    total_output_stream_in_group++;
                    NewNetworkObj.output_stream_group[k].push_back(i);                    
                    break;
                }
            }
        }
        else
        {
            total_output_stream_in_group++;
            NewNetworkObj.output_stream_group[0].push_back(i);
        }

        /* Get frame size of each stream */
        status = hailo_get_output_vstream_frame_size(NewNetworkObj.output_streams[i], &NewNetworkObj.host_output_frame_size[i]);
        REQUIRE_SUCCESS_CHECK(status, l_exit, "Failed getting output virtual stream frame size");
        
        DBG_INFO( "output_stream Size = " << NewNetworkObj.host_output_frame_size);
    }
    
    if (total_output_stream_in_group != NewNetworkObj.output_stream_cnt)
    {
        DBG_ERROR( "total output stream did not match from group" );
        goto l_exit;
    }    

    for (size_t i = 0; i < NUMBER_OF_INPUT_STREAMS; i++)
    {
        NewNetworkObj.pdata_waiting_for_inference_count[i] = new AtomicIntCount();
    }

    /* Save all information */
    
    //Move as NewNetworkObj will not be used and is only valid in current context
    //also this structure includes array of threads that can only be moved and not copied
    NetworkObjList.push_back(std::move(NewNetworkObj));
    
    //We could move NewNetworkInfo as well but we cannot be sure if the application
    //will use it after this method scope. Therefore we will have to copy instead of move.
    NetworkModelInfoList.push_back(NewNetworkInfo);        
    
    RetCode = MnpReturnCode::SUCCESS;
    
l_exit:
    
    return RetCode;
}
    

MnpReturnCode MultiNetworkPipeline::RemoveNetwork(const std::string &id_name )
{
    
    MnpReturnCode RetCode = MnpReturnCode::NOT_FOUND;

    if (!hailo_device_found)
        return MnpReturnCode::HAILO_NOT_INITIALIZED;
    
    //Any manipulation method we lock the resources first
    std::lock_guard<std::mutex> lock(mutex_class_protection);   
        
    if (NetworkModelInfoList.empty())
        return MnpReturnCode::NOT_FOUND;    

    int NetworkModelIndex = GetNetworkModelInfoIndexByName(id_name);    
    if (NetworkModelIndex < 0)
        return MnpReturnCode::NOT_FOUND;
        
    int NetworkObjectIndex = GetNetworkObjIdexByName(id_name);    
    if (NetworkObjectIndex < 0)
    {
        DBG_WARN("RemoveNetwork NetworkObj not found for id = " << id_name);
        return MnpReturnCode::NOT_FOUND;
    }
    
    //Since we are going to manipulate class resources such as NetworkObjList
    //we will need to make sure no network is running inference to prevent
    //thread from accessing resources while we manipulate it.    
    int activeNetworkIndex = GetActiveNetworkIndex();
    if (activeNetworkIndex >= 0)
    {                
        if (NetworkIsStillDoingInference(NetworkObjList[activeNetworkIndex]) == true)
        {
            return MnpReturnCode::RUNNING_INFERENCE;
        }
    }
    
    //Deactivate network if the removed network is currently active 
    if ((activeNetworkIndex >= 0) && (activeNetworkIndex == NetworkModelIndex))
    {
        DeactivateCurrentNetwork();
    }
        
    //Remove the network object            
    if (NetworkObjectIndex >= 0)
    { 
        for (stRecvThreadArgs& threadArg : NetworkObjList[NetworkObjectIndex].outputstream_recv_args)
            threadArg.status = STOP;

        for (auto& t: NetworkObjList[NetworkObjectIndex].outputstream_recv_pthreads)
        {
            if (t.joinable())
                t.join();        
        }
        
        for (size_t i = 0; i < NUMBER_OF_INPUT_STREAMS; i++)
        {
            delete NetworkObjList[NetworkObjectIndex].pdata_waiting_for_inference_count[i];
            NetworkObjList[NetworkObjectIndex].pdata_waiting_for_inference_count[i] = nullptr;
        }

        hailo_release_input_vstreams(NetworkObjList[NetworkObjectIndex].input_stream, NetworkObjList[NetworkObjectIndex].input_stream_cnt);

        hailo_release_output_vstreams(NetworkObjList[NetworkObjectIndex].output_streams, NetworkObjList[NetworkObjectIndex].output_stream_cnt);

        hailo_release_hef(NetworkObjList[NetworkObjectIndex].hef);
        NetworkObjList.erase(NetworkObjList.begin()+NetworkObjectIndex);        
    }

    //Remove the network model info
    NetworkModelInfoList.erase(NetworkModelInfoList.begin()+NetworkModelIndex);

    RetCode = MnpReturnCode::SUCCESS;
    
    return RetCode;
}


MnpReturnCode MultiNetworkPipeline::GetNetworkInputSize(const std::string &id_name, size_t &NetworkInputSize, size_t input_stream_index /*= 0*/)
{
    MnpReturnCode RetCode = MnpReturnCode::FAILED;
    NetworkInputSize = 0;
    
    if (!hailo_device_found)
        return MnpReturnCode::HAILO_NOT_INITIALIZED;    
    
    //Protect resource
    std::lock_guard<std::mutex> lock(mutex_class_protection);
    
    int index = GetNetworkObjIdexByName(id_name);
    if (index >= 0)
    {
        if (NetworkObjList[index].network_is_active)
        {
            NetworkInputSize = NetworkObjList[index].host_input_frame_size[input_stream_index];
            RetCode = MnpReturnCode::SUCCESS;
        }
        else
        {
            bool bSwitchNetwork = true;
            int activeNetworkIndex = GetActiveNetworkIndex();
            if (activeNetworkIndex >= 0)
            {    
                //If current active network is still busy running inference then we skip
                //network switch for now
                if (NetworkIsStillDoingInference(NetworkObjList[activeNetworkIndex]) == true)
                {
                    bSwitchNetwork = false;
                }   
            }

            if (bSwitchNetwork)
            {
                RetCode = SwitchNetwork(id_name);
                if (RetCode == MnpReturnCode::SUCCESS)
                {
                    NetworkInputSize = NetworkObjList[index].host_input_frame_size[input_stream_index];
                }
                else
                {
                    DBG_WARN("Unable to switch network for " << id_name << " with return code = " << (int)RetCode);
                }                         
            }
        }
    }
    else
    {
        RetCode = MnpReturnCode::NOT_FOUND;
    }

    return RetCode;
}         


MnpReturnCode MultiNetworkPipeline::GetNetworkQuantizationInfo(const std::string &id_name, std::vector<qp_zp_scale_t> &NetworkQuantInfo, bool get_from_output_stream /* = true */)
{
    MnpReturnCode RetCode = MnpReturnCode::FAILED;
    
    if (!hailo_device_found)
        return MnpReturnCode::HAILO_NOT_INITIALIZED;    
    
    //Protect resource
    std::lock_guard<std::mutex> lock(mutex_class_protection);
    
    int     index = GetNetworkObjIdexByName(id_name);
    bool    bquant_info_ready = false;
    if (index >= 0)
    {
        if (NetworkObjList[index].network_is_active)
        {
            bquant_info_ready = true;
            RetCode = MnpReturnCode::SUCCESS;
        }
        else
        {
            bool bSwitchNetwork = true;
            int activeNetworkIndex = GetActiveNetworkIndex();
            if (activeNetworkIndex >= 0)
            {    
                //If current active network is still busy running inference then we skip
                //network switch for now
                if (NetworkIsStillDoingInference(NetworkObjList[activeNetworkIndex]) == true)
                {
                    bSwitchNetwork = false;
                }   
            }

            if (bSwitchNetwork)
            {
                RetCode = SwitchNetwork(id_name);
                if (RetCode == MnpReturnCode::SUCCESS)
                {
                    bquant_info_ready = true;
                }
                else
                {
                    DBG_WARN("Unable to switch network for " << id_name << " with return code = " << (int)RetCode);
                }                         
            }
        }

        if (bquant_info_ready)
        {
            if (get_from_output_stream)
            {
                NetworkQuantInfo = NetworkObjList[index].output_quantization_info;
            }
            else
            {
                NetworkQuantInfo = NetworkObjList[index].input_quantization_info;
            }
        }
    }
    else
    {
        RetCode = MnpReturnCode::NOT_FOUND;
    }

    return RetCode;
}     


MnpReturnCode MultiNetworkPipeline::Infer(const std::string &id_name, const std::vector<uint8_t> &data, const std::string &data_id /*="N/A"*/, size_t input_stream_index /*=0*/)
{
    MnpReturnCode RetCode = MnpReturnCode::FAILED;       

    if (!hailo_device_found)
        return MnpReturnCode::HAILO_NOT_INITIALIZED;
    
    //Any manipulation method we lock the resources first
    std::lock_guard<std::mutex> lock(mutex_class_protection);   

    int index = GetNetworkObjIdexByName(id_name);

    if (index < 0)
    {
        RetCode = MnpReturnCode::NOT_FOUND;
    }
    else
    {
              
        ProtectMutex_MapQueueInferData.lock();

        //Send to Queue
        auto iter = MapQueueInferData.find(id_name);
        TimerMs ReqStartTimer;
        if(iter != MapQueueInferData.end())
        {                
            //std::cout << "Aaron Req time: " << ReqStartTimer.getStartTime() << std::endl;
            iter->second.push(stInfererenceDataInfo<uint8_t>{id_name, data, input_stream_index, data_id, ReqStartTimer});
        }
        else
        {                                            
            std::pair<std::string, std::queue<stInfererenceDataInfo<uint8_t>>> MapQueueData;

            stInfererenceDataInfo<uint8_t> Data = { id_name, 
                                                    data, 
                                                    input_stream_index,
                                                    data_id, 
                                                    ReqStartTimer};

            MapQueueData.first = id_name;
            MapQueueData.second.push(std::move(Data));
            MapQueueInferData.insert(std::move(MapQueueData));   

        }

        ProtectMutex_MapQueueInferData.unlock();
        
        RetCode = MnpReturnCode::SUCCESS;   
    }
    
    return RetCode;
}
    
const stNetworkModelInfo* MultiNetworkPipeline::GetNetworkInfoByName(const std::string &id_name)
{    
    //Any access method we lock the resources first
    std::lock_guard<std::mutex> lock(mutex_class_protection); 
    return GetNetworkInfoByName_Unprotect(id_name);    
}

std::vector<stNetworkModelInfo> MultiNetworkPipeline::GetAllAddedNetworkList()
{    
    //Any access method we lock the resources first
    std::lock_guard<std::mutex> lock(mutex_class_protection); 
    
    return NetworkModelInfoList; 
}


MnpReturnCode MultiNetworkPipeline::StartPowerMeasure(uint32_t device_id)
{    
    if (!hailo_device_found)
        return MnpReturnCode::HAILO_NOT_INITIALIZED;

    if (device_id >= hailo_device_found)
    {
        DBG_WARN("device id exceeded current maximum hailodevice found (" << hailo_device_found << ")");
        return MnpReturnCode::INVALID_PARAMETER;        
    }

    if (hailo_stop_power_measurement(devices[device_id]) != HAILO_SUCCESS)
    {
        DBG_ERROR("-E- Failed stopping former measurements");
        return MnpReturnCode::FAILED;
    }

    if (hailo_set_power_measurement(devices[device_id], 
                                    HAILO_MEASUREMENT_BUFFER_INDEX_0, HAILO_DVM_OPTIONS_AUTO, HAILO_POWER_MEASUREMENT_TYPES__POWER) != HAILO_SUCCESS)
    {
        DBG_ERROR("-E- Failed setting measurement params");
        return MnpReturnCode::FAILED; 
    }

    if (hailo_start_power_measurement(devices[device_id], HAILO_AVERAGE_FACTOR_256, HAILO_SAMPLING_PERIOD_1100US) != HAILO_SUCCESS)
    {
        DBG_ERROR("-E- Failed to start measurement");
        return MnpReturnCode::FAILED; 
    }

    return MnpReturnCode::SUCCESS;
}

MnpReturnCode MultiNetworkPipeline::StopAndMeasurePower(uint32_t device_id, HailoPower &Powervalue)
{
    hailo_power_measurement_data_t measurement_result;

    if (!hailo_device_found)
        return MnpReturnCode::HAILO_NOT_INITIALIZED;

    if (device_id >= hailo_device_found)
    {
        DBG_WARN("device id exceeded current maximum hailodevice found (" << hailo_device_found << ")");
        return MnpReturnCode::INVALID_PARAMETER;        
    }

    if (hailo_stop_power_measurement(devices[device_id]) != HAILO_SUCCESS)
    {
        DBG_ERROR("-E- Failed stopping former measurements");
        return MnpReturnCode::FAILED;
    }

    if (hailo_get_power_measurement(devices[device_id], HAILO_MEASUREMENT_BUFFER_INDEX_0, true, &measurement_result) != HAILO_SUCCESS)
    {
        DBG_ERROR("-E- Failed to get measurement results");
        return MnpReturnCode::FAILED; 
    }

    Powervalue.min = measurement_result.min_value;
    Powervalue.average = measurement_result.average_value;
    Powervalue.max = measurement_result.max_value;

    return MnpReturnCode::SUCCESS;
}
