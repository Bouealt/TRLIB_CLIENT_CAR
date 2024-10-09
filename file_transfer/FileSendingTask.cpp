#include "FileSendingTask.h"
#include <iostream>
#include <thread>

void fileSendingTask(const std::string &server, int port)
{
    // ����һ��Ψһ�� FileSender ʵ��
    std::unique_ptr<FileSender> file_send = FileSender::createNew(server, port);
    while (cKeepRunning)
    {
        std::string directoryToSend;
        int retryCount = 0; // ��ʼ�����Լ�����

        // �ȴ����������µ�Ŀ¼·��, �����������ֹ����
        {
            std::unique_lock<std::mutex> lock(processingToSendingQueueMutex);
            std::cout << "wait for data" << std::endl;
            processingToSendingQueueCondition.wait(lock, []
                                                   { return !processingToSendingQueue.empty() || !cKeepRunning; });

            if (!cKeepRunning && processingToSendingQueue.empty())
            {
                break; // ֹͣ���в��Ҷ���Ϊ��ʱ�˳�
            }

            directoryToSend = processingToSendingQueue.front();
            processingToSendingQueue.pop();
        }

        // ���Է��ͣ������������
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
                    std::this_thread::sleep_for(std::chrono::seconds(1)); // �ȴ�1��������
                }
            }
        }

        // �����Ȼʧ�����Ѵﵽ������Դ���
        if (!success && retryCount == 3)
        {
            std::cerr << "Failed to send directory: " << directoryToSend << " after 3 attempts, skipping." << std::endl;
        }
    }
}
