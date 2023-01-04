/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   MNP_Unittest.cpp
 * Author: Aaron
 * 
 * Created on July 5, 2021, 11:32 AM
 */


#include <vector>
#include "../MultiNetworkPipeline-scheduler.hpp"
#include "gtest/gtest.h"
#include <opencv2/opencv.hpp>
#include <algorithm>
#include <dirent.h>

#define TIME_OUT_SEC    (10)

class MNPTest1 : public testing::Test {
 protected:
  // Per-test-suite set-up.
  // Called before the first test in this test suite.
  // Can be omitted if not needed.
  static void SetUpTestSuite() {
      //TODO: uncomment below code once we resolve the memory limitation 
      //      issue when adding multiple network (big size such as yolov4)
      //MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
      //ASSERT_GT(pHailoPipeline->InitializeHailo(), 0);      
  }

  // Per-test-suite tear-down.
  // Called after the last test in this test suite.
  // Can be omitted if not needed.
  static void TearDownTestSuite() {
      
      //TODO: uncomment below code once we resolve the memory limitation 
      //      issue when adding multiple network (big size such as yolov4)
      //MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
      //EXPECT_EQ(pHailoPipeline->ReleaseAllResource(), MnpReturnCode::SUCCESS);
  }

  // You can define per-test set-up logic as usual.
  void SetUp() override {
      //TODO: remove below code once we resolve the memory limitation 
      //      issue when adding multiple network (big size such as yolov4)
      MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
      ASSERT_GT(pHailoPipeline->InitializeHailo(), 0);

  }

  // You can define per-test tear-down logic as usual.
  void TearDown() override {
      
      //TODO: remove below code once we resolve the memory limitation 
      //      issue when adding multiple network (big size such as yolov4)
      MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
      EXPECT_EQ(pHailoPipeline->ReleaseAllResource(), MnpReturnCode::SUCCESS);
  }

};

std::vector<std::string> GetRecords(std::string dirPath)
{
    std::vector<std::string> files;
    struct dirent *entry;
	DIR *dir = opendir(dirPath.c_str());

	if (dir == NULL) 
	{
	  return files;
	}
	while ((entry = readdir(dir)) != NULL) 
	{
        if ((strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0))
		    files.push_back(entry->d_name);
	}
	closedir(dir);
	
	return files;
}


TEST_F(MNPTest1, MNP1CheckInputsize) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);

    //Add Network
    stNetworkModelInfo Yolov4Network;
    Yolov4Network.id_name = "yolov4";
    Yolov4Network.hef_path = "Network/yolov4.hef";
    Yolov4Network.output_order_by_name.clear();        
    Yolov4Network.batch_size = 1;
    Yolov4Network.in_quantized = false;
    Yolov4Network.in_format = HAILO_FORMAT_TYPE_UINT8;
    Yolov4Network.out_quantized = false;
    Yolov4Network.out_format = HAILO_FORMAT_TYPE_FLOAT32;    
    
    std::vector<std::string> Yolov4OutputOrderByName{"conv133_centers", "conv133_scales", "conv133_obj", "conv133_probs",
                         "conv126_centers", "conv126_scales", "conv126_obj", "conv126_probs",
                         "conv118_centers", "conv118_scales", "conv118_obj", "conv118_probs" };
    Yolov4Network.output_order_by_name = Yolov4OutputOrderByName;

    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Yolov4Network, "0"), MnpReturnCode::SUCCESS);

    size_t ExpectedInputSizeInByte = 512*512*3; //Yolov4 input size
    size_t ReturnedInputSizeInByte = 0;
    pHailoPipeline->GetNetworkInputSize(Yolov4Network.id_name, ReturnedInputSizeInByte);
    EXPECT_EQ(ExpectedInputSizeInByte, ReturnedInputSizeInByte);

}



TEST_F(MNPTest1, MNP1CheckQuantizationInfo) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);

    //Add Network
    stNetworkModelInfo Yolov4Network;
    Yolov4Network.id_name = "yolov4";
    Yolov4Network.hef_path = "Network/yolov4.hef";
    Yolov4Network.output_order_by_name.clear();        
    Yolov4Network.batch_size = 1;
    Yolov4Network.in_quantized = false;
    Yolov4Network.in_format = HAILO_FORMAT_TYPE_UINT8;
    Yolov4Network.out_quantized = true;
    Yolov4Network.out_format = HAILO_FORMAT_TYPE_UINT8;    
    
    std::vector<std::string> Yolov4OutputOrderByName{"conv133_centers", "conv133_scales", "conv133_obj", "conv133_probs",
                         "conv126_centers", "conv126_scales", "conv126_obj", "conv126_probs",
                         "conv118_centers", "conv118_scales", "conv118_obj", "conv118_probs" };
    Yolov4Network.output_order_by_name = Yolov4OutputOrderByName;
    
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Yolov4Network, "0"), MnpReturnCode::SUCCESS);

    std::vector<qp_zp_scale_t> quantizationInfoList;
    EXPECT_EQ(pHailoPipeline->GetNetworkQuantizationInfo(Yolov4Network.id_name, quantizationInfoList), MnpReturnCode::SUCCESS);

    EXPECT_EQ(quantizationInfoList.size(), Yolov4OutputOrderByName.size());

    /* TODO: Get the actual output quant info list of this hef and compare each individually
    for (qp_zp_scale_t quantInfo : quantizationInfoList) {
         std::cout << "scale: " << quantInfo.qp_scale << std::endl;
         std::cout << "zp: " << quantInfo.qp_zp << std::endl;

    }
    */

}


TEST_F(MNPTest1, MNP3OneNetworkinferAndRemove) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);

    //Add Network
    stNetworkModelInfo Yolov4Network;
    Yolov4Network.id_name = "yolov4";
    Yolov4Network.hef_path = "Network/yolov4.hef";
    Yolov4Network.output_order_by_name.clear();        
    Yolov4Network.batch_size = 1;
    Yolov4Network.in_quantized = false;
    Yolov4Network.in_format = HAILO_FORMAT_TYPE_UINT8;
    Yolov4Network.out_quantized = false;
    Yolov4Network.out_format = HAILO_FORMAT_TYPE_FLOAT32;    
    
    std::vector<std::string> Yolov4OutputOrderByName{"conv133_centers", "conv133_scales", "conv133_obj", "conv133_probs",
                         "conv126_centers", "conv126_scales", "conv126_obj", "conv126_probs",
                         "conv118_centers", "conv118_scales", "conv118_obj", "conv118_probs" };
    Yolov4Network.output_order_by_name = Yolov4OutputOrderByName;
    
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Yolov4Network, "0"), MnpReturnCode::SUCCESS);


    std::vector<int> Yolov4OutputSizeInOrder{1536,1536,768,61440,
                                             6144,6144,3072,245760,
                                             24576,24576,12288,983040};    
    size_t InputSize = 512*512*3; //Yolov4 input size
    std::vector<uint8_t> TestInput(InputSize, 0);
    EXPECT_EQ(pHailoPipeline->Infer(Yolov4Network.id_name, TestInput, "0", 0),
                                    MnpReturnCode::SUCCESS);       
    
    
    std::vector<std::vector<float32_t>> output_buffer;

    pHailoPipeline->InitializeOutputBuffer<float32_t>(Yolov4Network.id_name, output_buffer, "0");

    MnpReturnCode status = pHailoPipeline->ReadOutputById(Yolov4Network.id_name, output_buffer, "0");
    ASSERT_EQ(status, MnpReturnCode::SUCCESS);
    
    
    EXPECT_EQ(output_buffer.size(), Yolov4OutputSizeInOrder.size());
        
    for (size_t i = 0; i < output_buffer.size(); i++)
    {
        EXPECT_EQ(output_buffer[i].size(), Yolov4OutputSizeInOrder[i]);
    }
    
    
}


TEST_F(MNPTest1, MNP5MultipleNetworkInfer) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);
    
    // 
    // Infer Yolov4 -> Yolov5m -> Yolov4
    // 
    //    
    
    //Add Network - Yolov4
    stNetworkModelInfo Yolov4Network;
    Yolov4Network.id_name = "yolov4";
    Yolov4Network.hef_path = "Network/yolov4.hef";
    Yolov4Network.output_order_by_name.clear();        
    Yolov4Network.batch_size = 1;
    Yolov4Network.in_quantized = false;
    Yolov4Network.in_format = HAILO_FORMAT_TYPE_UINT8;
    Yolov4Network.out_quantized = false;
    Yolov4Network.out_format = HAILO_FORMAT_TYPE_FLOAT32;    
    
    std::vector<std::string> Yolov4OutputOrderByName{"conv133_centers", "conv133_scales", "conv133_obj", "conv133_probs",
                                                     "conv126_centers", "conv126_scales", "conv126_obj", "conv126_probs",
                                                     "conv118_centers", "conv118_scales", "conv118_obj", "conv118_probs" };
    Yolov4Network.output_order_by_name = Yolov4OutputOrderByName;
    
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Yolov4Network), MnpReturnCode::SUCCESS);
    size_t Yolov4InputSize = 512*512*3; //Yolov4 input size
    std::vector<int> Yolov4OutputSizeInOrder{1536,1536,768,61440,
                                             6144,6144,3072,245760,
                                             24576,24576,12288,983040};    
    
                                                                                          
    //Add Network - Yolov5m
    stNetworkModelInfo Yolov5mNetwork;
    Yolov5mNetwork.id_name = "yolov5m";
    Yolov5mNetwork.hef_path = "Network/yolov5m.hef";
    Yolov5mNetwork.output_order_by_name.clear();        
    Yolov5mNetwork.batch_size = 1;
    Yolov5mNetwork.in_quantized = false;
    Yolov5mNetwork.in_format = HAILO_FORMAT_TYPE_UINT8;
    Yolov5mNetwork.out_quantized = false;
    Yolov5mNetwork.out_format = HAILO_FORMAT_TYPE_FLOAT32;    
    
    std::vector<std::string> Yolov5mOutputOrderByName{"conv107", "conv97", "conv87"};
    Yolov5mNetwork.output_order_by_name = Yolov5mOutputOrderByName;
    
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Yolov5mNetwork), MnpReturnCode::SUCCESS);
    size_t Yolov5mInputSize = 640*640*3; //Yolov5m input size
    std::vector<int> Yolov5mOutputSizeInOrder{  20*20*255,
                                                40*40*255,
                                                80*80*255}; 
                                             
                                             
    //Yolov4 Infer up to NumberOfInference  
    {
        std::vector<uint8_t> TestInput(Yolov4InputSize, 0);
        size_t NumberOfInference = 3;
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            EXPECT_EQ(pHailoPipeline->Infer(Yolov4Network.id_name, TestInput), 
                                            MnpReturnCode::SUCCESS);
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }

        std::vector<std::vector<float32_t>> output_buffer;
        pHailoPipeline->InitializeOutputBuffer<float32_t>(Yolov4Network.id_name, output_buffer);

        //Expect to get up to NumberOfInference
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            MnpReturnCode status = pHailoPipeline->ReadOutputById(Yolov4Network.id_name, output_buffer);
            ASSERT_EQ(status, MnpReturnCode::SUCCESS);

            EXPECT_EQ(output_buffer.size(), Yolov4OutputSizeInOrder.size());

            for (size_t i = 0; i < output_buffer.size(); i++)
            {
                EXPECT_EQ(output_buffer[i].size(), Yolov4OutputSizeInOrder[i]);
            }

        }
    }
    
    //Yolov5m Infer up to NumberOfInference  
    {
        std::vector<uint8_t> TestInput(Yolov5mInputSize, 0);
        size_t NumberOfInference = 5;
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            EXPECT_EQ(pHailoPipeline->Infer(Yolov5mNetwork.id_name, TestInput), 
                                            MnpReturnCode::SUCCESS);
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }

        //Expect to get up to NumberOfInference
        std::vector<std::vector<float32_t>> output_buffer;
        pHailoPipeline->InitializeOutputBuffer<float32_t>(Yolov5mNetwork.id_name, output_buffer);

        for (size_t i = 0; i < NumberOfInference; i++)
        {
            MnpReturnCode status = pHailoPipeline->ReadOutputById(Yolov5mNetwork.id_name, output_buffer);
            ASSERT_EQ(status, MnpReturnCode::SUCCESS);

            EXPECT_EQ(output_buffer.size(), Yolov5mOutputSizeInOrder.size());

            for (size_t i = 0; i < output_buffer.size(); i++)
            {
                EXPECT_EQ(output_buffer[i].size(), Yolov5mOutputSizeInOrder[i]);
            }
        }
    }
    
    //Yolov4 Infer up to NumberOfInference  
    {
        std::vector<uint8_t> TestInput(Yolov4InputSize, 0);
        size_t NumberOfInference = 3;
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            EXPECT_EQ(pHailoPipeline->Infer(Yolov4Network.id_name, TestInput), 
                                            MnpReturnCode::SUCCESS);
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }

        //Expect to get up to NumberOfInference
        std::vector<std::vector<float32_t>> output_buffer;
        pHailoPipeline->InitializeOutputBuffer<float32_t>(Yolov4Network.id_name, output_buffer);

        for (size_t i = 0; i < NumberOfInference; i++)
        {
            MnpReturnCode status = pHailoPipeline->ReadOutputById(Yolov4Network.id_name, output_buffer);
            ASSERT_EQ(status, MnpReturnCode::SUCCESS);

            EXPECT_EQ(output_buffer.size(), Yolov4OutputSizeInOrder.size());

            for (size_t i = 0; i < output_buffer.size(); i++)
            {
                EXPECT_EQ(output_buffer[i].size(), Yolov4OutputSizeInOrder[i]);
            }

        }
    }
    
}


TEST_F(MNPTest1, MNP8MultipleInferwithImageAndRemove) {

    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);

    //Add Network
    stNetworkModelInfo Network;
    Network.id_name = "resnet50";
    Network.hef_path = "Network/resnet_v1_50.hef";
    Network.output_order_by_name.clear();
    Network.batch_size = 1;
    Network.in_quantized = false;
    Network.in_format = HAILO_FORMAT_TYPE_UINT8;
    Network.out_quantized = false;
    Network.out_format = HAILO_FORMAT_TYPE_FLOAT32;

    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Network), MnpReturnCode::SUCCESS);



    //Get all files under Images/
    std::string ImagePath("Images/");
    std::vector<std::string> fileList = GetRecords(ImagePath);
    for (auto file : fileList)
    {
        size_t InputSize = 224*224*3;
        std::vector<uint8_t> ImageInput(InputSize, 0);
        cv::Mat Imageframe(224, 224, CV_8UC3);;

        //std::cout << ImagePath + file << std::endl;
        Imageframe = cv::imread(ImagePath + file);

        int totalsz = Imageframe.dataend-Imageframe.datastart;
        ImageInput.assign(Imageframe.datastart, Imageframe.datastart + totalsz);


        EXPECT_EQ(pHailoPipeline->Infer(Network.id_name, ImageInput),
                                        MnpReturnCode::SUCCESS);

        std::this_thread::sleep_for(std::chrono::microseconds(10));

    }

    for (auto file : fileList)
    {
        std::vector<std::vector<float32_t>> output_buffer;
        pHailoPipeline->InitializeOutputBuffer<float32_t>(Network.id_name, output_buffer);

        MnpReturnCode status = pHailoPipeline->ReadOutputById(Network.id_name, output_buffer);
        ASSERT_EQ(status, MnpReturnCode::SUCCESS);

        //We expect one array of 1000 element in this case (resnet50)
        EXPECT_EQ(output_buffer.size(), 1);
        EXPECT_EQ(output_buffer[0].size(), 1000);

        //Get the highest provability
        int maxElementIndex = std::max_element(output_buffer[0].begin(),
                                            output_buffer[0].end()) - output_buffer[0].begin();

        //std::cout << file << std::endl;
        std::size_t classPos = file.rfind('_');
        //std::cout << file.substr(0, classPos) << std::endl;

        //We expect the same class index from the prefix of the file
        EXPECT_EQ(maxElementIndex, std::stoi( file.substr(0, classPos)));
        std::cout << "ClassIndex = " << maxElementIndex << ", Provability % = " << output_buffer[0][maxElementIndex] * 100 << std::endl;

    }

}



TEST_F(MNPTest1, MNP10MultipleInferwithImageOnSameChannel) {

    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);
        
    //Add Network
    stNetworkModelInfo Network;
    Network.id_name = "resnet50";
    Network.hef_path = "Network/resnet_v1_50.hef";
    Network.output_order_by_name.clear();        
    Network.batch_size = 1;
    Network.in_quantized = false;
    Network.in_format = HAILO_FORMAT_TYPE_UINT8;
    Network.out_quantized = false;
    Network.out_format = HAILO_FORMAT_TYPE_FLOAT32;    
       
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Network), MnpReturnCode::SUCCESS);

    //Add Network (dummy)
    stNetworkModelInfo NetworkDummy;
    NetworkDummy.id_name = "yolov4";
    NetworkDummy.hef_path = "Network/yolov4.hef";
    NetworkDummy.output_order_by_name.clear();        
    NetworkDummy.batch_size = 1;
    size_t NetworkDummyInputSize = 512*512*3; //Yolov4 input size
    std::vector<uint8_t> DummyNetworkInput(NetworkDummyInputSize, 0);
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, NetworkDummy), MnpReturnCode::SUCCESS);
    
    //Get all files under Images/
    std::string ImagePath("Images/"); 
    std::vector<std::string> fileList = GetRecords(ImagePath);
    for (auto file : fileList)
    {
        size_t InputSize = 224*224*3;
        std::vector<uint8_t> ImageInput(InputSize, 0);
        cv::Mat Imageframe(224, 224, CV_8UC3);;
       
        //std::cout << ImagePath + file << std::endl;
        Imageframe = cv::imread(ImagePath + file);       
        
        int totalsz = Imageframe.dataend-Imageframe.datastart;
        ImageInput.assign(Imageframe.datastart, Imageframe.datastart + totalsz);

        //Infer Dummy network first on every restnet imager with data id,
        //this is to make sure the module works as expected in which 
        //eahc network inference is independent.  
        EXPECT_EQ(pHailoPipeline->Infer(NetworkDummy.id_name, DummyNetworkInput),
                                        MnpReturnCode::SUCCESS);   

        std::this_thread::sleep_for(std::chrono::microseconds(10));

        //Infer with data id (as name from file)
        EXPECT_EQ(pHailoPipeline->Infer(Network.id_name, ImageInput), MnpReturnCode::SUCCESS);


        std::this_thread::sleep_for(std::chrono::microseconds(10));

    }

    //Although above should suffice but does not hurt to make it a bit more complex for this test     
    for (int i = 0; i < 10; i++)
    {
        //Infer Dummy network
        EXPECT_EQ(pHailoPipeline->Infer(NetworkDummy.id_name, DummyNetworkInput),
                                        MnpReturnCode::SUCCESS);
                                        
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }

    for (auto file : fileList)
    {
        std::vector<std::vector<float32_t>> output_buffer;
        pHailoPipeline->InitializeOutputBuffer<float32_t>(Network.id_name, output_buffer);

        MnpReturnCode status = pHailoPipeline->ReadOutputById(Network.id_name, output_buffer);
        ASSERT_EQ(status, MnpReturnCode::SUCCESS);
        
        //We expect one array of 1000 element in this case (resnet50)
        EXPECT_EQ(output_buffer.size(), 1);
        EXPECT_EQ(output_buffer[0].size(), 1000);

        //Get the highest provability
        int maxElementIndex = std::max_element(output_buffer[0].begin(),
                                            output_buffer[0].end()) - output_buffer[0].begin();
            
        //std::cout << file << std::endl;
        std::size_t classPos = file.rfind('_');
        //std::cout << file.substr(0, classPos) << std::endl;

        //We expect the same class index from the prefix of the file
        EXPECT_EQ(maxElementIndex, std::stoi( file.substr(0, classPos)));    
        std::cout << "ClassIndex = " << maxElementIndex << ", Provability % = " << output_buffer[0][maxElementIndex] * 100 << std::endl;
    
    }   

}



TEST_F(MNPTest1, MNP12OneNetworkinferNoTransformAndRemove) {

    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);

    //Add Network
    stNetworkModelInfo Yolov4Network;
    Yolov4Network.id_name = "yolov4";
    Yolov4Network.hef_path = "Network/yolov4.hef";
    Yolov4Network.output_order_by_name.clear();
    Yolov4Network.batch_size = 1;
    Yolov4Network.in_quantized = false;
    Yolov4Network.in_format = HAILO_FORMAT_TYPE_UINT8;
    Yolov4Network.out_quantized = true;
    Yolov4Network.out_format = HAILO_FORMAT_TYPE_UINT8;

    std::vector<std::string> Yolov4OutputOrderByName{"conv133_centers", "conv133_scales", "conv133_obj", "conv133_probs",
                         "conv126_centers", "conv126_scales", "conv126_obj", "conv126_probs",
                         "conv118_centers", "conv118_scales", "conv118_obj", "conv118_probs" };
    Yolov4Network.output_order_by_name = Yolov4OutputOrderByName;

    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Yolov4Network), MnpReturnCode::SUCCESS);

    std::vector<int> Yolov4OutputSizeInOrder{1536,1536,768,61440,
                                             6144,6144,3072,245760,
                                             24576,24576,12288,983040};
    size_t InputSize = 512*512*3; //Yolov4 input size
    std::vector<uint8_t> TestInput(InputSize, 0);
    EXPECT_EQ(pHailoPipeline->Infer(Yolov4Network.id_name, TestInput),
                                    MnpReturnCode::SUCCESS);

    std::vector<std::vector<uint8_t>> output_buffer;
    pHailoPipeline->InitializeOutputBuffer<uint8_t>(Yolov4Network.id_name, output_buffer);

    MnpReturnCode status = pHailoPipeline->ReadOutputById(Yolov4Network.id_name, output_buffer);
    ASSERT_EQ(status, MnpReturnCode::SUCCESS);

    EXPECT_EQ(output_buffer.size(), Yolov4OutputSizeInOrder.size());

    for (size_t i = 0; i < output_buffer.size(); i++)
    {
        EXPECT_EQ(output_buffer[i].size(), Yolov4OutputSizeInOrder[i]);
    }

}


TEST_F(MNPTest1, MNP20MultipleNetworkCrossAddInfer) {

    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);

    // Add Yolov4 -> Infer Yolov4 -> Add Yolov5 -> Infer/GetPrediction yolov5 -> GetPrediction yolov4.

    //Add Network - Yolov4
    stNetworkModelInfo Yolov4Network;
    Yolov4Network.id_name = "yolov4";
    Yolov4Network.hef_path = "Network/yolov4.hef";
    Yolov4Network.output_order_by_name.clear();
    Yolov4Network.batch_size = 1;
    Yolov4Network.in_quantized = false;
    Yolov4Network.in_format = HAILO_FORMAT_TYPE_UINT8;
    Yolov4Network.out_quantized = false;
    Yolov4Network.out_format = HAILO_FORMAT_TYPE_FLOAT32;

    std::vector<std::string> Yolov4OutputOrderByName{"conv133_centers", "conv133_scales", "conv133_obj", "conv133_probs",
                                                     "conv126_centers", "conv126_scales", "conv126_obj", "conv126_probs",
                                                     "conv118_centers", "conv118_scales", "conv118_obj", "conv118_probs" };
    Yolov4Network.output_order_by_name = Yolov4OutputOrderByName;

    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Yolov4Network), MnpReturnCode::SUCCESS);
    size_t Yolov4InputSize = 512*512*3; //Yolov4 input size
    std::vector<int> Yolov4OutputSizeInOrder{1536,1536,768,61440,
                                             6144,6144,3072,245760,
                                             24576,24576,12288,983040};


    //Yolov4 Infer up to NumberOfInference
    size_t NumberOfInferenceYolov4 = 10;
    {
        std::vector<uint8_t> TestInput(Yolov4InputSize, 0);

        for (size_t i = 0; i < NumberOfInferenceYolov4; i++)
        {
            EXPECT_EQ(pHailoPipeline->Infer(Yolov4Network.id_name, TestInput),
                                            MnpReturnCode::SUCCESS);
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }

    //Add Network - Yolov5m
    stNetworkModelInfo Yolov5mNetwork;
    Yolov5mNetwork.id_name = "yolov5m";
    Yolov5mNetwork.hef_path = "Network/yolov5m.hef";
    Yolov5mNetwork.output_order_by_name.clear();
    Yolov5mNetwork.batch_size = 1;
    Yolov5mNetwork.in_quantized = false;
    Yolov5mNetwork.in_format = HAILO_FORMAT_TYPE_UINT8;
    Yolov5mNetwork.out_quantized = false;
    Yolov5mNetwork.out_format = HAILO_FORMAT_TYPE_FLOAT32;

    std::vector<std::string> Yolov5mOutputOrderByName{"conv107", "conv97", "conv87"};
    Yolov5mNetwork.output_order_by_name = Yolov5mOutputOrderByName;

    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Yolov5mNetwork), MnpReturnCode::SUCCESS);
    size_t Yolov5mInputSize = 640*640*3; //Yolov5m input size
    std::vector<int> Yolov5mOutputSizeInOrder{  20*20*255,
                                                40*40*255,
                                                80*80*255};


    //Yolov5m Infer up to NumberOfInference
    {
        std::vector<uint8_t> TestInput(Yolov5mInputSize, 0);
        size_t NumberOfInference = 5;
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            EXPECT_EQ(pHailoPipeline->Infer(Yolov5mNetwork.id_name, TestInput),
                                            MnpReturnCode::SUCCESS);
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }

        //Expect to get up to NumberOfInference
        std::vector<std::vector<float32_t>> output_buffer;
        pHailoPipeline->InitializeOutputBuffer<float32_t>(Yolov5mNetwork.id_name, output_buffer);

        for (size_t i = 0; i < NumberOfInference; i++)
        {
            MnpReturnCode status = pHailoPipeline->ReadOutputById(Yolov5mNetwork.id_name, output_buffer);
            ASSERT_EQ(status, MnpReturnCode::SUCCESS);

            EXPECT_EQ(output_buffer.size(), Yolov5mOutputSizeInOrder.size());

            for (size_t i = 0; i < output_buffer.size(); i++)
            {
                EXPECT_EQ(output_buffer[i].size(), Yolov5mOutputSizeInOrder[i]);
            }
        }
    }

    //Now get the yolov4 output prediction
    {
        std::vector<std::vector<float32_t>> output_buffer;
        pHailoPipeline->InitializeOutputBuffer<float32_t>(Yolov4Network.id_name, output_buffer);

        for (size_t i = 0; i < NumberOfInferenceYolov4; i++)
        {
            MnpReturnCode status = pHailoPipeline->ReadOutputById(Yolov4Network.id_name, output_buffer);
            ASSERT_EQ(status, MnpReturnCode::SUCCESS);

            EXPECT_EQ(output_buffer.size(), Yolov4OutputSizeInOrder.size());

            for (size_t i = 0; i < output_buffer.size(); i++)
            {
                EXPECT_EQ(output_buffer[i].size(), Yolov4OutputSizeInOrder[i]);
            }

        }
    }
}


TEST_F(MNPTest1, MNPX_OneNetwork2InputInferTest) {

    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);

    //Add Network
    stNetworkModelInfo Yolov5sLanenet;
    Yolov5sLanenet.id_name = "yolov5slanenet";
    Yolov5sLanenet.hef_path = "Network/joined_yolov5s_LPRNet_single_context.hef";
    Yolov5sLanenet.output_order_by_name.clear();
    Yolov5sLanenet.batch_size = 1;
    Yolov5sLanenet.in_quantized = false;
    Yolov5sLanenet.in_format = HAILO_FORMAT_TYPE_UINT8;
    Yolov5sLanenet.out_quantized = false;
    Yolov5sLanenet.out_format = HAILO_FORMAT_TYPE_FLOAT32;


    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Yolov5sLanenet, "0"), MnpReturnCode::SUCCESS);

    size_t ReturnedInput1SizeInByte = 0;
    pHailoPipeline->GetNetworkInputSize(Yolov5sLanenet.id_name, ReturnedInput1SizeInByte, 0);
    EXPECT_EQ(ReturnedInput1SizeInByte, 96*24*3);


    size_t ReturnedInput2SizeInByte = 0;
    pHailoPipeline->GetNetworkInputSize(Yolov5sLanenet.id_name, ReturnedInput2SizeInByte, 1);
    EXPECT_EQ(ReturnedInput2SizeInByte, 608*608*3);

    std::vector<uint8_t> TestInput1(ReturnedInput1SizeInByte, 0);
    EXPECT_EQ(pHailoPipeline->Infer(Yolov5sLanenet.id_name, TestInput1, "0", 0),
                                    MnpReturnCode::SUCCESS);


    std::vector<uint8_t> TestInput2(ReturnedInput2SizeInByte, 0);
    EXPECT_EQ(pHailoPipeline->Infer(Yolov5sLanenet.id_name, TestInput2, "0", 1),
                                    MnpReturnCode::SUCCESS);


    std::vector<std::vector<float32_t>> output_buffer;

    pHailoPipeline->InitializeOutputBuffer<float32_t>(Yolov5sLanenet.id_name, output_buffer, "0");

    MnpReturnCode status = pHailoPipeline->ReadOutputById(Yolov5sLanenet.id_name, output_buffer, "0");
    ASSERT_EQ(status, MnpReturnCode::SUCCESS);

    //We expect 6 outputs with this network
    EXPECT_EQ(output_buffer.size(), 6);

}



TEST_F(MNPTest1, MNPX_TwoNetworkGroup2InputInferTest) {

    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);

    //Add Network
    stNetworkModelInfo Yolov4Lanenet;
    Yolov4Lanenet.id_name = "yolov4lanenet_sc";
    Yolov4Lanenet.hef_path = "Network/joined_yolov4_tiny_lanenet_single_context.hef";
    Yolov4Lanenet.output_order_by_name.clear();
    Yolov4Lanenet.batch_size = 1;
    Yolov4Lanenet.in_quantized = false;
    Yolov4Lanenet.in_format = HAILO_FORMAT_TYPE_UINT8;
    Yolov4Lanenet.out_quantized = false;
    Yolov4Lanenet.out_format = HAILO_FORMAT_TYPE_FLOAT32;


    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Yolov4Lanenet, "0"), MnpReturnCode::SUCCESS);

    size_t ReturnedInput1SizeInByte = 0;
    pHailoPipeline->GetNetworkInputSize(Yolov4Lanenet.id_name, ReturnedInput1SizeInByte, 0);
    EXPECT_EQ(ReturnedInput1SizeInByte, 128*256*3);


    size_t ReturnedInput2SizeInByte = 0;
    pHailoPipeline->GetNetworkInputSize(Yolov4Lanenet.id_name, ReturnedInput2SizeInByte, 1);
    EXPECT_EQ(ReturnedInput2SizeInByte, 352*608*3);

    std::vector<uint8_t> TestInput1(ReturnedInput1SizeInByte, 0);
    EXPECT_EQ(pHailoPipeline->Infer(Yolov4Lanenet.id_name, TestInput1, "0", 0),
                                    MnpReturnCode::SUCCESS);


    std::vector<uint8_t> TestInput2(ReturnedInput2SizeInByte, 0);
    EXPECT_EQ(pHailoPipeline->Infer(Yolov4Lanenet.id_name, TestInput2, "0", 1),
                                    MnpReturnCode::SUCCESS);


    std::vector<std::vector<float32_t>> output_buffer;

    pHailoPipeline->InitializeOutputBuffer<float32_t>(Yolov4Lanenet.id_name, output_buffer, "0");

    MnpReturnCode status = pHailoPipeline->ReadOutputById(Yolov4Lanenet.id_name, output_buffer, "0");
    ASSERT_EQ(status, MnpReturnCode::SUCCESS);

    //We expect 5 outputs with this network
    EXPECT_EQ(output_buffer.size(), 5);

}



TEST_F(MNPTest1, MNPX_TwoNetworkGroup2InputMultiContextInferTest) {

    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);

    //Add Network
    stNetworkModelInfo Yolov4Lanenet;
    Yolov4Lanenet.id_name = "yolov4lanenet_sc";
    Yolov4Lanenet.hef_path = "Network/joined_yolov4_tiny_lanenet_mutiple_context.hef";
    Yolov4Lanenet.output_order_by_name.clear();
    Yolov4Lanenet.batch_size = 1;
    Yolov4Lanenet.in_quantized = false;
    Yolov4Lanenet.in_format = HAILO_FORMAT_TYPE_UINT8;
    Yolov4Lanenet.out_quantized = false;
    Yolov4Lanenet.out_format = HAILO_FORMAT_TYPE_FLOAT32;


    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Yolov4Lanenet, "0"), MnpReturnCode::SUCCESS);

    size_t ReturnedInput1SizeInByte = 0;
    pHailoPipeline->GetNetworkInputSize(Yolov4Lanenet.id_name, ReturnedInput1SizeInByte, 0);
    EXPECT_EQ(ReturnedInput1SizeInByte, 128*256*3);


    size_t ReturnedInput2SizeInByte = 0;
    pHailoPipeline->GetNetworkInputSize(Yolov4Lanenet.id_name, ReturnedInput2SizeInByte, 1);
    EXPECT_EQ(ReturnedInput2SizeInByte, 352*608*3);

    std::vector<uint8_t> TestInput1(ReturnedInput1SizeInByte, 0);
    EXPECT_EQ(pHailoPipeline->Infer(Yolov4Lanenet.id_name, TestInput1, "0", 0),
                                    MnpReturnCode::SUCCESS);


    std::vector<uint8_t> TestInput2(ReturnedInput2SizeInByte, 0);
    EXPECT_EQ(pHailoPipeline->Infer(Yolov4Lanenet.id_name, TestInput2, "0", 1),
                                    MnpReturnCode::SUCCESS);


    std::vector<std::vector<float32_t>> output_buffer;

    pHailoPipeline->InitializeOutputBuffer<float32_t>(Yolov4Lanenet.id_name, output_buffer, "0");

    MnpReturnCode status = pHailoPipeline->ReadOutputById(Yolov4Lanenet.id_name, output_buffer, "0");
    ASSERT_EQ(status, MnpReturnCode::SUCCESS);

    //We expect 14 outputs with this network
    EXPECT_EQ(output_buffer.size(), 14);

}


#if 0 //OLD TEST - For reference only as it uses old API



TEST_F(MNPTest1, MNP4OneNetworkMultipleInferAndRemove) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);
        
    //Add Network
    stNetworkModelInfo Yolov4Network;
    Yolov4Network.id_name = "yolov4";
    Yolov4Network.hef_path = "Network/yolov4.hef";
    Yolov4Network.output_order_by_name.clear();        
    Yolov4Network.batch_size = 1;
    Yolov4Network.in_quantized = false;
    Yolov4Network.in_format = HAILO_FORMAT_TYPE_UINT8;
    Yolov4Network.out_quantized = false;
    Yolov4Network.out_format = HAILO_FORMAT_TYPE_FLOAT32;    
    
    std::vector<std::string> Yolov4OutputOrderByName{"conv133_centers", "conv133_scales", "conv133_obj", "conv133_probs",
                                                     "conv126_centers", "conv126_scales", "conv126_obj", "conv126_probs",
                                                     "conv118_centers", "conv118_scales", "conv118_obj", "conv118_probs" };
    Yolov4Network.output_order_by_name = Yolov4OutputOrderByName;
    
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Yolov4Network), MnpReturnCode::SUCCESS);

    std::vector<int> Yolov4OutputSizeInOrder{1536,1536,768,61440,
                                             6144,6144,3072,245760,
                                             24576,24576,12288,983040};    
    
    //Infer up to NumberOfInference                                         
    size_t InputSize = 512*512*3; //Yolov4 input size
    std::vector<uint8_t> TestInput(InputSize, 0);
    size_t NumberOfInference = 20;
    for (size_t i = 0; i < NumberOfInference; i++)
    {
        EXPECT_EQ(pHailoPipeline->Infer(Yolov4Network.id_name, TestInput), 
                                        MnpReturnCode::SUCCESS);      
        std::this_thread::sleep_for(std::chrono::microseconds(10));  
    }
            
    //Expect to get up to NumberOfInference
    for (size_t i = 0; i < NumberOfInference; i++)
    {
        std::vector<std::vector<float32_t>> output_buffer;
        Timer TimeCount;
        
        //Make sure it contain nothing
        output_buffer.clear();
        
        while (1)
        {
            MnpReturnCode status = pHailoPipeline->ReadOutputById(Yolov4Network.id_name, output_buffer);
            EXPECT_NE(status, MnpReturnCode::NOT_FOUND);
            if (status == MnpReturnCode::SUCCESS)
                break;

            //Set a timer to make sure we are not in forever loop
            ASSERT_FALSE(TimeCount.isTimePastSec(TIME_OUT_SEC));
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        EXPECT_EQ(output_buffer.size(), Yolov4OutputSizeInOrder.size());

        for (size_t i = 0; i < output_buffer.size(); i++)
        {
            EXPECT_EQ(output_buffer[i].size(), Yolov4OutputSizeInOrder[i]);
        }
                
    }
    
    //Remove last Network
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(Yolov4Network.id_name), MnpReturnCode::SUCCESS);
 
    //Check total size (should be TotalNetwork)
    std::vector<stNetworkModelInfo> list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), 0);
}

TEST_F(MNPTest1, MNP5MultipleNetworkInfer) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);
    
    // 
    // Infer Yolov4 -> Yolov5m -> Yolov4
    // 
    //    
    
    //Add Network - Yolov4
    stNetworkModelInfo Yolov4Network;
    Yolov4Network.id_name = "yolov4";
    Yolov4Network.hef_path = "Network/yolov4.hef";
    Yolov4Network.output_order_by_name.clear();        
    Yolov4Network.batch_size = 1;
    Yolov4Network.in_quantized = false;
    Yolov4Network.in_format = HAILO_FORMAT_TYPE_UINT8;
    Yolov4Network.out_quantized = false;
    Yolov4Network.out_format = HAILO_FORMAT_TYPE_FLOAT32;    
    
    std::vector<std::string> Yolov4OutputOrderByName{"conv133_centers", "conv133_scales", "conv133_obj", "conv133_probs",
                                                     "conv126_centers", "conv126_scales", "conv126_obj", "conv126_probs",
                                                     "conv118_centers", "conv118_scales", "conv118_obj", "conv118_probs" };
    Yolov4Network.output_order_by_name = Yolov4OutputOrderByName;
    
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Yolov4Network), MnpReturnCode::SUCCESS);
    size_t Yolov4InputSize = 512*512*3; //Yolov4 input size
    std::vector<int> Yolov4OutputSizeInOrder{1536,1536,768,61440,
                                             6144,6144,3072,245760,
                                             24576,24576,12288,983040};    
    
                                                                                          
    //Add Network - Yolov5m
    stNetworkModelInfo Yolov5mNetwork;
    Yolov5mNetwork.id_name = "yolov5m";
    Yolov5mNetwork.hef_path = "Network/yolov5m.hef";
    Yolov5mNetwork.output_order_by_name.clear();        
    Yolov5mNetwork.batch_size = 1;
    Yolov5mNetwork.in_quantized = false;
    Yolov5mNetwork.in_format = HAILO_FORMAT_TYPE_UINT8;
    Yolov5mNetwork.out_quantized = false;
    Yolov5mNetwork.out_format = HAILO_FORMAT_TYPE_FLOAT32;    
    
    std::vector<std::string> Yolov5mOutputOrderByName{"conv107", "conv97", "conv87"};
    Yolov5mNetwork.output_order_by_name = Yolov5mOutputOrderByName;
    
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Yolov5mNetwork), MnpReturnCode::SUCCESS);
    size_t Yolov5mInputSize = 640*640*3; //Yolov5m input size
    std::vector<int> Yolov5mOutputSizeInOrder{  20*20*255,
                                                40*40*255,
                                                80*80*255}; 
                                             
                                             
    //Yolov4 Infer up to NumberOfInference  
    {
        std::vector<uint8_t> TestInput(Yolov4InputSize, 0);
        size_t NumberOfInference = 3;
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            EXPECT_EQ(pHailoPipeline->Infer(Yolov4Network.id_name, TestInput), 
                                            MnpReturnCode::SUCCESS);
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }

        //Expect to get up to NumberOfInference
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            std::vector<std::vector<float32_t>> output_buffer;
            Timer TimeCount;

            //Make sure it contain nothing
            output_buffer.clear();

            while (1)
            {
                MnpReturnCode status = pHailoPipeline->ReadOutputById(Yolov4Network.id_name, output_buffer);
                EXPECT_NE(status, MnpReturnCode::NOT_FOUND);
                if (status == MnpReturnCode::SUCCESS)
                    break;

                //Set a timer to make sure we are not in forever loop
                ASSERT_FALSE(TimeCount.isTimePastSec(TIME_OUT_SEC));
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }

            EXPECT_EQ(output_buffer.size(), Yolov4OutputSizeInOrder.size());

            for (size_t i = 0; i < output_buffer.size(); i++)
            {
                EXPECT_EQ(output_buffer[i].size(), Yolov4OutputSizeInOrder[i]);
            }

        }
    }
    
    //Yolov5m Infer up to NumberOfInference  
    {
        std::vector<uint8_t> TestInput(Yolov5mInputSize, 0);
        size_t NumberOfInference = 5;
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            EXPECT_EQ(pHailoPipeline->Infer(Yolov5mNetwork.id_name, TestInput), 
                                            MnpReturnCode::SUCCESS);
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }

        //Expect to get up to NumberOfInference
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            std::vector<std::vector<float32_t>> output_buffer;
            Timer TimeCount;

            //Make sure it contain nothing
            output_buffer.clear();

            while (1)
            {
                MnpReturnCode status = pHailoPipeline->ReadOutputById(Yolov5mNetwork.id_name, output_buffer);
                EXPECT_NE(status, MnpReturnCode::NOT_FOUND);
                if (status == MnpReturnCode::SUCCESS)
                    break;

                //Set a timer to make sure we are not in forever loop
                ASSERT_FALSE(TimeCount.isTimePastSec(TIME_OUT_SEC));
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }

            EXPECT_EQ(output_buffer.size(), Yolov5mOutputSizeInOrder.size());

            for (size_t i = 0; i < output_buffer.size(); i++)
            {
                EXPECT_EQ(output_buffer[i].size(), Yolov5mOutputSizeInOrder[i]);
            }
        }
    }
    
    //Yolov4 Infer up to NumberOfInference  
    {
        std::vector<uint8_t> TestInput(Yolov4InputSize, 0);
        size_t NumberOfInference = 3;
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            EXPECT_EQ(pHailoPipeline->Infer(Yolov4Network.id_name, TestInput), 
                                            MnpReturnCode::SUCCESS);
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }

        //Expect to get up to NumberOfInference
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            std::vector<std::vector<float32_t>> output_buffer;
            Timer TimeCount;

            //Make sure it contain nothing
            output_buffer.clear();

            while (1)
            {
                MnpReturnCode status = pHailoPipeline->ReadOutputById(Yolov4Network.id_name, output_buffer);
                EXPECT_NE(status, MnpReturnCode::NOT_FOUND);
                if (status == MnpReturnCode::SUCCESS)
                    break;

                //Set a timer to make sure we are not in forever loop
                ASSERT_FALSE(TimeCount.isTimePastSec(TIME_OUT_SEC));
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }

            EXPECT_EQ(output_buffer.size(), Yolov4OutputSizeInOrder.size());

            for (size_t i = 0; i < output_buffer.size(); i++)
            {
                EXPECT_EQ(output_buffer[i].size(), Yolov4OutputSizeInOrder[i]);
            }

        }
    }
    
    //Remove Network
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(Yolov4Network.id_name), MnpReturnCode::SUCCESS);
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(Yolov5mNetwork.id_name), MnpReturnCode::SUCCESS);
 
    //Check total size (should be TotalNetwork)
    std::vector<stNetworkModelInfo> list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), 0); 
    
}


TEST_F(MNPTest1, MNP7InferwithImageAndRemove) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);
        
    //Add Network
    stNetworkModelInfo Network;
    Network.id_name = "resnet50";
    Network.hef_path = "Network/resnet_v1_50.hef";
    Network.output_order_by_name.clear();        
    Network.batch_size = 1;
    Network.in_quantized = false;
    Network.in_format = HAILO_FORMAT_TYPE_UINT8;
    Network.out_quantized = false;
    Network.out_format = HAILO_FORMAT_TYPE_FLOAT32;    
       
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Network), MnpReturnCode::SUCCESS);

    size_t InputSize = 224*224*3;
    std::vector<uint8_t> ImageInput(InputSize, 0);

    cv::Mat Imageframe(224, 224, CV_8UC3);;
    Imageframe = cv::imread("Images/761_remotecontroller.jpg");       
    
    int totalsz = Imageframe.dataend-Imageframe.datastart;
    ImageInput.assign(Imageframe.datastart, Imageframe.datastart + totalsz);
                    
    
    EXPECT_EQ(pHailoPipeline->Infer(Network.id_name, ImageInput), 
                                    MnpReturnCode::SUCCESS);       
        
    std::vector<std::vector<float32_t>> output_buffer;
    Timer TimeCount;
    while (1)
    {
        MnpReturnCode status = pHailoPipeline->ReadOutputById(Network.id_name, output_buffer);
        EXPECT_NE(status, MnpReturnCode::NOT_FOUND);
        if (status == MnpReturnCode::SUCCESS)
            break;
        
        //Set a timer to make sure we are not in forever loop
        ASSERT_FALSE(TimeCount.isTimePastSec(TIME_OUT_SEC));
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    
    //We expect one array of 1000 element in this case (resnet50)
    EXPECT_EQ(output_buffer.size(), 1);
    EXPECT_EQ(output_buffer[0].size(), 1000);

    //Get the highest provability
    int maxElementIndex = std::max_element(output_buffer[0].begin(),
                                           output_buffer[0].end()) - output_buffer[0].begin();
        
    //We expect the class index of 761 which is remote controller
    EXPECT_EQ(maxElementIndex, 761);    
    //std::cout << "ClassIndex = " << maxElementIndex << ", Provability % = " << output_buffer[0][maxElementIndex] * 100 << std::endl;
               
    //Remove last Network
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(Network.id_name), MnpReturnCode::SUCCESS);
 
    //Check total size (should be TotalNetwork)
    std::vector<stNetworkModelInfo> list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), 0);
}

TEST_F(MNPTest1, MNP8MultipleInferwithImageAndRemove) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);
        
    //Add Network
    stNetworkModelInfo Network;
    Network.id_name = "resnet50";
    Network.hef_path = "Network/resnet_v1_50.hef";
    Network.output_order_by_name.clear();        
    Network.batch_size = 1;
    Network.in_quantized = false;
    Network.in_format = HAILO_FORMAT_TYPE_UINT8;
    Network.out_quantized = false;
    Network.out_format = HAILO_FORMAT_TYPE_FLOAT32;    
       
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Network), MnpReturnCode::SUCCESS);



    //Get all files under Images/
    std::string ImagePath("Images/"); 
    std::vector<std::string> fileList = GetRecords(ImagePath);
    for (auto file : fileList)
    {
        size_t InputSize = 224*224*3;
        std::vector<uint8_t> ImageInput(InputSize, 0);
        cv::Mat Imageframe(224, 224, CV_8UC3);;
       
        //std::cout << ImagePath + file << std::endl;
        Imageframe = cv::imread(ImagePath + file);       
        
        int totalsz = Imageframe.dataend-Imageframe.datastart;
        ImageInput.assign(Imageframe.datastart, Imageframe.datastart + totalsz);
                        
        
        EXPECT_EQ(pHailoPipeline->Infer(Network.id_name, ImageInput), 
                                        MnpReturnCode::SUCCESS);     
        
        std::this_thread::sleep_for(std::chrono::microseconds(10));  

    }
    
    for (auto file : fileList)
    {
        std::vector<std::vector<float32_t>> output_buffer;
        Timer TimeCount;
        while (1)
        {
            MnpReturnCode status = pHailoPipeline->ReadOutputById(Network.id_name, output_buffer);
            EXPECT_NE(status, MnpReturnCode::NOT_FOUND);
            if (status == MnpReturnCode::SUCCESS)
                break;
            
            //Set a timer to make sure we are not in forever loop
            ASSERT_FALSE(TimeCount.isTimePastSec(TIME_OUT_SEC));
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        //We expect one array of 1000 element in this case (resnet50)
        EXPECT_EQ(output_buffer.size(), 1);
        EXPECT_EQ(output_buffer[0].size(), 1000);

        //Get the highest provability
        int maxElementIndex = std::max_element(output_buffer[0].begin(),
                                            output_buffer[0].end()) - output_buffer[0].begin();
            
        //std::cout << file << std::endl;
        std::size_t classPos = file.rfind('_');
        //std::cout << file.substr(0, classPos) << std::endl;

        //We expect the same class index from the prefix of the file
        EXPECT_EQ(maxElementIndex, std::stoi( file.substr(0, classPos)));    
        std::cout << "ClassIndex = " << maxElementIndex << ", Provability % = " << output_buffer[0][maxElementIndex] * 100 << std::endl;
    
    }   
               
    //Remove last Network
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(Network.id_name), MnpReturnCode::SUCCESS);
 
    //Check total size (should be TotalNetwork)
    std::vector<stNetworkModelInfo> list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), 0);
}

TEST_F(MNPTest1, MNP9MultipleInferwithImageAndRemoveWithDataID) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);
        
    //Add Network
    stNetworkModelInfo Network;
    Network.id_name = "resnet50";
    Network.hef_path = "Network/resnet_v1_50.hef";
    Network.output_order_by_name.clear();        
    Network.batch_size = 1;
    Network.in_quantized = false;
    Network.in_format = HAILO_FORMAT_TYPE_UINT8;
    Network.out_quantized = false;
    Network.out_format = HAILO_FORMAT_TYPE_FLOAT32;    
       
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Network), MnpReturnCode::SUCCESS);



    //Get all files under Images/
    std::string ImagePath("Images/"); 
    std::vector<std::string> fileList = GetRecords(ImagePath);
    for (auto file : fileList)
    {
        size_t InputSize = 224*224*3;
        std::vector<uint8_t> ImageInput(InputSize, 0);
        cv::Mat Imageframe(224, 224, CV_8UC3);;
       
        //std::cout << ImagePath + file << std::endl;
        Imageframe = cv::imread(ImagePath + file);       
        
        int totalsz = Imageframe.dataend-Imageframe.datastart;
        ImageInput.assign(Imageframe.datastart, Imageframe.datastart + totalsz);
                        
        //Infer with data id (as name from file)
        EXPECT_EQ(pHailoPipeline->Infer(Network.id_name, ImageInput, file), 
                                        MnpReturnCode::SUCCESS);       

        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    
    for (auto file : fileList)
    {
        std::vector<std::vector<float32_t>> output_buffer;
        Timer TimeCount;
        while (1)
        {
            MnpReturnCode status = pHailoPipeline->ReadOutputById(Network.id_name, output_buffer, file);
            EXPECT_NE(status, MnpReturnCode::NOT_FOUND);
            if (status == MnpReturnCode::SUCCESS)
                break;
            
            //Set a timer to make sure we are not in forever loop
            ASSERT_FALSE(TimeCount.isTimePastSec(TIME_OUT_SEC));
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        //We expect one array of 1000 element in this case (resnet50)
        EXPECT_EQ(output_buffer.size(), 1);
        EXPECT_EQ(output_buffer[0].size(), 1000);

        //Get the highest provability
        int maxElementIndex = std::max_element(output_buffer[0].begin(),
                                            output_buffer[0].end()) - output_buffer[0].begin();
            
        //std::cout << file << std::endl;
        std::size_t classPos = file.rfind('_');
        //std::cout << file.substr(0, classPos) << std::endl;

        //We expect the same class index from the prefix of the file
        EXPECT_EQ(maxElementIndex, std::stoi( file.substr(0, classPos)));    
        std::cout << "ClassIndex = " << maxElementIndex << ", Provability % = " << output_buffer[0][maxElementIndex] * 100 << std::endl;
    
    }   
               
    //Remove last Network
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(Network.id_name), MnpReturnCode::SUCCESS);
 
    //Check total size (should be TotalNetwork)
    std::vector<stNetworkModelInfo> list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), 0);
}



TEST_F(MNPTest1, MNP11NetworkInputSizeCheck) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);
        
    //Add Network
    stNetworkModelInfo Network;
    Network.id_name = "resnet50";
    Network.hef_path = "Network/resnet_v1_50.hef";
    Network.output_order_by_name.clear();        
    Network.batch_size = 1;           
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Network), MnpReturnCode::SUCCESS);

    size_t ResnetExpectedInputSize = 224*224*3;

    //Add Network - Yolov5m
    stNetworkModelInfo Yolov5mNetwork;
    Yolov5mNetwork.id_name = "yolov5m";
    Yolov5mNetwork.hef_path = "Network/yolov5m.hef";
    Yolov5mNetwork.output_order_by_name.clear();        
    Yolov5mNetwork.batch_size = 1;     
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Yolov5mNetwork), MnpReturnCode::SUCCESS);
    
    size_t Yolov5mExpectedInputSize = 640*640*3; //Yolov5m input size
      
    //Check Resnet input size
    size_t ResnetActualInputSize = 0;
    EXPECT_EQ(pHailoPipeline->GetNetworkInputSize(Network.id_name, ResnetActualInputSize), MnpReturnCode::SUCCESS);
    EXPECT_EQ(ResnetActualInputSize, ResnetExpectedInputSize);
    
    //Check Yolov5m input size
    size_t Yolov5mActualInputSize = 0;
    EXPECT_EQ(pHailoPipeline->GetNetworkInputSize(Yolov5mNetwork.id_name, Yolov5mActualInputSize), MnpReturnCode::SUCCESS);
    EXPECT_EQ(Yolov5mActualInputSize, Yolov5mExpectedInputSize);
                                                
    //Last we do inference for Resnet just to make sure the input size check does not
    //affect future inference
    std::vector<uint8_t> ImageInput(ResnetActualInputSize, 0);
    cv::Mat Imageframe(224, 224, CV_8UC3);;
    Imageframe = cv::imread("Images/761_remotecontroller.jpg");       
    
    int totalsz = Imageframe.dataend-Imageframe.datastart;
    ImageInput.assign(Imageframe.datastart, Imageframe.datastart + totalsz);
                    
    
    EXPECT_EQ(pHailoPipeline->Infer(Network.id_name, ImageInput), 
                                    MnpReturnCode::SUCCESS);       
        
    std::vector<std::vector<float32_t>> output_buffer;
    Timer TimeCount;
    while (1)
    {
        MnpReturnCode status = pHailoPipeline->ReadOutputById(Network.id_name, output_buffer);
        EXPECT_NE(status, MnpReturnCode::NOT_FOUND);
        if (status == MnpReturnCode::SUCCESS)
            break;
        
        //Set a timer to make sure we are not in forever loop
        ASSERT_FALSE(TimeCount.isTimePastSec(TIME_OUT_SEC));
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    
    //We expect one array of 1000 element in this case (resnet50)
    EXPECT_EQ(output_buffer.size(), 1);
    EXPECT_EQ(output_buffer[0].size(), 1000);

    //Get the highest provability
    int maxElementIndex = std::max_element(output_buffer[0].begin(),
                                           output_buffer[0].end()) - output_buffer[0].begin();
        
    //We expect the class index of 761 which is remote controller
    EXPECT_EQ(maxElementIndex, 761);    
    //std::cout << "ClassIndex = " << maxElementIndex << ", Provability % = " << output_buffer[0][maxElementIndex] * 100 << std::endl;
               
    //Remove last Network
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(Network.id_name), MnpReturnCode::SUCCESS);
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(Yolov5mNetwork.id_name), MnpReturnCode::SUCCESS);
 
    //Check total size (should be TotalNetwork)
    std::vector<stNetworkModelInfo> list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), 0);

}

TEST_F(MNPTest1, MNP12OneNetworkinferNoTransformAndRemove) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);
        
    //Add Network
    stNetworkModelInfo Yolov4Network;
    Yolov4Network.id_name = "yolov4";
    Yolov4Network.hef_path = "Network/yolov4.hef";
    Yolov4Network.output_order_by_name.clear();        
    Yolov4Network.batch_size = 1;
    Yolov4Network.in_quantized = false;
    Yolov4Network.in_format = HAILO_FORMAT_TYPE_UINT8;
    Yolov4Network.out_quantized = true;
    Yolov4Network.out_format = HAILO_FORMAT_TYPE_UINT8;    
    
    std::vector<std::string> Yolov4OutputOrderByName{"conv133_centers", "conv133_scales", "conv133_obj", "conv133_probs",
                         "conv126_centers", "conv126_scales", "conv126_obj", "conv126_probs",
                         "conv118_centers", "conv118_scales", "conv118_obj", "conv118_probs" };
    Yolov4Network.output_order_by_name = Yolov4OutputOrderByName;
    
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Yolov4Network), MnpReturnCode::SUCCESS);

    std::vector<int> Yolov4OutputSizeInOrder{1536,1536,768,61440,
                                             6144,6144,3072,245760,
                                             24576,24576,12288,983040};    
    size_t InputSize = 512*512*3; //Yolov4 input size
    std::vector<uint8_t> TestInput(InputSize, 0);
    EXPECT_EQ(pHailoPipeline->Infer(Yolov4Network.id_name, TestInput), 
                                    MnpReturnCode::SUCCESS);       
    
    std::vector<std::vector<uint8_t>> output_buffer;
    Timer TimeCount;
    while (1)
    {
        MnpReturnCode status = pHailoPipeline->ReadOutputById(Yolov4Network.id_name, output_buffer);
        EXPECT_NE(status, MnpReturnCode::NOT_FOUND);
        if (status == MnpReturnCode::SUCCESS)
            break;
        
        //Set a timer to make sure we are not in forever loop
        ASSERT_FALSE(TimeCount.isTimePastSec(TIME_OUT_SEC));
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    
    EXPECT_EQ(output_buffer.size(), Yolov4OutputSizeInOrder.size());
        
    for (size_t i = 0; i < output_buffer.size(); i++)
    {
        EXPECT_EQ(output_buffer[i].size(), Yolov4OutputSizeInOrder[i]);
    }
    
    //Remove last Network
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(Yolov4Network.id_name), MnpReturnCode::SUCCESS);
 
    //Check total size (should be TotalNetwork)
    std::vector<stNetworkModelInfo> list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), 0);
}

TEST_F(MNPTest1, MNP13OneNetworkMultipleInferAndRemove) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);
        
    //Add Network
    stNetworkModelInfo Yolov4Network;
    Yolov4Network.id_name = "yolov4";
    Yolov4Network.hef_path = "Network/yolov4.hef";
    Yolov4Network.output_order_by_name.clear();        
    Yolov4Network.batch_size = 1;
    Yolov4Network.in_quantized = false;
    Yolov4Network.in_format = HAILO_FORMAT_TYPE_UINT8;
    Yolov4Network.out_quantized = true;
    Yolov4Network.out_format = HAILO_FORMAT_TYPE_UINT8;    
    
    std::vector<std::string> Yolov4OutputOrderByName{"conv133_centers", "conv133_scales", "conv133_obj", "conv133_probs",
                                                     "conv126_centers", "conv126_scales", "conv126_obj", "conv126_probs",
                                                     "conv118_centers", "conv118_scales", "conv118_obj", "conv118_probs" };
    Yolov4Network.output_order_by_name = Yolov4OutputOrderByName;
    
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Yolov4Network), MnpReturnCode::SUCCESS);

    std::vector<int> Yolov4OutputSizeInOrder{1536,1536,768,61440,
                                             6144,6144,3072,245760,
                                             24576,24576,12288,983040};    
    
    //Infer up to NumberOfInference                                         
    size_t InputSize = 512*512*3; //Yolov4 input size
    std::vector<uint8_t> TestInput(InputSize, 0);
    size_t NumberOfInference = 20;
    for (size_t i = 0; i < NumberOfInference; i++)
    {
        EXPECT_EQ(pHailoPipeline->Infer(Yolov4Network.id_name, TestInput), 
                                        MnpReturnCode::SUCCESS);  
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
            
    //Expect to get up to NumberOfInference
    for (size_t i = 0; i < NumberOfInference; i++)
    {
        std::vector<std::vector<uint8_t>> output_buffer;
        Timer TimeCount;
        
        //Make sure it contain nothing
        output_buffer.clear();
        
        while (1)
        {
            MnpReturnCode status = pHailoPipeline->ReadOutputById(Yolov4Network.id_name, output_buffer);
            EXPECT_NE(status, MnpReturnCode::NOT_FOUND);
            if (status == MnpReturnCode::SUCCESS)
                break;

            //Set a timer to make sure we are not in forever loop
            ASSERT_FALSE(TimeCount.isTimePastSec(TIME_OUT_SEC));
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        EXPECT_EQ(output_buffer.size(), Yolov4OutputSizeInOrder.size());

        for (size_t i = 0; i < output_buffer.size(); i++)
        {
            EXPECT_EQ(output_buffer[i].size(), Yolov4OutputSizeInOrder[i]);
        }
                
    }
    
    //Remove last Network
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(Yolov4Network.id_name), MnpReturnCode::SUCCESS);
 
    //Check total size (should be TotalNetwork)
    std::vector<stNetworkModelInfo> list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), 0);
}

TEST_F(MNPTest1, MNP14MultipleNetworkInferWithNoTransformMix) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);
    
    // 
    // Infer Yolov4 -> Yolov5m -> Yolov4
    // 
    //    
    
    //Add Network - Yolov4
    stNetworkModelInfo Yolov4Network;
    Yolov4Network.id_name = "yolov4";
    Yolov4Network.hef_path = "Network/yolov4.hef";
    Yolov4Network.output_order_by_name.clear();        
    Yolov4Network.batch_size = 1;
    Yolov4Network.in_quantized = false;
    Yolov4Network.in_format = HAILO_FORMAT_TYPE_UINT8;
    Yolov4Network.out_quantized = false;
    Yolov4Network.out_format = HAILO_FORMAT_TYPE_FLOAT32;    
    
    std::vector<std::string> Yolov4OutputOrderByName{"conv133_centers", "conv133_scales", "conv133_obj", "conv133_probs",
                                                     "conv126_centers", "conv126_scales", "conv126_obj", "conv126_probs",
                                                     "conv118_centers", "conv118_scales", "conv118_obj", "conv118_probs" };
    Yolov4Network.output_order_by_name = Yolov4OutputOrderByName;
    
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Yolov4Network), MnpReturnCode::SUCCESS);
    size_t Yolov4InputSize = 512*512*3; //Yolov4 input size
    std::vector<int> Yolov4OutputSizeInOrder{1536,1536,768,61440,
                                             6144,6144,3072,245760,
                                             24576,24576,12288,983040};    
    
                                                                                          
    //Add Network - Yolov5m
    stNetworkModelInfo Yolov5mNetwork;
    Yolov5mNetwork.id_name = "yolov5m";
    Yolov5mNetwork.hef_path = "Network/yolov5m.hef";
    Yolov5mNetwork.output_order_by_name.clear();        
    Yolov5mNetwork.batch_size = 1;
    Yolov5mNetwork.in_quantized = false;
    Yolov5mNetwork.in_format = HAILO_FORMAT_TYPE_UINT8;
    Yolov5mNetwork.out_quantized = true;
    Yolov5mNetwork.out_format = HAILO_FORMAT_TYPE_UINT8;    
    
    std::vector<std::string> Yolov5mOutputOrderByName{"conv107", "conv97", "conv87"};
    Yolov5mNetwork.output_order_by_name = Yolov5mOutputOrderByName;
    
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Yolov5mNetwork), MnpReturnCode::SUCCESS);
    size_t Yolov5mInputSize = 640*640*3; //Yolov5m input size
    std::vector<int> Yolov5mOutputSizeInOrder{  20*20*255,
                                                40*40*255,
                                                80*80*255}; 
                                             
                                             
    //Yolov4 Infer up to NumberOfInference  
    {
        std::vector<uint8_t> TestInput(Yolov4InputSize, 0);
        size_t NumberOfInference = 3;
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            EXPECT_EQ(pHailoPipeline->Infer(Yolov4Network.id_name, TestInput), 
                                            MnpReturnCode::SUCCESS);
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        }

        //Expect to get up to NumberOfInference
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            std::vector<std::vector<float32_t>> output_buffer;
            Timer TimeCount;

            //Make sure it contain nothing
            output_buffer.clear();

            while (1)
            {
                MnpReturnCode status = pHailoPipeline->ReadOutputById(Yolov4Network.id_name, output_buffer);
                EXPECT_NE(status, MnpReturnCode::NOT_FOUND);
                if (status == MnpReturnCode::SUCCESS)
                    break;

                //Set a timer to make sure we are not in forever loop
                ASSERT_FALSE(TimeCount.isTimePastSec(TIME_OUT_SEC));
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }

            EXPECT_EQ(output_buffer.size(), Yolov4OutputSizeInOrder.size());

            for (size_t i = 0; i < output_buffer.size(); i++)
            {
                EXPECT_EQ(output_buffer[i].size(), Yolov4OutputSizeInOrder[i]);
            }

        }
    }
    
    //Yolov5m Infer up to NumberOfInference  
    {
        std::vector<uint8_t> TestInput(Yolov5mInputSize, 0);
        size_t NumberOfInference = 5;
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            EXPECT_EQ(pHailoPipeline->Infer(Yolov5mNetwork.id_name, TestInput), 
                                            MnpReturnCode::SUCCESS);
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }

        //Expect to get up to NumberOfInference
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            std::vector<std::vector<uint8_t>> output_buffer;
            Timer TimeCount;

            //Make sure it contain nothing
            output_buffer.clear();

            while (1)
            {
                MnpReturnCode status = pHailoPipeline->ReadOutputById(Yolov5mNetwork.id_name, output_buffer);
                EXPECT_NE(status, MnpReturnCode::NOT_FOUND);
                if (status == MnpReturnCode::SUCCESS)
                    break;

                //Set a timer to make sure we are not in forever loop
                ASSERT_FALSE(TimeCount.isTimePastSec(TIME_OUT_SEC));
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }

            EXPECT_EQ(output_buffer.size(), Yolov5mOutputSizeInOrder.size());

            for (size_t i = 0; i < output_buffer.size(); i++)
            {
                EXPECT_EQ(output_buffer[i].size(), Yolov5mOutputSizeInOrder[i]);
            }
        }
    }
    
    //Yolov4 Infer up to NumberOfInference  
    {
        std::vector<uint8_t> TestInput(Yolov4InputSize, 0);
        size_t NumberOfInference = 3;
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            EXPECT_EQ(pHailoPipeline->Infer(Yolov4Network.id_name, TestInput), 
                                            MnpReturnCode::SUCCESS);
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }

        //Expect to get up to NumberOfInference
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            std::vector<std::vector<float32_t>> output_buffer;
            Timer TimeCount;

            //Make sure it contain nothing
            output_buffer.clear();

            while (1)
            {
                MnpReturnCode status = pHailoPipeline->ReadOutputById(Yolov4Network.id_name, output_buffer);
                EXPECT_NE(status, MnpReturnCode::NOT_FOUND);
                if (status == MnpReturnCode::SUCCESS)
                    break;

                //Set a timer to make sure we are not in forever loop
                ASSERT_FALSE(TimeCount.isTimePastSec(TIME_OUT_SEC));
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }

            EXPECT_EQ(output_buffer.size(), Yolov4OutputSizeInOrder.size());

            for (size_t i = 0; i < output_buffer.size(); i++)
            {
                EXPECT_EQ(output_buffer[i].size(), Yolov4OutputSizeInOrder[i]);
            }

        }
    }
    
    //Remove Network
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(Yolov4Network.id_name), MnpReturnCode::SUCCESS);
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(Yolov5mNetwork.id_name), MnpReturnCode::SUCCESS);
 
    //Check total size (should be TotalNetwork)
    std::vector<stNetworkModelInfo> list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), 0); 
    
}

TEST_F(MNPTest1, MNP15InferwithImageNoTransformationAndRemove) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);
        
    //Add Network
    stNetworkModelInfo Network;
    Network.id_name = "resnet50";
    Network.hef_path = "Network/resnet_v1_50.hef";
    Network.output_order_by_name.clear();        
    Network.batch_size = 1;
    Network.in_quantized = false;
    Network.in_format = HAILO_FORMAT_TYPE_UINT8;
    Network.out_quantized = true;
    Network.out_format = HAILO_FORMAT_TYPE_UINT8;    
       
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Network), MnpReturnCode::SUCCESS);

    size_t InputSize = 224*224*3;
    std::vector<uint8_t> ImageInput(InputSize, 0);

    cv::Mat Imageframe(224, 224, CV_8UC3);;
    Imageframe = cv::imread("Images/761_remotecontroller.jpg");       
    
    int totalsz = Imageframe.dataend-Imageframe.datastart;
    ImageInput.assign(Imageframe.datastart, Imageframe.datastart + totalsz);
                    
    
    EXPECT_EQ(pHailoPipeline->Infer(Network.id_name, ImageInput), 
                                    MnpReturnCode::SUCCESS);       
        
    std::vector<std::vector<uint8_t>> output_buffer;
    Timer TimeCount;
    while (1)
    {
        MnpReturnCode status = pHailoPipeline->ReadOutputById(Network.id_name, output_buffer);
        EXPECT_NE(status, MnpReturnCode::NOT_FOUND);
        if (status == MnpReturnCode::SUCCESS)
            break;
        
        //Set a timer to make sure we are not in forever loop
        ASSERT_FALSE(TimeCount.isTimePastSec(TIME_OUT_SEC));
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    
    //We expect one array of 1000 element in this case (resnet50)
    EXPECT_EQ(output_buffer.size(), 1);
    EXPECT_EQ(output_buffer[0].size(), 1000);

    std::vector<qp_zp_scale_t> NetworkQuantInfo;
    pHailoPipeline->GetNetworkQuantizationInfo(Network.id_name, NetworkQuantInfo);

    //Quantize the output (eg, transform to float 32)
    std::vector<float32_t> quantized_output_result;
    quantized_output_result.resize(output_buffer[0].size());
    for (size_t kk=0; kk < output_buffer[0].size(); kk++)
    {
        quantized_output_result[kk] = ((float32_t)output_buffer[0][kk] - NetworkQuantInfo[0].qp_zp) * NetworkQuantInfo[0].qp_scale;
    }

    //Get the highest provability
    int maxElementIndex = std::max_element(quantized_output_result.begin(),
                                           quantized_output_result.end()) - quantized_output_result.begin();
        
    //We expect the class index of 761 which is remote controller
    EXPECT_EQ(maxElementIndex, 761);    
    //std::cout << "ClassIndex = " << maxElementIndex << ", Provability % = " << quantized_output_result[maxElementIndex] * 100 << std::endl;
               
    //Remove last Network
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(Network.id_name), MnpReturnCode::SUCCESS);
 
    //Check total size (should be TotalNetwork)
    std::vector<stNetworkModelInfo> list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), 0);
}

#endif

