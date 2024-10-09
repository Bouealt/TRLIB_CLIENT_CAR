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

    bool processDirectories();  // ����Ŀ¼�ĺ���������Ϣ�����л�ȡĿ¼���д���

private:
    void alignAndSaveImages(const std::string &baseDir, const std::map<int, std::map<std::string, std::string>> &alignedImages); // ���벢����ͼ��

    static constexpr int timeWindow = 200;  // ʱ�䴰�ڴ�С�����룩
};

#endif // DATA_PROCESSING_H
