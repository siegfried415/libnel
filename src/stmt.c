
/*****************************************************************************/
/* This file, "stmt.c", contains the parse tree evaluator for the    */
/* application executive.                                                    */
/*****************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include <setjmp.h>
#include <string.h>

#include <engine.h>
#include <errors.h>
#include <io.h>
#include <type.h>
#include <expr.h>
#include <stmt.h>
#include "intp.h"
#include <lex.h>
#include "mem.h"


#define tab(_indent); \
        {                                                               \
           register int __indent = (_indent);                           \
           int __i;                                                     \
           for (__i = 0; (__i < __indent); __i++) {                     \
              fprintf (file, "   ");                                    \
           }                                                            \
        }


/*****************************************************************************/
/* nel_S_name () returns the string that is the identifier for <code>, a */
/* nel_S_token representing an statment type.                            */
/*****************************************************************************/
char *nel_S_name (register nel_S_token code)
{
	switch (code) {
	case nel_S_NULL:		return ("nel_S_NULL");
	case nel_S_DEC:		return ("nel_S_DEC");
	case nel_S_EXPR:		return ("nel_S_EXPR");
	case nel_S_RETURN:		return ("nel_S_RETURN");
	case nel_S_BRANCH:		return ("nel_S_BRANCH");
	case nel_S_WHILE:		return ("nel_S_WHILE");
	case nel_S_FOR:		return ("nel_S_FOR");
	case nel_S_TARGET:		return ("nel_S_TARGET");
	case nel_S_MAX:		return ("nel_S_MAX");
	case nel_S_GOTO:	return ("nel_S_GOTO");
	case nel_S_BREAK:	return ("nel_S_BREAK");
	case nel_S_CONTINUE:	return ("nel_S_CONTINUE");
	default:			return ("nel_S_BAD_TOKEN");
	}
}


/*********************************************************/
/* declarations for the nel_stmt structure allocator */
/*********************************************************/
unsigned_int nel_stmts_max = 0x1000;
nel_lock_type nel_stmts_lock = nel_unlocked_state;
static nel_stmt *nel_stmts_next = NULL;
static nel_stmt *nel_stmts_end = NULL;
static nel_stmt *nel_free_stmts = NULL;

/***************************************************************/
/* make a linked list of nel_stmt_chunk structures so that */
/* we may find the start of all chunks of memory allocated     */
/* to hold nel_stmt structures at garbage collection time. */
/***************************************************************/
static nel_stmt_chunk *nel_stmt_chunks = NULL;


/*****************************************************************************/
/* nel_stmt_alloc allocates space for an nel_stmt structure,         */
/* initializes its fields, and returns a pointer to the structure.           */
/* filename should reside in nel's name table - it is not automatically       */
/* copied there.                                                             */
/*****************************************************************************/
nel_stmt *nel_stmt_alloc (struct nel_eng *eng, register nel_S_token type, register char *filename, int line,  va_alist)
{
	va_list args;			/* scans arguments after size	*/
	register nel_stmt *retval;	/* return value			*/

	nel_debug ({ nel_trace (eng, "entering nel_stmt_alloc [\ntype = %J\nfilename = %s\nline = %d\n\n", type, filename, line); });

	va_start (args, line);

	/*******************************************/
	/* static allocators are critical sections */
	/*******************************************/
	nel_lock (&nel_stmts_lock);

	if (nel_free_stmts != NULL) {
		/*****************************************************************/
		/* first, try to re-use deallocated type nel_stmt structures */
		/*****************************************************************/
		retval = nel_free_stmts;
		nel_free_stmts = (nel_stmt *) nel_free_stmts->gen.next;
	}
	else {
		/***************************************************************/
		/* check for overflow of space allocated for nel_stmt      */
		/* structures.  on overflow, allocate another chunk of memory. */
		/***************************************************************/
		if (nel_stmts_next >= nel_stmts_end) {
			register nel_stmt_chunk *chunk;

			nel_debug ({ nel_trace (eng, "nel_stmt structure overflow - allocating more space\n\n"); });
			nel_malloc (nel_stmts_next, nel_stmts_max, nel_stmt);
			nel_stmts_end = nel_stmts_next + nel_stmts_max;

			/*************************************************/
			/* make an entry for this chunk of memory in the */
			/* list of nel_stmt_chunks.                  */
			/*************************************************/
			nel_malloc (chunk, 1, nel_stmt_chunk);
			chunk->start = nel_stmts_next;
			chunk->size = nel_stmts_max;
			chunk->next = nel_stmt_chunks;
			nel_stmt_chunks = chunk;
		}
		retval = nel_stmts_next++;
	}

	/*************************/
	/* exit critical section */
	/*************************/
	nel_unlock (&nel_stmts_lock);

	/****************************************/
	/* initialize the appropriate members.  */
	/****************************************/
	retval->gen.type = type;
	retval->gen.filename = filename;
	retval->gen.line = line;
	retval->gen.visited = 0;

	//added by zhangbin, 2006-6-13
	retval->gen.compiled=0;
	retval->gen.addr=0;
	retval->gen.patch_addr=0;
	//end

	switch (type) {

	/********************/
	/* declaration stmt */
	/********************/
	case nel_S_DEC:
		retval->dec.symbol = va_arg (args, nel_symbol*);
		retval->dec.next = va_arg (args, nel_stmt *);
		break;

	/*******************/
	/* expression stmt */
	/*******************/
	case nel_S_EXPR:
		retval->expr.expr = va_arg (args, nel_expr *);
		retval->expr.next = va_arg (args, nel_stmt *);
		break;

	/***************/
	/* return stmt */
	/***************/
	case nel_S_RETURN:
		retval->ret.retval = va_arg (args, nel_expr *);
		retval->ret.next = NULL;
		break;

	/**********************/
	/* conditional branch */
	/**********************/
	case nel_S_BRANCH:
		retval->branch.level = va_arg (args, int);
		retval->branch.cond = va_arg (args, nel_expr *);
		retval->branch.true_branch = va_arg (args, nel_stmt *);
		retval->branch.false_branch = va_arg (args, nel_stmt *);
		//added by zhangbin, 2006-5-20
		retval->branch.next = NULL;
		//end
		break;

	//added by zhangbin, 2006-5-20
	/***********************/
	/*      goto stmt      */
	/***********************/
	case nel_S_GOTO:
	case nel_S_BREAK:
	case nel_S_CONTINUE:
		retval->goto_stmt.next = NULL;
		retval->goto_stmt.goto_target = va_arg(args, nel_stmt*);
		break;
	//end

	case nel_S_WHILE:
		retval->while_stmt.level = va_arg (args, int);
		retval->while_stmt.cond = va_arg (args, nel_expr *);
		retval->while_stmt.body = va_arg (args, nel_stmt *);
		//added by zhangbin, 2006-6-15
		retval->while_stmt.break_target = NULL;
		retval->while_stmt.continue_target = NULL;
		//end
		break;

	case nel_S_FOR:
		retval->for_stmt.level = va_arg (args, int);
		retval->for_stmt.init= va_arg (args, nel_expr *);
		retval->for_stmt.cond = va_arg (args, nel_expr *);
		retval->for_stmt.inc = va_arg (args, nel_expr *);
		retval->for_stmt.body = va_arg (args, nel_stmt *);
		//added by zhangbin, 2006-6-15
		retval->for_stmt.break_target = NULL;
		retval->for_stmt.continue_target = NULL;
		//end
		break;

	/**********/
	/* target */
	/**********/
	case nel_S_TARGET:
		retval->target.level = va_arg (args, int);
		retval->target.next = va_arg (args, nel_stmt *);
		break;

	default:
		break;
	}

	va_end (args);
	nel_debug ({ nel_trace (eng, "] exiting nel_stmt_alloc\nretval =\n%1K\n", retval); });

	return (retval);
}


/*****************************************************************************/
/* nel_stmt_dealloc returns the nel_stmt structure pointed to by     */
/* <stmt> back to the free list (nel_free_stmts), so that the space may  */
/* be re-used.                                                               */
/*****************************************************************************/
void nel_stmt_dealloc (register nel_stmt *stmt)
{
	nel_debug ({
	if (stmt == NULL) {
		//nel_fatal_error (NULL, "(nel_stmt_dealloc #1): stmt == NULL");
	}
	});

	/*******************************************/
	/* static allocators are critical sections */
	/*******************************************/
	nel_lock (&nel_stmts_lock);

	stmt->gen.next = nel_free_stmts;
	nel_free_stmts = stmt;

	/*************************/
	/* exit critical section */
	/*************************/
	nel_unlock (&nel_stmts_lock);

}


nel_stmt *nel_stmt_tail(nel_stmt *head)
{
	nel_stmt *prev = NULL;
	nel_stmt *stmt = head;	

	while (stmt){
		prev = stmt;
		switch(stmt->gen.type) {
		case nel_S_BRANCH:
			stmt = stmt->branch.next ;
			break;

		case nel_S_WHILE:
			stmt = stmt->while_stmt.next ;
			break;

		case nel_S_FOR:
			stmt = stmt->for_stmt.next;
			break;

		case nel_S_DEC:
			stmt = stmt->dec.next ;
			break;

		case nel_S_EXPR:
			stmt = stmt->expr.next ;
			break;

		case nel_S_TARGET:
			stmt = stmt->target.next ;
			break;

		case nel_S_RETURN:
			stmt = stmt->ret.next;
			break;

		default:
			return NULL;
		}
	}

	return prev;

}

int nel_stmt_link(nel_stmt * tail , nel_stmt *head)
{
	if(!tail )
		return -1;
	
	switch(tail->gen.type) {
	//added by zhangbin, 2006-5-20
	case nel_S_GOTO:
	case nel_S_BREAK:
	case nel_S_CONTINUE:
		tail->goto_stmt.next = head;
		break;
	//end
	case nel_S_BRANCH:
		tail->branch.next = head;
		break;

	case nel_S_WHILE:
		tail->while_stmt.next = head;
		break;

	case nel_S_FOR:
		tail->for_stmt.next = head;
		break;

	case nel_S_DEC:
		tail->dec.next = head;
		break;

	case nel_S_EXPR:
		tail->expr.next = head;
		break;

	case nel_S_TARGET:
		tail->target.next = head;
		break;

	default:
		return -1;
	}

	return 0;
}

/*****************************************************************************/
/* nel_print_stmt () prints out an nel_stmt structure in a pretty    */
/* format.                                                                   */
/*****************************************************************************/
void nel_print_stmt (FILE *file, register nel_stmt *stmt, int indent)
{
	if (stmt == NULL) {
		tab (indent);
		fprintf (file, "NULL_stmt\n");
	}
	else {
		tab (indent);
		fprintf (file, "0x%p\n", stmt);
		tab (indent);
		fprintf (file, "type: %s\n", nel_S_name (stmt->gen.type));
		tab (indent);
		fprintf (file, "filename: \"%s\"\n", (stmt->gen.filename == NULL) ? "" : stmt->gen.filename);
		tab (indent);
		fprintf (file, "line: %d\n", stmt->gen.line);
		switch (stmt->gen.type) {

		/*******************************/
		/* declaration of a new symbol */
		/*******************************/
		case nel_S_DEC:
			tab (indent);
			fprintf (file, "symbol:\n");
			nel_print_symbol (file, stmt->dec.symbol, indent + 1);
			tab (indent);
			fprintf (file, "next: 0x%p\n", stmt->dec.next);
			nel_print_stmt(file, stmt->dec.next, indent +1);
			break;

		/************************/
		/* expression statement */
		/************************/
		case nel_S_EXPR:
			tab (indent);
			fprintf (file, "expr:\n");
			nel_print_expr (file, stmt->expr.expr, indent + 1);
			tab (indent);
			fprintf (file, "next: 0x%p\n", stmt->expr.next);
			nel_print_stmt(file, stmt->expr.next, indent + 1);
			break;

		/********************/
		/* return statement */
		/********************/
		case nel_S_RETURN:
			tab (indent);
			fprintf (file, "retval:\n");
			nel_print_expr (file, stmt->ret.retval, indent + 1);
			break;

		/**********************/
		/* conditional branch */
		/**********************/
		case nel_S_BRANCH:
			tab (indent);
			fprintf (file, "cond:\n");
			nel_print_expr (file, stmt->branch.cond, indent + 1);
			tab (indent);
			//fprintf (file, "true_branch: 0x%x\n", stmt->branch.true_branch);
			nel_print_stmt(file, stmt->branch.true_branch, indent );
			tab (indent);
			//fprintf (file, "false_branch: 0x%x\n", stmt->branch.false_branch);
			nel_print_stmt(file, stmt->branch.false_branch, indent);
			break;

		case nel_S_WHILE:
			tab (indent);
			fprintf (file, "cond:\n");
			nel_print_expr (file, stmt->while_stmt.cond, indent + 1);
			tab (indent);
			nel_print_stmt(file, stmt->while_stmt.body, indent );
			break;

		case nel_S_FOR:
			tab (indent);
			fprintf (file, "init:\n");
			nel_print_expr(file, stmt->for_stmt.init,indent+1);

			tab (indent);
			fprintf (file, "cond:\n");
			nel_print_expr(file, stmt->for_stmt.cond,indent+1);

			tab (indent);
			fprintf (file, "inc:\n");
			nel_print_expr(file, stmt->for_stmt.inc,indent+1);

			tab (indent);
			fprintf (file, "body:\n");
			nel_print_stmt(file, stmt->for_stmt.body, indent );
			break;

		/**********/
		/* target */
		/**********/
		case nel_S_TARGET:
			tab (indent);
			fprintf (file, "level: %d\n", stmt->target.level);
			tab (indent);
			fprintf (file, "next: 0x%p\n", stmt->target.next);
			break;

		default:
			tab (indent);
			fprintf (file, "next: 0x%p\n", stmt->gen.next);
			break;

		}
	}
}

int evt_stmt_update_dollar(struct nel_eng *eng, nel_stmt *stmts, int pos, int value)
{
	nel_stmt *stmt = stmts;
	int retval = 0;

	while( stmt != NULL) 
	{
		switch (stmt->gen.type) {
		/*******************************/
		/* declaration of a new symbol */
		/*******************************/
		case nel_S_DEC:
			if ((retval = evt_symbol_update_dollar(eng, 
				stmt->dec.symbol, pos, value)) < 0 ){
				goto error;
			}

			stmt = stmt->dec.next;
			break;

		/************************/
		/* expression statement */
		/************************/
		case nel_S_EXPR:
			if ( ( retval = nel_expr_update_dollar(eng, 
				stmt->expr.expr, pos, value)) < 0 ) {
				goto error;
			}

			stmt = stmt->expr.next;
			break;

		/********************/
		/* return statement */
		/********************/
		case nel_S_RETURN:
			if ((retval = nel_expr_update_dollar(eng, 
				stmt->ret.retval, pos, value )) < 0 ) {
				goto error;
			}

			stmt = stmt->ret.next;
			break;

		/**********************/
		/* conditional branch */
		/**********************/
		case nel_S_BRANCH:
			if ( (retval = nel_expr_update_dollar(eng, 
				stmt->branch.cond, pos, value )) < 0 
			||(retval = evt_stmt_update_dollar(eng,
				stmt->branch.true_branch, pos, value)) < 0
			||(retval = evt_stmt_update_dollar(eng,
				stmt->branch.false_branch,pos,value)) < 0)
			{
				goto error;
			}
			
			stmt = stmt->branch.next;
			break;

		case nel_S_WHILE:
			if ( (retval = nel_expr_update_dollar(eng, 
				stmt->while_stmt.cond, pos, value )) < 0 
			|| (  retval=evt_stmt_update_dollar(eng,
				stmt->while_stmt.body,pos,value)) < 0)
			{
				goto error;
			}
			
			stmt = stmt->while_stmt.next;
			break;	


		case nel_S_FOR:
			if ( (retval = nel_expr_update_dollar(eng, 
				stmt->for_stmt.init, pos, value )) < 0 
			|| (retval = nel_expr_update_dollar(eng, 
				stmt->for_stmt.cond, pos, value )) < 0 
			|| (retval = nel_expr_update_dollar(eng, 
				stmt->for_stmt.inc, pos, value )) < 0 
			||(retval=evt_stmt_update_dollar(eng,
				stmt->for_stmt.body, pos,value)) < 0)
			{
				goto error;
			}
			
			stmt = stmt->for_stmt.next;
			break;

		/**********/
		/* target */
		/**********/
		case nel_S_TARGET:
			stmt = stmt->target.next;
			break;

		default:
			break;

		}
	}
error:
	return retval; 

}



int nel_stmt_dup( struct nel_eng *eng, nel_stmt *stmts, nel_stmt **new_head, nel_stmt **new_tail)
{
	nel_stmt *first= NULL;
	nel_stmt *prev = NULL;
	nel_stmt *stmt = stmts;
	nel_stmt *new;

	/*
	if( old_head == NULL) {
		*new_head = *new_tail = NULL;
		return 1;
	}
	*/

	while ( stmt != NULL) 
	{

		switch (stmt->gen.type) {
		/*******************************/
		/* declaration of a new symbol */
		/*******************************/
		case nel_S_DEC:
			new = nel_stmt_alloc(eng, nel_S_DEC, 
				stmt->dec.filename, 
				stmt->dec.line, 
				nel_symbol_dup(eng, stmt->dec.symbol), 
				NULL);
			if(first == NULL) first = new ;
			if(prev) nel_stmt_link(prev, new);		
			prev = new;
			stmt = stmt->dec.next;

			break;

		/************************/
		/* expression statement */
		/************************/
		case nel_S_EXPR:
			new = nel_stmt_alloc(eng, nel_S_EXPR, 
				stmt->expr.filename, 
				stmt->expr.line, 
				nel_dup_expr(eng, stmt->expr.expr), 
				NULL);
			if(first == NULL) first = new ;
			if(prev) nel_stmt_link(prev, new);		
			prev = new;
			stmt = stmt->expr.next;
			break;

		/********************/
		/* return statement */
		/********************/
		case nel_S_RETURN:
			new = nel_stmt_alloc(eng, nel_S_RETURN, 
				stmt->ret.filename, 
				stmt->ret.line, 
				nel_dup_expr(eng, stmt->ret.retval), 
				NULL);
			if(first == NULL) first = new ;
			if(prev) nel_stmt_link(prev, new);		
			prev = new;
			stmt = stmt->ret.next;
			break;

		/**********************/
		/* conditional branch */
		/**********************/
		case nel_S_BRANCH:
			{
			nel_stmt *false_head, *false_tail;
			nel_stmt *true_head,true_tail;
				
			nel_stmt_dup(eng, stmt->branch.true_branch, 
					&true_head, &true_tail);
			nel_stmt_dup(eng, stmt->branch.false_branch, 
					&false_head, &false_tail);
			
			new = nel_stmt_alloc(eng, nel_S_BRANCH, 
				stmt->branch.filename, 
				stmt->branch.line, 
				stmt->branch.level, /*bugfix, found by ljh, 2006.06.21 */
				nel_dup_expr(eng,stmt->branch.cond),
						true_head, false_head);
			
			if(first == NULL) first = new ;
			if(prev) nel_stmt_link(prev, new);		
			prev = new;
			stmt = stmt->branch.next;
			}
			break;

		case nel_S_WHILE:
			{
			nel_stmt *body_head, *body_tail;
				
			nel_stmt_dup(eng, stmt->while_stmt.body, 
					&body_head, &body_tail);
			
			new = nel_stmt_alloc(eng, nel_S_WHILE, 
				stmt->while_stmt.filename, 
				stmt->while_stmt.line,
				stmt->while_stmt.level,	/*bugfix, found by ljh, 2006.06.21 */
				nel_dup_expr(eng, stmt->while_stmt.cond), 
				body_head);

			if(first == NULL) first = new ;
			if(prev) nel_stmt_link(prev, new);		
			prev = new;
			stmt = stmt->while_stmt.next;
			}
			break;

		case nel_S_FOR:
			{
			nel_stmt *body_head, *body_tail;
				
			nel_stmt_dup(eng, stmt->while_stmt.body, 
					&body_head, &body_tail);
			
			new = nel_stmt_alloc(eng, nel_S_FOR, 
				stmt->for_stmt.filename, 
				stmt->for_stmt.line,
				stmt->for_stmt.level,/*bugfix, found by ljh, 2006.06.21 */
				nel_dup_expr(eng, stmt->for_stmt.init), 
				nel_dup_expr(eng, stmt->for_stmt.cond), 
				nel_dup_expr(eng, stmt->for_stmt.inc), 
				body_head);

			if(first == NULL) first = new ;
			if(prev) nel_stmt_link(prev, new);		
			prev = new;
			stmt = stmt->for_stmt.next;
			}
			break;

		/**********/
		/* target */
		/**********/
		case nel_S_TARGET:
			new = nel_stmt_alloc(eng, nel_S_TARGET, 
				stmt->target.filename, 
				stmt->target.line, 
				stmt->target.level,
				NULL);
			if(first == NULL) first = new ;
			if(prev) nel_stmt_link(prev, new);		
			prev = new;
			stmt = stmt->target.next;
			break;

		//added by zhangbin, 2006-11-22
		case nel_S_BREAK:
		case nel_S_CONTINUE:
		case nel_S_GOTO:
			new = nel_stmt_alloc(eng, stmt->gen.type,
				stmt->goto_stmt.filename,
				stmt->goto_stmt.line,
				stmt->goto_stmt.level,
				stmt->goto_stmt.goto_target);
			if(first == NULL) first = new ;
			if(prev) nel_stmt_link(prev, new);		
			prev = new;
			stmt = stmt->target.next;
			break;
		//end

		default:
			break;

		}
	}

	*new_head = first;
	*new_tail = prev;

	return 0;

}


int ast_to_intp( struct nel_eng *eng, nel_stmt *stmts, nel_stmt **ret_head, nel_stmt **ret_tail)
{
	nel_stmt *first= NULL;
	nel_stmt *prev = NULL;
	nel_stmt *stmt = stmts;
	int retval = 0;

	while ( stmt != NULL) 
	{
		//added by zhangbin, 2006-5-20, to avoid looping infinetely
		if(stmt->gen.visited == 1)	//stmt has been interpreted
		{
			if(!first)
				first = stmt;
			if(prev)
				nel_stmt_link(prev, stmt);
			//NOTE!!!, NOT like other case,
			//in this case, prev must NOT be updated to stmt
			break;
		}
		stmt->gen.visited = 1;	//mark stmt as visited
		//end
		switch (stmt->gen.type) {
		
		//added by zhangbin, 2006-5-20
		case nel_S_GOTO:
			if(!first)
				first = stmt;
			if(prev)
				nel_stmt_link(prev, stmt);
			prev = stmt;
			stmt = stmt->goto_stmt.next;
			break;
		case nel_S_BREAK:
			stmt->gen.type=nel_S_GOTO;
			if(!first)
				first = stmt;
			if(prev)
				nel_stmt_link(prev, stmt);
			prev = stmt;
			if(stmt->goto_stmt.goto_target->gen.type == nel_S_WHILE)
				stmt->goto_stmt.goto_target = stmt->goto_stmt.goto_target->while_stmt.break_target;
			else if(stmt->goto_stmt.goto_target->gen.type == nel_S_FOR)
				stmt->goto_stmt.goto_target = stmt->goto_stmt.goto_target->for_stmt.break_target;
			else
				goto error;
			stmt = stmt->goto_stmt.next;
			break;
		case nel_S_CONTINUE:
			stmt->gen.type=nel_S_GOTO;
			if(!first)
				first = stmt;
			if(prev)
				nel_stmt_link(prev, stmt);
			prev = stmt;
			if(stmt->goto_stmt.goto_target->gen.type == nel_S_WHILE)
				stmt->goto_stmt.goto_target = stmt->goto_stmt.goto_target->while_stmt.continue_target;
			else if(stmt->goto_stmt.goto_target->gen.type == nel_S_FOR)
				stmt->goto_stmt.goto_target = stmt->goto_stmt.goto_target->for_stmt.continue_target;
			else
				goto error;
			stmt = stmt->goto_stmt.next;
			break;
		//end
		
		/*******************************/
		/* declaration of a new symbol */
		/*******************************/
		case nel_S_DEC:
			if(first == NULL)
				first = stmt;
			if(prev)
				nel_stmt_link(prev, stmt);		
			prev = stmt;
			stmt = stmt->dec.next;
			break;

		/************************/
		/* expression statement */
		/************************/
		case nel_S_EXPR:
			if(first == NULL)
				first = stmt;
			if(prev)
				nel_stmt_link(prev, stmt);		
			prev = stmt;
			stmt = stmt->expr.next;
			break;

		/********************/
		/* return statement */
		/********************/
		case nel_S_RETURN:
			if(first == NULL)
				first = stmt;
			if(prev)
				nel_stmt_link(prev, stmt);		
			prev = stmt;
			stmt = stmt->ret.next;
			break;

		/**********************/
		/* conditional branch */
		/**********************/
		case nel_S_BRANCH:
			/*************************************************/
			/* form the following stmt list:                 */
			/*                                               */
			/*             |                                 */
			/*             V                                 */
			/*      +--------------+                         */
			/*      | branch: cond |--+ f                    */
			/*      +--------------+  |                      */
			/*             | t        |                      */
			/*             V          |                      */
			/*      +--------------+  |                      */
			/*      |  true body   |  |                      */
			/*      +--------------+  |                      */
			/*             |          |                      */
			/*   +---------+          |                      */
			/*   |                    |                      */
			/*   |  +--------------+  |                      */
			/*   |  |  false body  |<-+                      */
			/*   |  +--------------+                         */
			/*   |         |                                 */
			/*   |         V                                 */
			/*   |  +--------------+                         */
			/*   +->|  end target  |                         */
			/*      +--------------+                         */
			/*             |                                 */
			/*             V                                 */
			/*                                               */
			/*************************************************/
			{
				register nel_stmt *if_stmt;
				register nel_stmt *end_target;	
				register nel_expr *cond;
				nel_stmt *head, *tail;
				register nel_stmt *next;

				if(first == NULL)
					first = stmt;
				if(prev)
					nel_stmt_link(prev, stmt);		

				next = stmt->branch.next;
				if ( (retval = ast_to_intp(eng,
						stmt->branch.true_branch, 
						&head, &tail)) < 0 ) {
					goto error;
				}

				prev = (tail) ? tail: head;
				end_target = nel_stmt_alloc (eng, nel_S_TARGET, 
						stmt->branch.filename, 
						stmt->branch.line, 
						stmt->branch.level, NULL);
				nel_stmt_link(prev, end_target);
				prev = end_target;

				
				if(stmt->branch.false_branch != NULL) {
					if ((retval = ast_to_intp(eng,
						stmt->branch.false_branch, 
						&head, &tail)) < 0 ) {
						goto error;
					}
					prev = (tail)? tail : head;
					nel_stmt_link(prev, end_target);
				}else {
					stmt->branch.false_branch = end_target;
				}

				prev = end_target;
				stmt = next;
			}
			break;


		case nel_S_WHILE:
			/*************************************************/
			/*    form the following stmt list:              */
			/*                                               */
			/*             |                                 */
			/*             V                                 */
			/*      +--------------+                         */
			/*   +->| cond_target  |                         */
			/*   |  +--------------+                         */
			/*   |         |                                 */
			/*   |         V                                 */
			/*   |  +--------------+ f                       */
			/*   |  | branch: cond |--+                      */
			/*   |  +--------------+  |                      */
			/*   |         | t        |                      */
			/*   |         V          |                      */
			/*   |  +--------------+  |                      */
			/*   |  |  loop body   |  |                      */
			/*   |  +--------------+  |                      */
			/*   |         |          |                      */
			/*   +---------+          |                      */
			/*                        |                      */
			/*      +--------------+  |                      */
			/*      |  end_target  |<-+                      */
			/*      +--------------+                         */
			/*             |                                 */
			/*             V                                 */
			/*                                               */
			/* cond_target is the continue target            */
			/* end_target is the break target                */
			/*************************************************/
			{
				register nel_expr *cond;
				register nel_stmt *if_stmt;
				register nel_stmt *cond_target;
				register nel_stmt *end_target;
				nel_stmt *head, *tail;
				register nel_stmt *next;

				next = stmt->while_stmt.next;
				cond_target = nel_stmt_alloc (eng, nel_S_TARGET,
						stmt->while_stmt.filename, 
						stmt->while_stmt.line, 
						stmt->while_stmt.level, NULL);
				
				if(first == NULL)
					first = cond_target;
				if(prev )
					nel_stmt_link(prev, cond_target);
				prev = cond_target;

				end_target = nel_stmt_alloc (eng, nel_S_TARGET, 
						stmt->while_stmt.filename, 
						stmt->while_stmt.line, 
						stmt->while_stmt.level, NULL);

				//added by zhangbin, 2006-6-15
				stmt->while_stmt.continue_target = cond_target;
				stmt->while_stmt.break_target = end_target;
				//end

				if_stmt = nel_stmt_alloc (eng, nel_S_BRANCH, 
						stmt->while_stmt.filename,
						stmt->while_stmt.line, 
							stmt->while_stmt.level,
							stmt->while_stmt.cond, 
						NULL, end_target);

				nel_stmt_link(prev, if_stmt);
				//nel_stmt_link(if_stmt, end_target);
				prev = if_stmt;

				if( stmt->while_stmt.body != NULL) {
					if ((retval = ast_to_intp(eng,stmt->while_stmt.body, &head, &tail)) < 0 ) {
						goto error;
					}
					if_stmt->branch.true_branch = head;	
					prev = (tail) ? tail : head;
				}

				nel_stmt_link(prev, cond_target);
				prev = end_target;
				stmt = next;

			}
			break;



		case nel_S_FOR:
			/*************************************************/
			/*    form the following stmt list:              */
			/*                                               */
			/*             |                                 */
			/*             V                                 */
			/*      +--------------+                         */
			/*      |  expr: init  |      (optional)         */
			/*      +--------------+                         */
			/*             |                                 */
			/*             V                                 */
			/*      +--------------+                         */
			/*   +->| cond_target  |                         */
			/*   |  +--------------+                         */
			/*   |         |                                 */
			/*   |         V                                 */
			/*   |  +--------------+ f                       */
			/*   |  | branch: cond |--+   (optional - branch */
			/*   |  +--------------+  |   			 */
			/*   |         | t        |                      */
			/*   |         V          |                      */
			/*   |  +--------------+  |                      */
			/*   |  |  loop body   |  |                      */
			/*   |  +--------------+  |                      */
			/*   |         |          |                      */
			/*   |         V          |                      */
			/*   |  +--------------+  |                      */
			/*   |  |  inc_target  |  |                      */
			/*   |  +--------------+  |                      */
			/*   |         |          |                      */
			/*   |         V          |                      */
			/*   |  +--------------+  |                      */
			/*   |  |  expr: inc   |  |   (optional)         */
			/*   |  +--------------+  |                      */
			/*   |         |          |                      */
			/*   +---------+          |                      */
			/*                        |                      */
			/*      +--------------+  |                      */
			/*      |  end_target  |<-+                      */
			/*      +--------------+                         */
			/*             |                                 */
			/*             V                                 */
			/*                                               */
			/* cond_target is the continue target            */
			/* end_target is the break target                */
			/*************************************************/
			{
				register nel_stmt *if_stmt;
				register nel_stmt *expr_stmt;
				register nel_stmt *cond_target;
				register nel_stmt *inc_target;
				register nel_stmt *end_target;
				nel_stmt *head, *tail;
				register nel_stmt *next;

				next = stmt->for_stmt.next;
				if (stmt->for_stmt.init != NULL) {
					expr_stmt = nel_stmt_alloc (eng, 
								nel_S_EXPR, 
						stmt->for_stmt.filename, 
						stmt->for_stmt.line, 
						stmt->for_stmt.init, NULL);
		
					if(first == NULL)
						first = expr_stmt;
					if(prev)
						nel_stmt_link(prev, expr_stmt);
					prev = expr_stmt;
				}

				cond_target = nel_stmt_alloc (eng, nel_S_TARGET,
						stmt->for_stmt.filename, 
						stmt->for_stmt.line, 
						stmt->for_stmt.level,
						NULL);
				if(first == NULL)
					first = cond_target;
				if(prev)
					nel_stmt_link(prev, cond_target);
				prev = cond_target;


				end_target = nel_stmt_alloc (eng, nel_S_TARGET, 
						stmt->for_stmt.filename, 
						stmt->for_stmt.line, 
						stmt->for_stmt.level, NULL);
				

				if (stmt->for_stmt.inc != NULL) {
					expr_stmt = nel_stmt_alloc (eng, 
							nel_S_EXPR, 
						stmt->for_stmt.filename, 
						stmt->for_stmt.line, 
						stmt->for_stmt.inc, 
						cond_target);
					inc_target = nel_stmt_alloc (eng, 
							nel_S_TARGET, 
						stmt->for_stmt.filename, 
						stmt->for_stmt.line, 
						stmt->for_stmt.level, 
						expr_stmt);

				} else {
					inc_target = nel_stmt_alloc (eng, 
							nel_S_TARGET,
						stmt->for_stmt.filename, 
						stmt->for_stmt.line, 
						stmt->for_stmt.level, 
						cond_target);
				}
				
				//added by zhangbin, 2006-6-15
				stmt->for_stmt.continue_target = inc_target;
				stmt->for_stmt.break_target = end_target;
				//
				
				if (stmt->for_stmt.cond != NULL) {
					if_stmt = nel_stmt_alloc (eng, 
							nel_S_BRANCH, 
						stmt->for_stmt.filename, 
						stmt->for_stmt.line, 
							stmt->for_stmt.level, 
							stmt->for_stmt.cond, 
						NULL, end_target);
					nel_stmt_link(prev, if_stmt);
					prev = if_stmt;
				}

				if(stmt->for_stmt.body != NULL) {
					if ((retval = ast_to_intp(eng,
						stmt->for_stmt.body, 
						&head, &tail)) < 0 ) {
						goto error;
					}

					/* bugfix, wyong, 2006.4.28 */
					if(!stmt->for_stmt.cond)
						nel_stmt_link(prev, head);
					else
						if_stmt->branch.true_branch = head;
					prev = (tail) ? tail : head;
				}


				nel_stmt_link(prev, inc_target);
				prev = end_target;
				stmt = next;

			}
			break;

		/**********/
		/* target */
		/**********/
		case nel_S_TARGET:
			if(first == NULL)
				first = stmt;
			if(prev)
				nel_stmt_link(prev, stmt);	
			prev = stmt;
			stmt = stmt->target.next;
			break;

		default:
			break;

		}
	}

	*ret_head = first;
	*ret_tail = prev;	

error:
	return retval;

}


/*****************************************************************************/
/* emit_stmts () prints out an nel_stmt structure in a pretty format.	     */
/*****************************************************************************/
void emit_stmt(FILE *file, register nel_stmt *stmts, int indent)
{
	nel_stmt *stmt = stmts;

	while (stmt != NULL) 
	{

		switch (stmt->gen.type) {
		/*******************************/
		/* declaration of a new symbol */
		/*******************************/
		case nel_S_DEC:
			tab (indent);
			emit_type(file, stmt->dec.symbol->type);
			fprintf(file, " ");
			emit_symbol(file, stmt->dec.symbol);
			fprintf(file,  ";\n");
			stmt = stmt->dec.next;
			break;

		/************************/
		/* expression statement */
		/************************/
		case nel_S_EXPR:
			tab (indent);
			emit_expr (file, stmt->expr.expr);
			fprintf(file,  ";\n");
			stmt = stmt->expr.next;
			break;

		/********************/
		/* return statement */
		/********************/
		case nel_S_RETURN:
			tab (indent);
			fprintf(file,  "return");
			if(stmt->ret.retval){
				fprintf(file,  "(");
				emit_expr (file, stmt->ret.retval);
				fprintf(file,  ")");
			}
			fprintf(file,  ";\n");
			stmt = stmt->ret.next;
			break;

		/**********************/
		/* conditional branch */
		/**********************/
		case nel_S_BRANCH:
			tab (indent);
			fprintf(file, "if(");
			emit_expr (file, stmt->branch.cond);
			fprintf(file, "){\n");
			emit_stmt(file,  stmt->branch.true_branch, indent + 1);

			tab (indent);
			fprintf(file, "}\n");
			
			if(stmt->branch.false_branch != NULL ) {
				tab (indent);
				fprintf(file, "else{\n");
				emit_stmt(file,  stmt->branch.false_branch, indent + 1);
				tab (indent);
				fprintf(file, "}\n");
			}

			stmt = stmt->branch.next;
			break;

		case nel_S_WHILE:
			tab (indent);
			fprintf(file, "while(");
			emit_expr (file, stmt->while_stmt.cond);
			fprintf(file, "){\n");
			emit_stmt(file,  stmt->while_stmt.body, indent + 1);
			fprintf(file, "\n");
			tab (indent);
			fprintf(file, "}\n");
			stmt = stmt->while_stmt.next;
			break;
			

		case nel_S_FOR:	
			tab (indent);
			fprintf(file, "for(");
			emit_expr (file, stmt->for_stmt.init);
			fprintf(file, ";");
			emit_expr (file, stmt->for_stmt.cond);
			fprintf(file, ";");
			emit_expr (file, stmt->for_stmt.inc);
			fprintf(file, ";){\n");
			emit_stmt(file,  stmt->for_stmt.body, indent + 1);
			fprintf(file, "\n");
			tab (indent);
			fprintf(file, "}\n");
			stmt = stmt->for_stmt.next;
			break;


		/**********/
		/* target */
		/**********/
		case nel_S_TARGET:
			/*
			if(stmt->target.level > level ) {
				fprintf(file, "{\n");				
				level++;
			}

			else if(stmt->target.level < level) {
				fprintf(file, "}\n");				
				level--;
			}
			*/

			stmt = stmt->target.next;
			break;

		default:
			//break; wyong, 2006.5.24 
			return;

		}
	}

	return;

}

//added by zhangbin, 2006-7-19
//calling nel_dealloca to free stmt chunk,
//after call this function, all stmt pointer should be illegal!!!!!!!!
void stmt_dealloc(struct nel_eng *eng)
{
	while(nel_stmt_chunks)
	{
		nel_stmt_chunk *chunk = nel_stmt_chunks->next;
		nel_dealloca(nel_stmt_chunks->start); 
		nel_dealloca(nel_stmt_chunks); 
		nel_stmt_chunks = chunk;
	}
	nel_stmts_next = nel_stmts_end = nel_free_stmts = NULL;
}
//end
