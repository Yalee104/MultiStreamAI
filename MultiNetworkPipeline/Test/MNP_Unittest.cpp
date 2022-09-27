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
#include "MultiNetworkPipeline.hpp"
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

#if 1

TEST_F(MNPTest1, MNP1SingleAddRemoveNetworkNoActivate) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);
    
    //Add Network
    stNetworkModelInfo Yolov4Network;
    Yolov4Network.id_name = "yolov4";
    Yolov4Network.hef_path = "Test/Network/yolov4.hef";
    Yolov4Network.output_order_by_name.clear();        
    Yolov4Network.batch_size = 1;
    Yolov4Network.in_quantized = false;
    Yolov4Network.in_format = HAILO_FORMAT_TYPE_UINT8;
    Yolov4Network.out_quantized = false;
    Yolov4Network.out_format = HAILO_FORMAT_TYPE_FLOAT32;    
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Yolov4Network), MnpReturnCode::SUCCESS);
    
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Yolov4Network), MnpReturnCode::DUPLICATE);

    /* HailoRT will print error message which generate confusion if test is 
     * failed or not, although there is nothing wrong with it as it is intended.
     * to generate such message      
    //Test add network with invalid HEF file path
    stNetworkModelInfo Yolov4NetworkWrongPath;
    Yolov4NetworkWrongPath.id_name = "yolov4dummy";
    Yolov4NetworkWrongPath.hef_path = "WrongPath";
    Yolov4NetworkWrongPath.output_order_by_name.clear();            
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Yolov4NetworkWrongPath), MnpReturnCode::FAILED);
    */
    
    //Get added network
    const stNetworkModelInfo* pYolov4Net = pHailoPipeline->GetNetworkInfoByName(Yolov4Network.id_name);    
    EXPECT_NE(pYolov4Net, nullptr);
    
    //Check total size
    std::vector<stNetworkModelInfo> list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), 1);
    
    //Get network given wrong id
    pYolov4Net = pHailoPipeline->GetNetworkInfoByName("DoesNotExist");    
    EXPECT_EQ(pYolov4Net, nullptr);
    
    //Remove Network given wrong id
    EXPECT_EQ(pHailoPipeline->RemoveNetwork("DoesNotExist"), MnpReturnCode::NOT_FOUND);
    
    //Remove Network
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(Yolov4Network.id_name), MnpReturnCode::SUCCESS);

    //Check total size
    list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), 0);
    
}

TEST_F(MNPTest1, MNP2MultipleAddRemoveNetworkNoActivate) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);
    
    int TotalNetwork = 4;
    std::string NetworkIdCommon("network");
    
    for (int i = 0; i < TotalNetwork; i++)
    {
        //Add Network
        stNetworkModelInfo Network;
        Network.id_name = std::string(NetworkIdCommon).append(std::to_string(i));
        Network.hef_path = "Test/Network/yolov5m.hef";
        Network.output_order_by_name.clear();        
        EXPECT_EQ(pHailoPipeline->AddNetwork(0, Network), MnpReturnCode::SUCCESS);
    }
    
    //Check total size (should be TotalNetwork)
    std::vector<stNetworkModelInfo> list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), TotalNetwork); 
    
    //Remove Network until one is left
    srand (time(NULL));
    int SkipIndex = rand() % TotalNetwork;
    for (int i = 0; i < TotalNetwork; i++)
    {
        if (i != SkipIndex)
        {
            EXPECT_EQ(pHailoPipeline->RemoveNetwork(std::string(NetworkIdCommon).append(std::to_string(i))), MnpReturnCode::SUCCESS);        
        }
    }
    
    //Check total size (should be 1 left)
    list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), 1);    
            
    //Get added network
    std::string LastNetworkId = std::string(NetworkIdCommon).append(std::to_string(SkipIndex));
    const stNetworkModelInfo* pNetwork = pHailoPipeline->GetNetworkInfoByName(LastNetworkId);    
    EXPECT_NE(pNetwork, nullptr);
    
    //Remove last Network
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(LastNetworkId), MnpReturnCode::SUCCESS);
        
    //Check total size (none left)
    list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), 0);    
}

TEST_F(MNPTest1, MNP3OneNetworkinferAndRemove) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);
        
    //Add Network
    stNetworkModelInfo Yolov4Network;
    Yolov4Network.id_name = "yolov4";
    Yolov4Network.hef_path = "Test/Network/yolov4.hef";
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
    size_t InputSize = 512*512*3; //Yolov4 input size
    std::vector<uint8_t> TestInput(InputSize, 0);
    EXPECT_EQ(pHailoPipeline->Infer(Yolov4Network.id_name, TestInput), 
                                    MnpReturnCode::SUCCESS);       
    
    std::vector<std::vector<float32_t>> output_buffer;
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

TEST_F(MNPTest1, MNP4OneNetworkMultipleInferAndRemove) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);
        
    //Add Network
    stNetworkModelInfo Yolov4Network;
    Yolov4Network.id_name = "yolov4";
    Yolov4Network.hef_path = "Test/Network/yolov4.hef";
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
    Yolov4Network.hef_path = "Test/Network/yolov4.hef";
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
    Yolov5mNetwork.hef_path = "Test/Network/yolov5m.hef";
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


#ifdef FAST_INFERENCE

TEST_F(MNPTest1, MNP6MultipleNetworkInferMixFastInference) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);
    
    
    //Add Network - Yolov4
    stNetworkModelInfo Yolov4Network;
    Yolov4Network.id_name = "yolov4";
    Yolov4Network.hef_path = "Test/Network/yolov4.hef";
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
    Yolov5mNetwork.hef_path = "Test/Network/yolov5m.hef";
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
                                             
                                             
    //We infer Yolov4 with Yolov5m in between, since we enabled FAST_INFERENCE
    //we should expect Yolov4 taking the priority while it is still active and
    //Yolov5m inference will be sent to queue for later process                                                  
    {
        std::vector<uint8_t> TestYolov4Input(Yolov4InputSize, 0);
        size_t First_Yolov4_NumberOfInference = 10;
        for (size_t i = 0; i < First_Yolov4_NumberOfInference; i++)
        {
            EXPECT_EQ(pHailoPipeline->Infer(Yolov4Network.id_name, TestYolov4Input), 
                                            MnpReturnCode::SUCCESS);
            
            std::this_thread::sleep_for(std::chrono::microseconds(10));                                            
        }

        //1ms sleep so that Yolov4 is activated before we continue sending data
        //otherwise it will most likely all going into queue as network won't yet
        //be activated.
        std::this_thread::sleep_for(std::chrono::microseconds(1000));
        
        std::vector<uint8_t> TestYolov5mInput(Yolov5mInputSize, 0);
        size_t Second_Yolo5m_NumberOfInference = 5;
        for (size_t i = 0; i < Second_Yolo5m_NumberOfInference; i++)
        {
            EXPECT_EQ(pHailoPipeline->Infer(Yolov5mNetwork.id_name, TestYolov5mInput), 
                                            MnpReturnCode::SUCCESS);
            std::this_thread::sleep_for(std::chrono::microseconds(10));                                            
        }        
        
        size_t Third_Yolov4_NumberOfInference = 15;
        for (size_t i = 0; i < Third_Yolov4_NumberOfInference; i++)
        {
            EXPECT_EQ(pHailoPipeline->Infer(Yolov4Network.id_name, TestYolov4Input), 
                                            MnpReturnCode::SUCCESS);
            std::this_thread::sleep_for(std::chrono::microseconds(10));                                            
        }
        
        
        //Expect to get up to Yolov4_TotalInference
        size_t Yolov4_TotalInference = First_Yolov4_NumberOfInference + Third_Yolov4_NumberOfInference;
        for (size_t i = 0; i < Yolov4_TotalInference; i++)
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
            
            //Here we try to get Yolov5m output data at the last 2 yolov4 output
            //we do not expect any yolov5m data at this state due to FAST_INFERENCE
            if (i == (Yolov4_TotalInference - 2))
            {
                output_buffer.clear();
                MnpReturnCode status = pHailoPipeline->ReadOutputById(Yolov5mNetwork.id_name, output_buffer);
                EXPECT_EQ(status, MnpReturnCode::NO_DATA_AVAILABLE);
            }
        }
        
        
        //Expect to get up to Yolov5m_TotalInference
        size_t Yolov5m_TotalInference = Second_Yolo5m_NumberOfInference;
        for (size_t i = 0; i < Yolov5m_TotalInference; i++)
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
    
    
    //Remove Network
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(Yolov4Network.id_name), MnpReturnCode::SUCCESS);
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(Yolov5mNetwork.id_name), MnpReturnCode::SUCCESS);
 
    //Check total size (should be TotalNetwork)
    std::vector<stNetworkModelInfo> list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), 0); 
    
}

#endif


TEST_F(MNPTest1, MNP7InferwithImageAndRemove) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);
        
    //Add Network
    stNetworkModelInfo Network;
    Network.id_name = "resnet50";
    Network.hef_path = "Test/Network/resnet_v1_50.hef";
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
    Imageframe = cv::imread("Test/Images/761_remotecontroller.jpg");       
    
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
    Network.hef_path = "Test/Network/resnet_v1_50.hef";
    Network.output_order_by_name.clear();        
    Network.batch_size = 1;
    Network.in_quantized = false;
    Network.in_format = HAILO_FORMAT_TYPE_UINT8;
    Network.out_quantized = false;
    Network.out_format = HAILO_FORMAT_TYPE_FLOAT32;    
       
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Network), MnpReturnCode::SUCCESS);



    //Get all files under Test/Images/
    std::string ImagePath("Test/Images/"); 
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
    Network.hef_path = "Test/Network/resnet_v1_50.hef";
    Network.output_order_by_name.clear();        
    Network.batch_size = 1;
    Network.in_quantized = false;
    Network.in_format = HAILO_FORMAT_TYPE_UINT8;
    Network.out_quantized = false;
    Network.out_format = HAILO_FORMAT_TYPE_FLOAT32;    
       
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Network), MnpReturnCode::SUCCESS);



    //Get all files under Test/Images/
    std::string ImagePath("Test/Images/"); 
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

#endif


#if 0
TEST_F(MNPTest1, BUG_REPRODUCE_MULTI_INFERENCE_ON_4_2) {
    
    //Description:
    //  Part 2 is addition to part 1 with other network during inference in between, just to make sure
    //  the module is handle it correctly when on multiple network inference and still provide
    //  accurate result (DataID) as it should be  

    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);
        
    //Add Network
    stNetworkModelInfo Network;
    Network.id_name = "resnet50";
    Network.hef_path = "Test/Network/resnet_v1_50.hef";
    Network.output_order_by_name.clear();        
    Network.batch_size = 1;
    Network.in_quantized = false;
    Network.in_format = HAILO_FORMAT_TYPE_UINT8;
    Network.out_quantized = false;
    Network.out_format = HAILO_FORMAT_TYPE_FLOAT32;    
       
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Network), MnpReturnCode::SUCCESS);

    //Add Network (dummy)
    stNetworkModelInfo NetworkDummy;
    NetworkDummy.id_name = "resnet50_dummy";
    NetworkDummy.hef_path = "Test/Network/resnet_v1_50_dummy.hef";
    NetworkDummy.output_order_by_name.clear();        
    NetworkDummy.batch_size = 1;
    NetworkDummy.in_quantized = false;
    NetworkDummy.in_format = HAILO_FORMAT_TYPE_UINT8;
    NetworkDummy.out_quantized = false;
    NetworkDummy.out_format = HAILO_FORMAT_TYPE_FLOAT32;    
       
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, NetworkDummy), MnpReturnCode::SUCCESS);


    //Get all files under Test/Images/
    std::string ImagePath("Test/Images/"); 
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
        std::vector<uint8_t> DummyNetworkInput = std::vector<uint8_t>(ImageInput);
        EXPECT_EQ(pHailoPipeline->Infer(NetworkDummy.id_name, DummyNetworkInput), 
                                        MnpReturnCode::SUCCESS);   

        std::this_thread::sleep_for(std::chrono::microseconds(10));

        //Infer with data id (as name from file)
        EXPECT_EQ(pHailoPipeline->Infer(Network.id_name, ImageInput, file), MnpReturnCode::SUCCESS);   


        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    
    //Although above should suffice but does not hurt to make it a bit more complex for this test     
    /*
    for (int i = 0; i < 10; i++)
    {
        //Infer Dummy network
        EXPECT_EQ(pHailoPipeline->Infer(NetworkDummy.id_name, DummyNetworkInput), 
                                        MnpReturnCode::SUCCESS);
                                        
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    */

    for (auto file : fileList)
    {
        std::string data_id;
        std::vector<std::vector<float32_t>> output_buffer;
        Timer TimeCount;
        while (1)
        {
            MnpReturnCode status = pHailoPipeline->ReadOutputById(Network.id_name, output_buffer, &data_id);
            EXPECT_NE(status, MnpReturnCode::NOT_FOUND);
            if (status == MnpReturnCode::SUCCESS)
                break;
            
            //Set a timer to make sure we are not in forever loop
            ASSERT_FALSE(TimeCount.isTimePastSec(TIME_OUT_SEC));
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        //We expect the returned data_id is what we expected
        EXPECT_EQ(data_id,file);

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


    //Remove resnet Network
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(Network.id_name), MnpReturnCode::SUCCESS);
 
    //Remove dummy Network
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(NetworkDummy.id_name), MnpReturnCode::SUCCESS);

    //Check total size (should be TotalNetwork)
    std::vector<stNetworkModelInfo> list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), 0);
}
#endif

#if 1
TEST_F(MNPTest1, MNP10MultipleInferwithImageAndRemoveWithDataIDPart2) {
    
    //Description:
    //  Part 2 is addition to part 1 with other network during inference in between, just to make sure
    //  the module is handle it correctly when on multiple network inference and still provide
    //  accurate result (DataID) as it should be  

    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);
        
    //Add Network
    stNetworkModelInfo Network;
    Network.id_name = "resnet50";
    Network.hef_path = "Test/Network/resnet_v1_50.hef";
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
    NetworkDummy.hef_path = "Test/Network/yolov4.hef";
    NetworkDummy.output_order_by_name.clear();        
    NetworkDummy.batch_size = 1;
    size_t NetworkDummyInputSize = 512*512*3; //Yolov4 input size
    std::vector<uint8_t> DummyNetworkInput(NetworkDummyInputSize, 0);
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, NetworkDummy), MnpReturnCode::SUCCESS);
    
    //Get all files under Test/Images/
    std::string ImagePath("Test/Images/"); 
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
        EXPECT_EQ(pHailoPipeline->Infer(Network.id_name, ImageInput, file), MnpReturnCode::SUCCESS);   


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

    sleep(1);

    //Remove resnet Network
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(Network.id_name), MnpReturnCode::SUCCESS);
 
    //Remove dummy Network
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(NetworkDummy.id_name), MnpReturnCode::SUCCESS);

    //Check total size (should be TotalNetwork)
    std::vector<stNetworkModelInfo> list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), 0);
}
#endif

#if 1

TEST_F(MNPTest1, MNP11NetworkInputSizeCheck) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);
        
    //Add Network
    stNetworkModelInfo Network;
    Network.id_name = "resnet50";
    Network.hef_path = "Test/Network/resnet_v1_50.hef";
    Network.output_order_by_name.clear();        
    Network.batch_size = 1;           
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Network), MnpReturnCode::SUCCESS);

    size_t ResnetExpectedInputSize = 224*224*3;

    //Add Network - Yolov5m
    stNetworkModelInfo Yolov5mNetwork;
    Yolov5mNetwork.id_name = "yolov5m";
    Yolov5mNetwork.hef_path = "Test/Network/yolov5m.hef";
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
    Imageframe = cv::imread("Test/Images/761_remotecontroller.jpg");       
    
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
    Yolov4Network.hef_path = "Test/Network/yolov4.hef";
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
    Yolov4Network.hef_path = "Test/Network/yolov4.hef";
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
    Yolov4Network.hef_path = "Test/Network/yolov4.hef";
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
    Yolov5mNetwork.hef_path = "Test/Network/yolov5m.hef";
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
    Network.hef_path = "Test/Network/resnet_v1_50.hef";
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
    Imageframe = cv::imread("Test/Images/761_remotecontroller.jpg");       
    
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

#if 1
#ifdef JOIN_NETWORK_SINGLE_CONTEXT_INDEPENDENT_INFER

TEST_F(MNPTest1, MNP16MergedNetworkinferAndRemove) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);

    std::string yolo_data_id = "yolo";
    std::string lprnet_data_id = "lprnet";

        
    //Add Network
    stNetworkModelInfo JoinNetwork;
    JoinNetwork.id_name = "join_yolov5s_lprnet";
    JoinNetwork.hef_path = "Test/Network/joined_yolov5s_LPRNet.hef";
    JoinNetwork.output_order_by_name.clear();
    JoinNetwork.input_order_by_name.clear();        
    JoinNetwork.batch_size = 1;
    JoinNetwork.in_quantized = false;
    JoinNetwork.in_format = HAILO_FORMAT_TYPE_UINT8;
    JoinNetwork.out_quantized = false;
    JoinNetwork.out_format = HAILO_FORMAT_TYPE_FLOAT32;    
    
    std::vector<std::string> InputOrderByName{"yolov5s/input_layer1", "LPRNet/input_layer1"};
    JoinNetwork.input_order_by_name = InputOrderByName;
    
    std::vector<std::string> OutputOrderByName{ "yolov5s/conv55", "yolov5s/conv63", "yolov5s/conv70", 
                                                "LPRNet/conv15", "LPRNet/conv5", "LPRNet/conv13" };
    JoinNetwork.output_order_by_name = OutputOrderByName;

    EXPECT_EQ(pHailoPipeline->AddNetwork(0, JoinNetwork), MnpReturnCode::SUCCESS);

    size_t NetworkInputSize = 0;
    size_t Yolov5sInputSize = 608*608*3;
    pHailoPipeline->GetNetworkInputSize(JoinNetwork.id_name, NetworkInputSize, 0);
    EXPECT_EQ(NetworkInputSize, Yolov5sInputSize);
    //std::cout << "yolov5s/input_layer1 input size = " << NetworkInputSize << std::endl;

    size_t LprNetInputSize = 96*24*3;
    pHailoPipeline->GetNetworkInputSize(JoinNetwork.id_name, NetworkInputSize, 1);
    EXPECT_EQ(NetworkInputSize, LprNetInputSize);
    //std::cout << "LPRNet/input_layer1 input size = " << NetworkInputSize << std::endl;


    /* Infer LPRNet */
    std::vector<uint8_t> LprnetTestInput(LprNetInputSize, 0);
    EXPECT_EQ(pHailoPipeline->Infer(JoinNetwork.id_name, LprnetTestInput, lprnet_data_id, 1), 
                                    MnpReturnCode::SUCCESS);       

    /* Get LPRNet prediction result */
    std::vector<std::vector<float32_t>> output_buffer;
    Timer TimeCount;
    while (1)
    {
        MnpReturnCode status = pHailoPipeline->ReadOutputById(JoinNetwork.id_name, output_buffer, lprnet_data_id);
        EXPECT_NE(status, MnpReturnCode::NOT_FOUND);
        if (status == MnpReturnCode::SUCCESS)
            break;        

        //Set a timer to make sure we are not in forever loop
        ASSERT_FALSE(TimeCount.isTimePastSec(TIME_OUT_SEC));
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    //We expect 6 output
    EXPECT_EQ(output_buffer.size(), OutputOrderByName.size());

    for (size_t i = 0; i < output_buffer.size(); i++)
    {
        //We expect the first 3 output (which is yolo output) to be 0 size
        //while the last 3 (which is LPRNet) to be non-zero size.
        if (i < 3)
        {
            EXPECT_EQ(output_buffer[i].size(), 0);            
        }
        else
        {
            EXPECT_NE(output_buffer[i].size(), 0);            
        }
    }
    
    /* Infer Yolo */
    std::vector<uint8_t> YoloTestInput(Yolov5sInputSize, 0);
    EXPECT_EQ(pHailoPipeline->Infer(JoinNetwork.id_name, YoloTestInput, yolo_data_id, 0), 
                                    MnpReturnCode::SUCCESS);       

    /* Get Yolo prediction result */
    while (1)
    {
        MnpReturnCode status = pHailoPipeline->ReadOutputById(JoinNetwork.id_name, output_buffer, yolo_data_id);
        EXPECT_NE(status, MnpReturnCode::NOT_FOUND);
        if (status == MnpReturnCode::SUCCESS)
            break;        

        //Set a timer to make sure we are not in forever loop
        ASSERT_FALSE(TimeCount.isTimePastSec(TIME_OUT_SEC));
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    //We expect 6 output
    EXPECT_EQ(output_buffer.size(), OutputOrderByName.size());

    for (size_t i = 0; i < output_buffer.size(); i++)
    {
        //We expect the first 3 output (which is yolo output) to be non-zero size
        //while the last 3 (which is LPRNet) to be 0 size.
        if (i < 3)
        {
            EXPECT_NE(output_buffer[i].size(), 0);            
        }
        else
        {
            EXPECT_EQ(output_buffer[i].size(), 0);            
        }
    }
    
    //Remove last Network
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(JoinNetwork.id_name), MnpReturnCode::SUCCESS);
 
    //Check total size (should be TotalNetwork)
    std::vector<stNetworkModelInfo> list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), 0);
}


TEST_F(MNPTest1, MNP17MergedNetworkinferAndRemove) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);

    std::string yolo_data_id = "yolo";
    std::string lanenet_data_id = "lanenet";

        
    //Add Network
    stNetworkModelInfo JoinNetwork;
    JoinNetwork.id_name = "join_yolov4_lanenet";
    JoinNetwork.hef_path = "Test/Network/joined_yolov4_tiny_lanenet_cs_one_works.hef";
    JoinNetwork.output_order_by_name.clear();
    JoinNetwork.input_order_by_name.clear();        
    JoinNetwork.batch_size = 1;
    JoinNetwork.in_quantized = false;
    JoinNetwork.in_format = HAILO_FORMAT_TYPE_UINT8;
    JoinNetwork.out_quantized = false;
    JoinNetwork.out_format = HAILO_FORMAT_TYPE_FLOAT32;    
    
    std::vector<std::string> InputOrderByName{"yolov4_tiny/input_layer1", "lanenet/input_layer1"};
    JoinNetwork.input_order_by_name = InputOrderByName;

    std::vector<std::string> OutputOrderByName{ "yolov4_tiny/conv19", "yolov4_tiny/conv21", "yolov4_tiny/conv24", 
                                                "lanenet/conv30", "lanenet/conv39" };
    JoinNetwork.output_order_by_name = OutputOrderByName;

    EXPECT_EQ(pHailoPipeline->AddNetwork(0, JoinNetwork), MnpReturnCode::SUCCESS);

    size_t NetworkInputSize = 0;
    size_t Yolov4tinyInputSize = 608*352*3;
    pHailoPipeline->GetNetworkInputSize(JoinNetwork.id_name, NetworkInputSize, 0);
    EXPECT_EQ(NetworkInputSize, Yolov4tinyInputSize);
    //std::cout << "yolov4tiny/input_layer1 input size = " << NetworkInputSize << std::endl;

    size_t LanenetInputSize = 256*128*3;
    pHailoPipeline->GetNetworkInputSize(JoinNetwork.id_name, NetworkInputSize, 1);
    EXPECT_EQ(NetworkInputSize, LanenetInputSize);
    //std::cout << "Lanenet/input_layer1 input size = " << NetworkInputSize << std::endl;

    std::vector<std::vector<float32_t>> output_buffer;
    Timer TimeCount;
    float YoloMaxTimeElapsedRecord = 0.0f;
    float YoloMinTimeElapsedRecord = 100.0f;
    float YoloSumTimeElapsedRecord = 0.0f;
    float YoloCountInfered = 0.0f;
    float LanenetMaxTimeElapsedRecord = 0.0f;
    float LanenetMinTimeElapsedRecord = 100.0f;
    float LanenetSumTimeElapsedRecord = 0.0f;
    float LanenetCountInfered = 0.0f;
    int   TotalInferCountRequired = 1000;
    bool  SkipFirstInference = true;
    while (TotalInferCountRequired)
    {
        TimeCount.reset();
        /* Infer LPRNet */
        std::vector<uint8_t> LanenetTestInput(LanenetInputSize, 0);
        EXPECT_EQ(pHailoPipeline->Infer(JoinNetwork.id_name, LanenetTestInput, lanenet_data_id, 1), 
                                        MnpReturnCode::SUCCESS);       

        while (1)
        {
            MnpReturnCode status = pHailoPipeline->ReadOutputById(JoinNetwork.id_name, output_buffer, lanenet_data_id);
            EXPECT_NE(status, MnpReturnCode::NOT_FOUND);
            if (status == MnpReturnCode::SUCCESS)
                break;        

            //Set a timer to make sure we are not in forever loop
            ASSERT_FALSE(TimeCount.isTimePastSec(TIME_OUT_SEC));
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        if (SkipFirstInference == false)
        {
            float Lanenet_time_elapsed = TimeCount.getElapsedInSec();
            LanenetSumTimeElapsedRecord += Lanenet_time_elapsed;
            LanenetCountInfered++;            
            if (LanenetMaxTimeElapsedRecord < Lanenet_time_elapsed)
                LanenetMaxTimeElapsedRecord = Lanenet_time_elapsed;

            if (LanenetMinTimeElapsedRecord > Lanenet_time_elapsed)
                LanenetMinTimeElapsedRecord = Lanenet_time_elapsed;
        }

        TimeCount.reset();
        /* Infer Yolo */
        std::vector<uint8_t> YoloTestInput(Yolov4tinyInputSize, 0);
        EXPECT_EQ(pHailoPipeline->Infer(JoinNetwork.id_name, YoloTestInput, yolo_data_id, 0), 
                                        MnpReturnCode::SUCCESS);       

        /* Get Yolo prediction result */
        while (1)
        {
            MnpReturnCode status = pHailoPipeline->ReadOutputById(JoinNetwork.id_name, output_buffer, yolo_data_id);
            EXPECT_NE(status, MnpReturnCode::NOT_FOUND);
            if (status == MnpReturnCode::SUCCESS)
                break;        

            //Set a timer to make sure we are not in forever loop
            ASSERT_FALSE(TimeCount.isTimePastSec(TIME_OUT_SEC));
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        if (SkipFirstInference == false)
        {

            float Yolo_time_elapsed = TimeCount.getElapsedInSec();
            YoloSumTimeElapsedRecord += Yolo_time_elapsed;
            YoloCountInfered++;
            if (YoloMaxTimeElapsedRecord < Yolo_time_elapsed)
                YoloMaxTimeElapsedRecord = Yolo_time_elapsed;

            if (YoloMinTimeElapsedRecord > Yolo_time_elapsed)
                YoloMinTimeElapsedRecord = Yolo_time_elapsed;
        }

        TotalInferCountRequired--;
        SkipFirstInference = false;

        //std::cout << "Lanaet: " << Lanenet_time_elapsed << std::endl;
        //std::cout << "Yolo: " << Yolo_time_elapsed << std::endl;

    }

    std::cout << "Lanaet Average: " << LanenetSumTimeElapsedRecord / LanenetCountInfered << std::endl;
    std::cout << "Lanaet Max: " << LanenetMaxTimeElapsedRecord << std::endl;
    std::cout << "Lanaet Min: " << LanenetMinTimeElapsedRecord << std::endl;
    std::cout << "Yolo Average: " << YoloSumTimeElapsedRecord / YoloCountInfered << std::endl;
    std::cout << "Yolo Max: " << YoloMaxTimeElapsedRecord << std::endl;
    std::cout << "Yolo Min: " << YoloMinTimeElapsedRecord << std::endl;

    //Remove last Network
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(JoinNetwork.id_name), MnpReturnCode::SUCCESS);
 
    //Check total size (should be TotalNetwork)
    std::vector<stNetworkModelInfo> list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), 0);
}


#else

TEST_F(MNPTest1, MNP17MergedNetworkinferMultiCOntextAndRemove) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);

    std::string data_id = "join_network";

        
    //Add Network
    stNetworkModelInfo JoinNetwork;
    JoinNetwork.id_name = "join_yolov4tiny_lanenet";
    JoinNetwork.hef_path = "Test/Network/joined_yolov4_tiny_lanenet_two_cs.hef";
    JoinNetwork.output_order_by_name.clear();
    JoinNetwork.input_order_by_name.clear();        
    JoinNetwork.batch_size = 1;
    JoinNetwork.in_quantized = false;
    JoinNetwork.in_format = HAILO_FORMAT_TYPE_UINT8;
    JoinNetwork.out_quantized = false;
    JoinNetwork.out_format = HAILO_FORMAT_TYPE_FLOAT32;    
    

    std::vector<std::string> InputOrderByName{"yolov4_tiny/input_layer1", "lanenet/input_layer1"};
    JoinNetwork.input_order_by_name = InputOrderByName;

    std::vector<std::string> OutputOrderByName{ "yolov4_tiny/conv19", "yolov4_tiny/conv21", "yolov4_tiny/conv24", 
                                                "lanenet/conv30", "lanenet/conv39" };
    JoinNetwork.output_order_by_name = OutputOrderByName;

    EXPECT_EQ(pHailoPipeline->AddNetwork(0, JoinNetwork), MnpReturnCode::SUCCESS);

    size_t NetworkInputSize = 0;
    size_t Yolov4tinyInputSize = 608*352*3;
    pHailoPipeline->GetNetworkInputSize(JoinNetwork.id_name, NetworkInputSize, 0);
    EXPECT_EQ(NetworkInputSize, Yolov4tinyInputSize);
    //std::cout << "yolov4tiny/input_layer1 input size = " << NetworkInputSize << std::endl;

    size_t LanenetInputSize = 256*128*3;
    pHailoPipeline->GetNetworkInputSize(JoinNetwork.id_name, NetworkInputSize, 1);
    EXPECT_EQ(NetworkInputSize, LanenetInputSize);
    //std::cout << "Lanenet/input_layer1 input size = " << NetworkInputSize << std::endl;


    /* Infer Lanenet */
    std::vector<uint8_t> LanenetTestInput(LanenetInputSize, 0);
    EXPECT_EQ(pHailoPipeline->Infer(JoinNetwork.id_name, LanenetTestInput, data_id, 1), 
                                    MnpReturnCode::SUCCESS);       

    /* Infer Yolo */
    std::vector<uint8_t> YoloTestInput(Yolov4tinyInputSize, 0);
    EXPECT_EQ(pHailoPipeline->Infer(JoinNetwork.id_name, YoloTestInput, data_id, 0), 
                                    MnpReturnCode::SUCCESS);       

    /* Get Lanenet prediction result */
    std::vector<std::vector<float32_t>> output_buffer;
    Timer TimeCount;
    while (1)
    {
        MnpReturnCode status = pHailoPipeline->ReadOutputById(JoinNetwork.id_name, output_buffer, data_id);
        EXPECT_NE(status, MnpReturnCode::NOT_FOUND);
        if (status == MnpReturnCode::SUCCESS)
            break;        

        //Set a timer to make sure we are not in forever loop
        ASSERT_FALSE(TimeCount.isTimePastSec(TIME_OUT_SEC));
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    //We expect 5 output
    EXPECT_EQ(output_buffer.size(), OutputOrderByName.size());

    for (size_t i = 0; i < output_buffer.size(); i++)
    {
        //std::cout << output_buffer[i].size() << std::endl;
        EXPECT_NE(output_buffer[i].size(), 0);
    }
    
    //Remove last Network
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(JoinNetwork.id_name), MnpReturnCode::SUCCESS);
 
    //Check total size (should be TotalNetwork)
    std::vector<stNetworkModelInfo> list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), 0);
}

TEST_F(MNPTest1, MNP18MergedNetworkinferMultiCOntextAndRemove) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);

    std::string data_id = "join_network";
        
    //Add Network
    stNetworkModelInfo JoinNetwork;
    JoinNetwork.id_name = "join_yolov4tiny_lanenet";
    JoinNetwork.hef_path = "Test/Network/joined_yolov4_tiny_lanenet_two_cs.hef";
    JoinNetwork.output_order_by_name.clear();
    JoinNetwork.input_order_by_name.clear();        
    JoinNetwork.batch_size = 1;
    JoinNetwork.in_quantized = false;
    JoinNetwork.in_format = HAILO_FORMAT_TYPE_UINT8;
    JoinNetwork.out_quantized = false;
    JoinNetwork.out_format = HAILO_FORMAT_TYPE_FLOAT32;    
    

    std::vector<std::string> InputOrderByName{"yolov4_tiny/input_layer1", "lanenet/input_layer1"};
    JoinNetwork.input_order_by_name = InputOrderByName;

    std::vector<std::string> OutputOrderByName{ "yolov4_tiny/conv19", "yolov4_tiny/conv21", "yolov4_tiny/conv24", 
                                                "lanenet/conv30", "lanenet/conv39" };
    JoinNetwork.output_order_by_name = OutputOrderByName;

    EXPECT_EQ(pHailoPipeline->AddNetwork(0, JoinNetwork), MnpReturnCode::SUCCESS);

    size_t NetworkInputSize = 0;
    size_t Yolov4tinyInputSize = 608*352*3;
    pHailoPipeline->GetNetworkInputSize(JoinNetwork.id_name, NetworkInputSize, 0);
    EXPECT_EQ(NetworkInputSize, Yolov4tinyInputSize);
    //std::cout << "yolov4tiny/input_layer1 input size = " << NetworkInputSize << std::endl;

    size_t LanenetInputSize = 256*128*3;
    pHailoPipeline->GetNetworkInputSize(JoinNetwork.id_name, NetworkInputSize, 1);
    EXPECT_EQ(NetworkInputSize, LanenetInputSize);
    //std::cout << "Lanenet/input_layer1 input size = " << NetworkInputSize << std::endl;

    std::vector<std::vector<float32_t>> output_buffer;
    Timer TimeCount;
    float MaxTimeElapsedRecord = 0.0f;
    float MinTimeElapsedRecord = 100.0f;
    float SumTimeElapsedRecord = 0.0f;
    float CountInfered = 0.0f;
    int   TotalInferCountRequired = 1000;
    bool  SkipFirstInference = true;
    while (TotalInferCountRequired)
    {
        TimeCount.reset();

        /* Infer Yolo */
        std::vector<uint8_t> YoloTestInput(Yolov4tinyInputSize, 0);
        EXPECT_EQ(pHailoPipeline->Infer(JoinNetwork.id_name, YoloTestInput, data_id, 0), 
                                        MnpReturnCode::SUCCESS);       

        /* Infer Lanenet */
        std::vector<uint8_t> LanenetTestInput(LanenetInputSize, 0);
        EXPECT_EQ(pHailoPipeline->Infer(JoinNetwork.id_name, LanenetTestInput, data_id, 1), 
                                        MnpReturnCode::SUCCESS);       

        while (1)
        {
            MnpReturnCode status = pHailoPipeline->ReadOutputById(JoinNetwork.id_name, output_buffer, data_id);
            EXPECT_NE(status, MnpReturnCode::NOT_FOUND);
            if (status == MnpReturnCode::SUCCESS)
                break;        

            //Set a timer to make sure we are not in forever loop
            ASSERT_FALSE(TimeCount.isTimePastSec(TIME_OUT_SEC));
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        if (SkipFirstInference == false)
        {
            float time_elapsed = TimeCount.getElapsedInSec();
            SumTimeElapsedRecord += time_elapsed;
            CountInfered++;
            if (time_elapsed > MaxTimeElapsedRecord)
                MaxTimeElapsedRecord = time_elapsed;
            
            if (MinTimeElapsedRecord > time_elapsed)
                MinTimeElapsedRecord = time_elapsed;
        }
        TotalInferCountRequired--;
        SkipFirstInference = false;
    }

    std::cout << "Max Time Elapsed: " << MaxTimeElapsedRecord << std::endl;
    std::cout << "Min Time Elapsed: " << MinTimeElapsedRecord << std::endl;
    std::cout << "Avg Time Elapsed: " << SumTimeElapsedRecord / CountInfered << std::endl;


    //Remove last Network
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(JoinNetwork.id_name), MnpReturnCode::SUCCESS);
 
    //Check total size (should be TotalNetwork)
    std::vector<stNetworkModelInfo> list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), 0);
}

#endif
#endif