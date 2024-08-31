#include "file_transfer/FileSender.h"
#include "data_collection/DataCollector.h"

std::string server = "tstit.x3322.net";
int port = 12345;
fs::path dir_path = "./data_collection/dataCapture";

int main()
{
    /* std::unique_ptr 智能指针，管理动态分配的对象 */
    /* 
    *  data_collector::createNew()静态成员函数，创建了一个data_collector对象，
    *  包装在一个 std::unique_ptr 中返回 
    *  std::unique_ptr 被用来初始化 data_collector
    */
     std::unique_ptr<DataCollector> data_collector = DataCollector::createNew();
    if (!data_collector->DataCollectorLoopStart())
    {
        std::cerr << "Error: DataCollector failed." << std::endl;
        return 0;
    }

    while (true) {
        std::cout << "main() running 10s main() Programme" << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(10)); // 主线程每10秒打印一次检测结果
        /*
        * std::this_thread::sleep_for:使当前线程暂停执行指定的时间段
        * std::chrono::seconds(10)：这是<chrono>头文件中的一个时间长度表示，表示10秒的时间间隔。
        * std::chrono命名空间提供了表示时间间隔和时钟的时间单位。
        */
    }


    std::cout<<"data end"<<std::endl;
    std::unique_ptr<FileSender> file_send = file_send->createNew(server, port, dir_path);
    std::cout<<"file_send->start()"<<std::endl;
    if (!file_send->start())
    {
        std::cerr << "Error: FileSend failed." << std::endl;
        return 0;
    }

    return 0;
}
