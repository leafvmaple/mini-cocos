#pragma once

#include <cstddef>
#include <cstdint>
#include "base/ZCStd.h"

namespace zocos {

class FileUtils {
public:
    virtual ~FileUtils() = default;

    static FileUtils& getInstance();

    bool getDataFromFile(const mstd::string& filename, mstd::vector<unsigned char>& outData) const;
    mstd::string getStringFromFile(const mstd::string& filename) const;
    bool writeStringToFile(const mstd::string& dataStr, const mstd::string& fullPath) const;

    bool readBinaryFile(const mstd::string& path, mstd::vector<unsigned char>& outData) const;
    mstd::string fullPathForFilename(const mstd::string& filename) const;
    mstd::string parentPath(const mstd::string& path) const;

    bool isAbsolutePath(const mstd::string& path) const;
    bool isFileExist(const mstd::string& path) const;
    bool isDirectoryExist(const mstd::string& path) const;
    std::uintmax_t getFileSize(const mstd::string& path) const;

    mstd::string getWritablePath() const;
    bool createDirectory(const mstd::string& dirPath) const;
    bool removeDirectory(const mstd::string& dirPath) const;
    bool removeFile(const mstd::string& path) const;
    bool renameFile(const mstd::string& oldPath, const mstd::string& newPath) const;

    void setSearchPaths(const mstd::vector<mstd::string>& searchPaths);
    void addSearchPath(const mstd::string& path, bool front = false);
    const mstd::vector<mstd::string>& getSearchPaths() const;

    void setSearchResolutionsOrder(const mstd::vector<mstd::string>& searchResolutionsOrder);
    void addSearchResolutionsOrder(const mstd::string& order, bool front = false);
    const mstd::vector<mstd::string>& getSearchResolutionsOrder() const;

    void setDefaultResourceRootPath(const mstd::string& path);
    const mstd::string& getDefaultResourceRootPath() const;

protected:
    FileUtils();

    virtual bool readBinaryFileImpl(const mstd::string& path,
                                    mstd::vector<unsigned char>& outData) const = 0;
    virtual bool writeBinaryFileImpl(const mstd::string& path, const unsigned char* data,
                                     mstd::size_t size) const = 0;
    virtual mstd::string parentPathImpl(const mstd::string& path) const = 0;
    virtual bool isAbsolutePathImpl(const mstd::string& path) const = 0;
    virtual bool isFileExistImpl(const mstd::string& path) const = 0;
    virtual bool isDirectoryExistImpl(const mstd::string& path) const = 0;
    virtual std::uintmax_t getFileSizeImpl(const mstd::string& path) const = 0;
    virtual mstd::string getWritablePathImpl() const = 0;
    virtual bool createDirectoryImpl(const mstd::string& dirPath) const = 0;
    virtual bool removeDirectoryImpl(const mstd::string& dirPath) const = 0;
    virtual bool removeFileImpl(const mstd::string& path) const = 0;
    virtual bool renameFileImpl(const mstd::string& oldPath, const mstd::string& newPath) const = 0;
    virtual mstd::string normalizePathImpl(const mstd::string& path) const = 0;
    virtual mstd::string joinPathImpl(const mstd::string& directory,
                                     const mstd::string& filename) const = 0;

private:
    mstd::vector<mstd::string> _searchPaths;
    mstd::vector<mstd::string> _searchResolutionsOrder;
    mstd::string _defaultResourceRootPath;
};

} // namespace zocos
