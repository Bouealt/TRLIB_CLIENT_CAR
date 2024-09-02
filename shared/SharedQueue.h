// SharedQueue.h
//�����洢��������
#ifndef SHARED_QUEUE_H
#define SHARED_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <atomic>

// ���干����к�ͬ��ԭ��
extern std::queue<std::string> directoryQueue;
extern std::mutex directoryQueueMutex;
extern std::condition_variable directoryQueueCondition;
extern std::atomic<bool> cKeepRunning;

#endif // SHARED_QUEUE_H
