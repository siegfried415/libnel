
#ifndef __CONV_CHAR_H__
#define	__CONV_CHAR_H__

//#include	<string.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<malloc.h>
#include	<ctype.h>
#include	<pthread.h>
#undef DEBUG
//#define DEBUG



#define CHARSET_MODE_GB2312             (1<<CHARSET_GB2312)
#define CHARSET_MODE_BIG5               (1<<CHARSET_BIG5)
#define CHARSET_MODE_UNICODE    (1<<CHARSET_UNICODE)
#define CHARSET_MODE_UTF8               (1<<UTF_8)


#if 0	 

	/* charset and encoding type's ID */

	#define		CHARSET_UNSURE		-1	// UNSURE CHARSET 
	#define		ENCODE_TYPE_UNSURE	-1	// UNSURE DECODE TYPE
	#define		CHARSET_GB2312		0	// character set	: GB2312
	#define		CHARSET_BIG5		1	// character set	: BIG5
	#define		CHARSET_UNICODE		2	// character set	: UNICODE
	#define		UTF_8		3		// encode type		: utf-8
	#define		BASE64		4		// encode type		: base64
	#define		QP		5		// encode type		: quoted-printable
	#define		UUENCODE	6		// encode type		: uuencode
	#define		HZ		7		// encode type		: HZ
	#define		GZIP		8		// encode type		: gzip
	#define		ORIGINAL	9  		// I define it ,for original state.
	#define		CHARSET_SJIS	10		// CHARSET SJIS
#else
	/* for nel cannot see macro, which is concep of C language preprocessor,
		  but can see enum */
	enum charset_type {

		CHARSET_UNSURE = -1,	// UNSURE CHARSET 

		CHARSET_GB2312 = 0,	// character set : GB2312
		CHARSET_BIG5 = 1,	// character set : BIG5
		CHARSET_UNICODE = 2,	// character set : UNICODE
		CHARSET_SJIS = 10,	// character set : SJIS
	};

	enum encode_type {

		ENCODE_TYPE_UNSURE = -1,	// UNSURE DECODE TYPE

		UTF_8 = 3,		// encode type : utf-8
		BASE64 = 4,		// encode type : base64
		QP = 5,			// encode type : quoted-printable
		UUENCODE = 6,		// encode type : uuencode
		HZ = 7,			// encode type : HZ
		GZIP = 8,		// encode type : gzip
		ORIGINAL = 9,		// I define it, for original state.
	};
#endif

/* function's return value */
#define RETURN_SUCCESS		0
#define RETURN_FAILURE		-1
#define RETURN_ERR_INPUT	-2
#define RETURN_ERROR		-3

typedef unsigned char 	u_bit_8;
typedef	unsigned short	u_bit_16;
typedef	unsigned int	u_bit_32;

struct coding_type {
	int		m_type;					// charset and encoding type's ID 
	char*	m_pname;				// charset and encoding type's name
};

struct conv_fun {
	int		m_coding_type_in;	
	int 	m_coding_type_out;		
	int 	(*m_pfun)();			
	char* 	m_pfile_name;			
};

struct conv_arg{
	u_bit_8*	m_pstr_in;		
	u_bit_8*	m_pstr_out;	
	u_bit_32	m_len_in;
	u_bit_32	m_len_out;
	u_bit_8*	m_pfile_name;
	u_bit_32	m_flag;		
};

/*
struct conv_tab{
	u_bit_16 m_src
	u_bit_16 m_dst;
};
*/

// convert function :
int decode_base64(struct conv_arg*);
int encode_base64(struct conv_arg*);
int decode_uuencode(struct conv_arg*);
int encode_uuencode(struct conv_arg*);
int decode_qp(struct conv_arg* arg);
int encode_qp(struct conv_arg* arg);
int decode_hz(struct conv_arg* arg);
int encode_hz(struct conv_arg* arg);
int decode_utf8(struct conv_arg* arg);
int encode_utf8(struct conv_arg* arg);
int ungzip(struct conv_arg* arg);
int gzip(struct conv_arg* arg);
int g2b(struct conv_arg* arg);
int b2g(struct conv_arg* arg);
int g2u(struct conv_arg* arg);
int u2g(struct conv_arg* arg);
int b2u(struct conv_arg* arg);
int u2b(struct conv_arg* arg);

extern struct coding_type all_charset_type[];
extern struct coding_type all_encoding_type[];
extern struct conv_fun all_fun[];

extern	u_bit_16* g2b_page[];
extern	u_bit_16* g2u_page[];
extern	u_bit_16* u2b_page[];
extern	u_bit_16* u2g_page[];
extern	u_bit_16* b2g_page[];
extern	u_bit_16* b2u_page[];
/* Diagnostic functions */
#ifdef DEBUG
#  define Assert(cond,msg) {if(!(cond)) Trace((stderr,msg));}
#  define Trace(x) fprintf x
#  define Tracev(x) {if (verbose) fprintf x ;}
#  define Tracevv(x) {if (verbose>1) fprintf x ;}
#  define Tracec(c,x) {if (verbose && (c)) fprintf x ;}
#  define Tracecv(c,x) {if (verbose>1 && (c)) fprintf x ;}
#else
#  define Assert(cond,msg)
#  define Trace(x)
#  define Tracev(x)
#  define Tracevv(x)
#  define Tracec(c,x)
#  define Tracecv(c,x)
#endif

#endif
