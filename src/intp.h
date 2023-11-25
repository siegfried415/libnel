
#ifndef EVAL_H
#define EVAL_H

#include "errors.h"
#include "expr.h"
#include "stmt.h"
#include "sym.h"
#include "type.h"

/*****************************************************/
/* we may have at most 30 words in the argument list */
/* (in addition to args passed in registers, if any) */
/*****************************************************/
#define nel_MAX_ARGS		30


struct eng_intp{

	/***************************************/
	/* generic structure - vars commmon to */
	/* both the interpreter and nel_stab () */
	/***************************************/
	nel_R_token type;
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
	nel_type *ret_type;
};


/*************************************************************************/
/* for intp_setjmp, store the information in the gen variant of the engine */
/* , and then call setjmp () to store the state of the run-time    */
/* stack (and the pc).  then restore the state parser state if a nonzero */
/* value is returned from setjmp (), because longjmp () will return to   */
/* just after where setjmp () was called.  we make sure to use temp      */
/* variables allocated in the engine.  any temporaries allocated in */
/* the block comprising the body of the macro can be legally deallocated */
/* upon exit of the block, and storage for them need not exist when      */
/* longjmping back.                                                      */
/*************************************************************************/
#define intp_setjmp(_eng,_val,_buffer) \
	{								\
	   (_eng)->intp->tmp_err_jmp = (_buffer);				\
	   (_eng)->intp->tmp_err_jmp->stmt_err_jmp = (_eng)->intp->stmt_err_jmp; \
	   (_eng)->intp->tmp_err_jmp->block_err_jmp = (_eng)->intp->block_err_jmp; \
	   intp_top_pointer ((_eng), (_eng)->intp->tmp_err_jmp->semantic_stack_next, 0); \
	   (_eng)->intp->tmp_err_jmp->level = (_eng)->intp->level;		\
	   (_eng)->intp->tmp_err_jmp->block_no = (_eng)->intp->block_no;	\
	   (_eng)->intp->tmp_err_jmp->clean_flag = (_eng)->intp->clean_flag; \
	   (_eng)->intp->tmp_err_jmp->dyn_values_next = (_eng)->intp->dyn_values_next; \
	   (_eng)->intp->tmp_err_jmp->dyn_symbols_next = (_eng)->intp->dyn_symbols_next; \
	   (_eng)->intp->tmp_err_jmp->context = (_eng)->intp->context;	\
	   (_val) = setjmp ((_eng)->intp->tmp_err_jmp->buf);		\
	   if ((_val) != 0) {						\
	      (_eng)->intp->tmp_err_jmp = (_buffer);			\
	      (_eng)->intp->stmt_err_jmp = (_eng)->intp->tmp_err_jmp->stmt_err_jmp; \
	      (_eng)->intp->block_err_jmp = (_eng)->intp->tmp_err_jmp->block_err_jmp; \
	      intp_set_stack ((_eng), (_eng)->intp->tmp_err_jmp->semantic_stack_next); \
	      (_eng)->intp->level = (_eng)->intp->tmp_err_jmp->level;	\
	      (_eng)->intp->block_no = (_eng)->intp->tmp_err_jmp->block_no;	\
	      (_eng)->intp->clean_flag = (_eng)->intp->tmp_err_jmp->clean_flag; \
	      (_eng)->intp->dyn_values_next = (_eng)->intp->tmp_err_jmp->dyn_values_next; \
	      (_eng)->intp->dyn_symbols_next = (_eng)->intp->tmp_err_jmp->dyn_symbols_next; \
	      (_eng)->intp->context = (_eng)->intp->tmp_err_jmp->context;	\
	      nel_purge_table (_eng, (_eng)->intp->level + 1, (_eng)->intp->dyn_ident_hash); \
	      nel_purge_table (_eng, (_eng)->intp->level + 1, (_eng)->intp->dyn_location_hash); \
	      nel_purge_table (_eng, (_eng)->intp->level + 1, (_eng)->intp->dyn_tag_hash); \
	      nel_purge_table (_eng, (_eng)->intp->level + 1, (_eng)->intp->dyn_label_hash); \
	   }								\
	}

extern int intp_warning (struct nel_eng *, char *, ...);
extern int intp_error (struct nel_eng *, char *, ...);
extern int intp_stmt_error(struct nel_eng *, char *, ...);
extern int intp_block_error (struct nel_eng *, char *, ...);
extern int intp_fatal_error(struct nel_eng *, char *, ...);

/*******************************************************/
/* stuff for pushing arguments onto the semantic stack */
/*******************************************************/
#define intp_push_block(_eng,_q) { nel_debug({nel_trace(_eng,"pushing block {\n%1B\n",(_q));}); if ((_eng)->intp->semantic_stack_next >= (_eng)->intp->semantic_stack_end) intp_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->intp->semantic_stack_next))).block = (_q); }
#define intp_push_C_token(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing C_token %C {\n\n", (_q)); }); if ((_eng)->intp->semantic_stack_next >= (_eng)->intp->semantic_stack_end) intp_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->intp->semantic_stack_next))).C_token = (_q); }
#define intp_push_D_token(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing D_token %D {\n\n", (_q)); }); if ((_eng)->intp->semantic_stack_next >= (_eng)->intp->semantic_stack_end) intp_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->intp->semantic_stack_next))).D_token = (_q); }
#define intp_push_integer(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing integer %d {\n\n", (_q)); }); if ((_eng)->intp->semantic_stack_next >= (_eng)->intp->semantic_stack_end) intp_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->intp->semantic_stack_next))).integer = (_q); }
#define intp_push_list(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing list {\n%1L\n", (_q)); }); if ((_eng)->intp->semantic_stack_next >= (_eng)->intp->semantic_stack_end) intp_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->intp->semantic_stack_next))).list = (_q); }
#define intp_push_name(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing name %s {\n\n", (_q)); }); if ((_eng)->intp->semantic_stack_next >= (_eng)->intp->semantic_stack_end) intp_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->intp->semantic_stack_next))).name = (_q); }
#define intp_push_member(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing member {\n%1M\n", (_q)); }); if ((_eng)->intp->semantic_stack_next >= (_eng)->intp->semantic_stack_end) intp_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->intp->semantic_stack_next))).member = (_q); }
#define intp_push_stab_type(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing stab_type {\n%1U\n", (_q)); }); if ((_eng)->intp->semantic_stack_next >= (_eng)->intp->semantic_stack_end) intp_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->intp->semantic_stack_next))).stab_type = (_q); }
#define intp_push_symbol(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing symbol {\n%1S\n", (_q)); }); if ((_eng)->intp->semantic_stack_next >= (_eng)->intp->semantic_stack_end) intp_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->intp->semantic_stack_next))).symbol = (_q); }

//#define intp_push_nel_symbol(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing nel symbol {\n%1S\n", (_q)); }); if ((_eng)->intp->semantic_stack_next >= (_eng)->intp->semantic_stack_end) intp_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->intp->semantic_stack_next))).nel_symbol = (_q); }

#define intp_push_rhs(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing nel rhs {\n%1S\n", (_q)); }); if ((_eng)->intp->semantic_stack_next >= (_eng)->intp->semantic_stack_end) intp_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->intp->semantic_stack_next))).nel_rhs = (_q); }

#define intp_push_type(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing type {\n%1T\n", (_q)); }); if ((_eng)->intp->semantic_stack_next >= (_eng)->intp->semantic_stack_end) intp_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->intp->semantic_stack_next))).type = (_q); }


#define intp_push_value(_eng,_q,_typ)		{ nel_debug ({ nel_trace (_eng, "pushing value 0x%x {\n\n", (_q)); }); if ((_eng)->intp->semantic_stack_next >= (_eng)->intp->semantic_stack_end) intp_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->intp->semantic_stack_next))).integer = (long) (_q); }


#define intp_push_expr(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing expr {\n%1X\n", (_q)); }); if ((_eng)->intp->semantic_stack_next >= (_eng)->intp->semantic_stack_end) intp_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->intp->semantic_stack_next))).expr = (_q); }
#define intp_push_expr_list(_eng,_q)	{ nel_debug ({ nel_trace (_eng, "pushing expr_list {\n%1Y\n", (_q)); }); if ((_eng)->intp->semantic_stack_next >= (_eng)->intp->semantic_stack_end) intp_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->intp->semantic_stack_next))).expr_list = (_q); }
#define intp_push_stmt(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing stmt {\n%1K\n", (_q)); }); if ((_eng)->intp->semantic_stack_next >= (_eng)->intp->semantic_stack_end) intp_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->intp->semantic_stack_next))).stmt = (_q); }
#define intp_push_token(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing token %N {\n\n", (_q)); }); if ((_eng)->intp->semantic_stack_next >= (_eng)->intp->semantic_stack_end) intp_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->intp->semantic_stack_next))).token = (_q); }



/*******************************************************/
/* stuff for popping arguments from the semantic stack */
/*******************************************************/
#define intp_pop_block(_eng,_q)		{ (_q) = (*(((_eng)->intp->semantic_stack_next)--)).block; nel_debug ({ nel_trace (_eng, "} popping block\n%1B\n", (_q)); }); }
#define intp_pop_C_token(_eng,_q)		{ (_q) = (*(((_eng)->intp->semantic_stack_next)--)).C_token; nel_debug ({ nel_trace (_eng, "} popping C_token %C\n\n", (_q)); }); }
#define intp_pop_D_token(_eng,_q)		{ (_q) = (*(((_eng)->intp->semantic_stack_next)--)).D_token; nel_debug ({ nel_trace (_eng, "} popping D_token %D\n\n", (_q)); }); }
#define intp_pop_integer(_eng,_q)		{ (_q) = (*(((_eng)->intp->semantic_stack_next)--)).integer; nel_debug ({ nel_trace (_eng, "} popping integer %d\n\n", (_q)); }); }
#define intp_pop_list(_eng,_q)			{ (_q) = (*(((_eng)->intp->semantic_stack_next)--)).list; nel_debug ({ nel_trace (_eng, "} popping list\n%1L\n", (_q)); }); }
#define intp_pop_name(_eng,_q)			{ (_q) = (*(((_eng)->intp->semantic_stack_next)--)).name; nel_debug ({ nel_trace (_eng, "} popping name %s\n\n", (_q)); }); }
#define intp_pop_member(_eng,_q)		{ (_q) = (*(((_eng)->intp->semantic_stack_next)--)).member; nel_debug ({ nel_trace (_eng, "} popping member\n%1M\n", (_q)); }); }
#define intp_pop_stab_type(_eng,_q)		{ (_q) = (*(((_eng)->intp->semantic_stack_next)--)).stab_type; nel_debug ({ nel_trace (_eng, "} popping stab_type\n%1U\n", (_q)); }); }
#define intp_pop_symbol(_eng,_q)		{ (_q) = (*(((_eng)->intp->semantic_stack_next)--)).symbol; nel_debug ({ nel_trace (_eng, "} popping symbol\n%1S\n", (_q)); }); }

#define intp_pop_rhs(_eng,_q)		{ (_q) = (*(((_eng)->intp->semantic_stack_next)--)).nel_rhs; nel_debug ({ nel_trace (_eng, "} popping nel rhs \n%1S\n", (_q)); }); }
#define intp_pop_type(_eng,_q)			{ (_q) = (*(((_eng)->intp->semantic_stack_next)--)).type; nel_debug ({ nel_trace (_eng, "} popping type\n%1T\n", (_q)); }); }
#define intp_pop_value(_eng,_q,_typ)		{ (_q) = (_typ) (*(((_eng)->intp->semantic_stack_next)--)).integer; nel_debug ({ nel_trace (_eng, "} popping value 0x%x\n\n", (_q)); }); }
#define intp_pop_expr(_eng,_q)		{ (_q) = (*(((_eng)->intp->semantic_stack_next)--)).expr; nel_debug ({ nel_trace (_eng, "} popping expr\n%1X\n", (_q)); }); }
#define intp_pop_expr_list(_eng,_q)	{ (_q) = (*(((_eng)->intp->semantic_stack_next)--)).expr_list; nel_debug ({ nel_trace (_eng, "} popping expr_list\n%1Y\n", (_q)); }); }
#define intp_pop_stmt(_eng,_q)		{ (_q) = (*(((_eng)->intp->semantic_stack_next)--)).stmt; nel_debug ({ nel_trace (_eng, "} popping stmt\n%1K\n", (_q)); }); }
#define intp_pop_token(_eng,_q)		{ (_q) = (*(((_eng)->intp->semantic_stack_next)--)).token; nel_debug ({ nel_trace (_eng, "} popping token %N\n\n", (_q)); }); }



/****************************************************************/
/* stuff for "tushing" arguments onto the semantic stack, i.e., */
/* assigning a value to a stack element i places from the top.  */
/****************************************************************/
#define intp_tush_block(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing block (n = %d)\n%1B\n", (_i), (_q)); }); (*((_eng)->intp->semantic_stack_next-(_i))).block = (_q); }
#define intp_tush_C_token(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing C_token (n = %d) %C\n\n", (_i), (_q)); }); (*((_eng)->intp->semantic_stack_next-(_i))).C_token = (_q); }
#define intp_tush_D_token(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing D_token (n = %d) %D\n\n", (_i), (_q)); }); (*((_eng)->intp->semantic_stack_next-(_i))).D_token = (_q); }
#define intp_tush_integer(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing integer (n = %d) %d\n\n", (_i), (_q)); }); (*((_eng)->intp->semantic_stack_next-(_i))).integer = (_q); }
#define intp_tush_list(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing list (n = %d)\n%1L\n", (_i), (_q)); }); (*((_eng)->intp->semantic_stack_next-(_i))).list = (_q); }
#define intp_tush_name(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing name (n = %d) %s\n\n", (_i), (_q)); }); (*((_eng)->intp->semantic_stack_next-(_i))).name = (_q); }
#define intp_tush_member(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing member (n = %d)\n%1M\n", (_i), (_q)); }); (*((_eng)->intp->semantic_stack_next-(_i))).member = (_q); }
#define intp_tush_stab_type(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing stab_type (n = %d)\n%1U\n", (_i), (_q)); }); (*((_eng)->intp->semantic_stack_next-(_i))).stab_type = (_q); }
#define intp_tush_symbol(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing symbol (n = %d)\n%1S\n", (_i), (_q)); }); (*((_eng)->intp->semantic_stack_next-(_i))).symbol = (_q); }


#define intp_tush_rhs(_eng,_q,_i)	{ nel_debug ({ nel_trace (_eng, "tushing nel rhs (n = %d)\n%1S\n", (_i), (_q)); }); (*((_eng)->intp->semantic_stack_next-(_i))).nel_rhs = (_q); }

#define intp_tush_type(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing type (n = %d)\n%1T\n", (_i), (_q)); }); (*((_eng)->intp->semantic_stack_next-(_i))).type = (_q); }
#define intp_tush_value(_eng,_q,_typ,_i)		{ nel_debug ({ nel_trace (_eng, "tushing value (n = %d) 0x%x\n\n", (_i), (_q)); }); (*((_eng)->intp->semantic_stack_next-(_i))).integer = (int) (_q); }

#define intp_tush_expr(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing expr (n = %d)\n%1X\n", (_i), (_q)); }); (*((_eng)->intp->semantic_stack_next-(_i))).expr = (_q); }
#define intp_tush_expr_list(_eng,_q,_i)	{ nel_debug ({ nel_trace (_eng, "tushing expr_list (n = %d)\n%1Y\n", (_i), (_q)); }); (*((_eng)->intp->semantic_stack_next-(_i))).expr_list = (_q); }
#define intp_tush_stmt(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing stmt (n = %d)\n%1K\n", (_i), (_q)); }); (*((_eng)->intp->semantic_stack_next-(_i))).stmt = (_q); }
#define intp_tush_token(_eng,_q,_i)	{ nel_debug ({ nel_trace (_eng, "tushing token (n = %d) %N\n\n", (_i), (_q)); }); (*((_eng)->intp->semantic_stack_next-(_i))).token = (_q); }



/******************************************************************/
/* stuff for "topping" arguments from the semantic stack, i.e.,   */
/* retreiving a value from a stack element i places from the top. */
/******************************************************************/
#define intp_top_block(_eng,_q,_i)		{ (_q) = (*((_eng)->intp->semantic_stack_next-(_i))).block; nel_debug ({ nel_trace (_eng, "topping block (n = %d)\n%1B\n", (_i), (_q)); }); }
#define intp_top_C_token(_eng,_q,_i)		{ (_q) = (*((_eng)->intp->semantic_stack_next-(_i))).C_token; nel_debug ({ nel_trace (_eng, "topping C_token (n = %d) %C\n\n", (_i), (_q)); }); }
#define intp_top_D_token(_eng,_q,_i)		{ (_q) = (*((_eng)->intp->semantic_stack_next-(_i))).D_token; nel_debug ({ nel_trace (_eng, "topping D_token (n = %d) %D\n\n", (_i), (_q)); }); }
#define intp_top_integer(_eng,_q,_i)		{ (_q) = (*((_eng)->intp->semantic_stack_next-(_i))).integer; nel_debug ({ nel_trace (_eng, "topping integer (n = %d) %d\n\n", (_i), (_q)); }); }
#define intp_top_list(_eng,_q,_i)		{ (_q) = (*((_eng)->intp->semantic_stack_next-(_i))).list; nel_debug ({ nel_trace (_eng, "topping list (n = %d)\n%1L\n", (_i), (_q)); }); }
#define intp_top_name(_eng,_q,_i)		{ (_q) = (*((_eng)->intp->semantic_stack_next-(_i))).name; nel_debug ({ nel_trace (_eng, "topping name (n = %d) %s\n\n", (_i), (_q)); }); }
#define intp_top_member(_eng,_q,_i)		{ (_q) = (*((_eng)->intp->semantic_stack_next-(_i))).member; nel_debug ({ nel_trace (_eng, "topping member (n = %d)\n%1M\n", (_i), (_q)); }); }
#define intp_top_stab_type(_eng,_q,_i)		{ (_q) = (*((_eng)->intp->semantic_stack_next-(_i))).stab_type; nel_debug ({ nel_trace (_eng, "topping stab_type (n = %d)\n%1U\n", (_i), (_q)); }); }
#define intp_top_symbol(_eng,_q,_i)		{ (_q) = (*((_eng)->intp->semantic_stack_next-(_i))).symbol; nel_debug ({ nel_trace (_eng, "topping symbol (n = %d)\n%1S\n", (_i), (_q)); }); }

//#define intp_top_nel_symbol(_eng,_q,_i)		{ (_q) = (*((_eng)->intp->semantic_stack_next-(_i))).nel_symbol; nel_debug ({ nel_trace (_eng, "topping nel symbol (n = %d)\n%1S\n", (_i), (_q)); }); }

#define intp_top_rhs(_eng,_q,_i)		{ (_q) = (*((_eng)->intp->semantic_stack_next-(_i))).nel_rhs; nel_debug ({ nel_trace (_eng, "topping nel rhs(n = %d)\n%1S\n", (_i), (_q)); }); }

#define intp_top_type(_eng,_q,_i)		{ (_q) = (*((_eng)->intp->semantic_stack_next-(_i))).type; nel_debug ({ nel_trace (_eng, "topping type (n = %d)\n%1T\n", (_i), (_q)); }); }
#define intp_top_value(_eng,_q,_typ,_i)		{ (_q) = (_typ) (*((_eng)->intp->semantic_stack_next-(_i))).integer; nel_debug ({ nel_trace (_eng, "topping value (n = %d) 0x%x\n\n", (_i), (_q)); }); }

#define intp_top_expr(_eng,_q,_i)		{ (_q) = (*((_eng)->intp->semantic_stack_next-(_i))).expr; nel_debug ({ nel_trace (_eng, "topping expr (n = %d)\n%1X\n", (_i), (_q)); }); }
#define intp_top_expr_list(_eng,_q,_i)	{ (_q) = (*((_eng)->intp->semantic_stack_next-(_i))).expr_list; nel_debug ({ nel_trace (_eng, "topping expr_list (n = %d)\n%1Y\n", (_i), (_q)); }); }
#define intp_top_stmt(_eng,_q,_i)		{ (_q) = (*((_eng)->intp->semantic_stack_next-(_i))).stmt; nel_debug ({ nel_trace (_eng, "topping stmt (n = %d)\n%1K\n", (_i), (_q)); }); }
#define intp_top_token(_eng,_q,_i)		{ (_q) = (*((_eng)->intp->semantic_stack_next-(_i))).token; nel_debug ({ nel_trace (_eng, "topping token (n = %d) %N\n\n", (_i), (_q)); }); }
/**************************************************************************/
/* intp_top_pointer (_eng,) returns a pointer to a stack element (i) places down */
/**************************************************************************/
#define intp_top_pointer(_eng,_q,_i)		{ (_q) = ((_eng)->intp->semantic_stack_next-(_i)); nel_debug ({ nel_trace (_eng, "topping pointer (n = %d) 0x%x\n\n", (_i), (_q)); }); }

/***********************************************************************/
/* intp_set_stack sets the semantic stack pointer directly               */
/* if nel_tracing is on, make sure the {'s and }'s line up in the trace */
/***********************************************************************/
#define intp_set_stack(_eng,_q) \
	{								\
	   nel_debug ({							\
	      nel_trace (_eng, "setting pointer 0x%x\n\n", (_q));	\
	      while ((_eng)->intp->semantic_stack_next > (_q)) {	\
	         nel_trace (_eng, "}\n\n");				\
	         (_eng)->intp->semantic_stack_next--;			\
	      }								\
	      while ((_eng)->intp->semantic_stack_next < (_q)) {	\
	         nel_trace (_eng, "{\n\n");				\
	         (_eng)->intp->semantic_stack_next++;			\
	      }								\
	   });								\
	   (_eng)->intp->semantic_stack_next = (nel_stack *) (_q);	\
	}

#if 0
/********************************************************/
/* search for an identifier in the location hash tables */
/* if there is no other identifier by that name.        */
/********************************************************/
#define intp_lookup_ident(_eng,_name)	\
	nel_lookup_ident(_eng, _name)	

#define intp_insert_ident(_eng,_symbol) \
	((((_symbol)->level <= 0) && ((_symbol)->class != nel_C_STATIC_GLOBAL) \
	     && ((_symbol)->class != nel_C_COMPILED_STATIC_FUNCTION)	\
	     && ((_symbol)->class != nel_C_NEL_STATIC_FUNCTION)) ?	\
	   nel_insert_symbol ((_eng), (_symbol), (_eng)->nel_static_ident_hash) :		\
	   nel_insert_symbol ((_eng), (_symbol), (_eng)->intp->dyn_ident_hash)	\
	)

#define intp_remove_ident(_eng,_symbol) \
	nel_remove_symbol (_eng, (_symbol))

#endif
void intp_insert_ident(struct nel_eng *eng, nel_symbol *symbol);
void intp_remove_ident(struct nel_eng *eng,  nel_symbol *symbol);
nel_symbol *intp_lookup_ident(struct nel_eng *eng, char *name);


/***********************************************************/
/* when searching for a location (ld symbol table entry),  */
/* check only the location hash tables, and skip the ident */
/* hash tables - they may have a symbol by the same name,  */
/* masking the entry.                                      */
/***********************************************************/
#define intp_lookup_location(_eng,_name)	\
	nel_lookup_symbol ((_name), (_eng)->intp->dyn_location_hash, (_eng)->nel_static_location_hash, NULL)


#define intp_insert_location(_eng,_symbol) \
	((((_symbol)->level <= 0) && ((_symbol)->class != nel_C_STATIC_LOCATION)) ? \
	   nel_insert_symbol ((_eng),(_symbol), (_eng)->nel_static_location_hash) :	\
	   nel_insert_symbol ((_eng),(_symbol), (_eng)->intp->dyn_location_hash)	\
	)

#define intp_remove_location(_eng,_symbol) \
	nel_remove_symbol (_eng, (_symbol))



#define intp_lookup_tag(_eng,_name) \
	nel_lookup_symbol ((_name), (_eng)->intp->dyn_tag_hash, (_eng)->nel_static_tag_hash, NULL)


#define intp_insert_tag(_eng,_symbol) \
	(((_symbol)->level <= 0) ?					\
	   nel_insert_symbol ((_eng),(_symbol), (_eng)->nel_static_tag_hash):\
	   nel_insert_symbol ((_eng), (_symbol), (_eng)->intp->dyn_tag_hash)\
	)

#define intp_remove_tag(_eng,_symbol) \
	nel_remove_symbol (_eng, (_symbol))


#define intp_lookup_label(_eng,_name) \
	nel_lookup_symbol ((_name), (_eng)->intp->dyn_label_hash, NULL)

#define intp_insert_label(_eng,_symbol) \
	nel_insert_symbol ((_eng),(_symbol), (_eng)->intp->dyn_label_hash)

#define intp_remove_label(_eng,_symbol) \
	nel_remove_symbol (_eng, (_symbol))




#define intp_lookup_file(_eng,_name) \
	nel_lookup_symbol ((_name), (_eng)->nel_static_file_hash, NULL)

#define intp_insert_file(_eng,_symbol) \
	nel_insert_symbol ((_eng), (_symbol), (_eng)->nel_static_file_hash)

#define intp_remove_file(_eng,_symbol) \
	nel_remove_symbol (_eng, _symbol)
/****************************************************************/
/* declarations for the routines defined in nel_intp_apply.c     */
/* there are extra arguments for machines where floating-point  */
/* args are passed in fp registers and integral args are passed */
/* in int registers.  sparcs pass arguments in registers, but   */
/* the argument register set is homogenous.                     */
/****************************************************************/

extern void intp_apply (struct nel_eng *, char *, union nel_TYPE *, char (*)(), int, int **);

/*************************************************************/
/* declarations for the routines defined in "nel_intp_eval.c" */
/*************************************************************/
extern void intp_coerce (struct nel_eng*, union nel_TYPE *, char *, union nel_TYPE *, char *);
extern struct nel_SYMBOL *intp_eval_call (struct nel_eng *, struct nel_SYMBOL *, register int, union nel_STACK *);
extern void intp_op (struct nel_eng *, nel_O_token, nel_type *, char *, char *, char *);
extern union nel_TYPE *intp_type_unary_op (struct nel_eng *, register union nel_TYPE *, register unsigned_int, register unsigned_int);
extern struct nel_SYMBOL *intp_eval_unary_op (struct nel_eng *, nel_O_token, register struct nel_SYMBOL *);
extern union nel_TYPE *intp_type_binary_op (struct nel_eng *, register union nel_TYPE *, register union nel_TYPE *, register unsigned_int, register unsigned_int);
extern struct nel_SYMBOL *intp_eval_binary_op (struct nel_eng *, nel_O_token, register struct nel_SYMBOL *, register struct nel_SYMBOL *);
extern void intp_insert_bit_field (struct nel_eng *, union nel_TYPE *, char *, register unsigned_int, register unsigned_int, register unsigned_int);
extern struct nel_SYMBOL *intp_eval_asgn (struct nel_eng *, register nel_O_token, register struct nel_SYMBOL *, register struct nel_SYMBOL *); 
extern struct nel_SYMBOL *intp_eval_inc_dec (struct nel_eng *, nel_O_token, register struct nel_SYMBOL *);
extern struct nel_SYMBOL *intp_eval_address (struct nel_eng *, register struct nel_SYMBOL *);
extern struct nel_SYMBOL *intp_eval_deref (struct nel_eng *, register struct nel_SYMBOL *);
extern struct nel_SYMBOL *intp_eval_cast (struct nel_eng *, register union nel_TYPE *, register struct nel_SYMBOL *);
extern struct nel_SYMBOL *intp_eval_sizeof (struct nel_eng *, register union nel_TYPE *);
extern struct nel_SYMBOL *intp_eval_typeof (struct nel_eng *, register union nel_TYPE *);
extern void intp_extract_bit_field (struct nel_eng *, union nel_TYPE *, char *, unsigned_int, register unsigned_int, register unsigned_int);
extern struct nel_SYMBOL *intp_eval_member (struct nel_eng *, register struct nel_SYMBOL *, register struct nel_SYMBOL *);
extern unsigned_int intp_nonzero (struct nel_eng *, register struct nel_SYMBOL *);
extern struct nel_SYMBOL *intp_eval_expr (struct nel_eng *, register union nel_EXPR *);
extern struct nel_SYMBOL *intp_eval_expr (struct nel_eng *, register union nel_EXPR *);
extern void intp_eval_stmt (struct nel_eng *, register union nel_STMT *);

extern void save_intp_context(struct nel_eng *_eng);
extern void restore_intp_context(struct nel_eng *_eng, int _level);
extern int intp_init(struct nel_eng *_eng, char *_filename, FILE *_infile, char *_retval_loc, va_list *_formal_ap, unsigned_int _declare_mode);
extern void intp_dealloc(struct nel_eng *_eng);

extern int intp_eval (struct nel_eng *, char *, register struct nel_SYMBOL *, va_list *);
extern int nel_func_name_call (struct nel_eng *, char *, char *, ...); 
extern int nel_func_call(struct nel_eng *, char *, struct nel_SYMBOL *, ...); 

extern char *intp_dyn_value_alloc (struct nel_eng *eng, register unsigned_int bytes, register unsigned_int alignment);
extern nel_symbol *intp_dyn_symbol_alloc (struct nel_eng *eng, char *name, nel_type *type, char *value, nel_C_token class, unsigned_int lhs, nel_L_token source_lang, int level);

nel_symbol *intp_eval_expr_2 (struct nel_eng *eng, register nel_expr *expr);
int intp_eval_stmt_2 (struct nel_eng *eng, nel_stmt *stmt);

#endif /* EVAL_H */


