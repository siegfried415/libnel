/*
 * $Header: /home/cvsroot/ns210501/baseline/src/official/linux/user/engine/src/public/lib_charset/conv_utf8.c,v 1.1.1.1 2005/12/27 06:40:38 zhaozh Exp $
 */

/*
 *	conv_utf8.c: A utf-8 encoder/decoder.
 * 
 * shiyan
 * 2002.5.14
 */
#include "conv_char.h"

#define BYTE2_HEAD	0xC0
#define BYTE3_HEAD	0xE0
#define	BYTES_TAIL	0x80

#define	MASK_1BIT 0x80
#define MASK_2BIT 0xC0
#define MASK_3BIT 0xE0
#define MASK_4BIT 0xF0

/*
 * Function :	int encode_utf8(struct conv_arg* parg)
 * Purpose  :   A UTF-8 encoder.
 * Arguments:   struct conv_arg* parg: look conv_char.h for explanation.
 * Return   :   int
 *				RETURN_ERROR    =>  failure
 *				RETURN_SUCCESS  =>  success
 */
int encode_utf8(struct conv_arg* parg)
{
	u_bit_8		*pin_str,*pin_end, *pout_str, *pout_end;
	u_bit_32	in_len=0, out_len;
	u_bit_16	unicode_value;
	int			value = RETURN_ERROR;

	Trace((stderr,"encode_utf8(): enter!\n"));
	if( parg == NULL ) {
		Trace((stderr,"encode_utf8(): input pointer is NULL!\n"));
		goto err;
	}
    
	in_len  = parg->m_len_in;
	out_len = parg->m_len_out;
	pin_str = parg->m_pstr_in;  
	pout_str = parg->m_pstr_out;

	if(pout_str == NULL || pin_str == NULL) {
		Trace((stderr,"encode_utf8(): input error!\n"));
		goto err;
	}
	pin_end = pin_str + in_len;
	pout_end = pout_str;

	memset(pout_str,'\0',out_len);

	/* input text is the big-endian */
	if(*(pin_str) == 0xFE && *(pin_str+1) == 0xFF ) {
		
		Trace((stdout,"encode_utf8(): find FE FF in the beginning.\n"));
		pin_str+=2;
		
		/* main conversion process */
		while(pin_str < pin_end - 1 && pout_end < pout_str+out_len - 2) {
			unicode_value = *(pin_str++);
			unicode_value = (unicode_value << 8) | *(pin_str++); 
			if(unicode_value < 0x80) {
				*(pout_end++) = unicode_value;
			}else if( unicode_value < 0x800) {
				*(pout_end++) = BYTE2_HEAD | ( unicode_value >> 6);
				*(pout_end++) = BYTES_TAIL | ( unicode_value & 0x3F);
			}else {
				*(pout_end++) = BYTE3_HEAD | ( unicode_value >> 12);
				*(pout_end++) = BYTES_TAIL | ( ( unicode_value >> 6 ) & 0x3F );
				*(pout_end++) = BYTES_TAIL | ( unicode_value & 0x3F);
			}
		}
		
	/* input text is the little-endian */
	}else if(*(pin_str) == 0xFF && *(pin_str+1) == 0xFE) {
			
		Trace((stdout,"encode_utf8():find FF FE in the beginning.\n"));
		pin_str+=2;

		/* main conversion process */
		while(pin_str < pin_end - 1 && pout_end < pout_str + out_len - 2) {
			unicode_value = *(pin_str++);
			unicode_value = unicode_value  | (*(pin_str++) << 8); 
			if(unicode_value < 0x80) {
				*(pout_end++) = unicode_value;
			}else if( unicode_value < 0x800) {
				*(pout_end++) = BYTE2_HEAD | ( unicode_value >> 6);
				*(pout_end++) = BYTES_TAIL | ( unicode_value & 0x3F);
			}else {
				*(pout_end++) = BYTE3_HEAD | ( unicode_value >> 12);
				*(pout_end++) = BYTES_TAIL | ( ( unicode_value >> 6 ) & 0x3F );
				*(pout_end++) = BYTES_TAIL | ( unicode_value & 0x3F);
			}
		}
	}else{
		Trace((stderr,"encode_utf8():No FF FE or FE FF in the beginning.\n"));
		goto err;
	}
    if( pin_str < pin_end - 1) {
        Trace((stderr,"encode_utf8():output buffer length is too short!\n"));
		Trace((stderr,"encode_utf8(): need %d, len is %d\n"
								,out_len,pout_end - pout_str));
		Trace((stderr,"pin_str = %d, pin_end = %d\n",
								(int)pin_str, (int)pin_end));
        goto err;
    }
	parg->m_len_out		= pout_end - pout_str;
	value = RETURN_SUCCESS;

err:
	return value;

}

/*
 * Function :	int decode_utf8(struct conv_arg* parg)
 * Purpose  :	A UTF-8 decoder.
 * Arguments:	struct conv_arg* parg: look conv_char.h for explanation.
 * Return   :	int
 *				RETURN_ERROR    =>  failure
 *				RETURN_SUCCESS  =>  success
 */
int decode_utf8(struct conv_arg* parg)
{
	u_bit_8		*pin_str,*pin_end, *pout_str, *pout_end;
	u_bit_32	in_len=0, out_len=0;
	u_bit_16	unicode_value;
	int			value = RETURN_ERROR;

	Trace((stdout,"decode_utf8(): enter!\n"));
	if( parg == NULL ) {
		Trace((stderr,"decode_utf8(): input pointer is NULL!\n"));
		goto err;
	}
    
	in_len  = parg->m_len_in;
	out_len = parg->m_len_out;
	pin_str = parg->m_pstr_in;  
	pout_str = parg->m_pstr_out;

	if(pout_str == NULL || pin_str == NULL) {
		Trace((stderr,"decode_utf8(): input error!\n"));
		goto err;
	}
	pin_end = pin_str + in_len;
	pout_end = pout_str;

	memset(pout_str,'\0',out_len);
	
	unicode_value = 0xFEFF;
	memcpy(pout_end,&unicode_value,2);
	pout_end+=2;

	/* main conversion process */
	while(pin_str < pin_end && pout_end < (pout_str + out_len) - 1) {


		unicode_value = *(pin_str++);
		/* value < 0x80, only 1 byte */
		if( (unicode_value & MASK_1BIT) == 0 ) {
				
		/* value < 0x800, only 2 bytes	*/
		}else if((unicode_value & MASK_3BIT) == 0xC0) {	
			if( ((*pin_str) & MASK_2BIT) == 0x80 ) {
				unicode_value &= 0x1F;
				unicode_value = ( unicode_value  << 6) | (*(pin_str++) & 0x3F) ;
			}else {
				Trace((stderr,"decode_utf8(): the 2nd error.\n"));
			}
		/* value < 0x10000, 3 bytes */
		}else if((unicode_value & MASK_4BIT) == 0xE0) {	
			if( ( ((*pin_str) & MASK_2BIT) == 0x80 ) 
						&& ( (*(pin_str+1) & MASK_2BIT) == 0x80) ) {
				unicode_value &= 0x0F;
				unicode_value = (unicode_value << 6) | (*(pin_str++) & 0x3F);
				unicode_value = (unicode_value << 6) | (*(pin_str++) & 0x3F);
			}else{
				Trace((stderr,"decode_utf8(): the 3rd byte error.\n"));
			}
		}else{
				Trace((stderr,"decode_utf8(): the 1st byte is error.\n"));
		}
		memcpy(pout_end, &unicode_value, 2);
		pout_end += 2;
	}

	/* output buffer is not enough */
	if( pin_str < pin_end ) {
		Trace((stderr,"decode_utf8():output buffer length is too short!\n"));
		goto err;
	}
	/* input content not completed, reading more chars then input  */
	if(pin_str > pin_end ) {
		Trace((stderr,"decode_utf8():input content is not completed!\n"));
		pout_end -= 2;
	}

	parg->m_len_out		= pout_end - pout_str;
	Trace((stdout,"decode_utf8():parg->m_len_out = %d\n",parg->m_len_out));
	value = RETURN_SUCCESS;

err:
	return value;
}
