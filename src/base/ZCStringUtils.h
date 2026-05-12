#pragma once

#include <string>
#include <vector>

namespace zocos {

class StringUtils {
public:
    static std::vector<int> decodeUtf8(const std::string& text);
};

} // namespace zocos