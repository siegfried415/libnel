
/*****************************************************************************/
/* This file, "nel.lex.h", contains declarations for the routines, any   */
/* #define statements, and extern declarations for any global variables that */
/* are defined in "nel.l".                                           */
/*****************************************************************************/



#ifndef LEX_H
#define LEX_H



struct nel_eng;


/****************************************************************************/
/* the codes for the tokens passed between the lexical analyzer and the     */
/* parser.  all tokens must agree with those defined in "nel.y".  */
/* "nel.y" is run through yacc (well, bison), to form the parser  */
/* "nel.c".  the %token statements in nel.y become      */
/* #define statements in "nel.c" which contain exactly one tab    */
/* between the #define and the macro body.  since "nel.c" also    */
/* #includes "nel.lex.h", in this file we must put exactly one tab      */
/* between the macro name and body in this file, to prevent macro redefined */
/* warning messages from being emitted.  (too bad cpp doesn't remove        */
/* leading whitespace from macros.)                                         */
/****************************************************************************/



/**************/
/* C keywords */
/**************/

#define nel_T_ASM		1
#define nel_T_AUTO		2
#define nel_T_BREAK		3
#define nel_T_CASE		4
#define nel_T_CHAR		5
#define nel_T_COMPLEX	6
#define nel_T_CONST		7
#define nel_T_CONTINUE	8
#define nel_T_DEFAULT	9
#define nel_T_DO		10
#define nel_T_DOUBLE	11
#define nel_T_ELSE		12
#define nel_T_ENUM		13
#define nel_T_EXTERN	14
#define nel_T_FLOAT		15
#define nel_T_FOR		16
#define nel_T_FORTRAN	17
//#define nel_T_GOTO		18	//modified by zhangbin
#define nel_T_IF		19
#define nel_T_INT		20
#define nel_T_LONG		21
#define nel_T_REGISTER	22
#define nel_T_RETURN	23
#define nel_T_SHORT		24
#define nel_T_SIGNED	25
#define nel_T_SIZEOF	26
#define nel_T_STATIC	27
#define nel_T_STRUCT	28
#define nel_T_SWITCH	29
#define nel_T_TYPEDEC	30
#define nel_T_TYPEDEF	31
#define nel_T_TYPEOF	32
#define nel_T_UNION		33
#define nel_T_UNSIGNED	34
#define nel_T_VOID		35
#define nel_T_VOLATILE	36
#define nel_T_WHILE		37



/*************************/
/* preprocessor keywords */
/*************************/

#define nel_T_P_DEFINE	38
#define nel_T_P_ELIF	39
#define nel_T_P_ELSE	40
#define nel_T_P_ENDIF	41
#define nel_T_P_ERROR	42
#define nel_T_P_IF		43
#define nel_T_P_IFDEF	44
#define nel_T_P_IFNDEF	45
#define nel_T_P_INCLUDE	46
#define nel_T_P_LINE	47
#define nel_T_P_PRAGMA	48
#define nel_T_P_POUNDS	49
#define nel_T_P_UNDEF	50



/********************/
/* operator symbols */
/********************/

#define nel_T_AMPER		51
#define nel_T_AMPEREQ	52
#define nel_T_BAR		53
#define nel_T_BAREQ		54
#define nel_T_COLON		55
#define nel_T_COMMA		56
#define nel_T_DAMPER	57
#define nel_T_DBAR		58
#define nel_T_DEQ		59
#define nel_T_DLARROW	60
#define nel_T_DLARROWEQ	61
#define nel_T_DMINUS	62
#define nel_T_DOT		63
#define nel_T_DPLUS		64
#define nel_T_DRARROW	65
#define nel_T_DRARROWEQ	66
#define nel_T_ELLIPSIS	67
#define nel_T_EQ		68
#define nel_T_EXCLAM	69
#define nel_T_EXCLAMEQ	70
#define nel_T_LARROW	71
#define nel_T_LARROWEQ	72
#define nel_T_LBRACE	73
#define nel_T_LBRACK	74
#define nel_T_LPAREN	75
#define nel_T_MINUS		76
#define nel_T_MINUSRARROW	77
#define nel_T_MINUSEQ	78
#define nel_T_PERCENT	79
#define nel_T_PERCENTEQ	80
#define nel_T_PLUS		81
#define nel_T_PLUSEQ	82
#define nel_T_QUESTION	83
#define nel_T_RARROW	84
#define nel_T_RARROWEQ	85
#define nel_T_RBRACE	86
#define nel_T_RBRACK	87
#define nel_T_RPAREN	88
#define nel_T_SEMI		89
#define nel_T_SLASH		90
#define nel_T_SLASHEQ	91
#define nel_T_STAR		92
#define nel_T_STAREQ	93
#define nel_T_TILDE		94
#define nel_T_UPARROW	95
#define nel_T_UPARROWEQ	96



/*************/
/* constants */
/*************/

#define nel_T_C_CHAR	97
#define nel_T_C_DOUBLE	98
#define nel_T_C_FLOAT	99
#define nel_T_C_LONG_DOUBLE	100
#define nel_T_C_SIGNED_INT	101
#define nel_T_C_SIGNED_LONG_INT	102
#define nel_T_C_STRING	103
#define nel_T_C_UNSIGNED_INT	104
#define nel_T_C_UNSIGNED_LONG_INT	105



/*********************/
/* identifier tokens */
/*********************/

#define nel_T_IDENT		106
#define nel_T_TYPEDEF_NAME	107

/*
 * new keywords for production declaration
 */
#define nel_T_ATOM		108
#define nel_T_EVENT		109
#define nel_T_INIT		110
#define nel_T_MAIN		114
#define nel_T_EXIT		115
#define nel_T_FINI		116
#define nel_T_NODELAY		111


/*
 * some data types for production 
 */
#define nel_T_TIMEOUT	112
#define nel_T_DOLLAR_IDENT		113

#define nel_T_REGEXP			117
#define nel_T_REGEXT			118
#define nel_T_EXCLAM_TILDE		119

#define nel_T_TILDEEQ			120
/*********************************************/
/* finally, nel_T_NULL and nel_T_MAX */
/*********************************************/

#define nel_T_NULL		0
#define nel_T_MAX		121




/***********************************************/
/* input buffer sizes for the lexical analyzer */
/***********************************************/
#define nel_YY_READ_BUF_SIZE	2048
#define nel_YY_BUF_SIZE		(nel_YY_READ_BUF_SIZE * 2)


#if 0
/*********************************************/
/* state buffer for the lexical analyzer     */
/*********************************************/
struct yy_buffer_state
{
        FILE *input_file;


        /* Whether this is an "interactive" input source; if so, and
         * if we're using stdio for input, then we want to use getc()
         * instead of fread(), to make sure we stop fetching input after
         * each newline.
         */
        int yy_is_interactive;

        char *ch_buf;		/* input buffer				*/
        char *buf_pos;		/* current position in yyinput buffer	*/
        int buf_size;		/* size of input buffer in bytes,	*/
        /* not including room for EOB characters*/
        int n_chars;		/* number of characters read into	*/
        /* yy_ch_buf, not including EOB chars	*/
        int eof_status;		/* whether we've seen an EOF on this buffer */
};

#endif


struct eng_lexier {
	/************************************************/
	/* vars for the lexical analyzer nel_parser_lex () */
	/************************************************/
	char *lexptr;
	char *lexptr_begin;
	char *lexend;
	//int   curline;
	//int   lasttok;
	char *tok;
	char *tokend;
	char *buf;
	int  want_assign;
	int  want_regexp;	/* wyong, 2006.6.23 */


	FILE *infile;
	//struct yy_buffer_state *current_buffer;
	//char hold_char;
	int last_char;
	//int n_chars;
	char *text;
	//int leng;
	//char *c_buf_p;
	int init;
	//int start;
	//int did_buffer_switch_on_eof;
	//int last_accepting_state;
	//char *last_accepting_cpos;
	//int more_flag;
	//int doing_more;
	//int more_len;
	/*****************************************************************/
	/* vars returning values from nel_parser_lex () to nel_parse () */
	/*****************************************************************/
	char char_lval;
	double double_lval;
	float float_lval;
	int int_lval;
	long_int long_int_lval;
	long_double long_double_lval;
	unsigned_int unsigned_int_lval;
	unsigned_long_int unsigned_long_int_lval;
};


/**********************************************************/
/* declarations for the routines defined in nel.l */
/**********************************************************/
extern int nel_lex (struct nel_eng *);
extern void nel_lex_init (struct nel_eng *);
extern char *nel_token_string (register int);
extern void nel_save_lex_context(struct nel_eng *eng);
extern void nel_restore_lex_context(struct nel_eng *eng);
extern void nel_parser_lex_init(struct nel_eng *_eng, FILE *_infile);
extern void nel_parser_lex_dealloc(struct nel_eng *_eng);





#endif /* LEX_H */

