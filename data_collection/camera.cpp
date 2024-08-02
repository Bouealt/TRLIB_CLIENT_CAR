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

std::queue<std::pair<cv::Mat, int>> frameQueueCamera1; // ��������ͷ���У�����֡��֡���
std::queue<std::pair<cv::Mat, int>> frameQueueCamera2; // USB����ͷ���У�����֡��֡���

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
    std::string baseDir = "."; // �������ǰĿ¼Ϊ����Ŀ¼
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
    cv::VideoCapture cap(devicePath); // ��ָ������ͷ�豸
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open camera at " << devicePath << std::endl;
        return;
    }

    int frameNumber = 0;
    while (keepRunning) {
        cv::Mat frame;
        cap >> frame; // ������ͷ��ȡһ֡

        if (frame.empty()) {
            std::cerr << "Error: Captured empty frame from " << devicePath << std::endl;
            continue;
        }

        // ��֡ͼ���֡��ŷ������
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            frameQueue.push(std::make_pair(frame, frameNumber++));
            dataCondition.notify_one();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(250)); // ÿ���ȡ4֡��ÿ������ͷÿ���ȡ1֡��
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

    std::this_thread::sleep_for(std::chrono::seconds(10)); // ����10������

    keepRunning = false;
    dataCondition.notify_all();

    captureThread1.join();
    //captureThread2.join();
    saveThread1.join();
    //saveThread2.join();

    return 0;
}

