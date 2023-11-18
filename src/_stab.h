
#ifndef NEL_STAB_H
#define NEL_STAB_H

#include "sym.h"


/*****************************************************************************/
/* turning tracing on while parsing the stab entries of the executable code  */
/* result in an enormous trace, so we toggle it off, except when the current */
/* stab #is between nel_stab_tstart and nel_stab_tend, inclusive.              */
/*****************************************************************************/
#ifdef NEL_DEBUG
extern int nel_stab_tstart;	/* tracing starts with this #string	*/
extern int nel_stab_tend;	/* tracing ends with this #string	*/
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
extern unsigned int nel_stab_check_redecs;


// add by wyong 
typedef struct elf_sym
{
        Elf32_Word	st_shndx;
        unsigned char st_type;
        unsigned char	st_other;
        Elf32_Half	st_desc;
        Elf32_Addr 	st_value;
} Elf_Sym;


/******************************************************************/
/* use the following macros to extract the name, type, value, and */
/* line number from a symbol table entry.  on ALLIANT_FX2800, the */
/* third lowest byte of the st_size field holds the type.         */
/******************************************************************/
#define stab_get_name(_stab)		((_stab)->st_shndx)
#define stab_get_type(_stab)		((_stab)->st_type )
#define stab_get_value(_stab)	((_stab)->st_value)
#define stab_get_line(_stab)		((_stab)->st_desc )




#include "engine.h"


struct eng_stab{
	/***************************************/
	/* generic structure - vars commmon to */
	/* both the interpreter and nel_stab () */
	/***************************************/
	int type;
	char *filename;
	int line;
	int error_ct;
	int tmp_int;
	struct nel_JMP_BUF *tmp_err_jmp;
	struct nel_JMP_BUF *return_pt;
	struct nel_JMP_BUF *stmt_err_jmp;
	struct nel_JMP_BUF *block_err_jmp;
	union nel_STACK *semantic_stack_start;
	union nel_STACK *semantic_stack_next;
	union nel_STACK *semantic_stack_end;
	int level;
	struct nel_SYMBOL *prog_unit;
	int block_no;
	int block_ct;
	unsigned_int clean_flag: 1;

	/********************************************/	
	/* vars for saving the state of the dynamic */
	/* name, symbol and value tables            */
	/* (also common to interpreter/nel_stab ())  */	
	/********************************************/
	char *dyn_values_start;
	char *dyn_values_next;
	char *dyn_values_end;
	struct nel_SYMBOL *dyn_symbols_start;
	struct nel_SYMBOL *dyn_symbols_next;
	struct nel_SYMBOL *dyn_symbols_end;
	struct nel_SYMBOL_TABLE *dyn_ident_hash;
	struct nel_SYMBOL_TABLE *dyn_location_hash;
	struct nel_SYMBOL_TABLE *dyn_tag_hash;
	struct nel_SYMBOL_TABLE *dyn_label_hash;
	union nel_STACK *context;
	/********************************************/
	/* the following four var maybe used in 
	interpreter mode, so put it here */
	/********************************************/
	unsigned_int declare_mode: 1;
	char *retval_loc;
	va_list *formal_ap;
	char *unit_prefix;

	/******************************************/
	/* variables used only by the stab parser */
	/******************************************/
	char *str_tab;
	int str_size;
	char *str_scan;

	
	Elf_Sym *sym_tab;
	int sym_size;
	Elf_Sym *sym_scan;
	Elf_Sym *last_sym;

	char *ld_str_tab;
	int ld_str_size;
	Elf_Sym *ld_sym_tab;
	int ld_sym_size;
	Elf_Sym *ld_sym_scan;


	int start_code;
	int sym_ct;
	int last_char;
	struct nel_SYMBOL *current_symbol;
	char *name_buffer;
	int N_TEXT_seen;
	struct nel_SYMBOL *comp_unit;
	char *comp_unit_prefix;
	nel_L_token source_lang;
	struct nel_SYMBOL *current_file;
	struct nel_LIST *incl_stack;
	struct nel_STAB_TYPE **type_hash;
	int incl_num;
	int incl_ct;
	unsigned_int C_types_inserted;
	unsigned_int fortran_types_inserted;
	struct nel_SYMBOL *common_block;
	struct nel_LIST *first_arg;
	struct nel_LIST *last_arg;
	struct nel_LIST *first_local;
	struct nel_LIST *last_local;
	struct nel_BLOCK *first_block;
	struct nel_BLOCK *last_block;
	struct nel_LIST *first_include;
	struct nel_LIST *last_include;
	struct nel_LIST *first_routine;
	struct nel_LIST *last_routine;
	struct nel_LIST *first_static_global;
	struct nel_LIST *last_static_global;
	char *block_start_address;
	char *block_end_address;
	struct nel_LINE **last_line;
};

struct nel_eng;
/*********************************************************/
/* for stab_setjmp, we also store the stab state.  */
/* because we have to restore several variables that are */
/* local to the parse routine, this macro must be called */
/* from within the body of a bison-generated stab.     */
/*********************************************************/
#define stab_setjmp(_eng,_val,_buffer) \
	{								\
	   (_eng)->stab->tmp_err_jmp = (_buffer);			\
	   (_eng)->stab->tmp_err_jmp->stmt_err_jmp = (_eng)->stab->stmt_err_jmp; \
	   (_eng)->stab->tmp_err_jmp->block_err_jmp = (_eng)->stab->block_err_jmp; \
	   stab_top_pointer ((_eng), (_eng)->stab->tmp_err_jmp->semantic_stack_next, 0); \
	   (_eng)->stab->tmp_err_jmp->level = (_eng)->stab->level;	\
	   (_eng)->stab->tmp_err_jmp->block_no = (_eng)->stab->block_no;\
	   (_eng)->stab->tmp_err_jmp->clean_flag=(_eng)->stab->clean_flag;\
	   (_eng)->stab->tmp_err_jmp->dyn_values_next = (_eng)->stab->dyn_values_next; \
	   (_eng)->stab->tmp_err_jmp->dyn_symbols_next = (_eng)->stab->dyn_symbols_next; \
	   (_eng)->stab->tmp_err_jmp->context = (_eng)->stab->context;	\
	   (_val) = setjmp ((_eng)->stab->tmp_err_jmp->buf);		\
	   if ((_val) != 0) {						\
	      (_eng)->stab->tmp_err_jmp = (_buffer);			\
	      (_eng)->stab->stmt_err_jmp = (_eng)->stab->tmp_err_jmp->stmt_err_jmp; \
	      (_eng)->stab->block_err_jmp = (_eng)->stab->tmp_err_jmp->block_err_jmp; \
	      stab_set_stack ((_eng), (_eng)->stab->tmp_err_jmp->semantic_stack_next); \
	      (_eng)->stab->level = (_eng)->stab->tmp_err_jmp->level;	\
	      (_eng)->stab->block_no = (_eng)->stab->tmp_err_jmp->block_no;	\
	      (_eng)->stab->clean_flag = (_eng)->stab->tmp_err_jmp->clean_flag; \
	      (_eng)->stab->dyn_values_next = (_eng)->stab->tmp_err_jmp->dyn_values_next; \
	      (_eng)->stab->dyn_symbols_next = (_eng)->stab->tmp_err_jmp->dyn_symbols_next; \
	      (_eng)->stab->context = (_eng)->stab->tmp_err_jmp->context;	\
	      nel_purge_table (_eng, (_eng)->stab->level + 1, (_eng)->stab->dyn_ident_hash); \
	      nel_purge_table (_eng, (_eng)->stab->level + 1, (_eng)->stab->dyn_location_hash); \
	      nel_purge_table (_eng, (_eng)->stab->level + 1, (_eng)->stab->dyn_tag_hash); \
	      nel_purge_table (_eng, (_eng)->stab->level + 1, (_eng)->stab->dyn_label_hash); \
	   }								\
	}

extern int stab_warning (struct nel_eng *, char *, ...);
extern int stab_error (struct nel_eng *, char *, ...);
extern int stab_stmt_error(struct nel_eng *, char *, ...);
extern int stab_block_error (struct nel_eng *, char *, ...);
extern int stab_fatal_error(struct nel_eng *, char *, ...);

/*******************************************************/
/* stuff for pushing arguments onto the semantic stack */
/*******************************************************/
#define stab_push_block(_eng,_q) { nel_debug({nel_trace(_eng,"pushing block {\n%1B\n",(_q));}); if ((_eng)->stab->semantic_stack_next >= (_eng)->stab->semantic_stack_end) stab_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->stab->semantic_stack_next))).block = (_q); }

#define stab_push_C_token(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing C_token %C {\n\n", (_q)); }); if ((_eng)->stab->semantic_stack_next >= (_eng)->stab->semantic_stack_end) stab_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->stab->semantic_stack_next))).C_token = (_q); }
#define stab_push_D_token(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing D_token %D {\n\n", (_q)); }); if ((_eng)->stab->semantic_stack_next >= (_eng)->stab->semantic_stack_end) stab_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->stab->semantic_stack_next))).D_token = (_q); }
#define stab_push_integer(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing integer %d {\n\n", (_q)); }); if ((_eng)->stab->semantic_stack_next >= (_eng)->stab->semantic_stack_end) stab_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->stab->semantic_stack_next))).integer = (_q); }
#define stab_push_list(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing list {\n%1L\n", (_q)); }); if ((_eng)->stab->semantic_stack_next >= (_eng)->stab->semantic_stack_end) stab_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->stab->semantic_stack_next))).list = (_q); }
#define stab_push_name(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing name %s {\n\n", (_q)); }); if ((_eng)->stab->semantic_stack_next >= (_eng)->stab->semantic_stack_end) stab_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->stab->semantic_stack_next))).name = (_q); }
#define stab_push_member(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing member {\n%1M\n", (_q)); }); if ((_eng)->stab->semantic_stack_next >= (_eng)->stab->semantic_stack_end) stab_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->stab->semantic_stack_next))).member = (_q); }
#define stab_push_stab_type(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing stab_type {\n%1U\n", (_q)); }); if ((_eng)->stab->semantic_stack_next >= (_eng)->stab->semantic_stack_end) stab_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->stab->semantic_stack_next))).stab_type = (_q); }
#define stab_push_symbol(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing symbol {\n%1S\n", (_q)); }); if ((_eng)->stab->semantic_stack_next >= (_eng)->stab->semantic_stack_end) stab_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->stab->semantic_stack_next))).symbol = (_q); }

#define stab_push_nel_symbol(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing nel symbol {\n%1S\n", (_q)); }); if ((_eng)->stab->semantic_stack_next >= (_eng)->stab->semantic_stack_end) stab_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->stab->semantic_stack_next))).nel_symbol = (_q); }

#define stab_push_rhs(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing nel rhs {\n%1S\n", (_q)); }); if ((_eng)->stab->semantic_stack_next >= (_eng)->stab->semantic_stack_end) stab_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->stab->semantic_stack_next))).nel_rhs = (_q); }

#define stab_push_type(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing type {\n%1T\n", (_q)); }); if ((_eng)->stab->semantic_stack_next >= (_eng)->stab->semantic_stack_end) stab_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->stab->semantic_stack_next))).type = (_q); }


#define stab_push_value(_eng,_q,_typ)		{ nel_debug ({ nel_trace (_eng, "pushing value 0x%x {\n\n", (_q)); }); if ((_eng)->stab->semantic_stack_next >= (_eng)->stab->semantic_stack_end) stab_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->stab->semantic_stack_next))).integer = (int) (_q); }
#define stab_push_expr(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing expr {\n%1X\n", (_q)); }); if ((_eng)->stab->semantic_stack_next >= (_eng)->stab->semantic_stack_end) stab_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->stab->semantic_stack_next))).expr = (_q); }
#define stab_push_expr_list(_eng,_q)	{ nel_debug ({ nel_trace (_eng, "pushing expr_list {\n%1Y\n", (_q)); }); if ((_eng)->stab->semantic_stack_next >= (_eng)->stab->semantic_stack_end) stab_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->stab->semantic_stack_next))).expr_list = (_q); }
#define stab_push_stmt(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing stmt {\n%1K\n", (_q)); }); if ((_eng)->stab->semantic_stack_next >= (_eng)->stab->semantic_stack_end) stab_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->stab->semantic_stack_next))).stmt = (_q); }
#define stab_push_token(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing token %N {\n\n", (_q)); }); if ((_eng)->stab->semantic_stack_next >= (_eng)->stab->semantic_stack_end) stab_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->stab->semantic_stack_next))).token = (_q); }


/*******************************************************/
/* stuff for popping arguments from the semantic stack */
/*******************************************************/
#define stab_pop_block(_eng,_q)		{ (_q) = (*(((_eng)->stab->semantic_stack_next)--)).block; nel_debug ({ nel_trace (_eng, "} popping block\n%1B\n", (_q)); }); }
#define stab_pop_C_token(_eng,_q)		{ (_q) = (*(((_eng)->stab->semantic_stack_next)--)).C_token; nel_debug ({ nel_trace (_eng, "} popping C_token %C\n\n", (_q)); }); }
#define stab_pop_D_token(_eng,_q)		{ (_q) = (*(((_eng)->stab->semantic_stack_next)--)).D_token; nel_debug ({ nel_trace (_eng, "} popping D_token %D\n\n", (_q)); }); }
#define stab_pop_integer(_eng,_q)		{ (_q) = (*(((_eng)->stab->semantic_stack_next)--)).integer; nel_debug ({ nel_trace (_eng, "} popping integer %d\n\n", (_q)); }); }
#define stab_pop_list(_eng,_q)			{ (_q) = (*(((_eng)->stab->semantic_stack_next)--)).list; nel_debug ({ nel_trace (_eng, "} popping list\n%1L\n", (_q)); }); }
#define stab_pop_name(_eng,_q)			{ (_q) = (*(((_eng)->stab->semantic_stack_next)--)).name; nel_debug ({ nel_trace (_eng, "} popping name %s\n\n", (_q)); }); }
#define stab_pop_member(_eng,_q)		{ (_q) = (*(((_eng)->stab->semantic_stack_next)--)).member; nel_debug ({ nel_trace (_eng, "} popping member\n%1M\n", (_q)); }); }
#define stab_pop_stab_type(_eng,_q)		{ (_q) = (*(((_eng)->stab->semantic_stack_next)--)).stab_type; nel_debug ({ nel_trace (_eng, "} popping stab_type\n%1U\n", (_q)); }); }
#define stab_pop_symbol(_eng,_q)		{ (_q) = (*(((_eng)->stab->semantic_stack_next)--)).symbol; nel_debug ({ nel_trace (_eng, "} popping symbol\n%1S\n", (_q)); }); }

#define stab_pop_rhs(_eng,_q)		{ (_q) = (*(((_eng)->stab->semantic_stack_next)--)).nel_rhs; nel_debug ({ nel_trace (_eng, "} popping nel rhs \n%1S\n", (_q)); }); }
#define stab_pop_type(_eng,_q)			{ (_q) = (*(((_eng)->stab->semantic_stack_next)--)).type; nel_debug ({ nel_trace (_eng, "} popping type\n%1T\n", (_q)); }); }
#define stab_pop_value(_eng,_q,_typ)		{ (_q) = (_typ) (*(((_eng)->stab->semantic_stack_next)--)).integer; nel_debug ({ nel_trace (_eng, "} popping value 0x%x\n\n", (_q)); }); }
#define stab_pop_expr(_eng,_q)		{ (_q) = (*(((_eng)->stab->semantic_stack_next)--)).expr; nel_debug ({ nel_trace (_eng, "} popping expr\n%1X\n", (_q)); }); }
#define stab_pop_expr_list(_eng,_q)	{ (_q) = (*(((_eng)->stab->semantic_stack_next)--)).expr_list; nel_debug ({ nel_trace (_eng, "} popping expr_list\n%1Y\n", (_q)); }); }
#define stab_pop_stmt(_eng,_q)		{ (_q) = (*(((_eng)->stab->semantic_stack_next)--)).stmt; nel_debug ({ nel_trace (_eng, "} popping stmt\n%1K\n", (_q)); }); }
#define stab_pop_token(_eng,_q)		{ (_q) = (*(((_eng)->stab->semantic_stack_next)--)).token; nel_debug ({ nel_trace (_eng, "} popping token %N\n\n", (_q)); }); }



/****************************************************************/
/* stuff for "tushing" arguments onto the semantic stack, i.e., */
/* assigning a value to a stack element i places from the top.  */
/****************************************************************/
#define stab_tush_block(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing block (n = %d)\n%1B\n", (_i), (_q)); }); (*((_eng)->stab->semantic_stack_next-(_i))).block = (_q); }
#define stab_tush_C_token(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing C_token (n = %d) %C\n\n", (_i), (_q)); }); (*((_eng)->stab->semantic_stack_next-(_i))).C_token = (_q); }
#define stab_tush_D_token(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing D_token (n = %d) %D\n\n", (_i), (_q)); }); (*((_eng)->stab->semantic_stack_next-(_i))).D_token = (_q); }
#define stab_tush_integer(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing integer (n = %d) %d\n\n", (_i), (_q)); }); (*((_eng)->stab->semantic_stack_next-(_i))).integer = (_q); }
#define stab_tush_list(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing list (n = %d)\n%1L\n", (_i), (_q)); }); (*((_eng)->stab->semantic_stack_next-(_i))).list = (_q); }
#define stab_tush_name(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing name (n = %d) %s\n\n", (_i), (_q)); }); (*((_eng)->stab->semantic_stack_next-(_i))).name = (_q); }
#define stab_tush_member(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing member (n = %d)\n%1M\n", (_i), (_q)); }); (*((_eng)->stab->semantic_stack_next-(_i))).member = (_q); }
#define stab_tush_stab_type(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing stab_type (n = %d)\n%1U\n", (_i), (_q)); }); (*((_eng)->stab->semantic_stack_next-(_i))).stab_type = (_q); }
#define stab_tush_symbol(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing symbol (n = %d)\n%1S\n", (_i), (_q)); }); (*((_eng)->stab->semantic_stack_next-(_i))).symbol = (_q); }


#define stab_tush_rhs(_eng,_q,_i)	{ nel_debug ({ nel_trace (_eng, "tushing nel rhs (n = %d)\n%1S\n", (_i), (_q)); }); (*((_eng)->stab->semantic_stack_next-(_i))).nel_rhs = (_q); }

#define stab_tush_type(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing type (n = %d)\n%1T\n", (_i), (_q)); }); (*((_eng)->stab->semantic_stack_next-(_i))).type = (_q); }
#define stab_tush_value(_eng,_q,_typ,_i)		{ nel_debug ({ nel_trace (_eng, "tushing value (n = %d) 0x%x\n\n", (_i), (_q)); }); (*((_eng)->stab->semantic_stack_next-(_i))).integer = (int) (_q); }

#define stab_tush_expr(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing expr (n = %d)\n%1X\n", (_i), (_q)); }); (*((_eng)->stab->semantic_stack_next-(_i))).expr = (_q); }
#define stab_tush_expr_list(_eng,_q,_i)	{ nel_debug ({ nel_trace (_eng, "tushing expr_list (n = %d)\n%1Y\n", (_i), (_q)); }); (*((_eng)->stab->semantic_stack_next-(_i))).expr_list = (_q); }
#define stab_tush_stmt(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing stmt (n = %d)\n%1K\n", (_i), (_q)); }); (*((_eng)->stab->semantic_stack_next-(_i))).stmt = (_q); }
#define stab_tush_token(_eng,_q,_i)	{ nel_debug ({ nel_trace (_eng, "tushing token (n = %d) %N\n\n", (_i), (_q)); }); (*((_eng)->stab->semantic_stack_next-(_i))).token = (_q); }



/******************************************************************/
/* stuff for "topping" arguments from the semantic stack, i.e.,   */
/* retreiving a value from a stack element i places from the top. */
/******************************************************************/
#define stab_top_block(_eng,_q,_i)		{ (_q) = (*((_eng)->stab->semantic_stack_next-(_i))).block; nel_debug ({ nel_trace (_eng, "topping block (n = %d)\n%1B\n", (_i), (_q)); }); }
#define stab_top_C_token(_eng,_q,_i)		{ (_q) = (*((_eng)->stab->semantic_stack_next-(_i))).C_token; nel_debug ({ nel_trace (_eng, "topping C_token (n = %d) %C\n\n", (_i), (_q)); }); }
#define stab_top_D_token(_eng,_q,_i)		{ (_q) = (*((_eng)->stab->semantic_stack_next-(_i))).D_token; nel_debug ({ nel_trace (_eng, "topping D_token (n = %d) %D\n\n", (_i), (_q)); }); }
#define stab_top_integer(_eng,_q,_i)		{ (_q) = (*((_eng)->stab->semantic_stack_next-(_i))).integer; nel_debug ({ nel_trace (_eng, "topping integer (n = %d) %d\n\n", (_i), (_q)); }); }
#define stab_top_list(_eng,_q,_i)		{ (_q) = (*((_eng)->stab->semantic_stack_next-(_i))).list; nel_debug ({ nel_trace (_eng, "topping list (n = %d)\n%1L\n", (_i), (_q)); }); }
#define stab_top_name(_eng,_q,_i)		{ (_q) = (*((_eng)->stab->semantic_stack_next-(_i))).name; nel_debug ({ nel_trace (_eng, "topping name (n = %d) %s\n\n", (_i), (_q)); }); }
#define stab_top_member(_eng,_q,_i)		{ (_q) = (*((_eng)->stab->semantic_stack_next-(_i))).member; nel_debug ({ nel_trace (_eng, "topping member (n = %d)\n%1M\n", (_i), (_q)); }); }
#define stab_top_stab_type(_eng,_q,_i)		{ (_q) = (*((_eng)->stab->semantic_stack_next-(_i))).stab_type; nel_debug ({ nel_trace (_eng, "topping stab_type (n = %d)\n%1U\n", (_i), (_q)); }); }
#define stab_top_symbol(_eng,_q,_i)		{ (_q) = (*((_eng)->stab->semantic_stack_next-(_i))).symbol; nel_debug ({ nel_trace (_eng, "topping symbol (n = %d)\n%1S\n", (_i), (_q)); }); }

#define stab_top_nel_symbol(_eng,_q,_i)		{ (_q) = (*((_eng)->stab->semantic_stack_next-(_i))).nel_symbol; nel_debug ({ nel_trace (_eng, "topping nel symbol (n = %d)\n%1S\n", (_i), (_q)); }); }

#define stab_top_rhs(_eng,_q,_i)		{ (_q) = (*((_eng)->stab->semantic_stack_next-(_i))).nel_rhs; nel_debug ({ nel_trace (_eng, "topping nel rhs(n = %d)\n%1S\n", (_i), (_q)); }); }

#define stab_top_type(_eng,_q,_i)		{ (_q) = (*((_eng)->stab->semantic_stack_next-(_i))).type; nel_debug ({ nel_trace (_eng, "topping type (n = %d)\n%1T\n", (_i), (_q)); }); }
#define stab_top_value(_eng,_q,_typ,_i)		{ (_q) = (_typ) (*((_eng)->stab->semantic_stack_next-(_i))).integer; nel_debug ({ nel_trace (_eng, "topping value (n = %d) 0x%x\n\n", (_i), (_q)); }); }

#define stab_top_expr(_eng,_q,_i)		{ (_q) = (*((_eng)->stab->semantic_stack_next-(_i))).expr; nel_debug ({ nel_trace (_eng, "topping expr (n = %d)\n%1X\n", (_i), (_q)); }); }
#define stab_top_expr_list(_eng,_q,_i)	{ (_q) = (*((_eng)->stab->semantic_stack_next-(_i))).expr_list; nel_debug ({ nel_trace (_eng, "topping expr_list (n = %d)\n%1Y\n", (_i), (_q)); }); }
#define stab_top_stmt(_eng,_q,_i)		{ (_q) = (*((_eng)->stab->semantic_stack_next-(_i))).stmt; nel_debug ({ nel_trace (_eng, "topping stmt (n = %d)\n%1K\n", (_i), (_q)); }); }
#define stab_top_token(_eng,_q,_i)		{ (_q) = (*((_eng)->stab->semantic_stack_next-(_i))).token; nel_debug ({ nel_trace (_eng, "topping token (n = %d) %N\n\n", (_i), (_q)); }); }
/**************************************************************************/
/* stab_top_pointer (_eng,) returns a pointer to a stack element (i) places down */
/**************************************************************************/
#define stab_top_pointer(_eng,_q,_i)		{ (_q) = ((_eng)->stab->semantic_stack_next-(_i)); nel_debug ({ nel_trace (_eng, "topping pointer (n = %d) 0x%x\n\n", (_i), (_q)); }); }



/************************************************************************/
/* stab_set_stack sets the semantic stack pointer directly            */
/* if nel_tracing is on, make sure the {'s and }'s line up in the trace */
/************************************************************************/
#define stab_set_stack(_eng,_q) \
	{								\
	   nel_debug ({							\
	      nel_trace (_eng, "setting pointer 0x%x\n\n", (_q));	\
	      while ((_eng)->stab->semantic_stack_next > (_q)) {	\
	         nel_trace (_eng, "}\n\n");				\
	         (_eng)->stab->semantic_stack_next--;			\
	      }								\
	      while ((_eng)->stab->semantic_stack_next < (_q)) {	\
	         nel_trace (_eng, "{\n\n");				\
	         (_eng)->stab->semantic_stack_next++;			\
	      }								\
	   });								\
	   (_eng)->stab->semantic_stack_next = (nel_stack *) (_q);	\
	}



void stab_insert_ident(struct nel_eng *eng, nel_symbol *symbol);
void stab_remove_ident(struct nel_eng *eng,  nel_symbol *symbol);
nel_symbol *stab_lookup_ident(struct nel_eng *eng, char *name);

/***********************************************************/
/* when searching for a location (ld symbol table entry),  */
/* check only the location hash tables, and skip the ident */
/* hash tables - they may have a symbol by the same name,  */
/* masking the entry.                                      */
/***********************************************************/
#define stab_lookup_location(_eng,_name)	\
	nel_lookup_symbol ((_name), (_eng)->stab->dyn_location_hash, (_eng)->nel_static_location_hash, NULL)
#define stab_insert_location(_eng,_symbol) \
	((((_symbol)->level <= 0) && ((_symbol)->class != nel_C_STATIC_LOCATION)) ? \
	   nel_insert_symbol ((_eng),(_symbol), (_eng)->nel_static_location_hash) :	\
	   nel_insert_symbol ((_eng),(_symbol), (_eng)->stab->dyn_location_hash)	\
	)
#define stab_remove_location(_eng,_symbol) \
	nel_remove_symbol (_eng, (_symbol))


#define stab_lookup_tag(_eng,_name) \
	nel_lookup_symbol ((_name), (_eng)->stab->dyn_tag_hash, (_eng)->nel_static_tag_hash, NULL)
#define stab_insert_tag(_eng,_symbol) \
	(((_symbol)->level <= 0) ?					\
	   nel_insert_symbol ((_eng),(_symbol), (_eng)->nel_static_tag_hash):\
	   nel_insert_symbol ((_eng), (_symbol), (_eng)->stab->dyn_tag_hash)\
	)
#define stab_remove_tag(_eng,_symbol) \
	nel_remove_symbol (_eng, (_symbol))


#define stab_lookup_label(_eng,_name) \
	nel_lookup_symbol ((_name), (_eng)->stab->dyn_label_hash, NULL)
#define stab_insert_label(_eng,_symbol) \
	nel_insert_symbol ((_eng),(_symbol), (_eng)->stab->dyn_label_hash)
#define stab_remove_label(_eng,_symbol) \
	nel_remove_symbol (_eng, (_symbol))

#define stab_lookup_file(_eng,_name) \
	nel_lookup_symbol ((_name), (_eng)->nel_static_file_hash, NULL)
#define stab_insert_file(_eng,_symbol) \
	nel_insert_symbol ((_eng), (_symbol), (_eng)->nel_static_file_hash)
#define stab_remove_file(_eng,_symbol) \
	nel_remove_symbol (_eng, _symbol)

/********************************************************/
/* declarations for the routines defined in "nel_stab.c" */
/********************************************************/
//int nel_stab (char *);
//int nel_stab_yylex (struct nel_eng *);


/*************************************************/
/* declarations for the nel_stab_type hash tables */
/*************************************************/
#define nel_stab_type_hash_max		0x0800



/***************************************************************/
/* use the following macro to allocate space for the stab type */
/* hash table, and initialize its chains to NULL.              */
/* use alloca() to allocate space on the stack.                */
/***************************************************************/
#define stab_type_hash_alloc(_table,_max) \
	{								\
	   register unsigned_int __max = (_max);			\
	   register int __i;						\
	   register nel_stab_type **__scan;				\
	   nel_alloca ((_table), __max, nel_stab_type *);			\
	   for (__i = 0, __scan = (_table); (__i < __max); __i++, *(__scan++) = NULL); \
	}

#define stab_type_hash_dealloc(_table) \
	nel_dealloca (_table);
void stab_clear_type_hash (register struct nel_STAB_TYPE **);
int stab_type_hash (int, int, int);



struct nel_STAB_TYPE *stab_lookup_type (register int, register int, struct nel_STAB_TYPE **, int);
struct nel_STAB_TYPE *stab_insert_type (struct nel_eng *, register struct nel_STAB_TYPE *, register struct nel_STAB_TYPE **, int);
//#define stab_lookup_type_2 (eng,file_num,type_num) stab_lookup_type (file_num, type_num, eng->stab->type_hash, nel_stab_type_hash_max)

struct nel_STAB_TYPE *stab_lookup_type_2 (struct nel_eng *, register int, register int );
struct nel_STAB_TYPE *stab_insert_type_2 (struct nel_eng *, register struct nel_STAB_TYPE *);



extern struct nel_STAB_TYPE *stab_type_alloc (struct nel_eng *, union nel_TYPE *, int, int);
extern void stab_type_dealloc (register struct nel_STAB_TYPE *);
extern void stab_print_type (FILE *, register struct nel_STAB_TYPE *, int);
extern void stab_print_types (FILE *, register struct nel_STAB_TYPE*, int);


/************************************************************/
/* declarations for the routines defined in nel_stab_parse.y */
/************************************************************/
//extern int nel_stab_yyparse (struct nel_eng *);
int stab_read_name (struct nel_eng *, char *, char **, register int);
void stab_ld_scan (struct nel_eng *);
extern void stab_global_acts (struct nel_eng *, register struct nel_SYMBOL *, register union nel_TYPE *);
extern void stab_static_global_acts (struct nel_eng *, register struct nel_SYMBOL *, register union nel_TYPE *);
extern void stab_local_acts (struct nel_eng *, register struct nel_SYMBOL *, register union nel_TYPE *);
extern void stab_register_local_acts (struct nel_eng *, register struct nel_SYMBOL *, register union nel_TYPE *);
extern void stab_static_local_acts (struct nel_eng *, register struct nel_SYMBOL *, register union nel_TYPE *);
extern void stab_parameter_acts (struct nel_eng *, register struct nel_SYMBOL *, register union nel_TYPE *, register char *, register unsigned_int);
extern void stab_routine_acts (struct nel_eng *, register struct nel_SYMBOL *, register union nel_TYPE *);
extern void stab_static_routine_acts (struct nel_eng *, register struct nel_SYMBOL *, register union nel_TYPE *);
extern void stab_start_routine (struct nel_eng *, register struct nel_SYMBOL *);
extern void stab_end_routine (struct nel_eng *);
extern unsigned_int stab_in_routine (struct nel_eng *);


extern int parse_stab_type_number (struct nel_eng *eng, const char **pp, int *typenums);
extern int parse_stab_type(struct nel_eng *eng, const char *typename, const char **pp);
extern int parse_number (const char **pp);
extern int parse_stab_type_ex(struct nel_eng *eng, const char *typename, const char **pp);


extern void nel_stab_init(struct nel_eng *_eng, char *_filename);
extern void nel_stab_dealloc(struct nel_eng *_eng);
extern int stab_file_parse(struct nel_eng *, char *);

//added by zhangbin, 2006-7-19
void stabtype_dealloc(struct nel_eng *eng);
//end
#endif
