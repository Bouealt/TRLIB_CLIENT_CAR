#include "DataCollector.h"

std::unique_ptr<DataCollector> DataCollector::createNew()
{
    return std::make_unique<DataCollector>();
}

DataCollector::DataCollector():
m_PDmanager(std::make_unique<PerceptionDeviceManager>()),
m_threadManager(std::make_unique<CameraThreadManager>(m_PDmanager->getDevices()))
{
}
DataCollector::~DataCollector()
{
    std::cout << "DataCollector destroyed" << std::endl;
}
bool DataCollector::start(int totalSeconds)
{
    try
    {
        m_threadManager->start(); // 启动线程
        // 打印线程信息
        const auto &threadInfoList = m_threadManager->getThreadInfoList();
        for (const auto &info : threadInfoList)
        {
            std::cout << "ThreadName: " << info.threadName << "\tThread ID: " << info.threadID << " \tcontrols device: " << info.deviceID << std::endl;
        }
        // 设置设备变化回调
        m_PDmanager->setDeviceChangeCallback([this](const std::vector<std::string> &newDevices, const std::vector<std::string> &offDevices)
                                          { this->m_threadManager->onDeviceChange(newDevices, offDevices); });
        // 主线程执行核心逻辑，根据传入的 totalSeconds 参数来计算需要循环的次数
        const int intervalSeconds = 10;                  // 每次循环的时间间隔为10秒
        int iterations = totalSeconds / intervalSeconds; // 计算需要循环的次数

        for (int i = 0; i < iterations; ++i)
        {
            std::cout << "main() running " << (i + 1) * intervalSeconds << " out of " << totalSeconds << " seconds of main code" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds)); // 主线程每10秒打印一次信息
        }

        // 处理剩余的时间（如果有），避免丢失精度
        int remainingSeconds = totalSeconds % intervalSeconds;
        if (remainingSeconds > 0)
        {
            std::cout << "main() running the remaining " << remainingSeconds << " seconds of main code" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(remainingSeconds));
        }
        std::cout << "Data collection completed successfully." << std::endl;
        return true; // 正常执行完成，返回true
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception caught in DataCollector::start: " << e.what() << std::endl;
        return false; // 捕获异常并返回false
    }
}
