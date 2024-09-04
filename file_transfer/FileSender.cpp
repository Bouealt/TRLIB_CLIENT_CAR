#include "FileSender.h"

std::unique_ptr<FileSender> FileSender::createNew(const std::string &server, int port)
{
    return std::make_unique<FileSender>(server, port);
}

FileSender::FileSender(const std::string &server, int port)
    : server_(server), port_(port), isConnected_(false)
{
    network_handler_ = NetworkHandler::createNew(server_, port_); // 在构造函数中创建NetworkHandler
    network_handler_->setDisconnectCallback([this]()
                                            {
        std::cerr << "Connection lost, trying to reconnect..." << std::endl;
        if (!reconnect())
        {
            std::cerr << "Reconnection failed." << std::endl;
        } });
}

bool FileSender::reconnect()
{
    std::cerr << "Attempting to reconnect..." << std::endl;
    network_handler_->close();

    if (!network_handler_->connect())
    {
        std::cerr << "Reconnection failed." << std::endl;
        isConnected_ = false;
        return false;
    }

    std::cerr << "Reconnected successfully." << std::endl;
    isConnected_ = true;
    return true;
}

bool FileSender::start(fs::path dir_path)
{
    try
    {
        // 检查连接状态，如果没有连接，则尝试建立连接
        if (!isConnected_)
        {
            if (!network_handler_->connect())
            {
                std::cerr << "Initial connection failed." << std::endl;
                return false;
            }
            isConnected_ = true;
        }
        // 遍历目录并处理每个文件
        for (const auto &entry : fs::recursive_directory_iterator(dir_path))
        {
            if (fs::is_regular_file(entry))
            {
                std::unique_ptr<FileHandler> file_handler = FileHandler::createNew(entry.path());

                // 发送文件
                network_handler_->sendFile(*file_handler);
            }
        }

        std::cout << "All files in directory sent successfully." << std::endl;
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
}