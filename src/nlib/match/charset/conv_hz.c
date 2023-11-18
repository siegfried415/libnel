/*
 * $Header: /home/cvsroot/ns210501/baseline/src/official/linux/user/engine/src/public/lib_charset/conv_hz.c,v 1.1.1.1 2005/12/27 06:40:38 zhaozh Exp $
 */

/*
 * conv_hz.c: A HZ encoder/decoder.
 * 
 * shiyan
 * 2002.5.15
 */
#include "conv_char.h"

#define ENTER_STR	"\r\n"

#define decode_char()	{\
	u_bit_8  ch1,ch2;					\
	u_bit_8 penter[3]={0,0,0};			\
	short   enter_len;					\
\
	strcpy(penter,ENTER_STR);			\
	enter_len = strlen(penter);			\
\
	while(pin_str + 1<pin_end && pout_end - pout_str < out_len - 1)	{	\
		/* get 2 chars.	*/				\
		ch1 = *(pin_str++);				\
		ch2 = *(pin_str++);				\
\
		if((ch1 & 0x80) == 1) {			\
			if(ch2 == '\r') {			\
				*(pout_end++) = ch1;	\
				flag = 2;				\
				continue;				\
			}else if (ch2 == '\n') {	\
				*(pout_end++) = ch1;	\
				memcpy(pout_end,penter,enter_len);	\
				pout_end += enter_len;	\
				flag = 2;				\
				continue;				\
			}else{						\
				*(pout_end++) = ch1;	\
				*(pout_end++) = (ch2 | 0x80);		\
				flag = 2;				\
				continue;				\
			}							\
		}else if(ch1 == '\r') {			\
			pin_str--;					\
			flag = 2;				\
			continue;					\
		}else if(ch1 == '\n') {			\
			memcpy(pout_end,penter,enter_len);		\
			pout_end += enter_len;		\
			pin_str--;					\
			flag = 2;				\
			continue;					\
		}else if(ch1 == '~') {			\
			if((ch2 & 0x80)==1) {		\
				*(pout_end++) = ch1;	\
				pin_str--;				\
				flag = 2;					\
				continue;				\
			}else if(ch2=='\r') {		\
				*(pout_end++) = ch1;	\
				flag = 2;					\
				continue;				\
			}else if(ch2 =='\n') {		\
				*(pout_end++) = ch1;	\
				memcpy(pout_end,penter,enter_len);	\
				pout_end += enter_len;	\
				flag = 2;					\
				continue;				\
			}else if(ch2 == '}') {		\
				/* *out_len = pout_end - pout_str; */		\
				flag = 0;\
				break;			\
			}else{						\
				*(pout_end++) = ch1;	\
				*(pout_end++) = ch2;	\
				flag = 2;					\
				continue;				\
			} 							\
		/* ch1 is a encoded character. */			\
		}else{							\
			if((ch2 & 0x80) ==1) {		\
				*(pout_end++) = ch1;	\
				pin_str--;				\
				flag = 2;					\
				continue;				\
			}else if(ch2 == '\r') {		\
				*(pout_end++) = ch1;	\
				flag = 2;					\
				continue;				\
			}else if(ch2 == '\n') {		\
				*(pout_end++) = ch1;	\
				memcpy(pout_end,penter,enter_len);	\
				pout_end += enter_len;	\
				flag = 2;					\
				continue;				\
			/* ch2 is a encoded character. */		\
			}else{						\
				*(pout_end++) = (ch1 | 0x80);		\
				*(pout_end++) = (ch2 | 0x80);		\
				flag = 2;					\
				continue;				\
			}							\
		}								\
	}									\
}							



/*
 * Function :	encode_hz(struct conv_arg*)
 * Purpose  :	A HZ encoder.	
 * Arguments:   struct conv_arg *parg   => look conv_char.h for explanation
 * Return   :	int
 *				RETURN_ERROR    =>  failure
 *				RETURN_SUCCESS  =>  success
 */
int encode_hz(struct conv_arg* parg)
{
	u_bit_8		*pin_str,*pin_end, *pout_str, *pout_end;
	u_bit_8		ch;
	u_bit_32	in_len=0, out_len = 0;
	int			value = RETURN_ERROR, flag = 0;

	if( parg == NULL ) {
		Trace((stderr,"encode_hz():input pointer is NULL!\n"));
		goto err;
	}

    in_len  = parg->m_len_in;
	out_len = parg->m_len_out;
    pin_str = parg->m_pstr_in;
	pout_str = parg->m_pstr_out;	

	if(pout_str == NULL || pin_str == NULL ) {
		Trace((stderr,"encode_hz():input pointer is NULL!\n"));
		goto err;
    }
	memset(pout_str,'\0',out_len);
	pout_end = pout_str;
	pin_end = pin_str + in_len;

	/* main conversion process */
	while(pin_str<pin_end && pout_end < pout_str + out_len - 3) {
		ch = *(pin_str++);

		if(flag == 0) {
			/* if input '~',output 2 bytes '~' instead */
			if(ch == '~') {
				memset(pout_end, '~',2);
				pout_end += 2;
				continue;
			}else if((ch & 0x80) == 0 ) {
				*(pout_end++) = ch;
				continue;
			/* if current character need encode */
			}else {
				memcpy(pout_end,"~{",2);
				pout_end+=2;
				*(pout_end++) = (ch & 0x7f);
				flag = 1;
				continue;
			}
		/* flag = 1 means encoding chars */
		}else{
			/* encoding input char */
			if((ch & 0x80) != 0) {
				*(pout_end++) = (ch & 0x7f);
			/* input char needn't encoding */
			}else{
				memcpy(pout_end,"~}",2);
				pout_end+=2;
				*(pout_end++) = ch;
				flag = 0;
				
			}
		}
	}

	if(pin_str < pin_end ) {
		Trace((stderr,"encode_hz():output buffer is not enough!\n"));
		goto err;
	}
	if(flag == 1 && out_len -2 >= pout_end - pout_str) {
		memcpy(pout_end,"~}",2);
		pout_end+=2;
	}

	parg->m_len_out  = pout_end - pout_str;
	value = RETURN_SUCCESS;
err:
	return value;

}
	

/*
 * Function :	decode_hz(struct conv_arg*)
 * Purpose  :	A HZ decoder.	
 * Arguments:   struct conv_arg *parg   => look conv_char.h for explanation
 * Return   :	int
 *				RETURN_ERROR    =>  failure
 *				RETURN_SUCCESS  =>  success
 */
int decode_hz(struct conv_arg *parg)
{
	u_bit_8		*pin_str, *pin_end, *pout_str, *pout_end, ch;
	u_bit_32	in_len=0, out_len;
	int value = -1, flag = 0;

	if( parg == NULL) {
		Trace((stderr,"parg is NULL!\n"));
		goto err;
	}

    in_len  = parg->m_len_in;
	out_len = parg->m_len_out;
    pin_str = parg->m_pstr_in;
	pout_str = parg->m_pstr_out;	

	flag = parg->m_flag;
	Trace((stdout,"decode_hz():input m_flag = %d,out_len = %d, in_len = %d\n",
				flag,out_len, in_len));

	if(pout_str == NULL || pin_str == NULL ) {
		Trace((stderr,"encode_hz():input pointer is NULL!\n"));
		goto err;
    }
	memset(pout_str,'\0',out_len);
	pout_end = pout_str;
	pin_end = pin_str + in_len;

	/* main conversion process */
	while(pin_str<pin_end && pout_end <= pout_str + out_len - 1) {

		ch = *(pin_str++);
		
		/* if (haven't get "~{" before)*/
		if (flag == 0) {
			
			/* if current char isn't '~', output it without conversion */
			if(ch!='~') {
				*(pout_end++) = ch;
				Trace((stdout,"ch = %d",ch));
				continue;
			}
			/* already get "~', set flag */
			flag = 1;
			continue;
		}
			
		/* if(get "~" before'{') */
		if (flag == 1 ) {
			
			/* if current char is '~', examine the next char */
//			ch = *(pin_str++);
			
			if ( ch != '{') {
				/* if next char is also '~', output one '~' only */
				if(ch == '~') {
					*(pout_end++) = '~';
					flag = 0;
					continue;
				/* if next char is '\n', ouput nothing */
				}else if(ch == '\n') {
					flag = 0;
					continue;
				/* if next chars are '\r' and '\n' ,jump them output nothing */ 
				}else if( ch == '\r' ) {
					*(pout_end++) = '~';
					flag = 0;
					continue;
				}else{
					*(pout_end++) = '~';
					*(pout_end++) = ch;
					flag = 0;
					continue;
				}
			}
			
			flag = 2;
			continue;
		}
		
		/* 
		 * already get "~{" 
		 * chars following them needed to be decoded 
		 */
		if (flag == 2) {

			pin_str--;
			
			decode_char();
			/*
			 * flag= 3 means already get "~" in the end.
			 * flag= 2 means already get "~{".
			 * flag= 1 means already get "~" before.
			 * flag= 0 means not get "~}" in the end.
			 * so when flag = 1, it may left one char without decode,
			 * and should saving it and decode it in the next time.
			 */ 
			if (0 == flag) {
				Trace((stdout,"decode_hz():after decode,flag = 0\n"));
				Trace((stdout,"decode_hz():after decode,next char = %c\n",
							*(pin_str)));
				continue;
			}else if(flag == 2 && (pin_str == pin_end - 1)){
				if (*pin_str == '~') {
					pin_str ++;
					flag = 3;
				}else {
					ch = *(pin_str++);
					*(pout_end++) = (ch | 0x80);
					Trace((stdout," out_len = %d\n",pout_end - pout_str));
					flag = 4;
				}
					
				break;
			}else{
				break;
			}
		}

		/* if get '~' befor '}' */
		if(flag == 3) {
			if (ch == '}') {
				flag = 0;
				continue;
			}else {
				*(pout_end++) = '~';
				flag = 2;
				pin_str--;
				continue;
			}
		}

		/* if during decode hz, already decode one char. */
		if( flag == 4) {
			*(pout_end++) = (ch | 0x80);
			flag = 2;
			continue;
		}
			
			
	}
	if(pout_end - pout_str > out_len - 1) {
		Trace((stderr,"dncode_hz():output buffer is not enough!\n"));
		Trace((stderr,"dncode_hz():pout_end = %d,pout_str = %d,out_len = %d\n",
					pout_end, pout_str, out_len));
		Trace((stderr,"dncode_hz():pout_end - pout_str = %d - %d = %d\n",
					pout_end, pout_str, pout_end - pout_str));
		goto err;
	}

	parg->m_flag = flag;
	Trace((stdout,"decode_hz.c():flag =  %d\n",parg->m_flag));
	Trace((stdout,"decode_hz.c():input %d\n",parg->m_len_in));
	parg->m_len_in = pin_str - parg->m_pstr_in;
	Trace((stdout,"decode_hz.c():decode %d\n",parg->m_len_in));
	parg->m_len_out = pout_end - pout_str;
	value = RETURN_SUCCESS;
err:
	return value;
}
