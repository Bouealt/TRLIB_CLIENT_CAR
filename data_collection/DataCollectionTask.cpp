#include "DataCollectionTask.h"
#include <iostream>
#include <thread>

// 采集模块线程执行的函数
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

    // 采集模块的其他逻辑可以放在这里
}
