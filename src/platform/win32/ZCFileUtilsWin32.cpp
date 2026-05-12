#include "platform/win32/ZCFileUtilsWin32.h"

#include <Windows.h>

#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>
#include <system_error>

namespace zocos {

namespace {

std::wstring utf8ToWide(const std::string& input) {
    if (input.empty()) {
        return {};
    }

    const int requiredCount = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, nullptr, 0);
    if (requiredCount <= 1) {
        return {};
    }

    std::wstring wide(static_cast<std::size_t>(requiredCount), L'\0');
    const int convertedCount =
        MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, wide.data(), requiredCount);
    if (convertedCount != requiredCount) {
        return {};
    }

    wide.pop_back();
    return wide;
}

} // namespace

bool FileUtilsWin32::readBinaryFileImpl(const std::string& path,
                                        std::vector<unsigned char>& outData) const {
    const std::wstring widePath = utf8ToWide(path);
    if (widePath.empty()) {
        return false;
    }

    HANDLE file = CreateFileW(widePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file, &size) || size.QuadPart < 0) {
        CloseHandle(file);
        return false;
    }

    if (size.QuadPart == 0) {
        CloseHandle(file);
        outData.clear();
        return true;
    }

    const auto maxSize = static_cast<unsigned long long>(std::numeric_limits<std::size_t>::max());
    if (static_cast<unsigned long long>(size.QuadPart) > maxSize) {
        CloseHandle(file);
        return false;
    }

    outData.resize(static_cast<std::size_t>(size.QuadPart));
    std::size_t totalRead = 0;

    while (totalRead < outData.size()) {
        const std::size_t remaining = outData.size() - totalRead;
        const DWORD toRead = remaining > static_cast<std::size_t>(std::numeric_limits<DWORD>::max())
                                 ? std::numeric_limits<DWORD>::max()
                                 : static_cast<DWORD>(remaining);

        DWORD readNow = 0;
        if (!ReadFile(file, outData.data() + totalRead, toRead, &readNow, nullptr)) {
            outData.clear();
            CloseHandle(file);
            return false;
        }

        if (readNow == 0) {
            break;
        }

        totalRead += static_cast<std::size_t>(readNow);
    }

    CloseHandle(file);

    if (totalRead != outData.size()) {
        outData.resize(totalRead);
    }

    return true;
}

bool FileUtilsWin32::writeBinaryFileImpl(const std::string& path, const unsigned char* data,
                                         std::size_t size) const {
    if (size > 0 && !data) {
        return false;
    }

    const std::wstring widePath = utf8ToWide(path);
    if (widePath.empty()) {
        return false;
    }

    HANDLE file = CreateFileW(widePath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    std::size_t totalWritten = 0;
    while (totalWritten < size) {
        const std::size_t remaining = size - totalWritten;
        const DWORD toWrite = remaining > static_cast<std::size_t>(std::numeric_limits<DWORD>::max())
                                  ? std::numeric_limits<DWORD>::max()
                                  : static_cast<DWORD>(remaining);

        DWORD writtenNow = 0;
        if (!WriteFile(file, data + totalWritten, toWrite, &writtenNow, nullptr)) {
            CloseHandle(file);
            return false;
        }

        if (writtenNow == 0) {
            break;
        }

        totalWritten += static_cast<std::size_t>(writtenNow);
    }

    const bool flushed = FlushFileBuffers(file) != 0;
    CloseHandle(file);
    return flushed && totalWritten == size;
}

std::string FileUtilsWin32::parentPathImpl(const std::string& path) const {
    const std::filesystem::path fsPath = std::filesystem::u8path(path).lexically_normal();
    return fsPath.parent_path().generic_string();
}

bool FileUtilsWin32::isAbsolutePathImpl(const std::string& path) const {
    const std::filesystem::path fsPath = std::filesystem::u8path(path);
    return fsPath.is_absolute();
}

bool FileUtilsWin32::isFileExistImpl(const std::string& path) const {
    std::error_code ec;
    const std::filesystem::path fsPath = std::filesystem::u8path(path);
    if (!std::filesystem::exists(fsPath, ec) || ec) {
        return false;
    }
    return std::filesystem::is_regular_file(fsPath, ec) && !ec;
}

bool FileUtilsWin32::isDirectoryExistImpl(const std::string& path) const {
    std::error_code ec;
    const std::filesystem::path fsPath = std::filesystem::u8path(path);
    return std::filesystem::is_directory(fsPath, ec) && !ec;
}

std::uintmax_t FileUtilsWin32::getFileSizeImpl(const std::string& path) const {
    std::error_code ec;
    const std::filesystem::path fsPath = std::filesystem::u8path(path);
    const std::uintmax_t size = std::filesystem::file_size(fsPath, ec);
    if (ec) {
        return 0;
    }
    return size;
}

std::string FileUtilsWin32::getWritablePathImpl() const {
    std::wstring tempPath(static_cast<std::size_t>(MAX_PATH), L'\0');
    DWORD count = GetTempPathW(static_cast<DWORD>(tempPath.size()), tempPath.data());
    if (count == 0) {
        return {};
    }

    if (count >= tempPath.size()) {
        tempPath.resize(static_cast<std::size_t>(count) + 1, L'\0');
        count = GetTempPathW(static_cast<DWORD>(tempPath.size()), tempPath.data());
        if (count == 0 || count >= tempPath.size()) {
            return {};
        }
    }

    tempPath.resize(count);

    std::error_code ec;
    std::filesystem::path writablePath = std::filesystem::path(tempPath) / "zocos";
    std::filesystem::create_directories(writablePath, ec);
    if (ec) {
        return {};
    }

    std::string utf8Path = writablePath.lexically_normal().generic_string();
    if (!utf8Path.empty() && utf8Path.back() != '/') {
        utf8Path.push_back('/');
    }
    return utf8Path;
}

bool FileUtilsWin32::createDirectoryImpl(const std::string& dirPath) const {
    if (dirPath.empty()) {
        return false;
    }

    std::error_code ec;
    const std::filesystem::path fsPath = std::filesystem::u8path(dirPath);
    if (std::filesystem::exists(fsPath, ec)) {
        return std::filesystem::is_directory(fsPath, ec) && !ec;
    }

    ec.clear();
    return std::filesystem::create_directories(fsPath, ec) && !ec;
}

bool FileUtilsWin32::removeDirectoryImpl(const std::string& dirPath) const {
    if (dirPath.empty()) {
        return false;
    }

    std::error_code ec;
    const std::filesystem::path fsPath = std::filesystem::u8path(dirPath);
    if (!std::filesystem::exists(fsPath, ec)) {
        return !ec;
    }

    if (!std::filesystem::is_directory(fsPath, ec) || ec) {
        return false;
    }

    const auto removedCount = std::filesystem::remove_all(fsPath, ec);
    return !ec && removedCount != static_cast<std::uintmax_t>(-1);
}

bool FileUtilsWin32::removeFileImpl(const std::string& path) const {
    if (path.empty()) {
        return false;
    }

    std::error_code ec;
    const std::filesystem::path fsPath = std::filesystem::u8path(path);
    if (!std::filesystem::exists(fsPath, ec)) {
        return !ec;
    }

    if (std::filesystem::is_directory(fsPath, ec) || ec) {
        return false;
    }

    const bool removed = std::filesystem::remove(fsPath, ec);
    return !ec && removed;
}

bool FileUtilsWin32::renameFileImpl(const std::string& oldPath, const std::string& newPath) const {
    const std::wstring oldWidePath = utf8ToWide(oldPath);
    const std::wstring newWidePath = utf8ToWide(newPath);
    if (oldWidePath.empty() || newWidePath.empty()) {
        return false;
    }

    return MoveFileExW(oldWidePath.c_str(), newWidePath.c_str(),
                       MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING)
           != 0;
}

std::string FileUtilsWin32::normalizePathImpl(const std::string& path) const {
    if (path.empty()) {
        return {};
    }

    const std::filesystem::path fsPath = std::filesystem::u8path(path).lexically_normal();
    return fsPath.generic_string();
}

std::string FileUtilsWin32::joinPathImpl(const std::string& directory,
                                         const std::string& filename) const {
    if (directory.empty()) {
        return normalizePathImpl(filename);
    }
    if (filename.empty()) {
        return normalizePathImpl(directory);
    }

    const std::filesystem::path joinedPath =
        (std::filesystem::u8path(directory) / std::filesystem::u8path(filename)).lexically_normal();
    return joinedPath.generic_string();
}

} // namespace zocos
