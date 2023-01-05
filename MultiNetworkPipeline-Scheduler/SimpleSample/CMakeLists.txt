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


###############################################################################
## target definitions for shared/static library ###############################
###############################################################################


set(MULTINETWORKLIB_LIB_SRC ../MultiNetworkPipeline-scheduler.cpp)
include_directories(../Utils)

#add_library(multi_network_pipeline SHARED ${MULTINETWORKLIB_LIB_SRC})
add_library(multi_network_pipeline STATIC ${MULTINETWORKLIB_LIB_SRC})
set_property(TARGET multi_network_pipeline PROPERTY POSITION_INDEPENDENT_CODE ON)


foreach(target runme_test.$ENV{LIB_VER})
    add_executable(${target} main_test.cpp) 
                                                          
    target_compile_options(${target} PRIVATE ${COMPILE_OPTIONS_CPP})
    target_link_libraries(${target} ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(${target} multi_network_pipeline -lpthread -lm)
    target_link_libraries(${target} ${HAILORT_LIB})    
endforeach(target)
