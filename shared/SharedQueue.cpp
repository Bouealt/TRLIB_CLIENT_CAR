// SharedQueue.cpp
#include "SharedQueue.h"

// ��ʼ���������
std::queue<std::string> directoryQueue;
std::mutex directoryQueueMutex;
std::condition_variable directoryQueueCondition;
std::atomic<bool> cKeepRunning(true);
