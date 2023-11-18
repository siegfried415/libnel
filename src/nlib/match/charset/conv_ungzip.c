/*
 * $Header: /home/cvsroot/ns210501/baseline/src/official/linux/user/engine/src/public/lib_charset/conv_ungzip.c,v 1.1.1.1 2005/12/27 06:40:38 zhaozh Exp $
 */

/*
 * conv_ungzip.c: doing gzip/ungzip work. 
 * 
 * shiyan
 * 2002.7.25
 */

#include "ungzip.h"

DECLARE(u_bit_8, inbuf_ungzip,  INBUFSIZ +INBUF_EXTRA);
DECLARE(u_bit_8, outbuf_ungzip, OUTBUFSIZ+OUTBUF_EXTRA);
DECLARE(u_bit_16, d_buf_ungzip,  DIST_BUFSIZE);
DECLARE(u_bit_8, window_ungzip, 2L*WSIZE);
DECLARE(u_bit_16, tab_prefix_ungzip, 1L<<BITS);

		/* local variables */

int method_ungzip = DEFLATED;/* compression method */
int level_ungzip = 6;        /* compression level */
int exit_code_ungzip = OK;   /* program exit code */
int save_orig_name_ungzip=0;   /* set if original name must be saved */
int part_nb_ungzip;          /* number of parts in .gz file */
long time_stamp_ungzip;      /* original time stamp (modification time) */

long bytes_in_ungzip;             /* number of input bytes */
long bytes_out_ungzip;            /* number of output bytes */
long total_in_ungzip = 0;         /* input bytes for all files */
long total_out_ungzip = 0;        /* output bytes for all files */
struct stat istat_ungzip;         /* status for input file */
unsigned insize_ungzip;           /* valid bytes in inbuf */
unsigned inptr_ungzip; 		/* index of next byte to be processed in inbuf */
unsigned outcnt_ungzip;		/* bytes in output buffer */

u_bit_8 *pin_str_ungzip, *pin_end_ungzip;
u_bit_8 *pout_str_ungzip, *pout_end_ungzip, *file_name_ungzip;
long bytes_all_ungzip;		/* all bytes that have been malloc */
long out_len_ungzip,in_len_ungzip;
static u_bit_32 crc_ungzip;       /* crc on uncompressed file data */
long header_bytes_ungzip;   /* number of bytes in gzip header */

pthread_mutex_t		mutex_ungzip;	


/* 
 * Function :   init_ungzip()
 * Purpose  :   Doing the initial works.
 * Arguments:   none
 * Return   :   none
 */
void init_ungzip()
{
	/* Allocate all global buffers (for DYN_ALLOC option) */

	ALLOC(u_bit_8, inbuf_ungzip,  INBUFSIZ +INBUF_EXTRA);
	ALLOC(u_bit_8, outbuf_ungzip, OUTBUFSIZ+OUTBUF_EXTRA);
	ALLOC(u_bit_16, d_buf_ungzip,  DIST_BUFSIZE);
	ALLOC(u_bit_8, window_ungzip, 2L*WSIZE);
	ALLOC(u_bit_16, tab_prefix_ungzip, 1L<<BITS);

	clear_bufs_ungzip(); /* clear input and output buffers */
	part_nb_ungzip = 0;
}

/*
 * Function :   do_exit()
 * Purpose  :   Free all dynamically allocated variables 
 * 						and exit with the given code.
 * Arguments:   int exitcode -- the code that used for exit().
 * Return   :   none
 */
void do_exit_ungzip()
{
	static int in_exit = 0;
	if (in_exit){ 
		return;
	}
	in_exit = 1;
	FREE(inbuf_ungzip);
	FREE(outbuf_ungzip);
	FREE(d_buf_ungzip);
	FREE(window_ungzip);
	FREE(tab_prefix_ungzip);
	return;
}


/*
 * Function :   get_method()
 * Purpose  :   used for unzip.Parse the header of the zip content.
 * Arguments:   none.
 * Return   :   int method_code -- compression method, 
 *									-1 for error, 
 *									-2 for warning.
 */
local int get_method()
{
	u_bit_8		flags;		/* compression flags */
	u_bit_8 	magic[2];	/* magic header */
	u_bit_32 	stamp;		/* time stamp */
	
	Trace((stdout,"get_method(): enter! \n"));

	magic[0] = (u_bit_8)get_byte();
	magic[1] = (u_bit_8)get_byte();

	method_ungzip = -1;                 /* unknown yet */
	part_nb_ungzip++;                   /* number of parts in gzip file */
	header_bytes_ungzip = 0;
    /* assume multiple members in gzip file except for record oriented I/O */

	if (memcmp(magic, GZIP_MAGIC, 2) == 0
		|| memcmp(magic, OLD_GZIP_MAGIC, 2) == 0) {

		method_ungzip = (int)get_byte();
		if (method_ungzip != DEFLATED) {
			exit_code_ungzip = ERROR;
			return -1;
		}
	
		flags  = (u_bit_8)get_byte();

		if ((flags & ENCRYPTED) != 0) {
			exit_code_ungzip = ERROR;
			return -1;
		}
		if ((flags & CONTINUATION) != 0) {
			exit_code_ungzip = ERROR;
			return -1;
		}
		if ((flags & RESERVED) != 0) {
			exit_code_ungzip = ERROR;
			return -1;
		}
		stamp  = (u_bit_32)get_byte();
		stamp |= ((u_bit_32)get_byte()) << 8;
		stamp |= ((u_bit_32)get_byte()) << 16;
		stamp |= ((u_bit_32)get_byte()) << 24;
		time_stamp_ungzip = stamp;

		(void)get_byte();  /* Ignore extra flags for the moment */
		(void)get_byte();  /* Ignore OS type for the moment */

		if ((flags & CONTINUATION) != 0) {
			unsigned part = (unsigned)get_byte();
			part |= ((unsigned)get_byte())<<8;
		}
		if ((flags & EXTRA_FIELD) != 0) {
			unsigned len = (unsigned)get_byte();
			len |= ((unsigned)get_byte())<<8;
			while (len--) {
				(void)get_byte();
			}
		}

		/* Get original file name if it was truncated */
		if ((flags & ORIG_NAME) != 0) {
			char c; 	// dummy used for NeXTstep 3.0 cc optimizer bug 
			int i=0;

			file_name_ungzip = (u_bit_8*)malloc(sizeof(u_bit_8)*128);
			do {
				c=get_byte();
				if(i>128) {
					Trace((stderr,"file name is too long:%d\n",i));
				}
				*(file_name_ungzip+i) = c;
				i++;
			} while (c != 0);
			file_name_ungzip = (u_bit_8 *)realloc(file_name_ungzip,
							sizeof(u_bit_8)*i);
		} /* if -- ORIG_NAME */

		/* Discard file comment if any */
		if ((flags & COMMENT) != 0) {
			while (get_byte() != 0);  // null 
		}
		if (part_nb_ungzip == 1) {
			header_bytes_ungzip = inptr_ungzip 
					+ 2*sizeof(long); // include crc and size 
		}
		if (method_ungzip >= 0) {
			return method_ungzip;
		}

		if (part_nb_ungzip == 1) {
			exit_code_ungzip = ERROR;
			return -1;
		} else {
			Trace((stderr,"get_method(): decode OK,but warning,part_nb=%d!\n",
					part_nb_ungzip));
			return -2;
		}
	}
	return -2;
}

/*
 * Function :	do_ungzip()
 * Purpose  :	doing the unzip work.
 * Arguments:	none
 * Return   :	RETURN_SUCCESS -- finished unzip work.	
 */
int do_ungzip() 
{
	u_bit_32 orig_crc = 0;       /* original crc */
	u_bit_32 orig_len = 0;       /* original uncompressed length */
	u_bit_8 buf[EXTHDR];        /* extended local header */
	int n;

	Trace((stdout,"do_ungzip(): enter!\n"));

	updcrc_ungzip(NULL, 0);           /* initialize crc */


	/* Decompress */
	if (method_ungzip == DEFLATED)  {
		int res = inflate();
		if (res == 3) {
			Trace((stderr,"do_unzip(): out of memory\n"));
			return RETURN_ERROR;
		} else if (res != 0) {
			Trace((stderr,"do_unzip():invalid compressed data\n"));
			return RETURN_ERROR;
		}
	}


	Trace((stdout,"do_unzip(): finished inflate()\n"));
	/* 
	 * Get the crc and original length 
	 * uncompressed input size modulo 2^32
	 */
	for (n = 0; n < 8; n++) {
		buf[n] = (u_bit_8)get_byte(); /* may cause an error if EOF */
	}
	Trace((stdout,"do_unzip(): get %d bytes in the end!\n",n));

	orig_crc = LG(buf);
	orig_len = LG(buf+4);

	/* Validate decompression */
	if (orig_crc != updcrc_ungzip(outbuf_ungzip, 0)) {
		Trace((stderr,"do_unzip(): invalid compressed data--crc error\n"));
		return RETURN_ERROR;
	}
	if (orig_len != (u_bit_32)bytes_out_ungzip) {
		Trace((stderr,"do_unzip(): invalid compressed data--length error\n"));
		return RETURN_ERROR;
		
	}
	Trace((stdout,"do_unzip() finished OK!\n"));
	return RETURN_SUCCESS;
}

/*
 * Function :	ungzip(struct conv_arg*)
 * Purpose  :	ungzip the input string.
 * Arguments:	struct conv_arg* parg -- look conv_char.h for explanation.
 * Return	:	RETURN_SUCCESS	: ungzip OK.
 *				RETURN_ERROR	: ungzip failure.
 */
int ungzip(struct conv_arg *parg) 
{
	int value=RETURN_ERROR;

	/* lock currect thread */
	pthread_mutex_lock(&mutex_ungzip);
	
	Trace((stdout,"ungzip(): enter \n"));

	pin_str_ungzip = parg->m_pstr_in;
	pin_end_ungzip = pin_str_ungzip + parg->m_len_in;

	pout_str_ungzip = parg->m_pstr_out;
	pout_end_ungzip = pout_str_ungzip;
	out_len_ungzip = parg->m_len_out;
	bytes_all_ungzip = parg->m_len_in;

	memset(pout_str_ungzip,'\0',out_len_ungzip);

	init_ungzip();

	method_ungzip = get_method();

	if (method_ungzip < 0) {
		Trace((stderr,"ungzip(): method < 0 !\n"));
		goto err;             /* error message already emitted */
	}
	/* 
	 * Actually do the compression/decompression. Loop over zipped members.
	 */
	for (;;) {
		if (do_ungzip() != RETURN_SUCCESS) {
			Trace((stderr,"ungzip(): do_unzip() return ERROR!\n"));
			/* force cleanup */
			method_ungzip = -1; 
			break;
		}
		/* end of file */
		if ( inptr_ungzip == insize_ungzip){
			 break;
		}
		method_ungzip = get_method();
		/* error message already emitted */
		if (method_ungzip < 0) {
			Trace((stderr,"ungzip():  method = %d\n",method_ungzip));
			break;    
		}
		/* required for length check */
		bytes_out_ungzip = 0;
		
	}

	parg->m_len_out = pout_end_ungzip - pout_str_ungzip;
	value = RETURN_SUCCESS;

err:
	if(value == RETURN_ERROR) {
		Trace((stderr,"gzip(): return error!\n"));
	}
	do_exit_ungzip();

	/* unlocd current thread */
	pthread_mutex_unlock(&mutex_ungzip);

	return value;

}


    
/*
 * Function :	buf_read(char*, unsigned )
 * Purpose  :	Read a new buffer from the current input , perform end-of-line
 *					translation, and update the crc and input file size.
 *					IN assertion: size >= 2 (for end-of-line translation)
 * Arguments:	char* buf 		-- the buffer's address.
 *				unsigned size 	-- the length that will be read. 
 * Return   :   int 	--	the real length that have been read.
 */
int buf_read_ungzip(char* buf, unsigned size)
{
	unsigned len;

	len = MIN( size, (pin_end_ungzip - pin_str_ungzip) );

	if (len == 0) {
		return (int)len;
	}
	
	memcpy( (char*)buf, pin_str_ungzip, len);
	crc_ungzip = updcrc_ungzip((u_bit_8*)buf, len);

	isize_ungzip += (unsigned long)len;
	pin_str_ungzip += len;
	
	return (int)len;
}
