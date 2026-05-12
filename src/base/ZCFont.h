#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace zocos {

class Font {
public:
    static const std::vector<std::string>& getDefaultFontCandidates();
    static std::string resolveFontPath(const std::string& preferredPath);

    bool loadFromFile(const std::string& path);

    bool isValid() const;
    const std::string& getPath() const { return _path; }
    const unsigned char* getData() const;
    std::size_t getDataSize() const { return _data.size(); }
    int getFontOffset() const { return _fontOffset; }

private:
    std::string _path;
    std::vector<unsigned char> _data;
    int _fontOffset = -1;
};

} // namespace zocos
