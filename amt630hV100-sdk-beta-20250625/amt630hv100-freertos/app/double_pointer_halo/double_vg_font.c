#include "vg_font.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef DOUBLE_POINTER_HALO

#define	MAX_TEXT_SIZE		64		// 定义显示支持的最大的字符长度

static VGuint  tmpGlyphIndices[MAX_TEXT_SIZE];
static VGfloat tmpAdjustmentsX[MAX_TEXT_SIZE];
static VGfloat tmpAdjustmentsY[MAX_TEXT_SIZE];

// Copyright (c) 2008-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

#define FONS_UTF8_ACCEPT 0
#define FONS_UTF8_REJECT 12

static unsigned int fons__decutf8(unsigned int* state, unsigned int* codep, unsigned int byte)
{
	static const unsigned char utf8d[] = {
		// The first part of the table maps bytes to character classes that
		// to reduce the size of the transition table and create bitmasks.
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
		10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

		// The second part is a transition table that maps a combination
		// of a state of the automaton and a character class to a state.
		0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
		12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
		12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
		12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
		12,36,12,12,12,12,12,12,12,12,12,12,
    };

	unsigned int type = utf8d[byte];

    *codep = (*state != FONS_UTF8_ACCEPT) ?
		(byte & 0x3fu) | (*codep << 6) :
		(0xff >> type) & (byte);

	*state = utf8d[256 + *state + type];
	return *state;
}

// 将UTF8的字符串转换为UTF32编码的32位整数串
int utf8_to_utf32 (const char* str, const char* end, 
						 unsigned int *unicode, int size)
{
	unsigned int utf8state = 0;
	unsigned int codepoint;
	int count = 0;
	for (; str != end; ++str) 
	{
		if (fons__decutf8(&utf8state, &codepoint, *(const unsigned char*)str))
			continue;
		*unicode ++ = codepoint;
		size --;
		count ++;
		if(size <= 0)
			break;
	}
	
	return count;
}

static VGint kerningsCompare(const void* arg0, const void* arg1) 
{
	const KerningEntry* krn0 = (const KerningEntry*)arg0;
	const KerningEntry* krn1 = (const KerningEntry*)arg1;

	return (VGint)krn0->key - (VGint)krn1->key;
}

const KerningEntry* kerningFromGlyphIndices(const Font* font,
                                            const VGint leftGlyphIndex,
                                            const VGint rightGlyphIndex) 
{
	const KerningEntry krn = 
	{
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
	return ((leftCharCode >= 0) && (rightCharCode >= 0)) ? kerningFromGlyphIndices(font, leftCharCode, rightCharCode) : NULL;
}

static void TextInfoBuild(const Font* font, const unsigned int* str, const size_t strLen)
{
	size_t i;
	VGint leftGlyphIndex = *str;

	// first glyph index
	for (i = 1; i < strLen; ++i) 
	{
		const VGint rightGlyphIndex = str[i];
		const KerningEntry* krn = kerningFromGlyphIndices(font, leftGlyphIndex, rightGlyphIndex);
		// append glyph index
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
// str 	UTF8编码的字符串, 字符串每个字符的字形数据需要添加到字库文件(如arial.c), 否则计算失败
// cx		保存字符串的逻辑宽度
// cy		保存字符串的逻辑高度(始终为1.0)
int vgTextSize(const Font* font, const char* str, VGfloat *cx, VGfloat *cy) 
{
	VGfloat glyphOrigin[2] = { 0.0f, 0.0f };
	size_t strLen;

	if(font == NULL || str == NULL)
		return -1;
	strLen = strlen(str);
	if(strLen == 0)
		return -1;
	
	strLen = utf8_to_utf32 (str, str + strLen, tmpGlyphIndices, MAX_TEXT_SIZE);

	// build the sequence of glyph indices and kerning data
	TextInfoBuild(font, tmpGlyphIndices, strLen);
	// calculate the metrics of the glyph sequence
	vgSetfv(VG_GLYPH_ORIGIN, 2, glyphOrigin);	// 设置开始的画笔位置, 此处为(0,0), 用于字符串宽度计算 
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_GLYPH_USER_TO_SURFACE);
	vgLoadIdentity();
	vgDrawGlyphs(font->openvgHandle, (VGint)strLen, tmpGlyphIndices, tmpAdjustmentsX, tmpAdjustmentsY, 0, VG_FALSE);
	vgGetfv(VG_GLYPH_ORIGIN, 2, glyphOrigin);	// 获取结束的画笔位置
	*cx = glyphOrigin[0];
	*cy = 1.0;	// 高度为1.0

	return 0;
}

VGErrorCode arialFontInit(void);
void arialFontDestroy(void);
VGErrorCode  courbdFontInit(void);
void  courbdFontDestroy(void);

int vgFontInit (void)
{
	arialFontInit();
	//courbdFontInit ();
	return 0;
}

int vgFontExit (void)
{
	arialFontDestroy();
	//courbdFontDestroy ();
	return 0;
}

// draw a line of text
// str 	UTF8编码的字符串, 字符串每个字符的字形数据需要添加到字库文件(如arial.c), 否则绘制失败
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

	strLen = utf8_to_utf32 (str, str + strLen, tmpGlyphIndices, MAX_TEXT_SIZE);
	// build the sequence of glyph indices and kerning data
	TextInfoBuild (font, tmpGlyphIndices, strLen);
 	// draw glyphs
	vgDrawGlyphs (font->openvgHandle, (VGint)strLen, tmpGlyphIndices, tmpAdjustmentsX, tmpAdjustmentsY, paintModes, VG_FALSE);

	return 0;
}

#endif
