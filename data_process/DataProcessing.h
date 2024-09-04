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

class DataProcessing
{
public:
    static std::unique_ptr<DataProcessing> createNew();
    DataProcessing();
    ~DataProcessing();

    bool processDirectories();  // 处理目录的函数，从消息队列中获取目录进行处理

private:
    void alignAndSaveImages(const std::string &baseDir, const std::map<int, std::map<std::string, std::string>> &alignedImages); // 对齐并保存图像

    static constexpr int timeWindow = 200;  // 时间窗口大小（毫秒）
};

#endif // DATA_PROCESSING_H
