/*
 * $Header: /home/cvsroot/ns210501/baseline/src/official/linux/user/engine/src/public/lib_charset/main.c,v 1.1.1.1 2005/12/27 06:40:39 zhaozh Exp $
 */

/*
 * conv_main.c
 *
 * shiyan
 *
 * 2002.5.20
 */

#include "conv_char.h"

#define BUFFER_LEN 18192



int main(int argc, char **argv)
{
   
	FILE    *fp_in;
	int 	str_len;
	char    *pstr_input;
	int     i, input_type=-1, output_type=-1, j;
	struct conv_arg arg;

	memset(&arg,0,sizeof(arg));

	if(argc != 4) {
		printf("Please enter: %s [INPUT TYPE] [OUTPUT TYPE] XXXX <enter>\n",argv[0]);
		printf("\tXXXX: file name or string\n");
		exit(1);
	}

	if( (fp_in = fopen(argv[3],"r")) == NULL) { //input is a sting
		str_len = strlen(argv[3]);
		pstr_input = (char*)malloc(sizeof(char)*str_len);
		memcpy(pstr_input,argv[3],str_len);
	} else{                 //input is a file name
		fseek(fp_in,0,SEEK_END);
		str_len = ftell(fp_in);
		pstr_input = (char*)malloc(sizeof(char)*str_len);
		fseek(fp_in,0,SEEK_SET);    //put the pointer to the begin.
		fread(pstr_input,sizeof(char),str_len,fp_in);
	}
	arg.m_len_in 	= str_len;
	arg.m_pstr_in	= pstr_input;
	arg.m_flag 		= 0777;
//	arg.m_pstr_out  = (u_bit_8*)malloc(sizeof(u_bit_8) * 20);	
//	arg.m_len_out 	= 20;
	arg.m_len_out 	= BUFFER_LEN;
	arg.m_pstr_out  = (u_bit_8*)malloc(sizeof(u_bit_8) * BUFFER_LEN);	

	i=0;   
	while(all_type[i].m_pname!=NULL) {
		if(strcmp(argv[1],all_type[i].m_pname)==0) {
			input_type = all_type[i].m_type;
		}
		if(strcmp(argv[2],all_type[i].m_pname)==0) {
			output_type = all_type[i].m_type;
		}
		i++;
	}
	if(input_type ==-1 || output_type == -1) {
		printf("conv type is err!\n");
		exit(1);
	}

	Trace((stdout,"begin conversion!\n"));
 
	i=0;
	while((j=all_fun[i].m_coding_type_in) >= 0) {
		if(j == input_type && all_fun[i].m_coding_type_out == output_type) {
			arg.m_pfile_name = all_fun[i].m_pfile_name;
//for(j=0;j<1000;j++) {
			if((*all_fun[i].m_pfun)(&arg)==RETURN_SUCCESS) {
				Trace((stderr,"success!\n"));
				fprintf (stderr,"arg.m_len_in = %d\n",arg.m_len_in);
				fwrite(arg.m_pstr_out,sizeof(char),arg.m_len_out,stdout);
			}else{
				Trace((stderr,"CONVERSION ERROR!\n"));
				exit(1);
			}
//}
		}
		i++;
	}
	exit(0);
}

