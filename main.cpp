#include "file_transfer/FileSendingTask.h"
#include "data_collection/DataCollectionTask.h"
#include "shared/SharedQueue.h"

std::string server = "tstit.x3322.net";
int port = 12345;

int main()
{
    /* std::unique_ptr 智能指针，管理动态分配的对象 */
    /*
     *  data_collector::createNew()静态成员函数，创建了一个data_collector对象，
     *  包装在一个 std::unique_ptr 中返回
     *  std::unique_ptr 被用来初始化 data_collector
     */


    // // 创建并启动数据采集线程
    // std::thread dataCollectionThread(dataCollectionTask);
    // // 创建并启动文件发送线程
    // std::thread fileSenderThread(fileSendingTask, server, port);

    // while (True)
    // {
    //     std::cout << "main() running 10s main() Programme" << std::endl;

    //     std::this_thread::sleep_for(std::chrono::seconds(5)); // 主线程每5秒打印一次检测结果
    //     /*
    //      * std::this_thread::sleep_for:使当前线程暂停执行指定的时间段
    //      * std::chrono::seconds(5)：这是<chrono>头文件中的一个时间长度表示，表示5秒的时间间隔。
    //      * std::chrono命名空间提供了表示时间间隔和时钟的时间单位。
    //      */
    // }
    // // 在退出之前等待数据采集线程结束
    // dataCollectionThread.join();

    // // 在退出之前等待文件发送线程结束
    // fileSenderThread.join();

    /*******************************测试用例***************************************************** */
    {
        // 创建并启动数据采集线程
        std::thread dataCollectionThread(dataCollectionTask);
        // 创建并启动文件发送线程
        std::thread fileSenderThread(fileSendingTask, server, port);

        // 测试时使用，持续10s
        bool testFlag = true;
        int testTime = 2;
        while (testFlag)
        //  while (True)
        {
            std::cout << "main() running 10s main() Programme" << std::endl;

            std::this_thread::sleep_for(std::chrono::seconds(5)); // 主线程每5秒打印一次检测结果
            /*
            * std::this_thread::sleep_for:使当前线程暂停执行指定的时间段
            * std::chrono::seconds(5)：这是<chrono>头文件中的一个时间长度表示，表示5秒的时间间隔。
            * std::chrono命名空间提供了表示时间间隔和时钟的时间单位。
            */
            // 测试时使用，持续10s
            testTime--;
            if(0 == testTime){
                std::cout<<"Main Program is ending ! 10s Arrived ！" << std::endl;
                testFlag = false;
                std::cout<<"Main Program is waiting for 5 seconds to Stop ! " << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    }
    /*******************************测试用例***************************************************** */
    
    return 0;
}
