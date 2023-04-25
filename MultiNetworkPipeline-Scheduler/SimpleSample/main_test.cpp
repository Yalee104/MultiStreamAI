
#include <vector>
#include "../MultiNetworkPipeline-scheduler.hpp"
#include <thread>

#include <unistd.h>

#define TOTAL_FRAME_TO_INFER    (1000)


unsigned long long getTotalSystemMemory()
{
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}

#include <ios>
#include <iostream>
#include <fstream>
#include <string>

#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

std::string timestamp()
{
    using namespace std::chrono;
    using clock = system_clock;
    
    const auto current_time_point {clock::now()};
    const auto current_time {clock::to_time_t (current_time_point)};
    const auto current_localtime {*std::localtime (&current_time)};
    const auto current_time_since_epoch {current_time_point.time_since_epoch()};
    const auto current_milliseconds {duration_cast<milliseconds> (current_time_since_epoch).count() % 1000};
    
    std::ostringstream stream;
    stream << std::put_time (&current_localtime, "%T") << "." << std::setw (3) << std::setfill ('0') << current_milliseconds;
    return stream.str();
}

void process_mem_usage(double& vm_usage, double& resident_set)
{
   using std::ios_base;
   using std::ifstream;
   using std::string;

   vm_usage     = 0.0;
   resident_set = 0.0;

   // 'file' stat seems to give the most reliable results
   //
   ifstream stat_stream("/proc/self/stat",ios_base::in);

   // dummy vars for leading entries in stat that we don't care about
   //
   string pid, comm, state, ppid, pgrp, session, tty_nr;
   string tpgid, flags, minflt, cminflt, majflt, cmajflt;
   string utime, stime, cutime, cstime, priority, nice;
   string O, itrealvalue, starttime;

   // the two fields we want
   //
   unsigned long vsize;
   long rss;

   stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
               >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
               >> utime >> stime >> cutime >> cstime >> priority >> nice
               >> O >> itrealvalue >> starttime >> vsize >> rss; // don't care about the rest

   stat_stream.close();

   long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
   vm_usage     = vsize / 1024.0;
   resident_set = rss * page_size_kb;
}


TimerMs ElapsedTime;

void Inference_thread(std::string NetworkName)
{
    std::cout << "Inference_Thread Started" << std::endl;
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();

    size_t ReturnedInputSizeInByte = 0;
    pHailoPipeline->GetNetworkInputSize(NetworkName, ReturnedInputSizeInByte, 0);
    std::vector<uint8_t> TestInput(ReturnedInputSizeInByte, 0);

    std::cout << "ReturnedInputSizeInByte= " << ReturnedInputSizeInByte << std::endl;
    
    ElapsedTime.reset();

    for (size_t i = 0; i < TOTAL_FRAME_TO_INFER; i++) {
        pHailoPipeline->Infer(NetworkName, TestInput, "0", 0);
        //std::cout << "Timestamp:" << timestamp() << ", Frame " << i << " Infered" << std::endl;
    }
}

void Prediction_thread(std::string NetworkName)
{
    std::cout << "Prediction_Thread Started" << std::endl;

    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();

    std::vector<std::vector<float32_t>> output_buffer;

    pHailoPipeline->InitializeOutputBuffer<float32_t>(NetworkName, output_buffer, "0");

    std::cout << "output_buffer list " << output_buffer.size() << std::endl;
        
    for (size_t i = 0; i < output_buffer.size(); i++) {
    	std::cout << "output_buffer[" << i << "] size is " << output_buffer[i].size() << std::endl;    	
    }

    /*
    std::cout << "getTotalSystemMemory= " << getTotalSystemMemory() << std::endl;
    double MemA, MemB;
    process_mem_usage(MemA, MemB);
    std::cout << "MEMORY = " << MemA << ", " << MemB << std::endl;
    */

    for (size_t i = 0; i < TOTAL_FRAME_TO_INFER; i++) {
        pHailoPipeline->ReadOutputById(NetworkName, output_buffer, "0");
        //std::cout << "Timestamp:" << timestamp() << ", Frame " << i << " output readed with total output list =" << output_buffer.size() << std::endl;

    }

    double TotalTimeElapsed = ElapsedTime.getElapsedInMs();
    std::cout << "Infer of " << TOTAL_FRAME_TO_INFER << " images took " << TotalTimeElapsed/1000.0 << " seconds" << std::endl;
    std::cout << "FPS = " << ((double)TOTAL_FRAME_TO_INFER)/(TotalTimeElapsed/1000.0) << std::endl;
}


void start() {

      MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
      pHailoPipeline->InitializeHailo();

      //Add Network
      stNetworkModelInfo Yolov5Network;
      Yolov5Network.id_name = "yolov5_ID1"; //WARNING: Each Network added to different stream MUST be given a unique id name
      Yolov5Network.hef_path = "../Test/Network/yolov5m.hef";
      Yolov5Network.output_order_by_name.clear();
      Yolov5Network.batch_size = 1;
      Yolov5Network.in_quantized = false;
      Yolov5Network.in_format = HAILO_FORMAT_TYPE_UINT8;
      Yolov5Network.out_quantized = true;
      Yolov5Network.out_format = HAILO_FORMAT_TYPE_UINT8;

      pHailoPipeline->AddNetwork(0, Yolov5Network, "0");

      sleep(1);
            
      std::cout << "Please Wait, running total of " << TOTAL_FRAME_TO_INFER << " frames...." << std::endl;
      std::thread t1(Inference_thread, Yolov5Network.id_name);

      std::thread t2(Prediction_thread, Yolov5Network.id_name);

      t1.join();

      t2.join();

      pHailoPipeline->ReleaseAllResource();

}

int main(int argc, char **argv) {
  
    start();

}


