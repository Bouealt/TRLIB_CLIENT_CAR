#ifndef DATA_PROCESSING_H
#define DATA_PROCESSING_H

#include <map>
#include <vector>
#include <string>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include "../shared/SharedQueue.h"
namespace fs = std::filesystem;

class DataProcessing
{
public:
    static std::unique_ptr<DataProcessing> createNew();
    DataProcessing();
    ~DataProcessing();

    bool processDirectories(); // 处理目录的函数，从消息队列中获取目录进行处理

private:
    // 时间窗口大小（以毫秒为单位）
    const int TIME_WINDOW_MS = 200;
};

#endif // DATA_PROCESSING_H
