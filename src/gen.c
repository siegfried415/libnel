
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "engine.h"
#include "errors.h"
#include "sym.h"
#include "evt.h"
#include "prod.h"
#include "expr.h"
#include "stmt.h"
#include "intp.h"
#include "type.h"
#include "termset.h"
#include "itemset.h"
#include "class.h"
#include "mem.h"
#include "gen.h"
#include "free_func.h"
#include "action.h"
#include "io.h" 
#include "action.h" 

void set_transition(struct nel_eng *, struct itemset *, nel_symbol *, struct itemset *);
void do_gen_itemset_closure(struct nel_eng *, struct nel_LIST *, struct lritem *);
struct itemset *gen_itemset_create(struct nel_eng *,struct itemset *, struct lritem *,nel_symbol *, int, int );
int itemset_symbol_cnt(struct nel_eng *eng,  struct itemset *is, nel_symbol *sym, int flag );

/*****************************************************************************/
/* parser_warning () prints out an error message, together with the current  */
/* input filename and line number, and then returns without incrementing     */
/* eng->parser->error_ct.                                                    */
/*****************************************************************************/
int gen_warning (struct nel_eng *eng, char *message, ...)
{
	va_list args;

	if (eng && (eng->gen_verbose_level >= NEL_WARN_LEVEL ) ) {
		va_start (args, message);

		/*******************************************/
		/* error message i/o is a critical section */
		/*******************************************/
		nel_lock (&nel_error_lock);

		//if (eng->gen->type != nel_R_NULL) {
		//	fprintf (stderr, "\"%s\", line %d: warning: ", eng->gen->filename, eng->gen->line);
		//}
		//else {
			fprintf (stderr, "warning: ");
		//}

		nel_do_print (stderr, message, args);
		fprintf (stderr, "\n");

		/*************************/
		/* exit critical section */
		/*************************/
		nel_unlock (&nel_error_lock);

		va_end (args);
	}

	return (0);

}



/*****************************************************************************/
/* gen_error()prints out an error message, together with the current input*/
/* filename and line number, increments eng->gen->error_ct, and returns.  */
/*****************************************************************************/
int gen_error (struct nel_eng *eng, char *message, ...)
{
	va_list args;

	if ( eng && (eng->gen_verbose_level >= NEL_ERROR_LEVEL )) {
		va_start (args, message);

		/*******************************************/
		/* error message i/o is a critical section */
		/*******************************************/
		nel_lock (&nel_error_lock);

		//if ((eng != NULL) 
		//	&& (eng->gen->type!= nel_R_NULL) 
		//	&& (eng->gen->filename != NULL)){
		//	fprintf (stderr, "\"%s\", line %d: ", eng->gen->filename, eng->gen->line);
		//}

		nel_do_print (stderr, message, args);
		fprintf (stderr, "\n");

		/******************************************/
		/* exit critical section                  */
		/******************************************/
		nel_unlock (&nel_error_lock);
		va_end (args);
	}

	return (0);

}

static int reach_term_num ;
static int reach_nonterm_num ;

int gen_compute_reachable(struct nel_eng *eng, nel_symbol *nt)
{
	struct nel_RHS *scan;
	struct nel_SYMBOL *prod;

	if (nt->_parent->_reachable == 0) {
		reach_nonterm_num++;
		nt->_parent->_reachable = 1;

		if(nt->_isolate) 
			return 0;

		for(prod=eng->productions; prod; prod=prod->next) {
			if (prod->type->prod.lhs != (nel_symbol *)nt->_parent) {
				continue;
			}

			for(scan=prod->type->prod.rhs; scan; scan = scan->next){
				nel_symbol *symbol = scan->symbol;
				if(symbol->class == nel_C_NONTERMINAL) {
					gen_compute_reachable(eng, symbol->_parent);
				} else if(symbol->class==nel_C_TERMINAL) {
					if(symbol->_parent->_reachable == 0 ) {
						symbol->_parent->_reachable = 1;
						reach_term_num++;
					}
				}
			}
		}
	}

	return 0;

}


#define DERIVABLE(eng,l,r) (eng->gen->derivable[l*eng->numNonterminals + r])
int gen_has_cyclic(struct nel_eng *eng)
{
	int i, j;
	int flag = 0;

	for( i = 0; i < eng->numNonterminals; i ++) {
		for(j = i; j < eng->numNonterminals; j ++) {
			if (DERIVABLE(eng, i, j) != 0
			&& DERIVABLE(eng, i, j) == DERIVABLE(eng, j, i)) {
				flag = 1;
			}
		}
	}
	return flag;
}


int gen_can_derivable(struct nel_eng *eng, nel_symbol *left, nel_symbol *right)
{
	return DERIVABLE(eng, left->id, right->id);
}


int gen_can_derivable_empty(struct nel_eng *eng, nel_symbol *nonterm)
{
	return gen_can_derivable(eng, nonterm, eng->emptySymbol);
}

int gen_add_derivable(struct nel_eng *eng, nel_symbol *left, nel_symbol *right)
{
	int changed = 0;

	/*--------------------------------------------------------------
		Almost as an aside, I'd like to track cyclicity in 
		grammars. It's always true that N ->* N, because 0 steps 
		are allowed.  A grammar is cyclic if N ->+ N, i.e. 
		it derives itself in 1 or more steps.  
		We can detect that fairly easily by tracking calls to 
		this fn with left==right.  Since N ->* N in 0 steps is 
		recorded during init (and *not* by calling this fn), the 
		only calls to this with left==right will be when the 
		derivability code detects a nonzero-length path.
	---------------------------------------------------------------*/

	if (left==right) {
		if (!left->_cyclic) {
			left->_cyclic = 1;
			eng->gen->cycle = 1;     // for grammar as a whole

			/*-------------------------------------------------
				Even though we didn't know this already, 
				it doesn't constitute a change in the 
				->* relation (which is what the derivability 
				code cares about), so we do *not* report a
				change for the cyclicty detection. 
			 -------------------------------------------------*/
		}
	}

	/*-----------------------------------------------------------
		We only made a change, and hence should return true,
		if there was a 0 here before
	 -----------------------------------------------------------*/
	if (eng->gen->derivable[left->id * eng->numNonterminals + right->id ]==0) {
		eng->gen->derivable[left->id * eng->numNonterminals + right->id ] = 1;
		changed = 1;
	}

	return changed;
}

int gen_compute_derivable(struct nel_eng *eng)
{
	struct nel_RHS *sl;
	struct nel_SYMBOL *prod;
	int changes = 1;

	while(changes) {
		changes = 0;
		for (prod = eng->productions; prod; prod = prod->next) {

			if ( prod->type->prod.rhs->symbol == eng->emptySymbol) {
				gen_add_derivable(eng, prod->type->prod.lhs, eng->emptySymbol);
				continue;
			}

			for( sl= prod->type->prod.rhs; sl ; sl = sl->next) {
				nel_symbol *sym = sl->symbol;
				if (sym->class == nel_C_TERMINAL) {
					break;
				}

				if(!gen_can_derivable(eng, prod->type->prod.lhs, sym)) {
					struct nel_RHS *rsl;
					int gen_can_derivable = 1;

					for(rsl = sl->next; rsl; rsl=rsl->next){
						if (rsl->symbol->class== nel_C_TERMINAL
							||!gen_can_derivable_empty(eng,rsl->symbol)
						   ) {
							gen_can_derivable = 0 ;
							break;
						}
					}

					if(gen_can_derivable) {
						gen_add_derivable(eng,prod->type->prod.lhs, sym);
						changes++;
					}

				}

				if(!gen_can_derivable_empty(eng, sym)) {
					break;
				}
			}
		}
	}

	if( gen_has_cyclic(eng)) {
		gen_error(eng, "there are cyclic in productions!\n");
		return -1;
	}

	return 0;
}

int gen_derivable_init(struct nel_eng *eng)
{
	int i, j;
	nel_malloc(eng->gen->derivable, eng->numNonterminals * eng->numNonterminals, char);

	for(i=0; i < eng->numNonterminals; i++) {
		for(j= 0; j < eng->numNonterminals; j++) {
			/*------------------------------------------------
				Every nonterminal can derive itself in 0 
				or more steps(specifically, in 0 steps, 
				at least) 
				eng->gen->derivable[i* eng->numNonterminals 
				+j] = (i==j) ;	
			 -------------------------------------------------*/
			eng->gen->derivable[i * eng->numNonterminals +j] = 0;
		}
	}

	return 0;
}


/*-----------------------------------------------------------------------
Compute, for each nonterminal, the "First" set, defined as:
First(N) = { x | N ->* x alpha }, where alpha is any sequence
				  of terminals or nonterminals
 
If N can derive emptySymbol, I'm going to say that empty is
*not* in First, despite what Aho/Sethi/Ullman says.  I do this
because I have that information readily as my derivable relation,
and because it violates the type system I've devised.
 
I also don't "compute" First for terminals, since they are trivial
(First(x) = {x}).
 -----------------------------------------------------------------------*/
int gen_compute_symbol_first(struct nel_eng *eng)
{
	nel_symbol *i, *j;
	int changes ;
	struct nel_SYMBOL *prod;

REDO_FIRST:
	changes =0;
	for(prod = eng->productions; prod; prod=prod->next) {
		struct nel_RHS *sl;
		TerminalSet ts;
		struct nel_SYMBOL *lhs;

		/* skip process production like 'not' : 'not' ...,
		 'others' : ' others', which are some pseudo productions  */
		lhs = prod->type->prod.lhs;
		if ( lhs->class == nel_C_TERMINAL) {
			continue;
		}

		termset_init(&ts, eng->numTerminals);

		//if(prod->type->prod.rel == REL_EX) {
		//	termset_set(&ts, eng->notSymbol->id);
		//} else 
		if(prod->type->prod.rel == REL_OT) {
			termset_set(&ts, eng->otherSymbol->id);
		}

		for(sl = prod->type->prod.rhs; sl; sl = sl->next) {
			/*then check if this can offer something to ts */
			if( sl->symbol->class == nel_C_TERMINAL ) {
				termset_set_2(eng, &ts, sl->symbol->id );
				break;
			}

			termset_merge_2(eng, &ts, &(sl->symbol->_first));
			if(!gen_can_derivable_empty(eng, sl->symbol)) {
				break;
			}
		}

		//termset_merge(&prod->firstSet,&ts);
		if(termset_merge(&(prod->type->prod.lhs->_first),&ts)) {
			changes++;
		}

		nel_dealloca(ts.bitmap); 
	}

	for(i= eng->nonterminals ; i ; i = i->next) {
		for( j = eng->nonterminals ; j; j=j->next) {
			if ( (i!=j) && (!strcmp(i->name, j->name))) {
				changes += termset_merge(&i->_first, &j->_first);
			}
		}
	}

	if (changes) {
		goto REDO_FIRST;
	}

	return 0;
}

/*
'timeout' is in dprod level, so we inplenment it here.
on the contract,'others' will be not processed until 
itemset is established in itemSetClosure.  
*/
int gen_compute_dprod_first(struct nel_eng *eng)
{
	struct nel_SYMBOL *prod;

	for(prod=eng->productions; prod; prod=prod->next) {
		struct nel_RHS *sl;
		int pos;
		nel_symbol *lhs = prod->type->prod.lhs;
		if(lhs->class == nel_C_TERMINAL ) {
			continue;
		}

		for(sl=prod->type->prod.rhs, pos=0; pos<= prod->type->prod.rhs_num; pos++,sl=sl->next) {
			struct nel_RHS *rsl;
			struct dprod *dprod;

			dprod = get_dprod(eng, prod, pos);
			termset_init(&dprod->firstSet, eng->numTerminals);
			if (!sl) {
				dprod->canDeriveEmpty = 1;
				break;
			}

			/*
			when we meet productions like 'A: B; A: others;',
			we need not set 'other' flag on in lookahead of dprod 
			derive from 'A:others'.
			*/
			if( prod->type->prod.rel != REL_OT
				&& termset_test(&lhs->_first, eng->otherSymbol->id)) {
				termset_set(&dprod->firstSet, eng->otherSymbol->id);
			}

			for(rsl = sl; rsl ; rsl = rsl->next) {
				if(rsl->symbol->class == nel_C_TERMINAL ) {
					termset_set_2(eng, &dprod->firstSet,rsl->symbol->id);
					break;
				}

				termset_merge_2(eng, &dprod->firstSet,&rsl->symbol->_first);
				if(!gen_can_derivable_empty(eng, rsl->symbol)) {
					break;
				}
			}

			if (!rsl) {
				dprod->canDeriveEmpty = 1;
			}
		}
	}

	return 0;

}

int gen_first_follow_init(struct nel_eng *eng)
{
	nel_symbol *nt;
	for (nt = eng->nonterminals; nt; nt=nt->next) {
		termset_init(&nt->_first, eng->numTerminals);
		//termset_init(&nt->_follow, eng->numTerminals);
	}

	return 0;
}

void do_gen_itemset_closure(struct nel_eng *eng, struct nel_LIST *donelist, struct lritem *it)
{
	nel_symbol *B;
	struct nel_SYMBOL *prod;
	int flag ;

	if ( it ) {
		/*---------------------------------------------
			get first itemset from pendlist,
			and append it to donelist 
		 --------------------------------------------*/
		__append_entry_to(&donelist, __make_entry((struct nel_SYMBOL *)it));

		if (is_dot_at_end(it->dprod)) {
			/* Since 'other','timeout' and 'not' are not for LR
			lookahead alogrithm , so clear it. */
			termset_unset(&it->lookahead, eng->otherSymbol->id);
			termset_unset(&it->lookahead, eng->timeoutSymbol->id);
			return ;
		}

		B = sym_after_dot(it->dprod);
		if(B->class == nel_C_TERMINAL) {
			return ;
		}

		/* get all production accroding to the it 's dprod */
		prod = eng->productions;
		flag = REL_EX;
		while(prod) {
			struct dprod *ndp, *beta;
			TerminalSet ts;
			struct lritem *nit;
			//nel_symbol *term;

			/* use this tech to have the 'REL_EX' production
			be processed first. */
			if (prod->type->prod.rel != flag
				|| strcmp(prod->type->prod.lhs->name, B->name)){
				goto get_next_prod;
			}

			/*---------------------------------------------
				ok, we got an production which is left 
				is sym 
			 ---------------------------------------------*/
			ndp = get_dprod(eng, prod, 0);
			beta = get_next_dprod(it->dprod);
			termset_init(&ts, eng->numTerminals);

			/*--------------------------------------------
			nit 's lookahead can be set from two way:
			1, if firstSet of it->dpor has 'other' set,
			for example, A: B;  A:other; then 
			firstSet of item 'A : * B' include 
			'other' ; 
			2, if lookahead of it has 'other' set,
			for example, B: C D, 'other'; then 
			B : *C D, 'other', and B: C * D, 'other' 
			this is becaue 'other' can be happend 
			anywhere. 	
			
			Note, the 2 is far different from normal logic
			of lookahead, because normal lookahead can only
			happen after the C and D,  
			 --------------------------------------------*/
			if(termset_test(&it->dprod->firstSet, eng->otherSymbol->id) 
				|| termset_test(&it->lookahead, eng->otherSymbol->id))
				termset_set(&ts,eng->otherSymbol->id);



			/*--------------------------------------------
				new lookahead termset 	
				get beta (what follows B in 'it') 
			 --------------------------------------------*/
			if(beta) {
				termset_merge_2(eng, &ts, &beta->firstSet);
				if(beta-> canDeriveEmpty) {
					termset_merge( &ts, &it->lookahead);
				}
			} else {
				termset_merge(&ts, &it->lookahead);
			}

			/*----------------------------------------------
			Not int pendlist, maybe in donelist 
			----------------------------------------------*/
			if ((nit = lookup_item(&donelist, ndp))) {
				if(termset_merge(&nit->lookahead,&ts)) {
					__get_item_out_from(&donelist, (struct nel_SYMBOL *)nit);
					do_gen_itemset_closure(eng, donelist,nit);
				}
				goto get_next_prod;
			}

			/*-------------------------------------
				otherwise make new item 
			 -------------------------------------*/
			nit = item_alloc(eng, ndp, &ts);
			nit->kernel = 0;
			nit->offset = it->dprod->dot + it->offset;

			/* recurive call the itemClosure */
			do_gen_itemset_closure(eng, donelist, nit);
			
			nel_dealloca(ts.bitmap); 
get_next_prod:
			prod = prod->next;
			if( !prod) {
				prod = eng->productions;
				if( flag == REL_EX )
					flag = REL_ON;
				else if(flag == REL_ON)
					flag = REL_OT;
				else if(flag == REL_OT)
					break;
			}

		}
	}

	return ;
}


void gen_itemset_closure(struct nel_eng *eng, struct itemset *is)
{
	struct nel_LIST *this, *next;
	struct nel_LIST *donelist = NULL;
	struct lritem *it;

	is->nonkernelItems = (struct lritem *)0;
	for(it=is->kernelItems;it;it=it->next) {
		__append_entry_to(&donelist,
			 __make_entry((struct nel_SYMBOL *)it));
		do_gen_itemset_closure(eng, donelist, it);
	}

	for(this = donelist;  this; this=next) {
		it = (struct lritem *)this->symbol;
		if (!it->kernel) {
			add_item_to_itemset_nonkernel(is, it);
		}
		next = this->next;
		//nel_free(this);	
	}

	return;

}

enum {
	ITEM_BOTH = 0 ,
	ITEM_NO_KERNEL,
	ITEM_KERNEL
};

/*----------------------------------------------------------------------
yield (by filling 'dest') a new itemset by moving the dot 
across the productions in 'source' that have 'symbol' to the 
right of the dot;
kernel 	== ITEM_BOTH : both kernel items and nokernel items
	== ITEM_NOKERNEL : nokernel item only	
	== ITEM_KERNEL : kernel item only
 ----------------------------------------------------------------------*/
struct itemset *gen_itemset_create(struct nel_eng *eng,struct itemset *src, struct lritem *src_it,nel_symbol *sym, int do_other, int flag)
{
	struct lritem *it, *nit;
	int dokernel;
	struct itemset *ret = NULL;
	
	if( do_other ) {
		ret = (termset_test(&src_it->lookahead, sym->id)) 
			? others_itemset_alloc(eng, src_it->dprod->dot + 1, 
				&src_it->lookahead) 
			:  NULL; 
		return ret;
	}	

	for(it=src->kernelItems,dokernel=1; it;
		it=(it->next)?
		   it->next
		   :( dokernel?
			  (dokernel=0,src->nonkernelItems)
			  :((struct lritem *)0)
			)
	   )
	{
		nel_symbol *s;

		if( flag == ITEM_NO_KERNEL && dokernel 
			|| flag == ITEM_KERNEL && !dokernel) {
			continue;
		}

		if( !is_dot_at_end(it->dprod)
			&& (s=sym_after_dot(it->dprod)) == sym
		  )
		{
			/*--------------------------------------------------
				move the dot; write dot-moved item into 
				'destIter'
			 --------------------------------------------------*/
			nit = item_alloc(eng, get_next_dprod(it->dprod), &it->lookahead);
			nit->kernel = 1;
			nit->offset = it->offset; 

			/* if it has timeout value, we need change nit 's
			timeout value to '-1' to stop the timer.
			but the nit may has another timeout value, so we set 
			it to ' -1 * (nit->dprod->timeout + 1)'. */
			if(it->dprod->timeout > 0) {
				nit->dprod->timeout = -1 * (nit->dprod->timeout + 1) ;
			}

			/*--------------------------------------------------
				add the new item to the itemset I'm building
			 --------------------------------------------------*/
			if (!ret) {
				ret = itemset_alloc(eng);
			}

			add_item_to_itemset_kernel(ret, nit);
		}
	}

	return ret;
}

int itemset_symbol_cnt(struct nel_eng *eng,  struct itemset *is, nel_symbol *sym, int flag )
{
	struct lritem *it, *nit;
	int dokernel;
	int ret = 0;

	for(it=is->kernelItems,dokernel=1; it;
		it=(it->next)?
		   it->next
		   :( dokernel?
			  (dokernel=0,is->nonkernelItems)
			  :((struct lritem *)0)
			)
	   )
	{
		nel_symbol *s;

		if( flag == ITEM_NO_KERNEL && dokernel 
			|| flag == ITEM_KERNEL && !dokernel) {
			continue;
		}

		if( !is_dot_at_end(it->dprod)
			&& (s=sym_after_dot(it->dprod)) == sym ) {
			ret++;
		}
	}

	return ret;
}

static int priority;
int do_gen_itemsets_create(struct nel_eng *eng, struct nel_LIST *donelist,  struct itemset *is)
{
	struct itemset *ois, *nis;
	struct lritem *it;
	nel_symbol *sym;
	int dokernel, do_other;
	int found_other;
	int max_dot;

	if(is) {
		do_other = 0;
		found_other = 0;
		max_dot = 0;
		sym = NULL;
		int k,n, flag;

		is->priority = priority ++;
		__append_entry_to(&donelist,__make_entry((struct nel_SYMBOL *)is));

start_itemset:
		for(it=is->kernelItems,dokernel=1; it;
			it=(it->next)?
			   it->next
			   :( dokernel?
				  (dokernel=0,is->nonkernelItems)
				  :((struct lritem *)0)
				)
			)
		{

			if(it->dprod->dotAtEnd ) {
				continue;
			}

			/* if found a 'other', no necessary to create a virtual
			itemset inhired from 'others: others,..., others'. */
			if(do_other == 0 ) {
				sym = sym_after_dot(it->dprod);
				found_other = (sym == eng->otherSymbol); 
			}
			else if(do_other > 0 ) {
				sym = eng->otherSymbol;
				if (it->dprod->dot <= max_dot ) {
					continue;
				}
				max_dot = it->dprod->dot;
			}
			
			/* 
			   (k > 0 && n = 0) || ( k ==0 && n > 0 ): flag = 0 
			   (k > 0 && n > 0) : if dokernel not set, flag = 1
						otherwise flag = 2 */
			n =  itemset_symbol_cnt(eng, is, sym, ITEM_NO_KERNEL );
			k =  itemset_symbol_cnt(eng, is, sym, ITEM_KERNEL);
			flag=(k * n) ? (dokernel ? ITEM_KERNEL:ITEM_NO_KERNEL)
								:ITEM_BOTH ;
			if ( ! ( nis = gen_itemset_create(eng, is, it, sym, do_other,flag ) ) ) {
				continue;
			}

			if((ois = lookup_itemset(&donelist, nis))) {
				itemset_dealloc(eng, nis);
				set_transition(eng, is, sym, ois);
				continue;
			}

			/* In the case of kernel item has multiple symbols,
			don't generate closure. */
			//if( flag == ITEM_BOTH || flag == ITEM_NO_KERNEL ){
				gen_itemset_closure(eng, nis);
			//}


			do_gen_itemsets_create(eng, donelist, nis);
			set_transition(eng, is, sym, nis);

		}

		/* when no 'other' found, we have to lookinto lookahead
		of each 'it' in 'is', add add virtual 'other' state 
		if necessary. */
		if( do_other == 0 && !found_other) {
			do_other = 1;
			goto start_itemset;
		}

	}

	return 0;

}

int gen_itemsets_create(struct nel_eng *eng)
{
	struct nel_LIST *donelist = NULL, *isl;
	struct itemset *is;
	struct nel_SYMBOL *prod;

	eng->gen->startState = is = itemset_alloc(eng);
	for(prod = eng->productions; prod; prod = prod->next) {
		if(prod->type->prod.lhs == eng->pseudo_symbol) {
			struct dprod *dp = get_dprod(eng, prod, 0);
			struct lritem *it = item_alloc (eng, dp, NULL);
			it->kernel = 1;
			it->offset = 0;
			add_item_to_itemset_kernel(is, it);
		}
	}

	gen_itemset_closure(eng, is);
	__append_entry_to(&donelist,
		__make_entry((struct nel_SYMBOL *)is));

	/* this is the core part of generator */
	do_gen_itemsets_create(eng, donelist, is);

	for(isl=donelist; isl; isl=isl->next) {
		struct itemset *is = (struct itemset *)isl->symbol;
		insert_itemset(eng, is);
	}


	sort_itemsets(eng->gen->itemSets);
	return 0;
}


//----------------------Parse Table Stuff -------------------------------
/* Set transition on 'sym' to be 'dest' */
void set_transition(struct nel_eng *eng, struct itemset *is, nel_symbol *sym, struct itemset *dest)
{
	struct nel_LIST **list, *entry;
	if ((struct nel_LIST **)0 == (list=(sym->class == nel_C_TERMINAL) 
			? &(is->termTransition[sym->id])
			: (	sym->class == nel_C_NONTERMINAL 
				? &(is->nontermTransition[sym->id])
				: NULL ))){	
		return;
	}

	if ( __lookup_item_from(list, (nel_symbol *)dest) == 0 ){
		if( dest && ( entry = __make_entry((nel_symbol *)dest))) {
			__append_entry_to(list, entry);
		}
	}
	
	return;

}



/* Query transition for an arbitrary symbol; 
Returns NULL if no transition is defined*/
struct nel_LIST *get_transition(struct nel_eng *eng, struct itemset *is, nel_symbol *sym)
{
	struct nel_LIST *ret = (sym != NULL) 
		? ((sym->class == nel_C_TERMINAL) 
			? is->termTransition[sym->id]
				: (  sym->class == nel_C_NONTERMINAL 
					? is->nontermTransition[sym->id]
			     : NULL 
			  ) 
		  )	
		: NULL;
	return ret;

}

/* Get the list of productions that are ready to reduce, given
that the next input symbol is 'lookahead' (i.e. in the follow 
of a production's LHS); parsing=true means we are actually parsing 
input, so certain tracing output is appropriate; 
'reductions' is a list of Productions */
struct nel_LIST *get_reductions(struct nel_eng *eng, struct itemset *is, nel_symbol *lookahead)
{
	struct nel_SYMBOL *prod;
	struct nel_LIST *ret =0, *entry;
	int kernel ;
	struct lritem *item;

	if(lookahead == eng->otherSymbol
		|| lookahead == eng->timeoutSymbol)
		return ret;

	for(kernel=1,item=is->kernelItems; item;
		item = (item->next)
			   ? (item->next)
			   : ((kernel == 1)
				  ?(kernel=0, is->nonkernelItems)
				  :(struct lritem *)0)
		)
	{

		if (!is_dot_at_end(item->dprod)){
			continue;
		}

		/* set default prod. */
		prod = item->dprod->prod;

		switch(eng->lr_level)
		{
		case LR0:
			/*-----------------------------------
				 don't check the lookahead
			 -----------------------------------*/
			break;
		case SLR1: 
			{
				/*------------------------------------
					the follow of its LHS must 
					include 'lookahead'
				 -------------------------------------*/
				union nel_TYPE *type = prod->type;
				struct nel_SYMBOL *lhs = type->prod.lhs ;
				if(!termset_test(&(lhs->_follow), lookahead->id)) {
					continue;
				}
			}
			break;

		case LR1:
		case LALR1: 
			{
				/*--------------------------------------
				 	the prod is either nodelay or 
					the item's lookahead must include 
					'lookahead'
				 ---------------------------------------*/
				if(/* !item->dprod->prod->nodelay && */
					!termset_test(&item->lookahead, lookahead->id)) {
					continue;
				}

				/* if lookahead is 'not', we use prod 'not' : 
				'not'...'not' rather then item->dprod->prod.  */
				if( lookahead == eng->notSymbol) {
					if(!(prod = lookup_prod(eng, eng->notSymbol, item->dprod->dot))) {
						continue;
					}
				}
			}
			break;

		default :
			gen_error(eng, "no LR variant specified !");
			continue;
		}

		// ok, this one's ready
		if( prod && ( entry = __make_entry(prod))) {
			__append_entry_to(&ret, entry);
		}
	}

	return ret;

}

int gen_resolve_conflicts(struct nel_eng *eng, struct itemset *state, nel_symbol *term, struct itemset *shiftDest, int numReductions, struct nel_LIST *reductions, int allowAmbig)
{
	struct nel_LIST *plist;
	//int dontWarns =0;
	struct nel_SYMBOL *prod;

	/*-------------------------------------
		How many actions are there?
	 -------------------------------------*/
	int actions = (shiftDest? 1 : 0) + numReductions;
	if (actions <= 1) {
		return 0;      // no conflict
	}

	if(shiftDest)
	{
		/*---------------------------------------------
			Resolve S/R conflicts via prec/assoc 
		 ---------------------------------------------*/
		for(plist = reductions; plist ; plist =plist->next) {

			prod = (struct nel_SYMBOL *)plist->symbol;
			if(prod->type->prod.precedence==0 || term->type->event.prec ==0 ) {
				/*---------------------------------------
					One of the two doesn't have a 
					precedence specification,so we 
					can do nothing, split  
				 ---------------------------------------*/
				continue;
			} else if(prod->type->prod.precedence > term->type->event.prec) {
				/*----------------------------------------
					Production's precedence is higher, 
					so we choose to reduce instead of 
					shift 
				 ----------------------------------------*/
				shiftDest = NULL;
				actions--;
				continue;
			} else if (prod->type->prod.precedence < term->type->event.prec) {
				/*----------------------------------------
					Symbol's precedence is higher, 
					so we shift 
				 ----------------------------------------*/
				actions--;
				continue;
			} else // equal
			{
				/*----------------------------------------
					Precedences are equal,so look 
					associativity
				 ----------------------------------------*/
				switch (term->type->event.assoc) {
				case LEFT:
					shiftDest = NULL;
					actions--;
					break;

				case RIGHT:
					actions--;
					break;

				case NOASSOC:
					/*--------------------------
					 Removed BOTH alternatives 
					 due to nonassociativity 
					---------------------------*/
					shiftDest = NULL;
					actions--;
					actions--;	//???
					break;

				default:
					/*---------------------------
					 	Bad assoc code 
					 ---------------------------*/
					eng->gen->errors ++;
					break;
				} //end of switch
			} // end of else
		}

		/*-------------------------------------------------------------
			There is still a potential for misbehavior.. e.g., 
			if there are two possible reductions (R1 and R2), 
			and one shift (S), then the user could have specified 
			prec/assoc to disambiguate, 
			e.g. 
				R1 < S,    
				S < R2,   
			so that R2 is the right choice; but if I consider 
			(S,R2) first, I'll simply drop S, leaving no way 
			to disambiguate R1 and R2 ..  for now I'll just note 
			the possibility...
		-------------------------------------------------------------*/

	}


	if (!allowAmbig && actions > 1)
	{
		/*--------------------------------------------------------
			Force only one action, using Bison's disambiguation:
			prefer shift to reduce. Prefer the reduction which 
			occurs first in the grammar file
		 ---------------------------------------------------------*/
		if(!shiftDest) {
			struct nel_SYMBOL *mini, *tmp;
			for(plist = reductions, prod = mini =(struct nel_SYMBOL *)plist->symbol;
				plist;
				plist=plist->next) {
				prod =  (struct nel_SYMBOL *)plist->symbol;
				if(prod->id  < mini->id) {
					/*------------------------------
						Swap mini and prod
					 ------------------------------*/
					tmp = mini;
					mini = prod;
					prod = tmp;
				}
				__get_item_out_from(&reductions, prod);
			}
		} else {
			/* __free_list(&reductions) */ ;
		}
		actions = 1;
	}

	return actions;

}




static int numAmbig = 0;
int ambig_entry_search(struct nel_eng *eng, ActionEntry entry, ActionEntry reduction)
{
	int start, total;
	int i;

	start = decodeAmbig(eng, entry);
	total = eng->ambigAction[start];

	for(i = 1; i <= total; i++) {
		if( eng->ambigAction[start + i ] == reduction ) {
			return 1;
		}
	}

	return 0; 
}

ActionEntry ambig_entry_alloc(struct nel_eng *eng)
{
	ActionEntry entry = 0;

	if(numAmbig > eng->numStates * (eng->numProductions + 1))  {
		gen_error(eng, "ambig_entry_alloc: ambig entry space overflowed!\n");
		return -1;
	}

	entry = encodeAmbig(eng, numAmbig);	
	eng->ambigAction[numAmbig++] = 0;	

	return entry;	

}

int ambig_entry_insert(struct nel_eng *eng, ActionEntry entry, ActionEntry reduction)
{
	int start, total, i;

	start = decodeAmbig(eng, entry);
	total = eng->ambigAction[start];

	if( start > numAmbig || (start + total + 1 ) != numAmbig ) {
		gen_error(eng, "ambig_entry_insert: entry was not last ambig!\n");
		return -1;
	}

	if( numAmbig > eng->numStates * (eng->numProductions + 1))  {
		gen_error(eng, "ambig_entry_insert: ambig entry space overflowed!\n");
		return -1;
	}

	eng->ambigAction[start]++;
	eng->ambigAction[numAmbig++] = reduction;

	return 0;

}


int shift_action_entry_alloc(struct nel_eng *eng,  struct itemset *state, nel_symbol *term, struct nel_LIST *shifts )
{
	ActionEntry entry;
	int ret = 0;
	struct nel_LIST *shift = shifts; 

	//if(state && term ) {
	//	printf("shift_action_entry_alloc: state->id(%d), term->id(%d)\n", state->id, term->id);
	//}
	while (shift) {
		struct itemset *shiftDest = (struct itemset *)shift->symbol;
		entry = actionEntry(eng, state->id, term->id); 
		if(entry == 0) {		
			//printf("shift_action_entry_alloc: entry==0, add(%d)\n", encodeShift(eng, shiftDest->id));
			actionEntry(eng, state->id, term->id) = encodeShift(eng, shiftDest->id);

			if(term->_parent ){
				int pid = term->_parent->id;
				if( actionEntry(eng, state->id, term->_parent->id) == 0 ){	// not set yet
					actionEntry(eng, state->id, term->_parent->id) = encodeShift(eng, eng->numStates);
				}
			}
				
			eng->stateTable[state->id].has_shift = 1 ;
			ret = 1;

		}else if(isAmbigAction(eng, entry)) {
			//printf("shift_action_entry_alloc: isAmbigAction\n");
			if ( !ambig_entry_search(eng, entry, encodeShift(eng,shiftDest->id))) {
				//printf("shift_action_entry_alloc: ambig_entry_search not found, insert %d\n", encodeShift(eng, shiftDest->id));
				ambig_entry_insert(eng, entry,encodeShift(eng,shiftDest->id));
				eng->stateTable[state->id].has_shift++;
				ret++;
			}

		}
		else if(entry != encodeShift(eng, shiftDest->id)) {
			//printf("shift_action_entry_alloc: entry(%d) != encodeShift(%d)\n", entry, encodeShift(eng, shiftDest->id));
			entry = ambig_entry_alloc(eng);

			
			//printf("shift_action_entry_alloc: insert %d\n", actionEntry(eng, state->id, term->id));
			ambig_entry_insert(eng, entry,  actionEntry(eng, state->id, term->id));
			//printf("shift_action_entry_alloc: insert %d\n", encodeShift(eng, shiftDest->id));
			ambig_entry_insert(eng, entry, encodeShift(eng, shiftDest->id));
			actionEntry(eng, state->id, term->id) = entry; 
			eng->stateTable[state->id].has_shift++;
			ret++;

		}
		else {
			/*entry == encodeShift(eng, shiftDest->id), need do nothing */
		}

		//if( ret > 0 && term != term->_parent ) {
		//	struct itemset tmp; tmp.id=eng->numStates;
		//	printf("------>call shift_action_entry_alloc");
		//	shift_action_entry_alloc(eng, state,term->_parent,&tmp);
		//}

		shift = shift->next;

	}

	return ret;

}


/* add reductions to state's reduce table rather then action table.  */
int reduce_action_entry_alloc(struct nel_eng *eng,  struct itemset *state, struct nel_LIST *reductions)
{
	ActionEntry entry;
	struct nel_SYMBOL *prod ;
	struct nel_LIST *reduction = reductions;
	int pid; 
	int ret = 0;

	while( reduction ){

		prod = (struct nel_SYMBOL *)reduction->symbol;
		pid = prod->id;
		entry = eng->stateTable[state->id].reduce;

		if(entry == 0) {
			eng->stateTable[state->id].reduce = encodeReduce(eng, pid);	
			eng->stateTable[state->id].has_reduce = 1;
			ret = 1;
		}

		else if (isAmbigAction(eng, entry)) {
			if ( !ambig_entry_search(eng, entry, encodeReduce(eng,pid))) {
				ambig_entry_insert(eng, entry,encodeReduce(eng,pid));
				eng->stateTable[state->id].has_reduce++;
				ret++;
			}
		} 

		else if(entry != encodeReduce(eng, pid)) {
			entry = ambig_entry_alloc(eng);
			ambig_entry_insert(eng, entry, eng->stateTable[state->id].reduce);
			ambig_entry_insert(eng, entry, encodeReduce(eng, pid));
			eng->stateTable[state->id].reduce = entry; 
			eng->stateTable[state->id].has_reduce++;
			ret++;
		}
		else {
			/*entry == encodeReduce(pid), need do nothing */

		}

		reduction = reduction->next;

	}

	return ret;

}

int gen_compute_action_tbl(struct nel_eng *eng)
{
	struct itemset *state;
	nel_symbol *term;
	struct nel_LIST *reductions;
	struct nel_LIST *shifts;

	for(state=eng->gen->itemSets; state; state=state->next) {
		for( term = eng->terminals; term; term = term->next) {
			//int actions =0, numReductions;
			shifts = get_transition(eng, state, term);
			reductions = get_reductions(eng, state, term );

			if(!shifts && !reductions) {
				continue;
			}

			/* 
			Since SS/SR/RR conflicts are allowed, resolve are not
			needed.  
			numReductions = __get_count_of(&reductions);
			actions= gen_resolve_conflicts(eng, state, term, 
					shiftDest,numReductions, reductions,1);
			*/

			//printf("--->call shift_action_entry_alloc\n");
			shift_action_entry_alloc(eng, state, term, shifts );
			reduce_action_entry_alloc(eng, state, reductions);

		}

	}

	return 0;

}



int goto_entry_alloc(struct nel_eng *eng,  struct itemset *state, nel_symbol *nt, struct nel_LIST *gotos )
{
	ActionEntry entry;
	int ret = 0;
	struct nel_LIST *_goto = gotos ;

	//gotoEntry(eng, state->id, nt->id) = encodeGotoError(eng);
	//if(state->id == 19) {
	//printf("goto_entry_alloc: state->id (%d) , nt->id(%d)\n", state->id, nt->id);
	//}

	while (_goto) {
		struct itemset *gotoDest = (struct itemset *)_goto->symbol;
		entry= gotoEntry(eng, state->id, nt->id); 
		if(entry == 0) {		
			//if(state->id == 19) {
			//printf("goto_entry_alloc: entry == 0, encodeGoto(eng, gotoDest->id) =%d \n", encodeGoto(eng, gotoDest->id));
			//}
			gotoEntry(eng, state->id, nt->id) = encodeGoto(eng, gotoDest->id);		
			if(nt->_parent ){
				if( gotoEntry(eng, state->id, nt->_parent->id) == 0 ){	// not set yet
					gotoEntry(eng, state->id, nt->_parent->id) = encodeGoto(eng, eng->numStates);
				}
			}
			eng->stateTable[state->id].has_shift = 1 ;
			ret = 1;

		}else if(isAmbigAction(eng, entry)) {
			//if(state->id == 19) {
			//printf("goto_entry_alloc: isAmbigAction\n");
			//}
			if ( !ambig_entry_search(eng, entry, encodeGoto(eng, gotoDest->id))) {
				//if(state->id == 19) {
				//printf("goto_entry_alloc: ambig_entry_search found\n");
				//}
				ambig_entry_insert(eng, entry,encodeGoto(eng, gotoDest->id));
				eng->stateTable[state->id].has_shift++;
				ret++;
			}

		}
		else if(entry != encodeGoto(eng, gotoDest->id)) {
			//if(state->id == 19) {
			//printf("goto_entry_alloc: entry(%d) != encodeGoto(%d)\n", entry, encodeGoto(eng, gotoDest->id));
			//}
			entry = ambig_entry_alloc(eng);
			//printf("goto_entry_alloc: insert %d\n", gotoEntry(eng, state->id, nt->id));
			ambig_entry_insert(eng, entry,  gotoEntry(eng, state->id, nt->id));
			//printf("goto_entry_alloc: insert %d\n", encodeGoto(eng, gotoDest->id));
			ambig_entry_insert(eng, entry, encodeGoto(eng, gotoDest->id));
			//printf("goto_entry_alloc: entry = %d\n", entry);
			gotoEntry(eng, state->id, nt->id) = entry; 
			eng->stateTable[state->id].has_shift++;
			ret++;

		}
		else {
			/*entry == encodeShift(eng, shiftDest->id), need do nothing */
		}

		//if( ret > 0 && nt != nt->_parent ) {
		//	struct itemset tmp; tmp.id=eng->numStates;
		//	goto_entry_alloc(eng, state, nt->_parent,&tmp);
		//}

		_goto = _goto->next;

	}

	return ret;

}

int gen_compute_goto_tbl(struct nel_eng *eng)
{
	nel_symbol *nt;
	struct itemset *state;

	for(state=eng->gen->itemSets; state; state=state->next) {
		for(nt=eng->nonterminals;nt; nt = nt->next ) {
			//GotoEntry cellGoto;
			/*--------------------------------------------
			 	Where do we go when we reduce to this 
				nonterminal?
			 --------------------------------------------*/
			struct nel_LIST *gotos = get_transition(eng, state,nt);


#if 0
			if (gotoDest) {
				cellGoto = encodeGoto(eng, gotoDest->id);
			} else {
				/*--------------------------------------
					This should never be accessed at 
					parse time..
				 ---------------------------------------*/
				cellGoto = encodeGotoError(eng);
			}

			/*-------------------------------------
				Fill in entry
			 -------------------------------------*/
			eng->gotoTable[state->id * eng->numNonterminals + nt->id] = cellGoto;
#else
			//printf("--->goto_entry_alloc\n");
			goto_entry_alloc(eng, state, nt, gotos );

#endif

		}
	}

	return 0;

}

int gen_compute_sym_info_tbl(struct nel_eng *eng)
{
	nel_symbol *sym;
	for(sym = eng->terminals; sym; sym = sym->next) {
		eng->symbolInfo[encodeSymbolId(eng, sym->id, nel_C_TERMINAL)] = sym;
	}

	for(sym = eng->nonterminals; sym; sym = sym->next) {
		eng->symbolInfo[encodeSymbolId(eng, sym->id, nel_C_NONTERMINAL)]=sym;
	}

	return 0;
}


int gen_compute_prod_info_tbl(struct nel_eng *eng)
{
	struct nel_SYMBOL *prod;

	for (prod=eng->productions; prod; prod=prod->next) {
		eng->prodInfo[prod->id].rhsLen = prod->type->prod.rhs_num;
		eng->prodInfo[prod->id].lhsIndex = encodeSymbolId(eng, prod->type->prod.lhs->id, prod->type->prod.lhs->class);
		eng->prodInfo[prod->id].rel = prod->type->prod.rel;
		eng->prodInfo[prod->id].action = prod->type->prod.action;
	}
	return 0;
}

int gen_compute_state_info_tbl(struct nel_eng *eng)
{
	int dot_pos;
	struct itemset *state;
	struct lritem *it;

	for(state= eng->gen->itemSets; state; state= state->next) {
		eng->stateTable[state->id].priority = state->priority;	
	
		for( dot_pos=0,it= state->kernelItems; it; it=it->next ) {
			if (dot_pos == 0 ){
				dot_pos = it->dprod->dot;
			}
			else if ( dot_pos != it->dprod->dot ) {
				dot_pos = -1; break;
			}
		}

		/* the length of stack for reduce must deteminated. */
		eng->stateTable[state->id].dot_pos = dot_pos;

	}

	return 0;
}

int gen_tables_init(struct nel_eng *eng)
{
	//int i;
	nel_calloc(eng->symbolInfo, eng->numTerminals + eng->numNonterminals+1, nel_symbol*);
	if(!eng->symbolInfo) {
		gen_error(eng,"nel_calloc failed in initSymbolInfoTable\n");
		return -1;
	}

	nel_zero((eng->numTerminals + eng->numNonterminals+1) * sizeof(nel_symbol *), eng->symbolInfo);
	eng->symbolInfo = eng->symbolInfo + eng->numNonterminals;

	nel_calloc(eng->prodInfo, eng->numProductions, ProdInfo);
	if(!eng->prodInfo) {
		gen_error(eng,"nel_calloc failed in initGotoTable\n");
		return -1;
	}

	nel_zero(eng->numProductions * sizeof(ProdInfo), eng->prodInfo);

	nel_calloc(eng->actionTable, eng->numStates * eng->numTerminals, ActionEntry);
	if(!eng->actionTable) {
		gen_error(eng, "nel_calloc failed in initActionTable:actionTable\n");
		return -1;
	}

	nel_zero(eng->numStates * eng->numTerminals * sizeof(ActionEntry), eng->actionTable);


#if 0
	/* default action tbl */
	if(!(eng->default_action_tbl = calloc(eng->numStates, sizeof(ActionEntry)))) {
		gen_error(eng, "calloc failed in initActionTable:default_action_tbl\n");
		return -1;
	}
#endif

	nel_calloc(eng->gotoTable, eng->numStates * eng->numNonterminals, GotoEntry);
	if(!eng->gotoTable) {
		gen_error(eng, "nel_calloc failed in initGotoTable\n");
		return -1;
	}

	nel_zero(eng->numStates * eng->numNonterminals * sizeof(GotoEntry), eng->gotoTable);

	nel_calloc(eng->ambigAction, eng->numStates * (eng->numProductions + 1), ActionEntry);
	if(!eng->ambigAction) {
		gen_error(eng, "nel_calloc failed in ambigAction\n");
		return -1;
	}
	nel_zero(eng->numStates * (eng->numProductions + 1) * sizeof(ActionEntry), eng->ambigAction);

	nel_calloc(eng->stateTable, eng->numStates, struct stateInfo);
	if(!eng->stateTable) {
		gen_error(eng, "nel_calloc failed in initActionTable:stateTable\n");
		return -1;
	}
	nel_zero(eng->numStates * sizeof(struct stateInfo), eng->stateTable);


	nel_malloc(eng->classTable, eng->numStates * (eng->numTerminals + 1 + eng->numNonterminals), struct classInfo);
	if(!eng->classTable) {
		gen_error(eng, "neLl_malloc failed in classTable\n");
		return -1;
	}
	nel_zero(eng->numStates * (eng->numTerminals + 1 + eng->numNonterminals) * sizeof(struct classInfo), eng->classTable);

#if 0
	if(!(eng->state_timeout_tbl=(int *)calloc(eng->numStates, sizeof(int)))) {
		gen_error(eng, "calloc failed in initActionTable:state_timeout_tbl\n");
		return -1;
	}
#endif

	return 0;
}

int gen_init(struct nel_eng *eng)
{
	if(!eng)
		return -1;

	nel_malloc(eng->gen, 1, struct eng_gen);
	nel_zero(sizeof(struct eng_gen), eng->gen);
	eng->gen->cycle = 0;
	eng->gen->errors = 0;
	eng->gen->derivable = NULL ;
	eng->gen->nextItemSetId = 0;
	eng->lr_init_flag = 1;

	return 0;

}

int rhs_num_max = 0;
int nel_generator(struct nel_eng *eng)
{
	int i;
	priority = 0;
	numAmbig = 0;
	/*NOTE,NOTE,NOTE, the function is bad for lossing error handle */ 
	if(eng->startSymbol == NULL || eng->startSymbol->type == NULL) {
		gen_warning(eng, "undeclaration of link\n"); 
	}
	
	if( pseudo_prod_alloc(eng) == NULL ) {
		gen_error (eng, "pseudo_prod_alloc error\n"); 
		return -1;		
	}


	for(i = 1; i<= rhs_num_max ; i++) {  
		if ( others_prod_alloc(eng, i) == NULL ) {
			gen_error(eng, "others prod alloc error(%d)", i);
			return -1;
		}	
	}
	
	if(remove_empty_prods(eng) < 0 ) {
		gen_error(eng, "errors in removing empty productions\n");	
		return -1;
	}

	emit_terminals(eng);
	emit_nonterminals(eng);
	emit_prods(eng);

	reach_nonterm_num = 0;
	reach_term_num = 0 ;
	gen_compute_reachable(eng,eng->pseudo_symbol); 

#if 0
	if( reach_nonterm_num < 3 && reach_term_num < 2 ) { 
		gen_error(eng, "undeclaration of target production\n");
		return -1;
	}

	if( reach_term_num < 2 /* end only */ ) {
		gen_error(eng, "no atom was used\n");	
		return -1;
	}

#else

	if( (reach_nonterm_num < 3 && reach_term_num < 2) 
		|| reach_term_num < 2 /* end only */ ) { 
		gen_warning(eng, "itemset generation ignored!\n");
		goto output;
	}
#endif

	/* initialize phase */
	if(gen_derivable_init(eng) < 0
		|| gen_compute_derivable(eng) < 0
		|| gen_first_follow_init(eng) < 0
		|| dprods_alloc(eng) < 0 ) {
		return -1;
	}


	/* do the real work here */
	if(gen_compute_symbol_first(eng) < 0
		|| gen_compute_dprod_first(eng) < 0
		|| gen_itemsets_create(eng) < 0 ) {
		return -1;
	}

	emit_itemsets(eng);

output:
	if( reduction_action_alloc(eng) < 0 
		|| evt_free_func_alloc(eng) < 0 ){
		return -1;
	}

	if ( gen_tables_init(eng) < 0
		|| gen_compute_action_tbl(eng) < 0
		|| gen_compute_goto_tbl(eng) < 0
		|| gen_compute_prod_info_tbl(eng) < 0
		|| gen_compute_sym_info_tbl(eng) < 0
		|| gen_compute_state_info_tbl(eng) < 0 ) {
		return -1;
	}

	class_alloc(eng);
	return 0;

}

void gen_dealloc(struct nel_eng *eng)
{
	//clean up all terminals
	nel_symbol *term;
	term = eng->terminals;
	while(term) {
		nel_symbol *tmp;
		tmp = term->next;
		event_symbol_dealloc(eng, term);
		term = tmp;
	}

	//clean up all nonterminals
	nel_symbol *nonterm = eng->nonterminals;
	while(nonterm) {
		nel_symbol *tmp = nonterm->next;
		if(nonterm->_first.bitmap)
			nel_dealloca(nonterm->_first.bitmap);
		event_symbol_dealloc(eng, nonterm);
		nonterm = tmp;
	}
	
	//clean up all dotted prods in eng->gen
	nel_symbol *prod = eng->productions;
	if(eng->gen->dottedProds) {
		for(prod=eng->productions; prod; prod=prod->next) {
			int id=prod->id;
			int pos;
			for(pos=0; pos <= prod->type->prod.rhs_num; pos++) {
				struct dprod *dp=&(eng->gen->dottedProds[id][pos]);
				if(dp->firstSet.bitmap)
					nel_dealloca(dp->firstSet.bitmap); 
			}
			nel_dealloca(eng->gen->dottedProds[id]); 
		}
		nel_dealloca(eng->gen->dottedProds); 
	}

	
	//clean up all productions
	prod=eng->productions;
	while(prod) {
		nel_symbol *temp_prod = prod->next;

		//clean up prod's rhs, which are allocated in nel.y
		struct nel_RHS *rhs = prod->type->prod.rhs;
		while(rhs) {
			struct nel_RHS *temp_rhs = rhs->next;
			nel_dealloca(rhs); 
			rhs = temp_rhs;
		}
		prod_symbol_dealloc(eng, prod);
		prod = temp_prod;
	}

	
	//clean up adjacent table
	nel_dealloca(eng->gen->derivable); 

	//for each item in every itemset
	//1, free item's lookahead
	//2, free item
	//3, free itemset's transiton array
	//4, free itemset
	struct itemset *is = eng->gen->itemSets;
	while(is) {
		struct itemset *temp_is = is->next;
		struct lritem *it = is->kernelItems;
		while(it) {
			struct lritem *temp_it = it->next;
			if(it->lookahead.bitmap)
				nel_dealloca(it->lookahead.bitmap); 
			nel_dealloca(it); 
			it = temp_it;
		}
		it = is->nonkernelItems;
		while(it) {
			struct lritem *temp_it = it->next;
			if(it->lookahead.bitmap)
				nel_dealloca(it->lookahead.bitmap); 
			nel_dealloca(it); 
			it = temp_it;
		}
		nel_dealloca(is->nontermTransition); 
		nel_dealloca(is->termTransition); 
		nel_dealloca(is); 
		is = temp_is;
	}

	
	//clean up info in eng, which are allocated in gen.c, so I think
	//the deallocate work should be done here

	
	if(eng->classTable)
		nel_dealloca(eng->classTable); 
	if(eng->symbolInfo) {
		eng->symbolInfo -= eng->numNonterminals;
		nel_dealloca(eng->symbolInfo); 
	}
	if(eng->prodInfo)
		nel_dealloca(eng->prodInfo); 
	if(eng->actionTable)
		nel_dealloca(eng->actionTable); 
	if(eng->gotoTable)
		nel_dealloca(eng->gotoTable); 
	if(eng->ambigAction)
		nel_dealloca(eng->ambigAction); 
	if(eng->stateTable)
		nel_dealloca(eng->stateTable); 
	
	rhs_num_max = 0;
	//clean up eng->gen itself
	if(eng->gen)
		nel_dealloca(eng->gen);
}
