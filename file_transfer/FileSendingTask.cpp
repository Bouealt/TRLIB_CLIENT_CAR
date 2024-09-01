#include "FileSendingTask.h"
#include <iostream>
#include <thread>

void fileSendingTask(const std::string &server, int port)
{
    std::unique_ptr<FileSender> file_send = FileSender::createNew(server, port);

    while (cKeepRunning)
    {
        std::string directoryToSend;
        int retryCount = 0;  // 初始化重试计数器

        // 等待队列中有新的目录路径,控制作用域防止死锁
        {
            std::unique_lock<std::mutex> lock(directoryQueueMutex);
            std::cout << "wait for data" << std::endl;
            directoryQueueCondition.wait(lock, []
                                { return !directoryQueue.empty() || !cKeepRunning; });

            if (!cKeepRunning && directoryQueue.empty())
            {
                break; // 停止运行并且队列为空时退出
            }

            directoryToSend = directoryQueue.front();
            directoryQueue.pop();
        }

        // 尝试发送，最多重试三次
        bool success = false;
        while (!success && retryCount < 3 && cKeepRunning)
        {
            std::cout << "Sending files in directory: " << directoryToSend << " (Attempt " << (retryCount + 1) << "/3)" << std::endl;
            success = file_send->start(directoryToSend);

            if (!success)
            {
                std::cerr << "Failed to send directory: " << directoryToSend << std::endl;
                retryCount++;
                if (retryCount < 3)
                {
                    std::this_thread::sleep_for(std::chrono::seconds(1)); // 等待5秒再重试
                }
            }
        }

        // 如果仍然失败且已达到最大重试次数
        if (!success && retryCount == 3)
        {
            std::cerr << "Failed to send directory: " << directoryToSend << " after 3 attempts, skipping." << std::endl;
            // 你可以在这里选择日志记录或者其他错误处理机制
        }
    }
}

