# CMake basics
# Note that the Pytorch 2.0.1 must use C++ 14
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(commune)
set(CMAKE_CXX_STANDARD 14)

# Add library src
aux_source_directory(. DIR_LIB_SRCS)
add_library (${PROJECT_NAME} ${DIR_LIB_SRCS})

# 3. Link Pcapplusplus static dependencies
target_link_libraries(${PROJECT_NAME} 
        libPcap++.a
        libPacket++.a
        libCommon++.a
)


# 2. Link DPDK libraries
target_link_libraries(${PROJECT_NAME} 
		${LIBDPDK_LDFLAGS}
		-Wl,--no-whole-archive
)


# 1. Link Basic System Libraries
target_link_libraries(${PROJECT_NAME} 
        pthread
        rt
        dl
        m
        numa
        pcap
)


# 0. Link Basic System Libraries
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(${PROJECT_NAME} OpenMP::OpenMP_CXX)
endif()