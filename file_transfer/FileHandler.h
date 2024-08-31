#ifndef FILEHANDLER_H
#define FILEHANDLER_H

#include "IFileHandler.h"
#include <openssl/md5.h>
#include <filesystem>

namespace fs = std::filesystem;

class FileHandler : public IFileHandler
{
public:
    static std::unique_ptr<FileHandler> createNew(const fs::path &file_path);
    FileHandler(const fs::path &file_path);
    std::string calculateMd5() override;
    std::vector<char> readChunk(size_t chunk_size) override;
    size_t fileSize() const override;
    std::string fileName() const override;

private:
    fs::path file_path_;
};

#endif // FILEHANDLER_H
