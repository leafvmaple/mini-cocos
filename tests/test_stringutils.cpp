#include "ZCTestFramework.h"

#include "base/ZCStd.h"
#include "base/ZCStringUtils.h"

using namespace zocos;

ZC_TEST(utf8_ascii_roundtrip) {
    mstd::u32string out;
    const bool ok = StringUtils::UTF8ToUTF32("Hello", out);
    ZC_CHECK(ok);
    ZC_CHECK_EQ(out.size(), static_cast<mstd::size_t>(5));
    ZC_CHECK_EQ(static_cast<mstd::uint32_t>(out[0]), static_cast<mstd::uint32_t>('H'));
    ZC_CHECK_EQ(static_cast<mstd::uint32_t>(out[4]), static_cast<mstd::uint32_t>('o'));
}

ZC_TEST(utf8_multibyte_codepoints) {
    // "A" + U+6C49 (汉, 3-byte) + U+1F600 (😀, 4-byte), via explicit bytes so the
    // test does not depend on the source file's encoding.
    mstd::string s = "A\xE6\xB1\x89\xF0\x9F\x98\x80";
    mstd::u32string out;
    const bool ok = StringUtils::UTF8ToUTF32(s, out);
    ZC_CHECK(ok);
    ZC_CHECK_EQ(out.size(), static_cast<mstd::size_t>(3));
    ZC_CHECK_EQ(static_cast<mstd::uint32_t>(out[0]), 0x41u);
    ZC_CHECK_EQ(static_cast<mstd::uint32_t>(out[1]), 0x6C49u);
    ZC_CHECK_EQ(static_cast<mstd::uint32_t>(out[2]), 0x1F600u);
}

ZC_TEST(utf8_invalid_byte_reports_failure) {
    // 0xFF is never a valid UTF-8 lead byte.
    mstd::string s = "ok\xFF";
    mstd::u32string out;
    const bool ok = StringUtils::UTF8ToUTF32(s, out);
    ZC_CHECK(!ok);
    // The two valid leading characters still decode.
    ZC_CHECK(out.size() >= static_cast<mstd::size_t>(2));
    ZC_CHECK_EQ(static_cast<mstd::uint32_t>(out[0]), 0x6Fu);
}

ZC_TEST(utf8_empty_string) {
    mstd::u32string out;
    const bool ok = StringUtils::UTF8ToUTF32("", out);
    ZC_CHECK(ok);
    ZC_CHECK_EQ(out.size(), static_cast<mstd::size_t>(0));
}
