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

#include "MultiNetworkPipeline-scheduler.hpp"


/**
 * Static methods should be defined outside the class.
 */

MultiNetworkPipeline* MultiNetworkPipeline::pinstance_{nullptr};
std::mutex MultiNetworkPipeline::mutex_class_protection;


/**
 * Internal Helper Functions
 */

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



/*
 * PRIVATE METHOD
 */

int MultiNetworkPipeline::FindAvailableHefObjSlot() 
{

    int HefObjSlot = -1;
    for (int k = 0; k < TOTAL_HEF_SUPPORTED; k++) {
        if (addedNetworkModel[k].hefObj == NULL) {
            HefObjSlot = k;
            break;
        }
    }
        
    return HefObjSlot;
}


bool MultiNetworkPipeline::isHefObjGivenIdDuplicated(const stNetworkModelInfo &NewNetworkInfo)
{
    bool IdDuplicated = false;
    for (int k = 0; k < TOTAL_HEF_SUPPORTED; k++) {
        if (addedNetworkModel[k].hefObj != NULL) {

            if (addedNetworkModel[k].NetworkModelInfo.id_name.compare(NewNetworkInfo.id_name) == 0) {
                IdDuplicated = true;
                break;
            }
        }
    }

    return IdDuplicated;
}


bool MultiNetworkPipeline::isStreamIdUnique(std::string stream_id)
{
    bool StreamIdIsUnique = true;

    for (size_t i = 0; i < hailo_device_found; i++) {
        for (size_t j = 0; j < NUMBER_STREAM_SUPPORTED; j++) {
            for (size_t k = 0; k < NUMBER_HEF_PER_STREAM_SUPPORTED; k++) {
                if (streamInfoList[i][j][k].SlotInUse) {
                    if (streamInfoList[j][j][k].Stream_Id.compare(stream_id) == 0) {
                        StreamIdIsUnique = false;
                        goto l_exit;
                    }
                }
            }
        }
    }

l_exit:
    return StreamIdIsUnique;
}


int MultiNetworkPipeline::FindHefObjSlot(const stNetworkModelInfo &NewNetworkInfo)
{
    int HefObjSlot = -1;
    for (int k = 0; k < TOTAL_HEF_SUPPORTED; k++) {
        if (addedNetworkModel[k].hefObj != NULL) {

            if (addedNetworkModel[k].NetworkModelInfo.id_name.compare(NewNetworkInfo.id_name) == 0) {
                HefObjSlot = k;
                break;
            }
        }
    }

    return HefObjSlot;
}


bool MultiNetworkPipeline::isHefObjAlreadyAdded(const stNetworkModelInfo &NewNetworkInfo)
{
    bool AlreadyAdded = false;
    for (int k = 0; k < TOTAL_HEF_SUPPORTED; k++) {
        if (addedNetworkModel[k].hefObj != NULL) {

            if (addedNetworkModel[k].NetworkModelInfo.hef_path.compare(NewNetworkInfo.hef_path) == 0) {
                AlreadyAdded = true;
                break;
            }
        }
    }

    return AlreadyAdded;
}


int  MultiNetworkPipeline::FindNetworkGroupCorrespondingStreamIndex(uint32_t device_id, std::string stream_id)
{
    int stream_channel_index = -1;

    for (int i=0; i < NUMBER_STREAM_SUPPORTED; i++) {

        for (int k=0; k < NUMBER_HEF_PER_STREAM_SUPPORTED; k++) {

            if (streamInfoList[device_id][i][k].SlotInUse)
            {
                if (streamInfoList[device_id][i][k].Stream_Id.compare(stream_id) == 0) {
                    stream_channel_index = i;
                    break;
                }
            }
        }

        //Found?
        if (stream_channel_index >= 0) {
            break;
        }
    }

    return stream_channel_index;
}


int  MultiNetworkPipeline::FindNetworkGroupAvailableStreamIndex(uint32_t device_id)
{
    int stream_channel_index = -1;

    for (int i=0; i < NUMBER_STREAM_SUPPORTED; i++) {

        stream_channel_index = i;

        for (int k=0; k < NUMBER_HEF_PER_STREAM_SUPPORTED; k++) {

            //If any hef per stream slot is in use then this stream index is occupied, we break and reset the stream_channel_index
            if (streamInfoList[device_id][i][k].SlotInUse)
            {
                stream_channel_index = -1;
                break;
            }
        }

        //We do find empty slot
        if (stream_channel_index >= 0)
            break;
    }

    return stream_channel_index;
}


int  MultiNetworkPipeline::FindNetworkGroupAvailableSlot(uint32_t device_id, uint32_t stream_channel, const stNetworkModelInfo &NewNetworkInfo)
{
    //-1 is already added (network already created for the selected stream on selected device), its not really an error and will be ignored
    //-2 reached capacity, need to adjust NUMBER_HEF_PER_STREAM_SUPPORTED if more network in pipeline is needed per stream.
    int NetworkGroupAvailableSlot = -2;

    for (int i=0; i < NUMBER_HEF_PER_STREAM_SUPPORTED; i++) {

        if (streamInfoList[device_id][stream_channel][i].SlotInUse)
        {
            if (streamInfoList[device_id][stream_channel][i].NetworkIdName.compare(NewNetworkInfo.id_name) == 0) {
                NetworkGroupAvailableSlot = -1;
                break;
            }
        }
        else {
            
            //Reaching here means slot is not in use, we will just record the first found slot.
            if (NetworkGroupAvailableSlot < 0) {
                NetworkGroupAvailableSlot = i;
            }
        }
    }

    return NetworkGroupAvailableSlot;

}


stHailoStreamInfo*  MultiNetworkPipeline::GetNetworkStreamInfoFromAnyMatchingNetwork(std::string id_name)
{
    for (size_t i = 0; i < hailo_device_found; i++) {
        for (int j = 0; j < NUMBER_STREAM_SUPPORTED; j++) {
            for (int k = 0; k < NUMBER_HEF_PER_STREAM_SUPPORTED; k++) {
                if (streamInfoList[i][j][k].SlotInUse) {
                    if (streamInfoList[i][j][k].NetworkIdName.compare(id_name) == 0) {
                        return &streamInfoList[i][j][k];
                    }
                }
            }
        }
    }

    return NULL;
}


stHailoStreamInfo*  MultiNetworkPipeline::GetNetworkStreamInfoFromStreamChannel(std::string id_name, std::string stream_id /*= "default" */)
{
    int     stream_channel_index = -1;
    size_t  device_id = 0;

    for (size_t i = 0; i < hailo_device_found; i++) {

        stream_channel_index = FindNetworkGroupCorrespondingStreamIndex(i, stream_id);

        if (stream_channel_index >= 0) {
            device_id = i;
            break;
        }
    }

    if (stream_channel_index >= 0) {
        for (int k = 0; k < NUMBER_HEF_PER_STREAM_SUPPORTED; k++) {
            if (streamInfoList[device_id][stream_channel_index][k].SlotInUse) {
                if (streamInfoList[device_id][stream_channel_index][k].NetworkIdName.compare(id_name) == 0) {
                    return &streamInfoList[device_id][stream_channel_index][k];
                }
            }
        }
    }

    return NULL;
}





/*
 * PUBLIC METHOD
 */


MultiNetworkPipeline::MultiNetworkPipeline()
{

#ifdef LARGE_INFER_QUEUE_FOR_UNIT_TEST
    DBG_WARN("LARGE_INFER_QUEUE_FOR_UNIT_TEST is defined, please remove from CMakeList if this is not a unit test build");
#endif

    hailo_device_found = 0;
            
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


MnpReturnCode MultiNetworkPipeline::ReleaseStreamChannel(uint32_t device_id, std::string stream_id /*= "default"*/)
{
    //TODO: When there is time, revise this as too many depth, may have to revise how we store streamInfoList

    if (!hailo_device_found)
        return MnpReturnCode::HAILO_NOT_INITIALIZED;

    if (device_id >= hailo_device_found)
    {
        DBG_WARN("device id exceeded current maximum hailodevice found (" << hailo_device_found << ")");
        return MnpReturnCode::INVALID_PARAMETER;
    }

    //Protect resource
    std::lock_guard<std::mutex> lock(mutex_class_protection);

    int stream_channel_index = FindNetworkGroupCorrespondingStreamIndex(device_id, stream_id);
    if (stream_channel_index >=0) {
        for (size_t k = 0; k < NUMBER_HEF_PER_STREAM_SUPPORTED; k++) {
            if (streamInfoList[device_id][stream_channel_index][k].SlotInUse) {

                (void)hailo_release_output_vstreams(streamInfoList[device_id][stream_channel_index][k].NetVstreamOutputs,
                                                    streamInfoList[device_id][stream_channel_index][k].NetVstreamOutputCount);

                (void)hailo_release_input_vstreams( streamInfoList[device_id][stream_channel_index][k].NetVstreamInputs,
                                                    streamInfoList[device_id][stream_channel_index][k].NetVstreamInputCount);


                streamInfoList[device_id][stream_channel_index][k].SlotInUse = false;
                streamInfoList[device_id][stream_channel_index][k].Stream_Id = "";
            }
        }
    }
    else {
        DBG_WARN("Given stream_id not found");
        return MnpReturnCode::INVALID_PARAMETER;
    }

    return MnpReturnCode::SUCCESS;
}


MnpReturnCode MultiNetworkPipeline::ReleaseAllResource()
{
    MnpReturnCode RetCode = MnpReturnCode::SUCCESS;        
    
    //Protect resource
    std::lock_guard<std::mutex> lock(mutex_class_protection);

    //TODO: When there is time, revise this as too many depth, may have to revise how we store streamInfoList
    for (size_t i = 0; i < NUMBER_OF_DEV_SUPPORTED; i++) {
        for (size_t j = 0; j < NUMBER_STREAM_SUPPORTED; j++) {
            for (size_t k = 0; k < NUMBER_HEF_PER_STREAM_SUPPORTED; k++) {
                if (streamInfoList[i][j][k].SlotInUse) {
                    (void)hailo_release_output_vstreams(streamInfoList[i][j][k].NetVstreamOutputs,
                                                        streamInfoList[i][j][k].NetVstreamOutputCount);

                    (void)hailo_release_input_vstreams( streamInfoList[i][j][k].NetVstreamInputs,
                                                        streamInfoList[i][j][k].NetVstreamInputCount);


                    streamInfoList[j][j][k].SlotInUse = false;
                    streamInfoList[j][j][k].Stream_Id = "";
                }
            }
        }
    }


    for (int j = 0; j < TOTAL_HEF_SUPPORTED; j++) {     
        if (addedNetworkModel[j].hefObj != NULL) {
            (void) hailo_release_hef(addedNetworkModel[j].hefObj);
        }
    }


    for (int i = 0; i < NUMBER_OF_DEV_SUPPORTED; i++) {
        if (vdevices[i] != NULL) {
            std::cout << "Release Device-" << i << "Address: " << &vdevices[i] << std::endl;
            (void) hailo_release_vdevice(vdevices[i]);
            
        }        
    }

    std::cout << "Release Done" << std::endl;

    hailo_device_found = 0;
    
l_exit:
    return RetCode;
}


size_t MultiNetworkPipeline::InitializeHailo()
{    
    hailo_status status = HAILO_SUCCESS;

    if (hailo_device_found)
        goto init_exit;
    
    for (int i = 0; i < NUMBER_OF_DEV_SUPPORTED; i++) {
        vdevices[i] = NULL;        
    }

    for (int j = 0; j < TOTAL_HEF_SUPPORTED; j++) {        
        addedNetworkModel[j].hefObj = NULL;
    }

    try {

        hailo_device_found = NUMBER_OF_DEV_SUPPORTED;
        hailo_scan_devices(NULL, device_ids, &hailo_device_found);
        
        if (hailo_device_found > NUMBER_OF_DEV_SUPPORTED)
        {
            DBG_ERROR("-E- found " << hailo_device_found << " device(s) but exceeding max device support of " << NUMBER_OF_DEV_SUPPORTED);
            hailo_device_found = 0;
        }

        for (size_t i = 0; i < hailo_device_found; i++) {
            hailo_vdevice_params_t params = {0};

            if (hailo_init_vdevice_params(&params) !=  HAILO_SUCCESS){
                DBG_ERROR("-E- failed to create hailo_init_vdevice_params");
                break;
            }

            //TODO: Here we create each device as individual entity, application can have the flexibility
            //      to decide multinetwork schedule for each hailo device, in the future we might be able to
            //      support multi-device network scheduler, when it does we will have to revice the code
            //      here and make sure we can have the option for it.
            params.device_count = 1;
            params.device_ids = &device_ids[i];
            params.scheduling_algorithm = HAILO_SCHEDULING_ALGORITHM_ROUND_ROBIN;
            params.multi_process_service = false;       //Set default to false, we can try to support this later
            status = hailo_create_vdevice(&params, &vdevices[i]);
            
            std::cout << "Allocated vDevice Address: " << &vdevices[i] << std::endl;

            REQUIRE_SUCCESS_CHECK(status, init_exit, "Failed to create hailo_create_vdevice");

        }

       
    } catch (std::exception const& e) {
        DBG_ERROR("-E- create device failed" << e.what());
    }

    
init_exit:
    
    return hailo_device_found;
}



MnpReturnCode MultiNetworkPipeline::AddNetwork(uint32_t device_id, const stNetworkModelInfo &NewNetworkInfo, std::string stream_id /* = "default" */)
{
    
    MnpReturnCode RetCode = MnpReturnCode::FAILED;
    hailo_status status = HAILO_SUCCESS;
    int NetworkGroupAvailableSlotIndex = -2;
    int stream_channel_index = -1;

    if (!hailo_device_found)
        return MnpReturnCode::HAILO_NOT_INITIALIZED;

    if (device_id >= hailo_device_found)
    {
        DBG_WARN("device id exceeded current maximum hailodevice found (" << hailo_device_found << ")");
        return MnpReturnCode::INVALID_PARAMETER;        
    }

    if (FindNetworkGroupCorrespondingStreamIndex(device_id, stream_id) <= 0) {
        if (FindNetworkGroupAvailableStreamIndex(device_id) < 0)
        {
            DBG_WARN("max stream allowed reached (Please increase NUMBER_STREAM_SUPPORTED)");
            return MnpReturnCode::INVALID_PARAMETER;
        }
    }

    if ((NewNetworkInfo.out_format != HAILO_FORMAT_TYPE_UINT8) && 
        (NewNetworkInfo.out_format != HAILO_FORMAT_TYPE_FLOAT32))
    {
        DBG_WARN("Configured output format not yet supported.");
        return MnpReturnCode::INVALID_PARAMETER;
    }

    //Any manipulation method we lock the resources first
    std::lock_guard<std::mutex> lock(mutex_class_protection);            
  
    /* Create HEF Obj */

    if (isHefObjAlreadyAdded(NewNetworkInfo) == false) {

        if (isHefObjGivenIdDuplicated(NewNetworkInfo)) {
            DBG_WARN("Add new network given id_name already exist (must be unique) : " << NewNetworkInfo.id_name);
            return MnpReturnCode::INVALID_PARAMETER;
        }

        int AvailableSLot = FindAvailableHefObjSlot();
        if (AvailableSLot < 0) {
            DBG_WARN("Max Hef allowed reached (Please increase TOTAL_HEF_SUPPORTED define).");
            return MnpReturnCode::INVALID_PARAMETER;
        }

        //Add the new network to the list
        //NOTE: Each network is only required to be added once (eg, hailo_create_hef_file). Creating same hef file MULTIPLE TIMES
        //      will waste resources (memory), although doing it will still work but its waste of memory especially in enbedded system with
        //      limited memory.

        addedNetworkModel[AvailableSLot].NetworkModelInfo = NewNetworkInfo;

        status = hailo_create_hef_file(&addedNetworkModel[AvailableSLot].hefObj, NewNetworkInfo.hef_path.c_str());
        REQUIRE_SUCCESS_CHECK(status, l_exit, "Failed to create hef file");
    }

    /* Config vdevice (network group)*/
    {
        int HefObjSlot = FindHefObjSlot(NewNetworkInfo);
        if (HefObjSlot < 0) {
            DBG_WARN("Unable to find matched id_name while hef path (network) is already added in the list");
            return MnpReturnCode::INVALID_PARAMETER;
        }

        //First find if the already in use stream, if not found it mean new stream and we get new available stream slot
        stream_channel_index = FindNetworkGroupCorrespondingStreamIndex(device_id, stream_id);
        if (stream_channel_index < 0) {
            //Not found, so it must be new stream, let's first check if the given stream_id is unique across all available devices
            if (isStreamIdUnique(stream_id) == false) {
                DBG_WARN("Give stream_id for new stream MUST be unique across all devices.");
                return MnpReturnCode::INVALID_PARAMETER;
            }

            stream_channel_index = FindNetworkGroupAvailableStreamIndex(device_id);
        }

        NetworkGroupAvailableSlotIndex = FindNetworkGroupAvailableSlot(device_id, stream_channel_index, NewNetworkInfo);
        if (NetworkGroupAvailableSlotIndex < 0) {
            if (NetworkGroupAvailableSlotIndex == -1) {
                DBG_WARN("Network already configured for the stream, network id_name: " << NewNetworkInfo.id_name << ", will skip as success");   
                return MnpReturnCode::SUCCESS;         
            }

            if (NetworkGroupAvailableSlotIndex == -2) {
                DBG_WARN("Available network group per stream exceed limitation, check NUMBER_HEF_PER_STREAM_SUPPORTED");   
                return MnpReturnCode::INVALID_PARAMETER;         
            }

        }

        hailo_configure_params_t configure_params = {0};
        size_t network_groups_size = 1;

        status = hailo_init_configure_params(addedNetworkModel[HefObjSlot].hefObj, HAILO_STREAM_INTERFACE_PCIE, &configure_params);
        REQUIRE_SUCCESS_CHECK(status, l_exit, "Failed to hailo_init_configure_params");

        status = hailo_configure_vdevice(   vdevices[device_id], 
                                            addedNetworkModel[HefObjSlot].hefObj, 
                                            &configure_params,
                                            &streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetworkGroups,
                                            &network_groups_size);
        REQUIRE_SUCCESS_CHECK(status, l_exit, "Failed to hailo_configure_vdevice");
        if (network_groups_size > 1) {
            DBG_WARN("Hef with more that one network group, currently not supported. Contact field support agent for detail");
        }

        streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetworkIdName = NewNetworkInfo.id_name;
        streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].SlotInUse = true;
        streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].Stream_Id = stream_id;

        /* Make Input/Output VStream */
        streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamInputCount = MAX_SUPPORTED_INPUT_LAYER;
        status = hailo_make_input_vstream_params(streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetworkGroups,
                                                NewNetworkInfo.in_quantized, 
                                                NewNetworkInfo.in_format,
                                                streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamInputParam,
                                                &streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamInputCount);
        REQUIRE_SUCCESS_CHECK(status, l_exit, "Failed making input virtual stream params");


        streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamOutputCount = MAX_SUPPORTED_OUTPUT_LAYER;
        status = hailo_make_output_vstream_params(  streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetworkGroups,
                                                    NewNetworkInfo.out_quantized, 
                                                    NewNetworkInfo.out_format,
                                                    streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamOutputParam,
                                                    &streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamOutputCount);
        
        REQUIRE_SUCCESS_CHECK(status, l_exit, "Failed making output virtual stream params");
    
    }

    /* Create Virtual Input Stream */
    {
        bool bUseDefaultInputOrder = true;
        if (!NewNetworkInfo.input_order_by_name.empty())
            bUseDefaultInputOrder = false;

        size_t InputStreamCount = streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamInputCount;
        for (size_t i = 0; i < InputStreamCount; i++)
        {

            if (bUseDefaultInputOrder)
            {

#ifdef LARGE_INFER_QUEUE_FOR_UNIT_TEST
                streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamInputParam[i].params.queue_size = 20;
#endif
                status = hailo_create_input_vstreams(   streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetworkGroups,
                                                        &streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamInputParam[i],
                                                        1, 
                                                        &streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamInputs[i]);
                REQUIRE_SUCCESS_CHECK(status, l_exit, "Failed creating virtual input stream");
                DBG_INFO( "inut_stream_by_name " << streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamInputParam[i].name);

            }
            else 
            {
                bool bFound = false;
                for (size_t j = 0; j < InputStreamCount; j++)
                {
                    if (strcmp(streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamInputParam[j].name, NewNetworkInfo.input_order_by_name[i].c_str()) == 0)
                    {
                        status = hailo_create_input_vstreams(   streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetworkGroups,
                                                                &streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamInputParam[j],
                                                                1, 
                                                                &streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamInputs[i]);

                        DBG_INFO( "inut_stream_by_name " << streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamInputParam[j].name);

                        REQUIRE_SUCCESS_CHECK(status, l_exit, "Failed creating virtual input stream");

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


            status = hailo_get_input_vstream_frame_size(streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamInputs[i],
                                                        &streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamInputFrameSize[i]);
            REQUIRE_SUCCESS_CHECK(status, l_exit, "Failed getting input virtual stream frame size");

            //Get input stream info
            hailo_vstream_info_t in_vstream_info;
            status = hailo_get_input_vstream_info(  streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamInputs[i],
                                                    &in_vstream_info);
            REQUIRE_SUCCESS_CHECK(status, l_exit, "Failed hailo_get_input_vstream_info");
            
            /* Get the quantization info */
            qp_zp_scale_t transformScale = {in_vstream_info.quant_info.qp_zp,
                                            in_vstream_info.quant_info.qp_scale};
            streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamInputQuantInfo[i] = transformScale;
            //std::cout << "input(" <<  in_vstream_info.name << ")stream format type: " << in_vstream_info.format.type << ", quant info scale = " << transformScale.qp_scale << " zero point = " << transformScale.qp_zp << std::endl;

        }    
    }

    /* Create Virtual Output Stream */
    {
        bool bUseDefaultOutputOrder = true;
        if (!NewNetworkInfo.output_order_by_name.empty())
            bUseDefaultOutputOrder = false;

        size_t OutputStreamCount = streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamOutputCount;

        for (size_t i = 0; i < OutputStreamCount; i++)
        {

            if (bUseDefaultOutputOrder)
            {
                status = hailo_create_output_vstreams(  streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetworkGroups,
                                                        &streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamOutputParam[i],
                                                        1, 
                                                        &streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamOutputs[i]);
                REQUIRE_SUCCESS_CHECK(status, l_exit, "Failed creating virtual output stream");

                DBG_INFO( "output_stream_by_name " << streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamOutputParam[i].name);

            }
            else 
            {
                bool bFound = false;
                for (size_t j = 0; j < OutputStreamCount; j++)
                {
                    if (strcmp(streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamOutputParam[j].name, NewNetworkInfo.output_order_by_name[i].c_str()) == 0)
                    {
                        status = hailo_create_output_vstreams(  streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetworkGroups,
                                                                &streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamOutputParam[j],
                                                                1, 
                                                                &streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamOutputs[i]);

                        DBG_INFO( "output_stream_by_name " << streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamOutputParam[j].name);

                        REQUIRE_SUCCESS_CHECK(status, l_exit, "Failed creating virtual output stream");


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


            status = hailo_get_output_vstream_frame_size(streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamOutputs[i],
                                                        &streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamOutputFrameSize[i]);
            REQUIRE_SUCCESS_CHECK(status, l_exit, "Failed getting output virtual stream frame size");

            //Get output stream info
            hailo_vstream_info_t out_vstream_info;
            status = hailo_get_output_vstream_info( streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamOutputs[i],
                                                    &out_vstream_info);
            REQUIRE_SUCCESS_CHECK(status, l_exit, "Failed hailo_get_output_vstream_info");
            
            /* Get the quantization info */
            qp_zp_scale_t transformScale = {out_vstream_info.quant_info.qp_zp,
                                            out_vstream_info.quant_info.qp_scale};
            streamInfoList[device_id][stream_channel_index][NetworkGroupAvailableSlotIndex].NetVstreamOutputQuantInfo[i] = transformScale;
            //std::cout << "output(" <<  out_vstream_info.name << ")stream format type: " << out_vstream_info.format.type << ", quant info scale = " << transformScale.qp_scale << " zero point = " << transformScale.qp_zp << std::endl;

        }    
    }

    RetCode = MnpReturnCode::SUCCESS;
    
l_exit:
    
    return RetCode;
}

MnpReturnCode MultiNetworkPipeline::GetNetworkInputSize(const std::string &id_name, size_t &NetworkInputSize, size_t input_stream_index /*= 0*/)
{
    MnpReturnCode RetCode = MnpReturnCode::NOT_FOUND;
    NetworkInputSize = 0;
    
    if (!hailo_device_found)
        return MnpReturnCode::HAILO_NOT_INITIALIZED;    

    //Protect resource
    std::lock_guard<std::mutex> lock(mutex_class_protection);

    stHailoStreamInfo* pStreamInfo = MultiNetworkPipeline::GetNetworkStreamInfoFromAnyMatchingNetwork(id_name);

    if (pStreamInfo) {

        if (input_stream_index >= pStreamInfo->NetVstreamInputCount) {
            RetCode = MnpReturnCode::INVALID_PARAMETER;
        }
        else {
            NetworkInputSize = pStreamInfo->NetVstreamInputFrameSize[input_stream_index];
            RetCode = MnpReturnCode::SUCCESS;
        }
    }
    
    return RetCode;
}         


MnpReturnCode MultiNetworkPipeline::GetNetworkQuantizationInfo(const std::string &id_name, std::vector<qp_zp_scale_t> &NetworkQuantInfo, bool get_from_output_stream /* = true */)
{
    MnpReturnCode RetCode = MnpReturnCode::NOT_FOUND;
    
    if (!hailo_device_found)
        return MnpReturnCode::HAILO_NOT_INITIALIZED;    

    //Protect resource
    std::lock_guard<std::mutex> lock(mutex_class_protection);

    stHailoStreamInfo* pStreamInfo = MultiNetworkPipeline::GetNetworkStreamInfoFromAnyMatchingNetwork(id_name);

    if (pStreamInfo) {

        if (get_from_output_stream) {

            for (size_t i = 0; i < pStreamInfo->NetVstreamOutputCount; i++) {
                NetworkQuantInfo.push_back(pStreamInfo->NetVstreamOutputQuantInfo[i]);
            }
        }
        else {

            for (size_t i = 0; i < pStreamInfo->NetVstreamInputCount; i++) {
                NetworkQuantInfo.push_back(pStreamInfo->NetVstreamInputQuantInfo[i]);
            }

        }

        RetCode = MnpReturnCode::SUCCESS;
    }
    
    return RetCode;
}     


MnpReturnCode MultiNetworkPipeline::Infer(const std::string &id_name, const std::vector<uint8_t> &data, std::string stream_id /*="default"*/, size_t input_stream_index /*=0*/)
{
    MnpReturnCode RetCode = MnpReturnCode::NOT_FOUND;       
    stHailoStreamInfo* pStreamInfo = nullptr;

    if (!hailo_device_found)
        return MnpReturnCode::HAILO_NOT_INITIALIZED;
    
    
    mutex_class_protection.lock();

    pStreamInfo = MultiNetworkPipeline::GetNetworkStreamInfoFromStreamChannel(id_name, stream_id);

    mutex_class_protection.unlock();

    if (pStreamInfo) {

        hailo_status status = HAILO_SUCCESS;
        status = hailo_vstream_write_raw_buffer(pStreamInfo->NetVstreamInputs[input_stream_index], 
                                                (void*)data.data(), 
                                                pStreamInfo->NetVstreamInputFrameSize[input_stream_index]);                                                
        REQUIRE_SUCCESS_CHECK(status, l_exit, "hailo_stream_sync_write_all_raw_buffer failed");

        RetCode = MnpReturnCode::SUCCESS;
    }

l_exit:
    
    return RetCode;
}



MnpReturnCode MultiNetworkPipeline::ReadOutputById(const std::string &id_name, std::vector<std::vector<float32_t>>& output_buffer, std::string stream_id /*= "default"*/)
{    
    MnpReturnCode RetCode = MnpReturnCode::NOT_FOUND;
    stHailoStreamInfo* pStreamInfo = nullptr;

    if (!hailo_device_found)
        return MnpReturnCode::HAILO_NOT_INITIALIZED;
    
    mutex_class_protection.lock();

    pStreamInfo = MultiNetworkPipeline::GetNetworkStreamInfoFromStreamChannel(id_name, stream_id);

    mutex_class_protection.unlock();

    if (pStreamInfo) {

       if (CheckOutputBuffer<float32_t>(pStreamInfo, output_buffer) != MnpReturnCode::SUCCESS) {
            RetCode = MnpReturnCode::INVALID_PARAMETER;
            DBG_ERROR("output_buffer needs to be initialized (InitializeOutputBuffer)");
            goto l_exit;
        }

        hailo_status status = HAILO_SUCCESS;
        for (size_t i = 0; i < pStreamInfo->NetVstreamOutputCount; i++) {

            status = hailo_vstream_read_raw_buffer( pStreamInfo->NetVstreamOutputs[i], 
                                                    output_buffer[i].data(), 
                                                    pStreamInfo->NetVstreamOutputFrameSize[i]);            

            REQUIRE_SUCCESS_CHECK(status, l_exit, "hailo_vstream_read_raw_buffer failed");                       

        }

        RetCode = MnpReturnCode::SUCCESS;
    }

l_exit:

    return RetCode;    
}


MnpReturnCode MultiNetworkPipeline::ReadOutputById(const std::string &id_name, std::vector<std::vector<uint8_t>>& output_buffer, std::string stream_id /*= "default"*/)
{

    MnpReturnCode RetCode = MnpReturnCode::NO_DATA_AVAILABLE;
    stHailoStreamInfo* pStreamInfo = nullptr;

    if (!hailo_device_found)
        return MnpReturnCode::HAILO_NOT_INITIALIZED;
    
    mutex_class_protection.lock();

    pStreamInfo = MultiNetworkPipeline::GetNetworkStreamInfoFromStreamChannel(id_name, stream_id);

    mutex_class_protection.unlock();

    if (pStreamInfo) {

        hailo_status status = HAILO_SUCCESS;

        if (CheckOutputBuffer<uint8_t>(pStreamInfo, output_buffer) != MnpReturnCode::SUCCESS) {
            RetCode = MnpReturnCode::INVALID_PARAMETER;
            DBG_ERROR("output_buffer needs to be initialized (InitializeOutputBuffer)");
            goto l_exit;
        }

        for (size_t i = 0; i < pStreamInfo->NetVstreamOutputCount; i++) {

            status = hailo_vstream_read_raw_buffer( pStreamInfo->NetVstreamOutputs[i], 
                                                    output_buffer[i].data(), 
                                                    pStreamInfo->NetVstreamOutputFrameSize[i]);

            REQUIRE_SUCCESS_CHECK(status, l_exit, "hailo_vstream_read_raw_buffer failed");                       
        }

        RetCode = MnpReturnCode::SUCCESS;
    }

l_exit:

    

    return RetCode;       

}
