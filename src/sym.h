
#ifndef SYMBOL_H
#define SYMBOL_H

struct nel_eng;
union nel_EXPR;

//#include "evt.h"
//#include "item.h"
#include "termset.h"
/*****************************symbol stuff**********************************/
/* type definition for compiler module */
//typedef struct nel_comp_type{
//	int t;
//	union nel_TYPE *ref;
//} CType;


/********************************************/
/* symbol class constants.  they begin with */
/* "nel_C_" and are called "nel_C_tokens".    */
/********************************************/
typedef enum nel_C_TOKEN {
        nel_C_NULL = 0,
        nel_C_COMPILED_FUNCTION, 	/* executable code		*/
        nel_C_COMPILED_STATIC_FUNCTION, /* function local to file	*/
        nel_C_NEL_FUNCTION,		/* interpreted code		*/
        nel_C_NEL_STATIC_FUNCTION,	/* interpreted code (file local)*/
        nel_C_INTRINSIC_FUNCTION, 	/* intrinsic function		*/
        nel_C_GLOBAL,			/* global var			*/
        nel_C_STATIC_GLOBAL,		/* static global var (local to file)*/
        nel_C_LOCAL,			/* local var			*/
        nel_C_STATIC_LOCAL,		/* local var, declared static	*/
        nel_C_REGISTER_LOCAL,		/* register local (compiled code only)*/
        				/* "register" ignored for interp code */
        nel_C_FORMAL,			/* formal argument (parameter)	*/
        nel_C_REGISTER_FORMAL,		/* register arg (compiled code only)*/
        				/* "register" ignored for interp code*/
        nel_C_CONST,			/* plain constant		*/
        nel_C_ENUM_CONST,		/* enum constant		*/
        nel_C_RESULT,			/* result of operation or functioncall*/
        nel_C_LABEL,			/* label			*/
        nel_C_TYPE,			/* typedef name, tag, predefined type*/
        nel_C_MEMBER,			/* structure / union member	*/
        nel_C_FILE,			/* file name			*/
        nel_C_LOCATION,			/* ld symbol table entry,extern bit 1*/
        nel_C_STATIC_LOCATION,		/* ld symbol table entry, extern bit 0*/
        nel_C_NAME,			/* name table entry		*/
        nel_C_BAD_SYMBOL,		/* bad symbol - undeclared ident*/
        nel_C_TERMINAL,			/* terminal */
        nel_C_NONTERMINAL,		/* non terminal */
        nel_C_RHS,			/* right part of production */
        nel_C_PRODUCTION,		/* production */
        nel_C_MAX
} nel_C_token;


/*****************************************************************************/
/* we may need to know what source language a given symbol was defined with, */
/* so we use nel_L_tokens, whose names start with "nel_L_", for that purpose.  */
/*****************************************************************************/
typedef enum nel_L_TOKEN {
        nel_L_NULL = 0,
        nel_L_NEL,		/* symbol created by the NEL interpreter*/
        nel_L_ASSEMBLY,		/* the other nel_L_tokens and corresponding*/
        nel_L_C,		/* symbols are generated while scanning the*/
        nel_L_FORTRAN,		/* executable stab, i.e. from compiled code.*/
        nel_L_NAME,		/* except for name table entries*/
        nel_L_MAX
} nel_L_token;


struct nel_EVENT
{
        int pid;		/* wyong, 2004.6.1 */
        int cyclic;		/* */
        TerminalSet first;	/* */
        TerminalSet follow;	/* */

        /* event 's extend field, wyong, 2005.3.23 */
        int state;		/* */
        int deep;		/* */
        int flag;
        //union nel_EXPR *expr;
        //union nel_EXPR *expr2;	/* wyong, 2004.5.23 */
        //struct nel_SYMBOL *at_class;
        //struct nel_SYMBOL *default_class;
        //int nodelay;		/* */
        int isEmptyString;	/* */
        int reachable;		/* */
	int isolate;		/* wyong, 2006.3.9 */
	struct nel_EVENT *next;

        struct nel_SYMBOL *parent;

} 
;
typedef struct nel_EVENT nel_event;


/**********************************************************************/
/* declarations for the symbols.                                      */
/* some compilers treat enumerated types as signed, so we need 6 bits */
/* for the class field, and 4 bits for the source_lang field, instead */
/* of the 5 and 3 bits needed for unsigned enumerated types.          */
/**********************************************************************/
typedef struct nel_SYMBOL
{
        nel_C_token class;		/* symbol class (see above)	 */
        char *name;			/* identifier string 		 */
        int id;				/* wyong, 2004.6.1 */
        union nel_TYPE *type;		/* type descriptor		 */
        char *value;			/* points to memory alloced for  */
        				/* value class nel_C_TERMINAL or */
					/* nel_C_NONTERMINAL should cast */
					/* value to (union nel_EXPR *)   */

        struct nel_SYMBOL_TABLE *table;	/* table symbol is inserted in	*/
	unsigned_int lhs: 1;		/* legal left hand side of assignment*/
	nel_L_token source_lang: 4;	/* source code language		*/
	unsigned_int declared: 1;	/* flag used when parsing declarations*/
	unsigned_int initialized: 1;	/* flag used when parsing initialization, wyong, 2006.4.30 */
	int level: 21;			/* scoping level (can be neg)	*/
        int reg_no;			/* register # (if register symbol)*/

        struct nel_SYMBOL *s_u;		/* if symbol is from a struct/union,*/
        struct nel_MEMBER *member;	/* s_u points to it, and member points*/
        /* to the member in the type desc.*/
        union {
                struct nel_SYMBOL *global;	/* global symbol accroding to 
						this, if current symbol is a 
						static variable */
                struct nel_EVENT  *event;	/* TERMINAL and NONTERMINAL */
        } aux;

#define _pid aux.event->pid
#define _state aux.event->state
#define _deep aux.event->deep
#define _flag aux.event->flag
#define _parent aux.event->parent
#define _first aux.event->first
#define _cyclic aux.event->cyclic
#define _follow  aux.event->follow
#define _isEmptyString aux.event->isEmptyString
#define _reachable aux.event->reachable
#define _isolate aux.event->isolate	//wyong, 2006.3.9 

#define _s_u aux.s_u
#define _member aux.member
#define _global aux.global

        int v;    			/* symbol token */
        int r;    			/* associated register */
        int c;    			/* associated number */

        char *data;			/* for use by application program,*/
        struct nel_SYMBOL *next;	/* used for chaining hash entries, */

}
nel_symbol;


#define MAX_RHS	16
#define NOASSOC 0
#define LEFT    1
#define RIGHT 	2

/***********************************************************/
/* identifiers should be shorter than nel_MAX_SYMBOL_LENGTH */
/* or name buffers will overflow.                          */
/***********************************************************/
#define	nel_MAX_SYMBOL_LENGTH	0x200



/*************************************************************/
/* linked lists are made of the following structures so that */
/* we may find the start of all chunks of memory allocated   */
/* to hold static symbol names at garbage collection time.   */
/* each chunk is of size nel_static_names_max, and it is      */
/* possible that this may be changed dynamically.            */
/*************************************************************/
typedef struct nel_STATIC_NAME_CHUNK
{
        char *start;
        unsigned_int size;
        struct nel_STATIC_NAME_CHUNK *next;
}
nel_static_name_chunk;



/**********************************************/
/* declarations for the static name allocator */
/**********************************************/
extern unsigned_int nel_static_names_max;
extern nel_lock_type nel_static_names_lock;



/************************************************/
/* declarations for the dynamic value allocator */
/************************************************/
extern unsigned_int nel_dyn_values_max;
/*********************************************************/
/* dyn_values_next and dyn_values_end are in nel_eng */
/*********************************************************/



/*************************************************************/
/* linked lists are made of the following structures so that */
/* we may find the start of all chunks of memory allocated   */
/* to hold static symbol values at garbage collection time.  */
/* each chunk is of size nel_static_values_max, and it is     */
/* possible that this may be changed dynamically.            */
/*************************************************************/
typedef struct nel_STATIC_VALUE_CHUNK
{
        char *start;
        unsigned_int size;
        struct nel_STATIC_VALUE_CHUNK *next;
}
nel_static_value_chunk;



/***********************************************/
/* declarations for the static value allocator */
/***********************************************/
extern unsigned_int nel_static_values_max;
extern nel_lock_type nel_static_values_lock;




/**********************/
/* nel_C_tokens macros */
/**********************/
#define nel_constant_C_token(_x) \
	(((_x) == nel_C_CONST)						\
	|| ((_x) == nel_C_ENUM_CONST))

#define nel_static_C_token(_x) \
	(((_x) == nel_C_COMPILED_STATIC_FUNCTION)			\
	|| ((_x) == nel_C_NEL_STATIC_FUNCTION)				\
	|| ((_x) == nel_C_STATIC_GLOBAL)				\
	|| ((_x) == nel_C_STATIC_LOCAL)					\
	|| ((_x) == nel_C_STATIC_LOCATION))

#define nel_compiled_C_token(_x) \
	(((_x) == nel_C_COMPILED_FUNCTION)				\
	|| ((_x) == nel_C_COMPILED_STATIC_FUNCTION))

#define nel_C_token(_x) \
	(((_x) == nel_C_NEL_FUNCTION)					\
	|| ((_x) == nel_C_NEL_STATIC_FUNCTION))





/******************************nel_eng  stuff ********************************/
typedef union nel_STACK {
        struct nel_BLOCK *block;
        nel_C_token C_token;
        nel_D_token D_token;
        int integer;
        struct nel_LIST *list;
        char *name;
        struct nel_MEMBER *member;
        struct nel_STAB_TYPE *stab_type;
        struct nel_SYMBOL *symbol;
        union nel_TYPE *type;

        union nel_EXPR *expr;
        struct nel_EXPR_LIST *expr_list;
        union nel_STMT *stmt;
        int token;
        struct nel_RHS *nel_rhs;
} nel_stack;



/********************************************************************/
/* use alloca () to allocate space for the semantic stack.          */
/* set the pointers to the start and end of the stack.              */
/* decrement the next pointer to just before the actual stack       */
/* space, as a push is # defined to be "increment the stack pointer */
/* and then assign the contents of the slot", not vice-versa.       */
/********************************************************************/
#define nel_stack_alloc(_start,_next,_end,_max)				\
	{								\
	   register unsigned_int __max = (_max);			\
	   nel_alloca ((_start), __max, nel_stack);			\
	   (_next) = (_start);						\
	   (_end) = (_next) + __max;					\
	   (_next)--;							\
	}

#if 0
#define nel_stack_dealloc(_ptr) 					\
	{								\
	   nel_debug ({							\
	      if ((_ptr) == NULL) {					\
	         /*nel_fatal_error (NULL, "(nel_stack_dealloc #1): ptr == NULL");*/ \
	      }								\
	   });								\
	   nel_dealloca ((_ptr));					\
	}
#endif 

#define nel_stack_dealloc(_ptr){ 					\
	   nel_dealloca ((_ptr));					\
	}


/******************************************************/
/* we check the stack for overflow on every push.     */
/* reuse the same error message to cut down on space. */
/******************************************************/
extern unsigned_int nel_semantic_stack_max;
extern char nel_semantic_stack_overflow_message[];









/**************************************************************/
/* since nel_C_tokens and nel_L_tokens are an enumed type, they */
/* must be defined before other header files are included     */
/* (there are no incomplete enum tags in C).                  */
/**************************************************************/



/*************************************************/
/* declarations for the dynamic symbol allocator */
/*************************************************/
extern unsigned_int nel_dyn_symbols_max;
/*******************************************************/
/* dyn_names_next and dyn_names_end are in nel_eng */
/*******************************************************/



/*************************************************************/
/* linked lists are made of the following structures so that */
/* we may find the start of all chunks of memory allocated   */
/* to hold static symbols at garbage collection time.        */
/* each chunk is of size nel_static_symbols_max, and it is    */
/* possible that this may be changed dynamically.            */
/*************************************************************/
typedef struct nel_STATIC_SYMBOL_CHUNK
{
        struct nel_SYMBOL *start;
        unsigned_int size;
        struct nel_STATIC_SYMBOL_CHUNK *next;
}
nel_static_symbol_chunk;



/************************************************/
/* declarations for the static symbol allocator */
/************************************************/
extern unsigned_int nel_static_symbols_max;
extern nel_lock_type nel_static_symbols_lock;



/************************************************************/
/* declarations for symbols for the integer values 0 and 1, */
/* which are boolean return values.                         */
/************************************************************/
extern struct nel_SYMBOL *nel_zero_symbol;
extern struct nel_SYMBOL *nel_one_symbol;



/***************************************************************************/
/* use the following macro to allocate space for the dynamic name, symbol, */
/* and value allocators. use nel_alloca () to push the space on the stack   */
/* so it will automatically be reclaimed when exiting the function.        */
/***************************************************************************/
#define nel_dyn_allocator_alloc(_start,_next,_end,_max,_type) 		\
{									\
	register unsigned_int __max = (_max);				\
	nel_alloca ((_start), __max, _type);				\
	(_next) = (_start);						\
	(_end) = (_start) + __max;					\
}

#define nel_dyn_allocator_dealloc(_ptr) 				\
{									\
	nel_debug ({							\
	if ((_ptr) == NULL) {						\
		/*nel_fatal_error (NULL, "(nel_dyn_allocator_dealloc #1): ptr == NULL");*/ \
	}								\
	});								\
	nel_dealloca ((_ptr));						\
}



/************************************************/
/* declarations for the symbol table strucutres */
/************************************************/



/*******************************************************/
/* each table has a pointer to an array of hash chains */
/*******************************************************/
typedef struct nel_SYMBOL_CHAIN
{
        struct nel_SYMBOL *symbols;
        nel_lock_type lock;
}
nel_symbol_chain;



/******************************************************/
/* the table also needs to store the number of chains */
/******************************************************/
typedef struct nel_SYMBOL_TABLE {
	unsigned_int max;
	struct nel_SYMBOL_CHAIN *chains;
}
nel_symbol_table;



/*******************************************************************/
/* use the following macros to allocate space for the hash tables, */
/* and to initialize all their chains to NULL.                     */
/*******************************************************************/
#define nel_dyn_hash_table_alloc(_table,_max) 				\
{									\
	register unsigned_int __max = (_max);				\
	register unsigned_int __i;					\
	register nel_symbol_chain *__scan;				\
	nel_alloca ((_table), 1, nel_symbol_table);			\
	nel_alloca ((_table)->chains, __max, nel_symbol_chain);		\
	for (__i = 0, __scan = (_table)->chains; (__i < __max); __i++, __scan++) { \
		__scan->symbols = NULL;					\
		__scan->lock = nel_unlocked_state;			\
	}								\
	(_table)->max = __max;						\
}


#define nel_dyn_hash_table_dealloc(_table) 				\
{									\
	nel_debug ({							\
	if ((_table) == NULL) {						\
		/*nel_fatal_error (NULL, "(nel_dyn_hash_table_dealloc #1): table == NULL"); */ \
	}								\
	});								\
	nel_dealloca ((_table)->chains);				\
	nel_dealloca ((_table));					\
}

#define nel_static_hash_table_alloc(_table,_max) 			\
{									\
	register unsigned_int __max = (_max);				\
	register unsigned_int __i;					\
	register nel_symbol_chain *__scan;				\
	nel_malloc ((_table), 1, nel_symbol_table);			\
	nel_malloc ((_table)->chains, __max, nel_symbol_chain);		\
	for (__i = 0, __scan = (_table)->chains; (__i < __max); __i++, __scan++) { \
		__scan->symbols = NULL;					\
		__scan->lock = nel_unlocked_state;			\
	}								\
	(_table)->max = __max;						\
}

//added by zhangbin, 2006-7-19
#define nel_static_hash_table_dealloc(_table) nel_dyn_hash_table_dealloc(_table)
//end


/**************************************************************************/
/* for every identifier encountered, a premanent symbol is created in the */
/* static name hash table.  all subsequent instances of that identifier   */
/* use the same string.                                                   */
/**************************************************************************/
#define nel_static_name_hash_max	0x2000
//extern struct nel_SYMBOL_TABLE *nel_static_name_hash;



/********************************************************************/
/* the following macros for lookup and insertion of symbols require */
/* that the nel_eng be non-NULL.  call the appropriate routines */
/* directly when using nel's symbol tables without the interpreter   */
/* or stab scanner.                                                 */
/********************************************************************/



/******************************************/
/* declarations for the ident hash tables */
/******************************************/
#define nel_dyn_ident_hash_max		0x0800
/************************************/
/* dyn_ident_hash is in nel_eng */
/************************************/
#define nel_static_ident_hash_max	0x1000
//extern struct nel_SYMBOL_TABLE *nel_static_ident_hash;


#if 0
/********************************************************/
/* search for an identifier in the location hash tables */
/* if there is no other identifier by that name.        */
/********************************************************/
/*
#define nel_lookup_ident(_eng,_name)	\
	nel_lookup_symbol ((_name), (_eng)->gen.dyn_ident_hash, (_eng)->gen.nel_static_ident_hash, (_eng)->gen.dyn_location_hash, (_eng)->gen.nel_static_location_hash, NULL) \
*/

#define nel_insert_ident(_eng,_symbol) \
	((((_symbol)->level <= 0) && ((_symbol)->class != nel_C_STATIC_GLOBAL) \
	     && ((_symbol)->class != nel_C_COMPILED_STATIC_FUNCTION)	\
	     && ((_symbol)->class != nel_C_NEL_STATIC_FUNCTION)) ?	\
	   nel_insert_symbol ((_eng), (_symbol), (_eng)->gen.nel_static_ident_hash) :		\
	   nel_insert_symbol ((_eng), (_symbol), (_eng)->gen.dyn_ident_hash)	\
	)

#define nel_remove_ident(_eng,_symbol) \
	nel_remove_symbol (_eng, (_symbol))

#endif

/*********************************************/
/* declarations for the location hash tables */
/*********************************************/
#define nel_dyn_location_hash_max	0x0100
/***************************************/
/* dyn_location_hash is in nel_eng */
/***************************************/
#define nel_static_location_hash_max	0x1000
//extern struct nel_SYMBOL_TABLE *nel_static_location_hash;



/***********************************************************/
/* when searching for a location (ld symbol table entry),  */
/* check only the location hash tables, and skip the ident */
/* hash tables - they may have a symbol by the same name,  */
/* masking the entry.                                      */
/***********************************************************/
#define nel_lookup_location(_eng,_name)	\
	nel_lookup_symbol ((_name), (_eng)->gen.dyn_location_hash,(_eng)->gen.nel_static_location_hash, NULL)


#define nel_insert_location(_eng,_symbol) \
	((((_symbol)->level <= 0) && ((_symbol)->class != nel_C_STATIC_LOCATION)) ? \
	   nel_insert_symbol ((_eng),(_symbol), (_eng)->gen.nel_static_location_hash) :	\
	   nel_insert_symbol ((_eng),(_symbol), (_eng)->gen.dyn_location_hash)	\
	)

#define nel_remove_location(_eng,_symbol) \
	nel_remove_symbol (_eng, (_symbol))


/****************************************/
/* declarations for the tag hash tables */
/****************************************/
#define nel_dyn_tag_hash_max		0x0080
/**********************************/
/* dyn_tag_hash is in nel_eng  */
/**********************************/
#define nel_static_tag_hash_max		0x0100
//extern struct nel_SYMBOL_TABLE *nel_static_tag_hash;


#define nel_lookup_tag(_eng,_name) \
	nel_lookup_symbol ((_name), (_eng)->gen.dyn_tag_hash, (_eng)->gen.nel_static_tag_hash, NULL)


#define nel_insert_tag(_eng,_symbol) \
	(((_symbol)->level <= 0) ?					\
	   nel_insert_symbol ((_eng),(_symbol), (_eng)->gen.nel_static_tag_hash) :		\
	   nel_insert_symbol ((_eng), (_symbol), (_eng)->gen.dyn_tag_hash)	\
	)

#define nel_remove_tag(_eng,_symbol) \
	nel_remove_symbol (_eng, (_symbol))


/*****************************************/
/* declarations for the label hash table */
/*****************************************/
#define nel_dyn_label_hash_max		0x0080
/************************************/
/* dyn_label_hash is in nel_eng  */
/************************************/


#define nel_lookup_label(_eng,_name) \
	nel_lookup_symbol ((_name), (_eng)->gen.dyn_label_hash, NULL)

#define nel_insert_label(_eng,_symbol) \
	nel_insert_symbol ((_eng),(_symbol), (_eng)->gen.dyn_label_hash)

#define nel_remove_label(_eng,_symbol) \
	nel_remove_symbol (_eng, (_symbol))



/*******************************************/
/* we also keep a hash table of file names */
/*******************************************/
#define nel_static_file_hash_max	0x0100
extern struct nel_SYMBOL_TABLE *nel_static_file_hash;


#define nel_lookup_file(_eng,_name) \
	nel_lookup_symbol ((_name), (_eng)->gen.nel_static_file_hash, NULL)

#define nel_insert_file(_eng,_symbol) \
	nel_insert_symbol ((_eng), (_symbol), (_eng)->gen.nel_static_file_hash)

#define nel_remove_file(_eng,_symbol) \
	nel_remove_symbol (_eng, _symbol)



//struct nel_eng;
/***********************************************************/
/* declarations for the routines defined in "nel_symbols.c" */
/***********************************************************/
extern nel_symbol *nel_symbol_dup(struct nel_eng *, nel_symbol *);
extern nel_symbol *nel_lookup_ident(struct nel_eng *, char *);
extern char *nel_static_name_alloc (struct nel_eng *, char *);
extern char *nel_dyn_value_alloc (struct nel_eng *, register unsigned_int, register unsigned_int);
extern char *nel_static_value_alloc (struct nel_eng *, register unsigned_int, register unsigned_int);
extern char *nel_C_name (register nel_C_token);
extern char *nel_L_name (register nel_L_token);
extern struct nel_SYMBOL *nel_dyn_symbol_alloc (struct nel_eng *, char *, union nel_TYPE *, char *, nel_C_token, unsigned_int, nel_L_token, int);
extern struct nel_SYMBOL *nel_static_symbol_alloc (struct nel_eng *, char *, union nel_TYPE *, char *, nel_C_token, unsigned_int, nel_L_token, int);
extern void nel_static_symbol_dealloc (struct nel_eng *, register struct nel_SYMBOL *);
extern void nel_print_symbol_name (FILE *, register struct nel_SYMBOL *, int);
extern void nel_print_symbol (FILE *, register struct nel_SYMBOL *, int);
extern unsigned_int nel_hash_symbol (register char *, unsigned_int);
extern struct nel_SYMBOL *nel_lookup_symbol (char *, ...);
extern void nel_list_symbols (struct nel_eng *, FILE *, struct nel_SYMBOL_TABLE *);
extern struct nel_SYMBOL *nel_insert_symbol (struct nel_eng *, register struct nel_SYMBOL *, struct nel_SYMBOL_TABLE *);
extern struct nel_SYMBOL *nel_remove_symbol (struct nel_eng *eng, register struct nel_SYMBOL *);
extern void nel_purge_table (struct nel_eng *eng, register int, struct nel_SYMBOL_TABLE *);
extern void nel_remove_NULL_values (struct nel_eng *eng, struct nel_SYMBOL_TABLE *, void (*)(struct nel_eng *, register nel_symbol *));
extern char *nel_insert_name (struct nel_eng *eng, register char *);
extern void nel_dyn_stack_dealloc (struct nel_eng *, char *, struct nel_SYMBOL *);
extern void nel_symbol_init (struct nel_eng *);
extern void emit_symbol(FILE *fp, nel_symbol *symbol);
extern int nel_symbol_diff(nel_symbol *s1, nel_symbol *s2);

//added by zhangbin, 2006-7-19
void static_symbol_dealloc(struct nel_eng *eng);
void static_value_dealloc(struct nel_eng *eng);
void static_name_dealloc(struct nel_eng *eng);
//end

//-----------------grammar parse tree--------------------------------
extern struct nel_LIST *c_include_list ;
extern struct nel_LIST *last_c_include;
extern int c_include_num ;

// the special terminal for the empty string; does not appear in the
// list of nonterminals or terminals for a grammar, but can be
// referenced by productions, etc.; the decision to explicitly have
// such a symbol, instead of letting it always be implicit, is
// motivated by things like the derivability relation, where it's
// nice to treat empty like any other symbol



#endif
