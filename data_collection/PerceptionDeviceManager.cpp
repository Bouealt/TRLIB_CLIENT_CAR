#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>
#include <opencv2/opencv.hpp>
#include "PerceptionDeviceManager.h"
#include <sys/inotify.h>    // inotify 是一个强大的 Linux 内核子系统，它可以监听文件系统事件，如创建、删除、修改等。
#include <unistd.h>



// 构造函数，初始化设备列表和互斥锁
PerceptionDeviceManager::PerceptionDeviceManager() {
    // 启动检测线程
    detectionThread = std::thread(&PerceptionDeviceManager::detectDevices, this);
    std::this_thread::sleep_for(std::chrono::seconds(2)); // 主线程暂停2s，使设备初始话
}

// 析构函数，确保线程退出
PerceptionDeviceManager::~PerceptionDeviceManager() {
    if (detectionThread.joinable()) {
        detectionThread.join();
    }
}

// 打印所有检测到的设备
void PerceptionDeviceManager::printDetectedDevices() const {
    std::lock_guard<std::mutex> lock(devicesMutex); // 加锁
    // std::lock_guard对象创建后，尝试获取它的mutex的所有权，当控制权不在作用域后，lock_guard对象被析构
    std::cout << "Detected video devices:" << std::endl;
    if (devices.empty()) {
        std::cout << "No video device detected." << std::endl;
        return;
    }
    for (const auto& device : devices) {    // 安全便利
        std::cout << device << std::endl;
    }
   // 离开作用域后，lock自动销毁并解锁
}

// 测试设备是否可用
void PerceptionDeviceManager::PrintAvailableDevices() const {
    std::lock_guard<std::mutex> lock(devicesMutex);
    for (const auto& device : devices) {
        if (isDeviceAvailable(device)) {
            std::cout << device << " is available." << std::endl;
        }
        else {
            std::cout << device << " is not available." << std::endl;
        }
    }
}

// 设备检测函数，在独立线程中运行
/*
* 要实现 USB 设备的热插拔检测，并在没有外部设备插拔时让检测程序进入休眠
* 可以使用 Linux 的 inotify 机制来监视 /dev 目录下的设备文件变化。
* inotify 是一个强大的 Linux 内核子系统，它可以监听文件系统事件，如创建、删除、修改等。
* #include <sys/inotify.h>    // inotify 是一个强大的 Linux 内核子系统，它可以监听文件系统事件，如创建、删除、修改等。
* #include <unistd.h>
*/
void PerceptionDeviceManager::detectDevices() {
    int inotifyFd = inotify_init(); // 初始化 inotify
    if (inotifyFd < 0) {
        std::cerr << "Failed to initialize inotify" << std::endl;
        return;
    }

    int wd = inotify_add_watch(inotifyFd, "/dev", IN_CREATE | IN_DELETE); // 监视 /dev 目录的创建和删除事件
    if (wd < 0) {
        std::cerr << "Failed to add inotify watch" << std::endl;
        close(inotifyFd);
        return;
    }

    while (true) {
        // 4种状态：A=devices, B=detectedDevices, C=offDevices, D=newDevices;
        // C=A-B,D=B-A  集合运算
        // 检测当前所有设备
        std::vector<std::string> detectedDevices;
        offDevices.clear();
        newDevices.clear();

        for (int i = 0; i < 20; i += 2) { // 假设最多有20个设备
            std::string devicePath = "/dev/video" + std::to_string(i);
            if (isDeviceExists(devicePath)) {
                detectedDevices.push_back(devicePath);
            }
        }

        // 计算offDevices（detectedDevices中不存在但devices中存在的设备）
        for (const auto& device : devices) {
            if (std::find(detectedDevices.begin(), detectedDevices.end(), device) == detectedDevices.end()) {
                offDevices.push_back(device);
            }
        }

        // 计算newDevices（detectedDevices中存在但devices中不存在的设备）
        for (const auto& device : detectedDevices) {
            if (std::find(devices.begin(), devices.end(), device) == devices.end()) {
                newDevices.push_back(device);
            }
        }
        std::cout << "Previous Devices :" << std::endl;
        this->printDetectedDevices();

        std::cout << "Currently detectedDevices Results:" << std::endl;
        for (const auto& detectedDevice : detectedDevices) {
            std::cout << detectedDevice << std::endl;
        }
        std::cout << "New Devices Print:" << std::endl;
        for (const auto& newDevice : newDevices) {
            std::cout << newDevice << std::endl;
        }
        std::cout << "need Off Devices Print:" << std::endl;
        for (const auto& offDevice : offDevices) {
            std::cout << offDevice << std::endl;
        }

        if(!InitPDManager){
            std::lock_guard<std::mutex> lock(devicesMutex);
            if (devices != detectedDevices) { // 检测到设备变化时，调用回调函数
                std::cout << "enter the devices != detectedDevices" << std::endl;
                devices = std::move(detectedDevices);   // 更新注册设备的内容

                // 调用回调函数通知设备变化
                if (deviceChangeCallback) {
                    std::cout << "enter the deviceChangeCallback" << std::endl;
                    deviceChangeCallback(newDevices, offDevices);
                }
            }
        }
        else {
            std::cout << "enter the devices Fisrt Init" << std::endl;
            std::lock_guard<std::mutex> lock(devicesMutex);
            devices = std::move(detectedDevices);
            InitPDManager = false;  // 仅进入一次
        }
        // 将devices通过detectedDevices更新
        std::cout << "Devices present Update devices:" << std::endl;
        for (const auto& Device : devices) {
            std::cout << Device << std::endl;
        }
        /*this->printDetectedDevices();
        std::cout << "pause this->printDetectedDevices();" << std::endl;
        this->PrintAvailableDevices();
        std::cout << "pause this->PrintAvailableDevices();" << std::endl;*/
        /*{
            std::lock_guard<std::mutex> lock(devicesMutex);
            devices = std::move(detectedDevices);
        }*/

        std::cout << "Waiting for device changes..." << std::endl;

        char buffer[1024];
        int length = read(inotifyFd, buffer, sizeof(buffer)); // 阻塞等待事件发生
        if (length < 0) {
            std::cerr << "inotify read error" << std::endl;
            break; // 错误发生，退出循环
        }

        // 处理事件
        struct inotify_event* event = (struct inotify_event*)&buffer[0];
        while (length > 0) {
            if (event->len) {
                if (event->mask & IN_CREATE) {
                    std::cout << "New device added: " << event->name << std::endl;
                }
                else if (event->mask & IN_DELETE) {
                    std::cout << "Device removed: " << event->name << std::endl;
                }
            }

            length -= sizeof(struct inotify_event) + event->len;
            event = (struct inotify_event*)((char*)event + sizeof(struct inotify_event) + event->len);
        }

        // 稍作休眠后再次检测
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    inotify_rm_watch(inotifyFd, wd);
    close(inotifyFd);
}



// 检查设备文件是否存在
bool PerceptionDeviceManager::isDeviceExists(const std::string& path) const {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

// 测试设备是否可用
bool PerceptionDeviceManager::isDeviceAvailable(const std::string& devicePath) const {
    /*
    * 在尝试打开设备之前，可以检查该设备是否已经在其他线程中被使用，并在这种情况下进行适当的处理。
    * 例如，可以跳过该设备或停止现有线程以释放设备。
    */
    /*{
        std::lock_guard<std::mutex> lock(devicesMutex);
        if (std::find(devices.begin(), devices.end(), devicePath) != devices.end()) {
            std::cerr << "Device " << devicePath << " is already in use." << std::endl;
            return false;
        }
    }*/

    cv::VideoCapture cap(devicePath);
    return cap.isOpened();
}

// 返回检测到的设备列表
const std::vector<std::string>& PerceptionDeviceManager::getDevices() const {
    return devices;
}

// 回调函数
void PerceptionDeviceManager::setDeviceChangeCallback(DeviceChangeCallback callback) {
    deviceChangeCallback = callback;
} 