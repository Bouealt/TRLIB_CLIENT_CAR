#include "DataProcessing.h"
#include "../shared/SharedQueue.h" // ���� SharedQueue.h ����������Ŀ��
#include <filesystem>
#include <regex>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

DataProcessing::DataProcessing() {}

DataProcessing::~DataProcessing() {}

std::unique_ptr<DataProcessing> DataProcessing::createNew()
{
    return std::make_unique<DataProcessing>();
}

bool DataProcessing::processDirectories()
{
    std::regex regexPattern(R"(.*\/(\d{4}-\d{2}-\d{2})\/(\d{2}-\d{2}-\d{2}))");
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
        std::string directoryPath = captureToProcessingQueue.front();
        captureToProcessingQueue.pop();
        lock.unlock();

        // ��ȡ���ں�ʱ�䲿����Ϊ��׼·��
        std::smatch matches;
        std::string baseDir, timeDir;
        if (std::regex_search(directoryPath, matches, regexPattern))
        {
            baseDir = matches[0].str(); // �����Ļ�׼·��
            timeDir = matches[2].str(); // ʱ�䲿�֣������ļ�������
        }
        else
        {
            std::cerr << "Error: Unable to parse directory path for date and time." << std::endl;
            return false; // �������ʧ�ܣ�����false
        }

        // ���Ҳ�����ͼ��
        std::map<int, std::map<std::string, std::string>> alignedImages;

        try
        {
            for (const auto &entry : fs::directory_iterator(directoryPath))
            {
                if (entry.is_regular_file())
                {
                    std::string filePath = entry.path().string();
                    std::regex filePattern(R"(.*\/(camera\d)-(\d{3})\.jpg$)");
                    std::smatch fileMatches;

                    if (std::regex_search(filePath, fileMatches, filePattern))
                    {
                        std::string cameraName = fileMatches[1].str();
                        int milliseconds = std::stoi(fileMatches[2].str());
                        int alignedMilliseconds = (milliseconds / timeWindow) * timeWindow;

                        alignedImages[alignedMilliseconds][cameraName] = filePath;
                    }
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error while processing directory: " << e.what() << std::endl;
            return false; // ����ڴ�������з����쳣������false
        }
        alignAndSaveImages(baseDir, alignedImages);
        // ����ɾ���������ԭʼĿ¼�������Ҫ��
        try
        {
            fs::remove_all(directoryPath);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: Failed to remove directory: " << e.what() << std::endl;
            return false; // ���ɾ��Ŀ¼ʧ�ܣ�����false
        }
    }
    return true; // ����������ɣ�����true
}

void DataProcessing::alignAndSaveImages(const std::string &baseDir, const std::map<int, std::map<std::string, std::string>> &alignedImages)
{
    for (const auto &entry : alignedImages)
    {
        int alignedMilliseconds = entry.first;
        const auto &imageFiles = entry.second;

        // ����������Ŀ¼������ "15-24-48-200"
        std::ostringstream alignedDirStream;
        alignedDirStream << baseDir << "-" << std::setw(3) << std::setfill('0') << alignedMilliseconds;
        std::string alignedDir = alignedDirStream.str();
        fs::create_directories(alignedDir);

        for (const auto &imageEntry : imageFiles)
        {
            std::string cameraName = imageEntry.first;
            std::string srcFilePath = imageEntry.second;
            std::string dstFilePath = alignedDir + "/" + cameraName + ".jpg";

            // ���в�������ͼƬ�ļ����൱���ƶ��ļ���
            fs::rename(srcFilePath, dstFilePath);
        }

        // ���������ļ���·�����͵����Ͷ���
        {
            std::lock_guard<std::mutex> lock(processingToSendingQueueMutex);
            processingToSendingQueue.push(alignedDir);
            processingToSendingQueueCondition.notify_one();
        }
    }
}