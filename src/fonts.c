#include "global.h"

ALIGNED(4) const u16 gFontSmallLatinGlyphs[] = INCBIN_U16("graphics/fonts/small.latfont");
ALIGNED(4) const u8 gFontSmallLatinGlyphWidths[] = {
    3,  5,  5,  5,  5,  5,  5,  5,  5,  4,  3,  4,  4,  5,  5,  5,
    5,  5,  5,  5,  5,  5,  5,  5,  3,  5,  5,  5,  5,  5,  4,  3,
    4,  4,  5,  5,  5,  6,  5,  5,  5,  5,  5,  5,  8,  7,  8,  3,
    3,  3,  3,  3,  8,  8,  4,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    5,  5,  5,  8,  8,  8,  8,  8,  8,  8,  4,  7,  5,  5,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  5,  3,  3,  3,  3,  3,  3,  4,
    3,  3,  3,  3,  3,  3,  3,  5,  3,  8,  8,  8,  8,  1,  2,  3,
    4,  5,  6,  7,  5,  7,  7,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    8,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  4,  5,  3,  5,  3,
    5,  6,  6,  3,  3,  7,  7,  8,  4,  8,  5,  5,  5,  5,  5,  5,
    5,  5,  5,  4,  5,  5,  4,  6,  5,  5,  5,  5,  5,  5,  4,  5,
    5,  6,  5,  4,  5,  5,  5,  5,  5,  5,  5,  5,  5,  4,  5,  5,
    4,  6,  5,  5,  5,  5,  5,  5,  5,  5,  5,  6,  5,  5,  5,  8,
    3,  5,  5,  5,  5,  5,  5,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  6,  3,  3,  3,  3,  3,  3,
    6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
    6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  3,
};

ALIGNED(4) const u16 gFontHGSSLatinGlyphs[] = INCBIN_U16("graphics/fonts/hgss.latfont");
ALIGNED(4) const u8 gFontHGSSLatinGlyphWidths[] = {
    3,  6,  6,  6,  6,  6,  6,  6,  6,  6,  3,  6,  6,  6,  6,  6,
    8,  6,  6,  6,  6,  6,  6,  6,  3,  6,  6,  6,  6,  6,  6,  3,
    6,  6,  6,  6,  6,  8,  6,  6,  6,  6,  6,  6,  9,  7,  6,  3,
    3,  3,  3,  3,  9,  8,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    6,  6,  4,  8,  8,  8,  7,  8,  8,  4,  6,  6,  4,  4,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  6,  3,  3,  3,  3,  3,  3,  6,
    3,  3,  3,  3,  3,  3,  3,  6,  3,  7,  7,  7,  7,  1,  2,  3,
    4,  5,  6,  7,  6,  6,  6,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    8,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  4,  6,  3,  6,  3,
    6,  6,  6,  3,  3,  6,  6,  6,  3,  7,  6,  6,  6,  6,  6,  6,
    6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
    6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  4,  5,  6,
    4,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  8,
    3,  6,  6,  6,  6,  6,  6,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    0, 10, 10, 10,  8, 10, 10,  8, 10, 10, 10, 10, 10, 10, 10, 10,
    0, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  3,
};

ALIGNED(4) const u16 gFontFRLGLatinGlyphs[] = INCBIN_U16("graphics/fonts/frlg.latfont");
ALIGNED(4) const u8 gFontFRLGLatinGlyphWidths[] = {
    6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  3,  6,  6,  6,  6,  6,
    8,  6,  6,  6,  6,  6,  6,  6,  3,  6,  6,  6,  6,  6,  6,  3,
    6,  6,  6,  6,  6,  8,  6,  6,  6,  6,  6,  6,  9,  8,  8,  3,
    3,  3,  3,  3, 10,  8,  5,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    6,  6,  6,  8,  8,  8,  8,  8,  8,  4,  6,  8,  5,  5,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  6,  3,  3,  3,  3,  3,  3,  6,
    3,  3,  3,  3,  3,  3,  3,  6,  3, 12, 12, 12, 12,  1,  2,  3,
    4,  5,  6,  7,  8,  8,  8,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    8,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  5,  6,  5,
    6,  6,  6,  3,  3,  6,  6,  8,  5,  9,  6,  6,  6,  6,  6,  6,
    6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
    6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  5,  6,  6,  4,  6,  5,
    5,  6,  5,  6,  6,  6,  5,  5,  5,  6,  6,  6,  6,  6,  6,  8,
    5,  6,  6,  6,  6,  6,  6,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    2, 12, 12, 12,  8, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    0, 10, 10, 10, 10, 10, 10, 10,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  3,
};

ALIGNED(4) const u16 gFontELatinGlyphs[] = INCBIN_U16("graphics/fonts/emerald.latfont");
ALIGNED(4) const u8 gFontELatinGlyphWidths[] = {
    3,  6,  6,  6,  6,  6,  6,  6,  6,  6,  3,  6,  6,  6,  6,  6,
    8,  6,  6,  6,  6,  6,  6,  6,  3,  6,  6,  6,  6,  6,  6,  3,
    6,  6,  6,  6,  6,  8,  6,  6,  6,  6,  6,  6,  9,  7,  6,  3,
    3,  3,  3,  3, 10,  8,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    6,  6,  4,  8,  8,  8,  7,  8,  8,  4,  6,  6,  4,  4,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  6,  3,  3,  3,  3,  3,  3,  6,
    3,  3,  3,  3,  3,  3,  3,  6,  3,  7,  7,  7,  7,  1,  2,  3,
    4,  5,  6,  7,  6,  6,  6,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    8,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  4,  6,  3,  6,  3,
    6,  6,  6,  3,  3,  6,  6,  6,  3,  7,  6,  6,  6,  6,  6,  6,
    6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
    6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  4,  5,  6,
    4,  6,  6,  6,  6,  6,  5,  6,  6,  6,  6,  6,  6,  6,  6,  8,
    3,  6,  6,  6,  6,  6,  6,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    0, 10, 10, 10,  8, 10, 10,  8, 10, 10, 10, 10, 10, 10, 10, 10,
    0, 10, 10, 10, 10, 10, 10, 10,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  3,
};

ALIGNED(4) const u16 gFontRSLatinGlyphs[] = INCBIN_U16("graphics/fonts/rs.latfont");
ALIGNED(4) const u8 gFontRSLatinGlyphWidths[] = {
    3,  6,  6,  6,  6,  6,  6,  6,  6,  6,  8,  6,  6,  6,  6,  6,
    6,  6,  6,  6,  6,  6,  6,  6,  8,  6,  6,  6,  6,  6,  4,  8,
    6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  8,  6,  6,  8,
    8,  8,  8,  8,  8,  8,  3,  8,  8,  8,  8,  8,  8,  8,  8,  8,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
    7,  6,  4,  8,  8,  8,  7,  8,  8,  4,  6,  6,  4,  4,  8,  8,
    8,  8,  8,  8,  8,  8,  8,  8,  6,  7,  8,  7,  7,  8,  8,  4,
    7,  8,  8,  8,  8,  8,  7,  6,  7,  7,  7,  7,  7,  7,  8,  7,
    7,  7,  7,  7,  5,  6,  6,  8,  8,  8,  8,  8,  8,  8,  8,  8,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
    8,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  3,  6,  3,  5,  3,
    6,  6,  6,  3,  3,  6,  6,  6,  3,  6,  6,  6,  6,  6,  6,  6,
    6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
    6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  4,  5,  6,
    3,  6,  6,  6,  6,  6,  5,  6,  6,  6,  6,  6,  6,  6,  6,  7,
    3,  6,  6,  6,  6,  6,  6,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    7,  7,  7,  7,  7, 10, 10,  11, 10, 10, 10, 10, 10, 10, 10, 1,
    0, 10, 10, 10, 10, 10, 10, 10, 10,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  3,
};

ALIGNED(4) const u16 gFontNarrowLatinGlyphs[] = INCBIN_U16("graphics/fonts/narrow.latfont");
ALIGNED(4) const u8 gFontNarrowLatinGlyphWidths[] = {
    3,  5,  5,  5,  5,  5,  5,  5,  5,  4,  3,  4,  4,  5,  5,  5,
    8,  5,  5,  5,  5,  6,  5,  5,  3,  5,  5,  5,  5,  5,  4,  3,
    4,  4,  5,  5,  5,  8,  5,  5,  5,  5,  5,  6,  9,  6,  6,  3,
    3,  3,  3,  3,  8,  8,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    5,  5,  4,  8,  8,  8,  7,  8,  8,  4,  4,  6,  4,  4,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  5,  3,  3,  3,  3,  3,  3,  4,
    3,  3,  3,  3,  3,  3,  3,  5,  3,  7,  7,  7,  7,  1,  2,  3,
    4,  5,  6,  7,  5,  6,  6,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    8,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  4,  5,  3,  5,  3,
    5,  5,  5,  3,  3,  5,  5,  6,  3,  6,  6,  5,  5,  5,  5,  5,
    5,  5,  5,  4,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,
    5,  5,  5,  4,  5,  5,  5,  5,  5,  5,  5,  5,  5,  4,  5,  5,
    4,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  8,
    3,  5,  5,  5,  5,  5,  5,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    10, 10, 10, 10,  8,  8, 10,  8, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  3,
};

ALIGNED(4) const u16 gFontSmallJapaneseGlyphs[] = INCBIN_U16("graphics/fonts/small.hwjpnfont");
ALIGNED(4) const u16 gFontNormalJapaneseGlyphs[] = INCBIN_U16("graphics/fonts/normal.hwjpnfont");

ALIGNED(4) const u16 gFontFRLGMaleJapaneseGlyphs[] = INCBIN_U16("graphics/fonts/frlg_male.fwjpnfont");
ALIGNED(4) const u8 gFontFRLGMaleJapaneseGlyphWidths[] = {
    0, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    9,  9,  9,  9,  9,  9,  9, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,  9,
    9,  9,  9,  9,  9,  9,  9, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    9,  8,  7,  8,  8,  8,  8,  8,  8,  8,  8,  5,  9, 10, 10, 10,
    8, 10, 10, 10, 10,  8,  8,  8, 10, 10,  8,  6,  6,  6,  6,  6,
    6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
    6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  5,  6,  6,  2,  4,  6,
    3,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
    5,  6,  6,  6,  6,  6,  6,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
};

ALIGNED(4) const u16 gFontFRLGFemaleJapaneseGlyphs[] = INCBIN_U16("graphics/fonts/frlg_female.fwjpnfont");
ALIGNED(4) const u8 gFontFRLGFemaleJapaneseGlyphWidths[] = {
    0, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,  9,
    9,  9,  9,  9,  9,  9,  9, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,  9,
    9,  9,  9,  9,  9,  9,  8, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    9,  8,  7,  8,  8,  8,  8,  8,  8,  8,  8,  5,  9, 10, 10, 10,
    8, 10, 10, 10, 10,  8,  8,  8, 10, 10,  8,  6,  6,  6,  6,  6,
    6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
    6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  5,  6,  6,  2,  4,  6,
    3,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
    5,  6,  6,  6,  6,  6,  6,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
};

ALIGNED(4) const u16 gFontShortJapaneseGlyphs[] = INCBIN_U16("graphics/fonts/short.fwjpnfont");
ALIGNED(4) const u8 gFontShortJapaneseGlyphWidths[] = {
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,  9,
    9,  9,  9,  9,  9,  9,  9, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,  9,
    9,  9,  9,  9,  9,  9,  9, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10,  8,  7,  8,  8,  8,  8,  8,  8,  8,  8,  5,  9, 10, 10, 10,
    8, 10, 10, 10, 10,  8,  8,  8, 10, 10,  8,  6,  6,  6,  6,  6,
    6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
    6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  5,  6,  6,  2,  4,  6,
    3,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
    5,  6,  6,  6,  6,  6,  6,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
};
