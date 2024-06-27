#include "FileHandler.h"
#include <fstream>
#include <stdexcept>

const size_t CHUNK_SIZE = 4096;

std::unique_ptr<FileHandler> FileHandler::createNew(const fs::path &file_path)
{
    return std::make_unique<FileHandler>(file_path);
}
FileHandler::FileHandler(const fs::path &file_path) : file_path_(file_path) {}

std::string FileHandler::calculateMd5()
{
    std::ifstream file(file_path_, std::ios::binary);
    if (!file)
    {
        throw std::runtime_error("Failed to open file: " + file_path_.string());
    }

    MD5_CTX md5Context;
    MD5_Init(&md5Context);

    std::vector<char> buffer(CHUNK_SIZE);

    while (file.read(buffer.data(), buffer.size()))
    {
        MD5_Update(&md5Context, buffer.data(), buffer.size());
    }
    MD5_Update(&md5Context, buffer.data(), file.gcount());

    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5_Final(digest, &md5Context);

    std::string result;
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i)
    {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02x", digest[i]);
        result.append(buf);
    }
    return result;
}

std::vector<char> FileHandler::readChunk(size_t chunk_size)
{
    std::ifstream file(file_path_, std::ios::binary);
    if (!file)
    {
        throw std::runtime_error("Failed to open file: " + file_path_.string());
    }

    std::vector<char> buffer(chunk_size);
    file.read(buffer.data(), chunk_size);
    return buffer;
}

size_t FileHandler::fileSize() const
{
    return fs::file_size(file_path_);
}

std::string FileHandler::fileName() const
{
    return file_path_.string();
}
