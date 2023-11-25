

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "engine.h"
#include "free_func.h"
#include "type.h"
#include "sym.h"
#include "class.h"
#include "gen.h"
#include "intp.h"
#include "itemset.h"
#include "type.h"
#include "stmt.h"
#include "opt.h"
#include "prod.h"

nel_symbol *evt_free_func_symbol_alloc(struct nel_eng *eng, FILE *fp, nel_symbol *func) 
{
	if(eng->compile_level > 0){
		/* output this func to temporary file */
		//emit_symbol(fp, func);
	}
	return func;
}


int evt_free_func_alloc(struct nel_eng *eng)
{
	nel_symbol *sym1, *sym2;
	FILE *fp = NULL;
	int flag1, flag2;

	if(eng->compile_level > 0){
		sym1 = eng->terminals;
		flag1 = 0;

		while(sym1){
			int outputted = 0;
			for(flag2 = 0, sym2= eng->terminals; sym2!= sym1; 
			sym2=sym2->next, sym2=(sym2==NULL)?(flag2==0? eng->nonterminals:NULL):sym2){

				if( sym1->type->event.free_hander && sym2->type->event.free_hander 
				&& !strcmp(sym2->type->event.free_hander->name,  sym1->type->event.free_hander->name)){ 
					outputted = 1;
					break;
				}
			}
			
			if(!outputted) {
				evt_free_func_symbol_alloc(eng, fp, sym1->type->event.free_hander);
			}
			
			
			sym1 = sym1->next;
			if ( !sym1 && flag1 == 0 ) {
				sym1 = eng->nonterminals;
				flag1 ++;
			}
				
		}
	}

	return 0;
}

