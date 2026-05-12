#include "base/ZCStringUtils.h"

namespace zocos {

std::vector<int> StringUtils::decodeUtf8(const std::string& text) {
    std::vector<int> codepoints;
    codepoints.reserve(text.size());

    const auto pushReplacement = [&codepoints]() { codepoints.push_back('?'); };

    std::size_t i = 0;
    while (i < text.size()) {
        const unsigned char c0 = static_cast<unsigned char>(text[i]);
        if (c0 < 0x80U) {
            codepoints.push_back(static_cast<int>(c0));
            ++i;
            continue;
        }

        if ((c0 & 0xE0U) == 0xC0U) {
            if (i + 1 >= text.size()) {
                pushReplacement();
                break;
            }
            const unsigned char c1 = static_cast<unsigned char>(text[i + 1]);
            if ((c1 & 0xC0U) != 0x80U) {
                pushReplacement();
                ++i;
                continue;
            }
            const int cp = ((static_cast<int>(c0 & 0x1FU) << 6) | static_cast<int>(c1 & 0x3FU));
            codepoints.push_back(cp >= 0x80 ? cp : '?');
            i += 2;
            continue;
        }

        if ((c0 & 0xF0U) == 0xE0U) {
            if (i + 2 >= text.size()) {
                pushReplacement();
                break;
            }
            const unsigned char c1 = static_cast<unsigned char>(text[i + 1]);
            const unsigned char c2 = static_cast<unsigned char>(text[i + 2]);
            if ((c1 & 0xC0U) != 0x80U || (c2 & 0xC0U) != 0x80U) {
                pushReplacement();
                ++i;
                continue;
            }
            const int cp = (static_cast<int>(c0 & 0x0FU) << 12) |
                           (static_cast<int>(c1 & 0x3FU) << 6) | static_cast<int>(c2 & 0x3FU);
            codepoints.push_back(cp >= 0x800 ? cp : '?');
            i += 3;
            continue;
        }

        if ((c0 & 0xF8U) == 0xF0U) {
            if (i + 3 >= text.size()) {
                pushReplacement();
                break;
            }
            const unsigned char c1 = static_cast<unsigned char>(text[i + 1]);
            const unsigned char c2 = static_cast<unsigned char>(text[i + 2]);
            const unsigned char c3 = static_cast<unsigned char>(text[i + 3]);
            if ((c1 & 0xC0U) != 0x80U || (c2 & 0xC0U) != 0x80U || (c3 & 0xC0U) != 0x80U) {
                pushReplacement();
                ++i;
                continue;
            }
            const int cp = (static_cast<int>(c0 & 0x07U) << 18) |
                           (static_cast<int>(c1 & 0x3FU) << 12) |
                           (static_cast<int>(c2 & 0x3FU) << 6) | static_cast<int>(c3 & 0x3FU);
            codepoints.push_back((cp >= 0x10000 && cp <= 0x10FFFF) ? cp : '?');
            i += 4;
            continue;
        }

        pushReplacement();
        ++i;
    }

    return codepoints;
}

} // namespace zocos