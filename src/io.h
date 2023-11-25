
/*****************************************************************************/
/* This file, "nel_i_o.h", contains declarations for the routines, any        */
/* #define statements, and extern declarations for any global variables that */
/* are defined in "nel_i_o.c".                                                */
/*****************************************************************************/



#ifndef _NEL_I_O_
#define _NEL_I_O_

#include "type.h"

struct nel_SYMBOL;
struct nel_BLOCK;
struct nel_DIM;
struct nel_LIST;
struct nel_MEMBER;
union nel_TYPE;
struct nel_STAB_TYPE;




/*****************************************/
/* global variables defined in "nel_i_o.c" */
/* that may be referenced externally     */
/*****************************************/
extern int verbose_level;
//extern unsigned_int nel_silent;		/* repress diagnostic messages	*/
extern int nel_stderr_backspaces;	/* # chars to backspace before printing	*/

#ifdef NEL_DEBUG
extern int debug_level;
//extern unsigned_int nel_tracing;	/* tracing flag			*/
extern int nel_stdout_backspaces;	/* #chars to backspace before printing*/
#endif /* NEL_DEBUG */



/*********************************************************************/
/* when nel_print_brief_types is nonzero, types descriptors (printed  */
/* with " %T") for structures and unions do not have their members   */
/* listed, and type descriptors for enumerated types do not have the */
/* constant list printed.                                            */
/*********************************************************************/
extern unsigned_int nel_print_brief_types;



/*******************************************************/
/* declarations for the routines defined in "nel_i_o.c" */
/*******************************************************/
extern void nel_do_print (FILE *, char *, va_list); 
extern char *rel2abs(const char *path, const char *base, char *result, int size);
extern int nel_make_file_prefix (struct nel_eng *, register char *, register char *, register int);


#endif /* _NEL_I_O_ */

