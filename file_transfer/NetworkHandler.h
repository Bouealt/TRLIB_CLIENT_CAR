#ifndef NETWORKHANDLER_H
#define NETWORKHANDLER_H

#include "INetworkHandler.h"
#include <string>
#include <functional> // 为了使用回调函数
#include <mutex>

class NetworkHandler : public INetworkHandler
{
public:
    static std::unique_ptr<NetworkHandler> createNew(const std::string &server, int port);
    NetworkHandler(const std::string &server, int port);

    bool connect() override;
    void close() override;
    void sendFile(IFileHandler &file_handler) override;
    bool isConnected() const; // 检查是否连接

    // 设置断开连接时的回调函数
    void setDisconnectCallback(const std::function<void()> &callback);

private:
    std::string server_;
    int port_;
    int sockfd_;
    bool connected_ = false;             // 记录当前连接状态
    std::function<void()> onDisconnect_; // 断开连接的回调函数
    static std::mutex mtx;               // 发送文件时的线程同步
};

#endif // NETWORKHANDLER_H
