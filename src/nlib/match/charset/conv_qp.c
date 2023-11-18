/*
 * $Header: /home/cvsroot/ns210501/baseline/src/official/linux/user/engine/src/public/lib_charset/conv_qp.c,v 1.1.1.1 2005/12/27 06:40:38 zhaozh Exp $
 */

/*
 * qp.c: A quoted-printable encoder/decoder.
 * 
 * shiyan
 * 2002.5.14
 */
#include "conv_char.h"


#define SP 	0x20
#define CR	0x0D
#define LF	0x0A
#define HT	0x09
#define FLAG	0x3d		//character '='.

#define MAX_QP_LINE_LEN	76	// The Quoted-Printable encoding REQUIRES that 
								// encoded lines be no more than 76
          						// characters long.   

#define IS_SPACE(c)		((c) == SP || (c) == HT)
#define NEED_ENCODE(c)	( ((c) > 126 ) \
						|| ( (c) == '=' )\
						|| ( (c) < SP && (c) != HT && (c) != CR && (c) != LF ))
#define DECODE_LOWER_CH(c)	( ( (c)>='a' && (c)<='f' ) ?\
								( 10 + (c) - 'a') : ( (c) - '0' ) )
/*
 * Function :	meet_enter(u_bit_8**)
 * Purpose  :	whether the position of pstr is "\r\n" or '\n'
 * Arguments:	
 * 		u_bit_8** ppstr	=> point to the string.
 * Return   :	
 *		0		=>	no enter			
 *		1		=>	find '\n'
 *		2		=>	find "\r\n"
 */
int meet_enter(u_bit_8** ppstr)
{
	if( **ppstr == CR && *(*ppstr+1)==LF){
		*ppstr += 2; 
		return 2;
	}else if(**ppstr == LF){
		*ppstr += 1;
		return 1;
	}
	return 0;
}


/*
 * Function :   encode_qp(struct conv_arg*)
 * Purpose  :   A QP encoder.
 * Arguments:   struct conv_arg* parg: look conv_char.h for explanation.
 * Return   :   int
 *				RETURN_ERROR    =>  failure
 *				RETURN_SUCCESS  =>  success
 */
int encode_qp(struct conv_arg	*parg)
{
	u_bit_8		*pin_str,*pin_end, *pout_str, *pout_end;
	u_bit_8		ch;
	u_bit_32	in_len=0, out_len, line_ch_num=0;
	int			value = RETURN_ERROR;
	int 		flag=1;		//flag=1 :output "=\r\n" in a line's end
							//flag=0 :needn't do that.

	if( parg == NULL ) {
		Trace((stderr,"encode_qp(): input pointer is NULL!\n"));
		goto err;
	}

    in_len  = parg->m_len_in;
	out_len = parg->m_len_out;
    pin_str = parg->m_pstr_in;	
	pout_str = parg->m_pstr_out;

	if(pout_str == NULL || pin_str == NULL) {
		Trace((stderr,"encode_qp(): input error!\n"));
		goto err;
    }
	pin_end = pin_str + in_len;
	pout_end = pout_str;

	memset(pout_str,'\0',out_len);

	/* main conversion process */
	while(pin_str<pin_end && pout_end < pout_str + out_len) {
		line_ch_num =0;
		/* encode one line */
		while(line_ch_num< (MAX_QP_LINE_LEN -3) 
						&& pin_str<pin_end
						&& pout_end < pout_str + out_len) {

			if(meet_enter(&pin_str)) {
				memcpy(pout_end,"\r\n",2);
				pout_end+=2;
				flag = 0;
				break;
			}	

			/* if current character need being encoded. */
			ch = *(pin_str++);
			if( NEED_ENCODE(ch) ) {
				*(pout_end++) = '=';
				sprintf(pout_end,"%.2X",ch);
				pout_end+=2;
				line_ch_num += 3;
				flag = 1;
				continue;
			}
			
			/* if current char is space and is the last char in the line */
			if(IS_SPACE(ch) && meet_enter(&pin_str) != 0 ) {
				*(pout_end++) = '=';
				sprintf(pout_end,"%.2X",ch);
				pout_end+=2;
				memcpy(pout_end,"\r\n",2);
				pout_end+=2;
				flag = 0;
				break;
			} 
			
			/* the current char needn't being encoded */
			*(pout_end++) = ch;
			line_ch_num++;
			flag = 1;
			
		}
		
		/* if original line is not finished, output '=' in new line's end */
		if(flag == 1) {
			memcpy(pout_end,"=\r\n",3);
			pout_end+=3;
		}
		flag = 1;
	}

	if( pin_str < pin_end ) {
		Trace((stderr,"encode_qp():output buffer length is too short!\n"));
		goto err;
	}	

	parg->m_len_out		= pout_end - pout_str;
	value = RETURN_SUCCESS;
err:
	return value;

}
	



/*
 * Function :	decode_qp(struct conv_arg*)
 * Purpose  :	A QP decoder.
 * Arguments:	struct conv_arg* parg: look conv_char.h for explanation.
 * Return   :	int
 *				RETURN_ERROR    =>  failure
 *				RETURN_SUCCESS  =>  success
 */
int decode_qp(struct conv_arg* parg)
{
	u_bit_8 	ch[2] ;
	u_bit_8		*pin_str, *pin_end, *pout_str, *pout_end;
	u_bit_32	in_len=0, out_len = 0;
	int value = RETURN_ERROR, flag;

	if( parg == NULL ) {
		Trace((stderr,"decode_qp(): input pointer is NULL!\n"));
		goto err;
	}

    in_len  = parg->m_len_in;
	out_len = parg->m_len_out;
    pin_str = parg->m_pstr_in;	
	pout_str = parg->m_pstr_out;

	if(pout_str == NULL || pin_str == NULL) {
		Trace((stderr,"decode_qp(): input error!\n"));
		goto err;
    }
	pin_end = pin_str + in_len;
	pout_end = pout_str;

	memset(pout_str,'\0',out_len);
	flag = 1;

	/* main conversion process */
	while(pin_str<pin_end && pout_end < pout_str + out_len) {
		ch[0] = *(pin_str++);
		
		/* ch[0] need being decoded. */
		if(ch[0] == '=') {
			/* if input content not enough */
			if(pin_str + 2 > pin_end ) {
				pin_str--;
				flag = 0;
				break;
			}
			ch[0] = *(pin_str++);
			ch[1] = *(pin_str++);
				
			/* meet a '=' in a line's end. */
			if( (ch[0]==CR) && (ch[1]==LF) ) {	
				continue;
			/* convert a string into a number: */
			}else {		
				/* 
				 * if two characters is hexadecimal digits 
				 * change them into lower case
				 * and then convert them
				 */
				if( isxdigit(ch[0]) != 0 && isxdigit(ch[1]) != 0) {
					ch[0] |= 0x20;		
					ch[1] |= 0x20;
					Trace((stderr,"ch[0] = %c,ch[1] = %c\n",ch[0],ch[1]));
					Trace((stderr,"ch[0] = %d\n",DECODE_LOWER_CH(ch[0]) ));
					*(pout_end++) = ( ( DECODE_LOWER_CH(ch[0]) << 4) |
									  (	DECODE_LOWER_CH(ch[1])     ) );
				}
			}
		}else{
			/* ch[0] needn't being decoded. */
			*(pout_end++) = ch[0];
		} /* if(ch[0] == '=') */
	} /* while(pin_str<pin_end) */

	/* if input not enough */
	if(pin_str < pin_end && 1 == flag ) {
		Trace((stderr,"decode_qp():output buffer length is too short!\n"));
		goto err;
	}	

	parg->m_len_out = pout_end - pout_str;
	parg->m_len_in  -= (pin_end - pin_str);
	value = RETURN_SUCCESS;
err:
	return value;
}
