/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   MNP_Statistics.cpp
 * Author: Aaron
 * 
 * Created on July 5, 2021, 11:32 AM
 */

#include <vector>
#include "MultiNetworkPipeline.hpp"
#include "gtest/gtest.h"
#include <opencv2/opencv.hpp>
#include <algorithm>

#define TIME_OUT_SEC    (10)

class MNPStatistic : public testing::Test {
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

TEST_F(MNPStatistic, MNP1Yolov4BatchSize1) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);
    
    // 
    // Infer Yolov4 with different batch size 
    // 
    //    
    
    //Add Network - Yolov4
    stNetworkModelInfo Yolov4Network;
    Yolov4Network.id_name = "yolov4";
    Yolov4Network.hef_path = "Test/Network/yolov4.hef";
    Yolov4Network.output_order_by_name.clear();        
    Yolov4Network.batch_size = 1;
    
    std::vector<std::string> Yolov4OutputOrderByName{"conv133_centers", "conv133_scales", "conv133_obj", "conv133_probs",
                                                     "conv126_centers", "conv126_scales", "conv126_obj", "conv126_probs",
                                                     "conv118_centers", "conv118_scales", "conv118_obj", "conv118_probs" };
    Yolov4Network.output_order_by_name = Yolov4OutputOrderByName;
    
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Yolov4Network), MnpReturnCode::SUCCESS);
    size_t Yolov4InputSize = 512*512*3; //Yolov4 input size
    std::vector<int> Yolov4OutputSizeInOrder{1536,1536,768,61440,
                                             6144,6144,3072,245760,
                                             24576,24576,12288,983040};    
    
                                                                                          
                                           
    
    Timer   StatisticTimer;
    double  CurrentElapsed = 0;
    std::cout << "[STATISTIC] " << "Start YoloV4 Activation + Infer" 
              << ", with batch size = " << Yolov4Network.batch_size <<std::endl;
    
    //Yolov4 Infer up to NumberOfInference  
    {
        std::vector<uint8_t> TestInput(Yolov4InputSize, 0);
        size_t NumberOfInference = 1;
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            EXPECT_EQ(pHailoPipeline->Infer(Yolov4Network.id_name, TestInput), 
                                            MnpReturnCode::SUCCESS);
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
        
        CurrentElapsed = StatisticTimer.getElapsedInSec();
        std::cout << "[STATISTIC] " << "End YoloV4 Infer, time elapsed = " 
                  << CurrentElapsed << ", with total sample of " << NumberOfInference
                  << std::endl;
        std::cout << "[STATISTIC] " << "FPS Performance (including activation time) = " 
                  << 1/(CurrentElapsed/NumberOfInference) << std::endl; 

    }
    
    //Yolov4 Infer to calculate latency, we need to do it separatly since first time
    //inference will include activation and we want to exclude it.
    StatisticTimer.reset();    
    {
        std::vector<uint8_t> TestInput(Yolov4InputSize, 0);
        size_t NumberOfInference = 100;
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            EXPECT_EQ(pHailoPipeline->Infer(Yolov4Network.id_name, TestInput), 
                                            MnpReturnCode::SUCCESS);
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
        
        CurrentElapsed = StatisticTimer.getElapsedInSec();
        std::cout << "[STATISTIC] " << "Latency (excluding activation time) = " 
                  << (CurrentElapsed*1000)/NumberOfInference << "ms" << std::endl; 
        std::cout << "[STATISTIC] " << "FPS Performance (excluding activation time) = " 
                  << 1/(CurrentElapsed/NumberOfInference) << std::endl; 

    }
    
    
    //Remove Network
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(Yolov4Network.id_name), MnpReturnCode::SUCCESS);
 
    //Check total size (should be TotalNetwork)
    std::vector<stNetworkModelInfo> list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), 0); 
    
}

TEST_F(MNPStatistic, MNP1Yolov4BatchSize4) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);
    
    // 
    // Infer Yolov4 with different batch size 
    // 
    //    
    
    //Add Network - Yolov4
    stNetworkModelInfo Yolov4Network;
    Yolov4Network.id_name = "yolov4";
    Yolov4Network.hef_path = "Test/Network/yolov4.hef";
    Yolov4Network.output_order_by_name.clear();        
    Yolov4Network.batch_size = 4;
    
    std::vector<std::string> Yolov4OutputOrderByName{"conv133_centers", "conv133_scales", "conv133_obj", "conv133_probs",
                                                     "conv126_centers", "conv126_scales", "conv126_obj", "conv126_probs",
                                                     "conv118_centers", "conv118_scales", "conv118_obj", "conv118_probs" };
    Yolov4Network.output_order_by_name = Yolov4OutputOrderByName;
    
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Yolov4Network), MnpReturnCode::SUCCESS);
    size_t Yolov4InputSize = 512*512*3; //Yolov4 input size
    std::vector<int> Yolov4OutputSizeInOrder{1536,1536,768,61440,
                                             6144,6144,3072,245760,
                                             24576,24576,12288,983040};    
    
                                                                                          
                                           
    
    Timer   StatisticTimer;
    double  CurrentElapsed = 0;
    std::cout << "[STATISTIC] " << "Start YoloV4 Activation + Infer" 
              << ", with batch size = " << Yolov4Network.batch_size <<std::endl;
    
    //Yolov4 Infer up to NumberOfInference  
    {
        std::vector<uint8_t> TestInput(Yolov4InputSize, 0);
        size_t NumberOfInference = 120;
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            EXPECT_EQ(pHailoPipeline->Infer(Yolov4Network.id_name, TestInput), 
                                            MnpReturnCode::SUCCESS);
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
        
        CurrentElapsed = StatisticTimer.getElapsedInSec();
        std::cout << "[STATISTIC] " << "End YoloV4 Infer, time elapsed = " 
                  << CurrentElapsed << ", with total sample of " << NumberOfInference
                  << std::endl;
        std::cout << "[STATISTIC] " << "FPS Performance (including activation time) = " 
                  << 1/(CurrentElapsed/NumberOfInference) << std::endl; 

    }
    
    //Yolov4 Infer to calculate latency, we need to do it separatly since first time
    //inference will include activation and we want to exclude it.
    StatisticTimer.reset();    
    {
        std::vector<uint8_t> TestInput(Yolov4InputSize, 0);
        size_t NumberOfInference = 4;
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            EXPECT_EQ(pHailoPipeline->Infer(Yolov4Network.id_name, TestInput), 
                                            MnpReturnCode::SUCCESS);
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

            if (i == 0)
            {
                double FirstOutputElapsed = StatisticTimer.getElapsedInSec();
                std::cout << "[STATISTIC] " << "Latency First Output (excluding activation time) = " 
                          << FirstOutputElapsed*1000 << "ms" << std::endl;
            }
            
            EXPECT_EQ(output_buffer.size(), Yolov4OutputSizeInOrder.size());

            for (size_t i = 0; i < output_buffer.size(); i++)
            {
                EXPECT_EQ(output_buffer[i].size(), Yolov4OutputSizeInOrder[i]);
            }

        }
        
        CurrentElapsed = StatisticTimer.getElapsedInSec();
        std::cout << "[STATISTIC] " << "Latency batch size (excluding activation time) = " 
                  << (CurrentElapsed*1000)/NumberOfInference << "ms" << std::endl; 
        std::cout << "[STATISTIC] " << "FPS Performance (excluding activation time) = " 
                  << 1/(CurrentElapsed/NumberOfInference) << std::endl; 

    }
    
    
    //Remove Network
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(Yolov4Network.id_name), MnpReturnCode::SUCCESS);
 
    //Check total size (should be TotalNetwork)
    std::vector<stNetworkModelInfo> list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), 0); 
    
}

TEST_F(MNPStatistic, MNP1Yolov4BatchSize8) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);
    
    // 
    // Infer Yolov4 with different batch size 
    // 
    //    
    
    //Add Network - Yolov4
    stNetworkModelInfo Yolov4Network;
    Yolov4Network.id_name = "yolov4";
    Yolov4Network.hef_path = "Test/Network/yolov4.hef";
    Yolov4Network.output_order_by_name.clear();        
    Yolov4Network.batch_size = 8;
    
    std::vector<std::string> Yolov4OutputOrderByName{"conv133_centers", "conv133_scales", "conv133_obj", "conv133_probs",
                                                     "conv126_centers", "conv126_scales", "conv126_obj", "conv126_probs",
                                                     "conv118_centers", "conv118_scales", "conv118_obj", "conv118_probs" };
    Yolov4Network.output_order_by_name = Yolov4OutputOrderByName;
    
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Yolov4Network), MnpReturnCode::SUCCESS);
    size_t Yolov4InputSize = 512*512*3; //Yolov4 input size
    std::vector<int> Yolov4OutputSizeInOrder{1536,1536,768,61440,
                                             6144,6144,3072,245760,
                                             24576,24576,12288,983040};    
    
                                                                                          
                                           
    
    Timer   StatisticTimer;
    double  CurrentElapsed = 0;
    std::cout << "[STATISTIC] " << "Start YoloV4 Activation + Infer" 
              << ", with batch size = " << Yolov4Network.batch_size <<std::endl;
    
    //Yolov4 Infer up to NumberOfInference  
    {
        std::vector<uint8_t> TestInput(Yolov4InputSize, 0);
        size_t NumberOfInference = 160;
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            EXPECT_EQ(pHailoPipeline->Infer(Yolov4Network.id_name, TestInput), 
                                            MnpReturnCode::SUCCESS);
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
        
        CurrentElapsed = StatisticTimer.getElapsedInSec();
        std::cout << "[STATISTIC] " << "End YoloV4 Infer, time elapsed = " 
                  << CurrentElapsed << ", with total sample of " << NumberOfInference
                  << std::endl;
        std::cout << "[STATISTIC] " << "FPS Performance (including activation time) = " 
                  << 1/(CurrentElapsed/NumberOfInference) << std::endl; 

    }
    
    //Yolov4 Infer to calculate latency, we need to do it separatly since first time
    //inference will include activation and we want to exclude it.
    StatisticTimer.reset();    
    {
        std::vector<uint8_t> TestInput(Yolov4InputSize, 0);
        size_t NumberOfInference = 8;
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            EXPECT_EQ(pHailoPipeline->Infer(Yolov4Network.id_name, TestInput), 
                                            MnpReturnCode::SUCCESS);
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

            if (i == 0)
            {
                double FirstOutputElapsed = StatisticTimer.getElapsedInSec();
                std::cout << "[STATISTIC] " << "Latency First Output (excluding activation time) = " 
                          << FirstOutputElapsed*1000 << "ms" << std::endl;
            }
            
            EXPECT_EQ(output_buffer.size(), Yolov4OutputSizeInOrder.size());

            for (size_t i = 0; i < output_buffer.size(); i++)
            {
                EXPECT_EQ(output_buffer[i].size(), Yolov4OutputSizeInOrder[i]);
            }

        }
        
        CurrentElapsed = StatisticTimer.getElapsedInSec();
        std::cout << "[STATISTIC] " << "Latency batch size (excluding activation time) = " 
                  << CurrentElapsed*1000 << "ms" << std::endl; 
        std::cout << "[STATISTIC] " << "FPS Performance (excluding activation time) = " 
                  << 1/(CurrentElapsed/NumberOfInference) << std::endl; 

    }
    
    
    //Remove Network
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(Yolov4Network.id_name), MnpReturnCode::SUCCESS);
 
    //Check total size (should be TotalNetwork)
    std::vector<stNetworkModelInfo> list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), 0); 
    
}

TEST_F(MNPStatistic, MNP2MultipleNetworkInfer) {
    
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
    
    std::vector<std::string> Yolov5mOutputOrderByName{"conv107", "conv97", "conv87"};
    Yolov5mNetwork.output_order_by_name = Yolov5mOutputOrderByName;
    
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Yolov5mNetwork), MnpReturnCode::SUCCESS);
    size_t Yolov5mInputSize = 640*640*3; //Yolov5m input size
    std::vector<int> Yolov5mOutputSizeInOrder{  20*20*255,
                                                40*40*255,
                                                80*80*255}; 
                                                
    //Add Network - Yolov5s
    stNetworkModelInfo Yolov5sNetwork;
    Yolov5sNetwork.id_name = "yolov5s";
    Yolov5sNetwork.hef_path = "Test/Network/yolov5s.hef";
    Yolov5sNetwork.output_order_by_name.clear();        
    Yolov5sNetwork.batch_size = 1; 
    
    std::vector<std::string> Yolov5sOutputOrderByName{"conv76", "conv68", "conv60"};
    Yolov5sNetwork.output_order_by_name = Yolov5sOutputOrderByName;
    
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, Yolov5sNetwork), MnpReturnCode::SUCCESS);
    size_t Yolov5sInputSize = 640*640*3; //Yolov5s input size
    std::vector<int> Yolov5sOutputSizeInOrder{  20*20*255,
                                                40*40*255,
                                                80*80*255};                                                 
    
    Timer   StatisticTimer;
    double  LastElapsed = 0;
    double  CurrentElapsed = 0;
    std::cout << "[STATISTIC] " << "Start YoloV4 Activation + Infer" << std::endl;
    
    //Yolov4 Infer up to NumberOfInference  
    {
        std::vector<uint8_t> TestInput(Yolov4InputSize, 0);
        size_t NumberOfInference = 1;
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            EXPECT_EQ(pHailoPipeline->Infer(Yolov4Network.id_name, TestInput), 
                                            MnpReturnCode::SUCCESS);
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
    
    
    CurrentElapsed = StatisticTimer.getElapsedInSec();
    std::cout << "[STATISTIC] " << "End YoloV4 Infer, time elapsed = " <<  
                 CurrentElapsed << ", Diff Since Last = " << CurrentElapsed - LastElapsed <<std::endl;
    LastElapsed = CurrentElapsed;
    std::cout << "[STATISTIC] " << "Start Yolov5m Infer" << std::endl;
        
    
    //Yolov5m Infer up to NumberOfInference  
    {
        std::vector<uint8_t> TestInput(Yolov5mInputSize, 0);
        size_t NumberOfInference = 1;
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            EXPECT_EQ(pHailoPipeline->Infer(Yolov5mNetwork.id_name, TestInput), 
                                            MnpReturnCode::SUCCESS);
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
    
    CurrentElapsed = StatisticTimer.getElapsedInSec();
    std::cout << "[STATISTIC] " << "End YoloV5m Infer, time elapsed = " <<  
                 CurrentElapsed << ", Diff Since Last = " << CurrentElapsed - LastElapsed <<std::endl;
    LastElapsed = CurrentElapsed;
    
    std::cout << "[STATISTIC] " << "Start Yolov5s Infer" << std::endl;

    //Yolov5s Infer up to NumberOfInference  
    {
        std::vector<uint8_t> TestInput(Yolov5sInputSize, 0);
        size_t NumberOfInference = 1;
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            EXPECT_EQ(pHailoPipeline->Infer(Yolov5sNetwork.id_name, TestInput), 
                                            MnpReturnCode::SUCCESS);
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
                MnpReturnCode status = pHailoPipeline->ReadOutputById(Yolov5sNetwork.id_name, output_buffer);
                EXPECT_NE(status, MnpReturnCode::NOT_FOUND);
                if (status == MnpReturnCode::SUCCESS)
                    break;

                //Set a timer to make sure we are not in forever loop
                ASSERT_FALSE(TimeCount.isTimePastSec(TIME_OUT_SEC));
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }

            EXPECT_EQ(output_buffer.size(), Yolov5sOutputSizeInOrder.size());

            for (size_t i = 0; i < output_buffer.size(); i++)
            {
                EXPECT_EQ(output_buffer[i].size(), Yolov5sOutputSizeInOrder[i]);
            }
        }
    }
    
    CurrentElapsed = StatisticTimer.getElapsedInSec();
    std::cout << "[STATISTIC] " << "End YoloV5s Infer, time elapsed = " <<  
                 CurrentElapsed << ", Diff Since Last = " << CurrentElapsed - LastElapsed <<std::endl;
    LastElapsed = CurrentElapsed;

        
    //Remove Network
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(Yolov4Network.id_name), MnpReturnCode::SUCCESS);
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(Yolov5mNetwork.id_name), MnpReturnCode::SUCCESS);
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(Yolov5sNetwork.id_name), MnpReturnCode::SUCCESS);
 
    //Check total size (should be TotalNetwork)
    std::vector<stNetworkModelInfo> list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), 0); 
    
}

/* No required as we can already run this with tool, but we will leave it here
 * in case we need it for quick test in the future.
 
TEST_F(MNPStatistic, MNP2MultipleNetworkInfer2) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);
    
    // 
    // Infer Centerpose no compression -> Centerpose compression
    // 
    //    
    
    //Add Network - Centerpose (No compression)
    stNetworkModelInfo CenterPoseNoCompressNetwork;
    CenterPoseNoCompressNetwork.id_name = "centerpose_no_compress";
    CenterPoseNoCompressNetwork.hef_path = "Test/Network/centerpose_regnetx_1.6gf_fpn.hef";
    CenterPoseNoCompressNetwork.output_order_by_name.clear();        
    CenterPoseNoCompressNetwork.batch_size = 1;
    std::vector<std::string> CenterPoseNoCompressOutputOrderByName{ "center_nms/conv2", 
                                                                    "centerpose_regnetx_1.6gf_fpn/conv79", 
                                                                    "centerpose_regnetx_1.6gf_fpn/conv80", 
                                                                    "centerpose_regnetx_1.6gf_fpn/conv81", 
                                                                    "joint_nms/conv2", 
                                                                    "centerpose_regnetx_1.6gf_fpn/conv83"};
                                                                    
    CenterPoseNoCompressNetwork.output_order_by_name = CenterPoseNoCompressOutputOrderByName;

    
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, CenterPoseNoCompressNetwork), MnpReturnCode::SUCCESS);
    size_t CenterPoseNoCompressInputSize = 640*640*3; //Input size
    std::vector<int> CenterPoseNoCompressOutputSizeInOrder{ 160*160*1,160*160*2,
                                                            160*160*34,160*160*2,
                                                            160*160*17,160*160*2};    
    

    //Add Network - Centerpose (With compression)
    stNetworkModelInfo CenterPoseWithCompressNetwork;
    CenterPoseWithCompressNetwork.id_name = "centerpose_with_compress";
    CenterPoseWithCompressNetwork.hef_path = "Test/Network/centerpose_regnetx_1.6gf_fpn_pcie_compress.hef";
    CenterPoseWithCompressNetwork.output_order_by_name.clear();        
    CenterPoseWithCompressNetwork.batch_size = 1;

    std::vector<std::string> CenterPoseWithCompressOutputOrderByName{ "center_nms/conv2", 
                                                                    "centerpose_regnetx_1.6gf_fpn/conv79", 
                                                                    "centerpose_regnetx_1.6gf_fpn/conv80", 
                                                                    "centerpose_regnetx_1.6gf_fpn/conv81", 
                                                                    "joint_nms/conv2", 
                                                                    "centerpose_regnetx_1.6gf_fpn/conv83"};
                                                                    
    CenterPoseWithCompressNetwork.output_order_by_name = CenterPoseWithCompressOutputOrderByName;
    
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, CenterPoseWithCompressNetwork), MnpReturnCode::SUCCESS);
    size_t CenterPoseWithCompressInputSize = 640*640*3; //Input size
    std::vector<int> CenterPoseWithCompressOutputSizeInOrder{ 160*160*1,160*160*2,
                                                              160*160*34,160*160*2,
                                                              160*160*17,160*160*2};    
                                                                                                             
    
    Timer   StatisticTimer;
    double  LastElapsed = 0;
    double  CurrentElapsed = 0;
    std::cout << "[STATISTIC] " << "Start CenterPose (No Compression) Activation + Infer" << std::endl;
    
    //Infer up to NumberOfInference  
    {
        std::vector<uint8_t> TestInput(CenterPoseNoCompressInputSize, 0);
        size_t NumberOfInference = 1;
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            EXPECT_EQ(pHailoPipeline->Infer(CenterPoseNoCompressNetwork.id_name, TestInput), 
                                            MnpReturnCode::SUCCESS);
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
                MnpReturnCode status = pHailoPipeline->ReadOutputById(CenterPoseNoCompressNetwork.id_name, output_buffer);
                EXPECT_NE(status, MnpReturnCode::NOT_FOUND);
                if (status == MnpReturnCode::SUCCESS)
                    break;

                //Set a timer to make sure we are not in forever loop
                ASSERT_FALSE(TimeCount.isTimePastSec(TIME_OUT_SEC));
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }

            EXPECT_EQ(output_buffer.size(), CenterPoseNoCompressOutputSizeInOrder.size());

            for (size_t i = 0; i < output_buffer.size(); i++)
            {
                EXPECT_EQ(output_buffer[i].size(), CenterPoseNoCompressOutputSizeInOrder[i]);
            }

        }
    }
    
    
    CurrentElapsed = StatisticTimer.getElapsedInSec();
    std::cout << "[STATISTIC] " << "End CenterPose (No Compress) Infer, time elapsed = " <<  
                 CurrentElapsed << ", Diff Since Last = " << CurrentElapsed - LastElapsed <<std::endl;
    LastElapsed = CurrentElapsed;
    std::cout << "[STATISTIC] " << "Start CenterPose (With Compress) Infer" << std::endl;
        
    
    //Infer up to NumberOfInference  
    {
        std::vector<uint8_t> TestInput(CenterPoseWithCompressInputSize, 0);
        size_t NumberOfInference = 1;
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            EXPECT_EQ(pHailoPipeline->Infer(CenterPoseWithCompressNetwork.id_name, TestInput), 
                                            MnpReturnCode::SUCCESS);
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
                MnpReturnCode status = pHailoPipeline->ReadOutputById(CenterPoseWithCompressNetwork.id_name, output_buffer);
                EXPECT_NE(status, MnpReturnCode::NOT_FOUND);
                if (status == MnpReturnCode::SUCCESS)
                    break;

                //Set a timer to make sure we are not in forever loop
                ASSERT_FALSE(TimeCount.isTimePastSec(TIME_OUT_SEC));
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }

            EXPECT_EQ(output_buffer.size(), CenterPoseWithCompressOutputSizeInOrder.size());

            for (size_t i = 0; i < output_buffer.size(); i++)
            {
                EXPECT_EQ(output_buffer[i].size(), CenterPoseWithCompressOutputSizeInOrder[i]);
            }
        }
    }
    
    CurrentElapsed = StatisticTimer.getElapsedInSec();
    std::cout << "[STATISTIC] " << "End CenterPose (With Compress) Infer, time elapsed = " <<  
                 CurrentElapsed << ", Diff Since Last = " << CurrentElapsed - LastElapsed <<std::endl;
    LastElapsed = CurrentElapsed;
    

        
    //Remove Network
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(CenterPoseNoCompressNetwork.id_name), MnpReturnCode::SUCCESS);
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(CenterPoseWithCompressNetwork.id_name), MnpReturnCode::SUCCESS);
 
    //Check total size (should be TotalNetwork)
    std::vector<stNetworkModelInfo> list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), 0); 
    
}


TEST_F(MNPStatistic, MNP2MultipleNetworkInfer3) {
    
    MultiNetworkPipeline *pHailoPipeline = MultiNetworkPipeline::GetInstance();
    ASSERT_NE(pHailoPipeline, nullptr);
    
    // 
    // Infer RetinaFace no compression -> RetinaFace compression
    // NOTE: Looks like both file has no difference and the HEF size is the same
    //       May need to check if retinaface has been generated correctly with
    //       the right allocator file
    // 
    
    //Add Network - RetinaFace (No compression)
    stNetworkModelInfo RetinaFaceNoCompressNetwork;
    RetinaFaceNoCompressNetwork.id_name = "retinaface_no_compress";
    RetinaFaceNoCompressNetwork.hef_path = "Test/Network/retinaface_mobilenet_v1_0.25.hef";
    RetinaFaceNoCompressNetwork.output_order_by_name.clear();        
    RetinaFaceNoCompressNetwork.batch_size = 1;
    
    std::vector<std::string> RetinaFaceNoCompressOutputOrderByName{"conv43", "conv44", 
                                                                   "conv45", "conv33", 
                                                                   "conv34", "conv35", 
                                                                   "conv23", "conv24", 
                                                                   "conv25"};
                                                                    
    RetinaFaceNoCompressNetwork.output_order_by_name = RetinaFaceNoCompressOutputOrderByName;
    
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, RetinaFaceNoCompressNetwork), MnpReturnCode::SUCCESS);
    size_t RetinaFaceNoCompressInputSize = 480*640*3; //Input size
    std::vector<int> RetinaFaceNoCompressOutputSizeInOrder{ 60*80*8,60*80*4,60*80*20,
                                                            30*40*8,30*40*4,30*40*20,
                                                            15*20*8,15*20*4,15*20*20};    
    

    //Add Network - RetinaFace (With compression)
    stNetworkModelInfo RetinaFaceWithCompressNetwork;
    RetinaFaceWithCompressNetwork.id_name = "retinaface_with_compress";
    RetinaFaceWithCompressNetwork.hef_path = "Test/Network/retinaface_mobilenet_v1_0.25_pcie_compress.hef";
    RetinaFaceWithCompressNetwork.output_order_by_name.clear();        
    RetinaFaceWithCompressNetwork.batch_size = 1;
    
    
    
    std::vector<std::string> RetinaFaceWithCompressOutputOrderByName{"conv43", "conv44", 
                                                                     "conv45", "conv33", 
                                                                     "conv34", "conv35", 
                                                                     "conv23", "conv24", 
                                                                     "conv25"};
                                                                    
    RetinaFaceWithCompressNetwork.output_order_by_name = RetinaFaceWithCompressOutputOrderByName;
    
    
    EXPECT_EQ(pHailoPipeline->AddNetwork(0, RetinaFaceWithCompressNetwork), MnpReturnCode::SUCCESS);
    size_t RetinaFaceWithCompressInputSize = 480*640*3; //Input size
    std::vector<int> RetinaFaceWithCompressOutputSizeInOrder{ 60*80*8,60*80*4,60*80*20,
                                                              30*40*8,30*40*4,30*40*20,
                                                              15*20*8,15*20*4,15*20*20};    
    
                                                                                                             
    
    Timer   StatisticTimer;
    double  LastElapsed = 0;
    double  CurrentElapsed = 0;
    std::cout << "[STATISTIC] " << "Start CenterPose (No Compression) Activation + Infer" << std::endl;
    
    //Infer up to NumberOfInference  
    {
        std::vector<uint8_t> TestInput(RetinaFaceNoCompressInputSize, 0);
        size_t NumberOfInference = 1;
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            EXPECT_EQ(pHailoPipeline->Infer(RetinaFaceNoCompressNetwork.id_name, TestInput), 
                                            MnpReturnCode::SUCCESS);
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
                MnpReturnCode status = pHailoPipeline->ReadOutputById(RetinaFaceNoCompressNetwork.id_name, output_buffer);
                EXPECT_NE(status, MnpReturnCode::NOT_FOUND);
                if (status == MnpReturnCode::SUCCESS)
                    break;

                //Set a timer to make sure we are not in forever loop
                ASSERT_FALSE(TimeCount.isTimePastSec(TIME_OUT_SEC));
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }

            EXPECT_EQ(output_buffer.size(), RetinaFaceNoCompressOutputSizeInOrder.size());

            for (size_t i = 0; i < output_buffer.size(); i++)
            {
                EXPECT_EQ(output_buffer[i].size(), RetinaFaceNoCompressOutputSizeInOrder[i]);
            }

        }
    }
    
    
    CurrentElapsed = StatisticTimer.getElapsedInSec();
    std::cout << "[STATISTIC] " << "End RetinaFace (No Compress) Infer, time elapsed = " <<  
                 CurrentElapsed << ", Diff Since Last = " << CurrentElapsed - LastElapsed <<std::endl;
    LastElapsed = CurrentElapsed;
    std::cout << "[STATISTIC] " << "Start RetinaFace (With Compress) Infer" << std::endl;
        
    
    //Infer up to NumberOfInference  
    {
        std::vector<uint8_t> TestInput(RetinaFaceWithCompressInputSize, 0);
        size_t NumberOfInference = 1;
        for (size_t i = 0; i < NumberOfInference; i++)
        {
            EXPECT_EQ(pHailoPipeline->Infer(RetinaFaceWithCompressNetwork.id_name, TestInput), 
                                            MnpReturnCode::SUCCESS);
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
                MnpReturnCode status = pHailoPipeline->ReadOutputById(RetinaFaceWithCompressNetwork.id_name, output_buffer);
                EXPECT_NE(status, MnpReturnCode::NOT_FOUND);
                if (status == MnpReturnCode::SUCCESS)
                    break;

                //Set a timer to make sure we are not in forever loop
                ASSERT_FALSE(TimeCount.isTimePastSec(TIME_OUT_SEC));
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }

            EXPECT_EQ(output_buffer.size(), RetinaFaceWithCompressOutputSizeInOrder.size());

            for (size_t i = 0; i < output_buffer.size(); i++)
            {
                EXPECT_EQ(output_buffer[i].size(), RetinaFaceWithCompressOutputSizeInOrder[i]);
            }
        }
    }
    
    CurrentElapsed = StatisticTimer.getElapsedInSec();
    std::cout << "[STATISTIC] " << "End RetinaFace (With Compress) Infer, time elapsed = " <<  
                 CurrentElapsed << ", Diff Since Last = " << CurrentElapsed - LastElapsed <<std::endl;
    LastElapsed = CurrentElapsed;
    

        
    //Remove Network
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(RetinaFaceNoCompressNetwork.id_name), MnpReturnCode::SUCCESS);
    EXPECT_EQ(pHailoPipeline->RemoveNetwork(RetinaFaceWithCompressNetwork.id_name), MnpReturnCode::SUCCESS);
 
    //Check total size (should be TotalNetwork)
    std::vector<stNetworkModelInfo> list = pHailoPipeline->GetAllAddedNetworkList();    
    EXPECT_EQ(list.size(), 0); 
    
} 
*/