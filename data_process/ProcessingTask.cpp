#include "ProcessingTask.h"
#include <iostream>
#include <thread>

void ProcessingTask()
{
    std::unique_ptr<DataProcessing> data_processing = DataProcessing::createNew();
    if(!data_processing->processDirectories())
    {
        std::cerr << "Error: DataProcessing failed." << std::endl;
        return;
    }
    while(cKeepRunning)
    {
    }
}