#include "base/ZCStd.h"
#include "base/ZCStringUtils.h"

namespace zocos {

bool StringUtils::UTF8ToUTF32(const mstd::string& utf8, std::u32string& outUtf32) {
    outUtf32.clear();
    outUtf32.reserve(utf8.size());

    bool ok = true;
    const auto pushReplacement = [&]() {
        outUtf32.push_back(U'?');
        ok = false;
    };

    mstd::size_t i = 0;
    while (i < utf8.size()) {
        const unsigned char c0 = static_cast<unsigned char>(utf8[i]);
        if (c0 < 0x80U) {
            outUtf32.push_back(static_cast<char32_t>(c0));
            ++i;
            continue;
        }

        if ((c0 & 0xE0U) == 0xC0U) {
            if (i + 1 >= utf8.size()) {
                pushReplacement();
                break;
            }
            const unsigned char c1 = static_cast<unsigned char>(utf8[i + 1]);
            if ((c1 & 0xC0U) != 0x80U) {
                pushReplacement();
                ++i;
                continue;
            }
            const char32_t cp =
                (static_cast<char32_t>(c0 & 0x1FU) << 6) | static_cast<char32_t>(c1 & 0x3FU);
            outUtf32.push_back(cp >= 0x80U ? cp : U'?');
            if (cp < 0x80U) {
                ok = false;
            }
            i += 2;
            continue;
        }

        if ((c0 & 0xF0U) == 0xE0U) {
            if (i + 2 >= utf8.size()) {
                pushReplacement();
                break;
            }
            const unsigned char c1 = static_cast<unsigned char>(utf8[i + 1]);
            const unsigned char c2 = static_cast<unsigned char>(utf8[i + 2]);
            if ((c1 & 0xC0U) != 0x80U || (c2 & 0xC0U) != 0x80U) {
                pushReplacement();
                ++i;
                continue;
            }
            const char32_t cp = (static_cast<char32_t>(c0 & 0x0FU) << 12) |
                                (static_cast<char32_t>(c1 & 0x3FU) << 6) |
                                static_cast<char32_t>(c2 & 0x3FU);
            outUtf32.push_back(cp >= 0x800U ? cp : U'?');
            if (cp < 0x800U) {
                ok = false;
            }
            i += 3;
            continue;
        }

        if ((c0 & 0xF8U) == 0xF0U) {
            if (i + 3 >= utf8.size()) {
                pushReplacement();
                break;
            }
            const unsigned char c1 = static_cast<unsigned char>(utf8[i + 1]);
            const unsigned char c2 = static_cast<unsigned char>(utf8[i + 2]);
            const unsigned char c3 = static_cast<unsigned char>(utf8[i + 3]);
            if ((c1 & 0xC0U) != 0x80U || (c2 & 0xC0U) != 0x80U || (c3 & 0xC0U) != 0x80U) {
                pushReplacement();
                ++i;
                continue;
            }
            const char32_t cp = (static_cast<char32_t>(c0 & 0x07U) << 18) |
                                (static_cast<char32_t>(c1 & 0x3FU) << 12) |
                                (static_cast<char32_t>(c2 & 0x3FU) << 6) |
                                static_cast<char32_t>(c3 & 0x3FU);
            const bool inRange = (cp >= 0x10000U && cp <= 0x10FFFFU);
            outUtf32.push_back(inRange ? cp : U'?');
            if (!inRange) {
                ok = false;
            }
            i += 4;
            continue;
        }

        pushReplacement();
        ++i;
    }

    return ok;
}

} // namespace zocos
