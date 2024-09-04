#ifndef SHARED_QUEUE_H
#define SHARED_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <atomic>

// ���ڴӲɼ�ģ�鴫��Ŀ¼·����ʱ���������ģ��Ķ���
extern std::queue<std::string> captureToProcessingQueue;
extern std::mutex captureToProcessingQueueMutex;
extern std::condition_variable captureToProcessingQueueCondition;
static constexpr int EXPECTED_CAMERA_COUNT = 2; // Ԥ�ڵ�����ͷ����

// ���ڴӴ���ģ�鴫�ݶ�����Ŀ¼·��������ģ��Ķ���
extern std::queue<std::string> processingToSendingQueue;
extern std::mutex processingToSendingQueueMutex;
extern std::condition_variable processingToSendingQueueCondition;

// ȫ�ֿ��Ʊ��������ڿ����̵߳�����״̬
extern std::atomic<bool> cKeepRunning;

#endif // SHARED_QUEUE_H
