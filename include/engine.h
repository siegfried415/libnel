
/*****************************************************************************/
/* This file, nel_eng.h, contains declarations for struct nel_eng.    */
/* one such structure is kept for each invocation of the interpreter (and    */
/* one is created when the stab parser is invoked) a pointer to it passed    */
/* throughout most of the calling sequence.  It also contains declarations   */
/* for the semantic stack (contained in the engine ) and macros which    */
/* push and pop it.                                                          */
/*****************************************************************************/

#ifndef ENGINE_H
#define ENGINE_H

#include <stdio.h>
#include <setjmp.h>
#include <elf.h>

/* wyong, 20230804 
#include "stream.h"
#include "match.h"
#include "shellcode.h"
*/

/* wyong, 2006.2.27 */
#include <stdlib.h>
void *calloc(size_t nmemb, size_t size);
void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);




/*******************/
/* debugging macro */
/*******************/
#ifdef NEL_DEBUG
#define nel_debug(_x)	_x
#else
#define nel_debug(_x)
#endif /* NEL_DEBUG */

struct peek_arg_link
{
	char type;
	char *name;
	struct peek_arg_link *next;
};
typedef struct peek_arg_link peek_arg_link_t;


#define NEL_TRACE_LEVEL	3
#define NEL_OVER_LEVEL	2	
#define NEL_DIAG_LEVEL	2	
#define NEL_WARN_LEVEL	1
#define NEL_ERROR_LEVEL 0
#define NEL_FATAL_LEVEL 0 

/*************************************************************/
/* discriminator constants for the nel_eng .  they begin */
/* with "nel_R_" for Rec, and are called "nel_R_tokens".       */
/*************************************************************/
typedef enum nel_R_TOKEN {
        nel_R_NULL = 0,
        nel_R_SIMPLE,
        nel_R_PARSER,
        nel_R_STAB,
        nel_R_INTP,
        nel_R_GENERATOR,
	nel_R_ANALYSIS,
        nel_R_MAX
} nel_R_token;


struct eng_comp;
struct nel_SYMBOL;
struct dll_ref ;
struct section ;
typedef unsigned int StateId;
typedef int ActionEntry;
typedef unsigned int GotoEntry;
typedef int SymbolId;

struct nel_eng {
	unsigned int init_flag;
	/*nel_lock_type nel_init_lock;*/

	int parser_init_flag ;
	int intp_init_flag ;
	int lr_init_flag ;
	int comp_init_flag ;

	peek_arg_link_t *peek_arg_link_begin ;
	int global_set_peek_flag ;

	unsigned int stab_debug_level : 1;
	unsigned int term_debug_level : 1;
	unsigned int nonterm_debug_level : 1;
	unsigned int prod_debug_level : 1;
	unsigned int itemset_debug_level : 1;
	unsigned int termclass_debug_level : 1;
	unsigned int nontermclass_debug_level : 1;
	unsigned int function_debug_level : 1;
	unsigned int declaration_debug_level : 1;
			
	/*some parameter */
	unsigned int compile_level: 2;	
	unsigned int optimize_level : 2 ;
	unsigned int verbose_level : 2;
	unsigned int stab_verbose_level : 2 ;
	unsigned int analysis_verbose_level : 2 ;
	unsigned int parser_verbose_level : 2;
	unsigned int intp_verbose_level : 2;
	unsigned int comp_verbose_level : 2;
	unsigned int gen_verbose_level : 2 ;

	struct nel_SYMBOL *nel_static_symbols_next;
	struct nel_SYMBOL *nel_static_symbols_end ;
	struct nel_SYMBOL *nel_free_static_symbols ;
	struct nel_SYMBOL_TABLE *nel_static_name_hash;
	struct nel_SYMBOL_TABLE *nel_static_ident_hash;
	struct nel_SYMBOL_TABLE *nel_static_location_hash;
	struct nel_SYMBOL_TABLE *nel_static_tag_hash;
	struct nel_SYMBOL_TABLE *nel_static_file_hash;

	/*********************************************/
	/* vars for saving terminals, nonterminals, */
	/* and production parse output */
	/*********************************************/
	int data_id;
	int numTerminals  ;
	int numNonterminals ;
	int numProductions;
	struct nel_SYMBOL *nonterminals ;
	struct nel_SYMBOL *last_nonterminal ;
	struct nel_SYMBOL *targetSymbol ;
	int target_id;
	struct nel_SYMBOL *notSymbol ;
	struct nel_SYMBOL *otherSymbol ;
	struct nel_SYMBOL *timeoutSymbol ;
	struct nel_SYMBOL *pseudo_symbol;
	struct nel_SYMBOL *terminals ;	
	struct nel_SYMBOL *last_terminal ;
	struct nel_SYMBOL *emptySymbol ;
	struct nel_SYMBOL *startSymbol ;
	int start_id;
	struct nel_SYMBOL *endSymbol ;
	int end_id;
	struct nel_SYMBOL *productions ;
	struct nel_SYMBOL *last_production ;
	struct nel_SYMBOL *at_productions ;
	
	/********************************************/
	/* vars for saving LR parse output include  */
	/* various of itemsets, action table, and */
	/* goto table, which will be used in analysis */
	/* module.				*/
	/*******************************************/
	int lr_level;
	int parse_method ;
	int classify_optimize;
	int pattern_method;

	int numStates;
	struct stateInfo *stateTable;
	struct nel_SYMBOL **symbolInfo;

	/********************************************************/
	/* map production id to information about the production*/
	/********************************************************/
	struct prodinfo *prodInfo;

	/********************************************************/
	/*  classifier infomation of each state'		*/
	/********************************************************/
	struct classInfo *classTable;

	/********************************************************/
	/* action table, indexed by (state*numTerms+lookahead),	*/
	/* each entry is one of:				*/
	/* +N+1, 0 <= N < numStates: shift, and go to state N	*/
	/* -N-1, 0 <= N < numProds:  reduce using production N	*/
	/* numStates+N+1, 0 <= N < numAmbig: ambiguous, use ambigAction N */ 
	/* 0:                                error		*/
	/* (there is no 'accept', acceptance is handled outside this table)*/
	/********************************************************/
	ActionEntry *actionTable; 
	ActionEntry *default_action_tbl;

	/************************************************************/
	/* goto table, indexed by (state*numNonterms + nontermId),  */
	/* each entry is N, the state to go to after having reduced */
	/* by 'nontermId'					    */
	/************************************************************/
	GotoEntry *gotoTable;

	/********************************************************/
	/* ambiguous action table, indexed by ambigActionId;	*/
	/* each entry is a pointer to an array of signed short; */
	/* the first number is the # of actions, and is followed */
	/* by that many actions, each interpreted the same way 	*/
	/* ordinary 'actionTable' entries are 			*/
	/********************************************************/
	ActionEntry* ambigAction;  
	int numAmbig ;

	/********************************************************/
	/* tables to describe whether a state is in a to_be_reduce */
	/* state or to_be_shift state */
	/********************************************************/
	int *has_reduce_tbl;	
	int *has_shift_tbl;
	

	/********************************************/	
	/* data structure for compiler output */
	/********************************************/
	struct section *symtab_section;
	struct section *strtab_section;
	struct section *stab_section;
	struct section *stabstr_section;
	struct section *text_section;
	struct section *data_section;
	struct section *bss_section;
	struct section *cur_text_section;
	struct section *bounds_section;
	struct section *lbounds_section;
	int output_type;
	/* include file handling */
	char **include_paths;
	int nb_include_paths;
	char **sysinclude_paths;
	int nb_sysinclude_paths;
	//struct cached_include **cached_includes;
	int nb_cached_includes;

	char **library_paths;
	int nb_library_paths;

	/*array of all loaded dlls 
	(including those referenced by loaded dlls)*/
	struct dll_ref **loaded_dlls;
	int nb_loaded_dlls;

	/* sections */
	struct section **sections;
	int nb_sections; /* number of sections, 
			including first dummy section */

	/* got handling */
	struct section *got;
	struct section *plt;
	unsigned long *got_offsets;
	int nb_got_offsets;
	/* give the correspondance from symtab indexes to dynsym indexes */
	int *symtab_to_dynsym;

	/* temporary dynamic symbol sections (for dll loading) */
	struct section *dynsymtab_section;
	/* exported dynamic symbol section */
	struct section *dynsym;

	int nostdinc; /* if true, no standard headers are added */
	int nostdlib; /* if true, no standard libraries are added */

	/* if true, static linking is performed */
	int static_link;

	/* if true, all symbols are exported */
	int rdynamic;

	/* if true, only link in referenced objects from archive */
	int alacarte_link;

	/* warning switches */
	int warn_write_strings;
	int warn_unsupported;
	int warn_error;
	int warn_none;

	union nel_STMT *init_head, *init_tail;
	union nel_STMT *fini_head, *fini_tail;
	union nel_STMT *main;
	union nel_STMT *exit;

	/* tiny assembler state */
	struct nel_SYMBOL *asm_labels;

	struct eng_stab *stab;
	struct eng_gram *parser;
	struct eng_gen *gen;
	struct eng_intp *intp;
	struct eng_comp *comp;

	//added by zhangbin, 2006-7-26, to support eng restart
	char *prog_name;
	char **nel_file_name;
	int nel_file_num;
	//end
};


/*********************************************************/
/* declarations for the routines defined in "engine.c". */
/*********************************************************/
extern void nel_init (struct nel_eng *eng);
extern struct nel_eng *nel_eng_alloc(void);
extern void  nel_eng_dealloc(struct nel_eng *eng);

extern int  nel_file_parse(struct nel_eng *, int, char **);
extern int stab_file_parse(struct nel_eng *, char *);

//added by zhangbin, 2006-7-26
extern int nel_eng_restart(struct nel_eng *eng);
//end
#endif /* ENGINE_H */

