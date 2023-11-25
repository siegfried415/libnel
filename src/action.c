

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "engine.h"
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
#include "action.h"

//#include "comp.h"

int reduction_action_compile(struct nel_eng *eng)
{
	struct nel_SYMBOL *prod;
	unsigned long val ;
	nel_symbol *func;

	for(prod = eng->productions; prod; prod=prod->next){
		if((func = prod->type->prod.action) != NULL) {

			// compile the func to machine code directly 
			//if ( comp_compile_func(eng, func) < 0 ) {
			if ( create_classify_func ( eng, func) < 0 ) { 
				printf("error in compiling %s\n", func->name );
				return -1;
			}

			eng->prodInfo[prod->id].action = func;
		}	
	}

	return 0;
}

nel_type *reduction_action_functype_alloc(struct nel_eng *eng, struct nel_SYMBOL *prod)
{
	nel_symbol *symbol;
	nel_type *type;
	nel_list *args;
	struct nel_RHS *rhs = NULL;
	int i;
	char name[16];

	/* create $0 */ 
	if(eng->startSymbol && eng->startSymbol->type ) {
		type = eng->startSymbol->type->event.descriptor;
	}else {
		type = nel_pointer_type;
	}

	symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,"$0"), type, NULL, nel_C_FORMAL, nel_lhs_type(type), nel_L_NEL, 1);
	args = nel_list_alloc(eng, 0, symbol, NULL);


	/*get rhs according to prod->rhs */ 
	for( i=1, rhs=prod->type->prod.rhs;i <= (prod->type->prod.rhs_num); i++, rhs= rhs->next){
		type = rhs->symbol->type->event.descriptor;
		sprintf(name, "$%d", i);
		symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,name),type , NULL, nel_C_FORMAL, nel_lhs_type(type), nel_L_NEL, 1);
		args = nel_list_alloc(eng, 0, symbol, args);
	} 

	/* return value */
	type = prod->type->prod.lhs->type->event.descriptor;

	/* create the type at last */
	type = nel_type_alloc(eng, nel_D_FUNCTION, 0,0, 0, 0, 0, 0, type, args, NULL, NULL);

	return type;
}


	
nel_symbol *reduction_action_symbol_alloc(struct nel_eng *eng, FILE *fp, char *name, nel_type *type, nel_stmt *head)
{
	nel_symbol *func;
	nel_stmt *start=NULL;
	nel_stmt *stmt, *prev_stmt=NULL;
	nel_list *scan;
        nel_stmt *target = NULL;
        char *value;
        nel_symbol *symbol;
        nel_expr *expr;
	nel_stmt *tail_stmt;


	/* nel_S_DEC */
	for(scan = type->function.args; scan; scan= scan->next ){
		nel_symbol *symbol = scan->symbol;
		stmt = nel_stmt_alloc(eng, nel_S_DEC, "", 0, symbol, NULL);
		if(!start)
			start = stmt;	
		if(prev_stmt) 
			nel_stmt_link(prev_stmt, stmt); 
		prev_stmt = stmt;
	}

	/* chain func body */
	if(prev_stmt && head ){
		nel_stmt_link(prev_stmt, head);
		tail_stmt = nel_stmt_tail(head);
		prev_stmt = (tail_stmt) ? tail_stmt: head;
	}


	/* add return 0 stmt at last */
        value = nel_static_value_alloc(eng, sizeof(int), nel_alignment_of(int)); 
	symbol =  nel_static_symbol_alloc(eng, "", nel_int_type,value, nel_C_CONST,0,0,0);
        *((int *)symbol->value) = 0;
        expr = nel_expr_alloc(eng, nel_O_SYMBOL, symbol);
        stmt = nel_stmt_alloc(eng, nel_S_RETURN, "", 0, expr, NULL);
        if(prev_stmt)
                nel_stmt_link(prev_stmt, stmt);
        prev_stmt = stmt;


	func = nel_static_symbol_alloc(eng, nel_insert_name(eng,name),type, (char *)start, nel_C_NEL_FUNCTION, nel_lhs_type(type), nel_L_NEL, 0);
        nel_insert_symbol (eng, func, eng->nel_static_ident_hash);

	if(eng->compile_level > 0){
		/* output this func to temporary file */
		//emit_symbol(fp, func);
	}

	return func;

}

int reduction_action_alloc(struct nel_eng *eng)
{
	struct nel_SYMBOL *prod;
	char name[256];
	FILE *fp = NULL;

	for(prod = eng->productions; prod; prod=prod->next){
		sprintf(name, "do_reduction_%d", prod->id);
		prod->type->prod.action = 
			(nel_symbol *)reduction_action_symbol_alloc(eng, fp, 
			name, reduction_action_functype_alloc(eng, prod), 
				prod->type->prod.stmts );	
	}

	return 0;
}
