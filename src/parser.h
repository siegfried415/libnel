

/*****************************************************************************/
/* This file, "parser.h", contains declarations for the routines, any       */
/* #define statements, and extern declarations for any global variables that */
/* are defined in "parser.c".                                               */
/*****************************************************************************/



#ifndef PARSER_H
#define PARSER_H 

#include "errors.h"
#include "sym.h"

struct eng_gram{

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

	/*************************************/
	/* vars used only by the interpreter */
	/*************************************/
	//unsigned_int declare_mode: 1;
	//char *retval_loc;
	//va_list *formal_ap;
	//char *unit_prefix;
	union nel_STMT *stmts;
	union nel_STMT *last_stmt;	//wyong, 2006.3.10
	union nel_STMT **append_pt;
	union nel_STMT *break_target;
	union nel_STMT *continue_target;

	int rhs_cnt;			//wyong, 2006.3.11 

	/*NOTE,NOTE,NOTE, maybe need put the following into 
	struct eng_lexier,wyong, 2005.10.04 */
	//struct eng_lexier *lex;

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



/*********************************************************/
/* for parser_setjmp, we also store the parser state.  */
/* because we have to restore several variables that are */
/* local to the parse routine, this macro must be called */
/* from within the body of a bison-generated parser.     */
/*********************************************************/
#define parser_yy_setjmp(_eng,_val,_buffer) \
	{								\
	   (_eng)->parser->tmp_err_jmp = (_buffer);				\
	   (_eng)->parser->tmp_err_jmp->stmt_err_jmp = (_eng)->parser->stmt_err_jmp; \
	   (_eng)->parser->tmp_err_jmp->block_err_jmp = (_eng)->parser->block_err_jmp; \
	   parser_top_pointer ((_eng), (_eng)->parser->tmp_err_jmp->semantic_stack_next, 0); \
	   (_eng)->parser->tmp_err_jmp->level = (_eng)->parser->level;		\
	   (_eng)->parser->tmp_err_jmp->block_no = (_eng)->parser->block_no;	\
	   (_eng)->parser->tmp_err_jmp->clean_flag = (_eng)->parser->clean_flag; \
	   (_eng)->parser->tmp_err_jmp->dyn_values_next = (_eng)->parser->dyn_values_next; \
	   (_eng)->parser->tmp_err_jmp->dyn_symbols_next = (_eng)->parser->dyn_symbols_next; \
	   (_eng)->parser->tmp_err_jmp->context = (_eng)->parser->context;	\
	   (_eng)->parser->tmp_err_jmp->state = yystate;			\
	   (_eng)->parser->tmp_err_jmp->n = yyn;				\
	   (_eng)->parser->tmp_err_jmp->ssp = yyssp;			\
	   (_eng)->parser->tmp_err_jmp->vsp = yyvsp;			\
	   (_eng)->parser->tmp_err_jmp->ss = yyss;				\
	   (_eng)->parser->tmp_err_jmp->vs = yyvs;				\
	   (_eng)->parser->tmp_err_jmp->len = yylen;			\
	   (_val) = setjmp ((_eng)->parser->tmp_err_jmp->buf);		\
	   if ((_val) != 0) {						\
	      (_eng)->parser->tmp_err_jmp = (_buffer);			\
	      (_eng)->parser->stmt_err_jmp = (_eng)->parser->tmp_err_jmp->stmt_err_jmp; \
	      (_eng)->parser->block_err_jmp = (_eng)->parser->tmp_err_jmp->block_err_jmp; \
	      parser_set_stack ((_eng), (_eng)->parser->tmp_err_jmp->semantic_stack_next); \
	      (_eng)->parser->level = (_eng)->parser->tmp_err_jmp->level;	\
	      (_eng)->parser->block_no = (_eng)->parser->tmp_err_jmp->block_no;	\
	      (_eng)->parser->clean_flag = (_eng)->parser->tmp_err_jmp->clean_flag; \
	      (_eng)->parser->dyn_values_next = (_eng)->parser->tmp_err_jmp->dyn_values_next; \
	      (_eng)->parser->dyn_symbols_next = (_eng)->parser->tmp_err_jmp->dyn_symbols_next; \
	      (_eng)->parser->context = (_eng)->parser->tmp_err_jmp->context;	\
	      yystate = (_eng)->parser->tmp_err_jmp->state;			\
	      yyn = (_eng)->parser->tmp_err_jmp->n;				\
	      yyssp = (_eng)->parser->tmp_err_jmp->ssp;			\
	      yyvsp = (_eng)->parser->tmp_err_jmp->vsp;			\
	      yyss = (_eng)->parser->tmp_err_jmp->ss;			\
	      yyvs = (_eng)->parser->tmp_err_jmp->vs;			\
	      yylen = (_eng)->parser->tmp_err_jmp->len;			\
	      yynerrs = 0;						\
	      nel_purge_table (_eng, (_eng)->parser->level + 1, (_eng)->parser->dyn_ident_hash); \
	      nel_purge_table (_eng, (_eng)->parser->level + 1, (_eng)->parser->dyn_location_hash); \
	      nel_purge_table (_eng, (_eng)->parser->level + 1, (_eng)->parser->dyn_tag_hash); \
	      nel_purge_table (_eng, (_eng)->parser->level + 1, (_eng)->parser->dyn_label_hash); \
	   }								\
	}

#define parser_setjmp(_eng,_val,_buffer) \
	{								\
	   (_eng)->parser->tmp_err_jmp = (_buffer);				\
	   (_eng)->parser->tmp_err_jmp->stmt_err_jmp = (_eng)->parser->stmt_err_jmp; \
	   (_eng)->parser->tmp_err_jmp->block_err_jmp = (_eng)->parser->block_err_jmp; \
	   parser_top_pointer ((_eng), (_eng)->parser->tmp_err_jmp->semantic_stack_next, 0); \
	   (_eng)->parser->tmp_err_jmp->level = (_eng)->parser->level;		\
	   (_eng)->parser->tmp_err_jmp->block_no = (_eng)->parser->block_no;	\
	   (_eng)->parser->tmp_err_jmp->clean_flag = (_eng)->parser->clean_flag; \
	   (_eng)->parser->tmp_err_jmp->dyn_values_next = (_eng)->parser->dyn_values_next; \
	   (_eng)->parser->tmp_err_jmp->dyn_symbols_next = (_eng)->parser->dyn_symbols_next; \
	   (_eng)->parser->tmp_err_jmp->context = (_eng)->parser->context;	\
	   (_val) = setjmp ((_eng)->parser->tmp_err_jmp->buf);		\
	   if ((_val) != 0) {						\
	      (_eng)->parser->tmp_err_jmp = (_buffer);			\
	      (_eng)->parser->stmt_err_jmp = (_eng)->parser->tmp_err_jmp->stmt_err_jmp; \
	      (_eng)->parser->block_err_jmp = (_eng)->parser->tmp_err_jmp->block_err_jmp; \
	      parser_set_stack ((_eng), (_eng)->parser->tmp_err_jmp->semantic_stack_next); \
	      (_eng)->parser->level = (_eng)->parser->tmp_err_jmp->level;	\
	      (_eng)->parser->block_no = (_eng)->parser->tmp_err_jmp->block_no;	\
	      (_eng)->parser->clean_flag = (_eng)->parser->tmp_err_jmp->clean_flag; \
	      (_eng)->parser->dyn_values_next = (_eng)->parser->tmp_err_jmp->dyn_values_next; \
	      (_eng)->parser->dyn_symbols_next = (_eng)->parser->tmp_err_jmp->dyn_symbols_next; \
	      (_eng)->parser->context = (_eng)->parser->tmp_err_jmp->context;	\
	      nel_purge_table (_eng, (_eng)->parser->level + 1, (_eng)->parser->dyn_ident_hash); \
	      nel_purge_table (_eng, (_eng)->parser->level + 1, (_eng)->parser->dyn_location_hash); \
	      nel_purge_table (_eng, (_eng)->parser->level + 1, (_eng)->parser->dyn_tag_hash); \
	      nel_purge_table (_eng, (_eng)->parser->level + 1, (_eng)->parser->dyn_label_hash); \
	   }								\
	}



extern int parser_warning (struct nel_eng *, char *, ...);
extern int parser_error (struct nel_eng *, char *, ...);
extern int parser_stmt_error(struct nel_eng *, char *, ...);
extern int parser_block_error (struct nel_eng *, char *, ...);
extern int parser_fatal_error(struct nel_eng *, char *, ...);

/*******************************************************/
/* stuff for pushing arguments onto the semantic stack */
/*******************************************************/
#define parser_push_block(_eng,_q) { nel_debug({nel_trace(_eng,"pushing block {\n%1B\n",(_q));}); if ((_eng)->parser->semantic_stack_next >= (_eng)->parser->semantic_stack_end) parser_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->parser->semantic_stack_next))).block = (_q); }

#define parser_push_C_token(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing C_token %C {\n\n", (_q)); }); if ((_eng)->parser->semantic_stack_next >= (_eng)->parser->semantic_stack_end) parser_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->parser->semantic_stack_next))).C_token = (_q); }
#define parser_push_D_token(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing D_token %D {\n\n", (_q)); }); if ((_eng)->parser->semantic_stack_next >= (_eng)->parser->semantic_stack_end) parser_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->parser->semantic_stack_next))).D_token = (_q); }
#define parser_push_integer(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing integer %d {\n\n", (_q)); }); if ((_eng)->parser->semantic_stack_next >= (_eng)->parser->semantic_stack_end) parser_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->parser->semantic_stack_next))).integer = (_q); }
#define parser_push_list(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing list {\n%1L\n", (_q)); }); if ((_eng)->parser->semantic_stack_next >= (_eng)->parser->semantic_stack_end) parser_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->parser->semantic_stack_next))).list = (_q); }
#define parser_push_name(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing name %s {\n\n", (_q)); }); if ((_eng)->parser->semantic_stack_next >= (_eng)->parser->semantic_stack_end) parser_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->parser->semantic_stack_next))).name = (_q); }
#define parser_push_member(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing member {\n%1M\n", (_q)); }); if ((_eng)->parser->semantic_stack_next >= (_eng)->parser->semantic_stack_end) parser_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->parser->semantic_stack_next))).member = (_q); }
#define parser_push_stab_type(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing stab_type {\n%1U\n", (_q)); }); if ((_eng)->parser->semantic_stack_next >= (_eng)->parser->semantic_stack_end) parser_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->parser->semantic_stack_next))).stab_type = (_q); }
#define parser_push_symbol(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing symbol {\n%1S\n", (_q)); }); if ((_eng)->parser->semantic_stack_next >= (_eng)->parser->semantic_stack_end) parser_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->parser->semantic_stack_next))).symbol = (_q); }

#define parser_push_nel_symbol(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing nel symbol {\n%1S\n", (_q)); }); if ((_eng)->parser->semantic_stack_next >= (_eng)->parser->semantic_stack_end) parser_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->parser->semantic_stack_next))).nel_symbol = (_q); }

#define parser_push_rhs(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing nel rhs {\n%1S\n", (_q)); }); if ((_eng)->parser->semantic_stack_next >= (_eng)->parser->semantic_stack_end) parser_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->parser->semantic_stack_next))).nel_rhs = (_q); }

#define parser_push_type(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing type {\n%1T\n", (_q)); }); if ((_eng)->parser->semantic_stack_next >= (_eng)->parser->semantic_stack_end) parser_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->parser->semantic_stack_next))).type = (_q); }


#define parser_push_value(_eng,_q,_typ)		{ nel_debug ({ nel_trace (_eng, "pushing value 0x%x {\n\n", (_q)); }); if ((_eng)->parser->semantic_stack_next >= (_eng)->parser->semantic_stack_end) parser_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->parser->semantic_stack_next))).integer = (int) (_q); }
#define parser_push_expr(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing expr {\n%1X\n", (_q)); }); if ((_eng)->parser->semantic_stack_next >= (_eng)->parser->semantic_stack_end) parser_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->parser->semantic_stack_next))).expr = (_q); }
#define parser_push_expr_list(_eng,_q)	{ nel_debug ({ nel_trace (_eng, "pushing expr_list {\n%1Y\n", (_q)); }); if ((_eng)->parser->semantic_stack_next >= (_eng)->parser->semantic_stack_end) parser_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->parser->semantic_stack_next))).expr_list = (_q); }
#define parser_push_stmt(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing stmt {\n%1K\n", (_q)); }); if ((_eng)->parser->semantic_stack_next >= (_eng)->parser->semantic_stack_end) parser_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->parser->semantic_stack_next))).stmt = (_q); }
#define parser_push_token(_eng,_q)		{ nel_debug ({ nel_trace (_eng, "pushing token %N {\n\n", (_q)); }); if ((_eng)->parser->semantic_stack_next >= (_eng)->parser->semantic_stack_end) parser_stmt_error ((_eng), nel_semantic_stack_overflow_message); (*(++((_eng)->parser->semantic_stack_next))).token = (_q); }


/*******************************************************/
/* stuff for popping arguments from the semantic stack */
/*******************************************************/
#define parser_pop_block(_eng,_q)		{ (_q) = (*(((_eng)->parser->semantic_stack_next)--)).block; nel_debug ({ nel_trace (_eng, "} popping block\n%1B\n", (_q)); }); }
#define parser_pop_C_token(_eng,_q)		{ (_q) = (*(((_eng)->parser->semantic_stack_next)--)).C_token; nel_debug ({ nel_trace (_eng, "} popping C_token %C\n\n", (_q)); }); }
#define parser_pop_D_token(_eng,_q)		{ (_q) = (*(((_eng)->parser->semantic_stack_next)--)).D_token; nel_debug ({ nel_trace (_eng, "} popping D_token %D\n\n", (_q)); }); }
#define parser_pop_integer(_eng,_q)		{ (_q) = (*(((_eng)->parser->semantic_stack_next)--)).integer; nel_debug ({ nel_trace (_eng, "} popping integer %d\n\n", (_q)); }); }
#define parser_pop_list(_eng,_q)			{ (_q) = (*(((_eng)->parser->semantic_stack_next)--)).list; nel_debug ({ nel_trace (_eng, "} popping list\n%1L\n", (_q)); }); }
#define parser_pop_name(_eng,_q)			{ (_q) = (*(((_eng)->parser->semantic_stack_next)--)).name; nel_debug ({ nel_trace (_eng, "} popping name %s\n\n", (_q)); }); }
#define parser_pop_member(_eng,_q)		{ (_q) = (*(((_eng)->parser->semantic_stack_next)--)).member; nel_debug ({ nel_trace (_eng, "} popping member\n%1M\n", (_q)); }); }
#define parser_pop_stab_type(_eng,_q)		{ (_q) = (*(((_eng)->parser->semantic_stack_next)--)).stab_type; nel_debug ({ nel_trace (_eng, "} popping stab_type\n%1U\n", (_q)); }); }
#define parser_pop_symbol(_eng,_q)		{ (_q) = (*(((_eng)->parser->semantic_stack_next)--)).symbol; nel_debug ({ nel_trace (_eng, "} popping symbol\n%1S\n", (_q)); }); }

#define parser_pop_rhs(_eng,_q)		{ (_q) = (*(((_eng)->parser->semantic_stack_next)--)).nel_rhs; nel_debug ({ nel_trace (_eng, "} popping nel rhs \n%1S\n", (_q)); }); }
#define parser_pop_type(_eng,_q)			{ (_q) = (*(((_eng)->parser->semantic_stack_next)--)).type; nel_debug ({ nel_trace (_eng, "} popping type\n%1T\n", (_q)); }); }
#define parser_pop_value(_eng,_q,_typ)		{ (_q) = (_typ) (*(((_eng)->parser->semantic_stack_next)--)).integer; nel_debug ({ nel_trace (_eng, "} popping value 0x%x\n\n", (_q)); }); }
#define parser_pop_expr(_eng,_q)		{ (_q) = (*(((_eng)->parser->semantic_stack_next)--)).expr; nel_debug ({ nel_trace (_eng, "} popping expr\n%1X\n", (_q)); }); }
#define parser_pop_expr_list(_eng,_q)	{ (_q) = (*(((_eng)->parser->semantic_stack_next)--)).expr_list; nel_debug ({ nel_trace (_eng, "} popping expr_list\n%1Y\n", (_q)); }); }
#define parser_pop_stmt(_eng,_q)		{ (_q) = (*(((_eng)->parser->semantic_stack_next)--)).stmt; nel_debug ({ nel_trace (_eng, "} popping stmt\n%1K\n", (_q)); }); }
#define parser_pop_token(_eng,_q)		{ (_q) = (*(((_eng)->parser->semantic_stack_next)--)).token; nel_debug ({ nel_trace (_eng, "} popping token %N\n\n", (_q)); }); }



/****************************************************************/
/* stuff for "tushing" arguments onto the semantic stack, i.e., */
/* assigning a value to a stack element i places from the top.  */
/****************************************************************/
#define parser_tush_block(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing block (n = %d)\n%1B\n", (_i), (_q)); }); (*((_eng)->parser->semantic_stack_next-(_i))).block = (_q); }
#define parser_tush_C_token(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing C_token (n = %d) %C\n\n", (_i), (_q)); }); (*((_eng)->parser->semantic_stack_next-(_i))).C_token = (_q); }
#define parser_tush_D_token(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing D_token (n = %d) %D\n\n", (_i), (_q)); }); (*((_eng)->parser->semantic_stack_next-(_i))).D_token = (_q); }
#define parser_tush_integer(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing integer (n = %d) %d\n\n", (_i), (_q)); }); (*((_eng)->parser->semantic_stack_next-(_i))).integer = (_q); }
#define parser_tush_list(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing list (n = %d)\n%1L\n", (_i), (_q)); }); (*((_eng)->parser->semantic_stack_next-(_i))).list = (_q); }
#define parser_tush_name(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing name (n = %d) %s\n\n", (_i), (_q)); }); (*((_eng)->parser->semantic_stack_next-(_i))).name = (_q); }
#define parser_tush_member(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing member (n = %d)\n%1M\n", (_i), (_q)); }); (*((_eng)->parser->semantic_stack_next-(_i))).member = (_q); }
#define parser_tush_stab_type(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing stab_type (n = %d)\n%1U\n", (_i), (_q)); }); (*((_eng)->parser->semantic_stack_next-(_i))).stab_type = (_q); }
#define parser_tush_symbol(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing symbol (n = %d)\n%1S\n", (_i), (_q)); }); (*((_eng)->parser->semantic_stack_next-(_i))).symbol = (_q); }


#define parser_tush_rhs(_eng,_q,_i)	{ nel_debug ({ nel_trace (_eng, "tushing nel rhs (n = %d)\n%1S\n", (_i), (_q)); }); (*((_eng)->parser->semantic_stack_next-(_i))).nel_rhs = (_q); }

#define parser_tush_type(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing type (n = %d)\n%1T\n", (_i), (_q)); }); (*((_eng)->parser->semantic_stack_next-(_i))).type = (_q); }
#define parser_tush_value(_eng,_q,_typ,_i)		{ nel_debug ({ nel_trace (_eng, "tushing value (n = %d) 0x%x\n\n", (_i), (_q)); }); (*((_eng)->parser->semantic_stack_next-(_i))).integer = (int) (_q); }

#define parser_tush_expr(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing expr (n = %d)\n%1X\n", (_i), (_q)); }); (*((_eng)->parser->semantic_stack_next-(_i))).expr = (_q); }
#define parser_tush_expr_list(_eng,_q,_i)	{ nel_debug ({ nel_trace (_eng, "tushing expr_list (n = %d)\n%1Y\n", (_i), (_q)); }); (*((_eng)->parser->semantic_stack_next-(_i))).expr_list = (_q); }
#define parser_tush_stmt(_eng,_q,_i)		{ nel_debug ({ nel_trace (_eng, "tushing stmt (n = %d)\n%1K\n", (_i), (_q)); }); (*((_eng)->parser->semantic_stack_next-(_i))).stmt = (_q); }
#define parser_tush_token(_eng,_q,_i)	{ nel_debug ({ nel_trace (_eng, "tushing token (n = %d) %N\n\n", (_i), (_q)); }); (*((_eng)->parser->semantic_stack_next-(_i))).token = (_q); }



/******************************************************************/
/* stuff for "topping" arguments from the semantic stack, i.e.,   */
/* retreiving a value from a stack element i places from the top. */
/******************************************************************/
#define parser_top_block(_eng,_q,_i)		{ (_q) = (*((_eng)->parser->semantic_stack_next-(_i))).block; nel_debug ({ nel_trace (_eng, "topping block (n = %d)\n%1B\n", (_i), (_q)); }); }
#define parser_top_C_token(_eng,_q,_i)		{ (_q) = (*((_eng)->parser->semantic_stack_next-(_i))).C_token; nel_debug ({ nel_trace (_eng, "topping C_token (n = %d) %C\n\n", (_i), (_q)); }); }
#define parser_top_D_token(_eng,_q,_i)		{ (_q) = (*((_eng)->parser->semantic_stack_next-(_i))).D_token; nel_debug ({ nel_trace (_eng, "topping D_token (n = %d) %D\n\n", (_i), (_q)); }); }
#define parser_top_integer(_eng,_q,_i)		{ (_q) = (*((_eng)->parser->semantic_stack_next-(_i))).integer; nel_debug ({ nel_trace (_eng, "topping integer (n = %d) %d\n\n", (_i), (_q)); }); }
#define parser_top_list(_eng,_q,_i)		{ (_q) = (*((_eng)->parser->semantic_stack_next-(_i))).list; nel_debug ({ nel_trace (_eng, "topping list (n = %d)\n%1L\n", (_i), (_q)); }); }
#define parser_top_name(_eng,_q,_i)		{ (_q) = (*((_eng)->parser->semantic_stack_next-(_i))).name; nel_debug ({ nel_trace (_eng, "topping name (n = %d) %s\n\n", (_i), (_q)); }); }
#define parser_top_member(_eng,_q,_i)		{ (_q) = (*((_eng)->parser->semantic_stack_next-(_i))).member; nel_debug ({ nel_trace (_eng, "topping member (n = %d)\n%1M\n", (_i), (_q)); }); }
#define parser_top_stab_type(_eng,_q,_i)		{ (_q) = (*((_eng)->parser->semantic_stack_next-(_i))).stab_type; nel_debug ({ nel_trace (_eng, "topping stab_type (n = %d)\n%1U\n", (_i), (_q)); }); }
#define parser_top_symbol(_eng,_q,_i)		{ (_q) = (*((_eng)->parser->semantic_stack_next-(_i))).symbol; nel_debug ({ nel_trace (_eng, "topping symbol (n = %d)\n%1S\n", (_i), (_q)); }); }

#define parser_top_nel_symbol(_eng,_q,_i)		{ (_q) = (*((_eng)->parser->semantic_stack_next-(_i))).nel_symbol; nel_debug ({ nel_trace (_eng, "topping nel symbol (n = %d)\n%1S\n", (_i), (_q)); }); }

#define parser_top_rhs(_eng,_q,_i)		{ (_q) = (*((_eng)->parser->semantic_stack_next-(_i))).nel_rhs; nel_debug ({ nel_trace (_eng, "topping nel rhs(n = %d)\n%1S\n", (_i), (_q)); }); }

#define parser_top_type(_eng,_q,_i)		{ (_q) = (*((_eng)->parser->semantic_stack_next-(_i))).type; nel_debug ({ nel_trace (_eng, "topping type (n = %d)\n%1T\n", (_i), (_q)); }); }
#define parser_top_value(_eng,_q,_typ,_i)		{ (_q) = (_typ) (*((_eng)->parser->semantic_stack_next-(_i))).integer; nel_debug ({ nel_trace (_eng, "topping value (n = %d) 0x%x\n\n", (_i), (_q)); }); }

#define parser_top_expr(_eng,_q,_i)		{ (_q) = (*((_eng)->parser->semantic_stack_next-(_i))).expr; nel_debug ({ nel_trace (_eng, "topping expr (n = %d)\n%1X\n", (_i), (_q)); }); }
#define parser_top_expr_list(_eng,_q,_i)	{ (_q) = (*((_eng)->parser->semantic_stack_next-(_i))).expr_list; nel_debug ({ nel_trace (_eng, "topping expr_list (n = %d)\n%1Y\n", (_i), (_q)); }); }
#define parser_top_stmt(_eng,_q,_i)		{ (_q) = (*((_eng)->parser->semantic_stack_next-(_i))).stmt; nel_debug ({ nel_trace (_eng, "topping stmt (n = %d)\n%1K\n", (_i), (_q)); }); }
#define parser_top_token(_eng,_q,_i)		{ (_q) = (*((_eng)->parser->semantic_stack_next-(_i))).token; nel_debug ({ nel_trace (_eng, "topping token (n = %d) %N\n\n", (_i), (_q)); }); }
/**************************************************************************/
/* parser_top_pointer (_eng,) returns a pointer to a stack element (i) places down */
/**************************************************************************/
#define parser_top_pointer(_eng,_q,_i)		{ (_q) = ((_eng)->parser->semantic_stack_next-(_i)); nel_debug ({ nel_trace (_eng, "topping pointer (n = %d) 0x%x\n\n", (_i), (_q)); }); }


/************************************************************************/
/* parser_set_stack sets the semantic stack pointer directly            */
/* if nel_tracing is on, make sure the {'s and }'s line up in the trace */
/************************************************************************/
#define parser_set_stack(_eng,_q) \
	{								\
	   nel_debug ({							\
	      nel_trace (_eng, "setting pointer 0x%x\n\n", (_q));	\
	      while ((_eng)->parser->semantic_stack_next > (_q)) {	\
	         nel_trace (_eng, "}\n\n");				\
	         (_eng)->parser->semantic_stack_next--;			\
	      }								\
	      while ((_eng)->parser->semantic_stack_next < (_q)) {	\
	         nel_trace (_eng, "{\n\n");				\
	         (_eng)->parser->semantic_stack_next++;			\
	      }								\
	   });								\
	   (_eng)->parser->semantic_stack_next = (nel_stack *) (_q);	\
	}

#if 0
/********************************************************/
/* search for an identifier in the location hash tables */
/* if there is no other identifier by that name.        */
/********************************************************/
#define parser_lookup_ident(_eng,_name)	\
	nel_lookup_ident(_eng, _name)	

#define parser_insert_ident(_eng,_symbol) \
	((((_symbol)->level <= 0) && ((_symbol)->class != nel_C_STATIC_GLOBAL) \
	     && ((_symbol)->class != nel_C_COMPILED_STATIC_FUNCTION)	\
	     && ((_symbol)->class != nel_C_NEL_STATIC_FUNCTION)) ?	\
	   nel_insert_symbol ((_eng), (_symbol), (_eng)->nel_static_ident_hash) :		\
	   nel_insert_symbol ((_eng), (_symbol), (_eng)->parser->dyn_ident_hash)	\
	)

#define parser_remove_ident(_eng,_symbol) \
	nel_remove_symbol (_eng, (_symbol))

#endif
void parser_insert_ident(struct nel_eng *eng, nel_symbol *symbol);
void parser_remove_ident(struct nel_eng *eng,  nel_symbol *symbol);
nel_symbol *parser_lookup_ident(struct nel_eng *eng, char *name);


/***********************************************************/
/* when searching for a location (ld symbol table entry),  */
/* check only the location hash tables, and skip the ident */
/* hash tables - they may have a symbol by the same name,  */
/* masking the entry.                                      */
/***********************************************************/
#define parser_lookup_location(_eng,_name)	\
	nel_lookup_symbol ((_name), (_eng)->parser->dyn_location_hash, (_eng)->nel_static_location_hash, NULL)


#define parser_insert_location(_eng,_symbol) \
	((((_symbol)->level <= 0) && ((_symbol)->class != nel_C_STATIC_LOCATION)) ? \
	   nel_insert_symbol ((_eng),(_symbol), (_eng)->nel_static_location_hash) :	\
	   nel_insert_symbol ((_eng),(_symbol), (_eng)->parser->dyn_location_hash)	\
	)

#define parser_remove_location(_eng,_symbol) \
	nel_remove_symbol (_eng, (_symbol))



#define parser_lookup_tag(_eng,_name) \
	nel_lookup_symbol ((_name), (_eng)->parser->dyn_tag_hash, (_eng)->nel_static_tag_hash, NULL)


#define parser_insert_tag(_eng,_symbol) \
	(((_symbol)->level <= 0) ?					\
	   nel_insert_symbol ((_eng),(_symbol), (_eng)->nel_static_tag_hash):\
	   nel_insert_symbol ((_eng), (_symbol), (_eng)->parser->dyn_tag_hash)\
	)

#define parser_remove_tag(_eng,_symbol) \
	nel_remove_symbol (_eng, (_symbol))


#define parser_lookup_label(_eng,_name) \
	nel_lookup_symbol ((_name), (_eng)->parser->dyn_label_hash, NULL)

#define parser_insert_label(_eng,_symbol) \
	nel_insert_symbol ((_eng),(_symbol), (_eng)->parser->dyn_label_hash)

#define parser_remove_label(_eng,_symbol) \
	nel_remove_symbol (_eng, (_symbol))




#define parser_lookup_file(_eng,_name) \
	nel_lookup_symbol ((_name), (_eng)->nel_static_file_hash, NULL)

#define parser_insert_file(_eng,_symbol) \
	nel_insert_symbol ((_eng), (_symbol), (_eng)->nel_static_file_hash)

#define parser_remove_file(_eng,_symbol) \
	nel_remove_symbol (_eng, _symbol)

//extern void nel_switch_parser_context(struct nel_eng *eng, char *fname,FILE *fp);
//extern void nel_back_parser_context(struct nel_eng *eng);

extern union nel_TYPE *parser_resolve_type (struct nel_eng *, register union nel_TYPE *, char *);

extern void save_parser_context(struct nel_eng *_eng);
extern void restore_parser_context(struct nel_eng *_eng, int _level);
extern int  parser_init(struct nel_eng *_eng, char *_filename, FILE *_infile, char *_retval_loc, va_list *_formal_ap, unsigned_int _declare_mode);
extern void parser_dealloc(struct nel_eng *_eng);

extern int parser_eval_yy(struct nel_eng *);
extern int  parser_eval(struct nel_eng *, char *, /* FILE *,*/ char *, va_list * /*, unsigned_int, unsigned_int */ );
extern int  nel_file_parse(struct nel_eng *, int, char **);

#endif /* _PARSER_H */
