#include "DataProcessing.h"
#include "../shared/SharedQueue.h" // 假设 SharedQueue.h 包含在主项目中
#include <filesystem>
#include <regex>
#include <iostream>
#include <sstream>


DataProcessing::DataProcessing() {}

DataProcessing::~DataProcessing() {}

std::unique_ptr<DataProcessing> DataProcessing::createNew()
{
    return std::make_unique<DataProcessing>();
}

bool DataProcessing::processDirectories()
{
    while (cKeepRunning)
    {
        std::unique_lock<std::mutex> lock(captureToProcessingQueueMutex);

        // 等待新目录路径的到来
        captureToProcessingQueueCondition.wait(lock, []
                                               { return !captureToProcessingQueue.empty() || !cKeepRunning; });

        if (!cKeepRunning && captureToProcessingQueue.empty())
        {
            return true; // 正常退出时返回true
        }

        // 从队列中获取目录路径
        SensorData tempData = captureToProcessingQueue.front();
        captureToProcessingQueue.pop();
        lock.unlock();

        // 解析时间戳，假设时间戳格式为 "YYYY-MM-DD HH-MM-SS-sss"，便于分类
        std::string date = tempData.readable_timestamp.substr(0, 10); // "YYYY-MM-DD"
        std::string time = tempData.readable_timestamp.substr(11);    // "HH-MM-SS-sss"
        int milliseconds = std::stoi(time.substr(9, 3));
        int timeWindowIndex = milliseconds / TIME_WINDOW_MS * TIME_WINDOW_MS;
        // 生成时间窗口字符串（例如 "HH-MM-SS-000"）
        std::string timeWindowStr = time.substr(0, 8) + "-" + std::to_string(timeWindowIndex).insert(0, 3 - std::to_string(timeWindowIndex).length(), '0');

        // 生成数据存储的键值（"YYYY-MM-DD HH-MM-SS-000"）
        std::string key = date + " " + timeWindowStr;

        // 创建目标文件夹并将文件移动到相应的文件夹
        std::string basePath = "Dataset/Car0001/" + date + "/" + timeWindowStr;
        fs::create_directories(basePath);

        // 对每个传感器类型创建子文件夹并移动文件
        std::string sensorFolderPath = basePath + "/" + tempData.sensor_type;
        fs::create_directories(sensorFolderPath);

        // 生成目标文件路径
        fs::path sourceFilePath(tempData.file_path);
        fs::path destinationFilePath = fs::path(sensorFolderPath) / sourceFilePath.filename();

        try {
            fs::rename(sourceFilePath, destinationFilePath);
            // std::cout << "Moved " << tempData.sensor_type << " file to " << destinationFilePath << "\n";
        } catch (const fs::filesystem_error &e) {
            std::cerr << "Error moving file: " << e.what() << "\n";
        }
        // // 将对齐后的文件夹路径推送到发送队列
        // {
        //     std::lock_guard<std::mutex> lock(processingToSendingQueueMutex);
        //     processingToSendingQueue.push(basePath);
        //     processingToSendingQueueCondition.notify_one();
        // }
    }
    return true; // 处理正常完成，返回true
}