#include "file_transfer/FileSendingTask.h"
#include "data_collection/DataCollectionTask.h"
#include "shared/SharedQueue.h"

std::string server = "tstit.x3322.net";
int port = 12345;

int main()
{
    /* std::unique_ptr ����ָ�룬����̬����Ķ��� */
    /*
     *  data_collector::createNew()��̬��Ա������������һ��data_collector����
     *  ��װ��һ�� std::unique_ptr �з���
     *  std::unique_ptr ��������ʼ�� data_collector
     */


    // // �������������ݲɼ��߳�
    // std::thread dataCollectionThread(dataCollectionTask);
    // // �����������ļ������߳�
    // std::thread fileSenderThread(fileSendingTask, server, port);

    // while (True)
    // {
    //     std::cout << "main() running 10s main() Programme" << std::endl;

    //     std::this_thread::sleep_for(std::chrono::seconds(5)); // ���߳�ÿ5���ӡһ�μ����
    //     /*
    //      * std::this_thread::sleep_for:ʹ��ǰ�߳���ִͣ��ָ����ʱ���
    //      * std::chrono::seconds(5)������<chrono>ͷ�ļ��е�һ��ʱ�䳤�ȱ�ʾ����ʾ5���ʱ������
    //      * std::chrono�����ռ��ṩ�˱�ʾʱ������ʱ�ӵ�ʱ�䵥λ��
    //      */
    // }
    // // ���˳�֮ǰ�ȴ����ݲɼ��߳̽���
    // dataCollectionThread.join();

    // // ���˳�֮ǰ�ȴ��ļ������߳̽���
    // fileSenderThread.join();

    /*******************************��������***************************************************** */
    {
        // �������������ݲɼ��߳�
        std::thread dataCollectionThread(dataCollectionTask);
        // �����������ļ������߳�
        std::thread fileSenderThread(fileSendingTask, server, port);

        // ����ʱʹ�ã�����10s
        bool testFlag = true;
        int testTime = 2;
        while (testFlag)
        //  while (True)
        {
            std::cout << "main() running 10s main() Programme" << std::endl;

            std::this_thread::sleep_for(std::chrono::seconds(5)); // ���߳�ÿ5���ӡһ�μ����
            /*
            * std::this_thread::sleep_for:ʹ��ǰ�߳���ִͣ��ָ����ʱ���
            * std::chrono::seconds(5)������<chrono>ͷ�ļ��е�һ��ʱ�䳤�ȱ�ʾ����ʾ5���ʱ������
            * std::chrono�����ռ��ṩ�˱�ʾʱ������ʱ�ӵ�ʱ�䵥λ��
            */
            // ����ʱʹ�ã�����10s
            testTime--;
            if(0 == testTime){
                std::cout<<"Main Program is ending ! 10s Arrived ��" << std::endl;
                testFlag = false;
                std::cout<<"Main Program is waiting for 5 seconds to Stop ! " << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    }
    /*******************************��������***************************************************** */
    
    return 0;
}
