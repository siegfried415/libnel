/*
 * optimize.c
 * $Id: opt.c,v 1.12 2006/11/28 06:15:31 yanf Exp $
 */

#include <stdio.h>
#include <stdlib.h>

#include "engine.h"
#include "errors.h"
#include "stmt.h"
#include "expr.h"
#include "opt.h"
#include "mem.h"
#include "class.h" 
#include "gen.h"

#define IS_FUNC_NODE	1
#define NOT_FUNC_NODE	0

void free_exp_node(struct exp_node *expnode, int is_func)
{
	if (expnode->id_list) {
		free_int_list(expnode->id_list);
		expnode->id_list = NULL;
	}
	if (is_func && expnode->opr.arglist) { 
		nel_free(expnode->opr.arglist);	
		expnode->opr.arglist = NULL;
	}
	nel_free(expnode);
	return;
}


struct exp_node *new_exp_node (int_list_t *id_list)
{
	struct exp_node *exp;

	nel_calloc(exp, 1, struct exp_node);
	if (!exp) {
		gen_error(NULL, "nel_malloc error \n");
		return NULL;
	}

	exp->id_list = id_list;
	exp->val = F;
	return exp;
}


struct exp_node *new_relate_exp (nel_expr *expr, int_list_t *id_list)
{
	struct exp_node *exp;
	nel_expr *num;

	exp = new_exp_node(id_list);
	if (!exp)
		return NULL;

	num = expr->binary.right;
	exp->opr.num = *((int *)num->symbol.symbol->value);
	exp->op = expr->binary.opcode;
	exp->expr = expr;

	return exp;
}


struct exp_node *new_func_exp (nel_expr_list *args, int id, nel_expr *expr)
{
	nel_expr_list *tmpargs;
	struct exp_node *exp;
	int_list_t *id_list; 
	int i;
	
	id_list = new_int_list(id, NULL); 
	exp = new_exp_node(id_list);
	if (!exp) {
		free_int_list(id_list);
		return NULL;
	}

	tmpargs = args;
	i = 0;
	while (tmpargs) {
		i++;
		tmpargs = tmpargs->next;
	}

	//nel_calloc(exp->opr.arglist, i, nel_expr*);
	nel_calloc(exp->opr.arglist, i+1, nel_expr*);  
	if (!exp->opr.arglist) {
		gen_error(NULL, "nel_malloc error \n");
		free_exp_node(exp, IS_FUNC_NODE);
		return NULL;
	}

	for (i=0; args; args=args->next, i++) {
		exp->opr.arglist[i] = args->expr; 
	}

	exp->expr = expr;
	return exp;
}


int add_relate_exp(struct prior_node *prior, int id, nel_expr *expr)
{
	nel_expr *num_expr;
	struct exp_node *exp, *nexp;
	int_list_t *id_list ;
	int number;

	if (!expr || !prior) {
		gen_error(NULL, "input error \n");
		return -1;
	}

	id_list = new_int_list(id, NULL);
	if (!id_list)
		return -1;

	prior->numcount++;
	nexp = new_relate_exp(expr, id_list);
	if (!nexp)
		return -1;

	if((exp = prior->exp)) {
		if (exp->prev) {
			exp->prev->next = nexp;
			nexp->prev = exp->prev;
		}
		nexp->next = exp;
		exp->prev = nexp;

		//if (exp == prior->exp)
			prior->exp = nexp;

		return 0;

	}else {
		/*the first exp, set prior->exp and prior->last_exp directly */	
		nexp->next = nexp->prev = NULL;
		prior->exp = prior->last_exp = nexp;

	}

	return 0;
}


int add_func_exp(struct prior_node *prior, int id, nel_expr *expr, nel_expr_list *args)
{
	int i;
	int found=0;	/* -1 -- not found;  
			    0 -- not sure;
			    1 -- found      */
	struct exp_node *exp;
	nel_expr_list *org_args = args;

	if (!args || !prior) {
		gen_error(NULL, "input error \n");
		return -1;
	}

	if (prior->exp) {
		/* Walk through the 'exp' list in the 'struct prior_node' */
		for (exp=prior->exp; exp; exp=exp->next) {
			nel_expr **exp_args = exp->opr.arglist;
			for (i=0; args && exp_args[i]; args=args->next, i++) {
				if (nel_expr_diff(args->expr, exp_args[i])) {
					break;
				}
			}	
	
			if (!args && !exp_args[i]) {
				/* so 'splitcount' unchanged */
				if (insert_int_list(&exp->id_list, id) == -1)
					return -1;
				return 0;	//bugfix, 2006.5.23 

			}
			
		} 

	}
		
	if (found != 1) {   
		prior->numcount++;
		exp = new_func_exp(org_args, id, expr);
		if (!exp)
			return -1;

		if (!prior->exp)
			prior->exp = exp;
		if (prior->last_exp){
			exp->prev = prior->last_exp;
			prior->last_exp->next = exp;
		}
		prior->last_exp = exp;
	}

	return 0;
}


/* free the the whole struct prior_node List pointed by 'prinode' */
void free_prior_node(struct prior_node *prinode, PRIORTYPE type)
{
	struct prior_node *pnode;
	struct exp_node *exp;
	
	pnode = prinode;
	while (pnode != NULL) {
		prinode = prinode->next;
		
		/* free the 'varlist' */	
		if (pnode->varnum > 0) {
			nel_free(pnode->varlist);	
			pnode->varlist = NULL;
			pnode->varnum = 0;
		}

		/* free the 'exp' */	
		exp = pnode->exp;
		while(exp){
			struct exp_node *tmp = exp;	
			exp = exp->next;
			if (tmp) {
				if (type==SIMP_FUNCTION || type==CLUS_FUNCTION)
					free_exp_node(tmp, IS_FUNC_NODE);
				else
					free_exp_node(tmp, NOT_FUNC_NODE);
			}
		}

		nel_free(pnode);	
		pnode =prinode;
	}
	return;
}

/* struct prior_node stuff */
struct prior_node *new_prior_node(struct nel_eng *eng, nel_expr *expr, int id, PRIORTYPE type)
{
	struct prior_node *prior;
	int i;
	
	if (!expr) {
		gen_error(NULL, "input error \n");
		return NULL;
	}

	nel_calloc(prior, 1, struct prior_node);
	if (!prior) {
		gen_error(NULL, "nel_malloc error \n");
		return NULL;
	}

	/* the keyword of a struct prior_node */
	switch(type){
	case SIMP_FUNCTION:
	case CLUS_FUNCTION: 
		{
			nel_expr_list *args;
			nel_symbol *func = nel_lookup_symbol(expr->funcall.func->symbol.symbol->name, 
					eng->nel_static_ident_hash, eng->nel_static_location_hash, NULL);

			prior->sbj = expr->funcall.func;
			prior->varnum = (func->type)->function.key_nums; 
			args = expr->funcall.args;
			if (prior->varnum > 0) {
				nel_calloc(prior->varlist, prior->varnum, nel_expr*);
				if (!prior->varlist) {
					gen_error(NULL, "nel_malloc error \n");
					free_prior_node(prior, type);
					return NULL;
				}

				for (i=0; i<prior->varnum; args=args->next, i++) {
					prior->varlist[i] = args->expr;
				}
			}	
			if (add_func_exp(prior, id, expr, args) == -1) {
				free_prior_node(prior, type);
				return NULL;
			}
		}
		break;
	
	case SIMP_RELATION:
		{

			prior->sbj = expr->binary.left;
			if (add_relate_exp(prior, id, expr) == -1) {
				free_prior_node(prior, type);
				return NULL;
			}
		}
		break;

	case COMP_RELATION:
		{
			struct exp_node *nexp;
			int_list_t *id_list;

			prior->numcount++;
			id_list = new_int_list(id, NULL);	
			if (!id_list) {
				free_prior_node(prior, type);
				return NULL;
			}
			nexp = new_exp_node(id_list);
			if (!nexp) {
				free_prior_node(prior, type);
				return NULL;
			}
			nexp->expr = expr;
			nexp->next = nexp->prev = NULL;
			prior->exp = prior->last_exp = nexp;
		}
		break;

	default:
		break;
	}

	return prior;
}


/*****************************************************************
 * Function: addto_priority (NODE *node, int id, PRIORTYPE type,
 *							struct prior_node **priority)
 *
 * Purpose: Add a Syntax sub-Tree into a given struct prior_node link list. 
 * 		
 * Arguments: node => root of the Syntax sub-Tree 
 *			  id=> logic id for the whole sub-Tree
 *			  type => RELATION, FUNCTION, PATMATCH
 *			  priority => the given struct prior_node link list
 *			  idx => the max idx of the variable
 *
 * Returns: 0 => succeed 	-1 => fail 
 *****************************************************************/
int addto_priority (	struct nel_eng *eng, 
			nel_expr *expr, 
			int id, 
			PRIORTYPE type, 
			struct prior_node **priority)
{
	struct prior_node *prior;
	nel_expr_list *args;
	int found=0, i;

	if (!expr || !priority) {
		gen_error(NULL, "input error \n");
		return -1;
	}

	if (type < SIMP_RELATION  || type > CLUS_FUNCTION) {
		gen_error(NULL, "input error \n");
		return -1;
	}

	switch (type) {
	case SIMP_RELATION:
		prior = priority[type];
		while (prior) {	
			if (!nel_expr_diff(expr->binary.left, prior->sbj)) {
				if (add_relate_exp(prior, id, expr) == -1)
					return -1;
				found = 1;
				break;
			} 
			prior = prior->next;
		}
		break;

	case COMP_RELATION:
		/* 
		  skip process of compilicated expression,though it is an 
		  interesting subject.  but it is far beyond our prespective 
		  So what we do now, is to add the expr directly into the 
		  'priority[COMP_RELATION]', which's gonna be done after this 
		  'switch()'.
		*/ 
		break;

	case SIMP_FUNCTION:
		prior = priority[type];
		while (prior) {
			if (!nel_expr_diff(expr->funcall.func, prior->sbj)) {
				if (add_func_exp(prior, id, expr, expr->funcall.args) == -1)
					return -1;
				found = 1;
				break;
			}
			prior = prior->next;
		}
		break;

	case CLUS_FUNCTION:
		prior = priority[type];
		while (prior) {	
			if (!nel_expr_diff(expr->funcall.func, prior->sbj)) {
				for (args=expr->funcall.args, i=0; i<prior->varnum;
						args=args->next,i++) {
					if (nel_expr_diff(args->expr, prior->varlist[i]))
						break;
				}	
				if (i == prior->varnum)	{ /* All variables the same */
					if (add_func_exp(prior, id, expr, args) == -1)
						return -1;
					found = 1;
					break;
				}
			}
			prior = prior->next;
		}
		break;

	default:
		break;

	}

	if (!found) {
		prior = priority[type];
		priority[type] = new_prior_node(eng, expr, id, type);
		if (!priority[type])
			return -1;
		priority[type]->next = prior;
	}

	return 0;
}

/*  all expression are complicated expression */
int is_simple_expr(struct prior_node **priority, nel_expr *expr)
{
	return 0;
}

/******************************************************************************/
/* int init_priority(nel_expr *expr, int id, struct prior_node **priority)    */
/* Purpose: Initiate the 'priority' according to the input Syntax sub-Tree.   */
/* Arguments: node => the root of the Syntax sub-Tree			      */
/*	  priority => the initialized 'struct prior_node' list struct	      */
/* Returns: 0 => succeed 	-1 => fail 				      */
/******************************************************************************/
int init_priority(	struct nel_eng *eng, 
			nel_expr *expr, 
			int id, 
			struct prior_node **priority)
{
	nel_symbol *func;

	if (!expr)
		return -1;

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
			/* I may split NE to a LT and GT */
			if (addto_priority(eng, expr, id, COMP_RELATION, priority) == -1)
				return -1;
			break;

		case nel_O_EQ:
		case nel_O_GE:
		case nel_O_GT:
		case nel_O_LE:
		case nel_O_LT:
			if(is_simple_expr(priority, expr)) {
				if (addto_priority(eng, expr, id, SIMP_RELATION, priority) == -1)
					return -1;
			} else {
				if (addto_priority(eng, expr, id, COMP_RELATION, priority) == -1)
					return -1;
			}
			break;

		case nel_O_COMPOUND:
			if (init_priority(eng, expr->binary.left, id, priority) == -1)
				return -1;
			if (init_priority(eng, expr->binary.right, id, priority) == -1)
				return -1;
			break;

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
			/* NOTE, NOTE, NOTE, check this is an SIMP_FUN, OR CLUS_FUN */
			func = nel_lookup_symbol(expr->funcall.func->symbol.symbol->name, 
				eng->nel_static_ident_hash, eng->nel_static_location_hash, NULL);
			if(!func)
				break;
			
			if(func->type->function.prev_hander ){
				if (addto_priority(eng, expr, id, CLUS_FUNCTION, priority) == -1)
					return -1;
			} else {
				if (addto_priority(eng, expr, id, SIMP_FUNCTION, priority) == -1)
					return -1;
			}
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



/*****************************************************************
 * Function: new_pred_node(char *predname, struct pred_node *next)
 *
 * Purpose: Create a new struct pred_node. 
 * 		
 * Arguments: predname => name of the predicate
 *			  next => point to the next struct pred_node in the list
 *
 * Returns: pred => pointer of the new created struct pred_node 
 *****************************************************************/
struct pred_node *new_pred_node(char *predname)
{
	struct pred_node *pred;
	nel_calloc(pred, 1, struct pred_node);
	if (!pred) {
		gen_error(NULL, "nel_malloc error \n");
		return NULL;
	}

	pred->predname = predname; 	/* no malloc! */
	return pred;
}

void free_pred_node(struct pred_node *predicate)
{
	struct pred_node *pred, *pred_next;
	int i;
	pred = predicate;
	while (pred) {
		pred_next = pred->next;
		
		free_int_list(pred->symbol_id_list);	
		free_int_list(pred->prior_id_list);	
		//free_priority(pred->priority);
		for (i=0; i<PRIORNUM; i++) {
			if (pred->priority[i]) {
				free_prior_node(pred->priority[i], i);
			}
		}
		nel_free(pred);	

		pred = pred_next;
	}
	return;
}

/*****************************************************************
 * Function: int init_predicate(struct nel_LIST *sym_list,
 *                              struct pred_node **predicate,
 *                              nel_symbol **SymbolID )
 * Purpose: to initiate the predicate according to the input Syntax
 *			Tree.
 * 		
 * Arguments: sym_list => the root of the Syntax Tree
 *			  predicate => the initialized struct pred_node struct
 *
 * Returns: 0 => succeed 	-1 => fail 
 *****************************************************************/
int init_predicate(	struct nel_eng *eng, 
			struct nel_LIST *sym_list,
			struct pred_node **predicate, 
			nel_symbol **SymbolID )
{
	struct pred_node *pred, *prev_pred=NULL;
	struct nel_LIST *psym_list;
	nel_symbol *psym;
	int found = 0;
	int id=-1;

	if (!sym_list) {
		gen_error(NULL,"input error \n");
		return -1;
	}		
	psym_list = sym_list;
	while (psym_list != NULL) {
		if ((psym = psym_list->symbol) != NULL) {
			SymbolID[++id] = psym;
			pred = *predicate;

			found = 0;
			while (pred) {
				if(psym->_pid == pred->predid){
					found = 1;
					break;
				}
				pred = pred->next;
			}

			if (!found) {
				/* 
				  OK, since we have not found pred of this symbol, then create 
				  a pred for this symbol, and link this to the others 
				*/
				pred = new_pred_node(psym->name);
				if (!pred) {
					if (predicate)
						free_pred_node(*predicate);
					return -1;
				}

				pred->predid = psym->_pid;
	
				/* link all pred together */
				if(!*predicate)
					*predicate = pred;
				if(prev_pred)
					prev_pred->next = pred;
				prev_pred = pred;
			}

			/* do initiation work */
			if (psym->id != psym->_pid) {
				insert_int_list(&pred->prior_id_list, id);
				insert_int_list(&pred->symbol_id_list, psym->id);
				if (init_priority(eng, (nel_expr *)psym->value,id,pred->priority)==-1){
					if (predicate)
						free_pred_node(*predicate);
					return -1;
				}	

			} else {
				pred->flag = 1;
			}		

		} /* No Symbols followed */ 
		psym_list = psym_list->next;
		
	} /* finished the symbol_list */

	return 0;
}


/******************************************************************************/
/* new_ture_id_list(struct prior_node *prinode, int_list_t *id_list,	      */
/*					int_list_t **new_id_list)	      */
/* Purpose: Check a given struct prior_node to see if any of the given logic  */
/*	id_list are All-TRUE and fill them into a new id chain.		      */
/* Arguments: prinode => the given struct prior_node			      */
/*			  id_list => the given logic id chain		      */
/*			  new_id_list => new created id chain		      */
/* Returns: 0 => succeed 	-1 => fail 				      */
/******************************************************************************/
int new_true_id_list(	struct prior_node *prinode, 
			int_list_t *id_list, 
			int_list_t **new_id_list)
{
	int_list_t *pid1, *pid2, 
		*prev_pid2 = NULL;
	struct exp_node *exp;
	boolean value=T;
	int count=0;

	if (!prinode || !id_list) {
		gen_error(NULL,"input error \n");
		return -1;
	}

	pid1 = id_list;	
	while (pid1 != NULL) {
		value = T;
		for(exp=prinode->exp; exp; exp=exp->next){
			if(check_int_list(exp->id_list, pid1->val) == 0)
				value = value && exp->val;
			if (!value)
				break;
		}
		if(!exp){
			count++;
			pid2 = new_int_list(pid1->val, NULL);
			if(!pid2){
				free_int_list(*new_id_list);
				return -1;
			}
			if(!*new_id_list)
				*new_id_list = pid2;
			if(prev_pid2)
				prev_pid2->next = pid2;
			prev_pid2 = pid2;
		}
		pid1 = pid1->next;
	}

	return count;
}


int reset_split_count(struct prior_node *prior)
{
	struct exp_node *exp_scan;
	if(prior){
		prior->splitcount = 0;
		exp_scan = prior->exp;
		while(exp_scan) {
			if(exp_scan->in_use_flag == 1)
				prior->splitcount ++;
			exp_scan = exp_scan->next;
		}
	}

	return 0;
}


int reset_in_use_flags(struct prior_node **priority, int_list_t *in_use_id_list)
{
	struct prior_node *prior;
	struct exp_node *exp;
	int_list_t *pid;
	int i;
	for (i=0; i<PRIORNUM; i++) {
		prior = priority[i];
		while (prior) {
			prior->splitcount = 0;
			prior->in_use_flag = 0;
			exp = prior->exp;
			while (exp) {
				/* initiate first */
				exp->in_use_flag = 0;

				if(exp->val == T ){	
					//if TRUE, needn't consieder 
					//it anymore 
					goto next_exp;
				}

				pid = exp->id_list;
				while (pid) {
					if (check_int_list(in_use_id_list, 
							pid->val) == 0) {
						prior->splitcount ++;	
					}

					pid = pid->next;

				}
next_exp:
				exp = exp->next;
			}

			prior = prior->next;
		}

	}

	return 0;
}

int do_relation_optimize(struct nel_eng *eng, 
			int_list_t *id_list, 
			struct prior_node *pnode, 
			struct prior_node **priority, 
			void *SymbolID, 
			nel_symbol *result,  
			nel_stmt **stmt_head, 
			nel_stmt **stmt_tail)
{

	nel_stmt *true_stmt_head=NULL, 
		*true_stmt_tail=NULL;
	nel_stmt *other_stmt_head=NULL, 
		*other_stmt_tail=NULL;
	nel_stmt *stmt=NULL, 
		*prev_stmt=NULL;
	nel_stmt *end_stmt=NULL, 
		*prev_end_stmt=NULL;
	struct exp_node *split = NULL;
	struct exp_node *exp;

	int_list_t *pid=NULL, *tmppid=NULL;
	int count = -1, flag=-1;	
	
	for (exp= pnode->exp; exp; exp=exp->next) {
		pid = exp->id_list;
		while(pid){ 
			if (check_int_list(id_list, pid->val) == 0) {
				break;
			}
			pid = pid->next;
		}

		if(pid == NULL)
			continue;

		stmt = nel_stmt_alloc(eng, 
				nel_S_BRANCH, /* stmt type */
					"", 	/* filename */
				0, 	/* line num */
				1,	/* level */
					exp->expr,
				NULL,	/* true stmt */
					NULL);	/* false stmt */ 
		if (stmt == NULL) 
			goto Error;

		if(!*stmt_head)
			*stmt_head = stmt;

		/*** When 'exp[split]' is supposed to be 'T' ***/
		exp->val = T;
	

		true_stmt_head = true_stmt_tail = NULL;
		if (do_optimize(eng, priority, 
			count_int_list(id_list), 
			id_list, 
			SymbolID, result, 
			&true_stmt_head, 
			&true_stmt_tail) == -1) {
			goto Error;
		}

		//nel_stmt_link(stmt, true_stmt_head);
		stmt->branch.true_branch = true_stmt_head;

		/* link new created 'stmt' with previous one */	
		if (prev_stmt)
			nel_stmt_link(prev_stmt, stmt);
	
		exp->val = F;
		prev_stmt = stmt;
		*stmt_tail = stmt;
	}


	return 0;
Error:
	if (*stmt_head)
		nel_stmt_dealloc(*stmt_head);
	*stmt_head = *stmt_tail = NULL;
	return -1;
}


int do_simp_function_optimize(	struct nel_eng *eng, 
				int_list_t *id_list, 
				struct prior_node *pnode, 
				struct prior_node **priority, 
				int type, 
				void *SymbolID, 
				nel_symbol *result, 
				nel_stmt **stmt_head, 
				nel_stmt **stmt_tail)
{
	nel_stmt *true_stmt_head = NULL, 
		     *true_stmt_tail = NULL;
	nel_stmt *stmt, *end_stmt;
	nel_stmt *prev_stmt = NULL,
		     *prev_end_stmt = NULL;

	struct exp_node *exp;
	int count;
	int flag = 0;

	for (exp=pnode->exp; exp; exp=exp->next) {
		int_list_t *pid = exp->id_list;
			while(pid){ 
				if (check_int_list(id_list, pid->val) == 0) {
					break;
				}
				pid = pid->next;
			}
	
			if(pid == NULL)
				continue;

		stmt = nel_stmt_alloc(	eng, 
					nel_S_BRANCH, /* stmt type */ 
					"", 	/* filename */
					0, 	/* line num */
					1, 	/* level */
					exp->expr,
					NULL,	/* true stmt */
					NULL);	/* false stmt */ 
		if (stmt == NULL) {
			if (*stmt_head)
				nel_stmt_dealloc(*stmt_head);
			return -1;
		}

		if (!*stmt_head) {
			*stmt_head = stmt;
		}

		exp->val = T;	/* turn on */
		true_stmt_head = true_stmt_tail = NULL;
		if (do_optimize(eng, 
			priority, 
			count_int_list(id_list), 
			id_list, 
				SymbolID, result, &true_stmt_head, 
			&true_stmt_tail) == -1) {
			if (*stmt_head)
				nel_stmt_dealloc(*stmt_head);
			return -1;
		}

		stmt->branch.true_branch = true_stmt_head;

		/* link new created stmts with previous one */	
		if(prev_stmt)
			nel_stmt_link(prev_stmt, stmt) ;
	
		exp->val = F;
		prev_stmt = stmt;
	}

	*stmt_tail = prev_stmt;
	return 0;
}

int do_clus_function_optimize(	struct nel_eng *eng, 
				int_list_t *id_list, 
				struct prior_node *pnode, 
				struct prior_node **priority, 
				int type, 
				void *SymbolID, 
				nel_symbol *result, 
				nel_stmt **stmt_head, 
				nel_stmt **stmt_tail)
{
	struct exp_node *exp;
	int_list_t *pid, *npid;
	int count;
	int flag = 0;
	nel_stmt *true_stmt_head = NULL, 
		      *true_stmt_tail = NULL;

	nel_stmt *stmt, 
	              *end_stmt;
	nel_stmt *head_stmt;

	nel_stmt *prev_stmt = NULL;

	nel_symbol *func_sym, *prev_func_sym,
	                      *post_func_sym;

	prev_func prev_hander;
	post_func post_hander;
	nel_expr *expr;

	flag = 1;
	if (!pnode->sbj || !(func_sym = pnode->sbj->symbol.symbol)) {
		gen_error(eng, "input error \n");
		return -1;
	}
	
	if ( !(prev_func_sym = (nel_symbol *)func_sym->type->function.prev_hander)) {
		gen_error(NULL, "no pre_func found \n");
		return -1;
	}

	prev_hander = (prev_func)prev_func_sym->value;
	expr = (*prev_hander)(eng, pnode);

	stmt = nel_stmt_alloc(eng, nel_S_BRANCH,/* stmt type */ 
					"", 	/* filename */
					0, 	/* line num */
					1, 	/* level */
					expr,
					NULL,	/* true stmt */
					NULL);	/* false stmt */ 
	if (!stmt) 
		return -1;

	head_stmt = stmt; 

	*stmt_head = head_stmt;
	for (exp=pnode->exp; exp; exp=exp->next) {
		exp->val = T;	/* turn on */
		for (pid=exp->id_list; pid; pid=pid->next) {

			if (check_int_list(id_list, pid->val) != 0) {
				continue;
			}

			npid =  new_int_list(pid->val, NULL);
			if ( !(post_func_sym = (nel_symbol *)func_sym->type->function.post_hander)){
				gen_error(NULL,"No post_func found.\n");
				return -1;
			}

			post_hander = (post_func)post_func_sym->value;
			expr = (*post_hander)(eng, pnode, pid->val);
			
			stmt = nel_stmt_alloc(	eng, 
						nel_S_BRANCH,/* stmt type */ 
						"", 	/* filename */
						0, 	/* line num */
						2,	/* level */
						expr,
						NULL,	/* true stmt */
						NULL);	/* false stmt */ 
			if (stmt == NULL) { 
				if (*stmt_head)
					nel_stmt_dealloc(*stmt_head);
				return -1;
			}

			true_stmt_head = true_stmt_tail = NULL;
			if (do_optimize(eng, 
				priority, 
				1, 	
				npid,	
					SymbolID, result,
					&true_stmt_head, 
				&true_stmt_tail) == -1) {
				if (*stmt_head)
					nel_stmt_dealloc(*stmt_head);
				nel_stmt_dealloc(stmt);
				return -1;
			}

			stmt->branch.true_branch = true_stmt_head;

			/* link new created 'stmt' with previous one */	
			if(head_stmt->branch.true_branch == NULL)
				head_stmt->branch.true_branch = stmt;
			if(prev_stmt)
				nel_stmt_link(prev_stmt, stmt);
			prev_stmt = stmt;

		}

		exp->val = F;

	}

	*stmt_tail = head_stmt;
	return 0;
}



/*****************************************************************
 * Function: int do_optimize (struct nel_eng *eng,
 *                            struct prior_node **priority,
 *                            int idcount,
 *                            int_list_t *in_use_id_list,
 *                            void *SymbolID,
 *                            nel_symbol *result,
 *                            nel_stmt **stmt_head,
 *                            nel_stmt **stmt_tail)
 *
 * Purpose: Organize and optimize the input predicate expressions
 *			into an efficient Decision Tree.
 * 		
 * Arguments:
 *   priority => a struct which's cumulated the predicate
 *               expressions according to the running
 *               efficience, which supply the information
 *               for the whole optimizing process.
 *   idcount => the count of logic id_list for the 'pid'
 *              followed in the argument list.
 *   in_use_id_list =>
 *   SymbolID =>
 *   result =>
 *   stmt_head =>
 *   stmt_tail =>
 *
 * Returns:
 *   = 0  =>
 *   < 0  =>
 *
 * Note, this is a Reccursive Funcition.
 *****************************************************************/
int do_optimize(		struct nel_eng *eng, 
				struct prior_node **priority, 
				int idcount, 
				int_list_t *in_use_id_list, 
				void *SymbolID, 
				nel_symbol *result,
				nel_stmt **stmt_head, 
				nel_stmt **stmt_tail)
{
	nel_stmt *stmt=NULL, *end_stmt = NULL;
	nel_stmt *sub_stmt_head=NULL, *sub_stmt_tail=NULL;

	int i;
	int maxindex = -1, maxsplit = 0;
	boolean value = T;
	
	struct prior_node *prior, *max_prior_node = NULL;
	struct exp_node *exp, *head, *tail;
	int_list_t *pid, *copy_in_use_id_list;
	int_list_t *prior_id_list;

	copy_in_use_id_list = dup_int_list(in_use_id_list);
	if (!copy_in_use_id_list)
		return -1;


	/* Whether we reach the leaf of the Decision-tree */
	pid = copy_in_use_id_list;
	while (pid) {
		i = 0;
		value = T;
		while (value && i<PRIORNUM) {
			if (priority[i] == NULL) {
				++i;
				continue;
			}
					
			prior = priority[i];
			while (prior != NULL)	{
				exp = prior->exp;
				while (value && exp) {
					if (check_int_list(exp->id_list, pid->val) == 0) {
						value = (value && exp->val);
					}
					exp = exp->next;
				}
				prior = prior->next;
			}
			++i;
		}
			
		if (value) {	
			/*found a symbol that we need to output to "result" */	
			stmt = class_output_stmt_alloc(eng, result, ((nel_symbol **)SymbolID)[pid->val] );
			if (!stmt) {
				if (*stmt_head)
					nel_stmt_dealloc(*stmt_head);
				return -1;
			}
				
			/* link the stmt to proper location */	
			if(!*stmt_head)
				*stmt_head = stmt;
			if(end_stmt)
				nel_stmt_link(end_stmt, stmt);	
			end_stmt = stmt;

			/* delete id from pid */
			copy_in_use_id_list = remove_int_list(copy_in_use_id_list, pid->val);

		}
		pid = pid->next;
	}


	while (copy_in_use_id_list) {

		maxsplit = 0;
		max_prior_node = NULL;

		reset_in_use_flags(priority, copy_in_use_id_list);

		/* there are still some logic ID that has not been optimized 
		   Select the most prior 'struct prior_node' with the biggest 'splitcount' */
		for (i=0; i<PRIORNUM; i++) {
			prior = priority[i];
			while (prior) {
				if(prior->splitcount > maxsplit) {
					maxsplit = prior->splitcount;
					max_prior_node = prior;
				}
				prior = prior->next;	
			}

			if (maxsplit > 0) {	 
				/* Do have choice in this Prior */
				maxindex = i;
				break;
			}
		}

		if(max_prior_node == NULL) {
			return 0;
		}

		prior_id_list = NULL;
		for(exp=max_prior_node->exp;  exp; exp=exp->next){
			pid = exp->id_list;
			while(pid){
				if (check_int_list(copy_in_use_id_list, pid->val) == 0) {
					if(prior_id_list)
						insert_int_list(&prior_id_list, pid->val);						
					else 
						prior_id_list = new_int_list(pid->val, NULL); 
				}
				pid = pid->next;
			}
		}	

		/* Begin the Classification NOW */
		switch (maxindex) {
			case SIMP_RELATION:
			case COMP_RELATION:
				sub_stmt_head = sub_stmt_tail = NULL;
				if (do_relation_optimize(eng, 
					/*head, tail, copy_in_use_id_list */ 
						prior_id_list,  max_prior_node, priority, SymbolID, 
						result, &sub_stmt_head, &sub_stmt_tail) == -1) {
					if (*stmt_head)
						nel_stmt_dealloc(*stmt_head);
					return -1;
				}
				break;
			
			case SIMP_FUNCTION:
				sub_stmt_head = sub_stmt_tail = NULL;
				if (do_simp_function_optimize(eng, prior_id_list, max_prior_node, 
						priority, maxindex, SymbolID, result, 
						&sub_stmt_head, &sub_stmt_tail) == -1) {
					if (*stmt_head)
						nel_stmt_dealloc(*stmt_head);
					return -1;
				}
				break;

			case CLUS_FUNCTION:
				sub_stmt_head = sub_stmt_tail = NULL;
				if (do_clus_function_optimize(eng, prior_id_list, 
						max_prior_node, priority, maxindex, SymbolID, 
						result, &sub_stmt_head, &sub_stmt_tail) == -1) {
					if (*stmt_head)
						nel_stmt_dealloc(*stmt_head);
					return -1;
				}
				break;
		
			default:
				gen_error(NULL,"Illegal PRIORTYPE in 'do_optimize()'.\n");
				break;
		}

		/* delete all id (that in max_prior_node) from copy_in_use_id_list */
		for(exp=max_prior_node->exp;  exp; exp=exp->next){
			pid = exp->id_list;
			while(pid){
				copy_in_use_id_list = remove_int_list(copy_in_use_id_list, pid->val);
				pid = pid->next;
			}
		}	

		if(!*stmt_head)
			*stmt_head = sub_stmt_head;
		if(end_stmt)
			nel_stmt_link(end_stmt, sub_stmt_head);
		end_stmt = sub_stmt_tail;

	}

	if(end_stmt)
		*stmt_tail = end_stmt;
	return 0;
}


int optimize_class_func_stmt_alloc(	struct nel_eng *eng, 
					struct nel_LIST *sym_list, 
					int count, 
					nel_symbol *result, 
					nel_stmt **stmt_head, 
					nel_stmt **stmt_tail)
{
	nel_symbol **symbol_ids;
	struct pred_node *pred=NULL;
	int id_count;

	nel_calloc(symbol_ids, count, nel_symbol*);
	if(!symbol_ids)
		goto Error;

	if (init_predicate(eng, sym_list, &pred, symbol_ids) == -1) {
		goto Error;
	}
	
	id_count = count_int_list(pred->prior_id_list);
	if (do_optimize(eng, pred->priority, id_count, pred->prior_id_list,
		symbol_ids, result, stmt_head, stmt_tail) == -1) {
		goto Error;
	}

	//notsure whether should free the following two.
	//free(SymbolID);
	//free_pred_node(pred);
	return 0;

Error:
	if (symbol_ids)
		nel_free(symbol_ids);	
	free_pred_node(pred);

	return -1;
}
