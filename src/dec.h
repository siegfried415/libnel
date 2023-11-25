
/*****************************************************************************/
/* This file, "dec.h", contains declarations for the routines, any */
/* #define statements, and extern declarations for any global variables that */
/* are defined in "dec.c".  							*/
/*****************************************************************************/



#ifndef DEC_H 
#define DEC_H 


/***************************************************************************/
/* when nel_static_global_equivs is true and the parser encounters a   */
/* declaration for a static global (file-local) variable <var_name>, it    */
/* looks for a non-static global var by the name <file_name>`<var_name>,   */
/* where <file_name> has _'s substituted in for characters not allowed in  */
/* C identifiers, and prepended with a _ if the name begins with a digit.  */
/* if it is found (and has the same type), then the static var assumes the */
/* same storage location as the non-static var.  if the non-static var is  */
/* not found, then it is created, and both vars assume the same storage.   */
/***************************************************************************/
extern unsigned_int nel_static_global_equivs;



/*************************************************************************/
/* when nel_static_local_equivs is true and the parser encounters a  */
/* declaration for a static local variable <var_name>, it looks for a    */
/* global var (or a static global var if the function we are in was also */
/* declared static) by the name <function_name>`<block_no>`<var_name>.   */
/* <block_no> is omitted if it is 0 (the outermost block).  if it is     */
/* found (and has the same type), then the local var assumes the same    */
/* storage location as the global var.  if the global var is not found,  */
/* then it is created, and both vars assume the same storage.            */
/*************************************************************************/
extern unsigned_int nel_static_local_equivs;



/************************************************************/
/* declarations for the routines defined in dec.c */
/************************************************************/
nel_symbol *nel_global_def (struct nel_eng *eng, register char *name, register nel_type *type, register unsigned_int static_class, register char *address, unsigned_int compiled);

nel_symbol *nel_global_dec (struct nel_eng *eng, register char *name, register nel_type *type, register unsigned_int static_class);

nel_symbol *nel_local_dec(struct nel_eng *eng, register char *name, register nel_type *type, register unsigned_int static_class);

nel_symbol *nel_func_dec (struct nel_eng *eng, register char *name, register nel_type *type, register unsigned_int static_class);

nel_symbol *nel_evt_dec(struct nel_eng *eng, char *name, nel_type *type, unsigned_int class );
nel_symbol *nel_dec (struct nel_eng *eng, register char *name, register nel_type *type, register int stor_class);

int nel_global_init(struct nel_eng *eng, nel_symbol *sym1, nel_expr *expr2);
int nel_dec_init(struct nel_eng *eng, nel_symbol *sym, nel_expr *expr);
int nel_s_u_init(struct nel_eng *eng, nel_symbol *sym1, nel_expr *expr2);

#endif /* DEC_H */

