#include "CameraThreadManager.h"

CameraThreadManager::CameraThreadManager(const std::vector<std::string>& devicePaths)
    : devicePaths(devicePaths), frameQueues(devicePaths.size()) {
}

CameraThreadManager::~CameraThreadManager() {
    stop();
    std::cout << "CameraThreadManager destroyed" << std::endl;
}

void CameraThreadManager::start() {
    keepRunning = true;

    // 为每个设备创建捕获帧的线程
    for (size_t i = 0; i < devicePaths.size(); ++i) {
        captureThreads.emplace_back(&CameraThreadManager::captureFrames, this, devicePaths[i], std::ref(frameQueues[i]));

        // 保存捕获线程的信息
        threadInfoList.push_back({ captureThreads.back().get_id(), devicePaths[i], "captureThread"});
    }

    // 为每个设备创建保存帧的线程
    for (size_t i = 0; i < devicePaths.size(); ++i) {
        std::string cameraName = "camera" + std::to_string(i);
        saveThreads.emplace_back(&CameraThreadManager::saveFramesWorker, this, std::ref(frameQueues[i]), cameraName);

        // 保存保存线程的信息
        threadInfoList.push_back({ saveThreads.back().get_id(), devicePaths[i],"saveThread"});
    }
}

void CameraThreadManager::stop() {
    keepRunning = false;
    dataCondition.notify_all();

    for (auto& t : captureThreads) {
        if (t.joinable()) {
            t.join();
        }
    }

    for (auto& t : saveThreads) {
        if (t.joinable()) {
            t.join();
        }
    }

    captureThreads.clear();
    saveThreads.clear();
}

const std::vector<CameraThreadManager::ThreadInfo>& CameraThreadManager::getThreadInfoList() const {
    return threadInfoList;
}

void CameraThreadManager::captureFrames(const std::string& devicePath, std::queue<std::pair<cv::Mat, int>>& frameQueue) {
 cvInit:
    cv::VideoCapture cap(devicePath);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open camera at " << devicePath << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(10));  // 使线程进入休眠状态
        // 通过休眠，避免线程自己关闭自己
        // 在检测到无法打开摄像头时，移除线程并退出线程函数
        //removeDeviceAndThreads(devicePath);
        return;
    }

    int frameNumber = 0;
    int ErrCapCount = 0;
    while (keepRunning) {
        cv::Mat frame;
        cap >> frame;

        if (frame.empty()) {
            std::cerr << "Error: Captured empty frame from " << devicePath << std::endl;
            if (5 != ErrCapCount)
            {
                ErrCapCount++;
                continue;
            }
            else {
                ErrCapCount = 0;
                goto cvInit;
            }
           
        }

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            frameQueue.push(std::make_pair(frame, frameNumber++));
            dataCondition.notify_one();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

void CameraThreadManager::removeDeviceAndThreads(const std::string& devicePath) {
    std::lock_guard<std::mutex> lock(queueMutex);

    // 查找设备路径对应的索引
    auto it = std::find(this->devicePaths.begin(), this->devicePaths.end(), devicePath);
    if (it != devicePaths.end()) {
        std::cout << "CameraThreadManager::removeDeviceAndThreads" << std::endl;
        size_t index = std::distance(devicePaths.begin(), it);
        // 打印将要移除的线程信息
        std::cout << "ThreadName: " << threadInfoList[2 * index].threadName << "\tThread ID: " << threadInfoList[2 * index].threadID << " \tcontrols device: " << threadInfoList[2 * index].deviceID << std::endl;
        std::cout << "ThreadName: " << threadInfoList[2 * index + 1].threadName << "\tThread ID: " << threadInfoList[2 * index + 1].threadID << " \tcontrols device: " << threadInfoList[2 * index + 1].deviceID << std::endl;

        // 停止并销毁相关线程
        // 提前做异常处理，避免
        // 线程在移除之前已经结束了，并且又尝试 join，可能会引发异常
        std::cout << "stop captureThreads" << std::endl;
        if (captureThreads[index].joinable()) { 
            stopThread(captureThreads[index]);
        }
        std::cout << "stop saveThreads" << std::endl;
        if (saveThreads[index+1].joinable()) {
            stopThread(saveThreads[index]);
        }
        std::cout << "stop Over" << std::endl;
        // 移除设备和线程信息
        captureThreads.erase(captureThreads.begin() + index);
        saveThreads.erase(saveThreads.begin() + index);
        frameQueues.erase(frameQueues.begin() + index);
        threadInfoList.erase(threadInfoList.begin() + 2 * index, threadInfoList.begin() + 2 * index + 2);
        devicePaths.erase(it);

        std::cout << "Removed device and threads for: " << devicePath << std::endl;
    }
}


void CameraThreadManager::saveFramesWorker(std::queue<std::pair<cv::Mat, int>>& frameQueue, const std::string& cameraName) {
    while (keepRunning || !frameQueue.empty()) {
        std::string currentDateTime;    // 当前时间
        std::unique_lock<std::mutex> lock(queueMutex);
        dataCondition.wait(lock, [&frameQueue, this] { return !frameQueue.empty() || !keepRunning; });

        if (!frameQueue.empty()) {
            //std::cout << "Processing frames from " << cameraName << std::endl;
            currentDateTime = getCurrentDateTimeString();
            saveFrames(frameQueue, currentDateTime, cameraName);
            //frameQueue.pop(); // 示例中仅移除帧，可以在此处理或保存帧数据
        }
    }
}

std::string CameraThreadManager::getCurrentDateTimeString() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_r(&now_time_t, &tm);

    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d/%H-%M-%S");

    return ss.str();
}

void CameraThreadManager::saveFrames(std::queue<std::pair<cv::Mat, int>>& frameQueue, const std::string& currentDateTime, const std::string& cameraName) {
    std::string baseDir = "."; // 假设程序当前目录为基础目录
    std::string carNumber = "0001";

    static std::string preDataTime = "";
    static int frameNum = 1; // 序号
    if (preDataTime != currentDateTime) {
        preDataTime = currentDateTime;
        frameNum = 1;
    }
    else {
        frameNum++;
    }

    std::string folderPath = baseDir + "/dataCapture/Car" + carNumber + "/" + currentDateTime + "/" + cameraName;
    fs::create_directories(folderPath);

    while (!frameQueue.empty()) {
        auto [frame, frameNumber] = frameQueue.front();
        frameQueue.pop();

        std::string filename = folderPath + "/frame" + std::to_string(frameNum) + ".jpg";
        cv::imwrite(filename, frame);
        std::cout << "Saved " << filename << std::endl;
    }
}

void CameraThreadManager::onDeviceChange(const std::vector<std::string>& newDevicePaths, const std::vector<std::string>& offDevicePaths) {
    std::lock_guard<std::mutex> lock(queueMutex);
    std::cout << "Device change detected, updating threads..." << std::endl;

    // 处理新增设备
    for (const auto& device : newDevicePaths) {
        if (std::find(devicePaths.begin(), devicePaths.end(), device) == devicePaths.end()) {
            std::queue<std::pair<cv::Mat, int>> newQueue;
            frameQueues.push_back(std::move(newQueue));
            devicePaths.push_back(device);

            size_t index = devicePaths.size() - 1;
            captureThreads.emplace_back(&CameraThreadManager::captureFrames, this, devicePaths[index], std::ref(frameQueues[index]));
            saveThreads.emplace_back(&CameraThreadManager::saveFramesWorker, this, std::ref(frameQueues[index]), "camera" + std::to_string(index));

            // 保存线程信息并打印
            ThreadInfo captureInfo = { captureThreads.back().get_id(), devicePaths[index], "captureThread" };
            ThreadInfo saveInfo = { saveThreads.back().get_id(), devicePaths[index], "saveThread" };
            threadInfoList.push_back(captureInfo);
            threadInfoList.push_back(saveInfo);

            std::cout << "ThreadName: " << captureInfo.threadName << "\tThread ID: " << captureInfo.threadID << " \tcontrols device: " << captureInfo.deviceID << std::endl;
            std::cout << "ThreadName: " << saveInfo.threadName << "\tThread ID: " << saveInfo.threadID << " \tcontrols device: " << saveInfo.deviceID << std::endl;

            std::cout << "Added new device and threads for: " << device << std::endl;
        }
    }

    // 处理移除设备
    for (const auto& device : offDevicePaths) {
        auto it = std::find(devicePaths.begin(), devicePaths.end(), device);
        if (it != devicePaths.end()) {
            size_t index = std::distance(devicePaths.begin(), it);

            // 打印将要移除的线程信息
            std::cout << "ThreadName: " << threadInfoList[2 * index].threadName << "\tThread ID: " << threadInfoList[2 * index].threadID << " \tcontrols device: " << threadInfoList[2 * index].deviceID << std::endl;
            std::cout << "ThreadName: " << threadInfoList[2 * index + 1].threadName << "\tThread ID: " << threadInfoList[2 * index + 1].threadID << " \tcontrols device: " << threadInfoList[2 * index + 1].deviceID << std::endl;

            // 停止相关线程
            stopThread(captureThreads[index]);
            stopThread(saveThreads[index]);

            // 移除线程、队列和设备路径
            captureThreads.erase(captureThreads.begin() + index);
            saveThreads.erase(saveThreads.begin() + index);
            frameQueues.erase(frameQueues.begin() + index);
            threadInfoList.erase(threadInfoList.begin() + 2 * index, threadInfoList.begin() + 2 * index + 2);
            devicePaths.erase(it);

            std::cout << "Removed device and threads for: " << device << std::endl;
        }
    }
}


void CameraThreadManager::stopThread(std::thread& t) {
    try {
        if (t.joinable()) {
            t.join();
        }
    }
    catch (const std::system_error& e) {
        std::cerr << "Error joining thread: " << e.what() << std::endl;
        // 连接线程错误，避免锁死
    }
}