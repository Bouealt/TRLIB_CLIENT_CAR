#include "DataProcessing.h"
#include "../shared/SharedQueue.h" // 假设 SharedQueue.h 包含在主项目中
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

void DataProcessing::processDirectories()
{
    std::regex regexPattern(R"(.*\/(\d{4}-\d{2}-\d{2})\/(\d{2}-\d{2}-\d{2}))");
    while (cKeepRunning)
    {
        std::unique_lock<std::mutex> lock(captureToProcessingQueueMutex);

        // 等待新目录路径的到来
        captureToProcessingQueueCondition.wait(lock, []
                                               { return !captureToProcessingQueue.empty() || !cKeepRunning; });

        if (!cKeepRunning && captureToProcessingQueue.empty())
        {
            return;
        }

        // 从队列中获取目录路径
        std::string directoryPath = captureToProcessingQueue.front();
        captureToProcessingQueue.pop();
        lock.unlock();

        // 提取日期和时间部分作为基准路径
        std::smatch matches;
        std::string baseDir, timeDir;
        if (std::regex_search(directoryPath, matches, regexPattern))
        {
            base·Dir = matches[0].str(); // 完整的基准路径
            timeDir = matches[2].str(); // 时间部分，用于文件夹命名
        }
        else
        {
            std::cerr << "Error: Unable to parse directory path for date and time." << std::endl;
            continue;
        }

        // 查找并对齐图像
        std::map<int, std::map<std::string, std::string>> alignedImages;

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

        alignAndSaveImages(baseDir, alignedImages);
        // 删除处理过的原始目录（如果需要）
        fs::remove_all(directoryPath);

    }
}

void DataProcessing::alignAndSaveImages(const std::string &baseDir, const std::map<int, std::map<std::string, std::string>> &alignedImages)
{
    for (const auto &entry : alignedImages)
    {
        int alignedMilliseconds = entry.first;
        const auto &imageFiles = entry.second;

        // 创建对齐后的目录，例如 "15-24-48-200"
        std::ostringstream alignedDirStream;
        alignedDirStream << baseDir << "-" << std::setw(3) << std::setfill('0') << alignedMilliseconds;
        std::string alignedDir = alignedDirStream.str();
        fs::create_directories(alignedDir);

        for (const auto &imageEntry : imageFiles)
        {
            std::string cameraName = imageEntry.first;
            std::string srcFilePath = imageEntry.second;
            std::string dstFilePath = alignedDir + "/" + cameraName + ".jpg";

            // 剪切并重命名图片文件（相当于移动文件）
            fs::rename(srcFilePath, dstFilePath);
        }

        // 将对齐后的文件夹路径推送到发送队列
        {
            std::lock_guard<std::mutex> lock(processingToSendingQueueMutex);
            processingToSendingQueue.push(alignedDir);
            processingToSendingQueueCondition.notify_one();
        }
    }
}