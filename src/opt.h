/*
 * optimize.h
 * $Id: opt.h,v 1.3 2006/05/25 06:18:24 wyong Exp $
 */

#ifndef __OPTIMIZE_H
#define __OPTIMIZE_H

#include "intlist.h"
#include "sym.h"

/********************** DEFINE **********************/
//#define PRIORNUM	4	/* RELATION, FUNC_HIGH, FUNC_LOW, PATMATCH */


/******************* DATA STRUCT *********************/

/*** For the whole Classification ***/
typedef enum { F=0, T=1 } boolean; 

typedef enum { 
	SIMP_RELATION,
	COMP_RELATION,
	SIMP_FUNCTION,
	CLUS_FUNCTION, 	
	PRIORNUM
} PRIORTYPE;

struct exp_node {
	int_list_t	*id_list;
	int		in_use_flag;
	nel_O_token op;

	union {
		int	num;
		char	*pat;
		nel_expr **arglist;
	} opr;		

	boolean	val;
	nel_expr *expr;	/* I needn't re compute the expr, 
				   not for optimize logic, wyong  */
	struct exp_node *prev;
	struct exp_node *next;
};

struct prior_node {

	nel_expr 	*sbj;
	char		*data;	
	
	/* for function */
	int		varnum;
	nel_expr 	**varlist;
	
	int		splited_flag;
	int		in_use_flag;

	int		splitcount;
	int		numcount;
	struct exp_node		*exp;
	struct exp_node		*last_exp;

	struct prior_node *next;
};

struct pred_node {
	char *predname;
	int predid;
	int flag;	/* whether to return "predid" or not */
	int_list_t *symbol_id_list;
	int_list_t *prior_id_list;
	struct prior_node *priority[PRIORNUM];
	struct pred_node *next;
};


union nel_EXPR;
union nel_TASK_REC;
typedef union nel_EXPR *(* prev_func)(struct nel_eng *eng, struct prior_node *pnode);
typedef union nel_EXPR *(* post_func)(struct nel_eng *eng, struct prior_node *pnode, int id);

extern int optimize_symbol_list(struct nel_eng *eng, struct nel_LIST *sym_list, int count, nel_symbol *result,  nel_stmt **stmt_head, nel_stmt **stmt_tail);

int do_optimize(struct nel_eng *eng, struct prior_node **priority, int idcount,
int_list_t *in_use_id_list, void *SymbolID, nel_symbol *result,nel_stmt **stmt_head, nel_stmt **stmt_tail);



#endif
