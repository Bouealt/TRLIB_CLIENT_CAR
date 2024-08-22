#include "FileSender.h"

std::unique_ptr<FileSender> FileSender::createNew(std::string server, int port, fs::path dir_path)
{
    return std::make_unique<FileSender>(server, port, dir_path);
}

FileSender::FileSender(std::string server, int port, fs::path dir_path)
    : server_(server), port_(port), dir_path_(dir_path)
{
}

bool FileSender::start()
{
    try
    {
        // 创建一个 NetworkHandler 实例并尝试连接服务器
        std::unique_ptr<NetworkHandler> network_handler = NetworkHandler::createNew(server_, port_);
        if (!network_handler->connect())
        {
            std::cerr << "Failed to connect to server." << std::endl;
            return false;
        }
        // 遍历目录并处理每个文件
        for (const auto &entry : fs::recursive_directory_iterator(dir_path_))
        {
            if (fs::is_regular_file(entry))
            {
                std::unique_ptr<FileHandler> file_handler = FileHandler::createNew(entry.path());
                network_handler->sendFile(*file_handler);
            }
        }

        // 所有文件处理完成后关闭连接
        network_handler->close();

        std::cout << "All files processed." << std::endl;
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
}


// bool FileSender::start()
// {
//     try
//     {
//         for (const auto &entry : fs::recursive_directory_iterator(dir_path_))
//         {
//             if (fs::is_regular_file(entry))
//             {
//                 std::unique_ptr<FileHandler> file_handler = FileHandler::createNew(entry.path());
//                 std::unique_ptr<NetworkHandler> network_handler = NetworkHandler::createNew(server_, port_);
//                 if (network_handler->connect())
//                 {
//                     network_handler->sendFile(*file_handler);
//                     network_handler->close();
//                 }
//             }
//         }

//         std::cout << "All files processed." << std::endl;
//         return true;
//     }
//     catch (const std::exception &e)
//     {
//         std::cerr << "Error: " << e.what() << std::endl;
//         return false;
//     }
// }