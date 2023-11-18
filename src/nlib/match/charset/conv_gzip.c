/*
 * $Header: /home/cvsroot/ns210501/baseline/src/official/linux/user/engine/src/public/lib_charset/conv_gzip.c,v 1.1.1.1 2005/12/27 06:40:38 zhaozh Exp $
 */

/*
 * conv_gzip.c: doing gzip/ungzip work. 
 * 
 * shiyan
 * 2002.6.4
 */

#include "gzip.h"

DECLARE(u_bit_8, inbuf_gzip,  INBUFSIZ +INBUF_EXTRA);
DECLARE(u_bit_8, outbuf_gzip, OUTBUFSIZ+OUTBUF_EXTRA);
DECLARE(u_bit_16, d_buf_gzip,  DIST_BUFSIZE);
DECLARE(u_bit_8, window_gzip, 2L*WSIZE);
DECLARE(u_bit_16, tab_prefix_gzip, 1L<<BITS);

		/* local variables */

int ascii_gzip = 0;        /* convert end-of-lines to local OS conventions */
int method_gzip = DEFLATED;/* compression method */
int level_gzip = 6;        /* compression level */
int exit_code_gzip = OK;   /* program exit code */
int save_orig_name_gzip=0;   /* set if original name must be saved */
int part_nb_gzip;          /* number of parts in .gz file */
long time_stamp_gzip;      /* original time stamp (modification time) */

long bytes_in_gzip;			/* number of input bytes */
long bytes_out_gzip;		/* number of output bytes */
long total_in_gzip = 0;		/* input bytes for all files */
long total_out_gzip = 0;	/* output bytes for all files */
struct stat istat_gzip;		/* status for input file */
unsigned insize_gzip;		/* valid bytes in inbuf */
unsigned inptr_gzip;		/* index of next byte to be processed in inbuf */
unsigned outcnt_gzip;		/* bytes in output buffer */

u_bit_8 *pin_str_gzip, *pin_end_gzip;
u_bit_8 *pout_str_gzip, *pout_end_gzip, *file_name_gzip;
long bytes_all_gzip;		/* all bytes that have been malloc */
long out_len_gzip,in_len_gzip;
static u_bit_32 crc_gzip;       /* crc on uncompressed file data */
long header_bytes_gzip;   /* number of bytes in gzip header */

pthread_mutex_t		mutex_gzip;

/* 
 * Function :   init_gzip()
 * Purpose  :   Doing the initial works.
 * Arguments:   none
 * Return   :   none
 */
void init_gzip()
{
	/* Allocate all global buffers (for DYN_ALLOC option) */

	ALLOC(u_bit_8, inbuf_gzip,  INBUFSIZ +INBUF_EXTRA);
	ALLOC(u_bit_8, outbuf_gzip, OUTBUFSIZ+OUTBUF_EXTRA);
	ALLOC(u_bit_16, d_buf_gzip,  DIST_BUFSIZE);
	ALLOC(u_bit_8, window_gzip, 2L*WSIZE);
	ALLOC(u_bit_16, tab_prefix_gzip, 1L<<BITS);

	clear_bufs_gzip(); /* clear input and output buffers */
	part_nb_gzip = 0;
}

/*
 * Function :   do_exit()
 * Purpose  :   Free all dynamically allocated variables 
 * 						and exit with the given code.
 * Arguments:   int exitcode -- the code that used for exit().
 * Return   :   none
 */
void do_exit_gzip()
{
	static int in_exit_gzip = 0;
	if (in_exit_gzip){ 
		return;
	}
	in_exit_gzip = 1;
	FREE(inbuf_gzip);
	FREE(outbuf_gzip);
	FREE(d_buf_gzip);
	FREE(window_gzip);
	FREE(tab_prefix_gzip);
	return;
}


/*
 * Function :	gzip(struct conv_arg*)
 * Purpose  :	gzip the input string.
 * Arguments:	struct conv_arg* parg -- look conv_char.h for explanation.
 * Return	:	RETURN_SUCCESS  : gzip OK.
 *				RETURN_ERROR    : gzip failure.
 */
int gzip(struct conv_arg *parg)
{
	u_bit_8  flags = 0;         /* general purpose bit flags */
	u_bit_16  attr = 0;          /* ascii/binary flag */
	u_bit_16  deflate_flags = 0; /* pkzip -es, -en or -ex equivalent */
	int value = RETURN_ERROR;

	Trace((stdout,"gzip(): enter!\n"));

	/* lock current thread */
	pthread_mutex_lock(&mutex_gzip);
	
	pin_str_gzip = parg->m_pstr_in;
	pin_end_gzip = pin_str_gzip + parg->m_len_in;
	pout_str_gzip = parg->m_pstr_out;
	pout_end_gzip = pout_str_gzip;
	out_len_gzip = parg->m_len_out;
	bytes_all_gzip = parg->m_len_in;
	
	memset(pout_str_gzip,'\0',out_len_gzip);
	
	Trace((stdout,"gzip(): now will beginnig init_gzip()!\n"));
	init_gzip();
	Trace((stdout,"gzip(): finished init zip!\n"));

	outcnt_gzip = 0;

	/* Write the header to the gzip file. See algorithm.doc for the format */
	method_gzip = DEFLATED;
	put_byte(GZIP_MAGIC[0]);		/* magic header */
	put_byte(GZIP_MAGIC[1]);
	put_byte(DEFLATED);				/* compression method */

	put_byte(flags);				/* general flags */
	put_long(time_stamp_gzip);

	/* Write deflated file to zip file */
	crc_gzip = updcrc_gzip(0, 0);

	bi_init();
	ct_init(&attr, &method_gzip);
	if(lm_init(level_gzip, &deflate_flags) == RETURN_ERROR) {
		Trace((stderr,"lm_init error!\n"));
		goto err;
	}

	put_byte((u_bit_8)deflate_flags); 	/* extra flags */
	put_byte((u_bit_8)OS_TYPE);			/* OS identifier */

	if (save_orig_name_gzip) {
		char *p = parg->m_pfile_name; 
		do {
			put_byte(*p);
		} while (*p++);
	}
	header_bytes_gzip = (long)outcnt_gzip;

	Trace((stdout,"gzip(): now going into deflate(),doing gzip work!\n"));
	if(deflate() == RETURN_ERROR) {
		Trace((stderr,"gzip(): deflate return error!\n"));
	}
	Trace((stdout,"gzip(): finished doing deflate()\n"));

	/* Write the crc and uncompressed size */
	put_long(crc_gzip);
	put_long(isize_gzip);
	header_bytes_gzip += 2*sizeof(long);
	Trace((stdout,"gzip():finished output crc and isize!\n"));

	flush_outbuf_gzip();
	Trace((stdout,"gzip():finished flush_outbuf()!\n"));	
	
	parg->m_len_out = pout_end_gzip - pout_str_gzip;
	value = RETURN_SUCCESS;
	Trace((stdout,"gzip():return success!\n"));
err:
	if(value == RETURN_ERROR) {
		Trace((stderr,"gzip(): return error!\n"));
	}
	do_exit_gzip();

	/* unlock current thread */
	pthread_mutex_unlock(&mutex_gzip);

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
int buf_read_gzip(char* buf, unsigned size)
{
	unsigned len;

	len = MIN( size, (pin_end_gzip - pin_str_gzip) );

	if (len == 0) {
		return (int)len;
	}
	
	memcpy( (char*)buf, pin_str_gzip, len);
	crc_gzip = updcrc_gzip((u_bit_8*)buf, len);

	isize_gzip += (unsigned long)len;
	pin_str_gzip += len;
	
	return (int)len;
}
