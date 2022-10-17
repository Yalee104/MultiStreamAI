/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   MultiNetworkPipeline.hpp
 * Author: Aaron
 * 
 * Created on July 5, 2021, 11:32 AM
 */

#ifndef _MultiNetworkPipeline_H_
#define _MultiNetworkPipeline_H_

#include <time.h>
#include <vector>
#include <ctype.h>
#include <cstring> 
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <queue>
#include <hailo/hailort.h>
#include "Utils/SharedQueue.hpp"
#include "Utils/Timer.hpp"
#include "Utils/counter-master/Counter.h"
#include <type_traits>


/*
 *  JOIN_NETWORK_SINGLE_CONTEXT_INDEPENDENT_INFER 
 *  Enabling this allows application to infer specific input network in a join network. HOWEVER, this 
 *  only works if the join network is compiled as single context, DO NOT enable this if you are
 *  going to use HEF file that is join network with multiple context. 
 */
//#define JOIN_NETWORK_SINGLE_CONTEXT_INDEPENDENT_INFER


//TODO: Implement a logging module for more flexibility
//#define LOG_INFO_ENABLE
#define LOG_WARN_ENABLE
#define LOG_ERROR_ENABLE
//#define LOG_DEBUG_ENABLE

#ifdef LOG_INFO_ENABLE 
#define DBG_INFO(MSG)   std::cout<<"[INFO] "<<MSG<<std::endl;
#else
#define DBG_INFO(MSG)
#endif

#ifdef LOG_WARN_ENABLE 
#define DBG_WARN(MSG)   std::cout<<"[WARNING] "<<MSG<<std::endl;
#else
#define DBG_WARN(MSG)
#endif

#ifdef LOG_ERROR_ENABLE 
#define DBG_ERROR(MSG)   std::cout<<"[ERROR] "<<MSG<<std::endl;
#else
#define DBG_ERROR(MSG)
#endif

#ifdef LOG_DEBUG_ENABLE 
#define DBG_DEBUG(MSG)   std::cout<<"[DEBUG] "<<MSG<<std::endl;
#else
#define DBG_DEBUG(MSG)
#endif


#define NUMBER_OF_DEV_SUPPORTED     (4)
#define NUMBER_OF_OUTPUT_STREAMS    (20)
#define NUMBER_OF_INPUT_STREAMS     (4)
#define MAX_NUM_GROUP_INFO          (4)

#define REQUIRE_SUCCESS_CHECK(status, label, msg) \
    do                                      \
    {                                       \
        if (HAILO_SUCCESS != (status))      \
        {                                   \
            DBG_ERROR("Hailo Code: " << status << " Msg: " << msg) \
            goto label;                     \
        }                                   \
    } while (0)


enum class MnpReturnCode {
    
    //Success Code List
    SUCCESS                 =0,
    DUPLICATED              =1,
    NO_DATA_AVAILABLE       =2,
    
    //Failure Code list
    FAILED                  =-1,
    NOT_FOUND               =-2,
    RUNNING_INFERENCE       =-3,
    HAILO_NOT_INITIALIZED   =-4,
    INVALID_PARAMETER       =-5,
    
};

enum eThreadStatus {
    
    HOLD        =   0,
    RUN         =   1,
    STOP        =   2,
};

typedef struct HailoPower {
    float32_t min;
    float32_t average;
    float32_t max;
} HailoPower;

struct AtomicIntCount {
    
    Counter32   value = 0;
    Counter32   accumulated = 0;
    std::mutex  protect_mutex;
    
    bool isZeroValue() {
        std::lock_guard<std::mutex> lock(protect_mutex);  
        bool isZero = (value==0) ? true : false; 
        return isZero;
    }
    
    //Do not allow assignment
    void operator = (int) = delete;
    
    // Overload ++ when used as prefix
    void operator ++ () {
        std::lock_guard<std::mutex> lock(protect_mutex); 
        ++value;
        ++accumulated;
    }

    // Overload ++ when used as postfix
    void operator ++ (int) {
        std::lock_guard<std::mutex> lock(protect_mutex); 
        value++;
        accumulated++;

        DBG_DEBUG("AtomicInt++ = " << value.ToUnsigned() << ", Accumulated = " << accumulated.ToUnsigned());
    }

    // Overload -- when used as prefix
    void operator -- () {
        std::lock_guard<std::mutex> lock(protect_mutex); 
        --value;
    }

    // Overload -- when used as postfix
    void operator -- (int) {
        std::lock_guard<std::mutex> lock(protect_mutex); 
        value--;

        DBG_DEBUG("AtomicInt-- = " << value.ToUnsigned() << ", Accumulated = " << accumulated.ToUnsigned());        
    }
    
};

struct stStreamingConfig {
    
    //NOTE: You can set MinInferFrame to a higher value (say 10) while still keeping MinInferWaitMs to 0
    //      in this case it will try to infer as much of frame in the queue of the same network.
    //      The basic mechanics of how it works is frame infer request are first pushed into queue (queue oer network)
    //      during actual infer (sent to Hailo) it will first look for the first infer frame request across network queue,
    //      when found it will eaither wait for MinInferWaitMs or infer up to MinInferFrame should the queue
    //      of the network contain more than 1 frame.
    size_t      MinInferFrame = 1;  //Minimum infer frame of same network request
    size_t      MinInferWaitMs = 0; //Minimum wait time millisecond before infer of same network
   
};

template <typename T>
struct stInfererenceDataInfo {
    
    std::string     network_id_name;
    std::vector<T>  data;
    size_t          input_stream_index;    
    std::string     data_id;
    TimerMs         requested_time;
};

template <typename T>
struct stStreamDataInfo {
    
    std::string     network_id_name;
    std::vector<T>  data;
   
};

struct stRecvThreadArgs
{
    int                                                 index;
    std::string                                         id_name;
    AtomicIntCount                                      *pdata_waiting_for_inference_count;
    SharedQueue<stStreamDataInfo<float32_t>>            *output_stream_queue_32f;
    SharedQueue<stStreamDataInfo<uint8_t>>              *output_stream_queue_uint8;
    hailo_output_vstream                                output_stream;
    eThreadStatus                                       status = HOLD;
    eThreadStatus                                       last_change_status = HOLD;
    size_t                                              host_output_frame_size;
    hailo_format_type_t                                 output_format;
};

struct stNetworkModelInfo {
    
    /* MUST BE UNIQUE - Name of the network given by user */
    std::string                 id_name;

    /* Path and file name of model hef file */
    std::string                 hef_path;

    /* The Specify the order of input layer, if empty order will be default order
       NOTE:    Usually a network has only one input, in such case input_order_by_name
                can be empty. However, if network has multiple input then it is
                highly suggested to provide input_order_by_name. This given order
                will be the input_stream_index when doing inference.
    */
    std::vector<std::string>    input_order_by_name;

    /* The layer output name by order, if empty default output order will be used. */                 
    std::vector<std::string>    output_order_by_name;

    /* Default to 1 if given 0 */
    unsigned int                batch_size = 1;

    /* NOTE: Suggest to set in one of the following combination
    *  Transformation By HailoRT
    *       In this mode HailoRT library will transform the output data from UINT8 to
    *       Float32.
    *       Setting:  out_quantized=false, out_format=HAILO_FORMAT_TYPE_FLOAT32
    * 
    * Transformation By Host
    *       In this mode HailoRT library will provide output prediction result in UINT8 (native result)
    *       this is useful when Host would like to get the result in raw and filter it before 
    *       transforming it to float 32 for further processing. Its usefull to save some CPU resources
    *       for large output prediction results such as Yolov5
    *       Setting:  out_quantized=true, out_format=HAILO_FORMAT_TYPE_UINT8
    *
    */
    bool                        out_quantized = false;                  //Set to False for default
    hailo_format_type_t         out_format = HAILO_FORMAT_TYPE_FLOAT32; //Set to HAILO_FORMAT_TYPE_FLOAT32 for default    

    /* WARNING: For reserve future use, currently not supported for
     *          any other initialized value as shown below
     */
    bool                        in_quantized = false;                   //Set to False for default
    hailo_format_type_t         in_format = HAILO_FORMAT_TYPE_UINT8;    //Default to HAILO_FORMAT_TYPE_UINT8 for default
};

typedef struct qp_zp_scale_t {
    float32_t qp_zp;
    float32_t qp_scale;
} qp_zp_scale_t;

struct stNetworkObjInfo {
    
    std::string                             id_name;
    bool                                    network_is_active;
    bool                                    network_is_multi_context;
    hailo_hef                               hef;
    hailo_activated_network_group           active_net_g;
    hailo_configured_network_group          network_group;
    hailo_input_vstream_params_by_name_t    input_stream_params[NUMBER_OF_INPUT_STREAMS];
    hailo_output_vstream_params_by_name_t   output_stream_params[NUMBER_OF_OUTPUT_STREAMS];    
    size_t                                  output_stream_cnt = NUMBER_OF_OUTPUT_STREAMS;
    size_t                                  input_stream_cnt = NUMBER_OF_INPUT_STREAMS;
    hailo_input_vstream                     input_stream[NUMBER_OF_INPUT_STREAMS];
    hailo_output_vstream                    output_streams[NUMBER_OF_OUTPUT_STREAMS];
    std::vector<std::string>                input_stream_group_names;
    std::vector<std::vector<size_t>>        output_stream_group;    
    size_t                                  host_input_frame_size[NUMBER_OF_INPUT_STREAMS];
    size_t                                  host_output_frame_size[NUMBER_OF_OUTPUT_STREAMS];
    AtomicIntCount                          *pdata_waiting_for_inference_count[NUMBER_OF_INPUT_STREAMS];
    std::vector<std::thread>                outputstream_recv_pthreads;
    std::vector<stRecvThreadArgs>           outputstream_recv_args;
    std::vector<SharedQueue<stStreamDataInfo<float32_t>>*> outputstream_queue_32f;
    std::vector<SharedQueue<stStreamDataInfo<uint8_t>>*>   outputstream_queue_uint8;
    std::vector<qp_zp_scale_t>                           output_quantization_info;       //Output stream quantization info
    std::vector<qp_zp_scale_t>                           input_quantization_info;        //input stream quantization info

    std::vector<std::string>                data_id_afterInfer_queue;

};

template<typename T>
struct stOutputstreamInfo {

    std::vector<std::vector<T>> recv_array;
    std::string                 data_id;
};

template<typename T1> using TypeOutStreamDataQueue = std::queue<stOutputstreamInfo<T1>>;
template<typename T1> using TypeOutStreamCollection = std::map<std::string, TypeOutStreamDataQueue<T1>>;


/**
 * MultiNetworkPipeline is a Singleton class that defines the `GetInstance` method
 * that serves as an alternative to constructor and lets clients access the 
 * same instance of this class over and over.
 */
class MultiNetworkPipeline
{
    
    /**
     * The Singleton's constructor/destructor should always be private to
     * prevent direct construction/desctruction calls with the `new`/`delete`
     * operator.
     */
private:
                     
    static MultiNetworkPipeline *   pinstance_;
    static std::mutex               mutex_class_protection;    
    size_t                          hailo_device_found;
    hailo_pcie_device_info_t        pcie_device_info[NUMBER_OF_DEV_SUPPORTED];
    hailo_device                    devices[NUMBER_OF_DEV_SUPPORTED];
    std::vector<stNetworkModelInfo> NetworkModelInfoList;    
    std::vector<stNetworkObjInfo>   NetworkObjList;
    stStreamingConfig               StreamingFlowControlConfig;

    std::mutex                                                          ProtectMutex_MapQueueInferData;
    std::map<std::string, std::queue<stInfererenceDataInfo<uint8_t>>>   MapQueueInferData;

    std::queue<std::string>         msg_to_out_data_process_thread; 
    std::thread                     infer_data_queue_process_thread;
    std::thread                     output_data_queue_process_thread;
    
    std::mutex                          collected_output_stream_mutex; 
    TypeOutStreamCollection<float32_t>  collected_output_stream_32f;
    TypeOutStreamCollection<uint8_t>    collected_output_stream_uint8;


private:

    /**
     * Get get network model info by name
     * this is unprotected (not locked by mutex) version opposed to the public one
     * @param  id_name
     * @return nullptr if not found
     */    
    const stNetworkModelInfo* GetNetworkInfoByName_Unprotect(const std::string &id_name);

    /**
     * Get get network object index by name
     * @param  id_name
     * @return index of found network object, -1 is not found
     */
    int GetNetworkObjIdexByName(const std::string &id_name);
    
    /**
     * Get get network model info index by name
     * @param  id_name
     * @return index of found network model info, -1 is not found
     */    
    int GetNetworkModelInfoIndexByName(const std::string &id_name);

     /**
     * Get current activated network
     * 
     * @return index of found network activated network, -1 is not found
     */    
    int GetActiveNetworkIndex();

    /**
     * Deactivate current network
     * @return SUCCESS, FAILED
     */
    MnpReturnCode DeactivateCurrentNetwork();
    
    /**
     * Activate network by id
     * @param id_name id name of the active network
     * @return SUCCESS, FAILED, NOT_FOUND
     */
    MnpReturnCode ActivateNetworkById(const std::string &id_name);
    
    /**
     * Prepare output stream thread for the active network
     * @param id_name id name of the active network
     */
    void PrepareOutputStreamThreadById(const std::string &id_name);
    
    /**
     * Switch the network by id
     * @param  id_name
     * @return SUCCESS, FAILED, NOT_FOUND
     */
    MnpReturnCode SwitchNetwork(const std::string &id_name);
    
    /**
     * Hailo inference, need to make sure the network object is currently a active network
     * @param NetworkObj the network object
     * @param data the data to infer
     * @return SUCCESS, FAILED
     */
    MnpReturnCode HailoInfer(stNetworkObjInfo &NetworkObj, const std::vector<uint8_t> &data, size_t input_stream_index = 0, std::string data_id = "N/A");
    

    template<typename T>
    MnpReturnCode ReadOutputByIdTyped(const std::string &id_name, TypeOutStreamCollection<T> &output_collected_stream, std::vector<std::vector<T>>& output_buffer, std::string data_id)
    {
        MnpReturnCode RetCode = MnpReturnCode::NO_DATA_AVAILABLE;

        typename TypeOutStreamCollection<T>::iterator iter_find;
        typename TypeOutStreamCollection<T>::iterator iter_end;

        iter_find = output_collected_stream.find(id_name);
        iter_end = output_collected_stream.end();

        if(iter_find != iter_end)
        {
            if (!iter_find->second.empty())
            {   
                stOutputstreamInfo<T> OutputStreamInfo = iter_find->second.front();

                if (OutputStreamInfo.data_id == data_id)
                {
                    //Make sure the provided output_buffer type is same as output transformation type given by applications (eg, stNetworkModelInfo's out_format)
                    if (std::is_same<decltype(output_buffer[0]), decltype(OutputStreamInfo.recv_array[0])>())
                    {
                        output_buffer = OutputStreamInfo.recv_array;
                    }
                    else
                    {
                        DBG_ERROR("output_buffer buffer vercto type is different from output transformation type (eg, stNetworkModelInfo's out_format)");
                    }

                    iter_find->second.pop();
                    RetCode = MnpReturnCode::SUCCESS;
                }
            }
        }

        return RetCode;
    }

    /**
     * Process the infer data in queue
     */
    void ProcessInferDataInQueue();

    /**
     * Process the output data from queue
     */    
    void ProcessOutputDataFromQueue();
    
    /**
     * Process output stream obtained from Hailo
     * @param args pointer to the output stream information
     */
    void ProcessOutputstream(void *args);
    
protected:

    MultiNetworkPipeline();
    
    ~MultiNetworkPipeline();
    
    
public:
    /**
     * MultiNetworkPipeline should not be cloneable.
     */
    MultiNetworkPipeline(MultiNetworkPipeline &other) = delete;
    
    /**
     * MultiNetworkPipeline should not be assignable.
     */
    void operator=(const MultiNetworkPipeline &) = delete;
    
    /**
     * This is the static method that controls the access to the singleton
     * instance. On the first run, it creates a singleton object and places it
     * into the static field. On subsequent runs, it returns the client existing
     * object stored in the static field.
     */
    static MultiNetworkPipeline *GetInstance();

    /**
     * Highly suggested to release all singleton resources, recommended for
     * application exits as well as reset the network to start over.
     * @return SUCCESS, RUNNING_INFERENCE
     */
    MnpReturnCode ReleaseAllResource();
    
    /**
     * Find and initialize hailo device
     * NOTE: Only one device will be initialized and used in current release
     * @return Number of Hailo Device found
     */
    size_t InitializeHailo();
    
    /**
     * Add new network, the network id_name MUST be unique 
     * NOTE: Cannot add network while running inference (eg, still having
     *       inference data in the queue) 
     * @param device_id is reserve for future use
     * @param NewNetworkInfo
     * @return SUCCESS, HAILO_NOT_INITIALIZED, RUNNING_INFERENCE, INVALID_PARAMETER
     */
    MnpReturnCode AddNetwork(uint32_t device_id, const stNetworkModelInfo &NewNetworkInfo );
    
    /**
     * WARNING: HailoRt library up to 2.10.0 (including) will not remove
     *          the network from the context, it get accumulated and will reach
     *          its capacity if too many add/remove is done dynamically.
     *          this will be fixed in 2.11.0 or future release.
     *          It is recommended to simply call ReleaseAllResource to release all
     *          resources and start over adding multiple network.
     * Remove Network from list
     * NOTE: Cannot remove network while running inference (eg, still having
     * inference data in the queue)
     * @param IdName
     * @return SUCCESS, NOT_FOUND, RUNNING_INFERENCE 
     */
    MnpReturnCode RemoveNetwork(const std::string &id_name );
    
    /**
     * WARNING: Calling this method require to switch the network (if current 
     *          active network is not the same) before we can obtain the 
     *          input size of the network stream. This is NOT recommended
     *          to be call often, if require try to call for all added network
     *          before inference.
     * 
     * @param [in]  id_name, the network id name to get the input size
     * @param [out] NetworkInputSize, the input size of the network 
     * @param [in]  input_stream_index, this is the input index base on the order given from AddNetwork
     *                                  NewNetworkInfo.input_order_by_name.
     *                                  If network only contain one input then this can be ignored as by default
     *                                  its set to 0 (first input stream).
     * 
     * @return SUCCESS, NOT_FOUND, FAILED, HAILO_NOT_INITIALIZED
     */
    MnpReturnCode GetNetworkInputSize(const std::string &id_name, size_t &NetworkInputSize, size_t input_stream_index = 0);
    
    /**
     * This is require if output format is not translated (eg, not quantized and UINT8 format) which
     * host is require to transfor it to float 32 for further post processing. 
     * 
     * WARNING: Calling this method require to switch the network (if current 
     *          active network is not the same) before we can obtain the 
     *          network output quantization info from the network stream. This is NOT recommended
     *          to be call often, if require try to call for all added network
     *          before inference.
     * 
     * @param [in]  id_name, the network id name to get the input size
     * @param [out] NetworkQuantInfo, the quatization info in order of output_order_by_name given when adding the network.
     *                                if output_order_by_name is not given then the order is default from network.  
     * @param [in]  get_from_output_stream, the quantization from output or input stream, 
     *                                      true to get from output stream and false to get input stream quantization info, 
     *                                      default to output stream
     * @return SUCCESS, NOT_FOUND, FAILED, HAILO_NOT_INITIALIZED
     */
    MnpReturnCode GetNetworkQuantizationInfo(const std::string &id_name, std::vector<qp_zp_scale_t> &NetworkQuantInfo, bool get_from_output_stream = true);

    /**
     * Infer data to specified network by id_name, data will be queued and is
     * FIFO.
     * 
     * WARNING: When batch size is greater than 1, it is application responsibility that 
     *          data infer is continuos and sequential to multiple of the network batch size
     *          For example, if 2 network, A with batch size 2, and B with batch size 1
     *          make sure when infer network A, data infer is continuos before infer network B
     *          such as
     *              "infer A frame 1" -> "infer A frame 2" -> "infer B frame 1" 
     *          Bad example will be
     *              "infer A frame 1" -> "infer B frame 1" -> "infer A frame 2"
     *          With Bad example above the infer B data frame WILL be dropped! 
     * 
     * @param id_name the network id name to infer
     * @param data the data to infer.
     * @param data_id Optional, but when given, the output of inference ReadOutputByIdWithDataID will return with the same data_id
     * @param input_stream_index, this is the input index base on the order given from AddNetwork
     *                            NewNetworkInfo.input_order_by_name.
     *                            If network only contain one input then this can be ignored as by default
     *                            its set to 0 (first input stream).
     * @return SUCCESS, NOT_FOUND, FAILED, HAILO_NOT_INITIALIZED
     */
    MnpReturnCode Infer(const std::string &id_name, const std::vector<uint8_t> &data, const std::string &data_id = "N/A", size_t input_stream_index = 0);


    /**
     * Get the output data by network name
     * @param id_name
     * @param output_buffer         -   all output layer in a vector/list in sequence given by
     *                                  output_order_by_name from network model info.
     * @param pdata_id [OPTIONAL]   -   if provided the data identification given by Infer will be returned 
     * @return SUCCESS, NOT_FOUND, NO_DATA_AVAILABLE
     */
    MnpReturnCode ReadOutputById(const std::string &id_name, std::vector<std::vector<float32_t>>& output_buffer, std::string data_id = "N/A")
    {    
        MnpReturnCode RetCode = MnpReturnCode::NO_DATA_AVAILABLE;

        if (!hailo_device_found)
            return MnpReturnCode::HAILO_NOT_INITIALIZED;
        
        collected_output_stream_mutex.lock();
        
        RetCode = ReadOutputByIdTyped<float32_t>(id_name, collected_output_stream_32f, output_buffer, data_id);
      
        collected_output_stream_mutex.unlock();       

        return RetCode;    
    }

    MnpReturnCode ReadOutputById(const std::string &id_name, std::vector<std::vector<uint8_t>>& output_buffer, std::string data_id = "N/A")
    {    
        MnpReturnCode RetCode = MnpReturnCode::NO_DATA_AVAILABLE;

        if (!hailo_device_found)
            return MnpReturnCode::HAILO_NOT_INITIALIZED;
        
        collected_output_stream_mutex.lock();
        
        RetCode = ReadOutputByIdTyped<uint8_t>(id_name, collected_output_stream_uint8, output_buffer, data_id);
      
        collected_output_stream_mutex.unlock();       

        return RetCode;    
    }

    /**
     * Get get network model info by name (id)
     * WARNING: Returned pointer should not be kept, it is not guarantee that
     *          its the same address throughout the application life cycle
     *          Also make sure Add or Remove network is not called while
     *          still accessing this returned pointer.
     * @return stNetworkModelInfo pointer to found network, nullptr otherwise
     */
    const stNetworkModelInfo* GetNetworkInfoByName(const std::string &id_name);
    
    /**
     * Get current list of added networks
     * @return list of stNetworkModelInfo
     */
    std::vector<stNetworkModelInfo> GetAllAddedNetworkList();
    
    /**
     * For Multiple Stream (eg more than one video channel) with more than one network inference 
     * it is suggested to try this streaming flow control with following configuration
     * MinInferFrame set to equal to the number of stream channel
     * MinWaitTimeMs set to value between 5 to 10ms
     * NOTE:    FLow control performance depends on the number of network and stream channel, it may or
     *          may not help.
     * @return MnpReturnCode
     */
    MnpReturnCode ConfigStreamingFlowControl(size_t MinInferFrame, size_t MinWaitTimeMs) 
    {
        MnpReturnCode RetCode = MnpReturnCode::SUCCESS;
        
        StreamingFlowControlConfig.MinInferFrame = MinInferFrame;
        StreamingFlowControlConfig.MinInferWaitMs = MinWaitTimeMs;
        
        return RetCode;
    }


    /**
     * This is ONLY available when using Hailo Module (mPCIE and M.2)
     * After calling this API please call StopAndMeasurePower to get the power.
     * @param device_id the device to measure, set to 0 if only one hailo module is available
     * @return SUCCESS, HAILO_NOT_INITIALIZED, INVALID_PARAMETER
     */
    MnpReturnCode StartPowerMeasure(uint32_t device_id);


    /**
     * This is ONLY available when using Hailo Module (mPCIE and M.2)
     * Make sure to call StartPowerMeasure fist before calling this API
     * @param device_id the device to measure, set to 0 if only one hailo module is available
     * @param Powervalue the power value returned if SUCCESS
     * @return SUCCESS, HAILO_NOT_INITIALIZED, INVALID_PARAMETER
     */
    MnpReturnCode StopAndMeasurePower(uint32_t device_id, HailoPower &Powervalue);


};



#endif //_MultiNetworkPipeline_H_

