/****************************************************************************************
 * 本源代码用于HTTP PROXY的NTLM算法中使用的base64算法。                                 *
 *                                                                                      *
 * 按照RFC2045的定义，Base64被定义为：Base64内容传送编码被设计用来把任意序列的8位字节描 *
 * 述为一种不易被人直接识别的形式。（The Base64 Content-Transfer-Encoding is designed   *
 * to represent arbitrary sequences of octets in a form that need not be humanly        *
 * readable.）                                                                          *
 ****************************************************************************************/

#include "common/arithmetic.h"

/*********************************  Base64 算法 begin *****************************/
/*
 ** Translation Table as described in RFC1113
 */
static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static unsigned char db64[256] = {
	255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255, 
	255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255, 
	255,  255,  255,  62,  255,  255,  255,  63,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  255,  255, 
	255,  0,  255,  255,  255,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,  11,  12,  13,  14, 
	15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  255,  255,  255,  255,  255,  255,  26,  27,  28, 
	29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48, 
	49,  50,  51,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255, 
	255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255, 
	255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255, 
	255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255, 
	255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255, 
	255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255, 
	255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255,  255};

/*
** encodeblock
**
** encode 3 8-bit binary bytes as 4 '6-bit' characters
*/
static void encodeblock( unsigned char in[3], unsigned char out[4], int len )
{
    out[0] = cb64[ in[0] >> 2 ];
    out[1] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
    out[2] = (unsigned char) (len > 1 ? cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
    out[3] = (unsigned char) (len > 2 ? cb64[ in[2] & 0x3f ] : '=');
}

void encode_base64(OUT char *dst, IN const char *src, IN int sz )
{
    unsigned char in[3], *out = (unsigned char *) dst;
    int i, len;

    while (sz > 0)
    {
        len = 0;
        for (i = 0; i < 3; i++, sz--)
        {
            if (sz > 0)
            {
                len++;
                in[i] = src[i];
            } else
                in[i] = 0;
        }
        src += 3;
        if (len)
        {
            encodeblock(in, out, len);
            out += 4;
        }
    }
    *out = '\0';
}

void decode_base64(IN char *src, IN int src_len, OUT char *dst,OUT int * dst_len)
{
	int j = 0, i = 0;
	for ( i = 0; i < src_len; i += 4) {
		dst[j++] = db64[src[i]] << 2 | db64[src[i + 1]] >> 4;
		dst[j++] = db64[src[i + 1]] << 4 | db64[src[i + 2]] >> 2;
		dst[j++] = db64[src[i + 2]] << 6 | db64[src[i + 3]];
	}
	dst[j] = '\0';
	* dst_len = j-1;
}

// end of part1: Base64 算法 end 
