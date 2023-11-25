/*****************************************************************************/
/* This file, "nel.y" contains the yacc grammar and rules for the  */
/* parsing routine used by the application executive.                        */
/*****************************************************************************/
%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "errors.h"
#include "type.h"
#include "io.h"
#include "lex.h"
#include "parser.h"
#include "sym.h"
#include "prod.h"
#include "evt.h"
#include "expr.h"
#include "stmt.h"
#include "intp.h"
#include "dec.h"
#include "mem.h"
#include "class.h"
#include "nlib/match.h"

#define PCRE_CASELESS           0x00000001
#define PCRE_MULTILINE          0x00000002
#define PCRE_DOTALL             0x00000004
#define PCRE_EXTENDED           0x00000008
#define PCRE_ANCHORED           0x00000010
#define PCRE_DOLLAR_ENDONLY     0x00000020
#define PCRE_EXTRA              0x00000040
#define PCRE_NOTBOL             0x00000080
#define PCRE_NOTEOL             0x00000100
#define PCRE_UNGREEDY           0x00000200
#define PCRE_NOTEMPTY           0x00000400
#define PCRE_UTF8               0x00000800
#define PCRE_NO_AUTO_CAPTURE    0x00001000
#define PCRE_NO_UTF8_CHECK      0x00002000
#define PCRE_AUTO_CALLOUT       0x00004000
#define PCRE_PARTIAL            0x00008000
#define PCRE_DFA_SHORTEST       0x00010000
#define PCRE_DFA_RESTART        0x00020000
#define PCRE_FIRSTLINE          0x00040000


#define NEL_PURE_ARRAYS 1	

	/*************************************/
	/* the syntax error handling routine */
	/*************************************/
#define yyerror(_message) \
	((eng->parser->last_char == YYEOF) ?				\
	   parser_stmt_error (eng, "syntax error at or near EOF") :	\
	   (((eng->parser->last_char == nel_T_IDENT) || (eng->parser->last_char == nel_T_TYPEDEF_NAME)) ?	\
	      parser_stmt_error (eng, "syntax error at or near symbol %s", eng->parser->text) : \
	      ((eng->parser->last_char == nel_T_C_STRING) ?		\
	         parser_stmt_error (eng, "syntax error at or near \"%s\"", eng->parser->text) : \
	         ((eng->parser->last_char > 0) ?				\
	            parser_stmt_error (eng, "syntax error at or near %s", eng->parser->text) : \
	            parser_stmt_error (eng, "syntax error")		\
	         )							\
	      )								\
	   )								\
	)



	/************************************************************************/
	/* #define the function declaration for yyparse() and the function call */
	/* to nel_lex() to pass the eng parameter throughout the entire     */
	/* call sequence.                                                       */
	/************************************************************************/
	int nel_yylex(struct nel_eng *eng);


	/******************************************************************/
	/* bison doesn't like tokens with the value 0, and nel_T_NULL */
	/* never appears in a production, so put it here.  put exactly    */
	/* one tab between nel_T_NULL and 0, for reasons explained in */
	/* "nel.lex.h".                                               */
	/******************************************************************/
#define nel_T_NULL	0



	/***********************************************************/
	/* many of the actions that construct expression trees are */
	/* nearly identical, and we can #define the entire action. */
	/***********************************************************/
#define unary_op_acts(_O_token)	 					\
	{								\
	   register nel_expr *__expr;					\
	   parser_pop_expr (eng, __expr);				\
	   __expr = nel_expr_alloc (eng, (_O_token), __expr);		\
	   parser_push_expr (eng, __expr);				\
	}

#define binary_op_acts(_O_token) \
	{								\
	   register nel_expr *__expr1;					\
	   register nel_expr *__expr2;					\
	   parser_pop_expr (eng, __expr2);				\
	   parser_pop_expr (eng, __expr1);				\
	   __expr1 = nel_expr_alloc (eng, (_O_token), __expr1, __expr2);\
	   parser_push_expr (eng, __expr1);				\
	}

int nel_O_asgn_expr(nel_O_token type)
{
	switch(type)
	{
	case nel_O_ASGN:
	case nel_O_ADD_ASGN:
	case nel_O_BIT_AND_ASGN:
	case nel_O_BIT_OR_ASGN:
	case nel_O_BIT_XOR_ASGN:
	case nel_O_DIV_ASGN:
	case nel_O_LSHIFT_ASGN:
	case nel_O_RSHIFT_ASGN:
	case nel_O_MOD_ASGN:
	case nel_O_MULT_ASGN:
	case nel_O_SUB_ASGN:
		return 1;
	default:
		return 0;
	}
}

%}


/******************************************/
/* all tokens must agree in value with    */
/* the constants defined in "nel_tokens.h" */
/******************************************/

/**************/
/* C keywords */
/**************/

%token nel_T_ASM		1	/*   asm		*/
%token nel_T_AUTO		2	/*   auto		*/
%token nel_T_BREAK		3	/*   break		*/
%token nel_T_CASE		4	/*   case		*/
%token nel_T_CHAR		5	/*   char		*/
%token nel_T_COMPLEX		6	/*   complex		*/
%token nel_T_CONST		7	/*   const		*/
%token nel_T_CONTINUE		8	/*   continue		*/
%token nel_T_DEFAULT		9	/*   default		*/
%token nel_T_DO			10	/*   do			*/
%token nel_T_DOUBLE		11	/*   double		*/
%token nel_T_ELSE		12	/*   else		*/
%token nel_T_ENUM		13	/*   enum		*/
%token nel_T_EXTERN		14	/*   extern		*/
%token nel_T_FLOAT		15	/*   float		*/
%token nel_T_FOR		16	/*   for		*/
%token nel_T_FORTRAN		17	/*   fortran		*/
%token nel_T_GOTO		18	/*   goto		*/
%token nel_T_IF			19	/*   if			*/
%token nel_T_INT		20	/*   int		*/
%token nel_T_LONG		21	/*   long		*/
%token nel_T_REGISTER		22	/*   register		*/
%token nel_T_RETURN		23	/*   return		*/
%token nel_T_SHORT		24	/*   short		*/
%token nel_T_SIGNED		25	/*   short		*/
%token nel_T_SIZEOF		26	/*   sizeof		*/
%token nel_T_STATIC		27	/*   static		*/
%token nel_T_STRUCT		28	/*   struct		*/
%token nel_T_SWITCH		29	/*   switch		*/
%token nel_T_TYPEDEC		30	/*   typedec		*/
%token nel_T_TYPEDEF		31	/*   typedef		*/
%token nel_T_TYPEOF		32	/*   typeof		*/
%token nel_T_UNION		33	/*   union		*/
%token nel_T_UNSIGNED		34	/*   unsigned		*/
%token nel_T_VOID		35	/*   void		*/
%token nel_T_VOLATILE		36	/*   volatile		*/
%token nel_T_WHILE		37	/*   while		*/



/*************************/
/* preprocessor keywords */
/*************************/

%token nel_T_P_DEFINE		38	/*   #define		*/
%token nel_T_P_ELIF		39	/*   #elif		*/
%token nel_T_P_ELSE		40	/*   #else		*/
%token nel_T_P_ENDIF		41	/*   #endif		*/
%token nel_T_P_ERROR		42	/*   #error		*/
%token nel_T_P_IF		43	/*   #if		*/
%token nel_T_P_IFDEF		44	/*   #ifdef		*/
%token nel_T_P_IFNDEF		45	/*   #ifndef		*/
%token nel_T_P_INCLUDE		46	/*   #include		*/
%token nel_T_P_LINE		47	/*   #line		*/
%token nel_T_P_PRAGMA		48	/*   #pragma		*/
%token nel_T_P_POUNDS		49	/*   #			*/
%token nel_T_P_UNDEF		50	/*   #undef		*/



/********************/
/* operator symbols */
/********************/

%token nel_T_AMPER		51	/*   &			*/
%token nel_T_AMPEREQ		52	/*   &=			*/
%token nel_T_BAR		53	/*   |			*/
%token nel_T_BAREQ		54	/*   |=			*/
%token nel_T_COLON		55	/*   :			*/
%token nel_T_COMMA		56	/*   ,			*/
%token nel_T_DAMPER		57	/*   &&			*/
%token nel_T_DBAR		58	/*   ||			*/
%token nel_T_DEQ		59	/*   ==			*/
%token nel_T_DLARROW		60	/*   <<			*/
%token nel_T_DLARROWEQ		61	/*   <<=		*/
%token nel_T_DMINUS		62	/*   --			*/
%token nel_T_DOT		63	/*   .			*/
%token nel_T_DPLUS		64	/*   ++			*/
%token nel_T_DRARROW		65	/*   >>			*/
%token nel_T_DRARROWEQ		66	/*   >>=		*/
%token nel_T_ELLIPSIS		67	/*   ...		*/
%token nel_T_EQ			68	/*   =			*/
%token nel_T_EXCLAM		69	/*   !			*/
%token nel_T_EXCLAMEQ		70	/*   !=			*/
%token nel_T_LARROW		71	/*   <			*/
%token nel_T_LARROWEQ		72	/*   <=			*/
%token nel_T_LBRACE		73	/*   {			*/
%token nel_T_LBRACK		74	/*   [			*/
%token nel_T_LPAREN		75	/*   (			*/
%token nel_T_MINUS		76	/*   -			*/
%token nel_T_MINUSRARROW	77	/*   ->			*/
%token nel_T_MINUSEQ		78	/*   -=			*/
%token nel_T_PERCENT		79	/*   %			*/
%token nel_T_PERCENTEQ		80	/*   %=			*/
%token nel_T_PLUS		81	/*   +			*/
%token nel_T_PLUSEQ		82	/*   +=			*/
%token nel_T_QUESTION		83	/*   ?			*/
%token nel_T_RARROW		84	/*   >			*/
%token nel_T_RARROWEQ		85	/*   >=			*/
%token nel_T_RBRACE		86	/*   }			*/
%token nel_T_RBRACK		87	/*   ]			*/
%token nel_T_RPAREN		88	/*   )			*/
%token nel_T_SEMI		89	/*   ;			*/
%token nel_T_SLASH		90	/*   /			*/
%token nel_T_SLASHEQ		91	/*   /=			*/
%token nel_T_STAR		92	/*   *			*/
%token nel_T_STAREQ		93	/*   *=			*/
%token nel_T_TILDE		94	/*   ~			*/
%token nel_T_UPARROW		95	/*   ^			*/
%token nel_T_UPARROWEQ		96	/*   ^=			*/



/*************/
/* constants */
/*************/

%token nel_T_C_CHAR		97	/*   char const		*/
%token nel_T_C_DOUBLE		98	/*   double const	*/
%token nel_T_C_FLOAT		99	/*   float const	*/
%token nel_T_C_LONG_DOUBLE	100	/*   long double const	*/
%token nel_T_C_SIGNED_INT	101	/*   int const		*/
%token nel_T_C_SIGNED_LONG_INT	102	/*   long int const	*/
%token nel_T_C_STRING		103	/*   string const	*/
%token nel_T_C_UNSIGNED_INT	104	/*   unsigned int const	*/
%token nel_T_C_UNSIGNED_LONG_INT	105	/*   unsigned long int const */



/*********************/
/* identifier tokens */
/*********************/

%token nel_T_IDENT		106	/*   symbol		*/
%token nel_T_TYPEDEF_NAME	107	/*   typedef'd type	*/



/*
 * new keywords for production declaration
 */
%token nel_T_ATOM		108	/* atom */
%token nel_T_EVENT		109	/* event */
%token nel_T_INIT		110	/* init */
%token nel_T_MAIN		114	/* main */
%token nel_T_EXIT		115	/* exit */
%token nel_T_FINI		116	/* fini */

%token nel_T_NODELAY		111	/* nodelay */

/*
 * some data types for production 
 */
%token nel_T_TIMEOUT		112	/* timeout */
%token nel_T_DOLLAR_IDENT	113	/* $[0-9]*/

%token nel_T_REGEXP		117
%token nel_T_REGEXT		118
%token nel_T_EXCLAM_TILDE	119
%token nel_T_TILDEEQ		120


/******************************************************/
/* finally, nel_T_NULL and nel_T_MAX          */
/* bison doesn't like 0-valued tokens, so we          */
/* #define nel_T_NULL in the declaration section. */
/******************************************************/

%token nel_T_MAX		121	/*   # of tokens	*/



%left nel_T_COMMA
%right nel_T_EQ nel_T_PLUSEQ nel_T_MINUSEQ nel_T_STAREQ nel_T_SLASHEQ nel_T_PERCENTEQ nel_T_DLARROWEQ nel_T_DRARROWEQ nel_T_AMPEREQ nel_T_UPARROWEQ nel_T_BAREQ
%left nel_T_QUESTION nel_T_COLON
%left nel_T_DBAR
%left nel_T_DAMPER
%left nel_T_BAR
%left nel_T_UPARROW
%left nel_T_AMPER
%left nel_T_DEQ nel_T_EXCLAMEQ
%left nel_T_LARROW nel_T_RARROW nel_T_LARROWEQ nel_T_RARROWEQ
%left nel_T_DLARROW nel_T_DRARROW
%left nel_T_PLUS nel_T_MINUS
%left nel_T_STAR nel_T_SLASH nel_T_PERCENT
%left nel_T_EXCLAM nel_T_EXCLAM_TILDE nel_T_TILDE nel_T_TILDEEQ nel_T_DPLUS nel_T_DMINUS nel_T_SIZEOF nel_T_TYPEOF
%left nel_T_LBRACK nel_T_RBRACK nel_T_LPAREN nel_T_RPAREN nel_T_DOT nel_T_MINUSRARROW

%start start

%pure_parser



/*****************************************************************************/
/* all code in the rules section has access to the variable "eng",        */
/*    which is defined local to the routine nel_parse().                 */
/*                                                                           */
/*****************************************************************************/



%%

/*****************************************************************************/
/* "start" is the start symbol.  (no shit)                                   */
/*****************************************************************************/
start:
empty {
	nel_debug ({ parser_trace (eng, "start->empty {} trans_unit\n\n"); });
	/********************************************************/
	/* set up the stack so that on the top is the default   */
	/* type descriptor (int), and one level down is the     */
	/* default storage class (NULL).  this is how the       */
	/* productions dec_specs->* set up the stack; this is   */
	/* a default for functions that are declared without    */
	/* any storage class, type qualifier, or type specifier */
	/* (i.e. implicitly delcared int.)                      */
	/********************************************************/
	{

		/********************************************/
		/* save the current context, so that error  */
		/* jumps for the outer level are allocated. */
		/********************************************/
		save_parser_context (eng);

		/***********************************************/
		/* this is the outer-level block error jump    */
		/* which scans for the first unmatched right   */
		/* bracket, or the right bracket matching the  */
		/* first left bracket (whichever is shorter).  */
		/* any more right brackets remaining in the    */
		/* input stream are deleted, along with a      */
		/* semicolon possibly following them (not      */
		/* necessary, as we allow semicolons in the    */
		/* outer level in this bastardized C grammar). */
		/***********************************************/
		eng->parser->stmt_err_jmp = eng->parser->block_err_jmp;
		parser_yy_setjmp (eng, eng->parser->tmp_int, eng->parser->stmt_err_jmp);
		if (eng->parser->tmp_int != 0)
		{
			/******************************************/
			/* there was an error - we jumped to here */
			/* a 2 is returned from setjmp if we      */
			/* should omit the "scanning for..." msg, */
			/* else a 1 is returned when longjmping.  */
			/* yacc may have cleared yychar, so use   */
			/* eng->parser->last_char for the value of   */
			/* the previous input token.              */
			/******************************************/
			if (eng->parser->tmp_int == 1) {
				parser_diagnostic (eng, "scanning for ';' or unmatched '}'");
			}
			nel_debug ({
				parser_trace (eng, "level 0 stmt error longjmp caught - scanning for ';' or unmathced '}' last_char = %N\n\n", eng->parser->last_char);
			});

			eng->parser->tmp_int = 0;
			do {
				if (eng->parser->last_char == nel_T_LBRACE) {
					eng->parser->tmp_int++;
				} else if (eng->parser->last_char == nel_T_RBRACE) {
					eng->parser->tmp_int--;
					if (eng->parser->tmp_int <= 0) {
						/******************************/
						/* clear out extraneous '}'s, */
						/******************************/
						do {
							yychar = nel_yylex (eng);
						} while (eng->parser->last_char == nel_T_RBRACE);
						parser_diagnostic (eng, "unmatched '}' found - parser reset");
						break;
					}
				} else if (eng->parser->last_char == nel_T_SEMI) {
					if (eng->parser->tmp_int <= 0) {
						parser_diagnostic (eng, "';' found - parser reset");
						break;
					}
				}
				yychar = nel_yylex(eng);
			} while (eng->parser->last_char > 0);

			/****************************************************/
			/* if we are at EOF, return.  if we try to continue */
			/* parsing and get another syntax error, we longjmp */
			/* back here, and are in an infinite loop.          */
			/****************************************************/
			if ( yychar == 0) {
				parser_diagnostic (eng, "EOF encountered");
				nel_longjmp (eng, *(eng->parser->return_pt), 1);
			}
		}

		/*********************************************/
		/* if the flag nel_save_stmt_err_jmp is true, */
		/* then set nel_stmt_err_jmp to point to the  */
		/* newly-set jump buffer.                    */
		/*********************************************/
		if (nel_save_stmt_err_jmp)
		{
			nel_stmt_err_jmp = eng->parser->stmt_err_jmp;
		}

		/***********************************************/
		/* push the dynamic value and symbol allocator */
		/* indeces, so that we may restore then after  */
		/* parsing each translataion unit - there may  */
		/* be some garbage produced in evaluating      */
		/* dimension bound expressions, bit field      */
		/* sizes, etc. at level 0.                     */
		/***********************************************/
		parser_push_value (eng, eng->parser->dyn_values_next, char *);
		parser_push_value (eng, eng->parser->dyn_symbols_next, nel_symbol *);

		/******************************************************/
		/* set up default type and storage class for function */
		/* declarations on the base of the stack, so that     */
		/* "func()" (function declaration without any storage */
		/* class, type qualifiers, or explicit type dec is    */
		/* given type int.  do this after the setjmp in case  */
		/* the stack gets corrupted (it shouldn't).           */
		/******************************************************/
		parser_push_token (eng, nel_T_NULL);
		parser_push_type (eng, nel_int_type);
	}
} trans_unit {
	nel_debug ({ parser_trace (eng, "start->empty trans_unit\n\n"); });
	{
		/******************************************/
		/* pop the default type and storage class */
		/******************************************/
		register int token;
		register nel_type *type;

		parser_pop_type (eng, type);
		parser_pop_token (eng, token);

		/**********************************************/
		/* pop the value and symbol allocator indeces */
		/**********************************************/
		parser_pop_value (eng, eng->parser->dyn_symbols_next, nel_symbol *);
		parser_pop_value (eng, eng->parser->dyn_values_next, char *);

		/******************************/
		/* restore the outer context. */
		/******************************/
		restore_parser_context (eng, eng->parser->level);
	}
};


trans_unit:
trans_unit extern_dec
{
	nel_debug ({ parser_trace (eng, "trans_unit->trans_unit extern_dec\n\n"); });
	{

		eng->parser->rhs_cnt = 0;

		/*************************************************/
		/* pop the value and symbol allocator indeces,   */
		/* cleaning up after (most) any garbage produced */
		/* in parsing the translation unit.  they are    */
		/* below the default type and storage class.     */
		/*************************************************/
		parser_top_value (eng, eng->parser->dyn_symbols_next, nel_symbol *, 2);
		parser_top_value (eng, eng->parser->dyn_values_next, char *, 3);
	}
}
|empty
{
	nel_debug ({ parser_trace (eng, "trans_unit->empty\n\n"); });
};


extern_dec:
func_def {
	nel_debug({parser_trace (eng, "extern_dec->func_def\n\n"); });
}
|dec_stmt {
	nel_debug({parser_trace (eng, "extern_dec->dec_stmt\n\n"); });
}
|production {
	nel_debug({parser_trace (eng, "extern_dec->production\n\n");});
}
|include_stmt {
	nel_debug({parser_trace (eng, "extern_dec->include_stmt\n\n");});
}
|nel_T_SEMI { 
	nel_debug({parser_trace (eng, "extern_dec->nel_T_SEMI\n\n"); });
}
;

/******************************************************************************/
/* recursively call nel_call_parse to parse the include file,		      */
/* we need save all the lex and yacc state before this call and restore    */
/* them manully. */
/******************************************************************************/
include_stmt	:
nel_T_P_INCLUDE nel_T_C_STRING {
	if(eng->parser->text[0] == '\0')
		parser_stmt_error(eng, "include nothing");
	parser_eval(eng, eng->parser->text, NULL,NULL);
}
;

/******************************************************************************/
/* production : lhs rhs_list nel_T_SEMI do not modify the stack, but when     */
/* errors occured this will not be ture, to prevent it, we need add some      */
/* error recovery process.	     */
/******************************************************************************/
production:
lhs rhs_list nel_T_SEMI {
	register nel_symbol *lhs;
	nel_debug ({ parser_trace (eng, "production -> lhs rhs_list nel_T_SEMI\n\n"); });
	parser_pop_symbol (eng, lhs);
}
;

/******************************************************************************/
/* left hand side of a production, it resident in parser stack until the      */
/* 'production: lhs    							      */
/* rhs_list nel_T_SEMI' reduced.					      */
/******************************************************************************/
lhs	: event
{
	register nel_symbol *symbol;
	register char *name;

	nel_debug ({ parser_trace (eng, "lhs -> event \n\n"); });
	parser_pop_symbol(eng, symbol);


	if(symbol->type == NULL) {
		parser_error(eng, "production: %s has no type declared",name);
	}
	
	if (symbol->class == nel_C_TERMINAL) {
		parser_error(eng, "the left part of production can't be atom");
	}

	eng->parser->rhs_cnt = 1;
	parser_push_symbol(eng, symbol);

}
;


/******************************************************************************/
/* three relationship indicator are now supported: '!', ':', and '!', the     */
/* indicator '->' was should not used anymore. 				      */
/******************************************************************************/
REL	:
nel_T_EXCLAM 	/* '!' */
{
	nel_debug ({ parser_trace (eng, "REL -> nel_T_EXCLAM \n\n"); });
	parser_push_integer(eng, REL_EX);
}

| nel_T_COLON /* ':' */
{
	nel_debug ({ parser_trace (eng, "REL -> nel_T_COLON\n\n"); });
	parser_push_integer(eng, REL_ON);
}

| nel_T_BAR  /* '|' */
{
	nel_debug ({ parser_trace (eng, "REL -> nel_T_BAR\n\n"); });
	parser_push_integer(eng, REL_ON);
}

| nel_T_MINUSRARROW	/* '->' */
{
	nel_debug ({ parser_trace (eng, "REL -> nel_T_MINUSRARROW\n\n");});
	parser_push_integer(eng, REL_AT);
}
;

/******************************************************************************/
/* there must an lhs symbol exist in the stack, so we can create the prod     */
/* at this point and insert prod into eng->parser->productions linked list,   */
/******************************************************************************/
rhs_list:
rel_rhs_block
{
	register nel_symbol *lhs;
	struct nel_RHS *rhs;
	register unsigned int rel;
	struct nel_SYMBOL *prod;
	struct nel_RHS *scan;
	nel_stmt *stmt;	

	nel_debug ({ parser_trace (eng, "rhs_list -> rel_rhs \n\n"); });
	parser_pop_stmt(eng, stmt);
	parser_pop_rhs(eng, rhs);
	parser_pop_integer(eng, rel);
	parser_top_symbol(eng, lhs, 0);

	/**********************************************************************/
	/* before we call prod_symbol_alloc to create production, create  */
	/* an type for this production, which record lhs, relationship, rhs,  */
	/* action, etc.     */
	/**********************************************************************/
	prod = prod_symbol_alloc (eng, NULL, nel_type_alloc(eng, nel_D_PRODUCTION, 0, nel_alignment_of (void *), 0, 0, lhs, rel, rhs, stmt));


	/**********************************************************************/
	/* whenever 'rhs_list : rel_rhs ' reduced, we free all the infomation */
	/* of rhs we have just parsed, and lhs left on the stack top, the fact*/
	/* is very important for production parsing  			      */
	/**********************************************************************/
	//parser_push_prod(eng, prod);

	/* free all instance variable symbol such as $1,... */
	for(scan = rhs; scan; scan = scan->next) {
		parser_remove_ident(eng, (nel_symbol *)scan->symbol->v );
	}

	eng->parser->rhs_cnt = 1;

}
| rhs_list rel_rhs_block
{
	register nel_symbol *lhs;
	struct nel_RHS *rhs;
	register unsigned int rel;
	struct nel_SYMBOL *prod;
	struct nel_RHS *scan;
	nel_stmt *stmt;

	nel_debug ({ parser_trace (eng, "rhs_list -> rhs_list rel_rhs \n\n");});
	parser_pop_stmt(eng, stmt);
	parser_pop_rhs(eng, rhs);
	parser_pop_integer(eng, rel);
	parser_top_symbol(eng, lhs, 0);

	/**********************************************************************/
	/* before we call prod_symbol_alloc to create production, create  */
	/* an type for this production, which record lhs, relationship, rhs,  */
	/* action, etc.     						      */
	/**********************************************************************/
	prod = prod_symbol_alloc (eng, NULL, nel_type_alloc(eng, nel_D_PRODUCTION, 0, nel_alignment_of (void *), 0, 0, lhs, rel, rhs, stmt));


	/**********************************************************************/
	/* whenever 'rhs_list : rel_rhs ' reduced, we free all the infomation */
	/* of rhs we have just parsed, and lhs left on the stack top, the fact*/
	/* is very important for production parsing  */
	/**********************************************************************/
	/* free all instance variable symbol such as $1,... */
	for(scan = rhs; scan; scan = scan->next) {
		parser_remove_ident(eng, (nel_symbol *)scan->symbol->v );
	}

	eng->parser->rhs_cnt = 1;

}
;


/******************************************************************************/
/* rel_rhs is construct from a relation identitator and a rhs */
/******************************************************************************/
rel_rhs_block :
REL rhs {
	nel_debug ({ parser_trace (eng, "rel_rhs -> REL rhs \n\n"); });
	parser_push_stmt(eng, NULL);	
}

| REL rhs {
	nel_debug ({ parser_trace (eng, "ident_expr_stmt -> ident_expr * block \n"); });

	/*************************************************************/
	/* set up the stmt lists to repare for block parsing  */
	/*************************************************************/
	eng->parser->stmts = NULL;
	eng->parser->block_ct = 0;
	eng->parser->block_no = 0;
	eng->parser->append_pt = &(eng->parser->stmts);
	eng->parser->break_target = eng->parser->continue_target = NULL;
	eng->parser->prog_unit = NULL;
	eng->parser->last_stmt = eng->parser->stmts;

} block {

	nel_debug({ parser_trace (eng, "expr_stmt_ident->expr_ident block * \n\n"); });
	parser_push_stmt(eng, eng->parser->stmts);	

}
;


/******************************************************************************/
/* rhs is the right hand side of production,not until an */
/* timeout_ident_expr_stmt reduced  we haven't collected enough information */
/* to create an struct nel_RHS. things are become  complicated for the fact */ 
/* that an production was defined as lhs : rhs1{} rhs2{} from user */
/* 's aspect, but the internal data struture for prod is lhs : rhs1  rhs2 {}, */
/* that means we have only one stmts with an internal production, this */
/* convertion was taken place here.*/
/******************************************************************************/
rhs:
ident_expr {
	register nel_symbol *symbol;
	struct nel_RHS *rhs, *this;
	

	nel_debug ({ parser_trace (eng, " rhs -> timeout_ident_expr_stmt \n\n"); });
	parser_pop_symbol (eng, symbol);


	/**********************************************************************/
	/* alloc a nel_RHS struct to hold the symbol, and link it             */
	/**********************************************************************/
	nel_malloc(rhs, 1, struct nel_RHS );
	rhs->symbol = symbol;

	rhs->offset = 1;
	rhs->last = rhs;
	rhs->next = rhs->prev = NULL;
	this = rhs;

	parser_push_rhs(eng, rhs);

}

| rhs ident_expr {

	register nel_symbol *symbol;
	int old_off;
	register nel_symbol *new_lhs, *lhs;
	struct nel_RHS *rhs, *this;
	struct nel_SYMBOL *prod;
	int flag = 0;

	nel_debug ({ parser_trace (eng, " rhs -> rhs timeout_ident_expr_stmt \n\n"); });
	parser_pop_symbol (eng, symbol);
	parser_pop_rhs(eng, rhs);

	/**********************************************************************/
	/* create an new RHS with symbol, and link it to previous RHS list, */
	/* increase the offset . 		  			     */
	/**********************************************************************/
	nel_malloc(this, 1, struct nel_RHS );
	this->symbol = symbol;

	this->filename = eng->parser->filename;
	this->line = eng->parser->line;

	this->offset = 1;
	if (rhs->last != NULL)
	{
		rhs->last->next = this ;
		this->prev = rhs->last;
		this->offset = rhs->last->offset;
	}
	this->next = NULL;
	this->offset ++;
	rhs->last = this;

	/**********************************************************************/
	/* use nel_expr_update_dollar to update symbol 's expr, it does :  */
	/* change $n in expr to $(n - rhs->offset + 1 );  	      */
	/**********************************************************************/
	if(nel_expr_update_dollar(eng, (union nel_EXPR *)symbol->value, 0, 1 - rhs->offset) < 0 ){
		parser_fatal_error(eng, "evt expr update dollar error!\n");
	}

	/**********************************************************************/
	/* check if the result $(n - rhs->offset) is outscope :             */
	/*	a) n == 0, or					              */
	/*	b) 1 <= n <= this->offset, when flag equal 0 		      */
	/*	c) 2 <= (n-rhs->offset+1) <= (this->offset-rhs->offset + 1),  */
	/*	   when flag equal 1;					      */
	/**********************************************************************/


	parser_push_rhs(eng, rhs);

}
;



/*****************************************************************************/
/* the ident_expr is an event with an expression (optional), which indicate  */
/* the event must be evalued before an successful identifition */
/*****************************************************************************/
ident_expr:
event
{
	register nel_symbol *symbol;
	register char *name;
	nel_debug ({ parser_trace (eng, "ident_expr -> event * \n"); });
	parser_pop_symbol(eng, symbol);	
	parser_push_symbol(eng, symbol);
}

| event nel_T_LPAREN expression nel_T_RPAREN
{
	register nel_symbol *old, *symbol;
	register char *name;
	register nel_expr *expr;

	nel_debug ({ parser_trace (eng, "ident_expr -> event nel_T_LPAREN expression nel_NTRP_RPAREN *\n"); });
	parser_pop_expr (eng, expr);

	nel_type* expr_type;
	expr_type = eval_expr_type(eng, expr);
	if(!expr_type)
		parser_stmt_error(eng, "illegal expr");

	parser_pop_symbol(eng, old);	

	if( old == eng->emptySymbol) {
		parser_warning(eng, "empty shouldn't have an expression, ignore!");
		symbol = old;	
	}
	else {
		/* we need create an new event symbol accroding to the old one, 
		we call the the old one 'parent', but this is not always true, 
		for example, parent may  have many childen which have same name 
		(the one without expression is the the parent), and what we have 
		found maybe an child. to prevent it, we initialize parent->
		_parent with himself and when we first meet a child. 
		we set child->_parent with parent->_parent (that is parent), 
		again, when we  meet the second child, set child2->parent with 
		either parent->_parent, or child->_parent will get the same 
		result, that is the 'real' parent  */

		symbol=event_symbol_alloc(eng, old->name,  old->type, 
			0, //NOTE,NOTE,NOTE, event with expr can't as lhs, so we
			   // can simply set _isolate flag to zero 	
			old->class, expr);

		symbol->_pid = old->_pid;
		symbol->_parent = old->_parent;
		symbol->v = old->v;

	}

	parser_push_symbol (eng, symbol);

}
;

event:
ident{
	register nel_type *type;
	register char *name;
	register nel_symbol *retval;
	register nel_symbol *symbol;
	char vname[16];

	nel_debug({ parser_trace (eng, "event -> ident \n\n"); });
	parser_pop_name (eng, name);

	/**********************************************************************/
	/* we can't use an undeclared event in production defination, we */
	/* don't think production defination before event declarationit is */ 
	/* an must task now */
	/**********************************************************************/
	if((symbol = lookup_event_symbol(eng, name)) == NULL) {
		parser_stmt_error (eng, "undeclaration of %s", name);
	}

	if(symbol == eng->endSymbol) {
		parser_stmt_error (eng, "can not use _terminator in production");
	}else if (symbol == eng->startSymbol){
		parser_stmt_error (eng, "can not use link in production");
	}

	type = symbol->type->event.descriptor;
	if(eng->parser->rhs_cnt == 0)
		sprintf(vname, "$$");
	else 
		sprintf(vname, "$%d", eng->parser->rhs_cnt);
	eng->parser->rhs_cnt++;


	retval = nel_static_symbol_alloc(eng, nel_insert_name(eng,vname), type, NULL, nel_C_FORMAL, nel_lhs_type(type), nel_L_NEL, 1);


	/**********************************************************************/
	/* make a struct nel_EVENT data structure for the symbol, which saving*/
	/* the necessary information of EVENT, so that the normal symbol won't*/
	/* need malloc memory space for those unnecessary info,this way, we   */
	/* can save a lot of memory usage */
	/**********************************************************************/
	
	retval->aux.event = nel_event_alloc(eng);
	//nel_malloc(retval->aux.event, 1, struct nel_EVENT);

	
	/**********************************************************************/
	/*pid is id of parent event , set pid equal id means the event has no */
	/*expression with it therefore need not an parent. */
	/**********************************************************************/
	retval->_pid = retval->id;

	retval->_parent = retval;
	retval->_cyclic = 0;
	retval->_flag = 0;
	//retval->_nodelay = 0;
	retval->_isEmptyString = 0;
	retval->_reachable = 0;
	retval->_deep = 0;
	retval->_state = -1;

	retval->_isolate = symbol->_isolate;



	/**********************************************************************/
	/* push the retval to semantic stack, so this variable can be seen    */
	/* within expr or stmt 						      */
	/**********************************************************************/
	parser_insert_ident (eng, retval);
	symbol->v = (char *) retval; 

	/**********************************************************************/
	/* push the retval to parse stack */
	/**********************************************************************/
	parser_push_symbol(eng, symbol);

}
;

/*****************************************************************************/
/* func_def->* productions do not modify the stack, but require the default  */
/* type specifier and storge class to be on the top of the stack before the  */
/* input symbols are absorbed.                                               */
/*****************************************************************************/

func_def:
dec_specs declarator pre_formals formal_dec_list post_formals block {
	nel_debug ({ parser_trace (eng, "func_def->dec_specs declarator pre_formals formal_dec_list post_formals block\n\n"); });
	{
		register nel_symbol *function;
		register nel_type *base_type;
		register int stor_class;
		parser_pop_symbol (eng, function);
		parser_pop_type (eng, base_type);
		parser_pop_token (eng, stor_class);
		nel_debug ({
					   if ((function == NULL) || (function->name == NULL) || (function->type == NULL)
							   || (function->type->simple.type != nel_D_FUNCTION) || (function != eng->parser->prog_unit)) {
					   parser_fatal_error (eng, "(func_def->dec_specs declarator pre_formals formal_dec_list post_formals block #1): bad function symbol\n%1S", function)
						   ;
					   }
				   });

		/**********************************************/
		/* set the function's value to the stmt list. */
		/* (set the value of the corresponding static */
		/* symbol also, if one exists.)               */
		/**********************************************/
		nel_global_def (eng, function->name, function->type, nel_static_C_token (function->class), (char *) (eng->parser->stmts), 0);
		

		//if ( comp_compile_func(eng, function) < 0 ) {
		if (create_classify_func(eng, function) < 0 ) { 
			parser_fatal_error (eng, "error in compiling %s\n", function->name );
		}

	}

}|
declarator pre_formals formal_dec_list post_formals block {
	nel_debug ({ parser_trace (eng, "func_def->declarator pre_formals formal_dec_list post_formals block\n\n"); });
	{
		register nel_symbol *function;
		parser_pop_symbol (eng, function);
		nel_debug ({
		if ((function == NULL) 
			|| (function->name == NULL) 
			|| (function->type == NULL) 
			|| (function->type->simple.type != nel_D_FUNCTION) 
			|| (function != eng->parser->prog_unit)) {
				parser_fatal_error(eng, "(func_def->declarator pre_formals formal_dec_list post_formals block #1): bad function symbol\n%1S", function);
		}
		});

		/**********************************************/
		/* set the function's value to the stmt list. */
		/* (set the value of the corresponding static */
		/* symbol also, if one exists.)               */
		/**********************************************/
		nel_global_def (eng, function->name, function->type, nel_static_C_token (function->class), (char *) (eng->parser->stmts), 0);
	
		//if ( comp_compile_func(eng, function) < 0 ) {
		if (create_classify_func(eng, function) < 0 ) { 
			parser_fatal_error (eng, "error in compiling %s\n", function->name );
		}

	}

}
| nel_T_INIT {
	nel_debug ({ parser_trace (eng, "func_def->empty {} block\n\n"); });
	{
		if(eng->parser->level == 0) {
			
			eng->parser->stmts = NULL;	
			eng->parser->block_ct = 0;
			eng->parser->block_no = 0;
			eng->parser->append_pt = &(eng->parser->stmts);
			eng->parser->break_target = eng->parser->continue_target = NULL;
			eng->parser->prog_unit = NULL;
			eng->parser->last_stmt = eng->parser->stmts;	
		}

		parser_push_integer (eng, eng->compile_level);
		eng->compile_level = 0;

	}
} block {
	nel_debug ({ parser_trace (eng, "func_def->empty block\n\n"); });
	{
		nel_stmt *head, *tail;
		parser_pop_integer(eng, eng->compile_level );
		ast_to_intp(eng, eng->parser->stmts, &head, &tail); 

#if 0
		intp_eval_stmt_2 (eng, head);

#else
		if(eng->init_head) {
			if(eng->init_tail) 
				nel_stmt_link(eng->init_tail, head);
			else 
				parser_fatal_error(eng, "eng->init_tail should never be NULL");
		} else {
			eng->init_head = head;
		}
		eng->init_tail = tail;
#endif

	}
}

| nel_T_FINI {
	nel_debug ({ parser_trace (eng, "func_def->nel_T_FINI {} block\n\n"); });
	{
		if(eng->parser->level == 0) {
			
			eng->parser->stmts = NULL;	
			eng->parser->block_ct = 0;
			eng->parser->block_no = 0;
			eng->parser->append_pt = &(eng->parser->stmts);
			eng->parser->break_target = eng->parser->continue_target = NULL;
			eng->parser->prog_unit = NULL;
			eng->parser->last_stmt = eng->parser->stmts;	
		}

		parser_push_integer (eng, eng->compile_level);
		eng->compile_level = 0;

	}
} block {
	nel_debug ({ parser_trace (eng, "func_def->nel_T_FINI block\n\n"); });
	{
		nel_stmt *head, *tail;
		parser_pop_integer(eng, eng->compile_level );
		ast_to_intp(eng, eng->parser->stmts, &head, &tail); 

		if(eng->fini_head) {
			if(eng->fini_tail) 
				nel_stmt_link(eng->fini_tail, head);
			else 
				parser_fatal_error(eng, "eng->fini_tail should never be NULL");
		} else {
			eng->fini_head = head;
		}
		eng->fini_tail = tail;

	}
}

| nel_T_MAIN  {
	nel_debug ({ parser_trace (eng, "func_def-> nel_T_MAIN {} block\n\n"); });
	{
		if (eng->main != NULL) {
			parser_stmt_error(eng, "main has declared");	
		}
		if(eng->parser->level == 0) {
			
			eng->parser->stmts = NULL;	
			eng->parser->block_ct = 0;
			eng->parser->block_no = 0;
			eng->parser->append_pt = &(eng->parser->stmts);
			eng->parser->break_target = eng->parser->continue_target = NULL;
			eng->parser->prog_unit = NULL;
			eng->parser->last_stmt = eng->parser->stmts;	
		}
		parser_push_integer (eng, eng->compile_level);
		eng->compile_level = 0;
	}
} block {
	nel_debug ({ parser_trace (eng, "func_def->empty block\n\n"); });
	{
		nel_stmt *head, *tail;
		parser_pop_integer(eng, eng->compile_level );
		ast_to_intp(eng, eng->parser->stmts, &head, &tail); 
		eng->main = head;	
	}
}
| nel_T_EXIT {
	nel_debug ({ parser_trace (eng, "func_def-> nel_T_EXIT {} block\n\n"); });
	{
		if (eng->exit != NULL) {
			parser_stmt_error(eng, "exit has declared");	
		}
		if(eng->parser->level == 0) {
			
			eng->parser->stmts = NULL;	
			eng->parser->block_ct = 0;
			eng->parser->block_no = 0;
			eng->parser->append_pt = &(eng->parser->stmts);
			eng->parser->break_target = eng->parser->continue_target = NULL;
			eng->parser->prog_unit = NULL;
			eng->parser->last_stmt = eng->parser->stmts;	
		}
		parser_push_integer (eng, eng->compile_level);
		eng->compile_level = 0;
	}
} block {
	nel_debug ({ parser_trace (eng, "func_def->empty block\n\n"); });
	{
		nel_stmt *head, *tail;
		parser_pop_integer(eng, eng->compile_level );
		ast_to_intp(eng, eng->parser->stmts, &head, &tail); 
		eng->exit = head;
	}
}
;


pre_formals:
empty {
	/**********************************************************/
	/* pop the function name and type, create a symbol table  */
	/* entry for it, and push it.                             */
	/* the formal args in the type structure for the function */
	/* are statically allocated and part of a permanent type  */
	/* descriptor for it.  insert copies of the formals into  */
	/* the dynamic ident hash table at level 1.  the symbols  */
	/* for the arguments will be deallocated upon exiting the */
	/* block.                                                 */
	/**********************************************************/
	nel_debug ({ parser_trace (eng, "pre_formals->empty\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int stor_class;
		register nel_symbol *symbol;

		/******************************/
		/* functions declarations are */
		/* illegal in declare mode    */
		/******************************/
#if 0

		if (eng->parser->declare_mode) {
			parser_block_error (eng, "In pre_formals: executable code not allowed in declare mode");
		}
#endif

		/**************************************************/
		/* pop the name and type, resolve the descriptor, */
		/* make sure it is a function, and check for an   */
		/* incomplete type.                               */
		/* nel_global_def() also checks    */
		/* this, but does not perform the block error.    */
		/**************************************************/
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		type = parser_resolve_type (eng, type, name);
		nel_debug ({
		if (name == NULL) {
			parser_fatal_error (eng, "(pre_formals->empty #1): NULL name");
		}
		if (type == NULL) {
			parser_fatal_error (eng, "(pre_formals->empty #2): NULL type");
		}
		});

		if (type->simple.type != nel_D_FUNCTION) {
			yyerror ("syntax error");
		}
		if (nel_type_incomplete (type)) {
			parser_block_error (eng, "incomplete type for %s", name);
		}

		/************************************/
		/* storage class is below base type */
		/************************************/
		parser_top_token (eng, stor_class, 1);

		/***************************************************/
		/* check for the appropriate class, and call       */
		/* nel_global_def () to create  */
		/* the symbol table entry(s) for the function.     */
		/* use a NULL value to check that we may later set */
		/* the value to the stmt list.                     */
		/***************************************************/
		switch (stor_class) {
		case nel_T_TYPEDEF:
		case nel_T_EXTERN:
		case nel_T_AUTO:
		case nel_T_REGISTER:
			parser_warning (eng, "illegal class for %s", name);
			/* fall through */
		case nel_T_NULL:
			symbol = nel_global_def (eng, name, type, 0, NULL, 0);
			/* 4th param == static */
			/* 6th param == static */
			break;
		case nel_T_STATIC:
			symbol = nel_global_def (eng, name, type, 1, NULL, 0);
			break;
		default:
			parser_fatal_error (eng, "(pre_formals->empty #3): bad class %N", stor_class);
			break;
		}

		/*********************************************/
		/* check for an inappropriate type qualifier */
		/*********************************************/
		if (type->function._const) {
			parser_warning (eng, "const type qualifier for %s ignored", name);
		}
		if (type->function._volatile ) {
			parser_warning (eng, "volatile type qualifier for %s ignored", name);
		}

		/*******************************/
		/* the program unit symbol is  */
		/* NULL if there was an error. */
		/*******************************/
		if (symbol == NULL) {
			/****************************************************/
			/* the appropriate error has already been emitted   */
			/* - we still need to jump to the end of the block. */
			/****************************************************/
			parser_block_error (eng, NULL);
		}
		if (type->function.new_style) {
			/*************************************************/
			/* for new style functions, we scan the list of  */
			/* nel_list structures, and insert the formal     */
			/* arguments into the symbol tables.             */
			/*************************************************/
			register nel_list *scan;
			for (scan = type->function.args; (scan != NULL); scan = scan->next) {
				register nel_symbol *formal;
				formal = scan->symbol;
				nel_debug ({
							   if ((formal == NULL) || (formal->type == NULL)) {
							   parser_fatal_error (eng, "(pre_formals->empty #4): bad symbol\n%1S", formal)
								   ;
							   }
						   });
				if (formal->name == NULL) {
					parser_error (eng, "incompletely specified argument");
				}
				parser_insert_ident (eng, formal);
			}
		} else {
			/***********************************************/
			/* for old-style declarations, just insert the */
			/* symbols into the symbol table.  we do not   */
			/* know their types yet.                       */
			/***********************************************/
			register nel_list *scan;
			for (scan = type->function.args; (scan != NULL); scan = scan->next) {
				register nel_symbol *formal;
				formal = scan->symbol;
				nel_debug ({
							   if ((formal == NULL) || (formal->name == NULL)) {
							   parser_fatal_error (eng, "(pre_formals->empty #5): bad formal\n%1S", formal)
								   ;
							   }
						   });
				parser_insert_ident (eng, formal);
			}
		}

		/**************************************/
		/* push the symbol back on the stack, */
		/**************************************/
		parser_push_symbol (eng, symbol);
	}
};



post_formals:
empty {
	/**********************************************************/
	/* scan through the formal argument list, checking that   */
	/* all formals were declared.                             */
	/**********************************************************/
	nel_debug ({ parser_trace (eng, "post_formals->empty\n\n"); });
	{
		nel_symbol *function;
		register nel_type *func_type;
		//register nel_type *type;
		register nel_list *scan;
		//va_list *ap;
		parser_top_symbol (eng, function, 0);
		nel_debug ({
					   if ((function == NULL) || (function->type == NULL)
							   || (function->type->simple.type != nel_D_FUNCTION)) {
					   parser_fatal_error (eng, "(post_formals->empty #1): bad function", function)
						   ;
					   }
				   });

		/*************************/
		/* set up the stmt lists */
		/*************************/
		eng->parser->block_ct = 0;
		eng->parser->block_no = 0;
		eng->parser->append_pt = &(eng->parser->stmts);
		eng->parser->break_target = eng->parser->continue_target = NULL;
		eng->parser->prog_unit = function;
		eng->parser->last_stmt = eng->parser->stmts;

		/***********************************************/
		/* now scan throught the formal args, creating */
		/* a declaration stmt struct for each arg.     */
		/***********************************************/

		//if (compile_level == 0) {
		func_type = function->type;
		for (scan = func_type->function.args; (scan != NULL); scan = scan->next) {
			register nel_symbol *formal = scan->symbol;
			register nel_stmt *stmt;
			if ((formal == NULL) || (formal->type == NULL) || (formal->level != 1)) {
				parser_block_error (eng, "argument %I not declared", formal);
			}

			/*************************************************/
			/* create a stmt structure for this declaration, */
			/* and append it to the current list.            */
			/*************************************************/
			stmt = nel_stmt_alloc (eng, nel_S_DEC, eng->parser->filename,
								   eng->parser->line, formal, NULL);
			append_stmt (stmt, &(stmt->dec.next));
		}
		//}// end of compile_level

	}
};





/*****************************************************************************/
/* productions for parsing formal declarations.                              */
/*****************************************************************************/

formal_dec_list:
formal_dec_list formal_dec {
	nel_debug ({ parser_trace (eng, "formal_dec_list->formal_dec_list formal_dec\n\n"); });
}|
empty {
	nel_debug ({ parser_trace (eng, "formal_dec_list->empty\n\n"); });
};



/*****************************************************************************/
/* formal_dec->* productions do not modify the stack.  They only check to    */
/* make sure that the formal arguments were declared properly, and add the   */
/* type descriptors to the symbols, which should already be inserted in      */
/* the dyn_ident_hash table by pre_formals->empty.                           */
/*****************************************************************************/

formal_dec:
dec_specs formal_dcltors nel_T_SEMI {
	nel_debug ({ parser_trace (eng, "formal_dec->dec_specs formal_dcltors nel_T_SEMI\n\n"); });
	{
		register nel_type *type;
		register int stor_class;
		parser_pop_type (eng, type);
		parser_pop_token (eng, stor_class);
	}
}|
dec_specs nel_T_SEMI {
	nel_debug ({ parser_trace (eng, "formal_dec->dec_specs nel_T_SEMI\n\n"); });
	{
		register nel_type *type;
		register int stor_class;
		parser_pop_type (eng, type);
		parser_pop_token (eng, stor_class);
	}
};



formal_dcltors:
formal_dcltors nel_T_COMMA formal_dcltor {
	nel_debug ({ parser_trace (eng, "formal_dcltors->formal_dcltors nel_T_COMMA formal_dcltor\n\n"); });
}|
formal_dcltor {
	nel_debug ({ parser_trace (eng, "formal_dcltors->formal_dcltor\n\n"); });
};



formal_dcltor:
declarator {
	/*********************************************************/
	/* for each declared formal argument, check that it has  */
	/* the apropriate class (register or auto), and that the */
	/* argument is only declared once.  promote the type to  */
	/* "pointer to ..." for arrays or functions.             */
	/*********************************************************/
	nel_debug ({ parser_trace (eng, "formal_dcltor->declarator\n\n"); });
	{
		register char *name;
		register nel_symbol *symbol;
		register nel_type *type;
		register int stor_class;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		nel_debug ({
					   if (name == NULL) {
					   parser_fatal_error (eng, "(formal_dcltor->declarator #1): NULL name")
						   ;
					   }
					   if (type == NULL) {
					   parser_fatal_error (eng, "(formal_dcltor->declarator #2): NULL type")
						   ;
					   }
				   });

		/************************************/
		/* storage class is below base type */
		/************************************/
		parser_top_token (eng, stor_class, 1);

		/**************************************************/
		/* look up the name in the symbol tables to get   */
		/* the symbol that was inserted after parsing the */
		/* function declarator parameter list.            */
		/**************************************************/
		symbol = parser_lookup_ident (eng, name);
		nel_debug ({
					   parser_trace (eng, "symbol =\n%1S\n", symbol);
				   });

		/*****************************************************/
		/* check to make sure that the symbol is indeed a    */
		/* formal argument, and that it has not already been */
		/* declared.                                         */
		/*****************************************************/
		if ((symbol == NULL) || (symbol->class != nel_C_FORMAL) || (symbol->level != 1)) {
			parser_error (eng, "declared argument %s is missing", name);
		} else if (symbol->declared) {
			parser_error (eng, "redeclaration of %s", name);
		} else {
			switch (type->simple.type) {
				/***************************************/
				/* arrays and functions get coerced to */
				/* pointers to arrays and functions.   */
				/***************************************/
			case nel_D_ARRAY:
				type = nel_type_alloc (eng, nel_D_POINTER, sizeof (char *), nel_alignment_of (char *), type->array._const, type->array._volatile, type->array.base_type);
				break;

			case nel_D_FUNCTION:
				type = nel_type_alloc (eng, nel_D_POINTER, sizeof (char *), nel_alignment_of (char *), 0, 0, type);
				break;

				/*********************************/
				/* floats are coerced to doubles */
				/*********************************/
			case nel_D_FLOAT:
				type = nel_double_type;
				break;

				/**********************************************/
				/* complex float is coerced to complex double */
				/**********************************************/
			case nel_D_COMPLEX:
			case nel_D_COMPLEX_FLOAT:
				type = nel_complex_double_type;
				break;

				/**************************/
				/* void types are illegal */
				/**************************/
			case nel_D_VOID:
				parser_error (eng, "void type for %I", symbol);
				break;

				/********************************/
				/* other types are not promoted */
				/********************************/
			default:
				break;
			}

			/*****************************/
			/* check for incomplete type */
			/*****************************/
			if (nel_type_incomplete (type)) {
				parser_error (eng, "incomplete type for %I", symbol);
				break;
			}

			/************************************/
			/* check for the appropriate class, */
			/* and mark the symbol as declared. */
			/************************************/
			switch (stor_class) {
			case nel_T_TYPEDEF:
			case nel_T_EXTERN:
			case nel_T_STATIC:
				parser_warning (eng, "illegal class for %I", symbol);
				/* fall through */
			case nel_T_AUTO:
			case nel_T_REGISTER:
			case nel_T_NULL:
				symbol->type = type;
				symbol->declared = 1;
				break;
			default:
				parser_fatal_error (eng, "(formal_dcltor->declarator #3): bad class %N", stor_class);
				break;
			}

			/*******************************/
			/* formal arguments can be a   */
			/* legal lhs of an assignment. */
			/*******************************/
			symbol->lhs = nel_lhs_type (type);
		}
		nel_debug ({
					   parser_trace (eng, "symbol =\n%1S\n", symbol);
				   });
	}
};





/*****************************************************************************/
/* productions for parsing local and external declarations.                  */
/* Note that, because we scan the input multiple times, symbols at level 0   */
/* may be declared multiple times.  symbols at deeper levels will be purged  */
/* from the tables upon exit from a block, and will not be found if we scan  */
/* the input again.                                                          */
/*****************************************************************************/



/*****************************************************************************/
/* dec_stmt->* productions do not modify the stack.  They only insert the    */
/* appropriate symbols values into the tables.                               */
/*****************************************************************************/

dec_stmt:
dec_specs init_dec_list nel_T_SEMI {
	nel_debug ({ parser_trace (eng, "dec_stmt->dec_specs init_dec_list nel_T_SEMI\n\n"); });
	{
		register nel_type *type;
		register int stor_class;
		parser_pop_type (eng, type);
		parser_pop_token (eng, stor_class);
	}
}|
dec_specs nel_T_SEMI {
	nel_debug ({ parser_trace (eng, "dec_stmt->dec_specs nel_T_SEMI\n\n"); });
	{
		register nel_type *type;
		register int stor_class;
		parser_pop_type (eng, type);
		parser_pop_token (eng, stor_class);
	}
}/* |
		//for predefined C variable initialization
		init_dec_list nel_T_SEMI {
		   nel_debug ({ parser_trace (eng, "dec_stmt->init_dec_list nel_T_SEMI\n\n"); });
		}
		*/
;



init_dec_list:
init_dec_list nel_T_COMMA init_dec {
	nel_debug ({ parser_trace (eng, "init_dec_list->init_dec_list nel_T_COMMA init_dec\n\n"); });
}|
init_dec {
	nel_debug ({ parser_trace (eng, "init_dec_list->init_dec\n\n"); });
};



init_dec:
declarator {
	nel_debug ({ parser_trace (eng, "init_dec->declarator\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int stor_class;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);

		/************************************/
		/* storage class is below base type */
		/************************************/
		parser_top_token (eng, stor_class, 1);
		nel_dec (eng, name, type, stor_class);
	}
}|
declarator nel_T_EQ {
	nel_debug ({ parser_trace (eng, "init_dec->declarator nel_T_EQ * initializer\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int stor_class;
		register nel_symbol *symbol;

		parser_pop_name (eng, name);
		parser_pop_type (eng, type);

		/************************************/
		/* storage class is below base type */
		/************************************/
		parser_top_token (eng, stor_class, 1);

		symbol = nel_dec (eng, name, type, stor_class);
		parser_push_symbol(eng, symbol);

	}
}
initializer {
	nel_debug ({ parser_trace (eng, "init_dec->declarator nel_T_EQ initializer\n\n"); });
	{
		register nel_symbol *symbol;
		nel_expr *expr;
		parser_pop_expr(eng, expr);
		parser_pop_symbol(eng, symbol);

		if ( symbol!=NULL && symbol->initialized == 0){
			nel_dec_init(eng, symbol, expr);
		}

		/* we consume everything, so push back nothing */
	}

}
;



/*****************************************************************************/
/* initializer->* productions push no items, but assume that the symbol      */
/* whose value is to be initialized is on the top of the semantic stack.     */
/*****************************************************************************/

initializer:
asgn_expr {
	nel_debug ({ parser_trace (eng, "initializer->asgn_expr\n\n"); });
}|
nel_T_LBRACE init_list nel_T_RBRACE {
	nel_debug ({ parser_trace (eng, "initializer->nel_T_LBRACE init_list nel_T_RBRACE\n\n"); });	
	{
		nel_expr *expr;
		nel_expr_list *exprs;
		int nexprs;
		parser_pop_integer (eng, nexprs);
		parser_pop_expr_list (eng, exprs);	/* end of arg list */
		parser_pop_expr_list (eng, exprs);	/* start of arg list */

		expr = nel_expr_alloc(eng, nel_O_LIST, exprs, nexprs);
		parser_push_expr(eng, expr);
	}

}|
nel_T_LBRACE init_list nel_T_COMMA nel_T_RBRACE {
	nel_debug ({ parser_trace (eng, "initializer->nel_T_LBRACE init_list nel_T_COMMA nel_T_RBRACE\n\n"); });
	{
		nel_expr *expr;
		nel_expr_list *exprs;
		int nexprs;
		parser_pop_integer (eng, nexprs);
		parser_pop_expr_list (eng, exprs);	/* end of arg list */
		parser_pop_expr_list (eng, exprs);	/* start of arg list */

		expr = nel_expr_alloc(eng, nel_O_LIST, exprs, nexprs);
		parser_push_expr(eng, expr);
	}

};



init_list:
init_list nel_T_COMMA initializer {
	
	/********************************************************/
	/* pop the next argument, then number of arguments and  */
	/* the last node, append the argument to the list, then */
	/* push the new node and the incremented arg count.     */
	/********************************************************/
	nel_debug ({ parser_trace (eng, "init_list->init_list nel_T_COMMA initializer\n\n"); });	
	{
		register nel_expr *expr;
		register int nexprs;
		register nel_expr_list *end;

		parser_pop_expr (eng, expr);
		parser_pop_integer (eng, nexprs);
		parser_pop_expr_list (eng, end);
		end->next = nel_expr_list_alloc (eng, expr, NULL);
		end = end->next;
		parser_push_expr_list (eng, end);
		parser_push_integer (eng, nexprs+ 1);
	}
}|
initializer {
	nel_debug ({ parser_trace (eng, "init_list->initializer\n\n"); });	

	/*******************************************************/
	/* form a node for the start of the expression list,   */
	/* push it twice (since it is currently the start and  */
	/* end of the list, and push the number of args (1).   */
	/*******************************************************/
	{
		register nel_expr *expr;
		register nel_expr_list *list;
		parser_pop_expr (eng, expr);
		list = nel_expr_list_alloc (eng, expr, NULL);
		parser_push_expr_list (eng, list);
		parser_push_expr_list (eng, list);
		parser_push_integer (eng, 1);
	}

};



/*****************************************************************************/
/* productions for parsing dec specs and type specs.                         */
/*****************************************************************************/



/*****************************************************************************/
/* dec_specs->* productions push two items on the semantic stack.  The       */
/* storage class and the type specifier (in that order, from bottom up).     */
/*****************************************************************************/

dec_specs:
dec_specs2 {
	nel_debug ({ parser_trace (eng, "dec_specs->dec_specs2\n\n"); });
	{
		/***********************************************/
		/* pop the type descriptor, storage class, and */
		/* const and volatile flags from the stack.    */
		/***********************************************/
		register nel_type *type;
		register int stor_class;
		register unsigned_int _const;
		register unsigned_int _volatile;
		parser_pop_type (eng, type);
		if (type == NULL) {
			type = nel_int_type;
		}
		parser_pop_token (eng, stor_class);
		parser_pop_integer (eng, _const);
		parser_pop_integer (eng, _volatile);

		/*****************************************************/
		/* if const or volatile were specified, create a     */
		/* duplicate type descriptor with the correct type   */
		/* qualifiers.  first, check to make sure that the   */
		/* type has not already had the qualifier associated */
		/* with it (possible with a typedef)                 */
		/*****************************************************/
		if ((_const) && (type->simple._const)) {
			parser_warning (eng, "extra const type qualifier ignored");
		}
		if ((_volatile) && (type->simple._volatile)) {
			parser_warning (eng, "extra volatile type qualifier ignored");
		}
		if (((_const) && (! type->simple._const)) ||
				((_volatile) && (! type->simple._volatile))) {
			register nel_type *new_type = nel_type_alloc (eng, nel_D_VOID, 0, 0, 0, 0);
			*new_type = *type;
			new_type->simple._const |= _const;
			new_type->simple._volatile |= _volatile;
			type = new_type;
		}

		/***************************************/
		/* now push the storage class and type */
		/* descriptor back on the stack.       */
		/***************************************/
		parser_push_token (eng, stor_class);
		parser_push_type (eng, type);
	}
}
;


dec_specs2:
dec_specs2 stor_cls_spec {
	/*********************************************/
	/* assume stack is set up as described below */
	/*********************************************/
	nel_debug ({ parser_trace (eng, "dec_specs2->dec_specs2 stor_cls_spec\n\n"); });
	{
		register int old_class;
		register int new_class;
		parser_pop_token (eng, new_class);
		parser_top_token (eng, old_class, 1);
		if (old_class != /*NULL*/ 0 ) {
			if((old_class != nel_T_ATOM || old_class!= nel_T_EVENT) 
			&& (new_class == nel_T_ATOM || new_class==nel_T_EVENT)){
				parser_warning (eng, "extra storage class specifier %N ignored", old_class);
				parser_tush_token (eng, new_class, 1);
			}else {
				parser_warning (eng, "extra storage class specifier %N ignored", new_class);
			}
				
		} else {
			parser_tush_token (eng, new_class, 1);
		}
	}
}|
stor_cls_spec {
	/*********************************************************/
	/* set up the stack so that on the top is the current    */
	/* type descriptor, one level down is the storage class, */
	/* and below that are the _const and _volatile flags.    */
	/*********************************************************/
	nel_debug ({ parser_trace (eng, "dec_specs2->stor_cls_spec\n\n"); });
	{
		register int stor_class;
		parser_pop_token (eng, stor_class);
		parser_push_integer (eng, 0);	/* volatile flag	*/
		parser_push_integer (eng, 0);	/* const flag		*/
		parser_push_token (eng, stor_class);
		parser_push_type (eng, NULL);	/* type descriptor	*/
	}
}|
dec_specs2 type_spec {
	nel_debug ({ parser_trace (eng, "dec_specs2->dec_specs2 type_spec\n\n"); });
	{
		register nel_type *old_type;
		register nel_type *new_type;
		parser_pop_type (eng, new_type);
		parser_pop_type (eng, old_type);
		if (old_type == NULL) {
			parser_push_type (eng, new_type);
		} else {
			if ((new_type = nel_merge_types (eng, old_type, new_type)) == NULL) {
				parser_stmt_error (eng, "illegal type combination");
			}
			parser_push_type (eng, new_type);
		}
	}
}|
type_spec {
	nel_debug ({ parser_trace (eng, "dec_specs2->type_spec\n\n"); });
	{
		register nel_type *type;
		parser_pop_type (eng, type);
		parser_push_integer (eng, 0);	/* volatile flag*/
		parser_push_integer (eng, 0);	/* const flag	*/
		parser_push_token (eng, /*NULL*/ 0 );/* storage class*/
		parser_push_type (eng, type);	/* type descriptor*/
	}
}|
dec_specs2 nel_T_CONST {
	nel_debug ({ parser_trace (eng, "dec_specs2->dec_specs2 nel_T_CONST\n\n"); });
	{
		register unsigned_int _const;
		parser_top_integer (eng, _const, 2);
		if (_const) {
			parser_warning (eng, "extra const type qualifier ignored");
		} else {
			parser_tush_integer (eng, 1, 2);	/* set const flag */
		}
	}
}|
nel_T_CONST {
	nel_debug ({ parser_trace (eng, "dec_specs2->nel_T_CONST\n\n"); });
	{
		parser_push_integer (eng, 0);	/* volatile flag*/
		parser_push_integer (eng, 1);	/* const flag	*/
		parser_push_token (eng, nel_T_NULL); /* storage class */
		parser_push_type (eng, NULL);	/* type descriptor*/
	}
}|
dec_specs2 nel_T_VOLATILE {
	nel_debug ({ parser_trace (eng, "dec_specs2->dec_specs2 nel_T_VOLATILE \n\n"); });
	{
		register unsigned_int _volatile;
		parser_top_integer (eng, _volatile, 3);
		if (_volatile) {
			parser_warning (eng, "extra volatile type qualifier ignored");
		} else {
			parser_tush_integer (eng, 1, 3);	/* set volatile flag */
		}
	}
}|
nel_T_VOLATILE {
	nel_debug ({ parser_trace (eng, "dec_specs2->nel_T_VOLATILE\n\n"); });
	{
		parser_push_integer (eng, 1);	/* volatile flag*/
		parser_push_integer (eng, 0);	/* const flag	*/
		parser_push_token (eng, nel_T_NULL);/* storage class */
		parser_push_type (eng, NULL);	/* type descriptor*/
	}
};



/*****************************************************************************/
/* stor_cls_spec->* productions push one item, the storage class (int token),*/
/* on the semantic stack.                                                    */
/*****************************************************************************/

stor_cls_spec:
nel_T_AUTO {
	nel_debug ({ parser_trace (eng, "stor_cls_spec->nel_T_AUTO\n\n"); });
	{
		parser_push_token (eng, nel_T_AUTO);
	}
}|
nel_T_REGISTER {
	nel_debug ({ parser_trace (eng, "stor_cls_spec->nel_T_REGISTER\n\n"); });
	{
		parser_push_token (eng, nel_T_REGISTER);
	}
}|
nel_T_STATIC {
	nel_debug ({ parser_trace (eng, "stor_cls_spec->nel_T_STATIC\n\n"); });
	{
		parser_push_token (eng, nel_T_STATIC);
	}
}|
nel_T_EXTERN {
	nel_debug ({ parser_trace (eng, "stor_cls_spec->nel_T_EXTERN\n\n"); });
	{
		parser_push_token (eng, nel_T_EXTERN);
	}
}|
nel_T_TYPEDEF {
	nel_debug ({ parser_trace (eng, "stor_cls_spec->nel_T_TYPEDEF\n\n"); });
	{
		parser_push_token (eng, nel_T_TYPEDEF);
	}
}|
nel_T_ATOM {
	nel_debug ({ parser_trace (eng, "stor_cls_spec->nel_T_ATOM\n\n"); });
	{
		parser_push_token (eng, nel_T_ATOM);
	}
}|
nel_T_EVENT {
	nel_debug ({ parser_trace (eng, "stor_cls_spec->nel_T_ATOM\n\n"); });
	{
		parser_push_token (eng, nel_T_EVENT);
	}
}
;



/*****************************************************************************/
/* type_spec->* productions push one item,  the type specifier (nel_type *),  */
/* on the semantic stack.                                                    */
/*****************************************************************************/

type_spec:
nel_T_CHAR {
	nel_debug ({ parser_trace (eng, "type_spec->nel_T_CHAR\n\n"); });
	{
		parser_push_type (eng, nel_char_type);
	}
}|
nel_T_COMPLEX {
	nel_debug ({ parser_trace (eng, "type_spec->nel_T_COMPLEX\n\n"); });
	{
		parser_push_type (eng, nel_complex_type);
	}
}|
nel_T_DOUBLE {
	nel_debug ({ parser_trace (eng, "type_spec->nel_T_DOUBLE\n\n"); });
	{
		parser_push_type (eng, nel_double_type);
	}
}|
nel_T_FLOAT {
	nel_debug ({ parser_trace (eng, "type_spec->nel_T_FLOAT\n\n"); });
	{
		parser_push_type (eng, nel_float_type);
	}
}|
nel_T_INT {
	nel_debug ({ parser_trace (eng, "type_spec->nel_T_INT\n\n"); });
	{
		parser_push_type (eng, nel_int_type);
	}
}|
nel_T_LONG {
	nel_debug ({ parser_trace (eng, "type_spec->nel_T_LONG\n\n"); });
	{
		parser_push_type (eng, nel_long_type);
	}
}|
nel_T_SHORT {
	nel_debug ({ parser_trace (eng, "type_spec->nel_T_SHORT\n\n"); });
	{
		parser_push_type (eng, nel_short_type);
	}
}|
nel_T_SIGNED {
	nel_debug ({ parser_trace (eng, "type_spec->nel_T_SIGNED\n\n"); });
	{
		parser_push_type (eng, nel_signed_type);
	}
}|
nel_T_UNSIGNED {
	nel_debug ({ parser_trace (eng, "type_spec->nel_T_UNSIGNED\n\n"); });
	{
		parser_push_type (eng, nel_unsigned_type);
	}
}|
nel_T_VOID {
	nel_debug ({ parser_trace (eng, "type_spec->nel_T_VOID\n\n"); });
	{
		parser_push_type (eng, nel_void_type);
	}
}|
s_u_spec {
	nel_debug ({ parser_trace (eng, "type_spec->s_u_spec\n\n"); });
}|
enum_spec {
	nel_debug ({ parser_trace (eng, "type_spec->enum_spec\n\n"); });
}|
typedef_name {
	nel_debug ({ parser_trace (eng, "type_spec->typedef_name\n\n"); });
	{
		register char *name;
		register nel_symbol *symbol;
		register nel_type *type;
		parser_pop_name (eng, name);
		symbol = parser_lookup_ident (eng, name);
		nel_debug ({
			if ((symbol == NULL) || (symbol->type == NULL) || (symbol->type->simple.type != nel_D_TYPEDEF_NAME)) {
				parser_fatal_error (eng, "(type_spec->typedef_name #1): bad typedef\nsymbol =\n%1S", symbol);
			}
		});
		type = symbol->type->typedef_name.descriptor;
		parser_push_type (eng, type);
	}
};



/*****************************************************************************/
/* s_u_spec->* productions push one item,  the struct/union type specifier   */
/* (nel_type *), on the semantic stack.                                       */
/*****************************************************************************/

s_u_spec:
nel_T_STRUCT tag nel_T_LBRACE {
	/**********************************************************/
	/* for structure definitions, we need to check that       */
	/* the members are equal if the structure tag already     */
	/* existed.                                               */
	/**********************************************************/
	nel_debug ({ parser_trace (eng, "s_u_spec->nel_T_STRUCT tag nel_T_LBRACE {} s_u_dec_list nel_T_RBRACE\n\n"); });
	{
		register nel_type *tag_type_d;
		register nel_type *struct_type_d;
		register nel_symbol *symbol;
		parser_pop_symbol (eng, symbol);
		if (symbol->type == NULL) {
			/**************************************************/
			/* we have a newly-defined tag - insert it in the */
			/* tag tables.  we know the tag->* productions    */
			/* allocated a new symbol.                        */
			/**************************************************/
			struct_type_d = nel_type_alloc (eng, nel_D_STRUCT, 0, 0, 0, 0, symbol, NULL);
			parser_push_type (eng, struct_type_d);
			tag_type_d = nel_type_alloc (eng, nel_D_STRUCT_TAG, 0, 0, 0, 0, struct_type_d);
			parser_pop_type (eng, struct_type_d);
			symbol->type = tag_type_d;
			symbol->value = (char *) struct_type_d;
			parser_insert_tag (eng, symbol);
			parser_push_type (eng, NULL);
		} else if (symbol->level != eng->parser->level) {
			/*******************************************/
			/* the tag name was used in an outer scope */
			/* - create a new symbol table entry.      */
			/* allocate space for a new tag, and       */
			/* insert it in the tag table.             */
			/*******************************************/
			symbol = nel_static_symbol_alloc (eng, symbol->name, NULL, NULL, nel_C_TYPE, 0, nel_L_NEL, eng->parser->level);
			struct_type_d = nel_type_alloc (eng, nel_D_STRUCT, 0, 0, 0, 0, symbol, NULL);
			parser_push_type (eng, struct_type_d);
			tag_type_d = nel_type_alloc (eng, nel_D_STRUCT_TAG, 0, 0, 0, 0, struct_type_d);
			parser_pop_type (eng, struct_type_d);
			symbol->type = tag_type_d;
			symbol->value = (char *) struct_type_d;
			parser_insert_tag (eng, symbol);
			parser_push_type (eng, NULL);
		} else if (symbol->type->simple.type != nel_D_STRUCT_TAG) {
			parser_stmt_error (eng, "redeclaration of %I", symbol);
		} else {
			struct_type_d = symbol->type->tag_name.descriptor;
			nel_debug ({
						   if ((struct_type_d == NULL) || (struct_type_d->simple.type != nel_D_STRUCT)) {
						   parser_fatal_error (eng, "(s_u_spec->nel_T_STRUCT tag nel_T_LBRACE {} s_u_dec_list nel_T_RBRACE #1): bad struct_type_d\n%1T", struct_type_d)
							   ;
						   }
					   });
			if ((struct_type_d->s_u.members == NULL) || ((struct_type_d->s_u.members->symbol == NULL)
					&& (struct_type_d->s_u.members->next == NULL))) {
				/************************************************/
				/* we are defining a previously incomplete type */
				/* the tag and the struct type descriptor are   */
				/* already set up - push NULL so that we do not */
				/* compare the new type descriptor with the     */
				/* incomplete one.                              */
				/************************************************/
				parser_push_type (eng, NULL);
			} else if (symbol->level > 0) {
				/*************************************************/
				/* we may scan a declaration at level 0 mulitple */
				/* times.  if we are at a higher scoping level,  */
				/* call parser_stmt_error to jump to the end of the  */
				/* struct declaration.                           */
				/*************************************************/
				parser_stmt_error (eng, "redeclaration of %I", symbol);
			} else {
				/************************************************/
				/* we are scanning a redeclaration at level 0 - */
				/* push the symbol's type descriptor so that we */
				/* may compare its type descriptor with the     */
				/* newly-generated structure type descriptor.   */
				/* do not create a new tag and insert it in the */
				/* symbol tables; any recursive references to   */
				/* the tag will refer to the previous type      */
				/* descriptor.                                  */
				/************************************************/
				parser_push_type (eng, struct_type_d);
				struct_type_d = nel_type_alloc (eng, nel_D_STRUCT, 0, 0, 0, 0, symbol, NULL);
			}
		}
		parser_push_type (eng, struct_type_d);
		parser_push_symbol (eng, symbol);
	}
} s_u_dec_list nel_T_RBRACE {
	nel_debug ({ parser_trace (eng, "s_u_spec->nel_T_STRUCT tag nel_T_LBRACE s_u_dec_list nel_T_RBRACE\n\n"); });
	{
		register nel_member *end;
		nel_member *members;
		register nel_symbol *symbol;
		register nel_type *struct_type_d;
		register nel_type *old_type;
		parser_pop_member (eng, end);
		parser_pop_member (eng, members);
		parser_pop_symbol (eng, symbol);
		parser_pop_type (eng, struct_type_d);
		parser_pop_type (eng, old_type);
		if (struct_type_d->s_u.members == NULL) {
			struct_type_d->s_u.members = members;
		} else if ((struct_type_d->s_u.members->symbol == NULL) && (struct_type_d->s_u.members->next == NULL)) {
			/*************************************************/
			/* must of had an incomplete type with a dummy   */
			/* member element.  overwrite the dummy with the */
			/* first member element, so that any other type  */
			/* descriptors pointing here are now defined.    */
			/*************************************************/
			nel_copy (sizeof (nel_member), struct_type_d->s_u.members, members);
		}
		nel_set_struct_offsets (eng, struct_type_d);
		if (old_type != NULL) {
			/************************************************/
			/* if we are at level 0 and have already seen a */
			/* complete declaration for this type, the old  */
			/* type descriptor was pushed.  now compare the */
			/* old type descriptor with the new one to make */
			/* certain they are equal.                      */
			/************************************************/
			nel_debug ({
						   if (old_type->simple.type != nel_D_STRUCT) {
						   parser_fatal_error (eng, "(s_u_spec->nel_T_STRUCT tag nel_T_LBRACE s_u_dec_list nel_T_RBRACE #1): bad old_type\n%1T", old_type)
							   ;
						   }
					   });
			if (nel_member_diff (old_type->s_u.members, members, 1)) {
				parser_error (eng, "redeclaration of %I", symbol);
			}
			parser_push_type (eng, old_type);
		} else {
			parser_push_type (eng, struct_type_d);
		}
	}
}|
nel_T_STRUCT nel_T_LBRACE s_u_dec_list nel_T_RBRACE {
	nel_debug ({ parser_trace (eng, "s_u_spec->nel_T_STRUCT nel_T_LBRACE s_u_dec_list nel_T_RBRACE\n\n"); });
	/**********************************************************/
	/* for an untagged struct or union, just form the type    */
	/* descriptor, set the offset, and push it.               */
	/**********************************************************/
	{
		register nel_member *end;
		nel_member *members;
		register nel_type *struct_type_d;
		parser_pop_member (eng, end);
		parser_pop_member (eng, members);
		struct_type_d = nel_type_alloc (eng, nel_D_STRUCT, 0, 0, 0, 0, NULL, members);
		nel_set_struct_offsets (eng, struct_type_d);
		parser_push_type (eng, struct_type_d);
	}
}|
nel_T_STRUCT tag {
	/**********************************************************/
	/* for an incompletely specified structure, push the type */
	/* descriptor if it exists, otherwise form an incomplete  */
	/* structure type descriptor.                             */
	/**********************************************************/
	nel_debug ({ parser_trace (eng, "s_u_spec->nel_T_STRUCT tag\n\n"); });
	{
		register nel_type *tag_type_d;
		register nel_type *struct_type_d;
		register nel_symbol *symbol;
		parser_pop_symbol (eng, symbol);
		if ((symbol->level != eng->parser->level) && (eng->parser->last_char == nel_T_SEMI)) {
			/****************************************************/
			/* the recondite rule: the tag appears in an outer  */
			/* scope, and there are no variables being declared */
			/* => this is a newly-defined incomplete type, most */
			/* likely part of a mutually recursive structure.   */
			/****************************************************/
			symbol = nel_static_symbol_alloc (eng, symbol->name, NULL, NULL, nel_C_TYPE, 0, nel_L_NEL, eng->parser->level);
			struct_type_d = nel_type_alloc (eng, nel_D_STRUCT, 0, 0, 0, 0, symbol, NULL);
			tag_type_d = nel_type_alloc (eng, nel_D_STRUCT_TAG, 0, 0, 0, 0, struct_type_d);
			symbol->type = tag_type_d;
			symbol->value = (char *) struct_type_d;
			parser_insert_tag (eng, symbol);
		} else if (symbol->type != NULL) {
			/**************************/
			/* the tag already exists */
			/**************************/
			if (symbol->type->simple.type != nel_D_STRUCT_TAG) {
				parser_stmt_error (eng, "redeclaration of %I", symbol);
			}
			struct_type_d = symbol->type->tag_name.descriptor;
		} else {
			/*************************************/
			/* form type descriptors for both    */
			/* the tag and the structure itself. */
			/*************************************/
			struct_type_d = nel_type_alloc (eng, nel_D_STRUCT, 0, 0, 0, 0, symbol, NULL);
			tag_type_d = nel_type_alloc (eng, nel_D_STRUCT_TAG, 0, 0, 0, 0, struct_type_d);
			symbol->type = tag_type_d;
			symbol->value = (char *) struct_type_d;
			parser_insert_tag (eng, symbol);
		}
		parser_push_type (eng, struct_type_d);
	}
}|
nel_T_UNION tag nel_T_LBRACE {
	/**********************************************************/
	/* for new union definitions, we need to check that the   */
	/* members are equal if the union tag already existed     */
	/**********************************************************/
	nel_debug ({ parser_trace (eng, "s_u_spec->nel_T_UNION tag nel_T_LBRACE {} s_u_dec_list nel_T_RBRACE\n\n"); });
	{
		register nel_type *tag_type_d;
		register nel_type *union_type_d;
		register nel_symbol *symbol;
		parser_pop_symbol (eng, symbol);
		if (symbol->type == NULL) {
			/**************************************************/
			/* we have a newly-defined tag - insert it in the */
			/* tag tables.  we know the tag->* productions    */
			/* allocated a new symbol.                        */
			/**************************************************/
			union_type_d = nel_type_alloc (eng, nel_D_UNION, 0, 0, 0, 0, symbol, NULL);
			tag_type_d = nel_type_alloc (eng, nel_D_UNION_TAG, 0, 0, 0, 0, union_type_d);
			symbol->type = tag_type_d;
			symbol->value = (char *) union_type_d;
			parser_insert_tag (eng, symbol);
			parser_push_type (eng, NULL);
		} else if (symbol->level != eng->parser->level) {
			/*******************************************/
			/* the tag name was used in an outer scope */
			/* - create a new symbol table entry.      */
			/* allocate space for a new tag, and       */
			/* insert it in the tag table.             */
			/*******************************************/
			symbol = nel_static_symbol_alloc (eng, symbol->name, NULL, NULL, nel_C_TYPE, 0, nel_L_NEL, eng->parser->level);
			union_type_d = nel_type_alloc (eng, nel_D_UNION, 0, 0, 0, 0, symbol, NULL);
			tag_type_d = nel_type_alloc (eng, nel_D_UNION_TAG, 0, 0, 0, 0, union_type_d);
			symbol->type = tag_type_d;
			symbol->value = (char *) union_type_d;
			parser_insert_tag (eng, symbol);
			parser_push_type (eng, NULL);
		} else if (symbol->type->simple.type != nel_D_UNION_TAG) {
			parser_stmt_error (eng, "redeclaration of %I", symbol);
		} else {
			union_type_d = symbol->type->tag_name.descriptor;
			nel_debug ({
						   if ((union_type_d == NULL) || (union_type_d->simple.type != nel_D_UNION)) {
						   parser_fatal_error (eng, "(s_u_spec->nel_T_UNION tag nel_T_LBRACE {} s_u_dec_list nel_T_RBRACE #1): bad union_type_d\n%1T", union_type_d)
							   ;
						   }
					   });
			if ((union_type_d->s_u.members == NULL) || ((union_type_d->s_u.members->symbol == NULL)
					&& (union_type_d->s_u.members->next == NULL))) {
				/************************************************/
				/* we are defining a previously incomplete type */
				/* the tag and the union type descriptor are    */
				/* already set up - push NULL so that we do not */
				/* compare the new type descriptor with the     */
				/* incomplete one.                              */
				/************************************************/
				parser_push_type (eng, NULL);
			} else if (symbol->level > 0) {
				/*************************************************/
				/* we may scan a declaration at level 0 mulitple */
				/* times.  if we are at a higher scoping level,  */
				/* call parser_stmt_error to jump to the end of the  */
				/* union declaration.                            */
				/*************************************************/
				parser_stmt_error (eng, "redeclaration of %I", symbol);
			} else {
				/************************************************/
				/* we are scanning a redeclaration at level 0 - */
				/* push the symbol's type descriptor so that we */
				/* may compare its type descriptor with the     */
				/* newly-generated union type descriptor.       */
				/* do not create a new tag and insert it in the */
				/* symbol tables; any recursive references to   */
				/* the tag will refer to the previous type      */
				/* descriptor.                                  */
				/************************************************/
				parser_push_type (eng, union_type_d);
				union_type_d = nel_type_alloc (eng, nel_D_UNION, 0, 0, 0, 0, symbol, NULL);
			}
		}
		parser_push_type (eng, union_type_d);
		parser_push_symbol (eng, symbol);
	}
} s_u_dec_list nel_T_RBRACE {
	nel_debug ({ parser_trace (eng, "s_u_spec->nel_T_UNION tag nel_T_LBRACE s_u_dec_list nel_T_RBRACE\n\n"); });
	{
		register nel_member *end;
		nel_member *members;
		register nel_symbol *symbol;
		register nel_type *union_type_d;
		register nel_type *old_type;
		nel_symbol *member_sym;
		parser_pop_member (eng, end);
		parser_pop_member (eng, members);
		parser_pop_symbol (eng, symbol);
		parser_pop_type (eng, union_type_d);
		parser_pop_type (eng, old_type);
		if (nel_bit_field_member (members, &member_sym)) {
			parser_stmt_error (eng, "bit field in union: %I", member_sym);
		}
		if (union_type_d->s_u.members == NULL) {
			union_type_d->s_u.members = members;
		} else if ((union_type_d->s_u.members->symbol == NULL) && (union_type_d->s_u.members->next == NULL)) {
			/*************************************************/
			/* must of had an incomplete type with a dummy   */
			/* member element.  overwrite the dummy with the */
			/* first member element, so that any other type  */
			/* descriptors pointing here are now defined.    */
			/*************************************************/
			nel_copy (sizeof (nel_member), union_type_d->s_u.members, members);
		}
		union_type_d->s_u.size = nel_s_u_size (eng, &(union_type_d->s_u.alignment), members);
		if(union_type_d->s_u.size < 0){
			parser_fatal_error (eng, "(nel_s_u_size #1): illegal nel_member structure:\n%1M", members);
		}

		if (old_type != NULL) {
			/************************************************/
			/* if we are at level 0 and have already seen a */
			/* complete declaration for this type, the old  */
			/* type descriptor was pushed.  now compare the */
			/* old type descriptor with the new one to make */
			/* certain they are equal.                      */
			/************************************************/
			nel_debug ({
						   if (old_type->simple.type != nel_D_UNION) {
						   parser_fatal_error (eng, "(s_u_spec->nel_T_UNION tag nel_T_LBRACE s_u_dec_list nel_T_RBRACE #1): bad old_type\n%1T", old_type)
							   ;
						   }
					   });
			if (nel_member_diff (old_type->s_u.members, members, 1)) {
				parser_error (eng, "redeclaration of %I", symbol);
			}
			parser_push_type (eng, old_type);
		} else {
			parser_push_type (eng, union_type_d);
		}
	}
}|
nel_T_UNION nel_T_LBRACE  s_u_dec_list nel_T_RBRACE {
	/**********************************************************/
	/* for an untagged struct or union, just form the type    */
	/* descriptor, set the offset, and push it.               */
	/**********************************************************/
	nel_debug ({ parser_trace (eng, "s_u_spec->nel_T_UNION nel_T_LBRACE s_u_dec_list nel_T_RBRACE\n\n"); });
	{
		register nel_member *end;
		register nel_member *members;
		register nel_type *union_type_d;
		nel_symbol *member_sym;
		parser_pop_member (eng, end);
		parser_pop_member (eng, members);
		if (nel_bit_field_member (members, &member_sym)) {
			parser_stmt_error (eng, "bit field in union: %I", member_sym);
		}
		union_type_d = nel_type_alloc (eng, nel_D_UNION, 0, 0, 0, 0, NULL, members);
		union_type_d->s_u.size = nel_s_u_size (eng, &(union_type_d->s_u.alignment), members);
		if(union_type_d->s_u.size < 0){
			parser_fatal_error (eng, "(nel_s_u_size #1): illegal nel_member structure:\n%1M", members);
		}


		parser_push_type (eng, union_type_d);
	}
}|
nel_T_UNION tag {
	/**********************************************************/
	/* for an incompletely specified union, push the type     */
	/* descriptor if it exists, otherwise form an incomplete  */
	/* union type descriptor.                                 */
	/**********************************************************/
	nel_debug ({ parser_trace (eng, "s_u_spec->nel_T_UNION tag\n\n"); });
	{
		register nel_type *tag_type_d;
		register nel_type *union_type_d;
		register nel_symbol *symbol;
		parser_pop_symbol (eng, symbol);
		if ((symbol->level != eng->parser->level) && (eng->parser->last_char == nel_T_SEMI)) {
			/****************************************************/
			/* the recondite rule: the tag appears in an outer  */
			/* scope, and there are no variables being declared */
			/* => this is a newly-defined incomplete type, most */
			/* likely part of a mutually recursive structure.   */
			/****************************************************/
			symbol = nel_static_symbol_alloc (eng, symbol->name, NULL, NULL, nel_C_TYPE, 0, nel_L_NEL, eng->parser->level);
			parser_push_symbol (eng, symbol);
			union_type_d = nel_type_alloc (eng, nel_D_UNION, 0, 0, 0, 0, symbol, NULL);
			parser_push_type (eng, union_type_d);
			tag_type_d = nel_type_alloc (eng, nel_D_UNION_TAG, 0, 0, 0, 0, union_type_d);
			parser_pop_type (eng, union_type_d);
			parser_pop_symbol (eng, symbol);
			symbol->type = tag_type_d;
			symbol->value = (char *) union_type_d;
			parser_insert_tag (eng, symbol);
		} else if (symbol->type != NULL) {
			/**************************/
			/* the tag already exists */
			/**************************/
			if (symbol->type->simple.type != nel_D_UNION_TAG) {
				parser_stmt_error (eng, "redeclaration of %I", symbol);
			}
			union_type_d = symbol->type->tag_name.descriptor;
		} else {
			/**********************************/
			/* form type descriptors for both */
			/* the tag and the union itself.  */
			/**********************************/
			union_type_d = nel_type_alloc (eng, nel_D_UNION, 0, 0, 0, 0, symbol, NULL);
			parser_push_type (eng, union_type_d);
			tag_type_d = nel_type_alloc (eng, nel_D_UNION_TAG, 0, 0, 0, 0, union_type_d);
			parser_pop_type (eng, union_type_d);
			symbol->type = tag_type_d;
			symbol->value = (char *) union_type_d;
			parser_insert_tag (eng, symbol);
		}
		parser_push_type (eng, union_type_d);
	}
};



tag:
ident {
	nel_debug ({ parser_trace (eng, "tag->ident\n\n"); });
	{
		register char *name;
		register nel_symbol *symbol;
		parser_pop_name (eng, name);
		symbol = parser_lookup_tag (eng, name);
		if (symbol == NULL) {
			symbol = nel_static_symbol_alloc (eng, name, NULL, NULL, nel_C_TYPE, 0, nel_L_NEL, eng->parser->level);
		}
		parser_push_symbol (eng, symbol);
	}
};



/*****************************************************************************/
/* s_u_dec_list->* pushes two items on the semantic stack, the start and end */
/* of the list of structure or union members (nel_member *).                  */
/*****************************************************************************/

s_u_dec_list:
s_u_dec_list s_u_dec {
	nel_debug ({ parser_trace (eng, "s_u_dec_list->s_u_dec_list s_u_dec\n\n"); });
}|
empty {
	nel_debug ({ parser_trace (eng, "s_u_dec_list->empty {} s_u_dec\n\n"); });
	/**********************************************************/
	/* set up the stack so that on the top of the stack is    */
	/* the current end and start of the member list (NULL for */
	/* for now).  Note that the spec_qual_list->* productions */
	/* will push two items on top of this, so when the        */
	/* s_u_dcltor_list->* productions change its value, the   */
	/* end of the list will be 2 places down on the stack.    */
	/**********************************************************/
	{
		parser_push_member (eng, NULL);  /* start of member list */
		parser_push_member (eng, NULL);  /* end of member list   */
	}
} s_u_dec {
	nel_debug ({ parser_trace (eng, "s_u_dec_list->empty s_u_dec\n\n"); });
};



s_u_dec:
spec_qual_list s_u_dcltor_list nel_T_SEMI {
	nel_debug ({ parser_trace (eng, "s_u_dec->spec_qual_list s_u_dcltor_list nel_T_SEMI\n\n"); });
	{
		register nel_type *type;
		parser_pop_type (eng, type);
	}
};



s_u_dcltor_list:
s_u_dcltor_list nel_T_COMMA s_u_dcltor {
	nel_debug ({ parser_trace (eng, "s_u_dcltor_list->s_u_dcltor_list nel_T_COMMA s_u_dcltor\n\n"); });
	/********************************************/
	/* the start and end of the member list are */
	/* below the type specifier on the stack.   */
	/********************************************/
	{
		register nel_member *new_member;
		register nel_member *end;
		parser_pop_member (eng, new_member);
		parser_top_member (eng, end, 1);		/* get end list */

		//make sure every member has different name, otherwise error
		nel_member *temp_member;
		parser_top_member(eng, temp_member, 2);
		while(temp_member)
		{
			if(temp_member == NULL || temp_member->symbol == NULL)
			{
				parser_stmt_error(eng, "illegal member", temp_member);
			}
			if(strcmp(temp_member->symbol->name, new_member->symbol->name) == 0)//error
			{
				parser_stmt_error(eng, "illegal member name", temp_member);
			}
			temp_member = temp_member->next;
		}
		
		end->next = new_member;
		parser_tush_member (eng, new_member, 1);	/* new end list */
	}
}|
s_u_dcltor {
	nel_debug ({ parser_trace (eng, "s_u_dcltor_list->s_u_dcltor\n\n"); });
	{
		register nel_member *new_member;
		register nel_member *end;
		parser_pop_member (eng, new_member);
		parser_top_member (eng, end, 1);
		if (end == NULL) {
			parser_tush_member (eng, new_member, 1); /* get start list */
			parser_tush_member (eng, new_member, 2); /* get end list   */
		} else {
			//make sure every member has different name, otherwise error
			nel_member *temp_member;
			parser_top_member(eng, temp_member, 2);
			while(temp_member) {
				if(temp_member == NULL || temp_member->symbol == NULL) {
					parser_stmt_error(eng, "illegal member", temp_member);
				}
				if(strcmp(temp_member->symbol->name, new_member->symbol->name) == 0) {
					parser_stmt_error(eng, "illegal member name", temp_member);
				}
				temp_member = temp_member->next;
			}
			end->next = new_member;
			parser_tush_member (eng, new_member, 1); /* new end list */
		}
	}
};



s_u_dcltor:
declarator {
	nel_debug ({ parser_trace (eng, "s_u_dcltor->declarator\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register nel_member *member;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		type = parser_resolve_type (eng, type, name);
		nel_debug ({
		if (name == NULL) {
			parser_fatal_error (eng, "(s_u_dcltor->declarator #1): NULL name");
		}
		if (type == NULL) {
			parser_fatal_error (eng, "(s_u_dcltor->declarator #2): NULL type");
		}
		});

		switch (type->simple.type) {
		case nel_D_FUNCTION:
			parser_stmt_error (eng, "function illegal in structure or union");
		case nel_D_VOID:
			parser_stmt_error (eng, "void type for %s", name);
		case nel_D_STRUCT:
		case nel_D_UNION:
			if (type->s_u.size <= 0) {
				parser_stmt_error (eng, "undefined structure or union");
			}
			break;
		default:
			if (nel_type_incomplete (type)) {
				parser_stmt_error (eng, "incomplete type in structure or union");
			} else if (type->simple.size <= 0) {
				parser_stmt_error (eng, "illegal type in structure or union");
			}
			break;
		}
		member = nel_member_alloc (eng, nel_static_symbol_alloc (eng, name, type, NULL, nel_C_MEMBER, nel_lhs_type (type), nel_L_NEL, eng->parser->level), 0, 0, 0, 0, NULL);
		parser_push_member (eng, member);
	}
}|
declarator nel_T_COLON int_asgn_expr {
	nel_debug ({ parser_trace (eng, "s_u_dcltor->declarator nel_T_COLON int_asgn_expr \n\n"); });
	{
		register int bit_size;
		register char *name;
		register nel_type *type;
		register nel_member *member;
		parser_pop_integer (eng, bit_size);
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		type = parser_resolve_type (eng, type, name);
		nel_debug ({
					   if (name == NULL) {
					   parser_fatal_error (eng, "(s_u_dcltor->declarator #1): NULL name")
						   ;
					   }
					   if (type == NULL) {
					   parser_fatal_error (eng, "(s_u_dcltor->declarator #2): NULL type")
						   ;
					   }
				   });
		switch (type->simple.type) {
		case nel_D_FUNCTION:
			parser_stmt_error (eng, "function illegal in struct or union");
		case nel_D_VOID:
			parser_stmt_error (eng, "void type for %s", name);
		case nel_D_CHAR:
		case nel_D_ENUM:
		case nel_D_INT:
		case nel_D_SHORT:
		case nel_D_SHORT_INT:
		case nel_D_SIGNED:
		case nel_D_SIGNED_CHAR:
		case nel_D_SIGNED_INT:
		case nel_D_SIGNED_SHORT:
		case nel_D_SIGNED_SHORT_INT:
		case nel_D_UNSIGNED:
		case nel_D_UNSIGNED_CHAR:
		case nel_D_UNSIGNED_INT:
		case nel_D_UNSIGNED_SHORT:
		case nel_D_UNSIGNED_SHORT_INT:
			break;
		default:
			parser_stmt_error (eng, "illegal field type");
		}
		if (bit_size <= 0) {
			parser_stmt_error (eng, "illegal field size: %d", member->bit_size);
		}
		if (bit_size > (CHAR_BIT * type->simple.size)) {
			parser_stmt_error (eng, "field too big: %d", member->bit_size);
		}
		member = nel_member_alloc (eng, nel_static_symbol_alloc (eng, name, type,
								   NULL, nel_C_MEMBER, nel_lhs_type (type), nel_L_NEL,
								   eng->parser->level), 1, bit_size, 0, 0, NULL);
		parser_push_member (eng, member);
	}
}|
nel_T_COLON int_asgn_expr {
	nel_debug ({ parser_trace (eng, "s_u_dcltor->nel_T_COLON int_asgn_expr\n\n"); });
	{
		register int bit_size;
		register nel_type *type;
		register nel_member *member;
		parser_pop_integer (eng, bit_size);
		parser_top_type (eng, type, 0);
		type = parser_resolve_type (eng, type, NULL);
		member = nel_member_alloc (eng, nel_static_symbol_alloc (eng, NULL, type,
								   NULL, nel_C_MEMBER, 0, nel_L_NEL, eng->parser->level),
								   1, bit_size, 0, 0, NULL);
		parser_push_member (eng, member);
	}
};



/*****************************************************************************/
/* spec_qual_list->* productions push two items on the semantic stack.  The  */
/* type qualifier and the type specifier.                                    */
/*****************************************************************************/

spec_qual_list:
spec_qual_list2 {
	nel_debug ({ parser_trace (eng, "spec_qual_list->spec_qual_list2\n\n"); });
	{
		/********************************************/
		/* pop the type descriptor, and the         */
		/* const and volatile flags from the stack. */
		/********************************************/
		register nel_type *type;
		register unsigned_int _const;
		register unsigned_int _volatile;
		parser_pop_type (eng, type);
		if (type == NULL) {
			type = nel_int_type;
		}
		parser_pop_integer (eng, _const);
		parser_pop_integer (eng, _volatile);

		/*****************************************************/
		/* if const or volatile were specified, create a     */
		/* duplicate type descriptor with the correct type   */
		/* qualifiers.  first, check to make sure that the   */
		/* type has not already had the qualifier associated */
		/* with it (possible with a typedef)                 */
		/*****************************************************/
		if ((_const) && (type->simple._const)) {
			parser_warning (eng, "extra const type qualifier ignored");
		}
		if ((_volatile) && (type->simple._volatile)) {
			parser_warning (eng, "extra volatile type qualifier ignored");
		}
		if (((_const) && (! type->simple._const)) ||
				((_volatile) && (! type->simple._volatile))) {
			register nel_type *new_type = nel_type_alloc (eng, nel_D_VOID, 0, 0, 0, 0);
			*new_type = *type;
			new_type->simple._const |= _const;
			new_type->simple._volatile |= _volatile;
			type = new_type;
		}

		/***************************************************/
		/* now push the type descriptor back on the stack. */
		/***************************************************/
		parser_push_type (eng, type);
	}
}
;



spec_qual_list2:
spec_qual_list2 type_spec {
	/*********************************************/
	/* assume stack is set up as described below */
	/*********************************************/
	nel_debug ({ parser_trace (eng, "spec_qual_list2->spec_qual_list2 type_spec\n\n"); });
	{
		register nel_type *old_type;
		register nel_type *new_type;
		parser_pop_type (eng, new_type);
		parser_pop_type (eng, old_type);
		if (old_type == NULL) {
			parser_push_type (eng, new_type);
		} else {
			if ((new_type = nel_merge_types (eng, old_type, new_type)) == NULL) {
				parser_stmt_error (eng, "illegal type combination");
			}
			parser_push_type (eng, new_type);
		}
	}
}|
type_spec {
	/**********************************************************/
	/* set up the stack so that on the top is the current     */
	/* type descriptor, and below that are the const flag     */
	/* and the volatile flag.                                 */
	/**********************************************************/
	nel_debug ({ parser_trace (eng, "spec_qual_list2->type_spec\n\n"); });
	{
		register nel_type *type;
		parser_pop_type (eng, type);
		parser_push_integer (eng, 0);	/* volatile flag	*/
		parser_push_integer (eng, 0);	/* const flag		*/
		parser_push_type (eng, type);
	}
}|
spec_qual_list2 nel_T_CONST {
	nel_debug ({ parser_trace (eng, "spec_qual_list2->spec_qual_list2 nel_T_CONST\n\n"); });
	{
		register unsigned_int _const;
		parser_top_integer (eng, _const, 1);
		if (_const) {
			parser_warning (eng, "extra const type qualifier ignored");
		} else {
			parser_tush_integer (eng, 1, 1);	/* set const flag */
		}
	}
}|
nel_T_CONST {
	nel_debug ({ parser_trace (eng, "spec_qual_list2->nel_T_CONST\n\n"); });
	{
		parser_push_integer (eng, 0);	/* volatile flag	*/
		parser_push_integer (eng, 1);	/* const flag		*/
		parser_push_type (eng, NULL);	/* type descriptor	*/
	}
}
spec_qual_list2 nel_T_VOLATILE {
	nel_debug ({ parser_trace (eng, "spec_qual_list2->spec_qual_list2 nel_T_VOLATILE \n\n"); });
	{
		register unsigned_int _volatile;
		parser_top_integer (eng, _volatile, 2);
		if (_volatile) {
			parser_warning (eng, "extra volatile type qualifier ignored");
		} else {
			parser_tush_integer (eng, 1, 2);/* set volatile flag	*/
		}
	}
}|
nel_T_VOLATILE {
	nel_debug ({ parser_trace (eng, "spec_qual_list2->nel_T_VOLATILE\n\n"); });
	{
		parser_push_integer (eng, 1);	/* volatile flag	*/
		parser_push_integer (eng, 0);	/* const flag		*/
		parser_push_type (eng, NULL);	/* type descriptor	*/
	}
};



/*****************************************************************************/
/* enum_spec->* productions push one item,  the enum type specifier          */
/* (nel_type *), on the semantic stack.                                       */
/*****************************************************************************/

enum_spec:
nel_T_ENUM tag nel_T_LBRACE {
	nel_debug ({ parser_trace (eng, "enum_spec->nel_T_ENUM tag nel_T_LBRACE {} enum_list nel_T_RBRACE\n\n"); });
	{
		/*******************************************/
		/* in violation of the ANSI C standard, we */
		/* allow incomplete enumerated types.      */
		/*******************************************/
		register nel_type *tag_type_d;
		register nel_type *enum_type_d;
		register nel_symbol *symbol;
		parser_pop_symbol (eng, symbol);
		if (symbol->type == NULL) {
			/**************************************************/
			/* we have a newly-defined tag - insert it in the */
			/* tag tables.  we know the tag->* productions    */
			/* allocated a new symbol.                        */
			/**************************************************/
			enum_type_d = nel_type_alloc (eng, nel_D_ENUM, sizeof (int), nel_alignment_of (int), 0, 0, symbol, NULL, NULL);
			parser_push_type (eng, enum_type_d);
			tag_type_d = nel_type_alloc (eng, nel_D_ENUM_TAG, 0, 0, 0, 0, enum_type_d);
			parser_pop_type (eng, enum_type_d);
			symbol->type = tag_type_d;
			symbol->value = (char *) enum_type_d;
			parser_insert_tag (eng, symbol);
			parser_push_type (eng, NULL);
		} else if (symbol->level != eng->parser->level) {
			/*******************************************/
			/* the tag name was used in an outer scope */
			/* - create a new symbol table entry.      */
			/* allocate space for a new tag, and       */
			/* insert it in the tag table.             */
			/*******************************************/
			symbol = nel_static_symbol_alloc (eng, symbol->name, NULL, NULL, nel_C_TYPE, 0, nel_L_NEL, eng->parser->level);
			enum_type_d = nel_type_alloc (eng, nel_D_ENUM, sizeof (int), nel_alignment_of (int), 0, 0, symbol, NULL, NULL);
			tag_type_d = nel_type_alloc (eng, nel_D_ENUM_TAG, 0, 0, 0, 0, enum_type_d);
			symbol->type = tag_type_d;
			symbol->value = (char *) enum_type_d;
			parser_insert_tag (eng, symbol);
			parser_push_type (eng, NULL);
		} else if (symbol->type->simple.type != nel_D_ENUM_TAG) {
			parser_stmt_error (eng, "redeclaration of %I", symbol);
		} else {
			enum_type_d = symbol->type->tag_name.descriptor;
			nel_debug ({
						   if ((enum_type_d == NULL) || (enum_type_d->simple.type != nel_D_ENUM)) {
						   parser_fatal_error (eng, "(enum_spec->nel_T_ENUM tag nel_T_LBRACE {} enum_list nel_T_RBRACE #1): bad enum_type_d\n%1T", enum_type_d)
							   ;
						   }
					   });
			if ((enum_type_d->enumed.consts == NULL) || ((enum_type_d->enumed.consts->symbol == NULL)
					&& (enum_type_d->enumed.consts->next == NULL))) {
				/************************************************/
				/* we are defining a previously incomplete type */
				/* the tag and the enum type descriptor are     */
				/* already set up - push NULL so that we do not */
				/* compare the new type descriptor with the     */
				/* incomplete one.                              */
				/************************************************/
				parser_push_type (eng, NULL);
			} else if (symbol->level > 0) {
				/*************************************************/
				/* we may scan a declaration at level 0 mulitple */
				/* times.  if we are at a higher scoping level,  */
				/* call parser_stmt_error to jump to the end of the  */
				/* enum declaration.                             */
				/*************************************************/
				parser_stmt_error (eng, "redeclaration of %I", symbol);
			} else {
				/************************************************/
				/* we are scanning a redeclaration at level 0 - */
				/* push the symbol's type descriptor so that we */
				/* may compare its type descriptor with the     */
				/* newly-generated structure type descriptor.   */
				/************************************************/
				parser_push_type (eng, enum_type_d);
				enum_type_d = nel_type_alloc (eng, nel_D_ENUM, sizeof (int), nel_alignment_of (int), 0, 0, symbol, NULL, 0);
			}
		}

		/*******************************************************/
		/* push the enum type descriptor, and then the tag.    */
		/* enum_list->* producitons assume the enum type       */
		/* descriptor is one level down on the stack before    */
		/* they are parsed.                                    */
		/* if we are at level 0, and an enum with the same tag */
		/* was already defined, it is pushed below these two   */
		/* entries so that we may check for a redeclaration of */
		/* the enum.  (otherwise, NULL is pushed)              */
		/*******************************************************/
		parser_push_type (eng, enum_type_d);
		parser_push_symbol (eng, symbol);
	}
} enum_list nel_T_RBRACE {
	/*************************************************/
	/* for an tagged enumerated type, check for a    */
	/* redeclaration of the tag.  then form the type */
	/* descriptor, set the offset, and push it.      */
	/*************************************************/
	nel_debug ({ parser_trace (eng, "enum_spec->nel_T_ENUM tag nel_T_LBRACE enum_list nel_T_RBRACE\n\n"); });
	{
		register int last_value;
		register unsigned_int nconsts;
		register nel_list *end;
		register nel_list *consts;
		register nel_symbol *symbol;
		register nel_type *enum_type_d;
		register nel_type *old_type;
		parser_pop_integer (eng, last_value);
		parser_pop_integer (eng, nconsts);
		parser_pop_list (eng, end);
		parser_pop_list (eng, consts);
		parser_pop_symbol (eng, symbol);
		parser_pop_type (eng, enum_type_d);
		parser_pop_type (eng, old_type);
		enum_type_d->enumed.consts = consts;
		enum_type_d->enumed.nconsts = nconsts;
		if (enum_type_d->enumed.consts == NULL) {
			enum_type_d->enumed.consts = consts;
		} else if ((enum_type_d->enumed.consts->symbol == NULL) && (enum_type_d->enumed.consts->next == NULL)) {
			/***********************************************/
			/* must of had an incomplete type with a dummy */
			/* list element.  overwrite the dummy with the */
			/* first list element, so that any other type  */
			/* descriptors pointing here are now defined.  */
			/***********************************************/
			nel_copy (sizeof (nel_list), enum_type_d->enumed.consts, consts);
		}
		if (old_type != NULL) {
			/************************************************/
			/* if we are at level 0 and have already seen a */
			/* complete declaration for this type, the old  */
			/* type descriptor was pushed.  now compare the */
			/* old type descriptor with the new one to make */
			/* certain they are equal.                      */
			/************************************************/
			nel_debug ({
						   if (old_type->simple.type != nel_D_ENUM) {
						   parser_fatal_error (eng, "(enum_spec->nel_T_ENUM tag nel_T_LBRACE enum_list nel_T_RBRACE #1): bad old_type\n%1T", old_type)
							   ;
						   }
					   });
			if (nel_enum_const_diff (old_type->enumed.consts, consts)) {
				parser_error (eng, "redeclaration of %I", symbol);
			}
			parser_push_type (eng, old_type);
		} else {
			parser_push_type (eng, enum_type_d);
		}
	}
}|
nel_T_ENUM nel_T_LBRACE {
	/**************************************************/
	/* for an untagged enumerated type, just form the */
	/* type descriptor an push it, then the NULL tag. */
	/* enum_list->* productions assume that the enum  */
	/* type descriptor and the tag are on the top of  */
	/* the stack before they are parsed.              */
	/**************************************************/
	nel_debug ({ parser_trace (eng, "enum_spec->nel_T_ENUM nel_T_LBRACE {} enum_list nel_T_RBRACE\n\n"); });
	{
		register nel_type *enum_type_d;
		enum_type_d = nel_type_alloc (eng, nel_D_ENUM, sizeof (int), nel_alignment_of (int), 0, 0, NULL, 0, NULL);
		parser_push_type (eng, enum_type_d);
		parser_push_symbol (eng, NULL);
	}
} enum_list nel_T_RBRACE {
	nel_debug ({ parser_trace (eng, "enum_spec->nel_T_ENUM nel_T_LBRACE enum_list nel_T_RBRACE\n\n"); });
	{
		register int last_value;
		register unsigned_int nconsts;
		register nel_list *end;
		register nel_list *consts;
		register nel_symbol *symbol;
		register nel_type *enum_type_d;
		parser_pop_integer (eng, last_value);
		parser_pop_integer (eng, nconsts);
		parser_pop_list (eng, end);
		parser_pop_list (eng, consts);
		parser_pop_symbol (eng, symbol);  /* is NULL */
		parser_top_type (eng, enum_type_d, 0);
		enum_type_d->enumed.nconsts = nconsts;
		enum_type_d->enumed.consts = consts;
	}
}|
nel_T_ENUM tag {
	/*******************************************************/
	/* for an incompletely specified enumeration, push the */
	/* type descriptor if it exists, otherwise form an     */
	/* incomplete enumued type descriptor.                 */
	/*******************************************************/
	nel_debug ({ parser_trace (eng, "s_u_spec->nel_T_ENUM tag\n\n"); });
	{
		register nel_type *tag_type_d;
		register nel_type *enum_type_d;
		register nel_symbol *symbol;
		parser_pop_symbol (eng, symbol);
		if ((symbol->level != eng->parser->level) && (eng->parser->last_char == nel_T_SEMI)) {
			/****************************************************/
			/* the recondite rule: the tag appears in an outer  */
			/* scope, and there are no variables being declared */
			/* => this is a newly-defined incomplete type.      */
			/****************************************************/
			symbol = nel_static_symbol_alloc (eng, symbol->name, NULL, NULL, nel_C_TYPE, 0, nel_L_NEL, eng->parser->level);
			parser_push_symbol (eng, symbol);
			enum_type_d = nel_type_alloc (eng, nel_D_ENUM, sizeof (int), nel_alignment_of (int), 0, 0, symbol, NULL);
			parser_push_type (eng, enum_type_d);
			tag_type_d = nel_type_alloc (eng, nel_D_ENUM_TAG, 0, 0, 0, 0, enum_type_d);
			parser_pop_type (eng, enum_type_d);
			parser_pop_symbol (eng, symbol);
			symbol->type = tag_type_d;
			symbol->value = (char *) enum_type_d;
			parser_insert_tag (eng, symbol);
		} else if (symbol->type != NULL) {
			/**************************/
			/* the tag already exists */
			/**************************/
			if (symbol->type->simple.type != nel_D_ENUM_TAG) {
				parser_stmt_error (eng, "redeclaration of %I", symbol);
			}
			enum_type_d = symbol->type->tag_name.descriptor;
		} else {
			/***************************************/
			/* form type descriptors for both      */
			/* the tag and the enumeration itself. */
			/***************************************/
			enum_type_d = nel_type_alloc (eng, nel_D_ENUM, sizeof (int), nel_alignment_of (int), 0, 0, symbol, NULL);
			tag_type_d = nel_type_alloc (eng, nel_D_ENUM_TAG, 0, 0, 0, 0, enum_type_d);
			symbol->type = tag_type_d;
			symbol->value = (char *) enum_type_d;
			parser_insert_tag (eng, symbol);
		}
		parser_push_type (eng, enum_type_d);
	}
};



/*****************************************************************************/
/* enum_list->* productions push 4 items on the semantic stack:  the start   */
/* and end of the list of enum constants (nel_list *), the number of enum     */
/* constants, and the value of the last enum const plus 1.  We assume the    */
/* enum type descriptor and then the tag are pushed before parsing the list. */
/*****************************************************************************/

enum_list:
enum_list nel_T_COMMA enumerator {
	nel_debug ({ parser_trace (eng, "enum_list->enum_list nel_T_COMMA enumerator\n\n"); });
	{
		register nel_list *new_const;
		register nel_list *end;
		register unsigned_int nconsts;
		register int value;
		parser_pop_list (eng, new_const);	/* new const		*/
		parser_pop_integer (eng, value);
		parser_pop_integer (eng, nconsts);
		parser_pop_list (eng, end);	/* old end of list	*/
		end->next = new_const;
		parser_push_list (eng, new_const);	/* new end of list	*/
		parser_push_integer (eng, nconsts + 1); /* new # of consts	*/
		parser_push_integer (eng, value + 1); /* new default value	*/
	}
}|
empty {
	nel_debug ({ parser_trace (eng, "enum_list->empty {} enumerator\n\n"); });
	{
		parser_push_list (eng, NULL);	/* start of list	*/
		parser_push_list (eng, NULL);	/* end of list		*/
		parser_push_integer (eng, 0);	/* 0 constants parsed	*/
		parser_push_integer (eng, 0);	/* constants start at 0	*/
	}
} enumerator {
	nel_debug ({ parser_trace (eng, "enum_list->empty enumerator\n\n"); });
	{
		register nel_list *new_const;
		register nel_list *dummy;
		register int value;
		register unsigned_int nconsts;
		parser_pop_list (eng, new_const);
		parser_pop_integer (eng, value);
		parser_pop_integer (eng, nconsts);
		parser_pop_list (eng, dummy);	/* old end (NULL)	*/
		parser_pop_list (eng, dummy);	/* old start (NULL)	*/
		parser_push_list (eng, new_const);	/* new start of list	*/
		parser_push_list (eng, new_const);	/* new end of list	*/
		parser_push_integer (eng, 1);	/* 1 constant parsed	*/
		parser_push_integer (eng, value + 1); /* new default value	*/
	}
};



/*****************************************************************************/
/* enumerator->enum_const enum_val pushes 2 items on the semantic stack:     */
/* the value of the enum constant (int), and the structure (nel_enum_const *) */
/* describing the constant.  assume that the top of the stack holds the      */
/* default value for this constant, and that the enum type descriptor is 4   */
/* places below that, below the start and end of the list of enum consts,    */
/* the number of constants and the default value.                            */
/*****************************************************************************/

enumerator:
enum_const enum_val {
	nel_debug ({ parser_trace (eng, "enumerator->enum_const enum_val\n\n"); });
	{
		register int value;
		register nel_symbol *symbol;
		register nel_list *enum_const;
		register nel_type *enum_type_d;
		parser_pop_symbol (eng, symbol);
		parser_top_integer (eng, value, 0);
		parser_top_type (eng, enum_type_d, 5);
		symbol->type = enum_type_d;
		enum_const = nel_list_alloc (eng, value, symbol, NULL);
		parser_push_list (eng, enum_const);

		/********************************************/
		/* check for a redeclaration of this symbol */
		/********************************************/
		{
			register nel_symbol *old_sym;
			old_sym = parser_lookup_ident (eng, symbol->name);
			nel_debug ({
						   parser_trace (eng, "old_sym =\n%1S\n", old_sym);
					   });
			if ((old_sym == NULL) || (old_sym->level != eng->parser->level))
			{
				symbol->value = nel_static_value_alloc (eng, sizeof (int), nel_alignment_of (int));
				*((int *) (symbol->value)) = value;
				parser_insert_ident (eng, symbol);
			} else if (eng->parser->level > 0)
			{
				parser_error (eng, "redeclaration of %I", symbol);
			} else
			{
				/**********************************************/
				/* we may scan a declaration at level 0       */
				/* multiple times - make sure the old symbol  */
				/* is a constant with the same value.         */
				/**********************************************/
				register nel_type *old_type = old_sym->type;
				nel_debug ({
							   if (old_sym->type == NULL) {
							   parser_fatal_error (eng, "(enumerator->enum_const enum_val #1): old_sym->type == NULL\n%1S", old_sym)
								   ;
							   }
						   });
				if ((old_type->simple.type != nel_D_ENUM) ||
						(old_sym->class != nel_C_ENUM_CONST) ||
						(*((int *) (old_sym->value)) != value)) {
					parser_error (eng, "redeclaration of %I", symbol);
				}
				nel_debug ({
							   if (old_sym->value == NULL) {
							   parser_fatal_error (eng, "(enumerator->enum_const enum_val #1): old_sym->value== NULL\n%1S", old_sym)
								   ;
							   }
						   });
				/********************************************/
				/* else this is a redeclaration of the enum */
				/* at level 0 (legal).  do not insert the   */
				/* constant in the symbol tables            */
				/********************************************/
			}
		}
	}
};



enum_const:
ident {
	nel_debug ({ parser_trace (eng, "enum_const->ident\n\n"); });
	{
		register char *name;
		register nel_symbol *symbol;
		parser_pop_name (eng, name);
		symbol = nel_static_symbol_alloc (eng, name, NULL, NULL, nel_C_ENUM_CONST, 0, nel_L_NEL, eng->parser->level);
		parser_push_symbol (eng, symbol);
	}
};



enum_val:
nel_T_EQ int_asgn_expr {
	nel_debug ({ parser_trace (eng, "enum_val->nel_T_EQ int_asgn_expr\n\n"); });
	/***********************************************/
	/* pop the value and replace the default value */
	/* one element down on the stack with it.      */
	/* the default value is just below the symbol. */
	/***********************************************/
	{
		register int value;
		parser_pop_integer (eng, value);
		parser_tush_integer (eng, value, 1);
	}
}|
empty {
	nel_debug ({ parser_trace (eng, "enum_val->empty\n\n"); });
	/**********************************/
	/* leave the default value one    */
	/* slot down the stack unchanged. */
	/**********************************/
};



/*****************************************************************************/
/* declarator->pre_dec indirect_dec pushes 2 items on the semantic stack,    */
/* declared type and identifier name.                                        */
/*****************************************************************************/

declarator:
pre_dec indirect_dec {
	/**********************************************************/
	/* pre_dec->empty and the indirect_dec-> * productions    */
	/* set up the top of the stack so that the top entry is a */
	/* name table entry for the identifier, and one place     */
	/* down on the stack is the base type.  below the base    */
	/* type is the number of nel_D_tokens that must be added   */
	/* to the base type to produce the final type.            */
	/* Now pop the nel_D_tokens one at a time, each time       */
	/* forming a new declarator as the result.  Below each    */
	/* nel_D_token there may be other entries which must be    */
	/* popped that are used in forming the declarator.        */
	/* Before reducing by pre_dec->empty, the base type       */
	/* should be on top of the semantic stack.                */
	/**********************************************************/
	nel_debug({ parser_trace (eng, "declarator->pre_dec indirect_dec\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int n;
		register nel_D_token D_token;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		if (type == NULL) {
			type = nel_int_type;
		}
		parser_pop_integer (eng, n);
		for (; (n > 0); n--) {
			parser_pop_D_token (eng, D_token);
			switch (D_token) {
			case nel_D_ARRAY: 
				{
					register unsigned_int dims;
					register int m;
					int val_len;
					parser_pop_integer(eng, val_len);
					parser_pop_integer (eng, dims);

#ifdef NEL_PURE_ARRAYS

					/**************************************/
					/* for ANSI C array addressing, ignore*/
					/* all but the last array bound.  the */
					/* other bounds are the left operands */
					/* of the comma operator.             */
					/**************************************/
					m = 0;

#else

					/**************************************/
					/* the extended version of declarators*/
					/* includes multidimensional arrays   */
					/* in row major order, with optional  */
					/* nonzero lower bounds, for example, */
					/*                                    */
					/*    int array[2,1:3];               */
					/*                                    */
					/* is equivalent to                   */
					/*                                    */
					/*    int array[1:3][2];              */
					/*                                    */
					/* an array with base 1 and ub 3 of   */
					/* arrays with base 0 and ub 2 of ints*/
					/**************************************/
					for (m = dims - 1; (m >= 0); m--)

#endif /* NEL_PURE_ARRAYS */

					{
						register int ub_known;
						register int ub_val;
						register nel_expr *ub_expr;
						register int lb_known;
						register int lb_val;
						register nel_expr *lb_expr;
						int lb;
						int ub;

						parser_top_integer (eng, ub_known, m * 4);
						if (ub_known)
						{
							parser_top_integer (eng, ub_val, m * 4 + 1);
						} else
						{
							parser_top_expr (eng, ub_expr, m * 4 + 1);
						}
						parser_top_integer (eng, lb_known, m * 4 + 2);
						if (lb_known)
						{
							parser_top_integer (eng, lb_val, m * 4 + 3);
						} else
						{
							parser_top_expr (eng, lb_expr, m * 4 + 3);
						}



#if 0

						if (lb_known && ub_known && (lb_val > ub_val))
						{
							parser_stmt_error (eng, "illegal dimension bound specification");
						}

						/*****************************************/
						/* note that we can't use the ? operator */
						/* here because the possible args for lb */
						/* or ub have different type.            */
						/*****************************************/
						if (lb_known)
						{
							if (ub_known) {
								type = nel_type_alloc (eng, nel_D_ARRAY, 0, type->simple.alignment, 1, 0, type, nel_int_type, 1, lb_val, 1, ub_val);
							} else {
								type = nel_type_alloc (eng, nel_D_ARRAY, 0, type->simple.alignment, 1, 0, type, nel_int_type, 1, lb_val, 0, ub_expr);
							}
						} else
						{
							if (ub_known) {
								type = nel_type_alloc (eng, nel_D_ARRAY, 0, type->simple.alignment, 1, 0, type, nel_int_type, 0, lb_expr, 1, ub_val);
							} else {
								type = nel_type_alloc (eng, nel_D_ARRAY, 0, type->simple.alignment, 1, 0, type, nel_int_type, 0, lb_expr, 0, ub_expr);
							}
						}

#else
						/***************************/
						/* extract the lower bound */
						/***************************/
						if (lb_known) {
							lb = lb_val;
						} else {
							register nel_expr *bound_expr;
							register nel_symbol *bound_sym = NULL;
							
							if ((bound_expr = lb_expr ) != NULL ){
								bound_sym =  intp_eval_expr_2(eng, bound_expr);
							}

							if ((bound_sym != NULL) && (bound_sym->type != NULL) && (bound_sym->value != NULL) && (nel_integral_D_token (bound_sym->type->simple.type))) {
								parser_coerce (eng, nel_int_type, (char *)&lb, bound_sym->type, (char *)bound_sym->value);

							} else {
								lb = 0;
								/*
								if ((bound_sym == NULL) || (bound_sym->name == NULL)) {
									if (name == NULL) {
										parser_warning (eng, "unresolved dim lb defaults to 0");
									} else {
										parser_warning (eng, "unresolved dim lb defaults to 0: symbol %s", name);
									}
								} else {
									if (name == NULL) {
										parser_warning (eng, "unresolved dim lb %I defaults to 0", bound_sym);
									} else {
										parser_warning (eng, "unresolved dim lb %I defaults to 0: symbol %s", bound_sym, name);
									}
								}
								*/
							}
						}

						/***************************/
						/* extract the upper bound */
						/***************************/
						if (ub_known) {
							ub = ub_val;
						} else {

							register nel_expr *bound_expr;
							register nel_symbol *bound_sym = NULL;
							

							if((bound_expr = ub_expr ) != NULL ){
								bound_sym =  intp_eval_expr_2(eng, bound_expr);

							}	

							if ((bound_sym != NULL) && (bound_sym->type != NULL) && (bound_sym->value != NULL) && (nel_integral_D_token (bound_sym->type->simple.type))) {
								parser_coerce (eng, nel_int_type, (char *)&ub, bound_sym->type, (char *)bound_sym->value);
								nel_debug ({
									nel_trace (eng, "bound_sym =\n%1Svalue = 0x%x\n*value = 0x%x\n\n", bound_sym, bound_sym->value, *((int *) (bound_sym->value)));
								});
							} else {
								ub = lb;

								/*
								if ((bound_sym == NULL) || (bound_sym->name == NULL)) {
									if (name == NULL) {
										parser_warning (eng, "unresolved dim ub defaults to lb (%d)", lb);
									} else {
										parser_warning (eng, "unresolved dim ub defaults to lb (%d): symbol %s", lb, name);
									}
								} else {
									if (name == NULL) {
										parser_warning (eng, "unresolved dim ub %I defaults to lb (%d)", bound_sym, lb);
									} else {
										parser_warning (eng, "unresolved dim ub %I defaults to lb (%d): symbol %s", bound_sym, lb, name);
									}
								}
								*/
							}

						}

						/****************************************/
						/* make sure lb <= ub.  if lb > ub, set */
						/* lb = ub, and emit a warning          */
						/****************************************/
						if (lb > ub) {
							if (name == NULL) {
								parser_warning (eng, "dim ub (%d) defaults to larger lb (%d)", ub, lb);
							} else {
								parser_warning (eng, "dim ub (%d) defaults to larger lb (%d): symbol %s", ub, lb, name);
							}
							ub = lb;
						}

						/***********************************************/
						/* now create the return value type descriptor */
						/* the base type must be integral, so don't    */
						/* bother calling parser_resolve_type on it        */
						/***********************************************/
						//int val_len = type->array.val_len;
						type = nel_type_alloc (eng, 
							nel_D_ARRAY, 
							(ub - lb + 1 ) 
								* type->simple.size, 
							type->simple.alignment, 
							1, 0,	type, 
						nel_int_type, 1, lb, 1, ub);
						type->array.val_len = val_len;

#endif


					}

					/**************************************/
					/* while debugging, pop the stack 1   */
					/* element at at time, so that the    */
					/* ['s and ]'s match up in the trace. */
					/**************************************/
					for (m = 0; (m < dims); m++) {
						register int integer;
						register nel_expr *expr;
						parser_pop_integer (eng, integer);
						if (integer) {
							parser_pop_integer (eng, integer);
						} else {
							parser_pop_expr (eng, expr);
						}
						parser_pop_integer (eng, integer);
						if (integer) {
							parser_pop_integer (eng, integer);
						} else {
							parser_pop_expr (eng, expr);
						}
					}

				}
				break;
			case nel_D_FUNCTION: 
				{
					register nel_list *args;
					register unsigned_int new_style;
					register unsigned_int var_args;
					parser_pop_integer (eng, new_style);
					parser_pop_integer (eng, var_args);
					parser_pop_list (eng, args);

					/************************************/
					/* check for an illegal return type */
					/************************************/
					switch (type->simple.type) {
					case nel_D_CHAR:
					case nel_D_COMPLEX:
					case nel_D_COMPLEX_DOUBLE:
					case nel_D_COMPLEX_FLOAT:
					case nel_D_DOUBLE:
					case nel_D_ENUM:
					case nel_D_FLOAT:
					case nel_D_INT:
					case nel_D_LONG:
					case nel_D_LONG_COMPLEX:
					case nel_D_LONG_COMPLEX_DOUBLE:
					case nel_D_LONG_COMPLEX_FLOAT:
					case nel_D_LONG_DOUBLE:
					case nel_D_LONG_FLOAT:
					case nel_D_LONG_INT:
					case nel_D_POINTER:
					case nel_D_SHORT:
					case nel_D_SHORT_INT:
					case nel_D_SIGNED:
					case nel_D_SIGNED_CHAR:
					case nel_D_SIGNED_INT:
					case nel_D_SIGNED_LONG:
					case nel_D_SIGNED_LONG_INT:
					case nel_D_SIGNED_SHORT:
					case nel_D_SIGNED_SHORT_INT:
					case nel_D_STRUCT:
					case nel_D_UNION:
					case nel_D_UNSIGNED:
					case nel_D_UNSIGNED_CHAR:
					case nel_D_UNSIGNED_INT:
					case nel_D_UNSIGNED_LONG:
					case nel_D_UNSIGNED_LONG_INT:
					case nel_D_UNSIGNED_SHORT:
					case nel_D_UNSIGNED_SHORT_INT:
					case nel_D_VOID:
						break;
					default:
						parser_stmt_error (eng, "illegal function return type");
					}
					type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0,
										   new_style, var_args, type, args, NULL, NULL);
				}
				break;
			case nel_D_POINTER: {
					register unsigned_int _const;
					register unsigned_int _volatile;
					parser_pop_integer (eng, _const);
					parser_pop_integer (eng, _volatile);
					type = nel_type_alloc (eng, nel_D_POINTER, sizeof (char *), nel_alignment_of (char *), _const, _volatile, type);
				}
				break;
			default:
				parser_fatal_error (eng, "(declarator->pre_dec indirect_dec #1): bad nel_D_token: %D", D_token);
				break;
			}
		}

		/***********************************/
		/* push the final type descriptor, */
		/* and then the name.              */
		/***********************************/
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
};



pre_dec:
empty {
	nel_debug ({ parser_trace (eng, "pre_dec->empty\n\n"); });
	/**********************************************************/
	/* set up the top of the stack so that the top entry is   */
	/* the name of the identifier, and below that is the base */
	/* type.  below that is the number of nel_D_tokens that    */
	/* must be added to the base type to produce the final    */
	/* declarator.  At the time we reduce by this production, */
	/* the base type should be on the top of the stack,       */
	/* either from the productions dec_specs->*,              */
	/* spec_qual_list->*, or trans_unit->external_dec.        */
	/**********************************************************/
	{
		register nel_type *type;
		parser_top_type (eng, type, 0);
		parser_push_integer (eng, 0);
		parser_push_type (eng, type);
		parser_push_name (eng, NULL);
	}
};



/*****************************************************************************/
/* indirect_dec->* productions pop a name (char *), a type descriptor        */
/* (nel_type *), and an integer (int) from the semantic stack, then push an   */
/* nel_D_token (nel_D_POINTER), the integer + 1, the type descriptor, and then */
/* the symbol back on the stack.                                             */
/*****************************************************************************/

indirect_dec:
nel_T_STAR indirect_dec {
	nel_debug ({ parser_trace (eng, "indirect_dec->nel_T_STAR indirect_dec\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_integer (eng, 0);	/* volatile flag	*/
		parser_push_integer (eng, 0);	/* const flag		*/
		parser_push_D_token (eng, nel_D_POINTER);
		parser_push_integer (eng, n + 1);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
}|
nel_T_STAR nel_T_CONST extra_cnsts indirect_dec {
	nel_debug ({ parser_trace (eng, "indirect_dec->nel_T_STAR nel_T_CONST extra_cnsts indirect_dec\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_integer (eng, 0);	/* volatile flag	*/
		parser_push_integer (eng, 1);	/* const flag		*/
		parser_push_D_token (eng, nel_D_POINTER);
		parser_push_integer (eng, n + 1);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
}|
nel_T_STAR nel_T_VOLATILE extra_vols indirect_dec {
	nel_debug ({ parser_trace (eng, "indirect_dec->nel_T_STAR nel_T_VOLATILE extra_vols indirect_dec\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_integer (eng, 1);	/* volatile flag	*/
		parser_push_integer (eng, 0);	/* const flag		*/
		parser_push_D_token (eng, nel_D_POINTER);
		parser_push_integer (eng, n + 1);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
}|
nel_T_STAR nel_T_CONST extra_cnsts nel_T_VOLATILE extra_cnst_vols indirect_dec {
	nel_debug ({ parser_trace (eng, "indirect_dec->nel_T_STAR nel_T_CONST extra_cnsts nel_T_VOLATILE extra_cnst_vols indirect_dec\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_integer (eng, 1);	/* volatile flag	*/
		parser_push_integer (eng, 1);	/* const flag		*/
		parser_push_D_token (eng, nel_D_POINTER);
		parser_push_integer (eng, n + 1);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
}|
nel_T_STAR nel_T_VOLATILE extra_vols nel_T_CONST extra_cnst_vols indirect_dec {
	nel_debug ({ parser_trace (eng, "indirect_dec->nel_T_STAR nel_T_VOLATILE extra_vols nel_T_CONST extra_cnst_vols indirect_dec\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_integer (eng, 1);	/* volatile flag	*/
		parser_push_integer (eng, 1);	/* const flag		*/
		parser_push_D_token (eng, nel_D_POINTER);
		parser_push_integer (eng, n + 1);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
}|
direct_dec {
	nel_debug ({ parser_trace (eng, "indirect_dec->direct_dec\n\n"); });
};



extra_cnsts:
nel_T_CONST extra_cnsts {
	nel_debug ({ parser_trace (eng, "extra_cnsts->nel_T_CONST extra_cnsts\n\n"); });
	{
		parser_warning (eng, "extra const type qualifier ignored");
	}
}|
empty {
	nel_debug ({ parser_trace (eng, "extra_cnsts->empty\n\n"); });
};



extra_vols:
nel_T_VOLATILE extra_vols {
	nel_debug ({ parser_trace (eng, "extra_vols->nel_T_VOLATILE extra_vols\n\n"); });
	{
		parser_warning (eng, "extra volatile type qualifier ignored");
	}
}|
empty {
	nel_debug ({ parser_trace (eng, "extra_vols->empty\n\n"); });
};



extra_cnst_vols:
nel_T_CONST extra_cnst_vols {
	nel_debug ({ parser_trace (eng, "extra_cnst_vols->nel_T_CONST extra_cnst_vols\n\n"); });
	{
		parser_warning (eng, "extra const type qualifier ignored");
	}
}|
nel_T_VOLATILE extra_cnst_vols {
	nel_debug ({ parser_trace (eng, "extra_cnst_vols->nel_T_VOLATILE extra_cnst_vols\n\n"); });
	{
		parser_warning (eng, "extra volatile type qualifier ignored");
	}
}|
empty {
	nel_debug ({ parser_trace (eng, "extra_cnst_vols->empty\n\n"); });
};



/*****************************************************************************/
/* direct_dec->* productions pop a name (char *), a type descriptor          */
/* (nel_type *), and an integer (int) from the semantic stack, then push a    */
/* nel_D_token (and possibly other items which can be determined from the     */
/* nel_D_token), the integer + 1, the type descriptor, and then the name      */
/* back on the stack.  When the actual identifier is reached, we replace the */
/* symbol (NULL) on the top of the stack with the symbol for the identifier  */
/* just encountered.                                                         */
/*****************************************************************************/

direct_dec:
ident {
	/**********************************************************/
	/* pop the name of the ident, and replace the NULL name   */
	/* on top of the stack with it.                           */
	/**********************************************************/
	nel_debug ({ parser_trace (eng, "direct_dec->ident\n\n"); });
	{
		register char *name;
		parser_pop_name (eng, name);
		parser_tush_name (eng, name, 0);
	}
}|
nel_T_LPAREN indirect_dec nel_T_RPAREN {
	nel_debug ({ parser_trace (eng, "direct_dec->nel_T_LPAREN indirect_dec nel_T_RPAREN\n\n"); });
}|
direct_dec nel_T_LBRACK dim_bd_list nel_T_RBRACK {
	/*********************************************************/
	/* pop the number of declarators seed so far, and leave  */
	/* everything else set up as dim_bd_list->* productions  */
	/* have formed it.  push the nel_D_token nel_D_ARRAY, then */
	/* the number of declarators + 1.                        */
	/*********************************************************/
	nel_debug ({ parser_trace (eng, "direct_dec->direct_dec nel_T_LBRACK dim_bd_list nel_T_RBRACK\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_integer(eng, 0);
		parser_push_D_token (eng, nel_D_ARRAY);
		parser_push_integer (eng, n + 1);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
}|
direct_dec nel_T_LBRACK nel_T_RBRACK {
	/*********************************************************/
	/* pop the number of declarators seed so far, and leave  */
	/* everything else set up as dim_bd_list->* productions  */
	/* have formed it.  push the nel_D_token nel_D_ARRAY, then */
	/* the number of declarators + 1.                        */
	/*********************************************************/
	nel_debug ({ parser_trace (eng, "direct_dec->direct_dec nel_T_LBRACK nel_T_RBRACK\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_integer (eng, 0);	/* lb		*/
		parser_push_integer (eng, 1);	/* lb known	*/
		parser_push_expr (eng, NULL);	/* ub		*/
		parser_push_integer (eng, 0);	/* ub not known	*/
		parser_push_integer (eng, 1);	/* single dim	*/
		parser_push_integer(eng, 1);	
		parser_push_D_token (eng, nel_D_ARRAY);
		parser_push_integer (eng, n + 1);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
}|
direct_dec nel_T_LPAREN param_type_list nel_T_RPAREN {
	/**********************************************************/
	/* for functions, first pop the argument list (nel_list *) */
	/* if there is one, then pop the name, type, and n,       */
	/* then push the argument list (or NULL if it is empty),  */
	/* a boolean signifying if this is a new-style dec or not,*/
	/* followed by the nel_D_token nel_D_FUNCTION, then push    */
	/* n + 1, the type, and the name back on the stack.       */
	/**********************************************************/
	nel_debug ({ parser_trace (eng, "direct_dec->direct_dec nel_T_LPAREN param_type_list nel_T_RPAREN\n\n"); });
	{
		register unsigned_int var_args;
		register nel_list *args;
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_integer (eng, var_args);
		parser_pop_list (eng, args);
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_list (eng, args);
		parser_push_integer (eng, var_args);
		parser_push_integer (eng, 1);  /* is new sytle dec */
		parser_push_D_token (eng, nel_D_FUNCTION);
		parser_push_integer (eng, n + 1);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
}|
direct_dec nel_T_LPAREN formal_list nel_T_RPAREN {
	nel_debug ({ parser_trace (eng, "direct_dec->direct_dec nel_T_LPAREN formal_list nel_T_RPAREN\n\n"); });
	{
		register nel_list *end;
		register nel_list *args;
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_list (eng, end);   /* formal_list->* pushes end */
		parser_pop_list (eng, args);  /* of list and start.        */
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_list (eng, args);
		parser_push_integer (eng, 0);  /* is not var # of args */
		parser_push_integer (eng, 0);  /* is not old sytle dec */
		parser_push_D_token (eng, nel_D_FUNCTION);
		parser_push_integer (eng, n + 1);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
}|
direct_dec nel_T_LPAREN nel_T_RPAREN {
	nel_debug ({ parser_trace (eng, "direct_dec->direct_dec nel_T_LPAREN nel_T_RPAREN\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_list (eng, NULL);
		parser_push_integer (eng, 0);  /* is not var # of args */
		parser_push_integer (eng, 0);  /* is not old sytle dec */
		parser_push_D_token (eng, nel_D_FUNCTION);
		parser_push_integer (eng, n + 1);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
};



dim_bd_list:
dim_bd_list nel_T_COMMA dim_bd {
	/*********************************************************/
	/* First, pop the name, type, and number of declarators. */
	/* for every dimension bound, push the lower bound, (an  */
	/* integer or an expression), a flag telling if the lb   */
	/* evaluated to an int, the upper bound and its flag.    */
	/* On top of this push the number of bounds between the  */
	/* brackets, then restore the number of declarators,     */
	/* identifier name, and type.                            */
	/*********************************************************/
	nel_debug ({ parser_trace (eng, "dim_bd_list->dim_bd_list nel_T_COMMA dim_bd\n\n"); });
	{
		register int ub_known;
		register int ub_val;
		register nel_expr *ub_expr;
		register int lb_known;
		register int lb_val;
		register nel_expr *lb_expr;
		register char *name;
		register nel_type *type;
		register int n;
		register unsigned_int dims;
		parser_pop_integer (eng, ub_known);
		if (ub_known) {
			parser_pop_integer (eng, ub_val);
		} else {
			parser_pop_expr (eng, ub_expr);
		}
		parser_pop_integer (eng, lb_known);
		if (lb_known) {
			parser_pop_integer (eng, lb_val);
		} else {
			parser_pop_expr (eng, lb_expr);
		}
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);	/* total # of decltrs	*/
		parser_pop_integer (eng, dims);	/* # dims between []	*/
		if (lb_known) {
			parser_push_integer (eng, lb_val);
		} else {
			parser_push_expr (eng, lb_expr);
		}
		parser_push_integer (eng, lb_known);
		if (ub_known) {
			parser_push_integer (eng, ub_val);
		} else {
			parser_push_expr (eng, ub_expr);
		}
		parser_push_integer (eng, ub_known);
		parser_push_integer (eng, dims + 1);
		parser_push_integer (eng, n);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
}|
dim_bd {
	/*********************************************************/
	/* set up the stack as described above, except that we   */
	/* do not popd the number of bounds seen so far, and     */
	/* push a 1 in its place, since this is the first bound. */
	/*********************************************************/
	nel_debug ({ parser_trace (eng, "dim_bd_list->dim_bd\n\n"); });
	{
		register int ub_known;
		register int ub_val;
		register nel_expr *ub_expr;
		register int lb_known;
		register int lb_val;
		register nel_expr *lb_expr;
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_integer (eng, ub_known);
		if (ub_known) {
			parser_pop_integer (eng, ub_val);
		} else {
			parser_pop_expr (eng, ub_expr);
		}
		parser_pop_integer (eng, lb_known);
		if (lb_known) {
			parser_pop_integer (eng, lb_val);
		} else {
			parser_pop_expr (eng, lb_expr);
		}
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);	/* total # of decltrs	*/
		if (lb_known) {
			parser_push_integer (eng, lb_val);
		} else {
			parser_push_expr (eng, lb_expr);
		}
		parser_push_integer (eng, lb_known);
		if (ub_known) {
			parser_push_integer (eng, ub_val);
		} else {
			parser_push_expr (eng, ub_expr);
		}
		parser_push_integer (eng, ub_known);
		parser_push_integer (eng, 1);	/* # dims between []	*/
		parser_push_integer (eng, n);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
};



dim_bd:
asgn_expr {
	/**********************************************************/
	/* pop the dimension size expression and subtract 1 from  */
	/* it.  push the lower bound (0), a 1 to indicate that it */
	/* is known, push the modified upper bound expression,    */
	/* and a 0 to indicate that it is unevaluated.            */
	/**********************************************************/
	nel_debug ({ parser_trace (eng, "dim_bd->asgn_expr\n\n"); });
	{
		register nel_expr *expr;
		parser_pop_expr (eng, expr);
		expr = nel_expr_alloc (eng, nel_O_SUB, expr, nel_one_expr);
		parser_push_integer (eng, 0);
		parser_push_integer (eng, 1);
		parser_push_expr (eng, expr);
		parser_push_integer (eng, 0);
	}
}|
asgn_expr nel_T_COLON asgn_expr {
	/*********************************************************/
	/* pop the ub expression, leave the lb expression on the */
	/* stack, push a 0 to indicate that it is unevaluated,   */
	/* push the ub expression back on the stack, then push   */
	/* another 0 to indicate that it is unevaluated.         */
	/*********************************************************/
	nel_debug ({ parser_trace (eng, "dim_bd->asgn_expr nel_T_COLON asgn_expr\n\n"); });
	{
		nel_expr *ub;
		parser_pop_expr (eng, ub);
		parser_push_integer (eng, 0);
		parser_push_expr (eng, ub);
		parser_push_integer (eng, 0);
	}
}|
asgn_expr nel_T_COLON {
	/**********************************************************/
	/* leave the lower bound pushed, and push a 0 to indicate */
	/* that it is unevaluated, push a NULL for the ub, and    */
	/* push a 0 to indicate that it is unevaluated/unkonwn.   */
	/**********************************************************/
	nel_debug ({ parser_trace (eng, "dim_bd->asgn_expr nel_T_COLON\n\n"); });
	{
		parser_push_integer (eng, 0);
		parser_push_expr (eng, NULL);
		parser_push_integer (eng, 0);
	}
};



/*****************************************************************************/
/* param_type_list->* productions push 2 itmes on the semantic stack: the    */
/* argument list (nel_list *), and a boolean, true iff there was ellipsis at  */
/* the end of the list (int).                                                */
/*****************************************************************************/

param_type_list:
param_list {
	nel_debug ({ parser_trace (eng, "param_type_list->param_list\n\n"); });
	{
		register nel_list *end;
		register nel_list *start;
		parser_pop_list (eng, end);
		parser_pop_list (eng, start);
		nel_debug ({
					   if ((start == NULL) || (start->symbol == NULL) || (start->symbol->type == NULL)) {
					   parser_fatal_error (eng, "(param_type_list->param_list #1): bad arg list\n%1L", start)
						   ;
					   }
				   });

		/************************************/
		/* check for an arg list consisting */
		/* solely of the keyword "void".    */
		/************************************/
		if ((start->symbol->name == NULL) && (start->next == NULL)
				&& (start->symbol->type->simple.type == nel_D_VOID)) {
			parser_push_list (eng, NULL);
			nel_list_dealloc (start);
		} else {
			/******************************************/
			/* scan through the formal args, checking */
			/* for void and incomplete types.         */
			/******************************************/
			register nel_list *scan = start;
			for (; (scan != NULL); (scan = scan->next)) {
				register nel_symbol *symbol = scan->symbol;
				nel_debug ({
							   if ((symbol == NULL) || (symbol->type == NULL)) {
							   parser_fatal_error (eng, "(param_type_list->param_list #2): bad formal\n%1S", symbol)
								   ;
							   }
						   });
				if (symbol->type->simple.type == nel_D_VOID) {
					if (symbol->name != NULL) {
						parser_stmt_error (eng, "void type for %I", symbol);
					} else {
						parser_stmt_error (eng, "void type for argument");
					}
				} else if (nel_type_incomplete (symbol->type)) {
					if (symbol->name != NULL) {
						parser_stmt_error (eng, "incomplete type for %I", symbol);
					} else {
						parser_stmt_error (eng, "incomplete type for argument");
					}
				}
			}
			parser_push_list (eng, start);
		}
		parser_push_integer (eng, 0);  /* not var # of args */
	}
}|
param_list nel_T_COMMA nel_T_ELLIPSIS {
	nel_debug ({ parser_trace (eng, "param_type_list->param_list\n\n"); });
	{
		register nel_list *start;
		register nel_list *end;
		parser_pop_list (eng, end);
		parser_pop_list (eng, start);
		{
			/******************************************/
			/* scan through the formal args, checking */
			/* for void and incomplete types.         */
			/******************************************/
			register nel_list *scan = start;
			for (; (scan != NULL); (scan = scan->next)) {
				register nel_symbol *symbol = scan->symbol;
				nel_debug ({
					if ((symbol == NULL) || (symbol->type == NULL)) {
						   parser_fatal_error (eng, "(param_type_list->param_list #2): bad formal\n%1S", symbol) ;
					}
				});
				if (symbol->type->simple.type == nel_D_VOID) {
					if (symbol->name != NULL) {
						parser_stmt_error (eng, "void type for %I", symbol);
					} else {
						parser_stmt_error (eng, "void type for argument");
					}
				} else if (nel_type_incomplete (symbol->type)) {
					if (symbol->name != NULL) {
						parser_stmt_error (eng, "incomplete type for %I", symbol);
					} else {
						parser_stmt_error (eng, "incomplete type for argument");
					}
				}
			}
		}
		parser_push_list (eng, start);
		parser_push_integer (eng, 1);  /* var # of args */
	}
};



param_list:
param_list nel_T_COMMA param_dec {
	/**********************************************************/
	/* pop the new arg and the end of the list, append the    */
	/* arg to the end of the list, then push the new end.     */
	/**********************************************************/
	nel_debug ({ parser_trace (eng, "param_list->param_list nel_T_COMMA param_dec\n\n"); });
	{
		register nel_list *new_arg;
		register nel_list *end;
		parser_pop_list (eng, new_arg);
		parser_pop_list (eng, end);
		end->next = new_arg;
		parser_push_list (eng, new_arg);
	}
}|
param_dec {
	/**********************************************************/
	/* the first argument is the start & end of the arg list  */
	/* so far.  duplicate top of stack, so that the new arg   */
	/* is in both places.                                     */
	/**********************************************************/
	nel_debug ({ parser_trace (eng, "param_list->param_dec\n\n"); });
	{
		register nel_list *list;
		parser_top_list (eng, list, 0);
		parser_push_list (eng, list); /* start & end of list */
	}
};



param_dec:
dec_specs declarator {
	nel_debug ({ parser_trace (eng, "param_dec->dec_specs declarator\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register nel_type *base_type;
		register int stor_class;
		register nel_list *list;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_type (eng, base_type);  /* pop the dec_spec->* stuff */
		parser_pop_token (eng, stor_class);
		nel_debug ({
					   if (name == NULL) {
					   parser_fatal_error (eng, "(param_dec->dec_specs declarator #1): NULL name")
						   ;
					   }
					   if (type == NULL) {
					   parser_fatal_error (eng, "(param_dec->dec_specs declarator #2): NULL type")
						   ;
					   }
				   });

		/*********************************************/
		/* promote arrays and functions to pointers. */
		/* since this is a new-style function, do    */
		/* NOT promote floats to doubles.            */
		/*********************************************/
		if (type->simple.type == nel_D_ARRAY) {
			type = nel_type_alloc (eng, nel_D_POINTER, sizeof (char *), nel_alignment_of (char *), type->array._const, type->array._volatile, type->array.base_type);
		} else if (type->simple.type == nel_D_FUNCTION) {
			type = nel_type_alloc (eng, nel_D_POINTER, sizeof (char *), nel_alignment_of (char *), 0, 0, type);
		}

		/*******************************/
		/* check for an illegal class. */
		/*******************************/
		switch (stor_class) {
		case nel_T_TYPEDEF:
		case nel_T_EXTERN:
		case nel_T_STATIC:
			parser_warning (eng, "illegal class for %s", name);
			break;
		case nel_T_AUTO:
		case nel_T_REGISTER:
		case nel_T_NULL:
			break;
		default:
			parser_fatal_error (eng, "(param_dec->dec_specs declarator #3): bad class %N", stor_class);
			break;
		}

		/*************************************************/
		/* allocate a symbol with class nel_C_FORMAL at   */
		/* level 1 (even though the current level is 0). */
		/* for a list structure around it.               */
		/*************************************************/
		list = nel_list_alloc (eng, 0, nel_static_symbol_alloc (eng, name, type,
							   NULL, nel_C_FORMAL, nel_lhs_type (type), nel_L_NEL, 1), NULL);
		parser_push_list (eng, list);
	}
}|
dec_specs abst_dec {
	nel_debug ({ parser_trace (eng, "param_dec->dec_specs abst_dec\n\n"); });
	{
		register nel_type *type;
		register nel_type *base_type;
		register int stor_class;
		register nel_list *list;
		parser_pop_type (eng, type);
		parser_pop_type (eng, base_type);  /* pop the dec_spec->* stuff */
		parser_pop_token (eng, stor_class);

		/*********************************************/
		/* promote arrays and functions to pointers. */
		/* since this is a new-style function, do    */
		/* NOT promote floats to doubles.            */
		/*********************************************/
		if (type->simple.type == nel_D_ARRAY) {
			type = nel_type_alloc (eng, nel_D_POINTER, sizeof (char *), nel_alignment_of (char *), type->array._const, type->array._volatile, type->array.base_type);
		} else if (type->simple.type == nel_D_FUNCTION) {
			type = nel_type_alloc (eng, nel_D_POINTER, sizeof (char *), nel_alignment_of (char *), 0, 0, type);
		}

		/*******************************/
		/* check for an illegal class. */
		/*******************************/
		switch (stor_class) {
		case nel_T_TYPEDEF:
			parser_warning (eng, "illegal class for argument: typedef");
			break;
		case nel_T_EXTERN:
			parser_warning (eng, "illegal class for argument: extern");
			break;
		case nel_T_STATIC:
			parser_warning (eng, "illegal class for argument: static");
			break;
		case nel_T_AUTO:
		case nel_T_REGISTER:
		case nel_T_NULL:
			break;
		default:
			parser_fatal_error (eng, "(param_dec->dec_specs abst_dec #1): bad class %N", stor_class);
			break;
		}

		/*************************************************/
		/* allocate a symbol with class nel_C_FORMAL at   */
		/* level 1 (even though the current level is 0). */
		/* for a list structure around it.               */
		/*************************************************/
		list = nel_list_alloc (eng, 0, nel_static_symbol_alloc (eng, NULL, type,
							   NULL, nel_C_FORMAL, nel_lhs_type (type), nel_L_NEL, 1), NULL);
		parser_push_list (eng, list);
	}
}|
dec_specs {
	nel_debug ({ parser_trace (eng, "param_dec->dec_specs\n\n"); });
	{
		register nel_type *type;
		register int stor_class;
		register nel_list *list;
		parser_pop_type (eng, type);
		parser_pop_token (eng, stor_class);

		/******************************************************/
		/* this can't be an array or function, so there is no */
		/* need to promote arrays and functions to pointers.  */
		/******************************************************/

		/*******************************/
		/* check for an illegal class. */
		/*******************************/
		switch (stor_class) {
		case nel_T_TYPEDEF:
			parser_warning (eng, "illegal class for argument: typedef");
			break;
		case nel_T_EXTERN:
			parser_warning (eng, "illegal class for argument: extern");
			break;
		case nel_T_STATIC:
			parser_warning (eng, "illegal class for argument: static");
			break;
		case nel_T_AUTO:
		case nel_T_REGISTER:
		case nel_T_NULL:
			break;
		default:
			parser_fatal_error (eng, "(param_dec->dec_specs #1): bad stor_class %N", stor_class);
			break;
		}

		/*************************************************/
		/* allocate a symbol with class nel_C_FORMAL at   */
		/* level 1 (even though the current level is 0). */
		/* for a list structure around it.               */
		/*************************************************/
		list = nel_list_alloc (eng, 0, nel_static_symbol_alloc (eng, NULL, type,
							   NULL, nel_C_FORMAL, nel_lhs_type (type), nel_L_NEL, 1), NULL);
		parser_push_list (eng, list);
	}
};



/*****************************************************************************/
/* formal_list->* productions push 2 items: the start and end of the arg     */
/* list (nel_list*).                                                          */
/*****************************************************************************/

formal_list:
formal_list nel_T_COMMA formal {
	/**********************************************************/
	/* pop the new arg and the end of the list, append the    */
	/* arg to the end of the list, then push the new end.     */
	/**********************************************************/
	nel_debug ({ parser_trace (eng, "formal_list->formal_list nel_T_COMMA formal\n\n"); });
	{
		register nel_list *new_arg;
		register nel_list *end;
		parser_pop_list (eng, new_arg);
		parser_pop_list (eng, end);
		end->next = new_arg;
		parser_push_list (eng, new_arg);
	}
}|
formal {
	/**********************************************************/
	/* the first argument is the start & end of the arg list  */
	/* so far.  duplicate top of stack, so that the new arg   */
	/* is in both places.                                     */
	/**********************************************************/
	nel_debug ({ parser_trace (eng, "formal_list->formal\n\n"); });
	{
		register nel_list *list;
		parser_top_list (eng, list, 0);
		parser_push_list (eng, list);
	}
};



formal:
ident {
	nel_debug ({ parser_trace (eng, "formal->ident\n\n"); });
	{
		/*************************************************/
		/* allocate a symbol with class nel_C_FORMAL at   */
		/* level 1 (even though the current level is 0). */
		/*************************************************/
		register char *name;
		register nel_symbol *symbol;
		register nel_list *list;
		parser_pop_name (eng, name);
		symbol = nel_static_symbol_alloc (eng, name, NULL, NULL, nel_C_FORMAL, 0, nel_L_NEL, 1);
		list = nel_list_alloc (eng, 0, symbol, NULL);
		parser_push_list (eng, list);
	}
};



/*****************************************************************************/
/* type_name->* productions push one item, the type descriptor (nel_type *)   */
/*****************************************************************************/

type_name:
spec_qual_list abst_dec {
	nel_debug ({ parser_trace (eng, "type_name->spec_qual_list abst_dec\n\n"); });
	/************************************/
	/* pop the type and base type, and  */
	/* push the type back on the stack. */
	/************************************/
	{
		register nel_type *type;
		register nel_type *base_type;
		parser_pop_type (eng, type);
		parser_pop_type (eng, base_type);
		parser_push_type (eng, type);
	}
}|
spec_qual_list {
	nel_debug ({ parser_trace (eng, "type_name->spec_qual_list\n\n"); });
	/******************************/
	/* leave the base type pushed */
	/******************************/
};



/*****************************************************************************/
/* abst_dec->pre_dec indrct_abst_dec pushes one item, the type descriptor    */
/* (nel_type *)                                                               */
/*****************************************************************************/

abst_dec:
pre_dec indrct_abst_dec {
	/**********************************************************/
	/* pre_dec->empty and the indrct_abst_dec->* productions  */
	/* set up the top of the stack so that the top entry is   */
	/* the name of the identifier, and below that is the base */
	/* type.  below that is the number of nel_D_tokens that    */
	/* must be added to the base type to produce the final    */
	/* abstract declarator.  In the case of abstract decs,    */
	/* no symbol is encountered, so the top of the stack      */
	/* contains NULL_symbol.  We must have it there, though:  */
	/* when we reduce by pre_dec->empty we do not know        */
	/* whether to expect an abstract declarator or just a     */
	/* declarator.                                            */
	/* Before reducing by pre_dec->empty, the base type       */
	/* should be on top of the semantic stack.                */
	/**********************************************************/
	nel_debug ({ parser_trace (eng, "abst_dec->pre_dec indrct_abst_dec\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int n;
		register nel_D_token D_token;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		for (; (n > 0); n--) {
			parser_pop_D_token (eng, D_token);
			switch (D_token) {
			case nel_D_ARRAY: {
					register unsigned_int dims;
					register int m;
					parser_pop_integer (eng, dims);

#ifdef NEL_PURE_ARRAYS

					/****************************************/
					/* for ANSI C array addressing, ignore  */
					/* all but the last array bound.  the   */
					/* other bounds are the left operand(s) */
					/* of the comma operator.               */
					/****************************************/
					m = 0;

#else

					/****************************************/
					/* the extended version of declarators  */
					/* includes multidimensional arrays     */
					/* in row major order, with optional    */
					/* nonzero lower bounds, for example,   */
					/*                                      */
					/*    int array[2,1:3];                 */
					/*                                      */
					/* is equivalent to                     */
					/*                                      */
					/*    int array[1:3][2];                */
					/*                                      */
					/* an array with base 1 and ub 3 of     */
					/* arrays with base 0 and ub 2 of ints. */
					/****************************************/
					for (m = dims - 1; (m >= 0); m--)

#endif /* NEL_PURE_ARRAYS */

					{
						register int ub_known;
						register int ub_val;
						register nel_expr *ub_expr;
						register int lb_known;
						register int lb_val;
						register nel_expr *lb_expr;
						parser_top_integer (eng, ub_known, m * 4);
						if (ub_known)
						{
							parser_top_integer (eng, ub_val, m * 4 + 1);
						} else
						{
							parser_top_expr (eng, ub_expr, m * 4 + 1);
						}
						parser_top_integer (eng, lb_known, m * 3 + 2);
						if (lb_known)
						{
							parser_top_integer (eng, lb_val, m * 4 + 3);
						} else
						{
							parser_top_expr (eng, lb_expr, m * 4 + 3);
						}
						if (lb_known && ub_known && (lb_val > ub_val))
						{
							parser_stmt_error (eng, "illegal dimension bound specification");
						}

						/*****************************************/
						/* note that we can't use the ? operator */
						/* here because the possible args for lb */
						/* or ub have different type.            */
						/*****************************************/
						if (lb_known)
						{
							if (ub_known) {
								type = nel_type_alloc (eng, nel_D_ARRAY, 0, type->simple.alignment, 1, 0, type, nel_int_type, 1, lb_val, 1, ub_val);
							} else {
								type = nel_type_alloc (eng, nel_D_ARRAY, 0, type->simple.alignment, 1, 0, type, nel_int_type, 1, lb_val, 0, ub_expr);
							}
						} else
						{
							if (ub_known) {
								type = nel_type_alloc (eng, nel_D_ARRAY, 0, type->simple.alignment, 1, 0, type, nel_int_type, 0, lb_expr, 1, ub_val);
							} else {
								type = nel_type_alloc (eng, nel_D_ARRAY, 0, type->simple.alignment, 1, 0, type, nel_int_type, 0, lb_expr, 0, ub_expr);
							}
						}
					}

#ifdef NEL_DEBUG
					/**************************************/
					/* while debugging, pop the stack 1   */
					/* element at at time, so that the    */
					/* ['s and ]'s match up in the trace. */
					/**************************************/
					for (m = 0; (m < dims); m++) {
						register int integer;
						register nel_expr *expr;
						parser_pop_integer (eng, integer);
						if (integer) {
							parser_pop_integer (eng, integer);
						} else {
							parser_pop_expr (eng, expr);
						}
						parser_pop_integer (eng, integer);
						if (integer) {
							parser_pop_integer (eng, integer);
						} else {
							parser_pop_expr (eng, expr);
						}
					}
#else
					/************************************/
					/* when not debugging pop the stack */
					/* by setting the stack pointer.    */
					/************************************/
					{
						nel_stack *new_top = eng->parser->semantic_stack_next - dims * 4;
						parser_set_stack (eng, new_top);
					}
#endif /* NEL_DEBUG */

				}
				break;
			case nel_D_FUNCTION: {
					register nel_list *args;
					register unsigned_int new_style;
					register unsigned_int var_args;
					parser_pop_integer (eng, new_style);
					parser_pop_integer (eng, var_args);
					parser_pop_list (eng, args);

					/************************************/
					/* check for an illegal return type */
					/************************************/
					switch (type->simple.type) {
					case nel_D_CHAR:
					case nel_D_COMPLEX:
					case nel_D_COMPLEX_DOUBLE:
					case nel_D_COMPLEX_FLOAT:
					case nel_D_DOUBLE:
					case nel_D_ENUM:
					case nel_D_FLOAT:
					case nel_D_INT:
					case nel_D_LONG:
					case nel_D_LONG_COMPLEX:
					case nel_D_LONG_COMPLEX_DOUBLE:
					case nel_D_LONG_COMPLEX_FLOAT:
					case nel_D_LONG_DOUBLE:
					case nel_D_LONG_FLOAT:
					case nel_D_LONG_INT:
					case nel_D_POINTER:
					case nel_D_SHORT:
					case nel_D_SHORT_INT:
					case nel_D_SIGNED:
					case nel_D_SIGNED_CHAR:
					case nel_D_SIGNED_INT:
					case nel_D_SIGNED_LONG:
					case nel_D_SIGNED_LONG_INT:
					case nel_D_SIGNED_SHORT:
					case nel_D_SIGNED_SHORT_INT:
					case nel_D_STRUCT:
					case nel_D_UNION:
					case nel_D_UNSIGNED:
					case nel_D_UNSIGNED_CHAR:
					case nel_D_UNSIGNED_INT:
					case nel_D_UNSIGNED_LONG:
					case nel_D_UNSIGNED_LONG_INT:
					case nel_D_UNSIGNED_SHORT:
					case nel_D_UNSIGNED_SHORT_INT:
					case nel_D_VOID:
						break;
					default:
						parser_stmt_error (eng, "illegal function return type");
					}
					type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, new_style, var_args, type, args, NULL, NULL);
				}
				break;
			case nel_D_POINTER: {
					register unsigned_int _const;
					register unsigned_int _volatile;
					parser_pop_integer (eng, _const);
					parser_pop_integer (eng, _volatile);
					type = nel_type_alloc (eng, nel_D_POINTER, sizeof (char *), nel_alignment_of (char *), _const, _volatile, type);
				}
				break;
			default:
				parser_fatal_error (eng, "(declarator->pre_dec indrct_abst_dec #1): bad nel_D_token: %D", D_token);
				break;
			}
		}
		parser_push_type (eng, type);
	}
};



/*****************************************************************************/
/* indrct_abst_dec->* productions pop a name (char *), which is              */
/* always NULL (see abst_dec->prec indrct_abst_dec for an explanation), a    */
/* type descriptor (nel_type *), and an integer (int) from the semantic       */
/* stack, then push an nel_D_token (nel_D_POINTER), the integer + 1, the type  */
/* descriptor, and then the NULL name back on the stack.                     */
/*****************************************************************************/

indrct_abst_dec:
nel_T_STAR indrct_abst_dec {
	nel_debug ({ parser_trace (eng, "indrct_abst_dec->nel_T_STAR indrct_abst_dec\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_integer (eng, 0);	/* volatile flag	*/
		parser_push_integer (eng, 0);	/* const flag		*/
		parser_push_D_token (eng, nel_D_POINTER);
		n++;
		parser_push_integer (eng, n);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
}|
nel_T_STAR {
	nel_debug ({ parser_trace (eng, "indrct_abst_dec->nel_T_STAR\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_integer (eng, 0);	/* volatile flag	*/
		parser_push_integer (eng, 0);	/* const flag		*/
		parser_push_D_token (eng, nel_D_POINTER);
		n++;
		parser_push_integer (eng, n);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
}|
nel_T_STAR nel_T_CONST extra_cnsts indrct_abst_dec {
	nel_debug ({ parser_trace (eng, "indrct_abst_dec->nel_T_STAR nel_T_CONST extra_cnsts indrct_abst_dec\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_integer (eng, 0);	/* volatile flag	*/
		parser_push_integer (eng, 1);	/* const flag		*/
		parser_push_D_token (eng, nel_D_POINTER);
		n++;
		parser_push_integer (eng, n);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
}|
nel_T_STAR nel_T_CONST extra_cnsts {
	nel_debug ({ parser_trace (eng, "indrct_abst_dec->nel_T_STAR nel_T_CONST extra_cnsts\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_integer (eng, 0);	/* volatile flag	*/
		parser_push_integer (eng, 1);	/* const flag		*/
		parser_push_D_token (eng, nel_D_POINTER);
		n++;
		parser_push_integer (eng, n);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
}|
nel_T_STAR nel_T_VOLATILE extra_vols indrct_abst_dec {
	nel_debug ({ parser_trace (eng, "indrct_abst_dec->nel_T_STAR nel_T_VOLATILE extra_vols indrct_abst_dec\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_integer (eng, 1);	/* volatile flag	*/
		parser_push_integer (eng, 0);	/* const flag		*/
		parser_push_D_token (eng, nel_D_POINTER);
		n++;
		parser_push_integer (eng, n);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
}|
nel_T_STAR nel_T_VOLATILE extra_vols {
	nel_debug ({ parser_trace (eng, "indrct_abst_dec->nel_T_STAR nel_T_VOLATILE extra_vols\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_integer (eng, 1);	/* volatile flag	*/
		parser_push_integer (eng, 0);	/* const flag		*/
		parser_push_D_token (eng, nel_D_POINTER);
		n++;
		parser_push_integer (eng, n);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
}|
nel_T_STAR nel_T_CONST extra_cnsts nel_T_VOLATILE extra_cnst_vols indrct_abst_dec {
	nel_debug ({ parser_trace (eng, "indrct_abst_dec->nel_T_STAR nel_T_CONST extra_cnsts nel_T_VOLATILE extra_cnst_vols indrct_abst_dec\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_integer (eng, 1);	/* volatile flag	*/
		parser_push_integer (eng, 1);	/* const flag		*/
		parser_push_D_token (eng, nel_D_POINTER);
		n++;
		parser_push_integer (eng, n);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
}|
nel_T_STAR nel_T_CONST extra_cnsts nel_T_VOLATILE extra_cnst_vols {
	nel_debug ({ parser_trace (eng, "indrct_abst_dec->nel_T_STAR nel_T_CONST extra_cnsts nel_T_VOLATILE extra_cnst_vols\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_integer (eng, 1);	/* volatile flag	*/
		parser_push_integer (eng, 1);	/* const flag		*/
		parser_push_D_token (eng, nel_D_POINTER);
		n++;
		parser_push_integer (eng, n);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
}|
nel_T_STAR nel_T_VOLATILE extra_vols nel_T_CONST extra_cnst_vols indrct_abst_dec {
	nel_debug ({ parser_trace (eng, "indrct_abst_dec->nel_T_STAR nel_T_VOLATILE extra_vols nel_T_CONST extra_cnst_vols indrct_abst_dec\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_integer (eng, 1);	/* volatile flag	*/
		parser_push_integer (eng, 1);	/* const flag		*/
		parser_push_D_token (eng, nel_D_POINTER);
		n++;
		parser_push_integer (eng, n);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
}|
nel_T_STAR nel_T_VOLATILE extra_vols nel_T_CONST extra_cnst_vols {
	nel_debug ({ parser_trace (eng, "indrct_abst_dec->nel_T_STAR nel_T_VOLATILE extra_vols nel_T_CONST extra_cnst_vols\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_integer (eng, 1);	/* volatile flag	*/
		parser_push_integer (eng, 1);	/* const flag		*/
		parser_push_D_token (eng, nel_D_POINTER);
		n++;
		parser_push_integer (eng, n);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
}|
direct_abst_dec {
	nel_debug ({ parser_trace (eng, "indrct_abst_dec->direct_abst_dec\n\n"); });
};



/*****************************************************************************/
/* indrct_abst_dec->* productions pop a name (char *), which is              */
/* always NULL (see abst_dec->pre_dec indrct_abst_dec for an explanation),   */
/* a type descriptor (nel_type *), and an integer (int) from the semantic     */
/* stack, then push an nel_D_token (and possibly other items which can be     */
/* determined from the nel_D_token), the integer + 1, the type descriptor,    */
/* and then the NULL name back on the stack.                                 */
/*****************************************************************************/

direct_abst_dec:
nel_T_LPAREN indrct_abst_dec nel_T_RPAREN {
	nel_debug ({ parser_trace (eng, "direct_abst_dec->nel_T_LPAREN indrct_abst_dec nel_T_RPAREN\n\n"); });
}|
direct_abst_dec nel_T_LBRACK dim_bd_list nel_T_RBRACK {
	nel_debug ({ parser_trace (eng, "direct_abst_dec->direct_abst_dec nel_T_LBRACK dim_bd_list nel_T_RBRACK\n\n"); });
	/*********************************************************/
	/* pop the number of declarators seed so far, and leave  */
	/* everything else set up as dim_bd_list->* productions  */
	/* have formed it.  push the nel_D_token nel_D_ARRAY, then */
	/* the number of declarators + 1.                        */
	/*********************************************************/
	nel_debug ({ parser_trace (eng, "direct_abst_dec->direct_abst_dec nel_T_LBRACK dim_bd_list nel_T_RBRACK\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_D_token (eng, nel_D_ARRAY);
		n++;
		parser_push_integer (eng, n);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
}|
nel_T_LBRACK dim_bd_list nel_T_RBRACK {
	/*********************************************************/
	/* pop the number of declarators seed so far, and leave  */
	/* everything else set up as dim_bd_list->* productions  */
	/* have formed it.  push the nel_D_token nel_D_ARRAY, then */
	/* the number of declarators + 1.                        */
	/*********************************************************/
	nel_debug ({ parser_trace (eng, "direct_abst_dec->nel_T_LBRACK dim_bd_list nel_T_RBRACK\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_D_token (eng, nel_D_ARRAY);
		n++;
		parser_push_integer (eng, n);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
}|
direct_abst_dec nel_T_LBRACK nel_T_RBRACK {
	nel_debug ({ parser_trace (eng, "direct_abst_dec->direct_abst_dec nel_T_LBRACK nel_T_RBRACK\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_integer (eng, 0);	/* lb		*/
		parser_push_integer (eng, 1);	/* lb_known	*/
		parser_push_expr (eng, NULL);	/* ub		*/
		parser_push_integer (eng, 0);	/* ub not known	*/
		parser_push_integer (eng, 1);	/* single dim	*/
		parser_push_D_token (eng, nel_D_ARRAY);
		n++;
		parser_push_integer (eng, n);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
}|
nel_T_LBRACK nel_T_RBRACK {
	nel_debug ({ parser_trace (eng, "direct_abst_dec->nel_T_LBRACK nel_T_RBRACK\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_integer (eng, 0);	/* lb		*/
		parser_push_integer (eng, 1);	/* lb_known	*/
		parser_push_expr (eng, NULL);	/* ub		*/
		parser_push_integer (eng, 0);	/* ub not known	*/
		parser_push_integer (eng, 1);	/* single dim	*/
		parser_push_D_token (eng, nel_D_ARRAY);
		n++;
		parser_push_integer (eng, n);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
}|
direct_abst_dec nel_T_LPAREN param_type_list nel_T_RPAREN {
	nel_debug ({ parser_trace (eng, "direct_abst_dec->direct_abst_dec nel_T_LPAREN param_type_list nel_T_RPAREN\n\n"); });
	{
		register unsigned_int var_args;
		register nel_list *args;
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_integer (eng, var_args);
		parser_pop_list (eng, args);
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_list (eng, args);
		parser_push_integer (eng, var_args);
		parser_push_integer (eng, 1);  /* is new sytle dec */
		parser_push_D_token (eng, nel_D_FUNCTION);
		parser_push_integer (eng, n + 1);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
}|
nel_T_LPAREN param_type_list nel_T_RPAREN {
	nel_debug ({ parser_trace (eng, "direct_abst_dec->nel_T_LPAREN param_type_list nel_T_RPAREN\n\n"); });
	{
		register unsigned_int var_args;
		register nel_list *args;
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_integer (eng, var_args);
		parser_pop_list (eng, args);
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_list (eng, args);
		parser_push_integer (eng, var_args);
		parser_push_integer (eng, 1);  /* is new sytle dec */
		parser_push_D_token (eng, nel_D_FUNCTION);
		parser_push_integer (eng, n + 1);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
}|
direct_abst_dec nel_T_LPAREN nel_T_RPAREN {
	nel_debug ({ parser_trace (eng, "direct_abst_dec->direct_abst_dec nel_T_LPAREN nel_T_RPAREN\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_list (eng, NULL);
		parser_push_integer (eng, 0);  /* is not var # of args */
		parser_push_integer (eng, 0);  /* is not old sytle dec */
		parser_push_D_token (eng, nel_D_FUNCTION);
		parser_push_integer (eng, n + 1);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
}|
nel_T_LPAREN nel_T_RPAREN {
	nel_debug ({ parser_trace (eng, "direct_abst_dec->nel_T_LPAREN nel_T_RPAREN\n\n"); });
	{
		register char *name;
		register nel_type *type;
		register int n;
		parser_pop_name (eng, name);
		parser_pop_type (eng, type);
		parser_pop_integer (eng, n);
		parser_push_list (eng, NULL);
		parser_push_integer (eng, 0);  /* is not var # of args */
		parser_push_integer (eng, 0);  /* is not old sytle dec */
		parser_push_D_token (eng, nel_D_FUNCTION);
		parser_push_integer (eng, n + 1);
		parser_push_type (eng, type);
		parser_push_name (eng, name);
	}
};






/**/
/*****************************************************************************/
/* productions for parsing non-declaration statements.                       */
/*****************************************************************************/



stmt_list:
stmt_list stmt {
	nel_debug ({ parser_trace (eng, "stmt_list->stmt_list stmt\n\n"); });
}|
empty {
	nel_debug ({ parser_trace (eng, "stmt_list->empty\n\n"); });
};



stmt:
empty {
	nel_debug ({ parser_trace (eng, "stmt->empty {} stmt2\n\n"); });
	{
		parser_yy_setjmp (eng, eng->parser->tmp_int, eng->parser->stmt_err_jmp);
		if (eng->parser->tmp_int != 0) {
			/******************************************/
			/* there was an error - we jumped to here */
			/* a 2 is returned from setjmp if we      */
			/* should omit the "scanning for..." msg, */
			/* else a 1 is returned when longjmping.  */
			/* yacc may have cleared yychar, so use   */
			/* eng->parser->last_char for the value of   */
			/* the previous input token.              */
			/******************************************/
			if (eng->parser->tmp_int == 1) {
				parser_diagnostic (eng, "scanning for ';' or unmatched '}'");
			}
			nel_debug ({
						   parser_trace (eng, "stmt error longjmp caught - scanning for ';' or unmatched '}' last_char = %N\n\n", eng->parser->last_char);
					   });
			eng->parser->tmp_int = 0; /* scope nesting level */
			do {
				if (eng->parser->last_char == nel_T_LBRACE) {
					eng->parser->tmp_int++;
				} else if (eng->parser->last_char == nel_T_RBRACE) {
					eng->parser->tmp_int--;
					if (eng->parser->tmp_int <= 0) {
						/***********************************/
						/* set yychar = }, then longjmp on */
						/* the block_err_jmp, since we are */
						/* at the end of the block.        */
						/* returning a 2 instead of 1      */
						/* suppresses the "scanning for.." */
						/* msg following a block error.    */
						/***********************************/
						yychar = nel_T_RBRACE;
						nel_debug ({
									   if (eng->parser->block_err_jmp == NULL) {
									   parser_fatal_error (eng, "(stmt->empty {} stmt2 #1): block_error_jmp not set")
										   ;
									   }
								   });
						nel_longjmp (eng, *(eng->parser->block_err_jmp), 2);
						break;
					}
				} else if ((eng->parser->last_char == nel_T_SEMI) && (eng->parser->tmp_int <= 0)) {
					/*********************************************/
					/* leave the semicolon in the input stream   */
					/* so that the parser will later complete    */
					/* the derivation:                           */
					/*                                           */
					/* empty                s                    */
					/* empty ;              r  stmt2-> ;         */
					/* empty stmt2          r  stmt->empty stmt2 */
					/* stmt                                      */
					/*********************************************/
					/**********************************/
					/* set yychar = ;, in case the    */
					/* parser has already cleared it. */
					/**********************************/
					yychar = nel_T_SEMI;
					parser_diagnostic (eng, "';' found - parser reset");
					break;
				}
				yychar = nel_yylex (eng);
			} while (eng->parser->last_char > 0);

			/****************************************************/
			/* if we are at EOF, return.  if we try to continue */
			/* parsing and get another syntax error, we longjmp */
			/* back here, and are in an infinite loop.          */
			/****************************************************/
			if (yychar == 0) {
				parser_diagnostic (eng, "EOF encountered");
				nel_longjmp (eng, *(eng->parser->return_pt), 1);
			}
		}

		/***********************************************/
		/* push the dynamic value and symbol allocator */
		/* indeces, so that we may restore then after  */
		/* parsing each translataion unit - there may  */
		/* be some garbage produced in evaluating      */
		/* dimension bound expressions (int static     */
		/* locals), bit field sizes, etc.              */
		/***********************************************/
		parser_push_value (eng, eng->parser->dyn_values_next, char *);
		parser_push_value (eng, eng->parser->dyn_symbols_next, nel_symbol *);
	}
} stmt2 {
	nel_debug ({ parser_trace (eng, "stmt->empty stmt2\n\n"); });
	{
		/***********************************************/
		/* pop the value and symbol allocator indeces, */
		/* reclaiming (most) garbage producede in      */
		/* parsing this stmt.  anything which needs to */
		/* be saved is statically allocated.           */
		/***********************************************/
		parser_pop_value (eng, eng->parser->dyn_symbols_next, nel_symbol *);
		parser_pop_value (eng, eng->parser->dyn_values_next, char *);
	}
};



stmt2:
block {
	nel_debug ({ parser_trace (eng, "stmt2->block\n\n"); });
}|
break_stmt {
	nel_debug ({ parser_trace (eng, "stmt2->break_stmt\n\n"); });
}|
continue_stmt {
	nel_debug ({ parser_trace (eng, "stmt2->continue_stmt\n\n"); });
}|
dec_stmt {
	nel_debug ({ parser_trace (eng, "stmt2->dec_stmt\n\n"); });
}|
do_while_stmt {
	nel_debug ({ parser_trace (eng, "stmt2->do_while_stmt\n\n"); });
}|
expression_stmt {
	nel_debug ({ parser_trace (eng, "stmt2->expression_stmt\n\n"); });
}|
for_stmt {
	nel_debug ({ parser_trace (eng, "stmt2->for_stmt\n\n"); });
}|
/*
goto_stmt {
	nel_debug ({ parser_trace (eng, "stmt2->goto_stmt\n\n"); });
}|
*/	
if_stmt {
	nel_debug ({ parser_trace (eng, "stmt2->if_stmt\n\n"); });
}|
/*
label_stmt {
	nel_debug ({ parser_trace (eng, "stmt2->label_stmt\n\n"); });
}|
*/	
return_stmt {
		  nel_debug ({ parser_trace (eng, "stmt2->return_stmt\n\n"); });
	  }|
	  while_stmt {
		  nel_debug ({ parser_trace (eng, "stmt2->while_stmt\n\n"); });
	  }|
	  nel_T_SEMI {
		  nel_debug ({ parser_trace (eng, "stmt2->nel_T_SEMI\n\n"); });
	  };



/**/



/*****************************************************************************/
/* we put target stmts at the begining and end of each block to indicate the */
/* scoping level.                                                            */
/*****************************************************************************/
block:
nel_T_LBRACE {
	nel_debug ({ parser_trace (eng, "block->nel_T_LBRACE {} stmt_list nel_T_RBRACE\n\n"); });
#if 0

	if (eng->parser->declare_mode)
	{
		parser_block_error (eng, "In block: executable code not allowed in declare mode");
	} else
#endif

	{
		/********************************************/
		/* increment the scoping level and block #, */
		/* and save the current context.            */
		/********************************************/
		eng->parser->level++;
		parser_push_integer (eng, eng->parser->block_no);
		eng->parser->block_no = eng->parser->block_ct++;
		save_parser_context (eng);

		/***********************************************/
		/* create a target node with the new level #   */
		/* and append it to the stmt list.  do not use */
		/* the append_stmt macro, as we do not want a  */
		/* "statement not reached" warning emitted.    */
		/***********************************************/
		{
			register nel_stmt *target;
			target = nel_stmt_alloc (eng, nel_S_TARGET, eng->parser->filename,
									 eng->parser->line, eng->parser->level, NULL);
			if (eng->parser->append_pt != NULL)
			{
				*(eng->parser->append_pt) = target;
			}
			eng->parser->append_pt = &(target->target.next);
			eng->parser->last_stmt = target;
		}

		/******************************************/
		/* now perform a setjmp for block errors. */
		/******************************************/
		parser_yy_setjmp (eng, eng->parser->tmp_int, eng->parser->block_err_jmp);
		if (eng->parser->tmp_int != 0) {
			/******************************************/
			/* there was an error - we jumped to here */
			/* a 2 is returned from setjmp if we      */
			/* should omit the "scanning for..." msg, */
			/* else a 1 is returned when longjmping.  */
			/* yacc may have cleared yychar, so use   */
			/* eng->parser->last_char for the value of   */
			/* the previous input token.              */
			/******************************************/
			if (eng->parser->tmp_int == 1) {
				parser_diagnostic (eng, "scanning for unmatched '}'");
			}
			nel_debug ({
						   parser_trace (eng, "block error longjmp caught - scanning for unmatched '}' last_char = %N\n\n", eng->parser->last_char);
					   });
			eng->parser->tmp_int = 0;
			do {
				if (eng->parser->last_char == nel_T_LBRACE) {
					eng->parser->tmp_int++;
				} else if (eng->parser->last_char == nel_T_RBRACE) {
					eng->parser->tmp_int--;
					/*********************************************/
					/* the left brace the block has already been */
					/* absorbed.  find an unmatched right brace. */
					/*********************************************/
					if (eng->parser->tmp_int < 0) {
						/******************************************/
						/* do not clear the right brace so that   */
						/* the parser will later complete the     */
						/* derivation:                            */
						/*                                        */
						/* e {              s                     */
						/* e { e            r stmt_list->e        */
						/* e { stmt_list    s                     */
						/* e { stmt_list }  r block->e {stmt_list}*/
						/* block                                  */
						/******************************************/
						/**********************************/
						/* set yychar = }, in case the    */
						/* parser has already cleared it. */
						/**********************************/
						yychar = nel_T_RBRACE;
						parser_diagnostic (eng, "unmatched '}' found - parser reset");
						break;
					}
				}
				yychar = nel_yylex (eng);
			} while (eng->parser->last_char > 0);

			/****************************************************/
			/* if we are at EOF, return.  if we try to continue */
			/* parsing and get another syntax error, we longjmp */
			/* back here, and are in an infinite loop.          */
			/****************************************************/
			if (yychar == 0) {
				parser_diagnostic (eng, "EOF encountered");
				nel_longjmp (eng, *(eng->parser->return_pt), 1);
			}
		}

		/*************************************************/
		/* now perform a setjmp for stmt errors, in case */
		/* one is encountered (by the lexer) before the  */
		/* intermediate code in the production           */
		/*                                               */
		/*    stmt->empty {} stmt2                       */
		/*                                               */
		/* is executed, and the stmt error jump set.     */
		/*************************************************/
		parser_yy_setjmp (eng, eng->parser->tmp_int, eng->parser->stmt_err_jmp);
		if (eng->parser->tmp_int != 0) {
			/******************************************/
			/* there was an error - we jumped to here */
			/* a 2 is returned from setjmp if we      */
			/* should omit the "scanning for..." msg, */
			/* else a 1 is returned when longjmping.  */
			/* yacc may have cleared yychar, so use   */
			/* eng->parser->last_char for the value of   */
			/* the previous input token.              */
			/******************************************/
			if (eng->parser->tmp_int == 1) {
				parser_diagnostic (eng, "scanning for ';' or unmatched '}'");
			}
			nel_debug ({
						   parser_trace (eng, "stmt error longjmp caught - scanning for ';' or unmatched '}' last_char = %N\n\n", eng->parser->last_char);
					   });
			eng->parser->tmp_int = 0; /* scope nesting level */
			do {
				if (eng->parser->last_char == nel_T_LBRACE) {
					eng->parser->tmp_int++;
				} else if (eng->parser->last_char == nel_T_RBRACE) {
					eng->parser->tmp_int--;
					if (eng->parser->tmp_int <= 0) {
						/***********************************/
						/* set yychar = }, in case the     */
						/* parser has already cleared it.  */
						/* no need to longjmp to the block */
						/* error jump, as it was just set  */
						/* previously in this production.  */
						/***********************************/
						yychar = nel_T_RBRACE;
						parser_diagnostic (eng, "'}' found - parser reset");
						break;
					}
				} else if ((eng->parser->last_char == nel_T_SEMI) && (eng->parser->tmp_int <= 0)) {
					/**********************************/
					/* set yychar = ;, in case the    */
					/* parser has already cleared it. */
					/**********************************/
					yychar = nel_T_SEMI;
					parser_diagnostic (eng, "';' found - parser reset");
					break;
				}
				yychar = eng->parser->last_char = nel_yylex (eng);
			} while (eng->parser->last_char > 0);

			/****************************************************/
			/* if we are at EOF, return.  if we try to continue */
			/* parsing and get another syntax error, we longjmp */
			/* back here, and are in an infinite loop.          */
			/****************************************************/
			if (yychar == 0) {
				parser_diagnostic (eng, "EOF encountered");
				nel_longjmp (eng, *(eng->parser->return_pt), 1);
			}
		}

		/*********************************************/
		/* if the flag nel_save_stmt_err_jmp is true, */
		/* then set nel_stmt_err_jmp to point to the  */
		/* newly-set jump buffer.                    */
		/*********************************************/
		if (nel_save_stmt_err_jmp) {
			nel_stmt_err_jmp = eng->parser->stmt_err_jmp;
		}
	}
} stmt_list nel_T_RBRACE {
	nel_debug ({ parser_trace (eng, "block->nel_T_LBRACE stmt_list nel_T_RBRACE\n\n"); });
	{

#if 0
		nel_debug ({
					   parser_trace (eng, "dynamic ident hash:\n");
					   nel_list_symbols (nel_tf, eng->parser->dyn_ident_hash);
					   parser_trace (eng, "\ndynamic tag hash:\n");
					   nel_list_symbols (nel_tf, eng->parser->dyn_tag_hash);
					   parser_trace (eng, "\ndynamic label hash:\n");
					   nel_list_symbols (nel_tf, eng->parser->dyn_label_hash);
					   parser_trace (eng, "\n");
				   });
#endif /* 0 */

		/****************************************/
		/* decrement the level, and restore the */
		/* previous context and block number.   */
		/****************************************/
		eng->parser->level--;
		restore_parser_context (eng, eng->parser->level);
		parser_pop_integer (eng, eng->parser->block_no);

		/****************************************************/
		/* purge the label hash only if we are exiting the  */
		/* routine (returning to  level 0), since the scope */
		/* of labels is the entire routine.                 */
		/****************************************************/
		if (eng->parser->level == 0) {
			nel_purge_table (eng, 0, eng->parser->dyn_label_hash);
		}

#if 0
		nel_debug ({
					   parser_trace (eng, "dynamic ident hash:\n");
					   nel_list_symbols (nel_tf, eng->parser->dyn_ident_hash);
					   parser_trace (eng, "\ndynamic tag hash:\n");
					   nel_list_symbols (nel_tf, eng->parser->dyn_tag_hash);
					   parser_trace (eng, "\ndynamic label hash:\n");
					   nel_list_symbols (nel_tf, eng->parser->dyn_label_hash);
					   parser_trace (eng, "\n");
				   });
#endif /* 0 */

		/***********************************************/
		/* create a target node with the new level #   */
		/* and append it to the stmt list.  do not use */
		/* the append_stmt macro, as we do not want a  */
		/* "statement not reached" warning emitted.    */
		/***********************************************/
		{
			register nel_stmt *target;
			target = nel_stmt_alloc (eng, nel_S_TARGET, eng->parser->filename,
									 eng->parser->line, eng->parser->level, NULL);
			if (eng->parser->append_pt != NULL)
			{
				*(eng->parser->append_pt) = target;
			}
			eng->parser->append_pt = &(target->target.next);
			eng->parser->last_stmt = target; 
		}
	}
}
;






/**/
/*****************************************************************************/
/* productions for parsing the individual statement types.                   */
/* all looping constructs should set eng->parser->continue_target to a target   */
/* statement just before the trip condition, and end_target to a target      */
/* statement just after the loop, so that break and continue statments work  */
/* properly.  these targets must be created before the actual body of the    */
/* loop is parsed.                                                           */
/*****************************************************************************/



break_stmt:
nel_T_BREAK nel_T_SEMI {
	nel_debug ({ parser_trace (eng, "break_stmt->nel_T_BREAK nel_T_SEMI\n\n"); });
	/***************************************************/
	/* append the current break stmt to the stmt list. */
	/* its next field points to the end of the loop.   */
	/* the next statement might be unreachable.        */
	/***************************************************/
	{
		if (eng->parser->break_target == NULL)
		{
			parser_error (eng, "break not within loop");
		} else
		{
			nel_stmt *break_stmt;
			break_stmt = nel_stmt_alloc(eng, nel_S_BREAK, eng->parser->filename, eng->parser->line, eng->parser->break_target);
			if(eng->parser->continue_target->gen.type == nel_S_TARGET)
				break_stmt->goto_stmt.type = nel_S_GOTO;		
			append_stmt (break_stmt, &(break_stmt->goto_stmt.next));
		}
	}
};



continue_stmt:
nel_T_CONTINUE nel_T_SEMI {
	nel_debug ({ parser_trace (eng, "continue_stmt->nel_T_CONTINUE nel_T_SEMI\n\n"); });
	/******************************************************/
	/* append the current continue stmt to the stmt list. */
	/* its next field points to the start of the loop (or */
	/* before the increment of a for loop).               */
	/* the next statement might be unreachable.           */
	/******************************************************/
	{
		if (eng->parser->continue_target == NULL)
		{
			parser_error (eng, "continue not within loop");
		} else
		{
			nel_stmt *goto_stmt;
			goto_stmt = nel_stmt_alloc(eng, nel_S_CONTINUE, eng->parser->filename, eng->parser->line, eng->parser->continue_target);
			if(eng->parser->continue_target->gen.type == nel_S_TARGET)
				goto_stmt->goto_stmt.type = nel_S_GOTO;
			append_stmt (goto_stmt, &(goto_stmt->goto_stmt.next));
		}
	}
};



do_while_stmt:
nel_T_DO {
	nel_debug ({ parser_trace (eng, "if_stmt->nel_T_DO {} stmt nel_T_WHILE nel_T_LPAREN expression nel_T_RPAREN nel_T_SEMI\n\n"); });
	/**********************************************************/
	/* form the following stmt list:                          */
	/*                                                        */
	/*          |                                             */
	/*          V                                             */
	/*   +--------------+                                     */
	/*   | begin_target |<-+                                  */
	/*   +--------------+  |                                  */
	/*          |          |                                  */
	/*          V          |                                  */
	/*   +--------------+  |                                  */
	/*   |  loop body   |  |                                  */
	/*   +--------------+  |                                  */
	/*          |          |                                  */
	/*          V          |                                  */
	/*   +--------------+  |                                  */
	/*   | cond_target  |  |                                  */
	/*   +--------------+  |                                  */
	/*          |          |                                  */
	/*          V          |                                  */
	/*   +--------------+  |                                  */
	/*   | branch: cond |--+ t                                */
	/*   +--------------+                                     */
	/*          | f                                           */
	/*          V                                             */
	/*   +--------------+                                     */
	/*   |  end_target  |                                     */
	/*   +--------------+                                     */
	/*          |                                             */
	/*          V                                             */
	/*                                                        */
	/* cond_target is the continue target                     */
	/* end_target is the break target                         */
	/**********************************************************/
	{
		register nel_stmt *begin_target;
		register nel_stmt *cond_target;
		register nel_stmt *end_target;
		begin_target = nel_stmt_alloc (eng, nel_S_TARGET, eng->parser->filename,
									   eng->parser->line, eng->parser->level, NULL);
		cond_target = nel_stmt_alloc (eng, nel_S_TARGET, eng->parser->filename,
									  eng->parser->line,eng->parser->level, NULL);
		end_target = nel_stmt_alloc (eng, nel_S_TARGET, eng->parser->filename,
									 eng->parser->line, eng->parser->level, NULL);
		append_stmt (begin_target, &(begin_target->target.next));
		parser_push_stmt (eng, eng->parser->break_target);
		parser_push_stmt (eng, eng->parser->continue_target);
		eng->parser->break_target = end_target;
		eng->parser->continue_target = cond_target;
		parser_push_stmt (eng, end_target);
		parser_push_stmt (eng, cond_target);
		parser_push_stmt (eng, begin_target);
	}
} block nel_T_WHILE nel_T_LPAREN expression nel_T_RPAREN nel_T_SEMI {
	nel_debug ({ parser_trace (eng, "if_stmt->nel_T_DO stmt nel_T_WHILE nel_T_LPAREN expression nel_T_RPAREN nel_T_SEMI\n\n"); });
	{
		register nel_expr *cond;
		register nel_stmt *begin_target;
		register nel_stmt *cond_target;
		register nel_stmt *end_target;
		register nel_stmt *if_stmt;
		parser_pop_expr (eng, cond);
		nel_type* expr_type;
		expr_type = eval_expr_type(eng, cond);
		if(!expr_type)
			parser_stmt_error(eng, "cannot evaluate condition expression type");
		if(!nel_numerical_D_token(expr_type->simple.type) && expr_type->simple.type != nel_D_POINTER)
			parser_stmt_error(eng, "scalar expression needed as condition");
		parser_pop_stmt (eng, begin_target);
		parser_pop_stmt (eng, cond_target);
		parser_pop_stmt (eng, end_target);
		if (eng->parser->append_pt != NULL) {
			*(eng->parser->append_pt) = cond_target;
		}
		nel_stmt* jmp_stmt;
		jmp_stmt = nel_stmt_alloc(eng, nel_S_GOTO, eng->parser->filename, eng->parser->line, begin_target);
		if_stmt = nel_stmt_alloc (eng, nel_S_BRANCH, eng->parser->filename, eng->parser->line, eng->parser->level, cond, jmp_stmt, end_target);
		end_target->target.line = eng->parser->line;
		cond_target->target.next = if_stmt;
		eng->parser->append_pt = &(end_target->target.next);

		eng->parser->last_stmt = end_target;

		parser_pop_stmt (eng, eng->parser->continue_target);
		parser_pop_stmt (eng, eng->parser->break_target);
	}
};



expression_stmt:
expression nel_T_SEMI {
	nel_debug ({ parser_trace (eng, "expression_stmt->expression nel_T_SEMI\n\n"); });
	/***************************************************/
	/* pop the symbol for the value of the expression, */
	/* and append it to the current list.              */
	/***************************************************/
	{
		register nel_expr *expr;
		register nel_stmt *stmt;
		parser_pop_expr (eng, expr);
		if(nel_O_asgn_expr(expr->gen.opcode) && expr->binary.left->gen.opcode==nel_O_SYMBOL && expr->binary.left->symbol.symbol->class == nel_C_CONST) {
			parser_stmt_error(eng, "illegal lhs of assignment operator");
		}
		if(expr->gen.opcode==nel_O_PRE_DEC || expr->gen.opcode==nel_O_PRE_INC ||
			expr->gen.opcode==nel_O_POST_DEC || expr->gen.opcode==nel_O_POST_INC) {
			if(expr->unary.operand->gen.opcode==nel_O_SYMBOL && expr->unary.operand->symbol.symbol->class==nel_C_CONST) {
				parser_stmt_error(eng, "illegal lhs of ++ or -- operator");
			}
		}
		
		if(!eval_expr_type(eng, expr)) {
			parser_stmt_error(eng, "illegal expression");
		}


		stmt = nel_stmt_alloc (eng, nel_S_EXPR, eng->parser->filename,
							   eng->parser->line, expr, NULL);
		append_stmt (stmt, &(stmt->expr.next));
	}
};


for_stmt:
nel_T_FOR nel_T_LPAREN opt_expr nel_T_SEMI opt_expr nel_T_SEMI opt_expr nel_T_RPAREN {
	nel_debug ({ parser_trace (eng, "for_stmt->nel_T_FOR nel_T_LPAREN opt_expr nel_T_SEMI opt_expr nel_T_SEMI opt_expr nel_T_RPAREN {} stmt\n\n"); });
	/**********************************************************/
	/*    form the following stmt list:                       */
	/*                                                        */
	/*             |                                          */
	/*             V                                          */
	/*      +--------------+                                  */
	/*      |  expr: init  |      (optional)                  */
	/*      +--------------+                                  */
	/*             |                                          */
	/*             V                                          */
	/*      +--------------+                                  */
	/*   +->| cond_target  |                                  */
	/*   |  +--------------+                                  */
	/*   |         |                                          */
	/*   |         V                                          */
	/*   |  +--------------+ f                                */
	/*   |  | branch: cond |--+   (optional - branch points   */
	/*   |  +--------------+  |   to loop body if deleted)    */
	/*   |         | t        |                               */
	/*   |         V          |                               */
	/*   |  +--------------+  |                               */
	/*   |  |  loop body   |  |                               */
	/*   |  +--------------+  |                               */
	/*   |         |          |                               */
	/*   |         V          |                               */
	/*   |  +--------------+  |                               */
	/*   |  |  inc_target  |  |                               */
	/*   |  +--------------+  |                               */
	/*   |         |          |                               */
	/*   |         V          |                               */
	/*   |  +--------------+  |                               */
	/*   |  |  expr: inc   |  |   (optional)                  */
	/*   |  +--------------+  |                               */
	/*   |         |          |                               */
	/*   +---------+          |                               */
	/*                        |                               */
	/*      +--------------+  |                               */
	/*      |  end_target  |<-+                               */
	/*      +--------------+                                  */
	/*             |                                          */
	/*             V                                          */
	/*                                                        */
	/* cond_target is the continue target                     */
	/* end_target is the break target                         */
	/**********************************************************/
	{
		register nel_expr *init;
		register nel_expr *cond;
		register nel_expr *inc;
		parser_pop_expr (eng, inc);
		parser_pop_expr (eng, cond);
		parser_pop_expr (eng, init);
#if 0
		nel_stmt *init_stmt = NULL;
		nel_stmt *cond_target = NULL;
		nel_stmt *if_stmt = NULL;
		nel_stmt *end_target = NULL;
		nel_stmt *inc_target = NULL;
		nel_stmt *inc_stmt = NULL;
		nel_expr *cond_expr = cond;
		if(init)
		{
			init_stmt = nel_stmt_alloc(eng, nel_S_EXPR, "init_stmt", eng->parser->line, init, NULL);
			append_stmt(init_stmt, &(init_stmt->expr.next));
		}
		cond_target = nel_stmt_alloc(eng, nel_S_TARGET, "cond_target", eng->parser->line, eng->parser->level, NULL);
		append_stmt(cond_target, &(cond_target->target.next));
		if(!cond) {
			cond_expr = nel_expr_alloc(eng, nel_O_SYMBOL, NULL);
			cond_expr->symbol.symbol = nel_static_symbol_alloc(eng, NULL, nel_signed_int_type, NULL, nel_C_CONST, 0, nel_L_NEL, eng->parser->level);
			cond_expr->symbol.symbol->value = nel_static_value_alloc(eng, sizeof(signed int), nel_alignment_of(signed int));
			*((signed int*)(cond_expr->symbol.symbol->value)) = 1;
			
		}
		else {
			nel_type* expr_type;
			expr_type = eval_expr_type(eng, cond);
			if(!expr_type)
				parser_stmt_error(eng, "cannot evaluate condition expression type");
			if(!nel_numerical_D_token(expr_type->simple.type) && expr_type->simple.type != nel_D_POINTER)
				parser_stmt_error(eng, "scalar expression needed as condition");
		}

		end_target = nel_stmt_alloc(eng, nel_S_TARGET, "end_target", eng->parser->line, eng->parser->level, NULL);
		if_stmt = nel_stmt_alloc(eng, nel_S_BRANCH, "if_stmt", eng->parser->level, eng->parser->level, cond_expr, NULL, end_target);
		append_stmt(if_stmt, &(if_stmt->branch.true_branch));
		if(inc)
		{
			inc_stmt = nel_stmt_alloc(eng, nel_S_EXPR, "inc_stmt", eng->parser->line, inc, NULL);
			inc_target = nel_stmt_alloc(eng, nel_S_TARGET, "inc_target", eng->parser->line, eng->parser->level, inc_stmt);
		}
		//save break/continue jmp;
		parser_push_stmt(eng, eng->parser->continue_target);
		parser_push_stmt(eng, eng->parser->break_target);
		eng->parser->break_target = end_target;
		eng->parser->continue_target = cond_target;

		//save other useful stmt
		parser_push_stmt(eng, cond_target);
		parser_push_stmt(eng, if_stmt);
		parser_push_stmt(eng, inc_target);
		parser_push_stmt(eng, inc_stmt);
		parser_push_stmt(eng, end_target);
#else
		
		register nel_stmt *for_stmt;
		if(cond) {
			nel_type *cond_type;
			cond_type = eval_expr_type(eng, cond);
			if(!cond_type)
				parser_stmt_error(eng, "cannot evaluate condition expression type");
			if(!nel_numerical_D_token(cond_type->simple.type) && cond_type->simple.type != nel_D_POINTER)
				parser_stmt_error(eng, "scalar expression needed as condition");
		}
		for_stmt = nel_stmt_alloc (eng, nel_S_FOR, eng->parser->filename,eng->parser->line, eng->parser->level, init, cond, inc, NULL);
		append_stmt (for_stmt, &(for_stmt->for_stmt.body));
		parser_push_stmt(eng, eng->parser->break_target);
		parser_push_stmt(eng, eng->parser->continue_target);
		eng->parser->break_target=for_stmt;
		eng->parser->continue_target = for_stmt;
		parser_push_stmt (eng, for_stmt);
#endif
	}
} block {
	nel_debug ({ parser_trace (eng, "for_stmt->nel_T_FOR nel_T_LPAREN opt_expr nel_T_SEMI opt_expr nel_T_SEMI opt_expr nel_T_RPAREN stmt\n\n"); });
	{
#if 0
		nel_stmt *cond_target = NULL;
		nel_stmt *if_stmt = NULL;
		nel_stmt *inc_target = NULL;
		nel_stmt *inc_stmt = NULL;
		nel_stmt *end_target = NULL;
		nel_stmt *jmp_stmt = NULL;

		parser_pop_stmt(eng, end_target);
		parser_pop_stmt(eng, inc_stmt);
		parser_pop_stmt(eng, inc_target);
		parser_pop_stmt(eng, if_stmt);
		parser_pop_stmt(eng, cond_target);

		end_target->target.line = eng->parser->line;
		if(inc_target)
			append_stmt(inc_target, &(inc_stmt->expr.next));
		jmp_stmt = nel_stmt_alloc(eng, nel_S_GOTO, eng->parser->filename, eng->parser->line, cond_target);
		append_stmt(jmp_stmt, &(if_stmt->branch.next));
		parser_pop_stmt(eng, eng->parser->break_target);
		parser_pop_stmt(eng, eng->parser->continue_target);
	
#else	
		register nel_stmt *for_stmt;
		parser_pop_stmt(eng, for_stmt);
		parser_pop_stmt(eng, eng->parser->continue_target);
		parser_pop_stmt(eng, eng->parser->break_target);
		eng->parser->append_pt = &(for_stmt->for_stmt.next);
		eng->parser->last_stmt = for_stmt;
#endif
	}
};



goto_stmt:
nel_T_GOTO label {
	/****************************************************/
	/* append the goto target to the current stmt list. */
	/* the next statement might be unreachable.         */
	/****************************************************/
	nel_debug ({ parser_trace (eng, "goto_stmt->nel_T_GOTO label\n\n"); });
	{
		register nel_symbol *symbol;
		register nel_stmt *stmt;
		parser_pop_symbol (eng, symbol);
		nel_debug ({
					   if (symbol->value == NULL) {
					   parser_fatal_error (eng, "(goto_stmt->nel_T_GOTO label #1): NULL value for label\n%1S", symbol)
						   ;
					   };
				   });
		stmt = (nel_stmt *) (symbol->value);
		append_stmt (stmt, NULL);
	}
};



label:
any_ident {
	nel_debug ({ parser_trace (eng, "label->any_ident\n\n"); });
	{
		register char *name;
		register nel_symbol *symbol;
		parser_pop_name (eng, name);
		symbol = parser_lookup_label (eng, name);
		if (symbol == NULL) {
			symbol = nel_static_symbol_alloc (eng, name, NULL, NULL, nel_C_LABEL , 0, nel_L_NEL, 0);
			symbol->value = (char *) nel_stmt_alloc (eng, nel_S_TARGET,
							eng->parser->filename, eng->parser->line, 0, NULL);
			parser_insert_label (eng, symbol);
		}
		parser_push_symbol (eng, symbol);
	}
};



if_stmt:
nel_T_IF nel_T_LPAREN expression nel_T_RPAREN {
	nel_debug ({ parser_trace (eng, "if_stmt->nel_T_IF nel_T_LPAREN expression nel_T_RPAREN {} stmt else_part\n\n"); });
	{
		register nel_expr *cond;
		register nel_stmt *if_stmt;
		parser_pop_expr (eng, cond);
		
		nel_type* expr_type;
		expr_type = eval_expr_type(eng, cond);
		if(!expr_type)
			parser_stmt_error(eng, "cannot evaluate condition expression type");
		if(!nel_numerical_D_token(expr_type->simple.type) && expr_type->simple.type != nel_D_POINTER)
			parser_stmt_error(eng, "scalar expression needed as condition");

		if_stmt = nel_stmt_alloc (eng, nel_S_BRANCH, eng->parser->filename, eng->parser->line, eng->parser->level, cond, NULL, NULL);
		append_stmt (if_stmt, &(if_stmt->branch.true_branch));
		parser_push_stmt (eng, if_stmt);
	}
} block else_part {
	nel_debug ({ parser_trace (eng, "if_stmt->nel_T_IF nel_T_LPAREN expression nel_T_RPAREN stmt else_part\n\n"); });
};



else_part:
empty {
	nel_debug ({ parser_trace (eng, "else_part->empty\n\n"); });
	{
		/**********************************************************/
		/* form the following stmt list:                          */
		/*                                                        */
		/*          |                                             */
		/*          V                                             */
		/*   +--------------+                                     */
		/*   | branch: cond |--+ f                                */
		/*   +--------------+  |                                  */
		/*          | t        |                                  */
		/*          V          |                                  */
		/*   +--------------+  |                                  */
		/*   |  true body   |  |                                  */
		/*   +--------------+  |                                  */
		/*          |          |                                  */
		/*          V          |                                  */
		/*   +--------------+  |                                  */
		/*   |  end_target  |<-+                                  */
		/*   +--------------+                                     */
		/*          |                                             */
		/*          V                                             */
		/*                                                        */
		/**********************************************************/
		register nel_stmt *if_stmt;

		//end_target = nel_stmt_alloc (eng, nel_S_TARGET, eng->parser->filename, eng->parser->line, eng->parser->level, NULL);
		parser_pop_stmt (eng, if_stmt);

		if_stmt->branch.false_branch = NULL;
		if (eng->parser->append_pt != NULL) {
			*(eng->parser->append_pt) = NULL;
		}

		eng->parser->append_pt = &(if_stmt->branch.next);
		eng->parser->last_stmt = if_stmt;

	}
}|
nel_T_ELSE {
	nel_debug ({ parser_trace (eng, "else_part->nel_T_ELSE {} stmt\n\n"); });

	{
		/**********************************************************/
		/* form the following stmt list:                          */
		/*                                                        */
		/*             |                                          */
		/*             V                                          */
		/*      +--------------+                                  */
		/*      | branch: cond |--+ f                             */
		/*      +--------------+  |                               */
		/*             | t        |                               */
		/*             V          |                               */
		/*      +--------------+  |                               */
		/*      |  true body   |  |                               */
		/*      +--------------+  |                               */
		/*             |          |                               */
		/*   +---------+          |                               */
		/*   |                    |                               */
		/*   |  +--------------+  |                               */
		/*   |  |  false body  |<-+                               */
		/*   |  +--------------+                                  */
		/*   |         |                                          */
		/*   |         V                                          */
		/*   |  +--------------+                                  */
		/*   +->|  end target  |                                  */
		/*      +--------------+                                  */
		/*             |                                          */
		/*             V                                          */
		/*                                                        */
		/**********************************************************/
		register nel_stmt *if_stmt;

		//end_target = nel_stmt_alloc (eng, nel_S_TARGET, eng->parser->filename, eng->parser->line, eng->parser->level, NULL);
		parser_pop_stmt (eng, if_stmt);
		if (eng->parser->append_pt != NULL)
		{
			*(eng->parser->append_pt) = NULL; //end_target;
		}
		eng->parser->append_pt = &(if_stmt->branch.false_branch);
		eng->parser->last_stmt = if_stmt;
		
		parser_push_stmt (eng, if_stmt);
	}
} block {
	nel_debug ({ parser_trace (eng, "else_part->nel_T_ELSE stmt\n\n"); });
	{
		register nel_stmt *if_stmt;
		parser_pop_stmt (eng, if_stmt);

		if (eng->parser->append_pt != NULL) {
			*(eng->parser->append_pt) = NULL; //end_target;
		}

		eng->parser->append_pt = &(if_stmt->branch.next);
		eng->parser->last_stmt 	= if_stmt; 

	}
}|
nel_T_ELSE {
	nel_debug ({ parser_trace (eng, "else_part->nel_T_ELSE {} stmt\n\n"); });

	{
		register nel_stmt *if_stmt;
		//end_target = nel_stmt_alloc (eng, nel_S_TARGET, eng->parser->filename, eng->parser->line, eng->parser->level, NULL);
		parser_pop_stmt (eng, if_stmt);
		if (eng->parser->append_pt != NULL)
		{
			*(eng->parser->append_pt) = NULL; //end_target;
		}
		eng->parser->append_pt = &(if_stmt->branch.false_branch);
		eng->parser->last_stmt = if_stmt;
		
		parser_push_stmt (eng, if_stmt);
	}
} if_stmt {
	nel_debug ({ parser_trace (eng, "else_part->nel_T_ELSE stmt\n\n"); });
	{
		register nel_stmt *if_stmt;
		parser_pop_stmt (eng, if_stmt);

		if (eng->parser->append_pt != NULL) {
			*(eng->parser->append_pt) = NULL; //end_target;
		}

		eng->parser->append_pt = &(if_stmt->branch.next);
		eng->parser->last_stmt 	= if_stmt; 

	}
};



label_stmt:
label nel_T_COLON {
	nel_debug ({ parser_trace (eng, "label_stmt->label nel_T_COLON {} stmt\n\n"); });
	{
		register nel_symbol *symbol;
		register nel_stmt *stmt;
		parser_pop_symbol (eng, symbol);
		nel_debug ({
					   if (symbol->value == NULL) {
					   parser_fatal_error (eng, "(goto_stmt->nel_T_GOTO label #1): NULL value for label\n%1S", symbol)
						   ;
					   };
				   });
		if (symbol->declared) {
			parser_stmt_error (eng, "%I redefined", symbol);
		}
		symbol->declared = 1;
		symbol->level = eng->parser->level;
		stmt = (nel_stmt *) (symbol->value);
		stmt->target.level = eng->parser->level;
		append_stmt (stmt, &(stmt->target.next));
	}
} stmt {
	nel_debug ({ parser_trace (eng, "label_stmt->label nel_T_COLON stmt\n\n"); });
};



return_stmt:
	  nel_T_RETURN nel_T_SEMI {
		  nel_debug ({ parser_trace (eng, "return_stmt->nel_T_RETURN nel_T_SEMI\n\n"); });
		  /********************************************/
		  /* append the return stmt to the stmt list. */
		  /* the next statement might be unreachable. */
		  /********************************************/
		  {
#if 1
				if(eng->parser->prog_unit) {
					if( !eng->parser->prog_unit->type ||
						eng->parser->prog_unit->type->simple.type != nel_D_FUNCTION ||
						!eng->parser->prog_unit->type->function.return_type) {
						parser_fatal_error(eng, "function return type NULL");
					}
					if(eng->parser->prog_unit->type->function.return_type->simple.type != nel_D_VOID) {
						parser_stmt_error(eng, "illegal expr: you should return a value");
					}
				}
#endif
			  register nel_stmt *stmt = nel_stmt_alloc (eng, nel_S_RETURN,
										eng->parser->filename, eng->parser->line, NULL, NULL);
			  append_stmt (stmt, /* NULL */  &(stmt->ret.next));
		  }
	  }|
	  nel_T_RETURN expression nel_T_SEMI {
		  nel_debug ({ parser_trace (eng, "return_stmt->nel_T_RETURN expression nel_T_SEMI\n\n"); });
		  /********************************************/
		  /* append the return stmt to the stmt list. */
		  /* the next statement might be unreachable. */
		  /********************************************/
		  {
			  register nel_expr *expr;
			  register nel_stmt *stmt;
			  parser_pop_expr (eng, expr);
#if 1
				nel_type *return_type;
				return_type = eval_expr_type(eng, expr);
				if(!return_type)
					parser_stmt_error(eng, "illegal expr: illegal return expression");
				
				if(eng->parser->prog_unit) {
					if(	!eng->parser->prog_unit->type ||
						eng->parser->prog_unit->type->simple.type != nel_D_FUNCTION ||
						!eng->parser->prog_unit->type->function.return_type) {
						parser_fatal_error(eng, "funtion return type NULL");
					}
					if(eng->parser->prog_unit->type->function.return_type->simple.type == nel_D_VOID) {
						parser_stmt_error(eng, "illegal expr: no value should be returned");
					}
					else if(!is_asgn_compatible(eng, eng->parser->prog_unit->type->function.return_type, return_type)) {
						parser_stmt_error(eng, "illegal expr: return incompatible value");
					}
				}
#endif
			  stmt = nel_stmt_alloc (eng, nel_S_RETURN, eng->parser->filename,
									 eng->parser->line, expr, NULL);
			  append_stmt (stmt, /* NULL */ &(stmt->ret.next));
		  }
	  };



while_stmt:
nel_T_WHILE nel_T_LPAREN expression nel_T_RPAREN {
	nel_debug ({ parser_trace (eng, "while_stmt->nel_T_WHILE nel_T_LPAREN expression nel_T_RPAREN {} stmt\n\n"); });
	/**********************************************************/
	/*    form the following stmt list:                       */
	/*                                                        */
	/*             |                                          */
	/*             V                                          */
	/*      +--------------+                                  */
	/*   +->| cond_target  |                                  */
	/*   |  +--------------+                                  */
	/*   |         |                                          */
	/*   |         V                                          */
	/*   |  +--------------+ f                                */
	/*   |  | branch: cond |--+                               */
	/*   |  +--------------+  |                               */
	/*   |         | t        |                               */
	/*   |         V          |                               */
	/*   |  +--------------+  |                               */
	/*   |  |  loop body   |  |                               */
	/*   |  +--------------+  |                               */
	/*   |         |          |                               */
	/*   +---------+          |                               */
	/*                        |                               */
	/*      +--------------+  |                               */
	/*      |  end_target  |<-+                               */
	/*      +--------------+                                  */
	/*             |                                          */
	/*             V                                          */
	/*                                                        */
	/* cond_target is the continue target                     */
	/* end_target is the break target                         */
	/**********************************************************/
	{
		register nel_expr *cond;
		parser_pop_expr (eng, cond);
		
		nel_type* expr_type;
		expr_type = eval_expr_type(eng, cond);
		if(!expr_type)
			parser_stmt_error(eng, "cannot evaluate condition expression type");
		if(!nel_numerical_D_token(expr_type->simple.type) && expr_type->simple.type != nel_D_POINTER)
			parser_stmt_error(eng, "scalar expression needed as condition");
#if 0		
		nel_stmt *if_stmt;
		nel_stmt *cond_target;
		nel_stmt *end_target;
		end_target = nel_stmt_alloc(eng, nel_S_TARGET, eng->parser->filename, eng->parser->line, eng->parser->level, NULL);
		if_stmt = nel_stmt_alloc(eng, nel_S_BRANCH, eng->parser->filename, eng->parser->line, eng->parser->level, cond, NULL, end_target);
		cond_target = nel_stmt_alloc(eng, nel_S_TARGET, eng->parser->filename, eng->parser->line, eng->parser->level, if_stmt);
		append_stmt(cond_target, &(if_stmt->branch.true_branch));
		parser_push_stmt(eng, eng->parser->break_target);
		parser_push_stmt(eng, eng->parser->continue_target);
		parser_push_stmt(eng, if_stmt);
		parser_push_stmt(eng, cond_target);
		parser_push_stmt(eng, end_target);
		eng->parser->continue_target = cond_target;
		eng->parser->break_target = if_stmt->branch.false_branch;
#else	
		register nel_stmt *while_stmt;
		while_stmt = nel_stmt_alloc(eng, nel_S_WHILE, eng->parser->filename, eng->parser->line,  eng->parser->level, cond, NULL);
		append_stmt (while_stmt, &(while_stmt->while_stmt.body));
		parser_push_stmt(eng, eng->parser->break_target);
		parser_push_stmt(eng, eng->parser->continue_target);
		eng->parser->break_target = while_stmt;
		eng->parser->continue_target = while_stmt;
		parser_push_stmt (eng, while_stmt);
#endif
	}
} block {
	nel_debug ({ parser_trace (eng, "while_stmt->nel_T_WHILE nel_T_LPAREN expression nel_T_RPAREN {} stmt\n\n"); });
	{
#if 1
		register nel_stmt *while_stmt;
		parser_pop_stmt (eng, while_stmt);
		parser_pop_stmt(eng, eng->parser->continue_target);
		parser_pop_stmt(eng, eng->parser->break_target);
		eng->parser->append_pt = &(while_stmt->while_stmt.next);
		eng->parser->last_stmt = while_stmt;
#else
		nel_stmt *cond_target;
		nel_stmt *if_stmt;
		nel_stmt *jmp_target;
		nel_stmt *end_target;
		parser_pop_stmt(eng, end_target);
		end_target->target.line = eng->parser->line;
		parser_pop_stmt(eng, cond_target);
		parser_pop_stmt(eng, if_stmt);
		jmp_target = nel_stmt_alloc(eng, nel_S_GOTO, eng->parser->filename, eng->parser->line, cond_target);
		append_stmt(jmp_target, NULL);
		eng->parser->append_pt = &(if_stmt->branch.next);
		parser_pop_stmt(eng, eng->parser->continue_target);
		parser_pop_stmt(eng, eng->parser->break_target);
		eng->parser->last_stmt = if_stmt;
#endif 
	}
};



/**/
/*****************************************************************************/
/* productions for parsing expressions.                                      */
/*****************************************************************************/



/*****************************************************************************/
/* expression->* , *_expr->*, and const->* productions all push 1 item on the */
/* semanitc stack, the epression tree (nel_expr *).                      */
/*****************************************************************************/

expression:
expression nel_T_COMMA asgn_expr {
	nel_debug ({ parser_trace (eng, "expression->expression nel_T_COMMA asgn_expr\n\n"); });
	binary_op_acts (nel_O_COMPOUND);
}|
asgn_expr {
	nel_debug ({ parser_trace (eng, "expression->asgn_expr\n\n"); });
	/*************************/
	/* leave asgn_expr pushed */
	/*************************/
};



/*****************************************************************************/
/* asgn_expr->* productions push the expression tree (nel_expr *)         */
/*****************************************************************************/

asgn_expr:
unary_expr nel_T_EQ asgn_expr {
	nel_debug ({ parser_trace (eng, "asgn_expr->unary_expr nel_T_EQ asgn_expr\n\n"); });
	binary_op_acts (nel_O_ASGN);
}|
unary_expr nel_T_PLUSEQ asgn_expr {
	nel_debug ({ parser_trace (eng, "asgn_expr->unary_expr nel_T_PLUSEQ asgn_expr\n\n"); });
	binary_op_acts (nel_O_ADD_ASGN);
}|
unary_expr nel_T_MINUSEQ asgn_expr {
	nel_debug ({ parser_trace (eng, "asgn_expr->unary_expr nel_T_MINUSEQ asgn_expr\n\n"); });
	binary_op_acts (nel_O_SUB_ASGN);
}|
unary_expr nel_T_STAREQ asgn_expr {
	nel_debug ({ parser_trace (eng, "asgn_expr->unary_expr nel_T_STAREQ asgn_expr\n\n"); });
	binary_op_acts (nel_O_MULT_ASGN);
}|
unary_expr nel_T_SLASHEQ asgn_expr {
	nel_debug ({ parser_trace (eng, "asgn_expr->unary_expr nel_T_SLASHEQ asgn_expr\n\n"); });
	binary_op_acts (nel_O_DIV_ASGN);
}|
unary_expr nel_T_PERCENTEQ asgn_expr {
	nel_debug ({ parser_trace (eng, "asgn_expr->unary_expr nel_T_PERCENTEQ asgn_expr\n\n"); });
	binary_op_acts (nel_O_MOD_ASGN);
}|
unary_expr nel_T_DLARROWEQ asgn_expr {
	nel_debug ({ parser_trace (eng, "asgn_expr->unary_expr nel_T_DLARROWEQ asgn_expr\n\n"); });
	binary_op_acts (nel_O_LSHIFT_ASGN);
}|
unary_expr nel_T_DRARROWEQ asgn_expr {
	nel_debug ({ parser_trace (eng, "asgn_expr->unary_expr nel_T_DRARROWEQ asgn_expr\n\n"); });
	binary_op_acts (nel_O_RSHIFT_ASGN);
}|
unary_expr nel_T_AMPEREQ asgn_expr {
	nel_debug ({ parser_trace (eng, "asgn_expr->unary_expr nel_T_AMPEREQ asgn_expr\n\n"); });
	binary_op_acts (nel_O_BIT_AND_ASGN);
}|
unary_expr nel_T_UPARROWEQ asgn_expr {
	nel_debug ({ parser_trace (eng, "asgn_expr->unary_expr nel_T_UPARROWEQ asgn_expr\n\n"); });
	binary_op_acts (nel_O_BIT_XOR_ASGN);
}|
unary_expr nel_T_BAREQ asgn_expr {
	nel_debug ({ parser_trace (eng, "asgn_expr->unary_expr nel_T_BAREQ asgn_expr\n\n"); });
	binary_op_acts (nel_O_BIT_OR_ASGN);
}|
binary_expr {
	nel_debug ({ parser_trace (eng, "asgn_expr->binary_expr\n\n"); });
	/***************************/
	/* leave binary_expr pushed */
	/***************************/
};



/*****************************************************************************/
/* binary_expr->* productions push the expression tree (nel_expr *)       */
/*****************************************************************************/

binary_expr:
binary_expr nel_T_STAR binary_expr {
	nel_debug ({ parser_trace (eng, "binary_expr->binary_expr nel_T_STAR binary_expr\n\n"); });
	binary_op_acts (nel_O_MULT);
}|
binary_expr nel_T_SLASH binary_expr {
	nel_debug ({ parser_trace (eng, "binary_expr->binary_expr nel_T_SLASH binary_expr\n\n"); });
	binary_op_acts (nel_O_DIV);
}|
binary_expr nel_T_PERCENT binary_expr {
	nel_debug ({ parser_trace (eng, "binary_expr->binary_expr nel_T_PERCENT binary_expr\n\n"); });
	binary_op_acts (nel_O_MOD);
}|
binary_expr nel_T_PLUS binary_expr {
	nel_debug ({ parser_trace (eng, "binary_expr->binary_expr nel_T_PLUS binary_expr\n\n"); });
	binary_op_acts (nel_O_ADD);
}|
binary_expr nel_T_MINUS binary_expr {
	nel_debug ({ parser_trace (eng, "binary_expr->binary_expr nel_T_MINUS binary_expr\n\n"); });
	binary_op_acts (nel_O_SUB);
}|
binary_expr nel_T_DLARROW binary_expr {
	nel_debug ({ parser_trace (eng, "binary_expr->binary_expr nel_T_DLARROW binary_expr\n\n"); });
	binary_op_acts (nel_O_LSHIFT);
}|
binary_expr nel_T_DRARROW binary_expr {
	nel_debug ({ parser_trace (eng, "binary_expr->binary_expr nel_T_DRARROW binary_expr\n\n"); });
	binary_op_acts (nel_O_RSHIFT);
}|
binary_expr nel_T_LARROW binary_expr {
	nel_debug ({ parser_trace (eng, "binary_expr->binary_expr nel_T_LARROW binary_expr\n\n"); });
	binary_op_acts (nel_O_LT);
}|
binary_expr nel_T_RARROW binary_expr {
	nel_debug ({ parser_trace (eng, "binary_expr->binary_expr nel_T_RARROW binary_expr\n\n"); });
	binary_op_acts (nel_O_GT);
}|
binary_expr nel_T_LARROWEQ binary_expr {
	nel_debug ({ parser_trace (eng, "binary_expr->binary_expr nel_T_LARROWEQ binary_expr\n\n"); });
	binary_op_acts (nel_O_LE);
}|
binary_expr nel_T_RARROWEQ binary_expr {
	nel_debug ({ parser_trace (eng, "binary_expr->binary_expr nel_T_RARROWEQ binary_expr\n\n"); });
	binary_op_acts (nel_O_GE);
}|
binary_expr nel_T_DEQ binary_expr {
	nel_debug ({ parser_trace (eng, "binary_expr->binary_expr nel_T_DEQ binary_expr\n\n"); });
	binary_op_acts (nel_O_EQ);
}|
binary_expr nel_T_EXCLAMEQ binary_expr {
	nel_debug ({ parser_trace (eng, "binary_expr->binary_expr nel_T_EXCLAMEQ binary_expr\n\n"); });
	binary_op_acts (nel_O_NE);
}|
binary_expr nel_T_AMPER binary_expr {
	nel_debug ({ parser_trace (eng, "binary_expr->binary_expr nel_T_AMPER binary_expr\n\n"); });
	binary_op_acts (nel_O_BIT_AND);
}|
binary_expr nel_T_UPARROW binary_expr {
	nel_debug ({ parser_trace (eng, "binary_expr->binary_expr nel_T_UPARROW binary_expr\n\n"); });
	binary_op_acts (nel_O_BIT_XOR);
}|
binary_expr nel_T_BAR binary_expr {
	nel_debug ({ parser_trace (eng, "binary_expr->binary_expr nel_T_BAR binary_expr\n\n"); });
	binary_op_acts (nel_O_BIT_OR); 
}|
binary_expr nel_T_DAMPER binary_expr {
	nel_debug ({ parser_trace (eng, "binary_expr->binary_expr nel_T_DAMPER binary_expr\n\n"); });
	binary_op_acts (nel_O_AND);
}|
binary_expr nel_T_DBAR binary_expr {
	nel_debug ({ parser_trace (eng, "binary_expr->binary_expr nel_T_DBAR binary_expr\n\n"); });
	binary_op_acts (nel_O_OR);
}|

binary_expr nel_T_TILDE { eng->parser->want_regexp = 1; } extend_regexp_expr {
	register nel_symbol *symbol, *func, *tag;
	register nel_expr   *extend = NULL, 
			    *data, *pattern; 
	register nel_expr   *expr;
	register nel_type   *type;
	register int stream_flag = 0;
	register int nocase_flag = 0;
	register int pcre_flag = 0;

	nel_debug ({ parser_trace (eng, "binary_expr->binary_expr nel_T_TILDE extend_regexp_expr\n\n"); });

	parser_pop_expr(eng, extend);
	parser_pop_expr(eng, pattern);
	parser_pop_expr(eng, data);	/* end of arg list */

	/* set nocase flag from extend regexp flag  */
	if(extend) {
		if( (symbol = extend->symbol.symbol ) != NULL )	{
			register char *p = symbol->value;
			register char c;
			while((c = *p) != '\0') {
				switch (c) {
					case 'i': 
						pcre_flag |= PCRE_CASELESS; 
						break;
					case 's':
						pcre_flag |= PCRE_DOTALL;
						break;
					case 'm':
						pcre_flag |= PCRE_MULTILINE;
						break;
					default:   break;
				}
				p++;
			}

		}

	}

	switch(pcre_flag)
	{
		case 0:		//no pcre extopt
			func = parser_lookup_ident(eng, "clus_buf_case_match");
			break;
		case PCRE_CASELESS:	//'i'
			func = parser_lookup_ident(eng, "clus_buf_nocase_match");
			break;
		case PCRE_DOTALL:	//'s'
			func = parser_lookup_ident(eng, "clus_buf_dotall_match");
			break;
		case PCRE_CASELESS | PCRE_DOTALL:	//'si'
			func = parser_lookup_ident(eng, "clus_buf_dotall_nocase_match");
			break;
		case PCRE_CASELESS | PCRE_DOTALL | PCRE_MULTILINE:	//'smi'
			func = parser_lookup_ident(eng, "clus_buf_dotall_multiline_nocase_match");
			break;
		case PCRE_DOTALL | PCRE_MULTILINE:	//'sm'
			func = parser_lookup_ident(eng, "clus_buf_dotall_multiline_match");
			break;
		default:
			func = NULL;
			break;
	}
	if(!func)
		printf("NULL func!\n");

	switch(data->gen.opcode) {
	case nel_O_ARRAY_RANGE:
		if(func != NULL ){
			register nel_expr_list *args;
            		if( eng->optimize_level == 0 ) { 
				register nel_symbol *temp_symbol;   
				register nel_type *temp_type;   
				struct match_info *ms;
	            
				func = parser_lookup_ident(eng, "buf_match");
				ms = match_init((unsigned char **)&pattern->symbol.symbol->value, 1, pcre_flag); 
				pattern->symbol.symbol->value = nel_static_value_alloc(eng, 
					sizeof(struct match_info*), nel_alignment_of(struct match_info *));

				//*((unsigned int*)(pattern->symbol.symbol->value)) = (unsigned int)ms;
				*((struct match_info **)(pattern->symbol.symbol->value)) = ms;



				temp_symbol = nel_lookup_symbol("match_info", eng->nel_static_tag_hash,NULL);
				temp_type = temp_symbol->type->typedef_name.descriptor;
				temp_type = nel_type_alloc(eng, nel_D_POINTER, sizeof(struct match_info *), 
						nel_alignment_of(struct match_info *), 0,0,temp_type);
				pattern->symbol.symbol->type = temp_type;
										
	        	}

			args = nel_expr_list_alloc(eng, pattern, NULL);
			args = nel_expr_list_alloc(eng, data->range.upper, args);
			args = nel_expr_list_alloc(eng, data->range.array, args);


			expr = nel_expr_alloc (eng, nel_O_FUNCALL, 
			nel_expr_alloc (eng, nel_O_SYMBOL, func), 3, args);
		}
		break;

	case nel_O_MEMBER:
		if( ((symbol = data->member.member) == NULL)
			|| ((type = symbol->type) == NULL) ) {
			if ( data != NULL ) { 
				parser_error(eng, "data (%s) not defined!\n",  data );
			} else {
				parser_error(eng, "data not defined!\n" );
			}
		}
		goto found_stream;

	case nel_O_SYMBOL:
		if( ((symbol = data->symbol.symbol) == NULL)
			|| ((type = symbol->type) == NULL) ) {
			if ( data != NULL ) { 
				parser_error(eng, "data (%s) not defined!\n", data );
			} else {
				parser_error(eng, "data not defined!\n" );
			}
		}

found_stream:
		if (type->simple.type == nel_D_POINTER ) {
			type = type->pointer.deref_type;
			if(type->simple.type == nel_D_STRUCT ) {
				tag = type->s_u.tag;
				if(tag && strcmp(tag->name, "stream") == 0 ){
					stream_flag = 1;
				} 
			}	
		}

		if(stream_flag == 1) {
			switch(pcre_flag)
			{
				case 0:	//no pcre extopt
					func = parser_lookup_ident(eng, "clus_stream_case_match");
					break;
				case PCRE_CASELESS:	//'i'
					func = parser_lookup_ident(eng, "clus_stream_nocase_match");
					break;
				case PCRE_CASELESS | PCRE_DOTALL:	//'si'
					func = parser_lookup_ident(eng, "clus_stream_dotall_nocase_match");
					break;
				case PCRE_CASELESS | PCRE_DOTALL | PCRE_MULTILINE:	//'smi'
					func = parser_lookup_ident(eng, "clus_stream_dotall_multiline_nocase_match");
					break;
				default:
					func = NULL;
					break;
			}

			if(func != NULL ){
				register nel_expr_list *args;
				if( eng->optimize_level == 0 ) {
					nel_symbol *temp_symbol;   
					nel_type *temp_type;   
					struct match_info *ms;

					func = parser_lookup_ident(eng, "stream_match");
					ms = match_init((unsigned char **)&pattern->symbol.symbol->value, 1, pcre_flag); 
					pattern->symbol.symbol->value = nel_static_value_alloc(eng, 
						sizeof(struct match_info*), nel_alignment_of(struct match_info *));

					//*((unsigned int*)(pattern->symbol.symbol->value)) = (unsigned int)ms;
					*((struct match_info **)(pattern->symbol.symbol->value)) = ms;


					pattern->symbol.symbol->class = nel_C_CONST;
					temp_symbol = nel_lookup_symbol("match_info", eng->nel_static_tag_hash,NULL);
					temp_type = temp_symbol->type->typedef_name.descriptor;
					temp_type = nel_type_alloc(eng, nel_D_POINTER, sizeof(struct match_info *), 
							nel_alignment_of(struct match_info *), 0,0,temp_type);
					pattern->symbol.symbol->type = temp_type;

                		}

				args = nel_expr_list_alloc(eng, pattern, NULL);
				args = nel_expr_list_alloc(eng, data, args);

				expr = nel_expr_alloc (eng, nel_O_FUNCALL, 
				nel_expr_alloc (eng, nel_O_SYMBOL, func), 2, args);
			}
			else
			{
				printf("NULL func!\n");
			}
		}
		break;

	default :
		break;
	}

	eng->parser->want_regexp = 0;
	parser_push_expr (eng, expr);

}|
binary_expr nel_T_EXCLAM_TILDE { eng->parser->want_regexp = 1; } extend_regexp_expr {
	register nel_symbol *symbol, *func, *tag;
	register nel_expr   *extend = NULL, 
			    *data, *pattern; 
	register nel_expr   *expr;
	register nel_type   *type;
	register int stream_flag = 0;
	register int nocase_flag = 0;
	register int pcre_flag = 0;

	nel_debug ({ parser_trace (eng, "binary_expr->binary_expr nel_T_EXCLAM_TILDE extend_regexp_expr\n\n"); });

	parser_pop_expr(eng, extend);
	parser_pop_expr(eng, pattern);
	parser_pop_expr(eng, data);	/* end of arg list */

	/* set nocase flag from extend regexp flag */
	if(extend) {
		if( (symbol = extend->symbol.symbol ) != NULL )	{
			register char *p = symbol->value;
			register char c;
			while((c = *p) != '\0') {
				switch (c) {					
					case 'i': 
						pcre_flag |= PCRE_CASELESS; 
						break;
					case 's':
						pcre_flag |= PCRE_DOTALL;
						break;
					case 'm':
						pcre_flag |= PCRE_MULTILINE;
						break;
					default:   break;
				}
				p++;
			}

		}

	}
	
	switch(pcre_flag)
	{
		case 0:		//no pcre extopt
			func = parser_lookup_ident(eng, "clus_buf_case_no_match");
			break;
		case PCRE_CASELESS:	//'i'
			func = parser_lookup_ident(eng, "clus_buf_nocase_no_match");
			break;
		case PCRE_DOTALL:	//'s'
			func = parser_lookup_ident(eng, "clus_buf_dotall_no_match");
			break;
		case PCRE_CASELESS | PCRE_DOTALL:	//'si'
			func = parser_lookup_ident(eng, "clus_buf_dotall_nocase_no_match");
			break;
		case PCRE_CASELESS | PCRE_DOTALL | PCRE_MULTILINE:	//'smi'
			func = parser_lookup_ident(eng, "clus_buf_dotall_multiline_nocase_no_match");
			break;
		case PCRE_DOTALL | PCRE_MULTILINE:	//'sm'
			func = parser_lookup_ident(eng, "clus_buf_dotall_multiline_no_match");
			break;
		default:
			func = NULL;
			break;
	}

	/* stream or buf? */
	switch(data->gen.opcode) {
	case nel_O_ARRAY_RANGE:
		if(func != NULL ){
			register nel_expr_list *args;
			if( !eng->optimize_level ) 
			{ 
				register nel_symbol *temp_symbol;  
				register nel_type *temp_type;   
				struct match_info *ms;
				func = parser_lookup_ident(eng, "buf_no_match");
				ms = match_init((unsigned char **)&pattern->symbol.symbol->value, 1, pcre_flag); 
				pattern->symbol.symbol->value = nel_static_value_alloc(eng, 
					sizeof(struct match_info*), nel_alignment_of(struct match_info *));

				//*((unsigned int*)(pattern->symbol.symbol->value)) = (unsigned int)ms;
				*((struct match_info **)(pattern->symbol.symbol->value)) = ms;


				temp_symbol = nel_lookup_symbol("match_info", eng->nel_static_tag_hash,NULL);
				temp_type = temp_symbol->type->typedef_name.descriptor;
				pattern->symbol.symbol->type = nel_type_alloc(eng, nel_D_POINTER, 
					sizeof(struct match_info *), nel_alignment_of(struct match_info *), 
											0,0,temp_type);
										
			}
			
			args = nel_expr_list_alloc(eng, pattern, NULL);
			args = nel_expr_list_alloc(eng, data->range.upper, args);
			args = nel_expr_list_alloc(eng, data->range.array, args);

			expr = nel_expr_alloc (eng, nel_O_FUNCALL, 
			nel_expr_alloc (eng, nel_O_SYMBOL, func), 3, args);
		}
		break;

	case nel_O_MEMBER:
		if( ((symbol = data->member.member) == NULL)
			|| ((type = symbol->type) == NULL) ) {
			if (data != NULL ) { 
				parser_error(eng, "data (%s) not defined!\n", data);
			} else {
				parser_error(eng, "data not defined!\n");
			}
		}
		goto found_stream_2;

	case nel_O_SYMBOL:
		if( ((symbol = data->symbol.symbol) == NULL)
			|| ((type = symbol->type) == NULL) ) {
			if ( data != NULL ) {
				parser_error(eng, "data (%s) not defined!\n", data);
			} else {
				parser_error(eng, "data not defined!\n" );
			}
		}

found_stream_2:
		if (type->simple.type == nel_D_POINTER ) {
			type = type->pointer.deref_type;
			if(type->simple.type == nel_D_STRUCT ) {
				tag = type->s_u.tag;
				if(tag && strcmp(tag->name, "stream") == 0 ){
					stream_flag = 1;
				} 
			}	
		}

		if(stream_flag == 1) {
			func = parser_lookup_ident(eng, nocase_flag 
					? "clus_stream_nocase_no_match" 			
				: "clus_stream_case_no_match");	
			if(func != NULL ){
				register nel_expr_list *args;
				args = nel_expr_list_alloc(eng, pattern, NULL);
				args = nel_expr_list_alloc(eng, data, args);

				expr = nel_expr_alloc (eng, nel_O_FUNCALL, 
				nel_expr_alloc (eng, nel_O_SYMBOL, func), 2, args);
			}
		}
		break;

	default :
		break;
	}

	eng->parser->want_regexp = 0;
	parser_push_expr (eng, expr);

}|
binary_expr nel_T_TILDEEQ { eng->parser->want_regexp = 0; } exact_pattern_expr {
	register nel_symbol *symbol, *func, *tag;
	register nel_expr   *data, *pattern; 
	register nel_expr   *expr;
	register nel_type   *type;
	register int stream_flag = 0;
	register nel_expr_list *args;

	nel_debug ({ parser_trace (eng, "binary_expr->binary_expr nel_T_TILDE nel_T_C_STRING\n\n"); });

#if 0
	{
		register char *name;
		register int size;
		register nel_type *type;
		register nel_symbol *symbol;
		register nel_expr *expr;

		name = nel_insert_name (eng, eng->parser->text);
		size = strlen (name) + 1;
		type = nel_type_alloc (eng, nel_D_ARRAY, size, nel_alignment_of(char)/* (char *)*/, 1, 0, nel_char_type, nel_int_type, 1, 0, 1, size - 1);
		symbol = nel_static_symbol_alloc (eng, name, type, NULL, nel_C_CONST, 0, nel_L_NEL, 0);
		symbol->value = name;
		pattern = nel_expr_alloc (eng, nel_O_SYMBOL, symbol);

	}
#endif

	parser_pop_expr(eng, pattern);
	parser_pop_expr(eng, data);	
	func = parser_lookup_ident(eng, "clus_buf_exact_match");
	if(!func)
		printf("NULL func!\n");

	switch(data->gen.opcode) {
	case nel_O_ARRAY_RANGE:
		//if(func != NULL ){
		if( eng->optimize_level == 0 ) { 

			register nel_symbol *temp_symbol;   
			register nel_type *temp_type;   
			struct exact_match_info *ms;
	    
			func = parser_lookup_ident(eng, "buf_exact_match");
			ms = exact_match_init((unsigned char **)&pattern->symbol.symbol->value, 
						1, 0 ); 


			pattern->symbol.symbol->value=nel_static_value_alloc(eng, 
					sizeof(struct exact_match_info*), 
				nel_alignment_of(struct exact_match_info *));

			//*((unsigned int*)(pattern->symbol.symbol->value)) = (unsigned int)ms;
			*((struct exact_match_info **)(pattern->symbol.symbol->value)) = ms;


			temp_symbol = nel_lookup_symbol("exact_match_info", 
						eng->nel_static_tag_hash,NULL);
			temp_type = temp_symbol->type->typedef_name.descriptor;
			temp_type = nel_type_alloc(eng, nel_D_POINTER, 
					sizeof(struct exact_match_info *), 
					nel_alignment_of(struct exact_match_info *), 
								0,0,temp_type);
			pattern->symbol.symbol->type = temp_type;
		}

		args = nel_expr_list_alloc(eng, pattern, NULL);
		args = nel_expr_list_alloc(eng, data->range.upper, args);
		args = nel_expr_list_alloc(eng, data->range.array, args);

		expr = nel_expr_alloc (eng, nel_O_FUNCALL, 
		nel_expr_alloc (eng, nel_O_SYMBOL, func), 3, args);
		//}
		break;

	case nel_O_MEMBER:
		if( ((symbol = data->member.member) == NULL)
			|| ((type = symbol->type) == NULL) ) {
			if ( data != NULL ) {
				parser_error(eng, "data (%s) not defined!\n", data );
			} else {
				parser_error(eng, "data not defined!\n" );
			}
		}
		goto found_exact_stream;

	case nel_O_SYMBOL:
		if( ((symbol = data->symbol.symbol) == NULL)
			|| ((type = symbol->type) == NULL) ) {
			if ( data != NULL ){
				parser_error(eng, "data (%s) not defined!\n", data );
			}else {
				parser_error(eng, "data not defined!\n" );
			}
		}

found_exact_stream:
		if (type->simple.type == nel_D_POINTER ) {
			type = type->pointer.deref_type;
			if(type->simple.type == nel_D_STRUCT ) {
				tag = type->s_u.tag;
				if(tag && strcmp(tag->name, "stream") == 0 ){
					stream_flag = 1;
				} 
			}	
		}

		if(stream_flag == 1) {
			func = parser_lookup_ident(eng, "clus_stream_exact_match");
			//if(func != NULL ){
			if( eng->optimize_level == 0 ) {
				nel_symbol *temp_symbol;   
				nel_type *temp_type;   
				struct exact_match_info *ms;
				//register nel_expr_list *args;

				func = parser_lookup_ident(eng, "stream_exact_match");
				ms = exact_match_init((unsigned char **)&pattern->symbol.symbol->value, 1, 0); 
				pattern->symbol.symbol->value = nel_static_value_alloc(eng, 
					sizeof(struct exact_match_info*), 
					nel_alignment_of(struct exact_match_info *));

				//*((unsigned int*)(pattern->symbol.symbol->value)) = (unsigned int)ms;
				*((struct exact_match_info **)(pattern->symbol.symbol->value)) = ms;


				pattern->symbol.symbol->class = nel_C_CONST;
				temp_symbol = nel_lookup_symbol("exact_match_info", eng->nel_static_tag_hash,NULL);
				temp_type = temp_symbol->type->typedef_name.descriptor;
				temp_type = nel_type_alloc(eng, 
							nel_D_POINTER, 
					sizeof(struct exact_match_info *), 
					nel_alignment_of(struct exact_match_info *), 
					0,0,temp_type);
				pattern->symbol.symbol->type = temp_type;

			}

			args = nel_expr_list_alloc(eng, pattern, NULL);
			args = nel_expr_list_alloc(eng, data, args);

			expr = nel_expr_alloc (eng, nel_O_FUNCALL, 
			nel_expr_alloc (eng, nel_O_SYMBOL, func), 2, args);
			//}
			//else
			//{
			//	printf("NULL func!\n");
			//}
		}
		break;

	default :
		break;
	}

	eng->parser->want_regexp = 0;
	parser_push_expr (eng, expr);

}|
asgn_expr nel_T_QUESTION asgn_expr nel_T_COLON asgn_expr {
	nel_debug ({ parser_trace (eng, "binary_expr->binary_expr nel_T_QUESTION binary_expr nel_T_COLON binary_expr\n\n"); });

	{
		register nel_expr *expr1;
		register nel_expr *expr2;
		register nel_expr *expr3;
		parser_pop_expr (eng, expr3);
		parser_pop_expr (eng, expr2);
		parser_pop_expr (eng, expr1);
		expr1 = nel_expr_alloc (eng, nel_O_COND, expr1, expr2, expr3);
		parser_push_expr (eng, expr1);
	}
}|
cast_expr {
	nel_debug ({ parser_trace (eng, "binary_expr->cast_expr\n\n"); });
	/*************************/
	/* leave cast_expr pushed */
	/*************************/
};

exact_pattern_expr: nel_T_C_STRING {
	nel_debug ({ parser_trace (eng, "exact_pattern_expr->nel_T_C_STRING\n\n"); });
	{
		register char *name;
		register int size;
		register nel_type *type;
		register nel_symbol *symbol;
		register nel_expr *expr;

		name = nel_insert_name (eng, eng->parser->text);
		size = strlen (name) + 1;
		type = nel_type_alloc (eng, nel_D_ARRAY, size, nel_alignment_of(char), 1, 0, nel_char_type, nel_int_type, 1, 0, 1, size - 1);
		symbol = nel_static_symbol_alloc (eng, name, type, NULL, nel_C_CONST, 0, nel_L_NEL, 0);
		symbol->value = name;
		expr = nel_expr_alloc (eng, nel_O_SYMBOL, symbol);
		parser_push_expr (eng, expr);
	}
};

extend_regexp_expr:  regexp_expr opt_extend_expr {
	nel_debug ({ parser_trace (eng, "extend_regexp_expr->regexp_expr opt_extend_expr\n\n"); });
};

regexp_expr: nel_T_REGEXP {
	nel_debug ({ parser_trace (eng, "regexp_expr->nel_T_REGEXP\n\n"); });
	{
		register char *name;
		register int size;
		register nel_type *type;
		register nel_symbol *symbol;
		register nel_expr *expr;

		name = nel_insert_name (eng, eng->parser->text);
		size = strlen (name) + 1;
		type = nel_type_alloc (eng, nel_D_ARRAY, size, nel_alignment_of(char), 1, 0, nel_char_type, nel_int_type, 1, 0, 1, size - 1);
		symbol = nel_static_symbol_alloc (eng, name, type, NULL, nel_C_CONST, 0, nel_L_NEL, 0);
		symbol->value = name;
		expr = nel_expr_alloc (eng, nel_O_SYMBOL, symbol);
		parser_push_expr (eng, expr);
	}
}


opt_extend_expr: {
	nel_debug ({ parser_trace (eng, "regexp_expr->regexp_expr_list nel_T_REGEXP\n\n"); });
	{
		parser_push_expr (eng, NULL);
	}
}|
nel_T_REGEXT {
	nel_debug ({ parser_trace (eng, "regexp_expr->regexp_expr_list nel_T_REGEXP\n\n"); });
	{
		register char *name;
		register int size;
		register nel_type *type;
		register nel_symbol *symbol;
		register nel_expr *expr;

		name = nel_insert_name (eng, eng->parser->text);
		size = strlen (name) + 1;
		type = nel_type_alloc (eng, nel_D_ARRAY, size, nel_alignment_of(char), 1, 0, nel_char_type, nel_int_type, 1, 0, 1, size - 1);
		symbol = nel_static_symbol_alloc (eng, name, type, NULL, nel_C_CONST, 0, nel_L_NEL, 0);
		symbol->value = name;
		expr = nel_expr_alloc (eng, nel_O_SYMBOL, symbol);
		parser_push_expr (eng, expr);
	}
}
;

/*****************************************************************************/
/* cast_expr->* productions push the expression tree (nel_expr *)         */
/*****************************************************************************/

cast_expr:
nel_T_LPAREN type_name nel_T_RPAREN cast_expr {
	{
		register nel_type *type;
		register nel_expr *expr;
		register nel_symbol *symbol;

		parser_pop_expr (eng, expr);
		parser_pop_type (eng, type);
		expr = nel_expr_alloc (eng, nel_O_CAST, type, expr);


#if 0
		symbol = intp_eval_expr_2(eng, expr);
		if (symbol->type == NULL) {
			if (symbol->name == NULL) {
				parser_stmt_error (eng, "NULL type for cast expression", symbol);
			} else {
				parser_stmt_error (eng, "NULL tyep for %I", symbol);
			}
		}
		expr = nel_expr_alloc (eng, nel_O_SYMBOL, symbol);
#endif

		parser_push_expr (eng, expr);
	}
}|
unary_expr {
	nel_debug ({ parser_trace (eng, "cast_expr->unary_expr\n\n"); });
	/**************************/
	/* leave unary_expr pushed */
	/**************************/
};



/*****************************************************************************/
/* unary_expr->* productions push the expression tree (nel_expr *)        */
/*****************************************************************************/

unary_expr:
nel_T_DPLUS unary_expr {
	nel_debug ({ parser_trace (eng, "unary_expr->nel_T_DPLUS unary_expr\n\n"); });
	unary_op_acts (nel_O_PRE_INC);
}|
nel_T_DMINUS unary_expr {
	nel_debug ({ parser_trace (eng, "unary_expr->nel_T_DMINUS unary_expr\n\n"); });
	unary_op_acts (nel_O_PRE_DEC);
}|
nel_T_AMPER cast_expr {
	nel_debug ({ parser_trace (eng, "unary_expr->nel_T_AMPER cast_expr\n\n"); });
	unary_op_acts (nel_O_ADDRESS);
}|
nel_T_STAR cast_expr {
	nel_debug ({ parser_trace (eng, "unary_expr->nel_T_STAR cast_expr\n\n"); });
	unary_op_acts (nel_O_DEREF);
}|
nel_T_PLUS cast_expr {
	nel_debug ({ parser_trace (eng, "unary_expr->nel_T_PLUS cast_expr\n\n"); });
	unary_op_acts (nel_O_POS);
}|
nel_T_MINUS cast_expr {
	nel_debug ({ parser_trace (eng, "unary_expr->nel_T_MINUS cast_expr\n\n"); });
	unary_op_acts (nel_O_NEG);
}|
nel_T_TILDE cast_expr {
	nel_debug ({ parser_trace (eng, "unary_expr->nel_T_TILDE cast_expr\n\n"); });
	unary_op_acts (nel_O_BIT_NEG);
}|
nel_T_EXCLAM cast_expr {
	nel_debug ({ parser_trace (eng, "unary_expr->nel_T_EXCLAM cast_expr\n\n"); });
	unary_op_acts (nel_O_NOT);
}|
nel_T_SIZEOF unary_expr {
	/*******************************************/
	/* only fill in the expr field of the node */
	/* leave the type field NULL.              */
	/*******************************************/
	nel_debug ({ parser_trace (eng, "unary_expr->nel_T_SIZEOF unary_expr\n\n"); });
	{
		register nel_expr *expr;
		register nel_symbol *symbol;

		parser_pop_expr (eng, expr);
		expr = nel_expr_alloc (eng, nel_O_SIZEOF, NULL, expr);

#if 0
		symbol = intp_eval_expr_2 (eng, expr);
		if (symbol->value == NULL) {
			if (symbol->name == NULL) {
				parser_stmt_error (eng, "NULL address for sizeof expression", symbol);
			} else {
				parser_stmt_error (eng, "NULL address for %I", symbol);
			}
		}
#else
		nel_type* expr_type;
		expr_type = eval_expr_type(eng, expr->type.expr);
		if(!expr_type)
		{
			parser_stmt_error(eng, "illegal expr");
		}
		symbol = nel_static_symbol_alloc(eng, NULL, nel_int_type, nel_static_value_alloc(eng, sizeof(int), nel_alignment_of(int)), nel_C_CONST, 0, nel_L_NEL, eng->parser->level);
		*((int*)(symbol->value)) = expr_type->simple.size;
		expr = nel_expr_alloc (eng, nel_O_SYMBOL, symbol);
#endif
		parser_push_expr (eng, expr);
	}
}|
nel_T_SIZEOF nel_T_LPAREN type_name nel_T_RPAREN {
	/*******************************************/
	/* only fill in the type field of the node */
	/* leave the expr field NULL.              */
	/*******************************************/
	nel_debug ({ parser_trace (eng, "unary_expr->nel_T_SIZEOF nel_T_LPAREN type_name nel_T_RPAREN\n\n"); });
	{
		register nel_type *type;
		register nel_expr *expr;
		register nel_symbol *symbol;

		parser_pop_type (eng, type);
		expr = nel_expr_alloc (eng, nel_O_SIZEOF, type, NULL);
#if 0
		symbol = intp_eval_expr_2 (eng, expr);
		if (symbol->value == NULL) {
			if (symbol->name == NULL) {
				parser_stmt_error (eng, "NULL address for sizeof expression", symbol);
			} else {
				parser_stmt_error (eng, "NULL address for %I", symbol);
			}
		}
#else
		nel_type* expr_type;

		//expr_type = eval_expr_type(eng, expr->type.expr);	
		expr_type = expr->type.type;

		if(!expr_type) {
			parser_stmt_error(eng, "illegal expr");
		}
		symbol = nel_static_symbol_alloc(eng, NULL, nel_int_type, nel_static_value_alloc(eng, sizeof(int), nel_alignment_of(int)), nel_C_CONST, 0, nel_L_NEL, eng->parser->level);
		*((int*)(symbol->value)) = expr_type->simple.size;
		expr = nel_expr_alloc (eng, nel_O_SYMBOL, symbol);
#endif
		parser_push_expr (eng, expr);
	}
}|
nel_T_TYPEOF unary_expr {
	/*******************************************/
	/* only fill in the expr field of the node */
	/* leave the type field NULL.              */
	/*******************************************/
	nel_debug ({ parser_trace (eng, "unary_expr->nel_T_TYPEOF unary_expr\n\n"); });
	{
		register nel_expr *expr;
		parser_pop_expr (eng, expr);
		expr = nel_expr_alloc (eng, nel_O_TYPEOF, NULL, expr);
		parser_push_expr (eng, expr);
	}
}|
nel_T_TYPEOF nel_T_LPAREN type_name nel_T_RPAREN {
	/*******************************************/
	/* only fill in the type field of the node */
	/* leave the expr field NULL.              */
	/*******************************************/
	nel_debug ({ parser_trace (eng, "unary_expr->nel_T_TYPEOF nel_T_LPAREN type_name nel_T_RPAREN\n\n"); });
	{
		register nel_type *type;
		register nel_expr *expr;
		parser_pop_type (eng, type);
		expr = nel_expr_alloc (eng, nel_O_TYPEOF, type, NULL);
		parser_push_expr (eng, expr);
	}
}|
postfix_expr {
	nel_debug ({ parser_trace (eng, "unary_expr->postfix_expr\n\n"); });
	/****************************/
	/* leave postfix_expr pushed */
	/****************************/
};



/****************************************************************************/
/* postfix_expr->* productions push the expression tree (nel_expr *)      */
/*****************************************************************************/

postfix_expr:
postfix_expr nel_T_LBRACK index_list nel_T_RBRACK {
	nel_debug ({ parser_trace (eng, "postfix_expr->postfix_expr nel_T_LBRACK index_list nel_T_RBRACK\n\n"); });
	{
		register unsigned_int dims;
		register unsigned_int i;
		register nel_expr *expr1;
		register nel_expr *expr2;

		/***********************************************/
		/* pop the number of indeces, and top the      */
		/* expression to the left of the left bracket. */
		/***********************************************/
		parser_pop_integer (eng, dims);
		parser_top_expr (eng, expr1, dims);	/* postfix expr */

#ifdef NEL_PURE_ARRAYS

		/*****************************************/
		/* for ANSI C array addressing, ignore   */
		/* all but the last array index.  the    */
		/* other indeces are the left operand(s) */
		/* of the comma operator.  pop the last  */
		/* index and form the tree, then pop all */
		/* the lhs(s) of the comma operator(s).  */
		/*****************************************/

#if 0
		parser_pop_expr (eng, expr2);
		expr1 = nel_expr_alloc (eng, nel_O_ARRAY_INDEX, expr1, expr2);
		for (i = 1; (i < dims); i++) {
			parser_pop_expr (eng, expr2);
		}
#else

		if(dims == 1 ) {
			parser_pop_expr (eng, expr2);
			expr1 = nel_expr_alloc (eng, nel_O_ARRAY_INDEX, expr1, expr2);
		}else if ( dims == 2 ) {
			register nel_expr *upper, *lower;
			parser_pop_expr(eng, upper);
			parser_pop_expr(eng, lower);
			expr1 = nel_expr_alloc(eng, nel_O_ARRAY_RANGE, expr1, upper, lower);
		}else {
			parser_stmt_error (eng, "array indicator error");
		}

#endif

#else

		/*************************************************/
		/* the extended version of arraysaddressing      */
		/* includes multidimensional arrays in row major */
		/* order, with optional nonzero lower bounds.    */
		/*************************************************/
		for (i = 0; (i < dims); i++) {
			parser_pop_expr (eng, expr2);
			expr1 = nel_expr_alloc (eng, nel_O_ARRAY_INDEX, expr1, expr2);
		}

#endif

		parser_pop_expr (eng, expr2);
		parser_push_expr (eng, expr1);
	}
}|
postfix_expr nel_T_DOT member {
	nel_debug ({ parser_trace (eng, "postfix_expr->postfix_expr nel_T_DOT member\n\n"); });
	{
		register nel_expr *s_u;
		register nel_symbol *member;
		register nel_type *type = NULL;
		register nel_symbol *symbol;

		parser_pop_symbol (eng, member);
		parser_pop_expr (eng, s_u);

		if ( (type = eval_expr_type(eng, s_u)) == NULL ) {
			parser_stmt_error (eng, "no type");
		}
	
		switch(type->simple.type){
			case nel_D_STRUCT:
			case nel_D_UNION:
				if (! s_u_has_member(eng, type, member)) {
					if(type->s_u.tag && type->s_u.tag->name ) {
						parser_stmt_error (eng, 
						"%s %s has no member %s", 
						nel_TC_name(type->simple.type), 
						type->s_u.tag->name, 
							member->name);
					}else {
						parser_stmt_error (eng, 
							"%s has no member %s", 
						nel_TC_name(type->simple.type), 
						member->name);
					}

				}
				break;

			default:
				parser_stmt_error (eng, 
				"%s is not struct or union",
				nel_TC_name(type->simple.type));
				break;

		}

		s_u = nel_expr_alloc (eng, nel_O_MEMBER, s_u, member);
		parser_push_expr (eng, s_u);
	}
}|
postfix_expr nel_T_MINUSRARROW member {
	nel_debug ({ parser_trace (eng, "postfix_expr->postfix_expr nel_T_MINUSRARROW member\n\n"); });
	{
		register nel_expr *s_u;
		register nel_symbol *member;
		register nel_type *type = NULL;
		register nel_symbol *symbol;

		parser_pop_symbol (eng, member);
		parser_pop_expr (eng, s_u);

		if ( (type = eval_expr_type(eng, s_u)) == NULL ) {
			parser_stmt_error (eng, "no type");
		}
	
		if (type->simple.type  == nel_D_POINTER ){

			if ((type = type->pointer.deref_type ) == NULL) {
				parser_stmt_error (eng, "no type");
			}
			
			switch(type->simple.type){
			case nel_D_STRUCT:
			case nel_D_UNION:
				if (! s_u_has_member(eng, type, member)) {
					if(type->s_u.tag && type->s_u.tag->name ) {
						parser_stmt_error (eng, 
						"%s %s has no member %s", 
						nel_TC_name(type->simple.type), 
						type->s_u.tag->name, 
							member->name);
					}else {
						parser_stmt_error (eng, 
							"%s has no member %s", 
						nel_TC_name(type->simple.type), 
						member->name);
					}

				}
				break;

			default:
				parser_stmt_error (eng, 
				"%s is not struct or union",
				nel_TC_name(type->simple.type));
				break;
			}

		}else {
			parser_stmt_error (eng, "%s is not pointer",
			nel_TC_name(type->simple.type));
		}


		s_u = nel_expr_alloc (eng, nel_O_DEREF, s_u);
		s_u = nel_expr_alloc (eng, nel_O_MEMBER, s_u, member);
		parser_push_expr (eng, s_u);

	}
}|
postfix_expr nel_T_DPLUS {
	nel_debug ({ parser_trace (eng, "postfix_expr->postfix_expr nel_T_DPLUS\n\n"); });
	unary_op_acts (nel_O_POST_INC);
}|
postfix_expr nel_T_DMINUS {
	nel_debug ({ parser_trace (eng, "postfix_expr->postfix_expr nel_T_DMINUS\n\n"); });
	unary_op_acts (nel_O_POST_DEC);
}|
postfix_expr nel_T_LPAREN arg_exp_list nel_T_RPAREN {
	nel_debug ({ parser_trace (eng, "postfix_expr->postfix_expr nel_T_LPAREN arg_exp_list nel_T_RPAREN\n\n"); });
	{
		nel_symbol *symbol;
		nel_expr *func;
		nel_expr_list *args;
		int nargs;
		parser_pop_integer (eng, nargs);
		parser_pop_expr_list (eng, args);	/* end of arg list */
		parser_pop_expr_list (eng, args);	/* start of arg list */
		parser_pop_expr (eng, func);
		symbol = func->symbol.symbol;
		
		func = nel_expr_alloc (eng, nel_O_FUNCALL, func, nargs, args);

		if (func->gen.opcode == nel_O_SYMBOL && !parser_lookup_ident(eng, symbol->name)) {
			parser_error(eng, "func (%s) not defined!\n", symbol->name);				
		}
		parser_push_expr (eng, func);
	}
}|
primary_expr {
	nel_debug ({ parser_trace (eng, "postfix_expr->primary_expr\n\n"); });
	/****************************/
	/* leave primary_expr pushed */
	/****************************/
};



index_list:
index_list nel_T_COMMA asgn_expr {
	nel_debug ({ parser_trace (eng, "index_list->index_list nel_T_COMMA asgn_expr\n\n"); });
	/*********************************************************/
	/* pop the expression, then the number of bounds seen so */
	/* far, then push the expression followed by the number  */
	/* of bounds so far + 1.                                 */
	/*********************************************************/
	{
		register int n;
		register nel_expr *expr;
		parser_pop_expr (eng, expr);
		parser_pop_integer (eng, n);
		n++;
		parser_push_expr (eng, expr);
		parser_push_integer (eng, n);
	}
}|
asgn_expr {
	nel_debug ({ parser_trace (eng, "index_list->asgn_expr\n\n"); });
	/*****************************************************/
	/* leave the expression pushed, then push a 1 on top */
	/* of it to indicate that there is 1 index so far.   */
	/*****************************************************/
	{
		parser_push_integer (eng, 1);
	}
};



member:
any_ident {
	nel_debug ({ parser_trace (eng, "member->nel_T_IDENT\n\n"); });
	/*****************************************/
	/* create a symbol as a template for the */
	/* member, and push it on the stack.     */
	/*****************************************/
	{
		register char *name;
		register nel_symbol *symbol;
		parser_pop_name (eng, name);
		symbol  = nel_static_symbol_alloc (eng, name, NULL, NULL, nel_C_NULL, 0, nel_L_NEL, eng->parser->level);
		parser_push_symbol (eng, symbol);
	}
};



/*****************************************************************************/
/* primary->* productions push the expression tree (nel_expr *)          */
/*****************************************************************************/

primary_expr:
ident {
	nel_debug ({ parser_trace (eng, "primary_expr->ident\n\n"); });
	/**********************************************/
	/* create a template symbol to hold the name, */
	/* and an nel_expr structure around it.   */
	/**********************************************/
	{
		register char *name;
		register nel_symbol *symbol;
		register nel_expr *expr;
		parser_pop_name (eng, name);
		if ((symbol = parser_lookup_ident (eng, name)) == NULL ) {
#if 0
			symbol = nel_static_symbol_alloc (eng, name, NULL, NULL, nel_C_NULL, 0, nel_L_NEL, eng->parser->level);
#else
			parser_stmt_error (eng, "undeclaration of %s", name);
#endif
		}

		if(nel_static_C_token(symbol->class) && symbol->_global) {
			symbol = symbol->_global;
		}

		expr = nel_expr_alloc (eng, nel_O_SYMBOL, symbol);
		parser_push_expr (eng, expr);
	}
}|
const
{
	nel_debug ({ parser_trace (eng, "primary_expr->const\n\n"); });
	/*************************/
	/* leave constant pushed */
	/*************************/
}|
nel_T_LPAREN expression nel_T_RPAREN {
	nel_debug ({ parser_trace (eng, "primary_expr->nel_T_LPAREN expression nel_T_RPAREN\n\n"); });
	/***************************/
	/* leave expression pushed */
	/***************************/
};



/*****************************************************************************/
/* const->* productions push the symbol for the constant (nel_symbol *)       */
/*****************************************************************************/

const:
nel_T_C_CHAR
{
	nel_debug ({ parser_trace (eng, "const->nel_T_C_CHAR\n\n"); });
	{
		register char *value;
		register nel_symbol *symbol;
		register nel_expr *expr;
		value = nel_static_value_alloc (eng, sizeof (char), nel_alignment_of (char));
		symbol = nel_static_symbol_alloc (eng, NULL, nel_char_type, value, nel_C_CONST, 0, nel_L_NEL, eng->parser->level);
		*((char *) symbol->value) = eng->parser->char_lval;
		expr = nel_expr_alloc (eng, nel_O_SYMBOL, symbol);
		parser_push_expr (eng, expr);
	}
}|
nel_T_C_DOUBLE {
	nel_debug ({ parser_trace (eng, "const->nel_T_C_DOUBLE\n\n"); });
	{
		register char *value;
		register nel_symbol *symbol;
		register nel_expr *expr;
		value = nel_static_value_alloc (eng, sizeof (double), nel_alignment_of (double));
		symbol = nel_static_symbol_alloc (eng, NULL, nel_double_type, value, nel_C_CONST, 0, nel_L_NEL, eng->parser->level);
		*((double *) symbol->value) = eng->parser->double_lval;
		expr = nel_expr_alloc (eng, nel_O_SYMBOL, symbol);
		parser_push_expr (eng, expr);
	}
}|
nel_T_C_FLOAT {
	nel_debug ({ parser_trace (eng, "const->nel_T_C_FLOAT\n\n"); });
	{
		register char *value;
		register nel_symbol *symbol;
		register nel_expr *expr;
		value = nel_static_value_alloc (eng, sizeof (float), nel_alignment_of (float));
		symbol = nel_static_symbol_alloc (eng, NULL, nel_float_type, value, nel_C_CONST, 0, nel_L_NEL, eng->parser->level);
		*((float *) symbol->value) = eng->parser->float_lval;
		expr = nel_expr_alloc (eng, nel_O_SYMBOL, symbol);
		parser_push_expr (eng, expr);
	}
}|
nel_T_C_LONG_DOUBLE {
	nel_debug ({ parser_trace (eng, "const->nel_T_C_LONG_DOUBLE\n\n"); });
	{
		register char *value;
		register nel_symbol *symbol;
		register nel_expr *expr;
		value = nel_static_value_alloc (eng, sizeof (long_double), nel_alignment_of (long_double));
		symbol = nel_static_symbol_alloc (eng, NULL, nel_long_double_type, value, nel_C_CONST, 0, nel_L_NEL, eng->parser->level);
		*((long_double *) symbol->value) = eng->parser->long_double_lval;
		expr = nel_expr_alloc (eng, nel_O_SYMBOL, symbol);
		parser_push_expr (eng, expr);
	}
}|
nel_T_C_SIGNED_INT {
	nel_debug ({ parser_trace (eng, "const->nel_T_C_SIGNED_INT\n\n"); });
	{
		register char *value;
		register nel_symbol *symbol;
		register nel_expr *expr;
		value = nel_static_value_alloc (eng, sizeof (int), nel_alignment_of (int));
		symbol = nel_static_symbol_alloc (eng, NULL, nel_signed_int_type, value, nel_C_CONST, 0, nel_L_NEL, eng->parser->level);
		*((int *) symbol->value) = eng->parser->int_lval;
		expr = nel_expr_alloc (eng, nel_O_SYMBOL, symbol);
		parser_push_expr (eng, expr);
	}
}|
nel_T_C_SIGNED_LONG_INT {
	nel_debug ({ parser_trace (eng, "const->nel_T_C_SIGNED_LONG_INT\n\n"); });
	{
		register char *value;
		register nel_symbol *symbol;
		register nel_expr *expr;
		value = nel_static_value_alloc (eng, sizeof (long_int), nel_alignment_of (long_int));
		symbol = nel_static_symbol_alloc (eng, NULL, nel_signed_long_int_type, value, nel_C_CONST, 0, nel_L_NEL, eng->parser->level);
		*((long_int *) symbol->value) = eng->parser->long_int_lval;
		expr = nel_expr_alloc (eng, nel_O_SYMBOL, symbol);
		parser_push_expr (eng, expr);
	}
}|
nel_T_REGEXP 
{
	nel_debug ({ parser_trace (eng, "const->nel_T_REGEXP\n\n"); });
	{
		register char *name;
		register int size;
		register nel_type *type;
		register nel_symbol *symbol;
		register nel_expr *expr;
		name = nel_insert_name (eng, eng->parser->text);
		size = strlen (name) + 1;
		type = nel_type_alloc (eng, nel_D_ARRAY, size, nel_alignment_of(char)/* (char *)*/, 1, 0, nel_char_type, nel_int_type, 1, 0, 1, size - 1);
		symbol = nel_static_symbol_alloc (eng, name, type, NULL, nel_C_CONST, 0, nel_L_NEL, 0);
		symbol->value = name;
		expr = nel_expr_alloc (eng, nel_O_SYMBOL, symbol);
		parser_push_expr (eng, expr);
	}
}|
nel_T_C_STRING {
	nel_debug ({ parser_trace (eng, "const->nel_T_C_STRING\n\n"); });
	{
		register char *name;
		register int size;
		register nel_type *type;
		register nel_symbol *symbol;
		register nel_expr *expr;

		name = nel_insert_name (eng, eng->parser->text);
		size = strlen (name) + 1;
		type = nel_type_alloc (eng, nel_D_ARRAY, size, nel_alignment_of(char)/* (char *)*/, 1, 0, nel_char_type, nel_int_type, 1, 0, 1, size - 1);
		symbol = nel_static_symbol_alloc (eng, name, type, NULL, nel_C_CONST, 0, nel_L_NEL, 0);
		symbol->value = name;
		expr = nel_expr_alloc (eng, nel_O_SYMBOL, symbol);
		parser_push_expr (eng, expr);
	}
}|
nel_T_C_UNSIGNED_INT {
	nel_debug ({ parser_trace (eng, "const->nel_T_C_UNSIGNED_INT\n\n"); });
	{
		register char *value;
		register nel_symbol *symbol;
		register nel_expr *expr;
		value = nel_static_value_alloc (eng, sizeof (unsigned_int), nel_alignment_of (unsigned_int));
		symbol = nel_static_symbol_alloc (eng, NULL, nel_unsigned_int_type, value, nel_C_CONST, 0, nel_L_NEL, eng->parser->level);
		*((unsigned_int *) symbol->value) = eng->parser->unsigned_int_lval;
		expr = nel_expr_alloc (eng, nel_O_SYMBOL, symbol);
		parser_push_expr (eng, expr);
	}
}|
nel_T_C_UNSIGNED_LONG_INT {
	nel_debug ({ parser_trace (eng, "const->nel_T_C_UNSIGNED_LONG_INT\n\n"); });
	{
		register char *value;
		register nel_symbol *symbol;
		register nel_expr *expr;
		value = nel_static_value_alloc (eng, sizeof (unsigned_long_int), nel_alignment_of (unsigned_long_int));
		symbol = nel_static_symbol_alloc (eng, NULL, nel_unsigned_long_int_type, value, nel_C_CONST, 0, nel_L_NEL, eng->parser->level);
		*((unsigned_long_int *) symbol->value) = eng->parser->unsigned_long_int_lval;
		expr = nel_expr_alloc (eng, nel_O_SYMBOL, symbol);
		parser_push_expr (eng, expr);
	}
};



int_asgn_expr:
asgn_expr {
	nel_debug ({ parser_trace (eng, "int_asgn_expr->asgn_expr\n\n"); });
	{
		register nel_expr *expr;
		register nel_symbol *symbol;
		//register nel_type *type;
		int value;

		parser_pop_expr (eng, expr);
		symbol = intp_eval_expr_2 (eng, expr);

		nel_debug ({
		if ((symbol == NULL) || (symbol->type == NULL)) {
			parser_fatal_error (eng, "(int_asgn_expr->expression #1): bad symbol\n%1S", symbol);
		}
		});

		if (symbol->value == NULL) {
			if (symbol->name == NULL) {
				parser_stmt_error (eng, "NULL address for integral expression", symbol);
			} else {
				parser_stmt_error (eng, "NULL address for %I", symbol);
			}
		}

		parser_coerce (eng, nel_int_type, (char *) (&value), symbol->type, symbol->value);
		parser_push_integer (eng, value);
	}
};


opt_expr:
expression {
	nel_debug ({ parser_trace (eng, "opt_expr->expression\n\n"); });
	/***************************/
	/* leave expression pushed */
	/***************************/
}|
empty {
	nel_debug ({ parser_trace (eng, "opt_expr->expression\n\n"); });
	/************************/
	/* push NULL expression */
	/************************/
	{
		parser_push_expr (eng, NULL);
	}
};



/*****************************************************************************/
/* arg_exp_list->* productions push the list of actual arguments on the      */
/* semantic stack (nel_expr_list *), the last node of the argument list  */
/* (nel_expr_list *), and the number of arguments (int).                 */
/*****************************************************************************/

arg_exp_list:
arg_exprs {
	nel_debug ({ parser_trace (eng, "arg_exp_list->arg_exprs\n\n"); });
}|
empty {
	/**********************************/
	/* push two NULL pointers and a 0 */
	/**********************************/
	nel_debug ({ parser_trace (eng, "arg_exp_list->empty\n\n"); });
	{
		parser_push_expr (eng, NULL);
		parser_push_expr (eng, NULL);
		parser_push_integer (eng, 0);
	}
};



arg_exprs:
arg_exprs nel_T_COMMA asgn_expr {
	/********************************************************/
	/* pop the next argument, then number of arguments and  */
	/* the last node, append the argument to the list, then */
	/* push the new node and the incremented arg count.     */
	/********************************************************/
	nel_debug ({ parser_trace (eng, "arg_exprs->arg_exprs nel_T_COMMA asgn_expr\n\n"); });
	{
		register nel_expr *arg;
		register int nargs;
		register nel_expr_list *end;
		parser_pop_expr (eng, arg);
		parser_pop_integer (eng, nargs);
		parser_pop_expr_list (eng, end);
		end->next = nel_expr_list_alloc (eng, arg, NULL);
		end = end->next;
		parser_push_expr_list (eng, end);
		parser_push_integer (eng, nargs + 1);
	}
}|
asgn_expr {
	nel_debug ({ parser_trace (eng, "arg_exprs->asgn_expr\n\n"); });
	/*******************************************************/
	/* form a node for the start of the expression list,   */
	/* push it twice (since it is currently the start and  */
	/* end of the list, and push the number of args (1).   */
	/*******************************************************/
	{
		register nel_expr *arg;
		register nel_expr_list *list;
		parser_pop_expr (eng, arg);
		list = nel_expr_list_alloc (eng, arg, NULL);
		parser_push_expr_list (eng, list);
		parser_push_expr_list (eng, list);
		parser_push_integer (eng, 1);
	}
};






/**/
/*****************************************************************************/
/* miscellaneous productions.                                                */
/*****************************************************************************/



any_ident:
ident {
	nel_debug ({ parser_trace (eng, "any_ident->ident\n\n"); });
	/*********************/
	/* leave name pushed */
	/*********************/
}|
typedef_name {
	nel_debug ({ parser_trace (eng, "any_ident->typedef_name\n\n"); });
	/*********************/
	/* leave name pushed */
	/*********************/
};



ident:
nel_T_IDENT {
	nel_debug ({ parser_trace (eng, "ident->nel_T_IDENT\n\n"); });
	/*****************************/
	/* push the name table entry */
	/*****************************/
	{
		char *name = nel_insert_name (eng, eng->parser->text);
		parser_push_name (eng, name);
	}
} |
nel_T_DOLLAR_IDENT{
	nel_debug ({ parser_trace (eng, "ident->nel_T_DOLLAR_IDENT\n\n"); });
	/*****************************/
	/* push the name table entry */
	/*****************************/
	{
		char *name = nel_insert_name (eng, eng->parser->text);
		parser_push_name (eng, name);
	}
}
;



typedef_name:
nel_T_TYPEDEF_NAME {
	nel_debug ({ parser_trace (eng, "ident->nel_T_TYPEDEF_NAME\n\n"); });
	/*****************************/
	/* push the name table entry */
	/*****************************/
	{
		char *name = nel_insert_name (eng, eng->parser->text);
		parser_push_name (eng, name);
	}
};



empty:
{
	nel_debug ({ parser_trace (eng, "empty->\n\n"); });
};





%%

