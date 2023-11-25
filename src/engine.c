
/*****************************************************************************/
/* This file, engine.c, contains routine for manipulating the nel_eng */
/* record passed throughout the entire call sequence of the interpreter and  */
/* stab scanner, and routines for manipulating the semantic stack (contained */
/* in the nel_eng ).                                                         */
/*****************************************************************************/
#include <stdlib.h>
#include <engine.h>
#include <errors.h>

#include "mem.h"
#include "intp.h"
#include "gen.h"
#include "expr.h"
#include "_stab.h"
#include "stmt.h"
#include "type.h"
#include "sym.h"
#include "evt.h" 
#include "parser.h"
#include "intrns.h"

//#include "comp.h"

/*****************************************************************************/
/* nel_R_name () returns the string that is the identifier for <code>, a      */
/* nel_R_token representing the type of an nel_eng.                       */
/*****************************************************************************/
char *nel_R_name (register nel_R_token code)
{
	switch (code) {
	case  nel_R_NULL:
		return ("nel_R_NULL");
	case  nel_R_SIMPLE:
		return ("nel_R_SIMPLE");
	case  nel_R_PARSER:
		return ("nel_R_PARSER");
	case  nel_R_STAB:
		return ("nel_R_STAB");
	case  nel_R_INTP:
		return ("nel_R_INTP");
	case  nel_R_GENERATOR:
		return ("nel_R_GENERATOR");
	case  nel_R_MAX:
		return ("nel_R_MAX");
	default:
		return ("nel_R_BAD_TOKEN");
	}
}

/******************************************************/
/* memory allocation should be in a critical section. */
/******************************************************/
nel_lock_type nel_malloc_lock = nel_unlocked_state;
nel_lock_type nel_calloc_lock = nel_unlocked_state;	
nel_lock_type nel_realloc_lock = nel_unlocked_state;


/*****************************************************************************/
/* _init () calls nel_init () to do the basic initialization if not    */
/* already done, and initializes some global variables for the interpreter,  */
/* if not already done, and then initializes the intrinsic functions.        */
/*****************************************************************************/
void nel_init(struct nel_eng *eng )
{

	/**********************************************************/
	/* don't put the initial flag check in a critical section */
	/**********************************************************/
	if (!eng->init_flag) {

		/***********************************************/
		/* if we haven't initialized, enter a critical */
		/* section and check the flag again.           */
		/***********************************************/
		//nel_lock (&nel_init_lock);

		nel_symbol_init(eng);
		nel_zero_expr = nel_expr_alloc (eng, nel_O_SYMBOL, nel_zero_symbol);
		nel_one_expr = nel_expr_alloc (eng, nel_O_SYMBOL, nel_one_symbol);

		eng->numTerminals = 0 ;
		eng->numNonterminals  = 0;
		eng->numProductions = 0;

		eng->nonterminals = NULL;
		eng->terminals = NULL;
		eng->productions = NULL;
		eng->last_production = NULL;
		eng->at_productions = NULL;

		/* LR parse */
		eng->lr_level = LALR1;
		eng->parse_method = TGLR;
		eng->classify_optimize = 0;
		eng->pattern_method = RBM;
		eng->numStates = 0;

		eng->pseudo_symbol = event_symbol_alloc(eng, "_pseudo", nel_type_alloc(eng, nel_D_EVENT, 0,0,0,0, nel_pointer_type,NULL) , 0, nel_C_NONTERMINAL,NULL);		
		eng->targetSymbol = event_symbol_alloc(eng, "_target", nel_type_alloc(eng,nel_D_EVENT,0,0,0,0, nel_pointer_type,NULL) , 0, nel_C_NONTERMINAL,NULL);		
		eng->target_id = eng->targetSymbol->id;


		eng->emptySymbol = event_symbol_alloc(eng, "_empty", nel_type_alloc(eng, nel_D_EVENT, 0,0,0,0,nel_pointer_type, NULL) , 0, nel_C_TERMINAL, NULL);	
		eng->endSymbol = event_symbol_alloc(eng, "_terminator", nel_type_alloc(eng, nel_D_EVENT, 0,0,0,0,nel_pointer_type,NULL), 0, nel_C_TERMINAL, NULL);

		eng->otherSymbol = event_symbol_alloc(eng, "_others", nel_type_alloc(eng, nel_D_EVENT,0,0,0,0,nel_pointer_type,NULL), 0, nel_C_TERMINAL, NULL);

		eng->notSymbol = event_symbol_alloc(eng, "_not", nel_type_alloc(eng, nel_D_EVENT, 0,0,0,0,nel_void_type,NULL), 0, nel_C_TERMINAL, NULL);

		eng->timeoutSymbol = event_symbol_alloc(eng, "_timeout",  nel_type_alloc(eng, nel_D_EVENT, 0,0,0,0, nel_pointer_type,NULL) , 0, nel_C_TERMINAL, NULL);

		{
			register nel_symbol *symbol;
			symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,"NULL"), nel_pointer_type, nel_static_value_alloc (eng, sizeof (void *), nel_alignment_of (void *)), nel_C_CONST, 0, nel_L_NEL, 0);
			*((void **) symbol->value) = (void *)0;
			
			symbol->declared = 1;
			parser_insert_ident (eng, symbol);

		}

		nel_ntrn_init (eng);
		//comp_init(eng);
		gen_init(eng);

		eng->init_flag = 1;

		//nel_unlock (&nel_init_lock);
	}

}

struct nel_eng *nel_eng_alloc(void)
{
	struct nel_eng *eng;
	nel_malloc(eng, 1, struct nel_eng );
	if(eng) {
		nel_zero(sizeof(struct nel_eng), eng );
		nel_init( eng );
	}

// move it into eng_init()
#if 0		
	//if(eng->compile_level > 0) {
		comp_init(eng);
	//}

	gen_init(eng);
#endif
	return eng;
}


void nel_eng_dealloc(struct nel_eng *eng)
{
	if(eng != NULL) {
		if(eng->exit && eng->compile_level != 1 ) {
			intp_eval_stmt_2 (eng, eng->exit);
		}

		if(eng->fini_head && eng->compile_level != 1 ) {
			intp_eval_stmt_2 (eng, eng->fini_head);
		}
	}

	if(eng->gen)
		gen_dealloc(eng);

	//if(eng->comp)
	//	comp_dealloc(eng);

	if(eng->parser)
		nel_dealloca(eng->parser); 

	event_dealloc(eng);
	
	line_dealloc(eng);
	list_dealloc(eng);
	type_dealloc(eng);
	member_dealloc(eng);
	block_dealloc(eng);

	stabtype_dealloc(eng);

	static_symbol_dealloc(eng);
	static_name_dealloc(eng);
	static_value_dealloc(eng);
	
	stmt_dealloc(eng);

	expr_dealloc(eng);
	expr_list_dealloc(eng);
	
	//free static hash table
	nel_static_hash_table_dealloc(eng->nel_static_name_hash);
	nel_static_hash_table_dealloc(eng->nel_static_ident_hash);
	nel_static_hash_table_dealloc(eng->nel_static_location_hash);
	nel_static_hash_table_dealloc(eng->nel_static_tag_hash);
	nel_static_hash_table_dealloc(eng->nel_static_file_hash);
}

//return: 0 if successfully, otherwise, return -1
int nel_eng_restart(struct nel_eng *eng)
{
	if(!eng || !eng->init_flag)
	{
		fprintf(stderr, "can not restart engine before engine initializing.\n");
		return -1;
	}
	
	//save useful params
	unsigned int stab_debug_level = eng->stab_debug_level;
	unsigned int term_debug_level = eng->term_debug_level;
	unsigned int nonterm_debug_level = eng->nonterm_debug_level;
	unsigned int prod_debug_level = eng->prod_debug_level;
	unsigned int itemset_debug_level = eng->itemset_debug_level;
	unsigned int termclass_debug_level = eng->termclass_debug_level;
	unsigned int nontermclass_debug_level = eng->nontermclass_debug_level;
	unsigned int function_debug_level = eng->function_debug_level;
	unsigned int declaration_debug_level = eng->declaration_debug_level;
	unsigned int compile_level = eng->compile_level;
	unsigned int optimize_level = eng->optimize_level;
	unsigned int verbose_level = eng->verbose_level;
	unsigned int stab_verbose_level = eng->stab_verbose_level;
	unsigned int analysis_verbose_level = eng->analysis_verbose_level;
	unsigned int parser_verbose_level = eng->parser_verbose_level;
	unsigned int intp_verbose_level = eng->intp_verbose_level;

	//unsigned int comp_verbose_level = eng->comp_verbose_level;
	unsigned int gen_verbose_level = eng->gen_verbose_level;
	char *prog_name = eng->prog_name;
	char **nel_file_name = eng->nel_file_name;
	int nel_file_num = eng->nel_file_num;
	
	//free engine
	nel_eng_dealloc(eng);

	//restart
	nel_zero(sizeof(struct nel_eng), eng );
	nel_init(eng);
	eng->stab_debug_level = stab_debug_level;
	eng->term_debug_level = term_debug_level;
	eng->nonterm_debug_level = nonterm_debug_level;
	eng->prod_debug_level = prod_debug_level;
	eng->itemset_debug_level = itemset_debug_level;
	eng->termclass_debug_level = termclass_debug_level;
	eng->nontermclass_debug_level = nontermclass_debug_level;
	eng->function_debug_level = function_debug_level;
	eng->declaration_debug_level = declaration_debug_level;
	eng->compile_level = compile_level;
	eng->optimize_level = optimize_level;
	eng->verbose_level = verbose_level;
	eng->stab_verbose_level = stab_verbose_level;
	eng->analysis_verbose_level = analysis_verbose_level;
	eng->parser_verbose_level = parser_verbose_level;
	eng->intp_verbose_level = intp_verbose_level;

	//eng->comp_verbose_level = comp_verbose_level;

	eng->gen_verbose_level = gen_verbose_level;
	eng->prog_name = prog_name;
	eng->nel_file_name = nel_file_name;
	eng->nel_file_num = nel_file_num;

	//stab parse
	if(stab_file_parse(eng, eng->prog_name) < 0)
		return -1;

	//nel file parse
	if(nel_file_parse(eng, eng->nel_file_num, eng->nel_file_name) != 0)
		return -1;
	
	return 0;
}
