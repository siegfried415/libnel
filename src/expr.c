

/*****************************************************************************/
/* This file, "expr.c", contains the parse tree evaluator for the    */
/* application executive.                                                    */
/*****************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include <setjmp.h>
#include <string.h>

#include "engine.h"
#include "errors.h"
#include "io.h"
#include "type.h"
#include "sym.h"
#include "expr.h"
#include "intp.h"
#include "lex.h"
#include "mem.h"
#include "parser.h" 

/*****************************************************************************/
/* nel_O_name () returns the string that is the identifier for <code>, a */
/* nel_O_token representing an operation.                                */
/*****************************************************************************/
char *nel_O_name (register nel_O_token code)
{
	switch (code) {
	case nel_O_NULL:		return ("nel_O_NULL");
	case nel_O_ADD:		return ("nel_O_ADD");
	case nel_O_ADD_ASGN:	return ("nel_O_ADD_ASGN");
	case nel_O_ADDRESS:	return ("nel_O_ADDRESS");
	case nel_O_AND:		return ("nel_O_AND");
	case nel_O_ASGN:		return ("nel_O_ASGN");
	case nel_O_ARRAY_INDEX:	return ("nel_O_ARRAY_INDEX");
	case nel_O_BIT_AND:	return ("nel_O_BIT_AND");
	case nel_O_BIT_AND_ASGN:	return ("nel_O_BIT_AND_ASGN");
	case nel_O_BIT_OR:		return ("nel_O_BIT_OR");
	case nel_O_BIT_OR_ASGN:	return ("nel_O_BIT_OR_ASGN");
	case nel_O_BIT_NEG:	return ("nel_O_BIT_NEG");
	case nel_O_BIT_XOR:	return ("nel_O_BIT_XOR");
	case nel_O_BIT_XOR_ASGN:	return ("nel_O_BIT_XOR_ASGN");
	case nel_O_CAST:		return ("nel_O_CAST");
	case nel_O_COMPOUND:	return ("nel_O_COMPOUND");
	case nel_O_COND:		return ("nel_O_COND");
	case nel_O_DEREF:		return ("nel_O_DEREF");
	case nel_O_DIV:		return ("nel_O_DIV");
	case nel_O_DIV_ASGN:	return ("nel_O_DIV_ASGN");
	case nel_O_EQ:		return ("nel_O_EQ");
	case nel_O_FUNCALL:	return ("nel_O_FUNCALL");
	case nel_O_GE:		return ("nel_O_GE");
	case nel_O_GT:		return ("nel_O_GT");
	case nel_O_LE:		return ("nel_O_LE");
	case nel_O_LSHIFT:		return ("nel_O_LSHIFT");
	case nel_O_LSHIFT_ASGN:	return ("nel_O_LSHIFT_ASGN");
	case nel_O_LT:		return ("nel_O_LT");
	case nel_O_MEMBER:		return ("nel_O_MEMBER");
	case nel_O_MEMBER_PTR:	return ("nel_O_MEMBER_PTR");
	case nel_O_MOD:		return ("nel_O_MOD");
	case nel_O_MOD_ASGN:	return ("nel_O_MOD_ASGN");
	case nel_O_MULT:		return ("nel_O_MULT");
	case nel_O_MULT_ASGN:	return ("nel_O_MULT_ASGN");
	case nel_O_NE:		return ("nel_O_NE");
	case nel_O_NEG:		return ("nel_O_NEG");
	case nel_O_NOT:		return ("nel_O_NOT");
	case nel_O_OR:		return ("nel_O_OR");
	case nel_O_POST_DEC:	return ("nel_O_POST_DEC");
	case nel_O_POST_INC:	return ("nel_O_POST_INC");
	case nel_O_POS:		return ("nel_O_POS");
	case nel_O_PRE_DEC:	return ("nel_O_PRE_DEC");
	case nel_O_PRE_INC:	return ("nel_O_PRE_INC");
	case nel_O_RETURN:		return ("nel_O_RETURN");
	case nel_O_RSHIFT:		return ("nel_O_RSHIFT");
	case nel_O_RSHIFT_ASGN:	return ("nel_O_RSHIFT_ASGN");
	case nel_O_SIZEOF:		return ("nel_O_SIZEOF");
	case nel_O_SUB:		return ("nel_O_SUB");
	case nel_O_SUB_ASGN:	return ("nel_O_SUB_ASGN");
	case nel_O_SYMBOL:		return ("nel_O_SYMBOL");
	case nel_O_TYPEOF:		return ("nel_O_TYPEOF");
	case nel_O_MAX:		return ("nel_O_MAX");
	default:			return ("nel_O_BAD_TOKEN");
	}
}


/*****************************************************************************/
/* nel_O_string () returns the string of operator symbols that <code>    */
/* represents.                                                               */
/*****************************************************************************/
char *nel_O_string (register nel_O_token code)
{
	switch (code) {
	case nel_O_NULL:		return ("nel_O_NULL");
	case nel_O_ADD:		return ("+");
	case nel_O_ADD_ASGN:	return ("+=");
	case nel_O_ADDRESS:	return ("&");
	case nel_O_AND:		return ("&&");
	case nel_O_ASGN:		return ("=");
	case nel_O_ARRAY_INDEX:	return ("[]");
	case nel_O_BIT_AND:	return ("&");
	case nel_O_BIT_AND_ASGN:	return ("&=");
	case nel_O_BIT_OR:		return ("|");
	case nel_O_BIT_OR_ASGN:	return ("|=");
	case nel_O_BIT_NEG:	return ("~");
	case nel_O_BIT_XOR:	return ("^");
	case nel_O_BIT_XOR_ASGN:	return ("^=");
	case nel_O_CAST:		return ("cast");
	case nel_O_COMPOUND:	return ("compund");
	case nel_O_COND:		return ("?");
	case nel_O_DEREF:		return ("*");
	case nel_O_DIV:		return ("/");
	case nel_O_DIV_ASGN:	return ("/=");
	case nel_O_EQ:		return ("==");
	case nel_O_FUNCALL:	return ("funcall");
	case nel_O_GE:		return (">=");
	case nel_O_GT:		return (">");
	case nel_O_LE:		return ("<=");
	case nel_O_LSHIFT:		return ("<<");
	case nel_O_LSHIFT_ASGN:	return ("<<=");
	case nel_O_LT:		return ("<");
	case nel_O_MEMBER:		return (".");
	case nel_O_MEMBER_PTR:	return ("->");
	case nel_O_MOD:		return ("%");
	case nel_O_MOD_ASGN:	return ("%=");
	case nel_O_MULT:		return ("*");
	case nel_O_MULT_ASGN:	return ("*=");
	case nel_O_NE:		return ("!=");
	case nel_O_NEG:		return ("-");
	case nel_O_NOT:		return ("!");
	case nel_O_OR:		return ("||");
	case nel_O_POST_DEC:	return ("--");
	case nel_O_POST_INC:	return ("++");
	case nel_O_POS:		return ("+");
	case nel_O_PRE_DEC:	return ("--");
	case nel_O_PRE_INC:	return ("++");
	case nel_O_RETURN:		return ("return");
	case nel_O_RSHIFT:		return (">>");
	case nel_O_RSHIFT_ASGN:	return (">>=");
	case nel_O_SIZEOF:		return ("sizeof");
	case nel_O_SUB:		return ("-");
	case nel_O_SUB_ASGN:	return ("-=");
	case nel_O_SYMBOL:		return ("symbol");
	case nel_O_TYPEOF:		return ("typeof");
	case nel_O_MAX:		return ("nel_O_MAX");
	default:			return ("nel_O_BAD_TOKEN");
	}
}


/*********************************************************/
/* declarations for the nel_expr structure allocator */
/*********************************************************/
unsigned_int nel_exprs_max = 0x1000;
nel_lock_type nel_exprs_lock = nel_unlocked_state;
static nel_expr *nel_exprs_next = NULL;
static nel_expr *nel_exprs_end = NULL;
static nel_expr *nel_free_exprs = NULL;

/***************************************************************/
/* make a linked list of nel_expr_chunk structures so that */
/* we may find the start of all chunks of memory allocated     */
/* to hold nel_expr structures at garbage collection time. */
/***************************************************************/
static nel_expr_chunk *nel_expr_chunks = NULL;



/*****************************************************************************/
/* nel_expr_alloc allocates space for an nel_expr structure,         */
/* initializes its fields, and returns a pointer to the structure.           */
/*****************************************************************************/
nel_expr *nel_expr_alloc (struct nel_eng *eng, nel_O_token opcode, va_alist)
{
	va_list args;			/* scans arguments after size	*/
	register nel_expr *retval;	/* return value			*/

	nel_debug ({ nel_trace (eng, "entering nel_expr_alloc [\nopcode = %O\n\n", opcode); });

	va_start (args, opcode);

	/*******************************************/
	/* static allocators are critical sections */
	/*******************************************/
	nel_lock (&nel_exprs_lock);

	if (nel_free_exprs != NULL) {
		/************************************************************/
		/* first, try to re-use deallocated nel_expr structures */
		/************************************************************/
		retval = nel_free_exprs;
		nel_free_exprs = (nel_expr *) nel_free_exprs->unary.operand;
	}
	else {
		/***************************************************************/
		/* check for overflow of space allocated for nel_expr      */
		/* structures.  on overflow, allocate another chunk of memory. */
		/***************************************************************/
		if (nel_exprs_next >= nel_exprs_end) {
			register nel_expr_chunk *chunk;

			nel_debug ({ nel_trace (eng, "nel_expr structure overflow - allocating more space\n\n"); });
			nel_malloc (nel_exprs_next, nel_exprs_max, nel_expr);
			nel_exprs_end = nel_exprs_next + nel_exprs_max;

			/*************************************************/
			/* make an entry for this chunk of memory in the */
			/* list of nel_expr_chunks.                       */
			/*************************************************/
			nel_malloc (chunk, 1, nel_expr_chunk);
			chunk->start = nel_exprs_next;
			chunk->size = nel_exprs_max;
			chunk->next = nel_expr_chunks;
			nel_expr_chunks = chunk;
		}
		retval = nel_exprs_next++;
	}

	/*************************/
	/* exit critical section */
	/*************************/
	nel_unlock (&nel_exprs_lock);

	/****************************************/
	/* initialize the appropriate members.  */
	/****************************************/
	retval->gen.opcode = opcode;
	switch (opcode) {

	/**************************/
	/* terminal node (symbol) */
	/**************************/
	case nel_O_SYMBOL:
		retval->symbol.symbol = va_arg (args, nel_symbol*);
		break;

	/* 
	case nel_O_NEL_SYMBOL:
		retval->nel_symbol.symbol = va_arg (args, nel_symbol *);
		break;
	*/

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
		retval->unary.operand = va_arg (args, nel_expr *);
		break;

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
	case nel_O_COMPOUND:
	case nel_O_DIV:
	case nel_O_DIV_ASGN:
	case nel_O_EQ:
	case nel_O_GE:
	case nel_O_GT:
	case nel_O_LE:
	case nel_O_LSHIFT:
	case nel_O_LSHIFT_ASGN:
	case nel_O_LT:
	case nel_O_MOD:
	case nel_O_MOD_ASGN:
	case nel_O_MULT:
	case nel_O_MULT_ASGN:
	case nel_O_NE:
	case nel_O_OR:
	case nel_O_RSHIFT:
	case nel_O_RSHIFT_ASGN:
	case nel_O_SUB:
	case nel_O_SUB_ASGN:
		retval->binary.left = va_arg (args, nel_expr *);
		retval->binary.right = va_arg (args, nel_expr *);
		break;

	case nel_O_ARRAY_RANGE:
		retval->range.array = va_arg(args, nel_expr *);
		retval->range.upper = va_arg(args, nel_expr *);	
		retval->range.lower = va_arg(args, nel_expr *);	
		break;

	/***********************/
	/* struct/union member */
	/***********************/
	case nel_O_MEMBER:
		retval->member.s_u = va_arg (args, nel_expr *);
		retval->member.member = va_arg (args, nel_symbol *);
		break;

	/*****************/
	/* function call */
	/*****************/
	case nel_O_FUNCALL:
		retval->funcall.func = va_arg (args, nel_expr *);
		retval->funcall.nargs = va_arg (args, int);
		retval->funcall.args = va_arg (args, nel_expr_list *);
		break;

	/**********************************************/
	/* type epxression: sizeof, typeof, or a cast */
	/**********************************************/
	case nel_O_CAST:
	case nel_O_SIZEOF:
	case nel_O_TYPEOF:
		retval->type.type = va_arg (args, nel_type *);
		retval->type.expr = va_arg (args, nel_expr *);
		break;

	/******************/
	/* conditional: ? */
	/******************/
	case nel_O_COND:
		retval->cond.cond = va_arg (args, nel_expr *);
		retval->cond.true_expr = va_arg (args, nel_expr *);
		retval->cond.false_expr = va_arg (args, nel_expr *);
		break;

	/******************/
	/* expr list:     */
	/******************/
	case nel_O_LIST:
		retval->list.list = va_arg(args, nel_expr_list *);
		retval->list.num  = va_arg(args, int);
		break;

	/*********************************************************/
	/* the following should never occur as the discriminator */
	/* of the nel_eval_expr union -                      */
	/* they are only used for error messages.                */
	/*********************************************************/
	case nel_O_MEMBER_PTR:
	case nel_O_RETURN:
	default:
		break;
	}

	va_end (args);
	nel_debug ({ nel_trace (eng, "] exiting nel_expr_alloc\nretval =\n%1X\n", retval); });

	return (retval);
}


/*****************************************************************************/
/* nel_expr_dealloc returns the nel_expr structure pointed to by     */
/* <expr> back to the free list (nel_free_exprs), so that the space may  */
/* be re-used.                                                               */
/*****************************************************************************/
void nel_expr_dealloc (register nel_expr *expr)
{
	nel_debug ({
	if (expr == NULL) {
		//nel_fatal_error (NULL, "(nel_expr_dealloc #1): expr == NULL");
	}
	});

	/*******************************************/
	/* static allocators are critical sections */
	/*******************************************/
	nel_lock (&nel_exprs_lock);

	expr->gen.next = nel_free_exprs;
	nel_free_exprs = expr;

	/*************************/
	/* exit critical section */
	/*************************/
	nel_unlock (&nel_exprs_lock);

}


#define tab(_indent); \
{								\
	register int __indent = (_indent);				\
	int __i;							\
	for (__i = 0; (__i < __indent); __i++) {			\
		fprintf (file, "   ");					\
	}								\
}



/*****************************************************************************/
/* nel_print_expr () prints out an expression tree in a pretty format.   */
/*****************************************************************************/
void nel_print_expr (FILE *file, register nel_expr *expr, int indent)
{
	if (expr == NULL) {
		tab (indent);
		fprintf (file, "NULL_expr\n");
	}
	else {
		tab (indent);
		fprintf (file, "0x%p\n", expr);
		tab (indent);
		fprintf (file, "opcode: %s\n", nel_O_name (expr->gen.opcode));

		switch (expr->gen.opcode) {

		/****************************/
		/* terminal node - a symbol */
		/****************************/
		case nel_O_SYMBOL:
			tab (indent);
			fprintf (file, "symbol:\n");
			nel_print_symbol (file, expr->symbol.symbol, indent + 1);
			break;

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
			tab (indent);
			fprintf (file, "operand:\n");
			nel_print_expr (file, expr->unary.operand, indent + 1);
			break;

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
		case nel_O_COMPOUND:
		case nel_O_DIV:
		case nel_O_DIV_ASGN:
		case nel_O_EQ:
		case nel_O_GE:
		case nel_O_GT:
		case nel_O_LE:
		case nel_O_LSHIFT:
		case nel_O_LSHIFT_ASGN:
		case nel_O_LT:
		case nel_O_MOD:
		case nel_O_MOD_ASGN:
		case nel_O_MULT:
		case nel_O_MULT_ASGN:
		case nel_O_NE:
		case nel_O_OR:
		case nel_O_RSHIFT:
		case nel_O_RSHIFT_ASGN:
		case nel_O_SUB:
		case nel_O_SUB_ASGN:
			tab (indent);
			fprintf (file, "left:\n");
			nel_print_expr (file, expr->binary.left, indent + 1);
			tab (indent);
			fprintf (file, "right:\n");
			nel_print_expr (file, expr->binary.right, indent + 1);
			break;

		/*****************/
		/* function call */
		/*****************/
		case nel_O_FUNCALL:
			tab (indent);
			fprintf (file, "func:\n");
			nel_print_expr (file, expr->funcall.func, indent + 1);
			tab (indent);
			fprintf (file, "nargs: %d\n", expr->funcall.nargs);
			tab (indent);
			fprintf (file, "args:\n");
			nel_print_expr_list (file, expr->funcall.args, indent + 1);
			break;

		/***********************/
		/* struct/union member */
		/***********************/
		case nel_O_MEMBER:
			tab (indent);
			fprintf (file, "s_u:\n");
			nel_print_expr (file, expr->member.s_u, indent + 1);
			tab (indent);
			fprintf (file, "member:\n");
			nel_print_symbol (file, expr->member.member, indent + 1);
			break;

		/**********************************************/
		/* type epxression: sizeof, typeof, or a cast */
		/**********************************************/
		case nel_O_CAST:
		case nel_O_SIZEOF:
		case nel_O_TYPEOF:
			tab (indent);
			fprintf (file, "type:\n");
			nel_print_type (file, expr->type.type, indent + 1);
			tab (indent);
			fprintf (file, "expr:\n");
			nel_print_expr (file, expr->type.expr, indent + 1);
			break;

		/******************/
		/* conditional: ? */
		/******************/
		case nel_O_COND:
			tab (indent);
			fprintf (file, "cond:\n");
			nel_print_expr (file, expr->cond.cond, indent + 1);
			tab (indent);
			fprintf (file, "true_expr:\n");
			nel_print_expr (file, expr->cond.true_expr, indent + 1);
			tab (indent);
			fprintf (file, "false_expr:\n");
			nel_print_expr (file, expr->cond.false_expr, indent + 1);
			break;

		/*********************************************************/
		/* the following should never occur as the discriminator */
		/* of the nel_eval_expr union -                      */
		/* they are only used for error messages.                */
		/*********************************************************/
		case nel_O_MEMBER_PTR:
		case nel_O_RETURN:
		default:
			break;
		}
	}
}


int nel_expr_diff(nel_expr *e1, nel_expr *e2)
{
	if(!e1 && !e2)
		return 0;
	
	if((!e1 && e2 ) || (e1 && !e2 ))
		return 1;
	
	if (e1->gen.opcode != e2->gen.opcode)
		return 1;

	switch (e1->gen.opcode) {

	/****************************/
	/* terminal node - a symbol */
	/****************************/
	case nel_O_SYMBOL:
		if(e1->symbol.symbol->type->simple.type 
			!= e2->symbol.symbol->type->simple.type )
			return 1;

		switch(e1->symbol.symbol->type->simple.type){
		case nel_D_ARRAY:
		case nel_D_FUNCTION:
		case nel_D_STRUCT:
		case nel_D_STRUCT_TAG:
		case nel_D_POINTER:
			return (strcmp(e1->symbol.symbol->name, e2->symbol.symbol->name)); 
			break;


		case nel_D_SHORT:
		case nel_D_SHORT_INT:
		case nel_D_SIGNED:
		case nel_D_SIGNED_SHORT:
		case nel_D_SIGNED_SHORT_INT:
		case nel_D_UNSIGNED_SHORT:
		case nel_D_UNSIGNED_SHORT_INT:
			return ((int)(*((short *)e1->symbol.symbol->value)) 
				!= (int )(*((short *)e2->symbol.symbol->value)));

		case nel_D_INT:
		case nel_D_SIGNED_INT:
		case nel_D_UNSIGNED:
		case nel_D_UNSIGNED_INT:
			return (*((int *)e1->symbol.symbol->value) 
				!= *((int *)e2->symbol.symbol->value));

		case nel_D_CHAR:
		case nel_D_SIGNED_CHAR:
		case nel_D_UNSIGNED_CHAR:
			return (*((char *)e1->symbol.symbol->value) 
				!= *((char *)e2->symbol.symbol->value));

		case nel_D_LONG:
		case nel_D_LONG_INT:
		case nel_D_SIGNED_LONG:
		case nel_D_SIGNED_LONG_INT:
		case nel_D_UNSIGNED_LONG:
		case nel_D_UNSIGNED_LONG_INT:
			return (*((long *)e1->symbol.symbol->value) 
				!= *((long *)e2->symbol.symbol->value));

		case nel_D_FLOAT:
			return ((double)(*((float*)e1->symbol.symbol->value)) 
				!= (double)(*((float*)e2->symbol.symbol->value)));

		case nel_D_DOUBLE:
			return (*((double *)e1->symbol.symbol->value) 
				!= *((double *)e2->symbol.symbol->value));

		case nel_D_NULL:
		case nel_D_COMMON:
		case nel_D_COMPLEX:		/* extension: basic complex type	*/
		case nel_D_COMPLEX_DOUBLE:	/* extension: ext. precision complex	*/
		case nel_D_COMPLEX_FLOAT:		/* extension: same as complex		*/
		case nel_D_ENUM:
		case nel_D_ENUM_TAG:		/* tags have type descriptors		*/
		case nel_D_FILE:			/* file names have type descriptors	*/
		case nel_D_LABEL:			/* goto target				*/
		case nel_D_LOCATION:		/* identifier from ld symbol table	*/
		case nel_D_LONG_COMPLEX:		/* extension: same as complex double	*/
		case nel_D_LONG_COMPLEX_DOUBLE:	/* extension: longest complex type	*/
		case nel_D_LONG_COMPLEX_FLOAT:	/* extension: same as complex double	*/
		case nel_D_LONG_DOUBLE:
		case nel_D_STAB_UNDEF:		/* placeholder until type is defined	*/
		case nel_D_LONG_FLOAT:		/* same as double			*/
		case nel_D_TYPE_NAME:		/* int, long, etc. have symbols, too	*/
		case nel_D_TYPEDEF_NAME:
		case nel_D_UNION:
		case nel_D_UNION_TAG:
		case nel_D_VOID:
		default:
			break;
		}


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
		return (nel_expr_diff(e1->unary.operand, e2->unary.operand));

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
	case nel_O_COMPOUND:
	case nel_O_DIV:
	case nel_O_DIV_ASGN:
	case nel_O_EQ:
	case nel_O_GE:
	case nel_O_GT:
	case nel_O_LE:
	case nel_O_LSHIFT:
	case nel_O_LSHIFT_ASGN:
	case nel_O_LT:
	case nel_O_MOD:
	case nel_O_MOD_ASGN:
	case nel_O_MULT:
	case nel_O_MULT_ASGN:
	case nel_O_NE:
	case nel_O_OR:
	case nel_O_RSHIFT:
	case nel_O_RSHIFT_ASGN:
	case nel_O_SUB:
	case nel_O_SUB_ASGN:

		return (nel_expr_diff(e1->binary.left, e2->binary.left) 
			|| nel_expr_diff(e1->binary.right, e2->binary.right));


	/*****************/
	/* function call */
	/*****************/
	case nel_O_FUNCALL:
		{
			register nel_expr_list *list1, *list2;
			if( nel_expr_diff ( e1->funcall.func, e2->funcall.func)){
				return 1;
			}

			for (list1 =e1->funcall.args, list2=e2->funcall.args;list1!=NULL && list2!=NULL ; list1=list1->next, list2=list2->next){
				if ( nel_expr_diff (list1->expr, list2->expr)) {
					return 1;
				}
			}

			return 0;
		}

	/***********************/
	/* struct/union member */
	/***********************/
	case nel_O_MEMBER:
		return ((nel_expr_diff(e1->member.s_u, e2->member.s_u)) 
			|| ( nel_symbol_diff(e1->member.member, e2->member.member)));

	/**********************************************/
	/* type epxression: sizeof, typeof, or a cast */
	/**********************************************/
	case nel_O_CAST:
	case nel_O_SIZEOF:
	case nel_O_TYPEOF:
		return (nel_expr_diff(e1->type.expr, e2->type.expr));

	/******************/
	/* conditional: ? */
	/******************/
	case nel_O_COND:
		return ((nel_expr_diff(e1->cond.cond, e2->cond.cond))
			|| (nel_expr_diff(e1->cond.true_expr, e2->cond.true_expr))
			|| (nel_expr_diff(e1->cond.false_expr, e2->cond.false_expr)));

	/*********************************************************/
	/* the following should never occur as the discriminator */
	/* of the nel_eval_expr union -                      */
	/* they are only used for error messages.                */
	/*********************************************************/
	case nel_O_MEMBER_PTR:
	case nel_O_RETURN:
	default:
		break;
	}

	return 0;
	 
}


/*********************************************************************/
/* declarations for expressions for (which point to the symbols for) */
/* the integer values 0 and 1, which are boolean return values.      */
/*********************************************************************/
nel_expr *nel_zero_expr;
nel_expr *nel_one_expr;


/**************************************************************/
/* declarations for the nel_expr_list structure allocator */
/**************************************************************/
unsigned_int nel_expr_lists_max = 0x800;
nel_lock_type nel_expr_lists_lock = nel_unlocked_state;
static nel_expr_list *nel_expr_lists_next = NULL;
static nel_expr_list *nel_expr_lists_end = NULL;
static nel_expr_list *nel_free_expr_lists = NULL;

/********************************************************************/
/* make a linked list of nel_expr_list_chunk structures so that */
/* we may find the start of all chunks of memory allocated          */
/* to hold list structures at garbage collection time.              */
/********************************************************************/
static nel_expr_list_chunk *nel_expr_list_chunks = NULL;


/*****************************************************************************/
/* nel_expr_list_alloc () allocates space for an nel_expr_list       */
/* structure, initializes of its fields, and returns a pointer to the        */
/* result.                                                                   */
/*****************************************************************************/
nel_expr_list *nel_expr_list_alloc (struct nel_eng *eng, register nel_expr *expr, register nel_expr_list *next)
{
	register nel_expr_list *retval;

	nel_debug ({ nel_trace (eng, "entering nel_expr_list_alloc [\nexpr =\n%1Xnext =\n%1Y\n", expr, next); });

	/*******************************************/
	/* static allocators are critical sections */
	/*******************************************/
	nel_lock (&nel_expr_lists_lock);

	if (nel_free_expr_lists != NULL) {
		/****************************************************/
		/* first, try to re-use deallocated list structures */
		/****************************************************/
		retval = nel_free_expr_lists;
		nel_free_expr_lists = nel_free_expr_lists->next;
	}
	else {
		/**************************************************************/
		/* check for overflow of space allocated for list structures. */
		/* on overflow, allocate another chunk of memory.             */
		/**************************************************************/
		if (nel_expr_lists_next >= nel_expr_lists_end) {
			register nel_expr_list_chunk *chunk;

			nel_debug ({ nel_trace (eng, "list structure overflow - allocating more space\n\n"); });
			nel_malloc (nel_expr_lists_next, nel_expr_lists_max, nel_expr_list);
			nel_expr_lists_end = nel_expr_lists_next + nel_expr_lists_max;

			/*************************************************/
			/* make an entry for this chunk of memory in the */
			/* list of nel_expr_list_chunks.             */
			/*************************************************/
			nel_malloc (chunk, 1, nel_expr_list_chunk);
			chunk->start = nel_expr_lists_next;
			chunk->next = nel_expr_list_chunks;
			nel_expr_list_chunks = chunk;
		}
		retval = nel_expr_lists_next++;
	}

	/*************************/
	/* exit critical section */
	/*************************/
	nel_unlock (&nel_expr_lists_lock);

	/**************************************/
	/* initialize the appropriate members */
	/**************************************/
	retval->expr = expr;
	retval->next = next;

	nel_debug ({ nel_trace (eng, "] exiting nel_expr_list_alloc\nretval =\n%1Y\n", retval); });
	return (retval);

}


/*****************************************************************************/
/* nel_expr_list_dealloc () returns the nel_expr_list structure      */
/* pointed to by <list> back to the free list (nel_free_expr_lists), so  */
/* that the space may be re-used.                                            */
/*****************************************************************************/
void nel_expr_list_dealloc (register nel_expr_list *list)
{
	nel_debug ({
	if (list == NULL) {
		//nel_fatal_error (NULL, "(nel_expr_list_dealloc #1): list == NULL");
	}
	});

	/*******************************************/
	/* static allocators are critical sections */
	/*******************************************/
	nel_lock (&nel_expr_lists_lock);

	list->next = nel_free_expr_lists;
	nel_free_expr_lists = list;

	/*************************/
	/* exit critical section */
	/*************************/
	nel_unlock (&nel_expr_lists_lock);

}


/*****************************************************************************/
/* nel_print_expr_list () prints out the list of symbols pointed to by   */
/* <list>.                                                                   */
/*****************************************************************************/
void nel_print_expr_list (FILE *file, register nel_expr_list *list, int indent)
{
	if (list == NULL) {
		tab (indent);
		fprintf (file, "NULL\n");
	}else{
		for (; (list != NULL); list = list->next) {
			tab (indent);
			fprintf (file, "expr:\n");
			nel_print_expr (file, list->expr, indent + 1);
		}
	}
}


int nel_expr_update_dollar(struct nel_eng *eng, register nel_expr *expr, int pos, int value)
{
	int retval = 0;

	if (expr != NULL){

		switch (expr->gen.opcode) {

		/****************************/
		/* terminal node - a symbol */
		/****************************/
		case nel_O_SYMBOL:
			retval = evt_symbol_update_dollar(eng, expr->symbol.symbol, pos, value);
			break;

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
			retval = nel_expr_update_dollar(eng,  expr->unary.operand, pos, value);
			break;

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
		case nel_O_COMPOUND:
		case nel_O_DIV:
		case nel_O_DIV_ASGN:
		case nel_O_EQ:
		case nel_O_GE:
		case nel_O_GT:
		case nel_O_LE:
		case nel_O_LSHIFT:
		case nel_O_LSHIFT_ASGN:
		case nel_O_LT:
		case nel_O_MOD:
		case nel_O_MOD_ASGN:
		case nel_O_MULT:
		case nel_O_MULT_ASGN:
		case nel_O_NE:
		case nel_O_OR:
		case nel_O_RSHIFT:
		case nel_O_RSHIFT_ASGN:
		case nel_O_SUB:
		case nel_O_SUB_ASGN:
			if (( retval = nel_expr_update_dollar(eng, expr->binary.left, pos, value)) < 0 ){
				break;
			}
			
			retval = nel_expr_update_dollar(eng, expr->binary.right, pos, value);
			break;

		/*****************/
		/* function call */
		/*****************/
		case nel_O_FUNCALL:
			{
				register nel_expr_list *list;
				if ((retval = nel_expr_update_dollar(eng, expr->funcall.func,pos,value)) < 0 ) {
					break;
				}

				for (list=expr->funcall.args; list!=NULL; list=list->next) {
					if ((retval = nel_expr_update_dollar(eng, list->expr, pos, value)) < 0 ) {
						break;
					} 
				
				}
			}
			break;

		/***********************/
		/* struct/union member */
		/***********************/
		case nel_O_MEMBER:
			retval = nel_expr_update_dollar(eng, expr->member.s_u, pos, value);
			break;

		/**********************************************/
		/* type epxression: sizeof, typeof, or a cast */
		/**********************************************/
		case nel_O_CAST:
		case nel_O_SIZEOF:
		case nel_O_TYPEOF:
			retval = nel_expr_update_dollar(eng, expr->type.expr, pos,value);
			break;

		/******************/
		/* conditional: ? */
		/******************/
		case nel_O_COND:
			if ((retval = nel_expr_update_dollar(eng, expr->cond.cond, pos, value)) < 0 ) {
				break;
			}
			
			if (( retval = nel_expr_update_dollar(eng, expr->cond.true_expr, pos, value)) < 0 ) {
				break;
			}

			retval = nel_expr_update_dollar(eng, expr->cond.false_expr, pos, value);
			break;

		/*********************************************************/
		/* the following should never occur as the discriminator */
		/* of the nel_eval_expr union -                      */
		/* they are only used for error messages.                */
		/*********************************************************/
		case nel_O_MEMBER_PTR:
		case nel_O_RETURN:
		default:
			retval = -1;
			break;
		}
	}

	return retval;

}


int is_indep_expr(register nel_expr *expr)
{
	if (expr != NULL){

		switch (expr->gen.opcode) {

		/****************************/
		/* terminal node - a symbol */
		/****************************/
		case nel_O_SYMBOL:
			{	
				nel_symbol *symbol = expr->symbol.symbol;
				int value =0;

				if(symbol->name != NULL && symbol->name[0] == '$'){
					sscanf(symbol->name, "$%d", &value);
					return value;
				}
			}
			break;

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
			return is_indep_expr( expr->unary.operand);

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
		case nel_O_COMPOUND:
		case nel_O_DIV:
		case nel_O_DIV_ASGN:
		case nel_O_EQ:
		case nel_O_GE:
		case nel_O_GT:
		case nel_O_LE:
		case nel_O_LSHIFT:
		case nel_O_LSHIFT_ASGN:
		case nel_O_LT:
		case nel_O_MOD:
		case nel_O_MOD_ASGN:
		case nel_O_MULT:
		case nel_O_MULT_ASGN:
		case nel_O_NE:
		case nel_O_OR:
		case nel_O_RSHIFT:
		case nel_O_RSHIFT_ASGN:
		case nel_O_SUB:
		case nel_O_SUB_ASGN:
			{
				int pos[2];
				int i;
				int prev = 0;

				pos[0] = is_indep_expr (expr->binary.left);
				pos[1] = is_indep_expr (expr->binary.right);

				for (i= 0;  i< 2; i++) {
					if(pos[i] < 0){
						return -1;
					} else if(pos[i] > 0){
						if( prev!= 0 && pos[i] != prev )
							return -1;	
						prev = pos[i];
					}
				}
				return prev;
			}

		/*****************/
		/* function call */
		/*****************/
		case nel_O_FUNCALL:
			{
				register nel_expr_list *list;
				int prev = 0;
				int pos; 

				prev = is_indep_expr(expr->funcall.func);
				if(prev < 0 )
					return -1;
				
				for (list=expr->funcall.args; list!=NULL; list=list->next) {
					pos = is_indep_expr(list->expr);
					if(pos < 0){
						return -1;
					} else if(pos > 0){
						if( prev!= 0 && pos != prev )return -1;	
						prev = pos;
					}
				}
				return prev;
			}
			break;

		/***********************/
		/* struct/union member */
		/***********************/
		case nel_O_MEMBER:
			return is_indep_expr(expr->member.s_u);

		/**********************************************/
		/* type epxression: sizeof, typeof, or a cast */
		/**********************************************/
		case nel_O_CAST:
		case nel_O_SIZEOF:
		case nel_O_TYPEOF:
			return is_indep_expr(expr->type.expr);

		/******************/
		/* conditional: ? */
		/******************/
		case nel_O_COND:
			{
				int pos[3];
				int i;
				int prev = 0;

				pos[0] = is_indep_expr (expr->cond.cond);
				pos[1] = is_indep_expr(expr->cond.true_expr); 	
				pos[2] = is_indep_expr(expr->cond.false_expr); 	

				for (i= 0;  i< 3; i++) {
					if(pos[i] < 0){
						return -1;
					} else if(pos[i] > 0){
						if( prev!= 0 && pos[i] != prev )
							return -1;	
						prev = pos[i];
					}
				}
				return prev;
			}

		/*********************************************************/
		/* the following should never occur as the discriminator */
		/* of the nel_eval_expr union -                      */
		/* they are only used for error messages.                */
		/*********************************************************/
		case nel_O_MEMBER_PTR:
		case nel_O_RETURN:
		default:
			return -1;
		}
	}

	return 0;

}


nel_expr *nel_dup_expr(struct nel_eng *eng, register nel_expr *expr)
{
	nel_expr *retval =NULL;
	if (expr != NULL){

		switch (expr->gen.opcode) {

		/****************************/
		/* terminal node - a symbol */
		/****************************/
		case nel_O_SYMBOL:
			{	
				nel_symbol *new, *old = expr->symbol.symbol;
				if(old->class == nel_C_TERMINAL 
					|| old->class == nel_C_NONTERMINAL) {
					new = old;
				}else {
					new = nel_symbol_dup(eng, old); 
				}

				retval = nel_expr_alloc(eng, nel_O_SYMBOL, new);

			}
			break;

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
			retval = nel_dup_expr(eng, expr->unary.operand);	
			retval = nel_expr_alloc(eng, expr->gen.opcode, retval);
			break;

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
		case nel_O_COMPOUND:
		case nel_O_DIV:
		case nel_O_DIV_ASGN:
		case nel_O_EQ:
		case nel_O_GE:
		case nel_O_GT:
		case nel_O_LE:
		case nel_O_LSHIFT:
		case nel_O_LSHIFT_ASGN:
		case nel_O_LT:
		case nel_O_MOD:
		case nel_O_MOD_ASGN:
		case nel_O_MULT:
		case nel_O_MULT_ASGN:
		case nel_O_NE:
		case nel_O_OR:
		case nel_O_RSHIFT:
		case nel_O_RSHIFT_ASGN:
		case nel_O_SUB:
		case nel_O_SUB_ASGN:
			{
				nel_expr *left = nel_dup_expr(eng, expr->binary.left);	
				nel_expr *right = nel_dup_expr(eng, expr->binary.right);	
				retval = nel_expr_alloc(eng, expr->gen.opcode, left, right);
			}
			break;

		/*****************/
		/* function call */
		/*****************/
		case nel_O_FUNCALL:
			{
				int i;
				register nel_expr_list *list,
					*pnlist=NULL, *nlist=NULL, *args=NULL;

				nel_expr *func = nel_dup_expr(eng, expr->funcall.func);
				for (i=0,list=expr->funcall.args; list!=NULL; i++,list=list->next) {
					nel_expr *arg = nel_dup_expr(eng, list->expr);
					nlist = nel_expr_list_alloc( eng, arg, NULL);	
					if(!args) 
						args = nlist;
					if(pnlist)
						pnlist->next = nlist;
					pnlist = nlist;
				}
				
				retval = nel_expr_alloc(eng, expr->gen.opcode, func, i, args);

			}
			break;

		/***********************/
		/* struct/union member */
		/***********************/
		case nel_O_MEMBER:
			{
				nel_expr *ns_u = nel_dup_expr(eng, expr->member.s_u);
				nel_symbol *member = expr->member.member;	
				nel_symbol *nmember= nel_static_symbol_alloc(eng, member->name, member->type,member->value, member->class, member->lhs, member->source_lang, member->level);
				retval = nel_expr_alloc(eng, expr->gen.opcode, ns_u, nmember);

			}
			break;

		/**********************************************/
		/* type epxression: sizeof, typeof, or a cast */
		/**********************************************/
		case nel_O_CAST:
		case nel_O_SIZEOF:
		case nel_O_TYPEOF:
			{
				nel_expr *nexpr = nel_dup_expr(eng, expr->type.expr); 
				retval = nel_expr_alloc(eng, expr->gen.opcode, expr->type.type, nexpr);	
			}
			break;

		/******************/
		/* conditional: ? */
		/******************/
		case nel_O_COND:
			{
				nel_expr *ncond, *ntrue, *nfalse;	
				ncond = nel_dup_expr(eng, expr->cond.cond);
				ntrue = nel_dup_expr(eng, expr->cond.true_expr);
				nfalse = nel_dup_expr(eng, expr->cond.false_expr);
				retval = nel_expr_alloc(eng, expr->gen.opcode, ncond, ntrue, nfalse);
			}
			break;

		/*********************************************************/
		/* the following should never occur as the discriminator */
		/* of the nel_eval_expr union -                      */
		/* they are only used for error messages.                */
		/*********************************************************/
		case nel_O_MEMBER_PTR:
		case nel_O_RETURN:
		default:
			break;
		}
	}

	return retval;
}

void emit_expr(FILE *fp, union nel_EXPR *expr)
{
	if(expr != NULL){

		switch (expr->gen.opcode) {

		/****************************/
		/* terminal node - a symbol */
		/****************************/
		case nel_O_SYMBOL:
			emit_symbol(fp,expr->symbol.symbol);
			break;


		/********************/
		/* unary operations */
		/********************/
		case nel_O_ADDRESS:
		case nel_O_BIT_NEG:
		case nel_O_NEG:
		case nel_O_NOT:
		case nel_O_POS:
		case nel_O_POST_DEC:
		case nel_O_POST_INC:
		case nel_O_PRE_DEC:
		case nel_O_PRE_INC:
			fprintf(fp, "%c", *((char *)nel_O_string(expr->gen.opcode)));
			emit_expr (fp, expr->unary.operand);
			break;

		case nel_O_DEREF:
			{
				emit_expr (fp, expr->unary.operand);
			}
			break;

		/*********************/
		/* binary operations */
		/*********************/
		case nel_O_ADD:
		case nel_O_ADD_ASGN:
		case nel_O_AND:
		case nel_O_BIT_AND:
		case nel_O_BIT_AND_ASGN:
		case nel_O_BIT_OR:
		case nel_O_BIT_OR_ASGN:
		case nel_O_BIT_XOR:
		case nel_O_BIT_XOR_ASGN:
		case nel_O_DIV:
		case nel_O_DIV_ASGN:
		case nel_O_GE:
		case nel_O_LE:
		case nel_O_LSHIFT:
		case nel_O_LSHIFT_ASGN:
		case nel_O_LT:
		case nel_O_MOD:
		case nel_O_MOD_ASGN:
		case nel_O_MULT:
		case nel_O_MULT_ASGN:
		case nel_O_NE:
		case nel_O_OR:
		case nel_O_RSHIFT:
		case nel_O_RSHIFT_ASGN:
		case nel_O_SUB:
		case nel_O_SUB_ASGN:
		case nel_O_EQ:
		case nel_O_GT:
		case nel_O_ASGN:
			emit_expr (fp, expr->binary.left);
			fprintf(fp, "%s", nel_O_string(expr->gen.opcode));
			emit_expr (fp, expr->binary.right);
			break;

		case nel_O_ARRAY_INDEX:
			emit_expr (fp, expr->binary.left);
			fprintf(fp, "[");
			emit_expr (fp, expr->binary.right);
			fprintf(fp, "]");
			break;

		case nel_O_COMPOUND:
			emit_expr (fp, expr->binary.left);
			fprintf(fp, ", ");
			emit_expr (fp, expr->binary.right);
			break;

		/*****************/
		/* function call */
		/*****************/
		case nel_O_FUNCALL:
			{
				register nel_expr_list *list;
				int first_arg; 
				nel_symbol *symbol = expr->funcall.func->symbol.symbol;
				fprintf(fp, "%s(", symbol->name);
				for (list=expr->funcall.args, first_arg = 1; list!=NULL; list=list->next, first_arg = 0) {
					if(!first_arg) {
						fprintf(fp, ",");
					}
					emit_expr(fp, list->expr);
				}
				fprintf(fp, ")");
			}
			break;

		/***********************/
		/* struct/union member */
		/***********************/
		case nel_O_MEMBER:
			emit_expr (fp, expr->member.s_u);
			if(expr->member.s_u->gen.opcode == nel_O_DEREF)
				fprintf(fp, "->");
			else 
				fprintf(fp, ".");
			
			emit_symbol(fp, expr->member.member);
			break;

		/**********************************************/
		/* type epxression: sizeof, typeof, or a cast */
		/**********************************************/
		case nel_O_CAST:
		case nel_O_SIZEOF:
		case nel_O_TYPEOF:
			emit_expr (fp, expr->type.expr);
			break;

		/******************/
		/* conditional: ? */
		/******************/
		case nel_O_COND:
			fprintf (fp, "if(");
			emit_expr (fp, expr->cond.cond);
			fprintf (fp, "){\n");
			emit_expr (fp, expr->cond.true_expr);
			fprintf (fp, "}\nelse{\n");
			emit_expr (fp, expr->cond.false_expr);
			fprintf (fp, "}\n");
			break;

		/*********************************************************/
		/* the following should never occur as the discriminator */
		/* of the nel_eval_expr union -                      */
		/* they are only used for error messages.                */
		/*********************************************************/
		case nel_O_MEMBER_PTR:
		case nel_O_RETURN:
		default:
			break;
		}
	}
}

//find if a expr contains a function call, 1 for yes, 0 for no
int expr_contain_funcall(struct nel_eng *eng, nel_expr *expr)
{
	nel_expr_list *list=NULL;
	
	if(!expr) {
		parser_stmt_error(eng, "NULL expr");
		return 0;
	}

	if(expr->gen.opcode == nel_O_FUNCALL)
		return 1;

	if(expr->gen.opcode == nel_O_SYMBOL || expr->gen.opcode == nel_O_NEL_SYMBOL)
		return 0;
	
	switch(expr->gen.opcode)
	{
	case nel_O_ADD:
	case nel_O_ADD_ASGN:
	case nel_O_AND:
	case nel_O_ASGN:
	case nel_O_ARRAY_INDEX:
	case nel_O_BIT_AND:
	case nel_O_BIT_AND_ASGN:
	case nel_O_BIT_OR:
	case nel_O_BIT_OR_ASGN:
	case nel_O_BIT_XOR:
	case nel_O_BIT_XOR_ASGN:
	case nel_O_COMPOUND:
	case nel_O_DIV:
	case nel_O_DIV_ASGN:
	case nel_O_EQ:
	case nel_O_GE:
	case nel_O_GT:
	case nel_O_LE:
	case nel_O_LSHIFT:
	case nel_O_LSHIFT_ASGN:
	case nel_O_LT:
	case nel_O_MULT:
	case nel_O_MULT_ASGN:
	case nel_O_MOD:
	case nel_O_MOD_ASGN:
	case nel_O_NE:
	case nel_O_OR:
	case nel_O_RSHIFT:
	case nel_O_RSHIFT_ASGN:
	case nel_O_SUB:
	case nel_O_SUB_ASGN:
		return expr_contain_funcall(eng, expr->binary.left) || expr_contain_funcall(eng, expr->binary.right);
		break;
	case nel_O_ADDRESS:
	case nel_O_BIT_NEG:
	case nel_O_DEREF:
	case nel_O_NOT:
	case nel_O_NEG:
	case nel_O_POS:
	case nel_O_PRE_INC:
	case nel_O_POST_INC:
	case nel_O_POST_DEC:
	case nel_O_PRE_DEC:
		return expr_contain_funcall(eng, expr->unary.operand);
		break;
	case nel_O_CAST:
	case nel_O_SIZEOF:
	case nel_O_TYPEOF:
		return expr_contain_funcall(eng, expr->type.expr);
		break;
	case nel_O_MEMBER:
	case nel_O_MEMBER_PTR:
		return expr_contain_funcall(eng, expr->member.s_u);
		break;
	case nel_O_COND:
		return expr_contain_funcall(eng, expr->cond.cond) || expr_contain_funcall(eng, expr->cond.true_expr) || expr_contain_funcall(eng, expr->cond.false_expr);
		break;
	case nel_O_LIST:
		list = expr->list.list;
		while(list)
		{
			if(expr_contain_funcall(eng, list->expr))
				return 1;
			list = list->next;
		}
		return 0;
		break;
	default:
		parser_stmt_error(eng, "illegal expr");
		return 0;
	}
}

//evaluate the type of a expression
nel_type* eval_expr_type(struct nel_eng *eng, nel_expr *expr)
{
	nel_type *lt=NULL;
	nel_type *rt=NULL;
	nel_type *temp_type = NULL;
	nel_member *member=NULL;
	nel_expr *e = NULL;
	nel_list *list=NULL;
	nel_symbol *symbol=NULL;
	nel_expr_list *expr_list=NULL;
	
	if(!expr) {
		parser_stmt_error(eng, "NULL expr");
		return NULL;
	}

	if(expr->gen.opcode == nel_O_SYMBOL || expr->gen.opcode == nel_O_NEL_SYMBOL)
		return expr->symbol.symbol->type;
	
	switch(expr->gen.opcode)
	{
	case nel_O_ADD:
	case nel_O_SUB:
	case nel_O_MULT:
	case nel_O_DIV:
		lt = eval_expr_type(eng, expr->binary.left);
		if(!lt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		rt = eval_expr_type(eng, expr->binary.right);
		if(!rt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		rt = intp_type_binary_op(eng, lt, rt, 0, 1);
		if(!rt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		return rt;
	case nel_O_ASGN:
		lt = eval_expr_type(eng, expr->binary.left);
		if(!lt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		rt = eval_expr_type(eng, expr->binary.right);
		if(!rt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		if(!is_asgn_compatible(eng, lt, rt))
		{
			parser_stmt_error(eng, "illegal expr: assignment incompatible");
			return NULL;
		}
		rt = intp_type_binary_op(eng, lt, rt, 0, 1);
		if(!rt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		return lt;
	case nel_O_ADD_ASGN:
	case nel_O_SUB_ASGN:
	case nel_O_MULT_ASGN:
	case nel_O_DIV_ASGN:
	case nel_O_MOD_ASGN:
		lt = eval_expr_type(eng, expr->binary.left);
		if(!lt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		rt = eval_expr_type(eng, expr->binary.right);
		if(!rt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		rt = intp_type_binary_op(eng, lt, rt, 0, 1);
		if(!rt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		if(is_asgn_compatible(eng, lt, rt))
			return lt;
		else
		{
			parser_stmt_error(eng, "illegal expr: assignment incompatible");
			return NULL;
		}
	case nel_O_NE:
	case nel_O_EQ:
	case nel_O_GE:
	case nel_O_GT:
	case nel_O_LE:
	case nel_O_LT:
	case nel_O_AND:
	case nel_O_OR:
		lt = eval_expr_type(eng, expr->binary.left);
		if(!lt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		rt = eval_expr_type(eng, expr->binary.right);
		if(!rt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		rt = intp_type_binary_op(eng, lt, rt, 0, 1);
		if(!rt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		return nel_signed_int_type;
	case nel_O_NOT:
		lt = eval_expr_type(eng, expr->unary.operand);
		if(!lt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		return nel_signed_int_type;
	case nel_O_POS:
	case nel_O_NEG:
		lt = eval_expr_type(eng, expr->unary.operand);
		if(!lt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		lt = intp_type_unary_op(eng, lt, 0, 1);	
		if(!lt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		return lt;
	case nel_O_MOD:
	case nel_O_BIT_AND:
	case nel_O_BIT_OR:
	case nel_O_BIT_XOR:
		lt = eval_expr_type(eng, expr->binary.left);
		if(!lt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		rt = eval_expr_type(eng, expr->binary.right);
		if(!rt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		rt = intp_type_binary_op(eng, lt, rt, 0, 1);
		if(!rt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		if(!nel_integral_D_token(rt->simple.type))
		{
			parser_stmt_error(eng, "illegal expr: integer operand needed");
			return NULL;
		}
		return rt;
	case nel_O_BIT_NEG:
		rt = eval_expr_type(eng, expr->unary.operand);
		if(!rt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		if(!nel_integral_D_token(rt->simple.type))
		{
			parser_stmt_error(eng, "illegal expr: integer operand needed");
			return NULL;
		}
		return rt;
	case nel_O_BIT_AND_ASGN:
	case nel_O_BIT_OR_ASGN:
	case nel_O_BIT_XOR_ASGN:
		lt = eval_expr_type(eng, expr->binary.left);
		if(!lt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		rt = eval_expr_type(eng, expr->binary.right);
		if(!rt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		rt = intp_type_binary_op(eng, lt, rt, 0, 1);
		if(!rt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		if(!nel_integral_D_token(rt->simple.type))
		{
			parser_stmt_error(eng, "illegal expr: integer operand needed");
			return NULL;
		}
		if(!is_asgn_compatible(eng, lt, rt))
		{
			parser_stmt_error(eng, "illegal expr: assignment incompatible");
			return NULL;
		}
		return lt;
	case nel_O_MEMBER:
		lt = eval_expr_type(eng, expr->member.s_u);
		if(!lt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		if(lt->simple.type != nel_D_STRUCT && lt->simple.type != nel_D_UNION)
		{
			parser_stmt_error(eng, "illegal expr: struct/union type needed");
			return NULL;
		}
		member = lt->s_u.members;
		if(!expr->member.member->name)
		{
			parser_stmt_error(eng, "illegal expr: NULL member name");
			return NULL;
		}
		while(member)
		{
			if(strcmp(member->symbol->name, expr->member.member->name))
				member = member->next;
			else
				break;
		}
		if(!member)
		{
			parser_stmt_error(eng, "illegal expr: no member %s", expr->member.member->name);
			return NULL;
		}
		return member->symbol->type;
	case nel_O_SIZEOF:
		if(!expr->type.type)
		{
			lt = eval_expr_type(eng, expr->type.expr);
			if(!lt)
			{
				parser_stmt_error(eng, "illegal expr");
				return NULL;
			}
		}
		return nel_unsigned_int_type;
	case nel_O_CAST:
		lt = eval_expr_type(eng, expr->type.expr);
		if(!lt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		return expr->type.type;
	case nel_O_TYPEOF:
		lt = eval_expr_type(eng, expr->type.expr);
		if(!lt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		return lt;
	case nel_O_ARRAY_INDEX:
		lt = eval_expr_type(eng, expr->binary.left);
		if(!lt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		if(lt->simple.type != nel_D_ARRAY && lt->simple.type != nel_D_POINTER)
		{
			parser_stmt_error(eng, "illegal expr: array/pointer type needed");
			return NULL;
		}
		rt = eval_expr_type(eng, expr->binary.right);
		if(!rt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		if(!nel_integral_D_token(rt->simple.type))
		{
			parser_stmt_error(eng, "illegal expr: integeral needed as array index");
			return NULL;
		}
		if(lt->simple.type == nel_D_ARRAY)
			return lt->array.base_type;
		else
			return lt->pointer.deref_type;
	case nel_O_LSHIFT:
	case nel_O_RSHIFT:
	case nel_O_LSHIFT_ASGN:
	case nel_O_RSHIFT_ASGN:
		lt = eval_expr_type(eng, expr->binary.left);
		if(!lt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		rt = eval_expr_type(eng, expr->binary.right);
		if(!rt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		if(!nel_integral_D_token(lt->simple.type) || !nel_integral_D_token(rt->simple.type))
		{
			parser_stmt_error(eng, "illegal expr: shift need intergral operands");
			return NULL;
		}
		return lt;
	case nel_O_ADDRESS:
		lt = eval_expr_type(eng, expr->unary.operand);
		if(!lt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		rt = nel_type_alloc(eng, nel_D_POINTER, sizeof(char*), nel_alignment_of(char*), lt->simple._const, lt->simple._volatile, lt);
		return rt;
	case nel_O_DEREF:
		lt = eval_expr_type(eng, expr->unary.operand);
		if(!lt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		if(lt->simple.type!=nel_D_POINTER && lt->simple.type!=nel_D_ARRAY)
		{
			parser_stmt_error(eng, "illegal expr: operator * need pointer type");
			return NULL;
		}
		if(lt->simple.type == nel_D_POINTER)
			return lt->pointer.deref_type;
		else
			return lt->array.base_type;
		break;
		
	case nel_O_FUNCALL:

		expr_list = expr->funcall.args;
		lt = eval_expr_type(eng, expr->funcall.func);
		if(!lt || lt->simple.type != nel_D_FUNCTION) {	
			parser_stmt_error(eng, "illegal expr: bad func");
			return NULL;
		}

		list = lt->function.args;
		while(list) {
			if(!expr_list) {
				parser_stmt_error(eng, "illegal expr: less arguments than function definition");
				return NULL;
			}
			rt = eval_expr_type(eng, expr_list->expr);
			if(!rt) {
				parser_stmt_error(eng, "illegal expr: bad argument %s", list->symbol->name);
				return NULL;
			}

			if(rt->simple.type == nel_D_EVENT) {
				if (list->symbol->type->simple.type != nel_D_EVENT){
					parser_stmt_error(eng, "illegal expr: pass argument %s type not match", list->symbol->name);
					return NULL;
				}
				
			}else if(!is_asgn_compatible(eng, list->symbol->type, rt)) {
				parser_stmt_error(eng, "illegal expr: pass argument %s type not match", list->symbol->name);
				return NULL;
			}
			list = list->next;
			expr_list = expr_list->next;
		}
		if(expr_list && !lt->function.var_args)
		{
			parser_stmt_error(eng, "illegal expr: more arguments than function definition");
			return NULL;
		}

		return lt->function.return_type;
		break;

	case nel_O_COND:
		lt = eval_expr_type(eng, expr->cond.cond);
		if(!lt)
		{
			parser_stmt_error(eng, "illegal expr: cond");
			return NULL;
		}
		if(!nel_numerical_D_token(lt->simple.type) && lt->simple.type != nel_D_POINTER)
		{
			parser_stmt_error(eng, "illegal expr: scalar needed in cond");
			return NULL;
		}
		lt = eval_expr_type(eng, expr->cond.true_expr);
		if(!lt)
		{
			parser_stmt_error(eng, "illegal expr: true_expr");
			return NULL;
		}
		rt = eval_expr_type(eng, expr->cond.false_expr);
		if(!rt)
		{
			parser_stmt_error(eng, "illegal expr: false_expr");
			return NULL;
		}
		temp_type = intp_type_binary_op(eng, lt, rt, 0, 1);
		if(!temp_type)
		{
			if(nel_type_diff(lt, rt, 1))
			{
				parser_stmt_error(eng, "illegal expr: true_expr false_expr type not match");
				return NULL;
			}
			else
				return lt;
		}
		return temp_type;
		break;
	
	case nel_O_COMPOUND:
		lt = eval_expr_type(eng, expr->binary.left);
		if(!lt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		rt = eval_expr_type(eng, expr->binary.right);
		if(!rt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		return rt;
		break;
	case nel_O_POST_DEC:
	case nel_O_POST_INC:
	case nel_O_PRE_INC:
	case nel_O_PRE_DEC:
		lt = eval_expr_type(eng, expr->unary.operand);
		if(!lt)
		{
			parser_stmt_error(eng, "illegal expr");
			return NULL;
		}
		if(!nel_numerical_D_token(lt->simple.type) && lt->simple.type != nel_D_POINTER)
		{
			parser_stmt_error(eng, "illegal expr: numerical type or pointer needed");
			return NULL;
		}
		return lt;
	default:
		return NULL;
	}
}

//call nel_dealloca to free expr chunk link, 
//after calling this function, all expr pointer should be illegal!!!!!!
void expr_dealloc(struct nel_eng *eng)
{
	while(nel_expr_chunks) {
		nel_expr_chunk *chunk = nel_expr_chunks->next;
		nel_dealloca(nel_expr_chunks->start); 
		nel_dealloca(nel_expr_chunks); 
		nel_expr_chunks = chunk;
	}
	nel_exprs_next = NULL;
	nel_exprs_end = NULL;
	nel_free_exprs = NULL;
}

//call nel_dealloca to free expr list chunk link, 
//after calling this function, all expr list pointer should be illegal!!!!!!
void expr_list_dealloc(struct nel_eng *eng)
{
	while(nel_expr_list_chunks) {
		nel_expr_list_chunk *chunk = nel_expr_list_chunks->next;
		nel_dealloca(nel_expr_list_chunks->start); 
		nel_dealloca(nel_expr_list_chunks); 
		nel_expr_list_chunks = chunk;
	}
	nel_expr_lists_next = NULL;
	nel_expr_lists_end = NULL;
	nel_free_expr_lists = NULL;
}

