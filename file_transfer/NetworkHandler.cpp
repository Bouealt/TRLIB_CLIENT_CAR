#include "NetworkHandler.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <cstring>
#include <iostream>
#include <fstream>

std::mutex NetworkHandler::mtx;
const size_t CHUNK_SIZE = 4096;

std::unique_ptr<NetworkHandler> NetworkHandler::createNew(const std::string &server, int port)
{
    return std::make_unique<NetworkHandler>(server, port);
}
NetworkHandler::NetworkHandler(const std::string &server, int port)
    : server_(server), port_(port) {}

bool NetworkHandler::connect()
{
    struct addrinfo hints, *res;
    int status;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(server_.c_str(), std::to_string(port_).c_str(), &hints, &res)) != 0)
    {
        std::cerr << "getaddrinfo: " << gai_strerror(status) << std::endl;
        return false;
    }
    sockfd_ = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd_ == -1)
    {
        std::cerr << "Error creating socket" << std::endl;
        freeaddrinfo(res);
        return false;
    }

    if (::connect(sockfd_, res->ai_addr, res->ai_addrlen) == -1)
    {
        perror("Connection Failed");
        close();
        freeaddrinfo(res);
        return false;
    }
    freeaddrinfo(res);
    connected_ = true;
    return true;
}
void NetworkHandler::close()
{
    if (sockfd_ != -1)
    {
        ::close(sockfd_);
        sockfd_ = -1;
    }
    connected_ = false;
}

bool NetworkHandler::isConnected() const
{
    return connected_;
}

void NetworkHandler::sendFile(IFileHandler &file_handler)
{
    if (!isConnected())
    {
        std::cerr << "Not connected to server." << std::endl;
        if (onDisconnect_)
            onDisconnect_(); // 如果连接断开，触发回调
        return;
    }

    try
    {
        std::string file_name = file_handler.fileName();
        uint32_t name_length = htonl(file_name.size());
        send(sockfd_, &name_length, sizeof(name_length), 0);
        send(sockfd_, file_name.c_str(), file_name.size(), 0);

        size_t file_size = file_handler.fileSize();
        uint32_t net_file_size = htonl(file_size);
        send(sockfd_, &net_file_size, sizeof(net_file_size), 0);

        std::ifstream file(file_handler.fileName(), std::ios::binary);
        std::vector<char> buffer(CHUNK_SIZE);
        size_t total_bytes_sent = 0;

        while (total_bytes_sent < file_size)
        {
            file.read(buffer.data(), CHUNK_SIZE);
            std::streamsize bytes_read = file.gcount();
            if (bytes_read > 0)
            {
                if (send(sockfd_, buffer.data(), bytes_read, 0) == -1)
                    throw std::runtime_error("Failed to send file data");

                total_bytes_sent += bytes_read;

                std::lock_guard<std::mutex> lock(mtx);
                std::cout << "\rProgress: " << (100 * total_bytes_sent / file_size) << "%" << std::flush;
            }
        }
        std::cout << std::endl;

        std::string md5_hash = file_handler.calculateMd5();
        uint32_t hash_length = htonl(md5_hash.size());
        send(sockfd_, &hash_length, sizeof(hash_length), 0);
        send(sockfd_, md5_hash.c_str(), md5_hash.size(), 0);

        std::lock_guard<std::mutex> lock(mtx);
        std::cout << "Sent file: " << file_name << " (" << file_size << " bytes), MD5: " << md5_hash << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error during file sending: " << e.what() << std::endl;
        if (onDisconnect_)
            onDisconnect_(); // 如果发生错误，触发断开连接回调
    }
}

void NetworkHandler::setDisconnectCallback(const std::function<void()> &callback)
{
    onDisconnect_ = callback;
}