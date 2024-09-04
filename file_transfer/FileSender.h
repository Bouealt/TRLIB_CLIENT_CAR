#ifndef FILESEND_H
#define FILESEND_H

#include <iostream>
#include <thread>
#include <future>
#include "FileHandler.h"
#include "NetworkHandler.h"
#include "../shared/SharedQueue.h"

class FileSender
{
public:
    static std::unique_ptr<FileSender> createNew(const std::string &server, int port);
    FileSender(const std::string &server, int port);
    bool start(fs::path dir_path);
    bool reconnect();
    
private:
    std::string server_;
    int port_;
    bool isConnected_; 
    std::unique_ptr<NetworkHandler> network_handler_; // NetworkHandler作为成员变量
};

#endif // FILESEND_H