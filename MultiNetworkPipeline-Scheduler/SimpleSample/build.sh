#!/bin/bash

# Set the target architecture and compiler path for the selected target architecture
TARGET_ARCH=x86_64 #One of x86_64, aarch64, or armv7l
declare -A COMPILER=( [x86_64]=/usr/bin/gcc
                      [aarch64]=/usr/bin/aarch64-linux-gnu-gcc 
                      [armv7l]=/usr/bin/arm-linux-gnueabi-gcc )

# Set path to hailort, please see README.md for detail
HAILORT_VERSION=4.13.0
HAILORT_INCLUDE=/usr/include/hailo/
HAILORT_LIB=/usr/lib/


# Set platform from the following
# 1 - GENERIC : OpenCV videocapture
# 2 - GSTREAMER : Reserve for future, not yet supported (WARNING: depending on specific platform, search for 'USE_GSTREAMER' in main.cpp and make adjustment for target platform)
PLATFORM_SEL=GENERIC

for ARCH in ${TARGET_ARCH}
do

    echo "-I- Building ${ARCH}"
    mkdir -p build/${ARCH}
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${HAILORT_ROOT}/lib/${ARCH}
    # export CXX=g++-9  #May require changes as well (eg, embedded system g++ version 9 is still g++ without the "-9" version specified at the end)
    PLATFORM_SEL=${PLATFORM_SEL} LIB_VER=${HAILORT_VERSION} HAILORT_INCLUDE=${HAILORT_INCLUDE} HAILORT_LIB=${HAILORT_LIB} cmake -H. -Bbuild/${ARCH} -DARCH=${ARCH} -DCMAKE_C_COMPILER=${COMPILER[${ARCH}]}
    PLATFORM_SEL=${PLATFORM_SEL} LIB_VER=${HAILORT_VERSION} HAILORT_INCLUDE=${HAILORT_INCLUDE} HAILORT_LIB=${HAILORT_LIB} cmake --build build/${ARCH}

done
if [[ -f "hailort.log" ]]; then
    rm hailort.log
fi
