#include "DataCollectionTask.h"
#include <iostream>
#include <thread>

// �ɼ�ģ���߳�ִ�еĺ���
void dataCollectionTask()
{
    std::unique_ptr<DataCollector> data_collector = DataCollector::createNew();
    if (!data_collector->DataCollectorLoopStart())
    {
        std::cerr << "Error: DataCollector failed." << std::endl;
        return;
    }
    while (cKeepRunning)
    {
    }

    // �ɼ�ģ��������߼����Է�������
}
