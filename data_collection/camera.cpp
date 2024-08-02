#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <opencv2/opencv.hpp>
#include <condition_variable>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

std::mutex queueMutex;
std::condition_variable dataCondition;
bool keepRunning = true;

std::queue<std::pair<cv::Mat, int>> frameQueueCamera1; // 集成摄像头队列，包含帧和帧编号
std::queue<std::pair<cv::Mat, int>> frameQueueCamera2; // USB摄像头队列，包含帧和帧编号

namespace fs = std::filesystem;

std::string getCurrentDateTimeString() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_r(&now_time_t, &tm);

    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d/%H-%M-%S");

    return ss.str();
}

void saveFrames(std::queue<std::pair<cv::Mat, int>>& frameQueue, const std::string& currentDateTime, const std::string& cameraName) {
    std::string baseDir = "."; // 假设程序当前目录为基础目录
    std::string carNumber = "0001";

    std::string folderPath = baseDir + "/dataCapture/Car" + carNumber + "/" + currentDateTime + "/" + cameraName;
    fs::create_directories(folderPath);

    while (!frameQueue.empty()) {
        auto [frame, frameNumber] = frameQueue.front();
        frameQueue.pop();

        std::string filename = folderPath + "/frame" + std::to_string(frameNumber) + ".jpg";
        cv::imwrite(filename, frame);
        std::cout << "Saved " << filename << std::endl;
    }
}

void captureFrames(const std::string& devicePath, std::queue<std::pair<cv::Mat, int>>& frameQueue) {
    cv::VideoCapture cap(devicePath); // 打开指定摄像头设备
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open camera at " << devicePath << std::endl;
        return;
    }

    int frameNumber = 0;
    while (keepRunning) {
        cv::Mat frame;
        cap >> frame; // 从摄像头获取一帧

        if (frame.empty()) {
            std::cerr << "Error: Captured empty frame from " << devicePath << std::endl;
            continue;
        }

        // 将帧图像和帧编号放入队列
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            frameQueue.push(std::make_pair(frame, frameNumber++));
            dataCondition.notify_one();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(250)); // 每秒获取4帧（每个摄像头每秒获取1帧）
    }
}

void saveFramesWorker(std::queue<std::pair<cv::Mat, int>>& frameQueue, const std::string& cameraName) {
    while (keepRunning || !frameQueue.empty()) {
        std::string currentDateTime;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            dataCondition.wait(lock, [&frameQueue] { return !frameQueue.empty() || !keepRunning; });

            if (!frameQueue.empty()) {
                currentDateTime = getCurrentDateTimeString();
                saveFrames(frameQueue, currentDateTime, cameraName);
            }
        }
    }
}

int main() {
    std::thread captureThread1(captureFrames, "/dev/video0", std::ref(frameQueueCamera1));
    //std::thread captureThread2(captureFrames, "/dev/video2", std::ref(frameQueueCamera2));

    std::thread saveThread1(saveFramesWorker, std::ref(frameQueueCamera1), "camera01");
    //std::thread saveThread2(saveFramesWorker, std::ref(frameQueueCamera2), "camera02");

    std::this_thread::sleep_for(std::chrono::seconds(10)); // 运行10秒后结束

    keepRunning = false;
    dataCondition.notify_all();

    captureThread1.join();
    //captureThread2.join();
    saveThread1.join();
    //saveThread2.join();

    return 0;
}

