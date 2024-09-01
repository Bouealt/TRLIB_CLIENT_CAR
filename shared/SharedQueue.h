// SharedQueue.h
//用来存储公共变量
#ifndef SHARED_QUEUE_H
#define SHARED_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <atomic>

// 定义共享队列和同步原语
extern std::queue<std::string> directoryQueue;
extern std::mutex directoryQueueMutex;
extern std::condition_variable directoryQueueCondition;
extern std::atomic<bool> cKeepRunning;

#endif // SHARED_QUEUE_H
