cmake_minimum_required(VERSION 3.1)
project(cpp_agent)
        
message(STATUS "HAILORT_ROOT: $ENV{HAILORT_ROOT}")
set(HAILORT_ROOT $ENV{HAILORT_ROOT})

#set(HAILORT_LIB $ENV{HAILORT_ROOT}/lib/${ARCH}/libhailort.so.$ENV{LIB_VER})
set(HAILORT_LIB /usr/lib/libhailort.so.$ENV{LIB_VER})


#set(HAILORT_INCLUDE_DIR "$ENV{HAILORT_ROOT}/include")
set(HAILORT_INCLUDE_DIR "/usr/include/hailo")

set(COMPILE_OPTIONS_CPP -Werror -g  -O0 -std=c++11)
# set(COMPILE_OPTIONS_CPP -Wall -Werror -O3 -DNDEBUG  -std=c++11)
set(COMPILE_OPTIONS_CPP -Wall -Werror -O3 -g -std=c++11)

include_directories(${HAILORT_INCLUDE_DIR} ./)

find_package(Threads)
find_package( OpenCV REQUIRED )

set(PLATFORM_SEL $ENV{PLATFORM_SEL})
if (PLATFORM_SEL STREQUAL "IMX8_QUAD_MAX")
add_definitions(-DPLATFORM_IS_IMX8QUADMAX)
endif()

###############################################################################
## target definitions for shared/static library ###############################
###############################################################################

#WARNING: This should be removed for actual project build, it is used only for testing purposes in unit test
add_definitions(-DLARGE_INFER_QUEUE_FOR_UNIT_TEST)


set(MULTINETWORKLIB_LIB_SRC MultiNetworkPipeline-scheduler.cpp)
include_directories(./Utils)

#add_library(multi_network_pipeline SHARED ${MULTINETWORKLIB_LIB_SRC})
add_library(multi_network_pipeline STATIC ${MULTINETWORKLIB_LIB_SRC})
set_property(TARGET multi_network_pipeline PROPERTY POSITION_INDEPENDENT_CODE ON)


foreach(target runme_test.$ENV{LIB_VER})
    add_executable(${target} main_test.cpp 
                             Test/MNP_Unittest.cpp)
                             #Test/MNP_Statistics.cpp)
    # https://github.com/llohse/libnpy
    # target_include_directories(${target} PUBLIC /home/nadave/repos/libnpy)
    target_compile_options(${target} PRIVATE ${COMPILE_OPTIONS_CPP})
    target_link_libraries(${target} ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(${target} ${OpenCV_LIBS})
    target_link_libraries(${target} multi_network_pipeline -lpthread -lm)
    target_link_libraries(${target} ${HAILORT_LIB})    
    target_link_libraries(${target} gtest -lpthread -lm) #Google Test Library
endforeach(target)