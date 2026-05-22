#pragma once

#include <cstddef>
#include "base/ZCStd.h"

namespace zocos {

class Font {
public:
    static const mstd::vector<mstd::string>& getDefaultFontCandidates();
    static mstd::string resolveFontPath(const mstd::string& preferredPath);

    bool loadFromFile(const mstd::string& path);

    bool isValid() const;
    const mstd::string& getPath() const { return _path; }
    const unsigned char* getData() const;
    mstd::size_t getDataSize() const { return _data.size(); }
    int getFontOffset() const { return _fontOffset; }

private:
    mstd::string _path;
    mstd::vector<unsigned char> _data;
    int _fontOffset = -1;
};

} // namespace zocos
