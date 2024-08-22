#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <opencv2/opencv.hpp>
#include <condition_variable>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

class CameraThreadManager {
public:
    CameraThreadManager(const std::vector<std::string>& devicePaths);
    ~CameraThreadManager();

    void start();
    void stop();

    struct ThreadInfo {     // 存储线程id和设备id，一一对应
        std::thread::id threadID;
        std::string deviceID;
        std::string threadName;
    };

    void onDeviceChange(const std::vector<std::string>& newDevicePaths, const std::vector<std::string>& offDevicePaths);
    const std::vector<ThreadInfo>& getThreadInfoList() const;

private:
    void captureFrames(const std::string& devicePath, std::queue<std::pair<cv::Mat, int>>& frameQueue);      // 帧捕获
    void saveFramesWorker(std::queue<std::pair<cv::Mat, int>>& frameQueue, const std::string& cameraName);  // 帧保存
    std::string getCurrentDateTimeString(void );
    void saveFrames(std::queue<std::pair<cv::Mat, int>>& frameQueue, const std::string& currentDateTime, const std::string& cameraName);
    void stopThread(std::thread& t);   // 停止线程函数
    void removeDeviceAndThreads(const std::string& devicePath); // 停止并销毁一个摄像头的线程

    std::vector<std::string> devicePaths;   // 所有摄像头设备路径
    std::vector<std::queue<std::pair<cv::Mat, int>>> frameQueues;   // 存储每个摄像头捕获到的帧队列
    // 每个 std::queue 对应一个摄像头设备
    // 队列中的元素是一个 std::pair<cv::Mat, int>
    // 包含捕获到的帧 (cv::Mat) 和该帧的编号 (int)
    std::vector<std::thread> captureThreads;    // 捕获视频帧的线程对象
    std::vector<std::thread> saveThreads;   // 处理和保存视频帧的线程对象

    std::mutex queueMutex;// 互斥锁
    std::condition_variable dataCondition;  // 条件变量
    bool keepRunning = true;    // 控制线程的运行状态

    std::vector<ThreadInfo> threadInfoList; // 用于保存线程 ID 和设备 ID 的列表


};