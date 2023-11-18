
/*****************************************************************************/
/* This file, "nel_intp_eval.c", contains the parse tree evaluator for the    */
/* application executive.                                                    */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include <setjmp.h>
#include <string.h>

#include "engine.h"
#include "errors.h"
#include "io.h"
#include "type.h"
#include "expr.h"
#include "stmt.h"
#include "intp.h"
#include "lex.h"
#include "mem.h"

//added by zhangbin, 2006-5-18
#include <math.h>
#define INTP_ZERO 1e-7

//wyong, 20230803 
//inline 
int check_intp_zero(struct nel_eng *eng, nel_type *type, char *right)
{
	if(!type || !right)
		return 0;
	
	switch(type->simple.type)
	{
		case nel_D_CHAR:
		case nel_D_SIGNED_CHAR:
		case nel_D_UNSIGNED_CHAR:
			return *right=='\0' ? 1 : 0;
			break;
		case nel_D_SHORT:
		case nel_D_SHORT_INT:
		case nel_D_SIGNED_SHORT:
		case nel_D_SIGNED_SHORT_INT:
			return *(short*)right==0 ? 1 : 0;
			break;
		case nel_D_UNSIGNED_SHORT:
		case nel_D_UNSIGNED_SHORT_INT:
			return *(unsigned short*)right==0 ? 1 : 0;
			break;
		case nel_D_INT:
		case nel_D_SIGNED_INT:
		case nel_D_SIGNED:
			return *(int*)right==0 ? 1 : 0;
			break;
		case nel_D_UNSIGNED:
		case nel_D_UNSIGNED_INT:
			return *(unsigned int*)right==0 ? 1 : 0;
			break;
		case nel_D_LONG:
		case nel_D_SIGNED_LONG:
		case nel_D_LONG_INT:
		case nel_D_SIGNED_LONG_INT:
			return *(long*)right==0 ? 1 : 0;
			break;
		case nel_D_UNSIGNED_LONG:
		case nel_D_UNSIGNED_LONG_INT:
			return *(unsigned long*)right==0 ? 1 : 0;
			break;
		case nel_D_FLOAT:
			return fabsf(*(float*)right)<INTP_ZERO ? 1 : 0;
			break;
		case nel_D_DOUBLE:
			return fabs(*(double*)right)<INTP_ZERO ? 1 : 0;
			break;
		case nel_D_LONG_DOUBLE:
			return fabsl(*(long double*)right)<INTP_ZERO ? 1 : 0;
			break;
		default:
			return 0;
	}
}
//end

void intp_trace (struct nel_eng *eng, char *message, ...)
{
        if (eng != NULL && eng->intp_verbose_level >= NEL_TRACE_LEVEL ) {
                va_list args;
                va_start (args, message);

		/*******************************************/
		/* error message i/o is a critical section */
		/*******************************************/
		nel_lock (&nel_error_lock);

                nel_do_print (stderr, message, args);

		/*************************/
		/* exit critical section */
		/*************************/
		nel_unlock (&nel_error_lock);

                va_end (args);
        }
}


void intp_diagnostic(struct nel_eng *eng, char *message, ...)
{
	char buffer[4096];
	va_list args;

	if ( eng && (eng->intp_verbose_level >= NEL_DIAG_LEVEL )) {
		va_start (args, message);

		/*******************************************/
		/* error message i/o is a critical section */
		/*******************************************/
		nel_lock (&nel_error_lock);

		sprintf(buffer, message, args);
		write(2, buffer, strlen(buffer));
		fprintf (stderr, "\n");

		//nel_do_print (stdout, message, args);

		/*************************/
		/* exit critical section */
		/*************************/
		nel_unlock (&nel_error_lock);
		va_end (args);
	}

	return ;
}

/*****************************************************************************/
/* intp_warning () prints out an error message, together with the current  */
/* input filename and line number, and then returns without incrementing     */
/* eng->intp->error_ct.                                                    */
/*****************************************************************************/
int intp_warning (struct nel_eng *eng, char *message, ...)
{
	va_list args;

	if (eng && (eng->intp_verbose_level >= NEL_WARN_LEVEL ) ) {
		va_start (args, message);

		/*******************************************/
		/* error message i/o is a critical section */
		/*******************************************/
		nel_lock (&nel_error_lock);

		if (eng->intp->type != nel_R_NULL) {
			fprintf (stderr, "\"%s\", line %d: warning: ", eng->intp->filename, eng->intp->line);
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

		va_end (args);
	}

	return (0);

}



/*****************************************************************************/
/* intp_error()prints out an error message, together with the current input*/
/* filename and line number, increments eng->intp->error_ct, and returns.  */
/*****************************************************************************/
int intp_error (struct nel_eng *eng, char *message, ...)
{
	va_list args;

	if ( eng && (eng->intp_verbose_level >= NEL_ERROR_LEVEL )) {
		va_start (args, message);

		/*******************************************/
		/* error message i/o is a critical section */
		/*******************************************/
		nel_lock (&nel_error_lock);

		if ((eng != NULL) 
			&& (eng->intp->type!= nel_R_NULL) 
			&& (eng->intp->filename != NULL)){
			fprintf (stderr, "\"%s\", line %d: ", eng->intp->filename, eng->intp->line);
		}
		nel_do_print (stderr, message, args);
		fprintf (stderr, "\n");

		/******************************************/
		/* exit critical section                  */
		/******************************************/
		nel_unlock (&nel_error_lock);
		va_end (args);
	}

	return (0);

}

/*****************************************************************************/
/* intp_stmt_error() prints out an error message, increments               */
/* eng->intp->error_ct, and nel_longjmps() back to the parser,	     */
/* which continues on to the next statement. 			             */
/*****************************************************************************/
int intp_stmt_error (struct nel_eng *eng, char *message, ...)
{
	va_list args;

	/* output error message first */
	if(eng != NULL && eng->intp_verbose_level >= NEL_ERROR_LEVEL ){
		va_start (args, message); 

		if ((eng->intp->type!= nel_R_NULL) && (eng->intp->filename!= NULL)){
			fprintf (stderr, "\"%s\", line %d: ", eng->intp->filename, eng->intp->line);
		}
                nel_do_print (stderr, message, args);
		fprintf (stderr, "\n");
		va_end (args);
	}

	eng->intp->error_ct++;

	/*******************************************************/
	/* now longjmp back to the parser, which will continue */
	/* with the next statement (or the next symbol table   */
	/* string), if the stmt_err_jmp is set.                */
	/*******************************************************/
	if ((eng != NULL) 
		&& (eng->intp->type != nel_R_NULL) 
		&& (eng->intp->stmt_err_jmp != NULL)) {
		nel_longjmp (eng, *(eng->intp->stmt_err_jmp), 1);
	}
	else {
		intp_fatal_error (eng, "(intp_stmt_error #1): stmt error jump not set",NULL);
	}

	
	return (0);
}

/*****************************************************************************/
/* intp_block_error () prints out an error message, increments eng->intp */
/* ->error_ct, and nel_longjmps() back to the parser, which continues */
/* after the end of the current block.  when scanning the stab strings, the  */
/* block error jump is set to abandon scanning and return.                   */
/*****************************************************************************/
int intp_block_error (struct nel_eng *eng, char *message, ...)
{
	
	va_list args;

	/* output error message first */
	if(eng != NULL && eng->intp_verbose_level >= NEL_ERROR_LEVEL ){
		va_start (args, message); 
		if ((eng->intp->type!= nel_R_NULL) && (eng->intp->filename!= NULL)){
			fprintf (stderr, "\"%s\", line %d: ", eng->intp->filename, eng->intp->line);
		}
                nel_do_print (stderr, message, args);
		fprintf (stderr, "\n");
		va_end (args);
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
        if ((eng != NULL) 
		&& ( eng->intp->type != nel_R_NULL) 
		&& ( eng->intp->block_err_jmp != NULL)) {
                nel_longjmp (eng, *(eng->intp->block_err_jmp), 1);
        }
        else {
		intp_fatal_error(eng, "(intp_block_error #1): block error jmp not set", NULL);

        }


	eng->intp->error_ct++;
	return (0);
}

/*****************************************************************************/
/* intp_fatal_error() prints out an error message, increments              */
/* eng->intp->error_ct, and exits the program.  it is called	*/ 
/* when an internal parser error is encountered.  if the global */ 
/* flag "nel_no_error_exit" is set, then intp_fatal_error()  */
/* nel_longjmps back to the top-level routine,  which returns  		*/
/* to the caller.                                              */
/*****************************************************************************/
int intp_fatal_error (struct nel_eng *eng, char *message, ...)
{
	va_list args;

	if ( eng != NULL &&  eng->intp_verbose_level >= NEL_FATAL_LEVEL ) {
		va_start (args, message);
		if ( (eng->intp->filename != NULL)){
			fprintf (stderr, "\"%s\", line %d: ", eng->intp->filename, eng->intp->line);
		}

		nel_do_print (stderr, message, args);
		fprintf (stderr, "\n");

		va_end (args);
	}

	eng->intp->error_ct++;

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
	if ((nel_no_error_exit) 
		&& (eng != NULL) 
		&& (eng->intp->type != nel_R_NULL) 
		&& (eng->intp->return_pt != NULL)) {
		/**************************************/
		/* exit the critical before returning */
		/* to the top-level routine.          */
		/**************************************/
		nel_unlock (&nel_error_lock);
		nel_longjmp (eng, *(eng->intp->return_pt), 1);
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

void intp_insert_ident(struct nel_eng *eng, nel_symbol *symbol)
{
	if((symbol->level <=0) 
		&& (symbol->class != nel_C_STATIC_GLOBAL) 
		&& (symbol->class != nel_C_COMPILED_STATIC_FUNCTION)
		&& (symbol->class != nel_C_NEL_STATIC_FUNCTION))  
		nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);
	else 
		nel_insert_symbol (eng, symbol, eng->intp->dyn_ident_hash);
	
}

void intp_remove_ident(struct nel_eng *eng,  nel_symbol *symbol)
{
	nel_remove_symbol(eng, symbol);
}

nel_symbol *intp_lookup_ident(struct nel_eng *eng, char *name)
{
	register nel_symbol *symbol = NULL;	

	/*bugfix, wyong, 2005.10.26 */
	symbol = nel_lookup_symbol(name, eng->intp->dyn_ident_hash,
			   eng->nel_static_ident_hash,
			   eng->intp->dyn_location_hash,
			   eng->nel_static_location_hash, NULL);
	if(symbol == NULL){
		symbol = (nel_symbol *)lookup_event_symbol(eng, name);
	}

	return symbol;

}


/*****************************************************************************/
/* intp_coerce coerces a data object of type <old_type> pointed to by      */
/* <old_value>, and coerces it to type <new_type>, storing the result in     */
/* <new_value>.                                                              */
/*****************************************************************************/
void intp_coerce (struct nel_eng *eng, nel_type *new_type, char *new_value, nel_type *old_type, char *old_value)
{
	register nel_D_token old_D_token;
	register nel_D_token new_D_token;
	int retval;

	nel_debug ({ intp_trace (eng, "entering intp_coerce [\neng = 0x%x\nnew_type = \n%1Tnew_value = 0x%x\nold_type =\n%1Told_value = 0x%x\n\n", eng, new_type, new_value, old_type, old_value); });

	nel_debug ({
		if ((new_type == NULL) || (new_value == NULL) || (old_type == NULL) || (old_value == NULL)) {
			intp_fatal_error (eng, "(intp_coerce #1): bad arguments for intp_coerce\nnew_type = \n%1Tnew_value = 0x%x\nold_type = \n%1Told_value = 0x%x", new_type, new_value, old_type, old_value);
		}
	});

	if (nel_coerce(eng, new_type, new_value, old_type, old_value) < 0 ) {
		intp_stmt_error (eng, "illegal type coercion");
	}

	nel_debug ({ intp_trace (eng, "] exiting intp_coerce\n\n"); });
	return ;

}


/*****************************************************************************/
/* intp_apply (retval, type, func, args) invokes the routine func () on   */
/* an argument list.  The arguments should me placed in consecutive memory   */
/* locations in ascending order, starting at *(args+1).  *args should        */
/* contain the number of words that the rest of the arguments occupy.        */
/* The return value is placed in *retval, and type should be it's type       */
/* descriptor.  This routine contains asm()'s, so it should be compiled      */
/* without optimizations, except when the default case is used for unknown   */
/* architectures.                                                            */
/* intp_apply () applies the function <func> the the list of arguments,   */
/* taking up <arg_size> words, starting at <arg_ptr>.  <func> returns a data */
/* object of type <type> in the location pointed to by <retval>.             */
/*   struct nel_eng *eng;			nel_eng		*/
/*   char *retval;			return value is placed here	*/
/*   nel_type *type;			type descriptor for *retval	*/
/*   char (*func)();				function being called	*/
/*   int arg_size;				size of argument list	*/
/*   int *args;					argument list		*/
/*****************************************************************************/
void intp_apply (struct nel_eng  *eng, char *retval, nel_type *type, char (*func)(), int arg_size, int **args)
{
	register nel_D_token D_token;

	nel_debug ({ intp_trace (eng, "entering intp_apply () [\neng = 0x%x\nretval = 0%x\ntype =\n%1Tfunc = 0x%x\narg_size = %d\nargs = 0x%x\n\n", eng, retval, type, func, arg_size, args); });

	nel_debug ({
	if (type == NULL) {
		intp_fatal_error (eng, "(intp_apply #1): type == NULL") ;
	}
	});

	/********************************************/
	/* find the standardized equivalent D_token */
	/********************************************/
	D_token = nel_D_token_equiv (type->simple.type);

	switch (D_token) {
		/***********************************************/
		/* use calls generated by "intp_apply_gen". */
		/***********************************************/

	case nel_D_SIGNED_CHAR:
		switch (arg_size) {
		case 0:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) ();
			break;
		case 1:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0]);
			break;
		case 2:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1]);
			break;
		case 3:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1], args[2]);
			break;
		case 4:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1], args[2], args[3]);
			break;
		case 5:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4]);
			break;
		case 6:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5]);
			break;
		case 7:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
			break;
		case 8:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
			break;
		case 9:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
			break;
		case 10:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
			break;
		case 11:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10]);
			break;
		case 12:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11]);
			break;
		case 13:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12]);
			break;
		case 14:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13]);
			break;
		case 15:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14]);
			break;
		case 16:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15]);
			break;
		case 17:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15], args[16]);
			break;
		case 18:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17]);
			break;
		case 19:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18]);
			break;
		case 20:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19]);
			break;
		case 21:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											args[20]);
			break;
		case 22:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											args[20], args[21]);
			break;
		case 23:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											args[20], args[21], args[22]);
			break;
		case 24:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											args[20], args[21], args[22], args[23]);
			break;
		case 25:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											args[20], args[21], args[22], args[23], args[24]);
			break;
		case 26:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											args[20], args[21], args[22], args[23], args[24], args[25]);
			break;
		case 27:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											args[20], args[21], args[22], args[23], args[24], args[25], args[26]);
			break;
		case 28:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											args[20], args[21], args[22], args[23], args[24], args[25], args[26], args[27]);
			break;
		case 29:
			*((signed_char *) retval) = (*((signed_char (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											args[20], args[21], args[22], args[23], args[24], args[25], args[26], args[27], args[28]);
			break;
		default:
			intp_stmt_error (eng, "too many arguments (30 words max)");
		}
		break;
	case nel_D_DOUBLE:
		switch (arg_size) {
		case 0:
			*((double *) retval) = (*((double (*) ()) func)) ();
			break;
		case 1:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0]);
			break;
		case 2:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1]);
			break;
		case 3:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1], args[2]);
			break;
		case 4:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1], args[2], args[3]);
			break;
		case 5:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4]);
			break;
		case 6:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5]);
			break;
		case 7:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
			break;
		case 8:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
			break;
		case 9:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
			break;
		case 10:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
			break;
		case 11:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10]);
			break;
		case 12:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11]);
			break;
		case 13:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12]);
			break;
		case 14:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13]);
			break;
		case 15:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14]);
			break;
		case 16:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15]);
			break;
		case 17:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15], args[16]);
			break;
		case 18:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17]);
			break;
		case 19:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18]);
			break;
		case 20:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19]);
			break;
		case 21:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
									   args[20]);
			break;
		case 22:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
									   args[20], args[21]);
			break;
		case 23:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
									   args[20], args[21], args[22]);
			break;
		case 24:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
									   args[20], args[21], args[22], args[23]);
			break;
		case 25:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
									   args[20], args[21], args[22], args[23], args[24]);
			break;
		case 26:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
									   args[20], args[21], args[22], args[23], args[24], args[25]);
			break;
		case 27:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
									   args[20], args[21], args[22], args[23], args[24], args[25], args[26]);
			break;
		case 28:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
									   args[20], args[21], args[22], args[23], args[24], args[25], args[26], args[27]);
			break;
		case 29:
			*((double *) retval) = (*((double (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
									   args[20], args[21], args[22], args[23], args[24], args[25], args[26], args[27], args[28]);
			break;
		default:
			intp_stmt_error (eng, "too many arguments (30 words max)");
		}
		break;
	case nel_D_SIGNED_INT:
		switch (arg_size) {
		case 0:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) ();
			break;
		case 1:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0]);
			break;
		case 2:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1]);
			//wyong, 20230821 
			//*((int **)args), *((int **)args+1));
			break;
		case 3:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1], args[2]);
			break;
		case 4:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1], args[2], args[3]);
			break;
		case 5:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1], args[2], args[3], args[4]);
			break;
		case 6:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1], args[2], args[3], args[4], args[5]);
			break;
		case 7:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
			break;
		case 8:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
			break;
		case 9:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
			break;
		case 10:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
			break;
		case 11:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
										   args[10]);
			break;
		case 12:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
										   args[10], args[11]);
			break;
		case 13:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
										   args[10], args[11], args[12]);
			break;
		case 14:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
										   args[10], args[11], args[12], args[13]);
			break;
		case 15:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
										   args[10], args[11], args[12], args[13], args[14]);
			break;
		case 16:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
										   args[10], args[11], args[12], args[13], args[14], args[15]);
			break;
		case 17:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
										   args[10], args[11], args[12], args[13], args[14], args[15], args[16]);
			break;
		case 18:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
										   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17]);
			break;
		case 19:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
										   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18]);
			break;
		case 20:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
										   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19]);
			break;
		case 21:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
										   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
										   args[20]);
			break;
		case 22:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
										   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
										   args[20], args[21]);
			break;
		case 23:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
										   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
										   args[20], args[21], args[22]);
			break;
		case 24:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
										   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
										   args[20], args[21], args[22], args[23]);
			break;
		case 25:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
										   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
										   args[20], args[21], args[22], args[23], args[24]);
			break;
		case 26:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
										   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
										   args[20], args[21], args[22], args[23], args[24], args[25]);
			break;
		case 27:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
										   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
										   args[20], args[21], args[22], args[23], args[24], args[25], args[26]);
			break;
		case 28:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
										   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
										   args[20], args[21], args[22], args[23], args[24], args[25], args[26], args[27]);
			break;
		case 29:
			*((signed_int *) retval) = (*((signed_int (*) ()) func)) (
										   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
										   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
										   args[20], args[21], args[22], args[23], args[24], args[25], args[26], args[27], args[28]);
			break;
		default:
			intp_stmt_error (eng, "too many arguments (30 words max)");
		}
		break;
	case nel_D_FLOAT:
		switch (arg_size) {
		case 0:
			*((float *) retval) = (*((float (*) ()) func)) ();
			break;
		case 1:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0]);
			break;
		case 2:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1]);
			break;
		case 3:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1], args[2]);
			break;
		case 4:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1], args[2], args[3]);
			break;
		case 5:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1], args[2], args[3], args[4]);
			break;
		case 6:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1], args[2], args[3], args[4], args[5]);
			break;
		case 7:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
			break;
		case 8:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
			break;
		case 9:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
			break;
		case 10:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
			break;
		case 11:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									  args[10]);
			break;
		case 12:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									  args[10], args[11]);
			break;
		case 13:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									  args[10], args[11], args[12]);
			break;
		case 14:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									  args[10], args[11], args[12], args[13]);
			break;
		case 15:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									  args[10], args[11], args[12], args[13], args[14]);
			break;
		case 16:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									  args[10], args[11], args[12], args[13], args[14], args[15]);
			break;
		case 17:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									  args[10], args[11], args[12], args[13], args[14], args[15], args[16]);
			break;
		case 18:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									  args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17]);
			break;
		case 19:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									  args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18]);
			break;
		case 20:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									  args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19]);
			break;
		case 21:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									  args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
									  args[20]);
			break;
		case 22:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									  args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
									  args[20], args[21]);
			break;
		case 23:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									  args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
									  args[20], args[21], args[22]);
			break;
		case 24:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									  args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
									  args[20], args[21], args[22], args[23]);
			break;
		case 25:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									  args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
									  args[20], args[21], args[22], args[23], args[24]);
			break;
		case 26:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									  args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
									  args[20], args[21], args[22], args[23], args[24], args[25]);
			break;
		case 27:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									  args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
									  args[20], args[21], args[22], args[23], args[24], args[25], args[26]);
			break;
		case 28:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									  args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
									  args[20], args[21], args[22], args[23], args[24], args[25], args[26], args[27]);
			break;
		case 29:
			*((float *) retval) = (*((float (*) ()) func)) (
									  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									  args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
									  args[20], args[21], args[22], args[23], args[24], args[25], args[26], args[27], args[28]);
			break;
		default:
			intp_stmt_error (eng, "too many arguments (30 words max)");
		}
		break;
	case nel_D_LONG_DOUBLE:
		switch (arg_size) {
		case 0:
			*((long_double *) retval) = (*((long_double (*) ()) func)) ();
			break;
		case 1:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0]);
			break;
		case 2:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1]);
			break;
		case 3:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1], args[2]);
			break;
		case 4:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1], args[2], args[3]);
			break;
		case 5:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4]);
			break;
		case 6:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5]);
			break;
		case 7:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
			break;
		case 8:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
			break;
		case 9:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
			break;
		case 10:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
			break;
		case 11:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10]);
			break;
		case 12:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11]);
			break;
		case 13:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12]);
			break;
		case 14:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13]);
			break;
		case 15:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14]);
			break;
		case 16:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15]);
			break;
		case 17:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15], args[16]);
			break;
		case 18:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17]);
			break;
		case 19:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18]);
			break;
		case 20:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19]);
			break;
		case 21:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											args[20]);
			break;
		case 22:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											args[20], args[21]);
			break;
		case 23:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											args[20], args[21], args[22]);
			break;
		case 24:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											args[20], args[21], args[22], args[23]);
			break;
		case 25:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											args[20], args[21], args[22], args[23], args[24]);
			break;
		case 26:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											args[20], args[21], args[22], args[23], args[24], args[25]);
			break;
		case 27:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											args[20], args[21], args[22], args[23], args[24], args[25], args[26]);
			break;
		case 28:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											args[20], args[21], args[22], args[23], args[24], args[25], args[26], args[27]);
			break;
		case 29:
			*((long_double *) retval) = (*((long_double (*) ()) func)) (
											args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											args[20], args[21], args[22], args[23], args[24], args[25], args[26], args[27], args[28]);
			break;
		default:
			intp_stmt_error (eng, "too many arguments (30 words max)");
		}
		break;
	case nel_D_POINTER:
		switch (arg_size) {
		case 0:
			*((char * *) retval) = (*((char * (*) ()) func)) ();
			break;
		case 1:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0]);
			break;
		case 2:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1]);
			break;
		case 3:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1], args[2]);
			break;
		case 4:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1], args[2], args[3]);
			break;
		case 5:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4]);
			break;
		case 6:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5]);
			break;
		case 7:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
			break;
		case 8:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
			break;
		case 9:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
			break;
		case 10:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
			break;
		case 11:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10]);
			break;
		case 12:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11]);
			break;
		case 13:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12]);
			break;
		case 14:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13]);
			break;
		case 15:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14]);
			break;
		case 16:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15]);
			break;
		case 17:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15], args[16]);
			break;
		case 18:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17]);
			break;
		case 19:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18]);
			break;
		case 20:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19]);
			break;
		case 21:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
									   args[20]);
			break;
		case 22:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
									   args[20], args[21]);
			break;
		case 23:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
									   args[20], args[21], args[22]);
			break;
		case 24:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
									   args[20], args[21], args[22], args[23]);
			break;
		case 25:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
									   args[20], args[21], args[22], args[23], args[24]);
			break;
		case 26:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
									   args[20], args[21], args[22], args[23], args[24], args[25]);
			break;
		case 27:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
									   args[20], args[21], args[22], args[23], args[24], args[25], args[26]);
			break;
		case 28:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
									   args[20], args[21], args[22], args[23], args[24], args[25], args[26], args[27]);
			break;
		case 29:
			*((char * *) retval) = (*((char * (*) ()) func)) (
									   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
									   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
									   args[20], args[21], args[22], args[23], args[24], args[25], args[26], args[27], args[28]);
			break;
		default:
			intp_stmt_error (eng, "too many arguments (30 words max)");
		}
		break;
	case nel_D_SIGNED_SHORT_INT:
		switch (arg_size) {
		case 0:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) ();
			break;
		case 1:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0]);
			break;
		case 2:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1]);
			break;
		case 3:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1], args[2]);
			break;
		case 4:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1], args[2], args[3]);
			break;
		case 5:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1], args[2], args[3], args[4]);
			break;
		case 6:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1], args[2], args[3], args[4], args[5]);
			break;
		case 7:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
			break;
		case 8:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
			break;
		case 9:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
			break;
		case 10:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
			break;
		case 11:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												 args[10]);
			break;
		case 12:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												 args[10], args[11]);
			break;
		case 13:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												 args[10], args[11], args[12]);
			break;
		case 14:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												 args[10], args[11], args[12], args[13]);
			break;
		case 15:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												 args[10], args[11], args[12], args[13], args[14]);
			break;
		case 16:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												 args[10], args[11], args[12], args[13], args[14], args[15]);
			break;
		case 17:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												 args[10], args[11], args[12], args[13], args[14], args[15], args[16]);
			break;
		case 18:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												 args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17]);
			break;
		case 19:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												 args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18]);
			break;
		case 20:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												 args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19]);
			break;
		case 21:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												 args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
												 args[20]);
			break;
		case 22:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												 args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
												 args[20], args[21]);
			break;
		case 23:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												 args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
												 args[20], args[21], args[22]);
			break;
		case 24:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												 args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
												 args[20], args[21], args[22], args[23]);
			break;
		case 25:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												 args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
												 args[20], args[21], args[22], args[23], args[24]);
			break;
		case 26:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												 args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
												 args[20], args[21], args[22], args[23], args[24], args[25]);
			break;
		case 27:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												 args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
												 args[20], args[21], args[22], args[23], args[24], args[25], args[26]);
			break;
		case 28:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												 args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
												 args[20], args[21], args[22], args[23], args[24], args[25], args[26], args[27]);
			break;
		case 29:
			*((signed_short_int *) retval) = (*((signed_short_int (*) ()) func)) (
												 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												 args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
												 args[20], args[21], args[22], args[23], args[24], args[25], args[26], args[27], args[28]);
			break;
		default:
			intp_stmt_error (eng, "too many arguments (30 words max)");
		}
		break;
	case nel_D_UNSIGNED_INT:
		switch (arg_size) {
		case 0:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) ();
			break;
		case 1:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0]);
			break;
		case 2:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1]);
			break;
		case 3:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1], args[2]);
			break;
		case 4:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1], args[2], args[3]);
			break;
		case 5:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1], args[2], args[3], args[4]);
			break;
		case 6:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1], args[2], args[3], args[4], args[5]);
			break;
		case 7:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
			break;
		case 8:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
			break;
		case 9:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
			break;
		case 10:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
			break;
		case 11:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											 args[10]);
			break;
		case 12:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											 args[10], args[11]);
			break;
		case 13:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											 args[10], args[11], args[12]);
			break;
		case 14:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											 args[10], args[11], args[12], args[13]);
			break;
		case 15:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											 args[10], args[11], args[12], args[13], args[14]);
			break;
		case 16:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											 args[10], args[11], args[12], args[13], args[14], args[15]);
			break;
		case 17:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											 args[10], args[11], args[12], args[13], args[14], args[15], args[16]);
			break;
		case 18:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											 args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17]);
			break;
		case 19:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											 args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18]);
			break;
		case 20:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											 args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19]);
			break;
		case 21:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											 args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											 args[20]);
			break;
		case 22:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											 args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											 args[20], args[21]);
			break;
		case 23:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											 args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											 args[20], args[21], args[22]);
			break;
		case 24:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											 args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											 args[20], args[21], args[22], args[23]);
			break;
		case 25:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											 args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											 args[20], args[21], args[22], args[23], args[24]);
			break;
		case 26:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											 args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											 args[20], args[21], args[22], args[23], args[24], args[25]);
			break;
		case 27:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											 args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											 args[20], args[21], args[22], args[23], args[24], args[25], args[26]);
			break;
		case 28:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											 args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											 args[20], args[21], args[22], args[23], args[24], args[25], args[26], args[27]);
			break;
		case 29:
			*((unsigned_int *) retval) = (*((unsigned_int (*) ()) func)) (
											 args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											 args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											 args[20], args[21], args[22], args[23], args[24], args[25], args[26], args[27], args[28]);
			break;
		default:
			intp_stmt_error (eng, "too many arguments (30 words max)");
		}
		break;
	case nel_D_UNSIGNED_CHAR:
		switch (arg_size) {
		case 0:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) ();
			break;
		case 1:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0]);
			break;
		case 2:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1]);
			break;
		case 3:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1], args[2]);
			break;
		case 4:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1], args[2], args[3]);
			break;
		case 5:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1], args[2], args[3], args[4]);
			break;
		case 6:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1], args[2], args[3], args[4], args[5]);
			break;
		case 7:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
			break;
		case 8:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
			break;
		case 9:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
			break;
		case 10:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
			break;
		case 11:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											  args[10]);
			break;
		case 12:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											  args[10], args[11]);
			break;
		case 13:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											  args[10], args[11], args[12]);
			break;
		case 14:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											  args[10], args[11], args[12], args[13]);
			break;
		case 15:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											  args[10], args[11], args[12], args[13], args[14]);
			break;
		case 16:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											  args[10], args[11], args[12], args[13], args[14], args[15]);
			break;
		case 17:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											  args[10], args[11], args[12], args[13], args[14], args[15], args[16]);
			break;
		case 18:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											  args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17]);
			break;
		case 19:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											  args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18]);
			break;
		case 20:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											  args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19]);
			break;
		case 21:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											  args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											  args[20]);
			break;
		case 22:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											  args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											  args[20], args[21]);
			break;
		case 23:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											  args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											  args[20], args[21], args[22]);
			break;
		case 24:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											  args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											  args[20], args[21], args[22], args[23]);
			break;
		case 25:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											  args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											  args[20], args[21], args[22], args[23], args[24]);
			break;
		case 26:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											  args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											  args[20], args[21], args[22], args[23], args[24], args[25]);
			break;
		case 27:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											  args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											  args[20], args[21], args[22], args[23], args[24], args[25], args[26]);
			break;
		case 28:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											  args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											  args[20], args[21], args[22], args[23], args[24], args[25], args[26], args[27]);
			break;
		case 29:
			*((unsigned_char *) retval) = (*((unsigned_char (*) ()) func)) (
											  args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
											  args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
											  args[20], args[21], args[22], args[23], args[24], args[25], args[26], args[27], args[28]);
			break;
		default:
			intp_stmt_error (eng, "too many arguments (30 words max)");
		}
		break;
	case nel_D_UNSIGNED_SHORT_INT:
		switch (arg_size) {
		case 0:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) ();
			break;
		case 1:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0]);
			break;
		case 2:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1]);
			break;
		case 3:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1], args[2]);
			break;
		case 4:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1], args[2], args[3]);
			break;
		case 5:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1], args[2], args[3], args[4]);
			break;
		case 6:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1], args[2], args[3], args[4], args[5]);
			break;
		case 7:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
			break;
		case 8:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
			break;
		case 9:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
			break;
		case 10:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
			break;
		case 11:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												   args[10]);
			break;
		case 12:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												   args[10], args[11]);
			break;
		case 13:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												   args[10], args[11], args[12]);
			break;
		case 14:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												   args[10], args[11], args[12], args[13]);
			break;
		case 15:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												   args[10], args[11], args[12], args[13], args[14]);
			break;
		case 16:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												   args[10], args[11], args[12], args[13], args[14], args[15]);
			break;
		case 17:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												   args[10], args[11], args[12], args[13], args[14], args[15], args[16]);
			break;
		case 18:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17]);
			break;
		case 19:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18]);
			break;
		case 20:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19]);
			break;
		case 21:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
												   args[20]);
			break;
		case 22:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
												   args[20], args[21]);
			break;
		case 23:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
												   args[20], args[21], args[22]);
			break;
		case 24:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
												   args[20], args[21], args[22], args[23]);
			break;
		case 25:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
												   args[20], args[21], args[22], args[23], args[24]);
			break;
		case 26:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
												   args[20], args[21], args[22], args[23], args[24], args[25]);
			break;
		case 27:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
												   args[20], args[21], args[22], args[23], args[24], args[25], args[26]);
			break;
		case 28:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
												   args[20], args[21], args[22], args[23], args[24], args[25], args[26], args[27]);
			break;
		case 29:
			*((unsigned_short_int *) retval) = (*((unsigned_short_int (*) ()) func)) (
												   args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
												   args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
												   args[20], args[21], args[22], args[23], args[24], args[25], args[26], args[27], args[28]);
			break;
		default:
			intp_stmt_error (eng, "too many arguments (30 words max)");
		}
		break;
	case nel_D_VOID:
		switch (arg_size) {
		case 0:
			((void (*) ()) func) ();
			break;
		case 1:
			((void (*) ()) func) (
				args[0]);
			break;
		case 2:
			((void (*) ()) func) (
				args[0], args[1]);
			break;
		case 3:
			((void (*) ()) func) (
				args[0], args[1], args[2]);
			break;
		case 4:
			((void (*) ()) func) (
				args[0], args[1], args[2], args[3]);
			break;
		case 5:
			((void (*) ()) func) (
				args[0], args[1], args[2], args[3], args[4]);
			break;
		case 6:
			((void (*) ()) func) (
				args[0], args[1], args[2], args[3], args[4], args[5]);
			break;
		case 7:
			((void (*) ()) func) (
				args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
			break;
		case 8:
			((void (*) ()) func) (
				args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
			break;
		case 9:
			((void (*) ()) func) (
				args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
			break;
		case 10:
			((void (*) ()) func) (
				args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
			break;
		case 11:
			((void (*) ()) func) (
				args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
				args[10]);
			break;
		case 12:
			((void (*) ()) func) (
				args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
				args[10], args[11]);
			break;
		case 13:
			((void (*) ()) func) (
				args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
				args[10], args[11], args[12]);
			break;
		case 14:
			((void (*) ()) func) (
				args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
				args[10], args[11], args[12], args[13]);
			break;
		case 15:
			((void (*) ()) func) (
				args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
				args[10], args[11], args[12], args[13], args[14]);
			break;
		case 16:
			((void (*) ()) func) (
				args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
				args[10], args[11], args[12], args[13], args[14], args[15]);
			break;
		case 17:
			((void (*) ()) func) (
				args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
				args[10], args[11], args[12], args[13], args[14], args[15], args[16]);
			break;
		case 18:
			((void (*) ()) func) (
				args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
				args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17]);
			break;
		case 19:
			((void (*) ()) func) (
				args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
				args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18]);
			break;
		case 20:
			((void (*) ()) func) (
				args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
				args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19]);
			break;
		case 21:
			((void (*) ()) func) (
				args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
				args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
				args[20]);
			break;
		case 22:
			((void (*) ()) func) (
				args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
				args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
				args[20], args[21]);
			break;
		case 23:
			((void (*) ()) func) (
				args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
				args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
				args[20], args[21], args[22]);
			break;
		case 24:
			((void (*) ()) func) (
				args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
				args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
				args[20], args[21], args[22], args[23]);
			break;
		case 25:
			((void (*) ()) func) (
				args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
				args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
				args[20], args[21], args[22], args[23], args[24]);
			break;
		case 26:
			((void (*) ()) func) (
				args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
				args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
				args[20], args[21], args[22], args[23], args[24], args[25]);
			break;
		case 27:
			((void (*) ()) func) (
				args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
				args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
				args[20], args[21], args[22], args[23], args[24], args[25], args[26]);
			break;
		case 28:
			((void (*) ()) func) (
				args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
				args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
				args[20], args[21], args[22], args[23], args[24], args[25], args[26], args[27]);
			break;
		case 29:
			((void (*) ()) func) (
				args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
				args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19],
				args[20], args[21], args[22], args[23], args[24], args[25], args[26], args[27], args[28]);
			break;
		default:
			intp_stmt_error (eng, "too many arguments (30 words max)");
		}
		break;


	case nel_D_STRUCT:
	case nel_D_UNION:
	default:
		intp_stmt_error (eng, "illegal function return type");
		break;
	}

	nel_debug ({ intp_trace (eng, "] exiting intp_apply ()\n\n"); });
}


/*****************************************************************************/
/* intp_dyn_value_alloc () allocates a space of <bytes> bytes in the dynamic   */
/* value table on an alignment-byte boundary, and returns a pointer to it.   */
/*****************************************************************************/
//wyong, 20230907
//char *intp_dyn_value_alloc (struct nel_eng *eng, register unsigned_int bytes, register unsigned_int alignment)
char *intp_dyn_value_alloc (struct nel_eng *eng, unsigned_int bytes, unsigned_int alignment)
{
	register char *retval;

	nel_debug ({ intp_trace (eng, "entering intp_dyn_value_alloc [\neng = 0x%x\nbytes = 0x%x\nalignment = 0x%x\n\n", eng, bytes, alignment); });

	nel_debug ({
	if (bytes == 0) {
		intp_fatal_error (eng, "(intp_dyn_value_alloc #1): bytes = 0") ;
	}
	if (alignment == 0) {
		intp_fatal_error (eng,"(intp_dyn_value_alloc #2): alignment=0");
	}
	});

	/*****************************************/
	/* we do not want memory fetches for two */
	/* different objects overlapping in the  */
	/* same word if the machine does not     */
	/* support byte-size loads and stores.   */
	/*****************************************/
	eng->intp->dyn_values_next = nel_align_ptr (eng->intp->dyn_values_next, nel_SMALLEST_MEM);

	/******************************************************/
	/* get a pointer to the start of the allocated space. */
	/* make sure it is on an properly-aligned boundary.   */
	/******************************************************/
	retval = (char *) eng->intp->dyn_values_next;
	retval = nel_align_ptr (retval, alignment);

	if (retval + bytes > eng->intp->dyn_values_end) {
		/***********************/
		/* longjmp on overflow */
		/***********************/
		intp_stmt_error (eng, "dynamic value table overflow");
		retval = NULL;
	} else {
		/*********************/
		/* reserve the space */
		/*********************/
		eng->intp->dyn_values_next = retval + bytes;
	}

	/*************************************************/
	/* zero out all elements of the allocated space  */
	/*************************************************/
	nel_zero (bytes, retval);

	nel_debug ( {
		intp_trace (eng, "] exiting intp_dyn_value_alloc\nretval = 0x%x\n\n", retval);
	});

	return (retval);

}
/*****************************************************************************/
/* intp_dyn_symbol_alloc () inserts a symbol into *nel_dyn_symbols.  <name>     */
/* should reside in nel's name table - it is not automatically copied there.  */
/*   struct nel_eng *eng;						*/
/*   char *name;		identifier string			*/
/*   nel_type *type;	type descriptor				*/
/*   char *value;		points to memory where value is stored	*/
/*   nel_C_token class;	symbol class					*/
/*   unsigned_int lhs;	legal left hand side of an assignment	*/
/*   nel_L_token source_lang; source language				*/
/*   int level;		scoping level				*/
/*****************************************************************************/
nel_symbol *intp_dyn_symbol_alloc (struct nel_eng *eng, char *name, nel_type *type, char *value, nel_C_token class, unsigned_int lhs, nel_L_token source_lang, int level)
{
	register nel_symbol *retval;

	nel_debug ({ intp_trace (eng, "entering intp_dyn_symbol_alloc [\neng = 0x%x\nname = %s\ntype =\n%1Tvalue = 0x%x\nclass = %C\nlhs = %d\nsource_lang = %H\nlevel = %d\n\n", eng, (name == NULL) ? "NULL" : name, type, value, class, lhs, source_lang, level); });

	/***********************/
	/* longjmp on overflow */
	/***********************/
	if (eng->intp->dyn_symbols_next >= eng->intp->dyn_symbols_end)
	{
		intp_stmt_error (eng, "dynamic symbol overflow");
	}

	/**************************************************/
	/* allocate space, initialize members, and return */
	/**************************************************/
	retval = eng->intp->dyn_symbols_next++;
	retval->name = name;
	retval->type = type;
	retval->value = value;
	retval->class = class;
	retval->lhs = lhs;
	retval->source_lang = source_lang;
	retval->declared = 0;
	retval->level = level;
	retval->reg_no = -1;
	retval->member = NULL;
	retval->table = NULL;
	retval->next = NULL;
	retval->data = 0;
	retval->_global = NULL;

	nel_debug ({ intp_trace (eng, "] exiting intp_dyn_symbol_alloc\nretval =\n%1S\n", retval); });
	return (retval);
}

//wyong, 20230907 
nel_symbol *intp_eval_call_2(struct nel_eng *eng, nel_symbol *func, int nargs, nel_stack *arg_start )
{
	/********************************************/
	/* register variables are not guaranteed to */
	/* survive a longjmp () on some systems     */
	/********************************************/
	nel_jmp_buf return_pt;
	struct eng_intp *old_intp = NULL;
	int val;
	nel_symbol *retval;

	//nel_debug ({ intp_trace (eng, "entering intp_eval_call_2 () [\nretloc = 0x%x\nsymbol =\n%1Sformal_ap = 0x%x\n\n", retloc, symbol, formal_ap); });
	if (( func == NULL) 
		|| (func->type == NULL)
		|| (func->type->simple.type != nel_D_FUNCTION)
		|| (func->class == nel_C_COMPILED_FUNCTION)
		|| (func->class == nel_C_COMPILED_STATIC_FUNCTION)
		|| (func->class == nel_C_INTRINSIC_FUNCTION)
		|| (func->value == NULL)) {
		intp_fatal_error (NULL, "(intp_eval_call_2 #1): bad function \n%1S", func );
	}


	if(eng->intp != NULL){
		old_intp = eng->intp;
	}

	/*********************************************/
	/* initialize the engine record that is passed */
	/* thoughout the entire call sequence.       */
	/* nel_intp_eval () will set the filename and */
	/* line numbers from the stmt structures     */
	/*********************************************/
	nel_malloc(eng->intp, 1, struct eng_intp );
	intp_init (eng, "", NULL, NULL, NULL, 0);

	//added by zhangbin, 2006-5-22
	eng->intp->ret_type = func->type->function.return_type;
	//end

	/**********************/
	/* call the evaluator */
	/**********************/
	eng->intp->return_pt = &return_pt;
	intp_setjmp (eng, val, &return_pt);
	if (val == 0) {
		retval = intp_eval_call(eng, func, nargs, arg_start );
	}

	/*******************************************************/
	/* deallocate any dynamically allocated substructs     */
	/* withing the engine (if they were stack allocated) */
	/* and return.                                         */
	/*******************************************************/
	//retval = eng->intp->error_ct;
	retval = nel_symbol_dup(eng, retval);  /*bugfix, wyong, 2006.4.12 */
	intp_dealloc (eng);

	nel_debug ({ intp_trace (eng, "] exiting intp_eval ()\nerror_ct = %d\n\n", eng->intp->error_ct); });
	
	/*bugfix, nel_dealloca before nel_debug, wyong, 2005.12.8 */
	nel_dealloca(eng->intp);
	//eng->intp = NULL;	//added by zhangbin, 2006-7-21
	
	if(old_intp != NULL){
		eng->intp = old_intp;
	}

	return (retval);
}


/*****************************************************************************/
/* intp_eval_call() takes the nel_symbol <func> and the list of nel_symbols   */
/* (there are <nargs> symbols, each a variant of ane of the <nargs> nel_stack*/
/* structs occupying consecutive memory locations starting at <arg_start>),  */
/* and lays out the values of the symbols in consecutive memory locations,   */
/* and then applies the value of <func>, the starting address of a function, */
/* to the arguments.                                                         */
/*****************************************************************************/
//register int nargs -> int args, wyong, 20230816 
nel_symbol *intp_eval_call (struct nel_eng *eng, nel_symbol *func, int nargs, nel_stack *arg_start)
{
	char (* address) ();
	register nel_type *type;
	register nel_stack *stack_scan;
	int i;
	register int arg_size;
	register char *arg_data;
	register char *ap;
	int arg_words;
	nel_symbol *retval;
	nel_type *return_type;
	int intrin = 0;	/* flag signalling intrinsic routine	*/
	int intp = 0;	/* flag signalling interpreted routine	*/

	//added by zhangbin, 2006-6-29
	char *v;
	nel_type *t;
	//end

	nel_debug ({ intp_trace (eng, "entering intp_eval_call () [\neng = 0x%x\nfunc =\n%1Snargs = %d\narg_start = 0x%x\n\n", eng, func, nargs, arg_start); });

	
	nel_debug ({//modified by zhangbin, 2006-5-24
	   if ((func == NULL) || (func->type == NULL) || (func->value == NULL)) {

//#if 0
//		   intp_fatal_error (eng, "(intp_eval_call #1): bad function\n%1S", func) ;
//#else
		   /* wyong, 2006.9.26 */
		   if(func && func->name)
			   intp_fatal_error (eng, "bad function %s", func->name) ;
		   else 
			   intp_fatal_error (eng, "bad function") ;
//#endif

	   }
	})
	;
	/****************************************************************/
	/* first, check to make sure that the func has a function type, */
	/* or if it is a pointer to a function, dereference it.         */
	/* an uncoerced location type is also acceptable.               */
	/* set address to the entry point of the function.              */
	/****************************************************************/
	type = func->type;
	if (type->simple.type == nel_D_POINTER) {
		if (func->value == NULL) {
			if (func->name == NULL) {
				intp_stmt_error (eng, "NULL address for pointer");
			} else {
				intp_stmt_error (eng, "NULL address for pointer: %I", func);
			}
		}
		type = type->pointer.deref_type;
		if (type->simple.type != nel_D_FUNCTION) {
			if (func->name == NULL) {
				intp_stmt_error (eng, "illegal function");
			} else {
				intp_stmt_error (eng, "illegal function: %I", func);
			}
		}
		address = (char (*) ()) (* ((char **) (func->value)));
		return_type = type->function.return_type;
		nel_debug ({
		if (return_type == NULL) {
			intp_fatal_error (eng, "(intp_eval_call #2): NULL function return type");
		}
		});

	} else if (type->simple.type == nel_D_FUNCTION) {
		/*************************************************/
		/* set a flag if this is an intrinsic routine to */
		/* be called directly, or an interpreted routine */
		/* to be called through nel_func_call().               */
		/*************************************************/
		address = (char (*) ()) (func->value);
		return_type = type->function.return_type;
		intrin = (func->class == nel_C_INTRINSIC_FUNCTION);
		intp = nel_C_token (func->class);

	} else if (type->simple.type == nel_D_LOCATION) {
		address = (char (*) ()) (func->value);
		return_type = nel_int_type;

	} else {
		if (func->name == NULL) {
			intp_stmt_error (eng, "illegal function");
		} else {
			intp_stmt_error (eng, "illegal function: %I", func);
		}
	}

	/**************************************/
	/* some systems have functions at 0x0 */
	/* chage the #if appropriately.       */
	/**************************************/
#if 0
	if (address == NULL) {
		if (func->name == NULL) {
			intp_stmt_error (eng, "NULL address for function");
		} else {
			intp_stmt_error (eng, "NULL address for function: %I", func);
		}
	}
#endif /* 0 */

	if (intrin) {
		/**************************************************************/
		/*for intrinsics, the address of the function is the symbol's */
		/*value, and all intrisics take as arguments the engine , */
		/*the # of args, and the start of the args pushed.            */
		/**************************************************************/
		retval = (*((nel_symbol *(*)()) (func->value))) (eng, func, nargs, arg_start);
	} else {
		/*******************************************/
		/* allocate the return value symbol.       */
		/* it is not a legal lhs of an assignment. */
		/*******************************************/
		nel_debug ({
		   if (return_type == NULL) {
			   intp_fatal_error (eng, "(intp_eval_call #3): NULL function return type") ;
		  }
		});
		retval = intp_dyn_symbol_alloc (eng, NULL, return_type,
			   ((return_type->simple.type != nel_D_VOID) ? intp_dyn_value_alloc (eng, return_type->simple.size, return_type->simple.alignment) : NULL), nel_C_RESULT, 0, nel_L_NEL, eng->intp->level);

//added by zhangbin, 2006-6-29
v=retval->value;
t=retval->type;
//end
		
		/******************************************/
		/* we don't do the type coercions exaclty */
		/* right for new-style functions          */
		/******************************************/
		if (type->function.new_style) {

#if 0		/*NOTE,NOTE,NOTE, wyong, 2006.3.1 */	
			if (func->name == NULL) {
				intp_warning (eng, "default parameter passing used");
			} else {
				intp_warning (eng, "default parameter passing used: %I", func);
			}
#endif

		}

		/***********************************************************/
		/* scan the arguments to find out how much space they take */
		/***********************************************************/
#define intp_inc_int_size(_type) \
{								\
	arg_size = nel_align_offset (arg_size, nel_alignment_of (_type)); \
	arg_size += sizeof (_type);					\
}

#define intp_inc_fp_size(_type) \
{								\
	arg_size = nel_align_offset (arg_size, nel_alignment_of (_type)); \
	arg_size += sizeof (_type);					\
}


		arg_size = 0;

		/************************************************/
		/* we need two additional pointer parameters to */
		/* to interpreted function through nel_func_call ().   */
		/************************************************/
		if (intp ) {
			/* the first parameter of nel_func_call is eng , 
			wyong, 2005.9.30 */
			intp_inc_int_size (char *);

			intp_inc_int_size (char *);
			intp_inc_int_size (char *);
		}

		for (i = 0, stack_scan = arg_start; (i < nargs); i++, stack_scan++) {
			register nel_symbol *actual;
			actual = stack_scan->symbol;
			nel_debug ({
			   if ((actual == NULL) || (actual->type == NULL)) {
				   intp_fatal_error (eng, "(intp_eval_call #4): bad actual\n%1S", actual) ;
			   }
			});

			/**************************/
			/* check for NULL address */
			/**************************/
			if (actual->value == NULL) {
				if (actual->name == NULL) {
					intp_stmt_error (eng, "NULL address for argument");
				} else {
					intp_stmt_error (eng, "NULL address for argument %I", actual);
				}
			}

			switch (actual->type->simple.type) {

				/***********************************************/
				/* chars and shorts undergo integral promotion */
				/* and regular-sized int's remain unpromoted.  */
				/***********************************************/
			case nel_D_CHAR:
			case nel_D_ENUM:
			case nel_D_INT:
			case nel_D_SHORT:
			case nel_D_SHORT_INT:
			case nel_D_SIGNED_CHAR:
			case nel_D_SIGNED:
			case nel_D_SIGNED_INT:
			case nel_D_SIGNED_SHORT:
			case nel_D_SIGNED_SHORT_INT:
			case nel_D_UNSIGNED:
			case nel_D_UNSIGNED_CHAR:
			case nel_D_UNSIGNED_INT:
			case nel_D_UNSIGNED_SHORT:
			case nel_D_UNSIGNED_SHORT_INT:
				intp_inc_int_size (int);

				//wyong, 20230821 
				arg_size = nel_align_offset (arg_size, 
						nel_alignment_of (char *)); 
				break;

			case nel_D_LONG:
			case nel_D_LONG_INT:
			case nel_D_SIGNED_LONG:
			case nel_D_SIGNED_LONG_INT:
			case nel_D_UNSIGNED_LONG:
			case nel_D_UNSIGNED_LONG_INT:
				intp_inc_int_size (long);
				//wyong, 20230821 
				arg_size = nel_align_offset (arg_size, 
						nel_alignment_of (char *)); 
				break;

				/**********************************/
				/* floats are coerced to doubles, */
				/* and doubles remain doubles.    */
				/**********************************/
			case nel_D_DOUBLE:
			case nel_D_FLOAT:
			case nel_D_LONG_FLOAT:
				intp_inc_fp_size (double);
				//wyong, 20230821 
				arg_size = nel_align_offset (arg_size, 
						nel_alignment_of (char *)); 
				break;

				/************************************/
				/* long doubles remain long doubles */
				/************************************/
			case nel_D_LONG_DOUBLE:
				intp_inc_fp_size (long_double);
				//wyong, 20230821 
				arg_size = nel_align_offset (arg_size, 
						nel_alignment_of (char *)); 
				break;

				/***************************************************/
				/* complex floats are promoted to complex doubles, */
				/* and complex doubles are treated as two doubles. */
				/***************************************************/
			case nel_D_COMPLEX:
			case nel_D_COMPLEX_DOUBLE:
			case nel_D_COMPLEX_FLOAT:
			case nel_D_LONG_COMPLEX:
			case nel_D_LONG_COMPLEX_FLOAT:
				intp_inc_fp_size (double);
				intp_inc_fp_size (double);
				//wyong, 20230821 
				arg_size = nel_align_offset (arg_size, 
						nel_alignment_of (char *)); 
				break;

				/********************************************************/
				/* long complex doubles are treated as two long doubles */
				/********************************************************/
			case nel_D_LONG_COMPLEX_DOUBLE:
				intp_inc_fp_size (long_double);
				intp_inc_fp_size (long_double);

				//todo, wyong, 20230821 
				arg_size = nel_align_offset (arg_size, 
						nel_alignment_of (char *)); 
				break;

			case nel_D_POINTER:
				intp_inc_int_size (char *);
				break;

				/****************************************************/
				/* arrays and functions get coerced to pointers to  */
				/* arrays and functions by the subroutine call.     */
				/* for common blocks, one may pass a pointer to the */
				/* start of the block, and ld symbol table entries  */
				/* (locations) are coerced to pointers, too.        */
				/****************************************************/
			case nel_D_ARRAY:
			case nel_D_COMMON:
			case nel_D_LOCATION:
				intp_inc_int_size (char *);
				break;

				/***************************************************/
				/* emit a warning when an interpreted or intrinsic */
				/* function is passed as an argument, as the       */
				/* corresponding formal arg will not reflect this. */
				/***************************************************/
			case nel_D_FUNCTION:
				if (nel_C_token (actual->class)) {
					intp_warning (eng, "interpreted function passed as argument");
				}
				if (actual->class == nel_C_INTRINSIC_FUNCTION) {
					intp_warning (eng, "intrinsic function passed as argument");
				}
				intp_inc_int_size (char *);
				break;

				/***********************************************/
				/* structures and unions are machine-dependent */
				/***********************************************/
			case nel_D_STRUCT:
			case nel_D_UNION:
				/**********************************************************/
				/* default assumes that structs and unions passed as args */
				/* take up space that is a multiple of sizeof (int).      */
				/* first align the struct/union on the correct boundary,  */
				/* in case it's alignment is larger than that of an int   */
				/* (can happen on IRIS, ALLIANT_FX2800).  increment the   */
				/* arg size.  then align the size again in case the size  */
				/* of the struct/union was not a multiple of sizeof (int) */
				/* (can happen on all machines except CRAY).              */
				/**********************************************************/
				arg_size = nel_align_offset (arg_size, actual->type->simple.alignment);
				arg_size += actual->type->simple.size;	//modified by zhangbin, 2006-5-11, '=' => '+=' to increase total size of arg list
				arg_size = nel_align_offset (arg_size, sizeof (char *));

				break;


				/**************************/
				/* void types are illegal */
				/**************************/
			case nel_D_VOID:
				if (actual->name == NULL) {
					intp_stmt_error (eng, "void type for argument");
				} else {
					intp_stmt_error (eng, "void type for argument %I", actual);
				}
				break;

			default:
				intp_fatal_error (eng, "(intp_eval_call #5): bad type for actual\n%1S", actual);
				break;

#undef intp_inc_int_size
#undef intp_inc_fp_size

			}
		}

		/*******************************************************/
		/* allocate space for the arguments, and scan the args */
		/* again, copying them to the allocated space.         */
		/*******************************************************/
		arg_size = nel_align_offset (arg_size, sizeof (int));
		nel_alloca (arg_data, arg_size, char);
		nel_zero (arg_size, arg_data);
		ap = arg_data;


#define intp_put_int_arg(_promo,_orig,_ptr) \
{								\
	ap = nel_align_ptr (ap, (_promo)->simple.alignment);		\
	intp_coerce (eng, (_promo), ap, (_orig), (char *) (_ptr));	\
	ap += (_promo)->simple.size;					\
}

#define intp_put_fp_arg(_promo,_orig,_ptr) \
{								\
	ap = nel_align_ptr (ap, (_promo)->simple.alignment);		\
	intp_coerce (eng, (_promo), ap, (_orig), (char *) (_ptr));	\
	ap += (_promo)->simple.size;					\
}

		/************************************************/
		/* when calling an interpreted function through */
		/* nel_func_call(), the first argument is the return  */
		/* value slot, and the second argument is a     */
		/* pointer to the function symbol.              */
		/************************************************/
		if (intp) {
			/* wyong, 2005.9.30 */	
			ap = nel_align_ptr (ap, nel_alignment_of (char *));
			*((char **) ap) = (char *) (eng);
			ap += sizeof (char *);

			ap = nel_align_ptr (ap, nel_alignment_of (char *));
			*((char **) ap) = (char *) (retval->value);
			ap += sizeof (char *);

			ap = nel_align_ptr (ap, nel_alignment_of (char *));
			*((char **) ap) = (char *) (func);
			ap += sizeof (char *);
		}

		for (i = 0, stack_scan = arg_start; (i < nargs); i++, stack_scan++) {
			register nel_symbol *actual;
			actual = stack_scan->symbol;
			nel_debug ({
						   if ((actual == NULL) || (actual->type == NULL)) {
						   intp_fatal_error (eng, "(intp_eval_call #6): bad actual\n%1S", actual)
							   ;
						   }
					   });

			switch (actual->type->simple.type) {

				/******************************************************/
				/* chars and shorts undergo integral promotion,       */
				/* and regular-sized int's remain unpromoted.         */
				/* promote signed to signed and unsigned to unsigned. */
				/******************************************************/
			case nel_D_CHAR:
			case nel_D_ENUM:
			case nel_D_INT:
			case nel_D_SHORT:
			case nel_D_SHORT_INT:
			case nel_D_SIGNED:
			case nel_D_SIGNED_INT:
			case nel_D_SIGNED_CHAR:
			case nel_D_SIGNED_SHORT:
			case nel_D_SIGNED_SHORT_INT:
				intp_put_int_arg (nel_signed_int_type, actual->type, actual->value);
				//wyong, 20230821 
				ap = nel_align_ptr (ap, nel_alignment_of (char *));

				break;

			case nel_D_UNSIGNED:
			case nel_D_UNSIGNED_CHAR:
			case nel_D_UNSIGNED_INT:
			case nel_D_UNSIGNED_SHORT:
			case nel_D_UNSIGNED_SHORT_INT:
				intp_put_int_arg (nel_unsigned_int_type, actual->type, actual->value);
				//wyong, 20230821 
				ap = nel_align_ptr (ap, nel_alignment_of (char *));

				break;

			case nel_D_LONG:
			case nel_D_LONG_INT:
			case nel_D_SIGNED_LONG:
			case nel_D_SIGNED_LONG_INT:
				intp_put_int_arg (nel_signed_long_int_type, actual->type, actual->value);
				//wyong, 20230821 
				ap = nel_align_ptr (ap, nel_alignment_of (char *));

				break;

			case nel_D_UNSIGNED_LONG:
			case nel_D_UNSIGNED_LONG_INT:
				intp_put_int_arg (nel_unsigned_long_int_type, actual->type, actual->value);
				//wyong, 20230821 
				ap = nel_align_ptr (ap, nel_alignment_of (char *));

				break;

				/**********************************/
				/* floats are coerced to doubles, */
				/* and doubles remain doubles.    */
				/**********************************/
			case nel_D_DOUBLE:
			case nel_D_FLOAT:
			case nel_D_LONG_FLOAT:
				intp_put_fp_arg (nel_double_type, actual->type, actual->value);
				//wyong, 20230821 
				ap = nel_align_ptr (ap, nel_alignment_of (char *));

				break;

				/************************************/
				/* long doubles remain long doubles */
				/************************************/
			case nel_D_LONG_DOUBLE:
				intp_put_fp_arg (nel_long_double_type, actual->type, actual->value);
				//wyong, 20230821 
				ap = nel_align_ptr (ap, nel_alignment_of (char *));

				break;

				/***************************************************/
				/* complex floats are promoted to complex doubles, */
				/* and complex doubles are treated as two doubles. */
				/***************************************************/
			case nel_D_COMPLEX:
			case nel_D_COMPLEX_FLOAT:
				intp_put_fp_arg (nel_double_type, nel_float_type, actual->value);
				intp_put_fp_arg (nel_double_type, nel_float_type, actual->value + sizeof (float));
				//wyong, 20230821 
				ap = nel_align_ptr (ap, nel_alignment_of (char *));

				break;

				/***********************************************/
				/* complex doubles are treated as two doubles. */
				/***********************************************/
			case nel_D_COMPLEX_DOUBLE:
			case nel_D_LONG_COMPLEX:
			case nel_D_LONG_COMPLEX_FLOAT:
				intp_put_fp_arg (nel_double_type, nel_double_type, actual->value);
				intp_put_fp_arg (nel_double_type, nel_double_type, actual->value + sizeof (double));
				//wyong, 20230821 
				ap = nel_align_ptr (ap, nel_alignment_of (char *));

				break;

				/********************************************************/
				/* long complex doubles are treated as two long doubles */
				/********************************************************/
			case nel_D_LONG_COMPLEX_DOUBLE:
				intp_put_fp_arg (nel_long_double_type, nel_long_double_type, actual->value);
				intp_put_fp_arg (nel_long_double_type, nel_long_double_type, actual->value + sizeof (long_double));
				//todo, wyong, 20230821 
				ap = nel_align_ptr (ap, nel_alignment_of (char *));

				break;

			case nel_D_POINTER:
				ap = nel_align_ptr (ap, nel_alignment_of (char *));
				*((char **) ap) = *((char **) (actual->value));
				ap += sizeof (char *);
				break;


				/****************************************************/
				/* arrays and functions get coerced to pointers to  */
				/* arrays and functions by the subroutine call.     */
				/* for common blocks, one may pass a pointer to the */
				/* start of the block, and ld symbol table entries  */
				/* (locations) are coerced to pointers, too.        */
				/****************************************************/
			case nel_D_ARRAY:
			case nel_D_COMMON:
			case nel_D_LOCATION:
			case nel_D_FUNCTION:
				ap = nel_align_ptr (ap, nel_alignment_of (char *));
				*((char **) ap) = actual->value;

				//bugfix, int -> char *, wyong, 20230823 
				ap += sizeof (char *);
				break;


				/***********************************************/
				/* structures and unions are machine-dependent */
				/***********************************************/
			case nel_D_STRUCT:
			case nel_D_UNION:

				/*******************************************/
				/* default assumes that structs and unions */
				/* passed as arguments have a size that is */
				/* a multiple of sizeof (int).             */
				/*******************************************/
				{
					register int size = actual->type->simple.size;
					ap = nel_align_ptr (ap, actual->type->simple.alignment);
					nel_copy (size, ap, actual->value);
					ap += size;
					ap = nel_align_ptr (ap, nel_alignment_of (char *));
				}
				break;


			default:
				intp_fatal_error (eng, "(intp_eval_call #7-2): bad type for actual\n%1S", actual);
				break;

#undef intp_put_int_arg
#undef intp_put_fp_arg

			}
		}

		/**************************************************************/
		/* the arguments are set up - apply the function to them      */
		/* we already know that arg_size is a multiple of sizeof (int)*/
		/**************************************************************/
		//wyong, 20230821 
		//arg_words = arg_size / sizeof (int);
		arg_words = arg_size / sizeof (char *);

		nel_debug ({
					   int *arg_scan;
					   intp_trace (eng, "arg_words = %d\n", arg_words);
					   intp_trace (eng, "arguments:\n");
					   for (i = arg_words, arg_scan = ((int *) arg_data) + arg_words - 1; (i > 0); i--, arg_scan--) {
					   intp_trace (eng, "   0x%x\n", *arg_scan)
						   ;
					   }
					   intp_trace (eng, "\n");
				   });

		if (! intp) {
			/******************************************************/
			/*call intp_apply() to do the actual function call. */
			/*we pass extra parameters on ALLIANT_FX2800.           */
			/*********************************************************/
			//(int *)arg_data -> (int **)arg_data , 20230821 
			intp_apply (eng, retval->value, return_type, address, arg_words, (int **) arg_data);

		} else {
			/**********************************************/
			/* have intp_apply () call nel_func_call() to do */
			/* the actual function call.  we pass extra   */
			/* parameters on ALLIANT_FX2800.              */
			/**********************************************/
			int nel_retval;
			//(int *)arg_data -> (int **)arg_data , 20230821 
			intp_apply (eng, (char *) (&nel_retval), nel_int_type, (char (*)()) nel_func_call, arg_words, (int **) arg_data);
			//added by zhangbin, 2006-6-29
			retval->value = v;
			retval->type = t;
			//end
		}
		nel_dealloca (arg_data);
	}

	nel_debug ({ intp_trace (eng, "] exiting intp_eval_call ()\nretval =\n%1S\n", retval); });
	return (retval);

}

/*****************************************************************************/
/* intp_op () perfoms operation with nel_O_token <O_token> upon       */
/* operands with data type <type>.  <left> and <right> point to the          */
/* operands, and the result is stored in <result>.                           */
/*****************************************************************************/
void intp_op (struct nel_eng *eng, nel_O_token O_token, nel_type *type, char *result, char *left, char *right)
{
	register nel_D_token D_token;

	nel_debug ({ intp_trace (eng, "entering intp_op [\neng = 0x%x\nO_token = %O\ntype = \n%1Tresult = 0x%x\nleft = 0x%x\nright = 0x%x\n\n", eng, O_token, type, result, left, right); });

	nel_debug ({
				   if (type == NULL) {
				   intp_fatal_error (eng, "(intp_op #1): NULL type")
					   ;
				   }
				   if (result == NULL) {
				   intp_fatal_error (eng, "(intp_op #2): NULL result")
					   ;
				   }
				   if (nel_unary_O_token (O_token) && ((left == NULL) || (right != NULL))) {
				   intp_fatal_error (eng, "(intp_op #3): wrong # of operands\nop = %O\nleft = 0x%x\nright = 0x%x", O_token, left, right)
					   ;
				   }
				   if (nel_binary_O_token (O_token) && ((left == NULL) || (right == NULL))) {
				   intp_fatal_error (eng, "(intp_op #4): wrong # of operands\nop = %O\nleft = 0x%x\nright = 0x%x", O_token, left, right)
					   ;
				   }
			   });

	/*********************************************/
	/* find the standardized equivalent D_tokens */
	/*********************************************/
	D_token = nel_D_token_equiv (type->simple.type);

	switch (D_token) {
		/********************************************************/
		/* include "intp_op_incl.h to perform the operation. */
		/********************************************************/

	case nel_D_SIGNED_CHAR:
		switch (O_token) {
		case nel_O_POST_DEC:
			*((signed_char *) result) = (*((signed_char *) left)) --;
			break;
		case nel_O_POST_INC:
			*((signed_char *) result) = (*((signed_char *) left)) ++;
			break;
		case nel_O_PRE_DEC:
			*((signed_char *) result) = -- (*((signed_char *) left));
			break;
		case nel_O_PRE_INC:
			*((signed_char *) result) = ++ (*((signed_char *) left));
			break;
		default:
			intp_fatal_error (eng, "(intp_op_incl.h #1): illegal type for op %O: %D", O_token, D_token);
		}
		break;
	case nel_D_COMPLEX_FLOAT:
		switch (O_token) {
		case nel_O_ADD:
			*nel_real_part (result, float) = (*nel_real_part (left, float)) + (*nel_real_part (right, float));
			*nel_imaginary_part (result, float) = (*nel_imaginary_part (left, float)) + (*nel_real_part (right, float));
			break;
		case nel_O_DIV:
			*nel_real_part (result, float) = ((*nel_real_part (left, float)) * (*nel_real_part (right, float)) + (*nel_imaginary_part (left, float)) * (*nel_imaginary_part (right, float))) / ((*nel_real_part (right, float)) * (*nel_real_part (right, float)) + (*nel_imaginary_part (right, float)) * (*nel_imaginary_part (right, float)));
			*nel_imaginary_part (result, float) = ((*nel_imaginary_part (left, float)) * (*nel_real_part (right, float)) - (*nel_real_part (left, float)) * (*nel_imaginary_part (right, float))) / ((*nel_real_part (right, float)) * (*nel_real_part (right, float)) + (*nel_imaginary_part (right, float)) * (*nel_imaginary_part (right, float)));
			break;
		case nel_O_EQ:
			*((int *) result) = ((*nel_real_part (left, float)) == (*nel_real_part (right, float))) && ((*nel_imaginary_part (left, float)) == (*nel_imaginary_part (right, float)));
			break;
		case nel_O_MULT:
			*nel_real_part (result, float) = (*nel_real_part (left, float)) * (*nel_real_part (right, float)) - (*nel_imaginary_part (left, float)) * (*nel_imaginary_part (right, float));
			*nel_imaginary_part (result, float) = (*nel_real_part (left, float)) * (*nel_imaginary_part (right, float)) - (*nel_imaginary_part (left, float)) * (*nel_real_part (right, float));
			break;
		case nel_O_NE:
			*((int *) result) = ((*nel_real_part (left, float)) != (*nel_real_part (right, float))) && ((*nel_imaginary_part (left, float)) != (*nel_imaginary_part (right, float)));
			break;
		case nel_O_NEG:
			*nel_real_part (result, float) = - (*nel_real_part (left, float));
			*nel_imaginary_part (result, float) = - (*nel_imaginary_part (left, float));
			break;
		case nel_O_POS:
			*nel_real_part (result, float) =   (*nel_real_part (left, float));
			*nel_imaginary_part (result, float) =   (*nel_imaginary_part (left, float));
			break;
		case nel_O_SUB:
			*nel_real_part (result, float) = (*nel_real_part (left, float)) - (*nel_real_part (right, float));
			*nel_imaginary_part (result, float) = (*nel_imaginary_part (left, float)) - (*nel_real_part (right, float));
			break;
		default:
			intp_fatal_error (eng, "(intp_op_incl.h #1): illegal type for op %O: %D", O_token, D_token);
		}
		break;
	case nel_D_COMPLEX_DOUBLE:
		switch (O_token) {
		case nel_O_ADD:
			*nel_real_part (result, double) = (*nel_real_part (left, double)) + (*nel_real_part (right, double));
			*nel_imaginary_part (result, double) = (*nel_imaginary_part (left, double)) + (*nel_real_part (right, double));
			break;
		case nel_O_DIV:
			*nel_real_part (result, double) = ((*nel_real_part (left, double)) * (*nel_real_part (right, double)) + (*nel_imaginary_part (left, double)) * (*nel_imaginary_part (right, double))) / ((*nel_real_part (right, double)) * (*nel_real_part (right, double)) + (*nel_imaginary_part (right, double)) * (*nel_imaginary_part (right, double)));
			*nel_imaginary_part (result, double) = ((*nel_imaginary_part (left, double)) * (*nel_real_part (right, double)) - (*nel_real_part (left, double)) * (*nel_imaginary_part (right, double))) / ((*nel_real_part (right, double)) * (*nel_real_part (right, double)) + (*nel_imaginary_part (right, double)) * (*nel_imaginary_part (right, double)));
			break;
		case nel_O_EQ:
			*((int *) result) = ((*nel_real_part (left, double)) == (*nel_real_part (right, double))) && ((*nel_imaginary_part (left, double)) == (*nel_imaginary_part (right, double)));
			break;
		case nel_O_MULT:
			*nel_real_part (result, double) = (*nel_real_part (left, double)) * (*nel_real_part (right, double)) - (*nel_imaginary_part (left, double)) * (*nel_imaginary_part (right, double));
			*nel_imaginary_part (result, double) = (*nel_real_part (left, double)) * (*nel_imaginary_part (right, double)) - (*nel_imaginary_part (left, double)) * (*nel_real_part (right, double));
			break;
		case nel_O_NE:
			*((int *) result) = ((*nel_real_part (left, double)) != (*nel_real_part (right, double))) && ((*nel_imaginary_part (left, double)) != (*nel_imaginary_part (right, double)));
			break;
		case nel_O_NEG:
			*nel_real_part (result, double) = - (*nel_real_part (left, double));
			*nel_imaginary_part (result, double) = - (*nel_imaginary_part (left, double));
			break;
		case nel_O_POS:
			*nel_real_part (result, double) =   (*nel_real_part (left, double));
			*nel_imaginary_part (result, double) =   (*nel_imaginary_part (left, double));
			break;
		case nel_O_SUB:
			*nel_real_part (result, double) = (*nel_real_part (left, double)) - (*nel_real_part (right, double));
			*nel_imaginary_part (result, double) = (*nel_imaginary_part (left, double)) - (*nel_real_part (right, double));
			break;
		default:
			intp_fatal_error (eng, "(intp_op_incl.h #1): illegal type for op %O: %D", O_token, D_token);
		}
		break;
	case nel_D_DOUBLE:
		switch (O_token) {
		case nel_O_ADD:
			*((double *) result) = (*((double *) left)) + (*((double *) right));
			break;
		case nel_O_DIV:
			//added by zhangbin, to check divide oprand
			if(check_intp_zero(eng, type, right))
			{
				intp_stmt_error(eng, "divided by zero");
				break;
			}
			*((double *) result) = (*((double *) left)) / (*((double *) right));
			break;
		case nel_O_EQ:
			*((int *) result) = (*((double *) left)) == (*((double *) right));
			break;
		case nel_O_GE:
			*((int *) result) = (*((double *) left)) >= (*((double *) right));
			break;
		case nel_O_GT:
			*((int *) result) = (*((double *) left)) > (*((double *) right));
			break;
		case nel_O_LE:
			*((int *) result) = (*((double *) left)) <= (*((double *) right));
			break;
		case nel_O_LT:
			*((int *) result) = (*((double *) left)) < (*((double *) right));
			break;
		case nel_O_MULT:
			*((double *) result) = (*((double *) left)) * (*((double *) right));
			break;
		case nel_O_NE:
			*((int *) result) = (*((double *) left)) != (*((double *) right));
			break;
		case nel_O_NEG:
			*((double *) result) =  - (*((double *) left));
			break;
		case nel_O_POST_DEC:
			*((double *) result) = (*((double *) left)) --;
			break;
		case nel_O_POST_INC:
			*((double *) result) = (*((double *) left)) ++;
			break;
		case nel_O_POS:
			*((double *) result) =    (*((double *) left));
			break;
		case nel_O_PRE_DEC:
			*((double *) result) = -- (*((double *) left));
			break;
		case nel_O_PRE_INC:
			*((double *) result) = ++ (*((double *) left));
			break;
		case nel_O_SUB:
			*((double *) result) = (*((double *) left)) - (*((double *) right));
			break;
		default:
			intp_fatal_error (eng, "(intp_op_incl.h #1): illegal type for op %O: %D", O_token, D_token);
		}
		break;
	case nel_D_SIGNED_INT:
		switch (O_token) {
		case nel_O_ADD:
			*((signed_int *) result) = (*((signed_int *) left)) + (*((signed_int *) right));
			break;
		case nel_O_BIT_AND:
			*((signed_int *) result) = (*((signed_int *) left)) & (*((signed_int *) right));
			break;
		case nel_O_BIT_OR:
			*((signed_int *) result) = (*((signed_int *) left)) | (*((signed_int *) right));
			break;
		case nel_O_BIT_NEG:
			*((signed_int *) result) = ~ (*((signed_int *) left));
			break;
		case nel_O_BIT_XOR:
			*((signed_int *) result) = (*((signed_int *) left)) ^ (*((signed_int *) right));
			break;
		case nel_O_DIV:
			//added by zhangbin, to check divide oprand
			if(check_intp_zero(eng, type, right))
			{
				intp_stmt_error(eng, "divided by zero");
				break;
			}
			*((signed_int *) result) = (*((signed_int *) left)) / (*((signed_int *) right));
			break;
		case nel_O_EQ:
			*((int *) result) = (*((signed_int *) left)) == (*((signed_int *) right));
			break;
		case nel_O_GE:
			*((int *) result) = (*((signed_int *) left)) >= (*((signed_int *) right));
			break;
		case nel_O_GT:
			*((int *) result) = (*((signed_int *) left)) > (*((signed_int *) right));
			break;
		case nel_O_LE:
			*((int *) result) = (*((signed_int *) left)) <= (*((signed_int *) right));
			break;
		case nel_O_LSHIFT:
			//added by zhangbin, 2006-5-19
			if(*((signed int*)right) < 0)
				intp_warning(eng, "left shift negative bits");
			else if(type->simple.size < *((signed int *)right))
				intp_warning(eng, "left shift too many bits");
			//end
			*((signed_int *) result) = (*((signed_int *) left)) << (*((signed_int *) right));
			break;
		case nel_O_LT:
			*((int *) result) = (*((signed_int *) left)) < (*((signed_int *) right));
			break;
		case nel_O_MOD:
			*((signed_int *) result) = (*((signed_int *) left)) % (*((signed_int *) right));
			break;
		case nel_O_MULT:
			*((signed_int *) result) = (*((signed_int *) left)) * (*((signed_int *) right));
			break;
		case nel_O_NE:
			*((int *) result) = (*((signed_int *) left)) != (*((signed_int *) right));
			break;
		case nel_O_NEG:
			*((signed_int *) result) =  - (*((signed_int *) left));
			break;
		case nel_O_POST_DEC:
			*((signed_int *) result) = (*((signed_int *) left)) --;
			break;
		case nel_O_POST_INC:
			*((signed_int *) result) = (*((signed_int *) left)) ++;
			break;
		case nel_O_POS:
			*((signed_int *) result) =    (*((signed_int *) left));
			break;
		case nel_O_PRE_DEC:
			*((signed_int *) result) = -- (*((signed_int *) left));
			break;
		case nel_O_PRE_INC:
			*((signed_int *) result) = ++ (*((signed_int *) left));
			break;
		case nel_O_RSHIFT:
			//added by zhangbin, 2006-5-19
			if(*((signed int*)right) < 0)
				intp_warning(eng, "right shift negative bits");
			else if(type->simple.size < *((signed int *)right))
				intp_warning(eng, "right shift too many bits");
			//end
			*((signed_int *) result) = (*((signed_int *) left)) >> (*((signed_int *) right));
			break;
		case nel_O_SUB:
			*((signed_int *) result) = (*((signed_int *) left)) - (*((signed_int *) right));
			break;
		default:
			intp_fatal_error (eng, "(intp_op_incl.h #1): illegal type for op %O: %D", O_token, D_token);
		}
		break;
	case nel_D_FLOAT:
		switch (O_token) {
		case nel_O_ADD:
			*((float *) result) = (*((float *) left)) + (*((float *) right));
			break;
		case nel_O_DIV:
			//added by zhangbin, to check divide oprand
			if(check_intp_zero(eng, type, right))
			{
				intp_stmt_error(eng, "divided by zero");
				break;
			}
			*((float *) result) = (*((float *) left)) / (*((float *) right));
			break;
		case nel_O_EQ:
			*((int *) result) = (*((float *) left)) == (*((float *) right));
			break;
		case nel_O_GE:
			*((int *) result) = (*((float *) left)) >= (*((float *) right));
			break;
		case nel_O_GT:
			*((int *) result) = (*((float *) left)) > (*((float *) right));
			break;
		case nel_O_LE:
			*((int *) result) = (*((float *) left)) <= (*((float *) right));
			break;
		case nel_O_LT:
			*((int *) result) = (*((float *) left)) < (*((float *) right));
			break;
		case nel_O_MULT:
			*((float *) result) = (*((float *) left)) * (*((float *) right));
			break;
		case nel_O_NE:
			*((int *) result) = (*((float *) left)) != (*((float *) right));
			break;
		case nel_O_NEG:
			*((float *) result) =  - (*((float *) left));
			break;
		case nel_O_POST_DEC:
			*((float *) result) = (*((float *) left)) --;
			break;
		case nel_O_POST_INC:
			*((float *) result) = (*((float *) left)) ++;
			break;
		case nel_O_POS:
			*((float *) result) =    (*((float *) left));
			break;
		case nel_O_PRE_DEC:
			*((float *) result) = -- (*((float *) left));
			break;
		case nel_O_PRE_INC:
			*((float *) result) = ++ (*((float *) left));
			break;
		case nel_O_SUB:
			*((float *) result) = (*((float *) left)) - (*((float *) right));
			break;
		default:
			intp_fatal_error (eng, "(intp_op_incl.h #1): illegal type for op %O: %D", O_token, D_token);
		}
		break;
	case nel_D_LONG_COMPLEX_DOUBLE:
		switch (O_token) {
		case nel_O_ADD:
			*nel_real_part (result, long_double) = (*nel_real_part (left, long_double)) + (*nel_real_part (right, long_double));
			*nel_imaginary_part (result, long_double) = (*nel_imaginary_part (left, long_double)) + (*nel_real_part (right, long_double));
			break;
		case nel_O_DIV:
			*nel_real_part (result, long_double) = ((*nel_real_part (left, long_double)) * (*nel_real_part (right, long_double)) + (*nel_imaginary_part (left, long_double)) * (*nel_imaginary_part (right, long_double))) / ((*nel_real_part (right, long_double)) * (*nel_real_part (right, long_double)) + (*nel_imaginary_part (right, long_double)) * (*nel_imaginary_part (right, long_double)));
			*nel_imaginary_part (result, long_double) = ((*nel_imaginary_part (left, long_double)) * (*nel_real_part (right, long_double)) - (*nel_real_part (left, long_double)) * (*nel_imaginary_part (right, long_double))) / ((*nel_real_part (right, long_double)) * (*nel_real_part (right, long_double)) + (*nel_imaginary_part (right, long_double)) * (*nel_imaginary_part (right, long_double)));
			break;
		case nel_O_EQ:
			*((int *) result) = ((*nel_real_part (left, long_double)) == (*nel_real_part (right, long_double))) && ((*nel_imaginary_part (left, long_double)) == (*nel_imaginary_part (right, long_double)));
			break;
		case nel_O_MULT:
			*nel_real_part (result, long_double) = (*nel_real_part (left, long_double)) * (*nel_real_part (right, long_double)) - (*nel_imaginary_part (left, long_double)) * (*nel_imaginary_part (right, long_double));
			*nel_imaginary_part (result, long_double) = (*nel_real_part (left, long_double)) * (*nel_imaginary_part (right, long_double)) - (*nel_imaginary_part (left, long_double)) * (*nel_real_part (right, long_double));
			break;
		case nel_O_NE:
			*((int *) result) = ((*nel_real_part (left, long_double)) != (*nel_real_part (right, long_double))) && ((*nel_imaginary_part (left, long_double)) != (*nel_imaginary_part (right, long_double)));
			break;
		case nel_O_NEG:
			*nel_real_part (result, long_double) = - (*nel_real_part (left, long_double));
			*nel_imaginary_part (result, long_double) = - (*nel_imaginary_part (left, long_double));
			break;
		case nel_O_POS:
			*nel_real_part (result, long_double) =   (*nel_real_part (left, long_double));
			*nel_imaginary_part (result, long_double) =   (*nel_imaginary_part (left, long_double));
			break;
		case nel_O_SUB:
			*nel_real_part (result, long_double) = (*nel_real_part (left, long_double)) - (*nel_real_part (right, long_double));
			*nel_imaginary_part (result, long_double) = (*nel_imaginary_part (left, long_double)) - (*nel_real_part (right, long_double));
			break;
		default:
			intp_fatal_error (eng, "(intp_op_incl.h #1): illegal type for op %O: %D", O_token, D_token);
		}
		break;
	case nel_D_LONG_DOUBLE:
		switch (O_token) {
		case nel_O_ADD:
			*((long_double *) result) = (*((long_double *) left)) + (*((long_double *) right));
			break;
		case nel_O_DIV:
			//added by zhangbin, to check divide oprand
			if(check_intp_zero(eng, type, right))
			{
				intp_stmt_error(eng, "divided by zero");
				break;
			}
			*((long_double *) result) = (*((long_double *) left)) / (*((long_double *) right));
			break;
		case nel_O_EQ:
			*((int *) result) = (*((long_double *) left)) == (*((long_double *) right));
			break;
		case nel_O_GE:
			*((int *) result) = (*((long_double *) left)) >= (*((long_double *) right));
			break;
		case nel_O_GT:
			*((int *) result) = (*((long_double *) left)) > (*((long_double *) right));
			break;
		case nel_O_LE:
			*((int *) result) = (*((long_double *) left)) <= (*((long_double *) right));
			break;
		case nel_O_LT:
			*((int *) result) = (*((long_double *) left)) < (*((long_double *) right));
			break;
		case nel_O_MULT:
			*((long_double *) result) = (*((long_double *) left)) * (*((long_double *) right));
			break;
		case nel_O_NE:
			*((int *) result) = (*((long_double *) left)) != (*((long_double *) right));
			break;
		case nel_O_NEG:
			*((long_double *) result) =  - (*((long_double *) left));
			break;
		case nel_O_POST_DEC:
			*((long_double *) result) = (*((long_double *) left)) --;
			break;
		case nel_O_POST_INC:
			*((long_double *) result) = (*((long_double *) left)) ++;
			break;
		case nel_O_POS:
			*((long_double *) result) =    (*((long_double *) left));
			break;
		case nel_O_PRE_DEC:
			*((long_double *) result) = -- (*((long_double *) left));
			break;
		case nel_O_PRE_INC:
			*((long_double *) result) = ++ (*((long_double *) left));
			break;
		case nel_O_SUB:
			*((long_double *) result) = (*((long_double *) left)) - (*((long_double *) right));
			break;
		default:
			intp_fatal_error (eng, "(intp_op_incl.h #1): illegal type for op %O: %D", O_token, D_token);
		}
		break;
	case nel_D_SIGNED_SHORT_INT:
		switch (O_token) {
		case nel_O_POST_DEC:
			*((signed_short_int *) result) = (*((signed_short_int *) left)) --;
			break;
		case nel_O_POST_INC:
			*((signed_short_int *) result) = (*((signed_short_int *) left)) ++;
			break;
		case nel_O_PRE_DEC:
			*((signed_short_int *) result) = -- (*((signed_short_int *) left));
			break;
		case nel_O_PRE_INC:
			*((signed_short_int *) result) = ++ (*((signed_short_int *) left));
			break;
		default:
			intp_fatal_error (eng, "(intp_op_incl.h #1): illegal type for op %O: %D", O_token, D_token);
		}
		break;
	case nel_D_UNSIGNED_INT:
		switch (O_token) {
		case nel_O_ADD:
			*((unsigned_int *) result) = (*((unsigned_int *) left)) + (*((unsigned_int *) right));
			break;
		case nel_O_BIT_AND:
			*((unsigned_int *) result) = (*((unsigned_int *) left)) & (*((unsigned_int *) right));
			break;
		case nel_O_BIT_OR:
			*((unsigned_int *) result) = (*((unsigned_int *) left)) | (*((unsigned_int *) right));
			break;
		case nel_O_BIT_NEG:
			*((unsigned_int *) result) = ~ (*((unsigned_int *) left));
			break;
		case nel_O_BIT_XOR:
			*((unsigned_int *) result) = (*((unsigned_int *) left)) ^ (*((unsigned_int *) right));
			break;
		case nel_O_DIV:
			//added by zhangbin, to check divide oprand
			if(check_intp_zero(eng, type, right))
			{
				intp_stmt_error(eng, "divided by zero");
				break;
			}
			*((unsigned_int *) result) = (*((unsigned_int *) left)) / (*((unsigned_int *) right));
			break;
		case nel_O_EQ:
			*((int *) result) = (*((unsigned_int *) left)) == (*((unsigned_int *) right));
			break;
		case nel_O_GE:
			*((int *) result) = (*((unsigned_int *) left)) >= (*((unsigned_int *) right));
			break;
		case nel_O_GT:
			*((int *) result) = (*((unsigned_int *) left)) > (*((unsigned_int *) right));
			break;
		case nel_O_LE:
			*((int *) result) = (*((unsigned_int *) left)) <= (*((unsigned_int *) right));
			break;
		case nel_O_LSHIFT:
			//added by zhangbin, 2006-5-19
			if(*((signed int*)right) < 0)
				intp_warning(eng, "left shift negative bits");
			else if(type->simple.size < *((signed int *)right))
				intp_warning(eng, "left shift too many bits");
			//end
			*((unsigned_int *) result) = (*((unsigned_int *) left)) << (*((unsigned_int *) right));
			break;
		case nel_O_LT:
			*((int *) result) = (*((unsigned_int *) left)) < (*((unsigned_int *) right));
			break;
		case nel_O_MOD:
			*((unsigned_int *) result) = (*((unsigned_int *) left)) % (*((unsigned_int *) right));
			break;
		case nel_O_MULT:
			*((unsigned_int *) result) = (*((unsigned_int *) left)) * (*((unsigned_int *) right));
			break;
		case nel_O_NE:
			*((int *) result) = (*((unsigned_int *) left)) != (*((unsigned_int *) right));
			break;
		case nel_O_NEG:
			*((unsigned_int *) result) =  - (*((unsigned_int *) left));
			break;
		case nel_O_POST_DEC:
			*((unsigned_int *) result) = (*((unsigned_int *) left)) --;
			break;
		case nel_O_POST_INC:
			*((unsigned_int *) result) = (*((unsigned_int *) left)) ++;
			break;
		case nel_O_POS:
			*((unsigned_int *) result) =    (*((unsigned_int *) left));
			break;
		case nel_O_PRE_DEC:
			*((unsigned_int *) result) = -- (*((unsigned_int *) left));
			break;
		case nel_O_PRE_INC:
			*((unsigned_int *) result) = ++ (*((unsigned_int *) left));
			break;
		case nel_O_RSHIFT:
			//added by zhangbin, 2006-5-19
			if(*((signed int*)right) < 0)
				intp_warning(eng, "right shift negative bits");
			else if(type->simple.size < *((signed int *)right))
				intp_warning(eng, "right shift too many bits");
			//end
			*((unsigned_int *) result) = (*((unsigned_int *) left)) >> (*((unsigned_int *) right));
			break;
		case nel_O_SUB:
			*((unsigned_int *) result) = (*((unsigned_int *) left)) - (*((unsigned_int *) right));
			break;
		default:
			intp_fatal_error (eng, "(intp_op_incl.h #1): illegal type for op %O: %D", O_token, D_token);
		}
		break;
	case nel_D_UNSIGNED_CHAR:
		switch (O_token) {
		case nel_O_POST_DEC:
			*((unsigned_char *) result) = (*((unsigned_char *) left)) --;
			break;
		case nel_O_POST_INC:
			*((unsigned_char *) result) = (*((unsigned_char *) left)) ++;
			break;
		case nel_O_PRE_DEC:
			*((unsigned_char *) result) = -- (*((unsigned_char *) left));
			break;
		case nel_O_PRE_INC:
			*((unsigned_char *) result) = ++ (*((unsigned_char *) left));
			break;
		default:
			intp_fatal_error (eng, "(intp_op_incl.h #1): illegal type for op %O: %D", O_token, D_token);
		}
		break;
	case nel_D_UNSIGNED_SHORT_INT:
		switch (O_token) {
		case nel_O_POST_DEC:
			*((unsigned_short_int *) result) = (*((unsigned_short_int *) left)) --;
			break;
		case nel_O_POST_INC:
			*((unsigned_short_int *) result) = (*((unsigned_short_int *) left)) ++;
			break;
		case nel_O_PRE_DEC:
			*((unsigned_short_int *) result) = -- (*((unsigned_short_int *) left));
			break;
		case nel_O_PRE_INC:
			*((unsigned_short_int *) result) = ++ (*((unsigned_short_int *) left));
			break;
		default:
			intp_fatal_error (eng, "(intp_op_incl.h #1): illegal type for op %O: %D", O_token, D_token);
		}
		break;

	case nel_D_SIGNED_LONG_INT:
		switch (O_token) {
		case nel_O_ADD:
			*((signed_long_int *) result) = (*((signed_long_int *) left)) + (*((signed_long_int *) right));
			break;
		case nel_O_BIT_AND:
			*((signed_long_int *) result) = (*((signed_long_int *) left)) & (*((signed_long_int *) right));
			break;
		case nel_O_BIT_OR:
			*((signed_long_int *) result) = (*((signed_long_int *) left)) | (*((signed_long_int *) right));
			break;
		case nel_O_BIT_NEG:
			*((signed_long_int *) result) = ~ (*((signed_long_int *) left));
			break;
		case nel_O_BIT_XOR:
			*((signed_long_int *) result) = (*((signed_long_int *) left)) ^ (*((signed_long_int *) right));
			break;
		case nel_O_DIV:
			//added by zhangbin, to check divide oprand
			if(check_intp_zero(eng, type, right))
			{
				intp_stmt_error(eng, "divided by zero");
				break;
			}
			*((signed_long_int *) result) = (*((signed_long_int *) left)) / (*((signed_long_int *) right));
			break;
		case nel_O_EQ:
			*((int *) result) = (*((signed_long_int *) left)) == (*((signed_long_int *) right));
			break;
		case nel_O_GE:
			*((int *) result) = (*((signed_long_int *) left)) >= (*((signed_long_int *) right));
			break;
		case nel_O_GT:
			*((int *) result) = (*((signed_long_int *) left)) > (*((signed_long_int *) right));
			break;
		case nel_O_LE:
			*((int *) result) = (*((signed_long_int *) left)) <= (*((signed_long_int *) right));
			break;
		case nel_O_LSHIFT:
			//added by zhangbin, 2006-5-19
			if(*((signed int*)right) < 0)
				intp_warning(eng, "left shift negative bits");
			else if(type->simple.size < *((signed int *)right))
				intp_warning(eng, "left shift too many bits");
			//end
			*((signed_long_int *) result) = (*((signed_long_int *) left)) << (*((signed_long_int *) right));
			break;
		case nel_O_LT:
			*((int *) result) = (*((signed_long_int *) left)) < (*((signed_long_int *) right));
			break;
		case nel_O_MOD:
			*((signed_long_int *) result) = (*((signed_long_int *) left)) % (*((signed_long_int *) right));
			break;
		case nel_O_MULT:
			*((signed_long_int *) result) = (*((signed_long_int *) left)) * (*((signed_long_int *) right));
			break;
		case nel_O_NE:
			*((int *) result) = (*((signed_long_int *) left)) != (*((signed_long_int *) right));
			break;
		case nel_O_NEG:
			*((signed_long_int *) result) =  - (*((signed_long_int *) left));
			break;
		case nel_O_POST_DEC:
			*((signed_long_int *) result) = (*((signed_long_int *) left)) --;
			break;
		case nel_O_POST_INC:
			*((signed_long_int *) result) = (*((signed_long_int *) left)) ++;
			break;
		case nel_O_POS:
			*((signed_long_int *) result) =    (*((signed_long_int *) left));
			break;
		case nel_O_PRE_DEC:
			*((signed_long_int *) result) = -- (*((signed_long_int *) left));
			break;
		case nel_O_PRE_INC:
			*((signed_long_int *) result) = ++ (*((signed_long_int *) left));
			break;
		case nel_O_RSHIFT:
			//added by zhangbin, 2006-5-19
			if(*((signed_long_int*)right) < 0)
				intp_warning(eng, "right shift negative bits");
			else if(type->simple.size < *((signed_long_int *)right))
				intp_warning(eng, "right shift too many bits");
			//end
			*((signed_long_int *) result) = (*((signed_long_int *) left)) >> (*((signed_long_int *) right));
			break;
		case nel_O_SUB:
			*((signed_long_int *) result) = (*((signed_long_int *) left)) - (*((signed_long_int *) right));
			break;
		default:
			intp_fatal_error (eng, "(intp_op_incl.h #1): illegal type for op %O: %D", O_token, D_token);
		}
		break;
	case nel_D_UNSIGNED_LONG_INT:
		switch (O_token) {
		case nel_O_ADD:
			*((unsigned_long_int *) result) = (*((unsigned_long_int *) left)) + (*((unsigned_long_int *) right));
			break;
		case nel_O_BIT_AND:
			*((unsigned_long_int *) result) = (*((unsigned_long_int *) left)) & (*((unsigned_long_int *) right));
			break;
		case nel_O_BIT_OR:
			*((unsigned_long_int *) result) = (*((unsigned_long_int *) left)) | (*((unsigned_long_int *) right));
			break;
		case nel_O_BIT_NEG:
			*((unsigned_long_int *) result) = ~ (*((unsigned_long_int *) left));
			break;
		case nel_O_BIT_XOR:
			*((unsigned_long_int *) result) = (*((unsigned_long_int *) left)) ^ (*((unsigned_long_int *) right));
			break;
		case nel_O_DIV:
			//added by zhangbin, to check divide oprand
			if(check_intp_zero(eng, type, right))
			{
				intp_stmt_error(eng, "divided by zero");
				break;
			}
			*((unsigned_long_int *) result) = (*((unsigned_long_int *) left)) / (*((unsigned_long_int *) right));
			break;
		case nel_O_EQ:
			*((int *) result) = (*((unsigned_long_int *) left)) == (*((unsigned_long_int *) right));
			break;
		case nel_O_GE:
			*((int *) result) = (*((unsigned_long_int *) left)) >= (*((unsigned_long_int *) right));
			break;
		case nel_O_GT:
			*((int *) result) = (*((unsigned_long_int *) left)) > (*((unsigned_long_int *) right));
			break;
		case nel_O_LE:
			*((int *) result) = (*((unsigned_long_int *) left)) <= (*((unsigned_long_int *) right));
			break;
		case nel_O_LSHIFT:
			//added by zhangbin, 2006-5-19
			if(*((signed int*)right) < 0)
				intp_warning(eng, "left shift negative bits");
			else if(type->simple.size < *((signed int *)right))
				intp_warning(eng, "left shift too many bits");
			//end
			*((unsigned_long_int *) result) = (*((unsigned_long_int *) left)) << (*((unsigned_long_int *) right));
			break;
		case nel_O_LT:
			*((int *) result) = (*((unsigned_long_int *) left)) < (*((unsigned_long_int *) right));
			break;
		case nel_O_MOD:
			*((unsigned_long_int *) result) = (*((unsigned_long_int *) left)) % (*((unsigned_long_int *) right));
			break;
		case nel_O_MULT:
			*((unsigned_long_int *) result) = (*((unsigned_long_int *) left)) * (*((unsigned_long_int *) right));
			break;
		case nel_O_NE:
			*((int *) result) = (*((unsigned_long_int *) left)) != (*((unsigned_long_int *) right));
			break;
		case nel_O_NEG:
			*((unsigned_long_int *) result) =  - (*((unsigned_long_int *) left));
			break;
		case nel_O_POST_DEC:
			*((unsigned_long_int *) result) = (*((unsigned_long_int *) left)) --;
			break;
		case nel_O_POST_INC:
			*((unsigned_long_int *) result) = (*((unsigned_long_int *) left)) ++;
			break;
		case nel_O_POS:
			*((unsigned_long_int *) result) =    (*((unsigned_long_int *) left));
			break;
		case nel_O_PRE_DEC:
			*((unsigned_long_int *) result) = -- (*((unsigned_long_int *) left));
			break;
		case nel_O_PRE_INC:
			*((unsigned_long_int *) result) = ++ (*((unsigned_long_int *) left));
			break;
		case nel_O_RSHIFT:
			//added by zhangbin, 2006-5-19
			if(*((unsigned_long_int*)right) < 0)
				intp_warning(eng, "right shift negative bits");
			else if(type->simple.size < *((unsigned_long_int *)right))
				intp_warning(eng, "right shift too many bits");
			//end
			*((unsigned_long_int *) result) = (*((unsigned_long_int *) left)) >> (*((unsigned_long_int *) right));
			break;
		case nel_O_SUB:
			*((unsigned_long_int *) result) = (*((unsigned_long_int *) left)) - (*((unsigned_long_int *) right));
			break;
		default:
			intp_fatal_error (eng, "(intp_op_incl.h #1): illegal type for op %O: %D", O_token, D_token);
		}
		break;
	default:
		intp_fatal_error (eng, "(intp_op #5): bad O_token: %O", O_token);
		break;
	}

	nel_debug ({ intp_trace (eng, "] exiting intp_op\n\n"); });
}



/*****************************************************************************/
/* intp_type_unary_op returns the type of the result of a unary operation */
/* upon a numerical operand with type <type>.  generally, this is either the */
/* type of the operation, or NULL for error.  if <integral> is true, then    */
/* floating point types are not allowed.  if <cmplx> is true, then complex   */
/* operands are allowed.                                                     */
/*****************************************************************************/
nel_type *intp_type_unary_op (struct nel_eng *eng, register nel_type *type, register unsigned_int integral, register unsigned_int cmplx)
{
	register nel_D_token D_token;
	register nel_type *retval;

	nel_debug ({
				   intp_trace (eng, "entering intp_type_unary_op [\ntype =\n%1Tintegral = %d\ncmplx = %d\n\n", type, integral, cmplx);
			   });

	if (type == NULL) {
		retval = NULL;
	} else {

		D_token = type->simple.type;

		switch (D_token) {

		case nel_D_LONG_COMPLEX_DOUBLE:
			if (integral || (! cmplx)) {
				retval = NULL;
			} else {
				retval = nel_long_complex_double_type;
			}
			break;

		case nel_D_COMPLEX_DOUBLE:
		case nel_D_LONG_COMPLEX:
		case nel_D_LONG_COMPLEX_FLOAT:
			if (integral || (! cmplx)) {
				retval = NULL;
			} else {
				retval = nel_complex_double_type;
			}
			break;

		case nel_D_COMPLEX:
		case nel_D_COMPLEX_FLOAT:
			if (integral || (! cmplx)) {
				retval = NULL;
			} else {
				retval = nel_complex_float_type;
			}
			break;

		case nel_D_LONG_DOUBLE:
			if (integral) {
				retval = NULL;
			} else {
				retval = nel_long_double_type;
			}
			break;

		case nel_D_DOUBLE:
		case nel_D_LONG_FLOAT:
			if (integral) {
				retval = NULL;
			} else {
				retval = nel_double_type;
			}
			break;

		case nel_D_FLOAT:
			if (integral) {
				retval = NULL;
			} else {
				retval = nel_float_type;
			}
			break;

		case nel_D_UNSIGNED_LONG:
		case nel_D_UNSIGNED_LONG_INT:
			retval = nel_unsigned_long_int_type;
			break;

		case nel_D_LONG:
		case nel_D_LONG_INT:
		case nel_D_SIGNED_LONG:
		case nel_D_SIGNED_LONG_INT:
			retval = nel_signed_long_int_type;
			break;

			/***************************************************/
			/* the following are integral promoted to unsigned */
			/***************************************************/
		case nel_D_UNSIGNED:
		case nel_D_UNSIGNED_INT:
unsigned_x:
			retval = nel_unsigned_int_type;
			break;

			/*****************************************************/
			/* the following are integral promoted to signed int */
			/*****************************************************/
		case nel_D_CHAR:
		case nel_D_ENUM:
		case nel_D_INT:
		case nel_D_SHORT:
		case nel_D_SHORT_INT:
		case nel_D_SIGNED:
		case nel_D_SIGNED_CHAR:
		case nel_D_SIGNED_INT:
		case nel_D_SIGNED_SHORT:
		case nel_D_SIGNED_SHORT_INT:
signed_x:
			retval = nel_signed_int_type;
			break;

			/****************************************************/
			/* the following are integral promoted to signed or */
			/* unsigned int, depending upon implementation.     */
			/****************************************************/
		case nel_D_UNSIGNED_CHAR:
			if (sizeof (unsigned_char) < sizeof (int)) {
				goto signed_x;
			} else {
				goto unsigned_x;
			}

		case nel_D_UNSIGNED_SHORT:
		case nel_D_UNSIGNED_SHORT_INT:
			if (sizeof (unsigned_short_int) < sizeof (int)) {
				goto signed_x;
			} else {
				goto unsigned_x;
			}

		default:
			retval = NULL;
			break;
		}
	}

	nel_debug ({
				   intp_trace (eng, "] exiting intp_type_unary_op\nretval =\n%1T\n", retval);
			   });

	return (retval);
}


/*****************************************************************************/
/* intp_eval_unary_op () perfoms the semantic actions for a unary         */
/* operation.  <op> is the nel_O_token for the operation to be           */
/* performed. <operand> is the symbol for the operand.                       */
/*****************************************************************************/
nel_symbol *intp_eval_unary_op (struct nel_eng *eng, nel_O_token op, register nel_symbol *operand)
{
	register nel_type *type;
	register nel_D_token D_token;
	nel_symbol *retval;

	nel_debug ({
				   intp_trace (eng, "entering intp_eval_unary_op [\neng = 0x%x\nop = %O\n\n", eng, op);
			   });

	nel_debug ({
				   if ((operand == NULL) || (operand->type == NULL)) {
				   intp_fatal_error (eng, "(intp_eval_unary_op #1): bad operand operand\n%1S", operand)
					   ;
				   }
			   });

	if (operand->value == NULL) {
		if (operand->name == NULL) {
			intp_stmt_error (eng, "NULL address for operand of %P", op);
		} else {
			intp_stmt_error (eng, "NULL address for %I", operand);
		}
	}

	type = operand->type;
	D_token = type->simple.type;

	if (nel_s_u_D_token (D_token)) {
		intp_stmt_error (eng, "%P is not a permitted struct/union operation", op);
	} else {
		/***********************************************/
		/* we have a numerical type, if all is correct */
		/***********************************************/
		register nel_type *res_type;
		char *promo;

		/*******************************/
		/* find the type of the result */
		/*******************************/
		res_type = intp_type_unary_op (eng, type, nel_integral_O_token (op), nel_complex_O_token (op));
		if (res_type == NULL) {
			intp_stmt_error (eng, "illegal operand of %P", op);
		}
		nel_debug ({
					   if (res_type->simple.size <= 0) {
					   intp_fatal_error (eng, "(intp_eval_unary_op #2): bad result type\n%1T", res_type)
						   ;
					   }
				   });

		/*****************************************/
		/* coerce the operand to the result type */
		/*****************************************/
		promo = intp_dyn_value_alloc (eng, res_type->simple.size, res_type->simple.alignment);
		intp_coerce (eng, res_type, promo, type, operand->value);

		/*******************************************/
		/* allocate a symbol and space for the     */
		/* the result, then perform the operation. */
		/*******************************************/
		retval = intp_dyn_symbol_alloc (eng, NULL, res_type, NULL, nel_C_RESULT, 0, nel_L_NEL, eng->intp->level);
		retval->value = intp_dyn_value_alloc (eng, res_type->simple.size, res_type->simple.alignment);
		intp_op (eng, op, res_type, retval->value, promo, NULL);
	}

	nel_debug ({
				   intp_trace (eng, "] exiting intp_eval_unary_op\nretval =\n%1S\n", retval);
			   });

	return (retval);
}


/*****************************************************************************/
/* intp_type_binary_op returns the type of the result of a binary         */
/* operation upon operands with types <type1> and <type2>. if <integral> is  */
/* true, then floating point types are not allowed.  if <cmplx> is true,     */
/* then complex operands are allowed.                                        */
/*****************************************************************************/
nel_type *intp_type_binary_op (struct nel_eng *eng, register nel_type *type1, register nel_type *type2, register unsigned_int integral, register unsigned_int cmplx)
{
	register nel_D_token D_token1;
	register nel_D_token D_token2;
	register nel_type *retval;

	nel_debug ({
				   intp_trace (eng, "entering intp_type_binary_op [\ntype1 =\n%1Ttype2 =\n%1Tintegral = %d\ncmplx = %d\n\n", type1, type2, integral, cmplx);
			   });

	if ((type1 == NULL) || (type2 == NULL)) {
		retval = NULL;
	} else {

		D_token1 = type1->simple.type;
		D_token2 = type2->simple.type;

		switch (D_token1) {


		case nel_D_LONG_COMPLEX_DOUBLE:

			if (integral || (! cmplx)) {
				retval = NULL;
			} else {
				switch (D_token2) {

				case nel_D_CHAR:
				case nel_D_COMPLEX:
				case nel_D_COMPLEX_DOUBLE:
				case nel_D_COMPLEX_FLOAT:
				case nel_D_DOUBLE:
				case nel_D_ENUM:
				case nel_D_FLOAT:
				case nel_D_INT:
				case nel_D_LONG:
				case nel_D_LONG_COMPLEX:
				case nel_D_LONG_COMPLEX_DOUBLE:
				case nel_D_LONG_COMPLEX_FLOAT:
				case nel_D_LONG_DOUBLE:
				case nel_D_LONG_FLOAT:
				case nel_D_LONG_INT:
				case nel_D_SHORT:
				case nel_D_SHORT_INT:
				case nel_D_SIGNED:
				case nel_D_SIGNED_CHAR:
				case nel_D_SIGNED_INT:
				case nel_D_SIGNED_LONG:
				case nel_D_SIGNED_LONG_INT:
				case nel_D_SIGNED_SHORT:
				case nel_D_SIGNED_SHORT_INT:
				case nel_D_UNSIGNED:
				case nel_D_UNSIGNED_CHAR:
				case nel_D_UNSIGNED_INT:
				case nel_D_UNSIGNED_LONG:
				case nel_D_UNSIGNED_LONG_INT:
				case nel_D_UNSIGNED_SHORT:
				case nel_D_UNSIGNED_SHORT_INT:
					retval = nel_long_complex_double_type;
					break;

				default:
					retval = NULL;
					break;
				}
			}
			break;


		case nel_D_COMPLEX_DOUBLE:
		case nel_D_LONG_COMPLEX:
		case nel_D_LONG_COMPLEX_FLOAT:

			if (integral || (! cmplx)) {
				retval = NULL;
			} else {
				switch (D_token2) {

				case nel_D_LONG_COMPLEX_DOUBLE:
				case nel_D_LONG_DOUBLE:
					retval = nel_long_complex_double_type;
					break;

				case nel_D_CHAR:
				case nel_D_COMPLEX:
				case nel_D_COMPLEX_DOUBLE:
				case nel_D_COMPLEX_FLOAT:
				case nel_D_DOUBLE:
				case nel_D_ENUM:
				case nel_D_FLOAT:
				case nel_D_INT:
				case nel_D_LONG:
				case nel_D_LONG_COMPLEX:
				case nel_D_LONG_COMPLEX_FLOAT:
				case nel_D_LONG_FLOAT:
				case nel_D_LONG_INT:
				case nel_D_SHORT:
				case nel_D_SHORT_INT:
				case nel_D_SIGNED:
				case nel_D_SIGNED_CHAR:
				case nel_D_SIGNED_INT:
				case nel_D_SIGNED_LONG:
				case nel_D_SIGNED_LONG_INT:
				case nel_D_SIGNED_SHORT:
				case nel_D_SIGNED_SHORT_INT:
				case nel_D_UNSIGNED:
				case nel_D_UNSIGNED_CHAR:
				case nel_D_UNSIGNED_INT:
				case nel_D_UNSIGNED_LONG:
				case nel_D_UNSIGNED_LONG_INT:
				case nel_D_UNSIGNED_SHORT:
				case nel_D_UNSIGNED_SHORT_INT:
					retval = nel_complex_double_type;
					break;

				default:
					retval = NULL;
					break;
				}
			}
			break;


		case nel_D_COMPLEX:
		case nel_D_COMPLEX_FLOAT:

			if (integral || (! cmplx)) {
				retval = NULL;
			} else {
				switch (D_token2) {

				case nel_D_LONG_COMPLEX_DOUBLE:
				case nel_D_LONG_DOUBLE:
					retval = nel_long_complex_double_type;
					break;

				case nel_D_COMPLEX_DOUBLE:
				case nel_D_DOUBLE:
				case nel_D_LONG_COMPLEX:
				case nel_D_LONG_COMPLEX_FLOAT:
				case nel_D_LONG_FLOAT:
					retval = nel_complex_double_type;
					break;

				case nel_D_CHAR:
				case nel_D_COMPLEX:
				case nel_D_COMPLEX_FLOAT:
				case nel_D_ENUM:
				case nel_D_FLOAT:
				case nel_D_INT:
				case nel_D_LONG:
				case nel_D_LONG_INT:
				case nel_D_SHORT:
				case nel_D_SHORT_INT:
				case nel_D_SIGNED:
				case nel_D_SIGNED_CHAR:
				case nel_D_SIGNED_INT:
				case nel_D_SIGNED_LONG:
				case nel_D_SIGNED_LONG_INT:
				case nel_D_SIGNED_SHORT:
				case nel_D_SIGNED_SHORT_INT:
				case nel_D_UNSIGNED:
				case nel_D_UNSIGNED_CHAR:
				case nel_D_UNSIGNED_INT:
				case nel_D_UNSIGNED_LONG:
				case nel_D_UNSIGNED_LONG_INT:
				case nel_D_UNSIGNED_SHORT:
				case nel_D_UNSIGNED_SHORT_INT:
					retval = nel_complex_float_type;
					break;

				default:
					retval = NULL;
					break;
				}
			}
			break;


		case nel_D_LONG_DOUBLE:

			if (integral) {
				retval = NULL;
			} else {
				switch (D_token2) {

				case nel_D_COMPLEX:
				case nel_D_COMPLEX_DOUBLE:
				case nel_D_COMPLEX_FLOAT:
				case nel_D_LONG_COMPLEX:
				case nel_D_LONG_COMPLEX_DOUBLE:
				case nel_D_LONG_COMPLEX_FLOAT:
					if (! cmplx) {
						retval = NULL;
					} else {
						retval = nel_long_complex_double_type;
					}
					break;

				case nel_D_CHAR:
				case nel_D_DOUBLE:
				case nel_D_ENUM:
				case nel_D_FLOAT:
				case nel_D_INT:
				case nel_D_LONG:
				case nel_D_LONG_DOUBLE:
				case nel_D_LONG_FLOAT:
				case nel_D_LONG_INT:
				case nel_D_SHORT:
				case nel_D_SHORT_INT:
				case nel_D_SIGNED:
				case nel_D_SIGNED_CHAR:
				case nel_D_SIGNED_INT:
				case nel_D_SIGNED_LONG:
				case nel_D_SIGNED_LONG_INT:
				case nel_D_SIGNED_SHORT:
				case nel_D_SIGNED_SHORT_INT:
				case nel_D_UNSIGNED:
				case nel_D_UNSIGNED_CHAR:
				case nel_D_UNSIGNED_INT:
				case nel_D_UNSIGNED_LONG:
				case nel_D_UNSIGNED_LONG_INT:
				case nel_D_UNSIGNED_SHORT:
				case nel_D_UNSIGNED_SHORT_INT:
					retval = nel_long_double_type;
					break;

				default:
					retval = NULL;
					break;
				}
			}
			break;


		case nel_D_DOUBLE:
		case nel_D_LONG_FLOAT:

			if (integral) {
				retval = NULL;
			} else {
				switch (D_token2) {

				case nel_D_LONG_COMPLEX_DOUBLE:
					if (! cmplx) {
						retval = NULL;
					} else {
						retval = nel_long_complex_double_type;
					}
					break;

				case nel_D_COMPLEX:
				case nel_D_COMPLEX_DOUBLE:
				case nel_D_COMPLEX_FLOAT:
				case nel_D_LONG_COMPLEX:
				case nel_D_LONG_COMPLEX_FLOAT:
					if (! cmplx) {
						retval = NULL;
					} else {
						retval = nel_complex_double_type;
					}
					break;

				case nel_D_LONG_DOUBLE:
					retval = nel_long_double_type;
					break;

				case nel_D_CHAR:
				case nel_D_DOUBLE:
				case nel_D_ENUM:
				case nel_D_FLOAT:
				case nel_D_INT:
				case nel_D_LONG:
				case nel_D_LONG_FLOAT:
				case nel_D_LONG_INT:
				case nel_D_SHORT:
				case nel_D_SHORT_INT:
				case nel_D_SIGNED:
				case nel_D_SIGNED_CHAR:
				case nel_D_SIGNED_INT:
				case nel_D_SIGNED_LONG:
				case nel_D_SIGNED_LONG_INT:
				case nel_D_SIGNED_SHORT:
				case nel_D_SIGNED_SHORT_INT:
				case nel_D_UNSIGNED:
				case nel_D_UNSIGNED_CHAR:
				case nel_D_UNSIGNED_INT:
				case nel_D_UNSIGNED_LONG:
				case nel_D_UNSIGNED_LONG_INT:
				case nel_D_UNSIGNED_SHORT:
				case nel_D_UNSIGNED_SHORT_INT:
					retval = nel_double_type;
					break;

				default:
					retval = NULL;
					break;
				}
			}
			break;


		case nel_D_FLOAT:

			if (integral) {
				retval = NULL;
			} else {
				switch (D_token2) {

				case nel_D_LONG_COMPLEX_DOUBLE:
					if (! cmplx) {
						retval = NULL;
					} else {
						retval = nel_long_complex_double_type;
					}
					break;

				case nel_D_COMPLEX_DOUBLE:
				case nel_D_LONG_COMPLEX:
				case nel_D_LONG_COMPLEX_FLOAT:
					if (! cmplx) {
						retval = NULL;
					} else {
						retval = nel_complex_double_type;
					}
					break;

				case nel_D_COMPLEX:
				case nel_D_COMPLEX_FLOAT:
					if (! cmplx) {
						retval = NULL;
					} else {
						retval = nel_complex_float_type;
					}
					break;

				case nel_D_LONG_DOUBLE:
					retval = nel_long_double_type;
					break;

				case nel_D_DOUBLE:
				case nel_D_LONG_FLOAT:
					retval = nel_double_type;
					break;

				case nel_D_CHAR:
				case nel_D_ENUM:
				case nel_D_FLOAT:
				case nel_D_INT:
				case nel_D_LONG:
				case nel_D_LONG_INT:
				case nel_D_SHORT:
				case nel_D_SHORT_INT:
				case nel_D_SIGNED:
				case nel_D_SIGNED_CHAR:
				case nel_D_SIGNED_INT:
				case nel_D_SIGNED_LONG:
				case nel_D_SIGNED_LONG_INT:
				case nel_D_SIGNED_SHORT:
				case nel_D_SIGNED_SHORT_INT:
				case nel_D_UNSIGNED:
				case nel_D_UNSIGNED_CHAR:
				case nel_D_UNSIGNED_INT:
				case nel_D_UNSIGNED_LONG:
				case nel_D_UNSIGNED_LONG_INT:
				case nel_D_UNSIGNED_SHORT:
				case nel_D_UNSIGNED_SHORT_INT:
					retval = nel_float_type;
					break;

				default:
					retval = NULL;
					break;
				}
			}
			break;


		case nel_D_UNSIGNED_LONG:
		case nel_D_UNSIGNED_LONG_INT:

			switch (D_token2) {

			case nel_D_LONG_COMPLEX_DOUBLE:
				if (integral || (! cmplx)) {
					retval = NULL;
				} else {
					retval = nel_long_complex_double_type;
				}
				break;

			case nel_D_COMPLEX_DOUBLE:
			case nel_D_LONG_COMPLEX:
			case nel_D_LONG_COMPLEX_FLOAT:
				if (integral || (! cmplx)) {
					retval = NULL;
				} else {
					retval = nel_complex_double_type;
				}
				break;

			case nel_D_COMPLEX:
			case nel_D_COMPLEX_FLOAT:
				if (integral || (! cmplx)) {
					retval = NULL;
				} else {
					retval = nel_complex_float_type;
				}
				break;

			case nel_D_LONG_DOUBLE:
				if (integral) {
					retval = NULL;
				} else {
					retval = nel_long_double_type;
				}
				break;

			case nel_D_DOUBLE:
			case nel_D_LONG_FLOAT:
				if (integral) {
					retval = NULL;
				} else {
					retval = nel_double_type;
				}
				break;

			case nel_D_FLOAT:
				if (integral) {
					retval = NULL;
				} else {
					retval = nel_float_type;
				}
				break;

			case nel_D_CHAR:
			case nel_D_ENUM:
			case nel_D_INT:
			case nel_D_LONG:
			case nel_D_LONG_INT:
			case nel_D_SHORT:
			case nel_D_SHORT_INT:
			case nel_D_SIGNED:
			case nel_D_SIGNED_CHAR:
			case nel_D_SIGNED_INT:
			case nel_D_SIGNED_LONG:
			case nel_D_SIGNED_LONG_INT:
			case nel_D_SIGNED_SHORT:
			case nel_D_SIGNED_SHORT_INT:
			case nel_D_UNSIGNED:
			case nel_D_UNSIGNED_CHAR:
			case nel_D_UNSIGNED_INT:
			case nel_D_UNSIGNED_LONG:
			case nel_D_UNSIGNED_LONG_INT:
			case nel_D_UNSIGNED_SHORT:
			case nel_D_UNSIGNED_SHORT_INT:
				retval = nel_unsigned_long_int_type;
				break;

			default:
				retval = NULL;
				break;
			}

			break;


		case nel_D_LONG:
		case nel_D_LONG_INT:
		case nel_D_SIGNED_LONG:
		case nel_D_SIGNED_LONG_INT:

			switch (D_token2) {

			case nel_D_LONG_COMPLEX_DOUBLE:
				if (integral || (! cmplx)) {
					retval = NULL;
				} else {
					retval = nel_long_complex_double_type;
				}
				break;

			case nel_D_COMPLEX_DOUBLE:
			case nel_D_LONG_COMPLEX:
			case nel_D_LONG_COMPLEX_FLOAT:
				if (integral || (! cmplx)) {
					retval = NULL;
				} else {
					retval = nel_complex_double_type;
				}
				break;

			case nel_D_COMPLEX:
			case nel_D_COMPLEX_FLOAT:
				if (integral || (! cmplx)) {
					retval = NULL;
				} else {
					retval = nel_complex_float_type;
				}
				break;

			case nel_D_LONG_DOUBLE:
				if (integral) {
					retval = NULL;
				} else {
					retval = nel_long_double_type;
				}
				break;

			case nel_D_DOUBLE:
			case nel_D_LONG_FLOAT:
				if (integral) {
					retval = NULL;
				} else {
					retval = nel_double_type;
				}
				break;

			case nel_D_FLOAT:
				if (integral) {
					retval = NULL;
				} else {
					retval = nel_float_type;
				}
				break;

			case nel_D_UNSIGNED_LONG:
			case nel_D_UNSIGNED_LONG_INT:
				retval = nel_unsigned_long_int_type;
				break;

			case nel_D_LONG:
			case nel_D_LONG_INT:
			case nel_D_SIGNED_LONG:
			case nel_D_SIGNED_LONG_INT:
				retval = nel_signed_long_int_type;
				break;

				/*******************************************************/
				/* the following are integral promoted to unsigned int */
				/*******************************************************/
			case nel_D_UNSIGNED:
			case nel_D_UNSIGNED_INT:
long_x_unsigned:
				if (sizeof (unsigned_int) < sizeof (signed_long_int)) {
					retval = nel_signed_long_int_type;
				} else {
					retval = nel_unsigned_long_int_type;
				}
				break;

				/*****************************************************/
				/* the following are integral promoted to signed int */
				/*****************************************************/
			case nel_D_CHAR:
			case nel_D_ENUM:
			case nel_D_INT:
			case nel_D_SHORT:
			case nel_D_SHORT_INT:
			case nel_D_SIGNED:
			case nel_D_SIGNED_CHAR:
			case nel_D_SIGNED_INT:
			case nel_D_SIGNED_SHORT:
			case nel_D_SIGNED_SHORT_INT:
long_x_signed:
				retval = nel_signed_long_int_type;
				break;

				/****************************************************/
				/* the following are integral promoted to signed or */
				/* unsigned int, depending upon implementation.     */
				/****************************************************/
			case nel_D_UNSIGNED_CHAR:
				if (sizeof (unsigned_char) < sizeof (signed_int)) {
					goto long_x_signed;
				} else {
					goto long_x_unsigned;
				}

			case nel_D_UNSIGNED_SHORT:
			case nel_D_UNSIGNED_SHORT_INT:
				if (sizeof (unsigned_short_int) < sizeof (signed_int)) {
					goto long_x_signed;
				} else {
					goto long_x_unsigned;
				}

			default:
				retval = NULL;
				break;
			}

			break;


			/*******************************************************/
			/* the following are integral promoted to unsigned int */
			/*******************************************************/
		case nel_D_UNSIGNED:
		case nel_D_UNSIGNED_INT:
unsigned_x:

			switch (D_token2) {

			case nel_D_LONG_COMPLEX_DOUBLE:
				if (integral || (! cmplx)) {
					retval = NULL;
				} else {
					retval = nel_long_complex_double_type;
				}
				break;

			case nel_D_COMPLEX_DOUBLE:
			case nel_D_LONG_COMPLEX:
			case nel_D_LONG_COMPLEX_FLOAT:
				if (integral || (! cmplx)) {
					retval = NULL;
				} else {
					retval = nel_complex_double_type;
				}
				break;

			case nel_D_COMPLEX:
			case nel_D_COMPLEX_FLOAT:
				if (integral || (! cmplx)) {
					retval = NULL;
				} else {
					retval = nel_complex_float_type;
				}
				break;

			case nel_D_LONG_DOUBLE:
				if (integral) {
					retval = NULL;
				} else {
					retval = nel_long_double_type;
				}
				break;

			case nel_D_DOUBLE:
			case nel_D_LONG_FLOAT:
				if (integral) {
					retval = NULL;
				} else {
					retval = nel_double_type;
				}
				break;

			case nel_D_FLOAT:
				if (integral) {
					retval = NULL;
				} else {
					retval = nel_float_type;
				}
				break;

			case nel_D_UNSIGNED_LONG:
			case nel_D_UNSIGNED_LONG_INT:
				retval = nel_unsigned_long_int_type;
				break;

			case nel_D_LONG:
			case nel_D_LONG_INT:
			case nel_D_SIGNED_LONG:
			case nel_D_SIGNED_LONG_INT:
				if (sizeof (unsigned_int) < sizeof (signed_long_int)) {
					retval = nel_signed_long_int_type;
				} else {
					retval = nel_unsigned_long_int_type;
				}
				break;

				/****************************************************/
				/* the following are integral promoted to signed or */
				/* unsigned int, depending upon implementation.     */
				/****************************************************/
			case nel_D_CHAR:
			case nel_D_ENUM:
			case nel_D_INT:
			case nel_D_SHORT:
			case nel_D_SHORT_INT:
			case nel_D_SIGNED:
			case nel_D_SIGNED_CHAR:
			case nel_D_SIGNED_INT:
			case nel_D_SIGNED_SHORT:
			case nel_D_SIGNED_SHORT_INT:
			case nel_D_UNSIGNED:
			case nel_D_UNSIGNED_INT:
			case nel_D_UNSIGNED_CHAR:
			case nel_D_UNSIGNED_SHORT:
			case nel_D_UNSIGNED_SHORT_INT:
				retval = nel_unsigned_int_type;
				break;

			default:
				retval = NULL;
				break;
			}

			break;


			/*****************************************************/
			/* the following are integral promoted to signed int */
			/*****************************************************/
		case nel_D_CHAR:
		case nel_D_ENUM:
		case nel_D_INT:
		case nel_D_SHORT:
		case nel_D_SHORT_INT:
		case nel_D_SIGNED:
		case nel_D_SIGNED_CHAR:
		case nel_D_SIGNED_INT:
		case nel_D_SIGNED_SHORT:
		case nel_D_SIGNED_SHORT_INT:
signed_x:

			switch (D_token2) {

			case nel_D_LONG_COMPLEX_DOUBLE:
				if (integral || (! cmplx)) {
					retval = NULL;
				} else {
					retval = nel_long_complex_double_type;
				}
				break;

			case nel_D_COMPLEX_DOUBLE:
			case nel_D_LONG_COMPLEX:
			case nel_D_LONG_COMPLEX_FLOAT:
				if (integral || (! cmplx)) {
					retval = NULL;
				} else {
					retval = nel_complex_double_type;
				}
				break;

			case nel_D_COMPLEX:
			case nel_D_COMPLEX_FLOAT:
				if (integral || (! cmplx)) {
					retval = NULL;
				} else {
					retval = nel_complex_float_type;
				}
				break;

			case nel_D_LONG_DOUBLE:
				if (integral) {
					retval = NULL;
				} else {
					retval = nel_long_double_type;
				}
				break;

			case nel_D_DOUBLE:
			case nel_D_LONG_FLOAT:
				if (integral) {
					retval = NULL;
				} else {
					retval = nel_double_type;
				}
				break;

			case nel_D_FLOAT:
				if (integral) {
					retval = NULL;
				} else {
					retval = nel_float_type;
				}
				break;

			case nel_D_UNSIGNED_LONG:
			case nel_D_UNSIGNED_LONG_INT:
				retval = nel_unsigned_long_int_type;
				break;

			case nel_D_LONG:
			case nel_D_LONG_INT:
			case nel_D_SIGNED_LONG:
			case nel_D_SIGNED_LONG_INT:
				retval = nel_signed_long_int_type;
				break;

				/*******************************************************/
				/* the following are integral promoted to unsigned int */
				/*******************************************************/
			case nel_D_UNSIGNED:
			case nel_D_UNSIGNED_INT:
signed_x_unsigned:
				retval = nel_unsigned_int_type;
				break;

				/*****************************************************/
				/* the following are integral promoted to signed int */
				/*****************************************************/
			case nel_D_CHAR:
			case nel_D_ENUM:
			case nel_D_INT:
			case nel_D_SHORT:
			case nel_D_SHORT_INT:
			case nel_D_SIGNED:
			case nel_D_SIGNED_CHAR:
			case nel_D_SIGNED_INT:
			case nel_D_SIGNED_SHORT:
			case nel_D_SIGNED_SHORT_INT:
signed_x_signed:
				retval = nel_signed_int_type;
				break;

				/****************************************************/
				/* the following are integral promoted to signed or */
				/* unsigned int, depending upon implementation.     */
				/****************************************************/
			case nel_D_UNSIGNED_CHAR:
				if (sizeof (unsigned_char) < sizeof (signed_int)) {
					goto signed_x_signed;
				} else {
					goto signed_x_unsigned;
				}

			case nel_D_UNSIGNED_SHORT:
			case nel_D_UNSIGNED_SHORT_INT:
				if (sizeof (unsigned_short_int) < sizeof (signed_int)) {
					goto signed_x_signed;
				} else {
					goto signed_x_unsigned;
				}

			default:
				retval = NULL;
				break;
			}

			break;


			/****************************************************/
			/* the following are integral promoted to signed or */
			/* unsigned int, depending upon implementation.     */
			/****************************************************/
		case nel_D_UNSIGNED_CHAR:
			if (sizeof (unsigned_char) < sizeof (int)) {
				goto signed_x;
			} else {
				goto unsigned_x;
			}


		case nel_D_UNSIGNED_SHORT:
		case nel_D_UNSIGNED_SHORT_INT:
			if (sizeof (unsigned_short_int) < sizeof (int)) {
				goto signed_x;
			} else {
				goto unsigned_x;
			}
		default:
			retval = NULL;
			break;
		}
	}

	//added by zhangbin, 2006-5-26
	if(type1->simple.type == nel_D_POINTER)
	{
		switch(type2->simple.type)
			{
			case nel_D_CHAR:
			case nel_D_SIGNED_CHAR:
			case nel_D_UNSIGNED_CHAR:
			case nel_D_INT:
			case nel_D_SIGNED_INT:
			case nel_D_UNSIGNED_INT:
			case nel_D_SIGNED:
			case nel_D_UNSIGNED:
			case nel_D_LONG:
			case nel_D_UNSIGNED_LONG:
			case nel_D_SIGNED_LONG:
			case nel_D_POINTER:
			case nel_D_FUNCTION:
				retval = type1;
				break;
			default:
				retval = NULL;
			}
	}
	if(type1->simple.type == nel_D_ARRAY)
	{
			switch(type2->simple.type)
			{
			case nel_D_CHAR:
			case nel_D_SIGNED_CHAR:
			case nel_D_UNSIGNED_CHAR:
			case nel_D_INT:
			case nel_D_SIGNED_INT:
			case nel_D_UNSIGNED_INT:
			case nel_D_SIGNED:
			case nel_D_UNSIGNED:
			case nel_D_LONG:
			case nel_D_UNSIGNED_LONG:
			case nel_D_SIGNED_LONG:
				retval = nel_type_alloc(eng, nel_D_POINTER, sizeof(char*), nel_alignment_of(char*),0,0,type1->array.base_type);
				break;
			default:
				retval = NULL;
			}	
	}

	if(type2->simple.type == nel_D_POINTER)
	{
		switch(type1->simple.type)
			{
			case nel_D_CHAR:
			case nel_D_SIGNED_CHAR:
			case nel_D_UNSIGNED_CHAR:
			case nel_D_INT:
			case nel_D_SIGNED_INT:
			case nel_D_UNSIGNED_INT:
			case nel_D_SIGNED:
			case nel_D_UNSIGNED:
			case nel_D_LONG:
			case nel_D_UNSIGNED_LONG:
			case nel_D_SIGNED_LONG:
				retval = type2;
				break;
			case nel_D_POINTER:
				retval = type1;
				break;
			default:
				retval = NULL;
			}
	}
	if(type2->simple.type == nel_D_ARRAY)
	{
			switch(type1->simple.type)
			{
			case nel_D_CHAR:
			case nel_D_SIGNED_CHAR:
			case nel_D_UNSIGNED_CHAR:
			case nel_D_INT:
			case nel_D_SIGNED_INT:
			case nel_D_UNSIGNED_INT:
			case nel_D_SIGNED:
			case nel_D_UNSIGNED:
			case nel_D_LONG:
			case nel_D_UNSIGNED_LONG:
			case nel_D_SIGNED_LONG:
				retval = nel_type_alloc(eng, nel_D_POINTER, sizeof(char*), nel_alignment_of(char*),0,0,type2->array.base_type);	
				break;
			case nel_D_POINTER:
				retval = type1;
				break;
			default:
				retval = NULL;
			}	
	}
	if(type1 == type2 && nel_s_u_D_token(type1->simple.type))
		retval = type1;
	//end

	nel_debug ({
				   intp_trace (eng, "] exiting intp_type_binary_op\nretval =\n%1T\n", retval);
			   });

	return (retval);
}


/*****************************************************************************/
/* intp_eval_binary_op () perfoms the semantic actions for a binary       */
/* operation.  <op> is the nel_O_token for the operation to be           */
/* performed. <left> and <right> are the left and right operands,            */
/* respectively.  for assignment operators (i.e. *=, /=, etc), only the      */
/* arithmetic part is done, no assigment is performed.  for array indexing,  */
/* (a[b]) only the address arithmetic is performed, the expression is not    */
/* dereferenced.                                                             */
/*****************************************************************************/
nel_symbol *intp_eval_binary_op (struct nel_eng *eng, nel_O_token op, register nel_symbol *left, register nel_symbol *right)
{
	register nel_type *l_type;
	register nel_type *r_type;
	register nel_D_token l_D_token;
	register nel_D_token r_D_token;
	register nel_O_token non_asgn_op;
	nel_symbol *retval;

	nel_debug ({
	intp_trace (eng, "entering intp_eval_binary_op [\neng = 0x%x\nop = %O\n\n", eng, op);
	});

	nel_debug ({
	if ((left == NULL) || (left->type == NULL)) {
		intp_fatal_error (eng, "(intp_eval_binary_op #1): bad left operand symbol\n%1S", left);
	}
	if ((right == NULL) || (right->type == NULL)) {
		intp_fatal_error (eng, "(intp_eval_binary_op #2): bad right operand symbol\n%1S", right);
	}
	});

	if (left->value == NULL) {
		if (left->name == NULL) {
			intp_stmt_error (eng, "NULL address for operand of %P", op);
		} else {
			intp_stmt_error (eng, "NULL address for %I", left);
		}
	}
	if (right->value == NULL) {
		if (right->name == NULL) {
			intp_stmt_error (eng, "NULL address for operand of %P", op);
		} else {
			intp_stmt_error (eng, "NULL address for %I", right);
		}
	}

	l_type = left->type;
	l_D_token = l_type->simple.type;
	r_type = right->type;
	r_D_token = r_type->simple.type;

	/*******************************************************************/
	/* if this an arithmetic/assignment operator, only the arithmetic  */
	/* part will be performed.  find the nel_O_token corresponding */
	/* to the arithmetic part without the assignment.                  */
	/*******************************************************************/
	switch (op) {
	case nel_O_ADD_ASGN:
		non_asgn_op = nel_O_ADD;
		break;

	case nel_O_BIT_AND_ASGN:
		non_asgn_op = nel_O_BIT_AND;
		break;

	case nel_O_BIT_OR_ASGN:
		non_asgn_op = nel_O_BIT_OR;
		break;

	case nel_O_BIT_XOR_ASGN:
		non_asgn_op = nel_O_BIT_XOR;
		break;

	case nel_O_DIV_ASGN:
		non_asgn_op = nel_O_DIV;
		break;

	case nel_O_LSHIFT_ASGN:
		non_asgn_op = nel_O_LSHIFT;
		break;

	case nel_O_MOD_ASGN:
		non_asgn_op = nel_O_MOD;
		break;

	case nel_O_MULT_ASGN:
		non_asgn_op = nel_O_MULT;
		break;

	case nel_O_RSHIFT_ASGN:
		non_asgn_op = nel_O_RSHIFT;
		break;

	case nel_O_SUB_ASGN:
		non_asgn_op = nel_O_SUB;
		break;

	default:
		non_asgn_op = op;
		break;
	}

	if (nel_s_u_D_token (l_D_token) || nel_s_u_D_token (r_D_token)) {
		intp_stmt_error (eng, "%P is not a permitted struct/union operation", op);
	}
	if (nel_pointer_D_token (l_D_token) || nel_pointer_D_token (r_D_token)) {
		if (nel_integral_O_token (op)) {
			intp_stmt_error (eng, "operands of %P have incompatible types", op);
		}
		if (nel_pointer_D_token (l_D_token) && nel_pointer_D_token (r_D_token)) {
			register char *l_addr;
			register char *r_addr;
			register nel_type *l_base_type;
			register nel_type *r_base_type;

			/***********************************************/
			/* we have two pointer types.                  */
			/* find the pointer values, array locations,   */
			/* function locations, common block locations, */
			/* or ld symbol locations.                     */
			/* (char *) pointers are compatible with a     */
			/* location type or a common block name.       */
			/***********************************************/

			if (l_D_token == nel_D_POINTER) {
				l_addr = (* ((char **) left->value));
				l_base_type = l_type->pointer.deref_type;
			} else if (l_D_token == nel_D_ARRAY) {
				l_addr = left->value;
				l_base_type = l_type->array.base_type;
			} else if (l_D_token == nel_D_FUNCTION) {
				l_addr = left->value;
				l_base_type = l_type;
			} else /* common, location */
			{
				l_addr = left->value;
				l_base_type = nel_void_type;
			}

			if (r_D_token == nel_D_POINTER) {
				r_addr = (* ((char **) right->value));
				r_base_type = r_type->pointer.deref_type;
			} else if (r_D_token == nel_D_ARRAY) {
				r_addr = right->value;
				r_base_type = r_type->array.base_type;
			} else if (l_D_token == nel_D_FUNCTION) {
				r_addr = right->value;
				r_base_type = r_type;
			} else /* common, location */
			{
				r_addr = right->value;
				r_base_type = nel_void_type;
			}

			/***************************************/
			/* subtraction and comparisons are the */
			/* only pointer/pointer operations.    */
			/***************************************/
			if (non_asgn_op == nel_O_SUB) {
				/*********************************************/
				/* we must know the size of both base types. */
				/* they should be identical, but if they     */
				/* aren't, it's a different error message.   */
				/*********************************************/
				if ((l_base_type->simple.size <= 0) || (r_base_type->simple.size <= 0)) {
					intp_stmt_error (eng, "unknown size");
				}

				/************************************************************/
				/* both pointers or locations must have the same base type. */
				/************************************************************/
				if (nel_type_diff (l_base_type, r_base_type, 0)) {
					intp_stmt_error (eng, "illegal pointer subtraction");
				}

				/************************************************************/
				/* result is the difference of the two operands, divided by */
				/* the size of their base types.  its type is signed int.   */
				/************************************************************/
				retval = intp_dyn_symbol_alloc (eng, NULL, nel_signed_int_type, NULL, nel_C_RESULT, 0, nel_L_NEL, eng->intp->level);
				retval->value = intp_dyn_value_alloc (eng, sizeof (signed_int), nel_alignment_of (signed_int));
				*((signed_int *) retval->value) = (l_addr - r_addr) / l_base_type->simple.size;
			} else if (nel_relational_O_token (non_asgn_op)) {
				/********************************************/
				/* both pointers or locations must have the */
				/* same base type, or one must be (void *). */
				/********************************************/
				if (nel_type_diff (l_base_type, r_base_type, 0) && (l_base_type->simple.type != nel_D_VOID)&& (r_base_type->simple.type != nel_D_VOID)) {
					intp_warning (eng, "illegal pointer combination");
				}

				/******************************/
				/* result type is signed int. */
				/******************************/
				retval = intp_dyn_symbol_alloc (eng, NULL, nel_signed_int_type, NULL, nel_C_RESULT, 0, nel_L_NEL, eng->intp->level);
				retval->value = intp_dyn_value_alloc (eng, sizeof (signed_int), nel_alignment_of (signed_int));

				/*******************************************************/
				/* coerce the addresses to unsigned long int, and call */
				/* intp_op to compare them.  on some systems, an    */
				/* unsigned int is not large enough to hold all the    */
				/* possible values of a pointer.                       */
				/*******************************************************/
				{
					unsigned_long_int l_int = (unsigned_long_int) l_addr;
					unsigned_long_int r_int = (unsigned_long_int) r_addr;
					intp_op (eng, non_asgn_op, nel_unsigned_long_int_type, retval->value, (char *) (&l_int), (char *) (&r_int));
				}
			} else {
				intp_stmt_error (eng, "operands of %P have incompatible types", op);
			}
		} else {
			register nel_type *base_type;
			register char *addr;
			register nel_symbol *offset_sym;
			nel_type *res_type;
			int offset;
			int lb = 0;

			/*********************************************/
			/* we have a pointer and some other type.    */
			/* find the pointer value, array location,   */
			/* function location, common block location, */
			/* or ld symbol location of the pointer type */
			/* and the value of the other operand.       */
			/*********************************************/
			if (nel_pointer_D_token (l_D_token)) {
				/************************************/
				/* left operand is pointer or array */
				/************************************/
				if (l_D_token == nel_D_POINTER) {
					addr = (* ((char **) left->value));
					base_type = l_type->pointer.deref_type;
				} else if (l_D_token == nel_D_ARRAY) {
					addr = left->value;
					base_type = l_type->array.base_type;

#ifndef NEL_PURE_ARRAYS

					if ((non_asgn_op == nel_O_ARRAY_INDEX) && (l_type->array.known_lb)) {
						lb = l_type->array.lb.value;
					}
#endif

				} else if (l_D_token == nel_D_FUNCTION) {
					addr = left->value;
					base_type = l_type;
				} else /* common, location */
				{
					addr = left->value;
					base_type = nel_void_type;
				}

				/***************************/
				/* right operand is offset */
				/***************************/
				offset_sym = right;
			} else /* nel_pointer_D_token (r_D_token) */
			{
				/*************************************/
				/* right operand is pointer or array */
				/*************************************/
				if (r_D_token == nel_D_POINTER) {
					addr = (* ((char **) right->value));
					base_type = r_type->pointer.deref_type;
				} else if (r_D_token == nel_D_ARRAY) {
					addr = right->value;
					base_type = r_type->array.base_type;

#ifndef NEL_PURE_ARRAYS
					/**********************************************************/
					/* only take nonzero array lower bounds into consideration*/
					/* when performing the address calculations if we are not */
					/* compiled with the NEL_PURE_ARRAYS option, the array has*/
					/* a known lower bound, and the array was indexed using   */
					/* the [] operator, not + or -.                           */
					/**********************************************************/
					if ((non_asgn_op == nel_O_ARRAY_INDEX) && (r_type->array.known_lb)) {
						lb = r_type->array.lb.value;
					}

#endif /* NEL_PURE_ARRAYS */

				} else if (r_D_token == nel_D_FUNCTION) {
					addr = right->value;
					base_type = r_type;
				} else /* common, location */
				{
					addr = right->value;
					base_type = nel_void_type;
				}

				/**************************/
				/* left operand is offset */
				/**************************/
				offset_sym = left;
			}

			if ((non_asgn_op == nel_O_ADD) || (non_asgn_op == nel_O_ARRAY_INDEX) || (non_asgn_op == nel_O_SUB)) {
				/**********************************************************/
				/* we are adding or subtracting an offset from a pointer. */
				/* we must know the size of the base type.                */
				/**********************************************************/
				if (base_type->simple.size <= 0) {
					intp_stmt_error (eng, "unknown size");
				}

				/*******************************************************/
				/* the non-pointer operand must be of integral type.   */
				/* for subtraction, the right operand must be the int. */
				/*******************************************************/
				if ((! nel_integral_D_token (offset_sym->type->simple.type)) ||
						((non_asgn_op == nel_O_SUB) && (offset_sym != right))) {
					intp_stmt_error (eng, "operands of %P have incompatible types", op);
				}

				/**********************************************************/
				/* coerce offset to signed int.                           */
				/* result value address +- (offset * sizeof (base_type)). */
				/* if the [] operator was used on an array operand and    */
				/* integer operand, then the lower bound of the array is  */
				/* subtracted from the base type.                         */
				/* result type is a pointer to the base type.             */
				/**********************************************************/
				intp_coerce (eng, nel_signed_int_type, (char *) (&offset), offset_sym->type, offset_sym->value);
				offset -= lb;
				res_type = nel_type_alloc (eng, nel_D_POINTER, sizeof (char *), nel_alignment_of (char *), 0, 0, base_type);
				retval = intp_dyn_symbol_alloc (eng, NULL, res_type, NULL, nel_C_RESULT, 0, nel_L_NEL, eng->intp->level);
				retval->value = intp_dyn_value_alloc (eng, sizeof (char *), nel_alignment_of (char *));
				if (non_asgn_op == nel_O_SUB) {
					//added by zhangbin, 2006-5-22
					if(offset >0 || offset * base_type->simple.size >= left->type->simple.size)
						if(l_D_token == nel_D_ARRAY)
							intp_stmt_error(eng, "memory access overflow");
					//end
					* ((char **) (retval->value)) = addr - (offset * base_type->simple.size);
				} else {
					//added by zhangbin, 2006-5-22
					if(offset * base_type->simple.size >= left->type->simple.size || 
						addr + offset * base_type->simple.size < addr)
						if(l_D_token == nel_D_ARRAY)
							intp_stmt_error(eng, "memory access overflow");
					//end
					* ((char **) (retval->value)) = addr + (offset * base_type->simple.size);
				}
			} else if (nel_relational_O_token (non_asgn_op)) {
				/**************************************************************/
				/* a relational operation between a pointer and an integral   */
				/* type is legal if the int is a constant and has value zero. */
				/* we should allow constant expressions with the value zero,  */
				/* but rarely is anything but the constant 0 used in          */
				/* practice, and only a warning will be emitted anyway.       */
				/* allow a pointer to be compared to a variable with the      */
				/* value 0 and the const type modifier, in case someone has   */
				/* declared "const int NULL" as a hack.                       */
				/**************************************************************/
				if (! nel_integral_D_token (offset_sym->type->simple.type)) {
					intp_stmt_error (eng, "operands of %P have incompatible types", op);
				}
				if ((intp_nonzero (eng, offset_sym)) || ((! nel_constant_C_token (offset_sym->class))&& (! offset_sym->type->simple._const))) {
					intp_warning (eng, "illegal combination of pointer and integer, op %P", op);
				}

				/***********************/
				/* result type is int. */
				/***********************/
				retval = intp_dyn_symbol_alloc (eng, NULL, nel_signed_int_type, NULL, nel_C_RESULT, 0, nel_L_NEL, eng->intp->level);
				retval->value = intp_dyn_value_alloc (eng, sizeof (signed_int), nel_alignment_of (signed_int));

				/*************************************************************/
				/* find which side the pointer is on, coerce it to unsigned  */
				/* long int, coerce the integral value to unsigned long int, */
				/* and call intp_op to compare them.  note that the       */
				/* integral operand could nonzero, with a warning emitted.   */
				/* on some systems, an unsigned int is not large enough to   */
				/* hold all the possible values of a pointer.                */
				/*************************************************************/
				if (left == offset_sym) {
					unsigned_long_int l_int;
					unsigned_long_int r_int = (unsigned_long_int) addr;
					intp_coerce (eng, nel_unsigned_long_int_type, (char *) (&l_int), l_type, left->value);
					intp_op (eng, non_asgn_op, nel_unsigned_long_int_type, retval->value, (char *) (&l_int), (char *) (&r_int));
				} else {
					unsigned_long_int l_int = (unsigned_long_int) addr;
					unsigned_long_int r_int;
					intp_coerce (eng, nel_unsigned_long_int_type, (char *) (&r_int), r_type, right->value);
					intp_op (eng, non_asgn_op, nel_unsigned_long_int_type, retval->value, (char *) (&l_int), (char *) (&r_int));
				}
			} else {
				intp_stmt_error (eng, "operands of %P have incompatible types", op);
			}
		}
	} else {
		/**************************************************/
		/* we have two numerical types, if all is correct */
		/**************************************************/
		register nel_type *res_type;
		char *l_promo;
		char *r_promo;

		/***************************************************/
		/* make sure that we aren't trying to evaluate the */
		/* + part of an array index ([]) on two numerical  */
		/* operands - all other illegal O_tokens should    */
		/* have been checked for by now.                   */
		/***************************************************/
		if (op == nel_O_ARRAY_INDEX) {
			intp_stmt_error (eng, "illegal indirection");
		}

		/*******************************/
		/* find the type of the result */
		/*******************************/
		res_type = intp_type_binary_op (eng, l_type, r_type, nel_integral_O_token (op), nel_complex_O_token (op));
		if (res_type == NULL) {
			intp_stmt_error (eng, "operands of %P have incompatible types", op);
		}
		nel_debug ({
					   if (res_type->simple.size <= 0) {
					   intp_fatal_error (eng, "(intp_eval_binary_op #3): bad result type\n%1T", res_type)
						   ;
					   }
				   });

		/********************************************************/
		/* coerce both operands to the result type.             */
		/* (if a comparison, this isn't really the result type) */
		/********************************************************/
		l_promo = intp_dyn_value_alloc (eng, res_type->simple.size, res_type->simple.alignment);
		r_promo = intp_dyn_value_alloc (eng, res_type->simple.size, res_type->simple.alignment);
		intp_coerce (eng, res_type, l_promo, l_type, left->value);
		intp_coerce (eng, res_type, r_promo, r_type, right->value);

		/************************************************************/
		/* the result type of a relational operation is signed int, */
		/* though we still do the same promotions on the operand as */
		/* for the other arithmetic operations.                     */
		/************************************************************/
		//added by zhangbin, 2006-5-19
		nel_type *old_res_type = res_type;;
		//end
		if (nel_relational_O_token (op)) {
			res_type = nel_signed_int_type;
		}

		/******************************************/
		/* allocate a symbol and space for the    */
		/* the result, then perform the operation */
		/******************************************/
		retval = intp_dyn_symbol_alloc (eng, NULL, res_type, NULL, nel_C_RESULT, 0, nel_L_NEL, eng->intp->level);
		retval->value = intp_dyn_value_alloc (eng, res_type->simple.size, res_type->simple.alignment);
		//added by zhangbin, 2006-5-19
		if( nel_relational_O_token(op) )
			intp_op (eng, non_asgn_op, old_res_type, retval->value, l_promo, r_promo);
		else//end
			intp_op (eng, non_asgn_op, res_type, retval->value, l_promo, r_promo);
	}

	nel_debug ({
				   intp_trace (eng, "] exiting intp_eval_binary_op\nretval =\n%1S\n", retval);
			   });

	return (retval);

}


/*****************************************************************************/
/* intp_insert_bit_field () inserts a sequence of bits taken from <word> into  */
/* *result.  we don't really need the <type> parameter as <word> is already  */
/* sign-extended and coerced to unsigned int, but it seems to belong in the  */
/* parameter list, so it is left there.                                      */
/*****************************************************************************/
void intp_insert_bit_field (struct nel_eng *eng, nel_type *type, char *result, register unsigned_int word, register unsigned_int lb, register unsigned_int size)
{
	unsigned_int result_temp;
	register unsigned_int mask;
	register int i;

	nel_debug ({ intp_trace (eng, "entering intp_insert_bit_field [\neng = 0x%x\ntype =\n%1Tresult = 0x%x\nword = 0x%x\nlb = %d\nsize = %d\n\n", eng, type, result, word, lb, size); });

	nel_debug ({
				   if (type == NULL) {
				   intp_fatal_error (eng, "(intp_insert_bit_field #1): NULL type")
					   ;
				   }
				   if (result == NULL) {
				   intp_fatal_error (eng, "(intp_insert_bit_field #2): NULL result")
					   ;
				   }
			   });

	/*******************************************************************/
	/* copy *result to result_temp, where we can use bit manipulations */
	/* on it.  *result it is not nessarily aligned at a word boundary. */
	/*******************************************************************/
	nel_copy (sizeof (unsigned_int), &result_temp, result);

	/*****************************************************/
	/* zero out the appropriate region of <result_temp>, */
	/* so that we may or the bit field back in.          */
	/*****************************************************/

	/***********************/
	/* big endian machines */
	/***********************/
	mask = 1 << lb;	//((CHAR_BIT * sizeof (unsigned_int)) - lb - size);	//modified by zhangbin, 2006-5-24


	for (i = 0; (i < size); i++, mask <<= 1) {
		result_temp &= ~mask;
	};

	/*******************************************************************/
	/* O.K., result_temp should be identical to the possibly-unaligned */
	/* word stored at <result>, with the appropriate bits zeroed out.  */
	/* now go work on the the bits to insert in their place.           */
	/*******************************************************************/

	/**********************************************************/
	/* shift left to discard high-order bits, then shift into */
	/* position.  position depends upon byte ordering.        */
	/**********************************************************/
	word <<= ((CHAR_BIT * sizeof (unsigned_int)) - size);		//modified by zhangbin, 2006-5-24


	/***********************/
	/* big endian machines */
	/***********************/
	word >>= ((CHAR_BIT*sizeof(unsigned_int)) - size - lb);		//modified by zhangbin, 2006-5-24


	/***************************************/
	/* now or <result_temp> and <word>     */
	/* together, and copy back to *result. */
	/***************************************/
	result_temp |= word;
	nel_copy (sizeof (unsigned_int), result, &result_temp);

	nel_debug ({ intp_trace (eng, "] exiting intp_insert_bit_field\nresult_temp = 0x%x\n\n", result_temp); });
}

/*****************************************************************************/
/* intp_eval_asgn () perfoms the semantic actions for an assignment.      */
/* op is the original operator; it is used only for error messages.          */
/* left and right are the symbols for the result of the left and right hand  */
/* sides, respectively.                                                      */
/*****************************************************************************/
nel_symbol *intp_eval_asgn (struct nel_eng *eng, register nel_O_token op, register nel_symbol *left, register nel_symbol *right)
{
	register nel_type *l_type;
	register nel_type *r_type;
	register nel_D_token l_D_token;
	register nel_D_token r_D_token;
	register char *res_val;
	nel_symbol *retval;

	nel_debug ({
		intp_trace (eng, "entering intp_eval_asgn [\neng = 0x%x\nop = %O\nleft =\n%1Sright = \n%1S\n", eng, op, left, right);
	});

	nel_debug ({
		if ((left == NULL) || (left->type == NULL)) {
			intp_fatal_error (eng, "(intp_eval_asgn #1): bad left operand symbol\n%1S", left);
		}

		if ((right == NULL) || (right->type == NULL)) {
			intp_fatal_error (eng, "(intp_eval_asgn #2): bad right operand symbol\n%1S", right);
		}
	});

	if (left->value == NULL) {
		if (left->name == NULL) {
			intp_stmt_error (eng, "NULL address for operand of %P", op);
		} else {
			intp_stmt_error (eng, "NULL address for %I", left);
		}
	}
	if (right->value == NULL) {
		if (right->name == NULL) {
			intp_stmt_error (eng, "NULL address for operand of %P", op);
		} else {
			intp_stmt_error (eng, "NULL address for %I", right);
		}
	}

	l_type = left->type;
	l_D_token = l_type->simple.type;
	r_type = right->type;
	r_D_token = r_type->simple.type;


#if 0
	if((l_D_token == nel_D_ARRAY) && (l_type->array.size == 0)) {
		//l_D_token = nel_D_POINTER;
		left->lhs = 1;
	}	
#endif

	/*************************************************************/
	/* make sure left is a legal left hand side of an assignment */
	/*************************************************************/
	/* allow array initializtion , bugfix, wyong, 2006.4.18*/
	if ( !(l_D_token == nel_D_ARRAY && r_D_token == nel_D_ARRAY ) 
		&& !left->lhs ){
		intp_stmt_error (eng, "illegal lhs of assignment operator");
	}

	/* wyong, 2006.4.17 */
	if((l_D_token == nel_D_ARRAY) && (r_D_token == nel_D_ARRAY )) {
		if (nel_type_diff (l_type, r_type, 0)) {
				intp_stmt_error (eng, "assignment of different structures");
		}
		if (nel_type_incomplete (l_type) || nel_type_incomplete (r_type)) {
			intp_stmt_error (eng, "undefined structure or union");
		}
		nel_copy (l_type->s_u.size, left->value, right->value);
		res_val = left->value;
	}
	else if (nel_s_u_D_token (l_D_token) && nel_s_u_D_token (r_D_token)) {
		if (nel_type_diff (l_type, r_type, 0)) {
			intp_stmt_error (eng, "assignment of different structures");
		}
		if (nel_type_incomplete (l_type) || nel_type_incomplete (r_type)) {
			intp_stmt_error (eng, "undefined structure or union");
		}
		nel_copy (l_type->s_u.size, left->value, right->value);
		res_val = left->value;
	} else if (nel_pointer_D_token (l_D_token) || (nel_pointer_D_token (r_D_token))) {

		if (nel_pointer_D_token (l_D_token) && (nel_pointer_D_token (r_D_token))) {
			register char *r_addr;
			register nel_type *l_base_type;
			register nel_type *r_base_type;

			/*************************************/
			/* pointers better not be bit fields */
			/*************************************/
			nel_debug ({
				if ((left->member != NULL) && (left->member->bit_field)) {
					intp_fatal_error (eng, "(intp_eval_asgn #3): pointer type in bit field\n%1S", left);
				}
				
				if ((right->member != NULL) && (right->member->bit_field)) {
					intp_fatal_error (eng, "(intp_eval_asgn #4): pointer type in bit field\n%1S", right);
				}

			});

			/****************************************************/
			/* we have two pointer types.                       */
			/* find the pointer values, array locations, common */
			/* block locations, or ld symbol locations.         */
			/* (char *) pointers are compatible with a location */
			/* type or a common block name.                     */
			/****************************************************/

			/****************************************************/
			/* the lhs must be strictly a pointer to something. */
			/* arrays, etc. are illegal lhs of assignments.     */
			/* (this should have been caught by now).           */
			/****************************************************/
			if (l_D_token != nel_D_POINTER) {
				intp_stmt_error (eng, "illegal lhs of assignment operator");
			}

			l_base_type = l_type->pointer.deref_type;

			if (r_D_token == nel_D_POINTER) {
				r_addr = (* ((char **) right->value));
				r_base_type = r_type->pointer.deref_type;
				//added by zhangbin, 2006-6-2
				if(r_base_type->simple.type == nel_D_FUNCTION)
					left->s_u = right->s_u;
				//end
			} else if (r_D_token == nel_D_ARRAY) {
				r_addr = right->value;
				r_base_type = r_type->array.base_type;
			} else if (r_D_token == nel_D_FUNCTION) {
				r_addr = right->value;
				r_base_type = r_type;
				//added by zhangbin, 2006-6-2
				left->s_u=right;
				//end
			} else /* common, location */
			{
				r_addr = right->value;
				r_base_type = nel_void_type;
			}

			/********************************************/
			/* both pointers or locations must have the */
			/* same base type, or one must be (void *). */
			/********************************************/
			if ((l_base_type->simple.type != nel_D_VOID) && (r_base_type->simple.type != nel_D_VOID) && nel_type_diff (l_base_type, r_base_type, 0)) {
				intp_warning (eng, "illegal pointer combination");
			}
			*((char **) (left->value)) = r_addr;
		} else {
			/******************************************/
			/* we have a pointer and some other type. */
			/******************************************/

			if (nel_pointer_D_token (l_D_token)) {
				/*************************************/
				/* pointers better not be bit fields */
				/*************************************/
				nel_debug ({
					if ((left->member != NULL) && (left->member->bit_field)) {
						intp_fatal_error (eng, "(intp_eval_asgn #5): pointer type in bit field\n%1S", left);
					}
				});

				/****************************************************/
				/* the lhs must be strictly a pointer to something. */
				/* arrays, etc. are illegal lhs of assignments.     */
				/* (this should have been caught by now).           */
				/****************************************************/
				if (l_D_token != nel_D_POINTER) {
					intp_stmt_error (eng, "illegal lhs of assignment operator");
				}

				/**************************************************************/
				/* one may assign the constant integral value zero to a       */
				/* pointer.  we should allow constant expressions with the    */
				/* value zero, but rarely is anything but the constant 0 used */
				/* in practice, and only a warning will be emitted anyway.    */
				/**************************************************************/
				if (! nel_integral_D_token (r_type->simple.type)) {
					intp_stmt_error (eng, "operands of %P have incompatible types", op);
				}
				if ((! nel_constant_C_token (right->class)) || (intp_nonzero (eng, right))) {
					intp_warning (eng, "illegal combination of pointer and integer, op =");
				}

				/***********************************************/
				/* call intp_coerce to coerce the right integral */
				/* operand to unsigned long int, and perform   */
				/* the assignment manually.  on some systems,  */
				/* an unsigned int is not large enough to hold */
				/* all the possible values of a pointer.       */
				/***********************************************/
				{
					unsigned_long_int r_int;
					intp_coerce (eng, nel_unsigned_long_int_type, (char *) (&r_int), r_type, right->value);
					*((char **) (left->value)) = (char *) r_int;
				}
			} else /* nel_pointer_D_token (r_D_token) */
			{
				register char *r_addr;

				/*************************************/
				/* pointers better not be bit fields */
				/*************************************/
				nel_debug ({
					if ((right->member != NULL) && (right->member->bit_field)) {
						intp_fatal_error (eng, "(intp_eval_asgn #6): pointer type in bit field\n%1S", right);
					}
				});

				/*******************************************/
				/* one can assign a pointer to an integer, */
				/* but a warning is emitted.               */
				/*******************************************/
				if (! nel_integral_D_token (l_type->simple.type)) {
					intp_stmt_error (eng, "operands of %P have incompatible types", op);
				}
				intp_warning (eng, "illegal combination of pointer and integer, op %P", op);
				if (r_D_token == nel_D_POINTER) {
					r_addr = (* ((char **) right->value));
				} else /* array, function, common, location */
				{
					r_addr = right->value;
				}

				if ((left->member != NULL) && left->member->bit_field) {
					/********************************************************/
					/* if this is a bit field, then the lhs symbol has a    */
					/* value which is a copy of the extracted bits.  find   */
					/* the word containing the bit field by adding offset   */
					/* of the member to the address of the struct, and call */
					/* intp_insert_bit_field () to do the bit twiddling.      */
					/********************************************************/
					register nel_member *member = left->member;
					register nel_symbol *s_u = left->s_u;
					register unsigned_int r_int;
					nel_debug ({
						if ((s_u == NULL) || (s_u->type == NULL)) {
							intp_fatal_error (eng, "(intp_eval_asgn #7): bad lhs member s_u\n%1S", s_u);
						}
					});
					if (s_u->value == NULL) {
						if (s_u->name == NULL) {
							intp_stmt_error (eng, "NULL address for operand of %P", op);
						} else {
							intp_stmt_error (eng, "NULL address for %I", s_u);
						}
					}

					/**********************************************/
					/* coerce the rhs to unsigned int, and then   */
					/* call intp_insert_bit_field () to  insert the */
					/* bits.  the largest allowable bit field has */
					/* as many bits as an int, so if a pointer is */
					/* larger than that, it will get truncated.   */
					/**********************************************/
					r_int = (unsigned_int) r_addr;
					intp_insert_bit_field (eng, left->type, s_u->value + member->offset, r_int, member->bit_lb, member->bit_size);
				} else {
					/******************************************************/
					/* coerce right pointer operand to unsigned long int  */
					/* manually, then call intp_coerce to do the assignment */
					/* to an arbitrary integral type.  on some systems,   */
					/* an unsigned int is not large enough to hold all    */
					/* the possible values of a pointer.                  */
					/******************************************************/
					unsigned_long_int r_int;
					r_int = (unsigned_long_int) r_addr;
					intp_coerce (eng, l_type, left->value, nel_unsigned_long_int_type, (char *) (&r_int));
				}
			}
		}
		res_val = left->value;
	} else {
		/**************************************************/
		/* we have two numerical types, if all is correct */
		/**************************************************/
		if ((! nel_numerical_D_token (l_D_token))|| (! nel_numerical_D_token (r_D_token))) {
			intp_stmt_error (eng, "operands of %P have incompatible types", op);
		}

		if ((left->member != NULL) && left->member->bit_field) {
			/************************************************************/
			/* if this is a bit field, then the lhs symbol has a value  */
			/* which a copy of the extracted bits.  find the symbol     */
			/* whose value is the actual address of the word containing */
			/* the bit field by following the member pointer, and       */
			/* call intp_insert_bit_field to do the bit twiddling.        */
			/************************************************************/
			register nel_symbol *s_u = left->s_u;
			register nel_member *member = left->member;
			unsigned_int word;
			nel_debug ({
				if (s_u == NULL) {
					intp_fatal_error (eng, "(intp_eval_asgn #8): bad lhs s_u symbol\n%1S", s_u);
				}
			});

			if (s_u->value == NULL) {
				if (s_u->name == NULL) {
					intp_stmt_error (eng, "NULL address for operand of %P", op);
				} else {
					intp_stmt_error (eng, "NULL address for %I", s_u);
				}
			}

			/********************************************/
			/* coerce the rhs to unsigned int, and call */
			/* intp_insert_bit_field insert the bits.     */
			/********************************************/
			intp_coerce (eng, nel_unsigned_int_type, (char *) (&word), r_type, right->value);
			intp_insert_bit_field (eng, left->type, s_u->value + member->offset, word, member->bit_lb, member->bit_size);

			/*************************************/
			/* the correct value for the result  */
			/* is in word not in in left->value. */
			/*************************************/
			res_val = (char *) (&word);
		} else {
			/*************************************************/
			/* just call intp_coerce to perform the assignment */
			/*************************************************/
			intp_coerce (eng, l_type, left->value, r_type, right->value);
			res_val = left->value;
		}
	}

	/*************************************************************/
	/* allocate the result symbol.  it is identical to left,    */
	/* except its lhs field is 0 - the result of an assignment   */
	/* cannot be used as the lhs of another assignment. we must  */
	/* copy the value of the lhs, another side effect on the lhs */
	/* later in this expression should not affect the result of  */
	/* this assignment.  return the result symbol.               */
	/*************************************************************/
	retval = intp_dyn_symbol_alloc (eng, NULL, l_type, NULL, nel_C_RESULT, 0, nel_L_NEL, eng->intp->level);
	retval->value = intp_dyn_value_alloc (eng, l_type->simple.size, l_type->simple.alignment);
	nel_copy (l_type->simple.size, retval->value, res_val);
	
	//added by zhangbin, 2006-6-2
	if(left->s_u==right)
		retval->s_u=right;
	//eng
	
	nel_debug ( {
		intp_trace (eng, "] exiting intp_eval_asgn\nretval = \n%1S\n", retval);
	}
	);

	return (retval);
}


/*****************************************************************************/
/* intp_eval_inc_dec () perfoms the semantic actions for prefix and       */
/* postfix ++ (increment) and -- (decrement).  non-integral types are        */
/* permitted.  <op> is the nel_O_token for the operation to be           */
/* performed.  <operand> is the symbol for the operand.                      */
/*****************************************************************************/
nel_symbol *intp_eval_inc_dec (struct nel_eng *eng, nel_O_token op, register nel_symbol *operand)
{
	register nel_type *type;
	register nel_D_token D_token;
	nel_symbol *retval;

	nel_debug ({
				   intp_trace (eng, "entering intp_eval_inc_dec [\neng = 0x%x\nop = %O\noperand =\n%1S\n", eng, op, operand);
			   });

	nel_debug ({
				   if ((operand == NULL) || (operand->type == NULL)) {
				   intp_fatal_error (eng, "(intp_eval_inc_dec #1): bad operand operand\n%1S", operand)
					   ;
				   }
			   });

	if (operand->value == NULL) {
		if (operand->name == NULL) {
			intp_stmt_error (eng, "NULL address for operand of %P", op);
		} else {
			intp_stmt_error (eng, "NULL address for %I", operand);
		}
	}

	type = operand->type;
	D_token = type->simple.type;

	/***************************************************************/
	/* make sure operand is a legal left hand side of an assignment */
	/***************************************************************/
	if (! operand->lhs) {
		intp_stmt_error (eng, "illegal lhs of assignment operator");
	}

	if (nel_s_u_D_token (D_token)) {
		intp_stmt_error (eng, "%P is not a permitted struct/union operation", op);
	}
	if (nel_pointer_D_token (D_token)) {
		register unsigned_int size;

		/***************************************************/
		/* operand must be stricly pointer type.           */
		/* arrays, commons, and locations are not allowed. */
		/***************************************************/
		if (D_token != nel_D_POINTER) {
			intp_stmt_error (eng, "illegal lhs of assignment operator");
		}

		nel_debug ({
					   if (type->pointer.deref_type == NULL) {
					   intp_fatal_error (eng, "(intp_eval_inc_dec #2): bad operand type\n%1T", type)
						   ;
					   }
				   });

		/***************************************/
		/* allocate the result symbol.  result */
		/* type is a pointer to the base type. */
		/* the result is not a legal lhs.      */
		/***************************************/
		retval = intp_dyn_symbol_alloc (eng, NULL, type, NULL, nel_C_RESULT, 0, nel_L_NEL, eng->intp->level);
		retval->value = intp_dyn_value_alloc (eng, sizeof (char *), nel_alignment_of (char *));

		/********************************************************/
		/* manually take care of the 4 possible cases.          */
		/* increment or decrement by the size of the base type. */
		/********************************************************/
		size = type->pointer.deref_type->simple.size;
		switch (op) {
		case nel_O_POST_DEC:
			*((char **) (retval->value)) = *((char **) operand->value);
			*((char **) (operand->value)) = (*((char **) (operand->value))) - size;
			break;
		case nel_O_POST_INC:
			*((char **) (retval->value)) = *((char **) (operand->value));
			*((char **) (operand->value)) = (*((char **) (operand->value))) + size;
			break;
		case nel_O_PRE_DEC:
			*((char **) (operand->value)) = (*((char **) (operand->value))) - size;
			*((char **) (retval->value)) = *((char **) (operand->value));
			break;
		case nel_O_PRE_INC:
			*((char **) (operand->value)) = (*((char **) (operand->value))) + size;
			*((char **) (retval->value)) = *((char **) (operand->value));
			break;
		default:
			intp_fatal_error (eng, "(intp_eval_inc_dec #3): bad op: %P", op);
		}
	} else {
		/***********************************************/
		/* we have a numerical type, if all is correct */
		/***********************************************/
		if (! nel_numerical_D_token (D_token)) {
			intp_stmt_error (eng, "illegal operand of %P", op);
		}

		/************************************************************/
		/* operands of ++ and -- do not undergo integral promotion, */
		/* so the result type is the operand's type.                */
		/************************************************************/

		/*******************************************/
		/* allocate a symbol and space for the     */
		/* the result, then perform the operation. */
		/* the result is not a legal lhs.          */
		/*******************************************/
		retval = intp_dyn_symbol_alloc (eng, NULL, type, NULL, nel_C_RESULT, 0, nel_L_NEL, eng->intp->level);
		retval->value = intp_dyn_value_alloc (eng, type->simple.size, type->simple.alignment);
		intp_op (eng, op, type, retval->value, operand->value, NULL);
	}

	nel_debug ({
				   intp_trace (eng, "] exiting intp_eval_inc_dec\nretval =\n%1S\n", retval);
			   });

	return (retval);
}



/**********************************************************************/
/* when the flag nel_intp_const_addresses is nonzero, the one may take */
/* the address of a constant this is useful when calling Fortran      */
/* routines, where arguments are passed by reference.                 */
/**********************************************************************/
unsigned_int nel_intp_const_addresses = 0;


/*****************************************************************************/
/* intp_eval_address () performs the semantic actions for taking the      */
/* address of the variable whose symbolic representation is <operand>.       */
/*****************************************************************************/
nel_symbol *intp_eval_address (struct nel_eng *eng, register nel_symbol *operand)
{
	register nel_type *type;
	register nel_symbol *retval;

	nel_debug ({
				   intp_trace (eng, "entering intp_eval_address [\neng = 0x%x\noperand =\n%1S\n", eng, operand);
			   });

	nel_debug ({
				   if ((operand == NULL) || (operand->type == NULL)) {
				   intp_fatal_error (eng, "(intp_eval_address #1): bad symbol\n%1S", operand)
					   ;
				   }
			   });
	type = operand->type;

	/********************************************************************/
	/* if the flag nel_intp_const_addresses is set, one may take the     */
	/* address of a constant.  arrays and functions only cause warnings */
	/* if the & operator is applied to them. they are handled below.    */
	/* taking the address of a bit field is always illegal.             */
	/********************************************************************/
	if ((! nel_lhs_D_token (type->simple.type) && (type->simple.type != nel_D_ARRAY)
			&& (type->simple.type != nel_D_FUNCTION)) || ((! nel_intp_const_addresses)
					&& ((operand->class == nel_C_CONST) || (operand->class == nel_C_ENUM_CONST)))) {
		intp_stmt_error (eng, "illegal operand of &");
	}

	/**********************************************************/
	/* ANSI C allows & to be applied to an array or function. */
	/* emit a warning if we are taking the address of an      */
	/* intrinsic or interpreted function.                     */
	/**********************************************************/
	if ((operand->class == nel_C_NEL_FUNCTION) || (operand->class == nel_C_NEL_STATIC_FUNCTION)) {
		intp_warning (eng, "& applied to interpreted function");
	} else if (operand->class == nel_C_INTRINSIC_FUNCTION) {
		intp_warning (eng, "& applied to intrinsic function");
	}
	if (operand->value == NULL) {
		if (operand->name == NULL) {
			intp_stmt_error (eng, "NULL address for operand of &");
		} else {
			intp_stmt_error (eng, "NULL address for %I", operand);
		}
	}

	type = nel_type_alloc (eng, nel_D_POINTER, sizeof (char *), nel_alignment_of (char *), 0, 0, type);
	retval = intp_dyn_symbol_alloc (eng, NULL, type, NULL, nel_C_RESULT, 0, nel_L_NEL, eng->intp->level);
	retval->value = intp_dyn_value_alloc (eng, sizeof (char *), nel_alignment_of (char *));
	*((char **) retval->value) = operand->value;

	nel_debug ({
				   intp_trace (eng, "] exiting intp_eval_address\nretval =\n%1S\n", retval);
			   });

	return (retval);

}


/*****************************************************************************/
/* intp_eval_deref () performs the semantic actions of dereferencing the  */
/* pointer or array whose symbolic representation is <operand>.              */
/*****************************************************************************/
nel_symbol *intp_eval_deref (struct nel_eng *eng, register nel_symbol *operand)
{
	register nel_type *type;
	register nel_symbol *retval;

	nel_debug ({
				   intp_trace (eng, "entering intp_eval_deref [\neng = 0x%x\noperand =\n%1S\n", eng, operand);
			   });

	nel_debug ({
				   if ((operand == NULL) || (operand->type == NULL)) {
				   intp_fatal_error (eng, "(intp_eval_deref #1): bad operand symbol\n%1S", operand)
					   ;
				   }
			   });

	type = operand->type;
	if (type->simple.type == nel_D_POINTER) {
		type = type->pointer.deref_type;
		nel_debug ({
					   if (type == NULL) {
					   intp_fatal_error (eng, "(intp_eval_deref #2): bad deref type\n%1T", type)
						   ;
					   }
				   });
		if (operand->value == NULL) {
			if (operand->name == NULL) {
				intp_stmt_error (eng, "NULL address for pointer operand of *");
			} else {
				intp_stmt_error (eng, "NULL address for pointer %I", operand);
			}
		}
		if (*((char **) (operand->value)) == NULL) {
			if (operand->name == NULL) {
				intp_stmt_error (eng, "NULL pointer operand of *");
			} else {
				intp_stmt_error (eng, "NULL pointer %I", operand);
			}
		}

		/******************************/
		/* result may be a legal lhs, */
		/* depending upon type.       */
		/******************************/
		//added by zhangbin. 2006-6-2
		if(type->simple.type == nel_D_FUNCTION)
			return operand->s_u;
		//end
		retval = intp_dyn_symbol_alloc (eng, NULL, type, *((char **) (operand->value)), nel_C_RESULT,
									   nel_lhs_type (type), nel_L_NEL, eng->intp->level);
	} else if (type->simple.type == nel_D_ARRAY) {
		type = type->array.base_type;
		nel_debug ({
					   if (type == NULL) {
					   intp_fatal_error (eng, "(intp_eval_deref #3): bad deref type\n%1T", type)
						   ;
					   }
				   });
		if (operand->value == NULL) {
			if (operand->name == NULL) {
				intp_stmt_error (eng, "NULL address for array operand of *");
			} else {
				intp_stmt_error (eng, "NULL address for array %I", operand);
			}
		}

		/******************************/
		/* result may be a legal lhs, */
		/* depending upon type.       */
		/******************************/
		retval = intp_dyn_symbol_alloc (eng, NULL, type, operand->value, nel_C_RESULT,
									   nel_lhs_type (type), nel_L_NEL, eng->intp->level);
	} else if (type->simple.type == nel_D_FUNCTION) {
		/*******************************************************/
		/* functions get promoted to pointer to functions when */
		/* use in an expression, and then dereferenced back to */
		/* functions by the '*'.                               */
		/*******************************************************/
		retval = intp_dyn_symbol_alloc (eng, NULL, type, operand->value, nel_C_RESULT, 0, nel_L_NEL, eng->intp->level);
	} else if (nel_pointer_D_token (type->simple.type)) /* common, location */
	{
		if (operand->value == NULL)
		{
			if (operand->name == NULL) {
				intp_stmt_error (eng, "NULL address for operand of *");
			} else {
				intp_stmt_error (eng, "NULL address for %I", operand);
			}
		}

		/*****************************************************/
		/* dereferenced type is void for commons & locations */
		/*****************************************************/
		retval = intp_dyn_symbol_alloc (eng, NULL, nel_void_type, operand->value, nel_C_RESULT, 1, nel_L_NEL, eng->intp->level);
	} else {
		intp_stmt_error (eng, "illegal indirection");
	}

	nel_debug ({
				   intp_trace (eng, "] exiting intp_eval_deref\nretval =\n%1S\n", retval);
			   });

	return (retval);

}


/*****************************************************************************/
/* intp_eval_cast () perfoms the semantic actions for a type cast.        */
/* <operand> is the symbol for the operand, which is casted to the new type  */
/* whose descriptor is <type>.                                               */
/*****************************************************************************/
nel_symbol *intp_eval_cast (struct nel_eng *eng, register nel_type *type, register nel_symbol *operand)
{
	register nel_D_token old_D_token;
	register nel_D_token new_D_token;
	register nel_symbol *retval;

	nel_debug ({
				   intp_trace (eng, "entering intp_eval_cast [\neng = 0x%x\ntype =\n%1Toperand =\n%1S\n", eng, type, operand);
			   });

	nel_debug ({
				   if ((operand == NULL) || (operand->type == NULL) || (operand->value == NULL)) {
				   intp_fatal_error (eng, "(intp_eval_cast #1): bad operand symbol\n%1S", operand)
					   ;
				   }
			   });

	old_D_token = operand->type->simple.type;
	new_D_token = type->simple.type;

	if (nel_s_u_D_token (new_D_token) || nel_s_u_D_token (old_D_token)) {
		//added by zhangbin, 2006-5-18
		if(new_D_token == old_D_token && nel_type_diff(operand->type, type, 0) == 0)
		{
			retval = intp_dyn_symbol_alloc (eng, NULL, type, NULL, nel_C_RESULT, operand->lhs, nel_L_NEL, eng->intp->level);
			retval->value = intp_dyn_value_alloc (eng, type->simple.size, type->simple.alignment);			
			nel_copy(type->simple.size, retval->value, operand->value);
		}
		else
		//end
			intp_stmt_error (eng, "cast is not a permitted struct/union operation");
	} else if (nel_pointer_D_token (new_D_token) || (nel_pointer_D_token (old_D_token))) {
		if (nel_pointer_D_token (new_D_token) && (nel_pointer_D_token (old_D_token))) {
			char *addr;

			/********************************************/
			/* we have two pointer types.  find the     */
			/* pointer values, array locations, common  */
			/* block locations, or ld symbol locations. */
			/********************************************/

			/**********************************************/
			/* the new type must be strictly a pointer to */
			/* something, arrays, etc. are illegal types  */
			/* for a cast result.                         */
			/**********************************************/
			if (new_D_token != nel_D_POINTER) {
				/*****************************************/
				/* the standard UNIX C compiler says:    */
				/* "illegal lhs of assignment operator". */
				/*****************************************/
				intp_stmt_error (eng, "illegal type for cast");
			}

			if (old_D_token == nel_D_POINTER) {
				addr = (* ((char **) operand->value));
			} else /* array, common, location */
			{
				addr = operand->value;
			}

			/************************************************/
			/* allocate the result symbol and perform the   */
			/* the coercions.  there is no need to compare  */
			/* base types - any pointer may legally be cast */
			/* to any pointer of another type.              */
			/************************************************/
			retval = intp_dyn_symbol_alloc (eng, NULL, type, NULL, nel_C_RESULT, /*zhangbin, 2006-5-18*/operand->lhs, nel_L_NEL, eng->intp->level);
			retval->value = intp_dyn_value_alloc (eng, sizeof (char *), nel_alignment_of (char *));
			*((char **) (retval->value)) = addr;
		} else {
			/******************************************/
			/* we have a pointer and some other type. */
			/******************************************/

			if (nel_pointer_D_token (new_D_token)) {
				/**********************************************/
				/* the new type must be strictly a pointer to */
				/* something, arrays, etc. are illegal types  */
				/* for a cast result.                         */
				/**********************************************/
				if (new_D_token != nel_D_POINTER) {
					/*****************************************/
					/* the standard UNIX C compiler says:    */
					/* "illegal lhs of assignment operator". */
					/*****************************************/
					intp_stmt_error (eng, "illegal type for cast");
				}

				/*********************************************/
				/* one may only cast integral types to       */
				/* pointers without a warning being emitted. */
				/*********************************************/
				if (! nel_integral_D_token (operand->type->simple.type)) {
					intp_warning (eng, "illegal pointer conversion");
				}

				/**********************************************/
				/* allocate the symbol for the result.        */
				/* coerce the integral operand to unsigned    */
				/* long int manually, then call intp_coerce to  */
				/* coerce this to a pointer. on some systems, */
				/* an unsigned int is not large enough to     */
				/* hold all the possible values of a pointer. */
				/**********************************************/
				{
					unsigned_long_int int_op;
					retval = intp_dyn_symbol_alloc (eng, NULL, type, NULL, nel_C_RESULT, /*zhangbin, 2006-5-18*/operand->lhs, nel_L_NEL, eng->intp->level);
					retval->value = intp_dyn_value_alloc (eng, sizeof (char *), nel_alignment_of (char *));
					intp_coerce (eng, nel_unsigned_long_int_type, (char *) (& int_op), operand->type, operand->value);
					*((char **) (retval->value)) = (char *) int_op;
				}
			} else /* nel_pointer_D_token (old_D_token) */
			{
				char *addr;

				/*********************************************/
				/* the operand has a pointer types.  find    */
				/* pointer value, array location, common the */
				/* block location, or ld symbol location.    */
				/*********************************************/
				if (old_D_token == nel_D_POINTER) {
					addr = (* ((char **) operand->value));
				} else /* array, common, location */
				{
					addr = operand->value;
				}

				/*******************************************/
				/* allocate the symbol for the result.     */
				/* coerce the pointer operand to unsigned  */
				/* long int manually, then call intp_coerce  */
				/* to coerce this to an arbitrary integral */
				/* type.  on some systems, an unsigned int */
				/* is not large enough to hold all the     */
				/* possible values of a pointer.           */
				/*******************************************/
				{
					unsigned_long_int int_res;
					retval = intp_dyn_symbol_alloc (eng, NULL, type, NULL, nel_C_RESULT, /*zhangbin, 2006-5-18*/operand->lhs, nel_L_NEL, eng->intp->level);
					retval->value = intp_dyn_value_alloc (eng, type->simple.size, type->simple.alignment);
					int_res = (unsigned_long_int) addr;
					intp_coerce (eng, type, retval->value, nel_unsigned_long_int_type, (char *) (& int_res));
				}
			}
		}
	} else {
		/**************************************************/
		/* we have two numerical types, if all is correct */
		/**************************************************/
		if ((! nel_numerical_D_token (new_D_token)) || (! nel_numerical_D_token (old_D_token))) {
			intp_stmt_error (eng, "illegal operand of cast");
		}

		/****************************************/
		/* allocate the result symbol, and call */
		/* intp_coerce to perform the cast.       */
		/****************************************/
		retval = intp_dyn_symbol_alloc (eng, NULL, type, NULL, nel_C_RESULT, /*zhangbin, 2006-5-18*/operand->lhs, nel_L_NEL, eng->intp->level);
		retval->value = intp_dyn_value_alloc (eng, type->simple.size, type->simple.alignment);
		intp_coerce (eng, type, retval->value, operand->type, operand->value);
	}

	nel_debug ({
				   intp_trace (eng, "] exiting intp_eval_cast\nretval =\n%1S\n", retval);
			   });

	return (retval);

}


/*****************************************************************************/
/* intp_eval_sizeof () perfoms the semantic actions for a sizeof          */
/* operation.  <type> should be the appropriate type descriptor.             */
/*****************************************************************************/
nel_symbol *intp_eval_sizeof (struct nel_eng *eng, register nel_type *type)
{
	register char *res_val;
	register nel_symbol *retval;

	nel_debug ({
				   intp_trace (eng, "entering intp_eval_sizeof [\neng = 0x%x\ntype =\n%1T\n", eng, type);
			   });

	nel_debug ({
				   if (type == NULL) {
				   intp_fatal_error (eng, "(intp_eval_sizeof #1): NULL type")
					   ;
				   }
			   });

	switch (type->simple.type) {
	case nel_D_ARRAY:
		if (type->array.size <= 0) {
			intp_stmt_error (eng, "undefined array bound");
		}
		goto sizeof_set_result1;

	case nel_D_FUNCTION:
		intp_stmt_error (eng, "cannot take size of a function");
		break;

	case nel_D_STRUCT:
	case nel_D_UNION:
		if ((type->s_u.members == NULL) || ((type->s_u.members->symbol == NULL)
											&& (type->s_u.members->next == NULL))) {
			intp_stmt_error (eng, "undefined structure or union");
			break;
		}
		goto sizeof_set_result1;

	case nel_D_ENUM_TAG:
	case nel_D_FILE:
	case nel_D_STAB_UNDEF:
	case nel_D_STRUCT_TAG:
	case nel_D_UNION_TAG:
		intp_fatal_error (eng, "(intp_eval_sizeof #2): illegal type\n%1T", type);
		break;

	default:
		/************************************************/
		/* allocate space for a pointer for the result, */
		/* and set it to the type of the argument.      */
		/************************************************/
sizeof_set_result1:
		if (type->simple.size <= 0) /* location ? */
		{
			intp_stmt_error (eng, "unknown size");
		} else {
			res_val = intp_dyn_value_alloc (eng, sizeof (int), nel_alignment_of (int));
			*((int *) res_val) = type->simple.size;
			retval = intp_dyn_symbol_alloc (eng, NULL, nel_int_type, res_val, nel_C_RESULT, 0, nel_L_NEL, eng->intp->level);
		}
	}

	nel_debug ({
				   intp_trace (eng, "] exiting intp_eval_sizeof\nretval =\n%1S\n", retval);
			   });

	return (retval);

}


/*****************************************************************************/
/* intp_eval_typeof () perfoms the semantic actions for a typeof          */
/* operation.  <type> should be the appropriate type descriptor.             */
/*****************************************************************************/
nel_symbol *intp_eval_typeof (struct nel_eng *eng, register nel_type *type)
{
	register char *res_val;
	register nel_symbol *tag;
	register nel_type *tag_type_d;
	register nel_type *type_type;
	register nel_symbol *retval;

	nel_debug ({
				   intp_trace (eng, "entering intp_eval_typeof [\neng = 0x%x\ntype =\n%1T\n", eng, type);
			   });

	nel_debug ({
				   if (type == NULL) {
				   intp_fatal_error (eng, "(intp_eval_typeof #1): NULL type")
					   ;
				   }
			   });

	/*******************************************************************/
	/* the return type of a typeof operation is (union nel_TYPE *).     */
	/* lookup nel_TYPE in the static tag table (it should be defined at */
	/* level 0; ignore any definition at an inner scope) and create    */
	/* the type descriptor for an incomplete union if it isn't found.  */
	/* then create a pointer to the union.                             */
	/*******************************************************************/
	if ((tag = nel_lookup_symbol ("nel_TYPE", eng->nel_static_tag_hash, NULL)) == NULL) {
		type_type = nel_type_alloc (eng, nel_D_UNION, 0, 0, 0, 0, NULL, NULL);
		tag_type_d = nel_type_alloc (eng, nel_D_UNION_TAG, 0, 0, 0, 0, type_type);
		tag = nel_static_symbol_alloc (eng, nel_insert_name (eng, "nel_TYPE"),
									   tag_type_d, (char *) type_type, nel_C_TYPE, 0, nel_L_NEL, 0);
		type_type->s_u.tag = tag;
		nel_insert_symbol (eng, tag, eng->nel_static_tag_hash);
	} else {
		nel_debug ({
					   if (tag->type == NULL) {
					   intp_fatal_error (eng, "(intp_eval_typeof #2): bad symbol\n%S", tag)
						   ;
					   }
				   });
		if (tag->type->simple.type != nel_D_UNION_TAG) {
			intp_stmt_error (eng, "redeclaration of nel_TYPE: typeof disabled");
		}
		nel_debug ({
					   if (tag->type->tag_name.descriptor == NULL) {
					   intp_fatal_error (eng, "(intp_eval_typeof #3): bad symbol\n%S", tag)
						   ;
					   }
				   });
		type_type = tag->type->tag_name.descriptor;
	}

	type_type = nel_type_alloc (eng, nel_D_POINTER, sizeof (char *), nel_alignment_of (char *), 0, 0, type_type);

	/************************************************/
	/* allocate space for a pointer for the result, */
	/* and set it to the type of the argument.      */
	/************************************************/
	res_val = intp_dyn_value_alloc (eng, sizeof (char *), nel_alignment_of (char *));
	*((nel_type **) res_val) = type;
	retval = intp_dyn_symbol_alloc (eng, NULL, type_type, res_val, nel_C_RESULT, 0, nel_L_NEL, eng->intp->level);

	nel_debug ({
				   intp_trace (eng, "] exiting intp_eval_typeof\nretval =\n%1S\n", retval);
			   });

	return (retval);

}


/*****************************************************************************/
/* intp_eval_member () perfoms the semantic actions for extracting a      */
/* member from a struct/union.                                               */
/*****************************************************************************/
nel_symbol *intp_eval_member (struct nel_eng *eng, register nel_symbol *s_u, register nel_symbol *member)
{
	register nel_type *type;
	register nel_member *members;
	register nel_symbol *retval;

	nel_debug ({
				   intp_trace (eng, "entering intp_eval_member [\neng = 0x%x\ns_u =\n%1S\nmember =\n%1S\n", eng, s_u, member);
			   });

	nel_debug ({
				   if ((s_u == NULL) || (s_u->type == NULL)) {
				   intp_fatal_error (eng, "(intp_eval_member #1): bad symbol\n%1S", s_u)
					   ;
				   }
			   });
	type = s_u->type;
	if ((type->simple.type != nel_D_STRUCT) && (type->simple.type != nel_D_UNION)) {
		intp_stmt_error (eng, "struct/union expected");
	}

	retval = NULL;
	for (members = type->s_u.members; (members != NULL); members = members->next) {
		nel_debug ({
					   if ((members->symbol == NULL) || (members->symbol->name == NULL)) {
					   intp_fatal_error (eng, "(intp_eval_member #2): bad symbol\n%1S", members->symbol)
						   ;
					   }
				   });
		if (! strcmp (members->symbol->name, member->name)) {
			retval = members->symbol;
			break;
		}
	}
	if (retval == NULL) {
		intp_stmt_error (eng, "%I undefined", member);
	}
	type = members->symbol->type;
	nel_debug ({
				   if ((type == NULL) || (type->simple.size <= 0)) {
				   intp_fatal_error (eng, "(intp_eval_member #3): bad type\n%1T", type)
					   ;
				   }
			   });

	/*******************************************************/
	/* allocate the retval symbol.                         */
	/* it is a legal lhs of an assignment if the member is */
	/* a legal lhs of an assignment.  (i.e. the member is  */
	/* not an array.)  just in case we start having const  */
	/* structures, make sure the struct itself is a legal  */
	/* lhs, also.                                          */
	/*******************************************************/
	retval = intp_dyn_symbol_alloc (eng, NULL, type, NULL, nel_C_RESULT,
								   nel_lhs_type (type) && s_u->lhs, nel_L_NEL, eng->intp->level);

	if (members->bit_field) {
		/****************************************************/
		/* extract the bit field from the entire word.  be  */
		/* sure to set the "member" and "s_u" fields of the */
		/* result symbol, so that we may insert a new value */
		/* in the bit field if this symbol appears as the   */
		/* lhs of an assignment.                            */
		/****************************************************/
		unsigned_int word;
		nel_copy (sizeof (int), &word, s_u->value + members->offset);
		retval->value = intp_dyn_value_alloc (eng, sizeof (int), nel_alignment_of (int));
		nel_extract_bit_field (eng, type, retval->value, word, members->bit_lb, members->bit_size);
		retval->member = members;
		retval->s_u = s_u;
	} else {
		retval->value = s_u->value + members->offset;
	}

	nel_debug ({
				   intp_trace (eng, "] exiting intp_eval_member\nretval =\n%1S\n", retval);
			   });

	return (retval);

}


/*****************************************************************************/
/* intp_nonzero () returns 1 if the value of <operand> is nonzero.        */
/* otherwise, it returns 0.                                                  */
/*****************************************************************************/
unsigned_int intp_nonzero (struct nel_eng *eng, register nel_symbol *operand)
{
	nel_type *type;
	register nel_D_token D_token;
	unsigned_int retval = 0;

	nel_debug ({
		intp_trace (eng, "entering intp_nonzero [\neng = 0x%x\nsym =\n%1S\n", eng, operand);
	});

	/*************************************************/
	/* set local variables to the appropriate fields */
	/*************************************************/

	nel_debug ({
		if ((operand == NULL) || (operand->type == NULL) 
			|| (operand->value == NULL)) {
			intp_fatal_error (eng, "(intp_nonzero #1): bad operand\n%1S", operand);
		}
	});

	type = operand->type;
	D_token = type->simple.type;

	if (nel_pointer_D_token (D_token)) {
		if (D_token == nel_D_POINTER) {
			retval = *((char **) (operand->value)) != 0;
		} else /* array, common, location, should always be nonzero */
		{
			retval = operand->value != 0;
		}
	} else if (nel_integral_D_token (D_token)) {
		/********************************************************/
		/* coerce ints to long to avoid overflow - don't worry  */
		/* about the sign changing on unsigned types, as we are */
		/* only comparing to 0.                                 */
		/********************************************************/
		signed_long_int value;
		intp_coerce (eng, nel_signed_long_int_type, (char *) (&value), type, operand->value);
		retval = (value != 0);
	} else if (nel_floating_D_token (D_token)) {
		/****************************************************/
		/* coerce floats's to long double to avoid overflow */
		/****************************************************/
		long_double value;
		intp_coerce (eng, nel_long_double_type, (char *) (&value), type, operand->value);
		retval = (value != 0);
	} else {
		intp_fatal_error (eng, "(intp_nonzero #2): illegal type\n%1T", type);
	}

	nel_debug ({
		intp_trace (eng, "] exiting intp_nonzero\nretval = %d\n\n", retval);
	});

	return (retval);

}



/*****************************************************************************/
/* intp_resolve_type ()traverses the type descriptor pointed to by <type>.    */
/* if any arrays are found with variables in their dimension bounds, the     */
/* of the bound(s) are extracted and a new type descriptor formed with       */
/* integral constants substituted for the expressions.  (a pointer to) the   */
/* new type descriptor is returned.  <name> is used for error messages.      */
/*****************************************************************************/
nel_type *intp_resolve_type (struct nel_eng *eng, register nel_type *type, char *name)
{
	register nel_type *retval;

	nel_debug ({ intp_trace (eng, "entering intp_resolve_type [\neng = 0x%x\ntype =\n%1T\n", eng, type); });
	if (type == NULL) {
		return NULL;
	} 

	switch (type->simple.type) {
	case nel_D_ARRAY: 
		{
		int lb;
		int ub;

		/***************************/
		/* extract the lower bound */
		/***************************/
		if (type->array.known_lb) {
			lb = type->array.lb.value;
		} else {
			register nel_symbol *bound_sym;
			bound_sym = intp_eval_expr (eng, type->array.lb.expr);

			if ((bound_sym != NULL) && (bound_sym->type != NULL) && (bound_sym->value != NULL) && (nel_integral_D_token (bound_sym->type->simple.type))) {
				intp_coerce (eng, nel_int_type, (char *)&lb, bound_sym->type, (char *)bound_sym->value);

			} else {
				lb = 0;
				if ((bound_sym == NULL) || (bound_sym->name == NULL)) {
					if (name == NULL) {
						intp_warning (eng, "unresolved dim lb defaults to 0");
					} else {
						intp_warning (eng, "unresolved dim lb defaults to 0: symbol %s", name);
					}
				} else {
					if (name == NULL) {
						intp_warning (eng, "unresolved dim lb %I defaults to 0", bound_sym);
					} else {
						intp_warning (eng, "unresolved dim lb %I defaults to 0: symbol %s", bound_sym, name);
					}
				}
			}
		}

		/***************************/
		/* extract the upper bound */
		/***************************/
		if (type->array.known_ub) {
			ub = type->array.ub.value;
		} else {
			register nel_symbol *bound_sym;
			bound_sym = intp_eval_expr (eng, type->array.ub.expr);

			if ((bound_sym != NULL) && (bound_sym->type != NULL) && (bound_sym->value != NULL) && (nel_integral_D_token (bound_sym->type->simple.type))) {
				intp_coerce (eng, nel_int_type, (char *)&ub, bound_sym->type, (char *)bound_sym->value);
				nel_debug ({
					intp_trace (eng, "bound_sym =\n%1Svalue = 0x%x\n*value = 0x%x\n\n", bound_sym, bound_sym->value, *((int *) (bound_sym->value)));
				});
			} else {
				ub = lb;
				if ((bound_sym == NULL) || (bound_sym->name == NULL)) {
					if (name == NULL) {
						intp_warning (eng, "unresolved dim ub defaults to lb (%d)", lb);
					} else {
						intp_warning (eng, "unresolved dim ub defaults to lb (%d): symbol %s", lb, name);
					}
				} else {
					if (name == NULL) {
						intp_warning (eng, "unresolved dim ub %I defaults to lb (%d)", bound_sym, lb);
					} else {
						intp_warning (eng, "unresolved dim ub %I defaults to lb (%d): symbol %s", bound_sym, lb, name);
					}
				}
			}
		}

		/****************************************/
		/* make sure lb <= ub.  if lb > ub, set */
		/* lb = ub, and emit a warning          */
		/****************************************/
		if (lb > ub) {
			if (name == NULL) {
				intp_warning (eng, "dim ub (%d) defaults to larger lb (%d)", ub, lb);
			} else {
				intp_warning (eng, "dim ub (%d) defaults to larger lb (%d): symbol %s", ub, lb, name);
			}
			ub = lb;
		}

		/***********************************************/
		/* now create the return value type descriptor */
		/* the base type must be integral, so don't    */
		/* bother calling intp_resolve_type on it        */
		/***********************************************/
		retval = intp_resolve_type (eng, type->array.base_type, name);
		if (retval != NULL) {
			retval = nel_type_alloc (eng, nel_D_ARRAY, (ub - lb + 1) * retval->simple.size, retval->simple.alignment, type->array._const, type->array._volatile, retval, nel_int_type, 1, lb, 1, ub);

		}
		}
		break;

	case nel_D_FUNCTION:
		retval = nel_type_alloc (eng, 
					nel_D_FUNCTION, 
					0, 
					0,
					type->function._const, 
					type->function._volatile,
					type->function.new_style, 
					type->function.var_args,
					intp_resolve_type(eng,type->function.return_type,name),
					type->function.args, 
					type->function.blocks,
					type->function.file 
			);
		break;

	case nel_D_POINTER:
		retval = nel_type_alloc (eng, 
					nel_D_POINTER, 
					sizeof (char *),
					nel_alignment_of (char *), 
					type->pointer._const, 
					type->pointer._volatile,
					intp_resolve_type (eng, type->pointer.deref_type, name)
				);
		break;

	case nel_D_STRUCT:
	case nel_D_UNION:
#if 1
		/****************************************************/
		/* struct/unions members must be defined statically */
		/****************************************************/
		retval = type;
		break;

#else
		{
		register nel_member *scan;
		register nel_member *start = NULL;
		register nel_member *last = NULL;

		for (scan = type->s_u.members; (scan != NULL); scan = scan->next)
		{
			register nel_symbol *symbol = scan->symbol;
			register nel_member *member;
			symbol = nel_static_symbol_alloc (eng, symbol->name, intp_resolve_type (eng, symbol->type, symbol->name), symbol->value, symbol->class, symbol->lhs, symbol->source_lang, symbol->level);
			member = nel_member_alloc (eng, symbol, scan->bit_field, scan->bit_size, scan->bit_lb, scan->offset, NULL);
			symbol->member = member;
			if (last == NULL) {
				start = last = member;
			} else {
				last->next = member;
				last = member;
			}
		}
		retval = nel_type_alloc (eng, type->s_u.type, type->s_u.size, type->s_u.alignment, type->s_u._const, type->s_u._volatile, type->s_u.tag, start);
		if (type->simple.type == nel_D_STRUCT)
		{
			nel_set_struct_offsets (eng, retval);
		} else /* (type->simple.type == nel_D_UNION) */
		{
			unsigned_int alignment;
			retval->s_u.size = nel_s_u_size (eng, &alignment, retval->s_u.members);
			retval->s_u.alignment = alignment;
		}
		}
		break;
#endif /* 0 */

		/****************/
		/* simple types */
		/****************/
	default:
		retval = type;
		break;
	}

	nel_debug ({ intp_trace (eng, "] exiting intp_resolve_type\nretval =\n%1T\n", retval); });
	return (retval);
}

/*****************************************************************************/
/* intp_eval_expr () evaluates the expression tree and returns the symbol */
/* representing the result.                                                  */
/*****************************************************************************/
nel_symbol *intp_eval_expr (struct nel_eng *eng, register nel_expr *expr)
{
	//register -> , wyong, 20230816 
	//register nel_symbol *retval;
	nel_symbol *retval;

	nel_debug ({
		intp_trace (eng, "entering intp_eval_expr [\neng = 0x%x\nexpr =\n%1X\n", eng, expr);
	});

	if (expr == NULL) {
		retval = NULL;
	} else {
		switch (expr->gen.opcode) {

			/****************************/
			/* terminal node - a symbol */
			/****************************/
		case nel_O_SYMBOL: 
			retval = expr->symbol.symbol;
			nel_debug ({
			if (retval == NULL) {
				intp_fatal_error (eng, "(intp_eval_expr #1): bad expr\n%1X", expr);
			}
			});

			/**********************************************************/
			/* constants already have their class set to nel_C_CONST,  */
			/* and no lookup is performed on them.  all other symbols */
			/* are condisered template symbols (though those that     */
			/* have static storage will be the actual symbol returned */
			/* from the lookup, unless nel_load_dyn_scope() has masked */
			/* the symbol in scope at parse time).                    */
			/**********************************************************/
			if ((retval->class != nel_C_CONST) 
			&& (retval->name != NULL)) {

				if(retval->class==nel_C_STATIC_GLOBAL){
					retval = retval->_global;
				}

				if ( (retval = intp_lookup_ident(eng, retval->name)) == NULL ){
					nel_debug ({
					if (expr->symbol.symbol->name == NULL) {
						intp_fatal_error (eng, "(intp_eval_expr #2): unnamed template symbol\n%1S", expr->symbol.symbol);
					}
					});

					intp_stmt_error (eng, "%I undefined", expr->symbol.symbol);

				}

			}
			break;

			/****************************/
			/* regular unary operations */
			/****************************/
		case nel_O_BIT_NEG:
		case nel_O_NEG:
		case nel_O_POS:
			retval = intp_eval_expr (eng, expr->unary.operand);
			retval = intp_eval_unary_op (eng, expr->unary.opcode, retval);
			break;

			/**********************************/
			/* increment/decrement operations */
			/**********************************/
		case nel_O_POST_DEC:
		case nel_O_POST_INC:
		case nel_O_PRE_DEC:
		case nel_O_PRE_INC:
			retval = intp_eval_expr (eng, expr->unary.operand);
			retval = intp_eval_inc_dec (eng, expr->unary.opcode, retval);
			break;

			/*****************************/
			/* regular binary operations */
			/*****************************/
		case nel_O_ADD:
		case nel_O_BIT_AND:
		case nel_O_BIT_OR:
		case nel_O_BIT_XOR:
		case nel_O_DIV:
		case nel_O_EQ:
		case nel_O_GE:
		case nel_O_GT:
		case nel_O_LE:
		case nel_O_LSHIFT:
		case nel_O_LT:
		case nel_O_MOD:
		case nel_O_MULT:
		case nel_O_NE:
		case nel_O_RSHIFT:
		case nel_O_SUB: {
				register nel_symbol *left = intp_eval_expr (eng, expr->binary.left);
				register nel_symbol *right = intp_eval_expr (eng, expr->binary.right);
				retval = intp_eval_binary_op (eng, expr->binary.opcode, left, right);
			}
			break;

			/**********************/
			/* regular assignment */
			/**********************/
		case nel_O_ASGN: {
				register nel_symbol *left = intp_eval_expr (eng, expr->binary.left);
				register nel_symbol *right = intp_eval_expr (eng, expr->binary.right);
				//added by zhangbin, 2006-5-17
				if(!right)
				{
					if(expr->binary.right->gen.opcode == nel_O_LIST 
						&& (left->type->simple.type == nel_D_STRUCT || left->type->simple.type == nel_D_UNION))//init struct/union var
					{
						if(!nel_s_u_init(eng, left, expr->binary.right))
						{
							intp_stmt_error(eng, "struct/union var init failed");
							retval = NULL;
						}
						retval = intp_dyn_symbol_alloc (eng, NULL, left->type, NULL, nel_C_RESULT, 0, nel_L_NEL, eng->intp->level);
						retval->value = intp_dyn_value_alloc (eng, left->type->simple.size, left->type->simple.alignment);
						nel_copy (left->type->simple.size, retval->value, left->value);
						break;
					}
				}
				else//end
					retval = intp_eval_asgn (eng, expr->binary.opcode, left, right);
			}
			break;

			/*********************/
			/* other assignments */
			/*********************/
		case nel_O_ADD_ASGN:
		case nel_O_BIT_AND_ASGN:
		case nel_O_BIT_OR_ASGN:
		case nel_O_BIT_XOR_ASGN:
		case nel_O_DIV_ASGN:
		case nel_O_LSHIFT_ASGN:
		case nel_O_MOD_ASGN:
		case nel_O_MULT_ASGN:
		case nel_O_RSHIFT_ASGN:
		case nel_O_SUB_ASGN: {
				register nel_symbol *left = intp_eval_expr (eng, expr->binary.left);
				register nel_symbol *right = intp_eval_expr (eng, expr->binary.right);
				retval = intp_eval_binary_op (eng, expr->binary.opcode, left, right);
				retval = intp_eval_asgn (eng, expr->binary.opcode, left, retval);
			}
			break;

		case nel_O_COMPOUND:
			intp_eval_expr (eng, expr->binary.left);
			retval = intp_eval_expr (eng, expr->binary.right);
			break;

			/***********************************************/
			/* logical operations                          */
			/* AND, NOT, and OR, they get you pretty far!  */
			/* conjunction junction, what's your function? */
			/***********************************************/
		case nel_O_AND:
			if (! intp_nonzero (eng, intp_eval_expr (eng, expr->binary.left))) {
				retval = nel_zero_symbol;
				//added by zhangbin, 2006-5-19, only to check if the right operand is legal
				//intp_nonzero(eng, intp_eval_expr(eng, expr->binary.right));
				//end
			} else if (! intp_nonzero (eng, intp_eval_expr (eng, expr->binary.right))) {
				retval = nel_zero_symbol;
			} else {
				retval = nel_one_symbol;
			}
			break;

		case nel_O_OR:
			if (intp_nonzero (eng, intp_eval_expr (eng, expr->binary.left))) {
				retval = nel_one_symbol;
				//added by zhangbin, 2006-5-19, only to check if the right operand if legal
				//intp_nonzero(eng, intp_eval_expr(eng, expr->binary.right));
				//end
			} else if (intp_nonzero (eng, intp_eval_expr (eng, expr->binary.right))) {
				retval = nel_one_symbol;
			} else {
				retval = nel_zero_symbol;
			}
			break;

		case nel_O_NOT:
			if (intp_nonzero (eng, intp_eval_expr (eng, expr->unary.operand))) {
				retval = nel_zero_symbol;
			} else {
				retval = nel_one_symbol;
			}
			break;

			/******************************************************/
			/* address and pointer dereferencing / array indexing */
			/******************************************************/
		case nel_O_ADDRESS:
			retval = intp_eval_expr (eng, expr->unary.operand);
			retval = intp_eval_address (eng, retval);
			break;

		case nel_O_ARRAY_INDEX: {
				register nel_symbol *left;
				register nel_symbol *right;
				left = intp_eval_expr (eng, expr->binary.left);
				right = intp_eval_expr (eng, expr->binary.right);
				left = intp_eval_binary_op (eng, nel_O_ARRAY_INDEX, left, right);
				retval = intp_eval_deref (eng, left);
			}
			break;

		case nel_O_DEREF:
			retval = intp_eval_expr (eng, expr->unary.operand);
			retval = intp_eval_deref (eng, retval);
			break;

			/*****************/
			/* function call */
			/*****************/
		case nel_O_FUNCALL: {
				//register -> , wyong, 20230816 	
				//register nel_expr_list *args;
				//register nel_stack *arg_start;
				//register int nargs;
				//register nel_symbol *func = intp_eval_expr (eng, expr->funcall.func);
				nel_expr_list *args;
				nel_stack *arg_start;
				int nargs;
				nel_symbol *func = intp_eval_expr (eng, expr->funcall.func);

//added by zhangbin, 2006-5-22
//				intp_push_symbol(eng, eng->intp->prog_unit);
//				eng->intp->prog_unit = func;
//end

				/*******************************************************/
				/* scan through the argument list, pushing the symbols */
				/* for the evaluated arguments.  we use the stack to   */
				/* allocate space for the symbols, as it needs to be   */
				/* reclaimed after the call.                           */
				/*******************************************************/
//added by zhangbin, 2006-5-20
				nel_list *temp_list = func->type->function.args;
//end
				for (args = expr->funcall.args, nargs = 0; (args != NULL); args = args->next, nargs++) 
				{
					/* wyong, 2006.5.24 
					//added by zhangbin, 2006-5-22
					if(!temp_list)
					{
						if(!func->type->function.var_args)
							intp_stmt_error(eng, "more arguments than %s definition", func->name);
					}
					else
						temp_list = temp_list->next;
					//end
					*/
					register nel_symbol *result = intp_eval_expr (eng, args->expr);
					intp_push_symbol (eng, result);
				}

				/* wyong, 2006.5.24 
				//added by zhangbin, 2006-5-22
				if(temp_list)
					intp_stmt_error(eng, "less arguments than %s definition", func->name);
				//end
				*/

				/*******************************************************/
				/* retreive a pointer to the start of the pushed args. */
				/* and call intp_eval_call() to perform the call.        */
				/*******************************************************/
				intp_top_pointer (eng, arg_start, nargs - 1);
				retval = intp_eval_call (eng, func, nargs, arg_start);

#ifdef NEL_DEBUG
				/**************************************************/
				/* while debugging, pop the args individually, so */
				/* that the ['s and ]'s match up in the trace.    */
				/**************************************************/
				for (; (nargs > 0); nargs--) {
					register nel_symbol *arg;
					intp_pop_symbol (eng, arg);
				}
#else
				/********************************************/
				/* when not debugging pop the args (and the */
				/* function) by setting the stack pointer.  */
				/********************************************/
				intp_set_stack (eng, arg_start - 1);
#endif /* NEL_DEBUG */

//added by zhangbin, 2006-5-22
//				intp_pop_symbol(eng, eng->intp->prog_unit);
//end
			}
			break;

			/*************/
			/* type cast */
			/*************/
		case nel_O_CAST:
			retval = intp_eval_expr (eng, expr->type.expr);
			retval = intp_eval_cast (eng, expr->type.type, retval);
			break;

			/*******************/
			/* sizeof operator */
			/*******************/
		case nel_O_SIZEOF:
			if (expr->type.type != NULL) {
				retval = intp_eval_sizeof (eng, expr->type.type);
			} else {
				nel_debug ({
							   if (expr->type.expr == NULL) {
							   intp_fatal_error (eng, "(intp_eval_expr #3): bad type expr\n%1X", expr)
								   ;
							   }
						   });
				retval = intp_eval_expr (eng, expr->type.expr);
				nel_debug ({
							   if (retval == NULL) {
							   intp_fatal_error (eng, "(intp_eval_expr #4): NULL evaluated sizeof arg")
								   ;
							   }
						   });
				retval = intp_eval_sizeof (eng, retval->type);
			}
			break;

			/*******************/
			/* typeof operator */
			/*******************/
		case nel_O_TYPEOF:
			if (expr->type.type != NULL) {
				register nel_type *type = intp_resolve_type (eng, expr->type.type, NULL);
				retval = intp_eval_typeof (eng, type);
			} else {
				nel_debug ({
							   if (expr->type.expr == NULL) {
							   intp_fatal_error (eng, "(intp_eval_expr #5): bad type expr\n%1X", expr)
								   ;
							   }
						   });
				retval = intp_eval_expr (eng, expr->type.expr);
				nel_debug ({
							   if (retval == NULL) {
							   intp_fatal_error (eng, "(intp_eval_expr #6): NULL evaluated typeof arg")
								   ;
							   }
						   });
				retval = intp_eval_typeof (eng, retval->type);
			}
			break;

			/***********************/
			/* struct/union member */
			/***********************/
		case nel_O_MEMBER:
			retval = intp_eval_expr (eng, expr->member.s_u);
			retval = intp_eval_member (eng, retval, expr->member.member);
			break;

			/******************/
			/* conditional: ? */
			/******************/
		case nel_O_COND:
			if (intp_nonzero (eng, intp_eval_expr (eng, expr->cond.cond))) {
				retval = intp_eval_expr (eng, expr->cond.true_expr);
			} else {
				retval = intp_eval_expr (eng, expr->cond.false_expr);
			}
			break;
		
		//added by zhangbin, to support union/struct var init while declaraing union/struct var
		case nel_O_LIST:
			retval = (nel_symbol*)NULL;
			break;
		//end, 2006-5-17
		
			/*********************************************************/
			/* the following should never occur as the discriminator */
			/* of the intp_eval_expr union -                      */
			/* they are only used for error messages.                */
			/*********************************************************/
		case nel_O_MEMBER_PTR:
		case nel_O_RETURN:
		default:
			intp_fatal_error (eng, "(intp_eval_expr #2): illegal opcode %O", expr->gen.opcode);
			break;
		}
	}

	nel_debug ({
				   intp_trace (eng, "] exiting intp_eval_expr\nretval =\n%1S\n", retval);
			   });

	return (retval);

}

/*****************************************************************************/
/* intp_eval_expr_2() interprets the routine whose symbol table entry is   */
/* <symbol>, passing the remaining arguments to it, and returning the result */
/* in *<retloc>.                                                             */
/*****************************************************************************/
nel_symbol *intp_eval_expr_2 (struct nel_eng *eng, register nel_expr *expr)
{
	/********************************************/
	/* register variables are not guaranteed to */
	/* survive a longjmp () on some systems     */
	/********************************************/
	nel_jmp_buf return_pt;
	struct eng_intp *old_intp = NULL;
	int val;
	nel_symbol *retval=NULL;

	//nel_debug ({ intp_trace (eng, "entering intp_eval () [\nretloc = 0x%x\nsymbol =\n%1Sformal_ap = 0x%x\n\n", retloc, symbol, formal_ap); });
	if(eng->intp != NULL){
		old_intp = eng->intp;
	}

	/*********************************************/
	/* initialize the engine record that is passed */
	/* thoughout the entire call sequence.       */
	/* nel_intp_eval () will set the filename and */
	/* line numbers from the stmt structures     */
	/*********************************************/
	nel_malloc(eng->intp, 1, struct eng_intp );
	intp_init (eng, "", NULL, NULL, NULL, 0);

	/**********************/
	/* call the evaluator */
	/**********************/
	eng->intp->return_pt = &return_pt;
	intp_setjmp (eng, val, &return_pt);
	if (val == 0) {
		retval = intp_eval_expr (eng, (nel_expr *) expr );
	}

	/*******************************************************/
	/* deallocate any dynamically allocated substructs     */
	/* withing the engine (if they were stack allocated) */
	/* and return.                                         */
	/*******************************************************/
	//retval = eng->intp->error_ct;  /*bugfix, wyong, 2006.4.12 */
	retval = nel_symbol_dup(eng, retval);  /*bugfix, wyong, 2006.4.12 */
	intp_dealloc (eng);

	nel_debug ({ intp_trace (eng, "] exiting intp_eval ()\nerror_ct = %d\n\n", eng->intp->error_ct); });
	if(old_intp != NULL){
		nel_dealloca(eng->intp);	//added by zhangbin, 2006-7-20
		eng->intp = old_intp;
	}

	return (retval);
}

/*****************************************************************************/
/* intp_eval_stmt () traverses the stmt tree, executing the appropriate   */
/* actions.                                                                  */
/*****************************************************************************/
void intp_eval_stmt (struct nel_eng *eng, register nel_stmt *list)
{
	int val;		/* return code for intp_setjmp      	*/
	nel_stack *context;	/* context ptr for outermost level	*/
	char *filename = eng->intp->filename; /* save filename and line #*/
	int line = eng->intp->line; /* in current nel_eng  - will restore.*/

	//nel_symbol *file;

	nel_debug ({
		intp_trace (eng, "entering intp_eval_stmt [\neng = 0x%x\n\n", eng);
	});


	/**************************************************************/
	/* first, save the current context, but leave the level at 0. */
	/* unless we are called from the interpreter, the error jumps */
	/* have not been allocated yet.  save a pointer to this outer */
	/* context in case we exit from an inner level.               */
	/**************************************************************/
	save_intp_context (eng);
	context = eng->intp->context;

	while (list != NULL) {

		nel_debug ({
			intp_trace (eng, "list =\n%1K\n", list);
		});

		/***************************************/
		/* set the filename and line number in */
		/* the engine  for error messages. */
		/***************************************/
		eng->intp->filename = list->gen.filename;
		eng->intp->line = list->gen.line;


		/* wyong 2004.5.19 */
		//file = nel_lookup_file(eng, list->gen.filename);
		//if(file){
		//	nel_load_static_scope(eng, file);
		//}


		switch (list->gen.type) {

		/*******************************/
		/* declaration of a new symbol */
		/*******************************/
		case nel_S_DEC:
			intp_setjmp (eng, val, eng->intp->stmt_err_jmp);
			if (val != 0) {
				nel_debug ({
					intp_trace (eng, "stmt error longjmp caught\n\n");
				});
				intp_diagnostic (eng, "continuing with next statement");
			} else {
				register nel_symbol *symbol = list->dec.symbol;
				register nel_type *type;
				nel_debug ({
					if ((symbol == NULL) || (symbol->type == NULL)) {
						intp_fatal_error (eng, "(intp_eval_stmt #1): bad symbol\n%1S", symbol);
					}
				});

				type = symbol->type;
				switch (symbol->class) {

				case nel_C_LOCAL:
				case nel_C_REGISTER_LOCAL:
					/*********************************************/
					/* for local variables, create a copy of the */
					/* symbol with the type descriptor resolved, */
					/* allocate space for it, and insert it in   */
					/* the hash tables.                          */
					/* first, resolve any non-constant dimension */
					/* bounds and make sure the type is complete */
					/*********************************************/
					//bugfix, wyong, 2005.11.25
					//type = intp_resolve_type (eng, type, symbol->name);
					nel_debug ({
					if (type == NULL) {
						intp_fatal_error (eng, "(intp_eval_stmt #2): NULL resolved type");
					}
					});

					if (type->simple.type == nel_D_VOID)
					{
						intp_error (eng, "void type for %I", symbol);
					} else if (nel_type_incomplete (type))
					{
						intp_error (eng, "incomplete type for %I", symbol);
					} else
					{
						symbol = intp_dyn_symbol_alloc (eng, symbol->name, type, intp_dyn_value_alloc (eng, type->simple.size, type->simple.alignment), symbol->class, symbol->lhs, nel_L_NEL, symbol->level);
						symbol->declared = 1;
						intp_insert_ident (eng, symbol);
					}
					break;

				case nel_C_STATIC_LOCAL:
					/*********************************************/
					/* for static local variables, create a copy */
					/* of the symbol, but use the same storage   */
					/* as is in the template.  the storage has   */
					/* has already been allocated.               */
					/*********************************************/
					symbol = intp_dyn_symbol_alloc (eng, symbol->name, type, symbol->value, symbol->class, symbol->lhs, nel_L_NEL, symbol->level);
					intp_insert_ident (eng, symbol);
					break;

				case nel_C_FORMAL:
				case nel_C_REGISTER_FORMAL:
					/*********************************************/
					/* for arguments, create a copy of the       */
					/* symbol with the type descriptor resolved, */
					/* retrieve the value using va_arg(), and    */
					/* insert the symbol in the hash tables.     */
					/* first, resolve any non-constant dimension */
					/* bounds and make sure the type is complete */
					/*********************************************/
					//bugfix, wyong, 2005.11.25
					//type = intp_resolve_type (eng, type, symbol->name);
					nel_debug ({
					if (type == NULL) {
						intp_fatal_error (eng, "(intp_eval_stmt #3): NULL resolved type");
					}
					});
					if (type->simple.type == nel_D_VOID)
					{
						intp_error (eng, "void type for %I", symbol);
					} else if (nel_type_incomplete (type))
					{
						intp_error (eng, "incomplete type for %I", symbol);
					} else
					{
						symbol = intp_dyn_symbol_alloc (eng, symbol->name, type, intp_dyn_value_alloc (eng, type->simple.size, type->simple.alignment), symbol->class, symbol->lhs, nel_L_NEL, symbol->level);

						switch (type->simple.type) {

							/************************************************/
							/* note that arguments shorter than an int have */
							/* undergone integral promotion by the caller,  */
							/* and must be fetched using va_arg on int or   */
							/* unsigned.  the argument is still of the      */
							/* shorter data type, even though it was not    */
							/* passed that way.  this can be seen by        */
							/* sizeof (char parameter) == 1.                */
							/* floats that are promoted to doubles, and     */
							/* arrays and functions that are promoted to    */
							/* pointer do not behave like this, however -   */
							/* both the formal and the actual arguments     */
							/* undergo the promotion, not just the actuals. */
							/************************************************/

#define get_arg(_token,_lhs_type,_rhs_type) \
	case (_token):							\
	symbol->value = intp_dyn_value_alloc(eng,sizeof(_lhs_type),nel_alignment_of (_lhs_type)); \
	*((_lhs_type *) (symbol->value)) = va_arg (*eng->intp->formal_ap, _rhs_type); \
	break;

							get_arg (nel_D_CHAR, char, int)
							get_arg (nel_D_DOUBLE, double, double)
							get_arg (nel_D_ENUM, int, int)
							get_arg (nel_D_INT, int, int)
							get_arg (nel_D_LONG, long, long)
							get_arg (nel_D_LONG_DOUBLE, long_double, long_double)

							/****************************************/
							/* float formals to new-style functions */
							/* will already have been promoted.     */
							/****************************************/
							get_arg (nel_D_LONG_FLOAT, long_float, long_float)
							get_arg (nel_D_LONG_INT, long_int, long_int)
							get_arg (nel_D_POINTER, char *, char *)
							get_arg (nel_D_SHORT, short, int)
							get_arg (nel_D_SHORT_INT, short_int, int)
							get_arg (nel_D_SIGNED, signed, signed)
							get_arg (nel_D_SIGNED_CHAR, signed_char, signed_int)
							get_arg (nel_D_SIGNED_INT, signed_int, signed_int)
							get_arg (nel_D_SIGNED_LONG, signed_long, signed_long)
							get_arg (nel_D_SIGNED_LONG_INT, signed_long_int, signed_long_int)
							get_arg (nel_D_SIGNED_SHORT, signed_short, signed)
							get_arg (nel_D_SIGNED_SHORT_INT, signed_short_int, signed_int)
							get_arg (nel_D_UNSIGNED, unsigned, unsigned)
							get_arg (nel_D_UNSIGNED_CHAR, unsigned_char, unsigned_int)
							get_arg (nel_D_UNSIGNED_INT, unsigned_int, unsigned_int)
							get_arg (nel_D_UNSIGNED_LONG, unsigned_long, unsigned_long)
							get_arg (nel_D_UNSIGNED_LONG_INT, unsigned_long_int, unsigned_long_int)
							get_arg (nel_D_UNSIGNED_SHORT, unsigned, unsigned_int)
							get_arg (nel_D_UNSIGNED_SHORT_INT, unsigned_short_int, unsigned_int)

#undef    get_arg


							/********************************************/
							/* complex types are treated as two floats. */
							/* float formals to new-style functions     */
							/* will already have been promoted.         */
							/********************************************/
						case nel_D_COMPLEX:
						case nel_D_COMPLEX_FLOAT:
							symbol->value = intp_dyn_value_alloc (eng, 2 * sizeof (float), nel_alignment_of (float));
							*((float *) (symbol->value)) = va_arg (*eng->intp->formal_ap, float);
							*(((float *) (symbol->value)) + 1) = va_arg (*eng->intp->formal_ap, float);
							break;

							/***********************************************/
							/* complex doubles are treated as two doubles. */
							/***********************************************/
						case nel_D_COMPLEX_DOUBLE:
						case nel_D_LONG_COMPLEX:
						case nel_D_LONG_COMPLEX_FLOAT:
							symbol->value = intp_dyn_value_alloc (eng, 2 * sizeof (double), nel_alignment_of (double));
							*((double *) (symbol->value)) = va_arg (*eng->intp->formal_ap, double);
							*(((double *) (symbol->value)) + 1) = va_arg (*eng->intp->formal_ap, double);
							break;

							/********************************/
							/* long complex doubles are     */
							/* treated as two long doubles. */
							/********************************/
						case nel_D_LONG_COMPLEX_DOUBLE:
							symbol->value = intp_dyn_value_alloc (eng, 2 * sizeof (long_double), nel_alignment_of (long_double));
							*((long_double *) (symbol->value)) = va_arg (*eng->intp->formal_ap, long_double);
							*(((long_double *) (symbol->value)) + 1) = va_arg (*eng->intp->formal_ap, long_double);
							break;


							/*******************************************/
							/* default assumes that structs and unions */
							/* passed as arguments have a size that is */
							/* a multiple of sizeof (int).  call       */
							/* va_arg  on an int-sized struct in case  */
							/* structs and unions are passed in a      */
							/* different area of memory.  allocate     */
							/* extra space for the padding, if any.    */
							/*******************************************/
						case nel_D_STRUCT:
						case nel_D_UNION: {
								struct tag {
									int x;
								};
								register unsigned_int size;
								register unsigned_int i;
								register struct tag *scan;
								size = nel_align_offset (type->s_u.size, sizeof (int));
								symbol->value = intp_dyn_value_alloc (eng, size, type->s_u.alignment);
								scan = (struct tag *) (symbol->value);
								for (i = 0; (i < size); i += sizeof (int)) {
									*(scan++) = va_arg (*eng->intp->formal_ap, struct tag);
								}
							}
							break;


							/**********************************************/
							/* arrays and functions should have undergone */
							/* promotion to a pointer already.            */
							/**********************************************/
						case nel_D_ARRAY:
						case nel_D_FUNCTION:
						default:
							intp_fatal_error (eng, "(intp_eval_stmt #4): bad type for formal\n%1S", symbol);
						}
						symbol->declared = 1;
						intp_insert_ident (eng, symbol);
					}
					break;

				case nel_C_TYPE:
					//type = intp_resolve_type (eng, type, symbol->name);
					nel_debug ({
					if (type == NULL) {
						intp_fatal_error (eng, "(intp_eval_stmt #5): NULL resolved type");
					}
					});
					symbol = intp_dyn_symbol_alloc (eng, symbol->name, type, symbol->value, symbol->class, symbol->lhs, nel_L_NEL, symbol->level);
					switch (type->simple.type)
					{
					case nel_D_TYPEDEF_NAME:
						intp_insert_ident (eng, symbol);
						break;

					case nel_D_STRUCT_TAG:
					case nel_D_UNION_TAG:
						intp_insert_tag (eng, symbol);
						break;

					case nel_D_ENUM_TAG:
						type = type->tag_name.descriptor;
						if ((type == NULL) || (type->simple.type != nel_D_ENUM)) {
							intp_fatal_error (eng, "(intp_eval_stmt #6): illegal local enumed type:\n%1S", symbol);
						} else {
							register nel_list *scan;
							intp_insert_tag (eng, symbol);

							/*********************************/
							/* insert the constants into the */
							/* dynamic ident hash table.     */
							/*********************************/
							for (scan = type->enumed.consts; (scan != NULL); scan = scan->next) {
								register nel_symbol *constant = scan->symbol;
								if ((constant == NULL) || (constant->name == NULL)) {
									continue;
								}
								constant = intp_dyn_symbol_alloc (eng, constant->name, type, constant->value,
																 constant->class, constant->lhs, nel_L_NEL, constant->level);
								intp_insert_ident (eng, constant);
							}
						}
						break;

					default:
						intp_fatal_error (eng, "(intp_eval_stmt #7): illegal local type\n%1S", symbol);
						break;
					}
					break;

				default:
					intp_fatal_error (eng, "(intp_eval_stmt #8): bad class for symbol\n%1S", symbol);

				} //end of switch(symbol->class)

			} // val == 0 in nel_S_DEC

			list = list->dec.next;
			break;

			/************************/
			/* expression statement */
			/************************/
		case nel_S_EXPR:
			intp_setjmp (eng, val, eng->intp->stmt_err_jmp);
			if (val != 0) {
				nel_debug ({
					intp_trace (eng, "stmt error longjmp caught\n\n");
				});
				intp_diagnostic (eng, "continuing with next statement");
			} else {
				/*****************************************************/
				/* push the dynamic symbol and value table indeces,  */
				/* so that we may later reclaim any garbage produced */
				/* in evaluating this stmt.                          */
				/*****************************************************/
				nel_symbol *dum_dyn_symbols_next;
				char *dum_dyn_values_next;
				eng->intp->clean_flag = 0;
				intp_push_value (eng, eng->intp->dyn_values_next, char *);
				intp_push_value (eng, eng->intp->dyn_symbols_next, nel_symbol *);
				intp_eval_expr (eng, list->expr.expr);
				if (eng->intp->clean_flag) {
					/*****************************************/
					/* dynamic symbols/values were allocated */
					/* by this statement, so do not pop the  */
					/* indeces into dummy vars.              */
					/*****************************************/
					intp_pop_value (eng, dum_dyn_symbols_next, nel_symbol *);
					intp_pop_value (eng, dum_dyn_values_next, char *);
				} else {
					/*******************/
					/* reclaim garbage */
					/*******************/
					intp_pop_value (eng, eng->intp->dyn_symbols_next, nel_symbol *);
					intp_pop_value (eng, eng->intp->dyn_values_next, char *);
				}
			}
			list = list->expr.next;
			break;


			/********************/
			/* return statement */
			/********************/
		case nel_S_RETURN:
			intp_setjmp (eng, val, eng->intp->stmt_err_jmp);
			if (val != 0) {
				nel_debug ({
					intp_trace (eng, "stmt error longjmp caught\n\n");
				});
				intp_warning (eng, "did not set return value");
				/***********************************/
				/* go ahead and perform the return */
				/* with no return value.           */
				/***********************************/
			} else if (list->ret.retval != NULL) {
				register nel_symbol *left;
				register nel_symbol *right;

				/***************************************************/
				/* check to make sure a NULL return value location */
				/* wasn't passed to the top level routine.         */
				/***************************************************/
				if (eng->intp->retval_loc == NULL) {
					intp_stmt_error (eng, "NULL second arg to nel_fp() - cannot return value");
				}

				/****************************************/
				/* evaluate the return value expression */
				/****************************************/
				right = intp_eval_expr (eng, list->ret.retval);
				nel_debug ({
				if ((right == NULL) || (right->type == NULL)) {
					intp_fatal_error (eng, "(intp_eval_stmt #9): bad symbol\n%1S", right);
				}
				});

				if (right->value == NULL) {
					if (right->name == NULL) {
						intp_stmt_error (eng, "NULL address for operand of return");
					} else {
						intp_stmt_error (eng, "NULL address for %I", right);
					}
				}

				/*************************************************/
				/* now create symbol for the return value, and   */
				/* call intp_eval_asgn () to assign the value */
				/* of the expression to it.                      */
				/*************************************************/
				if (eng->intp->prog_unit != NULL) {
					/*************************************************/
					/* we're in a named routine, so the return value */
					/* is coerced to the return type of the routine. */
					/*************************************************/
					register nel_type *type = eng->intp->prog_unit->type;
					nel_debug ({
					if ((type == NULL) || (type->simple.type != nel_D_FUNCTION) || (type->function.return_type == NULL)) {
						intp_fatal_error (eng, "(intp_eval_stmt #10): bad function type\n%1T", type);
					}
					});

					type = type->function.return_type;
					left = intp_dyn_symbol_alloc (eng, NULL, type, eng->intp->retval_loc, nel_C_RESULT, 1, nel_L_NEL, eng->intp->level);

				} else {
					/**************************************************/
					/* return type is the same type as the expression */
					/**************************************************/
					//modified by zhangbin, 2006-5-22
					if(eng->intp->ret_type)
						left = intp_dyn_symbol_alloc(eng, NULL, eng->intp->ret_type, 
													 eng->intp->retval_loc, nel_C_RESULT, 1, nel_L_NEL, eng->intp->level);
					else
					//end
						left = intp_dyn_symbol_alloc (eng, NULL, right->type, eng->intp->retval_loc, nel_C_RESULT, 1, nel_L_NEL, eng->intp->level);
				}

				/***************************************/
				/* call intp_eval_asgn () to assign */
				/* the value to the correct location.  */
				/***************************************/
				intp_eval_asgn (eng, nel_O_RETURN, left, right);
				nel_debug ( {
				if (eng->intp->return_pt == NULL) {
					intp_fatal_error (eng, "(intp_eval_stmt #11): NULL return_pt");
				}
				});
			}
			//added by zhangbin, 2006-5-22
			else
			{
				if(eng->intp->ret_type != NULL)
				{
					if(eng->intp->ret_type->simple.type != nel_D_VOID)
						intp_stmt_error(eng, "a value need to be returned");
				}
			}
			//end

			/*********************************************/
			/* if the flag nel_save_stmt_err_jmp is true, */
			/* then reset nel_stmt_err_jmp to NULL.       */
			/*********************************************/
			if (nel_save_stmt_err_jmp) {
				nel_stmt_err_jmp = NULL;
			}

			/**********************/
			/* now do the longjmp */
			/**********************/
			nel_longjmp (eng, *(eng->intp->return_pt), 1);
			break;

		//added by zhangbin, 2006-5-20
		case nel_S_GOTO:
			nel_debug({
				intp_trace(eng, "intp_eval_stmt, nel_S_GOTO");
			});
			list = list->goto_stmt.goto_target;
			break;
		//end

			/**********************/
			/* conditional branch */
			/**********************/
		case nel_S_BRANCH:
			intp_setjmp (eng, val, eng->intp->stmt_err_jmp);
			if (val != 0) {
				nel_debug ({
					intp_trace (eng, "stmt error longjmp caught\n\n");
				});
				intp_diagnostic (eng, "exiting evaluator");
				goto end;
			} else {
				register nel_symbol *cond;
				nel_debug ({
					intp_trace (eng, "intp_eval_stmt, nel_S_BRANCH, before intp_eval_expr\n");
				});
				cond = intp_eval_expr (eng, list->branch.cond);
				nel_debug ({
					intp_trace (eng, "intp_eval_stmt, nel_S_BRANCH, intp_eval_expr over\n");
				});
				if (intp_nonzero (eng, cond)) {
					nel_debug ({
						intp_trace (eng, "intp_eval_stmt, nel_S_BRANCH, goto true branch\n");
					});
					list = list->branch.true_branch;
				} else {
					nel_debug ({
						intp_trace (eng, "intp_eval_stmt, nel_S_BRANCH, goto false branch\n");
					});
					list = list->branch.false_branch;
				}
			}
			break;

#if 0
		case nel_S_WHILE:			

			intp_setjmp (eng, val, eng->intp->stmt_err_jmp);
			if (val != 0) {
				nel_debug ({
					intp_trace (eng, "stmt error longjmp caught\n\n");
				});
				intp_diagnostic (eng, "exiting evaluator");
				goto end;
			} else {
				register nel_symbol *cond = intp_eval_expr (eng, list->branch.cond);
				if (intp_nonzero (eng, cond)) {
					list = list->while_stmt.body;
				} else {
					list = list->while_stmt.next;
				}
			}
			break;

		case nel_S_FOR:
			break;
#endif
			/**********/
			/* target */
			/**********/
		case nel_S_TARGET:
			while (list->target.level > eng->intp->level) {
				/*****************************/
				/* increment the level, and  */
				/* save the current context. */
				/*****************************/
				eng->intp->level++;
				save_intp_context (eng);

				/**********************************************************/
				/* perform an intp_setjmp on the jmp_buf.  check the return */
				/* code to see if we are really dynamically returning to  */
				/* here from an error.                                    */
				/**********************************************************/
				intp_setjmp (eng, val, eng->intp->block_err_jmp);
				if (val != 0) {
					nel_debug ({
						intp_trace (eng, "block error longjmp caught\n\n");
					});
					intp_diagnostic (eng, "exiting evaluator");
					goto end;
				}
			}
			while (list->target.level < eng->intp->level) {
				/*********************************/
				/* decrement the level, and      */
				/* restore the previous context. */
				/*********************************/
				eng->intp->level--;
				restore_intp_context (eng, eng->intp->level);
			}
			list = list->target.next;
			break;

		default:
			intp_fatal_error (eng, "(intp_eval_stmt #12): illegal stmt structure\n%1J", list);
		}
	}

	/***********************************/
	/* we jump to here on block errors */
	/* and certain stmt errors.        */
	/***********************************/
end:
	;

	/*****************************/
	/* restore the outer context */
	/*****************************/
	eng->intp->context = context; /* just in case */
	restore_intp_context (eng, eng->intp->level);

	/************************************/
	/* restore the filename and line #. */
	/************************************/
	eng->intp->filename = filename;
	eng->intp->line = line;

	nel_debug ({
		intp_trace (eng, "] exiting intp_eval_stmt\n\n");
	});

}



/*****************************************************************************/
/* intp_eval_stmt_2() interprets the routine whose symbol table entry is   */
/* <symbol>, passing the remaining arguments to it, and returning the result */
/* in *<retloc>.                                                             */
/*****************************************************************************/
//register -> , wyong, 20230816 
//int intp_eval_stmt_2 (struct nel_eng *eng, register nel_stmt *stmt)
int intp_eval_stmt_2 (struct nel_eng *eng, nel_stmt *stmt)
{
	/********************************************/
	/* register variables are not guaranteed to */
	/* survive a longjmp () on some systems     */
	/********************************************/
	nel_jmp_buf return_pt;
	struct eng_intp *old_intp = NULL;
	int val;
	int retval;

	//nel_debug ({ intp_trace (eng, "entering intp_eval () [\nretloc = 0x%x\nsymbol =\n%1Sformal_ap = 0x%x\n\n", retloc, symbol, formal_ap); });
	if(eng->intp != NULL){
		old_intp = eng->intp;
	}

	/*********************************************/
	/* initialize the engine record that is passed */
	/* thoughout the entire call sequence.       */
	/* nel_intp_eval () will set the filename and */
	/* line numbers from the stmt structures     */
	/*********************************************/
	nel_malloc(eng->intp, 1, struct eng_intp );
	intp_init (eng, "", NULL, NULL, NULL, 0);

	/**********************/
	/* call the evaluator */
	/**********************/
	eng->intp->return_pt = &return_pt;
	intp_setjmp (eng, val, &return_pt);
	if (val == 0) {
		intp_eval_stmt (eng, (nel_stmt *) stmt );
	}

	/*******************************************************/
	/* deallocate any dynamically allocated substructs     */
	/* withing the engine (if they were stack allocated) */
	/* and return.                                         */
	/*******************************************************/
	retval = eng->intp->error_ct;
	intp_dealloc(eng);
	nel_dealloca(eng->intp);
	//eng->intp = NULL;	//added by zhangbin, 2006-7-21
	
	nel_debug ({ intp_trace (eng, "] exiting intp_eval ()\nerror_ct = %d\n\n", eng->intp->error_ct); });
	if(old_intp != NULL){
		//nel_dealloca(eng->intp);
		eng->intp = old_intp;
	}

	return (retval);
}



/*****************************************************************************/
/* intp_dyn_stack_dealloc () removes all of the dynamic symbols allocated    */
/* with new_index <= &symbol < eng->intp->dyn_symbols_next, and deallocates  */
/* the memory allocated for the values, and the symbols themselves.  it is   */
/* possible that some of the deallocated symbols are still in hash tables,   */
/* so for each one, we have to check the inserted field and remove the       */
/* symbol from the tables it is in (if it is in any).                        */
/*****************************************************************************/
void intp_dyn_stack_dealloc (struct nel_eng *eng, char *dyn_values_next, nel_symbol *dyn_symbols_next)
{
	nel_symbol *scan;

	nel_debug ({ intp_trace (eng, "entering intp_dyn_stack_dealloc [\neng = 0x%x\ndyn_values_next = 0x%x\ndyn_symbols_next = 0x%x\n\n", eng, dyn_values_next, dyn_symbols_next); });

	for (scan = eng->intp->dyn_symbols_next - 1; (scan >= dyn_symbols_next); scan--) {
		if (scan->table != NULL) {
			nel_remove_symbol (eng, scan);
		}
	}
	eng->intp->dyn_values_next = dyn_values_next;
	eng->intp->dyn_symbols_next = dyn_symbols_next;

	nel_debug ({ intp_trace (eng, "] exiting intp_dyn_stack_dealloc\n\n"); });
}

/********************************************************************/
/* use the following macros to save and restore the values of the   */
/* various stacks in the nel_eng .  first push the indeces of the   */
/* value and symbol stacks, then push the old stmt_err_jmp and      */
/* allocate a new one, then push the old block_err_jmp and allocate */
/* a new one, then push the old context pointer, then save the      */
/* current stack pointer in eng->intp->context.  when restoring a     */
/* context, first purge the symbol tables of any symbols at the     */
/* current level (except for dyn_label_hash, since labels do not go */
/* out of scope when exiting any block, except for the outermost in */
/* a routine), then call intp_dyn_stack_dealloc to deallocate any     */
/* dynamically allocated symbols.                                   */
/* a new context does not necessarily correpond to an inner scoping */
/* level, so the level is not incremented/decremented with these    */
/* macros.                                                          */
/* we save a chain of pointers to the previous context, in case the */
/* semantic stack was corrupted in the previous context.            */
/********************************************************************/
void save_intp_context(struct nel_eng *_eng)
{
	intp_push_value ((_eng), (_eng)->intp->dyn_values_next, char *);
	intp_push_value ((_eng), (_eng)->intp->dyn_symbols_next, nel_symbol *);
	intp_push_value ((_eng), (_eng)->intp->stmt_err_jmp, nel_jmp_buf *);
	(_eng)->intp->stmt_err_jmp = (nel_jmp_buf *) intp_dyn_value_alloc ((_eng), sizeof (nel_jmp_buf), nel_MAX_ALIGNMENT);
	intp_push_value ((_eng), (_eng)->intp->block_err_jmp, nel_jmp_buf *);
	(_eng)->intp->block_err_jmp = (nel_jmp_buf *) intp_dyn_value_alloc ((_eng), sizeof (nel_jmp_buf), nel_MAX_ALIGNMENT);
	intp_push_value ((_eng), (_eng)->intp->context, nel_stack *);
	intp_top_pointer ((_eng), (_eng)->intp->context, 0);
}

void restore_intp_context(struct nel_eng *_eng, int _level)
{
	register int __level = (_level);
	intp_set_stack ((_eng), (_eng)->intp->context);
	intp_pop_value ((_eng), (_eng)->intp->context, nel_stack *);
	intp_pop_value ((_eng), (_eng)->intp->block_err_jmp, nel_jmp_buf *);
	intp_pop_value ((_eng), (_eng)->intp->stmt_err_jmp, nel_jmp_buf *);
	intp_pop_value ((_eng), (_eng)->intp->dyn_symbols_next, nel_symbol *);
	intp_pop_value ((_eng), (_eng)->intp->dyn_values_next, char *);

	nel_purge_table (_eng, __level + 1, (_eng)->intp->dyn_ident_hash);
	nel_purge_table (_eng, __level + 1, (_eng)->intp->dyn_location_hash);
	nel_purge_table (_eng, __level + 1, (_eng)->intp->dyn_tag_hash);
	intp_dyn_stack_dealloc ((_eng), (_eng)->intp->dyn_values_next, (_eng)->intp->dyn_symbols_next);
}


int intp_init(struct nel_eng *_eng, char *_filename, FILE *_infile, char *_retval_loc, va_list *_formal_ap, unsigned_int _declare_mode)
{

	//register char *__filename = (_filename);
	nel_debug ({
	if ((_eng) == NULL) {
		intp_fatal_error (NULL, "(nel_intp_init #1): eng == NULL");
	}
	});


	//if(!(_eng)->nel_intp_init_flag) {
	(_eng)->intp->type = nel_R_INTP;
	if (_filename != NULL) {
		(_eng)->intp->filename = nel_insert_name ((_eng), _filename);
	} else {
		(_eng)->intp->filename = NULL;
	}
	(_eng)->intp->line = 1;
	(_eng)->intp->error_ct = 0;
	(_eng)->intp->tmp_int = 0;
	(_eng)->intp->tmp_err_jmp = NULL;
	(_eng)->intp->return_pt = NULL;
	(_eng)->intp->stmt_err_jmp = NULL;
	(_eng)->intp->block_err_jmp = NULL;
	nel_stack_alloc ((_eng)->intp->semantic_stack_start,(_eng)->intp->semantic_stack_next,(_eng)->intp->semantic_stack_end, nel_semantic_stack_max);
	(_eng)->intp->level = 0;
	(_eng)->intp->prog_unit = NULL;
	(_eng)->intp->block_no = 0;
	(_eng)->intp->block_ct = 0;
	(_eng)->intp->clean_flag = 0;
	nel_dyn_allocator_alloc ((_eng)->intp->dyn_values_start,	(_eng)->intp->dyn_values_next, (_eng)->intp->dyn_values_end, nel_dyn_values_max, char);

	nel_dyn_allocator_alloc ((_eng)->intp->dyn_symbols_start,(_eng)->intp->dyn_symbols_next, (_eng)->intp->dyn_symbols_end,nel_dyn_symbols_max, struct nel_SYMBOL);

	nel_dyn_hash_table_alloc((_eng)->intp->dyn_ident_hash , nel_dyn_ident_hash_max);
	nel_dyn_hash_table_alloc ((_eng)->intp->dyn_location_hash, nel_dyn_location_hash_max);
	nel_dyn_hash_table_alloc ((_eng)->intp->dyn_tag_hash, nel_dyn_tag_hash_max);
	nel_dyn_hash_table_alloc ((_eng)->intp->dyn_label_hash , nel_dyn_label_hash_max);

	(_eng)->intp->context = NULL;
	(_eng)->intp->retval_loc = (_retval_loc);
	(_eng)->intp->formal_ap = (_formal_ap);
	(_eng)->intp->declare_mode = (_declare_mode);
	nel_alloca ((_eng)->intp->unit_prefix, nel_MAX_SYMBOL_LENGTH, char);
	nel_make_file_prefix ((_eng), (_eng)->intp->unit_prefix,
						  (_eng)->intp->filename, nel_MAX_SYMBOL_LENGTH);


	/* the rec has been initialized */
	//(_eng)->nel_intp_init_flag = 1;
	//}
	
	//added by zhangbin, 2006-5-22
	(_eng)->intp->ret_type = NULL;
	//end
	return 0;

}


void intp_dealloc(struct nel_eng *_eng)
{

	//int i, n;
	if(!(_eng))
		return;

	//if((_eng)->nel_intp_init_flag == 1) {
	nel_dealloca ((_eng)->intp->unit_prefix);
	nel_purge_table (_eng, 0, (_eng)->intp->dyn_ident_hash);
	nel_purge_table (_eng, 0, (_eng)->intp->dyn_location_hash);
	nel_purge_table (_eng, 0, (_eng)->intp->dyn_tag_hash);
	nel_purge_table (_eng, 0, (_eng)->intp->dyn_label_hash);
	nel_dyn_hash_table_dealloc ((_eng)->intp->dyn_label_hash);
	nel_dyn_hash_table_dealloc ((_eng)->intp->dyn_tag_hash);
	nel_dyn_hash_table_dealloc ((_eng)->intp->dyn_location_hash);
	nel_dyn_hash_table_dealloc ((_eng)->intp->dyn_ident_hash);
	nel_dyn_allocator_dealloc ((_eng)->intp->dyn_symbols_start);
	nel_dyn_allocator_dealloc ((_eng)->intp->dyn_values_start);
	nel_stack_dealloc ((_eng)->intp->semantic_stack_start);


	//added by zhangbin, 2006-7-20
	//nel_dealloca(_eng->intp);
	//end
	
	//	(_eng)->nel_intp_init_flag = 0;
	//}
}


/*****************************************************************************/
/* intp_eval () interprets the routine whose symbol table entry is   */
/* <symbol>, passing the remaining arguments to it, and returning the result */
/* in *<retloc>.                                                             */
/*****************************************************************************/
int intp_eval_symbol_2(struct nel_eng *eng, char *retloc, register nel_symbol *symbol, va_list *formal_ap)
{
	/********************************************/
	/* register variables are not guaranteed to */
	/* survive a longjmp () on some systems     */
	/********************************************/
	nel_jmp_buf return_pt;
	struct eng_intp *old_intp = NULL;
	int val;
	int retval;

	nel_debug ({ intp_trace (eng, "entering intp_eval () [\nretloc = 0x%x\nsymbol =\n%1Sformal_ap = 0x%x\n\n", retloc, symbol, formal_ap); });
	if ((symbol == NULL) 
		|| (symbol->type == NULL)
		|| (symbol->type->simple.type != nel_D_FUNCTION)
		|| (symbol->class == nel_C_COMPILED_FUNCTION)
		|| (symbol->class == nel_C_COMPILED_STATIC_FUNCTION)
		|| (symbol->class == nel_C_INTRINSIC_FUNCTION)
		|| (symbol->value == NULL)) {
		intp_fatal_error (NULL, "(intp_eval #1): bad symbol\n%1S", symbol);
	}


	if(eng->intp != NULL){
		old_intp = eng->intp;
	}

	/*********************************************/
	/* initialize the engine record that is passed */
	/* thoughout the entire call sequence.       */
	/* nel_intp_eval () will set the filename and */
	/* line numbers from the stmt structures     */
	/*********************************************/
	nel_malloc(eng->intp, 1, struct eng_intp );
	intp_init (eng, "", NULL, retloc, formal_ap, 0);

	//added by zhangbin, 2006-5-22
	eng->intp->ret_type = symbol->type->function.return_type;
	//end

	/**********************/
	/* call the evaluator */
	/**********************/
	eng->intp->return_pt = &return_pt;
	intp_setjmp (eng, val, &return_pt);
	if (val == 0) {
		intp_eval_stmt (eng, (nel_stmt *) (symbol->value));
	}

	/* wyong, 2005.6.10 */
	va_end (*eng->intp->formal_ap);

	/*******************************************************/
	/* deallocate any dynamically allocated substructs     */
	/* withing the engine (if they were stack allocated) */
	/* and return.                                         */
	/*******************************************************/
	retval = eng->intp->error_ct;
	intp_dealloc (eng);

	nel_debug ({ intp_trace (eng, "] exiting intp_eval ()\nerror_ct = %d\n\n", eng->intp->error_ct); });
	
	/*bugfix, nel_dealloca before nel_debug, wyong, 2005.12.8 */
	nel_dealloca(eng->intp);
	//eng->intp = NULL;	//added by zhangbin, 2006-7-21
	
	if(old_intp != NULL){
		eng->intp = old_intp;
	}

	return (retval);
}

/*****************************************************************************/
/* nel_func_name_call() (call function name) looks up the symbol for <routine>, and     */
/* calls intp_eval () to interpret it.  the remaining arguments are  */
/* passed to the interpreted routine.                                        */
/*****************************************************************************/
int nel_func_name_call (struct nel_eng *eng, char *retloc, char *routine, ...)
{
	va_list ap;
	register nel_symbol *symbol;
	int retval = -1;

	nel_debug ({ intp_trace (eng, "entering nel_func_name_call() [\nretloc = 0x%x\nroutine = %s\n\n", retloc, routine == NULL ? "NULL" : routine); });

	/****************************************************************/
	/* look up the symbol in the static ident hash table, then make */
	/* certain that it is found and is an interpreted routine       */
	/****************************************************************/
	symbol = nel_lookup_symbol (routine, eng->nel_static_ident_hash, NULL);
	if (symbol == NULL) {
		intp_error (eng, 0, NULL, 0, "routine %s not found", routine);
	} else if ((symbol->type == NULL) || (symbol->type->simple.type != nel_D_FUNCTION)) {
		intp_error (eng, 0, NULL, 0,  "%s: not a function", routine);
	} else if ((symbol->class == nel_C_COMPILED_FUNCTION)
			   || (symbol->class == nel_C_COMPILED_STATIC_FUNCTION)) {
		intp_error (eng,  0, NULL, 0, "%s: compiled function", routine);
	} else if (symbol->class == nel_C_INTRINSIC_FUNCTION) {
		intp_error (eng,  0, NULL, 0, "%s: intrinsic function", routine);
	} else if (symbol->value == NULL) {
		intp_error (eng,  0, NULL, 0, "%s: NULL value", routine);
	} else {
		/********************************************/
		/* initialize the variable arg list ptr(s). */
		/********************************************/
		va_start (ap, routine );

		/***********************************************/
		/* call intp_eval to do the evaluation */
		/* retval is the # of errors.                  */
		/***********************************************/
		retval = intp_eval_symbol_2 (eng, retloc, symbol, &ap);
	}

	nel_debug ({ intp_trace (eng, "] exiting nel_func_name_call()\nrretval = %d\n\n", retval); });
	return (retval);
}



/*****************************************************************************/
/* nel_func_call() (call function symbol) calls intp_eval () to interpret  */
/* the routine whose symbol table entry is <routine>.  the remaining args    */
/* are passed to the interpreted routine.  It is identical to nel_func_name_call, except */
/* that no symbol table lookup is performed.                                 */
/*****************************************************************************/
int nel_func_call (struct nel_eng *eng, char *retloc, nel_symbol *routine, ...)
{
	va_list ap;
	int retval = -1;

	nel_debug ({ 
		intp_trace (eng, "entering nel_func_call () [\nretloc = 0x%x\nroutine =\n%S\n", retloc, routine); 
	});

	/*********************************************/
	/* make certain that the symbol is not NULL, */
	/* and that is an interpreted routine        */
	/*********************************************/
	if (routine == NULL) {
		intp_error (eng, "NULL argument to nel_func_call");
	} else if ((routine->type == NULL) || (routine->type->simple.type != nel_D_FUNCTION)) {
		if (routine->name == NULL)
			intp_error (eng, "argument to nel_func_call: not a function");
		else
			intp_error (eng, "%s: not a function", routine);
	} else if ((routine->class == nel_C_COMPILED_FUNCTION)
			   || (routine->class == nel_C_COMPILED_STATIC_FUNCTION)) {
		if (routine->name == NULL)
			intp_error (eng, "argument to nel_func_call: compiled function");
		else
			intp_error (eng, "%s: compiled function", routine);
	} else if (routine->class == nel_C_INTRINSIC_FUNCTION) {
		if (routine->name == NULL)
			intp_error (eng, "argument to nel_func_call: intrinsic function");
		else
			intp_error (eng, "%s: intrinsic function", routine);
	} else if (routine->value == NULL) {
		if (routine->name == NULL)
			intp_error (eng, "argument to nel_func_call: NULL value");
		else
			intp_error (eng, "%s: NULL value", routine);
	} else {
		/********************************************/
		/* initialize the variable arg list ptr(s). */
		/********************************************/
		va_start (ap, routine );

		/***********************************************/
		/* call intp_eval to do the evaluation */
		/* retval is the # of errors.                  */
		/***********************************************/
		retval = intp_eval_symbol_2 (eng, retloc, routine, &ap);
	}

	nel_debug ({ 
		intp_trace (eng, "] exiting nel_func_call ()\nrretval = %d\n\n", retval); 
	});


	return (retval);

}
//zhangbin
