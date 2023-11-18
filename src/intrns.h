
/*****************************************************************************/
/* This file, "ntrns.h", contains declarations for the routines, any */
/* #define statements, and extern declarations for any global variables that */
/* are defined in "lib.c".                                         */
/*****************************************************************************/



#ifndef LIB_H 
#define LIB_H 



struct nel_SYMBOL;
union nel_STACK;
struct nel_eng;



/***************************************************************/
/* declarations for the routines defined in "lib.c". */
/***************************************************************/
extern void nel_pretty_print (FILE *, register union nel_TYPE *, char *, int);

extern struct nel_SYMBOL *nel_ntrn_cmai (struct nel_eng *, register struct nel_SYMBOL *, register int, register union nel_STACK *);
extern struct nel_SYMBOL *nel_ntrn_getrec (struct nel_eng *, register struct nel_SYMBOL *, register int, register union nel_STACK *);
extern struct nel_SYMBOL *nel_ntrn_print (struct nel_eng *, register struct nel_SYMBOL *, register int, register union nel_STACK *);
extern struct nel_SYMBOL *nel_ntrn_symbolof (struct nel_eng *, register struct nel_SYMBOL *, register int, register union nel_STACK *);
extern struct nel_SYMBOL *nel_ntrn_NEL_ID_OF(struct nel_eng *, register struct nel_SYMBOL *, register int, register union nel_STACK *);
extern void nel_ntrn_init (struct nel_eng *eng);
int nel_func_init(struct nel_eng *eng);


#endif /* LIB_H */

