/*
 * $Header: /home/cvsroot/ns210501/baseline/src/official/linux/user/engine/src/public/lib_charset/conv_uuencode.c,v 1.1.1.1 2005/12/27 06:40:38 zhaozh Exp $
 */

/*
 * conv_uuencode.c
 * 
 * shiyan
 * 2002.5.9
 */
#include "conv_char.h"
#include <string.h>
#define MAX_NAME_LENGTH 128			// max file name length.
#define CH_PER_LINE		45			
#define	ENC(c) ((c) ? ((c) & 0x3F) + ' ': '`')	// character encode
#define	DEC(c)	(((c) - ' ') & 0x3F)   			// character decode
#define IS_SPACE(c) ( (c)==' ' || (c) == '\r' || (c) == '\n' || (c) == '\t')

/*
 * Function :	encode_uuencode(struct conv_arg *parg)
 * Purpose  :	A uuencode encoder.	
 * Arguments:	struct conv_arg *parg 	=> look conv_char.h for explanation
 * Return   :	int
 *				RETURN_ERROR    =>  failure
 *				RETURN_SUCCESS  =>  success 
 */
int encode_uuencode(struct conv_arg *parg)
{
	u_bit_8		*pin_str, *pout_str;
	u_bit_8		ch;
	u_bit_8		*penter = NULL, pmode[4];
	u_bit_16 	line_counter=0, line_number=0, line_ch_num=0;
	u_bit_32	in_pos=0, out_pos=0;
	u_bit_32	in_len=0, out_len=0, flag;
	u_bit_16    i, enter_len, file_name_len, mode_len;
	int			value = RETURN_ERROR;

	if(parg == NULL ) {
		Trace((stderr,"encode_uuencode(): input pointer is NULL!\n"));
		goto err;
	}

	
	if(parg->m_flag>>(sizeof(parg->m_flag)*8-1) == 1 ) {
								//first bit of m_flag is 1; enter is '\n' 
		Trace((stdout,"encode_uuencode():enter is \'\\n\'!\n"));
		penter = (u_bit_8 *)calloc(2,sizeof(u_bit_8));
		memcpy(penter,"\n\0",2);
		enter_len = 1;
	}else{						//first bit of m_flag is 0;,enter is "\r\n"
		Trace((stdout,"encode_uuencode():enter is \'\\r\\n\'!\n"));
		penter = (u_bit_8 *)calloc(3,sizeof(u_bit_8));
		memcpy(penter,"\r\n\0",3);
		enter_len = 2;
	}

    in_len  = parg->m_len_in;
    out_len = parg->m_len_out;
	pin_str = parg->m_pstr_in;
	pout_str = parg->m_pstr_out;	
	flag = parg->m_flag;
	
	if(pout_str == NULL || pin_str == NULL) {
		Trace((stderr,"encode_uuencode():input error!\n"));
		goto err;
    }

	mode_len		= 3;
	//get mode string.
	pmode[0] = ( ( flag >> 6 ) & 7) + '0';
	pmode[1] = ( ( flag >> 3 ) & 7) + '0';
	pmode[2] = (   flag	       & 7) + '0';
	pmode[3] = '\0';

	Trace((stdout,"encode_uuencode():get mode OK!\n"));
	
	if(in_len == 0 || out_len == 0 ) {
		Trace((stderr,"encode_uuencode():buffer length == 0!\n"));
		goto err;
	} 
	line_number = (in_len+44)/CH_PER_LINE;

	/* output "begin..." line */
	if( (flag & (1<<30) ) != 0 )	{
		Trace((stdout,"encode_uuencode():needn't output \"begin\" line!\n"));
		file_name_len	= strlen(parg->m_pfile_name);
		if(file_name_len == 0 ){
			Trace(( stderr,"encode_uuencode(): not input file name !"));
			goto err;
		}
		sprintf(pout_str,"begin %s %s%s",pmode,parg->m_pfile_name,penter);
		out_pos += (file_name_len+10+enter_len);
	}
	
	/* encoding: */
	Trace((stdout,"encode_uuencode():will enter while()!\n"));
	line_counter = 0; 
	while(line_counter <line_number && out_pos < out_len - 3) {
		line_ch_num = (line_counter+1==line_number)?in_len-in_pos:CH_PER_LINE;
		memset(pout_str+out_pos,ENC(line_ch_num),1);
		out_pos++;
		i=0;
		while(i<line_ch_num && out_pos < out_len - 3 ) {
			ch = pin_str[in_pos]  >> 2;
			pout_str[out_pos] = ENC(ch);

			ch = ((pin_str[in_pos] & 0x3) << 4)	
					| (pin_str[in_pos+1] >> 4) ;
			pout_str[out_pos+1] = ENC(ch);

			ch = ((pin_str[in_pos+1] & 0xF) << 2)
					| (pin_str[in_pos+2]>> 6) ;
			pout_str[out_pos+2] = ENC(ch);

			ch = pin_str[in_pos+2] & 0x3F ;
			pout_str[out_pos+3] = ENC(ch);
			in_pos += 3;
			out_pos += 4;
			i+=3;
		}
		if(i == line_ch_num +1) {
			pout_str[out_pos-1] = '`';
		}else if(i == line_ch_num +2) {
			pout_str[out_pos-1] = '`';
			pout_str[out_pos-2] = '`';
		}
		memcpy(pout_str+out_pos,penter,enter_len);
		out_pos += enter_len;
		line_counter++;
	}
	Trace((stdout,"encode_uuencode():finished while!\n"));
	
	if(out_pos >= out_len - 3) {
		Trace((stderr,"encode_uuencode():buffer is not enough\n"));
		goto err;
	}

	/* output "`" and "end..." lines */
	if( (flag & (1<<29) ) != 0 )	{
		/* buffer is enough */
		if(out_pos <= out_len - 6) {
			sprintf(pout_str+out_pos,"`%send%s",penter,penter);
			out_pos += 4+2*enter_len;
		/* buffer is not enough */
		}else{
			Trace((stderr,"encode_uuencode():buffer is not enough\n"));
			goto err;
		}
	}
	
	parg->m_len_out  = out_pos;
	value = RETURN_SUCCESS;
err:
	if (penter != NULL) {
		free(penter);
	}
	return value;

}
	



/*
 * Function :	decode_uuencode(struct conv_arg*)
 * Purpose  :	A uuencode decoder.	
 * Arguments:	struct conv_arg *parg 	=> look conv_char.h for explanation
 * Return   :	int	
 *				RETURN_ERROR    =>  failure
 *				RETURN_SUCCESS  =>  success 
 */
int decode_uuencode(struct conv_arg* parg)
{
	int 		n, value = RETURN_ERROR;
	int			line_ch_num;
	u_bit_8		 ch; 
	u_bit_8		*ptemp = NULL,	*pin_str, *pout_str;
	u_bit_8		penter[3] = {0,0,0};
	u_bit_32	in_pos=0, out_pos=0;
	u_bit_32	in_len=0, out_len=0;
	u_bit_32    enter_len, flag;
	unsigned int	mode;

	if(parg == NULL ) {
		Trace((stderr,"decode_uuencode(): input parg is NULL\n"));
		goto err;
	}

    in_len  = parg->m_len_in;
	out_len = parg->m_len_out;
    pin_str = parg->m_pstr_in;	
	pout_str = parg->m_pstr_out;

	/* Jump the spaces and enters in the beginning. */
	while(IS_SPACE(*(pin_str+in_pos) ) ) {
		in_pos++;
	}

	/* search the first enter ,"\r\n",or '\n' */
	n = strcspn(pin_str+in_pos,"\r\n");

	/* enter is "\r\n". */
	if (*(pin_str+in_pos+n) == '\r') {
		Trace((stdout,"decode_uuencode(): enter is \"\\r\\n\"\n"));
		enter_len = 2;
		memcpy(penter,"\r\n\0",3);
		flag = 0;		//set bit 31 : 0 (in input string,enter is "\r\n")
	/* enter is "\n". */
	}else if(*(pin_str+in_pos+n) == '\n') {
		Trace((stderr,"decode_uuencode(): enter is \'\\n\'\n"));
		enter_len = 1;
		memcpy(penter,"\n\0",2);
		flag = (1 << 31);	//set bit 31 : 1 (in input string,enter is "\n")
	}
	Trace((stdout,"decode_uuencode():first line's length is %d!\n",n));

	/* get the first line. */
	ptemp = (u_bit_8 *)calloc(n,sizeof(u_bit_8));
	memcpy(ptemp, pin_str+in_pos, n);

	/* search the "begin " line. */
	strstr(ptemp,"begin ");
	
	/* input string without "begin" line. */
	if(strstr(ptemp,"begin ") == NULL) {
		Trace((stderr,"Not find \"begin\"\n"));
		flag &= 0xBFFFFFFF;	
						//set bit 30 : 0 (input string hasn't "begin" line)
		if((n-1)/4 != (DEC(*(pin_str+in_pos)) + 2)/3) {
			Trace((stderr,"the first line (%d characters) will be cut:\n",n)); 
			Trace((stderr,"\ttoo long, or not integrate!\n"));
			in_pos += (n+enter_len);
		}
		free(ptemp);
	/* input string has a "begin" line. */
	}else{					
		Trace((stderr,"decode_uuencode():Find \"begin \" line\n"));
		free(ptemp);
		flag |= (1<<30);	//set bit 30 : 1 (input string has "begin" line)
		ptemp = strstr(pin_str+in_pos,"begin ");
		in_pos = (ptemp - pin_str);
		in_pos += 6;

		/* search the mode number. */
		sscanf(pin_str+in_pos,"%o",&mode);;
		if( mode<0111 || mode>0777 ) {
			Trace((stderr,"mode value is  err\n"));
			goto err;
		}
		flag |= mode;
		in_pos += 4;

		/* search the first enter ,"\r\n",or '\n'; */
		ptemp = strstr(pin_str+in_pos, penter);
		n = (ptemp - pin_str) - in_pos;

		/* file name is too long,err. */
		if(n>MAX_NAME_LENGTH || n<1 ){
			Trace((stderr,"file name is too long\n"));
			goto err;
		}
		// get the file name.
/*		if((parg->m_pfile_name=(u_bit_8 *)calloc(n+1,sizeof(u_bit_8)))==NULL) {
			Trace((stderr,"decode_uuencode():malloc file name err!\n"));
			goto err;
		}
*/
		if(parg->m_pfile_name == NULL) {
			Trace((stderr,"encode_uuencode():m_pfilename pointer is NULL\n"));
		}
		memcpy(parg->m_pfile_name,pin_str+in_pos,n);
		in_pos += n;
		in_pos += enter_len;
	}

	Trace((stderr,"in_pos is %d,char is %d\n",in_pos,*(pin_str+in_pos)));

	//now is the main content.
	while( in_pos < in_len && out_pos < out_len - 2) {

		if(memcmp(pin_str+in_pos,penter,enter_len) == 0){
			Trace((stderr,"meet unexpected enter, break!\n"));
			in_pos+=enter_len;
			break;
		}
		// character's number of current line.
		line_ch_num = DEC(*(pin_str+in_pos));	
		in_pos += 1;
	
		//get the "`" line,end.
		if(line_ch_num == 0) {
			in_pos += enter_len;
			break;
		}

//		out_len += line_ch_num;

		while( line_ch_num > 0 )	{
			ch = DEC(pin_str[in_pos]) << 2 
					| DEC(pin_str[in_pos+1]) >> 4;
			*(pout_str + out_pos) = ch;

			ch = DEC(pin_str[in_pos+1]) << 4 
					| DEC(pin_str[in_pos+2]) >> 2;
			*(pout_str + out_pos + 1) = ch;

			ch = DEC(pin_str[in_pos+2]) << 6 
					| DEC(pin_str[in_pos+3]);
			*(pout_str + out_pos + 2) = ch;

			out_pos += 3;
			line_ch_num -= 3;
			in_pos += 4;

		}
		in_pos += enter_len;
		if(line_ch_num < 0 )	{
			Trace((stdout,"decode_uuencode(): line_ch_num < 0 !\n"));
			out_pos += line_ch_num;
		}
	}
	
	if(memcmp(pin_str+in_pos,"end",3) == 0) {
		Trace((stderr,"find \"end\" line!\n"));
		flag |= 0x2FFFFFFF;	//set bit 29 : 1 (input string has "end" line) 
		in_pos += 3;
		in_pos += enter_len;
		if(memcmp(pin_str+in_pos,penter,enter_len)!=0) {
			in_pos -= enter_len;
			Trace((stderr,"decode_uuencode():no enter after \"end\"!\n"));
		}
	}else{
		Trace((stderr,"not find \"end\"!\n"));
		flag &= 0xDFFFFFFF;  //set bit 29 : 0 (input string hasn't "end" line)
	}
	

	parg->m_len_out = out_pos;
	parg->m_flag = flag;
	value = RETURN_SUCCESS;
err:
	if (ptemp != NULL) {
		free(ptemp);
	}
	return value;


}

