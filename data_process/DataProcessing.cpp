#include "DataProcessing.h"
#include "../shared/SharedQueue.h" // ���� SharedQueue.h ����������Ŀ��
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

        // �ȴ���Ŀ¼·���ĵ���
        captureToProcessingQueueCondition.wait(lock, []
                                               { return !captureToProcessingQueue.empty() || !cKeepRunning; });

        if (!cKeepRunning && captureToProcessingQueue.empty())
        {
            return true; // �����˳�ʱ����true
        }

        // �Ӷ����л�ȡĿ¼·��
        SensorData tempData = captureToProcessingQueue.front();
        captureToProcessingQueue.pop();
        lock.unlock();

        // ����ʱ���������ʱ�����ʽΪ "YYYY-MM-DD HH-MM-SS-sss"�����ڷ���
        std::string date = tempData.readable_timestamp.substr(0, 10); // "YYYY-MM-DD"
        std::string time = tempData.readable_timestamp.substr(11);    // "HH-MM-SS-sss"
        int milliseconds = std::stoi(time.substr(9, 3));
        int timeWindowIndex = milliseconds / TIME_WINDOW_MS * TIME_WINDOW_MS;
        // ����ʱ�䴰���ַ��������� "HH-MM-SS-000"��
        std::string timeWindowStr = time.substr(0, 8) + "-" + std::to_string(timeWindowIndex).insert(0, 3 - std::to_string(timeWindowIndex).length(), '0');

        // �������ݴ洢�ļ�ֵ��"YYYY-MM-DD HH-MM-SS-000"��
        std::string key = date + " " + timeWindowStr;

        // ����Ŀ���ļ��в����ļ��ƶ�����Ӧ���ļ���
        std::string basePath = "Dataset/Car0001/" + date + "/" + timeWindowStr;
        fs::create_directories(basePath);

        // ��ÿ�����������ʹ������ļ��в��ƶ��ļ�
        std::string sensorFolderPath = basePath + "/" + tempData.sensor_type;
        fs::create_directories(sensorFolderPath);

        // ����Ŀ���ļ�·��
        fs::path sourceFilePath(tempData.file_path);
        fs::path destinationFilePath = fs::path(sensorFolderPath) / sourceFilePath.filename();

        try {
            fs::rename(sourceFilePath, destinationFilePath);
            // std::cout << "Moved " << tempData.sensor_type << " file to " << destinationFilePath << "\n";
        } catch (const fs::filesystem_error &e) {
            std::cerr << "Error moving file: " << e.what() << "\n";
        }
        // // ���������ļ���·�����͵����Ͷ���
        // {
        //     std::lock_guard<std::mutex> lock(processingToSendingQueueMutex);
        //     processingToSendingQueue.push(basePath);
        //     processingToSendingQueueCondition.notify_one();
        // }
    }
    return true; // ����������ɣ�����true
}