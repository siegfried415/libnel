

/*****************************************************************************/
/* This file, "prod.c", contains routines for the manipulating symbol        */
/* the symbol table / hash table / value table entries for the application   */
/* executive.                                                                */
/*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "engine.h"
#include "errors.h"
#include "sym.h"
#include "prod.h"
#include "stmt.h"
#include "intp.h"
#include "evt.h"
#include "mem.h"
#include "gen.h" 

int lookup_prod_lhs(struct nel_eng *eng, nel_symbol *symbol)
{
	struct nel_SYMBOL *scan;
	for(scan = eng->productions; scan; scan = scan->next) {
		if(!nel_symbol_diff(scan->type->prod.lhs, symbol)){
			return 1;
		}
	}
	return 0;	
}


static struct nel_LIST *emptylist = NULL;
/******************************************************************************/
/* nel_rhs_copy_except() copy <rhs> list but <except> and return the newly    */
/* create one 								      */
/******************************************************************************/
struct nel_RHS *rhs_copy_except(struct nel_eng *eng, struct nel_RHS *rhs, struct nel_RHS *except)
{
	struct nel_RHS  *result = NULL, 
			*scan,
			*list;
	nel_symbol *symbol;
	int i, pos = 0, flag = 0;
	int j = 1;

	for(i=1, scan = rhs; scan; i++, scan = scan->next){
		if(flag == 0) { /* haven't seen except yet */
			if(scan == except ) {
				pos = i; flag ++; continue;
			}else {
				symbol = event_symbol_copy(eng, scan->symbol);
				nel_malloc(list, 1, struct nel_RHS);
				list->last = NULL;
				list->symbol = symbol;
				list->offset = j++;
			}
		}else {
			symbol = event_symbol_copy(eng, scan->symbol);
			/* update expr variable name such as $'N' to $'N-1', 
			if 'N' > pos. */ 
			if(symbol->value) { 
				
				if (nel_expr_update_dollar(eng, (union nel_EXPR *)symbol->value, pos, -1) < 0 ){
					/*NOTE,NOTE,NOTE, don't forget release 
					all lhs has been duplicated */
					nel_error(eng, nel_R_GENERATOR, 
						scan->filename, 
						scan->line, 
						"incorrect event variable indicator",NULL );
					return NULL;
				}
				
			}

			nel_malloc(list, 1, struct nel_RHS);
			list->last = NULL;
			list->symbol = symbol;
			list->filename = scan->filename;
			list->line = scan->line;
			list->offset = j++;
		}

		/* append the new created symbol to result */
		if(!result) 
			result = list;
		if(result->last) {
			result->last->next = list;
			list->prev = result->last;
		}

		list->next = NULL;
		result->last = list;
	}

	if(result)
		result->offset = pos + 1;
	return result;
}

/******************************************************************************/
/* nel_remove_empty_prods() removes <empty> event from every productions 's   */
/* rhs 's list, and create a new production. 		  		      */
/******************************************************************************/
void replace_empty_event(struct nel_eng *eng, nel_symbol *empty)
{
	struct nel_RHS *scan, *rhs, *nrhs;
	struct nel_SYMBOL *prod, 
			  *nprod,
			  *prev = NULL;
	struct nel_SYMBOL *lhs;

	prod = eng->productions;
	while(prod){
		nel_type *type = prod->type;
		if(type == NULL 
			|| type->simple.type != nel_D_PRODUCTION){
			goto do_next;
		}


		lhs = type->prod.lhs;
		for( scan = type->prod.rhs; scan; scan = scan->next) {
			nel_symbol *symbol = scan->symbol;
			if( symbol == empty ){
#if 0
				if(scan==type->prod.rhs && !scan->next)
					goto do_next;
#endif
				if((nrhs = rhs_copy_except(eng, 
				prod->type->prod.rhs,scan))) {

					/*update nrhs->last->stmts */
					nel_stmt *head, *tail;
					nel_stmt_dup(eng, prod->type->prod.stmts , &head, &tail);
					if(evt_stmt_update_dollar(eng,
						head, scan->offset, -1 ) < 0) {
						//parser_fatal_error(eng, "evt stmt update dollar error!\n");
						nel_error(eng, 
							nel_R_GENERATOR, 
							head->gen.filename, 
							head->gen.line, 
							"incorrect event variable indicator in stmt",
							NULL);
						goto do_next;
						
					}

					nprod = prod_symbol_alloc (eng, NULL, 
						nel_type_alloc(eng, 
							nel_D_PRODUCTION, 0, 
							nel_alignment_of(void *), 
							0, 0, 
							prod->type->prod.lhs, 
							prod->type->prod.rel, 
							nrhs, head)
						);
						
					/* eng->last_production can't be null
					because prod still in list */
					if( nprod && 
						eng->last_production != nprod){
						eng->last_production->next = nprod;		
						eng->last_production = nprod;
					}
#if 0
					if(prev)
						prev->next = prod->next;
					else
						eng->productions = prod->next;
					if(eng->last_production == prod)
						eng->last_production = prod;
					nel_dealloca(prod);
					prod = prev;						
#endif
					goto do_next;

				}
				
				/* nrhs == NULL, lhs can be empty */	
				 __append_entry_to(&emptylist, 
					__make_entry((struct nel_SYMBOL *)lhs));

				/* pick the prod out of eng->productions ,
				if empty was same as eng->emptySymbol */
				if( lookup_prod_lhs(eng, empty) == 0 )  {
					if(prev) { 
						prev->next = prod->next; 
					} else { 
						eng->productions=prod->next; 
					}
					if(eng->last_production == prod)
						eng->last_production = prev;
					
					goto do_next_2;
				}
				
				goto do_next;	
			}

		}

do_next:
		prev = prod;
do_next_2:
#if 0
		if(!prod)
			prod = eng->productions;
		else
#endif
		prod = prod->next;

	}

	return ;
}

/******************************************************************************/
/* nel_remove_empty_prods() removes all the empty events in all productions,  */
/* this been done by recusicelly calling replace_empty_event when found   */
/* an empty deriable event note that this function must be called after we    */
/* have finished the grammer parse, for production interception must be       */
/* consideration.		 			  		      */
/******************************************************************************/
int remove_empty_prods(struct nel_eng *eng)
{
	struct nel_SYMBOL *empty;

	/* insert _empty  into emptylist first */
	emptylist = NULL;
	__append_entry_to(&emptylist,
		__make_entry((struct nel_SYMBOL *)eng->emptySymbol));

	while(__get_count_of(&emptylist)) {
		empty = __get_first_out_from(&emptylist);
		replace_empty_event(eng, empty); 
	}

	return 0;

}



/******************************************************************************/
/* lookup_prod () finds the production with <symbol> as his lhs and       */
/* cnt as his rhs_num , returns a pointer to the production, or NULL if none  */
/* is found 								      */
/******************************************************************************/
struct nel_SYMBOL *lookup_prod(struct nel_eng *eng, nel_symbol *symbol, int cnt)
{
	struct nel_SYMBOL *scan;
	for(scan = eng->productions; scan; scan = scan->next) {
		if(!nel_symbol_diff(scan->type->prod.lhs, symbol) 
				&& scan->type->prod.rhs_num == cnt){
			return scan ;
		}
	}
	return NULL;	
}



/******************************************************************************/
/* prod_symbol_alloc() malloc a production and insert it into             */
/* eng->productions. 						      */
/*   struct nel_eng *eng;            				              */
/*   char *name: identifier string,should reside in nel's name table ,        */
/*			it is not automatically copied there.		      */
/*   nel_type *type;	type descriptor					      */
/******************************************************************************/
nel_symbol *prod_symbol_alloc(struct nel_eng *eng, char *name, union nel_TYPE *type)
{
	nel_symbol *retval;
	nel_symbol *prod;

	if(type->simple.type != nel_D_PRODUCTION) {
		gen_error (eng, "nel_event_symbol_alloc: not an production type for production (%s) !\n", name);
		return NULL;
	}

	prod = eng->productions;
	while( prod ) {
		if(nel_type_diff( prod->type, type, 1 ) == 0 ) {
			/* this production has already in eng->productions */
			return NULL;
		}
		prod = prod->next;
	}


	nel_malloc(retval, 1, nel_symbol);
	if(!retval ){
		gen_error (eng, "production symbol (%s) malloc error!\n", name);
		return NULL;
	}

	/**********************************************************************/
	/* the major part of production has initialized in type, so we can    */
	/* avoid some dirty work here 					      */
	/**********************************************************************/
	retval->name =  name; //strdup(name);
	retval->type = type;
	retval->class = nel_D_PRODUCTION;
	retval->value = (char *)0;

	/**********************************************************************/
	/* production symbol has his own name space, so id was counted by     */
	/* eng->numProductions. 					      */
	/**********************************************************************/
	retval->id  = eng->numProductions++;


	/**********************************************************************/
	/* insert production into linked list start by eng->productions   */
	/* ended by eng->last_production. 				      */
	/**********************************************************************/
	switch(type->prod.rel) {
		case REL_EX:
		case REL_ON:
		case REL_OR:
		case REL_OT:
			//(eng->last_production) ? eng->last_production->next: 
			//	eng->productions = retval;
			if(eng->last_production) 
				eng->last_production->next = retval;
			else
				eng->productions = retval;
			eng->last_production = retval;
			break;

		case REL_AT:
			gen_warning(eng, "production '->' not supported!\n");
		default:
			prod_symbol_dealloc(eng, retval);
			retval = NULL;
	}

	retval->next = NULL;	
	return retval;

}

struct nel_SYMBOL *others_prod_alloc(struct nel_eng *eng, int rhs_number)
{
	register nel_symbol *lhs;
	struct nel_SYMBOL *retval;
	struct nel_RHS *this;
	struct nel_RHS *rhs = NULL;
	int i;

	if(rhs_number <= 0 ) {
		gen_error (eng, "others_prod_alloc: the count of rhs must larger then 0 !\n");
		return NULL;
	}

	if((lhs = eng->otherSymbol) == NULL) {
		gen_error (eng, "others_prod_alloc: others symbol not declared!\n");
		return NULL;
	}

	for(i = 0; i < rhs_number; i++ ) {
		nel_malloc(this, 1, struct nel_RHS );
		this->symbol = eng->otherSymbol;
		if ( rhs == NULL) {
			rhs = this; /*rhs->last_stmt = NULL;*/ this->prev = NULL;
		} else if(rhs->last) {
			rhs->last->next = this; this->prev = rhs->last;
		}

		this->next = NULL;
		this->offset = i + 1;
		/*this->stmts = this->last_stmt=  NULL;*/
		rhs->last = this;
		/*rhs->timeout = 0;*/
	}

	retval = prod_symbol_alloc (eng, NULL, 
		nel_type_alloc(eng, nel_D_PRODUCTION, 0, nel_alignment_of (void *), 0, 0, lhs, REL_ON, rhs, NULL ));

	return retval;	
}

struct nel_SYMBOL *pseudo_prod_alloc(struct nel_eng *eng)
{
	register nel_symbol *lhs;
	struct nel_RHS *rhs, *this;
	struct nel_SYMBOL *retval;

	if((lhs = eng->pseudo_symbol) == NULL) {
		gen_error (eng, "pseudo_prod_alloc: pseudo symbol not declared!\n");
		return NULL;
	}

	nel_malloc(rhs, 1, struct nel_RHS );
	if ((rhs->symbol = eng->targetSymbol) == NULL){
		gen_error (eng, "pseudo_prod_alloc: target symbol not declared!\n");
		//NOTE,NOTE,NOTE, don't forget free rhs list
		return NULL;
	}

	rhs->offset = 1;
	rhs->next = rhs->prev = NULL;
	/*rhs->stmts = NULL;*/
	/*rhs->last_stmt = NULL;*/


	nel_malloc(this, 1, struct nel_RHS );
	if (( this->symbol = eng->endSymbol) == NULL){
		gen_error (eng, "pseudo_prod_alloc: end symbol not declared!\n");
		//NOTE,NOTE,NOTE, don't forget free rhs list
		return NULL;
	}

	this->offset = 2;
	/*this->stmts = NULL;*/
	/*this->last_stmt = NULL;	*/
	this->next = NULL;
	this->prev = rhs;
	rhs->next = this;
	rhs->last = this;

	retval = prod_symbol_alloc (eng, NULL, nel_type_alloc(eng, nel_D_PRODUCTION, 0, nel_alignment_of (void *), 0, 0, lhs, REL_ON, rhs, NULL ));

	return retval;	

}

/******************************************************************************/
/* prod_symbol_dealloc() get rid of production from eng->productions  */
/* list and free the the production 					      */
/******************************************************************************/
void prod_symbol_dealloc(struct nel_eng *eng, nel_symbol *prod)
{
	nel_symbol *scan, *prev; 
	for(scan=eng->productions, prev=NULL; scan; prev=scan,scan=scan->next) {
		if( scan == prod) {
			//(prev) ? prev->next : eng->productions = prod->next;
			if(prev)
				prev->next = prod->next;
			else
				eng->productions = prod->next ;
			if(eng->last_production == prod) eng->last_production = prev;
			nel_free(prod);	
			break;
		}
	}
	return ;

}


void emit_prods(struct nel_eng *eng /*,FILE *fp*/)
{
	FILE *fp;
	if(eng->prod_debug_level){
		if((fp=fopen("productions", "w"))) {
			struct nel_SYMBOL *prod;
			fprintf(fp, "Productions: \n");
			for(prod=eng->productions;prod;prod=prod->next) {
				emit_symbol(fp, prod);
			}
			fclose(fp);
		}
	}
}
