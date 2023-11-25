
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
#include "action.h"
#include "prod.h"

//#include "comp.h"


/************************************************************************/
/* will create a func call					 	*/
/*	addSymbolId(SymbolSet *result, int id);				*/
/* rather then calling:							*/ 
/* 	result->symbolIds[result->num] = id;				*/
/* 	result->num++ 							*/
/* this will make our life simpler :)					*/
/************************************************************************/
nel_stmt *class_output_stmt_alloc(struct nel_eng *eng, nel_symbol *result, nel_symbol *out_symbol)
{	
	nel_stmt *stmt;
	nel_expr *func, *fun_call;
	nel_expr *expr;
	char *value;
	nel_symbol *symbol;
	nel_expr_list *ms_expr_list, 
			*Symbol_expr_list;

	/* create the second parameter id */
	value = nel_static_value_alloc(NULL, sizeof(int), nel_alignment_of(int));
	symbol = nel_static_symbol_alloc(eng, "id", nel_int_type,value, nel_C_CONST,0,0,0);

	if (out_symbol->class == nel_C_TERMINAL)
		*((int *)symbol->value) = out_symbol->id + 1;
	else if (out_symbol->class == nel_C_NONTERMINAL)
		*((int *)symbol->value) = - out_symbol->id - 1 ;

	expr = nel_expr_alloc(eng, nel_O_SYMBOL, symbol);
	Symbol_expr_list = nel_expr_list_alloc(eng, expr, NULL);	

	/* create the first parameter accroding to symbol "result" */
	expr = nel_expr_alloc(eng, nel_O_SYMBOL, result);
	ms_expr_list  = nel_expr_list_alloc(eng, expr, Symbol_expr_list);	

	/* then is func */
	symbol = nel_lookup_symbol("addSymbolId", eng->nel_static_ident_hash, eng->nel_static_location_hash, NULL);
	func = nel_expr_alloc(eng, nel_O_SYMBOL, symbol); 

	/* last, alloc fun_call expr and stmt */
	fun_call  = nel_expr_alloc(eng, nel_O_FUNCALL, func, 2, ms_expr_list);
	stmt = nel_stmt_alloc(eng, nel_S_EXPR, "", 0, fun_call, NULL);

	return stmt;
}


int do_class_stmt_alloc(struct nel_eng *eng, nel_expr *expr, nel_stmt *output_stmt, nel_stmt *end_stmt, nel_stmt **stmt_head, nel_stmt **stmt_tail)
{
	nel_stmt *stmt;
	nel_symbol *func = NULL;


	if (!expr) {
		gen_error(NULL, "input error \n");
		return -1;
	}

	switch (expr->gen.opcode) {

		/**************************/
		/* terminal node (symbol) */
		/**************************/
		case nel_O_SYMBOL:
		case nel_O_NEL_SYMBOL:
			//break;

		/********************/
		/* unary operations */
		/********************/
		case nel_O_ADDRESS:
		case nel_O_BIT_NEG:
		case nel_O_DEREF:
		case nel_O_NEG:
		case nel_O_NOT:
		case nel_O_POS:
		case nel_O_POST_DEC:
		case nel_O_POST_INC:
		case nel_O_PRE_DEC:
		case nel_O_PRE_INC:
			//retval->unary.operand = va_arg (args, nel_expr *);
			//break;

		/*********************/
		/* binary operations */
		/*********************/
		case nel_O_ADD:
		case nel_O_ADD_ASGN:
		case nel_O_AND:
		case nel_O_ARRAY_INDEX:
		case nel_O_ASGN:
		case nel_O_BIT_AND:
		case nel_O_BIT_AND_ASGN:
		case nel_O_BIT_OR:
		case nel_O_BIT_OR_ASGN:
		case nel_O_BIT_XOR:
		case nel_O_BIT_XOR_ASGN:
		case nel_O_DIV:
		case nel_O_DIV_ASGN:
		case nel_O_LSHIFT:
		case nel_O_LSHIFT_ASGN:
		case nel_O_MOD:
		case nel_O_MOD_ASGN:
		case nel_O_MULT:
		case nel_O_MULT_ASGN:
		case nel_O_OR:
		case nel_O_RSHIFT:
		case nel_O_RSHIFT_ASGN:
		case nel_O_SUB:
		case nel_O_SUB_ASGN:
		case nel_O_NE:
			/* skip process of compilicated expression, though 
			it is an interesting subject.
			but it is far beyond our prespective */ 
			stmt = nel_stmt_alloc(eng, nel_S_BRANCH, 
						"", 	/* filename */
						0, 	/* line num */
							1,	/* level */
						expr,
						output_stmt,	/* true stmt */
						NULL);	/* false stmt */ 
			if (!stmt) 
				return -1;
			
			//nel_stmt_link(stmt, end_stmt);
			*stmt_head = *stmt_tail = stmt;
			break;

		case nel_O_EQ:
		case nel_O_GE:
		case nel_O_GT:
		case nel_O_LE:
		case nel_O_LT:
			stmt = nel_stmt_alloc(eng, nel_S_BRANCH,
					"", 	/* filename */
					0, 	/* line num */
						1,	/* level */
						expr,
					output_stmt,	/* true stmt */
						NULL);	/* false stmt */ 
			if (!stmt) 
				return -1;

			//nel_stmt_link(stmt, end_stmt);
			*stmt_head = *stmt_tail = stmt;	
			break;

		case nel_O_COMPOUND:
#if 0
			{
				nel_stmt *temp_stmt, *sub_stmt_head, *sub_stmt_tail;
			
				sub_stmt_head = sub_stmt_tail = NULL;
				if (do_class_stmt_alloc(eng, expr->binary.right, output_stmt, end_stmt, &sub_stmt_head, &sub_stmt_tail) == -1)
				{
					//nel_stmt_dealloc(*stmt_head);
					return -1;
				}
				temp_stmt = sub_stmt_head;


				sub_stmt_head = sub_stmt_tail = NULL;
				if (do_class_stmt_alloc(eng, expr->binary.left, NULL,  end_stmt, &sub_stmt_head, &sub_stmt_tail) == -1)
					return -1;

				sub_stmt_head->branch.true_branch = temp_stmt;

				//nel_stmt_link(sub_stmt_tail, end_stmt);
				*stmt_head = sub_stmt_head;
				*stmt_tail = sub_stmt_tail;
			}
			break;
#else
			stmt = nel_stmt_alloc(eng, nel_S_BRANCH,
					"", 	/* filename */
					0, 	/* line num */
						1,	/* level */
					nel_expr_alloc(eng, nel_O_AND, 
						expr->binary.right, 
						expr->binary.left),
					output_stmt,	/* true stmt */
						NULL);	/* false stmt */ 
			if (!stmt) 
				return -1;

			//nel_stmt_link(stmt, end_stmt);
			*stmt_head = *stmt_tail = stmt;	

#endif

		/***********************/
		/* struct/union member */
		/***********************/
		case nel_O_MEMBER:
			//retval->member.s_u = va_arg (args, nel_expr *);
			//retval->member.member = va_arg (args, nel_symbol *);
			break;

		/*****************/
		/* function call */
		/*****************/
		case nel_O_FUNCALL:
#if 0
			{
			nel_symbol *func, *prev_func_sym, *post_func_sym;
			func = nel_lookup_symbol(expr->funcall.func->symbol.symbol->name, eng->nel_static_ident_hash, eng->nel_static_location_hash, NULL);
			if (!func)
				break;
			
			if ((prev_func_sym = func->type->function.prev_hander)
				&& (post_func_sym = func->type->function.post_hander)){
				nel_expr *new_expr;
				nel_stmt *pre_func_stmt, *post_func_stmt;
				prev_func prev_hander;
				post_func post_hander;
				struct prior_node *pnode;
				
				prev_hander = (prev_func)prev_func_sym->value;
				post_hander = (post_func)post_func_sym->value;

				pnode = new_prior_node(eng, expr, 0, CLUS_FUNCTION);
				if (!pnode)
					return -1;

				new_expr = (*prev_hander)(eng, pnode);
				pre_func_stmt=nel_stmt_alloc(eng, nel_S_BRANCH,
							"", 	/* filename */
							0, 	/* line num */
							1,	/* level */
								new_expr,
							NULL,	/* true stmt */
							NULL);	/* false stmt */ 
				if (!pre_func_stmt) { 
					free_prior_node(pnode, CLUS_FUNCTION);
					return -1; 
				}

				new_expr = (*post_hander)(eng, pnode, 0);
				post_func_stmt=nel_stmt_alloc(eng, nel_S_BRANCH,
							"", 	/* filename */
							0, 	/* line num */
							1,	/* level */
								new_expr,
							output_stmt,	/* true stmt */
							NULL);	/* false stmt */ 
				if (post_func_stmt == NULL) {
					nel_stmt_dealloc(pre_func_stmt);
					free_prior_node(pnode, CLUS_FUNCTION);
					return -1;
				}

				pre_func_stmt->branch.true_branch = post_func_stmt;

				//nel_stmt_link(pre_func_stmt, end_stmt);
				*stmt_head = *stmt_tail = pre_func_stmt;

				free_prior_node(pnode, CLUS_FUNCTION);		
			}
			else {
#endif
				stmt = nel_stmt_alloc(eng, nel_S_BRANCH, 
						"", 	/* filename */
						0, 	/* line num */
							1,	/* level */
						expr,
						output_stmt,	/* true stmt */
							NULL);	/* false stmt */
		 		if (!stmt) 
					return -1;

				*stmt_head = *stmt_tail = stmt;

#if 0
			}
			}
#endif

			break;

		/**********************************************/
		/* type epxression: sizeof, typeof, or a cast */
		/**********************************************/
		case nel_O_CAST:
		case nel_O_SIZEOF:
		case nel_O_TYPEOF:
			//retval->type.type = va_arg (args, nel_type *);
			//retval->type.expr = va_arg (args, nel_expr *);
			break;

		/******************/
		/* conditional: ? */
		/******************/
		case nel_O_COND:
			//retval->cond.cond = va_arg (args, nel_expr *);
			//retval->cond.true_expr = va_arg (args, nel_expr *);
			//retval->cond.false_expr = va_arg (args, nel_expr *);
			break;

		/*********************************************************/
		/* the following should never occur as the discriminator */
		/* of the nel_intp_eval_expr union -                      */
		/* they are only used for error messages.                */
		/*********************************************************/
		case nel_O_MEMBER_PTR:
		case nel_O_RETURN:
		default:
			break;
	}

	return 0;	
}


int class_func_stmt_alloc(struct nel_eng *eng, struct nel_LIST *sym_list, nel_symbol *result, nel_stmt **stmt_head, nel_stmt **stmt_tail)
{
	nel_stmt *prev_stmt = NULL;
	struct nel_LIST *scan;

	for (scan= sym_list; scan != NULL; scan=scan->next){
		nel_symbol *psym;

		if ((psym = scan->symbol) != NULL) {
			nel_stmt *sub_stmt_head, *sub_stmt_tail;

			if (do_class_stmt_alloc(eng, (nel_expr *)psym->value, 
				class_output_stmt_alloc(eng, result,psym), NULL,
					&sub_stmt_head, &sub_stmt_tail) == -1) {
				if (*stmt_head)
					nel_stmt_dealloc(*stmt_head);
				return -1;
			}

			if (!*stmt_head)
				*stmt_head= sub_stmt_head;
			if (prev_stmt)
				nel_stmt_link(prev_stmt, sub_stmt_head);	
			prev_stmt = sub_stmt_tail;
		} 
	} 

	*stmt_tail = prev_stmt; 
	return 0;
}

/*****************************************************************
 * Purpose: Carry out an optimized decision tree, according to 
 * 			the input syntax tree. 
 *****************************************************************/
int class_func_body_alloc(struct nel_eng *eng, struct nel_LIST *sym_list, int count, nel_symbol *result,  nel_stmt **stmt_head, nel_stmt **stmt_tail)
{
	nel_stmt *sub_stmt_head=NULL, 
		*sub_stmt_tail=NULL,
		*prev_stmt_tail =NULL;
	struct nel_LIST *scan,
			*prev = NULL;
	
	/* output those symbols without expression */	
	for (scan =sym_list; scan; scan = scan->next ){
		if(scan->symbol->value == NULL) {
			nel_stmt *stmt;

			stmt = class_output_stmt_alloc(eng, result, scan->symbol);
			if (!stmt)
				goto Error;

			if(!*stmt_head)
				*stmt_head = stmt;
			if(prev_stmt_tail)
				nel_stmt_link(prev_stmt_tail, stmt);
			prev_stmt_tail = stmt;

			if(prev){
				prev->next = scan->next;
			}else {
				sym_list = scan->next;
			}
			count--;
		}else
			prev = scan;
	}

	/* if still have symbol with expression, output it */
	if( sym_list != NULL ){	
		if (eng->optimize_level) {
			if(optimize_class_func_stmt_alloc(eng,sym_list, count,
				 result, &sub_stmt_head, &sub_stmt_tail ) < 0)
				goto Error;
		} else {
			if (class_func_stmt_alloc(eng, sym_list, result, 
					&sub_stmt_head, &sub_stmt_tail) < 0 )
				goto Error;
		}

		if(!*stmt_head)
			*stmt_head= sub_stmt_head;
		if(prev_stmt_tail) 
			nel_stmt_link(prev_stmt_tail, sub_stmt_head);
		if(sub_stmt_tail)
			prev_stmt_tail = sub_stmt_tail;
	}

	if(prev_stmt_tail)
		*stmt_tail = prev_stmt_tail; 
	
	return 0;

Error:
	if (*stmt_head)
		nel_stmt_dealloc(*stmt_head);
	return -1;
}

nel_type *class_func_type_alloc(struct nel_eng *eng, struct lritem *it, nel_symbol **result)
{
	nel_symbol *symbol;
	nel_type *type;
	nel_list *args;
	int i;
	struct nel_RHS *rhs = NULL; 
	char name[16];

	/* create $0 */
	if(eng->startSymbol && eng->startSymbol->type) {
		type = eng->startSymbol->type->event.descriptor;
	}else {
		type = nel_pointer_type;
	}

	symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,"$0"), type, NULL, nel_C_FORMAL, nel_lhs_type(type), nel_L_NEL, 1);
	args = nel_list_alloc(eng, 0, symbol, NULL);


	/* if it is kernel(it->offset == 0), we needn't do this process, 
	otherwise, create NULL param for pos */ 
	for(i = 1; i<=it->offset; i++) { 
		type = nel_pointer_type;
		sprintf(name, "$%d", i);
		symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,name), type, NULL, nel_C_FORMAL, nel_lhs_type(type), nel_L_NEL, 1);
		args = nel_list_alloc(eng, 0, symbol, args);
	}


	/*get rhs according to it->dprod->dot*/ 
	for( i=it->offset+1, rhs=it->dprod->prod->type->prod.rhs; i <= (it->offset + it->dprod->dot + 1); i++, rhs= rhs->next) {
		type = rhs->symbol->type->event.descriptor;	
		sprintf(name, "$%d", i);
		symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,name), type, NULL, nel_C_FORMAL, nel_lhs_type(type), nel_L_NEL, 1);
		args = nel_list_alloc(eng, 0, symbol, args);
	} 

	/* the last param is result */
	symbol = nel_lookup_symbol("SymbolSet", eng->nel_static_tag_hash,NULL);
	type = symbol->type->typedef_name.descriptor;
	type = nel_type_alloc(eng, nel_D_POINTER, sizeof(void *), nel_alignment_of(void *), 0, 0, type);
	*result = symbol = nel_static_symbol_alloc(eng, "result", type, NULL, nel_C_FORMAL, nel_lhs_type(type), nel_L_NEL, 1);
	args = nel_list_alloc(eng, 0, symbol, args);


	/* create the type at last */
	type = nel_type_alloc(eng, nel_D_FUNCTION, 0,0, 0, 0, 0, 0, nel_int_type, args, NULL, NULL);

	return type;
}


nel_symbol *class_func_symbol_alloc(struct nel_eng *eng, char *name, nel_type *type, nel_stmt *head, nel_stmt *tail)
{
	nel_symbol *func;
	nel_stmt *start=NULL;
	nel_stmt *stmt, *prev_stmt=NULL;
	nel_list *scan;
	nel_stmt *target = NULL;
	char *value;
	nel_symbol *symbol;
	nel_expr *expr;


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

	/* add '{' */
	target = nel_stmt_alloc (eng, nel_S_TARGET, NULL/*filename*/,
					0/*line*/, 1 /*level*/, NULL);
	if(prev_stmt)
		nel_stmt_link(prev_stmt, target);
	prev_stmt = target;	


	/* add func body*/
	if(prev_stmt)
		nel_stmt_link(prev_stmt, head);
	prev_stmt = tail;


	/* add  return stmt */
	value = nel_static_value_alloc(eng, sizeof(int), nel_alignment_of(int));
	symbol =  nel_static_symbol_alloc(eng, "", nel_int_type,value, nel_C_CONST,0,0,0);
	*((int *)symbol->value) = 0;
	expr = nel_expr_alloc(eng, nel_O_SYMBOL, symbol);
	stmt = nel_stmt_alloc(eng, nel_S_RETURN, "", 0, expr, NULL);
	if(prev_stmt)
		nel_stmt_link(prev_stmt, stmt);
	prev_stmt = stmt;
	

	/* add '}' */
	target = nel_stmt_alloc (eng, nel_S_TARGET, NULL/*filename*/,
					0/*line*/, 1 /*level*/, NULL);
	if(prev_stmt)
		nel_stmt_link(prev_stmt, target);
	prev_stmt = target;	


	func = nel_static_symbol_alloc(eng, nel_insert_name(eng, name),type,(char *)start, nel_C_NEL_FUNCTION, nel_lhs_type(type), nel_L_NEL, 0);

        nel_insert_symbol (eng, func, eng->nel_static_ident_hash);

	
	return func;
}

int create_classify_func (struct nel_eng *eng, nel_symbol *symbol)
{
        eng->compile_level = 0;

        if(symbol == NULL
                || symbol->type == NULL
                || symbol->type->simple.type != nel_D_FUNCTION
                || symbol->value == NULL) {
                return 0;
        }

        if(eng->compile_level < 2) { 
                nel_stmt *head, *tail;
                ast_to_intp(eng, (nel_stmt *)symbol->value, &head, &tail);
                symbol->value = (char *)head;
                return 0;
        }

        return 0;
}



//int term_classify_compile(struct nel_eng *eng)
int create_term_classify_func (struct nel_eng *eng)
{
	int sid, tid; 
	unsigned long val;
	nel_symbol *func;

	/* update func */
	for(sid = 0; sid < eng->numStates; sid ++){
		for(tid = 0; tid < eng->numTerminals ; tid++) {
			if ( (func = SYM_OF_TERM_CLASS(eng, sid, 
				encodeSymbolId(eng, tid, nel_C_TERMINAL)))) { 
			
				// compile the func to machine code directly 
				//if ( comp_compile_func(eng, func) < 0 ){
				if ( create_classify_func(eng, func) < 0 ){
					printf("error in compiling %s\n", func->name );
					return -1;
				}
			}
		}	
	}

	return 0;
}

//int nonterm_classify_compile(struct nel_eng *eng)
int create_nonterm_classify_func(struct nel_eng *eng)
{
	int sid, nid;
	unsigned long val;
	nel_symbol *func;

	for(sid = 0; sid < eng->numStates; sid ++) {	
		for(nid = 0; nid < eng->numNonterminals; nid ++){

			if((func = SYM_OF_NONTERM_CLASS(eng,sid, 
				encodeSymbolId(eng, nid, nel_C_NONTERMINAL)))) {
				
				// compile the func to machine code directly 
				//if ( comp_compile_func(eng, func) < 0 ) {
				if ( create_classify_func (eng, func) < 0 ) {
					printf("error in compiling %s\n", func->name );
					return -1;
				}

			}
		}	
	}

	return 0;
}
	
int term_classify_alloc(struct nel_eng *eng)
{
	struct nel_LIST *list=NULL, *this;
	nel_symbol *term;
	struct lritem *it;
	struct itemset *is;
	int count ;
	FILE *fp = NULL;

	for(is = eng->gen->itemSets; is; is = is->next)
	{
		int dokernel ;
		int len ;	
		nel_type *type;
		int outputting_term_id;
		nel_symbol *result;

		for(term= eng->terminals; term; term=term->next) {
			term->_flag = 0;
		}
output_next_term:

		len = 0;
		outputting_term_id = -1;
		count =0;

		if(is->id == 0){
			;
		}

		for(it=is->kernelItems,dokernel=1; it;
		    it=(it->next)? 
			it->next
			:( dokernel?
				(dokernel=0,is->nonkernelItems)
				:((struct lritem *)0)
			 )
		   )
		{
			for( term = eng->terminals; term; term = term->next)
			{
				//union nel_EXPR *expr = (union nel_EXPR *)term->value;
				if(term->_flag ) continue;

				if (!is_dot_at_end(it->dprod)){ 
					if (term != sym_after_dot(it->dprod)|| 
					!is->termTransition[term->id] ) {
						continue;
					}	
				}

				else { 
					continue;
#if 0
					if(!ts_test(&it->lookahead,term->id)){
						continue;
					}

					/* we use this technique to elimate 
					classification before reduce, hope this
					will speed up parser largely */ 
					expr = NULL;
#endif
				}

				/* have got an term need to be output,create func type first */
				if(outputting_term_id == -1){
					type=class_func_type_alloc(eng, it,&result);
					len =it->offset + it->dprod->dot; 
					outputting_term_id = term->_pid;
				}


				/* output term only have same pid */
				if(term->_pid != outputting_term_id){
					continue;
				}


				if( term->value){
					if(it->offset){
						term->data = term->value;
						term->value = (char *) nel_dup_expr( eng, (union nel_EXPR *)term->value);
						nel_expr_update_dollar(eng, (union nel_EXPR *)term->value, 0 ,it->offset);
					}else{
						term->data = NULL;
					}
				}
						

				/* link the term for later optimize*/
				this = nel_list_alloc(eng, 0, term, NULL);
				__append_entry_to(&list, this);


				term->_flag = 1; 
				count++;


			}//end of term


		} //end of it 

		if(outputting_term_id >= 0 ) {
			nel_stmt *head=NULL, *tail=NULL;
			char name[256];

			sprintf(name, "term_class_%d_%d", is->id, outputting_term_id);
			class_func_body_alloc(eng, list,count, result, &head, &tail);

			SYM_OF_TERM_CLASS(eng, is->id, 
			encodeSymbolId(eng,outputting_term_id,nel_C_TERMINAL))= 
				class_func_symbol_alloc(eng, name, type, head, tail); 
			LEN_OF_TERM_CLASS(eng, is->id, 
			encodeSymbolId(eng,outputting_term_id,nel_C_TERMINAL))=
				len;

			while(list){
				struct nel_LIST *next = list->next;
				if(list->symbol->data ){
					list->symbol->value = list->symbol->data;
					list->symbol->data = NULL;
				}
				nel_list_dealloc(list);list = next;
			}	

			goto output_next_term;	
		}

	} // end of is

	return 0;
}

int nonterm_classify_alloc(struct nel_eng *eng)
{
	struct itemset *is;
	struct lritem *it;
	nel_symbol *nt;
	struct nel_LIST *list=NULL, *this;
	int count;
	FILE *fp = NULL;

	/*--------------------------------------------------------
		We will generate switch , rather then an array so 
		no need to use index version: for (i=0; i< numStates; i++) 
	 ---------------------------------------------------------*/
	for(is = eng->gen->itemSets; is; is = is->next)
	{
		int dokernel ;
		int len;
		nel_type *type;
		int outputting_symbol_id;
		nel_symbol *result;

		for(nt=eng->nonterminals; nt; nt=nt->next) {
			nt->_flag = 0;
		}

output_next_nonterm:
		len = 0;
		outputting_symbol_id = -1;
		count =0;

		for(it=is->kernelItems,dokernel=1; it;
			    it= ((it->next) ? 
					(it->next)
					:( dokernel ?
						(dokernel=0,is->nonkernelItems)
						:((struct lritem *)0)
					 )
				)
		  )
		{
			struct nel_RHS *rhs;
			nel_symbol *sym;

			if( (sym = sym_after_dot(it->dprod)) 
				&& (sym->class == nel_C_NONTERMINAL)
				&& (nt = (nel_symbol *)sym)
			 && (is->nontermTransition[nt->id])
			  )
			{
				int i;
				if(sym->_flag )
					continue;

				// get rhs according to it->dprod->dot 
				for( i = 1, rhs = it->dprod->prod->type->prod.rhs;
					i < it->dprod->dot + 1; 
						i++, rhs= rhs->next ) ;

				if(!rhs || rhs->symbol != sym ){
					// we ignore this error symbol, 
					continue;
				}

				if (outputting_symbol_id == -1 ) {
					len =it->offset + it->dprod->dot; 
					type=class_func_type_alloc(eng, it,&result);
					outputting_symbol_id= sym->_pid;
				}


				if(sym->_pid != outputting_symbol_id){
					continue;
				}

				if(sym->value){
					if(it->offset){
						sym->data = sym->value;
						sym->value = (char *)nel_dup_expr(eng, (union nel_EXPR *)sym->value);
						nel_expr_update_dollar(eng, (union nel_EXPR *)sym->value, 0 ,it->offset);
					}else{
						sym->data = NULL;
					}
				}
	
				/*add this term to list for later optimize*/
				this = nel_list_alloc(eng, 0, sym, NULL);
				__append_entry_to(&list, this);

				count++;
				sym->_flag = 1;
			
			}

		}

	
		if(outputting_symbol_id >= 0 ) {
			nel_stmt *head=NULL, *tail=NULL;
			char name[256];

			sprintf(name, "nonterm_class_%d_%d", is->id, outputting_symbol_id);
			class_func_body_alloc(eng, list,count, result, &head, &tail);

			SYM_OF_NONTERM_CLASS(eng, is->id, 
			encodeSymbolId(eng,outputting_symbol_id, nel_C_NONTERMINAL)) = class_func_symbol_alloc(eng, name, type, head, tail);
			LEN_OF_NONTERM_CLASS(eng, is->id, 
			encodeSymbolId(eng,outputting_symbol_id, nel_C_NONTERMINAL)) = len;
			while(list){
				struct nel_LIST *next = list->next;
				if(list->symbol->data){
					list->symbol->value = list->symbol->data;
					list->symbol->data = NULL;
				}
				nel_list_dealloc(list);list = next;
			}	

			goto output_next_nonterm;
		}

	}
	
	return 0;
}
	

void emit_term_class(struct nel_eng *eng )
{
	FILE *fp;
	if(eng->termclass_debug_level) {
		if((fp=fopen("termclass", "w"))) {
			struct itemset *is;
			nel_symbol *term;

			fprintf(fp, "Term Classifiers \n"); 	
			for(is = eng->gen->itemSets; is ; is = is->next) {
				for(term = eng->terminals; term; term = term->next){
					emit_symbol(fp,  SYM_OF_TERM_CLASS(eng, is->id, encodeSymbolId(eng,term->id,nel_C_TERMINAL)));
				}
			}
			fclose(fp);	
		}

	}
	return ;
}

void emit_nonterm_class(struct nel_eng *eng )
{
	FILE *fp;
	if(eng->nontermclass_debug_level) {
		if((fp=fopen("nontermclass", "w"))) {
			struct itemset *is;
			nel_symbol *nt;

			fprintf(fp, "---------------------------NonTerm Classifiers ------------------\n"); 	
			for(is = eng->gen->itemSets; is ; is = is->next)
			{
				for(nt = eng->nonterminals; nt; nt= nt->next){
					emit_symbol(fp, SYM_OF_NONTERM_CLASS(eng,is->id, encodeSymbolId(eng,nt->id, nel_C_NONTERMINAL)));
				}
			}
			fclose(fp);
		}
	}

	return ;
}


int class_alloc(struct nel_eng *eng)
{
	//comp_init(eng);

	/* first generator term and nonterm class function */
	if(term_classify_alloc(eng) < 0 
		|| nonterm_classify_alloc(eng) < 0) {
		return -1;
	}

	emit_term_class(eng);
	emit_nonterm_class(eng);

	/* compile the classify function to :
	1, intpcode, (eng->compile_level == 0)
	2, machine code, (eng->compile_level == 2)
	3, syntax check only, do nothing, (eng->compile_level == 1)
	*/ 
	//term_classify_compile(eng);
	//nonterm_classify_compile(eng);
	create_term_classify_func(eng);
	create_nonterm_classify_func(eng); 


	/* evt_free_func and reduction should are not compiled at 
	parser time (in nel.y), because generator would modify them */
	//evt_free_func_compile(eng);
	reduction_action_compile(eng);

	// link and load only when eng->compile_level == 2 
	//comp_relocate(eng);
	
	return 0;
}

