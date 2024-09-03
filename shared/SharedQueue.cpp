#include "SharedQueue.h"

// 初始化用于从采集模块到处理模块的共享队列和同步原语
std::queue<std::string> captureToProcessingQueue;
std::mutex captureToProcessingQueueMutex;
std::condition_variable captureToProcessingQueueCondition;

// 初始化用于从处理模块到发送模块的共享队列和同步原语
std::queue<std::string> processingToSendingQueue;
std::mutex processingToSendingQueueMutex;
std::condition_variable processingToSendingQueueCondition;

// 初始化全局控制变量
std::atomic<bool> cKeepRunning(true);
