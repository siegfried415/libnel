/*****************************************************************************/
/* This file, "nel_stab.c", contains routines which scan the symbol table     */
/* (commonly referred to as "stab" strings) of the executable code (the file */
/* containing the code currently being executed).                            */
/*****************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include <setjmp.h>
#include <stab.h>
#include <stdarg.h>
#include <setjmp.h>
#include <string.h>
#include <dlfcn.h>
#include <stdint.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <complex.h>

#include "engine.h"
#include "analysis.h" 
#include "errors.h"
#include "type.h"
#include "sym.h"
#include "_stab.h"
#include "io.h"
#include "intp.h"
#include "mem.h"
#include "evt.h"
#include "nlib.h"

static int parse_complex_type(struct nel_eng *eng, const char **pp);
static FILE *stab_debug_file;
nel_stab_type *stab_remove_type (struct nel_eng *eng, register nel_stab_type *stab_type, register nel_stab_type **table, int max);

/*
 
The following figure shows the data structure constructed by the
stab parser (nel_stab_parse ()) for each compilation unit.
 
                                            ------------------
                                            |                |
                                            | stab_type_hash |
                                            |                |
                                            ------------------
                                                    |
                                         -----------o------------o--------------
                                         |   TYPES DEFINED IN    |             |
                                         | THIS COMPILATION UNIT |             |
                                         V                       V             |
                                    -----------------     -----------------    |
      for all langs but SPARC C,    |<nel_stab_type>|     |<nel_stab_type>|    |
      the types list for include    | type_num: j   |     | type_num: j-1 |    |
      files is always null.   -->   | file_num: 1   |     | file_num: 1   |    |
      all types are in the list     |     type      |->t  |     type      |->t |
      for the main comp unit.       |  file_next    |---->|  file_next    |--> |
                                    -----------------     -----------------    |
                                                  ^                            |
                                                  |  ----------------------    |
                                                  |  |     <nel_type>     |    |
                                 ---------------  |  |     nel_D_FILE     |    |
                        -------->| <nel_symbol>|  ---|     stab_types     |    |
                        |        |  name: *.h  |     |   routines: NULL   |    |
                        |        |    type     |---->|static_globals: NULL|    |
                        |        | value: NULL |     |   includes: NULL   |    |
                        |        |  nel_D_FILE |     ----------------------    |
                        |        ---------------                               |
                        |                     ^                                |
                        |   ----------------  |                                |
                        |   |  <nel_list>  |  |                                |
                        |   |   symbol     |---                                |
                        |   |    next      |---> NEXT INCLUDE FILE             |
                        |   ----------------                                   |
                        |      ^                                               |
    FILE HASH TABLE     |      |                                               |
  --------------------  |      |   STATIC GLOBAL VARS / STATIC ROUTINES        |
  |                  |  |      |   ---------------     ---------------         |
  | static file hash |--o      |   |  <nel_list> |     |  <nel_list> |         |
  |                  |  |      |   |   symbol    |->s  |   symbol    |         |
  --------------------  |      |   |    next     |---->|    next     |-->      |
                        |      |   ---------------     ---------------         |
                        |      |          ^                                    |
  COMP UNIT SYMBOL      |      |          |                                    |
  ---------------       |      -----------+---                                 |
  | <nel_symbol>|<-------                 |  |                                 |
  |    name     |->n  ------------------  |  |                                 |
  |    type     |---->|   <nel_type>   |  |  |                                 |
  | value: NULL |     |   nel_D_FILE   |  |  |                                 |
  |  nel_C_FLE  |     |   stab_types   |--+--+-------                          |
  ---------------   --|    routines    |  |  |      |                          |
        ^           | | static_globals |---  |      |                          |
        |           | |    includes    |------      |                          |
        |           | ------------------            |                          |
---------           |                               |                          |
|                   |            -----------------  |  -----------------       |
|                   |            |<nel_stab_type>|<--  |<nel_stab_type>|       |
|                   |            | type_num: k   |     | type_num: k-1 |       |
| ---------------   |            | file_num: 0   |     | file_num: 0   |       |
| |  <nel_list> |<---            |     type      |->t  |     type      |->t    |
| |   symbol    |------          |  file_next    |---->|  file_next    |->     |
| |    next     |---  |          ----------------      -----------------       |
| ---------------  |  |                ^                       ^               |
|                  |  |                | MORE TYPES DEFINED IN |               |
| NEXT FUNCTION <---  |                | THIS COMPILATION UNIT |               |
|                     |                -----------------------------------------
|                     |
| ---------------     |
| | <nel_symbol>|<-----
| |    name     |->n  ------------------
| |    type     |---->|   <nel_type>   |
| |    value    |->v  | nel_D_FUNCTION |          ARGUMENT LIST
| | nel_C_COMP- |     |  return_type   |->t --------------     --------------
| |ILED_FUNCTION|     |     args       |--->| <nel_list> |     | <nel_list> |
| ---------------  ---|    blocks      |    |  symbol    |->s  |  symbol    |->s
-------------------+--|     file       |    |   next     |---->|   next     |-->
                   |  ------------------    --------------     --------------
                   |
  ---------------  |
  | <nel_block> |<--     LOCAL VARS (BLOCK1, LEVEL 1)
  | block_no: 1 |     --------------     --------------
  |   locals    |---->| <nel_list> |     | <nel_list> |
  |   blocks    |---  |  symbol    |->s  |  symbol    |->s
  | next: NULL  |  |  |   next     |---->|   next     |-->
  ---------------  |  --------------     --------------
                   |
                   |  ---------------
                   -->| <nel_block> |      LOCAL VARS (BLOCK 2, LEVEL 2)
                      | block_no: 2 |     --------------     --------------
                      |   locals    |---->| <nel_list> |     | <nel_list> |
                      |   blocks    |---  |  symbol    |->s  |  symbol    |->s
                   ---|    next     |  |  |   next     |---->|   next     |
                   |  ---------------  |  --------------     --------------
                   |                   |
                   |                   --> NESTED BLOCKS
                   |  ---------------
                   -->| <nel_block> |      LOCAL VARS (BLOCK N, LEVEL 2)
                      | block_no: N |     --------------     --------------
                      |   locals    |---->| <nel_list> |     | <nel_list> |
                      |   blocks    |---  |  symbol    |->s  |  symbol    |->s
                   ---|    next     |  |  |    next    |---->|   next     |
                   |  ---------------  |  --------------     --------------
                   |                   |
                   V                   --> NESTED BLOCKS
 
 
 
  n   => name table entry
  s   => symbol table entry (not shown explicitly)
  t   => type descriptor
  v   => value
 <V^> => pointers
  o   => lines join
  +   => lines cross, but do not join
 
 
Types are inserted in the stab type hash table as they are parsed.
The table is cleared between compilations.  For the SPARC C compiler,
(and not the gnu C compiler for sparcs nor sparc fortran)
the nel_stab_types are reinserted in the table every subsequent time that a
header file is included in a subsequent compilation unit.  The type_num
field remains the same across compilations, but the file_num field is
changed, depending on the order in which the files are included.
For other architectures, the type is redefined in subsequent compilation
units, and the new definition inserted in the stab type hash table each
time.
 
We use eng->simple.stmt_err_jmp to jump the the end of a stab string
if an error is encountered while parsing it, and eng->simple.block_err_jmp
for a fatal error due to a bad symbol table, not a programming bug.
 
*/


/*********************************************************/
/* declarations for the nel_stab_type structure allocator */
/*********************************************************/
unsigned_int nel_stab_types_max = 0x200;
nel_lock_type nel_stab_types_lock = nel_unlocked_state;
static nel_stab_type *nel_stab_types_next = NULL;
static nel_stab_type *nel_stab_types_end = NULL;
static nel_stab_type *nel_free_stab_types = NULL;

/***************************************************************/
/* make a linked list of nel_stab_type_chunk structures so that */
/* we may find the start of all chunks of memory allocated     */
/* to hold nel_stab_type structures at garbage collection time. */
/***************************************************************/
static nel_stab_type_chunk *nel_stab_type_chunks = NULL;



#define tab(_indent); \
	{								\
	   register unsigned_int __indent = (_indent);			\
	   int __i;							\
	   for (__i = 0; (__i < __indent); __i++) {			\
	      fprintf (file, "   ");					\
	   }								\
	}


void stab_trace (struct nel_eng *eng, char *message, ...)
{
        if (eng != NULL && eng->stab_verbose_level >= NEL_TRACE_LEVEL ) {
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

int stab_overwritable (struct nel_eng *eng, char *message, int arg1)
{

	if ( eng != NULL 
		&& eng->stab_verbose_level >= NEL_OVER_LEVEL 
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



void stab_diagnostic(struct nel_eng *eng, char *message, ...)
{
	char buffer[4096];
	va_list args;

	if ( eng && (eng->stab_verbose_level >= NEL_DIAG_LEVEL )) {
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
/* stab_warning () prints out an error message, together with the current    */
/* input filename and line number, and then returns without incrementing     */
/* eng->stat->error_ct.                                                      */
/*****************************************************************************/
int stab_warning (struct nel_eng *eng, char *message, ...)
{
	va_list args;

	if (eng && (eng->stab_verbose_level >= NEL_WARN_LEVEL ) ) {
		va_start (args, message);

		/*******************************************/
		/* error message i/o is a critical section */
		/*******************************************/
		nel_lock (&nel_error_lock);

		if (eng->stab->type != nel_R_NULL) {
			fprintf (stderr, "\"%s\", line %d: warning: ", eng->stab->filename, eng->stab->line);
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
/* stab_error () prints out an error message, together with the current input*/
/* filename and line number, increments eng->stab->error_ct, and returns.    */
/*****************************************************************************/
int stab_error (struct nel_eng *eng, char *message, ...)
{
	va_list args;

	if ( eng && (eng->stab_verbose_level >= NEL_WARN_LEVEL )) {
		va_start (args, message);

		/*******************************************/
		/* error message i/o is a critical section */
		/*******************************************/
		nel_lock (&nel_error_lock);

		if ((eng != NULL) 
			&& (eng->stab->type!= nel_R_NULL) 
			&& (eng->stab->filename != NULL)){
			fprintf (stderr, "\"%s\", line %d: ", eng->stab->filename, eng->stab->line);
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
/* stab_stmt_error() prints out an error message, increments               */
/* eng->stab->error_ct, and nel_longjmps() back to the stab,	     */
/* which continues on to the next statement. 			             */
/*****************************************************************************/
int stab_stmt_error (struct nel_eng *eng, char *message, ...)
{

	va_list args;

	/* output error message first */
	if(eng != NULL && eng->stab_verbose_level >= NEL_WARN_LEVEL ){
		va_start (args, message); 

		if ((eng->stab->type!= nel_R_NULL) && (eng->stab->filename!= NULL)){
			fprintf (stderr, "\"%s\", line %d: ", eng->stab->filename, eng->stab->line);
		}
                nel_do_print (stderr, message, args);
		fprintf (stderr, "\n");
		va_end (args);
	}

	eng->stab->error_ct++;

	/*******************************************************/
	/* now longjmp back to the parser, which will continue */
	/* with the next statement (or the next symbol table   */
	/* string), if the stmt_err_jmp is set.                */
	/*******************************************************/
	if ((eng != NULL) 
		&& (eng->stab->type != nel_R_NULL) 
		&& (eng->stab->stmt_err_jmp != NULL)) {
		nel_longjmp (eng, *(eng->stab->stmt_err_jmp), 1);
	}
	else {
		stab_fatal_error (eng, "(nel_stmt_error #1): stmt error jump not set",NULL);
	}

	
	return (0);

}

/*****************************************************************************/
/* stab_block_error () prints out an error message, increments               */
/* eng->stab->error_ct, and nel_longjmps () back to the stab, which continues*/
/* after the end of the current block.  when scanning the stab strings, the  */
/* block error jump is set to abandon scanning and return.                   */
/*****************************************************************************/
int stab_block_error (struct nel_eng *eng, char *message, ...)
{
	va_list args;

	/* output error message first */
	if(eng != NULL && eng->stab_verbose_level >= NEL_WARN_LEVEL ){
		va_start (args, message); 
		if ((eng->stab->type!= nel_R_NULL) && (eng->stab->filename!= NULL)){
			fprintf (stderr, "\"%s\", line %d: ", eng->stab->filename, eng->stab->line);
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
		&& ( eng->stab->type != nel_R_NULL) 
		&& ( eng->stab->block_err_jmp != NULL)) {
                nel_longjmp (eng, *(eng->stab->block_err_jmp), 1);
        }
        else {
		stab_fatal_error(eng, "(nel_block_error #1): block error jmp not set", NULL);

        }


	eng->stab->error_ct++;
	return (0);
}

/*****************************************************************************/
/* stab_fatal_error() prints out an error message, increments              */
/* eng->stab->error_ct, and exits the program.  it is called	*/ 
/* when an internal stab error is encountered.  if the global */ 
/* flag "nel_no_error_exit" is set, then stab_fatal_error()  */
/* nel_longjmps back to the top-level routine,  which returns  		*/
/* to the caller.                                              */
/*****************************************************************************/
int stab_fatal_error (struct nel_eng *eng, char *message, ...)
{
	va_list args;
	if ( eng != NULL &&  eng->stab_verbose_level >= NEL_FATAL_LEVEL ) {
		va_start (args, message);
		if ( (eng->stab->filename != NULL)){
			fprintf (stderr, "\"%s\", line %d: ", eng->stab->filename, eng->stab->line);
		}

		nel_do_print (stderr, message, args);
		fprintf (stderr, "\n");

		va_end (args);
	}

	eng->stab->error_ct++;

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
		&& (eng->stab->type != nel_R_NULL) 
		&& (eng->stab->return_pt != NULL)) {
		/**************************************/
		/* exit the critical before returning */
		/* to the top-level routine.          */
		/**************************************/
		nel_unlock (&nel_error_lock);
		nel_longjmp (eng, *(eng->stab->return_pt), 1);
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

//to parse "3;{8/16/24};0;"
//output: push complex type into the semantic stack
static int parse_complex_type(struct nel_eng *eng, const char **pp)
{
	int num;
	const char **hold = pp;
	num = parse_number(pp);
	switch(num)
	{
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			break;
		default:
			return -1;
	}
	if(**pp != ';')
		return -1;
	(*pp)++;
	num = parse_number(pp);
	switch(num)
	{
		case sizeof(float complex):
			stab_push_type (eng, nel_complex_float_type);
			break;
		case sizeof(complex):
			stab_push_type (eng, nel_complex_type);
			break;
		case sizeof(long double complex):
			stab_push_type (eng, nel_long_complex_double_type);
			break;
		default:
			return -1;
	}
	if(**pp != ';')
		return -1;
	(*pp)++;
	parse_number(pp);
	if(**pp != ';')
		return -1;
	(*pp)++;
	return 0;
}

/*****************************************************************************/
/* stab_type_alloc () allocates space for an nel_stab_type structure,      */
/* initializes its fields, and returns a pointer to the structure.           */
/*****************************************************************************/
nel_stab_type *stab_type_alloc (struct nel_eng *eng, nel_type *type, int file_num, int type_num)
{
	register nel_stab_type *retval;

	nel_debug ({ stab_trace (eng, "entering stab_type_alloc [\ntype =\n%1Tfile_num = %d\ntype_num = %d\n\n", type, file_num, type_num); });

	/*******************************************/
	/* static allocators are critical sections */
	/*******************************************/
	nel_lock (&nel_stab_types_lock);

	if (nel_free_stab_types != NULL) {
		/*********************************************************/
		/* first, try to re-use deallocated stab_type structures */
		/*********************************************************/
		retval = nel_free_stab_types;
		nel_free_stab_types = nel_free_stab_types->next;
	} else {
		/*******************************************************************/
		/* check for overflow of space allocated for stab_type structures. */
		/* on overflow, allocate another chunk of memory.                  */
		/*******************************************************************/
		if (nel_stab_types_next >= nel_stab_types_end) {
			register nel_stab_type_chunk *chunk;

			nel_debug ({ stab_trace (eng, "nel_stab_type structure overflow - allocating more space\n\n"); });
			nel_malloc (nel_stab_types_next, nel_stab_types_max, nel_stab_type);
			nel_stab_types_end = nel_stab_types_next + nel_stab_types_max;

			/*************************************************/
			/* make an entry for this chunk of memory in the */
			/* list of nel_stab_type_chunks.                  */
			/*************************************************/
			nel_malloc (chunk, 1, nel_stab_type_chunk);
			chunk->start = nel_stab_types_next;
			chunk->next = nel_stab_type_chunks;
			nel_stab_type_chunks = chunk;
		}
		retval = nel_stab_types_next++;
	}

	/*************************/
	/* exit critical section */
	/*************************/
	nel_unlock (&nel_stab_types_lock);

	/**************************************/
	/* initialize the appropriate members */
	/**************************************/
	retval->type = type;
	retval->file_num = file_num;
	retval->type_num = type_num;
	retval->inserted = 0;
	retval->next = NULL;
	retval->file_next = NULL;

	nel_debug ({ stab_trace (eng, "] exiting stab_type_alloc\nretval =\n%1U\n", retval); });
	return (retval);
}



/*****************************************************************************/
/* nel_stab_type_dealloc () returns the nel_stab_type structure pointed to by  */
/* <stab_type> back to the free list (nel_free_stab_types), so that the space */
/* may be re-used.                                                           */
/*****************************************************************************/
void stab_type_dealloc (register nel_stab_type *stab_type)
{
	nel_debug ({
	if (stab_type == NULL) {
		//stab_fatal_error (NULL, "(nel_stab_type_dealloc #1): stab_type == NULL");
	}
	});

	/*******************************************/
	/* static allocators are critical sections */
	/*******************************************/
	nel_lock (&nel_stab_types_lock);

	stab_type->next = nel_free_stab_types;
	nel_free_stab_types = stab_type;

	/*************************/
	/* exit critical section */
	/*************************/
	nel_unlock (&nel_stab_types_lock);
}



/*****************************************************************************/
/* nel_print_stab_type () prints out an nel_stab_type structure.               */
/*****************************************************************************/
void stab_print_type (FILE *file, register nel_stab_type *type, int indent)
{
	if (type == NULL) {
		tab (indent);
		fprintf (file, "NULL\n");
	} else {
		tab (indent);
		fprintf (file, "file_num: %d\n", type->file_num);
		tab (indent);
		fprintf (file, "type_num: %d\n", type->type_num);
		tab (indent);
		fprintf (file, "inserted: %d\n", type->inserted);
		tab (indent);
		fprintf (file, "type:\n");
		nel_print_type (file, type->type, indent + 1);
	}
}



/*****************************************************************************/
/* nel_print_stab_types () prints out a list of nel_stab_type structures,      */
/* chained using the "file_next" field.                                      */
/*****************************************************************************/
void stab_print_types (FILE *file, register nel_stab_type *types, int indent)
{
	if (types == NULL) {
		tab (indent);
		fprintf (file, "NULL_stab_type\n");
	} else {
		for (; (types != NULL); types = types->file_next) {
			tab (indent);
			fprintf (file, "stab_type:\n");
			stab_print_type (file, types, indent + 1);
		}
	}
}



void stab_insert_ident(struct nel_eng *eng, nel_symbol *symbol)
{
	if((symbol->level <=0) 
		&& (symbol->class != nel_C_STATIC_GLOBAL) 
		&& (symbol->class != nel_C_COMPILED_STATIC_FUNCTION)
		&& (symbol->class != nel_C_NEL_STATIC_FUNCTION)) { 
		nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);
	}
	else {
		nel_insert_symbol (eng, symbol, eng->stab->dyn_ident_hash);
	}

#ifdef STAB_DEBUG
	if(eng->stab_debug_level ) {
		nel_print_symbol( stab_debug_file, symbol, 0);	
	}
#endif
	
}

void stab_remove_ident(struct nel_eng *eng,  nel_symbol *symbol)
{
	nel_remove_symbol(eng, symbol);
}

nel_symbol *stab_lookup_ident(struct nel_eng *eng, char *name)
{
	return (nel_lookup_symbol(name, eng->stab->dyn_ident_hash,
				   eng->nel_static_ident_hash,
				   eng->stab->dyn_location_hash,
				   eng->nel_static_location_hash, NULL));
}

/*****************************************************************************/
/* nel_stab_read_name reads characters from <*source> into <buffer> until a   */
/* '\0', ':' or ';' is encountered.  <buffer> should be of at least          */
/* length <n>.  <source> is incremented to the character after the end of    */
/* the identifier, even if the identifier contains more than <n> - 1 chars.  */
/* no more than <n> - 1 character (plus a '\0') will be stored in <buffer>.  */
/* nel_stab_read_name () returns strlen (buffer).                             */
/*****************************************************************************/
int stab_read_name (struct nel_eng *eng, char *buffer, char **source, register int n)
{
	register char *scan;
	register char ch = '_';	/* anything but '\0', ':', ';' */
	register int i;

	/********************************************/
	/* read in chars until we find a '\0', ':', */
	/* or ';', or until the buffer overflows.   */
	/********************************************/
	scan = buffer;
	for (i = 0; (((ch = *((*source)++)) != '\0') &&
				 (ch != ':') && (ch != ';') && (i < n - 1)); i++, *(scan++) = ch)
		;

	/**********************************/
	/* terminate the result with '\0' */
	/**********************************/
	*scan = '\0';

	/************************************/
	/* back up over the last character, */
	/* which is not part of the string. */
	/************************************/
	(*source)--;

	/**********************/
	/* check for overflow */
	/**********************/
	if ((ch != '\0') && (ch != ':') && (ch != ';')) {
		/***********************************/
		/* on overflow, scan for the end   */
		/* of the string.  emit a warning. */
		/***********************************/
		while ((ch != '\0') && (ch != ':') && (ch != ';')) {
			ch = *((*source)++);
			;
			i++;
		}
		stab_warning (eng, "stab #%d: name truncated to %s", eng->stab->sym_ct, buffer);
	}

	/************************************/
	/* return the length of the string. */
	/************************************/
	return (++i);
}



/*****************************************************************************/
/* turning tracing on while parsing the stab entries of the executable code  */
/* result in an enormous trace, so we toggle it off, except when the current */
/* stab #is between nel_stab_tstart and nel_stab_tend, inclusive.              */
/*****************************************************************************/
#ifdef NEL_DEBUG
//static unsigned_int nel_stab_old_tracing; /* old value of nel_tracing	*/
int nel_stab_tstart = 0;	/* tracing starts with this #string	*/
int nel_stab_tend = 0;		/* tracing ends with this #string	*/
#endif /* NEL_DEBUG */



/************************************************************************/
/* when nel_stab_check_redecs, we check all type names and data objects  */
/* for redeclarations when scanning the symbol table.  this requires a  */
/* substantial amount of time, which can be saved by resetting the flag */
/* to 0.  regardless of the value of the flag, the only first definiton */
/* of a type name or data object is inserted in the symbol tables.      */
/* (though if the first definition did not have a valid value there may */
/* be some discrepancies with this rule.                                */
/************************************************************************/
unsigned_int nel_stab_check_redecs = 0;

/**********************************************************************/
/* use the following macro to allcoate space for the stab variant     */
/* of an struct nel_eng structure and initialize the appropriate fields. */
/**********************************************************************/
void stab_init(struct nel_eng *_eng, char *_filename)
{
	register char *__filename = (_filename);
	nel_debug ({
	if ((_eng) == NULL) {
		stab_fatal_error (NULL, "(stab_init #1): eng == NULL");
	}
	});

	(_eng)->stab->type = nel_R_STAB;
	if (__filename != NULL) {
		(_eng)->stab->filename = nel_insert_name((_eng), __filename);
	} else {
		(_eng)->stab->filename = NULL;
	}
	(_eng)->stab->line = 1;
	(_eng)->stab->error_ct = 0;
	(_eng)->stab->tmp_int = 0;
	(_eng)->stab->tmp_err_jmp = NULL;
	(_eng)->stab->return_pt = NULL;
	(_eng)->stab->stmt_err_jmp = NULL;
	(_eng)->stab->block_err_jmp = NULL;
	nel_stack_alloc ((_eng)->stab->semantic_stack_start,
					 (_eng)->stab->semantic_stack_next,
					 (_eng)->stab->semantic_stack_end, nel_semantic_stack_max);
	(_eng)->stab->level = 0;
	(_eng)->stab->prog_unit = NULL;
	(_eng)->stab->block_no = 0;
	(_eng)->stab->block_ct = 0;
	(_eng)->stab->clean_flag = 0;
	nel_dyn_allocator_alloc ((_eng)->stab->dyn_values_start,
							 (_eng)->stab->dyn_values_next, (_eng)->stab->dyn_values_end,
							 nel_dyn_values_max, char);
	nel_dyn_allocator_alloc ((_eng)->stab->dyn_symbols_start,
							 (_eng)->stab->dyn_symbols_next, (_eng)->stab->dyn_symbols_end,
							 nel_dyn_symbols_max, struct nel_SYMBOL);
	nel_dyn_hash_table_alloc ((_eng)->stab->dyn_ident_hash,  nel_dyn_ident_hash_max);
	nel_dyn_hash_table_alloc ((_eng)->stab->dyn_location_hash ,  nel_dyn_location_hash_max);
	nel_dyn_hash_table_alloc ((_eng)->stab->dyn_tag_hash ,  nel_dyn_tag_hash_max);
	nel_dyn_hash_table_alloc ((_eng)->stab->dyn_label_hash , nel_dyn_label_hash_max);
	(_eng)->stab->context = NULL;
	(_eng)->stab->str_tab = NULL;
	(_eng)->stab->str_size = 0;
	(_eng)->stab->str_scan = NULL;
	(_eng)->stab->sym_tab = NULL;
	(_eng)->stab->sym_size = 0;
	(_eng)->stab->sym_scan = NULL;
	(_eng)->stab->last_sym = NULL;
	(_eng)->stab->ld_str_tab = NULL;
	(_eng)->stab->ld_str_size = 0;
	(_eng)->stab->ld_sym_tab = NULL;
	(_eng)->stab->ld_sym_size = 0;
	(_eng)->stab->ld_sym_scan = NULL;
	(_eng)->stab->start_code = 0;
	(_eng)->stab->sym_ct = 0;
	(_eng)->stab->last_char = 0;
	(_eng)->stab->current_symbol = NULL;
	nel_alloca ((_eng)->stab->name_buffer, nel_MAX_SYMBOL_LENGTH, char);
	*((_eng)->stab->name_buffer) = '\0';
	(_eng)->stab->N_TEXT_seen = 1;
	(_eng)->stab->comp_unit = NULL;
	nel_alloca ((_eng)->stab->comp_unit_prefix, nel_MAX_SYMBOL_LENGTH, char);
	*((_eng)->stab->comp_unit_prefix) = '\0';
	(_eng)->stab->source_lang = nel_L_NULL;
	(_eng)->stab->current_file = NULL;
	(_eng)->stab->incl_stack = NULL;
	stab_type_hash_alloc (_eng->stab->type_hash, nel_stab_type_hash_max);
	(_eng)->stab->incl_num = 0;
	(_eng)->stab->incl_ct = 0;
	(_eng)->stab->C_types_inserted = 0;
	//(_eng)->stab->fortran_types_inserted = 0;
	(_eng)->stab->common_block = NULL;
	(_eng)->stab->first_arg = NULL;
	(_eng)->stab->last_arg = NULL;
	(_eng)->stab->first_local = NULL;
	(_eng)->stab->last_local = NULL;
	(_eng)->stab->first_block = NULL;
	(_eng)->stab->last_block = NULL;
	(_eng)->stab->first_include = NULL;
	(_eng)->stab->last_include = NULL;
	(_eng)->stab->first_routine = NULL;
	(_eng)->stab->last_routine = NULL;
	(_eng)->stab->first_static_global = NULL;
	(_eng)->stab->last_static_global = NULL;
	(_eng)->stab->block_start_address = NULL;
	(_eng)->stab->block_end_address = NULL;
	(_eng)->stab->last_line = NULL;
}


void stab_dealloc(struct nel_eng *_eng)
{
	stab_type_hash_dealloc ((_eng)->stab->type_hash);
	nel_dealloca ((_eng)->stab->comp_unit_prefix);
	nel_dealloca ((_eng)->stab->name_buffer);
	nel_purge_table (_eng, 0, (_eng)->stab->dyn_ident_hash);
	nel_purge_table (_eng, 0, (_eng)->stab->dyn_location_hash);
	nel_purge_table (_eng, 0, (_eng)->stab->dyn_tag_hash);
	nel_purge_table (_eng, 0, (_eng)->stab->dyn_label_hash);
	nel_dyn_hash_table_dealloc ((_eng)->stab->dyn_label_hash);
	nel_dyn_hash_table_dealloc ((_eng)->stab->dyn_tag_hash);
	nel_dyn_hash_table_dealloc ((_eng)->stab->dyn_location_hash);
	nel_dyn_hash_table_dealloc ((_eng)->stab->dyn_ident_hash);
	nel_dyn_allocator_dealloc ((_eng)->stab->dyn_symbols_start);
	nel_dyn_allocator_dealloc ((_eng)->stab->dyn_values_start);
	nel_stack_dealloc ((_eng)->stab->semantic_stack_start);

	nel_dealloca(_eng->stab); 
}




/*****************************************************************************/
/* nel_stab_global_acts () performs the sematics actions when the stab for a  */
/* global var is encountered.  <symbol> should be the symbol for the var; it */
/* should have no fields set except for the name.  <type> should be its data */
/* type.                                                                     */
/*****************************************************************************/
void stab_global_acts (struct nel_eng *eng, register nel_symbol *symbol, register nel_type *type)
{
	nel_debug ({
				   stab_trace (eng, "entering nel_stab_global_acts [\neng = 0x%x\nsymbol =\n%1Stype =\n%1T\n", eng, symbol, type);
			   });

	if ((symbol != NULL) && (type != NULL)) {
		/****************************************************/
		/* the address of a global variable is defined in   */
		/* the ld symbol table entry, not in the dbx stab   */
		/* entry we are currently parsing.  set the type    */
		/* field of the symbol table entry.                 */
		/****************************************************/
		register nel_symbol *old_sym;
		symbol->class = nel_C_GLOBAL;
		symbol->type = type;
		symbol->lhs = nel_lhs_type (type);
		symbol->level = 0;  /* just in case */

		/******************************************************/
		/* find the ld symbol table entry for the global var. */
		/* the ld symbol names are prepended with an '_'.     */
		/******************************************************/
		old_sym = stab_lookup_location (eng, symbol->name);
		if (old_sym == NULL) {
			stab_error (eng, "stab #%d: no ld entry for global var %I", eng->stab->sym_ct, symbol);
		} else {
			/***************************************************/
			/* set the location from the ld symbol table entry */
			/***************************************************/
			symbol->value = old_sym->value;
			old_sym = nel_lookup_symbol (symbol->name, eng->stab->dyn_ident_hash, eng->nel_static_ident_hash, NULL ) ; /* add NULL at tail */

			nel_debug ({
						   stab_trace (eng, "old_sym =\n%1S\n", old_sym);
					   });
			if ((old_sym != NULL) && (old_sym->level == 0)) {
				if (nel_stab_check_redecs) {
					if (nel_type_diff (old_sym->type, type, 1) == 1) {
						stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, symbol);
					} else if (old_sym->value == NULL) {
						old_sym->value = symbol->value;
					} else if (old_sym->value != symbol->value) {
						stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, symbol);
					}
				} else if ((old_sym->value == NULL) && (symbol->value != NULL)) {
					/************************************************/
					/* remove the old symbol from the static ident  */
					/* hash table, and replace it with the new one, */
					/* in case the old symbol did not refer to the  */
					/* same object.                                 */
					/************************************************/
					nel_remove_symbol (eng, old_sym);
					stab_insert_ident (eng, symbol);
				}
			} else {
				/****************************/
				/* insert the symbol in the */
				/* static ident hash table  */
				/****************************/
				stab_insert_ident (eng, symbol);
			}
		}
	}

	nel_debug ({
				   stab_trace (eng, "] exiting nel_stab_global_acts\n\n");
			   });
}


/*****************************************************************************/
/* nel_stab_static_global_acts () performs the sematics actions when the stab */
/* for a static global var is encountered.  <symbol> should be the symbol    */
/* for the var; it should have no fields set except for the name.  <type>    */
/* should be its data type.                                                  */
/*****************************************************************************/
void stab_static_global_acts (struct nel_eng *eng, register nel_symbol *symbol, register nel_type *type)
{
	nel_debug ({
				   stab_trace (eng, "entering nel_stab_static_global_acts [\neng = 0x%x\nsymbol =\n%1Stype =\n%1T\n", eng, symbol, type);
			   });

	if ((symbol != NULL) && (type != NULL)) {
		/**************************************************/
		/* set the appropriate symbol fields.             */
		/* the address of a global variable is defined in */
		/* the ld symbol table entry, not in the dbx stab */
		/* entry we are currently parsing.  we will set   */
		/* it when we scan the ld stab strings later.     */
		/**************************************************/
		register nel_symbol *old_sym;
		register nel_list *list;
		symbol->type = type;
		symbol->value = (char *)(long ) stab_get_value (eng->stab->last_sym);
		symbol->class = nel_C_STATIC_GLOBAL;
		symbol->lhs = nel_lhs_type (type);
		symbol->level = 0;  /* just in case */

		if (eng->stab->comp_unit == NULL) {
			stab_error (eng, "stab #%d: static global var not in comp unit: %I", eng->stab->sym_ct, symbol);
		} else {
			nel_debug ({
						   if ((eng->stab->comp_unit->type == NULL) || (eng->stab->comp_unit->type->simple.type != nel_D_FILE)) {
						   stab_fatal_error (eng, "stab #%d (nel_stab_static_global_acts #1): bad comp_unit\n%1S", eng->stab->sym_ct, eng->stab->comp_unit)
							   ;
						   }
					   });

			/*******************************************/
			/* check for a redeclaration of the symbol */
			/*******************************************/
			old_sym = stab_lookup_ident (eng, symbol->name);
			nel_debug ({
				stab_trace (eng, "old_sym =\n%1S\n", old_sym);
			});

			if ((old_sym != NULL) && (old_sym->level == 0)) {
				if (nel_stab_check_redecs) {
					if (nel_type_diff (old_sym->type, type, 1) == 1) {
						stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, old_sym);
					} else if (old_sym->value == NULL) {
						old_sym->value = symbol->value;
					} else if (old_sym->value != symbol->value) {
						stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, old_sym);
					}
				} else if ((old_sym->value == NULL) && (symbol->value != NULL)) {
					/************************************************/
					/* remove the old symbol from the static ident  */
					/* hash table, and replace it with the new one, */
					/* in case the old symbol did not refer to the  */
					/* same object.                                 */
					/************************************************/
					nel_remove_symbol (eng, old_sym);
					stab_insert_ident (eng, symbol);
				}
			} else {
				/*********************************************/
				/* insert the symbol in the dynamic ident    */
				/* hash table at level 0 (the macro          */
				/* stab_insert_ident knows to put it in the    */
				/* dynamic table even though the level       */
				/* field is 0 because the class is           */
				/* nel_C_STATIC_GLOBAL), and append it to     */
				/* the list of static globals for this file. */
				/*********************************************/
				stab_insert_ident (eng, symbol);
				list = nel_list_alloc (eng, 0, symbol, NULL);
				if (eng->stab->last_static_global == NULL) {
					eng->stab->comp_unit->type->file.static_globals =
						eng->stab->first_static_global = eng->stab->last_static_global = list;
				} else {
					eng->stab->last_static_global->next = list;
					eng->stab->last_static_global = list;
				}
			}

			/****************************************/
			/* create a global variable by the name */
			/* <file_name>`<var_name> by which we   */
			/* may also reference this variable.    */
			/* its class is nel_C_GLOBAL, so it gets */
			/* inserted in the static ident table.  */
			/****************************************/
			{
				char *buffer;
				nel_alloca (buffer, strlen (eng->stab->comp_unit_prefix) + strlen (symbol->name) + 2, char);
				sprintf (buffer, "%s`%s", eng->stab->comp_unit_prefix, symbol->name);
				symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, buffer), type, symbol->value, nel_C_GLOBAL, symbol->lhs, eng->stab->source_lang, 0);
				nel_dealloca (buffer);

				/*******************************************/
				/* check for a redeclaration of the symbol */
				/*******************************************/
				old_sym = stab_lookup_ident (eng, symbol->name);
				nel_debug ({
							   stab_trace (eng, "old_sym =\n%1S\n", old_sym);
						   });
				if ((old_sym != NULL) && (old_sym->level == 0)) {
					if (nel_stab_check_redecs) {
						if (nel_type_diff (old_sym->type, type, 1) == 1) {
							stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, old_sym);
						} else if (old_sym->value == NULL) {
							old_sym->value = symbol->value;
						} else if (old_sym->value != symbol->value) {
							stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, old_sym);
						}
					} else if ((old_sym->value == NULL) && (symbol->value != NULL)) {
						/************************************************/
						/* remove the old symbol from the static ident  */
						/* hash table, and replace it with the new one, */
						/* in case the old symbol did not refer to the  */
						/* same object.                                 */
						/************************************************/
						nel_remove_symbol (eng, old_sym);
						stab_insert_ident (eng, symbol);
					}
				} else {
					/****************************/
					/* insert the symbol in the */
					/* static ident hash table  */
					/****************************/
					stab_insert_ident (eng, symbol);
				}
			}
		}
	}

	nel_debug ({
				   stab_trace (eng, "] exiting nel_stab_static_global_acts\n\n");
			   });
}


/*****************************************************************************/
/* nel_stab_local_acts () performs the sematics actions when the stab for a   */
/* local var is encountered.  <symbol> should be the symbol for the var; it  */
/* should have no fields set except for the name.  <type> should be its data */
/* type.                                                                     */
/*****************************************************************************/
void stab_local_acts (struct nel_eng *eng, register nel_symbol *symbol, register nel_type *type)
{
	nel_debug ({
				   stab_trace (eng, "entering nel_stab_local_acts [\neng = 0x%x\nsymbol =\n%1Stype =\n%1T\n", eng, symbol,type);
			   });

	if ((symbol != NULL) && (type != NULL)) {
		/************************************************/
		/* set the appropriate symbol fields.           */
		/* value is the offset from the arg ptr.        */
		/* on SPARC/SPARC_GCC, locals appear before the */
		/* block in which they are declared, so their   */
		/* level field is the current level plus 1.     */
		/************************************************/
		register nel_symbol *old_sym;
		register nel_list *list;
		symbol->type = type;
		symbol->value = (char *)(long ) stab_get_value (eng->stab->last_sym);
		symbol->class = nel_C_LOCAL;
		symbol->lhs = nel_lhs_type (type);
		symbol->level = eng->stab->level + 1;

		/*******************************************/
		/* check for a redeclaration of the symbol */
		/*******************************************/
		old_sym = stab_lookup_ident (eng, symbol->name);
		nel_debug ({
					   stab_trace (eng, "old_sym =\n%1S\n", old_sym);
				   });
		if ((old_sym != NULL) && (old_sym->level == symbol->level)) {
			if (nel_stab_check_redecs) {
				if (nel_type_diff (old_sym->type, type, 1) == 1) {
					stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, old_sym);
				} else if (old_sym->value == NULL) {
					/****************************************/
					/* value of local can legitamitely be 0 */
					/* (offset), or just not be set yet     */
					/****************************************/
					old_sym->value = symbol->value;

					/***************************************************/
					/* change class to plain local to reflect the fact */
					/* that the value (offset from fp) has been set.   */
					/* (symbol->reg_no > 0) => register # is valid.    */
					/***************************************************/
					if (old_sym->class == nel_C_REGISTER_LOCAL) {
						old_sym->class = nel_C_LOCAL;
					}
				} else if (old_sym->value != symbol->value) {
					stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, old_sym);
				}
			} else if ((old_sym->value == NULL) && (old_sym->class != nel_C_LOCAL)
					   && (old_sym->class != nel_C_REGISTER_LOCAL)) {
				/********************************************************/
				/* if the old symbol was a non-static local var, we     */
				/* must assume that the 0 value for the offset was      */
				/* valid, and we do not replace the symbol.  otherwise, */
				/* remove the old symbol from the static ident          */
				/* hash table, and replace it with the new one,         */
				/* in case the old symbol did not refer to the          */
				/* same object.                                         */
				/********************************************************/
				nel_remove_symbol (eng, old_sym);
				stab_insert_ident (eng, symbol);
			}
		} else {
			/***********************************************/
			/* insert the symbol in the dynamic ident hash */
			/* table, and append it to the list of locals  */
			/* in this block                               */
			/***********************************************/
			stab_insert_ident (eng, symbol);
			list = nel_list_alloc (eng, 0, symbol, NULL);
			if (eng->stab->last_local == NULL) {
				eng->stab->first_local = eng->stab->last_local = list;
			} else {
				eng->stab->last_local->next = list;
				eng->stab->last_local = list;
			}
		}
	}

	nel_debug ({
				   stab_trace (eng, "] exiting nel_stab_local_acts\n\n");
			   });
}


/*****************************************************************************/
/* nel_stab_register_local_acts () performs the sematics actions when the     */
/* stab for a register local var is encountered.  <symbol> should be the     */
/* symbol for the var; it should have no fields set except for the name.     */
/* <type> should be its data type.                                           */
/*****************************************************************************/
void stab_register_local_acts (struct nel_eng *eng, register nel_symbol *symbol, register nel_type *type)
{
	nel_debug ({
				   stab_trace (eng, "entering nel_stab_register_local_acts [\neng =0x%x\nsymbol =\n%1Stype =\n%1T\n", eng, symbol, type);
			   });

	if ((symbol != NULL) && (type != NULL)) {
		/************************************************/
		/* set the appropriate symbol fields.           */
		/* value is the offset from the arg ptr.        */
		/* on SPARC/SPARC_GCC, locals appear before the */
		/* block in which they are declared, so their   */
		/* level field is the current level plus 1.     */
		/************************************************/
		register nel_symbol *old_sym;
		register nel_list *list;
		symbol->type = type;
		symbol->class = nel_C_REGISTER_LOCAL;
		symbol->lhs = nel_lhs_type (type);

		/***********************************/
		/* set the reg_no field, not value */
		/***********************************/
		symbol->reg_no = (int) stab_get_value (eng->stab->last_sym);
		symbol->level = eng->stab->level + 1;

		/*******************************************/
		/* check for a redeclaration of the symbol */
		/*******************************************/
		old_sym = stab_lookup_ident (eng, symbol->name);
		nel_debug ({
					   stab_trace (eng, "old_sym =\n%1S\n", old_sym);
				   });
		if ((old_sym != NULL) && (old_sym->level == symbol->level)) {
			/************************************************/
			/* sometimes an extra stab appears for a symbol */
			/* where the first stab contains the normal     */
			/* address, and the second stab the register #  */
			/************************************************/
			if (nel_stab_check_redecs) {
				if (nel_type_diff (old_sym->type, type, 1) == 1) {
					stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, old_sym);
				} else if (old_sym->reg_no < 0) {
					old_sym->reg_no = symbol->reg_no;
				} else if (old_sym->reg_no != symbol->reg_no) {
					stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, old_sym);
				}
			} else if ((old_sym->value == NULL) && (old_sym->class != nel_C_LOCAL)
					   && (old_sym->class != nel_C_REGISTER_LOCAL)) {
				/********************************************************/
				/* if the old symbol was a non-static local var, we     */
				/* must assume that the 0 value for the offset was      */
				/* valid, and we do not replace the symbol.  otherwise, */
				/* remove the old symbol from the static ident          */
				/* hash table, and replace it with the new one,         */
				/* in case the old symbol did not refer to the          */
				/* same object.                                         */
				/********************************************************/
				nel_remove_symbol (eng, old_sym);
				stab_insert_ident (eng, symbol);
			}
		} else {
			/***********************************************/
			/* insert the symbol in the dynamic ident hash */
			/* table, and append it to the list of locals  */
			/* in this block                               */
			/***********************************************/
			stab_insert_ident (eng, symbol);
			list = nel_list_alloc (eng, 0, symbol, NULL);
			if (eng->stab->last_local == NULL) {
				eng->stab->first_local = eng->stab->last_local = list;
			} else {
				eng->stab->last_local->next = list;
				eng->stab->last_local = list;
			}
		}
	}

	nel_debug ({
				   stab_trace (eng, "] exiting nel_stab_register_local_acts\n\n");
			   });
}


/*****************************************************************************/
/* nel_stab_static_local_acts () performs the sematics actions when the stab  */
/* for a static local var is encountered.  <symbol> should be the symbol for */
/* the var; it should have no fields set except for the name.  <type> should */
/* be its data type.                                                         */
/*****************************************************************************/
void stab_static_local_acts (struct nel_eng *eng, register nel_symbol *symbol, register nel_type *type)
{
	nel_debug ({
				   stab_trace (eng, "entering nel_stab_static_local_acts [\neng =0x%x\nsymbol =\n%1Stype =\n%1T\n", eng, symbol, type);
			   });

	if ((symbol != NULL) && (type != NULL)) {
		/************************************************/
		/* set the appropriate symbol fields.           */
		/* value is the offset from the arg ptr.        */
		/* on SPARC/SPARC_GCC, locals appear before the */
		/* block in which they are declared, so their   */
		/* level field is the current level plus 1.     */
		/************************************************/
		register nel_symbol *old_sym;
		register nel_list *list;
		register int block_no;
		symbol->type = type;
		symbol->class = nel_C_STATIC_LOCAL;
		symbol->lhs = nel_lhs_type (type);
		symbol->level = eng->stab->level + 1;
		if (eng->stab->common_block == NULL) {
			symbol->value = (char *)(long ) stab_get_value (eng->stab->last_sym);
		} else {
			/***********************************************/
			/* this var is in a common block.  the address */
			/* the is address of the common block + the    */
			/* offset (value field of this stab entry).    */
			/***********************************************/
			register nel_symbol *common = eng->stab->common_block;

			symbol->value = (char *) (common->value + ((int) stab_get_value (eng->stab->last_sym)));

			nel_debug ({
				   if ((common->type == NULL) || (common->type->simple.type != nel_D_COMMON)) {
				   stab_fatal_error (eng, "stab #%d (nel_stab_static_local_acts #1): bad type for common\n%1S", eng->stab->sym_ct, common)
					   ;
				   }
			   });

			/*************************************************/
			/* the size of the common block must be at least */
			/* the offset of the variable plus its size.     */
			/*************************************************/
			if (common->type->simple.size < (((int) stab_get_value (eng->stab->last_sym)) + type->simple.size)) {
				common->type->simple.size = (int) stab_get_value (eng->stab->last_sym) + type->simple.size;
			}
		}

		/*******************************************/
		/* check for a redeclaration of the symbol */
		/*******************************************/
		old_sym = stab_lookup_ident (eng, symbol->name);
		nel_debug ({
					   stab_trace (eng, "old_sym =\n%1S\n", old_sym);
				   });
		if ((old_sym != NULL) && (old_sym->level == symbol->level)) {
			if (nel_stab_check_redecs) {
				if (nel_type_diff (old_sym->type, type, 1) == 1) {
					stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, old_sym);
				} else if (old_sym->value == NULL) {
					old_sym->value = symbol->value;
				} else if (old_sym->value != symbol->value) {
					stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, old_sym);
				}
			} else if ((old_sym->value == NULL) && (symbol->value != NULL)) {
				/************************************************/
				/* remove the old symbol from the static ident  */
				/* hash table, and replace it with the new one, */
				/* in case the old symbol did not refer to the  */
				/* same object.                                 */
				/************************************************/
				nel_remove_symbol (eng, old_sym);
				stab_insert_ident (eng, symbol);
			}
		} else {
			/***********************************************/
			/* insert the symbol in the dynamic ident hash */
			/* table, and append it to the list of locals  */
			/* in this block                               */
			/***********************************************/
			stab_insert_ident (eng, symbol);
			list = nel_list_alloc (eng, 0, symbol, NULL);
			if (eng->stab->last_local == NULL) {
				eng->stab->first_local = eng->stab->last_local = list;
			} else {
				eng->stab->last_local->next = list;
				eng->stab->last_local = list;
			}
		}

		/*****************************************/
		/* create a global variable by the name  */
		/* <prog_unit>`<block>`<name> by which   */
		/* we may also reference this variable.  */
		/* (omit <block> if it is 0 - keep "``") */
		/* on SPARC/SPARC_GCC:                   */
		/* we are before the block in which the  */
		/* varible is declared; the block will   */
		/* have number eng->stab->block_ct (the   */
		/* future value of eng->stab->block_no).  */
		/*****************************************/
		block_no = eng->stab->block_ct;
		if (eng->stab->prog_unit == NULL) {
			stab_error (eng, "stab #%d: static var not local to program unit: %I", eng->stab->sym_ct, symbol);
		} else {
			char *buffer;
			if ((block_no == 0) ||  (eng->stab->common_block != NULL)) {
				nel_alloca (buffer, strlen (eng->stab->prog_unit->name) + strlen (symbol->name) + 3, char);
				sprintf (buffer, "%s``%s", eng->stab->prog_unit->name, symbol->name);
			} else {
				/* block_no won't have more than 20 digits */
				nel_alloca (buffer, strlen (eng->stab->prog_unit->name) + strlen (symbol->name) + 24, char);
				sprintf (buffer, "%s`%d`%s", eng->stab->prog_unit->name, block_no, symbol->name);
			}
			symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, buffer), type, symbol->value,
											  nel_C_GLOBAL, symbol->lhs, eng->stab->source_lang, 0);
			nel_dealloca (buffer);

			/*******************************************/
			/* check for a redeclaration of the symbol */
			/*******************************************/
			old_sym = stab_lookup_ident (eng, symbol->name);
			nel_debug ({
						   stab_trace (eng, "old_sym =\n%1S\n", old_sym);
					   });
			if ((old_sym != NULL) && (old_sym->level == symbol->level)) {
				if (nel_stab_check_redecs) {
					if (nel_type_diff (old_sym->type, type, 1) == 1) {
						stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, old_sym);
					} else if (old_sym->value == NULL) {
						old_sym->value = symbol->value;
					} else if (old_sym->value != symbol->value) {
						stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, old_sym);
					}
				} else if ((old_sym->value == NULL) && (symbol->value != NULL)) {
					/************************************************/
					/* remove the old symbol from the static ident  */
					/* hash table, and replace it with the new one, */
					/* in case the old symbol did not refer to the  */
					/* same object.                                 */
					/************************************************/
					nel_remove_symbol (eng, old_sym);
					stab_insert_ident (eng, symbol);
				}
			} else {
				/****************************/
				/* insert the symbol in the */
				/* static ident hash table  */
				/****************************/
				stab_insert_ident (eng, symbol);
			}
		}
	}

	nel_debug ({
				   stab_trace (eng, "] exiting nel_stab_static_local_acts\n\n");
			   });
}


/*****************************************************************************/
/* nel_stab_parameter_acts () performs the sematics actions when the stab for */
/* a formal argument is encountered.  <symbol> should be the symbol for the  */
/* var; it should have no fields set except for the name.  <type> should be  */
/* its data type.  <value> is the value of the symbol, i.e., it's offset     */
/* from the argument pointer.  reference should be true is the parameter is  */
/* passed by reference, otherwise it is assumed to be passed by value.       */
/*****************************************************************************/
void stab_parameter_acts (struct nel_eng *eng, register nel_symbol *symbol, register nel_type *type, register char *value, unsigned_int reference)
{
	nel_debug ({
				   stab_trace (eng, "entering nel_stab_parameter_acts [\neng = 0x%x\nsymbol =\n%1Stype =\n%1Tvalue = 0x%x\nreference = %d\n\n", eng, symbol, type, value, reference);
			   });

	if ((symbol != NULL) && (type != NULL)) {
		/*********************************************/
		/* set the appropriate symbol fields.  the   */
		/* value is the offset from the arg pointer. */
		/* all arguments are at level 1.             */
		/*********************************************/
		register nel_symbol *old_sym;
		register nel_list *list;
		if (reference) {
			type = nel_type_alloc (eng, nel_D_POINTER, sizeof (char *), nel_alignment_of (char *), 0, 0, type);
		}
		symbol->type = type;
		symbol->value = value;
		symbol->class = nel_C_FORMAL;
		symbol->lhs = nel_lhs_type (type);
		symbol->level = 1;

		if (eng->stab->prog_unit == NULL) {
			stab_error (eng, "stab #%d: formal arg not local to program unit: %I", eng->stab->sym_ct, symbol);
		} else {
			register nel_type *prog_type = eng->stab->prog_unit->type;
			nel_debug ({
						   if ((prog_type == NULL) || (prog_type->simple.type != nel_D_FUNCTION)) {
						   stab_fatal_error (eng, "stab #%d (nel_stab_parameter_acts #1): bad eng->stab->prog_unit\n%1S", eng->stab->sym_ct, eng->stab->prog_unit)
							   ;
						   }
					   });

			/*******************************************/
			/* check for a redeclaration of the symbol */
			/*******************************************/
			old_sym = stab_lookup_ident (eng, symbol->name);
			nel_debug ({
						   stab_trace (eng, "old_sym =\n%1S\n", old_sym);
					   });
			if ((old_sym != NULL) && (old_sym->level == symbol->level)) {
				if (nel_stab_check_redecs) {
					if (nel_type_diff (old_sym->type, type, 1) == 1) {
						stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, symbol);
					} else if (old_sym->value == NULL) {
						old_sym->value = symbol->value;
					} else if (old_sym->value != symbol->value) {
						stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, symbol);
					}
				} else if ((old_sym->value == NULL) && (symbol->value != NULL)) {
					/************************************************/
					/* remove the old symbol from the static ident  */
					/* hash table, and replace it with the new one, */
					/* in case the old symbol did not refer to the  */
					/* same object.                                 */
					/************************************************/
					nel_remove_symbol (eng, old_sym);
					stab_insert_ident (eng, symbol);
				}
			} else {
				/********************************************/
				/* insert the symbol in the dynamic ident   */
				/* hash table, and append it to the list of */
				/* arguments to this routine.               */
				/********************************************/
				stab_insert_ident (eng, symbol);
				list = nel_list_alloc (eng, 0, symbol, NULL);
				if (eng->stab->first_arg == NULL) {
					prog_type->function.args = eng->stab->first_arg = eng->stab->last_arg = list;
				} else {
					eng->stab->last_arg->next = list;
					eng->stab->last_arg = list;
				}
			}
		}
	}

	nel_debug ({
				   stab_trace (eng, "] exiting nel_stab_parameter_acts\n\n");
			   });
}


/*****************************************************************************/
/* nel_stab_end_routine () is called after parsing the body of a procedure.   */
/* it creates a block structure for the routine's type descirptor, resets    */
/* the lists of arguments and locals, and purges the local symbols from the  */
/* hash tables.                                                              */
/*****************************************************************************/
void stab_end_routine (struct nel_eng *eng)
{
	nel_debug ({ stab_trace (eng, "entering nel_stab_end_routine [\neng = 0x%x\n\n", eng); });

	if (eng->stab->prog_unit != NULL) {
		/*************************************************************/
		/* if there was no block comprising the body of the previous */
		/* program unit, create a nel_block structure for the block,  */
		/* and add it to the type descriptor for the routine.        */
		/*************************************************************/
		register nel_type *type = eng->stab->prog_unit->type;
		nel_block *block;
		nel_debug ({
					   if ((type == NULL) || (type->simple.type != nel_D_FUNCTION)) {
					   stab_fatal_error (eng, "stab #%d (nel_stab_end_routine #1): bad prog_unit\n%1S", eng->stab->sym_ct, eng->stab->prog_unit)
						   ;
					   }
				   });
		if (type->function.blocks == NULL) {
			/**************************************/
			/* no block for previous program unit */
			/**************************************/
			block = nel_block_alloc (eng, eng->stab->block_no, NULL, NULL, eng->stab->first_local, eng->stab->first_block, NULL);
			type->function.blocks = block;
			nel_debug ({ stab_trace (eng, "main block for %I =\n%1B\n", eng->stab->prog_unit, block); });
		}

		/*************************************************/
		/* the starting address of the routine should be */
		/* the value of the function - there may be some */
		/* prologue code in between the entry point and  */
		/* what the symbol table says is the start of    */
		/* the first block.                              */
		/*************************************************/
		type->function.blocks->start_address = eng->stab->prog_unit->value;

		/**************************************/
		/* set the end address of the routine */
		/* to the last address encountered.   */
		/**************************************/
		type->function.blocks->end_address = eng->stab->block_end_address;
	}

	/***************************************/
	/* purge the dynamic tables of all     */
	/* local symbols, if not done already. */
	/***************************************/
	nel_purge_table (eng, 1, eng->stab->dyn_ident_hash);
	nel_purge_table (eng, 1, eng->stab->dyn_tag_hash);
	nel_purge_table (eng, 1, eng->stab->dyn_label_hash);

	/***************************************************************/
	/* clear the lists of formal arguments and local variables for */
	/* this outer block.  reset the start and end block addresses. */
	/***************************************************************/
	eng->stab->first_arg = NULL;
	eng->stab->last_arg = NULL;
	eng->stab->first_local = NULL;
	eng->stab->last_local = NULL;
	eng->stab->first_block = NULL;
	eng->stab->last_block = NULL;
	eng->stab->block_start_address = NULL;
	eng->stab->block_end_address = NULL;

	/*********************************************/
	/* reset the scoping level and block number. */
	/* clear eng->stab->prog_unit.                */
	/*********************************************/
	eng->stab->level = 0;
	eng->stab->block_ct = 0;
	eng->stab->block_no = 0;
	eng->stab->prog_unit = NULL;

	nel_debug ({ stab_trace (eng, "] exiting nel_stab_end_routine\n\n"); });
}


/*****************************************************************************/
/* nel_stab_start_routine () is called when entering a procedure or function. */
/* <symbol> should be the symbol for the routine, and already be inserted in */
/* the appropriate table(s).  if the previous routine has not been cleaned   */
/* up after this is done, then this is done first.  the preliminary block    */
/* structure for the new routine is then formed, and eng->stab->prog_unit is  */
/* set to the symbol.                                                        */
/*****************************************************************************/
void stab_start_routine (struct nel_eng *eng, register nel_symbol *symbol)
{
	nel_debug ({
				   stab_trace (eng, "entering nel_stab_start_routine [\neng =0x%x\nsymbol =\n%1S\n", eng, symbol);
			   });

	/************************************************/
	/* clean up after the previous symbol.         */
	/* if there was no block for the main procedure */
	/* body, and no N_EFUN stab to signal the end   */
	/* of the previous symbol, we do not know the  */
	/* the procedure is done with until now.        */
	/************************************************/
	if (eng->stab->prog_unit != NULL) {
		stab_end_routine (eng);
	}

	/***************************/
	/* set eng->stab->prog_unit */
	/***************************/
	eng->stab->prog_unit = symbol;

	if (eng->stab->comp_unit == NULL) {
		stab_error (eng, "stab# %d: routine not in comp unit: %I", eng->stab->sym_ct, symbol);
	} else {
		register nel_list *list;

		nel_debug ({
					   if ((eng->stab->comp_unit->type == NULL) || (eng->stab->comp_unit->type->simple.type != nel_D_FILE)) {
					   stab_fatal_error (eng, "stab #%d (nel_stab_start_routine #1): bad comp_unit\n%1S", eng->stab->sym_ct, eng->stab->comp_unit)
						   ;
					   }
				   });

		/**************************************/
		/* append the symbol to the list of   */
		/* routines in this compilation unit. */
		/**************************************/
		list = nel_list_alloc (eng, 0, symbol, NULL);
		if (eng->stab->last_routine == NULL) {
			eng->stab->comp_unit->type->file.routines = eng->stab->first_routine
					= eng->stab->last_routine = list;
		} else {
			eng->stab->last_routine->next = list;
			eng->stab->last_routine = list;
		}
	}

	/****************************************************/
	/* on ALLIANT_FX and ALLIANT_FX2800, there are no   */
	/* brackets around the outermost block comprising   */
	/* the function body, and the N_EFUN symbol appears */
	/* to signal the end of the block.  increment       */
	/* eng->stab->level to 1, and we will reset it to 0  */
	/* when the N_EFUN stab is encountered.  this outer */
	/* block will be block 0, so set eng->stab->block_ct */
	/* and eng->stab->block_no appoprpriately.           */
	/****************************************************/
	eng->stab->level = 1;
	eng->stab->block_no = 0;
	eng->stab->block_ct = 1;

	nel_debug ( {
					stab_trace (eng, "] exiting nel_stab_start_routine\n\n");
				}
			  );
}

/*****************************************************************************/
/* nel_stab_routine_acts () performs the sematics actions when the stab for a */
/* routine is encountered.  <symbol> should be the symbol for the routine,   */
/* with no fields set other than the name.  <return_type> should be the type */
/* descriptor for the return value of the routine.                           */
/*****************************************************************************/
void stab_routine_acts (struct nel_eng *eng, register nel_symbol *symbol, register nel_type *return_type)
{
	nel_debug ({
				   stab_trace (eng, "entering nel_stab_routine_acts [\neng = 0x%x\nsymbol =\n%1Sreturn_type =\n%1T\n", eng, symbol, return_type);
			   });

	if ((symbol != NULL) && (return_type != NULL)) {
		register nel_symbol *old_sym;
		symbol->type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 0, return_type, NULL, NULL, eng->stab->comp_unit);
		symbol->value = (char *)(long ) stab_get_value (eng->stab->last_sym);
		symbol->class = nel_C_COMPILED_FUNCTION;
		symbol->lhs = 0;
		symbol->level = 0;


		/*******************************************/
		/* check for a redeclaration of the symbol */
		/*******************************************/
		old_sym = stab_lookup_ident (eng, symbol->name);
		nel_debug ({
					   stab_trace (eng, "old_sym =\n%1S\n", old_sym);
				   });
		if ((old_sym != NULL) && (old_sym->level == 0)) {
			if (nel_stab_check_redecs) {
				if (nel_type_diff (old_sym->type, symbol->type, 1) == 1) {
					stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, symbol);
				} else if (old_sym->value == NULL) {
					old_sym->value = symbol->value;
				} else if (old_sym->value != symbol->value) {
					stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, symbol);
				}
			} else if ((symbol->value != NULL)) {
				/************************************************/
				/* remove the old symbol from the static ident  */
				/* hash table, and replace it with the new one, */
				/* in case the old symbol did not refer to the  */
				/* same object.                                 */
				/************************************************/
				nel_remove_symbol (eng, old_sym);
				stab_insert_ident (eng, symbol);
			}
		} else {
			/****************************/
			/* insert the symbol in the */
			/* static ident hash table  */
			/****************************/
			stab_insert_ident (eng, symbol);
		}

		/************************************************/
		/* call nel_stab_start_routine to clean up after */
		/* the previous routine and start forming the   */
		/* block strucutre for the new routine.         */
		/************************************************/
		stab_start_routine (eng, symbol);
	}

	nel_debug ({
		stab_trace (eng, "] exiting nel_stab_routine_acts\n\n");
	});
}


/*****************************************************************************/
/* nel_stab_static_routine_acts () performs the sematics actions when the     */
/* stab for a static function is encountered.  <symbol> should be the symbol */
/* for the routine, with no fields set other than the name.  <return_type>   */
/* should be the type descriptor for the return value of the routine.        */
/*****************************************************************************/
void stab_static_routine_acts (struct nel_eng *eng, register nel_symbol *symbol, register nel_type *return_type)
{
	nel_debug ({
				   stab_trace (eng, "entering nel_stab_static_routine_acts [\neng = 0x%x\nsymbol =\n%1Sreturn_type =\n%1T\n", eng, symbol, return_type);
			   });

	if (symbol != NULL) {
		register nel_symbol *old_sym;
		register nel_list *list;
		symbol->type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 0, return_type, NULL, NULL, eng->stab->comp_unit);
		symbol->value = (char *)(long ) stab_get_value (eng->stab->last_sym);
		symbol->class = nel_C_COMPILED_STATIC_FUNCTION;
		symbol->lhs = 0;
		symbol->level = 0;

		/***********************************************/
		/* make sure we are within the scope of a file */
		/***********************************************/
		if (eng->stab->comp_unit == NULL) {
			stab_error (eng, "stab #%d: static function not in comp unit: %I", eng->stab->sym_ct, symbol);
		} else {
			nel_debug ({
						   if ((eng->stab->comp_unit->type == NULL) || (eng->stab->comp_unit->type->simple.type != nel_D_FILE)) {
						   stab_fatal_error (eng, "stab #%d (nel_stab_static_routine_acts #1): bad comp_unit\n%1S", eng->stab->sym_ct, eng->stab->comp_unit)
							   ;
						   }
					   });

			/*******************************************/
			/* check for a redeclaration of the symbol */
			/*******************************************/
			old_sym = stab_lookup_ident (eng, symbol->name);
			if ((old_sym != NULL) && (old_sym->level == 0)) {
				if (nel_stab_check_redecs) {
					if (nel_type_diff (old_sym->type, symbol->type, 1) == 1) {
						stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, symbol);
					} else if (old_sym->value == NULL) {
						old_sym->value = symbol->value;
					} else if (old_sym->value != symbol->value) {
						stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, symbol);
					}
				} else if ((old_sym->value == NULL) && (symbol->value != NULL)) {
					/************************************************/
					/* remove the old symbol from the static ident  */
					/* hash table, and replace it with the new one, */
					/* in case the old symbol did not refer to the  */
					/* same object.                                 */
					/************************************************/
					nel_remove_symbol (eng, old_sym);
					stab_insert_ident (eng, symbol);
				}
			} else {
				/*********************************************/
				/* insert the symbol in the dynamic ident    */
				/* hash table at level 0 (stab_insert_ident    */
				/* knows to put it in the dynamic table even */
				/* the level field is 0 because the class is */
				/* nel_C_COMPILED_STATIC_FUNCTION).           */
				/*********************************************/
				stab_insert_ident (eng, symbol);
			}

			/**********************************r*/
			/* append the symbol to the list of */
			/* static globals for this file.    */
			/**********************************r*/
			list = nel_list_alloc (eng, 0, symbol, NULL);
			if (eng->stab->last_static_global == NULL) {
				eng->stab->comp_unit->type->file.static_globals =
					eng->stab->first_static_global = eng->stab->last_static_global = list;
			} else {
				eng->stab->last_static_global->next = list;
				eng->stab->last_static_global = list;
			}

			/************************************************/
			/* call nel_stab_start_routine to clean up after */
			/* the previous routine and start forming the   */
			/* block strucutre for the new routine.         */
			/************************************************/
			stab_start_routine (eng, symbol);

			/*******************************************/
			/* create a global function by the name    */
			/* <file_name>`<var_name> by which we may  */
			/* also reference this routine.  its class */
			/* is nel_C_COMPILED_FUNCTION, so it gets   */
			/* inserted in the static ident table.     */
			/*******************************************/
			{
				char *buffer;
				nel_alloca (buffer, strlen (eng->stab->comp_unit_prefix) + strlen (symbol->name) + 2, char);
				sprintf (buffer, "%s`%s", eng->stab->comp_unit_prefix, symbol->name);
				symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, buffer), symbol->type,
												  symbol->value, nel_C_COMPILED_FUNCTION, symbol->lhs, eng->stab->source_lang, 0);
				nel_dealloca (buffer);

				/*******************************************/
				/* check for a redeclaration of the symbol */
				/*******************************************/
				old_sym = stab_lookup_ident (eng, symbol->name);
				nel_debug ({
							   stab_trace (eng, "old_sym =\n%1S\n", old_sym);
						   });
				if ((old_sym != NULL) && (old_sym->level == 0)) {
					if (nel_stab_check_redecs) {
						if (nel_type_diff (old_sym->type, symbol->type, 1) == 1) {
							stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, symbol);
						} else if (old_sym->value == NULL) {
							old_sym->value = symbol->value;
						} else if (old_sym->value != symbol->value) {
							stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, symbol);
						}
					} else if ((old_sym->value == NULL) && (symbol->value != NULL)) {
						/************************************************/
						/* remove the old symbol from the static ident  */
						/* hash table, and replace it with the new one, */
						/* in case the old symbol did not refer to the  */
						/* same object.                                 */
						/************************************************/
						nel_remove_symbol (eng, old_sym);
						stab_insert_ident (eng, symbol);
					}
				} else {
					/****************************/
					/* insert the symbol in the */
					/* static ident hash table  */
					/****************************/
					stab_insert_ident (eng, symbol);
				}
			}
		}
	}

	nel_debug ({
				   stab_trace (eng, "] exiting nel_stab_static_routine_acts\n\n");
			   });
}







/*****************************************************************************/
/* nel_stab_in_routine () returns true if we are currently scanning a program */
/* unit.  on most machines, we simply examine eng->stab->level and see if it  */
/* is nonzero.  on SPARC/SPARC_GCC, however, vars and types local to a block */
/* appear before the block, so we call this routine to scan ahead and see if */
/* we are within a block of code or at level 0.                              */
/*****************************************************************************/
unsigned_int stab_in_routine (struct nel_eng *eng)
{
	register unsigned_int retval;

	nel_debug ({ stab_trace (eng, "entering nel_stab_in_routine [\neng = 0x%x\n\n", eng); });

	if (eng->stab->level > 0) {
		retval = 1;
		goto end;
	} else {
		register int i = eng->stab->sym_ct;
		//register struct nlist *scan = eng->stab->sym_scan;
		register Elf_Sym *scan = eng->stab->sym_scan;
		for (; (i < eng->stab->sym_size); i++, scan++) {

			switch (stab_get_type (scan)) {

			case N_LBRAC:	/* before a block => inside routine	*/
			case N_RBRAC:	/* inside block => inside routine	*/
			case N_RSYM:	/* register var => inside routine	*/
			case N_PSYM:	/* parameter => inside routine		*/
			case N_BCOMM:	/* common => inside routine		*/
			case N_ECOMM:	/* common => inside routine		*/
			case N_ENTRY:	/* ENTRY stmt => inside routine		*/
			case N_ECOML:	/* common => inside routine		*/
				retval = 1;
				goto end;

			case N_GSYM:	/* global var => outside routine	*/
			case N_FUN:	/* new func => outside routine		*/
			case N_FNAME:	/* new func => outside routine		*/
			case N_PC:	/* global var => main program		*/
				retval = 0;
				goto end;
			}
		}

		/**************************/
		/* shouldn't make it here */
		/**************************/
		retval = 0;
	}

end:
	nel_debug ({ stab_trace (eng, "] exiting nel_stab_in_routine\nretval = %d\n\n", retval); });
	return (retval);
}




/* Parse a range type.  */

int parse_stab_range_type (struct nel_eng *eng, const char *typename, const char **pp, const int *typenums)
{
	nel_stab_type *stab_type;
	const char *orig;
	int nums[2];
	int type_num;
	int file_num;
	int known;

	orig = *pp;


	/* First comes a type we are a subrange of.
	   In C it is usually 0, 1 or the type being defined.  */
	if (parse_stab_type_number (eng, pp, nums) == -1)
		return -1;

	file_num = nums[0];
	type_num = nums[1];

	if (**pp == '=') {
		*pp = orig;
		if (parse_stab_type (eng, (const char *) NULL, pp) == -1)
			return -1;
	} else {
		//stab_pop_integer (eng, type_num);
		//stab_pop_integer (eng, file_num);
		stab_lookup_type_2 (eng, file_num, type_num);
		stab_type = stab_lookup_type_2 (eng, file_num, type_num);

		/*******************************************************/
		/* if we don't find the type, allocate a nel_stab_undef */
		/* type as a placeholder, and push it, after inserting */
		/* the correspoinding nel_stab_type.                    */
		/*******************************************************/
		if (stab_type == NULL) {
			stab_type = stab_type_alloc (eng, NULL, file_num, type_num);
			stab_type->type = nel_type_alloc (eng, nel_D_STAB_UNDEF, 0, 0, 0, 0, stab_type);
			stab_insert_type_2 (eng, stab_type);

		} else {
			nel_debug ({
						   if (stab_type->type == NULL) {
						   stab_fatal_error (eng, "stab #%d: (type_def->type_num #1): stab_type->type == NULL\n%1U", eng->stab->sym_ct, stab_type)
							   ;
						   }
					   });
		}
		stab_push_type (eng, stab_type->type);
	}

	if (**pp == ';')
		++*pp;


	/* The remaining two operands are usually lower and upper bounds of
	   the range.  But in some special cases they mean something else.  */


	known = parse_number (pp);
#if 0 
	//stab_pop_integer (eng, known);
	if (known) {
		register int bound;
		stab_pop_integer (eng, bound);
	} else {
		register nel_symbol *symbol;
		stab_pop_symbol (eng, symbol);
	}
#endif


	if (**pp != ';') {
		nel_debug({stab_trace (eng, "bad stab: %s\n", orig);});
		return -1;
	}
	++*pp;

	known = parse_number (pp);
#if 0 
	//stab_pop_integer (eng, known);
	if (known) {
		register int bound;
		stab_pop_integer (eng, bound);
	} else {
		register nel_symbol *symbol;
		stab_pop_symbol (eng, symbol);
	}
#endif

	if (**pp != ';') {
		nel_debug({stab_trace (eng, "bad stab: %s\n", orig);});
		return -1;
	}
	++*pp;

	/*************************************/
	/* leave the type_desc pushed  -     */
	/* no subrange checking is done in C */
	/*************************************/

	return 0;
}



/* Handle an enum type.  */

int parse_stab_enum_type (struct nel_eng *eng, const char **pp)
{
	int ord_value;
	nel_symbol *symbol;
	nel_list *list;
	const char *orig;
	int first;

	orig = *pp;

	/* FIXME: gdb checks os9k_stabs here.  */

	/* The aix4 compiler emits an extra field before the enum members;
	   my guess is it's a type of some sort.  Just ignore it.  */
	if (**pp == '-') {
		while (**pp != ':')
			++*pp;
		++*pp;
	}

	{
		stab_push_type (eng, NULL);
	}

	/* Read the value-names and their values.
	   The input syntax is NAME:VALUE,NAME:VALUE, and so on.
	   A semicolon or comma instead of a NAME means the end.  */

	first = 1;
	while (**pp != '\0' && **pp != ';' && **pp != ',') {
		const char *p;
		char *name;

		//to deal with strings end with '\\'.
		if (**pp == '\\') {
			*pp += 2;
		}

		p = *pp;
		while (*p != ':')
			++p;

		//name = savestring (*pp, p - *pp);
		//eng->stab->str_scan--;
		stab_read_name (eng, eng->stab->name_buffer, (char **)pp, nel_MAX_SYMBOL_LENGTH);
		name = nel_insert_name(eng, eng->stab->name_buffer);
		//stab_push_name (eng, name);
		//stab_pop_name (eng, name);

		symbol = nel_static_symbol_alloc (eng, name, NULL, NULL, nel_C_NULL, 0, eng->stab->source_lang, eng->stab->level);
		stab_push_symbol (eng, symbol);

		*pp = p + 1;
		ord_value = parse_number (pp);
		//stab_pop_integer (eng, ord_value);
		stab_pop_symbol (eng, symbol);

		/***************************************************/
		/* give the enum const type int for now - later it */
		/* will be changed to the enumed type descriptor.  */
		/***************************************************/
		symbol->type = nel_int_type;
		symbol->class = nel_C_ENUM_CONST;
		list = nel_list_alloc (eng, ord_value, symbol, NULL);
		stab_push_list (eng, list);

		if (**pp != ',') {
			nel_debug({stab_trace (eng, "bad stab: %s\n", orig);});
			return -1;
		}

		if (first) {
			first = 0;
			/*******************************/
			/* duplicate the top of stack. */
			/*******************************/
			stab_top_list (eng, list, 0);
			stab_push_list (eng, list);

		} else {

			/**************************************************/
			/* pop the new element and the end of the list,   */
			/* append the new element, then push the new end. */
			/**************************************************/
			{
				register nel_list *new;
				register nel_list *end;
				stab_pop_list (eng, new);
				stab_pop_list (eng, end);
				end->next = new;
				stab_push_list (eng, new);
			}
		}
		++*pp;

	}

	if (**pp == ';')
		++*pp;


	/**********************************************/
	/* enumeration - pop the list of enum consts, */
	/* form the type descriptor and push it.      */
	/**********************************************/
	{
		register nel_list *end;
		register nel_list *consts;
		register nel_type *base_type;
		register nel_type *enum_type_d;
		register int nconsts;
		register nel_list *scan;
		stab_pop_list (eng, end);
		stab_pop_list (eng, consts);
		stab_pop_type (eng, base_type);
		if (base_type != NULL)
		{
			switch (base_type->simple.type) {
			case nel_D_INT:
			case nel_D_UNSIGNED_INT:
				break;
			default:
				stab_stmt_error (eng, "stab #%d: non-integer enumed type: %I", eng->stab->sym_ct, eng->stab->current_symbol);
			}
		}
		enum_type_d = nel_type_alloc (eng, nel_D_ENUM, sizeof (int), nel_alignment_of (int), 0, 0, NULL, 0, consts);
		stab_push_type (eng, enum_type_d);

		/**********************************/
		/* count the number of constants, */
		/* and set their type and class.  */
		/**********************************/
		for (nconsts = 0, scan = consts; (scan != NULL); nconsts++, scan = scan->next)
		{
			register nel_symbol *symbol = scan->symbol;
			nel_debug ({
						   if ((symbol == NULL) || (symbol->name == NULL)) {
						   stab_fatal_error (eng, "stab #%d (type_def->'e' enum_list ';' #1): bad symbol\n%1S", eng->stab->sym_ct, symbol)
							   ;
						   }
					   });
			symbol->type = enum_type_d;
			symbol->class = nel_C_ENUM_CONST;

			/**********************************************/
			/* if we are at level 0, insert the constants */
			/* into the ident table, and set their values */
			/**********************************************/
			if (eng->stab->level == 0) {
				register nel_symbol *old_sym;
				old_sym = stab_lookup_ident (eng, symbol->name);
				nel_debug ({
							   stab_trace (eng, "old_sym =\n%1S\n", old_sym);
						   });
				if (old_sym == NULL) {
					symbol->value = nel_static_value_alloc (eng, sizeof (int), nel_alignment_of (int));
					*((int *) (symbol->value)) = scan->value;
					stab_insert_ident (eng, symbol);
				} else {
					/**********************************************/
					/* we may scan a declaration at level 0       */
					/* multiple times - make sure the old symbol  */
					/* is a constant with the same value.         */
					/**********************************************/
					register nel_type *old_type = old_sym->type;
					nel_debug ({
								   if ((old_sym->type == NULL) || (old_sym->value == NULL)) {
								   stab_fatal_error (eng, "stab #%d (enumerator->enum_const enum_val #1): bad symbol\n%1S", eng->stab->sym_ct, old_sym)
									   ;
								   }
							   });
					if ((old_type->simple.type != nel_D_ENUM) ||
							(old_sym->class != nel_C_ENUM_CONST) ||
							(*((int *) (old_sym->value)) != scan->value)) {
						stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, symbol);
					}
				}
			}
		}
		enum_type_d->enumed.nconsts = nconsts;
	}

	return 0;
}



/* Read a definition of an array type.  */

int parse_stab_array_type (struct nel_eng *eng, const char **pp)
{
	nel_type *type;
	nel_type *index_type;
	nel_type *base_type;
	unsigned_int known_lb, known_ub;
	union array_bound lb, ub;

	const char *orig;

	/* Format of an array type:
	   "ar<index type>;lower;upper;<array_contents_type>".
	   OS9000: "arlower,upper;<array_contents_type>".

	   Fortran adjustable arrays use Adigits or Tdigits for lower or upper;
	   for these, produce a type like float[][].  */


	orig = *pp;

	parse_stab_type (eng, (const char *) NULL, pp);
	if (**pp != ';') {
		nel_debug({stab_trace (eng, "bad stab: %s\n", orig);});
		return -1;
	}
	++*pp;

	if (! isdigit(**pp) && **pp != '-') {
		++*pp;
	}

	known_lb= parse_number (pp);
	if (known_lb >= 0) {
		//register int value;
		//stab_pop_integer (eng, value);
		//lb.value = value;
		lb.value = known_lb;
		known_lb = 1;
	} else {

		/********************************/
		/* unevaluated dimension bounds */
		/* are stored as expressions.   */
		/********************************/
		//register nel_expr *expr;
		//stab_pop_expr (eng, expr);
		//lb.expr = expr;
		lb.expr = NULL;
		known_lb = 0;
	}

	if (**pp != ';') {
		nel_debug({stab_trace (eng, "bad stab: %s\n", orig);});
		return -1;
	}
	++*pp;

	if (! isdigit(**pp) && **pp != '-') {
		++*pp;
	}

	known_ub = parse_number (pp);
	if (known_ub >= 0) {
		//register int value;
		//stab_pop_integer (eng, value);
		//ub.value = value;
		ub.value = known_ub;
		known_ub = 1;
	} else {
		/********************************/
		/* unevaluated dimension bounds */
		/* are stored as expressions.   */
		/********************************/
		//register nel_expr *expr;
		//stab_pop_expr (eng, expr);
		//ub.expr = expr;
		ub.expr = NULL;
		known_ub = 0;
	}

	if (**pp != ';') {
		nel_debug({stab_trace (eng, "bad stab: %s\n", orig);});
		return -1;
	}
	++*pp;

	parse_stab_type_ex (eng, (const char *) NULL, pp);


	stab_pop_type (eng, base_type);
	type = nel_type_alloc (eng, nel_D_ARRAY, 0, base_type->simple.alignment, 0, 0, base_type, nel_int_type, 0, NULL, 0, NULL);
	type->array.known_lb = known_lb;
	if(known_lb)
		type->array.lb.value = lb.value;
	else 
		type->array.lb.expr  = lb.expr;

	type->array.known_ub = known_ub;
	if(known_ub)
		type->array.ub.value = ub.value;
	else 
		type->array.ub.expr  = ub.expr;


	if (type->array.known_lb && type->array.known_ub) {
		/***************************************/
		/* some systems signal that this is an */
		/* open-ended array by making lb > ub  */
		/***************************************/
		if (type->array.lb.value <= type->array.ub.value) {
			type->array.size = base_type->simple.size * (type->array.ub.value - type->array.lb.value + 1);
		} else {
			type->array.known_ub = 0;

//#ifdef NEL_NTRP

			type->array.ub.expr = NULL;
//#else
//
//			type->array.ub.symbol = NULL;
//#endif /* NEL_NTRP */

		}
	}
	stab_pop_type (eng, index_type);
	stab_push_type (eng, type);

	return 0;
}


/* Read a number from a string.  */

int parse_number (const char **pp)
{
	unsigned long ul;
	const char *orig;


	orig = *pp;

	ul = strtoul (*pp, (char **)pp, 0);

	/* If the number is meant to be negative,
	   we have to make sure that we sign extend properly. */
	if (*orig == '-') {
		return (long)ul;
	}

	return ul;
}


/* Parse a single field in a struct or union.  */

int parse_stab_one_struct_field (struct nel_eng *eng, const char **pp, const char *p)
{
	nel_symbol *symbol;
	const char *orig;
	int bit_offset, bit_size;
	char *name;

#if 0 
	enum debug_visibility visibility;
	debug_type type;
#endif

	orig = *pp;

	/* FIXME: gdb checks ARM_DEMANGLING here.  */

	//name = savestring (*pp, p - *pp);
	//eng->stab->str_scan--;
	//nel_stab_read_name (eng, eng->stab->name_buffer, &eng->stab->str_scan, nel_MAX_SYMBOL_LENGTH);
	stab_read_name (eng, eng->stab->name_buffer, (char **)pp, nel_MAX_SYMBOL_LENGTH);
	name = nel_insert_name(eng, eng->stab->name_buffer);
	//stab_push_name (eng, name);
	//stab_pop_name (eng, name);
	symbol = nel_static_symbol_alloc (eng, name, NULL, NULL, nel_C_NULL, 0, eng->stab->source_lang, eng->stab->level);
	stab_push_symbol (eng, symbol);

	*pp = p + 1;

	if (**pp != '/') {
		//visibility = DEBUG_VISIBILITY_PUBLIC;
	} else {
		++*pp;
		switch (**pp) {
		case '0':
			//visibility = DEBUG_VISIBILITY_PRIVATE;
			break;
		case '1':
			//visibility = DEBUG_VISIBILITY_PROTECTED;
			break;
		case '2':
			//visibility = DEBUG_VISIBILITY_PUBLIC;
			break;
		default:
			nel_debug({stab_trace (eng, "unknown visibility character for field: %s\n", orig);});
			//visibility = DEBUG_VISIBILITY_PUBLIC;
			break;
		}
		++*pp;
	}

	parse_stab_type_ex (eng, (const char *) NULL, pp);

	if (**pp == ':') {
		char *varname;

		/* This is a static class member.  */
		++*pp;
		p = strchr (*pp, ';');
		if (p == NULL) {
			nel_debug({stab_trace (eng, "bad stab: %s\n", orig);});
			return -1;
		}

		//varname = savestring (*pp, p - *pp);
		eng->stab->str_scan--;
		stab_read_name (eng, eng->stab->name_buffer, &eng->stab->str_scan, nel_MAX_SYMBOL_LENGTH);
		varname = nel_insert_name(eng, eng->stab->name_buffer);
		//stab_push_name (eng, varname);
		//stab_pop_name (eng, varname);
		symbol = nel_static_symbol_alloc (eng, varname, NULL, NULL, nel_C_NULL, 0, eng->stab->source_lang, eng->stab->level);
		stab_push_symbol (eng, symbol);

		*pp = p + 1;

		return 0;
	}

	if (**pp != ',') {
		nel_debug({stab_trace (eng, "bad stab: %s\n", orig);});
		return -1;
	}
	++*pp;

	bit_offset = parse_number (pp);
	if (**pp != ',') {
		nel_debug({stab_trace (eng, "bad stab: %s\n", orig);});
		return -1;
	}
	++*pp;

	bit_size = parse_number (pp);
	if (**pp != ';') {
		nel_debug({stab_trace (eng, "bad stab: %s\n", orig);});
		return -1;
	}
	++*pp;

	if (bit_offset == 0 && bit_size == 0) {
		/* This can happen in two cases: (1) at least for gcc 2.4.5 or
		so, it is a field which has been optimized out.  The correct
		stab for this case is to use VISIBILITY_IGNORE, but that is a
		recent invention.  (2) It is a 0-size array.  For example
		union { int num; char str[0]; } foo.  Printing "<no value>"
		for str in "p foo" is OK, since foo.str (and thus foo.str[3])
		will continue to work, and a 0-size array as a whole doesn't
		have any contents to print.

		I suspect this probably could also happen with gcc -gstabs
		(not -gstabs+) for static fields, and perhaps other C++
		extensions.  Hopefully few people use -gstabs with gdb, since
		it is intended for dbx compatibility.  */
		//visibility = DEBUG_VISIBILITY_IGNORE;
	}

	{
		register nel_type *type;
		//register int bit_offset;
		//register int bit_size;
		register nel_symbol *symbol;
		register nel_member *member;
		register unsigned_int size;
		register unsigned_int alignment;

		//stab_pop_integer (eng, bit_size);
		//stab_pop_integer (eng, bit_offset);
		stab_pop_type (eng, type);
		stab_pop_symbol (eng, symbol);

		nel_debug ({
					   if (symbol == NULL) {
					   stab_fatal_error (eng, "stab #%d (field->SYMBOL ':' type_desc ',' INTEGER ',' INTEGER ';' #1): bad symbol\n%1S", eng->stab->sym_ct, symbol)
						   ;
					   }
				   });

		/******************************************************/
		/* Mark the symbol as a member, so that it will be    */
		/* garbage collected should we remove type descriptor */
		/* this is a part of at a later date.  We know the    */
		/* symbol was statically allocated, as all member     */
		/* symbols should be.  set the type field, too.       */
		/******************************************************/
		symbol->type = type;
		symbol->class = nel_C_MEMBER;

		nel_debug ({
					   if (type == NULL) {
					   stab_fatal_error (eng, "stab #%d (field->SYMBOL ':' type_desc ',' INTEGER ',' INTEGER ';' #2): NULL member type", eng->stab->sym_ct)
						   ;
					   }
				   });

		/******************************************************/
		/* Now check to see if we have a bit field.  A bit    */
		/* field must be of an integral type, and integral    */
		/* types are already entered in the nel_stab_type_hash */
		/* table, so we assume that an nel_stab_undef struct   */
		/* (with 0 size) implies that this isn't a bit field. */
		/* For types whose size is already defined, check to  */
		/* see that the bit size is equal to the size field   */
		/* field of the type descriptor, and that the bit     */
		/* offset is a multiple of the word size, or for data */
		/* types smaller than one word, a multiple of the     */
		/* size of the data object.                           */
		/******************************************************/
		size = type->simple.size;
		alignment = type->simple.alignment;
		if (bit_size < 0) {
			stab_stmt_error (eng, "stab #%d: illegal field size: %I", eng->stab->sym_ct, symbol);
		}
		if ((size == 0) || ((bit_size == CHAR_BIT * size)
			&& ( type->simple.alignment != 0  && ( bit_offset % (type->simple.alignment * CHAR_BIT) == 0)))) {
			member = nel_member_alloc (eng, symbol, 0, 0, 0, bit_offset / CHAR_BIT, NULL);
		} else {
			/***************************************************/
			/* we have a bit field.  try to make the offset    */
			/* as small as possible and the bit_offset         */
			/* proportionately larger, so that we do not fetch */
			/* data outside of the struct when retreiving the  */
			/* datum which contains the bit field, in case the */
			/* bit field is near the end.                      */
			/***************************************************/
			register int offset;
			if (bit_size > CHAR_BIT * size) {
				stab_stmt_error (eng, "stab #%d: bit field too big: %I", eng->stab->sym_ct, symbol);
			}
			offset = ((bit_offset + bit_size - 1) / CHAR_BIT) + 1;
			offset = nel_align_offset (offset, alignment);
			offset -= size;
			if (offset < 0) {
				offset = 0;
			}
			bit_offset -= (offset * CHAR_BIT);
			member = nel_member_alloc (eng, symbol, 1, bit_size, bit_offset, offset, NULL);
		}
		stab_push_member (eng, member);
	}

	/* FIXME: gdb does some stuff here to mark fields as unpacked.  */

	return 0;
}


/* Read struct or class data fields.  They have the form:
 
	NAME : [VISIBILITY] TYPENUM , BITPOS , BITSIZE ;
 
   At the end, we see a semicolon instead of a field.
 
   In C++, this may wind up being NAME:?TYPENUM:PHYSNAME; for
   a static field.
 
   The optional VISIBILITY is one of:
 
	'/0'	(VISIBILITY_PRIVATE)
	'/1'	(VISIBILITY_PROTECTED)
	'/2'	(VISIBILITY_PUBLIC)
	'/9'	(VISIBILITY_IGNORE)
 
   or nothing, for C style fields with public visibility.
 
   Returns 0 for success, -1 for failure.  */

int parse_stab_struct_fields (struct nel_eng *eng, const char **pp)
{
	nel_member *member, *end;
	const char *orig;
	const char *p;
	unsigned int c;


	orig = *pp;
	c = 0;

	while (**pp != ';') {
		/* FIXME: gdb checks os9k_stabs here.  */

		//to deal with strings end with '\\'.
		if (**pp == '\\') {
			*pp += 2;
		}


		p = *pp;

#if 0
		/* Add 1 to c to leave room for NULL pointer at end.  */
		if (c + 1 >= alloc) {
			alloc += 10;
			fields = ((debug_field *) xrealloc (fields, alloc * sizeof *fields));
		}

		/* If it starts with CPLUS_MARKER it is a special abbreviation,
		unless the CPLUS_MARKER is followed by an underscore, in
		which case it is just the name of an anonymous type, which we
		should handle like any other type name.  We accept either '$'
		or '.', because a field name can never contain one of these
		characters except as a CPLUS_MARKER.  */

		if ((*p == '$' || *p == '.') && p[1] != '_') {
			++*pp;
			if (! parse_stab_cpp_abbrev (dhandle, info, pp, fields + c))
				return FALSE;
			++c;
			continue;
		}
#endif

		/* Look for the ':' that separates the field name from the field
		values.  Data members are delimited by a single ':', while member
		functions are delimited by a pair of ':'s.  When we hit the member
		functions (if any), terminate scan loop and return.  */

		p = strchr (p, ':');
		if (p == NULL) {
			nel_debug({stab_trace (eng, "bad stab: %s\n", orig);});
			return -1;
		}

		if (p[1] == ':')
			break;

		if (parse_stab_one_struct_field (eng, pp, p) == -1)
			return -1;

		++c;


		if (c > 1) {
			stab_pop_member (eng, member);
			stab_pop_member (eng, end);
			end->next = member;
			stab_push_member (eng, member);
		} else {
			/*******************************/
			/* duplicate the top of stack, */
			/* the first field             */
			/*******************************/
			stab_top_member (eng, member, 0);
			stab_push_member (eng, member);
		}

	}


	return 0;
}


/* Read the description of a structure (or union type) and return an object
   describing the type.
 
   PP points to a character pointer that points to the next unconsumed token
   in the stabs string.  For example, given stabs "A:T4=s4a:1,0,32;;",
   *PP will point to "4a:1,0,32;;".  */

int parse_stab_struct_type (struct nel_eng *eng, const char *tagname, const char **pp, const int *typenums)
{

	nel_type *type;
	nel_member *start;
	nel_member *end;
	unsigned_int size;
	const char *orig;


	/********************/
	/* structure/record */
	/********************/

	orig = *pp;

	/* Get the size.  */
	size = parse_number (pp);
	
	union nel_STACK *temp;
	temp = eng->stab->semantic_stack_next;


	if (parse_stab_struct_fields (eng, pp) == -1)
		return -1;
	++*pp; //skip the final ';'

	if(temp == eng->stab->semantic_stack_next) {
		if(size)
			return -1;
		type = nel_type_alloc(eng, nel_D_STRUCT, 0, 0, 0, 0, NULL, NULL);
		stab_push_type(eng, type);
		return 0;
	}

	stab_pop_member (eng, end);
	stab_pop_member (eng, start);
	//stab_pop_integer (eng, size);
	type = nel_type_alloc (eng, nel_D_STRUCT, size, 0, 0, 0, NULL, start);
	if( nel_s_u_size (eng, &(type->s_u.alignment), start) < 0){
		stab_fatal_error (eng, "(nel_s_u_size #1): illegal nel_member structure:\n%1M", start);
	}
	stab_push_type (eng, type);

	return 0;
}


/* Read the description of a structure (or union type) and return an object
   describing the type.
 
   PP points to a character pointer that points to the next unconsumed token
   in the stabs string.  For example, given stabs "A:T4=s4a:1,0,32;;",
   *PP will point to "4a:1,0,32;;".  */

int parse_stab_union_type (struct nel_eng *eng, const char *tagname, const char **pp, const int *typenums)
{

	nel_type *type;
	nel_member *start;
	nel_member *end;
	unsigned_int size;
	nel_symbol *member_sym;
	const char *orig;

	/***********/
	/* C union */
	/***********/

	orig = *pp;

	/* Get the size.  */
	size = parse_number (pp);

	union nel_STACK *temp;
	temp = eng->stab->semantic_stack_next;


	if (parse_stab_struct_fields (eng, pp) == -1)
		return -1;
	++*pp; //skip the final ';'

	if(temp == eng->stab->semantic_stack_next) {
		if(size)
			return -1;
		type = nel_type_alloc(eng, nel_D_UNION, 0, 0, 0, 0, NULL, NULL);
		stab_push_type(eng, type);
		return 0;
	}

	stab_pop_member (eng, end);
	stab_pop_member (eng, start);
	//stab_pop_integer (eng, size);
	if (nel_bit_field_member (start, &member_sym)) {
		stab_stmt_error (eng, "stab #%d: bit field in union: %I", eng->stab->sym_ct, member_sym);
	}
	type = nel_type_alloc (eng, nel_D_UNION, size, 0, 0, 0, NULL, start);
	if( nel_s_u_size (eng, &(type->s_u.alignment), start) < 0){
		stab_fatal_error (eng, "(nel_s_u_size #1): illegal nel_member structure:\n%1M", start);
	}
	stab_push_type (eng, type);

	return 0;
}



/* Read a number by which a type is referred to in dbx data, or
   perhaps read a pair (FILENUM, TYPENUM) in parentheses.  Just a
   single number N is equivalent to (0,N).  Return the two numbers by
   storing them in the vector TYPENUMS.  */

int parse_stab_type_number (struct nel_eng *eng, const char **pp, int *typenums)
{
	const char *orig;


	orig = *pp;

	if (**pp != '(') {
		typenums[0] = 0;
		typenums[1] = (int) parse_number (pp);
	} else {
		++*pp;
		typenums[0] = (int) parse_number (pp);
		if (**pp != ',') {
			nel_debug({stab_trace (eng, "bad stab: %s\n", orig);});
			return -1;
		}
		++*pp;
		typenums[1] = (int) parse_number (pp);
		if (**pp != ')') {
			nel_debug({stab_trace (eng, "bad stab: %s\n", orig);});
			return -1;
		}
		++*pp;
	}

	return 0;
}


/* Parse a stabs type.  The typename argument is non-NULL if this is a
   typedef or a tag definition.  The pp argument points to the stab
   string, and is updated. */

int parse_stab_type(struct nel_eng *eng, const char *typename, const char **pp)
{
	nel_stab_type *stab_type;
	nel_type *ntype;
	nel_symbol *symbol;
	int typenums[2]; /* typenums[0]-file_num, typenums[1]-type_num */
	int descriptor;
	int size=-1;
	const char *orig;
	char *name;

	if (typename !=NULL && strcmp(typename, "stream") == 0 ) {
		printf("parse_stab_type, found stream!\n"); 
	}

	orig = *pp;

	/* Read type number if present.  The type number may be omitted.
	   for instance in a two-dimensional array declared with type
	   "ar1;1;10;ar1;1;10;4".  */
	if (! isdigit(**pp) && **pp != '(' && **pp != '-') {
		/* 'typenums=' not present, type is anonymous.  Read and return
		the definition, but don't put it in the type vector.  */
		typenums[0] = typenums[1] = -1;
	} else {
		if (parse_stab_type_number (eng, pp, typenums) == -1)
			return -1;

		if (**pp != '=') {
			/* Type is not being defined here.  Either it already
			   exists, or this is a forward reference to it.  */

			//stab_pop_integer (eng, type_num);
			//stab_pop_integer (eng, file_num);
			//stab_type = nel_stab_lookup (eng, file_num, type_num);
			stab_type = stab_lookup_type_2 (eng, typenums[0], typenums[1]);

			/*******************************************************/
			/* if we don't find the type, allocate a nel_stab_undef */
			/* type as a placeholder, and push it, after inserting */
			/* the correspoinding nel_stab_type.                    */
			/*******************************************************/
			if (stab_type == NULL) {
				//stab_type = stab_type_alloc (eng, NULL, file_num, type_num);
				stab_type = stab_type_alloc (eng, NULL, typenums[0], typenums[1]);
				stab_type->type = nel_type_alloc (eng, nel_D_STAB_UNDEF, 0, 0, 0, 0, stab_type);
				stab_insert_type_2 (eng, stab_type);

			} else {
				nel_debug ({
							   if (stab_type->type == NULL) {
							   stab_fatal_error (eng, "stab #%d: (type_def->type_num #1): stab_type->type == NULL\n%1U", eng->stab->sym_ct, stab_type)
								   ;
							   }
						   });
			}
			stab_push_type (eng, stab_type->type);

			return 0;
		}

		/* Only set the slot if the type is being defined.  This means
		   that the mapping from type numbers to types will only record
		   the name of the typedef which defines a type.  If we don't do
		   this, then something like
		typedef int foo;
		int i;
		will record that i is of type foo.  Unfortunately, stabs
		information is ambiguous about variable types.  For this code,
		typedef int foo;
		int i;
		foo j;
		the stabs information records both i and j as having the same
		type.  This could be fixed by patching the compiler.  */
		//if (slotp != NULL && typenums[0] >= 0 && typenums[1] >= 0)
		//  *slotp = stab_find_slot (info, typenums);

		/* Type is being defined here.  */
		/* Skip the '='.  */
		++*pp;

		while (**pp == '@') {
			const char *p = *pp + 1;
			const char *attr;


			if (isdigit(*p) || *p == '(' || *p == '-')
				/* Member type.  */
				break;

			/* Type attributes.  */
			attr = p;

			for (; *p != ';'; ++p) {
				if (*p == '\0') {
					nel_debug({stab_trace (eng, "bad stab: %s\n", orig);});
					return -1;
				}
			}
			*pp = p + 1;

			switch (*attr) {
			case 'a': //duno wheather it's useful...
				/***************/
				/* align class */
				/***************/
				{
					int align;
					align = atoi (attr + 1);
				}
				break;

			case 'p': //duno wheather it's useful...
				/*****************/
				/* pointer class */
				/*****************/
				{
					int pointer;
					pointer = atoi (attr + 1);
				}
				break;

			case 's':
				/**************/
				/* size class */
				/**************/
				size = atoi (attr + 1);
				size /= 8;  /* Size is in bits.  We store it in bytes.  */
				if (size <= 0)
					size = -1;
				break;

			case 'S':
				//stringp = TRUE;
				break;

			default:
				/* Ignore unrecognized type attributes, so future
				compilers can invent new ones.  */
				break;
			}
		}

		//stab_pop_integer (eng, type_num);
		//stab_pop_integer (eng, file_num);
		stab_type = stab_type_alloc (eng, NULL, typenums[0], typenums[1]);
		stab_type->type = nel_type_alloc (eng, nel_D_STAB_UNDEF, 0, 0, 0, 0, stab_type);
		stab_insert_type_2 (eng, stab_type);
		stab_push_stab_type (eng, stab_type);
	}

	descriptor = **pp;
	++*pp;

	switch (descriptor) {
	case 'x': {
			const char *q1, *q2, *p;

			/* A cross reference to another type.  */
			switch (**pp) {
			case 's':
				/*******************************/
				/* incomplete structure/record */
				/*******************************/
				{
					/****************************************************/
					/* lookup the tag to see if it is already defined   */
					/* (in a previous compilation unit).  if it is, use */
					/* that type descriptor.  if we used thie current   */
					/* type descriptor we would have to insert it in a  */
					/* table somewhere so that we could later complete  */
					/* it (which may not be a bad idea).  if we have    */
					/* two structs or two unions with the same tag name */
					/* in two different compilation units which are     */
					/* structurally different, and the second type is   */
					/* incomplete at some point, the type information   */
					/* for it will be incorrect.                        */
					/****************************************************/
					register nel_type *struct_type_d;

					++*pp;
					stab_read_name (eng, eng->stab->name_buffer, (char **)pp, nel_MAX_SYMBOL_LENGTH);
					name = nel_insert_name(eng, eng->stab->name_buffer);
					//stab_push_name (eng, name);

					//stab_pop_name (eng, name);
					if ((symbol = stab_lookup_tag (eng, name)) == NULL)
					{
						register nel_type *tag_type_d;
						symbol = nel_static_symbol_alloc (eng, name, NULL, NULL, nel_C_TYPE, 0, eng->stab->source_lang, eng->stab->level);
						struct_type_d = nel_type_alloc (eng, nel_D_STRUCT, 0, 0, 0, 0, symbol, NULL);
						tag_type_d = nel_type_alloc (eng, nel_D_STRUCT_TAG, 0, 0, 0, 0, struct_type_d);
						symbol->type = tag_type_d;
						symbol->value = (char *) struct_type_d;
						stab_insert_tag (eng, symbol);
					} else
					{
						nel_debug ({
									   if (symbol->type == NULL) {
									   stab_fatal_error (eng, "stab #%d (engord->'x' 's' NAME ':' #1): symbol->type == NULL", eng->stab->sym_ct)
										   ;
									   }
								   });
						if (symbol->type->simple.type != nel_D_STRUCT_TAG) {
							/***********************************************/
							/* if the identifier is already the tag of a   */
							/* non-struct, emit an error message, and form */
							/* a new type descriptor anyway.               */
							/***********************************************/
							register nel_type *tag_type_d;
							stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, symbol);
							symbol = nel_static_symbol_alloc (eng, name, NULL, NULL, nel_C_TYPE, 0, eng->stab->source_lang, eng->stab->level);
							struct_type_d = nel_type_alloc (eng, nel_D_STRUCT, 0, 0, 0, 0, symbol, NULL);
							tag_type_d = nel_type_alloc (eng, nel_D_STRUCT_TAG, 0, 0, 0, 0, struct_type_d);
							symbol->type = tag_type_d;
							symbol->value = (char *) struct_type_d;
						} else {
							struct_type_d = symbol->type->tag_name.descriptor;
							nel_debug ({
										   if (struct_type_d == NULL) {
										   stab_fatal_error (eng, "stab #%d (engord->'x' 's' NAME ':' #2): struct_type_d == NULL", eng->stab->sym_ct)
											   ;
										   }
									   });
						}
					}
					stab_push_type (eng, struct_type_d);
				}
				//code = DEBUG_KIND_STRUCT;
				break;

			case 'u':
				/**********************/
				/* incomplete C union */
				/**********************/
				{
					/****************************************************/
					/* lookup the tag to see if it is already defined   */
					/* (in a previous compilation unit).  if it is, use */
					/* that type descriptor.  if we used thie current   */
					/* type descriptor we would have to insert it in a  */
					/* table somewhere so that we could later complete  */
					/* it (which may not be a bad idea).  if we have    */
					/* two structs or two unions with the same tag name */
					/* in two different compilation units which are     */
					/* structurally different, and the second type is   */
					/* incomplete at some point, the type information   */
					/* for it will be incorrect.                        */
					/****************************************************/
					register nel_type *union_type_d;

					++*pp;
					stab_read_name (eng, eng->stab->name_buffer, (char **)pp, nel_MAX_SYMBOL_LENGTH);
					name = nel_insert_name(eng, eng->stab->name_buffer);
					//stab_push_name (eng, name);

					//stab_pop_name (eng, name);
					if ((symbol = stab_lookup_tag (eng, name)) == NULL)
					{
						register nel_type *tag_type_d;
						symbol = nel_static_symbol_alloc (eng, name, NULL, NULL, nel_C_TYPE, 0, eng->stab->source_lang, eng->stab->level);
						union_type_d = nel_type_alloc (eng, nel_D_UNION, 0, 0, 0, 0, symbol, NULL);
						tag_type_d = nel_type_alloc (eng, nel_D_UNION_TAG, 0, 0, 0, 0, union_type_d);
						symbol->type = tag_type_d;
						symbol->value = (char *) union_type_d;
						stab_insert_tag (eng, symbol);
					} else
					{
						nel_debug ({
									   if (symbol->type == NULL) {
									   stab_fatal_error (eng, "stab #%d (engord->'x' 'u' NAME ':' #1): symbol->type == NULL", eng->stab->sym_ct)
										   ;
									   }
								   });
						if (symbol->type->simple.type != nel_D_UNION_TAG) {
							/**********************************************/
							/* if the identifier is already the tag of a  */
							/* non-union, emit an error message, and form */
							/* a new type descriptor anyway.              */
							/**********************************************/
							register nel_type *tag_type_d;
							stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, symbol);
							symbol = nel_static_symbol_alloc (eng, name, NULL, NULL, nel_C_TYPE, 0, eng->stab->source_lang, eng->stab->level);
							union_type_d = nel_type_alloc (eng, nel_D_UNION, 0, 0, 0, 0, symbol, NULL);
							tag_type_d = nel_type_alloc (eng, nel_D_UNION_TAG, 0, 0, 0, 0, union_type_d);
							symbol->type = tag_type_d;
							symbol->value = (char *) union_type_d;
						} else {
							union_type_d = symbol->type->tag_name.descriptor;
							nel_debug ({
										   if (union_type_d == NULL) {
										   stab_fatal_error (eng, "stab #%d (engord->'x' 'u' NAME ':' #2): union_type_d == NULL", eng->stab->sym_ct)
											   ;
										   }
									   });
						}
					}
					stab_push_type (eng, union_type_d);
				}
				//code = DEBUG_KIND_UNION;
				break;

			case 'e':
				/**************************/
				/* incomplete enumeration */
				/**************************/
				{
					/****************************************************/
					/* lookup the tag to see if it is already defined   */
					/* (in a previous compilation unit).  if it is, use */
					/* that type descriptor.  if we used thie current   */
					/* type descriptor we would have to insert it in a  */
					/* table somewhere so that we could later complete  */
					/* it (which may not be a bad idea).  if we have    */
					/* two enumerated types with the same tag name      */
					/* in two different compilation units which are     */
					/* structurally different, and the second type is   */
					/* incomplete at some point, the type information   */
					/* for it will be incorrect.                        */
					/****************************************************/
					register nel_type *enum_type_d;

					++*pp;
					stab_read_name (eng, eng->stab->name_buffer, (char **)pp, nel_MAX_SYMBOL_LENGTH);
					name = nel_insert_name(eng, eng->stab->name_buffer);
					//stab_push_name (eng, name);

					//stab_pop_name (eng, name);
					if ((symbol = stab_lookup_tag (eng, name)) == NULL)
					{
						register nel_type *tag_type_d;
						symbol = nel_static_symbol_alloc (eng, name, NULL, NULL, nel_C_TYPE, 0, eng->stab->source_lang, eng->stab->level);
						enum_type_d = nel_type_alloc (eng, nel_D_ENUM, sizeof (int), nel_alignment_of (int), 0, 0, symbol, NULL, 0);
						tag_type_d = nel_type_alloc (eng, nel_D_ENUM_TAG, 0, 0, 0, 0, enum_type_d);
						symbol->type = tag_type_d;
						symbol->value = (char *) enum_type_d;
						stab_insert_tag (eng, symbol);
					} else
					{
						nel_debug ({
									   if (symbol->type == NULL) {
									   stab_fatal_error (eng, "stab #%d (enumeration->'x' 'e' NAME ':' #1): symbol->type == NULL", eng->stab->sym_ct)
										   ;
									   }
								   });
						if (symbol->type->simple.type != nel_D_ENUM_TAG) {
							/*********************************************/
							/* if the identifier is already the tag of a */
							/* non-enum, emit an error message, and form */
							/* a new type descriptor anyway.             */
							/*********************************************/
							register nel_type *tag_type_d;
							stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, symbol);
							symbol = nel_static_symbol_alloc (eng, name, NULL, NULL, nel_C_TYPE, 0, eng->stab->source_lang, eng->stab->level);
							enum_type_d = nel_type_alloc (eng, nel_D_ENUM, sizeof (int), nel_alignment_of (int), 0, 0, symbol, NULL);
							tag_type_d = nel_type_alloc (eng, nel_D_ENUM_TAG, 0, 0, 0, 0, enum_type_d);
							symbol->type = tag_type_d;
							symbol->value = (char *) enum_type_d;
						} else {
							enum_type_d = symbol->type->tag_name.descriptor;
							nel_debug ({
										   if (enum_type_d == NULL) {
										   stab_fatal_error (eng, "stab #%d (enumeration->'x' 'e' NAME ':' #2): enum_type_d == NULL", eng->stab->sym_ct)
											   ;
										   }
									   });
						}
					}
					stab_push_type (eng, enum_type_d);
				}
				//code = DEBUG_KIND_ENUM;
				break;

			default:
				/* Complain and keep going, so compilers can invent new
				   cross-reference types.  */
				nel_debug({stab_trace (eng, "unrecognized cross reference type: %s\n", orig);});
				//code = DEBUG_KIND_STRUCT;
				break;
			}
			//++*pp;

			q1 = strchr (*pp, '<');
			p = strchr (*pp, ':');
			if (p == NULL) {
				nel_debug({stab_trace (eng, "bad stab: %s\n", orig);});
				return -1;
			}
			if (q1 != NULL && p > q1 && p[1] == ':') {
				int nest = 0;

				for (q2 = q1; *q2 != '\0'; ++q2) {
					if (*q2 == '<')
						++nest;
					else if (*q2 == '>')
						--nest;
					else if (*q2 == ':' && nest == 0)
						break;
				}
				p = q2;
				if (*p != ':') {
					nel_debug({stab_trace (eng, "bad stab: %s\n", orig);});
					return -1;
				}
			}

			/* Some versions of g++ can emit stabs like
			       fleep:T20=xsfleep:
			   which define structures in terms of themselves.  We need to
			   tell the caller to avoid building a circular structure.  */
			//if (typename != NULL
			//    && strncmp (typename, *pp, p - *pp) == 0
			//    && typename[p - *pp] == '\0')
			//  info->self_crossref = TRUE;

			//dtype = stab_find_tagged_type (dhandle, info, *pp, p - *pp, code);

			*pp = p + 1;
		}
		break;

	case '-':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case '(':
		--*pp;
		parse_stab_type_ex (eng, (const char *) NULL, pp);
		break;

	case '*':
		/************************/
		/* pointer to type_desc */
		/************************/
		parse_stab_type_ex (eng, (const char *) NULL, pp);
		stab_pop_type (eng, ntype);
		ntype = nel_type_alloc (eng, nel_D_POINTER, sizeof (char *), nel_alignment_of (unsigned long), 0, 0, ntype);
		//end modified
		stab_push_type (eng, ntype);
		break;

#if 0 

	case '&':
		/* Reference to another type.  */
		{
			register nel_type *type;
			type = parse_stab_type (dhandle, info, (const char *) NULL, pp, (debug_type **) NULL);
			//stab_pop_type (eng, type);
			//type = nel_type_alloc (eng, ??nel_D_POINTER, sizeof (char *), nel_alignment_of (char), 0, 0, type);
			//stab_push_type (eng, type);
			return type;
		}
		break;
#endif

	case 'f':
		/* Function returning another type.  */
		/* FIXME: gdb checks os9k_stabs here.  */

		/*******************/
		/* C function type */
		/*******************/
		{
			register nel_type *type;
			register nel_type *return_type;
			int temp_num[2];
			char *temp_pp = *((char **)pp);

			if( parse_stab_type_number( eng, pp, temp_num ) != 0 ) {
				stab_fatal_error( eng, "parse_stab_type_number FAILED\n" );
			}
			*pp = temp_pp;
			if( stab_lookup_type_2 (eng, temp_num[0], temp_num[1]) ) {
				parse_stab_type (eng, (const char *) NULL, pp);
				stab_pop_type (eng, return_type);
			}
			else {
				nel_stab_type *stab_type;
				parse_stab_type (eng, (const char *) NULL, pp);
				stab_pop_stab_type (eng, stab_type);
				return_type = stab_type->type;
			}
			type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 0, return_type, NULL, NULL, NULL);
			stab_push_type (eng, type);
		}
		break;

	case 'F':
		/*******************/
		/* C function type */
		/*******************/
		{
			register nel_type *type;
			register nel_type *return_type;
			parse_stab_type (eng, (const char *) NULL, pp);
			stab_pop_type (eng, return_type);
			type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 0, return_type, NULL, NULL, NULL);
			stab_push_type (eng, type);
		}
		break;


	case 'k':

#if 1
		parse_stab_type_ex (eng, (const char *) NULL, pp);

#else
		/* Const qualifier on some type (Sun).  */
		/* FIXME: gdb accepts 'c' here if os9k_stabs.  */
		//dtype = debug_make_const_type (dhandle,
		//   parse_stab_type (dhandle, info, (const char *) NULL, pp, (debug_type **) NULL));
		/**************************************************/
		/* valid only on ALLIANT_FX2800.                  */
		/* value parameter of type type_desc              */
		/* (hence the "1" in the arg list to              */
		/* nel_stab_parameter_acts().                      */
		/**************************************************/
		{

			//register nel_type *type;
			//register nel_symbol *symbol;
			parse_stab_type_number (eng, pp, typenums); 
			return 0;
			//parse_stab_type (eng, (const char *) NULL, pp);
			//stab_pop_type (eng, type);
			//stab_pop_symbol (eng, symbol);
			//nel_stab_parameter_acts (eng, symbol, type, (char *) stab_get_value (eng->stab->last_sym), 0);
		}
#endif
		break;

#if 0 
	case 'B':
		/* Volatile qual on some type (Sun).  */
		/* FIXME: gdb accepts 'i' here if os9k_stabs.  */
		dtype = (debug_make_volatile_type
				 (dhandle,
				  parse_stab_type (dhandle, info, (const char *) NULL, pp,
								   (debug_type **) NULL)));
		break;

	case '@':
		/* Offset (class & variable) type.  This is used for a pointer
		   relative to an object.  */
		{
			debug_type domain;
			debug_type memtype;

			/* Member type.  */

			domain = parse_stab_type (dhandle, info, (const char *) NULL, pp,
									  (debug_type **) NULL);
			if (domain == DEBUG_TYPE_NULL)
				return DEBUG_TYPE_NULL;

			if (**pp != ',')
			{
				nel_debug({stab_trace (eng, "bad stab: %s\n", orig);});
				return -1;
			}
			++*pp;

			memtype = parse_stab_type (dhandle, info, (const char *) NULL, pp,
									   (debug_type **) NULL);
			if (memtype == DEBUG_TYPE_NULL)
				return DEBUG_TYPE_NULL;

			dtype = debug_make_offset_type (dhandle, domain, memtype);
		}
		break;

	case '#':
		/* Method (class & fn) type.  */
		if (**pp == '#') {
			debug_type return_type;

			++*pp;
			return_type = parse_stab_type (dhandle, info, (const char *) NULL,
										   pp, (debug_type **) NULL);
			if (return_type == DEBUG_TYPE_NULL)
				return DEBUG_TYPE_NULL;
			if (**pp != ';') {
				nel_debug({stab_trace (eng, "bad stab: %s\n", orig);});
				return -1;
			}
			++*pp;
			dtype = debug_make_method_type (dhandle, return_type,
											DEBUG_TYPE_NULL,
											(debug_type *) NULL, FALSE);
		} else {
			debug_type domain;
			debug_type return_type;
			debug_type *args;
			unsigned int n;
			unsigned int alloc;
			bfd_boolean varargs;

			domain = parse_stab_type (dhandle, info, (const char *) NULL,
									  pp, (debug_type **) NULL);
			if (domain == DEBUG_TYPE_NULL)
				return DEBUG_TYPE_NULL;

			if (**pp != ',') {
				nel_debug({stab_trace (eng, "bad stab: %s\n", orig);)};
				return -1;
			}
			++*pp;

			return_type = parse_stab_type (dhandle, info, (const char *) NULL,
										   pp, (debug_type **) NULL);
			if (return_type == DEBUG_TYPE_NULL)
				return DEBUG_TYPE_NULL;

			alloc = 10;
			args = (debug_type *) xmalloc (alloc * sizeof *args);
			n = 0;
			while (**pp != ';') {
				if (**pp != ',') {
					nel_debug({stab_trace (eng, "bad stab: %s\n", orig);});
					return -1;
				}
				++*pp;

				if (n + 1 >= alloc) {
					alloc += 10;
					args = ((debug_type *)
							xrealloc (args, alloc * sizeof *args));
				}

				args[n] = parse_stab_type (dhandle, info, (const char *) NULL,
										   pp, (debug_type **) NULL);
				if (args[n] == DEBUG_TYPE_NULL)
					return DEBUG_TYPE_NULL;
				++n;
			}
			++*pp;

			/* If the last type is not void, then this function takes a
			   variable number of arguments.  Otherwise, we must strip
			   the void type.  */
			if (n == 0
					|| debug_get_type_kind (dhandle, args[n - 1]) != DEBUG_KIND_VOID)
				varargs = TRUE;
			else {
				--n;
				varargs = FALSE;
			}

			args[n] = DEBUG_TYPE_NULL;

			dtype = debug_make_method_type (dhandle, return_type, domain, args,
											varargs);
		}
		break;

	case 'b':
		/* FIXME: gdb checks os9k_stabs here.  */
		/* Sun ACC builtin int type.  */
		dtype = parse_stab_sun_builtin_type (dhandle, pp);
		break;

	case 'R':
		/* Sun ACC builtin float type.  */
		dtype = parse_stab_sun_floating_type (dhandle, pp);
		break;
#endif

	case 'r':
		/* Range type.  */
		parse_stab_range_type (eng, typename, pp, typenums);
		break;

	case 'e':
		/* Enumeration type.  */
		parse_stab_enum_type (eng, pp);
		break;

	case 's':
		/* Struct or record type.  */
		parse_stab_struct_type (eng, typename, pp, typenums);
		break;
	case 'u':
		/* Union type.  */
		parse_stab_union_type (eng, typename, pp, typenums);
		break;

	case 'a':
		/* Array type.  */
		if (**pp != 'r') {
			nel_debug({stab_trace (eng, "bad stab: %s\n", orig);});
			return -1;
		}
		++*pp;

		parse_stab_array_type (eng, pp);
		break;

	case 'S':
		/********************/
		/* set of type_desc */
		/********************/
		{
			/********************************/
			/* leave type descriptor pushed */
			/********************************/
			stab_warning (eng, "stab #%d: set coerced to base type - symbol %I", eng->stab->sym_ct, eng->stab->current_symbol);
		}
		break;
	
	case 'R':
		if( parse_complex_type(eng, pp) != 0 )	//failed to parse complex type
		{
			nel_debug({stab_trace (eng, "bad stab: %s\n", orig);});
			return -1;	
		}
		break;
	
	default:
		//don't know whether should add anything here ...
		//break;
		return 0;
	}

	/**********************************************************/
	/* definition of type number - pop the type descriptor    */
	/* (nel_type *), and the nel_stab_type structure, and set   */
	/* type member of the stab_type (presently pointing to an */
	/* stab_undef structure, entered in as_stab_type_hash)    */
	/* with the new type descriptor, then overwrite the       */
	/* stab_undef type descriptor with a copy of the new type */
	/* descriptor, so that any pointers to the previously     */
	/* undefined type are now defined.  this is really        */
	/* inelegant, but it is efficient and works.              */
	/**********************************************************/
	{
		stab_pop_type (eng, ntype);
		stab_top_stab_type (eng, stab_type, 0);

		nel_debug ({
		if (ntype == NULL) {
			stab_fatal_error (eng, "stab #%d: (type_asgn->type_num '=' type_attrs type_def #1): NULL ntype", eng->stab->sym_ct);
		}
		if ((stab_type == NULL) || (stab_type->type == NULL) || (stab_type->type->simple.type != nel_D_STAB_UNDEF)) {
			stab_fatal_error (eng, "stab #%d: (type_asgn->type_num '=' type_attrs type_def #1): bad stab_type\n%1U", eng->stab->sym_ct, stab_type);
		}
		});

		/**************************************************/
		/* set the type field of the symbol table entry   */
		/* by overwriting the undefined type descriptor.  */
		/* it's inelegant, but efficient, and it works.   */
		/* if this is a tagged type, we must also create  */
		/* a new tag, and a type descriptor for it.       */
		/* if the members list of a struct/union or the   */
		/* consts list of an enum are NULL, create a      */
		/* dummy element as a placeholder.  when the type */
		/* becomes completely defined, we will overwrite  */
		/* the dummy element with the first list element  */
		/* of the complete type, and all copies of this   */
		/* type descriptor become complete.               */
		/**************************************************/
		{
			register nel_type *new_type = stab_type->type;
			register nel_symbol *old_tag;
			register nel_type *tag_type_d;
			nel_copy (sizeof (nel_type), new_type, ntype);
			switch (ntype->simple.type)
			{

			case nel_D_STRUCT:
				/*****************************************/
				/* if the members list is NULL, make a   */
				/* dummy element in it as a placeholder. */
				/*****************************************/
				if (ntype->s_u.members == NULL) {
					new_type->s_u.members = ntype->s_u.members = nel_member_alloc (eng, NULL, 0, 0, 0, 0, NULL);
				}

				/*****************************/
				/* if this is a tagged type, */
				/* create a new tag symbol.  */
				/*****************************/
				if ((old_tag = ntype->s_u.tag) == NULL) {
					break;
				}
				tag_type_d = nel_type_alloc (eng, nel_D_STRUCT_TAG, 0, 0, 0, 0, new_type);
				new_type->s_u.tag = nel_static_symbol_alloc (eng, old_tag->name, tag_type_d, (char *) new_type,
									nel_C_TYPE, 0, eng->stab->source_lang, old_tag->level);
				break;

			case nel_D_UNION:
				/*****************************************/
				/* if the members list is NULL, make a   */
				/* dummy element in it as a placeholder. */
				/*****************************************/
				if (ntype->s_u.members == NULL) {
					new_type->s_u.members = ntype->s_u.members = nel_member_alloc (eng, NULL, 0, 0, 0, 0, NULL);
				}

				/*****************************/
				/* if this is a tagged type, */
				/* create a new tag symbol.  */
				/*****************************/
				if ((old_tag = ntype->s_u.tag) == NULL) {
					break;
				}
				tag_type_d = nel_type_alloc (eng, nel_D_UNION_TAG, 0, 0, 0, 0, new_type);
				new_type->s_u.tag = nel_static_symbol_alloc (eng, old_tag->name, tag_type_d, (char *) new_type,
									nel_C_TYPE, 0, eng->stab->source_lang, old_tag->level);
				break;

			case nel_D_ENUM:
				/*****************************************/
				/* if the consts list is NULL, make a    */
				/* dummy element in it as a placeholder. */
				/*****************************************/
				if (ntype->enumed.consts == NULL) {
					new_type->enumed.consts = ntype->enumed.consts = nel_list_alloc (eng, 0, NULL, NULL);
				}

				/*****************************/
				/* if this is a tagged type, */
				/* create a new tag symbol.  */
				/*****************************/
				if ((old_tag = ntype->enumed.tag) == NULL) {
					break;
				}
				tag_type_d = nel_type_alloc (eng, nel_D_ENUM_TAG, 0, 0, 0, 0, new_type);
				new_type->enumed.tag = nel_static_symbol_alloc (eng, old_tag->name, tag_type_d, (char *) new_type,
									   nel_C_TYPE, 0, eng->stab->source_lang, old_tag->level);
				break;

			default:
				break;

			}

		}

		/***********************************/
		/* stab_type->type now points to   */
		/* the overwritten type descriptor */
		/***********************************/

		nel_debug ({
		nel_debug({stab_trace (eng, "stab_type =\n%1U\n", stab_type);});
		});

	}

	return 0;
}


int parse_stab_type_ex(struct nel_eng *eng, const char *typename, const char **pp)
{
	nel_stab_type *stab_type;
	int typenums[2];
	char *hold;

	hold = (char *)*pp;
	if (**pp == '(') {
		if (parse_stab_type_number (eng, pp, typenums) == -1)
			return -1;
		if (**pp == '=') {
			*pp = hold;
			parse_stab_type (eng, (const char *) NULL, pp);

			stab_pop_stab_type (eng, stab_type);
			nel_debug ({
						   if ((stab_type == NULL) || (stab_type->type == NULL)) {
						   stab_fatal_error (eng, "stab #%d (type_desc->type_id #1): bad stab_type structure\n%1U", eng->stab->sym_ct, stab_type)
							   ;
						   }
					   });
			stab_push_type (eng, stab_type->type);
			return 0;
		}
	}
	*pp = hold;
	parse_stab_type (eng, (const char *) NULL, pp);
	return 0;
}

int dummy_func(void)
{
	printf("Hi, don't forget to remove the dummy_func!\n");
	return 1;
}

/* Parse the stabs string.  */

int parse_stab_string (struct nel_eng *eng)
{
	nel_type *ntype;
	nel_symbol *symbol;
	char *name, *string;
	const char *p;
	int value;
	int type;

	string = eng->stab->str_scan;
	p = strchr (string, ':');
	if (p == NULL) {
		return 0;
	}

	while (p[1] == ':') {
		p += 2;
		p = strchr (p, ':');
		if (p == NULL) {
			nel_debug({stab_trace (eng, "bad stab: %s\n", string);});
			return -1;
		}
	}

	if (p == string || (string[0] == ' ' && p == string + 1)) {
		name = NULL;
		/**********************************************************/
		/* push NULL so that the lower-level productions know     */
		/* that there is no name associated with this stab entry. */
		/**********************************************************/
		stab_push_symbol (eng, NULL);
		eng->stab->current_symbol == NULL;
	} else {
		//eng->stab->str_scan--;

		stab_read_name (eng, eng->stab->name_buffer, &string, nel_MAX_SYMBOL_LENGTH);
		name = nel_insert_name(eng, eng->stab->name_buffer);

		//stab_push_name (eng, name);
		//stab_pop_name (eng, name);
		symbol = nel_static_symbol_alloc (eng, name, NULL, NULL, nel_C_NULL, 0, eng->stab->source_lang, eng->stab->level);
		stab_push_symbol (eng, symbol);

		/*******************************************/
		/* set eng->stab->current_symbol for error messages */
		/*******************************************/
		stab_top_symbol (eng, symbol, 0);
		eng->stab->current_symbol = symbol;
	}

	++p;
	type = *p++;
	switch (type) {

#if 0 
	case 'a':
		/* Reference parameter which is in a register.  */
		dtype = parse_stab_type (dhandle, info, (const char *) NULL, &p,
								 (debug_type **) NULL);
		if (dtype == DEBUG_TYPE_NULL)
			return FALSE;
		if (! debug_record_parameter (dhandle, name, dtype, DEBUG_PARM_REF_REG,
									  value))
			return FALSE;
		break;
#endif

	case 'c':

		/* c is a special case, not followed by a type-number.
		SYMBOL:c=iVALUE for an integer constant symbol.
		SYMBOL:c=rVALUE for a floating constant symbol.
		SYMBOL:c=eTYPE,INTVALUE for an enum constant symbol.
		e.g. "b:c=e6,0" for "const b = blob1"
		(where type 6 is defined by "blobs:t6=eblob1:0,blob2:1,;").  */

		if (*p != '=') {
			nel_debug({stab_trace (eng, "bad stab: %s\n", string);});
			return -1;
		}
		++p;

		switch (*p++) {

		case 'b':
			/**************************************************/
			/* constants do nothing - BOOLEAN pop the integer */
			/**************************************************/
		case 'c':
			/****************************************************/
			/* constants do nothing - CHARACTOR pop the integet */
			/****************************************************/
		case 'i':
			/**************************************************/
			/* constants do nothing - INTEGER pop the integer */
			/**************************************************/

			{
				register int value;
				value = atoi(p);
				stab_push_integer(eng, value);
			}
			break;

		case 'e':
			/* SYMBOL:c=eTYPE,INTVALUE for a constant symbol whose value
			   can be represented as integral.
			   e.g. "b:c=e6,0" for "const b = blob1"
			   (where type 6 is defined by "blobs:t6=eblob1:0,blob2:1,;").  */

			/***********************************/
			/* constants do nothing - pop the  */
			/* integer and the type descriptor */
			/***********************************/
			parse_stab_type (eng, (const char *) NULL, &p);
			stab_pop_integer (eng, value);
			stab_pop_type (eng, ntype);

			if (*p != ',') {
				nel_debug({stab_trace (eng, "bad stab: %s\n", string);});
				return -1;
			}
			break;

		case 'r':
			/*****************************************************/
			/* constants do nothing - REAL never pushed anything */
			/*****************************************************/
		case 's':
			/*******************************************************/
			/* constants do nothing - STRING never pushed anything */
			/*******************************************************/
			break;

		case 'S':
			/****************************************************/
			/* constants do nothing - SET pop the char *, the 2 */
			/* integers, and the type descriptor                */
			/****************************************************/
			{
				register unsigned_int num_bits;
				register unsigned_int num_elements;

				parse_stab_type (eng, (const char *) NULL, &p);
				/* stab_pop_value (eng, ch, char *); */
				stab_pop_integer (eng, num_bits);
				stab_pop_integer (eng, num_elements);
				stab_pop_type (eng, ntype);
			}
			break;

		default:
			nel_debug({stab_trace (eng, "bad stab: %s\n", string);});
			return -1;
		}

		/***************************************/
		/* constants do nothing - nothing is   */
		/* left pushed except the symbol name. */
		/***************************************/
		{
			register nel_symbol *symbol;
			stab_pop_symbol (eng, symbol);
		}
		break;

#if 0 

	case 'C':
		/* The name of a caught exception.  */
		dtype = parse_stab_type (dhandle, info, (const char *) NULL,
								 &p, (debug_type **) NULL);
		if (dtype == DEBUG_TYPE_NULL)
			return FALSE;
		if (! debug_record_label (dhandle, name, dtype, value))
			return FALSE;
		break;

	case 'f':
	case 'F':
		/* A function definition.  */
		dtype = parse_stab_type (dhandle, info, (const char *) NULL, &p,
								 (debug_type **) NULL);
		if (dtype == DEBUG_TYPE_NULL)
			return FALSE;
		if (! debug_record_function (dhandle, name, dtype, type == 'F', value))
			return FALSE;

		/* Sun acc puts declared types of arguments here.  We don't care
		about their actual types (FIXME -- we should remember the whole
		function prototype), but the list may define some new types
		that we have to remember, so we must scan it now.  */
		while (*p == ';') {
			++p;
			if (parse_stab_type (dhandle, info, (const char *) NULL, &p,
								 (debug_type **) NULL)
					== DEBUG_TYPE_NULL)
				return FALSE;
		}
#endif
	case 'f':
		/*******************************************/
		/* local function returning type type_desc */
		/* this rule must appear before the rule:  */
		/* procedure_type->'f' type_desc.          */
		/*******************************************/
		{
			register nel_type *return_type;
			register nel_symbol *symbol;
			parse_stab_type_ex (eng, (const char *) NULL, &p);

			stab_pop_type (eng, return_type);
			stab_pop_symbol (eng, symbol);
			stab_static_routine_acts (eng, symbol, return_type);
		}
		break;

	case 'F':
		/*******************************************/
		/* function returning type type_desc       */
		/* this rule must appear before the rule:  */
		/* procedure_type->'F' type_desc.          */
		/*******************************************/
		{
			register nel_type *return_type;
			register nel_symbol *symbol;
			parse_stab_type_ex (eng, (const char *) NULL, &p);

			stab_pop_type (eng, return_type);
			stab_pop_symbol (eng, symbol);
			stab_routine_acts (eng, symbol, return_type);
		}
		break;

	case 'G':
		/*************************************************/
		/* global variable of type type_desc             */
		/* on ALLIANT_FX2800, can also be a routine.     */
		/* check the type of the stab to find out which. */
		/*************************************************/
		{
			nel_symbol *symbol;
			parse_stab_type_ex (eng, (const char *) NULL, &p);

			stab_pop_type (eng, ntype);
			stab_pop_symbol (eng, symbol);
			if (stab_get_type (eng->stab->last_sym) == N_FUN)
			{
				stab_routine_acts (eng, symbol, ntype);
			} else
			{
				stab_global_acts (eng, symbol, ntype);
			}
		}
		break;

	case 'i': /* var_proc->'i' SYMBOL ':' type_desc */
		/************************************************/
		/* ALLIANT_FX and ALLIANT_FX2800 only.          */
		/* reference parameter of same value as SYMBOL. */
		/* (hence the "1" in the arg list to            */
		/* nel_stab_parameter_acts().                    */
		/************************************************/
		{
			register nel_symbol *value_sym;
			register nel_symbol *symbol;

			eng->stab->str_scan--;
			stab_read_name (eng, eng->stab->name_buffer, &eng->stab->str_scan, nel_MAX_SYMBOL_LENGTH);
			name = nel_insert_name(eng, eng->stab->name_buffer);
			//stab_push_name (eng, name);
			//stab_pop_name (eng, name);
			symbol = nel_static_symbol_alloc (eng, name, NULL, NULL, nel_C_NULL, 0, eng->stab->source_lang, eng->stab->level);
			stab_push_symbol (eng, symbol);

			eng->stab->str_scan--;
			stab_read_name (eng, eng->stab->name_buffer, &eng->stab->str_scan, nel_MAX_SYMBOL_LENGTH);
			name = nel_insert_name(eng, eng->stab->name_buffer);
			//stab_push_name (eng, name);
			//stab_pop_name (eng, name);
			value_sym = nel_static_symbol_alloc (eng, name, NULL, NULL, nel_C_NULL, 0, eng->stab->source_lang, eng->stab->level);
			stab_push_symbol (eng, value_sym);

			parse_stab_type (eng, (const char *) NULL, &p);


			stab_pop_type (eng, ntype);
			stab_pop_symbol (eng, value_sym);
			stab_pop_symbol (eng, symbol);
			if ((value_sym = stab_lookup_ident (eng, value_sym->name)) != NULL)
			{
				nel_debug ({
							   if (value_sym->type == NULL) {
							   stab_fatal_error (eng, "stab # (var_proc->'i' SYMBOL ':' type_desc #1): NULL type for\n%1S", eng->stab->sym_ct, value_sym)
								   ;
							   }
						   });
				stab_parameter_acts (eng, symbol, ntype, value_sym->value, 1);
			} else
			{
				stab_error (eng, "invalid stab for parameter %I", symbol);
			}
		}
		break;

	case 'I':
		/***************************************************/
		/* internal procedure (different calling sequence) */
		/***************************************************/
		{
			register nel_symbol *symbol;
			stab_top_symbol (eng, symbol, 0);
			if (symbol != NULL)
			{
				symbol->type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 0, nel_void_type, NULL, NULL, eng->stab->comp_unit);
				symbol->class = nel_C_COMPILED_FUNCTION;
				symbol->value = (char *)(long ) stab_get_value (eng->stab->last_sym);

				/********************************/
				/* don't insert procedure -     */
				/* we don't know how to call it */
				/********************************/

				/************************************************/
				/* call nel_stab_start_routine to clean up after */
				/* the previous routine and start forming the   */
				/* block strucutre for the new routine.         */
				/************************************************/
				stab_start_routine (eng, symbol);
			}
		}
		break;

	case 'J':
		/*********************/
		/* internal function */
		/*********************/
		{
			register nel_symbol *symbol;
			parse_stab_type (eng, (const char *) NULL, &p);
			stab_pop_type (eng, ntype);
			stab_top_symbol (eng, symbol, 0);
			if ((symbol != NULL) && (ntype != NULL))
			{
				symbol->type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 0, ntype, NULL, NULL, eng->stab->comp_unit);
				symbol->class = nel_C_COMPILED_FUNCTION;
				symbol->value = (char *)(long ) stab_get_value (eng->stab->last_sym);

				/********************************/
				/* don't insert funciton -      */
				/* we don't know how to call it */
				/********************************/

				/************************************************/
				/* call nel_stab_start_routine to clean up after */
				/* the previous routine and start forming the   */
				/* block strucutre for the new routine.         */
				/************************************************/
				stab_start_routine (eng, symbol);
			}
		}
		break;

	case 'k':
		/**************************************************/
		/* valid only on ALLIANT_FX2800.                  */
		/* value parameter of type type_desc              */
		/* (hence the "1" in the arg list to              */
		/* nel_stab_parameter_acts().                      */
		/**************************************************/
		{
			register nel_symbol *symbol;
			parse_stab_type (eng, (const char *) NULL, &p);
			stab_pop_type (eng, ntype);
			stab_pop_symbol (eng, symbol);
			stab_parameter_acts (eng, symbol, ntype, (char *)(long ) stab_get_value (eng->stab->last_sym), 0);
		}
		break;

	case 'p':
		/*************************************/
		/* value parameter of type type_desc */
		/* (hence the "1" in the arg list to */
		/* nel_stab_parameter_acts().         */
		/*************************************/
		{
			register nel_symbol *symbol;
			parse_stab_type_ex (eng, (const char *) NULL, &p);

			stab_pop_type (eng, ntype);
			stab_pop_symbol (eng, symbol);
			stab_parameter_acts (eng, symbol, ntype, (char *)(long ) stab_get_value (eng->stab->last_sym), 0);
		}
		break;

	case 'P':
		/********************/
		/* global procedure */
		/********************/
		{
			register nel_symbol *symbol;
			stab_pop_symbol (eng, symbol);
			stab_routine_acts (eng, symbol, nel_void_type);
		}

#if 0 
		/* Fall through.  */
	case 'R':
		/* Parameter which is in a register.  */
		dtype = parse_stab_type (dhandle, info, (const char *) NULL, &p,
								 (debug_type **) NULL);
		if (dtype == DEBUG_TYPE_NULL)
			return FALSE;
		if (! debug_record_parameter (dhandle, name, dtype, DEBUG_PARM_REG,
									  value))
			return FALSE;
#endif

		break;

	case 'Q':
		/*******************/
		/* local procedure */
		/*******************/
		{
			register nel_symbol *symbol;
			stab_pop_symbol (eng, symbol);
			stab_static_routine_acts (eng, symbol, nel_void_type);
		}
		break;

	case 'r':
		/****************************************************************/
		/* register variable (either global or local) of type type_desc */
		/****************************************************************/
		{
			register nel_symbol *symbol;
			parse_stab_type_ex (eng, (const char *) NULL, &p);
			stab_pop_type (eng, ntype);
			stab_pop_symbol (eng, symbol);
			stab_register_local_acts (eng, symbol, ntype);
		}
		break;

	case 'S':
		/**********************************************************/
		/* module variable of type type_desc (static global in C) */
		/* this rule must appear before the rule                  */
		/* type_def->'S' type_desc                                */
		/**********************************************************/
		{
			register nel_symbol *symbol;
			parse_stab_type_ex (eng, (const char *) NULL, &p);

			stab_pop_type (eng, ntype);
			stab_pop_symbol (eng, symbol);
			stab_static_global_acts (eng, symbol, ntype);
		}
		break;

	case 't':
		/******************************/
		/* type name for type type_id */
		/******************************/
		{
			nel_stab_type *stab_type;
			nel_symbol *symbol;

			parse_stab_type (eng, (const char *) NULL, &p);

			stab_pop_stab_type (eng, stab_type);
			stab_pop_symbol (eng, symbol);
			nel_debug ({
						   if ((stab_type == NULL) || (stab_type->type == NULL)) {
						   stab_fatal_error (eng, "stab #%d (named_type->'t' type_id #1): bad stab_type\n%1U", eng->stab->sym_ct, stab_type)
							   ;
						   }
					   });
			ntype = stab_type->type;
			if ((symbol != NULL) && (symbol->name != NULL))
			{
				register nel_symbol *old_sym;
				old_sym = stab_lookup_ident (eng, symbol->name);
				nel_debug ({
					stab_trace (eng, "old_sym =\n%1S\n", old_sym);
						   });
				if ((old_sym != NULL) && (old_sym->type != NULL) && (old_sym->type->simple.type == nel_D_TYPE_NAME)) {
					/************************************************/
					/* if this is a predefined type, overwrite the  */
					/* type field of the nel_stab_type structure if  */
					/* the stab_type was previously undefined.      */
					/* otherwise, we have one predefined type that  */
					/* is subrange of the other (char is a subrange */
					/* of int in some stabs), so create a new       */
					/* nel_stab_type, and insert it in the           */
					/* nel_stab_hash table (possibly hiding a        */
					/* previous entry by the same file/type num).   */
					/************************************************/
					if (stab_type->type->simple.type == nel_D_STAB_UNDEF) {
						nel_copy (sizeof (nel_type), stab_type->type, old_sym->type->typedef_name.descriptor);
						/****************************************/
						/* there are no tagged predefined types */
						/* - don't worry about copying a tag.   */
						/****************************************/
					} else {
						register int file_num = stab_type->file_num;
						register int type_num = stab_type->type_num;
						stab_type = stab_type_alloc (eng, old_sym->type->typedef_name.descriptor, file_num, type_num);
						stab_insert_type_2 (eng, stab_type);
					}
					nel_debug ({
								   stab_trace (eng, "predefined type - nel_stab_type_hash entry =\n%1U\n", stab_type);
							   });
				} else if (stab_in_routine (eng)) {
					/*************************************/
					/* we have a local type - include it */
					/* with the list of local variables  */
					/* after setting appropriate fields  */
					/*************************************/
					register nel_list *list;
					symbol->type = nel_type_alloc (eng, nel_D_TYPEDEF_NAME, 0, 0, 0, 0, ntype);
					symbol->class = nel_C_TYPE;
					symbol->value = (char *) ntype;
					symbol->level = eng->stab->level + 1;

					if ((old_sym != NULL) && (old_sym->level == symbol->level)) {
						/******************************************/
						/* check to make sure that this defintion */
						/* is equivalent to earlier ones.         */
						/* the symbol must be a typedef, so its   */
						/* value field should point to the type   */
						/* descriptor (which is not a typedef     */
						/* descriptor like the type field is.)    */
						/******************************************/
						nel_debug ({
									   if (old_sym->type == NULL) {
									   stab_fatal_error (eng, "stab #%d (named_type->'t' type_id #2): bad symbol\n%1S", eng->stab->sym_ct, old_sym)
										   ;
									   }
								   });
						if (nel_stab_check_redecs && ((old_sym->type->simple.type != nel_D_TYPEDEF_NAME)
													  || (nel_type_diff (old_sym->type->typedef_name.descriptor, ntype, 1) == 1))) {
							stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, symbol);
						}
					} else {
						stab_insert_ident (eng, symbol);
						list = nel_list_alloc (eng, 0, symbol, NULL);
						if (eng->stab->last_local == NULL) {
							eng->stab->first_local = eng->stab->last_local = list;
						} else {
							eng->stab->last_local->next = list;
							eng->stab->last_local = list;
						}
					}
				} else {
					if (old_sym != NULL) {
						/******************************************/
						/* check to make sure that this defintion */
						/* is equivalent to earlier ones.         */
						/* the symbol must be a typedef, so its   */
						/* value field should point to the type   */
						/* descriptor (which is not a typedef     */
						/* descriptor like the type field is.)    */
						/******************************************/
						nel_debug ({
									   if (old_sym->type == NULL) {
									   stab_fatal_error (eng, "stab #%d (named_type->'t' type_id #3): bad symbol\n%1S", eng->stab->sym_ct, old_sym)
										   ;
									   }
								   });
						if (nel_stab_check_redecs && ((old_sym->type->simple.type != nel_D_TYPEDEF_NAME)
													  || (nel_type_diff (old_sym->type->typedef_name.descriptor, ntype, 1) == 1))) {
							stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, symbol);
						}
					} else {
						/*********************************************/
						/* this is the first time a globally-defined */
						/* type is being defined - insert the new    */
						/* type definition in the ident table        */
						/*********************************************/
						symbol->type = nel_type_alloc (eng, nel_D_TYPEDEF_NAME, 0, 0, 0, 0, ntype);
						symbol->class = nel_C_TYPE;
						symbol->value = (char *) ntype;
#if    defined (SPARC) || defined (SPARC_GCC)

						symbol->level = 0;  /* just in case */
#endif /* defined (SPARC) || defined (SPARC_GCC) */

						stab_insert_ident (eng, symbol);
					}
				}
			}
		}
		break;

	case 'T':
		/*******************************************/
		/* C structure tag name for struct type_id */
		/*******************************************/
		{
			nel_stab_type *stab_type;
			register nel_symbol *symbol;
			parse_stab_type (eng, (const char *) NULL, &p);

			stab_pop_stab_type (eng, stab_type);
			stab_pop_symbol (eng, symbol);
			nel_debug ({
						   if ((stab_type == NULL) || (stab_type->type == NULL)) {
						   stab_fatal_error (eng, "stab #%d (named_type->'T' type_id #1): bad stab_type\n%1U", eng->stab->sym_ct, stab_type)
							   ;
						   }
					   });
			ntype = stab_type->type;
			if ((symbol != NULL) && (symbol->name != NULL))
			{
				register nel_symbol *old_sym;
				old_sym = stab_lookup_tag (eng, symbol->name);
				nel_debug ({
							   stab_trace (eng, "old_sym =\n%1S\n", old_sym);
						   });
				if ((old_sym == NULL) || (stab_in_routine (eng) && (old_sym->level != eng->stab->level + 1))) {
					/********************************/
					/* we have a newly-defined type */
					/********************************/
					symbol->value = (char *) (ntype);
					switch (ntype->simple.type) {
					case nel_D_STRUCT:
						symbol->type = nel_type_alloc (eng, nel_D_STRUCT_TAG, 0, 0, 0, 0, ntype);
						ntype->s_u.tag = symbol;
						symbol->class = nel_C_TYPE;
						symbol->value = (char *) ntype;
						break;
					case nel_D_UNION:
						symbol->type = nel_type_alloc (eng, nel_D_UNION_TAG, 0, 0, 0, 0, ntype);
						symbol->class = nel_C_TYPE;
						ntype->s_u.tag = symbol;
						symbol->value = (char *) ntype;
						break;
					case nel_D_ENUM:
						symbol->type = nel_type_alloc (eng, nel_D_ENUM_TAG, 0, 0, 0, 0, ntype);
						ntype->s_u.tag = symbol;
						symbol->class = nel_C_TYPE;
						symbol->value = (char *) ntype;
						break;
					default:
						stab_error (eng, "stab #%d: struct/union/enum expected: %I", eng->stab->sym_ct, symbol);
						break;
					}
					if (stab_in_routine (eng)) {
						/*************************************/
						/* we have a local type - include it */
						/* with the list of local variables  */
						/*************************************/
						register nel_list *list;
						symbol->level = eng->stab->level + 1;
						stab_insert_tag (eng, symbol);
						list = nel_list_alloc (eng, 0, symbol, NULL);
						if (eng->stab->last_local == NULL) {
							eng->stab->first_local = eng->stab->last_local = list;
						} else {
							eng->stab->last_local->next = list;
							eng->stab->last_local = list;
						}
					} else {
						/*********************************************/
						/* this is the first time a globally-defined */
						/* type is being defined - insert the new    */
						/* type definition in the tag table          */
						/*********************************************/
						symbol->level = 0;  /* just in case */
						stab_insert_tag (eng, symbol);
					}
				} else {  /* redeclaration of type */
					/******************************************/
					/* check to make sure that this defintion */
					/* is equivalent to earlier ones.         */
					/* we could have an incomplete type that  */
					/* is finally being defined.  if so, set  */
					/* the members field.  note that there    */
					/* will be a dummy list element at the    */
					/* start of an incomplete struct/union's  */
					/* member list, or an enum's const list.  */
					/* copy the first list element over the   */
					/* dummy, so that the members or const    */
					/* list of this type descriptor and all   */
					/* copies of it get set.                  */
					/******************************************/
					register nel_type *old_type = old_sym->type;
					nel_debug ({
								   if (old_type == NULL) {
								   stab_fatal_error (eng, "stab #%d: (named_type->'T' type_id #2): old_type == NULL", eng->stab->sym_ct)
									   ;
								   }
							   });
					switch (ntype->simple.type) {

					case nel_D_STRUCT:
						symbol->type = nel_type_alloc (eng, nel_D_STRUCT_TAG, 0, 0, 0, 0, ntype);
						ntype->s_u.tag = symbol;
						symbol->class = nel_C_TYPE;
						symbol->value = (char *) ntype;
						if (old_type->simple.type != nel_D_STRUCT_TAG) {
							stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, symbol);
						} else {
							old_type = old_type->tag_name.descriptor;
							nel_debug ({
										   if ((old_type == NULL) || (old_type->simple.type != nel_D_STRUCT)) {
										   stab_fatal_error (eng, "stab #%d: (named_type->'T' type_id #3): bad old_type\n%1T", eng->stab->sym_ct, old_type)
											   ;
										   }
									   });
							if (ntype->s_u.members != NULL) {
								if (old_type->s_u.members == NULL) {
									/******************************/
									/* we have an incomplete type */
									/* that is becoming defined.  */
									/* (but no dummy in list)     */
									/******************************/
									old_type->s_u.members = ntype->s_u.members;
									old_type->s_u.size = ntype->s_u.size;
									old_type->s_u.alignment = ntype->s_u.alignment;
								} else if ((old_type->s_u.members->symbol == NULL) && (old_type->s_u.members->next == NULL)) {
									/******************************/
									/* we have an incomplete type */
									/* that is becoming defined.  */
									/* copy the first member over */
									/* the dummy element.         */
									/******************************/
									nel_copy (sizeof (nel_member), old_type->s_u.members, ntype->s_u.members);
									old_type->s_u.size = ntype->s_u.size;
									old_type->s_u.alignment = ntype->s_u.alignment;
								} else if (nel_stab_check_redecs && (nel_type_diff (ntype, old_type, 1) == 1)) {
									stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, symbol);
								}
							}
						}
						break;

					case nel_D_UNION:
						symbol->type = nel_type_alloc (eng, nel_D_UNION_TAG, 0, 0, 0, 0, ntype);
						ntype->s_u.tag = symbol;
						symbol->class = nel_C_TYPE;
						symbol->value = (char *) ntype;
						if (old_type->simple.type != nel_D_UNION_TAG) {
							stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, symbol);
						} else {
							old_type = old_type->tag_name.descriptor;
							nel_debug ({
										   if ((old_type == NULL) || (old_type->simple.type != nel_D_UNION)) {
										   stab_fatal_error (eng, "stab #%d: (named_type->'T' type_id #4): bad old_type\n%1T", eng->stab->sym_ct, old_type)
											   ;
										   }
									   });
							if (ntype->s_u.members != NULL) {
								if (old_type->s_u.members == NULL) {
									/******************************/
									/* we have an incomplete type */
									/* that is becoming defined.  */
									/* (but no dummy in list)     */
									/******************************/
									old_type->s_u.members = ntype->s_u.members;
									old_type->s_u.size = ntype->s_u.size;
								} else if ((old_type->s_u.members->symbol == NULL) && (old_type->s_u.members->next == NULL)) {
									/******************************/
									/* we have an incomplete type */
									/* that is becoming defined.  */
									/* copy the first member over */
									/* the dummy element.         */
									/******************************/
									nel_copy (sizeof (nel_member), old_type->s_u.members, ntype->s_u.members);
									old_type->s_u.size = ntype->s_u.size;
								}
							}
						}
						break;

					case nel_D_ENUM:
						symbol->type = nel_type_alloc (eng, nel_D_ENUM_TAG, 0, 0, 0, 0, ntype);
						ntype->enumed.tag = symbol;
						symbol->class = nel_C_TYPE;
						symbol->value = (char *) ntype;
						if (old_type->simple.type != nel_D_ENUM_TAG) {
							stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, symbol);
						} else {
							old_type = old_type->tag_name.descriptor;
							nel_debug ({
										   if ((old_type == NULL) || (old_type->simple.type != nel_D_ENUM)) {
										   stab_fatal_error (eng, "stab #%d: (named_type->'T' type_id #5): bad old_type\n%1T", eng->stab->sym_ct, old_type)
											   ;
										   }
									   });
							if (ntype->enumed.consts != NULL) {
								if (old_type->enumed.consts == NULL) {
									/******************************/
									/* we have an incomplete type */
									/* that is becoming defined.  */
									/* (but no dummy in list)     */
									/******************************/
									old_type->enumed.consts = ntype->enumed.consts;
									old_type->enumed.nconsts = ntype->enumed.nconsts;
								} else if ((old_type->enumed.consts->symbol == NULL) && (old_type->enumed.consts->next == NULL)) {
									/******************************/
									/* we have an incomplete type */
									/* that is becoming defined.  */
									/* copy the first const over  */
									/* the dummy element.         */
									/******************************/
									nel_copy (sizeof (nel_list), old_type->enumed.consts, ntype->enumed.consts);
									old_type->enumed.nconsts = ntype->enumed.nconsts;
								}
							}
						}
						break;

					default:
						stab_stmt_error (eng, "stab #%d: struct/union/enum expected: %I", eng->stab->sym_ct, symbol);
						break;
					}
				}
			}
		}
		break;

	case 'V':
		/******************************************************/
		/* own variable of type type_desc (static local in C) */
		/* (could be in a common block)                       */
		/* on ALLIANT_FX2800, can also be a static global.    */
		/******************************************************/
		{
			register nel_symbol *symbol;
			parse_stab_type_ex (eng, (const char *) NULL, &p);

			stab_pop_type (eng, ntype);
			stab_pop_symbol (eng, symbol);
			if (eng->stab->prog_unit == NULL)
			{
				stab_static_global_acts (eng, symbol, ntype);
			} else
			{
				stab_static_local_acts (eng, symbol, ntype);
			}
		}
		break;

	case 'v':
		/*****************************************/
		/* reference parameter of type type_desc */
		/* (hence the "1" in the arg list to     */
		/* nel_stab_parameter_acts().             */
		/*****************************************/
		{
			register nel_type *type;
			register nel_symbol *symbol;
			parse_stab_type (eng, (const char *) NULL, &p);

			stab_pop_type (eng, type);
			stab_pop_symbol (eng, symbol);
			stab_parameter_acts (eng, symbol, type, (char *)(long )stab_get_value (eng->stab->last_sym), 1);
		}
		break;

	case 'X':
#if 0 
		/* This is used by Sun FORTRAN for "function result value".
		Sun claims ("dbx and dbxtool interfaces", 2nd ed)
		that Pascal uses it too, but when I tried it Pascal used
		"x:3" (local symbol) instead.  */
		dtype = parse_stab_type (dhandle, info, (const char *) NULL, &p,
								 (debug_type **) NULL);
		if (dtype == DEBUG_TYPE_NULL)
			return FALSE;
		if (! stab_record_variable (dhandle, info, name, dtype, DEBUG_LOCAL,
									value))
			return FALSE;
#endif
		//not sure whether should be pasted here....."class->'X' export_info".
		/****************************************************/
		/* export or import information - (for N_MOD2 only) */
		/****************************************************/
		{
			/*************************/
			/* skip to end of string */
			/*************************/
			register nel_symbol *symbol;
			stab_pop_symbol (eng, symbol);
			stab_stmt_error (eng, NULL);
		}
		break;

	case '-':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case '(': {
			nel_type *type;
			--p;
			parse_stab_type_ex (eng, (const char *) NULL, &p);

			stab_pop_type (eng, type);
			stab_pop_symbol (eng, symbol);
			stab_local_acts (eng, symbol, type);
		}
		break;

	default:
		nel_debug({stab_trace (eng, "bad stab: %s\n", string);});
		return -1;
	}

	/* FIXME: gdb converts structure values to structure pointers in a
	   couple of cases, depending upon the target.  */

	return 0;
}


/*****************************************************************************/
/* stab_dyn_value_alloc () allocates a space of <bytes> bytes in the dynamic   */
/* value table on an alignment-byte boundary, and returns a pointer to it.   */
/*****************************************************************************/
char *stab_dyn_value_alloc (struct nel_eng *eng, register unsigned_int bytes, register unsigned_int alignment)
{
	register char *retval;

	nel_debug ({ stab_trace (eng, "entering stab_dyn_value_alloc [\neng = 0x%x\nbytes = 0x%x\nalignment = 0x%x\n\n", eng, bytes, alignment); });

	nel_debug ({
	if (bytes == 0) {
		stab_fatal_error (eng, "(stab_dyn_value_alloc #1): bytes = 0") ;
	}
	if (alignment == 0) {
		stab_fatal_error (eng,"(stab_dyn_value_alloc #2): alignment=0");
	}
	});

	/*****************************************/
	/* we do not want memory fetches for two */
	/* different objects overlapping in the  */
	/* same word if the machine does not     */
	/* support byte-size loads and stores.   */
	/*****************************************/
	eng->stab->dyn_values_next = nel_align_ptr (eng->stab->dyn_values_next, nel_SMALLEST_MEM);

	/******************************************************/
	/* get a pointer to the start of the allocated space. */
	/* make sure it is on an properly-aligned boundary.   */
	/******************************************************/
	retval = (char *) eng->stab->dyn_values_next;
	retval = nel_align_ptr (retval, alignment);

	if (retval + bytes > eng->stab->dyn_values_end) {
		/***********************/
		/* longjmp on overflow */
		/***********************/
		stab_stmt_error (eng, "dynamic value table overflow");
		retval = NULL;
	} else {
		/*********************/
		/* reserve the space */
		/*********************/
		eng->stab->dyn_values_next = retval + bytes;
	}

	/*************************************************/
	/* zero out all elements of the allocated space  */
	/*************************************************/
	nel_zero (bytes, retval);
	nel_debug ( {
		stab_trace (eng, "] exiting stab_dyn_value_alloc\nretval = 0x%x\n\n", retval);
	});

	return (retval);

}

int stab_parse(struct nel_eng *eng)
{
	int val;


	/**************************************************/
	/* save the dynamic value table index, so that we */
	/* may clear it before actually interpreting any  */
	/* code.                                          */
	/**************************************************/
	stab_push_value (eng, eng->stab->dyn_values_next, char *);

	/******************************************************/
	/* allocate space for the default error nel_jmp_buf,   */
	/* and perfrom a setjmp on it.  check the return code */
	/* to see if we are dynamically returning here from   */
	/* an error.  the dynamic table indeces are saved     */
	/* after this, so it is safe to allocate the jmp_buf  */
	/* dynamically, and it will not be deallocated until  */
	/* nel_stab () returns.                                */
	/******************************************************/
	eng->stab->stmt_err_jmp = (nel_jmp_buf *) stab_dyn_value_alloc (eng, sizeof (nel_jmp_buf), nel_MAX_ALIGNMENT);

	stab_setjmp (eng, val, eng->stab->stmt_err_jmp);

	while (1) {

		/**************************************************************/
		/* jump to the next string.                                   */
		/* reset eng->stab->current_symbol to NULL for error messages. */
		/**************************************************************/
		eng->stab->sym_ct++;
		eng->stab->sym_scan++;
		eng->stab->current_symbol = NULL;

		//printf("stab_parse, process sym (%d of %d)\n", eng->stab->sym_ct, eng->stab->sym_size );

		if (eng->stab->sym_ct > eng->stab->sym_size) {
			/**********************/
			/* return 0 when done */
			/**********************/

			/******************************************************/
			/* restore the dynamic value table index to clear the */
			/* table.  we do not use any other dynamic tables in  */
			/* the symbol table parser.                           */
			/******************************************************/
			stab_pop_value (eng, eng->stab->dyn_values_next, char *);

			return 0;
		}

		/***********************************************/
		/* nel_tracing should only be set for stab #'s */
		/* between nel_stab_tstart and nel_stab_tend.  */
		/***********************************************/
		//nel_debug ({
		//	if (eng->stab->sym_ct >= nel_stab_tstart) {
		//		nel_tracing = nel_stab_old_tracing;
		//	}
		//	if (eng->stab->sym_ct > nel_stab_tend) {
		//		nel_tracing = 0;
		//	}
		//});

		/************************************************************/
		/* print out the stab # every 100'th time through the loop. */
		/* call stab_overwritable () so that the message is written  */
		/* over by any error messages or trace messages.            */
		/* we must also print out stab #1 and the stab # after      */
		/* any new error messages.  the screen is a sequential      */
		/* device anyway, so don't worry about old_error_ct not     */
		/* being a member of struct nel_eng.                          */
		/************************************************************/
		{
			static int old_error_ct;
			if ((eng->stab->sym_ct == 1)
					|| (eng->stab->sym_ct % 100 == 0)
					|| (old_error_ct != eng->stab->error_ct))
			{
				stab_overwritable (eng, "stab #%d", eng->stab->sym_ct);
			}
			old_error_ct = eng->stab->error_ct;
		}

		/******************************************************************/
		/* if we are starting to read a new symbol table entry            */
		/* (not a continuation of an old one), update eng->stab->last_sym. */
		/******************************************************************/
		eng->stab->last_sym = eng->stab->sym_scan;

		/****************************************/
		/* check to make sure we are interested */
		/* in this kind of symbol               */
		/****************************************/
		switch (stab_get_type (eng->stab->sym_scan)) {

			/***********************************/
			/* variables and type declarations */
			/* break out of the case stmt, and */
			/* read in the string - return     */
			/* characters from it.             */
			/***********************************/
		case N_GSYM:	/* global symbol: name,,0,type,0 */
		case N_STSYM:	/* static symbol: name,,0,type,address */
		case N_LCSYM:	/* .lcomm symbol: name,,0,type,address */
		case N_RSYM:	/* register sym: name,,0,type,register */
		case N_LSYM:	/* local sym: name,,0,type,offset */
		case N_PSYM:	/* parameter: name,,0,type,offset */
		case N_PC:	/* global pascal symbol: name,,0,subtype,line */

			/************************************/
			/* check to make sure there is a    */
			/* string associated with this stab */
			/************************************/
			if (stab_get_name (eng->stab->sym_scan) == 0) {
				stab_error (eng, "stab #%d: NULL string", eng->stab->sym_ct);
				//return -1;
			}

			/******************************************************/
			/* set eng->stab->str_scan to the start of the string. */
			/******************************************************/
			eng->stab->str_scan = eng->stab->str_tab + stab_get_name (eng->stab->sym_scan);

			/*************************************************/
			/* we have a valid string that we want to parse. */
			/*************************************************/
			nel_debug ({
				stab_trace (eng, "stab #%d: type = 0x%x value = 0x%x\n%s\n\n",
						eng->stab->sym_ct, 
						stab_get_type (eng->stab->sym_scan), 
						stab_get_value (eng->stab->sym_scan),
						eng->stab->str_tab + stab_get_name(eng->stab->sym_scan));
			});

			//printf("stab_parse, process sym(#%d): name =%s, type = 0x%x value = 0x%x\n",
			//		eng->stab->sym_ct, 
			//		eng->stab->str_tab + stab_get_name(eng->stab->sym_scan),
			//		stab_get_type (eng->stab->sym_scan), 
			//		stab_get_value (eng->stab->sym_scan));

			parse_stab_string(eng);
			break;


		case N_LBRAC:	/* left bracket: 0,,0,nesting level,address */
			nel_debug ({
						   stab_trace (eng, "left bracket: stab #%d address = 0x%x\n\n",
									  eng->stab->sym_ct, stab_get_value (eng->stab->sym_scan));
					   });
			{
				eng->stab->level++;
				stab_push_integer (eng, eng->stab->block_no);
				eng->stab->block_no = eng->stab->block_ct++;

				/*******************************************************/
				/* save the queues of symbols allocated 1 level out.   */
				/* (or for SPARC/SPARC_GCC, for symbols at this level) */
				/* start new queues of symbols allocated at this level */
				/* (or for SPARC/SPARC_GCC, for symbols in an inner    */
				/* scope) these are the queues that become the local   */
				/* var list for the type descriptor for the function.  */
				/*******************************************************/
				stab_push_list (eng, eng->stab->first_local);
				stab_push_list (eng, eng->stab->last_local);
				eng->stab->first_local = NULL;
				eng->stab->last_local = NULL;

				/************************************************/
				/* save the block list, and start new one.      */
				/* push the address of the start of the block,  */
				/* but not the end - the end of the outer block */
				/* has at least as high of address as the end   */
				/* as the inner block.                          */
				/************************************************/
				stab_push_value (eng, eng->stab->block_start_address, char *);
				stab_push_block (eng, eng->stab->first_block);
				stab_push_block (eng, eng->stab->last_block);
				eng->stab->first_block = NULL;
				eng->stab->last_block = NULL;
				eng->stab->block_start_address = (char *)(long ) stab_get_value (eng->stab->last_sym);
			}

			break;

		case N_RBRAC:	/* right bracket: 0,,0,nesting level,address */
			nel_debug ({
						   stab_trace (eng, "right bracket: stab #%d address = 0x%x\n\n",
									  eng->stab->sym_ct, stab_get_value (eng->stab->sym_scan));
					   });
			{
				/***************************************************/
				/* create a new nel_block structure for this block, */
				/* set the start address and end addresses, and    */
				/* add the list of nested blocks to it.            */
				/***************************************************/
				{
					nel_block *block;

					//the semantic stack was accidentally reseted.
					if (eng->stab->semantic_stack_next == eng->stab->semantic_stack_start) {
						break;
					}

					block = nel_block_alloc (eng, eng->stab->block_no, NULL, NULL, NULL, eng->stab->first_block, NULL);
					block->start_address = eng->stab->block_start_address;
					block->end_address = (char *)(long)stab_get_value (eng->stab->last_sym);
					stab_pop_block (eng, eng->stab->last_block);
					stab_pop_block (eng, eng->stab->first_block);

					/************************************/
					/* append the new block to the list */
					/* of blocks at this scoping level. */
					/************************************/
					if (eng->stab->last_block == NULL)
					{
						eng->stab->first_block = eng->stab->last_block = block;

#if defined (SPARC) || defined (SPARC_GCC)

						if (eng->stab->level == 1) {
							/*******************************************/
							/* this is main block - the function body. */
							/* SPARCs have no special stab entry for   */
							/* end of procedure.                       */
							/*******************************************/
							if (eng->stab->prog_unit == NULL) {
								stab_error (eng, "stab #%d: block not local to program unit", eng->stab->sym_ct);
							} else {
								register nel_type *type = eng->stab->prog_unit->type;
								nel_debug ({
											   if ((type == NULL) || (type->simple.type != nel_D_FUNCTION)) {
											   stab_fatal_error (eng, "stab #%d (block->'{' {} entry_list '}' #1): bad eng->stab->prog_unit\n%1S", eng->stab->sym_ct, eng->stab->prog_unit)
												   ;
											   }
										   });
								type->function.blocks = block;
							}
						}

#endif /* defined (SPARC) || defined (SPARC_GCC) */

					} else
					{
						eng->stab->last_block->next = block;
						eng->stab->last_block = block;
					}

					/**************************************************/
					/* restore the address of the start of the block. */
					/* (the end address is at least as high as the    */
					/* address so far)                                */
					/**************************************************/
					stab_pop_value (eng, eng->stab->block_start_address, char *);
				}

#if defined (SPARC) || defined (SPARC_GCC)

				/****************************************************/
				/* on SPARC/SPARC_GCC ,                             */
				/* since local vars appear before the left bracket  */
				/* at the start of the block, they were pushed, not */
				/* the list of vars in the outer scope.  pop the    */
				/* list, and attach it to the new block, remove any */
				/* symbols that are still in a hash table from the  */
				/* table, and start a new list.  for SPARCS, pop    */
				/* the lists, THEN use them (then reset them).      */
				/****************************************************/
				stab_pop_list (eng, eng->stab->last_local);
				stab_pop_list (eng, eng->stab->first_local);

				eng->stab->last_block->locals = eng->stab->first_local;

				/*****************************************************/
				/* purge the dynamic tables of symbols at this level */
				/*****************************************************/
				nel_purge_table (eng, eng->stab->level, eng->stab->dyn_ident_hash);
				nel_purge_table (eng, eng->stab->level, eng->stab->dyn_tag_hash);
				nel_purge_table (eng, eng->stab->level, eng->stab->dyn_label_hash);

				/**********************************/
				/* start a new list of vars local */
				/* to a block, for the next block */
				/**********************************/
				eng->stab->first_local = NULL;
				eng->stab->last_local = NULL;

#else  /* not a SPARC */

				/*******************************************************/
				/* on other architectures, the local appear in the     */
				/* block itself, as they should.  so attach the list   */
				/* of locals to the new block, remove any symbols that */
				/* are still in a hash table from the table, and pop   */
				/* the list for the outer scope.  for non-SPARCS, use  */
				/* the lists, THEN pop the old ones.                   */
				/*******************************************************/
				eng->stab->last_block->locals = eng->stab->first_local;

				/*****************************************************/
				/* purge the dynamic tables of symbols at this level */
				/*****************************************************/
				nel_purge_table (eng, eng->stab->level, eng->stab->dyn_ident_hash);
				nel_purge_table (eng, eng->stab->level, eng->stab->dyn_tag_hash);
				nel_purge_table (eng, eng->stab->level, eng->stab->dyn_label_hash);

				/*************************************************/
				/* restore the lists of vars local to this block */
				/*************************************************/
				stab_pop_list (eng, eng->stab->last_local);
				stab_pop_list (eng, eng->stab->first_local);

#endif /* defined (SPARC) || defined (SPARC_GCC) */

				/**************************************************/
				/* restore the old scoping level and block number */
				/**************************************************/
				eng->stab->level--;
				stab_pop_integer (eng, eng->stab->block_no);
				if (eng->stab->level == 0) {
					eng->stab->prog_unit = NULL;
				}
			}
			break;

		case N_SO:
			if (stab_get_name (eng->stab->sym_scan) == 0) {
	                        eng->stab->level = 0;

				//stab_block_error (eng, "stab #%d: NULL source file name", eng->stab->sym_ct);
			} else if (eng->stab->level != 0) {
				stab_block_error (eng, "stab #%d: improper block nesting", eng->stab->sym_ct);
			} else {
				char *filename = eng->stab->str_tab + stab_get_name (eng->stab->sym_scan);

				/**********************************************/
				/* append an nel_line structure with address 0 */
				/* to the previous list of line # / address   */
				/* pairs as a dummy terminator.               */
				/**********************************************/
				if (eng->stab->last_line != NULL) {
					*eng->stab->last_line = nel_line_alloc (eng, 0, NULL, NULL);
				}

				/*****************************************************/
				/* if this is only the directory where the file will */
				/* be found, start over on the next string, which    */
				/* should be the file name.                          */
				/*****************************************************/
				if (filename[strlen (filename) - 1] == '/') {
					nel_debug ({
						stab_trace (eng, "source file directory: stab #%d: type = 0x%x\n%s\n\n", eng->stab->sym_ct, stab_get_type (eng->stab->sym_scan), filename);
					});
					break;
				}
				nel_debug ({
					stab_trace (eng, "source file: stab #%d: type = 0x%x\n%s\n\n",
					eng->stab->sym_ct, stab_get_type (eng->stab->sym_scan), filename);
				});

				/******************************************************/
				/* print out the name of the new source file, so that */
				/* if an error occurrs in an header file, we know     */
				/* from which source file it was #included.  this     */
				/* overwrite the stab count, so print it out again.   */
				/******************************************************/
				stab_diagnostic (eng, "%s:", filename);
				stab_overwritable (eng, "stab #%d", eng->stab->sym_ct);

				/************************************************/
				/* clean up after the previous compilation unit */
				/************************************************/

				/*****************************************************/
				/* if there was no body for the last routine in the  */
				/* previous file, we need to set its block list, and */
				/* clean up after it.                                */
				/*****************************************************/
				if (eng->stab->prog_unit != NULL) {
					stab_end_routine (eng);
				}

				/*******************************************************/
				/* purge the dynamic tables of all symbols - static    */
				/* global symbols wil still be inserted in the dynamic */
				/* hash tables at level 0.                             */
				/*******************************************************/
				nel_purge_table (eng, 0, eng->stab->dyn_ident_hash);
				nel_purge_table (eng, 0, eng->stab->dyn_tag_hash);
				nel_purge_table (eng, 0, eng->stab->dyn_label_hash);

				/******************************************************/
				/* eng->stab->incl_stack should either be NULL (first  */
				/* compilation unit), or else it should have exactly  */
				/* one entry (previous source file). emit an error    */
				/* message if we did not properly exit from all of    */
				/* the header files in the previous compilation unit. */
				/******************************************************/
				if (eng->stab->incl_stack != NULL) {
					nel_list *prev;
#if 0

					if (eng->stab->incl_stack->next != NULL) {
						stab_error (eng, "stab #%d: bad #include file nesting", eng->stab->sym_ct);
						//return -1;
					}
#endif
					prev = NULL;
					while (eng->stab->incl_stack != NULL) {
						prev = eng->stab->incl_stack;
						eng->stab->incl_stack = eng->stab->incl_stack->next;
						nel_list_dealloc (prev);
					}
				}

				/*****************************/
				/* clear the stab hash table */
				/*****************************/
				stab_clear_type_hash (eng->stab->type_hash);
				
				/****************************************************/
				/* remove the symbols for the predefined type names */
				/* in the previous compilation from the ident hash  */
				/* tables.  it is a good idea to do this, even if   */
				/* we are just going to reinsert them (when the     */
				/* source language of this file is the same as the  */
				/* previous one), in case a symbol by the same name */
				/* as a type was inserted, masking the type symbol. */
				/* this can't really happen with a good compiler,   */
				/* but it is possible in C to define global symbols */
				/* with fortran type names, such as "integer", so   */
				/* we must reinsert the type names when changing    */
				/* source code languages.                           */
				/****************************************************/
				if (eng->stab->C_types_inserted) {
					nel_remove_C_types (eng);
					eng->stab->C_types_inserted = 0;
				}
				
				/*
				if (eng->stab->fortran_types_inserted) {
					nel_remove_fortran_types (eng);
					eng->stab->fortran_types_inserted = 0;
				}
				*/

				/**************************************************/
				/* now, set stuff up for the new compilation unit */
				/**************************************************/
				{
					/****************************************************/
					/* first determine the language of the source file  */
					/* based upon its suffix, and insert the predefined */
					/* type names for the source language in the ident  */
					/* hash tables.                                     */
					/****************************************************/
					register int len = strlen (filename);
					register char suffix;
					if ((len >= 2) && (filename[len - 2] == '.')) {
						suffix = filename[len - 1];
					} else {
						suffix = '\0';
					}
					if (suffix == 's') {
						//nel_insert_fortran_types (eng);
						//eng->stab->fortran_types_inserted = 1;
						nel_insert_C_types (eng);
						eng->stab->C_types_inserted = 1;
						eng->stab->source_lang = nel_L_ASSEMBLY;
					}
					else if (suffix == 'c') {
						nel_insert_C_types (eng);
						eng->stab->C_types_inserted = 1;
						eng->stab->source_lang = nel_L_C;
					} 
					/* else if (suffix == 'f') {
						nel_insert_fortran_types (eng);
						eng->stab->fortran_types_inserted = 1;
						eng->stab->source_lang = nel_L_FORTRAN;
					}*/
					else {
						stab_warning (eng, "stab #%d: source file with unknown suffix: %s",
									 eng->stab->sym_ct, filename);
						eng->stab->source_lang = nel_L_ASSEMBLY;
					}
				}

				/***********************************************/
				/* now make the symbol, and insert it in the   */
				/* nel_static_file_hash table.  also make the   */
				/* prefix for static (local to file) variables */
				/* so that we may rename them appropriately.   */
				/***********************************************/
				{
					register nel_symbol *symbol;
					register nel_type *type;
					nel_make_file_prefix (eng, eng->stab->comp_unit_prefix, filename, nel_MAX_SYMBOL_LENGTH);
					symbol = stab_lookup_file (eng, filename);
					if (symbol != NULL)
					{
						stab_warning (eng, "source file %I multiply defined", symbol);
					}
					type = nel_type_alloc (eng, nel_D_FILE, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL);
					symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, filename),
													  type, NULL, nel_C_FILE, 0, eng->stab->source_lang, 0);
					eng->stab->comp_unit = eng->stab->current_file = symbol;
					stab_insert_file (eng, symbol);

					/*********************************************/
					/* set eng->stab->filename for error messages */
					/*********************************************/
					eng->stab->filename = symbol->name;
					eng->stab->line = 0;

					/*************************************/
					/* start counting include files at 1 */
					/* this file is file_num 0           */
					/* start a new incl_stack            */
					/*************************************/
					eng->stab->incl_ct = 0;
					eng->stab->incl_num = eng->stab->incl_ct++;
					;
					eng->stab->incl_stack = nel_list_alloc (eng, eng->stab->incl_num, symbol, NULL);

					/***********************************************/
					/* the first entry in the include file list is */
					/* the symbol for the compilation unit itself. */
					/***********************************************/
					type->file.includes = eng->stab->first_include = eng->stab->last_include =
											  nel_list_alloc (eng, eng->stab->incl_num, symbol, NULL);

					/*******************************************/
					/* clear the lists of formal arguments and */
					/* local variables for this outer block.   */
					/*******************************************/
					eng->stab->first_arg = NULL;
					eng->stab->last_arg = NULL;
					eng->stab->first_local = NULL;
					eng->stab->last_local = NULL;
					eng->stab->first_block = NULL;
					eng->stab->last_block = NULL;

					/****************************************************/
					/* reset the lists of include files, routines, and  */
					/* static global symbols for this compilation unit. */
					/****************************************************/
					eng->stab->first_routine = NULL;
					eng->stab->last_routine = NULL;
					eng->stab->first_static_global = NULL;
					eng->stab->last_static_global = NULL;

					/**************************************/
					/* start appending nel_line structures */
					/* to the file's type descriptor when */
					/* N_SLINE stabs are encountered.     */
					/**************************************/
					eng->stab->last_line = &(type->file.lines);

				}
			}
			break;

		case N_SOL:
			nel_debug ({
			   stab_trace (eng, "rejecting symbol: stab #%d: type = 0x%x\n%s\n\n", eng->stab->sym_ct,
						  stab_get_type (eng->stab->sym_scan), eng->stab->str_tab + stab_get_name (eng->stab->sym_scan));
		   });
			break;

		case N_BINCL:
			if (stab_get_name (eng->stab->sym_scan) == 0) {
				stab_block_error (eng, "stab #%d: NULL header file name", eng->stab->sym_ct);
			}
			nel_debug ({
				   stab_trace (eng, "include file: stab #%d: type = 0x%x\n%s\n\n",
							  eng->stab->sym_ct, stab_get_type (eng->stab->sym_scan),
							  eng->stab->str_tab + stab_get_name (eng->stab->sym_scan));
			   });

			{
				register nel_symbol *symbol;
				register nel_type *type;
				nel_debug ({
					   if (eng->stab->incl_stack == NULL) {
					   stab_fatal_error (eng, "stab #%d (nel_stab_lex #4): eng->stab->incl_stack == NULL", eng->stab->sym_ct)
						   ;
						   //return -1;
					   }
				   });
				eng->stab->incl_num = eng->stab->incl_ct++;

				/*****************************************/
				/* read in the file name, and look it up */
				/*****************************************/
				symbol = stab_lookup_file (eng, eng->stab->str_tab + stab_get_name (eng->stab->sym_scan));
				if (symbol == NULL) {
					/******************************************/
					/* for a new include file, allocate a new */
					/* type descriptor and symbol, and insert */
					/* it in the file hash table.             */
					/******************************************/
					type = nel_type_alloc (eng, nel_D_FILE, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL);
					symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
													  eng->stab->str_tab + stab_get_name (eng->stab->sym_scan)),
													  type, NULL, nel_C_FILE, 0, eng->stab->source_lang, 0);
					stab_insert_file (eng, symbol);
				} else {
					type = symbol->type;
					nel_debug ({
						   if ((type == NULL) || (type->simple.type != nel_D_FILE)) {
						   stab_fatal_error (eng, "stab #%d (nel_stab_lex #5): bad file type\n%1T",
											eng->stab->sym_ct, symbol->type)
							   ;
							   //return -1;
						   }
						   stab_trace (eng, "include file previously encountered: %I\n\n", symbol);
					   });
				}

				/********************************************/
				/* append the file symbol to the list of    */
				/* include files for this compilation unit. */
				/********************************************/
				if (eng->stab->comp_unit == NULL) {
					stab_error (eng, "include file not in compliation unit: %I", symbol);
					//return -1;
				} else {
					register nel_list *list;
					nel_debug ({
						   if ((eng->stab->comp_unit->type == NULL)
								   || (eng->stab->comp_unit->type->simple.type != nel_D_FILE)) {
						   stab_fatal_error (eng, "stab #%d (nel_stab_lex #6):bad comp_unit\n%1S",
											eng->stab->sym_ct,
											eng->stab->comp_unit)
							   ;
							   //return -1;
						   }
						   if (eng->stab->last_include == NULL) {
						   stab_fatal_error (eng, "stab #%d (nel_stab_lex #7): last_incl == NULL")
							   ;
							   //return -1;
						   }
					   });
					list = nel_list_alloc (eng, eng->stab->incl_num, symbol, NULL);
					eng->stab->last_include->next = list;
					eng->stab->last_include = list;
				}

				/****************************************************/
				/* push the old include file, with its include num. */
				/****************************************************/
				eng->stab->incl_stack = nel_list_alloc (eng, eng->stab->incl_num, symbol, eng->stab->incl_stack);

				/*************************************************/
				/* set eng->stab->current_file to the symbol, and */
				/* set eng->stab->filename for error messages     */
				/*************************************************/
				eng->stab->current_file = symbol;
				eng->stab->filename = symbol->name;
				eng->stab->line = 0;

				/**************************************/
				/* start appending nel_line structures */
				/* to the file's type descriptor when */
				/* N_SLINE stabs are encountered.     */
				/**************************************/
				eng->stab->last_line = &(type->file.lines);
			}
			break;

		case N_EINCL: {
				nel_type *type;

				/************************************************/
				/* pop the stack, and deallocate the structure. */
				/* goto end_include to restore info from the    */
				/* previous file.                               */
				/************************************************/
				nel_list *temp = eng->stab->incl_stack;
				eng->stab->incl_stack = eng->stab->incl_stack->next;
				nel_list_dealloc (temp);

				nel_debug ({
					   if ((eng->stab->incl_stack == NULL) ||
							   (eng->stab->incl_stack->symbol == NULL) ||
							   (eng->stab->incl_stack->symbol->name == NULL) ||
							   (eng->stab->incl_stack->symbol->type == NULL) ||
							   (eng->stab->incl_stack->symbol->type->simple.type != nel_D_FILE)) {
					   stab_fatal_error (eng, "stab #%d (nel_stab_lex #8): bad eng->stab->incl_stack\n%1L", eng->stab->sym_ct, eng->stab->incl_stack)
						   ;
						   //return -1;
					   }
					   stab_trace (eng, "end of include file: stab #%d: type = 0x%x\n\n", eng->stab->sym_ct, stab_get_type (eng->stab->sym_scan));
					   stab_trace (eng, "returning to file\n%1S\n", eng->stab->incl_stack->symbol);
				   });

		/*********************************************************/
				/* append an nel_line structure with address 0 to the     */
				/* list of line # / address pairs as a dummy terminator. */
				/*********************************************************/
				if (eng->stab->last_line != NULL) {
					*eng->stab->last_line = nel_line_alloc (eng, 0, NULL, NULL);
				}

				/***************************************************/
				/* set the incl number from the top stack element. */
				/* set eng->stab->current_file_types to the type    */
				/* descriptor for the new include file.            */
				/***************************************************/
				type = eng->stab->incl_stack->symbol->type;
				eng->stab->incl_num = eng->stab->incl_stack->value;
				eng->stab->current_file = eng->stab->incl_stack->symbol;

				/*********************************************/
				/* set eng->stab->filename for error messages */
				/*********************************************/
				eng->stab->filename = eng->stab->incl_stack->symbol->name;
				eng->stab->line = 0;

				/*****************************************************/
				/* scan down the list of line structures to the end. */
				/* start appending nel_line structures there when     */
				/* N_SLINE stabs are encountered.                    */
				/*****************************************************/
				for (eng->stab->last_line = &(type->file.lines); (*eng->stab->last_line != NULL);
						eng->stab->last_line = &((*eng->stab->last_line)->next))
					;
			}
			break;

		case N_EXCL:
			/* excluded include file */
			if (stab_get_name (eng->stab->sym_scan) == 0) {
				stab_block_error (eng, "stab #%d: NULL excl header file name", eng->stab->sym_ct);
			}
			nel_debug ({
						   stab_trace (eng, "excluded include file: stab #%d: type = 0x%x\n%s\n\n",
									  eng->stab->sym_ct, stab_get_type (eng->stab->sym_scan), eng->stab->str_tab + stab_get_name (eng->stab->sym_scan));
					   });

			{
				register nel_symbol *symbol;
				register nel_type *type;
				register nel_stab_type *scan;
				int incl_num = eng->stab->incl_ct++;

				/*****************************************/
				/* read in the file name, and look it up */
				/*****************************************/
				symbol = stab_lookup_file (eng, eng->stab->str_tab + stab_get_name (eng->stab->sym_scan));
				if (symbol == NULL) {
					stab_fatal_error (eng, "stab #%d (nel_stab_lex #9): excl header file not in table: %s",
									 eng->stab->sym_ct, eng->stab->str_tab + stab_get_name (eng->stab->sym_scan));
					//return -1;
				}
				type = symbol->type;
				nel_debug ({
				   if ((type == NULL) || (type->simple.type != nel_D_FILE)) {
							   stab_fatal_error (eng, "stab #%d (nel_stab_lex #10): bad type\n%1T",
												eng->stab->sym_ct, type)
								   ;
								   //return -1;
							   }
							   stab_trace (eng, "excluded include file\n%1S\n", symbol);
						   });

				/******************************************************/
				/* we set the file_num field to -1 when we encounter  */
				/* compilation unit, so it will be non-zero if we     */
				/* have already inserted the stab_type list into the  */
				/* eng->stab->type_hash table.  if we have not done so */
				/* already, insert the types, after setting their     */
				/* file_num field to the current include file #.      */
				/******************************************************/
				for (scan = type->file.stab_types; (scan != NULL); scan = scan->file_next) {

					/********************************************************/
					/* if the nel_stab_type structure has not been inserted  */
					/* into the eng->stab->type_hash table yet, it will have */
					/* a file_num field of -1, as we set all the file_num   */
					/* fields to -1 when we clear the eng->stab->type_hash   */
					/* table.  if this file has already appeared as an      */
					/* include file in this unit, the nel_stab_types will    */
					/* already be inserted - do not reinsert them.  if the  */
					/* type information for this include file differs from  */
					/* appearances of the same include file in earlier      */
					/* compilation units (usually because the files are     */
					/* included in differing orders), the information is    */
					/* redeclared entirely, and new nel_stab_types inserted  */
					/* at the start of the list of stab_types.  after the   */
					/* compilation unit is parsed, we revert back to the    */
					/* old type information, which appears at the end of    */
					/* the list of stab_types.  when we insert the older    */
					/* stab_types at the end, their definitions overwrite   */
					/* the earlier type_nums in the eng->stab->type_hash     */
					/* table, and  nel_lookup_type () will return the   */
					/* earliest defintion of a type num for this file.      */
					/********************************************************/

					if (scan->file_num < 0) {
						scan->file_num = incl_num;
						if (stab_lookup_type_2 (eng, incl_num, scan->type_num) == NULL) {
							stab_insert_type (eng, scan, eng->stab->type_hash, nel_stab_type_hash_max);
						} else {
							nel_debug ({
										   stab_trace (eng, "stab_type has been redefined\n%1U\n", scan);
									   });
						}

					} else {

#if 1
						if (stab_lookup_type_2 (eng, incl_num, scan->type_num) == NULL) {
							nel_stab_type *tmp = stab_remove_type(eng, scan, eng->stab->type_hash, nel_stab_type_hash_max);
							tmp->file_num = incl_num;
							stab_insert_type (eng, tmp, eng->stab->type_hash, nel_stab_type_hash_max);
						} 
#endif

						nel_debug ({
									   stab_trace (eng, "stab_type already inserted\n%1U\n", scan);
								   });
					}


				}
			}
			break;


			/***************************************************************/
			/* for common blocks, set eng->stab->common_block to the common */
			/* block symbol upon entry, and back to NULL upon exit.        */
			/***************************************************************/
		case N_BCOMM:	 /* begin common: name,, */
			nel_debug ({
						   stab_trace (eng, "begin common: stab #%d: type = 0x%x\n%s\n\n",
									  eng->stab->sym_ct, stab_get_type (eng->stab->sym_scan),
									  eng->stab->str_tab + stab_get_name (eng->stab->sym_scan));
					   });

			{
				/***************************************/
				/* read in the common name, look it up */
				/***************************************/
				register nel_symbol *symbol;
				symbol = stab_lookup_ident (eng, eng->stab->str_tab + stab_get_name (eng->stab->sym_scan));

				nel_debug ({
							   if ((symbol != NULL) && (symbol->type == NULL)) {
							   stab_fatal_error (eng, "stab #%d (nel_stab_lex #12): bad type\n%1S",
												eng->stab->sym_ct, symbol)
								   ;
								   //return -1;
							   }
						   });

				if ((symbol != NULL) && (symbol->type->simple.type == nel_D_COMMON)) {
					/****************************************************/
					/* if we found the symbol and it is a common block, */
					/* just set eng->stab->common_block to it.           */
					/****************************************************/
					eng->stab->common_block = symbol;
				} else {
					/*****************************************************/
					/* if the symbol was not found (first time the block */
					/* is encountered), allocate a symbol for the block  */
					/* and insert it in the static ident hash table.     */
					/* get the location of the block from the ld hash    */
					/* table entry.                                      */
					/* if the symbol was not a common block, emit an     */
					/* error message and do the same, but don't insert   */
					/* it in the static ident hash table.                */
					/*****************************************************/
					register nel_symbol *ld_sym;
					register nel_symbol *new_sym;
					register char *buffer;
					new_sym = nel_static_symbol_alloc (eng, nel_insert_name(eng,
													   eng->stab->str_tab + stab_get_name (eng->stab->sym_scan)),
													   nel_type_alloc (eng, nel_D_COMMON, 0, 0, 0, 0), NULL, nel_C_GLOBAL, 0, nel_L_FORTRAN, 0);
					/******************************************************/
					/* find the ld new_sym table entry for the global var. */
					/* the ld new_sym names are prepended with an '_'.     */
					/* since this is FORTRAN, they also have an '_'       */
					/* appended, but on everything but ALLIANT_FX2800,    */
					/* the trailing '_' is also in the debug new_sym table */
					/* entry, so don't add another one.                   */
					/******************************************************/
					nel_alloca (buffer, strlen (new_sym->name) + 3, char);
					sprintf (buffer, "_%s_", new_sym->name);

					ld_sym = stab_lookup_location (eng, buffer);
					nel_dealloca (buffer);
					if (ld_sym == NULL) {
						/************************************/
						/* if we didn't find the ld entry,  */
						/* emit an error message, and scan  */
						/* for the end of the common block. */
						/************************************/
						stab_error (eng, "stab #%d: no ld entry for common block %I", eng->stab->sym_ct, new_sym);
						//should 'error' not return???
						for (;;) {
							eng->stab->sym_ct++;
							eng->stab->sym_scan++;
							if (eng->stab->sym_ct > eng->stab->sym_size) {
								return 0; //should be '0'???
							}
							if (stab_get_type (eng->stab->sym_scan) == N_ECOMM) {
								break;
							}
						}
						eng->stab->common_block = NULL;
					} else {
						new_sym->value = ld_sym->value;
						/**************************************************/
						/* insert the new symbol in the static ident hash */
						/* table, or else emit the error message if this  */
						/* is a redeclaration of the symbol.              */
						/**************************************************/
						if (symbol == NULL) {
							stab_insert_ident (eng, new_sym);
						} else {
							stab_error (eng, "stab #%d: redeclaration of %I", eng->stab->sym_ct, symbol);
							//return -1;
						}
						eng->stab->common_block = new_sym;
					}
				}
				nel_debug ({
							   stab_trace (eng, "eng->stab->common_block =\n%1S\n", eng->stab->common_block);
						   });
			}
			break;

		case N_ECOMM:	/* end common: name,, */
			nel_debug ({
						   stab_trace (eng, "end common: stab #%d: type = 0x%x\n\n",
									  eng->stab->sym_ct,
									  stab_get_type(eng->stab->sym_scan));
					   });
			eng->stab->common_block = NULL;
			break;

		case N_FUN:
			/* if N_FUN with 0 value , it means function end here,
			   fall through , otherwise it is an begin of a fuction , 
			   read the keyword */
			if(stab_get_name(eng->stab->sym_scan)) {

#if 0
				char *funcname = eng->stab->str_tab + stab_get_name (eng->stab->sym_scan);
				if (funcname != 0) {
					printf("stab_parse: FUN=%s\n", funcname );
					if(strcmp(funcname, "read_http_ack:F(57,20)" ) == 0 ){
						dummy_func();
					}

				}
			
#endif
				/************************************/
				/* check to make sure there is a    */
				/* string associated with this stab */
				/************************************/
				if (stab_get_name (eng->stab->sym_scan) == 0) {
					stab_error (eng, "stab #%d: NULL string", eng->stab->sym_ct);
					break;
				}

				/******************************************************/
				/* set eng->stab->str_scan to the start of the string. */
				/******************************************************/
				eng->stab->str_scan = eng->stab->str_tab + stab_get_name (eng->stab->sym_scan);

				/*************************************************/
				/* we have a valid string that we want to parse. */
				/*************************************************/
				nel_debug ({
					   stab_trace (eng, "stab #%d: type = 0x%x value = 0x%x\n%s\n\n",
								  eng->stab->sym_ct, stab_get_type (eng->stab->sym_scan), stab_get_value (eng->stab->sym_scan),
								  eng->stab->str_tab + stab_get_name (eng->stab->sym_scan));
				   });

				parse_stab_string(eng);
				break;
			}

			/********************************************************/
			/* on ALLIANT_FX and ALIANT_FX2800 machines, the end of */
			/* a function is explicitly marked, so perform all the  */
			/* actions we would otherwise do at the end of a block, */
			/* but don't pop the old values of lists, as they were  */
			/* never saved at the start of the routine.             */
			/********************************************************/
			//case N_EFUN:

			nel_debug ({
						   stab_trace (eng, "end routine: stab #%d: type = 0x%x\n\n", eng->stab->sym_ct, stab_get_type (eng->stab->sym_scan));
					   });
			//if (eng->stab->prog_unit == NULL) {
			//	stab_error (eng, "stab #%d: extraneous end of routine", eng->stab->sym_ct);
			//return -1;
			//}

			/*******************************************/
			/* save the address of the end of the      */
			/* routine in eng->stab->block_end_address, */
			/* so that nel_stab_end_routine will        */
			/* correctly set it.                       */
			/*******************************************/
			eng->stab->block_end_address = (char *)(long) stab_get_value (eng->stab->sym_scan);

			/********************************************************/
			/* call nel_stab_end_routine to create a block structure */
			/* for the main procedure body, and to reset the lists  */
			/* of parameters and local vars, and purge the local    */
			/* symbols from the hash tables.                        */
			/********************************************************/
			stab_end_routine (eng);

			/******************************************************/
			/* reset the scoping level and block number.          */
			/* these are manually incremented at the start of the */
			/* procedure as there is no block for the main body.  */
			/* clear eng->stab->prog_unit.                         */
			/******************************************************/
			eng->stab->level = 0;
			eng->stab->block_ct = 0;
			eng->stab->block_no = 0;
			eng->stab->prog_unit = NULL;

			break;

			/********************************************************/
			/* source line - set the line number for error messages */
			/* and keep track of the address, in case we are in a   */
			/* block which is terminated without a N_LBRAC stab.    */
			/********************************************************/
		case N_SLINE:	/* src line: 0,,0,linenumber,address */
			nel_debug ({
						   stab_trace (eng, "source line #%d: stab #%d: type = 0x%x address = 0x%x\n\n",
									  stab_get_line (eng->stab->sym_scan),
									  eng->stab->sym_ct, stab_get_type (eng->stab->sym_scan),
									  stab_get_value (eng->stab->sym_scan));
					   });
			eng->stab->line = stab_get_line (eng->stab->sym_scan);

			/**************************************/
			/* record this address as the highest */
			/* address so far in the block.       */
			/**************************************/
			eng->stab->block_end_address = (char *)(long) stab_get_value (eng->stab->sym_scan);

			/**************************************/
			/* append an nel_line structure to the */
			/* list of line # / address pairs.    */
			/**************************************/
			if (eng->stab->last_line != NULL) {
				register nel_line *line = nel_line_alloc (eng, eng->stab->line, (char *) (long )stab_get_value (eng->stab->sym_scan), NULL);
				*eng->stab->last_line = line;
				eng->stab->last_line = &(line->next);
			}
			break;

		default:
			nel_debug ({
						   stab_trace (eng, "rejecting symbol: stab #%d: type = 0x%x\n%s\n\n", eng->stab->sym_ct,
									  stab_get_type (eng->stab->sym_scan), eng->stab->str_tab + stab_get_name (eng->stab->sym_scan));
					   });
			break;


		case N_OPT:
			break;

			/*****************/
			/* other symbols */
			/*****************/
			//case N_FN:
			//case N_FN_SEQ:
		case N_OBJ:
		case N_MAIN:
			//case N_ENDM:
			//case N_WARNING:
		case N_FNAME:	/* procedure name (f77 kludge): name,,0 */
		case N_SSYM:	/* structure elt: name,,0,type,struct_offset */
		case N_ENTRY:	/* alternate entry: name,linenumber,address */
		case N_ECOML:	/* end common (local name): ,,address */
		case N_LENG:	/* second stab entry with length information */

			break;
		}


#if 0
		{
			struct nel_STAB_TYPE *stab_type;
			stab_type = stab_lookup_type_2(eng, 71, 2 );
			if(stab_type != NULL && stab_type->type != NULL) {
				printf("Hi, stab_type->type->simple = %s\n", (stab_type->type->simple.type == nel_D_STAB_UNDEF ) ? "nel_D_STAB_UNDEF" : "nel_D_STAB_POINTER");
			}
		
		}
#endif


	}

	/* should not be here */
	return -1;
}

/*****************************************************************************/
/* nel_stab_type_hash () applies the hash function used for table lookup of   */
/* nel_stab_type structures to type_num / file_num and returns the result.    */
/* <max> is the number of chains in the hash table; we assume that it is a   */
/* power of 2, so that we may divide modulo by it efficiently by and'ing     */
/* with <max> - 1.                                                           */
/*****************************************************************************/
int stab_type_hash (int file_num, int type_num, int max)
{
	register int retval = 0;
	register char *ch;
	register int mask = max - 1;
	register int i;

	/*********************************************************/
	/* loop through the bytes in the integer file_num, and   */
	/* add their values, shifting left the result each time, */
	/* and dividing modulo the table size.                   */
	/*********************************************************/
	for (ch = (char *) (&file_num), i = 0; (i < sizeof (int)); ch++, i++) {
		retval <<= 1;
		retval += *ch;
		retval &= mask;
	}

	/***************************/
	/* do the same to type_num */
	/***************************/
	for (ch = (char *) (&type_num), i = 0; (i < sizeof (int)); ch++, i++) {
		retval <<= 1;
		retval += *ch;
		retval &= mask;
	}

	return (retval);
}

/*****************************************************************************/
/* nel_stab_remove_type() removes the stab_type structure pointed to by   */
/* <type> into the <table> on the appropriate chain.  <table> should have    */
/* <max> chains in it.  The type is returned.                                */
/*****************************************************************************/
nel_stab_type *stab_remove_type (struct nel_eng *eng, register nel_stab_type *stab_type, register nel_stab_type **table, int max)
{
	register int index;
	register nel_stab_type *scan, *prev;

	nel_debug ({
	if (stab_type == NULL) {
		stab_fatal_error (eng, "stab #%d (nel_stab_remove_type #1): NULL stab_type", eng->stab->sym_ct);
	}
	if (!stab_type->inserted) {
		stab_fatal_error (eng, "stab #%d (nel_stab_remove_type #2): stab_type not inserted\n%1U", eng->stab->sym_ct, stab_type);
	}
	});

	/****************************************************************/
	/* find the nel_stab_type at the start of the appropriate chain */
	/****************************************************************/
	index = stab_type_hash (stab_type->file_num, stab_type->type_num, max);
	for (prev = NULL, scan = table[index]; 
		((scan != NULL) && (scan != stab_type)); scan = scan->next){
		prev = scan;
	}

	if(scan == NULL) {
		//stab_fatal_error (eng, "stab #%d (nel_stab_remove_type #3): stab_type not found\n%1U", eng->stab->sym_ct, stab_type);
		return NULL;
	}

	if(!prev){	/* the first element is stab_type */
			table[index] = scan->next;
	}	
	else if(prev) {
		prev->next = scan->next;
	}

	nel_debug ({ stab_trace (eng, "] exiting nel_stab_insert_type\nstab_type =\n%1U\n", stab_type); });

	stab_type->inserted = 0;
	return (stab_type);
}


/*****************************************************************************/
/* nel_stab_lookup_type () finds the nel_stab_type structure with the correct  */
/* file_num and type_num members in <table>, or returns NULL if none exists. */
/* <table> should have <max> chains in it.                                   */
/*****************************************************************************/
nel_stab_type *stab_lookup_type (register int file_num, register int type_num, nel_stab_type **table, int max)
{
	register nel_stab_type *scan;
	register int index;

	/********************************/
	/* apply the hash function, and */
	/* search the appropriate chain */
	/********************************/
	index = stab_type_hash (file_num, type_num, max);
	for (scan = table[index]; ((scan != NULL) && ((file_num != scan->file_num) || (type_num != scan->type_num))); scan = scan->next)
		;

	/***********************************/
	/* return the table entry, or NULL */
	/***********************************/
	return (scan);
}

nel_stab_type *stab_lookup_type_2 (struct nel_eng *eng, register int file_num, register int type_num) 
{
	return stab_lookup_type (file_num, type_num, eng->stab->type_hash, nel_stab_type_hash_max);
}

/*****************************************************************************/
/* nel_stab_insert_type () inserts the nel_stab_type structure pointed to by   */
/* <type> into the <table> on the appropriate chain.  <table> should have    */
/* <max> chains in it.  The type is returned.                                */
/*****************************************************************************/
nel_stab_type *stab_insert_type (struct nel_eng *eng, register nel_stab_type *stab_type, register nel_stab_type **table, int max)
{
	register int index;

	nel_debug ({ stab_trace (eng, "entering nel_stab_insert_type [\neng = 0x%x\nstab_type =\n%1Utable = 0x%x\nmax = 0x%x\n\n", eng, stab_type, table, max); });

	nel_debug ({
	if (stab_type == NULL) {
		stab_fatal_error (eng, "stab #%d (nel_stab_insert_type #1): NULL stab_type", eng->stab->sym_ct);
	}
	if (stab_type->inserted) {
		stab_fatal_error (eng, "stab #%d (nel_stab_insert_type #2): stab_type already inserted\n%1U", eng->stab->sym_ct, stab_type);
	}
	});

	/**********************************/
	/* insert the nel_stab_type at the */
	/* start of the appropriate chain */
	/**********************************/
	index = stab_type_hash (stab_type->file_num, stab_type->type_num, max);
	stab_type->next = table[index];
	table[index] = stab_type;
	stab_type->inserted = 1;

	nel_debug ({ stab_trace (eng, "] exiting nel_stab_insert_type\nstab_type =\n%1U\n", stab_type); });
	return (stab_type);
}


/*****************************************************************************/
/* nel_stab_insert_type_2 () inserts <stab_type> into stab type hash table, and also */
/* into the linked list of types defined in the appropriate file, which      */
/* is not necessarily the current file.                                      */
/*****************************************************************************/
nel_stab_type *stab_insert_type_2 (struct nel_eng *eng, register nel_stab_type *stab_type)
{
	register nel_list *scan;
	register nel_type *type;

	nel_debug ({ stab_trace (eng, "entering nel_stab_insert [\neng = 0x%x\nstab_type =\n%1U\n", eng, stab_type); });

	nel_debug ({
	if (stab_type == NULL) {
		stab_fatal_error (eng, "stab #%d (nel_stab_insert #1): NULL stab_type", eng->stab->sym_ct);
	}
	});

	/**************************************/
	/* insert the type in the hash table. */
	/**************************************/
	stab_insert_type (eng, stab_type, eng->stab->type_hash, nel_stab_type_hash_max);

	/*******************************************************/
	/* scan the list of include files for this compilation */
	/* unit for the one with correct file_num.             */
	/*******************************************************/
	for (scan = eng->stab->first_include; ((scan != NULL) && (scan->value != stab_type->file_num)); scan = scan->next)
		;

	if (scan == NULL) {
		stab_stmt_error (eng, "stab #%d: bad type number", eng->stab->sym_ct);
	}

	nel_debug ({
	if ((scan->symbol == NULL) || (scan->symbol->type == NULL) || (scan->symbol->type->simple.type != nel_D_FILE)) {
		stab_fatal_error (eng, "stab #%d (nel_stab_insert #2): bad include list\n%1L", eng->stab->sym_ct, eng->stab->first_include);
	}
	});

	/*****************************************************/
	/* insert the stab_type in the linked list of types. */
	/*****************************************************/
	type = scan->symbol->type;
	stab_type->file_next = type->file.stab_types;
	type->file.stab_types = stab_type;

	nel_debug ({ stab_trace (eng, "] exiting nel_stab_insert\nstab_type =\n%1U\n", stab_type); });
	return (stab_type);
}


/*****************************************************************************/
/* nel_stab_clear_type_hash () clears the stab type hash table, scanning through   */
/* each chain, setting the file_num field of each stab_type to -1.           */
/* a negative file_num field (and a zero "inserted" field) indicates the     */
/* stab_type is not inserted in the hash table.                              */
/*****************************************************************************/
void stab_clear_type_hash (register nel_stab_type **table)
{
	register int i;
	for (i = 0; (i < nel_stab_type_hash_max); table++, i++) {
		register nel_stab_type *scan = *table;
		register nel_stab_type *prev;
		while (scan != NULL) {
			prev = scan;
			scan = scan->next;
			prev->file_num = -1;
			prev->inserted = 0;
			prev->next = NULL;
		}
		*table = NULL;
	}
}


/*****************************************************************************/
/* nel_stab_ld_scan scans the ld symbol table, and inserts entries for ld     */
/* symbols with type N_TEXT, N_DATA, and N_BSS (for system V, symbol types   */
/* STT_NOTYPE, STT_OBJECT, and STT_FUNC) into the dynamic and static (if the */
/* external bit is set) location hash tables.                                */
/* The symbols are given type nel_D_LOCATION and class nel_C_LOCATION or       */
/* nel_C_STATIC_LOCATION.                                                     */
/*****************************************************************************/
void stab_ld_scan (struct nel_eng *eng)
{
	Elf64_Sym *scan;
	nel_debug ({ stab_trace (eng, "entering nel_stab_ld_scan [\neng = 0x%x\n\n", eng); });

	nel_debug ({
		   if (eng->stab->ld_str_tab == NULL) {
		   stab_fatal_error (eng, "stab #%d: (nel_stab_ld_scan #1): NULL ld string table", eng->stab->sym_ct)
			   ;
		   }
		   if (eng->stab->ld_sym_tab == NULL) {
		   stab_fatal_error (eng, "stab #%d: (nel_stab_ld_scan #2): NULL ld symbol table", eng->stab->sym_ct)
			   ;
		   }
	   });

	*(eng->stab->comp_unit_prefix) = '\0';

	/********************************************************/
	/* loop through all the symbols in the ld symbol table. */
	/* on Berkely architectures, the ld symbol table and    */
	/* the debug symbol table are one in the same.          */
	/********************************************************/
	for (eng->stab->sym_ct = 1, scan = eng->stab->ld_sym_tab; (eng->stab->sym_ct <= eng->stab->ld_sym_size); scan++, eng->stab->sym_ct++) {
		/**********************************************/
		/* nel_tracing should only be set for stab #'s */
		/* between -nel_stab_tstart and -nel_stab_tend. */
		/**********************************************/
		//nel_debug ({
		//   if (eng->stab->sym_ct >= -nel_stab_tstart) {
		//      nel_tracing = nel_stab_old_tracing;
		//   }
		//   if (eng->stab->sym_ct > -nel_stab_tend) {
		//      nel_tracing = 0;
		//   }
		//});

		/************************************************************/
		/* print out the stab # every 100'th time through the loop. */
		/* call stab_overwritable () so that the message is written   */
		/* over by any error messages or trace messages.            */
		/* we must also print out stab #1 and the stab # after      */
		/* any new error messages.                                  */
		/************************************************************/
		{
			static int old_error_ct;
			if ((eng->stab->sym_ct == 1) || (eng->stab->sym_ct % 100 == 0) || (old_error_ct != eng->stab->error_ct))
			{
				stab_overwritable (eng, "ld stab #%d", eng->stab->sym_ct);
			}
			old_error_ct = eng->stab->error_ct;
		}

		/******************************************************/
		/* if there is no symbol name for this stab, or it    */
		/* doesn't have a value, continue with the next stab. */
		/******************************************************/
		if ( (scan->st_name == 0) || (scan->st_name >= eng->stab->ld_str_size)
				|| (((char *) stab_get_value (scan)) == NULL)) {
			continue;
		}

		switch (scan->st_info) {
			/***********************************************************/
			/* for global vars and functions, just look up the string, */
			/* with the leading '_' stripped.  if there is no '_' this */
			/* isn't a legal compiled identifier.                      */
			/***********************************************************/
			//#define ELF32_ST_INFO(b,t) (((b)<<4) + ((t)&0x0f))

			case ELF64_ST_INFO (STB_GLOBAL, STT_NOTYPE):
			case ELF64_ST_INFO (STB_GLOBAL, STT_OBJECT):
			case ELF64_ST_INFO (STB_GLOBAL, STT_FUNC): {
				register nel_symbol *symbol;
				char *name = eng->stab->ld_str_tab + scan->st_name;
				stab_read_name (eng, eng->stab->name_buffer, &name, nel_MAX_SYMBOL_LENGTH);
				symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, eng->stab->name_buffer), nel_location_type, (char *) stab_get_value (scan), nel_C_LOCATION, 0, nel_L_NULL, 0);
				stab_insert_location (eng, symbol);
			}
			break;
		}
	}

	nel_debug ({ stab_trace (eng, "\n] exiting nel_stab_ld_scan\n\n"); });
}

/*****************************************************************************/
/* stab_file_parse() scans the symbol table of <filename>, forming symbols for      */
/* data types and objects in the executable file, and inserts the            */
/* appropriate symbols into the eng 's symbol tables.       */
/* it returns the number of symbols parsed.                                  */
/*****************************************************************************/
int stab_file_parse(struct nel_eng *eng, char *filename)
{
	nel_jmp_buf return_pt;
	Elf64_Ehdr header;	/* read file header in here		*/
	Elf64_Shdr sec_hdr;	/* read section headers	in here		*/

	off_t shoff;		/* offset of section header (scan var)	*/
	off_t sh_str_ofst = 0;	/* offset of section hdr string table	*/
	off_t sh_str_size = 0;	/* size of section hdr string table	*/
	off_t ld_str_ofst = 0;	/* offset of ld string table		*/
	int ld_str_size = 0;	/* size of ld string table		*/
	char *ld_str_tab = NULL;	/* ld string table		*/
	off_t ld_sym_ofst;	/* offset of ld symbol table		*/
	int ld_sym_size;		/* size of ld symbol table	*/

	Elf64_Sym *ld_sym_tab = NULL; /* ld symbol table			*/
	off_t str_ofst = 0;	/* offset of debug string table		*/
	int str_size = 0;		/* size of debug string table	*/
	char *str_tab = NULL;	/* debug string table			*/
	off_t sym_ofst = 0;	/* offset of debug symbol table		*/
	int sym_size = 0;		/* size of debug symbol table	*/

	Elf_Sym *sym_tab = NULL;/* debug symbol table			*/
	FILE *file;		/* opened on filename			*/
	int i;			/* counter var				*/
	int retval = -1;	/* return val: # of symbols parsed      */

	nel_debug ({ 
		stab_trace(eng, "entering stab_file_parse [\nfilename = %s\n\n", filename); 
	});

	eng->prog_name = filename;
	

#ifdef STAB_DEBUG
	if(eng->stab_debug_level ) {
		if (!(stab_debug_file = fopen("stab.log", "w"))) {
			stab_error (eng, "can't open debug output file \"%s\"", "stab");
			return -1;
		}
	}
#endif

	/************************************************/
	/* initialize the nel_eng for the stab variant */
	/************************************************/
	nel_malloc(eng->stab, 1, struct eng_stab);
	stab_init(eng, filename);


	/******************************************************/
	/* eng->stab->sym_ct is the return value of nel_stab () */
	/* set it to 0 in case we jump to end on an error     */
	/******************************************************/
	eng->stab->sym_ct = 0;

	//printf("stab_file_parse:3\n");
	if ((file = fopen (filename, "r")) == NULL) {
		stab_error (eng, "can't open executable file \"%s\"", filename);
		goto end;
	}


	/**************************************************************/
	/* system V executable files - search for the .debug section  */
	/* for the symbols, and for the .debugstrtab for the strings. */
	/**************************************************************/

	//printf("stab_file_parse:4\n");
	if (fread ((char *) &header, sizeof (Elf64_Ehdr), 1, file) != 1) {
		stab_error (eng, "error reading \"%s\": can't read Elf header", filename);
		fclose (file);
		goto end;
	}

#define E_BADMAG(hdr)	(( hdr.e_ident[EI_MAG0] != ELFMAG0) || (hdr.e_ident[EI_MAG1] != ELFMAG1) || (hdr.e_ident[EI_MAG2] != ELFMAG2) || (hdr.e_ident[EI_MAG3] != ELFMAG3))

//#define E_BADELF(hdr) 	((hdr.e_ident[EI_VERSION] != EV_CURRENT) ||(hdr.e_ident[EI_CLASS] != ELFCLASS32)) 
#define E_BADELF(hdr) 	((hdr.e_ident[EI_VERSION] != EV_CURRENT) || (hdr.e_ident[EI_CLASS] != ELFCLASS64)) 


	if (E_BADMAG (header)) {
		stab_error (eng, "error reading \"%s\": bad magic number", filename);
		fclose (file);
		goto end;
	}
	if (E_BADELF (header)) {
		stab_error (eng, "error reading \"%s\": bad Elf header", filename);
		fclose (file);
		goto end;
	}
	if (header.e_shstrndx == SHN_UNDEF) {
		stab_error (eng, "error reading \"%s\": no string section", filename);
		fclose (file);
		goto end;
	}

	/*****************************************************/
	/* find section header for the section string table. */
	/* it contains the names of the section headers.     */
	/*****************************************************/
	fseek(file, header.e_shoff + header.e_shentsize * header.e_shstrndx, 0);
	if (fread ((char *) &sec_hdr, sizeof (Elf64_Shdr), 1, file) != 1) {
		stab_error (eng, "error reading \"%s\": can't read string section header", filename);
		fclose (file);
		goto end;
	}
	sh_str_ofst = sec_hdr.sh_offset;
	sh_str_size = sec_hdr.sh_size;

	/****************************************************************/
	/* now search through the section table, searching for sections */
	/* named ".strtab", ".symtab" ".debugstrtab", and ".debug"      */
	/* (ld string and symbol tables, and debug string and symbol    */
	/* tables, respectively.                                        */
	/****************************************************************/
	shoff = header.e_shoff;
	for (i = 0; i < header.e_shnum; i++, shoff += header.e_shentsize) {
		char name[13]; /* 13 = strlen (".debugstrtab") + 1 */
		if (fseek (file, shoff, 0) != 0) {
			continue;
		}

		if (fread ((char *) &sec_hdr, sizeof (Elf64_Shdr), 1, file) != 1) {
			continue;
		}
		if ((sec_hdr.sh_name == /*NULL */ 0) 
			|| (sec_hdr.sh_name >= sh_str_size)) {
			continue;
		}
		if (fseek (file, sh_str_ofst + sec_hdr.sh_name, 0) != 0) {
			continue;
		}
		if (fscanf (file, "%12s", name) != 1) {
			continue;
		}

		/******************************************/
		/* we have a valid section name in name[] */
		/******************************************/
		if (! strcmp (name, ".strtab")) {
			ld_str_ofst = sec_hdr.sh_offset;
			ld_str_size = sec_hdr.sh_size;
		} else if (! strcmp (name, ".symtab")) {
			ld_sym_ofst = sec_hdr.sh_offset;
			ld_sym_size = sec_hdr.sh_size;
		} else if (! strcmp (name, ".stabstr")) { 	/* .debugstrtab */
			str_ofst = sec_hdr.sh_offset;
			str_size = sec_hdr.sh_size;
		} else if (! strcmp (name, ".stab")) {		/* .debug */
			sym_ofst = sec_hdr.sh_offset;
			sym_size = sec_hdr.sh_size;
		}
	}

	/*****************************************************/
	/* make sure we found all of the following sections: */
	/*    ".strtab"      - ld string table               */
	/*    ".symtab"      - ld symbol table               */
	/*    ".debugstrtab" - debug string table            */
	/*    ".debug"       - debug symbol table            */
	/* if a sections is not found, the var its offset    */
	/* is stored in will have the value 0.               */
	/*****************************************************/
	if (ld_str_size == 0) {
		stab_error (eng, "error reading \"%s\": .strtab section not found", filename);
		fclose (file);
		goto end;
	}
	if (ld_sym_size == 0) {
		stab_error (eng, "error reading \"%s\": .symtab section not found", filename);
		fclose (file);
		goto end;
	}
	if (str_size == 0) {
		stab_error (eng, "error reading \"%s\": .debugstrtab section not found", filename);
		fclose (file);
		goto end;
	}
	if (sym_size == 0) {
		stab_error (eng, "error reading \"%s\": .debug section not found", filename);
		fclose (file);
		goto end;
	}

	/******************************************************/
	/* allocate space for the string table and read it in */
	/******************************************************/
	nel_alloca (ld_str_tab, ld_str_size, char);
	if (fseek (file, ld_str_ofst, 0) || (fread ((char *) ld_str_tab, ld_str_size, 1, file) != 1)) {
		stab_error (eng, "error reading \"%s\": can't read ld string table", filename);
		fclose (file);
		goto end;
	}
	eng->stab->ld_str_tab = ld_str_tab;
	eng->stab->ld_str_size = ld_str_size;

	/*********************************************************/
	/* allocate space for the ld symbol table and read it in */
	/*********************************************************/
	ld_sym_size /= sizeof (Elf64_Sym);
	nel_alloca (ld_sym_tab, ld_sym_size, Elf64_Sym);
	if (fseek (file, ld_sym_ofst, 0) || (fread ((char *) ld_sym_tab, ld_sym_size * sizeof (Elf64_Sym), 1, file) != 1)) {
		stab_error (eng, "error reading \"%s\": can't read ld symbol table", filename);
		fclose (file);
		goto end;
	}
	eng->stab->ld_sym_tab = ld_sym_tab;
	eng->stab->ld_sym_size = ld_sym_size;

	/************************************************************/
	/* allocate space for the debug string table and read it in */
	/************************************************************/
	nel_alloca (str_tab, str_size, char);
	if (fseek (file, str_ofst, 0) || (fread ((char *) str_tab, str_size, 1, file) != 1)) {
		stab_error (eng, "error reading \"%s\": can't read debug string table", filename);
		fclose (file);
		goto end;
	}
	eng->stab->str_tab = str_tab;
	eng->stab->str_size = str_size;

	/*************************************************************/
	/* allocate space for the debug symbol table and read it in */
	/*************************************************************/
	sym_size /= sizeof (Elf_Sym);
	nel_alloca (sym_tab, sym_size, Elf_Sym);
	if (fseek (file, sym_ofst, 0) || (fread ((char *) sym_tab, sym_size * sizeof (Elf_Sym), 1, file) != 1)) {
		stab_error (eng, "error reading \"%s\": can't read debug symbol table", filename);
		fclose (file);
		goto end;
	}
	eng->stab->sym_tab = sym_tab;
	eng->stab->sym_size = sym_size;


	/******************************************************************/
	/* we have now read in the debug and ld symbol and string tables. */
	/* On System V architectures, these are distinct tables.          */
	/* on Berkeley, they are the same, so ld_str_tab = str_tab        */
	/* and ld_sym_tab = sym_tab.                                      */
	/******************************************************************/

	/**************************************************************/
	/* set a jmp_buf for a fatal error scanning the symbol table, */
	/* if the user has the flag nel_no_error_exit set.             */
	/**************************************************************/
	eng->stab->return_pt = &return_pt;
	if (setjmp (eng->stab->return_pt->buf) != 0) {
		retval = 0;
		stab_error (eng, "symbol table scan abandon");
	} else {
		/*************************/
		/* scan the symbol table */
		/*************************/

		nel_jmp_buf stab_err_jmp;

		/******************************************************/
		/* if we are debugging, save the value of nel_tracing, */
		/* so that we may restore it after scanning the stab. */
		/* it will be switched on for debug stab #'s that are */
		/* in the range  nel_stab_tstart <= n <= nel_stab_tend, */
		/* and for link stab #'s that are in the range        */
		/* -nel_stab_tstart <= n <= -nel_stab_tend.             */
		/******************************************************/
		//nel_debug ({
		//   nel_stab_old_tracing = nel_tracing;
		//   nel_tracing = 0;
		//});

		/***********************************************/
		/* reset all the apropriate struct nel_eng fields */
		/***********************************************/
		eng->stab->current_symbol = NULL;
		*(eng->stab->name_buffer) = '\0';
		eng->stab->N_TEXT_seen = 1;
		eng->stab->comp_unit = NULL;
		*(eng->stab->comp_unit_prefix) = '\0';
		eng->stab->source_lang = nel_L_NULL;
		eng->stab->current_file = NULL;
		eng->stab->incl_stack = NULL;
		eng->stab->incl_num = 0;
		eng->stab->incl_ct = 0;
		eng->stab->C_types_inserted = 0;
		//eng->stab->fortran_types_inserted = 0;
		eng->stab->common_block = NULL;
		eng->stab->first_arg = NULL;
		eng->stab->last_arg = NULL;
		eng->stab->first_local = NULL;
		eng->stab->last_local = NULL;
		eng->stab->first_block = NULL;
		eng->stab->last_block = NULL;
		eng->stab->first_include = NULL;
		eng->stab->last_include = NULL;
		eng->stab->first_routine = NULL;
		eng->stab->last_routine = NULL;
		eng->stab->first_static_global = NULL;
		eng->stab->last_static_global = NULL;

		/********************************************************/
		/* print a diagnostic message for the link symbol table */
		/********************************************************/
		stab_diagnostic (eng, "scanning %d link symbols", eng->stab->ld_sym_size);

		/****************************************************/
		/* scan the ld symbol table, inserting symbols into */
		/* the static location table. set the filename and  */
		/* line number for error messages.                  */
		/****************************************************/
		eng->stab->filename = nel_insert_name(eng, filename);
		eng->stab->line = 0;
		stab_ld_scan (eng);

		/*********************************************************/
		/* print a diagnostic message for the debug symbol table */
		/*********************************************************/
		stab_diagnostic (eng, "parsing %d debug symbols", eng->stab->sym_size);

		/******************************************************************/
		/* set a jump_buf for the nel_stab_fatal_err_jump, for errors      */
		/* where we abandon scanning the symbol table. because alloca'ing */
		/* tables modifies the local stack frame, we must perform this    */
		/* setjmp afterwards.  retuning to a setjmp that was set before   */
		/* alloca () was called causes a bus error on sparcs.             */
		/******************************************************************/
		eng->stab->block_err_jmp = &stab_err_jmp;
		if (setjmp (eng->stab->block_err_jmp->buf) != 0) {
			/**************************************/
			/* if we bombed on the n'th sting, we */
			/* sucessfully read only n-1 strings  */
			/**************************************/
#if 0
			if (eng->stab->sym_ct > 0) {
				retval = eng->stab->sym_ct - 1;
			} else {
				retval = 0;
			}
#endif
			stab_error (eng, "symbol table parsing abandon");
			goto post_stab;
		}

		/******************************************************************/
		/* when eng->stab->start_code is true, we know that we must goto   */
		/* a new string, so set it.  set eng->stab->sym_ct back to 0.      */
		/* we will immediately increment eng->stab->sym_scan, so set it to */
		/* just before the first symbol.  reset the filename and line #   */
		/* for error messages.  the scoping level starts out at 0.        */
		/******************************************************************/
		eng->stab->start_code = 1;
		eng->stab->sym_scan = eng->stab->last_sym = eng->stab->sym_tab - 1;
		eng->stab->sym_ct = 0;
		eng->stab->filename = nel_insert_name(eng, filename);
		eng->stab->line = 0;
		eng->stab->level = 0;

		/*******************/
		/* call the parser */
		/*******************/
		stab_parse (eng);

#if 0
		/******************************************************/
		/* eng->stab->incl_stack should either be have zero no */
		/* entries or one entry (last source file).  emit an  */
		/* error message if we did not properly exit from all */
		/* of the header files in the last compilation unit.  */
		/******************************************************/
		if ((eng->stab->incl_stack != NULL) && (eng->stab->incl_stack->next != NULL)) {
			stab_error (eng, "stab #%d: bad #include file nesting", eng->stab->sym_ct);
		}
#endif

		/****************************************/
		/* we have scanned the symbol table, or */
		/* abandon it and jumped ot here.       */
		/****************************************/
post_stab:
		;

		/******************************************************/
		/* eng->stab->sym_ct will be 1 more than the number of */
		/* symbols if the stab parse ran to completion .      */
		/* print a diagnostic with the # of symbols parsed.   */
		/******************************************************/
#if 0
		if (eng->stab->sym_ct > eng->stab->sym_size) {
			retval = eng->stab->sym_size;
		} else {
			retval = eng->stab->sym_ct;
		}

		/**********************************************/
		/* print a diagnostic message upon completion */
		/**********************************************/
		stab_diagnostic (eng, "%d debug symbols parsed", retval);
#else
		retval = eng->stab->error_ct;
#endif
		/***********************************************/
		/* reset eng->stab->filename and eng->stab->line */
		/* in case there is an error before we exit.   */
		/***********************************************/
		eng->stab->filename = nel_insert_name(eng, filename);
		eng->stab->line = 0;

		/***********************************/
		/* restore the value of nel_tracing */
		/***********************************/
		//nel_debug ({
		//   nel_tracing = nel_stab_old_tracing;
		//});

		/************************/
		/* clear the incl_stack */
		/************************/
		if (eng->stab->incl_stack != NULL) {
			nel_list *prev = NULL;
			while (eng->stab->incl_stack != NULL) {
				prev = eng->stab->incl_stack;
				eng->stab->incl_stack = eng->stab->incl_stack->next;
				nel_list_dealloc (prev);
			}
		}

		/***************************************/
		/* clear the stab hash table of the    */
		/* types in the last compilation unit. */
		/***************************************/
		stab_clear_type_hash (eng->stab->type_hash);

		/*************************************************/
		/* the predefined type names for the language of */
		/* the last source file will still be in nel's    */
		/* symbol table - remove them.                   */
		/*************************************************/
		if (eng->stab->C_types_inserted) {
			nel_remove_C_types (eng);
			eng->stab->C_types_inserted = 0;
		}

		/*
		if (eng->stab->fortran_types_inserted) {
			nel_remove_fortran_types (eng);
			eng->stab->fortran_types_inserted = 0;
		}
		*/

		/*****************************/
		/* close the executable file */
		/*****************************/
		fclose (file);

		/*******************************************************/
		/* remove all symbols with NULL value fields from the  */
		/* static_ident_hash table, and return them to the     */
		/* free list.                                          */
		/* we used to scan the debugging symbols and then the  */
		/* ld symbols, setting the values of global variables, */
		/* so it was possible that some debug symbols did not  */
		/* have corresponding corresponding ld symbol, and     */
		/* would have NULL value fields.  this shouldn't       */
		/* happen now, but this is a good precaution.          */
		/*******************************************************/
		nel_remove_NULL_values (eng, eng->nel_static_ident_hash, nel_static_symbol_dealloc);

		add_symbol_id_init(eng);
		nel_lib_init(eng);

	}


end:
	if(eng->stab_debug_level ) {
		nel_symbol *test_t_symbol; 
		//test_t_symbol = stab_lookup_ident(eng, "stream");
		test_t_symbol = nel_lookup_symbol("test_t", eng->stab->dyn_ident_hash,
                                   eng->nel_static_ident_hash,
                                   eng->stab->dyn_location_hash,
                                   eng->nel_static_location_hash, NULL);
		if (test_t_symbol != NULL) {
			printf("stab_file_parse, found test_t!\n") ;
			//nel_print_symbol(stab_debug_file, test_t_symbol, 0 ) ;
		}else {
			printf("stab_file_parse, not found test_t !\n") ;
		}
	}


	if (ld_str_tab != NULL) {
		nel_dealloca (ld_str_tab);
	}
	if (ld_sym_tab != NULL) {
		nel_dealloca (ld_sym_tab);
	}
	if (str_tab != NULL) {
		nel_dealloca (str_tab);
	}
	if (sym_tab != NULL) {
		nel_dealloca (sym_tab);
	}

	stab_dealloc(eng);

	nel_debug ({ 
		stab_trace(eng, "] exiting stab_file_parse\nretval = %d\n\n", retval); 
	});


#ifdef STAB_DEBUG
	if(eng->stab_debug_level ){
		if(stab_debug_file) fclose(stab_debug_file);
	}
#endif
	
	return (retval);

}


//calling nel_dealloca to free stab type chunks,
//affter doing this, all stab type pointer should be illegal!!!!!!!!!!!!1
void stabtype_dealloc(struct nel_eng *eng)
{
	while(nel_stab_type_chunks) {
		nel_stab_type_chunk *chunk = nel_stab_type_chunks->next;
		nel_dealloca(nel_stab_type_chunks->start); 
		nel_dealloca(nel_stab_type_chunks); 
		nel_stab_type_chunks = chunk;
	}
	nel_stab_types_end = NULL;
	nel_stab_types_next = NULL;
	nel_free_stab_types = NULL;
}
