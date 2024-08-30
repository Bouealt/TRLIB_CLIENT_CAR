// VehicleClientProj.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。

#pragma once

#ifndef PERCEPTION_DEVICE_MANAGER_H
#define PERCEPTION_DEVICE_MANAGER_H

#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include <thread>
#include <atomic>
#include <functional>
#include <mutex>
#include <opencv2/opencv.hpp>

// 感知设备管理类
class PerceptionDeviceManager {
public:

    // 构造函数，初始化设备列表和互斥锁
    PerceptionDeviceManager();       // 启动检测线程
    ~PerceptionDeviceManager(); // 析构函数，确保线程退出

    using DeviceChangeCallback = std::function<void(const std::vector<std::string>&, const std::vector<std::string>&)>;
    // 确保在PerceptionDeviceManager.h或相应的头文件中，定义了DeviceChangeCallback的类型，且其签名与实际使用时一致

    // 打印所有检测到的设备
    void printDetectedDevices() const;  // 在成员函数的后面加上const时，意味着这个函数承诺不会更改对象的任何成员变量（也称为成员数据）。
    void PrintAvailableDevices() const;        // 打印测试设备是否可用

    const std::vector<std::string>& getDevices() const; // 新增接口获取设备列表
    void setDeviceChangeCallback(DeviceChangeCallback callback);    // 设备变化回调函数

private:
    std::vector<std::string> devices;   // 设备注册信息
    std::vector<std::string> offDevices, newDevices;   // 设备注册信息

    mutable std::mutex devicesMutex;    // 互斥锁
    std::thread detectionThread;

    std::atomic<bool> stopRequested;         // 控制线程退出的标志变量

    bool InitPDManager = true;  // 是否是实例化对象

    void detectDevices();      // 设备检测函数，在独立线程中运行
    bool isDeviceExists(const std::string& path) const;     // 检查设备文件是否存在
    bool isDeviceAvailable(const std::string& devicePath) const;    // 测试设备是否可用

    DeviceChangeCallback deviceChangeCallback;
};




#endif // PERCEPTION_DEVICE_MANAGER_H


// TODO: 在此处引用程序需要的其他标头。