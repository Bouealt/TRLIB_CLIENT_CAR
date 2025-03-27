// CameraManager.cpp
#include "CameraManager.h"
#include <sstream>
#include <iomanip>

CameraManager::CameraManager(int numWorkers)
    : numWorkers(numWorkers)
{
}

CameraManager::~CameraManager()
{
    stop();
}

bool CameraManager::start(const std::vector<std::string> &devicePaths)
{
    if (running)
    {
        std::cerr << "Camera manager is already running" << std::endl;
        return false;
    }

    // 初始化相机设备
    {
        std::lock_guard<std::mutex> lock(mutex);
        cameras.clear();

        int deviceIndex = 0;
        for (const auto &path : devicePaths)
        {
            CameraDevice device;
            device.devicePath = path;
            device.cameraId = "camera" + std::to_string(deviceIndex++);
            device.isActive = false;
            cameras.push_back(device);
        }
    }

    // 设置运行标志
    running = true;

    // 启动工作线程
    workerThreads.clear();
    for (int i = 0; i < numWorkers; ++i)
    {
        workerThreads.emplace_back(&CameraManager::frameProcessorThread, this);
    }

    // 启动采集线程
    captureThread = std::thread(&CameraManager::cameraCapturerThread, this);

    return true;
}

void CameraManager::stop()
{
    // 设置停止标志
    running = false;

    // 通知所有等待的线程
    condition.notify_all();

    // 等待所有线程结束
    if (captureThread.joinable())
    {
        captureThread.join();
    }

    for (auto &thread : workerThreads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    // 关闭所有摄像头
    std::lock_guard<std::mutex> lock(mutex);
    for (auto &camera : cameras)
    {
        if (camera.capture.isOpened())
        {
            camera.capture.release();
        }
        camera.isActive = false;
    }
}

void CameraManager::setFrameProcessor(FrameProcessCallback processor)
{
    std::lock_guard<std::mutex> lock(mutex);
    frameProcessor = processor;
}

std::vector<std::string> CameraManager::discoverDevices() const
{
    std::vector<std::string> devices;

    for (int i = 0; i < 20; i += 2)
    {
        std::string devicePath = "/dev/video" + std::to_string(i);
        if(fs::exists(devicePath) && isDeviceAvailable(devicePath))
        {
            devices.push_back(devicePath);
        }
    }
    return devices;
}

std::vector<std::string> CameraManager::getActiveDevices() const
{
    std::vector<std::string> activeDevices;
    std::lock_guard<std::mutex> lock(mutex);

    for (const auto &camera : cameras)
    {
        if (camera.isActive)
        {
            activeDevices.push_back(camera.devicePath);
        }
    }

    return activeDevices;
}

void CameraManager::cameraCapturerThread()
{
    auto lastDeviceCheck = std::chrono::steady_clock::now();

    while (running)
    {
        // 定期检查设备状态
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastDeviceCheck).count() >= deviceCheckInterval)
        {
            updateDeviceStatus();
            lastDeviceCheck = now;
        }

        // 从每个活动相机捕获帧
        {
            std::lock_guard<std::mutex> lock(mutex);
            for (auto &camera : cameras)
            {
                if (camera.isActive)
                {
                    cv::Mat frame;
                    if (camera.capture.read(frame))
                    {
                        if (!frame.empty())
                        {
                            CameraFrame cameraFrame;
                            cameraFrame.frame = frame.clone(); // 复制帧以确保安全
                            cameraFrame.cameraId = camera.cameraId;
                            cameraFrame.timestamp = std::chrono::system_clock::now();

                            // 将帧加入队列
                            frameQueue.push(cameraFrame);
                            condition.notify_one();
                        }
                    }
                    else
                    {
                        // 捕获失败，标记设备为非活动
                        std::cerr << "Failed to capture frame from " << camera.devicePath << std::endl;
                        camera.isActive = false;
                    }
                }
            }
        }

        // 控制捕获速率
        std::this_thread::sleep_for(std::chrono::milliseconds(captureInterval));
    }
}

void CameraManager::frameProcessorThread()
{
    while (running)
    {
        CameraFrame frame;
        bool hasFrame = false;

        // 从队列获取帧
        {
            std::unique_lock<std::mutex> lock(mutex);
            condition.wait(lock, [this]
                           { return !frameQueue.empty() || !running; });

            if (!running && frameQueue.empty())
            {
                break;
            }

            if (!frameQueue.empty())
            {
                frame = frameQueue.front();
                frameQueue.pop();
                hasFrame = true;
            }
        }

        // 处理帧
        if (hasFrame)
        {
            // 保存帧到文件
            if (!saveFrameToFile(frame))
            {
                std::cerr << "Failed to save frame from " << frame.cameraId << std::endl;
            }

            // 调用用户处理回调
            if (frameProcessor)
            {
                try
                {
                    frameProcessor(frame);
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Error in frame processor: " << e.what() << std::endl;
                }
            }
        }
    }
}

std::string CameraManager::getCurrentTimeString() const
{
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tm_now;
    localtime_r(&now_time_t, &tm_now);

    std::stringstream ss;
    ss << std::put_time(&tm_now, "%Y-%m-%d/%H-%M-%S") << '-' << std::setw(3) << std::setfill('0') << now_ms.count();
    return ss.str();
}

bool CameraManager::saveFrameToFile(const CameraFrame &frame)
{
    try
    {
        std::string timeStr = getCurrentTimeString();
        std::string baseDir = fs::current_path().string();
        std::string curDateTime = timeStr.substr(0, 19);
        std::string msTime = timeStr.substr(20, 23);

        std::string folderPath = baseDir + "/dataCapture/Car0001/Camera/" + curDateTime;

        // 创建目录
        fs::create_directories(folderPath);

        // 保存图像
        std::string filename = folderPath + "/" + frame.cameraId + "-" + msTime + ".jpg";
        if (!cv::imwrite(filename, frame.frame))
        {
            return false;
        }

        // 构建传感器数据（如果需要集成到现有系统）
        SensorData data;
        data.sensor_type = "camera";
        data.readable_timestamp = timeStr;
        data.file_path = filename;

        // 将数据推送到处理队列
        {
            std::lock_guard<std::mutex> lock(captureToProcessingQueueMutex);
            captureToProcessingQueue.push(data);
            captureToProcessingQueueCondition.notify_one();
        }

        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error saving frame to file: " << e.what() << std::endl;
        return false;
    }
}

bool CameraManager::isDeviceAvailable(const std::string &devicePath) const
{
    cv::VideoCapture cap(devicePath);
    return cap.isOpened();
}

void CameraManager::updateDeviceStatus()
{
    std::lock_guard<std::mutex> lock(mutex);

    for (auto &camera : cameras)
    {
        // 处理当前非活动设备
        if (!camera.isActive)
        {
            // 尝试打开设备
            camera.capture.open(camera.devicePath);
            if (camera.capture.isOpened())
            {
                // 设置采集参数
                camera.capture.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
                camera.capture.set(cv::CAP_PROP_FRAME_HEIGHT, 720);
                camera.isActive = true;
                std::cout << "Camera " << camera.devicePath << " activated" << std::endl;
            }
        }
        // 检查当前活动设备是否仍然可用
        else if (!camera.capture.isOpened())
        {
            camera.isActive = false;
            std::cout << "Camera " << camera.devicePath << " deactivated" << std::endl;
        }
    }
}