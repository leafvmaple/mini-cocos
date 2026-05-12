#include "platform/ZCFileUtils.h"

#if !defined(_WIN32)
#error "FileUtils currently requires a Win32 implementation."
#endif

#include "platform/win32/ZCFileUtilsWin32.h"

#include <algorithm>

namespace zocos {

FileUtils::FileUtils() {
    _searchPaths.emplace_back(".");
    _searchResolutionsOrder.emplace_back("");
}

FileUtils& FileUtils::getInstance() {
    static FileUtilsWin32 instance;
    return instance;
}

bool FileUtils::getDataFromFile(const std::string& filename,
                                std::vector<unsigned char>& outData) const {
    return readBinaryFile(filename, outData);
}

std::string FileUtils::getStringFromFile(const std::string& filename) const {
    std::vector<unsigned char> bytes;
    if (!getDataFromFile(filename, bytes)) {
        return {};
    }
    return std::string(bytes.begin(), bytes.end());
}

bool FileUtils::writeStringToFile(const std::string& dataStr, const std::string& fullPath) const {
    if (fullPath.empty()) {
        return false;
    }

    const std::string normalizedPath = normalizePathImpl(fullPath);
    if (normalizedPath.empty()) {
        return false;
    }

    const std::string dirPath = parentPathImpl(normalizedPath);
    if (!dirPath.empty() && !isDirectoryExistImpl(dirPath) && !createDirectoryImpl(dirPath)) {
        return false;
    }

    const auto* bytes = reinterpret_cast<const unsigned char*>(dataStr.data());
    return writeBinaryFileImpl(normalizedPath, bytes, dataStr.size());
}

bool FileUtils::readBinaryFile(const std::string& path,
                               std::vector<unsigned char>& outData) const {
    outData.clear();
    if (path.empty()) {
        return false;
    }

    const std::string fullPath = fullPathForFilename(path);
    if (fullPath.empty()) {
        return false;
    }

    return readBinaryFileImpl(fullPath, outData);
}

std::string FileUtils::fullPathForFilename(const std::string& filename) const {
    if (filename.empty()) {
        return {};
    }

    if (isAbsolutePath(filename)) {
        return isFileExistImpl(filename) ? normalizePathImpl(filename) : std::string{};
    }

    if (isFileExistImpl(filename)) {
        return normalizePathImpl(filename);
    }

    for (const std::string& searchPath : _searchPaths) {
        const std::string rootPath = _defaultResourceRootPath.empty()
                                         ? searchPath
                                         : joinPathImpl(_defaultResourceRootPath, searchPath);
        for (const std::string& resolutionOrder : _searchResolutionsOrder) {
            const std::string resolvedBasePath =
                resolutionOrder.empty() ? rootPath : joinPathImpl(rootPath, resolutionOrder);
            const std::string candidatePath =
                resolvedBasePath.empty() ? filename : joinPathImpl(resolvedBasePath, filename);
            if (isFileExistImpl(candidatePath)) {
                return normalizePathImpl(candidatePath);
            }
        }
    }

    return {};
}

std::string FileUtils::parentPath(const std::string& path) const {
    if (path.empty()) {
        return {};
    }
    return parentPathImpl(normalizePathImpl(path));
}

bool FileUtils::isAbsolutePath(const std::string& path) const {
    if (path.empty()) {
        return false;
    }
    return isAbsolutePathImpl(path);
}

bool FileUtils::isFileExist(const std::string& path) const {
    if (path.empty()) {
        return false;
    }
    return !fullPathForFilename(path).empty();
}

bool FileUtils::isDirectoryExist(const std::string& path) const {
    if (path.empty()) {
        return false;
    }

    const std::string normalizedPath = normalizePathImpl(path);
    if (normalizedPath.empty()) {
        return false;
    }
    return isDirectoryExistImpl(normalizedPath);
}

std::uintmax_t FileUtils::getFileSize(const std::string& path) const {
    if (path.empty()) {
        return 0;
    }

    const std::string fullPath = fullPathForFilename(path);
    if (fullPath.empty()) {
        return 0;
    }
    return getFileSizeImpl(fullPath);
}

std::string FileUtils::getWritablePath() const {
    return getWritablePathImpl();
}

bool FileUtils::createDirectory(const std::string& dirPath) const {
    if (dirPath.empty()) {
        return false;
    }
    return createDirectoryImpl(normalizePathImpl(dirPath));
}

bool FileUtils::removeDirectory(const std::string& dirPath) const {
    if (dirPath.empty()) {
        return false;
    }
    return removeDirectoryImpl(normalizePathImpl(dirPath));
}

bool FileUtils::removeFile(const std::string& path) const {
    if (path.empty()) {
        return false;
    }

    const std::string fullPath = fullPathForFilename(path);
    if (fullPath.empty()) {
        return false;
    }
    return removeFileImpl(fullPath);
}

bool FileUtils::renameFile(const std::string& oldPath, const std::string& newPath) const {
    if (oldPath.empty() || newPath.empty()) {
        return false;
    }

    const std::string oldFullPath = fullPathForFilename(oldPath);
    if (oldFullPath.empty()) {
        return false;
    }

    const std::string newFullPath = normalizePathImpl(newPath);
    if (newFullPath.empty()) {
        return false;
    }

    const std::string parentDir = parentPathImpl(newFullPath);
    if (!parentDir.empty() && !isDirectoryExistImpl(parentDir) && !createDirectoryImpl(parentDir)) {
        return false;
    }

    return renameFileImpl(oldFullPath, newFullPath);
}

void FileUtils::setSearchPaths(const std::vector<std::string>& searchPaths) {
    _searchPaths.clear();
    for (const std::string& path : searchPaths) {
        addSearchPath(path);
    }

    if (_searchPaths.empty()) {
        _searchPaths.emplace_back(".");
    }
}

void FileUtils::addSearchPath(const std::string& path, bool front) {
    if (path.empty()) {
        return;
    }

    const std::string normalizedPath = normalizePathImpl(path);
    if (normalizedPath.empty()) {
        return;
    }

    if (std::find(_searchPaths.begin(), _searchPaths.end(), normalizedPath) != _searchPaths.end()) {
        return;
    }

    if (front) {
        _searchPaths.insert(_searchPaths.begin(), normalizedPath);
    } else {
        _searchPaths.push_back(normalizedPath);
    }
}

const std::vector<std::string>& FileUtils::getSearchPaths() const {
    return _searchPaths;
}

void FileUtils::setSearchResolutionsOrder(
    const std::vector<std::string>& searchResolutionsOrder) {
    _searchResolutionsOrder.clear();
    for (const std::string& order : searchResolutionsOrder) {
        addSearchResolutionsOrder(order);
    }

    if (_searchResolutionsOrder.empty()) {
        _searchResolutionsOrder.emplace_back("");
    }
}

void FileUtils::addSearchResolutionsOrder(const std::string& order, bool front) {
    const std::string normalizedOrder = normalizePathImpl(order);
    if (order.empty() || normalizedOrder.empty()) {
        if (order.empty() && std::find(_searchResolutionsOrder.begin(), _searchResolutionsOrder.end(), "")
                                       == _searchResolutionsOrder.end()) {
            if (front) {
                _searchResolutionsOrder.insert(_searchResolutionsOrder.begin(), "");
            } else {
                _searchResolutionsOrder.emplace_back("");
            }
        }
        return;
    }

    if (std::find(_searchResolutionsOrder.begin(), _searchResolutionsOrder.end(), normalizedOrder)
        != _searchResolutionsOrder.end()) {
        return;
    }

    if (front) {
        _searchResolutionsOrder.insert(_searchResolutionsOrder.begin(), normalizedOrder);
    } else {
        _searchResolutionsOrder.push_back(normalizedOrder);
    }
}

const std::vector<std::string>& FileUtils::getSearchResolutionsOrder() const {
    return _searchResolutionsOrder;
}

void FileUtils::setDefaultResourceRootPath(const std::string& path) {
    if (path.empty()) {
        _defaultResourceRootPath.clear();
        return;
    }

    _defaultResourceRootPath = normalizePathImpl(path);
}

const std::string& FileUtils::getDefaultResourceRootPath() const {
    return _defaultResourceRootPath;
}

} // namespace zocos
