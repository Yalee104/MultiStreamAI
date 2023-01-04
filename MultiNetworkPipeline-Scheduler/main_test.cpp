#if 1
#include "gtest/gtest.h"


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
#else

#include <vector>
#include "MultiNetworkPipeline-scheduler.hpp"
#include <opencv2/opencv.hpp>
#include <algorithm>
#include <dirent.h>
#include <thread>

#define TOTAL_FRAME_TO_INFER    (1500)

TimerMs ElapsedTime;

void Inference_thread(std::string NetworkName)
{
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();

    size_t ReturnedInputSizeInByte = 0;
    pHailoPipeline->GetNetworkInputSize(NetworkName, ReturnedInputSizeInByte, 0);
    std::vector<uint8_t> TestInput(ReturnedInputSizeInByte, 0);

    ElapsedTime.reset();

    for (size_t i = 0; i < TOTAL_FRAME_TO_INFER; i++) {
        pHailoPipeline->Infer(NetworkName, TestInput, "0", 0);
    }
}

void Prediction_thread(std::string NetworkName)
{
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();

    std::vector<std::vector<float32_t>> output_buffer;

    pHailoPipeline->InitializeOutputBuffer<float32_t>(NetworkName, output_buffer, "0");

    for (size_t i = 0; i < TOTAL_FRAME_TO_INFER; i++) {
        pHailoPipeline->ReadOutputById(NetworkName, output_buffer, "0");
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
      Yolov5Network.id_name = "yolov5";
      Yolov5Network.hef_path = "Test/Network/yolov5m.hef";
      Yolov5Network.output_order_by_name.clear();
      Yolov5Network.batch_size = 1;
      Yolov5Network.in_quantized = false;
      Yolov5Network.in_format = HAILO_FORMAT_TYPE_UINT8;
      Yolov5Network.out_quantized = false;
      Yolov5Network.out_format = HAILO_FORMAT_TYPE_FLOAT32;

      //std::vector<std::string> Yolov5mOutputOrderByName{"conv97", "conv107", "conv87"};
      //Yolov5Network.output_order_by_name = Yolov5mOutputOrderByName;

      pHailoPipeline->AddNetwork(0, Yolov5Network, "0");

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


#endif
