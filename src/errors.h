
/*****************************************************************************/
/* This file, "nel_errors.h", contains declarations for the routines, any     */
/* #define statements, and extern declarations for any global variables that */
/* are defined in "nel_errors.c".                                             */
/*****************************************************************************/



#ifndef _NEL_ERRORS_
#define _NEL_ERRORS_


#include "type.h"
union nei_eng;
//enum nel_R_token;

/******************************************/
/* use nel_error_lock to create a critical */
/* section of code where error messages.  */
/******************************************/
extern nel_lock_type nel_error_lock;


/****************************************************************************/
/* define a structure for nel_longjmp () to use when performing a nonlocal   */
/* exit.  we may need to restore the state of the parser or stab       */
/* parser when a non-fatal error is encountered.  use #define statements to */
/* save the state of the parser and perform the setjmp, and to restore the  */
/* parser state after longjmp.  we also #define a regular version of setjmp */
/* that is not called from within a parser.  if we try to call a subroutine */
/* to perform the setjmp, and then continue, the stack frame pointed to in  */
/* the jmp_buf is overwritten.                                              */
/****************************************************************************/
typedef struct nel_JMP_BUF
{
        jmp_buf buf;
        struct nel_JMP_BUF *stmt_err_jmp;
        struct nel_JMP_BUF *block_err_jmp;
        union nel_STACK *semantic_stack_next;
        int level;
        int block_no;
	unsigned_int clean_flag : 1;
        char *dyn_values_next;
        struct nel_SYMBOL *dyn_symbols_next;
        union nel_STACK *context;
        int state;
        int n;
        short *ssp;
        int *vsp;
        short *ss;
        int *vs;
        int len;
}
nel_jmp_buf;




/************************************************************/
/* for nel_longjmp (), just call _longjmp () on the jmp_buf. */
/************************************************************/
#define nel_longjmp(_eng,_buffer,_val)	longjmp ((_buffer).buf, (_val))



/*************************************************************/
/* when the flag nel_save_stmt_err_jmp is set, then when the  */
/* interpreter saves its state in (eng->xxx->stmt_err_jmp)    */
/* prior to evaluating a statement, it also sets             */
/* nel_stmt_err_jmp to point to the jump buffer.         */
/*************************************************************/
extern unsigned_int nel_save_stmt_err_jmp;
extern struct nel_JMP_BUF *nel_stmt_err_jmp;



/***************************************************/
/* when nel_no_error_exit is true, we return when a */
/* fatal error is encountered, instead of exiting. */
/***************************************************/
extern unsigned_int nel_no_error_exit;



/***********************************************************/
/* declarations for the routines defined in "nel_errors.c". */
/***********************************************************/
extern void nel_fprintf (FILE *, char *, ...);
extern void nel_printf (char *, ...);
extern void nel_trace(struct nel_eng *, char *, ...);
extern int nel_diagnostic (struct nel_eng *, char *, ...);
extern int nel_overwritable (struct nel_eng *, char *, int arg1);

extern int nel_warning(struct nel_eng *,int, char *, int, char *, va_list);
extern int nel_error (struct nel_eng *, int, char *, int, char *, va_list); 
extern int nel_stmt_error (struct nel_eng *, int, char *, int, struct nel_JMP_BUF *, char *message, va_list );
extern int nel_block_error (struct nel_eng *, int, char *, int, struct nel_JMP_BUF *, char *, va_list );
extern int nel_fatal_error (struct nel_eng *, int, char *, int, struct nel_JMP_BUF *,  char *, va_list );


#endif /* _NEL_ERRORS_ */

