/*
 * $Header: /home/cvsroot/ns210501/baseline/src/official/linux/user/engine/src/public/lib_charset/conv_base64.c,v 1.1.1.1 2005/12/27 06:40:38 zhaozh Exp $
 */

/*
 * conv_base64.c
 *
 * shiyan
 *
 * 2002.5.17
 */

#include "conv_char.h"

#define LINE_CH_NUM	76 		/* size of encoded lines */
#define XX      	255		/* illegal base64 char */
#define EQ      	254		/* padding */
#define INVALID 	XX

const u_bit_8 etable[] =
   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

const u_bit_8 dtable[256] = {
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,62, XX,XX,XX,63,
    52,53,54,55, 56,57,58,59, 60,61,XX,XX, XX,EQ,XX,XX,
    XX, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
    15,16,17,18, 19,20,21,22, 23,24,25,XX, XX,XX,XX,XX,
    XX,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
    41,42,43,44, 45,46,47,48, 49,50,51,XX, XX,XX,XX,XX,

    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
};


/*
 * Function :	encode_base64(struct conv_arg*)
 * Purpose  :	A base64 encoder.	
 * Arguments:	struct conv_arg* parg: look conv_char.h for explanation.	
 * Return   :	int
 *				RETURN_ERROR	=>	failure
 *				RETURN_SUCCESS	=>	success
 *		
 */
int encode_base64(struct conv_arg *parg) 
{
	u_bit_16		line_ch_pos=0, line_counter=0;
	u_bit_32		in_len, out_len;
	u_bit_8			*pin_str, *pout_str, *pin_end, *pout_end;
	int				value = RETURN_ERROR;

	Trace((stdout,"encode_base64(): enter!\n"));

	in_len  = parg->m_len_in;
	out_len = parg->m_len_out;
	pin_str = parg->m_pstr_in;	
	pout_str= parg->m_pstr_out;
	if(pout_str == NULL || pin_str == NULL) {
		Trace((stderr,"encode_base64(): input pointer is NULL!\n"));
		value = RETURN_ERR_INPUT;
		goto err;
	}
	memset(pout_str,'\0',out_len);
	pin_end = pin_str + in_len;
	pout_end = pout_str;
	/* main conversion process */
	while (pin_str<pin_end && pout_end<pout_str+out_len) {
		line_ch_pos = 0;
		while((line_ch_pos < LINE_CH_NUM) && (pin_str<pin_end)) {
			*(pout_end++) = etable[ (*pin_str) >> 2];
			*(pout_end++) = etable[( ( (*pin_str) & 0x3) << 4)
					| ((*(pin_str+1))  >> 4)];
			*(pout_end++) = etable[ ( ( (*(pin_str+1)) & 0xF) << 2)
					| ((*(pin_str+2)) >> 6) ];
			*(pout_end++) = etable[(*(pin_str+2)) & 0x3F];

			pin_str+=3;
			line_ch_pos+=4;
		}
		*(pout_end++) = '\r';
		*(pout_end++) = '\n';
		line_counter++;
	}

	if(pin_str < pin_end) {
		Trace((stderr,"encode_base64(): output buffer length is too short!\n"));
		goto err;
	}
	/* 
	 * Replace characters in output stream with "=" pad
	 * characters if fewer than three characters were
	 * read from the end of the input stream. 
	 */
	if ( pin_str - pin_end == 1 ) {
		*(pout_end-3) = '=';
	}else if ( pin_str - pin_end == 2) {
		*(pout_end-3) = '=';
		*(pout_end-4) = '=';
	}

	parg->m_len_out 	= pout_end - pout_str;
	value = RETURN_SUCCESS ;
err:
	return value;

}


/*
 * Function :	decode_base64(struct conv_arg *parg)
 * Purpose  :	A base64 decoder.	
 * Arguments:	
 *				struct conv_arg *parg	=> look conv_char.h for explanation.
 * Return   :	int
 *				RETURN_ERROR	=>	failure
 *				RETURN_SUCCESS	=>	success
 */
int decode_base64(struct conv_arg *parg)
{
	u_bit_8		in_group[4]={0,0,0,0};
	u_bit_8		*pin_str, *pout_str, *pin_end, *pout_end;
	u_bit_32	in_len=0, out_len=0;
	int		value = RETURN_ERROR;
	int		i,counter, flag = 0;
	char	tmp[2048];
	
	Trace((stdout,"coming in decode_base64()\n"));

	in_len  = parg->m_len_in;
	out_len = parg->m_len_out;
	pin_str = parg->m_pstr_in;	
	pout_str= parg->m_pstr_out;
	if(pout_str == NULL || pin_str == NULL) {
		Trace((stderr,"decode_base64(): input pointer is NULL!\n"));
		value = RETURN_ERR_INPUT;
		goto err;
	}
	memset(pout_str,'\0',out_len);
	pin_end = pin_str + in_len;
	pout_end = pout_str;

	memcpy(tmp, pin_str, in_len);
	memset(tmp + in_len, '\0', 1);
	
	Trace((stdout,"parg->pin_end = %d\n",pin_end));
	Trace((stdout,"parg->pin_str = [%d]:%s[len:%d]\n",
				pin_str,tmp,in_len));

	/*
	 * the  main conversion process .
	 * if the character's number is = 4n+x,(x is 1,2or3)
	 * cut the last x  characters.
	 */ 
	while(pin_str < pin_end - 3 && pout_end < pout_str + out_len-2){
		Trace((stdout,"decode_base64():in while()\n"));
		counter = 0;
		for(i=0; i<4 && (pin_str + counter)<pin_end + i - 3;i++) {
			in_group[i] = dtable[(u_bit_8)(*(pin_str+i+counter))];
			if(in_group[i] == XX ) {
				i--;
				counter++;
			}
		}

		if (counter != 0) {
			Trace((stdout,"counter = %d\n",counter));
		}
		if( i != 4) {
			flag = 1;
			Trace((stdout,"will break,pin_str = %d, i+ counter = %d+%d = %d\n",
						pin_str, i, counter,i+counter));
			break;
		}
		
		*(pout_end++) = (in_group[0] << 2) | ( in_group[1] >> 4);
		*(pout_end++) = (in_group[1] << 4) | ( in_group[2] >> 2);
		*(pout_end++) = (in_group[2] << 6) | ( in_group[3]);
		pin_str += (4+counter);
	}
	
	if(pout_end > pout_str + out_len-2) {
		Trace((stderr,"decode_base64(): output buffer is not enougth!\n"));
		goto err;
	}
	

	if(*(pin_str - 2) == '=') {
		pout_end -= 2;
	}else if (*(pin_str - 1) == '=') {
		pout_end -= 1;
	}

	parg->m_len_out 	= pout_end - pout_str;
	parg->m_len_in		-= (pin_end - pin_str);
	Trace((stdout,"parg->pin_end = %d\n",pin_end));
	Trace((stdout,"parg->pin_str = %d\n",pin_str));
	memset(pout_str + parg->m_len_out, '\0',1);
	Trace((stdout,"parg->pout_str = %s\n",pout_str));
	Trace((stdout,"parg->m_len_in = %d\n",parg->m_len_in));
	value = RETURN_SUCCESS;
err:
	return value;
}

