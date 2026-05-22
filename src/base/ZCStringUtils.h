#pragma once

#include "base/ZCStd.h"

namespace zocos {

class StringUtils {
public:
    // Cocos2d-x style: decode a UTF-8 string into UTF-32 code points.
    // Returns false if the input contained malformed bytes (they are replaced
    // with U+003F '?' in the output regardless).
    static bool UTF8ToUTF32(const mstd::string& utf8, std::u32string& outUtf32);
};

} // namespace zocos