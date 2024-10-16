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

    bool processDirectories(); // ����Ŀ¼�ĺ���������Ϣ�����л�ȡĿ¼���д���

private:
    // ʱ�䴰�ڴ�С���Ժ���Ϊ��λ��
    const int TIME_WINDOW_MS = 200;
};

#endif // DATA_PROCESSING_H
