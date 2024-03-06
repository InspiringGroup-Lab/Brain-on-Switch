# CMake basics
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(IMIS CXX CUDA)
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_BUILD_TYPE "Debug")

add_definitions(-TORCH_USE_CUDA_DSA)

# add dpdk directories
set(PKG_CONFIG_PATH /usr/local/lib/pkgconfig/)
find_package(PkgConfig)
pkg_search_module(LIBDPDK REQUIRED libdpdk)
# MESSAGE(STATUS ${LIBDPDK_LIBRARIES})
# MESSAGE(STATUS ${LIBDPDK_INCLUDE_DIRS})

# Add path_prefix and options for PyTorch C++
execute_process(COMMAND python3 "-c" "import torch;print(torch.utils.cmake_prefix_path)" OUTPUT_VARIABLE FOO)
string(STRIP ${FOO} FOO)
set(CMAKE_PREFIX_PATH ${FOO})

find_package(Torch REQUIRED)
include_directories(${TORCH_INCLUDE_DIRS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

# performance optimization
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mssse3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

# Add sub-directories
add_subdirectory(commune)

# Add the target sorce code files
aux_source_directory(. DIR_SRCS)
add_executable(${PROJECT_NAME} "${DIR_SRCS}")

# Add the libraries dependencies
target_link_libraries(${PROJECT_NAME} commune)
target_link_libraries(${PROJECT_NAME} ${TORCH_LIBRARIES})
