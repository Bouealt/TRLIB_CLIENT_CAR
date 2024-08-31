#ifndef FILESEND_H
#define FILESEND_H

#include <iostream>
#include <thread>
#include <future>
#include "FileHandler.h"
#include "NetworkHandler.h"

class FileSender
{
public:
    static std::unique_ptr<FileSender> createNew(std::string server, int port, fs::path dir_path);
    FileSender(std::string server, int port, fs::path dir_path);
    bool start();

private:
    std::string server_;
    int port_;
    fs::path dir_path_;
};

#endif // FILESEND_H