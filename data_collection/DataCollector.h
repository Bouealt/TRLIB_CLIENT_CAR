#ifndef DATA_COLLECTOR_H
#define DATA_COLLECTOR_H

#include <iostream>
#include <thread>
#include <future>
#include "PerceptionDeviceManager.h"
#include "CameraThreadManager.h"

class DataCollector
{
public:
    static std::unique_ptr<DataCollector> createNew();
    DataCollector();
    bool start(int totalSeconds);

private:


};


#endif // DATA_COLLECTOR_H