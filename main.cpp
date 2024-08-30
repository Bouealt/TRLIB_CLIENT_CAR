#include "file_transfer/FileSender.h"
#include "data_collection/DataCollector.h"

std::string server = "tstit.x3322.net";
int port = 12345;
fs::path dir_path = "./data_collection/dataCapture";

int main()
{
    {
        std::unique_ptr<DataCollector> data_collector = data_collector->createNew();
        if (!data_collector->start(2))
        {
            std::cerr << "Error: DataCollector failed." << std::endl;
            return 0;
        }
    }
    std::cout<<"dataend"<<std::endl;
    std::unique_ptr<FileSender> file_send = file_send->createNew(server, port, dir_path);
    std::cout<<"file_send->start()"<<std::endl;
    if (!file_send->start())
    {
        std::cerr << "Error: FileSend failed." << std::endl;
        return 0;
    }

    return 0;
}
