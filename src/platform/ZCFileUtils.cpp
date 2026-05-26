#include "platform/ZCFileUtils.h"

#if !defined(_WIN32)
#error "FileUtils currently requires a Win32 implementation."
#endif

#include "platform/win32/ZCFileUtilsWin32.h"

#include "base/ZCStd.h"

namespace zocos {

FileUtils::FileUtils() {
    _searchPaths.emplace_back(".");
    _searchResolutionsOrder.emplace_back("");
}

FileUtils& FileUtils::getInstance() {
    static FileUtilsWin32 instance;
    return instance;
}

bool FileUtils::getDataFromFile(const mstd::string& filename,
                                mstd::vector<unsigned char>& outData) const {
    return readBinaryFile(filename, outData);
}

mstd::string FileUtils::getStringFromFile(const mstd::string& filename) const {
    mstd::vector<unsigned char> bytes;
    if (!getDataFromFile(filename, bytes)) {
        return {};
    }
    return mstd::string(bytes.begin(), bytes.end());
}

bool FileUtils::writeStringToFile(const mstd::string& dataStr, const mstd::string& fullPath) const {
    if (fullPath.empty()) {
        return false;
    }

    const mstd::string normalizedPath = normalizePathImpl(fullPath);
    if (normalizedPath.empty()) {
        return false;
    }

    const mstd::string dirPath = parentPathImpl(normalizedPath);
    if (!dirPath.empty() && !isDirectoryExistImpl(dirPath) && !createDirectoryImpl(dirPath)) {
        return false;
    }

    const auto* bytes = reinterpret_cast<const unsigned char*>(dataStr.data());
    return writeBinaryFileImpl(normalizedPath, bytes, dataStr.size());
}

bool FileUtils::readBinaryFile(const mstd::string& path,
                               mstd::vector<unsigned char>& outData) const {
    outData.clear();
    if (path.empty()) {
        return false;
    }

    const mstd::string fullPath = fullPathForFilename(path);
    if (fullPath.empty()) {
        return false;
    }

    return readBinaryFileImpl(fullPath, outData);
}

mstd::string FileUtils::fullPathForFilename(const mstd::string& filename) const {
    if (filename.empty()) {
        return {};
    }

    if (isAbsolutePath(filename)) {
        return isFileExistImpl(filename) ? normalizePathImpl(filename) : mstd::string{};
    }

    if (isFileExistImpl(filename)) {
        return normalizePathImpl(filename);
    }

    for (const mstd::string& searchPath : _searchPaths) {
        const mstd::string rootPath = _defaultResourceRootPath.empty()
                                         ? searchPath
                                         : joinPathImpl(_defaultResourceRootPath, searchPath);
        for (const mstd::string& resolutionOrder : _searchResolutionsOrder) {
            const mstd::string resolvedBasePath =
                resolutionOrder.empty() ? rootPath : joinPathImpl(rootPath, resolutionOrder);
            const mstd::string candidatePath =
                resolvedBasePath.empty() ? filename : joinPathImpl(resolvedBasePath, filename);
            if (isFileExistImpl(candidatePath)) {
                return normalizePathImpl(candidatePath);
            }
        }
    }

    return {};
}

mstd::string FileUtils::parentPath(const mstd::string& path) const {
    if (path.empty()) {
        return {};
    }
    return parentPathImpl(normalizePathImpl(path));
}

bool FileUtils::isAbsolutePath(const mstd::string& path) const {
    if (path.empty()) {
        return false;
    }
    return isAbsolutePathImpl(path);
}

bool FileUtils::isFileExist(const mstd::string& path) const {
    if (path.empty()) {
        return false;
    }
    return !fullPathForFilename(path).empty();
}

bool FileUtils::isDirectoryExist(const mstd::string& path) const {
    if (path.empty()) {
        return false;
    }

    const mstd::string normalizedPath = normalizePathImpl(path);
    if (normalizedPath.empty()) {
        return false;
    }
    return isDirectoryExistImpl(normalizedPath);
}

mstd::uintmax_t FileUtils::getFileSize(const mstd::string& path) const {
    if (path.empty()) {
        return 0;
    }

    const mstd::string fullPath = fullPathForFilename(path);
    if (fullPath.empty()) {
        return 0;
    }
    return getFileSizeImpl(fullPath);
}

mstd::string FileUtils::getWritablePath() const {
    return getWritablePathImpl();
}

bool FileUtils::createDirectory(const mstd::string& dirPath) const {
    if (dirPath.empty()) {
        return false;
    }
    return createDirectoryImpl(normalizePathImpl(dirPath));
}

bool FileUtils::removeDirectory(const mstd::string& dirPath) const {
    if (dirPath.empty()) {
        return false;
    }
    return removeDirectoryImpl(normalizePathImpl(dirPath));
}

bool FileUtils::removeFile(const mstd::string& path) const {
    if (path.empty()) {
        return false;
    }

    const mstd::string fullPath = fullPathForFilename(path);
    if (fullPath.empty()) {
        return false;
    }
    return removeFileImpl(fullPath);
}

bool FileUtils::renameFile(const mstd::string& oldPath, const mstd::string& newPath) const {
    if (oldPath.empty() || newPath.empty()) {
        return false;
    }

    const mstd::string oldFullPath = fullPathForFilename(oldPath);
    if (oldFullPath.empty()) {
        return false;
    }

    const mstd::string newFullPath = normalizePathImpl(newPath);
    if (newFullPath.empty()) {
        return false;
    }

    const mstd::string parentDir = parentPathImpl(newFullPath);
    if (!parentDir.empty() && !isDirectoryExistImpl(parentDir) && !createDirectoryImpl(parentDir)) {
        return false;
    }

    return renameFileImpl(oldFullPath, newFullPath);
}

void FileUtils::setSearchPaths(const mstd::vector<mstd::string>& searchPaths) {
    _searchPaths.clear();
    for (const mstd::string& path : searchPaths) {
        addSearchPath(path);
    }

    if (_searchPaths.empty()) {
        _searchPaths.emplace_back(".");
    }
}

void FileUtils::addSearchPath(const mstd::string& path, bool front) {
    if (path.empty()) {
        return;
    }

    const mstd::string normalizedPath = normalizePathImpl(path);
    if (normalizedPath.empty()) {
        return;
    }

    if (mstd::find(_searchPaths.begin(), _searchPaths.end(), normalizedPath) != _searchPaths.end()) {
        return;
    }

    if (front) {
        _searchPaths.insert(_searchPaths.begin(), normalizedPath);
    } else {
        _searchPaths.push_back(normalizedPath);
    }
}

const mstd::vector<mstd::string>& FileUtils::getSearchPaths() const {
    return _searchPaths;
}

void FileUtils::setSearchResolutionsOrder(
    const mstd::vector<mstd::string>& searchResolutionsOrder) {
    _searchResolutionsOrder.clear();
    for (const mstd::string& order : searchResolutionsOrder) {
        addSearchResolutionsOrder(order);
    }

    if (_searchResolutionsOrder.empty()) {
        _searchResolutionsOrder.emplace_back("");
    }
}

void FileUtils::addSearchResolutionsOrder(const mstd::string& order, bool front) {
    const mstd::string normalizedOrder = normalizePathImpl(order);
    if (order.empty() || normalizedOrder.empty()) {
        if (order.empty() && mstd::find(_searchResolutionsOrder.begin(), _searchResolutionsOrder.end(), "")
                                       == _searchResolutionsOrder.end()) {
            if (front) {
                _searchResolutionsOrder.insert(_searchResolutionsOrder.begin(), "");
            } else {
                _searchResolutionsOrder.emplace_back("");
            }
        }
        return;
    }

    if (mstd::find(_searchResolutionsOrder.begin(), _searchResolutionsOrder.end(), normalizedOrder)
        != _searchResolutionsOrder.end()) {
        return;
    }

    if (front) {
        _searchResolutionsOrder.insert(_searchResolutionsOrder.begin(), normalizedOrder);
    } else {
        _searchResolutionsOrder.push_back(normalizedOrder);
    }
}

const mstd::vector<mstd::string>& FileUtils::getSearchResolutionsOrder() const {
    return _searchResolutionsOrder;
}

void FileUtils::setDefaultResourceRootPath(const mstd::string& path) {
    if (path.empty()) {
        _defaultResourceRootPath.clear();
        return;
    }

    _defaultResourceRootPath = normalizePathImpl(path);
}

const mstd::string& FileUtils::getDefaultResourceRootPath() const {
    return _defaultResourceRootPath;
}

} // namespace zocos
