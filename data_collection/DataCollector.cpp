#include "DataCollector.h"

std::unique_ptr<DataCollector> DataCollector::createNew()
{
    return std::make_unique<DataCollector>();
}

DataCollector::DataCollector()
{
}

bool DataCollector::start(int totalSeconds)
{
    try
    {
        PerceptionDeviceManager PDmanager;                    // �豸����̡߳� ʵ���������������д�����һ���߳�����
        std::this_thread::sleep_for(std::chrono::seconds(2)); // ���߳���ͣ1s��ʹ�豸��ʼ��

        const std::vector<std::string> &devices = PDmanager.getDevices();
        if (devices.empty())
        {
            std::cerr << "Error: No devices detected." << std::endl;
            return false;
        }
        CameraThreadManager threadManager(devices); // ����̹߳�����
        threadManager.start();
        // ��ӡ�߳���Ϣ
        const auto &threadInfoList = threadManager.getThreadInfoList();
        for (const auto &info : threadInfoList)
        {
            std::cout << "ThreadName: " << info.threadName << "\tThread ID: " << info.threadID << " \tcontrols device: " << info.deviceID << std::endl;
        }
        // �����豸�仯�ص�
        PDmanager.setDeviceChangeCallback([&threadManager](const std::vector<std::string> &newDevices, const std::vector<std::string> &offDevices)
                                          { threadManager.onDeviceChange(newDevices, offDevices); });
        // ���߳�ִ�к����߼������ݴ���� totalSeconds ������������Ҫѭ���Ĵ���
        const int intervalSeconds = 10;                  // ÿ��ѭ����ʱ����Ϊ10��
        int iterations = totalSeconds / intervalSeconds; // ������Ҫѭ���Ĵ���

        for (int i = 0; i < iterations; ++i)
        {
            std::cout << "main() running " << (i + 1) * intervalSeconds << " out of " << totalSeconds << " seconds of main code" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds)); // ���߳�ÿ10���ӡһ����Ϣ
        }

        // ����ʣ���ʱ�䣨����У������ⶪʧ����
        int remainingSeconds = totalSeconds % intervalSeconds;
        if (remainingSeconds > 0)
        {
            std::cout << "main() running the remaining " << remainingSeconds << " seconds of main code" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(remainingSeconds));
        }

        std::cout << "Data collection completed successfully." << std::endl;
        return true; // ����ִ����ɣ�����true
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception caught in DataCollector::start: " << e.what() << std::endl;
        return false; // �����쳣������false
    }
}
