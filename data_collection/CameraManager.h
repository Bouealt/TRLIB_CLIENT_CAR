// CameraManager.h
#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <functional>
#include <memory>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include "../shared/SharedQueue.h"
namespace fs = std::filesystem;

// 相机帧结构体
struct CameraFrame
{
    cv::Mat frame;
    std::string cameraId;
    std::chrono::system_clock::time_point timestamp;
};

// 相机设备信息
struct CameraDevice
{
    std::string devicePath;
    cv::VideoCapture capture;
    std::string cameraId;
    bool isActive;
};

// 相机数据处理回调接口
using FrameProcessCallback = std::function<void(const CameraFrame &)>;

class CameraManager
{
public:
    CameraManager(int numWorkers = 2);
    ~CameraManager();

    // 初始化并启动相机管理器
    bool start(const std::vector<std::string> &devicePaths);

    // 停止相机管理器
    void stop();

    // 设置帧处理回调
    void setFrameProcessor(FrameProcessCallback processor);

    // 获取活动设备列表
    std::vector<std::string> discoverDevices() const;

    // 检查相机状态
    std::vector<std::string> getActiveDevices() const;

private:
    // 相机采集线程函数
    void cameraCapturerThread();

    // 帧处理工作线程函数
    void frameProcessorThread();

    // 生成时间戳字符串
    std::string getCurrentTimeString() const;

    // 保存图像到文件
    bool saveFrameToFile(const CameraFrame &frame);

    // 检查设备是否可用
    bool isDeviceAvailable(const std::string &devicePath) const;

    // 更新设备状态
    void updateDeviceStatus();

private:
    // 相机设备列表
    std::vector<CameraDevice> cameras;

    // 工作线程列表
    std::vector<std::thread> workerThreads;

    // 相机采集线程
    std::thread captureThread;

    // 帧处理回调
    FrameProcessCallback frameProcessor;

    // 帧队列
    std::queue<CameraFrame> frameQueue;

    // 同步相关
    mutable std::mutex mutex;
    std::condition_variable condition;
    std::atomic<bool> running{false};

    // 工作线程数量
    int numWorkers;

    // 设备检查间隔(ms)
    const int deviceCheckInterval = 5000;

    // 帧捕获间隔(ms)
    const int captureInterval = 200;
};