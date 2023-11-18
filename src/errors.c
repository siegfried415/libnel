
/*****************************************************************************/
/* This file, "nel_errors.c", contains the error handling routines for the    */
/* application executive.                                                    */
/*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include <setjmp.h>
#include <stdarg.h>
#include <type.h>

#include "engine.h"
#include "type.h"
#include "errors.h"
#include "intp.h"
#include "lex.h"
#include "io.h"

/******************************************/
/* use nel_error_lock to create a critical */
/* section of code where error messages.  */
/******************************************/
nel_lock_type nel_error_lock = nel_unlocked_state;

/*************************************************************/
/* when the flag nel_save_stmt_err_jmp is set, then when the  */
/* interpreter saves its state in (eng->xxx->stmt_err_jmp)    */
/* prior to evaluating a statement, it also sets             */
/* nel_stmt_err_jmp to point to the jump buffer.         */
/*************************************************************/
unsigned_int nel_save_stmt_err_jmp = 0;
nel_jmp_buf *nel_stmt_err_jmp = NULL;



/***************************************************/
/* when nel_no_error_exit is true, we return when a */
/* fatal error is encountered, instead of exiting. */
/***************************************************/
unsigned_int nel_no_error_exit = 1;



/*****************************************************************************/
/* the error handling routines.  they print a message to stderr, increments  */
/* eng->xxx->error_ct (except nel_diagnostic () and nel_warning ()), then    */
/* longjmps to the approptiate stack frame (except nel_diagnostic (),        */
/* nel_warning (), and nel_error ()).   If message is NULL, then the error   */
/* handler is called  to perform the longjmp only, so no error message is    */
/* printed and eng->xxx->error_ct is not incremented.  they are declared     */
/* as int () so that they may be used in comma expressions with ints.        */
/*****************************************************************************/




/*****************************************************************************/
/* nel_trace behaves like nel_printf (), but only when the "nel_tracing" flag*/
/* is set.                                                                   */
/*****************************************************************************/
void nel_trace (struct nel_eng *eng, char *message, ...)
{
	if (eng && eng->verbose_level >= NEL_TRACE_LEVEL ) {
		va_list args;
		va_start (args, message);
		nel_do_print (stderr, message, args);
		va_end (args);
	}	
}


/*****************************************************************************/
/* nel_diagnostic () prints out a diagnostic message to the screen, unless    */
/* the global flag "nel_silent" is set".  It does not print out the current   */
/* input file name and line number, a do the other error handling routines.  */
/*****************************************************************************/
int nel_diagnostic (struct nel_eng *eng, char *message, ...)
{
	//char buffer[4096];
	va_list args;

	if ( eng && eng->verbose_level >= NEL_DIAG_LEVEL 
		 &&   (message != NULL)) {

		va_start (args, message);

		/*******************************************/
		/* error message i/o is a critical section */
		/*******************************************/
		nel_lock (&nel_error_lock);


		//sprintf(buffer, message, args);
		//write(2, buffer, strlen(buffer));

		nel_do_print (stderr, message, args);
		//fprintf (stderr, "\n");

		/*************************/
		/* exit critical section */
		/*************************/
		nel_unlock (&nel_error_lock);
		va_end (args);
	}

	return (0);
}


/*****************************************************************************/
/* nel_overwritable () prints out a diagnostic message to the screen, (unless */
/* the global flag "nel_silent" is set), just like nel_diagnostic, but no      */
/* newline follows the error message, and the message is backspaced over the */
/* next time a message comes to the same file.  the message should not       */
/* contain a newline, or we cannot backspace over the message.  the message  */
/* should contain at most extra one integral argument (%[dio], but not %f    */
/* nor any of nel's special print formats %[A-Z]).                            */
/*****************************************************************************/
int nel_overwritable (struct nel_eng *eng, char *message, int arg1)
{
	if ( eng && eng->verbose_level >= NEL_DIAG_LEVEL
		&& (message != NULL)) {
		char buffer[80];
		int len;

		/*******************************************/
		/* error message i/o is a critical section */
		/*******************************************/
		nel_lock (&nel_error_lock);

		sprintf (buffer, message, arg1);
		len = strlen (buffer);
		fprintf (stderr, "%s", buffer);

		/*******************************************************/
		/* nel_do_print will automatically print the backspaces */
		/* if we set nel_[te]f_backspaces accordingly.          */
		/*******************************************************/
		nel_stderr_backspaces = len;

		/*******************************************************************/
		/* don't print to trace file - stderr is usually the screen,        */
		/* but stdout is usually a file - don't clutter it with backspaces. */
		/*******************************************************************/

		/*************************/
		/* exit critical section */
		/*************************/
		nel_unlock (&nel_error_lock);

	}

	return (0);
}


/*****************************************************************************/
/* nel_warning () prints out an error message, together with the current      */
/* input filename and line number, and then returns without incrementing     */
/* eng->xxx->error_ct.                                                        */
/*****************************************************************************/
int nel_warning (struct nel_eng *eng, int type, char *filename, int line, char *message, /*...*/ va_list args)
{
	//va_list args;

	if ( eng->verbose_level >= NEL_WARN_LEVEL &&  message != NULL) {
		//va_start (args, message);

		/*******************************************/
		/* error message i/o is a critical section */
		/*******************************************/
		nel_lock (&nel_error_lock);

		if ((eng != NULL) && (type != nel_R_NULL)) {
			fprintf (stderr, "\"%s\", line %d: warning: ", filename, line);
		}
		else {
			fprintf (stderr, "warning: ");
		}

		nel_do_print (stderr, message, args);
		fprintf (stderr, "\n");

		/*************************/
		/* exit critical section */
		/*************************/
		nel_unlock (&nel_error_lock);

		//va_end (args);
	}
	return (0);
}



/*****************************************************************************/
/* nel_error () prints out an error message, together with the current input  */
/* filename and line number, increments eng->xxx->error_ct, and returns.      */
/*****************************************************************************/
int nel_error (struct nel_eng *eng, int type, char *filename, int line, char *message, /*...*/ va_list args)
{
	//va_list args;
	
	if ( eng && eng->verbose_level >= NEL_ERROR_LEVEL &&  message != NULL) {
		//va_start (args, message);

		/*******************************************/
		/* error message i/o is a critical section */
		/*******************************************/
		nel_lock (&nel_error_lock);

		if ((eng != NULL) && (type!= nel_R_NULL) && (filename != NULL)){
			fprintf (stderr, "\"%s\", line %d: ", filename, line);
		}
		nel_do_print (stderr, message, args);
		fprintf (stderr, "\n");

		/******************************************/
		/* exit critical section                  */
		/******************************************/
		nel_unlock (&nel_error_lock);
		//va_end (args);
	}

	return (0);
}


/*****************************************************************************/
/* nel_stmt_error () prints out an error message, increments                  */
/* eng->xxx->error_ct, and nel_longjmps() back to the parser, which continues */
/* on to the next statement.  when scanning the stab strings, the stmt error */
/* jump is set to to continue with the next string.                          */
/*****************************************************************************/
int nel_stmt_error (struct nel_eng *eng, int type, char *filename, int line, struct nel_JMP_BUF *stmt_err_jmp, char *message, /*...*/ va_list args)
{
	//va_list args;

	if ( eng && eng->verbose_level >= NEL_ERROR_LEVEL && message != NULL) {
		//va_start (args, message); 

		/*******************************************/
		/* error message i/o is a critical section */
		/*******************************************/
		nel_lock (&nel_error_lock);

		if ((eng != NULL) && (type!= nel_R_NULL) && (filename!= NULL)){
			fprintf (stderr, "\"%s\", line %d: ", filename, line);
		}
		nel_do_print (stderr, message, args);
		fprintf (stderr, "\n");

		/*****************************************/
		/* exit critical section 		 */
		/*****************************************/
		nel_unlock (&nel_error_lock);

		//va_end (args);
	}

	/*******************************************************/
	/* now longjmp back to the parser, which will continue */
	/* with the next statement (or the next symbol table   */
	/* string), if the stmt_err_jmp is set.                */
	/*******************************************************/
	if ((eng != NULL) && (type != nel_R_NULL) && (stmt_err_jmp != NULL)) {
		nel_longjmp (eng, *stmt_err_jmp, 1);
	}
	else {
		nel_fatal_error (eng, type, filename, line, stmt_err_jmp, 
			"(nel_stmt_error #1): stmt error jump not set",NULL);
	}

	return (0);
}


/*****************************************************************************/
/* nel_block_error () prints out an error message, increments                 */
/* eng->xxx->error_ct, and nel_longjmps() back to the parser, which continues */
/* after the end of the current block.  when scanning the stab strings, the  */
/* block error jump is set to abandon scanning and return.                   */
/*****************************************************************************/
int nel_block_error (struct nel_eng *eng, int type, char *filename, int line, struct nel_JMP_BUF *block_err_jmp, char *message, /*...*/ va_list args)
{
	//va_list args;

	if ( eng && eng->verbose_level >= NEL_ERROR_LEVEL &&  message != NULL) {
		//va_start (args, message);

		/*******************************************/
		/* error message i/o is a critical section */
		/*******************************************/
		nel_lock (&nel_error_lock);

		if ((eng != NULL) && (type!= nel_R_NULL) && (filename!= NULL)){
			fprintf (stderr, "\"%s\", line %d: ", filename, line);
		}
		nel_do_print (stderr, message, args);
		fprintf (stderr, "\n");

		/******************************************/
		/* exit critical section 	          */
		/******************************************/
		nel_unlock (&nel_error_lock);

		//va_end (args);
	}

	/********************************************************/
	/* if the flag nel_save_stmt_err_jmp is true, then reset */
	/* nel_stmt_err_jmp to NULL, as the block error longjmp  */
	/* could pop the semantic stack below the point where   */
	/* the stmt error jump was saved.                       */
	/********************************************************/
	if (nel_save_stmt_err_jmp) {
		nel_stmt_err_jmp = NULL;
	}

	/*******************************************************/
	/* now longjmp back to the parser, which will continue */
	/* after the current block (or abandon scanning the    */
	/* symbol table), if the block_err_jmp is set.         */
	/*******************************************************/
	if ((eng != NULL) && (type != nel_R_NULL) && (block_err_jmp != NULL)) {
		nel_longjmp (eng, *block_err_jmp, 1);
	}
	else {
		nel_fatal_error (eng,type, filename, line, block_err_jmp, 
			"(nel_block_error #1): block error jmp not set", NULL);
	}

	return (0);
}


/*****************************************************************************/
/* nel_fatal_error () prints out an error message, increments                 */
/* eng->xxx->error_ct, and exits the program.  it is called when an internal  */
/* interpretrer error is encountered.  if the global flag "nel_no_error_exit" */
/* is set, then nel_fatal_error () nel_longjmps back to the top-level routine, */
/* which returns to the caller.                                              */
/*****************************************************************************/
int nel_fatal_error (struct nel_eng *eng, int type, char *filename, int line, struct nel_JMP_BUF *return_pt,  char *message, /*...*/ va_list args)
{
	//va_list args;

	if ( eng && eng->verbose_level >= NEL_FATAL_LEVEL && message != NULL) {
		//va_start (args, message);

		if ((eng != NULL) && (type!= nel_R_NULL) && (filename!= NULL)){
			fprintf (stderr, "\"%s\", line %d: ", filename, line);
		}
		nel_do_print (stderr, message, args);
		fprintf (stderr, "\n");

		//va_end (args);
	}

	/********************************************/
	/* if the flag nel_save_stmt_err_jmp is true */
	/* then reset nel_stmt_err_jmp to NULL.      */
	/********************************************/
	if (nel_save_stmt_err_jmp) {
		nel_stmt_err_jmp = NULL;
	}

	/***********************************************/
	/* we have a fatal error, so exit the program, */
	/* unless nel_no_error_exit is set.             */
	/***********************************************/
	if ((nel_no_error_exit) && (eng != NULL) && (type != nel_R_NULL) && (return_pt != NULL)) {
		/**************************************/
		/* exit the critical before returning */
		/* to the top-level routine.          */
		/**************************************/
		nel_unlock (&nel_error_lock);
		nel_longjmp (eng, *return_pt, 1);
	}
	else {
		/********************************************/
		/* print an "exiting" message, exit the     */
		/* critical section, then exit the program. */
		/********************************************/
		fprintf (stderr, "exiting\n");

		nel_unlock (&nel_error_lock);
		exit (-1 );
	}

	return (0);

}

