
/*****************************************************************************/
/* This file, "symbols.c", contains routines for the manipulating symbol  */
/* the symbol table / hash table / value table entries for the application   */
/* executive.                                                                */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <engine.h>
#include <errors.h>
#include <io.h>
#include <sym.h>
#include <stmt.h>
#include <intp.h>
#include <evt.h>
#include <type.h>
#include <expr.h>
#include <prod.h>
#include <mem.h>

/*
 
The following figure shows the interaction of the
symbol data structures.  Any arrow(s) on a box indicates
the data structure is managed as a stack, growing in
the direction pointed to.
 
 
                            -------------------
                            | static_names    |
                            -------------------
                                  ^    ^
                                  |    |
    ^^^^^^^^^^       ^^^^^^^^^^^  |    | ___________       __________
    |        |       |         |  |    | |         |       |        |
    |        |       |---------|  |    | |         |       |        |
    |        |       |name=NULL|  |    | |         |       |        |
    |        |    t<-|type     |  |    | |---------|       |        |
    |        |<------|value    |<-+--  --|name     |----   |        |
    |--------|       |---------|  | |    |type     |->t    |        |
    |        |       |name     |--- o--->|value    |----   |        |
    |        |    t<-|type     |    |    |---------|   |   |        |
    |        |<------|value    |<---o    |         |   --->|        |
    |--------|       |---------|    |    |         |       |--------|
    |        |       |         |    |    |         |       |        |
    | dyn    |       | dyn     |    |    | static  |       | static |
    | values |       | symbols |    |    | symbols |       | values |
    |________|    -->|_________|    |    |_________|<--    |________|
                  |                 |                 |
         ----------        ---------o        ---------o---------
         |                 |        |        |        |        |
  _______|_______   _______|_______ | _______|_______ | _______|_______
  |             |   |             | | |             | | |             |
  |     dyn     |   |     dyn     | | |   static    | | |   static    |
  | label hash  |   | ident hash  | | | ident hash  | | |  file hash  |
  |_____________|   |_____________| | |_____________| | |_____________|
                                    |                 |
                           ---------o        ---------o---------
                           |                 |        |        |
                    _______|_______   _______|_______ | _______|_______
                    |             |   |             | | |             |
                    |     dyn     |   |   static    | | |   static    |
                    |  tag hash   |   |  tag hash   | | |location hash|
                    |_____________|   |_____________| | |_____________|
                                                      |
                                             -------------------
                                             |                 |
                                      _______|_______   _______________
                                      |             |   |             |
                                      |     dyn     |   |   static    |
                                      |location hash|   |  name hash  |
                                      |_____________|   |_____________|
 
 
  t   => type descriptor
  ^   => stack growth
  >   => stack growth
 <^V> => pointers
  o   => lines join
  +   => lines cross, but do not join
 
 
Temporaries for expression evaluation are placed on the dyn_symbols
stack, and are unnamed.  They are indexed by pointers in the
semantic stack.
 
Whenever an identifier name is read, it is searched for in the
static name hash table.  if it is not found, then an entry for it
is created.  The character string for the name becomes the name field
in all symbols by that name that are subsequently created.  This
implies that names may be compared by comparing pointers and not doing
a strcmp(), although this feature is not currently used in nel.
 
Labels, tags, other identifiers have three separate name spaces.
The symbols for all name spaces appear in dyn symbols and static symbols,
but are indexed by separate hash tables.  The three separate name
space and corresponding tables are:
 
   identifiers:
      dyn_ident_hash
      static_ident_hash
      dyn_location_hash
      static_location_hash
 
   struct/union/enum tags:
      dyn_tag_hash
      static_tag_hash
 
   labels:
      dyn_label_hash
 
The static_location_hash table contain entries created when (if)
the ld symbol table is scanned.  such identifiers behave in the much
the same manner as character array: they are coerced to pointers
when used in an expression or in an argument.
 
When looking up a symbol in the appropriate name space, we
search the tables in the order listed, unless looking for a
location (ld symbol entry), in which case we search only the
static location hash table, and not the dyn/static ident hash table.
 
Space is allocated for the dyn value, dyn symbol, and dyn hash tables
upon entrance to nel_call_eval()/nel_call_parse(), and deallocated upon exit.
Space is allocated for the static name, static value, static symbol, and
all static hash tables the first time the Engine is called.
 
An identifier declared static, local to a file, is put in the
dyn ident hash table, even though it is at level 0.  See the macros
nel_symbol_alloc () and nel_insert_<ident/location/tag/label/file> ()
for the criterion as to whether a symbol is inserted in a dynamic or
static hash table.  When a program unit is parsed, all symbols for
it are statically allocated.  When that routine is subsequently
interpreted, dynamically allocated copies of the local symbols are
made, and any expression evaluation temporaries are also dynamically
allocated.  If a symbol is dynamically allocated, it must be inserted
in a dynamic table.  All symbols allocated while scanning the stab
entries of the executable code are statically allocated, as the local
vars become part of a permanent type descriptor for each compiled
function.
 
We also keep a hash table of symbols for file names.  This is necessary
when scanning the stab entries of the executable code, and is also used
translating a file name and line number into an absolute address.
 
The hashing scheme involves adding the values of each character in the
symbol's name, shifting left one, incrementing if greater than the
number of chains in the table,  and taking the result modulo number
of chains between each sucessive addition.  In order to perform an
efficient modulo division, we 'and' all bits with a mask.  We assume
that the size of all hash tables is a power of 2, so that the mask
is equal to (2 * size) - 1.
 
The type descriptors for each symbol are statically allocated
and garbage collected.  This way we can obtain the type descriptor
for a subtype by simply copying the pointer in the supertype's
descriptor, and not have to copy the entire structure pointed to,
to keep descriptors distinct, manually returning them to a free
list when references to them go out of scope.
 
Symbols which are members of a structure or union, or the placeholders
that appear in a function's formal argument list (not the copies that
are made of them) are also statically allocated and garbage collected,
as they are actually part of a type descriptor, and are not entered in
the symbol table directly.  Garbage collection is mark and sweep, and
proceeds as follows:
 
   1) Mark all types and symbols as garbage, except those on free lists.
 
   2) Scan through the chunks of memory allocated for static and
      dynamic symbols, skipping member symbols and formal placeholders.
      Traverse the type descriptor of each symbol, marking the entire
      structure (member symbols included) as not garbage.
 
   3) Scan through the chunks of memory allocated for types, returning
      type descriptors marked as garbage to the free list.
 
   4) Scan through the chunks of memory allocated for symbols,
      returning only member symbols and formal placeholders marked
      as garbage to the free list.  There is at least one reference
      to all other symbols allocated at the current scoping level
      or an outer level.  Whenever symbols are duplicated, as in
      the case of a redeclaration of a typedef name or a tag at
      level 0 (see the comments in nel_parse.y for why this is
      allowed), the duplicates are manually returned to the
      free list.
 
Garbage collection has not been implemented yet.
 
*/



/*************************************************************/
/* linked lists are made of the following structures so that */
/* we may find the start of all chunks of memory allocated   */
/* to hold static symbol names at garbage collection time.   */
/* each chunk is of size nel_static_symbols_max, and it is    */
/* possible that this may be changed dynamically.            */
/*************************************************************/
static nel_static_name_chunk *nel_static_name_chunks = NULL;



/**********************************************/
/* declarations for the static name allocator */
/**********************************************/
unsigned_int nel_static_names_max = 0x2000;
nel_lock_type nel_static_names_lock = nel_unlocked_state;
static char *nel_static_names_next = NULL;
static char *nel_static_names_end = NULL;

nel_list *c_include_list = NULL ;
nel_list *last_c_include = NULL;
int c_include_num = 0;


/*****************************************************************************/
/* nel_static_name_alloc () inserts a name in *nel_static_names                */
/*****************************************************************************/
char *nel_static_name_alloc (struct nel_eng *eng, char *name)
{
	register char *source;
	register char *dest;
	register char ch = '\1';	/* nonzero value triggers overflow */
	register char *retval;

	nel_debug ({ nel_trace (eng, "entering nel_static_name_alloc [\nname = %s\n\n", name); });

	/*******************************************/
	/* static allocators are critical sections */
	/*******************************************/
	nel_lock (&nel_static_names_lock);

start:

	/*************/
	/* copy name */
	/*************/
	for (source = name, retval = dest = nel_static_names_next;
			((dest < nel_static_names_end) &&
			 ((*(dest++) = ch = *(source++)) != '\0'));)
		;

	/***************************************************/
	/* on overflow, allocate more space and start over */
	/***************************************************/
	if (ch != '\0') {
		register nel_static_name_chunk *chunk;
		register unsigned_int len;

		nel_debug ({ nel_trace (eng, "static name/string table overflow - allocating more space\n\n"); });
		if ((len = strlen (name)) > nel_static_names_max) {
			nel_static_names_max = len;
		}
		nel_malloc (nel_static_names_next, nel_static_names_max, char);
		nel_static_names_end = nel_static_names_next + nel_static_names_max;

		/*************************************************/
		/* make an entry for this chunk of memory in the */
		/* list of nel_static_name_chunks.                */
		/*************************************************/
		nel_malloc (chunk, 1, nel_static_name_chunk);
		chunk->start = nel_static_names_next;
		chunk->size = nel_static_names_max;
		chunk->next = nel_static_name_chunks;
		nel_static_name_chunks = chunk;
		goto start;
	}

	/*************************/
	/* exit critical section */
	/*************************/
	nel_unlock (&nel_static_names_lock);

	/*********************************************************/
	/* all OK.  advance nel_static_names_next to after string */
	/* and return a pointer to the string.                   */
	/*********************************************************/
	nel_static_names_next = dest;

	nel_debug ({ nel_trace (eng, "] exiting nel_static_name_alloc\nretval = %s\n\n", retval); });
	return (retval);
}



/************************************************/
/* declarations for the dynamic value allocator */
/************************************************/
unsigned_int nel_dyn_values_max = 0x80000;
/*********************************************************/
/* dyn_values_next and dyn_values_end are in nel_eng  */
/*********************************************************/




/*************************************************************/
/* linked lists are made of the following structures so that */
/* we may find the start of all chunks of memory allocated   */
/* to hold static values at garbage collection time.         */
/* each chunk is of size nel_static_symbols_max, and it is    */
/* possible that this may be changed dynamically.            */
/*************************************************************/
static nel_static_value_chunk *nel_static_value_chunks = NULL;



/*****************************************************/
/* declarations for the static value value allocator */
/*****************************************************/
unsigned_int nel_static_values_max = 0x8000;
nel_lock_type nel_static_values_lock = nel_unlocked_state;
static char *nel_static_values_next = NULL;
static char *nel_static_values_end = NULL;



/*****************************************************************************/
/* nel_static_value_alloc () allocates a space of <bytes> bytes in the static */
/* value table on an alignment-byte boundary, and returns a pointer to it.   */
/*****************************************************************************/
char *nel_static_value_alloc (struct nel_eng *eng, register unsigned_int bytes, register unsigned_int alignment)
{
	register char *retval;

	nel_debug ({ nel_trace (eng, "entering nel_static_value_alloc [\neng = 0x%x\nbytes = 0x%x\nalignment = 0x%x\n\n", eng, bytes, alignment); });

	nel_debug ({
	if (bytes == 0) {
		//nel_fatal_error (eng, "(nel_static_value_alloc #1): bytes = 0");
		return NULL;
	}
	if (alignment == 0) {
		//nel_fatal_error (eng, "(nel_static_value_alloc #2): alignment = 0");
		return NULL;
	}
	});

	/*******************************************/
	/* static allocators are critical sections */
	/*******************************************/
	nel_lock (&nel_static_values_lock);

start:

	/*****************************************/
	/* we do not want memory fetches for two */
	/* different objects overlapping in the  */
	/* same word if the machine does not     */
	/* support byte-size loads and stores.   */
	/*****************************************/
	nel_static_values_next = nel_align_ptr (nel_static_values_next, nel_SMALLEST_MEM);

	/**************************************************/
	/* get pointer to the start of allocated space.   */
	/* make sure it is on an alignment-byte boundary. */
	/**************************************************/
	retval = nel_static_values_next;
	retval = nel_align_ptr (retval, alignment);

	if (retval + bytes > nel_static_values_end) {
		/***************************************************/
		/* on overflow, allocate more space and start over */
		/***************************************************/
		register nel_static_value_chunk *chunk;

		if (bytes > nel_static_values_max) {
			/******************************************/
			/* make sure that we will allocate enough */
			/* memory next time around.               */
			/******************************************/
			nel_static_values_max = nel_align_offset (bytes, nel_SMALLEST_MEM);
			nel_static_values_max = nel_align_offset (bytes, alignment);
		}
		nel_debug ({ nel_trace (eng, "static value table overflow - allocating more space\n\n"); });
		nel_malloc (nel_static_values_next, nel_static_values_max, char);
		nel_static_values_end = nel_static_values_next + nel_static_values_max;

		/*************************************************/
		/* make an entry for this chunk of memory in the */
		/* list of nel_static_value_chunks                */
		/*************************************************/
		nel_malloc (chunk, 1, nel_static_value_chunk);
		chunk->start = nel_static_values_next;
		chunk->size = nel_static_values_max;
		chunk->next = nel_static_value_chunks;
		nel_static_value_chunks = chunk;
		goto start;
	} else {
		/*********************/
		/* reserve the space */
		/*********************/
		nel_static_values_next = retval + bytes;
	}

	/*************************/
	/* exit critical section */
	/*************************/
	nel_unlock (&nel_static_values_lock);

	/***********************************************/
	/* zero out all elements of the allocate space */
	/* note that nel_static_values_next now points  */
	/* to just above the allocated space.          */
	/***********************************************/
	nel_zero (bytes, retval);

	nel_debug ( {
	nel_trace (eng, "] exiting nel_static_value_alloc\nretval = 0x%x\n\n", retval);
	}
	);
	return (retval);
}



/*****************************************************************************/
/* nel_C_name () returns the string that is the identifier for <code>, a      */
/* nel_C_token representing a symbol's class.                                 */
/*****************************************************************************/
char *nel_C_name (register nel_C_token code)
{
	switch (code) {
	case  nel_C_NULL:
		return ("nel_C_NULL");
	case  nel_C_COMPILED_FUNCTION:
		return ("nel_C_COMPILED_FUNCTION");
	case  nel_C_COMPILED_STATIC_FUNCTION:
		return ("nel_C_COMPILED_STATIC_FUNCTION");
	case  nel_C_NEL_FUNCTION:
		return ("nel_C_NEL_FUNCTION");
	case  nel_C_NEL_STATIC_FUNCTION:
		return ("nel_C_NEL_STATIC_FUNCTION");
	case  nel_C_INTRINSIC_FUNCTION:
		return ("nel_C_INTRINSIC_FUNCTION");
	case  nel_C_GLOBAL:
		return ("nel_C_GLOBAL");
	case  nel_C_STATIC_GLOBAL:
		return ("nel_C_STATIC_GLOBAL");
	case  nel_C_LOCAL:
		return ("nel_C_LOCAL");
	case  nel_C_STATIC_LOCAL:
		return ("nel_C_STATIC_LOCAL");
	case  nel_C_REGISTER_LOCAL:
		return ("nel_C_REGISTER_LOCAL");
	case  nel_C_FORMAL:
		return ("nel_C_FORMAL");
	case  nel_C_REGISTER_FORMAL:
		return ("nel_C_REGISTER_FORMAL");
	case  nel_C_CONST:
		return ("nel_C_CONST");
	case  nel_C_ENUM_CONST:
		return ("nel_C_ENUM_CONST");
	case  nel_C_RESULT:
		return ("nel_C_RESULT");
	case  nel_C_LABEL:
		return ("nel_C_LABEL");
	case  nel_C_TYPE:
		return ("nel_C_TYPE");
	case  nel_C_MEMBER:
		return ("nel_C_MEMBER");
	case  nel_C_FILE:
		return ("nel_C_FILE");
	case  nel_C_LOCATION:
		return ("nel_C_LOCATION");
	case  nel_C_STATIC_LOCATION:
		return ("nel_C_STATIC_LOCATION");
	case  nel_C_NAME:
		return ("nel_C_NAME");
	case  nel_C_BAD_SYMBOL:
		return ("nel_C_BAD_SYMBOL");
	case  nel_C_MAX:
		return ("nel_C_MAX");
	default:
		return ("nel_C_BAD_TOKEN");
	}
}



/*****************************************************************************/
/* nel_L_name () returns the string that is the identifier for <code>, a      */
/* nel_L_token representing a symbol's source language.                       */
/*****************************************************************************/
char *nel_L_name (register nel_L_token code)
{
	switch (code) {
	case  nel_L_NULL:
		return ("nel_L_NULL");
	case  nel_L_NEL:
		return ("nel_L_NEL");
	case  nel_L_ASSEMBLY:
		return ("nel_L_ASSEMBLY");
	case  nel_L_C:
		return ("nel_L_C");
	case  nel_L_FORTRAN:
		return ("nel_L_FORTRAN");
	case  nel_L_NAME:
		return ("nel_L_NAME");
	case  nel_L_MAX:
		return ("nel_L_MAX");
	default:
		return ("nel_L_BAD_TOKEN");
	}
}

char *nel_Rel_name (Relation code)
{
	switch (code) {
	case  REL_EX :
		return ("!");
	case  REL_ON:
		return (":");
	case  REL_OR:
		return ("|");
	case  REL_OT:
		return (":");
	case  REL_AT:
		return ("->");
	case  REL_UN:
	default:
		return ("Unkown Rel");
	}
}


/*************************************************/
/* declarations for the dynamic symbol allocator */
/*************************************************/
unsigned_int nel_dyn_symbols_max = 0x4000;
/***********************************************************/
/* dyn_symbols_next and dyn_symbols_end are in nel_eng */
/***********************************************************/



/****************************************************************/
/* the linked list of nel_static_symbol_chunk structures so that */
/* we may find the start of all chunks of memory allocated      */
/* to hold static symbols at garbage collection time.           */
/****************************************************************/
static nel_static_symbol_chunk *nel_static_symbol_chunks = NULL;



/*************************************************************/
/* linked lists are made of the following structures so that */
/* we may find the start of all chunks of memory allocated   */
/* to hold static symbols at garbage collection time.        */
/* each chunk is of size nel_static_symbols_max, and it is    */
/* possible that this may be changed dynamically.            */
/*************************************************************/
unsigned_int nel_static_symbols_max = 0x400;
nel_lock_type nel_static_symbols_lock = nel_unlocked_state;
static nel_symbol *nel_static_symbols_next = NULL;
static nel_symbol *nel_static_symbols_end = NULL;
static nel_symbol *nel_free_static_symbols = NULL;

static int static_symbol_id = 1024 ;

/*****************************************************************************/
/* nel_static_symbol_alloc () allocates a symbol in *nel_static_symbols.       */
/* <name> is not copied - it should reside in non-volitale memory.           */
/*   char *name;		identifier string				*/
/*   nel_type *type;	type descriptor				*/
/*   char *value;		points to memory where value is stored	*/
/*   nel_C_token class;	symbol class					*/
/*   unsigned_int lhs;	legal left hand side of an assignment	*/
/*   nel_L_token source_lang; source language				*/
/*   int level;		scoping level				*/
/*****************************************************************************/
nel_symbol *nel_static_symbol_alloc (struct nel_eng *eng, char *name, nel_type *type, char *value, nel_C_token class, unsigned_int lhs, nel_L_token source_lang, int level)
{
	register nel_symbol *retval;

	nel_debug ({ nel_trace (eng, "entering nel_static_symbol_alloc [\nname = %s\ntype =\n%1Tvalue = 0x%x\nclass = %C\nlhs = %d\nsource_lang = %H\nlevel = %d\n\n", (name == NULL) ? "NULL" : name, type, value, class, lhs, source_lang, level); });

	/*******************************************/
	/* static allocators are critical sections */
	/*******************************************/
	nel_lock (&nel_static_symbols_lock);

	if (nel_free_static_symbols != NULL)
	{
		/***************************************************/
		/* first, try to re-use deallocated static symbols */
		/***************************************************/
		retval = nel_free_static_symbols;
		nel_free_static_symbols = nel_free_static_symbols->next;
	} else
	{
		/************************************/
		/* on overflow, allocate more space */
		/************************************/
		if (nel_static_symbols_next >= nel_static_symbols_end) {
			register nel_static_symbol_chunk *chunk;
			nel_debug ({ nel_trace (eng, "nel_symbol structure overflow - allocating more space\n\n"); });
			nel_malloc (nel_static_symbols_next, nel_static_symbols_max, nel_symbol);
			nel_static_symbols_end = nel_static_symbols_next + nel_static_symbols_max;

			/*************************************************/
			/* make an entry for this chunk of memory in the */
			/* list of nel_static_symbol_chunks               */
			/*************************************************/
			nel_malloc (chunk, 1, nel_static_symbol_chunk);
			chunk->start = nel_static_symbols_next;
			chunk->size = nel_static_symbols_max;
			chunk->next = nel_static_symbol_chunks;
			nel_static_symbol_chunks = chunk;
		}
		retval = nel_static_symbols_next++;
	}

	/*************************/
	/* exit critical section */
	/*************************/
	nel_unlock (&nel_static_symbols_lock);

	//added by zhangbin, 2006-11-22
	nel_zero(sizeof(nel_symbol), retval);
	//end
	
	/*********************************/
	/* initialize members and return */
	/*********************************/
	retval->name = name;
	retval->type = type;
	retval->value = value;
	retval->class = class;
	retval->lhs = lhs;
	retval->source_lang = source_lang;
	retval->declared = 0;
	retval->level = level;
	retval->reg_no = -1;
	retval->next = NULL;
	retval->table = NULL;
	retval->member = NULL;
	retval->data = 0;

	retval->id = ++static_symbol_id ;

	//retval->prev_hander = NULL;
	//retval->post_hander = NULL;
	//retval->run_hander = NULL;
	//retval->free_hander = NULL;

	retval->_global = NULL;

	/******************************************************************/
	/* note that symbols read from the symbol table of the executable */
	/* code are statically allocated, even though their level field   */
	/* is not necessarily 0.                                          */
	/******************************************************************/

	nel_debug ({ nel_trace (eng, "] exiting nel_static_symbol_alloc\nretval =\n%1S\n", retval); });
	return (retval);
}


/*****************************************************************************/
/* nel_static_symbol_dealloc () returns the nel_symbol structure pointed to by */
/* <symbol> back to the free list (nel_free_static_symbols), so that the      */
/* memory may be re-used.                                                    */
/*****************************************************************************/
void nel_static_symbol_dealloc (struct nel_eng *eng, register nel_symbol *symbol)
{
	nel_debug ({
	if (symbol == NULL) {
		//nel_fatal_error (eng, "(nel_static_symbol_dealloc #1): symbol == NULL");
	}
	});

	if (symbol->table != NULL) {
		nel_remove_symbol (eng, symbol);
	}
	symbol->next = nel_free_static_symbols;
	nel_free_static_symbols = symbol;
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
/* nel_print_symbol_name () prints out a symbol's name, or NULL_symbol, or    */
/* unnamed_symbol.  It does not print out the end of line marker.            */
/*****************************************************************************/
void nel_print_symbol_name (FILE *file, register nel_symbol *symbol, int indent)
{
	tab (indent);
	if (symbol == NULL) {
		fprintf (file, "NULL_symbol");
	} else if (symbol->name == NULL) {
		fprintf (file, "unnamed_symbol");
	} else {
		fprintf (file, "%s", symbol->name);
	}
}


/*****************************************************************************/
/* nel_print_symbol () prints out a symbol's name, type descriptor, value,    */
/* and level.                                                                */
/*****************************************************************************/
void nel_print_symbol (FILE *file, register nel_symbol *symbol, int indent)
{
	if (symbol != NULL) {
		tab (indent);
		fprintf (file, "0x%x\n", symbol);
		tab (indent);
		fprintf (file, "name: ");
		nel_print_symbol_name (file, symbol, 0);
		fprintf (file, "\n");
		tab (indent);
		fprintf (file, "type:\n");
		nel_print_type (file, symbol->type, indent + 1);
		tab (indent);
		fprintf (file, "value: 0x%x\n", symbol->value);
		tab (indent);
		fprintf (file, "class: %s\n", nel_C_name (symbol->class));
		tab (indent);
		fprintf (file, "lhs: %d\n", symbol->lhs);
		tab (indent);
		fprintf (file, "source_lang: %s\n", nel_L_name (symbol->source_lang));
		tab (indent);
		fprintf (file, "declared: %d\n", symbol->declared);
		tab (indent);
		fprintf (file, "level: %d\n", symbol->level);
		tab (indent);
		fprintf (file, "table: 0x%x\n", symbol->table);
		tab (indent);
		fprintf (file, "s_u: 0x%x\n", symbol->s_u);
		tab (indent);
		fprintf (file, "member: 0x%x\n", symbol->member);
		tab (indent);
		fprintf (file, "next: 0x%x\n", symbol->next);
		tab (indent);
		fprintf (file, "data: 0x%x\n", symbol->data);
	} else {
		tab (indent);
		nel_print_symbol_name (file, symbol, 0);
		fprintf (file, "\n");
	}
}


/************************************************************/
/* declarations for symbols for the integer values 0 and 1, */
/* which are boolean return values.                         */
/************************************************************/
nel_symbol *nel_zero_symbol;
nel_symbol *nel_one_symbol;



/*****************************************************************************/
/* declarations for the hash tables which will index symbols.  Symbols fall  */
/* into three categories: labels (goto targets), struct/union/enum tags, and */
/* other symbols (from now on, called idents).  Symbols are hashed by adding */
/* the values of each letter in their names, while shifting the result left  */
/* one bit, incrementing it if it is greater than the table max, and taking  */
/* it modulo the table max between each addition.  Conflicts are managed by  */
/* chaining.                                                                 */
/*****************************************************************************/
/****************************************************************/
/* dyn_[ident/location/tag/label]_hash  are all in nel_eng. */
/****************************************************************/
//nel_symbol_table *nel_static_name_hash;
//nel_symbol_table *nel_static_ident_hash;
//nel_symbol_table *nel_static_location_hash;
//nel_symbol_table *nel_static_tag_hash;
//nel_symbol_table *nel_static_file_hash;


/*****************************************************************************/
/* nel_hash_symbol () applies the hash function used for table lookup of      */
/* symbols to <name> and returns the result.  <max> is the number of chains  */
/* in the table;  we assume that it is a power of 2, so that we may divide   */
/* modulo by it efficiently by and'ing with <max> - 1.                       */
/*****************************************************************************/
unsigned_int nel_hash_symbol (register char *name, unsigned_int max)
{
	register unsigned_int retval = 0;
	register unsigned_int mask = max - 1;
	register unsigned_int nmask = ~mask;

	for (; (*name != '\0'); name++) {
		retval <<= 1;
		retval += *name;
		if (retval & nmask) {
			retval++;
			retval &= mask;
		}
	}

	return (retval);
}


/*****************************************************************************/
/* nel_lookup_symbol () finds the symbol named <name> by searching the NULL   */
/* terminated list of tables in order, until the symbol is found, or we run  */
/* out of tables.  It returns a pointer to the symbol, or NULL if none is    */
/* found.                                                                    */
/*****************************************************************************/
nel_symbol *nel_lookup_symbol (char *name, ...)
{
	register nel_symbol *scan = NULL;
	register unsigned_int index;
	register nel_symbol_chain *chain;
	register nel_symbol_table *table;
	va_list args;

	if (name == NULL) {
		return (NULL);
	}

	va_start (args, name);

	while ((scan == NULL) && ((table = va_arg (args, nel_symbol_table *)) != NULL)) {
		/**************************************/
		/* apply hash function for each table */
		/**************************************/
		index = nel_hash_symbol (name, table->max);

		/****************************/
		/* lookup the name in table */
		/****************************/

		chain = table->chains + index;
		nel_lock (&(chain->lock)) ;
		for (scan = chain->symbols; scan != NULL; scan = scan->next)	//modified by zhangbin, 2006-5-26
		{
			if(!scan->name)
				continue;
			if(!strcmp(name, scan->name))
				break;
		}
		
		nel_unlock (&(chain->lock));
	}

	va_end (args);
	return (scan);
}

nel_symbol *nel_symbol_dup(struct nel_eng *eng, nel_symbol *old)
{
	nel_symbol *new;
	char *nname = NULL, *nvalue = NULL; 

	if(old != NULL) {
		/* dup name if necessary */
		if(old->name) {
			nname = strdup ( old->name); 
		}

		/* dup value if necessary */
		if(old->type->simple.size > 0 && old->value){
			nvalue = nel_static_value_alloc(eng, old->type->simple.size, old->type->simple.alignment);  
			nel_copy (old->type->simple.size, nvalue, old->value);
		}

		new = nel_static_symbol_alloc(eng, nname, old->type,nvalue, old->class, old->lhs, old->source_lang, old->level);

		/* wyong, 2004.5.23 */
		//new->prev_hander = old->prev_hander;
		//new->post_hander = old->post_hander;
		//new->run_hander = old->run_hander;

		return new;
	}

	return NULL;
}

int nel_symbol_diff(nel_symbol *s1, nel_symbol *s2)
{
	switch(s1->class) {
	case nel_C_PRODUCTION:
		return (nel_type_diff(s1->type, s2->type, 0));

	case nel_C_NONTERMINAL:
	case nel_C_TERMINAL:
		return (s1 != s2 );
		
	default:
		return (strcmp(s1->name, s2->name));
	}

	return 0;
}


/*****************************************************************************/
/* nel_list_symbols () is a debugging routine which writes to <file> all      */
/* symbols in <table>, and their scoping levels.                             */
/*****************************************************************************/
void nel_list_symbols (struct nel_eng *eng, FILE *file, nel_symbol_table *table)
{
	register unsigned_int i;
	register nel_symbol_chain *chain_scan;
	register nel_symbol *scan;

	nel_debug ({ nel_trace (eng, "entering nel_list_symbols [\ntable = 0x%x\n\n", table); });

	for (i = 0, chain_scan = table->chains; (i < table->max); i++, chain_scan++) {
		nel_lock (&(chain_scan->lock))
		;
		scan = table->chains[i].symbols;
		if (scan != NULL) {
			fprintf (file, "chain 0x%x\n", i);
		}
		for (; (scan != NULL); scan = scan->next) {
			fprintf (file, "level %d : %I\n", scan->level, scan);
		}
		nel_unlock (&(chain_scan->lock))
		;
	}

	nel_debug ({ nel_trace (eng, "\n] exiting nel_list_symbols\n\n"); });
}


/*****************************************************************************/
/* nel_insert_symbol () inserts the symbol <symbol> into <table> on the       */
/* appropriate chain.                                                        */
/*****************************************************************************/
nel_symbol *nel_insert_symbol (struct nel_eng *eng, register nel_symbol *symbol, nel_symbol_table *table)
{
	register unsigned_int index;
	register nel_symbol_chain *chain;
	register nel_symbol **scan;

	nel_debug ({ nel_trace (eng, "entering nel_insert_symbol [\nsymbol = %I\ntable = 0x%x\n\n", symbol, table); });

	/**************************************************************/
	/* we should really exit the program if an error occurrs      */
	/* at this point, but it may be that we a called by nel ()     */
	/* interpretively, so just print an error message and return. */
	/**************************************************************/
	if (symbol == NULL) {
		nel_diagnostic (eng, "cannot insert NULL symbol");
	}
	if (table == NULL) {
		nel_diagnostic (eng, "cannot insert symbol %I into NULL table", symbol);
	}
	if (symbol->table != NULL) {
		nel_diagnostic (eng, "symbol already inserted: %I", symbol);
		return (symbol);
	}

	/***********************************************************************/
	/* insert a dynamic symbol before all other symbols at the same level, */
	/* which is always the start of the chain, except perhaps in the case  */
	/* of static global vars (local to a file) while scannning the stab,   */
	/* or when calling nel_load_dyn_scope () or nel_load_static_scope ().    */
	/***********************************************************************/
	index = nel_hash_symbol (symbol->name, table->max);
	chain = table->chains + index;
	nel_lock (&(chain->lock))
	;
	for (scan = &(chain->symbols); ((*scan != NULL) && ((*scan)->level > symbol->level)); scan = &((*scan)->next))
		;
	symbol->next = *scan;
	*scan = symbol;
	symbol->table = table;
	nel_unlock (&(chain->lock))
	;

	nel_debug ({ nel_trace (eng, "] exiting nel_insert_symbol\nsymbol =\n%1S\n", symbol); });
	return (symbol);
}


/*****************************************************************************/
/* nel_remove_symbol () removes <symbol> from <table>.  <table> should        */
/* contain <max> chains.  When searching for the symbol, we compare only the */
/* pointers to the symbols, and not the names.                               */
/*****************************************************************************/
nel_symbol *nel_remove_symbol (struct nel_eng *eng, register nel_symbol *symbol)
{
	nel_debug ({ nel_trace (eng, "entering nel_remove_symbol [\nsymbol =\n%1S\n\n", symbol); });

	if ((symbol != NULL) && (symbol->table != NULL)) {
		/***************************************************************/
		/* find the correct chain to search, then search it. remove    */
		/* the sybmol from the chain, and set its table field to NULL. */
		/***************************************************************/
		register unsigned_int index;
		register nel_symbol_chain *chain;
		register nel_symbol **scan;
		index = nel_hash_symbol (symbol->name, symbol->table->max);
		chain = symbol->table->chains + index;
		nel_lock (&(chain->lock))
		;
		for (scan = &(chain->symbols); ((*scan != NULL) && (*scan != symbol)); scan = &((*scan)->next))
			;
		if (*scan != NULL) {
			*scan = (*scan)->next;
			symbol->table = NULL;
			symbol->next = NULL;
		}
		nel_unlock (&(chain->lock))
		;
	}

	nel_debug ({ nel_trace (eng, "] exiting nel_remove_symbol\nsymbol =\n%1S\n", symbol); });
	return (symbol);
}

void nel_clear_table(nel_symbol_table *table)
{
	register unsigned_int i;
	register nel_symbol_chain *chain_scan;

	for (i = 0, chain_scan = table->chains; (i < table->max); i++, chain_scan++) {
		chain_scan->symbols = NULL;
	}
}


/*****************************************************************************/
/* nel_purge_table () purges table of the symbols whose scoping level is      */
/* >= <level>.                                                               */
/*****************************************************************************/
void nel_purge_table (struct nel_eng *eng, register int level, nel_symbol_table *table)
{
	register unsigned_int i;
	register unsigned_int max = table->max;
	register nel_symbol_chain *chain_scan;
	register nel_symbol *symbol_scan;

	nel_debug ({ nel_trace (eng, "entering nel_purge_table [\nlevel = %d\ntable = 0x%x\n\n", level, table); });

	/**************************************************************/
	/* purge all symbols from <table> whose level field >= level. */
	/* reset all the table fields of the purged symbols to NULL.  */
	/**************************************************************/
	for (i = 0, chain_scan = table->chains; (i < max); i++, chain_scan++) {
		nel_lock (&(chain_scan->lock))
		;
		for (symbol_scan = chain_scan->symbols; ((symbol_scan != NULL) && (symbol_scan->level >= level)); ) {
			register nel_symbol *temp = symbol_scan;
			symbol_scan = symbol_scan->next;
			temp->table = NULL;
			temp->next = NULL;
		}
		chain_scan->symbols = symbol_scan;
		nel_unlock (&(chain_scan->lock))
		;
	}

	nel_debug ({ nel_trace (eng, "] exiting nel_purge_table\n\n"); });
}


/*****************************************************************************/
/* nel_remove_NULL_values () purges <table> of all symbol which have a NULL   */
/* value field.  Should dealloc be non-nil, it is called with the            */
/* deallocated symbol as an argument to  return any purged symbols to a free */
/* list.                                                                     */
/*****************************************************************************/
void nel_remove_NULL_values (struct nel_eng *eng, nel_symbol_table *table, void (*dealloc)(struct nel_eng *, register nel_symbol *))
{
	register unsigned_int i;
	register unsigned_int max = table->max;
	register nel_symbol_chain *chain_scan;
	register nel_symbol *symbol_scan;
	register nel_symbol *prev;

	nel_debug ({ nel_trace (eng, "entering nel_remove_NULL_values [\ntable = 0x%x\n\n", table); });

	/*************************************************************/
	/* purge all symbols from <table> whose value field is NULL. */
	/*************************************************************/
	for (chain_scan = table->chains, i = 0; (i < max); chain_scan++, i++) {
		nel_lock (&(chain_scan->lock))
		;
		for (symbol_scan = chain_scan->symbols, prev = NULL; (symbol_scan != NULL); ) {
			if (symbol_scan->value == NULL) {
				nel_debug ({
							   nel_trace (eng, "NULL-valued symbol encountered: %I\n", symbol_scan);
						   });
				if (prev == NULL) {
					chain_scan->symbols = symbol_scan->next;
					symbol_scan->table = NULL;
					symbol_scan->next = NULL;
					if (dealloc != NULL) {
						(*dealloc) (eng, symbol_scan);
					}
					symbol_scan = chain_scan->symbols;
				} else {
					prev->next = symbol_scan->next;
					symbol_scan->table = NULL;
					symbol_scan->next = 0;
					if (dealloc != NULL) {
						(*dealloc) (eng, symbol_scan);
					}
					symbol_scan = prev->next;
				}
			} else {
				prev = symbol_scan;
				symbol_scan = symbol_scan->next;
			}
		}
		nel_unlock (&(chain_scan->lock))
		;
	}

	nel_debug ({ nel_trace (eng, "\n] exiting nel_remove_NULL_values\n\n"); });
}


/*****************************************************************************/
/* for every identifier encountered, a permanent symbol is created in the    */
/* static name hash table.  all subsequent instances of that identifier use  */
/* the same string.  nel_insert_name () checks to see if a string has         */
/* already been inserted in the static name table (and a corresponding       */
/* symbol in the static name hash table), creates new entries if none        */
/* currently exist, and returns a pointer to the string.  this save space    */
/* for allocating multiple instances of a string, and time when making       */
/* copies of symbols, as the name need not be copied also.                   */
/*****************************************************************************/
char *nel_insert_name (struct nel_eng *eng, register char *name)
{
	register nel_symbol *symbol;
	register char *retval;

	if ((symbol = nel_lookup_symbol (name, eng->nel_static_name_hash, NULL)) == NULL) {
		retval = nel_static_name_alloc (eng, name);
		symbol = nel_static_symbol_alloc (eng, retval, NULL, NULL, nel_C_NAME, 0, nel_L_NAME, 0);
		nel_insert_symbol (eng, symbol, eng->nel_static_name_hash);
	} else {
		retval = symbol->name;
	}
	return (retval);
}

void emit_symbol(FILE *fp, nel_symbol *symbol)
{

	if(symbol != NULL) {

		if(!symbol->type) {
			fprintf(fp,"%s", symbol->name);
			return;
		}

		switch(symbol->type->simple.type) {
		case nel_D_ARRAY: {
				nel_type *base_type = symbol->type->array.base_type;
				if(base_type->simple.type == nel_D_CHAR) {
					fprintf(fp, "\"%s\"", symbol->name);
				} else {
					fprintf(fp, "%s", (char *)symbol->name);
				}
			}
			break;

		case nel_D_FUNCTION: {
				int first_arg, nargs = 0;
				nel_list *scan;

				/* output func 's  return type */
				emit_type(fp, symbol->type->function.return_type);
				fprintf(fp, " %s(", symbol->name);

				/* output func 's param list */
				for(first_arg = 1, scan = symbol->type->function.args; scan;
						scan=scan->next, first_arg = 0) {
					if(!first_arg)
						fprintf(fp, ",");
					emit_symbol(fp, scan->symbol);
					nargs++;
				}
				fprintf(fp, ")\n{\n");

				/* output func 's body */
				emit_stmt(fp, (nel_stmt *)symbol->value, 1);
				fprintf(fp, "}\n\n");
			}
			break;


		case nel_D_STRUCT:
		case nel_D_STRUCT_TAG:
			fprintf(fp, "%s",symbol->name);
			break;

		case nel_D_SHORT:
		case nel_D_SHORT_INT:
		case nel_D_SIGNED:
		case nel_D_SIGNED_SHORT:
		case nel_D_SIGNED_SHORT_INT:
		case nel_D_UNSIGNED_SHORT:
		case nel_D_UNSIGNED_SHORT_INT:
			/* bugfix, wyong, 2006.4.26 */
			if(symbol->class == nel_C_CONST)
				fprintf(fp, "%d", (int) (*((short *)symbol->value)));
			else
				fprintf(fp, "%s",  symbol->name);
			break;


		case nel_D_INT:
		case nel_D_SIGNED_INT:
		case nel_D_UNSIGNED:
		case nel_D_UNSIGNED_INT:
			if(symbol->class == nel_C_CONST)
				fprintf(fp, "%d", *((int *)symbol->value));
			else
				fprintf(fp, "%s",  symbol->name);
			break;

		case nel_D_CHAR:
		case nel_D_SIGNED_CHAR:
		case nel_D_UNSIGNED_CHAR:
			if(symbol->class == nel_C_CONST)
				fprintf(fp, "%c", *((char *)symbol->value));
			else
				fprintf(fp, "%s",  symbol->name);
			break;

		case nel_D_LONG:
		case nel_D_LONG_INT:
		case nel_D_SIGNED_LONG:
		case nel_D_SIGNED_LONG_INT:
		case nel_D_UNSIGNED_LONG:
		case nel_D_UNSIGNED_LONG_INT:
			if(symbol->class == nel_C_CONST)
				fprintf(fp, "%ld", *((long *) symbol->value));
			else
				fprintf(fp, "%s",  symbol->name);
			break;

		case nel_D_FLOAT:
			if(symbol->class == nel_C_CONST)
				fprintf (fp, "%f", (double) (*((float *) symbol->value)));
			else
				fprintf(fp, "%s",  symbol->name);
			break;

		case nel_D_DOUBLE:
			if(symbol->class == nel_C_CONST)
				fprintf (fp, "%f", *((double *) symbol->value));
			else
				fprintf(fp, "%s",  symbol->name);
			break;

		case nel_D_POINTER:
			fprintf(fp, "%s",symbol->name);
			break;

		case nel_D_ENUM:
		case nel_D_ENUM_TAG:		/* tags have type descriptors		*/
			/* bug fix, wyong, 2004.6.29 */
			fprintf(fp, "%s",symbol->name);
			break;

		case nel_D_TERMINAL:
		case nel_D_NONTERMINAL:
		case nel_D_EVENT:
			fprintf(fp, " %s ", symbol->name);
			if(symbol->value) {
				fprintf(fp, "(");
				emit_expr(fp, (union nel_EXPR *) symbol->value);
				fprintf(fp, ")");
			}
			/*
			if(symbol->aux.event 
				&& symbol->aux.event->reachable > 0 ) {
				fprintf(fp, ", reacheable");
			}
			*/
			break;

		case nel_D_PRODUCTION: {
				struct nel_RHS *rhs;
				fprintf(fp,"%d. %s %s", symbol->id, symbol->type->prod.lhs->name, nel_Rel_name(symbol->type->prod.rel));
				for(rhs = symbol->type->prod.rhs; rhs; rhs = rhs->next) {
					emit_symbol(fp, rhs->symbol);
				}

				fprintf(fp, "\nAction:\n");
				emit_stmt(fp, (union nel_STMT *)symbol->type->prod.action, 0);
				fprintf(fp, ";\n");
			}
			break;

		case nel_D_NULL:
		case nel_D_COMMON:
		case nel_D_COMPLEX:		/* extension: basic complex type	*/
		case nel_D_COMPLEX_DOUBLE:	/* extension: ext. precision complex	*/
		case nel_D_COMPLEX_FLOAT:		/* extension: same as complex		*/
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
	}

	return ;

}


/*****************************************************************************/
/* nel_symbol_init () performs whatever initialization is necessary of the    */
/* static hash tables.  Space for the static name and symbol tables is       */
/* automatically allocated.  Space for the dyn hash, name, and symbol tables */
/* is allocated using alloca() in the top level routine.                     */
/*****************************************************************************/
void nel_symbol_init (struct nel_eng *eng)
{
	//register nel_symbol **scan;

	nel_debug ({ nel_trace (eng, "entering nel_symbol_init [\n\n"); });

	/**********************************************************************/
	/* space for the static name and symbol allocators will automatically */
	/* be allocated when nel_static_symbol_alloc is called the first time. */
	/**********************************************************************/

	/***************************************************/
	/* allocate space for the five static hash tables, */
	/* and initialize all their chains to NULL.        */
	/***************************************************/
	nel_static_hash_table_alloc (eng->nel_static_name_hash, nel_static_name_hash_max);
	nel_static_hash_table_alloc (eng->nel_static_ident_hash, nel_static_ident_hash_max);
	nel_static_hash_table_alloc (eng->nel_static_location_hash, nel_static_location_hash_max);
	nel_static_hash_table_alloc (eng->nel_static_tag_hash, nel_static_tag_hash_max);
	nel_static_hash_table_alloc (eng->nel_static_file_hash, nel_static_file_hash_max);

	/*************************************************************/
	/* now that we have initialized the hash tables (mainly,     */
	/* nel_static_name_hash), call nel_type_init() to form symbols */
	/* and type descriptors for all the predefined types.        */
	/*************************************************************/
	nel_type_init (eng);

	/****************************************************/
	/* allocate symbols for the integer values 0 and 1, */
	/* which are returned by boolean operations.        */
	/* nel_type_init() initialized nel t_type.          */
	/****************************************************/
	nel_zero_symbol = nel_static_symbol_alloc (eng, NULL, nel_int_type,
					  nel_static_value_alloc (eng, sizeof (int), nel_alignment_of (int)),
					  nel_C_CONST, 0, nel_L_NEL, 0);
	*((int *) (nel_zero_symbol->value)) = 0;

	nel_one_symbol = nel_static_symbol_alloc (eng, NULL, nel_int_type,
					 nel_static_value_alloc (eng, sizeof (int), nel_alignment_of (int)),
					 nel_C_CONST, 0, nel_L_NEL, 0);
	*((int *) (nel_one_symbol->value)) = 1;

	nel_debug ({ nel_trace (eng, "] exiting nel_symbol_init\n\n"); });

}

int evt_symbol_update_dollar(struct nel_eng *eng, nel_symbol *symbol, int pos, int value)
{
	int old_value =0;

	if(eng  && symbol->name != NULL && symbol->name[0] == '$'){
		sscanf(symbol->name, "$%d", &old_value);
		if(old_value < 0 
			/* if pos > 0, we do empty remove, therefore pos 
			equal to old_value is not allowed, wyong, 2006.9.15 */
			|| (pos > 0 && old_value == pos)) {
			/*
			gen_error(eng, nel_R_GENERATOR, NULL,0, 
				"incorrect position of symbol %s",symbol->name);
			*/
			return -1;
		}

		if(old_value > pos ) {
			sprintf(symbol->name, "$%d", old_value + value );
		}

		return 0;
	}

	return 0;
}

//added by zhangbin, 2006-7-19
void static_symbol_dealloc(struct nel_eng *eng)
{
	while(nel_static_symbol_chunks)
	{
		nel_static_symbol_chunk *chunk = nel_static_symbol_chunks->next;
		nel_dealloca(nel_static_symbol_chunks->start);
		nel_dealloca(nel_static_symbol_chunks); 
		nel_static_symbol_chunks = chunk;
	}
	nel_static_symbols_next = nel_static_symbols_end = nel_free_static_symbols = NULL;
}
//end


//added by zhangbin, 2006-7-19
void static_name_dealloc(struct nel_eng *eng)
{
	while(nel_static_name_chunks)
	{
		nel_static_name_chunk *chunk = nel_static_name_chunks->next;
		nel_dealloca(nel_static_name_chunks->start); 
		nel_dealloca(nel_static_name_chunks); 
		nel_static_name_chunks = chunk;
	}
	nel_static_names_next = nel_static_names_end = NULL;
}
//end


//added by zhangbin, 2006-7-19
void static_value_dealloc(struct nel_eng *eng)
{
	while(nel_static_value_chunks)
	{
		nel_static_value_chunk *chunk = nel_static_value_chunks->next;
		nel_dealloca(nel_static_value_chunks->start); 
		nel_dealloca(nel_static_value_chunks); 
		nel_static_value_chunks = chunk;
	}
	nel_static_values_next = nel_static_values_end = NULL;
}
//end
