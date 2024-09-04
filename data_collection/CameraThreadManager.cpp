#include "CameraThreadManager.h"

CameraThreadManager::CameraThreadManager(const std::vector<std::string> &devicePaths)
    : devicePaths(devicePaths), frameQueues(devicePaths.size())
{
}

CameraThreadManager::~CameraThreadManager()
{
    stop();
}

void CameraThreadManager::start()
{
    keepRunning = true;
    // 为每个设备创建捕获帧的线程
    for (size_t i = 0; i < devicePaths.size(); ++i)
    {
        captureThreads.emplace_back(&CameraThreadManager::captureFrames, this, devicePaths[i], std::ref(frameQueues[i]));

        // 保存捕获线程的信息
        threadInfoList.push_back({captureThreads.back().get_id(), devicePaths[i], "captureThread"});

        std::string cameraName = "camera" + std::to_string(i);
        saveThreadsRunning.push_back(true);
        saveThreads.emplace_back(&CameraThreadManager::saveFramesWorker, this, std::ref(frameQueues[i]), cameraName);
        saveThreads[i].detach();
        // 保存保存线程的信息
        threadInfoList.push_back({saveThreads.back().get_id(), devicePaths[i], "saveThread"});
    }
}

void CameraThreadManager::stop()
{
    keepRunning = false;
    dataCondition.notify_all();

    for (auto &t : captureThreads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }

    for (auto &t : saveThreads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }

    captureThreads.clear();
    saveThreads.clear();
}

const std::vector<CameraThreadManager::ThreadInfo> &CameraThreadManager::getThreadInfoList() const
{
    return threadInfoList;
}

void CameraThreadManager::captureFrames(const std::string &devicePath, std::queue<std::pair<cv::Mat, int>> &frameQueue)
{
cvInit:
    cv::VideoCapture cap(devicePath);
    if (!cap.isOpened())
    {
        std::cerr << "Error: Could not open camera at " << devicePath << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1)); // 使线程进入休眠状态
        // 通过休眠，避免线程自己关闭自己
        // 在检测到无法打开摄像头时，移除线程并退出线程函数
        // removeDeviceAndThreads(devicePath);
        return;
    }

    int frameNumber = 0;
    int ErrCapCount = 0;
    while (keepRunning)
    {
        cv::Mat frame;
        cap >> frame;

        if (frame.empty())
        {
            std::cerr << "Error: Captured empty frame from " << devicePath << std::endl;
            if (5 != ErrCapCount)
            {
                ErrCapCount++;
                continue;
            }
            else
            {
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

// void CameraThreadManager::removeDeviceAndThreads(const std::string& devicePath) {
//     std::lock_guard<std::mutex> lock(queueMutex);
//
//     // 查找设备路径对应的索引
//     auto it = std::find(this->devicePaths.begin(), this->devicePaths.end(), devicePath);
//     if (it != devicePaths.end()) {
//         std::cout << "CameraThreadManager::removeDeviceAndThreads" << std::endl;
//         const size_t index = std::distance(devicePaths.begin(), it);
//         // 打印将要移除的线程信息
//         std::cout << "ThreadName: " << threadInfoList[2 * index].threadName << "\tThread ID: " << threadInfoList[2 * index].threadID << " \tcontrols device: " << threadInfoList[2 * index].deviceID << std::endl;
//         std::cout << "ThreadName: " << threadInfoList[2 * index + 1].threadName << "\tThread ID: " << threadInfoList[2 * index + 1].threadID << " \tcontrols device: " << threadInfoList[2 * index + 1].deviceID << std::endl;
//
//         // 停止并销毁相关线程
//         // 提前做异常处理，避免
//         // 线程在移除之前已经结束了，并且又尝试 join，可能会引发异常
//         //
//         // 先停止 captureThread
//         std::cout << "Stopping captureThreads for device: " << devicePath << std::endl;
//         stopThread(captureThreads[index]);
//
//         // 通知并停止 saveThread
//         saveThreadsRunning[index] = false;
//         dataCondition.notify_one(); // 通知保存线程停止
//         std::cout << "Stopping saveThreads for device: " << devicePath << std::endl;
//         stopThread(saveThreads[index]);
//         std::cout << "stop Over" << std::endl;
//
//         // 清理资源
//         captureThreads.erase(captureThreads.begin() + index);
//         saveThreads.erase(saveThreads.begin() + index);
//         frameQueues.erase(frameQueues.begin() + index);
//         threadInfoList.erase(threadInfoList.begin() + 2 * index, threadInfoList.begin() + 2 * index + 2);
//         saveThreadsRunning.erase(saveThreadsRunning.begin() + index);
//         devicePaths.erase(it);
//
//         std::cout << "Removed device and threads for: " << devicePath << std::endl;
//     }
// }

void CameraThreadManager::saveFramesWorker(std::queue<std::pair<cv::Mat, int>> &frameQueue, const std::string &cameraName)
{
    size_t threadIndex = 0;
    for (size_t i = 0; i < saveThreads.size(); ++i)
    {
        if (saveThreads[i].get_id() == std::this_thread::get_id())
        {
            threadIndex = i;
            break;
        }
    }

    /*while (saveThreadsRunning[threadIndex] || !frameQueue.empty()) {*/
    while (true)
    {
        std::string currentDateTime; // 当前时间
        std::unique_lock<std::mutex> lock(queueMutex);

        // 增加日志，查看是否卡在等待中
        // std::cout << "saveFramesWorker() Thread " << threadIndex << " waiting for data or shutdown signal." << std::endl;

        // 等待条件：frameQueue 非空，或者 saveThreadsRunning[threadIndex] 为 false（表示需要停止线程）
        dataCondition.wait(lock, [&frameQueue, this, threadIndex]
                           { return !frameQueue.empty() || !saveThreadsRunning[threadIndex]; });

        // std::cout << "saveFramesWorker() Thread " << threadIndex << " woke up." << std::endl;
        // 如果线程需要停止并且队列为空，退出循环
        if (!saveThreadsRunning[threadIndex] && frameQueue.empty())
        {
            break;
        }

        // 处理帧数据
        if (!frameQueue.empty())
        {
            // std::cout << "Processing frames from " << cameraName << std::endl;
            currentDateTime = getCurrentDateTimeString();
            saveFrames(frameQueue, currentDateTime, cameraName);
            // frameQueue.pop(); // 示例中仅移除帧，可以在此处理或保存帧数据
        }
    }
}

std::string CameraThreadManager::getCurrentDateTimeString()
{
    // 获取当前时间点
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    // 获取当前时间的tm结构
    std::tm tm;
    localtime_r(&now_time_t, &tm);

    // 将时间信息格式化为字符串
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d/%H-%M-%S") << '-' << std::setfill('0') << std::setw(3) << now_ms.count();
    // std::cout << ss.str() << std::endl;     // 打印当前的时间，时-分-秒-毫秒
    return ss.str();
}

void CameraThreadManager::saveFrames(std::queue<std::pair<cv::Mat, int>> &frameQueue, const std::string &currentDateTime, const std::string &cameraName)
{
    std::string baseDir = std::filesystem::current_path().string(); // 假设程序当前目录为基础目录
    std::string carNumber = "0001";
    std::string curDateTime = currentDateTime.substr(0, 19);
    std::string msTime = currentDateTime.substr(20, 23); // 切割出毫秒

    static std::string preDataTime = "";
    // static int frameNum = 1; // 序号，这里原本使用来对每秒的截图进行编号的，现在使用时间戳，就不能使用这个序号
    // if (preDataTime != currentDateTime) {
    //     preDataTime = currentDateTime;
    //     frameNum = 1;
    // }
    // else {
    //     frameNum++;
    // }

    // std::string folderPath = baseDir + "/dataCapture/Car" + carNumber + "/" + currentDateTime + "/" + cameraName;
    std::string folderPath = baseDir + "/DataSet/Car" + carNumber + "/" + curDateTime;

    fs::create_directories(folderPath);

    static int CutScreenCount = 0;

    while (!frameQueue.empty())
    {
        auto [frame, frameNumber] = frameQueue.front();
        frameQueue.pop();

        // std::string filename = folderPath + "/Frame" + std::to_string(frameNum) + ".jpg";
        std::string filename = folderPath + "/" + cameraName + "-" + msTime + ".jpg";
        cv::imwrite(filename, frame);
        // std::cout << "Saved " << filename << std::endl; // 打印每一次的保存信息
        CutScreenCount++;
        if (20 == CutScreenCount)
        {
            /* 设置每20次截图保存打印一次信息 */
            CutScreenCount = 0;
            std::cout << "20 Frames had saved! ..." << std::endl;
            std::cout << "Lastest Frame is Saved " << filename << std::endl;
        }
    }
    {
        std::lock_guard<std::mutex> lock(captureToProcessingQueueMutex);
        captureToProcessingQueue.push(folderPath);   // 推送目录路径
        captureToProcessingQueueCondition.notify_one(); // 通知处理模块有新数据
    }
}

void CameraThreadManager::onDeviceChange(const std::vector<std::string> &newDevicePaths, const std::vector<std::string> &offDevicePaths)
{
    std::lock_guard<std::mutex> lock(queueMutex);
    std::cout << "Device change detected, updating threads..." << std::endl;

    // 处理新增设备
    for (const auto &device : newDevicePaths)
    {
        if (std::find(devicePaths.begin(), devicePaths.end(), device) == devicePaths.end())
        {
            std::queue<std::pair<cv::Mat, int>> newQueue;
            frameQueues.push_back(std::move(newQueue));
            devicePaths.push_back(device);

            size_t index = devicePaths.size() - 1;
            captureThreads.emplace_back(&CameraThreadManager::captureFrames, this, devicePaths[index], std::ref(frameQueues[index]));
            saveThreadsRunning.push_back(true);
            saveThreads.emplace_back(&CameraThreadManager::saveFramesWorker, this, std::ref(frameQueues[index]), "camera" + std::to_string(index));
            saveThreads[index].detach();
            // 保存线程信息并打印
            ThreadInfo captureInfo = {captureThreads.back().get_id(), devicePaths[index], "captureThread"};
            ThreadInfo saveInfo = {saveThreads.back().get_id(), devicePaths[index], "saveThread"};
            threadInfoList.push_back(captureInfo);
            threadInfoList.push_back(saveInfo);

            std::cout << "ThreadName: " << captureInfo.threadName << "\tThread ID: " << captureInfo.threadID << " \tcontrols device: " << captureInfo.deviceID << std::endl;
            std::cout << "ThreadName: " << saveInfo.threadName << "\tThread ID: " << saveInfo.threadID << " \tcontrols device: " << saveInfo.deviceID << std::endl;

            std::cout << "Added new device and threads for: " << device << std::endl;
        }
    }

    // 处理移除设备
    for (const auto &device : offDevicePaths)
    {
        // removeDeviceAndThreads(device);
        auto it = std::find(devicePaths.begin(), devicePaths.end(), device);
        if (it != devicePaths.end())
        {
            std::cout << "Stopping threads for device: " << device << std::endl;
            const size_t index = std::distance(devicePaths.begin(), it);
            // 打印将要移除的线程信息
            std::cout << "ThreadName: " << threadInfoList[2 * index].threadName << "\tThread ID: " << threadInfoList[2 * index].threadID << " \tcontrols device: " << threadInfoList[2 * index].deviceID << std::endl;
            std::cout << "ThreadName: " << threadInfoList[2 * index + 1].threadName << "\tThread ID: " << threadInfoList[2 * index + 1].threadID << " \tcontrols device: " << threadInfoList[2 * index + 1].deviceID << std::endl;

            // 停止相关线程
            /*std::cout << "stop saveThreads" << std::endl;
            stopThread(saveThreads[index]);*/
            std::cout << "stop captureThreads" << std::endl;
            stopThread(captureThreads[index]);

            saveThreadsRunning[index] = false;
            dataCondition.notify_all();
            // std::this_thread::sleep_for(std::chrono::seconds(1));
            /*if (saveThreads[index].joinable()) {
                std::cout << "saveThreads is still running or has finished but not joined." << std::endl;
                stopThread(saveThreads[index]);
                std::cout << "saveThreads is not joinable (already joined or never started)." << std::endl;
            }
            else {
                std::cout << "saveThreads is not joinable (already joined or never started)." << std::endl;
            }*/

            /*std::cout << "stop captureThreads" << std::endl;
            if (captureThreads[index].joinable()) {
                stopThread(captureThreads[index]);
            }
            std::cout << "stop saveThreads" << std::endl;
            if (saveThreads[index].joinable()) {
                stopThread(saveThreads[index]);
            }*/
            std::cout << "stop threads over" << std::endl;
            // 移除线程、队列和设备路径
            captureThreads.erase(captureThreads.begin() + index);
            saveThreads.erase(saveThreads.begin() + index);
            frameQueues.erase(frameQueues.begin() + index);
            /* 每个摄像头会有两个线程，一个捕获，一个保存，因此要删除相邻的两个迭代器,左闭右开区间 */
            threadInfoList.erase(threadInfoList.begin() + 2 * index, threadInfoList.begin() + 2 * index + 2);
            saveThreadsRunning.erase(saveThreadsRunning.begin() + index);
            devicePaths.erase(it);

            std::cout << "Removed device and threads for: " << device << std::endl;
        }
    }
}

void CameraThreadManager::stopThread(std::thread &t)
{
    try
    {
        if (t.joinable())
        {
            std::cout << "Enter Join(), want to stop :" << t.get_id() << std::endl;
            t.join();
            std::cout << "Join Finished!!!!!" << std::endl;
        }
        else
        {
            std::cerr << "Warning: Thread was not joinable, it may have already ended or been detached." << std::endl;
        }
    }
    catch (const std::system_error &e)
    {
        std::cerr << "Error joining thread: " << e.what() << std::endl;
    }
}
