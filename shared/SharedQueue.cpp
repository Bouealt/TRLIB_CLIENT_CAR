// SharedQueue.cpp
#include "SharedQueue.h"

// 初始化共享变量
std::queue<std::string> directoryQueue;
std::mutex directoryQueueMutex;
std::condition_variable directoryQueueCondition;
std::atomic<bool> cKeepRunning(true);
