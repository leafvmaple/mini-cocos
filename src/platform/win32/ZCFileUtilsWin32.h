#pragma once

#include "platform/ZCFileUtils.h"

#include <cstddef>
#include <cstdint>

namespace zocos {

class FileUtilsWin32 final : public FileUtils {
protected:
    bool readBinaryFileImpl(const std::string& path,
                            std::vector<unsigned char>& outData) const override;
    bool writeBinaryFileImpl(const std::string& path, const unsigned char* data,
                             std::size_t size) const override;
    std::string parentPathImpl(const std::string& path) const override;
    bool isAbsolutePathImpl(const std::string& path) const override;
    bool isFileExistImpl(const std::string& path) const override;
    bool isDirectoryExistImpl(const std::string& path) const override;
    std::uintmax_t getFileSizeImpl(const std::string& path) const override;
    std::string getWritablePathImpl() const override;
    bool createDirectoryImpl(const std::string& dirPath) const override;
    bool removeDirectoryImpl(const std::string& dirPath) const override;
    bool removeFileImpl(const std::string& path) const override;
    bool renameFileImpl(const std::string& oldPath, const std::string& newPath) const override;
    std::string normalizePathImpl(const std::string& path) const override;
    std::string joinPathImpl(const std::string& directory,
                             const std::string& filename) const override;
};

} // namespace zocos
