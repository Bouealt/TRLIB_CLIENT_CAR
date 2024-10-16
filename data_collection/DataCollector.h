#ifndef DATA_COLLECTOR_H
#define DATA_COLLECTOR_H

#include <iostream>
#include <thread>
#include <future>
#include "PerceptionDeviceManager.h"
#include "CameraThreadManager.h"
#include "Audio.h"
#include "Imu.h"
class DataCollector
{
public:
    static std::unique_ptr<DataCollector> createNew();
    DataCollector();
    ~DataCollector();
    bool DataCollectorLoopStart(void );

private:
    std::unique_ptr<PerceptionDeviceManager> m_PDmanager;
    std::unique_ptr<CameraThreadManager> m_threadManager;

};


#endif // DATA_COLLECTOR_H