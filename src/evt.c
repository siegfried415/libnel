
/*****************************************************************************/
/* This file, "event.c", contains routines for the manipulating symbol  */
/* the symbol table / hash table / value table entries for the application   */
/* executive.                                                                */
/*****************************************************************************/
#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "errors.h"
#include "parser.h" 
#include "sym.h"
#include "evt.h"
#include "stmt.h"
#include "intp.h"
#include "mem.h"


/*****************************************************************************/
/* lookup_event_symbol () finds the symbol named <name> by searching the */
/* nonterminals list and terminals list in order, until the symbol is found, */
/* or we run out of lists. It returns a pointer to the symbol, or NULL if none*/
/* is found*/
/*****************************************************************************/
nel_symbol *lookup_event_symbol(struct nel_eng *eng, char *name) 
{
	nel_symbol *scan;

	/**********************************************************************/
	/* Although terminal and nonterminal has their own name space,that is*/
	/* the id was counted by eng->numTerminals and eng->numNonterminals */ 
	/* sperately. but when the a terminal exist, the same name nonterminal*/
	/* was not allowed, this will simpliy the design of nel at the cost of*/
	/*decreasing the language 's integrality */
	/*********************************************************************/
	scan = eng->nonterminals;
	while( scan ) {
		if(strcmp(scan->name, name) == 0) return scan;
		scan = scan->next;
	}

	/**********************************************************************/
	/* if the symbol not in eng->nonterminals list, then search terminals */
	/**********************************************************************/
	scan = eng->terminals;
	while(scan){
		if(strcmp(scan->name, name) == 0) return scan;
		scan = scan->next;
	}


	return NULL;

}




/*****************************************************************************/
/* event_symbol_alloc () malloc a event symbol and insert it into eng->  */
/* terminals when it is an terminal, and eng->nonterminals when nonterminal. */ 
/*   struct nel_eng *eng;						*/
/*   char *name: identifier string,should reside in nel's name table ,	*/
/*			it is not automatically copied there.		*/
/*   nel_type *type;	type descriptor					*/
/*   char *value;		points to memory where value is stored	*/
/*   nel_C_token class;	symbol class					*/
/*   unsigned_int lhs;	legal left hand side of an assignment		*/
/*   nel_L_token source_lang; source language				*/
/*   int level;		scoping level					*/
/*****************************************************************************/
nel_symbol *event_symbol_alloc(struct nel_eng *eng, char *name, union nel_TYPE *type, int isolate, int class, union nel_EXPR *expr)
{
	nel_symbol *retval;

	nel_malloc(retval, 1, nel_symbol);
	if(!retval ){
		parser_error (eng, "event symbol (%s) malloc error!\n", name);
		return NULL;
	}

	retval->name =  name; //strdup(name);
	retval->type = type;
	retval->class = class;
	retval->value = (char *)expr;
	retval->data = NULL;

	/*********************************************************************/
	/* event (terminal & nonterminal) has their own name space, so id was */
	/* count by eng->numTerminals / eng->numNonterminals by sperate. */
	/* use linked list to chain all the terminals/nonterminals by seprate.*/
	/* rather then a hash, because the number of event won't be that much.*/
	/**********************************************************************/
	if( retval->class == nel_C_TERMINAL) {
		if(eng->last_terminal)
			eng->last_terminal->next = retval;
		else
			eng->terminals = retval;
		eng->last_terminal = retval;
		retval->id = eng->numTerminals ++;
	}
	else if(retval->class == nel_C_NONTERMINAL){
		if(eng->last_nonterminal)
			eng->last_nonterminal->next = retval;
		else
			eng->nonterminals = retval;
		eng->last_nonterminal = retval;
		retval->id  = eng->numNonterminals ++;
	}


	/**********************************************************************/
	/* make a struct nel_EVENT data structure for the symbol, which saving*/
	/* the necessary information of EVENT, so that the normal symbol won't*/
	/* need malloc memory space for those unnecessary info,this way, we   */
	/* can save a lot of memory usage */
	/**********************************************************************/
	
	retval->aux.event = nel_event_alloc(eng);	
	//nel_malloc(retval->aux.event, 1, struct nel_EVENT);
	if(!retval->aux.event ){
		event_symbol_dealloc(eng, retval);	
		return NULL;
	}
	
	/**********************************************************************/
	/*pid is id of parent event , set pid equal id means the event has no */
	/* expression with it therefore need not an parent. */
	/**********************************************************************/
	retval->_pid = retval->id;

	retval->_parent = retval;
	retval->_cyclic = 0;
	retval->_flag = 0;
	//retval->_nodelay = 0;
	retval->_isEmptyString = 0;
	retval->_reachable = 0;
	retval->_deep = 0;
	retval->_state = -1;

	retval->_isolate = isolate;

	retval->next = (nel_symbol *)0;
	return (retval);	

}


/******************************************************************************/
/* event_symbol_dealloc() get rid of event symbol from eng->terminals list */
/* or eng->nonterminals, and  free the the event symbol */
/******************************************************************************/
void event_symbol_dealloc(struct nel_eng *eng, nel_symbol *symbol)
{
	nel_symbol *scan = NULL, *prev; 

	if( symbol->class == nel_C_TERMINAL) {
		for(scan=eng->terminals, prev=NULL; scan; prev=scan,scan=scan->next) {
			if( scan == symbol) {
				//(prev) ? prev->next : eng->terminals = symbol->next;
				if(prev)
					prev->next = symbol->next;
				else
					eng->terminals = symbol->next;
				if(eng->last_terminal == symbol) eng->last_terminal = prev;
				break;
			}
		}
	}

	else if(symbol->class == nel_C_NONTERMINAL){
		for(scan=eng->nonterminals, prev=NULL; scan; prev=scan,scan=scan->next) {
			if( scan == symbol) {
				//(prev) ? prev->next : eng->nonterminals = symbol->next;
				if(prev)
					prev->next = symbol->next;
				else
					eng->nonterminals = symbol->next;
				if(eng->last_nonterminal == symbol) eng->last_nonterminal = prev;
				break;
			}
		}

	}

	if(scan) {
#if 1	
		nel_event_dealloc(symbol->aux.event);
		nel_free(symbol);	
#else
		free(symbol->aux.event);
		free(symbol); 
#endif
	}

	return ;

}

/******************************************************************************/
/* event_symbol_copy () create a event symbol 's copy  and insert it into */
/* eng->terminals when it is an terminal, and eng->nonterminals when nonterminal */ 
/******************************************************************************/
nel_symbol *event_symbol_copy(struct nel_eng *eng, nel_symbol *symbol)
{
	nel_symbol *retval;

	if(symbol == NULL) {
		parser_error (eng, "event_symbol_copy: event symbol is null !\n");
		return NULL;
	}

	/**********************************************************************/
	/* if no value (expression) associated with it, why should we copy a */
	/* new one ? */
	/**********************************************************************/
	if(symbol->value == NULL) {
		//nel_waring(eng, "event_symbol_copy: event symbol(%s) 's value is null!\n", 
		//	symbol->name);
		return symbol;
	}

	/**********************************************************************/
	/*then create an new event symbol with a duplicated value (expression)*/
	/**********************************************************************/
	if (!(retval = event_symbol_alloc(eng, symbol->name, symbol->type, 
		symbol->_isolate, 
		symbol->class, nel_dup_expr(eng, (union nel_EXPR *)symbol->value)))){
		return NULL;
	}

	retval->_parent = symbol->_parent;
	retval->_pid = symbol->_pid;


	return retval;

}

void emit_terminals(struct nel_eng *eng )
{
	FILE *fp;
	if(eng->term_debug_level) {
		if((fp=fopen("terminals", "w"))) {
			nel_symbol *t;
			fprintf(fp, "Terminals:\n");
			for(t = eng->terminals; t; t = t->next){
				fprintf(fp, "%d,", t->id);
				emit_symbol(fp, t);
				fprintf(fp, "\n");
			}
			fclose(fp);
		}
	}
}

void emit_nonterminals(struct nel_eng *eng )
{
	FILE *fp;
	if(eng->nonterm_debug_level ) {
		if((fp=fopen("nonterminals", "w"))) {
			nel_symbol *nt;
			fprintf(fp, "None Terminals:\n");
			for(nt=eng->nonterminals; nt; nt=nt->next){
				fprintf(fp, "%d,", nt->id);
					emit_symbol(fp, nt);
				fprintf(fp, "\n");
			}
			fclose(fp);
		}
	}

}

typedef struct nel_EVENT_CHUNK
{
	unsigned_int size;
	struct nel_EVENT *start;
	struct nel_EVENT_CHUNK *next;
}nel_event_chunk;

unsigned_int nel_events_max = 0x800;
nel_lock_type nel_events_lock = nel_unlocked_state;
static struct nel_EVENT *nel_events_next = NULL;
static struct nel_EVENT *nel_events_end = NULL;
static struct nel_EVENT *nel_free_events = NULL;
static nel_event_chunk *nel_event_chunks = NULL;

struct nel_EVENT *nel_event_alloc(struct nel_eng *eng)
{
	struct nel_EVENT *retval;
	nel_lock(&nel_events_lock);
	if(nel_free_events) {
		retval = nel_free_events;
		nel_free_events = nel_free_events->next;
	}
	else {
		if(nel_events_next >= nel_events_end) {
			nel_event_chunk *chunk;
			nel_malloc ( nel_events_next, nel_events_max, nel_event );
			nel_events_end = nel_events_next + nel_events_max;
			nel_malloc(chunk, 1, nel_event_chunk);
			chunk->start = nel_events_next;
			chunk->next = nel_event_chunks;
			nel_event_chunks = chunk;
		}
		retval = nel_events_next++;
	}
	nel_zero(sizeof(struct nel_EVENT), retval);
	nel_unlock(&nel_events_lock);
	return retval;
}

void nel_event_dealloc(struct nel_EVENT *evt)
{
	nel_lock(&nel_events_lock);
	evt->next = nel_free_events;
	nel_free_events = evt;
	nel_unlock(&nel_events_lock);
}

void event_dealloc(struct nel_eng *eng)
{
	while(nel_event_chunks) {
		nel_event_chunk *chunk = nel_event_chunks->next;
		nel_dealloca(nel_event_chunks->start); 
		nel_dealloca(nel_event_chunks); 
		nel_event_chunks = chunk;
	}

	nel_events_next = NULL;
	nel_events_end = NULL;
	nel_free_events = NULL;
}
