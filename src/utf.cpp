#include "uxs/utf.h"

#include <algorithm>

namespace uxs {

bool is_utf_code_printable(std::uint32_t code) noexcept {
    static const UXS_CONSTEXPR std::uint32_t v[] = {
        0x12,       0x1d9,      0x377,      0x37d,      0x37f,      0x380,      0x381,      0x382,      0x383,
        0x384,      0x385,      0x386,      0x387,      0x388,      0x389,      0x38a,      0x38b,      0x38c,
        0x1f0000,   0x21007f,   0xad,       0x6f0300,   0x10378,    0x30380,    0x38b,      0x38d,      0x3a2,
        0x60483,    0x530,      0x10557,    0x1058b,    0x2d0590,   0x5bf,      0x105c1,    0x105c4,    0x805c7,
        0x305eb,    0x1005f5,   0xa0610,    0x61c,      0x14064b,   0x670,      0x706d6,    0x506df,    0x106e7,
        0x306ea,    0x1070e,    0x711,      0x1c0730,   0xa07a6,    0xd07b2,    0x807eb,    0x207fb,    0x30816,
        0x8081b,    0x20825,    0x60829,    0x83f,      0x40859,    0x85f,      0x4086b,    0x10088f,   0x3808ca,
        0x93a,      0x93c,      0x70941,    0x94d,      0x60951,    0x10962,    0x981,      0x984,      0x1098d,
        0x10991,    0x9a9,      0x9b1,      0x209b3,    0x209ba,    0x9be,      0x509c1,    0x109c9,    0x9cd,
        0xc09cf,    0x9de,      0x309e2,    0x409fe,    0xa04,      0x30a0b,    0x10a11,    0xa29,      0xa31,
        0xa34,      0xa37,      0x30a3a,    0x170a41,   0xa5d,      0x60a5f,    0x10a70,    0xa75,      0xb0a77,
        0xa84,      0xa8e,      0xa92,      0xaa9,      0xab1,      0xab4,      0x20aba,    0x70ac1,    0xaca,
        0x20acd,    0xe0ad1,    0x30ae2,    0x60af2,    0x70afa,    0xb04,      0x10b0d,    0x10b11,    0xb29,
        0xb31,      0xb34,      0x20b3a,    0x10b3e,    0x50b41,    0x10b49,    0xe0b4d,    0xb5e,      0x30b62,
        0xa0b78,    0xb84,      0x20b8b,    0xb91,      0x20b96,    0xb9b,      0xb9d,      0x20ba0,    0x20ba5,
        0x20bab,    0x40bba,    0xbc0,      0x20bc3,    0xbc9,      0x20bcd,    0x140bd1,   0x50bfb,    0xc04,
        0xc0d,      0xc11,      0xc29,      0x20c3a,    0x20c3e,    0x120c45,   0x10c5b,    0x10c5e,    0x30c62,
        0x60c70,    0xc81,      0xc8d,      0xc91,      0xca9,      0xcb4,      0x20cba,    0xcbf,      0xcc2,
        0x10cc5,    0xcc9,      0x100ccc,   0xcdf,      0x30ce2,    0xcf0,      0xd0cf4,    0xd0d,      0xd11,
        0x10d3b,    0xd3e,      0x40d41,    0xd49,      0xd4d,      0x30d50,    0xd57,      0x30d62,    0x10d80,
        0xd84,      0x20d97,    0xdb2,      0xdbc,      0x10dbe,    0x80dc7,    0x50dd2,    0x60ddf,    0x10df0,
        0xb0df5,    0xe31,      0xa0e34,    0x70e47,    0x240e5c,   0xe83,      0xe85,      0xe8b,      0xea4,
        0xea6,      0xeb1,      0x80eb4,    0x10ebe,    0xec5,      0x80ec7,    0x10eda,    0x1f0ee0,   0x10f18,
        0xf35,      0xf37,      0xf39,      0xf48,      0x110f6d,   0x40f80,    0x10f86,    0x300f8d,   0xfc6,
        0xfcd,      0x240fdb,   0x3102d,    0x51032,    0x11039,    0x1103d,    0x11058,    0x2105e,    0x31071,
        0x1082,     0x11085,    0x108d,     0x109d,     0x10c6,     0x410c8,    0x110ce,    0x1249,     0x1124e,
        0x1257,     0x1259,     0x1125e,    0x1289,     0x1128e,    0x12b1,     0x112b6,    0x12bf,     0x12c1,
        0x112c6,    0x12d7,     0x1311,     0x11316,    0x4135b,    0x2137d,    0x5139a,    0x113f6,    0x113fe,
        0x1680,     0x2169d,    0x616f9,    0x21712,    0x81716,    0x11732,    0x81737,    0xd1752,    0x176d,
        0xe1771,    0x117b4,    0x617b7,    0x17c6,     0xa17c9,    0x217dd,    0x517ea,    0x517fa,    0x4180b,
        0x5181a,    0x61879,    0x11885,    0x18a9,     0x418ab,    0x918f6,    0x3191f,    0x11927,    0x3192c,
        0x1932,     0x61939,    0x21941,    0x1196e,    0xa1975,    0x319ac,    0x519ca,    0x219db,    0x11a17,
        0x21a1b,    0x1a56,     0x81a58,    0x1a62,     0x71a65,    0xc1a73,    0x51a8a,    0x51a9a,    0x551aae,
        0x61b34,    0x1b3c,     0x1b42,     0x21b4d,    0x81b6b,    0x21b7f,    0x31ba2,    0x11ba8,    0x21bab,
        0x1be6,     0x11be8,    0x1bed,     0x21bef,    0x71bf4,    0x71c2c,    0x41c36,    0x21c4a,    0x61c89,
        0x11cbb,    0xa1cc8,    0xc1cd4,    0x61ce2,    0x1ced,     0x1cf4,     0x11cf8,    0x41cfb,    0x3f1dc0,
        0x11f16,    0x11f1e,    0x11f46,    0x11f4e,    0x1f58,     0x1f5a,     0x1f5c,     0x1f5e,     0x11f7e,
        0x1fb5,     0x1fc5,     0x11fd4,    0x1fdc,     0x11ff0,    0x1ff5,     0x101fff,   0x72028,    0x10205f,
        0x12072,    0x208f,     0x2209d,    0x3e20c1,   0x3218c,    0x182427,   0x14244b,   0x12b74,    0x2b96,
        0x22cef,    0x42cf4,    0x2d26,     0x42d28,    0x12d2e,    0x62d68,    0xe2d71,    0x82d97,    0x2da7,
        0x2daf,     0x2db7,     0x2dbf,     0x2dc7,     0x2dcf,     0x2dd7,     0x202ddf,   0x212e5e,   0x2e9a,
        0xb2ef4,    0x192fd6,   0x42ffc,    0x5302a,    0x3040,     0x33097,    0x43100,    0x3130,     0x318f,
        0xb31e4,    0x321f,     0x2a48d,    0x8a4c7,    0x13a62c,   0x3a66f,    0x9a674,    0x1a69e,    0x1a6f0,
        0x7a6f8,    0x4a7cb,    0xa7d2,     0xa7d4,     0x17a7da,   0xa802,     0xa806,     0xa80b,     0x1a825,
        0x3a82c,    0x5a83a,    0x7a878,    0x9a8c4,    0x17a8da,   0xa8ff,     0x7a926,    0xaa947,    0xaa954,
        0x5a97d,    0xa9b3,     0x3a9b6,    0x1a9bc,    0xa9ce,     0x3a9da,    0xa9e5,     0xa9ff,     0x5aa29,
        0x1aa31,    0xaaa35,    0xaa43,     0xaa4c,     0x1aa4e,    0x1aa5a,    0xaa7c,     0xaab0,     0x2aab2,
        0x1aab7,    0x1aabe,    0xaac1,     0x17aac3,   0x1aaec,    0xaaaf6,    0x1ab07,    0x1ab0f,    0x8ab17,
        0xab27,     0xab2f,     0x3ab6c,    0xabe5,     0xabe8,     0x2abed,    0x5abfa,    0xbd7a4,    0x3d7c7,
        0x3d7fc,    0x18ffe000, 0x1fa6e,    0x25fada,   0xbfb07,    0x4fb18,    0xfb1e,     0xfb37,     0xfb3d,
        0xfb3f,     0xfb42,     0xfb45,     0xffbc3,    0x1fd90,    0x6fdc8,    0x1ffdd0,   0xffe00,    0x15fe1a,
        0xfe53,     0xfe67,     0x3fe6c,    0xfe75,     0x3fefd,    0x1ff9e,    0x2ffbf,    0x1ffc8,    0x1ffd0,
        0x1ffd8,    0x2ffdd,    0xffe7,     0xcffef,    0x1fffe,    0xc,        0x27,       0x3b,       0x3e,
        0x1004e,    0x21005e,   0x400fb,    0x30103,    0x20134,    0x18f,      0x2019d,    0x2e01a1,   0x8201fd,
        0x2029d,    0xf02d1,    0x302fc,    0x80324,    0x4034b,    0x90376,    0x39e,      0x303c4,    0x2903d6,
        0x1049e,    0x504aa,    0x304d4,    0x304fc,    0x70528,    0xa0564,    0x57b,      0x58b,      0x593,
        0x596,      0x5a2,      0x5b2,      0x5ba,      0x4205bd,   0x80737,    0x90756,    0x170768,   0x786,
        0x7b1,      0x4407bb,   0x10806,    0x809,      0x836,      0x20839,    0x1083d,    0x856,      0x7089f,
        0x2f08b0,   0x8f3,      0x408f6,    0x2091c,    0x4093a,    0x3f0940,   0x309b8,    0x109d0,    0xe0a01,
        0xa14,      0xa18,      0x90a36,    0x60a49,    0x60a59,    0x1f0aa0,   0x50ae5,    0x80af7,    0x20b36,
        0x10b56,    0x40b73,    0x60b92,    0xb0b9d,    0x4f0bb0,   0x360c49,   0xc0cb3,    0x60cf3,    0xb0d24,
        0x1250d3a,  0xe7f,      0x20eaa,    0x10eae,    0x4d0eb2,   0x70f28,    0xa0f46,    0x150f5a,   0x30f82,
        0x250f8a,   0x130fcc,   0x80ff7,    0x1001,     0xe1038,    0x3104e,    0x1070,     0x11073,    0xb1076,
        0x310b3,    0x110b9,    0x10bd,     0xd10c2,    0x610e9,    0x810fa,    0x41127,    0x8112d,    0x71148,
        0x1173,     0xa1177,    0x811b6,    0x311c9,    0x11cf,     0x11e0,     0xa11f5,    0x1212,     0x2122f,
        0x1234,     0x11236,    0x123e,     0x3e1241,   0x1287,     0x1289,     0x128e,     0x129e,     0x512aa,
        0x12df,     0xc12e3,    0x712fa,    0x1304,     0x1130d,    0x11311,    0x1329,     0x1331,     0x1334,
        0x2133a,    0x133e,     0x1340,     0x11345,    0x11349,    0x1134e,    0xb1351,    0x9b1364,   0x71438,
        0x21442,    0x1446,     0x145c,     0x145e,     0x1d1462,   0x14b0,     0x514b3,    0x14ba,     0x14bd,
        0x114bf,    0x114c2,    0x714c8,    0xa514da,   0x15af,     0x515b2,    0x115bc,    0x115bf,    0x2315dc,
        0x71633,    0x163d,     0x1163f,    0xa1645,    0x5165a,    0x12166d,   0x16ab,     0x16ad,     0x516b0,
        0x16b7,     0x516ba,    0x3516ca,   0x4171b,    0x31722,    0x81727,    0xb81747,   0x8182f,    0x11839,
        0x63183c,   0xb18f3,    0x11907,    0x1190a,    0x1914,     0x1917,     0x1930,     0x1936,     0x31939,
        0x193e,     0x1943,     0x81947,    0x45195a,   0x119a8,    0x719d4,    0x19e0,     0x1a19e5,   0x91a01,
        0x51a33,    0x31a3b,    0x81a47,    0x51a51,    0x21a59,    0xc1a8a,    0x11a98,    0xc1aa3,    0x61af9,
        0xf51b0a,   0x1c09,     0xd1c30,    0x1c3f,     0x91c46,    0x21c6d,    0x181c90,   0x61caa,    0x11cb2,
        0x4a1cb5,   0x1d07,     0x1d0a,     0x141d31,   0x81d47,    0x51d5a,    0x1d66,     0x1d69,     0x31d8f,
        0x1d95,     0x1d97,     0x61d99,    0x1351daa,  0x11ef3,    0x81ef9,    0x1f11,     0x71f36,    0x1f40,
        0x1f42,     0x551f5a,   0xe1fb1,    0xc1ff2,    0x65239a,   0x246f,     0xa2475,    0xa4b2544,  0xc2ff3,
        0x103430,   0xfb83447,  0x21b84647, 0x66a39,    0x6a5f,     0x36a6a,    0x6abf,     0x56aca,    0x66aee,
        0x96af6,    0x66b30,    0x96b46,    0x6b5a,     0x6b62,     0x46b78,    0x2af6b90,  0x646e9b,   0x46f4b,
        0xa6f88,    0x3f6fa0,   0xb6fe4,    0xd6ff2,    0x787f8,    0x298cd6,   0x22e68d09, 0xaff4,     0xaffc,
        0xafff,     0xeb123,    0x1cb133,   0x1b153,    0xdb156,    0x7b168,    0x903b2fc,  0x4bc6b,    0x2bc7d,
        0x6bc89,    0x1bc9a,    0x1bc9d,    0x12afbca0, 0x3bcfc4,   0x9d0f6,    0x1d127,    0xd165,     0x2d167,
        0x14d16e,   0x6d185,    0x3d1aa,    0x14d1eb,   0x2d242,    0x79d246,   0xbd2d4,    0xbd2f4,    0x8d357,
        0x86d379,   0xd455,     0xd49d,     0x1d4a0,    0x1d4a3,    0x1d4a7,    0xd4ad,     0xd4ba,     0xd4bc,
        0xd4c4,     0xd506,     0x1d50b,    0xd515,     0xd51d,     0xd53a,     0xd53f,     0xd545,     0x2d547,
        0xd551,     0x1d6a6,    0x1d7cc,    0x36da00,   0x31da3b,   0xda75,     0xda84,     0x473da8c,  0x5df1f,
        0x104df2b,  0x91e06e,   0x9e12d,    0x1e13e,    0x3e14a,    0x13fe150,  0x11e2ae,   0x3e2ec,    0x4e2fa,
        0x1cfe300,  0x3e4ec,    0x2e5e4fa,  0xe7e7,     0xe7ec,     0xe7ef,     0xe7ff,     0x1e8c5,    0x2fe8d0,
        0x6e944,    0x3e94c,    0x3e95a,    0x310e960,  0x4becb5,   0xc1ed3e,   0xee04,     0xee20,     0xee23,
        0x1ee25,    0xee28,     0xee33,     0xee38,     0xee3a,     0x5ee3c,    0x3ee43,    0xee48,     0xee4a,
        0xee4c,     0xee50,     0xee53,     0x1ee55,    0xee58,     0xee5a,     0xee5c,     0xee5e,     0xee60,
        0xee63,     0x1ee65,    0xee6b,     0xee73,     0xee78,     0xee7d,     0xee7f,     0xee8a,     0x4ee9c,
        0xeea4,     0xeeaa,     0x33eebc,   0x10deef2,  0x3f02c,    0xbf094,    0x1f0af,    0xf0c0,     0xf0d0,
        0x9f0f6,    0x37f1ae,   0xcf203,    0x3f23c,    0x6f249,    0xdf252,    0x99f266,   0x3f6d8,    0x2f6ed,
        0x2f6fd,    0x3f777,    0x5f7da,    0x3f7ec,    0xef7f1,    0x3f80c,    0x7f848,    0x5f85a,    0x7f888,
        0x1f8ae,    0x4df8b2,   0xbfa54,    0x1fa6e,    0x2fa7d,    0x6fa89,    0xfabe,     0x7fac6,    0x3fadc,
        0x6fae9,    0x6faf9,    0xfb93,     0x24fbcb,   0x405fbfa,  0x1fa6e0,   0x5b73a,    0x1b81e,    0xdcea2,
        0xc1eebe1,  0x5e1fa1e,  0x4134b,    0xdc4f23b0, 0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000,
        0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000};

    const std::uint32_t higher = code >> 16;
    if (higher >= 0x11) { return false; }
    code &= 0xffff;
    const auto* first = &v[v[higher]];
    const auto* last = &v[v[higher + 1]];
    const auto* lower = std::lower_bound(first, last, code + 1,
                                         [](std::uint32_t x, std::uint32_t v) { return (x & 0xffff) < v; });
    return lower == first || code > (*(lower - 1) & 0xffff) + (*(lower - 1) >> 16);
}

unsigned get_utf_code_width(std::uint32_t code) noexcept {
    static const UXS_CONSTEXPR std::uint32_t v[] = {
        0x12,      0x4e,     0x77,       0x7d,       0x7f,      0x7f,       0x7f,       0x7f,       0x7f,
        0x7f,      0x7f,     0x7f,       0x7f,       0x7f,      0x7f,       0x7f,       0x7f,       0x7f,
        0x5f1100,  0x1231a,  0x12329,    0x323e9,    0x23f0,    0x23f3,     0x125fd,    0x12614,    0xb2648,
        0x267f,    0x2693,   0x26a1,     0x126aa,    0x126bd,   0x126c4,    0x26ce,     0x26d4,     0x26ea,
        0x126f2,   0x26f5,   0x26fa,     0x26fd,     0x2705,    0x1270a,    0x2728,     0x274c,     0x274e,
        0x22753,   0x2757,   0x22795,    0x27b0,     0x27bf,    0x12b1b,    0x2b50,     0x2b55,     0x192e80,
        0x582e9b,  0xd52f00, 0xb2ff0,    0x283001,   0xe3030,   0x553041,   0x64309b,   0x2a3105,   0x5d3131,
        0x533190,  0x2e31f0, 0x273220,   0x723c3250, 0x36a490,  0x1ca960,   0x2ba3ac00, 0x16df900,  0x69fa70,
        0x9fe10,   0x22fe30, 0x12fe54,   0x3fe68,    0x5fff01,  0x6ffe0,    0x36fe0,    0x17f77000, 0x4d58800,
        0x88d00,   0x3aff0,  0x6aff5,    0x1affd,    0x122b000, 0xb132,     0x2b150,    0xb155,     0x3b164,
        0x18bb170, 0xf004,   0xf0cf,     0xf18e,     0x9f191,   0x2f200,    0x2bf210,   0x8f240,    0x1f250,
        0x5f260,   0xfaf300, 0x24ff400,  0x45f680,   0xf6cc,    0x2f6d0,    0x2f6d5,    0x3f6dc,    0x1f6eb,
        0x8f6f4,   0xbf7e0,  0xf7f0,     0xfff900,   0xcfa70,   0x8fa80,    0x2dfa90,   0x6fabf,    0xdface,
        0x8fae0,   0x8faf0,  0xa6df0000, 0x1039a700, 0xddb740,  0x1681b820, 0x1d30ceb0, 0x21df800,  0x134a0000,
        0x105f1350};
    const std::uint32_t higher = code >> 16;
    if (higher >= 0x11) { return 1; }
    code &= 0xffff;
    const auto* first = &v[v[higher]];
    const auto* last = &v[v[higher + 1]];
    const auto* lower = std::lower_bound(first, last, code + 1,
                                         [](std::uint32_t x, std::uint32_t v) { return (x & 0xffff) < v; });
    return lower == first || code > (*(lower - 1) & 0xffff) + (*(lower - 1) >> 16) ? 1 : 2;
}

}  // namespace uxs