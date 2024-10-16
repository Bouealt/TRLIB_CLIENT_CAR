#ifndef SHARED_QUEUE_H
#define SHARED_QUEUE_H

#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <atomic>

struct SensorData
{
    std::string sensor_type;    // ����������
    std::string readable_timestamp; // �ɶ���ʱ���
    std::string file_path;  // �ļ�·��
};

// ���ڴӲɼ�ģ�鴫��Ŀ¼·����ʱ���������ģ��Ķ���
extern std::queue<SensorData> captureToProcessingQueue;
extern std::mutex captureToProcessingQueueMutex;
extern std::condition_variable captureToProcessingQueueCondition;


// ���ڴӴ���ģ�鴫�ݶ�����Ŀ¼·��������ģ��Ķ���
extern std::queue<std::string> processingToSendingQueue;
extern std::mutex processingToSendingQueueMutex;
extern std::condition_variable processingToSendingQueueCondition;

// ȫ�ֿ��Ʊ��������ڿ����̵߳�����״̬
extern std::atomic<bool> cKeepRunning;

#endif // SHARED_QUEUE_H
