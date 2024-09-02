cmake_minimum_required(VERSION 3.10)

# ������Ŀ����
project ("VehicleClientProjV1.0")

# ����C++��׼
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pthread -g -O0 -w -Wno-deprecated-declarations")

# ���Ҳ�����������
find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenCV REQUIRED)
find_package(PCL 1.2 REQUIRED)

# ����ȫ�ְ���Ŀ¼
include_directories(${Boost_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS} ${PCL_INCLUDE_DIRS} /usr/include/boost)

# ������Ŀ��ͷ�ļ�·��
include_directories(${CMAKE_SOURCE_DIR}/file_transfer ${CMAKE_SOURCE_DIR}/data_collection ${CMAKE_SOURCE_DIR}/shared)

# ��ȡ����Դ�ļ�
file(GLOB FILE_TRANSFER_SOURCES ${CMAKE_SOURCE_DIR}/file_transfer/*.cpp)
file(GLOB DATA_COLLECTION_SOURCES ${CMAKE_SOURCE_DIR}/data_collection/*.cpp)
file(GLOB SHARED_SOURCES ${CMAKE_SOURCE_DIR}/shared/*.cpp)

# �������յĿ�ִ���ļ�
add_executable(VehicleClient main.cpp ${FILE_TRANSFER_SOURCES} ${DATA_COLLECTION_SOURCES} ${SHARED_SOURCES})

# ���ӿ⵽��ִ���ļ�
target_link_libraries(VehicleClient ${Boost_LIBRARIES} ${OpenCV_LIBS} ${PCL_LIBRARIES} pthread ssl crypto)