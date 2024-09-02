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

    // �������������ݲɼ��߳�
    std::thread dataCollectionThread(dataCollectionTask);
    // �����������ļ������߳�
    std::thread fileSenderThread(fileSendingTask, server, port);

    while (true)
    {
        std::cout << "main() running 10s main() Programme" << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(10)); // ���߳�ÿ10���ӡһ�μ����
        /*
         * std::this_thread::sleep_for:ʹ��ǰ�߳���ִͣ��ָ����ʱ���
         * std::chrono::seconds(10)������<chrono>ͷ�ļ��е�һ��ʱ�䳤�ȱ�ʾ����ʾ10���ʱ������
         * std::chrono�����ռ��ṩ�˱�ʾʱ������ʱ�ӵ�ʱ�䵥λ��
         */
    }

    // ���˳�֮ǰ�ȴ����ݲɼ��߳̽���
    dataCollectionThread.join();

    // ���˳�֮ǰ�ȴ��ļ������߳̽���
    fileSenderThread.join();
    return 0;
}
