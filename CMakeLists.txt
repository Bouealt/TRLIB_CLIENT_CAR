cmake_minimum_required(VERSION 3.10)

# 定义项目名称
project ("VehicleClientProjV1.0")

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pthread -g -O0 -w -Wno-deprecated-declarations")

# 查找并配置依赖库
find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenCV REQUIRED)
find_package(PCL 1.2 REQUIRED)

# 包含全局包含目录
include_directories(${Boost_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS} ${PCL_INCLUDE_DIRS} /usr/include/boost)

# 包含项目的头文件路径
include_directories(${CMAKE_SOURCE_DIR}/file_transfer ${CMAKE_SOURCE_DIR}/data_collection ${CMAKE_SOURCE_DIR}/shared)

# 获取所有源文件
file(GLOB FILE_TRANSFER_SOURCES ${CMAKE_SOURCE_DIR}/file_transfer/*.cpp)
file(GLOB DATA_COLLECTION_SOURCES ${CMAKE_SOURCE_DIR}/data_collection/*.cpp)
file(GLOB SHARED_SOURCES ${CMAKE_SOURCE_DIR}/shared/*.cpp)

# 定义最终的可执行文件
add_executable(VehicleClient main.cpp ${FILE_TRANSFER_SOURCES} ${DATA_COLLECTION_SOURCES} ${SHARED_SOURCES})

# 链接库到可执行文件
target_link_libraries(VehicleClient ${Boost_LIBRARIES} ${OpenCV_LIBS} ${PCL_LIBRARIES} pthread ssl crypto)
