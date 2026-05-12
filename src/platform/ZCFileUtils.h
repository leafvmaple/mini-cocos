#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace zocos {

class FileUtils {
public:
    virtual ~FileUtils() = default;

    static FileUtils& getInstance();

    bool getDataFromFile(const std::string& filename, std::vector<unsigned char>& outData) const;
    std::string getStringFromFile(const std::string& filename) const;
    bool writeStringToFile(const std::string& dataStr, const std::string& fullPath) const;

    bool readBinaryFile(const std::string& path, std::vector<unsigned char>& outData) const;
    std::string fullPathForFilename(const std::string& filename) const;
    std::string parentPath(const std::string& path) const;

    bool isAbsolutePath(const std::string& path) const;
    bool isFileExist(const std::string& path) const;
    bool isDirectoryExist(const std::string& path) const;
    std::uintmax_t getFileSize(const std::string& path) const;

    std::string getWritablePath() const;
    bool createDirectory(const std::string& dirPath) const;
    bool removeDirectory(const std::string& dirPath) const;
    bool removeFile(const std::string& path) const;
    bool renameFile(const std::string& oldPath, const std::string& newPath) const;

    void setSearchPaths(const std::vector<std::string>& searchPaths);
    void addSearchPath(const std::string& path, bool front = false);
    const std::vector<std::string>& getSearchPaths() const;

    void setSearchResolutionsOrder(const std::vector<std::string>& searchResolutionsOrder);
    void addSearchResolutionsOrder(const std::string& order, bool front = false);
    const std::vector<std::string>& getSearchResolutionsOrder() const;

    void setDefaultResourceRootPath(const std::string& path);
    const std::string& getDefaultResourceRootPath() const;

protected:
    FileUtils();

    virtual bool readBinaryFileImpl(const std::string& path,
                                    std::vector<unsigned char>& outData) const = 0;
    virtual bool writeBinaryFileImpl(const std::string& path, const unsigned char* data,
                                     std::size_t size) const = 0;
    virtual std::string parentPathImpl(const std::string& path) const = 0;
    virtual bool isAbsolutePathImpl(const std::string& path) const = 0;
    virtual bool isFileExistImpl(const std::string& path) const = 0;
    virtual bool isDirectoryExistImpl(const std::string& path) const = 0;
    virtual std::uintmax_t getFileSizeImpl(const std::string& path) const = 0;
    virtual std::string getWritablePathImpl() const = 0;
    virtual bool createDirectoryImpl(const std::string& dirPath) const = 0;
    virtual bool removeDirectoryImpl(const std::string& dirPath) const = 0;
    virtual bool removeFileImpl(const std::string& path) const = 0;
    virtual bool renameFileImpl(const std::string& oldPath, const std::string& newPath) const = 0;
    virtual std::string normalizePathImpl(const std::string& path) const = 0;
    virtual std::string joinPathImpl(const std::string& directory,
                                     const std::string& filename) const = 0;

private:
    std::vector<std::string> _searchPaths;
    std::vector<std::string> _searchResolutionsOrder;
    std::string _defaultResourceRootPath;
};

} // namespace zocos
