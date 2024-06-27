#ifndef IFILEHANDLER_H
#define IFILEHANDLER_H

#include <string>
#include <vector>
#include <memory>

class IFileHandler
{
public:
    virtual ~IFileHandler() = default;
    virtual std::string calculateMd5() = 0;
    virtual std::vector<char> readChunk(size_t chunk_size) = 0;
    virtual size_t fileSize() const = 0;
    virtual std::string fileName() const = 0;
};

#endif // IFILEHANDLER_H
