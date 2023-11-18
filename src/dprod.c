
#include <stdlib.h>

#include "engine.h"
#include "errors.h"
#include "prod.h"
#include "dprod.h"
#include "gen.h"
#include "sym.h"
#include "mem.h"

/*----------------------------------------------------------
	Dot must not be at the start (left edge)
 ----------------------------------------------------------*/
nel_symbol *sym_before_dot(struct dprod *dprod) 
{
	int i;
	struct nel_RHS *sl;

	for(i=1, sl=dprod->prod->type->prod.rhs; 
			i<dprod->dot; i++, sl= sl->next); 

	return (i==dprod->dot)? (sl->symbol) : (nel_symbol *)0 ;
}

nel_symbol *sym_after_dot(struct dprod *dprod) 
{
	return dprod->afterDot;
}

/*---------------------------------------------------------
	Performance optimization: NULL if dot at end, 
	or else pointer to the symbol right after the dot
	Symbol SymbolafterDot();
 ---------------------------------------------------------*/
int is_dot_at_start(struct dprod *dprod) 
{ 
	return (dprod->dot==0); 
}

int is_dot_at_end(struct dprod *dprod)
{ 
	return (dprod->afterDot==NULL); 
}


struct dprod *get_dprod(struct nel_eng *eng, struct nel_SYMBOL *prod, int posn)
{
	return &( eng->gen->dottedProds[prod->id][posn]);
}

/*----------------------------------------------------------
	Given a dprod, yield the one obtained by 
	moving the dot one place to the right
 ----------------------------------------------------------*/
struct dprod *get_next_dprod(struct dprod *dp) 
{
  	return 	(dp + 1);
}


int dprod_alloc(struct nel_eng *eng, struct nel_SYMBOL *prod)
{
	int posn, rhsLen = prod->type->prod.rhs_num;
	struct nel_RHS *sl;
	
	//modified by zhangbin, 2006-7-17, malloc=>nel_malloc
#if 1
	nel_malloc(eng->gen->dottedProds[prod->id], rhsLen+1, struct dprod);
#else
	eng->gen->dottedProds[prod->id]= (struct dprod *)malloc(sizeof(struct dprod)*(rhsLen+1));
#endif
	//end modified, 2006-7-17

	for (posn=0, sl=prod->type->prod.rhs; posn<=rhsLen;posn++,sl= sl ? sl->next:(struct nel_RHS *)0) {
		struct dprod *dprod= &eng->gen->dottedProds[prod->id][posn]; 
		dprod->prod = prod;
		dprod->dot = posn;
		dprod->afterDot = (posn != rhsLen)?sl->symbol:(nel_symbol *)0;
		dprod->dotAtEnd = (posn == rhsLen);
		dprod->canDeriveEmpty = 0;
		//added by zhangbin, 2006-7-24
		dprod->firstSet.bitmap = NULL;
		dprod->firstSet.bitmapLen = dprod->firstSet.numbers = 0;
		//end
	}

	return 0;
}


int dprods_alloc(struct nel_eng *eng)
{
	struct nel_SYMBOL *prod;
	nel_malloc(eng->gen->dottedProds, eng->numProductions, struct dprod *);
	if(!eng->gen->dottedProds){
		gen_error(eng, "nel_dprods_alloc: nel_malloc error !\n");	
		return -1;
	}

	for(prod=eng->productions; prod; prod=prod->next) {
		dprod_alloc(eng, prod); 
	}
	
	return 0;
}

void emit_dprod(struct nel_eng *eng,FILE * fp, struct dprod* dprod, int offset)
{
	int i;	
	struct nel_RHS *rhs;

	fprintf(fp, "\tItem_%d.(offset=%d) %s : ", dprod->prod->id, offset,  dprod->prod->type->prod.lhs->name);

	for(i = 0, rhs = dprod->prod->type->prod.rhs; rhs; i++, rhs=rhs->next){
		if(dprod->dot == i ) fprintf(fp, " * ");
		emit_symbol(fp, rhs->symbol);
	}

	if(dprod->dot == i) {
		fprintf(fp, " * ");

	}

}
