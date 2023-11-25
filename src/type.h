
/*****************************************************************************/
/* This file, "nel_types.h", contains the definitions for the data type       */
/* descriptors used by the application executive, and declarations for the   */
/* routines, any #define staements, and extern declarations for any global   */
/* variables that are defined in "nel_types.c".                               */
/*****************************************************************************/



#ifndef _NEL_TYPES_
#define _NEL_TYPES_


/*************************************/
/* support for new ansi C data types */
/*************************************/
/* char = char */
/* double = double */
/* float = float */
/* int = int */
/* long = long */
#define long_double		long double
#define long_float		double
#define long_int		long int
/* short = short */
#define short_int		short int
/* signed = signed */
#define signed_char		signed char
#define signed_int		signed int
#define signed_long		signed long
#define signed_long_int		signed long int
#define signed_short		signed short
#define signed_short_int	signed short int
/* unsigned = unsigned */
#define unsigned_char		unsigned char
#define unsigned_int		unsigned int
#define unsigned_long		unsigned long
#define unsigned_long_int	unsigned long int
#define unsigned_short		unsigned short
#define unsigned_short_int	unsigned short int
/* void = void */


/*****************************************************************************/
/* nel_largest_type has the size of the largest simple type for this machine  */
/*****************************************************************************/
//typedef long_double nel_largest_type;



/************************************************************************/
/* simple types longer than nel_MAX_ALIGNMENT are allocated on an        */
/* nel_MAX_ALIGNMENT byte boundary.  types shorter than nel_MAX_ALIGNMENT */
/* are allocated on a boundary that is the size of the type.            */
/* most machines align 8 byte doubles on 8 byte boundaries, and the     */
/* CRAY_YMP aligns 16 bytes doubles on a word (8 byte) boundary).       */
/************************************************************************/
#define nel_MAX_ALIGNMENT	8



/****************************************************/
/* use the following macro only with simple types   */
/****************************************************/
#define nel_alignment_of(_type) \
	((sizeof (_type) <= nel_MAX_ALIGNMENT) ? sizeof (_type) : nel_MAX_ALIGNMENT)



/*****************************************************/
/* nel_align () is written to not cause overflows if  */
/* (ptr => 0) and ( ptr <= LONG_MAX - alignment + 1) */
/*****************************************************/
#define nel_align_ptr(_ptr,_alignment) \
	(((char *) (_ptr)) + ((_alignment) - (((((unsigned_long_int) (_ptr)) - 1) % (_alignment)) + 1)))



/*************************************************************/
/* use nel_align_offset () to align the offset from a memory  */
/* address offset (an int) instead of the address itself.    */
/*************************************************************/
#define nel_align_offset_ex(_offset,_alignment) \
	((_offset) + ((_alignment) - ((((_offset) - 1) % (_alignment)) + 1)))

#define nel_align_offset(_offset, _alignment)	\
	(_alignment == 0) ? (_offset) : nel_align_offset_ex(_offset, _alignment)


#define MAXPATHLEN 	256

/**************************************************************/
/* nel_SMALLEST_MEM is the smallest chunk of memory that can   */
/* be read/written to/from memory.  CRAY_YMP has 8 byte words */
/* and is word addressable.  all other machines ported to so  */
/* far support byte loads and stores.                         */
/**************************************************************/
#define nel_SMALLEST_MEM		1



/*******************************************************************/
/* include <stdarg.h> for ansi standard for passing var # of args. */
/* traditional compilers #include <varargs.h>.                     */
/*******************************************************************/
#include <stdarg.h>

/**************************************************************************/
/* ansi standard uses "..." instead of "va_alist" in formal argument list */
/**************************************************************************/
#define va_alist	...




/***********************************/
/* maximum/minimum integral values */
/***********************************/
#include <limits.h>


/*****************************************************************************/
/* On parallel architectures, #define nel_lock() and nel_unlock() to the       */
/* appropriate routines, and typedef nel_lock_type to a lock data strucuture. */
/*****************************************************************************/

/***************************************************************/
/* sequential architecutre - define the routines to do nothing */
/***************************************************************/
#define nel_lock(x)
#define nel_unlock(x)
#define nel_unlocked_state	('\0')
typedef char nel_lock_type;



/*****************************************************************/
/* discriminator constants for the type descriptors.  they begin */
/* with "nel_D_" for Data type, and are called "nel_D_tokens".     */
/*****************************************************************/
typedef enum nel_D_TOKEN {
        nel_D_NULL = 0,
        nel_D_ARRAY,
        nel_D_CHAR,
        nel_D_COMMON,
        nel_D_COMPLEX,		/* extension: basic complex type	*/
        nel_D_COMPLEX_DOUBLE,	/* extension: ext. precision complex	*/
        nel_D_COMPLEX_FLOAT,	/* extension: same as complex		*/
        nel_D_DOUBLE,
        nel_D_ENUM,
        nel_D_ENUM_TAG,		/* tags have type descriptors		*/
        nel_D_FILE,		/* file names have type descriptors	*/
        nel_D_FLOAT,
        nel_D_FUNCTION,
        nel_D_INT,
        nel_D_LABEL,		/* goto target				*/
        nel_D_LOCATION,		/* identifier from ld symbol table	*/
        nel_D_LONG,
        nel_D_LONG_COMPLEX,	/* extension: same as complex double	*/
        nel_D_LONG_COMPLEX_DOUBLE,/* extension: longest complex type	*/
        nel_D_LONG_COMPLEX_FLOAT,/* extension: same as complex double	*/
        nel_D_LONG_DOUBLE,
        nel_D_LONG_FLOAT,	/* same as double			*/
        nel_D_LONG_INT,
        nel_D_LONG_LONG,
        nel_D_POINTER,
        nel_D_SHORT,
        nel_D_SHORT_INT,
        nel_D_SIGNED,
        nel_D_SIGNED_CHAR,
        nel_D_SIGNED_INT,
        nel_D_SIGNED_LONG,
        nel_D_SIGNED_LONG_INT,
        nel_D_SIGNED_LONG_LONG,	
        nel_D_SIGNED_SHORT,
        nel_D_SIGNED_SHORT_INT,
        nel_D_STAB_UNDEF,	/* placeholder until type is defined	*/
        nel_D_STRUCT,
        nel_D_STRUCT_TAG,
        nel_D_TYPE_NAME,	/* int, long, etc. have symbols, too	*/
        nel_D_TYPEDEF_NAME,
        nel_D_UNION,
        nel_D_UNION_TAG,
        nel_D_UNSIGNED,
        nel_D_UNSIGNED_CHAR,
        nel_D_UNSIGNED_INT,
        nel_D_UNSIGNED_LONG,
        nel_D_UNSIGNED_LONG_INT,
        nel_D_UNSIGNED_LONG_LONG,
        nel_D_UNSIGNED_SHORT,
        nel_D_UNSIGNED_SHORT_INT,
        nel_D_VOID,
        nel_D_TERMINAL,			/* terminal */
        nel_D_NONTERMINAL,		/* nonterminal */
        nel_D_EVENT,
        nel_D_RHS,			/* right side part of production */
        nel_D_PRODUCTION,		/* production */
        nel_D_MAX
} nel_D_token;

#define nel_long_long_D_token(_x) \
	(((_x) == nel_D_LONG_LONG)					\
	|| ((_x) == nel_D_SIGNED_LONG_LONG)				\
	|| ((_x) == nel_D_UNSIGNED_LONG_LONG))

#define nel_unsigned_D_token(_x) \
	(((_x) == nel_D_UNSIGNED)					\
	|| ((_x) == nel_D_UNSIGNED_CHAR)				\
	|| ((_x) == nel_D_UNSIGNED_INT)					\
	|| ((_x) == nel_D_UNSIGNED_LONG)				\
	|| ((_x) == nel_D_UNSIGNED_LONG_INT)				\
	|| ((_x) == nel_D_UNSIGNED_SHORT)				\
	|| ((_x) == nel_D_UNSIGNED_SHORT_INT))

/*********************/
/* nel_D_token macros */
/*********************/
#define nel_integral_D_token(_x) \
	(((_x) == nel_D_CHAR)						\
	|| ((_x) == nel_D_ENUM)						\
	|| ((_x) == nel_D_INT)						\
	|| ((_x) == nel_D_LONG)						\
	|| ((_x) == nel_D_LONG_INT)					\
	|| ((_x) == nel_D_SHORT)					\
	|| ((_x) == nel_D_SHORT_INT)					\
	|| ((_x) == nel_D_SIGNED)					\
	|| ((_x) == nel_D_SIGNED_CHAR)					\
	|| ((_x) == nel_D_SIGNED_INT)					\
	|| ((_x) == nel_D_SIGNED_LONG)					\
	|| ((_x) == nel_D_SIGNED_LONG_INT)				\
	|| ((_x) == nel_D_SIGNED_SHORT)					\
	|| ((_x) == nel_D_SIGNED_SHORT_INT)				\
	|| ((_x) == nel_D_UNSIGNED)					\
	|| ((_x) == nel_D_UNSIGNED_CHAR)				\
	|| ((_x) == nel_D_UNSIGNED_INT)					\
	|| ((_x) == nel_D_UNSIGNED_LONG)				\
	|| ((_x) == nel_D_UNSIGNED_LONG_INT)				\
	|| ((_x) == nel_D_UNSIGNED_SHORT)				\
	|| ((_x) == nel_D_UNSIGNED_SHORT_INT))

#define nel_promoted_int_D_token(_x) \
	(((_x) == nel_D_ENUM)						\
	|| ((_x) == nel_D_INT)						\
	|| ((_x) == nel_D_LONG)						\
	|| ((_x) == nel_D_LONG_INT)					\
	|| ((_x) == nel_D_SIGNED)					\
	|| ((_x) == nel_D_SIGNED_INT)					\
	|| ((_x) == nel_D_SIGNED_LONG)					\
	|| ((_x) == nel_D_SIGNED_LONG_INT)				\
	|| ((_x) == nel_D_UNSIGNED)					\
	|| ((_x) == nel_D_UNSIGNED_INT)					\
	|| ((_x) == nel_D_UNSIGNED_LONG)				\
	|| ((_x) == nel_D_UNSIGNED_LONG_INT))

#define nel_floating_D_token(_x) \
	(((_x) == nel_D_COMPLEX)					\
	|| ((_x) == nel_D_COMPLEX_DOUBLE)				\
	|| ((_x) == nel_D_COMPLEX_FLOAT)				\
	|| ((_x) == nel_D_DOUBLE)					\
	|| ((_x) == nel_D_FLOAT)					\
	|| ((_x) == nel_D_LONG_COMPLEX)					\
	|| ((_x) == nel_D_LONG_COMPLEX_DOUBLE)				\
	|| ((_x) == nel_D_LONG_COMPLEX_FLOAT)				\
	|| ((_x) == nel_D_LONG_DOUBLE)					\
	|| ((_x) == nel_D_LONG_FLOAT))

#define nel_complex_D_token(_x) \
	(((_x) == nel_D_COMPLEX)					\
	|| ((_x) == nel_D_COMPLEX_DOUBLE)				\
	|| ((_x) == nel_D_COMPLEX_FLOAT)				\
	|| ((_x) == nel_D_LONG_COMPLEX)					\
	|| ((_x) == nel_D_LONG_COMPLEX_DOUBLE)				\
	|| ((_x) == nel_D_LONG_COMPLEX_FLOAT))

#define nel_numerical_D_token(_x) \
	(nel_integral_D_token (_x)					\
	|| nel_floating_D_token (_x))

#define nel_pointer_D_token(_x) \
	(((_x) == nel_D_ARRAY)						\
	|| ((_x) == nel_D_COMMON)					\
	|| ((_x) == nel_D_FUNCTION)					\
	|| ((_x) == nel_D_LOCATION)					\
	|| ((_x) == nel_D_POINTER))

#define nel_s_u_D_token(_x) \
	(((_x) == nel_D_STRUCT)						\
	|| ((_x) == nel_D_UNION))

#define nel_tag_D_token(_x) \
	(((_x) == nel_D_ENUM_TAG)					\
	|| ((_x) == nel_D_STRUCT_TAG)					\
	|| ((_x) == nel_D_UNION_TAG))

/************************************/
/* is a var of this type a legal    */
/* left hand side of an assignment? */
/************************************/
#define nel_lhs_D_token(_x) \
	(nel_numerical_D_token (_x)					\
	|| ((_x) == nel_D_POINTER)					\
	|| ((_x) == nel_D_STRUCT)					\
	|| ((_x) == nel_D_UNION))



/******************************************************/
/* since nel_D_tokens are an enumed type, they must be */
/* defined before other header files are included     */
/* (there are no incomplete enum tags in C).          */
/******************************************************/
struct nel_eng;
struct nel_SYMBOL;
union nel_STMT;


/****************************************************/
/* the union of different type descriptors.         */
/* some compilers treat enumerated types as signed, */
/* so we need 7 bits for the type field, and 3 bits */
/* for the qual field.                              */
/****************************************************/

typedef union nel_TYPE {

        /**********************************************************************/
        /* the simple types, common blocks, and location types (ld entries). */
        /* their fields are common to all variants.                          */
        /**********************************************************************/
        struct {
		nel_D_token type:7;		/* the discriminator field*/
		unsigned_int _const:1;		/* declared const	*/
		unsigned_int _volatile:1;	/* declared volatile	*/
		unsigned_int garbage:1;		/* used for garbage collection*/
		unsigned_int traversed:1;	/* used to keep routines from*/
                /* recursing infinetely			*/
                unsigned_int size;		/* size in bytes 	*/
                unsigned_int alignment; 	/* n-byte bndry for allocation*/

		union nel_TYPE *next ; 	
        }
        simple;


        /*************************************************************/
        /* array-valued types.  for open-ended arrays, known_lb or   */
        /* known_ub is zero, the corresponding symbol entry is NULL. */
        /*************************************************************/
        struct {
		nel_D_token type:7;
		unsigned_int _const:1;
		unsigned_int _volatile:1;
		unsigned_int garbage:1;
		unsigned_int traversed:1;
                unsigned_int size;
                unsigned_int alignment;
		union nel_TYPE *next ; 	

		unsigned_int known_lb:1;	/* true if we integral value for bound	*/
		unsigned_int known_ub:1;	/* is evaluated.	*/
                union nel_TYPE *base_type;	/* type descriptor for element's type	*/
                union nel_TYPE *index_type;	/* type descriptor for indeces*/
                union array_bound {
                        int value;

                        /******************************************************/
                        /* if we don't include the interpreter in the library */
                        /* the expression bounds must be evaluated to ints or */
                        /* stored in symbols.  if the interpreter is included */
                        /* in the lib, arbitrary expressions are allowed.     */
                        /******************************************************/
			union nel_EXPR *expr;

                } lb, ub;
        int val_len;
		}
        array;


        /****************/
        /* Enumerations */
        /****************/
        struct {
		nel_D_token type:7;
		unsigned_int _const:1;
		unsigned_int _volatile:1;
		unsigned_int garbage:1;
		unsigned_int traversed:1;
                unsigned_int size;
                unsigned_int alignment;
		union nel_TYPE *next ; 

                struct nel_SYMBOL *tag;	/* symbol table entry for tag		*/
                unsigned_int nconsts;	/* #of constants in this enumed type	*/
                struct nel_LIST *consts;	/* list of structs for each const	*/
        }
        enumed;


        /*****************************************************/
        /* source file name - either compiled or interpreted */
        /*****************************************************/
        struct {
		nel_D_token type:7;
		unsigned_int _const:1;
		unsigned_int _volatile:1;
		unsigned_int garbage:1;
		unsigned_int traversed:1;
                unsigned_int size;
                unsigned_int alignment;
		union nel_TYPE *next ; 

                struct nel_STAB_TYPE *stab_types;	/* list of type #'s		*/
                struct nel_LIST *routines;		/* list of routines		*/
                struct nel_LIST *static_globals;	/* idents local to comp unit	*/
                struct nel_LIST *static_locations;	/* ld "locations" local to unit	*/
                struct nel_LIST *includes;		/* list of #included files	*/
                struct nel_LINE *lines;		/* list of line #'s/addresses	*/
        }
        file;


        /**************/
        /* functions. */
        /**************/
        struct {
		nel_D_token type:7;
		unsigned_int _const:1;
		unsigned_int _volatile:1;
		unsigned_int garbage:1;
		unsigned_int traversed:1;
                unsigned_int size;
                unsigned_int alignment;
		union nel_TYPE *next ; 	

		unsigned_int new_style:1;
		unsigned_int var_args:1;	/* var #of args ? (also 1 if old-style)	*/
                union nel_TYPE *return_type; /* function's return type		*/
                struct nel_LIST *args;	/* list of structs for each arg		*/
                struct nel_BLOCK *blocks;	/* lists of structs for each block	*/
                /* (compiled functions only)		*/
                struct nel_SYMBOL *file;	/* symbol for file function appears in	*/

                unsigned_int key_nums; 	
                struct nel_SYMBOL *prev_hander;	/* */
                struct nel_SYMBOL *post_hander;	/* */

        }
        function;


        /*********************************/
        /* pointers to other data types. */
        /*********************************/
        struct {
		nel_D_token type:7;
		unsigned_int _const:1;
		unsigned_int _volatile:1;
		unsigned_int garbage:1;
		unsigned_int traversed:1;
                unsigned_int size;
                unsigned_int alignment;
		union nel_TYPE *next ; 	

                union nel_TYPE *deref_type; /* type desc for dereferenced ptr	*/
        }
        pointer;


        /*********************************************************************/
        /* when scanning the symbol table, we may encounter references to    */
        /* types that are not yet defined and entered into the symbol table. */
        /* for such types, a stab_undef type descriptor is allocated as a    */
        /* placeholder for the type.  when a type is entered into the symbol */
        /* table, we traverse the type descriptor, and replace all the       */
        /* the stab_undef type descriptors with type->stab_type->type, which */
        /* should be defined by such time.                                   */
        /*********************************************************************/
        struct {
		nel_D_token type:7;
		unsigned_int _const:1;
		unsigned_int _volatile:1;
		unsigned_int garbage:1;
		unsigned_int traversed:1;
                unsigned_int size;
                unsigned_int alignment;
		union nel_TYPE *next ; 	

                struct nel_STAB_TYPE *stab_type;/* nel_stab_type_hash table	entry	*/
        }
        stab_undef;


        /**************************/
        /* structures and unions. */
        /**************************/
        struct {
		nel_D_token type:7;
		unsigned_int _const:1;
		unsigned_int _volatile:1;
		unsigned_int garbage:1;
		unsigned_int traversed:1;
                unsigned_int size;
                unsigned_int alignment;
		union nel_TYPE *next ; 	

                struct nel_SYMBOL *tag;	/* symbol table entry for tag		*/
                struct nel_MEMBER *members; /* list of structs for each member	*/
        }
        s_u;


        /*************************************************************************/
        /* structure, union, and enum tags need a type descriptor.  the symbol   */
        /* table entry (in the tag table) for a tag name will have these         */
        /* structures for the symbol's type descriptor.  the symbol's value      */
        /* member, as well as the descriptor member of this structure, will      */
        /* point to the type descriptor that results from saying "struct <tag>". */
        /*************************************************************************/
        struct {
		nel_D_token type:7;
		unsigned_int _const:1;
		unsigned_int _volatile:1;
		unsigned_int garbage:1;
		unsigned_int traversed:1;
                unsigned_int size;
                unsigned_int alignment;
		union nel_TYPE *next ; 

                union nel_TYPE *descriptor;/* type descriptor this is a name for	*/
        }
        tag_name;


        /**********************************************************************/
        /* typedef names and predefined type names (int, unsigned int, etc.)  */
        /* also need a type descriptor.  the symbol table entry (in the ident */
        /* table) for a typedef name will have these structures for the       */
        /* symbol's type descriptor.  the symbol's value member, as well as   */
        /* the descriptor member of this structure, will point to the type    */
        /* descriptor that results from saying "<typedef_name>".  predefined  */
        /* C and fortran types have the discriminator nel_D_TYPE_NAME, and     */
        /* use this structure.  it is necessary to have symbol table entries  */
        /* for the predefiend types when scanning the stab table in the       */
        /* executable code.                                                   */
        /**********************************************************************/
        struct {
		nel_D_token type:7;
		unsigned_int _const:1;
		unsigned_int _volatile:1;
		unsigned_int garbage:1;
		unsigned_int traversed:1;
                unsigned_int size;
                unsigned_int alignment;
		union nel_TYPE *next ; 

                union nel_TYPE *descriptor;/* type descriptor this is a name for	*/
        }
        typedef_name;


        struct {
		nel_D_token type:7;
		unsigned_int _const:1;
		unsigned_int _volatile:1;
		unsigned_int garbage:1;
		unsigned_int traversed:1;
                unsigned_int size;
                unsigned_int alignment;
		union nel_TYPE *next ; 

                int token_type;			/* nel_D_TERMINAL, nel_D_NONTERMINAL */
                int prec;			/* event 's precedencei, no use */
                int assoc;			/* event 's associativity, no use */
                struct nel_SYMBOL *init_hander;	/*init function defined by user,
						only $0 was supported now. */

                struct nel_SYMBOL *free_hander;	/* free function defined by user*/

                //struct nel_SYMBOL *default_class;	/* default classifier for this event */
                struct nel_SYMBOL *at_class;	/* "->" classifier for this event */
                //struct nel_SYMBOL *prev_hander;	/* */
                //struct nel_SYMBOL *post_hander;	/* */
                struct nel_SYMBOL *parent;	/* terminal a(P()) 's parent is terminal a */
                /* nonterminal A(P()) 's parent is nonterminal A */
                int defprod;
                union nel_TYPE *descriptor;	/* type descriptor for this event,
                					for example, the descriptor would be a
                					pointer to "struct foo" if user defined A as 
                					event struct foo A; */

        }
        event ;


        struct {

		nel_D_token type:7;
		unsigned_int _const:1;
		unsigned_int _volatile:1;
		unsigned_int garbage:1;
		unsigned_int traversed:1;
                unsigned_int size;
                unsigned_int alignment;
		union nel_TYPE *next ; 	
                //nel_D_token type;

		int rel;
                struct nel_SYMBOL *lhs;	/*left hand side; must be nonterminal */
                struct nel_RHS *rhs;	/*list of structs for each arg*/
                struct nel_RHS *stop;	/* */
		union  nel_STMT *stmts; 
                int rhs_num;		/* length of RHS */
                int precedence;		/*precedence level for disambiguation */
                struct nel_SYMBOL *action;	/* production 's action function symbol */
        }
        prod;


} nel_type;


/*****************************************************************************/
/* an object of a given type is legal on the lhs of an assigment if the type */
/* has the appropriate nel_D_token and the object was not declared const.     */
/*****************************************************************************/
#define nel_lhs_type(_type) \
	(nel_lhs_D_token ((_type)->simple.type) && (! (_type)->simple._const))



/***************************************************************/
/* linked lists are made of the following structures so that   */
/* we may find the start of all the chunks of memory allocated */
/* to hold type descriptors at garbage collection time.        */
/***************************************************************/
typedef struct nel_TYPE_CHUNK
{
        union nel_TYPE *start;
        unsigned_int size;
        struct nel_TYPE_CHUNK *next;
}
nel_type_chunk;



/**************************************************/
/* declarations for the type descriptor allocator */
/**************************************************/
extern unsigned_int nel_types_max;
extern nel_lock_type nel_types_lock;




/*****************************************************************************/
/* the structres for keeping track of blocks in COMPILED routines.  these    */
/* structures are generated when scanning the symbol table of the compiled   */
/* code only, not when interpreting any actual C code.  an nel_block          */
/* structure is allocated for every block (surrounded by '{' and '}' - not   */
/* a basic block as defined for compiler optimizations) in the code, and     */
/* lists of symbols in that block's scope (the lists are made of nel_symbol   */
/* structures connected together with nel_list structures) are attached to    */
/* the nel_block structure.                                                   */
/*****************************************************************************/
typedef struct nel_BLOCK
{
	unsigned_int garbage:1;	/* garbage collection bit		*/
	int block_no:31;		/* block number				*/
        char *start_address;		/* starting address			*/
        char *end_address;		/* ending address			*/
        struct nel_LIST *locals;	/* idents local to block		*/
        struct nel_BLOCK *blocks;	/* nested blocks			*/
        struct nel_BLOCK *next;	/* next block at this level		*/
}
nel_block;



/***************************************************************/
/* linked lists are made of the following structures so that   */
/* we may find the start of all the chunks of memory allocated */
/* to hold nel_block structures at garbage collection time.     */
/***************************************************************/
typedef struct nel_BLOCK_CHUNK
{
        unsigned_int size;
        struct nel_BLOCK *start;
        struct nel_BLOCK_CHUNK *next;
}
nel_block_chunk;



/*****************************************************/
/* declarations for the nel_block structure allocator */
/*****************************************************/
extern unsigned_int nel_blocks_max;
extern nel_lock_type nel_blocks_lock;



/*****************************************************************************/
/* the structures comprising lists of line # / address pairs.                */
/*****************************************************************************/
typedef struct nel_LINE
{
	unsigned_int garbage:1;	/* garbage collection bit	*/
	int line_no:31;		/* line number 			*/
        char *address;		/* corresponding address	*/
        struct nel_LINE *next;	/* next in list			*/
}
nel_line;



/***************************************************************/
/* linked lists are made of the following structures so that   */
/* we may find the start of all the chunks of memory allocated */
/* to hold nel_line structures at garbage collection time.      */
/***************************************************************/
typedef struct nel_LINE_CHUNK
{
        unsigned_int size;
        struct nel_LINE *start;
        struct nel_LINE_CHUNK *next;
}
nel_line_chunk;



/****************************************************/
/* declarations for the nel_list structure allocator */
/****************************************************/
extern unsigned_int nel_lines_max;
extern nel_lock_type nel_lines_lock;



/*****************************************************************************/
/* the structures comprising lists of symbols.  they are used in various     */
/* data structures.                                                          */
/*****************************************************************************/
typedef struct nel_LIST
{
	unsigned_int garbage:1;	/* garbage collection bit	*/
	int value:31;		/* used for whatever		*/
        struct nel_SYMBOL *symbol;	/* symbol			*/
        struct nel_LIST *next;	/* next in list			*/
}
nel_list;



/***************************************************************/
/* linked lists are made of the following structures so that   */
/* we may find the start of all the chunks of memory allocated */
/* to hold nel_list structures at garbage collection time.      */
/***************************************************************/
typedef struct nel_LIST_CHUNK
{
        unsigned_int size;
        struct nel_LIST *start;
        struct nel_LIST_CHUNK *next;
}
nel_list_chunk;



/****************************************************/
/* declarations for the nel_list structure allocator */
/****************************************************/
extern unsigned_int nel_lists_max;
extern nel_lock_type nel_lists_lock;



/*********************************************************************/
/* the structures representing each member in a union or struct def. */
/*********************************************************************/
typedef struct nel_MEMBER
{
	unsigned_int garbage:1;	/* garabge collection bit		*/
	unsigned_int bit_field:1;	/* is a bit field ? 			*/
	unsigned_int bit_size:15;	/* size of bit field in bits		*/
	unsigned_int bit_lb:15;	/* lowest bit, if a bit field		*/
        unsigned_int offset;		/* offset of member in bytes from base	*/
        struct nel_SYMBOL *symbol;	/* symbol entry for this member		*/
        struct nel_MEMBER *next;	/* next member				*/
}
nel_member;



/***************************************************************/
/* linked lists are made of the following structures so that   */
/* we may find the start of all the chunks of memory allocated */
/* to hold nel_member structures at garbage collection time.    */
/***************************************************************/
typedef struct nel_MEMBER_CHUNK
{
        unsigned_int size;
        struct nel_MEMBER *start;
        struct nel_MEMBER_CHUNK *next;
}
nel_member_chunk;



/******************************************************/
/* declarations for the nel_member structure allocator */
/******************************************************/
extern unsigned_int nel_members_max;
extern nel_lock_type nel_members_lock;



/****************************************************************************/
/* declarations for the nel_stab_type structures - for every file, each type */
/* descriptor has an associated integer by which it is referenced in the    */
/* stab strings, and these structures are entries into a hash table which   */
/* is used to lookup the type descriptors.  they remain a permanent part of */
/* the type descriptor for any file, and possibly other identifiers if      */
/* there is an error scanning the symbol table.                             */
/****************************************************************************/
typedef struct nel_STAB_TYPE
{
        union nel_TYPE *type;		/* assiciated type descriptor		*/
        int file_num;		/* nth file (SPARC only)		*/
        int type_num;		/* code for type			*/
	unsigned_int inserted:1;	/* inserted in nel_stab_hash table ?	*/
        
        struct nel_STAB_TYPE *next;	/* used for chaining hash entries	*/
        struct nel_STAB_TYPE *file_next;/* next in program unit		*/
}
nel_stab_type;



/***************************************************************/
/* linked lists are made of the following structures so that   */
/* we may find the start of all the chunks of memory allocated */
/* to hold nel_member structures at garbage collection time.    */
/***************************************************************/
typedef struct nel_STAB_TYPE_CHUNK
{
        unsigned_int size;
        struct nel_STAB_TYPE *start;
        struct nel_STAB_TYPE_CHUNK *next;
}
nel_stab_type_chunk;



/*********************************************************/
/* declarations for the nel_stab_type structure allocator */
/*********************************************************/
extern unsigned_int nel_stab_types_max;
extern nel_lock_type nel_stab_types_lock;



/****************************************************************************/
/* use the following macros to retrieve a pointer to the real or imaginary  */
/* part of a complex number, given a pointer to the entire number, and      */
/* the type of each of its parts.  on all architectures so far, the fortran */
/* compiler stores the imaginary part immediately following the real part.  */
/****************************************************************************/
#define nel_real_part(_ptr,_real_type)		((_real_type *) _ptr)
#define nel_imaginary_part(_ptr,_real_type)	(((_real_type *) _ptr) + 1)



/**********************************************************************/
/* pointers to type descriptors for the simple types are statically   */
/* allocated, initialized one the first invocation of the application */
/* executive, and are referenced when a simple type descriptor is     */
/* needed.  they are shared between many symbols, reducing the amount */
/* of space needed, and the amount of garbage generated.  each type   */
/* also has a symbol table entry (as we need to look up the type name */
/* when scanning the stab table), so we statically allocate a symbol  */
/* for the type, too.  note that some compilers reverse the order     */
/* of long/short with signed/unsigned, so they may call "signed long" */
/* "long signed", so we need a symbol table entry for both, but only  */
/* a single type descriptor.                                          */
/**********************************************************************/
extern union nel_TYPE *nel_char_type;
extern union nel_TYPE *nel_complex_type;
extern union nel_TYPE *nel_complex_double_type;
extern union nel_TYPE *nel_complex_float_type;
extern union nel_TYPE *nel_double_type;
extern union nel_TYPE *nel_float_type;
extern union nel_TYPE *nel_int_type;
extern union nel_TYPE *nel_long_type;
extern union nel_TYPE *nel_long_complex_type;
extern union nel_TYPE *nel_long_complex_double_type;
extern union nel_TYPE *nel_long_complex_float_type;
extern union nel_TYPE *nel_long_double_type;
extern union nel_TYPE *nel_long_float_type;
extern union nel_TYPE *nel_long_int_type;
extern union nel_TYPE *nel_long_long_type;
extern union nel_TYPE *nel_short_type;
extern union nel_TYPE *nel_short_int_type;
extern union nel_TYPE *nel_signed_type;
extern union nel_TYPE *nel_signed_char_type;
extern union nel_TYPE *nel_signed_int_type;
extern union nel_TYPE *nel_signed_long_type;
extern union nel_TYPE *nel_signed_long_int_type;
extern union nel_TYPE *nel_signed_long_long_type;
extern union nel_TYPE *nel_signed_short_type;
extern union nel_TYPE *nel_signed_short_int_type;
extern union nel_TYPE *nel_unsigned_type;
extern union nel_TYPE *nel_unsigned_char_type;
extern union nel_TYPE *nel_unsigned_int_type;
extern union nel_TYPE *nel_unsigned_long_type;
extern union nel_TYPE *nel_unsigned_long_int_type;
extern union nel_TYPE *nel_unsigned_long_long_type;
extern union nel_TYPE *nel_unsigned_short_type;
extern union nel_TYPE *nel_unsigned_short_int_type;
extern union nel_TYPE *nel_void_type;

/************************************/
/* the symbols for the C type names */
/************************************/
extern struct nel_SYMBOL *nel_char_symbol;
extern struct nel_SYMBOL *nel_complex_symbol;
extern struct nel_SYMBOL *nel_complex_double_symbol;
extern struct nel_SYMBOL *nel_complex_float_symbol;
extern struct nel_SYMBOL *nel_double_symbol;
extern struct nel_SYMBOL *nel_float_symbol;
extern struct nel_SYMBOL *nel_int_symbol;
extern struct nel_SYMBOL *nel_long_symbol;
extern struct nel_SYMBOL *nel_long_complex_symbol;
extern struct nel_SYMBOL *nel_long_complex_double_symbol;
extern struct nel_SYMBOL *nel_long_complex_float_symbol;
extern struct nel_SYMBOL *nel_long_double_symbol;
extern struct nel_SYMBOL *nel_long_float_symbol;
extern struct nel_SYMBOL *nel_long_int_symbol;
extern struct nel_SYMBOL *nel_short_symbol;
extern struct nel_SYMBOL *nel_short_int_symbol;
extern struct nel_SYMBOL *nel_signed_symbol;
extern struct nel_SYMBOL *nel_signed_char_symbol;
extern struct nel_SYMBOL *nel_signed_int_symbol;
extern struct nel_SYMBOL *nel_signed_long_symbol;
extern struct nel_SYMBOL *nel_long_signed_symbol;
extern struct nel_SYMBOL *nel_signed_long_int_symbol;
extern struct nel_SYMBOL *nel_long_signed_int_symbol;
extern struct nel_SYMBOL *nel_signed_short_symbol;
extern struct nel_SYMBOL *nel_short_signed_symbol;
extern struct nel_SYMBOL *nel_signed_short_int_symbol;
extern struct nel_SYMBOL *nel_short_signed_int_symbol;
extern struct nel_SYMBOL *nel_unsigned_symbol;
extern struct nel_SYMBOL *nel_unsigned_char_symbol;
extern struct nel_SYMBOL *nel_unsigned_int_symbol;
extern struct nel_SYMBOL *nel_unsigned_long_symbol;
extern struct nel_SYMBOL *nel_long_unsigned_symbol;
extern struct nel_SYMBOL *nel_unsigned_long_int_symbol;
extern struct nel_SYMBOL *nel_long_unsigned_int_symbol;
extern struct nel_SYMBOL *nel_unsigned_short_symbol;
extern struct nel_SYMBOL *nel_short_unsigned_symbol;
extern struct nel_SYMBOL *nel_unsigned_short_int_symbol;
extern struct nel_SYMBOL *nel_short_unsigned_int_symbol;
extern struct nel_SYMBOL *nel_void_symbol;
extern struct nel_SYMBOL *nel_long_long_symbol;
extern struct nel_SYMBOL *nel_unsigned_long_long_symbol;


/*************************************************************/
/* we might as well have a location type so we don't keep    */
/* allocating hunreds of identical location type descriptors */
/* when scanning the ld symbol table of the executable file. */
/* a generic pointer type comes in handy, also.              */
/*************************************************************/
extern union nel_TYPE *nel_pointer_type;
extern union nel_TYPE *nel_location_type;



/*******************************************************/
/* declarations for the routines defined in "nel_s_u.c" */
/*******************************************************/
extern void nel_set_struct_offsets (struct nel_eng *, union nel_TYPE *);
extern unsigned_int nel_s_u_size (struct nel_eng *, unsigned_int *, register struct nel_MEMBER *);
extern unsigned_int nel_bit_field_member (register struct nel_MEMBER *, struct nel_SYMBOL **);


/***********************************************************/
/* declarations for the routines defined in "nel_types.c".  */
/***********************************************************/
extern char *nel_D_name (register nel_D_token);
extern char *nel_TC_name (register nel_D_token);
extern nel_D_token nel_D_token_equiv (register nel_D_token);
extern union nel_TYPE *nel_type_alloc (struct nel_eng *, register nel_D_token, register unsigned_int, register unsigned_int, register unsigned_int, register unsigned_int, ...);
extern void nel_type_dealloc (register union nel_TYPE *);
extern void nel_print_type (FILE *, register union nel_TYPE *, int);
extern struct nel_BLOCK *nel_block_alloc (struct nel_eng *, int, char *, char *, struct nel_LIST *, struct nel_BLOCK *, struct nel_BLOCK *);
extern void nel_block_dealloc (register struct nel_BLOCK *);
extern void nel_print_blocks (FILE *, register struct nel_BLOCK *, int);
extern struct nel_LINE *nel_line_alloc (struct nel_eng *, int, char *, struct nel_LINE *);
extern void nel_line_dealloc (register struct nel_LINE *);
extern void nel_print_lines (FILE *, register struct nel_LINE *, int);


extern struct nel_LIST *__make_entry(struct nel_SYMBOL *);
extern void __append_entry_to(struct nel_LIST **, struct nel_LIST *);
extern void __get_item_out_from(struct nel_LIST **, struct nel_SYMBOL *);
extern struct nel_SYMBOL * __get_first_out_from(struct nel_LIST **);
extern int __get_count_of(struct nel_LIST **);
extern struct nel_LIST *nel_list_alloc (struct nel_eng *, int, struct nel_SYMBOL *, struct nel_LIST *);
extern void nel_list_dealloc (register struct nel_LIST *);
extern void nel_print_list (FILE *, register struct nel_LIST *, int);

extern struct nel_MEMBER *nel_member_alloc (struct nel_eng *, struct nel_SYMBOL *, unsigned_int, unsigned_int, unsigned_int, unsigned_int, struct nel_MEMBER *);
extern void nel_member_dealloc (register struct nel_MEMBER *);
extern void nel_print_members (FILE *, register struct nel_MEMBER *, int);

extern void nel_type_init (struct nel_eng *eng);

extern void nel_insert_C_types (struct nel_eng *eng);
extern void nel_remove_C_types (struct nel_eng *eng);

extern union nel_TYPE *nel_merge_types (struct nel_eng *, register union nel_TYPE *, register union nel_TYPE *);
extern unsigned_int nel_arg_diff (register struct nel_LIST *, register struct nel_LIST *, register unsigned_int);
extern unsigned_int nel_enum_const_lt (register struct nel_LIST *, struct nel_LIST *);
extern unsigned_int nel_enum_const_diff (register struct nel_LIST *, register struct nel_LIST *);
extern unsigned_int nel_member_diff (register struct nel_MEMBER *, register struct nel_MEMBER *, register unsigned_int);
extern unsigned_int nel_type_diff (register union nel_TYPE *, register union nel_TYPE *, register unsigned_int);
extern unsigned_int nel_type_incomplete (register union nel_TYPE *);
void emit_type (FILE *file, register nel_type *type);
int is_compatible_types(nel_type *type1, nel_type *type2);

int is_asgn_compatible(struct nel_eng *eng, nel_type* type1, nel_type *type2);
void list_dealloc(struct nel_eng *eng);
void line_dealloc(struct nel_eng *eng);
void type_dealloc(struct nel_eng *eng);
void block_dealloc(struct nel_eng *eng);
void member_dealloc(struct nel_eng *eng);

int __lookup_item_from(struct nel_LIST **plist, struct nel_SYMBOL *symbol);
int nel_coerce (struct nel_eng *eng, nel_type *new_type, char *new_value, nel_type *old_type, char *old_value);
int nel_extract_bit_field (struct nel_eng *eng, nel_type *type, char *result, unsigned_int word, register unsigned_int lb, register unsigned_int size);

int s_u_has_member(struct nel_eng *eng, nel_type *type, struct nel_SYMBOL *member);

#endif /* _NEL_TYPES_ */

