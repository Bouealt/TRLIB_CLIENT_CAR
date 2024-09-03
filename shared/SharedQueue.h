#ifndef SHARED_QUEUE_H
#define SHARED_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <atomic>

// 用于从采集模块传递目录路径和时间戳到处理模块的队列
extern std::queue<std::string> captureToProcessingQueue;
extern std::mutex captureToProcessingQueueMutex;
extern std::condition_variable captureToProcessingQueueCondition;
static constexpr int EXPECTED_CAMERA_COUNT = 2; // 预期的摄像头数量

// 用于从处理模块传递对齐后的目录路径到发送模块的队列
extern std::queue<std::string> processingToSendingQueue;
extern std::mutex processingToSendingQueueMutex;
extern std::condition_variable processingToSendingQueueCondition;

// 全局控制变量，用于控制线程的运行状态
extern std::atomic<bool> cKeepRunning;

#endif // SHARED_QUEUE_H
