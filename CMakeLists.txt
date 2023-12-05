cmake_minimum_required(VERSION 3.0)
project(yolo_server)
set(CMAKE_CXX_STANDARD 11)
	
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

find_package(CUDA REQUIRED)
find_package(OpenCV REQUIRED)

# /usr/lib/x86_64-linux-gnu/libnvinfer.so
find_library(NVINFER NAMES nvinfer)

# /usr/lib/x86_64-linux-gnu/libnvinfer_plugin.so
find_library(NVINFER_PLUGIN NAMES nvinfer_plugin)

include_directories(${CUDA_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/inc)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SRC_LIST)
#file(GLOB SRC_LIST1 ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cu)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/hc_lib)	
link_directories(/home/user/dyn/YoloServer/hc_lib)	
link_directories(/usr/local/lib)	
link_directories(/usr/lib)	

cuda_add_executable(${PROJECT_NAME} main.cpp ${SRC_LIST} ) 

target_link_libraries(${PROJECT_NAME} pthread PlayCtrl hcnetsdk json mxml ${NVINFER} ${NVINFER_PLUGIN} ${CUDA_LIBRARIES} ${OpenCV_LIBRARIES})







