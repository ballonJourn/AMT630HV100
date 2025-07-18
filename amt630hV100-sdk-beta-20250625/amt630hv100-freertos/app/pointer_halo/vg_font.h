#ifndef VG_FONT_H
#define VG_FONT_H

#include <VG/openvg.h>


typedef struct {
    // glyph index within the OpenVG font object
    VGuint glyphIndex;
    // the advance width for this glyph
    VGfloat escapement[2];
    // OpenVG path commands defining the glyph geometry
    VGint commandsCount;
    const VGubyte* commands;
    // OpenVG path coordinates defining the glyph geometry
    VGint coordinatesCount;
    const VGfloat* coordinates;
    // fill rule of glyph geometry
    VGFillRule fillRule;
} Glyph;

typedef struct {
    // the key representing two glyph indices ((leftGlyphIndex << 16) + rightGlyphIndex)
    VGuint key;
    // the kerning amount relative to the chars couple
    VGfloat x;
    VGfloat y;
} KerningEntry;

typedef struct {
    // OpenVG font object
    VGFont openvgHandle;
    // glyphs data
    const Glyph* glyphs;
    // number of glyphs
    const VGuint glyphsCount;
    // kerning table
    const KerningEntry* kerningTable;
    // number of kerning entries
    const VGuint kerningTableSize;
} Font;

#ifdef __cplusplus
extern "C" {
#endif

// given a character code, return its glyph index
VGint glyphIndexFromCharCode(const Font* font,
                             const VGint charCode);

// given a glyph index, return the associated Glyph structure
const Glyph* glyphFromGlyphIndex(const Font* font,
                                 const VGint glyphIndex);

// given a character code, return the associated Glyph structure
const Glyph* glyphFromCharCode(const Font* font,
                               const VGint charCode);

// given a couple of glyph indices, return the relative kerning (NULL if kerning is zero)
const KerningEntry* kerningFromGlyphIndices(const Font* font,
                                            const VGint leftGlyphIndex,
                                            const VGint rightGlyphIndex);

// given a couple of character codes, return the relative kerning (NULL if kerning is zero)
const KerningEntry* kerningFromCharCodes(const Font* font,
                                         const VGint leftCharCode,
                                         const VGint rightCharCode);

// ¼ÆËã×Ö·û´®³ß´ç
int vgTextSize(const Font* font, const char* str, VGfloat *cx, VGfloat *cy);


int vgTextOut (const Font* font,
                  const char* str,
                  const VGbitfield paintModes);

int vgFontInit (void);

int vgFontExit (void);


#ifdef __cplusplus
}
#endif

#endif /* VG_FONT_H */
