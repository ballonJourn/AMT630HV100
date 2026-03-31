#include "usb_os_adapter.h"
#include <linux/errno.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

static int utf8_to_utf16le(const char *s, __le16 *cp, unsigned len)
{
	int	count = 0;
	u8	c;
	u16	uchar;

	/*
	 * this insists on correct encodings, though not minimal ones.
	 * BUT it currently rejects legit 4-byte UTF-8 code points,
	 * which need surrogate pairs.  (Unicode 3.1 can use them.)
	 */
	while (len != 0 && (c = (u8) *s++) != 0) {
		if ((c & 0x80)) {
			/*
			 * 2-byte sequence:
			 * 00000yyyyyxxxxxx = 110yyyyy 10xxxxxx
			 */
			if ((c & 0xe0) == 0xc0) {
				uchar = (c & 0x1f) << 6;

				c = (u8) *s++;
				if ((c & 0xc0) != 0x80)
					goto fail;
				c &= 0x3f;
				uchar |= c;

			/*
			 * 3-byte sequence (most CJKV characters):
			 * zzzzyyyyyyxxxxxx = 1110zzzz 10yyyyyy 10xxxxxx
			 */
			} else if ((c & 0xf0) == 0xe0) {
				uchar = (c & 0x0f) << 12;

				c = (u8) *s++;
				if ((c & 0xc0) != 0x80)
					goto fail;
				c &= 0x3f;
				uchar |= c << 6;

				c = (u8) *s++;
				if ((c & 0xc0) != 0x80)
					goto fail;
				c &= 0x3f;
				uchar |= c;

				/* no bogus surrogates */
				if (0xd800 <= uchar && uchar <= 0xdfff)
					goto fail;

			/*
			 * 4-byte sequence (surrogate pairs, currently rare):
			 * 11101110wwwwzzzzyy + 110111yyyyxxxxxx
			 *     = 11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
			 * (uuuuu = wwww + 1)
			 * FIXME accept the surrogate code points (only)
			 */
			} else
				goto fail;
		} else
			uchar = c;
		put_unaligned_le16(uchar, cp++);
		count++;
		len--;
	}
	return count;
fail:
	return -1;
}

int usb_gadget_get_string(struct usb_gadget_strings *table, int id, u8 *buf)
{
	struct usb_string	*s;
	int			len;

	if (!table)
		return -EINVAL;

	/* descriptor 0 has the language id */
	if (id == 0) {
		buf[0] = 4;
		buf[1] = USB_DT_STRING;
		buf[2] = (u8) table->language;
		buf[3] = (u8) (table->language >> 8);
		return 4;
	}
	for (s = table->strings; s && s->s; s++)
		if (s->id == id)
			break;

	/* unrecognized: stall. */
	if (!s || !s->s)
		return -EINVAL;

	/* string descriptors have length, tag, then UTF16-LE text */
	len = min((size_t) 126, strlen(s->s));
	memset(buf + 2, 0, 2 * len);	/* zero all the bytes */
	len = utf8_to_utf16le(s->s, (__le16 *)&buf[2], len);
	if (len < 0)
		return -EINVAL;
	buf[0] = (len + 1) * 2;
	buf[1] = USB_DT_STRING;
	return buf[0];
}
