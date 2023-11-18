/*
 * 将iso-2022-jp字符集的编码转换成shift-JIS编码
 * 输入：buf中保存的是双字节的iso-2022-jp编码
 * 输出：在buf中保存转换后的编码(shift-JIS编码)。
 */

#include <stdio.h>
#include <stdlib.h>
#include "jis2sjis.h"
//#include "../mail/debug.h"
//xiayu 2004-12-13
#define DEBUG_SMTP(...) 
#define SMTP_DEBUG(...) 
#define SMTP_DBG	0 

static char *memstr(const char *s1, int len1,
					const char *s2, int len2)
{
	char *p1, *p2;
	int i;
	int len = len1;

	//printf("enter memstr()!\n");
	
	if (len2 <=0) {

		/* an empty s2 */
		return ((char *)s1);
	}

	if (len <= len2) {

		/* error, s1 is less than s2 */
		return (NULL);

	}

	while (1) {

		for (; (len >= len2) && ((*s1) != (*s2)); s1++, len--);
		if (len < len2) {
			return (NULL);
		}

		/* found first character of s2, see if the rest matches */
		p1 = (char *)s1;
		p2 = (char *)s2;
		i = len2;

		for (++p1, ++p2, --i; ((*p1) == (*p2)) && (i >0); ++p1, ++p2, --i);
		if (i <= 0) {

			/* second string ended, a match */
			break;
		}

		/* didn't find a match here, try starting at next character in s1 */
		s1++;
		len--;
	}

	/* the point of matched string */
	//printf(" now will exit memstr()!\n");
	return ((char *)s1);
}




/********************************************************
function:		*search_shift_tag(char *buf, int *buf_len)
description:	search & remove SHIFT_IN/OUT tag,
				move the rest data to front,
				return the data that need to be convented
return value:	NULL	- no jis double bytes exist
				jis		- the jis double bytes to convent
*********************************************************/

int search_shift_tag(char *buf, int *buf_len, 
					char **match, int *match_len)
{
	int rest_len = 0;
	char *p			= NULL;
	char *p_begin	= NULL;
	char *p_end		= NULL;
	
	p = buf;

	//printf("in search_shift_tag: enter!\n");
	//printf("in search_shift_tag: buf_len = %d\n", *buf_len);
	
	/* search SHIFT_IN tag */
	p_begin = memstr(p, *buf_len, SHIFT_IN, SHIFT_LEN);
	//printf("p_begin = %p\n", p_begin);
	if (NULL == p_begin) {
		DEBUG_SMTP(SMTP_DBG, 
			"in search_shift_tag: not find SHIFT_IN tag!\n");
		return FALSE;
	}
//	printf("line %d\n", __LINE__);
//	DEBUG_SMTP(SMTP_DBG, 
//		"in search_shift_tag: find SHIFT_IN tag!\n");
//	printf("line %d\n", __LINE__);
//	printf("in search_shift_tag: find SHIFT_IN tag!\n");

	/* remove SHIFT_IN tag, move the rest data to front */
	DEBUG_SMTP(SMTP_DBG, 
		"in search_shift_tag: will remove SHIFT_IN tag!\n");
//	printf("in search_shift_tag: will remove SHIFT_IN tag!\n");

//	printf("line %d\n", __LINE__);
	rest_len = *buf_len - (p_begin + SHIFT_LEN - buf);
//	printf("line %d\n", __LINE__);
	DEBUG_SMTP(SMTP_DBG, "rest_len = %d\n", rest_len);
	DEBUG_SMTP(SMTP_DBG, "buf_len = %d\n", *buf_len);
//	printf("rest_len = %d\n", rest_len);
//	printf("buf_len = %d\n", *buf_len);

	memcpy(p_begin, p_begin + SHIFT_LEN, rest_len);
	*buf_len = p_begin + rest_len - buf;
	DEBUG_SMTP(SMTP_DBG, "in search_shift_tag: removed SHIFT_IN tag!\n");
	DEBUG_SMTP(SMTP_DBG, "new buf_len = %d\n", *buf_len);
//	printf("in search_shift_tag: removed SHIFT_IN tag!\n");
//	printf("new buf_len = %d\n", *buf_len);

	/* search SHIFT_OUT tag */
	p_end = memstr(p_begin, *buf_len, SHIFT_OUT, SHIFT_LEN);
	if (NULL == p_end) {
		DEBUG_SMTP(SMTP_DBG,
			"in search_shift_tag: not find SHIFT_OUT tag!\n");
//		printf("in search_shift_tag: not find SHIFT_OUT tag!\n");
		return FALSE;
	}

	/* remove SHIFT_OUT tag, move the rest data to front */
	DEBUG_SMTP(SMTP_DBG,
		"in search_shift_tag: will remove SHIFT_OUT tag!\n");
//	printf("in search_shift_tag: will remove SHIFT_OUT tag!\n");

	rest_len = *buf_len - (p_end + SHIFT_LEN - buf);
	DEBUG_SMTP(SMTP_DBG, "rest_len = %d\n", rest_len);
	DEBUG_SMTP(SMTP_DBG, "buf_len = %d\n", *buf_len);
//	printf("rest_len = %d\n", rest_len);
//	printf("buf_len = %d\n", *buf_len);

	if (0 < rest_len) {
		memcpy(p_end, p_end + SHIFT_LEN, rest_len);
	}
	*buf_len = p_end + rest_len - buf;
	DEBUG_SMTP(SMTP_DBG, "in search_shift_tag: removed SHIFT_OUT tag!\n");
	DEBUG_SMTP(SMTP_DBG, "new buf_len = %d\n", *buf_len);
//	printf("in search_shift_tag: removed SHIFT_OUT tag!\n");
//	printf("new buf_len = %d\n", *buf_len);

	/* record the data that need to be convented */
	*match_len = p_end - p_begin;
	*match = p_begin;
	DEBUG_SMTP(SMTP_DBG, "return: *match_len = %d\n", *match_len);
//	printf("return: *match_len = %d\n", *match_len);

	return TRUE;
}




/********************************************************



********************************************************/

int decode_iso2022jp_sjis(char* buf, int buf_len)
{
	int i = 0;
	unsigned char first_ch =0, second_ch = 0;
	unsigned char first_ch_new = 0, second_ch_new = 0;
	unsigned int page_pos = 0, pos = 0;

	DEBUG_SMTP(SMTP_DBG, "enter decode_iso2022jp_sjis()!\n");
	
	if (SMTP_DBG) {

		DEBUG_SMTP(SMTP_DBG, "input buffer: %s\n", buf);
		for (i = 0; i < buf_len; i++) {
			DEBUG_SMTP(SMTP_DBG, "buf[%d]:%x\n", i, buf[i]);
		}
	}

	
	/* 输入必须是偶数字节 */
	if ((buf_len & 0x01) != 0) {
		return FALSE;
	}

	/* change every word */
	for (i = 0; i < buf_len - 1 ; i+=2) {

		/* get word */
		first_ch = buf[i];
		second_ch = buf[i+1];
		if (!IS_ISO2022JP_WORD(first_ch, second_ch)) {
			return FALSE;
		}

		/* conversion */

		/* get first byte */
		page_pos = (first_ch - ISO2022JP_BYTE_LOWER_LIMIT) /2;

		if (page_pos < SJIS_PAGE_NUMBER_1) {
			first_ch_new = SJIS_PAGE_LOWER_LIMIT_1 + page_pos;
		}else{
			first_ch_new = SJIS_PAGE_LOWER_LIMIT_2 + page_pos 
							- SJIS_PAGE_NUMBER_1;
		}

		/* get second byte */
		pos = ((first_ch - ISO2022JP_BYTE_LOWER_LIMIT) % 2 )
			* ISO2022JP_PAGE_NUMBER
			+ (second_ch - ISO2022JP_BYTE_LOWER_LIMIT) ;

		if (pos + SJIS_PAGE_FIRST_CHAR  < SJIS_PAGE_NULL_CHAR) {
			second_ch_new = pos + SJIS_PAGE_FIRST_CHAR;
		}else{
			second_ch_new = pos + SJIS_PAGE_FIRST_CHAR + 1;
		}

		if (second_ch_new > SJIS_PAGE_LAST_CHAR) {
			return FALSE;
		}

		/* change the buffer */
		buf[i] = first_ch_new;
		buf[i+1] = second_ch_new;
	}
	return TRUE;
}



/********************************************************



********************************************************/

int jis2sjis(char *buf_in, int len_in, char *buf_out, int *len_out)
{
	int retval = 0;
	int i = 0;
	int match_len = 0;
	char *p = NULL;
	char *match = NULL;

	*len_out = len_in;
//	printf("in jis2sjis(): len_out = %d\n", *len_out);
	memcpy(buf_out, buf_in, len_in);

	while (*len_out > 0) {

		/* search & remove shift_in & shift_out tag */
		retval = search_shift_tag(buf_out, len_out, &match, &match_len);

		/* 未发现SHIFT_IN & SHIFT_OUT标志 */
		if (FALSE == retval) {
			DEBUG_SMTP(SMTP_DBG, 
				"in jis2sjis(): now, no SHIFT tag exists, return!\n");
//			printf("in jis2sjis(): now, no SHIFT tag exists, return!\n");
			return TRUE;
		}

		/* decode double bytes from iso2022jp(jis) to sjis */
		retval = decode_iso2022jp_sjis(match, match_len);
		
		if (SMTP_DBG) {

			DEBUG_SMTP(SMTP_DBG, "out match buffer: %s\n", match);
			for (i = 0; i < match_len; i++) {
				DEBUG_SMTP(SMTP_DBG, "match[%d]:%x\n", i, match[i]);
			}
		}
		
		if (FALSE == retval) {
			DEBUG_SMTP(SMTP_DBG,
				"in jis2sjis(): decode_iso2022jp_sjis() error\n!");
//			printf("in jis2sjis(): decode_iso2022jp_sjis() error\n!");
			return FALSE;
		}

	}
//	printf("in jis2sjis(): len_out <= 0!\n");

	return TRUE;
}

/*
main()
{
	unsigned char buffer[] = "\x20\x1B\x24\x40\x25\x61\x21\x3c\x25\x6b\x1B\x28\x42\x20\x1B\x24\x40\x25\x61\x21\x3c\x25\x6b\x1B\x28\x42";
	unsigned char buf_out[26];
	int i = 0;
	int retval = 0;
	int len_out = 0;
	int len_in = 26;
	
	retval = jis2sjis(buffer, len_in, buf_out, &len_out);
	if (retval < 0) {
		printf("jis2sjis error return!\n");
	}
	
	printf("in buffer: %s\n", buffer);
	printf("in buffer len: %d\n", len_in);
	for (i = 0; i < len_in; i++) {
		printf("buffer[%d]:%x\n", i, buffer[i]);
	}

	printf("out buffer: %s\n", buf_out);
	printf("out buffer len: %d\n", len_out);
	for (i = 0; i < len_out; i++) {
		printf("buf_out[%d]:%x\n", i, buf_out[i]);
	}

}

*/
