/*
 * $Header: /home/cvsroot/ns210501/baseline/src/official/linux/user/engine/src/public/lib_charset/char_set.c,v 1.1.1.1 2005/12/27 06:40:38 zhaozh Exp $
 */

/*
 * char_set.c
 *
 * conversion characters between different character sets.
 *
 * shiyan
 *
 * 2002.5.22
 */

#include "conv_char.h"


struct coding_type all_encoding_type[] = {
	{BASE64,"base64"},
	{QP,"quoted-printable"},
	{HZ,"hz-gb-2312"},
	{UTF_8,"utf-8"},
	{GZIP,"gzip"},
};
struct coding_type all_charset_type[] = {
	{CHARSET_GB2312,"gb2312"},
	{CHARSET_GB2312,"gb"},

	
	/* begin: added by zhaozh, 2004.6.15 */
	{CHARSET_SJIS,"iso-2022-jp"},
	/* end */
	
	{CHARSET_BIG5,"big5"},
	{HZ,"hz-gb-2312"},
	{UTF_8,"utf-8"},
	{-1,NULL},
};

struct conv_fun all_fun[] = {
	{BASE64,ORIGINAL,decode_base64,NULL},
	{ORIGINAL,BASE64,encode_base64,NULL},
	{UUENCODE,ORIGINAL,decode_uuencode,NULL},
	{ORIGINAL,UUENCODE,encode_uuencode,"test_uu_encode.txt"},
	{QP,ORIGINAL,decode_qp,NULL},
	{ORIGINAL,QP,encode_qp,NULL},
	{HZ,ORIGINAL,decode_hz,NULL},
	{ORIGINAL,HZ,encode_hz,NULL},
	{UTF_8,ORIGINAL,decode_utf8,NULL},
	{ORIGINAL,UTF_8,encode_utf8,NULL},
	{GZIP,ORIGINAL,ungzip,NULL},
	{ORIGINAL,GZIP,gzip,NULL},
	{CHARSET_GB2312,CHARSET_BIG5,g2b,"table/g2b.tab"},
	{CHARSET_BIG5,CHARSET_GB2312,b2g,"table/b2g.tab"},
	{CHARSET_GB2312,CHARSET_UNICODE,g2u,"g2u.tab"},
	{CHARSET_UNICODE,CHARSET_GB2312,u2g,"u2g.tab"},
	{CHARSET_BIG5,CHARSET_UNICODE,b2u,"b2u.tab"},
	{CHARSET_UNICODE,CHARSET_BIG5,u2b,"u2b.tab"},
	{-1,-1,NULL,NULL},
};



extern u_bit_16* g2b_page[];
extern u_bit_16* b2g_page[];
extern u_bit_16* g2u_page[];
extern u_bit_16* u2g_page[];
extern u_bit_16* b2u_page[];
extern u_bit_16* u2b_page[];


#define	CONV_ERR 0xFFFF

/*
 * Function	:   int conv_set_gb_bg(struct conv_arg*,u_bit_16**)	 
 * Purpose  :  	convert characters between gb2312 and big5 character set. 
 * Arguments:	struct conv_arg* parg	: look conv_char.h for explanation.
 *				u_bit_16** 	pptab	: conversion array
 * Return   :   int
 *				RETURN_ERROR	=>  failure
 *				RETURN_SUCCESS	=>  success
 */
int conv_set_gb_bg(struct conv_arg *parg, u_bit_16** pptab) 
{
	u_bit_32	in_len, out_len;
	u_bit_16	out_val, *ptable;
	u_bit_8		*pin_str, *pout_str, *pin_end, *pout_end;
	u_bit_8		ch[2];
	int			value=RETURN_ERROR;

	Trace((stdout,"conv_set_gb_bg(): bigin!\n"));

	in_len  = parg->m_len_in;
	out_len = parg->m_len_out;
	pin_str = parg->m_pstr_in;
	pout_str =parg->m_pstr_out;
	pin_end = pin_str + in_len;
	if(pout_str == NULL) {
		Trace((stderr,"conv_set_gb_bg(): out_string's buffer is NULL!\n"));
		goto err;
	}
	memset(pout_str,'\0',out_len);
	pout_end = pout_str;

	while (pin_str<pin_end && pout_end < pout_str + out_len - 1) {

		ch[0] = *(pin_str++);
		
		/* 
		 * input char is a ASCII character (1 byte) 
		 */
		if((ch[0] & 0x80) == 0) {		
			*(pout_end++)=ch[0];
			continue;
		}
		
		/* 
		 * input char is  a chinese character (2 bytes) 
		 */
		
		/* use the page number(1st byte) to find the page's conversion table */
		ptable = *(pptab+ch[0]);
		if(ptable == NULL) {
			Trace((stderr,"conv_set_gb_bg():input is error in pos %d\n",
							pin_str - parg->m_pstr_in -1));
			continue;
		}
		/* use the page converison to find the character's conversion value */
		ch[1] = *(pin_str++);
		out_val = *(ptable+ch[1]);

		if(out_val!=CONV_ERR) {
			*(pout_end++)=(char)(out_val>>8);
			*(pout_end++)=(char)(out_val&0xFF);
		} else {
			Trace((stderr,"conv_set_gb_bg(): output an error char in %d!\n",
						pin_str - parg->m_pstr_in));
//			*(pout_end++)='?';
//			*(pout_end++)='?';
		}

	}

	/* output buffer is not enough */
	if(pin_str < pin_end) {
		Trace((stderr,"conv_set_gb_bg():output is longer then expected!\n"));
		goto err;
	/* input buffer is not correct, last character is not integrate */
	}else if(pin_str > pin_end) {
		Trace((stderr,"conv_set_gb_bg():output buffer is not enough.\n"));
		pout_end -= 2;
	}

	parg->m_len_out		= pout_end - pout_str; 
	value = RETURN_SUCCESS;
err:
	return value;

}

/*
 * Function	:   int conv_set_gu_bu(struct conv_arg*,u_bit_16**)	 
 * Purpose  :  	convert characters :from gb2312 and big5 into unicode. 
 * Arguments:	struct conv_arg* parg	: look conv_char.h for explanation.
 *				u_bit_16**	 pptab		: conversion array
 * Return   :   int
 *				RETURN_ERROR	=>  failure
 *				RETURN_SUCCESS	=>  success
 */
int conv_set_gu_bu(struct conv_arg *parg, u_bit_16** pptab) 
{
	u_bit_32	in_len,out_len;
	u_bit_8		*pin_str, *pout_str, *pin_end, *pout_end;
	u_bit_8		ch[2];
	u_bit_16	out_val, *ptable;
	int			value = RETURN_FAILURE;

	Trace((stderr,"conv_set_gu_bu(): enter!\n"));

	in_len  = parg->m_len_in;
	out_len = parg->m_len_out;
	pin_str = parg->m_pstr_in;
	pout_str =parg->m_pstr_out;
	
	if(pout_str == NULL) {
		Trace((stderr,"conv_set_gu_bu(): out_string's buffer is error!\n"));
		goto err;
	}
	memset(pout_str,'\0',out_len);
	pout_end = pout_str;
	pin_end = pin_str + in_len;

	/* set 2 bytes of the output buffer :FF FE or FE FF */
	out_val = 0xFEFF;					
	memcpy(pout_end,&out_val,2);
	pout_end+=2;
		
	/* main conversion part */
	while (pin_str<pin_end && pout_end < pout_str + out_len - 1) {

		ch[0] = *(pin_str++);

		/* 
		 * input char is a ASCII character (1 byte) 
		 */
		if((ch[0] & 0x80) == 0) {
			out_val = ch[0];
			memcpy(pout_end,&out_val,2);
			pout_end += 2;
			continue;
		}

		/* 
		 * input char is  a chinese character (2 bytes) 
		 */
		
		/* use the page number(1st byte) to find the page's conversion table */
		ptable = *(pptab+ch[0]);
		if(ptable == NULL) {
			Trace((stderr,"conv_set_gu_bu(): input char err,in positoin %d\n",
					pin_str - parg->m_pstr_in));
			continue;
		}
		/* use the page converison to find the character's conversion value */
		ch[1] = *(pin_str++);
		out_val = *(ptable+ch[1]);
		if(out_val==CONV_ERR) {
			Trace((stderr,"conv_set_gu_bu(): input char err,in positoin %d\n",
					pin_str - parg->m_pstr_in));
//			out_val = '?';
		}
		memcpy(pout_end,&out_val,2);
		pout_end += 2;
		
	}
	/* output buffer is not enough */
	if(pin_str < pin_end) {
		Trace((stderr,"conv_set_gu_bu():output buffer is not enough!\n"));
		goto err;
	/* input buffer is not correct, last character is not integrate */
	}else if(pin_str > pin_end) {
		Trace((stderr,"conv_set_gu_bu(): output buffer is not enough.\n"));
		pout_end -= 2;
	}

	parg->m_len_out		= pout_end - pout_str; 
	value = RETURN_SUCCESS;
err:
	return value;

}



/*
 * Function	:   int conv_set_ug_ub(struct conv_arg*,u_bit_16**)	 
 * Purpose  :  	convert characters :from unicode into gb2312 or big5. 
 * Arguments:	struct conv_arg* parg	: look conv_char.h for explanation.
 *				u_bit_16** pptab	: conversion array
 * Return   :   int
 *				RETURN_ERROR	=>  failure
 *				RETURN_SUCCESS	=>  success
 */
int conv_set_ug_ub(struct conv_arg *parg, u_bit_16** pptab) 
{
	u_bit_32	in_len, out_len;
	u_bit_8		ch[2];
	u_bit_8		*pin_str, *pout_str, *pin_end, *pout_end;
	u_bit_16	out_val, *ptable;
	int			value = RETURN_ERROR;

	Trace((stdout,"conv_set_ug_ub(): enter!\n"));

	in_len  = parg->m_len_in;
	out_len = parg->m_len_out;
	pin_str = parg->m_pstr_in;
	pout_str =parg->m_pstr_out;
	if(pout_str == NULL) {
		Trace((stderr,"conv_set_ug_ub(): output buffer is error!\n"));
		goto err;
	}
	memset(pout_str,'\0',out_len);
	pout_end = pout_str;
	pin_end = pin_str + in_len;

	/* input text is the big-endian */
	if(*(pin_str) == 0xFE && *(pin_str+1) == 0xFF ) {   

		Trace((stdout,"conv_set_ug_ub():find FE FF in the beginning.\n"));
		pin_str+=2;
		
		/* main conversion part */
		while (pin_str<pin_end && pout_end < pout_str + out_len - 1) {
			ch[0] = *(pin_str++);
			ch[1] = *(pin_str++);
			/* 
		 	* input char is a ASCII character (1st byte is '\0') 
		 	*/
			if(ch[0] == 0 ) {
				*(pout_end++) = ch[1];
				continue;
			}

			/* 
			 * input char is  a chinese character (2 bytes) 
			 */
		
			/* use the page number(1st byte) to find the conversion table */
			ptable = *(pptab + ch[0]);
			if(ptable == NULL) {
				Trace((stderr,"conv_set_ug_ub(): one char error in pos %d\n",
							pin_str - parg->m_pstr_in - 1));
				continue;
			}
			/* use converison table to find the character's conversion value */
			out_val = *(ptable+ch[1]);

			if(out_val!=CONV_ERR) {
				*(pout_end++)=(out_val>>8);
				*(pout_end++)=(out_val&0xFF);
			} else {
//				*(pout_end++) = '?';
//				*(pout_end++) = '?';
			}
		}
	/* input text is the little-endian */
	}else if(*(pin_str) == 0xFF && *(pin_str+1) == 0xFE) {  

        Trace((stdout,"conv_set_ug_ub(): find FF FE in the beginning.\n"));
        pin_str+=2;

		/* main conversion part */
		while (pin_str<pin_end && pout_end < pout_str + out_len - 1) {
			ch[1] = *(pin_str++);
			ch[0] = *(pin_str++);
			/* 
		 	* input char is a ASCII character (2nd byte is '\0') 
		 	*/
			if(ch[0] == 0 ) {
				*(pout_end++) = ch[1];
				continue;
			}

			/* 
			 * input char is  a chinese character (2 bytes) 
			 */
		
			/* use the page number(1st byte) to find the conversion table */
			ptable = *(pptab + ch[0]);
			if(ptable == NULL) {
				Trace((stderr,"conv_set_ug_ub(): character error!\n"));
				continue;
			}

			/* use converison table to find the character's conversion value */
			out_val = *(ptable+ch[1]);
			if(out_val!=CONV_ERR) {
				*(pout_end++)= (out_val>>8);
				*(pout_end++)= (out_val&0xFF);
			} else {
//				*(pout_end++)='?';
//				*(pout_end++)='?';
			}
		}
    }else{
		Trace((stderr,"not find FF FE or FE FF in the beginning.\n"));
	}
	/* output buffer is not enough */
	if(pin_str < pin_end) {
		Trace((stderr,"conv_set_gb_bg():output is longer then expected!\n"));
		goto err;
	/* input buffer is not correct, last character is not integrate */
	}else if(pin_str > pin_end) {
		Trace((stderr,"conv_set_gb_bg():output buffer is not enough.\n"));
		pout_end -= 2;
	}

	parg->m_len_out		= pout_end - pout_str; 
	value = RETURN_SUCCESS;
err:
	return value;

}

/*
 * Function	:	int g2b(struct conv_arg *parg)   	 
 * Purpose  :  	convert character's in gb2312 set into big5 set 
 * Arguments:	struct conv_arg* parg	: look conv_char.h for explanation.
 * Return   :   int
 *				RETURN_ERROR	=>  failure
 *				RETURN_SUCCESS	=>  success
 */
int g2b(struct conv_arg *parg)
{
	return conv_set_gb_bg(parg,g2b_page);
}


/*
 * Function	:   int b2g(struct conv_arg *parg)	 
 * Purpose  :  	convert character's in big5 set into gb2312 set.
 * Arguments:	struct conv_arg* parg	: look conv_char.h for explanation.
 * Return   :   int
 *				RETURN_ERROR	=>  failure
 *				RETURN_SUCCESS	=>  success
 */
int b2g(struct conv_arg* parg)
{
	return conv_set_gb_bg(parg,b2g_page);
}

/*
 * Function	:   int g2u(struct conv_arg *parg)	 
 * Purpose  :  	convert character's in gb2312 set into unicode set.
 * Arguments:	struct conv_arg* parg	: look conv_char.h for explanation.
 * Return   :   int
 *				RETURN_ERROR	=>  failure
 *				RETURN_SUCCESS	=>  success
 */
int g2u(struct conv_arg* parg)
{
	return conv_set_gu_bu(parg,g2u_page);
}


/*
 * Function	:   int u2g(struct conv_arg *parg)	 
 * Purpose  :  	convert character's in unicode set into gb2312 set.
 * Arguments:	struct conv_arg* parg	: look conv_char.h for explanation.
 * Return   :   int
 *				RETURN_ERROR	=>  failure
 *				RETURN_SUCCESS	=>  success
 */
int u2g(struct conv_arg* parg)
{
	return conv_set_ug_ub(parg,u2g_page);
}


/*
 * Function	:   int b2u(struct conv_arg *parg)	 
 * Purpose  :  	convert character's in big5 set into unicode set.
 * Arguments:	struct conv_arg* parg	: look conv_char.h for explanation.
 * Return   :   int
 *				RETURN_ERROR	=>  failure
 *				RETURN_SUCCESS	=>  success
 */
int b2u(struct conv_arg* parg)
{
	return conv_set_gu_bu(parg,b2u_page);
}


/*
 * Function	:   int u2b(struct conv_arg *parg)	 
 * Purpose  :  	convert character's in unicode set into big5 set.
 * Arguments:	struct conv_arg* parg	: look conv_char.h for explanation.
 * Return   :   int
 *				RETURN_ERROR	=>  failure
 *				RETURN_SUCCESS	=>  success
 */
int u2b(struct conv_arg* parg)
{
	return conv_set_ug_ub(parg,u2b_page);
}

