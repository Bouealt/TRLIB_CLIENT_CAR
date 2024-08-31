#ifndef INETWORKHANDLER_H
#define INETWORKHANDLER_H

#include "IFileHandler.h"

class INetworkHandler {
public:
    virtual ~INetworkHandler() = default;
    virtual bool connect() = 0;
    virtual void sendFile(IFileHandler& file_handler) = 0;
    virtual void close() = 0;
};

#endif // INETWORKHANDLER_H
