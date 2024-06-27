#include "file_transfer/FileSend.h"

std::string server = "tstit.x3322.net";
int port = 12345;
fs::path dir_path = "/mnt/hgfs/share/DataSet/B";

int main()
{
    std::unique_ptr<FileSend> file_send = file_send->createNew(server, port, dir_path);
    if (!file_send->start())
    {
        std::cerr << "Error: FileSend failed." << std::endl;
        return 0;
    }
    return 0;
}
