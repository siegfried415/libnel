

/*****************************************************************************/
/* This file, "expr.h", contains declarations */
/*****************************************************************************/



#ifndef EXPR_H
#define EXPR_H
#include <engine.h>



/**********************************************************/
/* operator type constants.  they begin with "nel_O_" */
/* for Operation, and are called "nel_O_tokens".      */
/**********************************************************/
typedef enum nel_O_TOKEN {
        nel_O_NULL = 0,
        nel_O_ADD,		/* x + y			*/
        nel_O_ADD_ASGN,		/* x += y			*/
        nel_O_ADDRESS,		/* &x				*/
        nel_O_AND,		/* x && y			*/
        nel_O_ASGN,		/* x = y			*/
        nel_O_ARRAY_INDEX,	/* x[y]				*/
        nel_O_ARRAY_RANGE,	/* x[y,z] range,wyong, 2006.6.22*/
        nel_O_BIT_AND,		/* x & y			*/
        nel_O_BIT_AND_ASGN,	/* x &= y			*/
        nel_O_BIT_OR,		/* x | y			*/
        nel_O_BIT_OR_ASGN,	/* x |= y			*/
        nel_O_BIT_NEG,		/* ~y				*/
        nel_O_BIT_XOR,		/* x ^ y			*/
        nel_O_BIT_XOR_ASGN,	/* x ^= y			*/
        nel_O_CAST,		/* (<type>) x			*/
        nel_O_COMPOUND,		/* x , y			*/
        nel_O_COND,		/* x ? y : z     		*/
        nel_O_DEREF,		/* *x				*/
        nel_O_DIV,		/* x / y			*/
        nel_O_DIV_ASGN,		/* x /= y			*/
        nel_O_EQ,		/* x == y			*/
        nel_O_FUNCALL,		/* x (y)			*/
        nel_O_GE,		/* x >= y			*/
        nel_O_GT,		/* x > y			*/
        nel_O_LE,		/* x <= y			*/
        nel_O_LSHIFT,		/* x << y			*/
        nel_O_LSHIFT_ASGN,	/* x <<= y			*/
        nel_O_LT,		/* x < y			*/
        nel_O_MEMBER,		/* x.y				*/
        nel_O_MEMBER_PTR,	/* x->y				*/
        nel_O_MOD,		/* x % y			*/
        nel_O_MOD_ASGN,		/* x %= y			*/
        nel_O_MULT,		/* x * y			*/
        nel_O_MULT_ASGN,		/* x *= y			*/
        nel_O_NE,		/* x != y			*/
        nel_O_NEG,		/* -x				*/
        nel_O_NOT,		/* ! x				*/
        nel_O_OR,		/* x || y			*/
        nel_O_POST_DEC,		/* x--				*/
        nel_O_POST_INC,		/* x++				*/
        nel_O_POS,		/* +x				*/
        nel_O_PRE_DEC,		/* --x				*/
        nel_O_PRE_INC,		/* ++x				*/
        nel_O_RETURN,		/* return x			*/
        nel_O_RSHIFT,		/* x >> y			*/
        nel_O_RSHIFT_ASGN,	/* x >>= y			*/
        nel_O_SIZEOF,		/* sizeof x / sizeof (<type>)	*/
        nel_O_SUB,		/* x - y			*/
        nel_O_SUB_ASGN,		/* x -= y			*/
        nel_O_SYMBOL,		/* x (by itself)		*/
        nel_O_NEL_SYMBOL,	/* NEL symbol		*/
        nel_O_TYPEOF,		/* typeof x / typeof (<type>)	*/
	nel_O_LIST,		/* expr list, wyong, 2005.10.10 */
        nel_O_MAX
} nel_O_token;


/***********************************************/
/* use the following macros to tell what kind  */
/* of operation an nel_O_token represents. */
/***********************************************/
#define nel_unary_O_token(_x) \
	(((_x) == nel_O_ADDRESS)					\
	|| ((_x) == nel_O_BIT_NEG)					\
	|| ((_x) == nel_O_CAST)					\
	|| ((_x) == nel_O_FUNCALL)					\
	|| ((_x) == nel_O_NEG)					\
	|| ((_x) == nel_O_NOT)					\
	|| ((_x) == nel_O_POST_DEC)					\
	|| ((_x) == nel_O_POST_INC)					\
	|| ((_x) == nel_O_POS)					\
	|| ((_x) == nel_O_PRE_DEC)					\
	|| ((_x) == nel_O_PRE_INC)					\
	|| ((_x) == nel_O_SIZEOF))

#define nel_prefix_O_token(_x) \
	(((_x) == nel_O_ADDRESS)					\
	|| ((_x) == nel_O_BIT_NEG)					\
	|| ((_x) == nel_O_CAST)					\
	|| ((_x) == nel_O_NEG)					\
	|| ((_x) == nel_O_NOT)					\
	|| ((_x) == nel_O_POS)					\
	|| ((_x) == nel_O_PRE_DEC)					\
	|| ((_x) == nel_O_PRE_INC)					\
	|| ((_x) == nel_O_SIZEOF))

#define nel_postfix_O_token(_x) \
	(((_x) == nel_O_POST_DEC)					\
	|| ((_x) == nel_O_POST_INC))

#define nel_binary_O_token(_x) \
	(((_x) == nel_O_ADD)					\
	|| ((_x) == nel_O_ADD_ASGN)					\
	|| ((_x) == nel_O_AND)					\
	|| ((_x) == nel_O_ASGN)					\
	|| ((_x) == nel_O_ARRAY_INDEX)				\
	|| ((_x) == nel_O_BIT_AND)					\
	|| ((_x) == nel_O_BIT_AND_ASGN)				\
	|| ((_x) == nel_O_BIT_OR)					\
	|| ((_x) == nel_O_BIT_OR_ASGN)				\
	|| ((_x) == nel_O_BIT_XOR)					\
	|| ((_x) == nel_O_BIT_XOR_ASGN)				\
	|| ((_x) == nel_O_COMPOUND)					\
	|| ((_x) == nel_O_DIV)					\
	|| ((_x) == nel_O_DIV_ASGN)					\
	|| ((_x) == nel_O_EQ)					\
	|| ((_x) == nel_O_GE)					\
	|| ((_x) == nel_O_GT)					\
	|| ((_x) == nel_O_LE)					\
	|| ((_x) == nel_O_LSHIFT)					\
	|| ((_x) == nel_O_LSHIFT_ASGN)				\
	|| ((_x) == nel_O_LT)					\
	|| ((_x) == nel_O_MEMBER)					\
	|| ((_x) == nel_O_MEMBER_PTR)				\
	|| ((_x) == nel_O_MOD)					\
	|| ((_x) == nel_O_MOD_ASGN)					\
	|| ((_x) == nel_O_MULT)					\
	|| ((_x) == nel_O_MULT_ASGN)				\
	|| ((_x) == nel_O_NE)					\
	|| ((_x) == nel_O_OR)					\
	|| ((_x) == nel_O_RSHIFT)					\
	|| ((_x) == nel_O_RSHIFT_ASGN)				\
	|| ((_x) == nel_O_SUB)					\
	|| ((_x) == nel_O_SUB_ASGN))

/*************************************************/
/* operations which allow only integral operands */
/*************************************************/
#define nel_integral_O_token(_x) \
	(((_x) == nel_O_BIT_AND)					\
	|| ((_x) == nel_O_BIT_AND_ASGN)				\
	|| ((_x) == nel_O_BIT_NEG)					\
	|| ((_x) == nel_O_BIT_OR)					\
	|| ((_x) == nel_O_BIT_OR_ASGN)				\
	|| ((_x) == nel_O_BIT_XOR)					\
	|| ((_x) == nel_O_BIT_XOR_ASGN)				\
	|| ((_x) == nel_O_LSHIFT)					\
	|| ((_x) == nel_O_LSHIFT_ASGN)				\
	|| ((_x) == nel_O_MOD)					\
	|| ((_x) == nel_O_MOD_ASGN)					\
	|| ((_x) == nel_O_RSHIFT)					\
	|| ((_x) == nel_O_RSHIFT_ASGN))

/*****************************************************/
/* operations which allow both integral and floating */
/* point (and possibly pointer) operands.            */
/*****************************************************/
#define nel_arithmetic_O_token(_x) \
	(((_x) == nel_O_ADD)					\
	|| ((_x) == nel_O_ADD_ASGN)					\
	|| ((_x) == nel_O_DIV)					\
	|| ((_x) == nel_O_DIV_ASGN)					\
	|| ((_x) == nel_O_EQ)					\
	|| ((_x) == nel_O_GE)					\
	|| ((_x) == nel_O_GT)					\
	|| ((_x) == nel_O_LE)					\
	|| ((_x) == nel_O_LT)					\
	|| ((_x) == nel_O_MULT)					\
	|| ((_x) == nel_O_MULT_ASGN)				\
	|| ((_x) == nel_O_NE)					\
	|| ((_x) == nel_O_NEG)					\
	|| ((_x) == nel_O_POS)					\
	|| ((_x) == nel_O_POST_DEC)					\
	|| ((_x) == nel_O_POST_INC)					\
	|| ((_x) == nel_O_PRE_DEC)					\
	|| ((_x) == nel_O_PRE_INC)					\
	|| ((_x) == nel_O_SUB)					\
	|| ((_x) == nel_O_SUB_ASGN))

/********************************************/
/* operations which allow complex operands. */
/********************************************/
#define nel_complex_O_token(_x) \
	(((_x) == nel_O_ADD)					\
	|| ((_x) == nel_O_ADD_ASGN)					\
	|| ((_x) == nel_O_DIV)					\
	|| ((_x) == nel_O_DIV_ASGN)					\
	|| ((_x) == nel_O_EQ)					\
	|| ((_x) == nel_O_MULT)					\
	|| ((_x) == nel_O_MULT_ASGN)				\
	|| ((_x) == nel_O_NE)					\
	|| ((_x) == nel_O_NEG)					\
	|| ((_x) == nel_O_POS)					\
	|| ((_x) == nel_O_SUB)					\
	|| ((_x) == nel_O_SUB_ASGN))

/****************************************/
/* any operator which can be applied to */
/* an operand of any numerical type.    */
/****************************************/
#define nel_numerical_O_token(_x) \
	(((_x) == nel_O_ADD)					\
	|| ((_x) == nel_O_ADD_ASGN)					\
	|| ((_x) == nel_O_BIT_AND)					\
	|| ((_x) == nel_O_BIT_AND_ASGN)				\
	|| ((_x) == nel_O_BIT_NEG)					\
	|| ((_x) == nel_O_BIT_OR)					\
	|| ((_x) == nel_O_BIT_OR_ASGN)				\
	|| ((_x) == nel_O_BIT_XOR)					\
	|| ((_x) == nel_O_BIT_XOR_ASGN)				\
	|| ((_x) == nel_O_DIV)					\
	|| ((_x) == nel_O_DIV_ASGN)					\
	|| ((_x) == nel_O_EQ)					\
	|| ((_x) == nel_O_GE)					\
	|| ((_x) == nel_O_GT)					\
	|| ((_x) == nel_O_LE)					\
	|| ((_x) == nel_O_LSHIFT)					\
	|| ((_x) == nel_O_LSHIFT_ASGN)				\
	|| ((_x) == nel_O_LT)					\
	|| ((_x) == nel_O_MOD)					\
	|| ((_x) == nel_O_MOD_ASGN)					\
	|| ((_x) == nel_O_MULT)					\
	|| ((_x) == nel_O_MULT_ASGN)				\
	|| ((_x) == nel_O_NE)					\
	|| ((_x) == nel_O_NEG)					\
	|| ((_x) == nel_O_POS)					\
	|| ((_x) == nel_O_POST_DEC)					\
	|| ((_x) == nel_O_POST_INC)					\
	|| ((_x) == nel_O_PRE_DEC)					\
	|| ((_x) == nel_O_PRE_INC)					\
	|| ((_x) == nel_O_RSHIFT)					\
	|| ((_x) == nel_O_RSHIFT_ASGN)				\
	|| ((_x) == nel_O_SUB)					\
	|| ((_x) == nel_O_SUB_ASGN))

#define nel_relational_O_token(_x) \
	(((_x) == nel_O_EQ)						\
	|| ((_x) == nel_O_GE)					\
	|| ((_x) == nel_O_GT)					\
	|| ((_x) == nel_O_LE)					\
	|| ((_x) == nel_O_LT)					\
	|| ((_x) == nel_O_NE))

#define nel_inc_dec_O_token(_x) \
	(((_x) == nel_O_POST_DEC)					\
	|| ((_x) == nel_O_POST_INC)					\
	|| ((_x) == nel_O_PRE_DEC)					\
	|| ((_x) == nel_O_PRE_INC))

#define nel_asgn_O_token(_x) \
	(((_x) == nel_O_ADD_ASGN)					\
	|| ((_x) == nel_O_ASGN)					\
	|| ((_x) == nel_O_BIT_AND_ASGN)				\
	|| ((_x) == nel_O_BIT_OR_ASGN)				\
	|| ((_x) == nel_O_BIT_XOR_ASGN)				\
	|| ((_x) == nel_O_DIV_ASGN)					\
	|| ((_x) == nel_O_LSHIFT_ASGN)				\
	|| ((_x) == nel_O_MOD_ASGN)					\
	|| ((_x) == nel_O_MULT_ASGN)				\
	|| ((_x) == nel_O_RSHIFT_ASGN)				\
	|| ((_x) == nel_O_SUB_ASGN)					\
	|| (nel_inc_dec_O_token (_x)))




/*********************************************************/
/* structures which form of expression trees.            */
/* nel_O_tokens are the discriminator for the union. */
/*********************************************************/
typedef union nel_EXPR {

        /*********************/
        /* generic structure */
        /*********************/
        struct {
                nel_O_token opcode;
                union nel_EXPR *next;		/* for use in free list */
        }
        gen;

	struct {
		nel_O_token opcode;
		struct nel_EXPR_LIST *list;	
		int num;
	}list;

        /****************************/
        /* terminal node - a symbol */
        /****************************/
        struct {
                nel_O_token opcode;
                struct nel_SYMBOL *symbol;
        }
        symbol;

        /* wyong */
        struct {
                nel_O_token opcode;
                struct symbol *symbol;
        }
        nel_symbol;

        /*******************/
        /* unary operation */
        /*******************/
        struct {
                nel_O_token opcode;
                union nel_EXPR *operand;
        }
        unary;

        /********************/
        /* binary operation */
        /********************/
        struct {
                nel_O_token opcode;
                union nel_EXPR *left;
                union nel_EXPR *right;
        }
        binary;

        /*****************/
        /* function call */
        /*****************/
        struct {
                nel_O_token opcode;
                union nel_EXPR *func;
                int nargs;
                struct nel_EXPR_LIST *args;
        }
        funcall;

        /***********************/
        /* struct/union member */
        /***********************/
        struct {
                nel_O_token opcode;
                union nel_EXPR *s_u;
                struct nel_SYMBOL *member;
        }
        member;

        /**********************************************/
        /* type epxression: sizeof, typeof, or a cast */
        /**********************************************/
        struct {
                nel_O_token opcode;
                union nel_TYPE *type;
                union nel_EXPR *expr;
        }
        type;

        /******************/
        /* conditional: ? */
        /******************/
        struct {
                nel_O_token opcode;
                union nel_EXPR *cond;
                union nel_EXPR *true_expr;
                union nel_EXPR *false_expr;
        }
        cond;

	/* wyong, 2006.6.22 */
	struct {			
                nel_O_token opcode;
                union nel_EXPR *array;
                union nel_EXPR *upper;
                union nel_EXPR *lower;
		
	}range;

} nel_expr;



/***************************************************************/
/* linked lists are made of the following structures so that   */
/* we may find the start of all the chunks of memory allocated */
/* to hold nel_expr structures at garbage collection time. */
/***************************************************************/
typedef struct nel_EXPR_CHUNK
{
        unsigned_int size;
        nel_expr *start;
        struct nel_EXPR_CHUNK *next;
}
nel_expr_chunk;



/*********************************************************/
/* declarations for the nel_expr structure allocator */
/*********************************************************/
extern unsigned_int nel_exprs_max;
extern nel_lock_type nel_exprs_lock;



/*********************************************************************/
/* declarations for expressions for (which point to the symbols for) */
/* the integer values 0 and 1, which are boolean return values.      */
/*********************************************************************/
extern union nel_EXPR *nel_zero_expr;
extern union nel_EXPR *nel_one_expr;




/*******************************************************/
/* some expression nodes require lists of other trees, */
/* so we first create a structure for list of nodes.   */
/*******************************************************/
typedef struct nel_EXPR_LIST
{
        union nel_EXPR *expr;
        struct nel_EXPR_LIST *next;
}
nel_expr_list;



/********************************************************************/
/* linked lists are made of the following structures so that        */
/* we may find the start of all the chunks of memory allocated      */
/* to hold nel_expr_list structures at garbage collection time. */
/********************************************************************/
typedef struct nel_EXPR_LIST_CHUNK
{
        unsigned_int size;
        nel_expr_list *start;
        struct nel_EXPR_LIST_CHUNK *next;
}
nel_expr_list_chunk;



/**************************************************************/
/* declarations for the nel_expr_list structure allocator */
/**************************************************************/
extern unsigned_int nel_expr_lists_max;
extern nel_lock_type nel_expr_lists_lock;



/*************************************************************/
/* declarations for the routines defined in "expr.c" */
/*************************************************************/
extern char *nel_O_name (register nel_O_token);
extern char *nel_O_string (register nel_O_token);
extern union nel_EXPR *nel_expr_alloc (struct nel_eng *, nel_O_token, ...);
extern void nel_expr_dealloc (register union nel_EXPR *);
extern void nel_print_expr (FILE *, register union nel_EXPR *, int);
extern struct nel_EXPR_LIST *nel_expr_list_alloc (struct nel_eng *, register union nel_EXPR *, register struct nel_EXPR_LIST *);
extern void nel_expr_list_dealloc (register struct nel_EXPR_LIST *);
extern void nel_print_expr_list (FILE *, register struct nel_EXPR_LIST *, int);
extern nel_expr *nel_dup_expr(struct nel_eng *, nel_expr *);
extern void emit_expr(FILE *fp, union nel_EXPR *expr);

extern int nel_expr_update_dollar(struct nel_eng *, register nel_expr *expr, int pos, int value);
int nel_expr_diff(nel_expr *e1, nel_expr *e2);

//added by zhangbin, 2006-5-25
inline nel_type* eval_expr_type(struct nel_eng *eng, nel_expr *expr);
inline int expr_contain_funcall(struct nel_eng *eng, nel_expr *expr);
//edd

//added by zhangbin, 2006-7-19
void expr_dealloc(struct nel_eng *eng);
//end

#endif /* EXPR_H */
