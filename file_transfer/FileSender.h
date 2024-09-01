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
    static std::unique_ptr<FileSender> createNew(std::string server, int port);
    FileSender(std::string server, int port);
    bool start(fs::path dir_path);

private:
    std::string server_;
    int port_;
};

#endif // FILESEND_H