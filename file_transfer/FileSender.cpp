#include "FileSender.h"

std::unique_ptr<FileSender> FileSender::createNew(const std::string &server, int port)
{
    return std::make_unique<FileSender>(server, port);
}

FileSender::FileSender(const std::string &server, int port)
    : server_(server), port_(port), isConnected_(false)
{
    network_handler_ = NetworkHandler::createNew(server_, port_); // �ڹ��캯���д���NetworkHandler
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
        // �������״̬�����û�����ӣ����Խ�������
        if (!isConnected_)
        {
            if (!network_handler_->connect())
            {
                std::cerr << "Initial connection failed." << std::endl;
                return false;
            }
            isConnected_ = true;
        }
        // ����Ŀ¼������ÿ���ļ�
        for (const auto &entry : fs::recursive_directory_iterator(dir_path))
        {
            if (fs::is_regular_file(entry))
            {
                std::unique_ptr<FileHandler> file_handler = FileHandler::createNew(entry.path());

                // �����ļ�
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