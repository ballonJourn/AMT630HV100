#include "vg_font.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef VG_DRIVER
#if !defined(VG_ONLY) && !defined(AWTK)

#define	MAX_TEXT_SIZE		32		// 定义显示支持的最大的字符长度

static VGuint  tmpGlyphIndices[MAX_TEXT_SIZE];
static VGfloat tmpAdjustmentsX[MAX_TEXT_SIZE];
static VGfloat tmpAdjustmentsY[MAX_TEXT_SIZE];


// given a character code, return its glyph index
VGint glyphIndexFromCharCode(const Font* font,
                             const VGint charCode) 
{
   return charCode;
}

// compare two glyphs, by their glyph indices
static VGint glyphsCompare(const void* arg0, const void* arg1) 
{
	const Glyph* glyph0 = (const Glyph*)arg0;
	const Glyph* glyph1 = (const Glyph*)arg1;

	return (VGint)glyph0->glyphIndex - (VGint)glyph1->glyphIndex;
}

// given a glyph index, return the associated Glyph structure
const Glyph* glyphFromGlyphIndex(const Font* font, const VGint glyphIndex) 
{
	Glyph glyph;

	glyph.glyphIndex = glyphIndex;
	return (const Glyph*)bsearch(&glyph, font->glyphs, font->glyphsCount, sizeof(Glyph), glyphsCompare);
}

// given a character code, return the associated Glyph structure
const Glyph* glyphFromCharCode(const Font* font,const VGint charCode) 
{
	const VGint glyphIndex = glyphIndexFromCharCode(font, charCode);
	return (glyphIndex >= 0) ? glyphFromGlyphIndex(font, glyphIndex) : NULL;
}

// compare two kerning entries
static VGint kerningsCompare(const void* arg0, const void* arg1) 
{
	const KerningEntry* krn0 = (const KerningEntry*)arg0;
	const KerningEntry* krn1 = (const KerningEntry*)arg1;

	return (VGint)krn0->key - (VGint)krn1->key;
}

// given a couple of glyph indices, return the relative kerning (NULL if kerning is zero)
const KerningEntry* kerningFromGlyphIndices(const Font* font,
                                            const VGint leftGlyphIndex,
                                            const VGint rightGlyphIndex) 
{
	const KerningEntry krn = 
	{
		// the key
		((VGuint)leftGlyphIndex << 16) + (VGuint)rightGlyphIndex,
		 0.0f, 0.0f
	};

	return (const KerningEntry*)bsearch(&krn, font->kerningTable, font->kerningTableSize, sizeof(KerningEntry), kerningsCompare);
}

// given a couple of character codes, return the relative kerning (NULL if kerning is zero)
const KerningEntry* kerningFromCharCodes(const Font* font,
                                         const VGint leftCharCode,
                                         const VGint rightCharCode) 
{
	const VGint leftGlyphIndex = glyphIndexFromCharCode(font, leftCharCode);
	const VGint rightGlyphIndex = glyphIndexFromCharCode(font, rightCharCode);

	// if both glyph indices have been found, try searching for the kerning information
	return ((leftGlyphIndex >= 0) && (rightGlyphIndex >= 0)) ? kerningFromGlyphIndices(font, leftGlyphIndex, rightGlyphIndex) : NULL;
}

static void textLineBuild(const Font* font, const char* str, const size_t strLen)
{
	size_t i;
	VGint leftGlyphIndex = glyphIndexFromCharCode(font, str[0]);

	// first glyph index
	tmpGlyphIndices[0] = leftGlyphIndex;
	for (i = 1; i < strLen; ++i) 
	{
		const VGint rightGlyphIndex = glyphIndexFromCharCode(font, str[i]);
		const KerningEntry* krn = kerningFromGlyphIndices(font, leftGlyphIndex, rightGlyphIndex);
		// append glyph index
		tmpGlyphIndices[i] = rightGlyphIndex;
		// initialize adjustment
		tmpAdjustmentsX[i - 1] = 0.0f;
		tmpAdjustmentsY[i - 1] = 0.0f;
		// add kerning info
		if (krn != NULL) 
		{
			tmpAdjustmentsX[i - 1] += krn->x;
			tmpAdjustmentsY[i - 1] += krn->y;
		}
		leftGlyphIndex = rightGlyphIndex;
	}
	// last adjustment entry
	tmpAdjustmentsX[strLen - 1] = 0.0f;
	tmpAdjustmentsY[strLen - 1] = 0.0f;
}

// 计算字符串尺寸
int vgTextSize(const Font* font, const char* str, VGfloat *cx, VGfloat *cy) 
{
	VGfloat glyphOrigin[2] = { 0.0f, 0.0f };
	size_t strLen;

	if(font == NULL || str == NULL)
		return -1;
	strLen = strlen(str);
	if(strLen == 0)
		return -1;

	// build the sequence of glyph indices and kerning data
	textLineBuild(font, str, strLen);
	// calculate the metrics of the glyph sequence
	vgSetfv(VG_GLYPH_ORIGIN, 2, glyphOrigin);
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_GLYPH_USER_TO_SURFACE);
	vgLoadIdentity();
	vgDrawGlyphs(font->openvgHandle, (VGint)strLen, tmpGlyphIndices, tmpAdjustmentsX, tmpAdjustmentsY, 0, VG_FALSE);
	vgGetfv(VG_GLYPH_ORIGIN, 2, glyphOrigin);
	*cx = glyphOrigin[0];
	*cy = 1.0;	// 高度为1.0

	return 0;
}

VGErrorCode arialFontInit(void);
void arialFontDestroy(void);

int vgFontInit (void)
{
	arialFontInit ();
	return 0;
}

int vgFontExit (void)
{
	arialFontDestroy ();
	return 0;
}

// draw a line of text
int vgTextOut (const Font* font,
                  const char* str,
                  const VGbitfield paintModes) 
{
	size_t strLen;
	if(font == NULL || str == NULL)
		return -1;
	strLen = strlen(str);
	if(strLen == 0)
		return 0;
	if(strLen > MAX_TEXT_SIZE)
		strLen = MAX_TEXT_SIZE;

	// build the sequence of glyph indices and kerning data
	textLineBuild (font, str, strLen);
 	// draw glyphs
	vgDrawGlyphs (font->openvgHandle, (VGint)strLen, tmpGlyphIndices, tmpAdjustmentsX, tmpAdjustmentsY, paintModes, VG_FALSE);

	return 0;
}

#endif
#endif
