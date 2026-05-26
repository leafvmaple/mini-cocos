#pragma once

#include "platform/ZCFileUtils.h"

namespace zocos {

class FileUtilsWin32 final : public FileUtils {
protected:
    bool readBinaryFileImpl(const mstd::string& path,
                            mstd::vector<unsigned char>& outData) const override;
    bool writeBinaryFileImpl(const mstd::string& path, const unsigned char* data,
                             mstd::size_t size) const override;
    mstd::string parentPathImpl(const mstd::string& path) const override;
    bool isAbsolutePathImpl(const mstd::string& path) const override;
    bool isFileExistImpl(const mstd::string& path) const override;
    bool isDirectoryExistImpl(const mstd::string& path) const override;
    mstd::uintmax_t getFileSizeImpl(const mstd::string& path) const override;
    mstd::string getWritablePathImpl() const override;
    bool createDirectoryImpl(const mstd::string& dirPath) const override;
    bool removeDirectoryImpl(const mstd::string& dirPath) const override;
    bool removeFileImpl(const mstd::string& path) const override;
    bool renameFileImpl(const mstd::string& oldPath, const mstd::string& newPath) const override;
    mstd::string normalizePathImpl(const mstd::string& path) const override;
    mstd::string joinPathImpl(const mstd::string& directory,
                             const mstd::string& filename) const override;
};

} // namespace zocos
