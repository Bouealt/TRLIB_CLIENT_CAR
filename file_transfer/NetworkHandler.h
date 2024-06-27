#ifndef NETWORKHANDLER_H
#define NETWORKHANDLER_H

#include "INetworkHandler.h"
#include <string>
#include <mutex>

class NetworkHandler : public INetworkHandler
{
public:
    static std::unique_ptr<NetworkHandler> createNew(const std::string &server, int port);
    NetworkHandler(const std::string &server, int port);
    bool connect() override;
    void sendFile(IFileHandler &file_handler) override;
    void close() override;

private:
    std::string server_;
    int port_;
    int sockfd_;
    static std::mutex mtx;
};

#endif // NETWORKHANDLER_H
