
#include <stdio.h>
#include <stdlib.h>

#include "engine.h"
#include "type.h"
#include "itemset.h"
#include "gen.h"
#include "errors.h"
#include "sym.h"
#include "mem.h"
#include "prod.h"

extern struct nel_LIST *get_transition(struct nel_eng *eng, struct itemset *is, nel_symbol *sym);

#if 0
/* 'dest' has already been established to have the same kernel 
items as 'this' -- so merge all the kernel lookahead items of 
'this' into 'dest'; return 'true' if any changes were made to 
'dest' */
int itemset_merge_lookaheads(struct itemset *src, struct itemset *dest) 
{        
	int changes = 0;
	struct lritem *si, *di;

	for(si=src->kernelItems, di=dest->kernelItems; si && di; 
		si=si->next, di=di->next) {
		if(item_merge(si, di))  changes =  1;
	}

	return changes;
}

int is_reduce_itemset(struct itemset *is)
{
	struct lritem *it;
	for(it=is->kernelItems; it;it=it->next) {
		if( !is_dot_at_end(it->dprod) ) {
			return 0;
		}
	}
	return 1;
}

/* After adding all kernel items, call this */
void sort_itemset_kernels(struct itemset *is)
{
	struct lritem *i,*j, *m, *pi, *pj, *pm; 
	int s;

	for(i=is->kernelItems;i;pi=i,i=i->next) {
		for (m=j=i; j; pj=j, j=j->next) {
			if( j->dprod->prod->id < m->dprod->prod->id 
				|| ( j->dprod->prod->id == m->dprod->prod->id 
				     && j->dprod->dot < m->dprod->dot)  
			) {
				pm = pj; m = j;
			}
		}

		if (i != m) {
			pm->next = m->next;	
			pi->next = m;
			m->next = i;
		}
	}	

}
#endif

/*------------------------------------------------------
	Since we sometimes consider a state more than 
	once, the states end up out of order; put them 
	back in order
 ------------------------------------------------------*/
void sort_itemsets(struct itemset *is)
{
	struct itemset *i,*j, *m;
	struct itemset *pi, *pj, *pm; 

	for(i=is;i;pi=i,i=i->next) {
		for (m=j=i; j; pj=j, j=j->next) {
			if (j->id < m->id) {
				pm = pj; 
				m = j;
			}
		}

		if (i != m){
			pm->next = m->next;
			pi->next = m;
			m->next = i;
		}
	}	

}


/* Add a kernel item; used while constructing the state */
int add_item_to_itemset_kernel(struct itemset *is, struct lritem *item)
{
	struct lritem *i, *prev;

	item->kernel=1;
	item->next = (struct lritem *)0;
	if(is->kernelItems) {
		for(i=is->kernelItems; i; prev=i, i= i->next);
		if (prev) {
			prev->next = item;
		}
	}
	else {
		is->kernelItems = item;
	}

	if(abs(item->dprod->timeout) > abs(is->timeout)) {
		is->timeout = item->dprod->timeout;
	}

	return 0;
}



/* Add a nonkernel item; used while computing closure; this 
item must not already be in the item set */
void add_item_to_itemset_nonkernel(struct itemset *is, struct lritem *item)
{
	struct lritem *i, *prev;
	item->next = (struct lritem *)0;
	if(is->nonkernelItems) {
		for(i=is->nonkernelItems; i; prev=i, i= i->next);
		if (prev) {
			prev->next = item;
		}
	}
	else {
		is->nonkernelItems = item;
	}

}


struct itemset *others_itemset_alloc(struct nel_eng *eng, int dot, TerminalSet *ts)
{
	struct itemset *ret = NULL;
	struct lritem *nit;
	struct nel_SYMBOL *prod;

	/* 'others' :  others, ..., others { return NULL; }; */
	if( !(prod = lookup_prod(eng,eng->otherSymbol, dot)) ) {
		gen_error(eng, "can't find others prod(%d)", dot);
		return NULL;
	}


	if((nit=item_alloc(eng, get_dprod(eng, prod, dot), ts))){
		//nit->offset = dot;
		nit->kernel = 1;	
		if ((ret = itemset_alloc(eng))){
			add_item_to_itemset_kernel(ret, nit); 
			//others_itemset_append(eng, ret);
		}
	}

	return ret;
}

#if 0
struct itemset *timeout_itemset_alloc(struct nel_eng *eng, int dot, TerminalSet *ts)
{
	struct itemset *ret = NULL;
	struct lritem *it, *nit;
	struct nel_SYMBOL *prod;

	/* 'timeout' :  timeout, ..., timout  { return NULL; }; */
	if( !(prod = lookup_prod(eng, eng->timeoutSymbol, dot)) ) {
		gen_error(eng, "can't find 'timeout' prod(%d)", dot);
		return NULL;
	}

	if((nit=item_alloc(eng, get_dprod(eng, prod, dot), ts))){
		//nit->offset = dot;
		nit->kernel = 1;	
		if ((ret = itemset_alloc(eng))){
			add_item_to_itemset_kernel(ret, nit); 
			//timeout_itemset_append(eng, ret);
		}
	}

	return ret;
}

struct itemset *not_itemset_alloc(struct nel_eng *eng, int dot, TerminalSet *ts)
{
	struct itemset *ret = NULL;
	struct lritem *it, *nit;
	struct nel_SYMBOL *prod;

	/* 'not' :  not, ..., not { return NULL; }; */
	if( !(prod = lookup_prod(eng, eng->notSymbol, dot)) ) {
		gen_error(eng, "can't find 'not' prod(%d)", dot);
		return NULL;
	}

	if((nit=item_alloc(eng, get_dprod(eng, prod, dot), ts))){
		//nit->offset = dot;
		nit->kernel = 1;	
		if ((ret = itemset_alloc(eng))){
			add_item_to_itemset_kernel(ret, nit); 
		}
	}

	return ret;
}
#endif

/*------------------------------------------------------
This func seems to lose something.
left = right <==> left<=right && left>=right
Everything is of the same order, so this func 
can work well.
 ------------------------------------------------------*/
int itemset_equal(struct itemset *left, struct itemset *right)
{
	struct lritem *i, *j;

	for(i=left->kernelItems, j = right->kernelItems; i && j; 
					i=i->next, j = j->next) {
		if (!item_equal(i, j)) {
			return 0;
		}
	}

	return (!i && !j);
}

struct itemset *lookup_itemset(struct nel_LIST **list, struct itemset *is)
{
	struct nel_LIST *isl;
	for(isl = *list; isl; isl = isl->next) {
		if (itemset_equal((struct itemset *)isl->symbol, is)) {
			return (struct itemset *)isl->symbol;
		}
	}
	return (struct itemset *)0;
}

void insert_itemset(struct nel_eng *eng, struct itemset *is)
{
	struct itemset *i, *prev;

	if(eng->gen->itemSets) {
		for(i= eng->gen->itemSets; i; prev = i, i=i->next);
		if(prev)
		{
			prev->next = is;	
		}
	} 
	else {
		 eng->gen->itemSets = is;
	}

	is->next = NULL;
}

struct itemset *itemset_alloc(struct nel_eng *eng)
{
	struct itemset *is = NULL; 
	int i;

	nel_malloc(is, 1, struct itemset);
	if(is == NULL ) {
		gen_error(eng, "itemset_alloc: malloc error!\n");
		return NULL;
	}

	is->id = eng->numStates++;
	is->nonkernelItems = NULL;
	is->kernelItems = NULL;
	//is->timeout = 0;

	nel_malloc(is->termTransition,eng->numTerminals, struct nel_LIST *);
	for(i=0; i< eng->numTerminals; i++) {
		is->termTransition[i] = (struct nel_LIST *)NULL;
	}

	nel_malloc(is->nontermTransition,eng->numNonterminals, struct nel_LIST *);
	for(i=0; i< eng->numNonterminals; i++) {
		is->nontermTransition[i] = (struct nel_LIST *)NULL;
	}

	is->next = NULL;
	is->next_transition = NULL;
	return is;
}

void itemset_dealloc(struct nel_eng *eng, struct itemset *is)
{
	if(is) {
		if(is->termTransition) {
			nel_free(is->termTransition);
		}

		if(is->nontermTransition){
			nel_free(is->nontermTransition);
		}

		eng->numStates--;
		nel_free(is);	
	}

}

void emit_itemset(struct nel_eng *eng,FILE *fp, struct itemset *is)
{
	nel_symbol *symbol;	
	int flag = 0;
	struct nel_LIST *dest;
	struct lritem *lrItem;

	fprintf(fp, "itemset %d\n", is->id);
	for(lrItem = is->kernelItems; lrItem; lrItem = lrItem->next){
		emit_item(eng, fp, lrItem);
	}		
	for(lrItem = is->nonkernelItems; lrItem; lrItem = lrItem->next){
		emit_item(eng, fp, lrItem);
	}	

	fprintf(fp, "\tPRIORIT:%d\n", is->priority);
	//fprintf(fp, "\tTIMEOUT:%d\n", is->timeout);
	fprintf(fp, "\tTRANSIT:\n");
	symbol = eng->terminals;
	while(symbol){
		dest = get_transition(eng, is, symbol);
		while(dest) {
			struct itemset *it;
			fprintf(fp, "\t\t--->");
			emit_symbol(fp, symbol);
			it = (struct itemset *)dest->symbol;
			fprintf(fp, "--->\t itemset %d", it->id);
			fprintf(fp, "\n");
			dest = dest->next;
		}
	
		if( symbol->next)
			symbol = symbol->next;
		else if(flag == 0){
			symbol = eng->nonterminals;
			flag ++;
		}
		else 
			symbol = NULL;
	}

	fprintf(fp, "\n");	
}

void  emit_itemsets(struct nel_eng *eng /*,FILE *fp */ )
{
	FILE *fp;
	if(eng->itemset_debug_level ) {
		if((fp=fopen("itemsets", "w"))) {
			struct itemset *is;
			fprintf(fp, "Item Sets:\n"); 	
			for(is = eng->gen->itemSets; is ; is = is->next) {
				emit_itemset(eng, fp, is);
			}
			fclose(fp);
		}
	}
}
