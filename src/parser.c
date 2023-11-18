

/************************************************************************************/
/* This file, "parser.c", contains the main parser routine nel_file_parse()  for the Engine */
/************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <engine.h>
#include <errors.h>
#include <io.h>
#include <intp.h>
#include <parser.h>
#include <lex.h>
#include "mem.h"


#ifdef LICENSE
#include <license.h>
#endif


void parser_trace (struct nel_eng *eng, char *message, ...)
{
        if (eng != NULL && eng->parser_verbose_level >= NEL_TRACE_LEVEL ) {
                va_list args;
                va_start (args, message);

		/*******************************************/
		/* error message i/o is a critical section */
		/*******************************************/
		nel_lock (&nel_error_lock);

                nel_do_print (stderr, message, args);

		/*************************/
		/* exit critical section */
		/*************************/
		nel_unlock (&nel_error_lock);

                va_end (args);
        }
}


void parser_diagnostic(struct nel_eng *eng, char *message, ...)
{
	char buffer[4096];
	va_list args;

	if ( eng && (eng->parser_verbose_level >= NEL_DIAG_LEVEL )) {
		va_start (args, message);

		/*******************************************/
		/* error message i/o is a critical section */
		/*******************************************/
		nel_lock (&nel_error_lock);

		sprintf(buffer, message, args);
		write(2, buffer, strlen(buffer));
		fprintf (stderr, "\n");

		//nel_do_print (stdout, message, args);

		/*************************/
		/* exit critical section */
		/*************************/
		nel_unlock (&nel_error_lock);
		va_end (args);
	}

	return ;
}

/*****************************************************************************/
/* parser_warning () prints out an error message, together with the current  */
/* input filename and line number, and then returns without incrementing     */
/* eng->parser->error_ct.                                                    */
/*****************************************************************************/
int parser_warning (struct nel_eng *eng, char *message, ...)
{
	va_list args;

	if (eng && (eng->parser_verbose_level >= NEL_WARN_LEVEL ) ) {
		va_start (args, message);

		/*******************************************/
		/* error message i/o is a critical section */
		/*******************************************/
		nel_lock (&nel_error_lock);

		if (eng->parser->type != nel_R_NULL) {
			fprintf (stderr, "\"%s\", line %d: warning: ", eng->parser->filename, eng->parser->line);
		}
		else {
			fprintf (stderr, "warning: ");
		}

		nel_do_print (stderr, message, args);
		fprintf (stderr, "\n");

		/*************************/
		/* exit critical section */
		/*************************/
		nel_unlock (&nel_error_lock);

		va_end (args);
	}

	return (0);

}



/*****************************************************************************/
/* parser_error()prints out an error message, together with the current input*/
/* filename and line number, increments eng->parser->error_ct, and returns.  */
/*****************************************************************************/
int parser_error (struct nel_eng *eng, char *message, ...)
{
	va_list args;

	if ( eng && (eng->parser_verbose_level >= NEL_ERROR_LEVEL )) {
		va_start (args, message);

		/*******************************************/
		/* error message i/o is a critical section */
		/*******************************************/
		nel_lock (&nel_error_lock);

		if ((eng != NULL) 
			&& (eng->parser->type!= nel_R_NULL) 
			&& (eng->parser->filename != NULL)){
			fprintf (stderr, "\"%s\", line %d: ", eng->parser->filename, eng->parser->line);
		}
		nel_do_print (stderr, message, args);
		fprintf (stderr, "\n");

		/******************************************/
		/* exit critical section                  */
		/******************************************/
		nel_unlock (&nel_error_lock);
		va_end (args);
	}

	return (0);

}

/*****************************************************************************/
/* parser_stmt_error() prints out an error message, increments               */
/* eng->parser->error_ct, and nel_longjmps() back to the parser,	     */
/* which continues on to the next statement. 			             */
/*****************************************************************************/
int parser_stmt_error (struct nel_eng *eng, char *message, ...)
{
	va_list args;

	/* output error message first */
	if(eng != NULL && eng->parser_verbose_level >= NEL_ERROR_LEVEL ){
		va_start (args, message); 

		if ((eng->parser->type!= nel_R_NULL) && (eng->parser->filename!= NULL)){
			fprintf (stderr, "\"%s\", line %d: ", eng->parser->filename, eng->parser->line);
		}
                nel_do_print (stderr, message, args);
		fprintf (stderr, "\n");
		va_end (args);
	}

	eng->parser->error_ct++;

	/*******************************************************/
	/* now longjmp back to the parser, which will continue */
	/* with the next statement (or the next symbol table   */
	/* string), if the stmt_err_jmp is set.                */
	/*******************************************************/
	if ((eng != NULL) 
		&& (eng->parser->type != nel_R_NULL) 
		&& (eng->parser->stmt_err_jmp != NULL)) {
		nel_longjmp (eng, *(eng->parser->stmt_err_jmp), 1);
	}
	else {
		parser_fatal_error (eng, "(nel_stmt_error #1): stmt error jump not set",NULL);
	}

	
	return (0);
}

/*****************************************************************************/
/* parser_block_error () prints out an error message, increments eng->parser */
/* ->error_ct, and nel_longjmps() back to the parser, which continues */
/* after the end of the current block.  when scanning the stab strings, the  */
/* block error jump is set to abandon scanning and return.                   */
/*****************************************************************************/
int parser_block_error (struct nel_eng *eng, char *message, ...)
{
	
	va_list args;

	/* output error message first */
	if(eng != NULL && eng->parser_verbose_level >= NEL_ERROR_LEVEL ){
		va_start (args, message); 
		if ((eng->parser->type!= nel_R_NULL) && (eng->parser->filename!= NULL)){
			fprintf (stderr, "\"%s\", line %d: ", eng->parser->filename, eng->parser->line);
		}
                nel_do_print (stderr, message, args);
		fprintf (stderr, "\n");
		va_end (args);
	}

        /********************************************************/
        /* if the flag nel_save_stmt_err_jmp is true, then reset */
        /* nel_stmt_err_jmp to NULL, as the block error longjmp  */
        /* could pop the semantic stack below the point where   */
        /* the stmt error jump was saved.                       */
        /********************************************************/
        if (nel_save_stmt_err_jmp) {
                nel_stmt_err_jmp = NULL;
        }

        /*******************************************************/
        /* now longjmp back to the parser, which will continue */
        /* after the current block (or abandon scanning the    */
        /* symbol table), if the block_err_jmp is set.         */
        /*******************************************************/
        if ((eng != NULL) 
		&& ( eng->parser->type != nel_R_NULL) 
		&& ( eng->parser->block_err_jmp != NULL)) {
                nel_longjmp (eng, *(eng->parser->block_err_jmp), 1);
        }
        else {
		parser_fatal_error(eng, "(nel_block_error #1): block error jmp not set", NULL);

        }


	eng->parser->error_ct++;
	return (0);
}

/*****************************************************************************/
/* parser_fatal_error() prints out an error message, increments              */
/* eng->parser->error_ct, and exits the program.  it is called	*/ 
/* when an internal parser error is encountered.  if the global */ 
/* flag "nel_no_error_exit" is set, then parser_fatal_error()  */
/* nel_longjmps back to the top-level routine,  which returns  		*/
/* to the caller.                                              */
/*****************************************************************************/
int parser_fatal_error (struct nel_eng *eng, char *message, ...)
{
	va_list args;

	if ( eng != NULL &&  eng->parser_verbose_level >= NEL_FATAL_LEVEL ) {
		va_start (args, message);
		if ( (eng->parser->filename != NULL)){
			fprintf (stderr, "\"%s\", line %d: ", eng->parser->filename, eng->parser->line);
		}

		nel_do_print (stderr, message, args);
		fprintf (stderr, "\n");

		va_end (args);
	}

	eng->parser->error_ct++;

	/********************************************/
	/* if the flag nel_save_stmt_err_jmp is true */
	/* then reset nel_stmt_err_jmp to NULL.      */
	/********************************************/
	if (nel_save_stmt_err_jmp) {
		nel_stmt_err_jmp = NULL;
	}

	/***********************************************/
	/* we have a fatal error, so exit the program, */
	/* unless nel_no_error_exit is set.             */
	/***********************************************/
	if ((nel_no_error_exit) 
		&& (eng != NULL) 
		&& (eng->parser->type != nel_R_NULL) 
		&& (eng->parser->return_pt != NULL)) {
		/**************************************/
		/* exit the critical before returning */
		/* to the top-level routine.          */
		/**************************************/
		nel_unlock (&nel_error_lock);
		nel_longjmp (eng, *(eng->parser->return_pt), 1);
	}
	else {
		/********************************************/
		/* print an "exiting" message, exit the     */
		/* critical section, then exit the program. */
		/********************************************/
		fprintf (stderr, "exiting\n");

		nel_unlock (&nel_error_lock);
		exit (1);	//wyong, 2006.4.10 
	}

	return (0);
}



/******************************************************/
/* we check the stack for overflow on every push.     */
/* reuse the same error message to cut down on space. */
/******************************************************/
unsigned_int nel_semantic_stack_max = /* 0x1000 */ 256 ;
char nel_semantic_stack_overflow_message[] = "semantic stack overflow";


void parser_insert_ident(struct nel_eng *eng, nel_symbol *symbol)
{
	if((symbol->level <=0) 
		&& (symbol->class != nel_C_STATIC_GLOBAL) 
		&& (symbol->class != nel_C_COMPILED_STATIC_FUNCTION)
		&& (symbol->class != nel_C_NEL_STATIC_FUNCTION))  
		nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);
	else 
		nel_insert_symbol (eng, symbol, eng->parser->dyn_ident_hash);
	
}

void parser_remove_ident(struct nel_eng *eng,  nel_symbol *symbol)
{
	nel_remove_symbol(eng, symbol);
}

nel_symbol *parser_lookup_ident(struct nel_eng *eng, char *name)
{
	register nel_symbol *symbol = NULL;
	symbol = nel_lookup_symbol(name, eng->parser->dyn_ident_hash,
			   eng->nel_static_ident_hash,
			   eng->parser->dyn_location_hash,
			   eng->nel_static_location_hash, NULL);
	if(symbol == NULL){
		symbol = (nel_symbol *)lookup_event_symbol(eng, name);
	}

	return symbol;
}

/*****************************************************************************/
/* parser_dyn_symbol_alloc () inserts a symbol into *nel_dyn_symbols.  <name>     */
/* should reside in nel's name table - it is not automatically copied there.  */
/*   struct nel_eng *eng;						*/
/*   char *name;		identifier string			*/
/*   nel_type *type;	type descriptor				*/
/*   char *value;		points to memory where value is stored	*/
/*   nel_C_token class;	symbol class					*/
/*   unsigned_int lhs;	legal left hand side of an assignment	*/
/*   nel_L_token source_lang; source language				*/
/*   int level;		scoping level				*/
/*****************************************************************************/
nel_symbol *parser_dyn_symbol_alloc (struct nel_eng *eng, char *name, nel_type *type, char *value, nel_C_token class, unsigned_int lhs, nel_L_token source_lang, int level)
{
	register nel_symbol *retval;

	nel_debug ({ nel_trace (eng, "entering parser_dyn_symbol_alloc [\neng = 0x%x\nname = %s\ntype =\n%1Tvalue = 0x%x\nclass = %C\nlhs = %d\nsource_lang = %H\nlevel = %d\n\n", eng, (name == NULL) ? "NULL" : name, type, value, class, lhs, source_lang, level); });

	/***********************/
	/* longjmp on overflow */
	/***********************/
	if (eng->parser->dyn_symbols_next >= eng->parser->dyn_symbols_end)
	{
		parser_stmt_error (eng, "dynamic symbol overflow");
	}

	/**************************************************/
	/* allocate space, initialize members, and return */
	/**************************************************/
	retval = eng->parser->dyn_symbols_next++;
	retval->name = name;
	retval->type = type;
	retval->value = value;
	retval->class = class;
	retval->lhs = lhs;
	retval->source_lang = source_lang;
	retval->declared = 0;
	retval->level = level;
	retval->reg_no = -1;
	retval->member = NULL;
	retval->table = NULL;
	retval->next = NULL;
	retval->data = 0;
	retval->_global = NULL;

	nel_debug ({ nel_trace (eng, "] exiting parser_dyn_symbol_alloc\nretval =\n%1S\n", retval); });
	return (retval);
}

/*****************************************************************************/
/* parser_dyn_value_alloc() allocates a space of <bytes> bytes in the dynamic*/
/* value table on an alignment-byte boundary, and returns a pointer to it.   */
/*****************************************************************************/
char *parser_dyn_value_alloc (struct nel_eng *eng, register unsigned_int bytes, register unsigned_int alignment)
{
	register char *retval;

	nel_debug ({ nel_trace (eng, "entering parser_dyn_value_alloc [\neng = 0x%x\nbytes = 0x%x\nalignment = 0x%x\n\n", eng, bytes, alignment); });

	nel_debug ({
	if (bytes == 0) {
		parser_fatal_error (eng, "(parser_dyn_value_alloc #1): bytes = 0") ;
	}
	if (alignment == 0) {
		parser_fatal_error (eng, "(parser_dyn_value_alloc #2): alignment = 0");
	}
	});

	/*****************************************/
	/* we do not want memory fetches for two */
	/* different objects overlapping in the  */
	/* same word if the machine does not     */
	/* support byte-size loads and stores.   */
	/*****************************************/
	eng->parser->dyn_values_next = nel_align_ptr (eng->parser->dyn_values_next, nel_SMALLEST_MEM);

	/******************************************************/
	/* get a pointer to the start of the allocated space. */
	/* make sure it is on an properly-aligned boundary.   */
	/******************************************************/
	retval = (char *) eng->parser->dyn_values_next;
	retval = nel_align_ptr (retval, alignment);

	if (retval + bytes > eng->parser->dyn_values_end) {
		/***********************/
		/* longjmp on overflow */
		/***********************/
		parser_stmt_error (eng, "dynamic value table overflow");
		retval = NULL;
	} else {
		/*********************/
		/* reserve the space */
		/*********************/
		eng->parser->dyn_values_next = retval + bytes;
	}

	/*************************************************/
	/* zero out all elements of the allocated space  */
	/*************************************************/
	nel_zero (bytes, retval);
	nel_debug ( {
		nel_trace (eng, "] exiting parser_dyn_value_alloc\nretval = 0x%x\n\n", retval);
	});

	return (retval);
}

/*****************************************************************************/
/* parser_dyn_stack_dealloc() removes all of the dynamic symbols allocated   */
/* with new_index <= &symbol <eng->parser->dyn_symbols_next, and deallocates */
/* the memory allocated for the values, and the symbols themselves.  it is   */
/* possible that some of the deallocated symbols are still in hash tables,   */
/* so for each one, we have to check the inserted field and remove the       */
/* symbol from the tables it is in (if it is in any).                        */
/*****************************************************************************/
void parser_dyn_stack_dealloc (struct nel_eng *eng, char *dyn_values_next, nel_symbol *dyn_symbols_next)
{
	nel_symbol *scan;

	nel_debug ({ nel_trace (eng, "entering parser_dyn_stack_dealloc [\neng = 0x%x\ndyn_values_next = 0x%x\ndyn_symbols_next = 0x%x\n\n", eng, dyn_values_next, dyn_symbols_next); });

	for (scan = eng->parser->dyn_symbols_next - 1; (scan >= dyn_symbols_next); scan--) {
		if (scan->table != NULL) {
			nel_remove_symbol (eng, scan);
		}
	}
	eng->parser->dyn_values_next = dyn_values_next;
	eng->parser->dyn_symbols_next = dyn_symbols_next;

	nel_debug ({ nel_trace (eng, "] exiting parser_dyn_stack_dealloc\n\n"); });
}

/********************************************************************/
/* use the following macros to save and restore the values of the   */
/* various stacks in the nel_eng .  first push the indeces of the   */
/* value and symbol stacks, then push the old stmt_err_jmp and      */
/* allocate a new one, then push the old block_err_jmp and allocate */
/* a new one, then push the old context pointer, then save the      */
/* current stack pointer in eng->parser->context.  when restoring a     */
/* context, first purge the symbol tables of any symbols at the     */
/* current level (except for dyn_label_hash, since labels do not go */
/* out of scope when exiting any block, except for the outermost in */
/* a routine), then call parser_dyn_stack_dealloc to deallocate any     */
/* dynamically allocated symbols.                                   */
/* a new context does not necessarily correpond to an inner scoping */
/* level, so the level is not incremented/decremented with these    */
/* macros.                                                          */
/* we save a chain of pointers to the previous context, in case the */
/* semantic stack was corrupted in the previous context.            */
/********************************************************************/
void save_parser_context(struct nel_eng *_eng) 
{
	parser_push_value ((_eng), (_eng)->parser->dyn_values_next, char *);
	parser_push_value ((_eng), (_eng)->parser->dyn_symbols_next, nel_symbol *);
	parser_push_value ((_eng), (_eng)->parser->stmt_err_jmp, nel_jmp_buf *);
	(_eng)->parser->stmt_err_jmp = (nel_jmp_buf *) parser_dyn_value_alloc ((_eng), sizeof (nel_jmp_buf), nel_MAX_ALIGNMENT);
	parser_push_value ((_eng), (_eng)->parser->block_err_jmp, nel_jmp_buf *);
	(_eng)->parser->block_err_jmp = (nel_jmp_buf *) parser_dyn_value_alloc ((_eng), sizeof (nel_jmp_buf), nel_MAX_ALIGNMENT);
	parser_push_value ((_eng), (_eng)->parser->context, nel_stack *);
	parser_top_pointer ((_eng), (_eng)->parser->context, 0);
}

void restore_parser_context(struct nel_eng *_eng, int _level) 
{
	register int __level = (_level);
	parser_set_stack ((_eng), (_eng)->parser->context);
	parser_pop_value ((_eng), (_eng)->parser->context, nel_stack *);
	parser_pop_value ((_eng), (_eng)->parser->block_err_jmp, nel_jmp_buf *); 
	parser_pop_value ((_eng), (_eng)->parser->stmt_err_jmp, nel_jmp_buf *); 
	parser_pop_value ((_eng), (_eng)->parser->dyn_symbols_next, nel_symbol *);
	parser_pop_value ((_eng), (_eng)->parser->dyn_values_next, char *);
	nel_purge_table (_eng, __level + 1, (_eng)->parser->dyn_ident_hash);
	nel_purge_table (_eng, __level + 1, (_eng)->parser->dyn_location_hash);
	nel_purge_table (_eng, __level + 1, (_eng)->parser->dyn_tag_hash);
	parser_dyn_stack_dealloc ((_eng), (_eng)->parser->dyn_values_next, (_eng)->parser->dyn_symbols_next);
}

/*****************************************************************************/
/* parser_coerce coerces a data object of type <old_type> pointed to by      */
/* <old_value>, and coerces it to type <new_type>, storing the result in     */
/* <new_value>.                                                              */
/*****************************************************************************/
void parser_coerce (struct nel_eng *eng, nel_type *new_type, char *new_value, nel_type *old_type, char *old_value)
{
	register nel_D_token old_D_token;
	register nel_D_token new_D_token;
	int retval;

	nel_debug ({ nel_trace (eng, "entering parser_coerce [\neng = 0x%x\nnew_type = \n%1Tnew_value = 0x%x\nold_type =\n%1Told_value = 0x%x\n\n", eng, new_type, new_value, old_type, old_value); });

	nel_debug ({
		if ((new_type == NULL) || (new_value == NULL) || (old_type == NULL) || (old_value == NULL)) {
			parser_fatal_error (eng, "(parser_coerce #1): bad arguments for parser_coerce\nnew_type = \n%1Tnew_value = 0x%x\nold_type = \n%1Told_value = 0x%x", new_type, new_value, old_type, old_value);
		}
	});

	if (nel_coerce(eng, new_type, new_value, old_type, old_value) < 0 ) {
		parser_stmt_error (eng, "illegal type coercion");
	}

	nel_debug ({ nel_trace (eng, "] exiting parser_coerce\n\n"); });
	return ;

}

/*****************************************************************************/
/* parser_resolve_type() traverses the type descriptor pointed to by <type>. */
/* if any arrays are found with variables in their dimension bounds, the     */
/* of the bound(s) are extracted and a new type descriptor formed with       */
/* integral constants substituted for the expressions.  (a pointer to) the   */
/* new type descriptor is returned.  <name> is used for error messages.      */
/*****************************************************************************/
nel_type *parser_resolve_type (struct nel_eng *eng, register nel_type *type, char *name)
{
	register nel_type *retval;

	nel_debug ({ nel_trace (eng, "entering parser_resolve_type [\neng = 0x%x\ntype =\n%1T\n", eng, type); });

	if (type == NULL) {
		return NULL;
	} 


	switch (type->simple.type) {
	case nel_D_ARRAY: 

#if 0
		/* do nothing here , wyong, 2006.4.30 */
		{
		int lb;
		int ub;

		/***************************/
		/* extract the lower bound */
		/***************************/
		if (type->array.known_lb) {
			lb = type->array.lb.value;
		} else {
			register nel_expr *bound_expr;
			register nel_symbol *bound_sym = NULL;
			
			if ((bound_expr = type->array.lb.expr ) != NULL ){
				if(bound_expr->gen.opcode  !=  nel_O_SYMBOL )	{
					parser_fatal_error(eng, "found expression in array declaration!\n");
				}	
		
				bound_sym = bound_expr->symbol.symbol;
				if (bound_sym != NULL) {
					bound_sym = parser_lookup_ident (eng, bound_sym->name);
				}
			}

			if ((bound_sym != NULL) && (bound_sym->type != NULL) && (bound_sym->value != NULL) && (nel_integral_D_token (bound_sym->type->simple.type))) {
				parser_coerce (eng, nel_int_type, (char *)&lb, bound_sym->type, (char *)bound_sym->value);

			} else {
				lb = 0;
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
			}
		}

		/***************************/
		/* extract the upper bound */
		/***************************/
		if (type->array.known_ub) {
			ub = type->array.ub.value;
		} else {

			register nel_expr *bound_expr;
			register nel_symbol *bound_sym = NULL;
			

			if((bound_expr = type->array.ub.expr ) != NULL ){

#if 0
				if(bound_expr->gen.opcode != nel_O_SYMBOL){
					parser_fatal_error(eng, "found expression in array declaration!\n");
				}
	
				bound_sym = bound_expr->symbol.symbol;
				if (bound_sym != NULL) {
					bound_sym = parser_lookup_ident (eng, bound_sym->name);
				}

#else

				/* wyong, 2006.4.30 */
				bound_sym =  intp_eval_expr_2(eng, bound_expr);

#endif
			}	

			if ((bound_sym != NULL) && (bound_sym->type != NULL) && (bound_sym->value != NULL) && (nel_integral_D_token (bound_sym->type->simple.type))) {
				parser_coerce (eng, nel_int_type, (char *)&ub, bound_sym->type, (char *)bound_sym->value);
				nel_debug ({
					nel_trace (eng, "bound_sym =\n%1Svalue = 0x%x\n*value = 0x%x\n\n", bound_sym, bound_sym->value, *((int *) (bound_sym->value)));
				});
			} else {
				ub = lb;
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
		retval = parser_resolve_type (eng, type->array.base_type, name);
		if (retval != NULL) {
			retval = nel_type_alloc (eng, nel_D_ARRAY, 
				(ub - lb + 1 ) * retval->simple.size, 
				retval->simple.alignment, 
					type->array._const, 
					type->array._volatile, 
				retval, 
				nel_int_type, 1, lb, 1, ub);
		}
		}
#else
		retval = type;

#endif

		break;

	case nel_D_FUNCTION:
		retval = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, type->function._const, type->function._volatile, type->function.new_style, type->function.var_args, parser_resolve_type (eng, type->function.return_type, name), type->function.args, type->function.blocks, type->function.file );
		break;

	case nel_D_POINTER:
		retval = nel_type_alloc (eng, nel_D_POINTER, sizeof (char *), nel_alignment_of (char *), type->pointer._const, type->pointer._volatile, parser_resolve_type (eng, type->pointer.deref_type, name));
		break;

	case nel_D_STRUCT:
	case nel_D_UNION:
		/* wyong, 2005.6.2 */
#if 1
		/****************************************************/
		/* struct/unions members must be defined statically */
		/****************************************************/
		retval = type;
		if(type->s_u.tag != NULL && type->s_u.members == NULL ) {
			parser_warning(eng, "unresolved type(%s)", type->s_u.tag->name);
		}
		break;

#else
		{
		register nel_member *scan;
		register nel_member *start = NULL;
		register nel_member *last = NULL;

		if(type->s_u.members == NULL ) {
			parser_warning (eng, "unresolved type(%s)", type->s_u.tag->name);
		}


		for (scan=type->s_u.members; (scan != NULL); scan = scan->next)
		{
			register nel_symbol *symbol = scan->symbol;
			register nel_member *member;
			symbol = nel_static_symbol_alloc (eng, symbol->name, parser_resolve_type (eng, symbol->type, symbol->name), symbol->value, symbol->class, symbol->lhs, symbol->source_lang, symbol->level);
			member = nel_member_alloc (eng, symbol, scan->bit_field, scan->bit_size, scan->bit_lb, scan->offset, NULL);
			symbol->member = member;
			if (last == NULL) {
				start = last = member;
			} else {
				last->next = member;
				last = member;
			}
		}
		retval = nel_type_alloc (eng, type->s_u.type, type->s_u.size, type->s_u.alignment, type->s_u._const, type->s_u._volatile, type->s_u.tag, start);
		if (type->simple.type == nel_D_STRUCT)
		{
			nel_set_struct_offsets (eng, retval);
		} else /* (type->simple.type == nel_D_UNION) */
		{
			unsigned_int alignment;
			retval->s_u.size = nel_s_u_size (eng, &alignment, retval->s_u.members);
			retval->s_u.alignment = alignment;
		}
		}
		break;
#endif /* 0 */

		/****************/
		/* simple types */
		/****************/
	default:
		retval = type;
		break;
	}

	nel_debug ({ nel_trace (eng, "] exiting parser_resolve_type\nretval =\n%1T\n", retval); });
	return (retval);
}

/**********************************************************************/
/* use the following macro to allocate space for the parser variant     */
/* of an nel_eng structure and initialize the appropriate fields. */
/**********************************************************************/
int parser_init(struct nel_eng *_eng, char *_filename, FILE *_infile, char *_retval_loc, va_list *_formal_ap, unsigned_int _declare_mode) 
{								
	//register char *__filename = (_filename);			
	nel_debug ({							
	if ((_eng) == NULL) {					
		parser_fatal_error (NULL, "(nel_parser_init #1): eng == NULL"); 
	}								
	});								

	//if((_eng)->nel_parser_init_flag == 0 ){
	(_eng)->parser->type = nel_R_PARSER;				
	if (_filename != NULL) {					
		(_eng)->parser->filename = nel_insert_name (_eng, _filename);	
	}								
	else {							
		(_eng)->parser->filename = NULL;				
	}								
	(_eng)->parser->line = 1;					
	(_eng)->parser->error_ct = 0;					
	(_eng)->parser->tmp_int = 0;					
	(_eng)->parser->tmp_err_jmp = NULL;				
	(_eng)->parser->return_pt = NULL;				
	(_eng)->parser->stmt_err_jmp = NULL;				
	(_eng)->parser->block_err_jmp = NULL;				
	nel_stack_alloc ((_eng)->parser->semantic_stack_start,(_eng)->parser->semantic_stack_next,(_eng)->parser->semantic_stack_end, nel_semantic_stack_max);							
	(_eng)->parser->level = 0;					
	(_eng)->parser->prog_unit = NULL;				
	(_eng)->parser->block_no = 0;					
	(_eng)->parser->block_ct = 0;					
	(_eng)->parser->clean_flag = 0;					
	nel_dyn_allocator_alloc ((_eng)->parser->dyn_values_start,	(_eng)->parser->dyn_values_next, (_eng)->parser->dyn_values_end, nel_dyn_values_max, char);
	
	nel_dyn_allocator_alloc ((_eng)->parser->dyn_symbols_start,(_eng)->parser->dyn_symbols_next, (_eng)->parser->dyn_symbols_end,nel_dyn_symbols_max, struct nel_SYMBOL);			
	
	nel_dyn_hash_table_alloc((_eng)->parser->dyn_ident_hash , nel_dyn_ident_hash_max);
	nel_dyn_hash_table_alloc ((_eng)->parser->dyn_location_hash, nel_dyn_location_hash_max); 
	nel_dyn_hash_table_alloc ((_eng)->parser->dyn_tag_hash, nel_dyn_tag_hash_max); 
	nel_dyn_hash_table_alloc ((_eng)->parser->dyn_label_hash , nel_dyn_label_hash_max); 

	(_eng)->parser->context = NULL;					
	(_eng)->parser->retval_loc = (_retval_loc);			
	(_eng)->parser->formal_ap = (_formal_ap);			
	(_eng)->parser->declare_mode = (_declare_mode);			
	nel_alloca ((_eng)->parser->unit_prefix, nel_MAX_SYMBOL_LENGTH, char); 
	nel_make_file_prefix ((_eng), (_eng)->parser->unit_prefix,	
	(_eng)->parser->filename, nel_MAX_SYMBOL_LENGTH);			
	(_eng)->parser->stmts = NULL;					
	(_eng)->parser->append_pt = &((_eng)->parser->stmts);		
	(_eng)->parser->break_target = NULL;				
	(_eng)->parser->continue_target = NULL;				
	if ((_infile) != NULL) {					
		nel_parser_lex_init ((_eng), (_infile));	   		
	}								
	else {							
		(_eng)->parser->infile = NULL;				
	}								

	/* the rec has been initialized */
	//(_eng)->nel_parser_init_flag = 1;
	//}

	return 0;

}


void parser_dealloc(struct nel_eng *_eng)
{								
	//int i, n;

	if( (_eng) != NULL ) {

		//if((_eng)->nel_parser_init_flag == 1) {
		if ((_eng)->parser->infile != NULL) {				
			nel_parser_lex_dealloc ((_eng));
		}							

		nel_dealloca ((_eng)->parser->unit_prefix);		
		nel_purge_table (_eng, 0, (_eng)->parser->dyn_ident_hash);
		nel_purge_table (_eng, 0, (_eng)->parser->dyn_location_hash);	
		nel_purge_table (_eng, 0, (_eng)->parser->dyn_tag_hash);		
		nel_purge_table (_eng, 0, (_eng)->parser->dyn_label_hash);		
		nel_dyn_hash_table_dealloc ((_eng)->parser->dyn_label_hash);	
		nel_dyn_hash_table_dealloc ((_eng)->parser->dyn_tag_hash);	
		nel_dyn_hash_table_dealloc ((_eng)->parser->dyn_location_hash);	
		nel_dyn_hash_table_dealloc ((_eng)->parser->dyn_ident_hash);	
		nel_dyn_allocator_dealloc ((_eng)->parser->dyn_symbols_start);	
		nel_dyn_allocator_dealloc ((_eng)->parser->dyn_values_start);	
		nel_stack_dealloc ((_eng)->parser->semantic_stack_start);	

		//(_eng)->nel_parser_init_flag = 0;
		
		//added by zhangbin, 2006-7-20
		//nel_dealloca(_eng->parser);
		//end
	}

}


/******************************************************************************/
/* parser_eval () initializes the parser of engine , and call nel_parse    */
/* do real work */
/******************************************************************************/
int parser_eval (struct nel_eng *eng, char *filename, char *retval_loc, va_list *formal_ap )
{
	/********************************************/
	/* register variables are not guaranteed to */
	/* survive a longjmp () on some systems     */
	/********************************************/

	nel_jmp_buf return_pt;
	int val;
	struct eng_gram *old_parser=NULL;
	int retval;

	char abs_fname[MAXPATHLEN];
	char cwd[MAXPATHLEN];
	FILE *file;

	nel_debug ({ nel_trace (eng, "entering parser_eval () [\nfilename = %s\nretval_loc = 0x%x\nformal_ap = 0x%x\n\n", filename, retval_loc, formal_ap); });

	/*******************************/
	/* check for a NULL input file */
	/*******************************/
	if(!filename){
		fprintf(stderr, "NULL file passed to nel_fp ()");
		return -1;
	}

	if( eng->parser != NULL) {
		old_parser = eng->parser;
		strncpy(cwd, old_parser->filename, MAXPATHLEN);
	}else {
		if(!getcwd(cwd, MAXPATHLEN)){
			parser_error (eng, "cann't get current directory.\n");
			return -1;
		}

	}

	if(!rel2abs(filename, cwd, abs_fname,MAXPATHLEN)){
		fprintf(stderr, "cann't open file %s", abs_fname);
		return -1;
	} 
	
	//added by zhangbin, 2006-6-2
	if(!strcmp(eng->parser->filename, abs_fname))
		parser_stmt_error(eng, "include %s itself", filename);
	//end

#ifdef LICENSE 
	if(check_rule(abs_fname) < 0 ){
		fprintf(stderr, "%s has been modified!\n", abs_fname);
		return -1;
	}
#endif

	file = fopen(abs_fname, "r");
	if (file == NULL) {
		fprintf(stderr, "can't open file%s\n", abs_fname);
		return -1;
	}
 
	/*********************************************/
	/* initialize the engine that is passed */
	/* thoughout the entire call sequence.       */
	/*********************************************/
	nel_malloc(eng->parser, 1, struct eng_gram); 
	parser_init(eng, abs_fname, file, retval_loc, formal_ap, 0);

	/******************************************/
	/* perform a setjmp for the return point, */
	/* and call the interpreter               */
	/******************************************/
	eng->parser->return_pt = &return_pt;
	parser_setjmp (eng, val, &return_pt);
	if (val == 0) {
		if (parser_eval_yy(eng) != 0 ) { 
			nel_debug ({ nel_trace (eng, "] parser_eval error \nerror_ct = %d\n\n", eng->parser->error_ct); });
			fclose(file);	/* wyong, 2006.10.14 */
			return -1;
		}
	}

	/*******************************************************/
	/* deallocate any dynamically allocated substructs     */
	/* withing the engine (if they were stack allocated) */
	/* and return.                                         */
	/*******************************************************/
	retval = eng->parser->error_ct;
	parser_dealloc(eng);
	fclose(file);	/* wyong, 2006.10.14 */

	nel_debug ({ nel_trace (eng, "] exiting parser_eval ()\nerror_ct = %d\n\n", eng->parser->error_ct); });
	if(old_parser != NULL ) {
		nel_dealloca(eng->parser);	//added by zhangbin, 2006-7-20
		eng->parser = old_parser;
	}

	return 0;
}

//xiayu 2006.1.19 add syntax checking
int do_nel_file_parse(struct nel_eng *eng, char *filename)
{
	nel_jmp_buf return_pt;
	int val;
	int retval;

	char abs_fname[MAXPATHLEN];
	char cwd[MAXPATHLEN];
	FILE *file;


	/*******************************/
	/* check for a NULL input file */
	/*******************************/
	if(!filename){
		fprintf(stderr, "NULL file passed to nel_fp ()");
		return -1;
	}
	//printf("do_nel_file_parse(10), filename = %s\n", filename ) ; 

	if(!getcwd(cwd, MAXPATHLEN)){
		parser_error (eng, "cann't get current directory.\n");
		return -1;
	}

	//added by zhangbin, 2006-6-9
	cwd[strlen(cwd)+1]='\0';
	cwd[strlen(cwd)]='/';
	//end

	//printf("do_nel_file_parse(20), cwd = %s\n", cwd ) ; 

	if(!rel2abs(filename, cwd, abs_fname,MAXPATHLEN)){
		fprintf(stderr, "cann't open file %s", abs_fname);
		return -1;
	} 


#ifdef LICENSE 
	if(check_rule(abs_fname) < 0 ){
		fprintf(stderr, "%s has been modified!\n", abs_fname);
		return -1;
	}
#endif

	file = fopen(abs_fname, "r");
	if (file == NULL) {
		fprintf(stderr, "can't open file%s\n", abs_fname);
		return -1;
	}
 
	/*********************************************/
	/* initialize the engine that is passed */
	/* thoughout the entire call sequence.       */
	/*********************************************/
	nel_malloc(eng->parser, 1, struct eng_gram); 
	parser_init(eng, abs_fname, file, NULL, NULL, 0);

	/******************************************/
	/* perform a setjmp for the return point, */
	/* and call the interpreter               */
	/******************************************/
	eng->parser->return_pt = &return_pt;
	parser_setjmp (eng, val, &return_pt);
	if (val == 0) {
		if ( parser_eval_yy(eng) != 0 ) { 
			nel_debug ({ nel_trace (eng, "] parser_eval error \nerror_ct = %d\n\n", eng->parser->error_ct); });
			//added by zhangbin, 2006-10-11
			fclose(file);
			//end
			return -1;
		}
	}

	/*******************************************************/
	/* deallocate any dynamically allocated substructs     */
	/* withing the engine (if they were stack allocated) */
	/* and return.                                         */
	/*******************************************************/
	retval = eng->parser->error_ct;
	parser_dealloc(eng);

	nel_debug ({ nel_trace (eng, "] exiting parser_eval ()\nerror_ct = %d\n\n", eng->parser->error_ct); });

	//added by zhangbin, 2006-10-11
	fclose(file);
	//end
	return retval;
}

int nel_file_parse(struct nel_eng *eng, int argc, char **argv)
{
	int retval;
	int i;

	nel_debug ({ 
		nel_trace(eng, "entering nel_file_parse()\n"); 
	});

	//added by zhangbin, 2006-7-26
	eng->nel_file_num = argc;
	eng->nel_file_name = argv;
	//end
	
	for (i=0; i<argc; i++) {
		if( (retval = do_nel_file_parse(eng, argv[i])) != 0 ){
			nel_debug ({ nel_trace (eng, "can't parse file %s", argv[i]); });
			return retval; /* wyong, 2006.4.10 */ 
		}
	}

	//added bu zhangbin, 2006-5-15
	if(eng->peek_arg_link_begin)
		PrintPeekInfo(eng);
	//end
	
	
	if(eng->init_head != NULL && eng->compile_level != 1 ) {
		if ((retval = intp_eval_stmt_2 (eng, eng->init_head)) != 0 ) {
			nel_debug ({ nel_trace (eng, "can't execute init"); });
			return retval;
		}
	}

	if ( (retval = nel_generator(eng)) != 0) { 
		nel_debug ({ nel_trace (eng, "] nel_generator error\n"); });
		return retval;		/* wyong, 2006.4.10 */
	}
	
	if(eng->main != NULL && eng->compile_level != 1 ) {
		if ((retval = intp_eval_stmt_2 (eng, eng->main)) != 0 ) {
			nel_debug ({ nel_trace (eng, "can't execute main"); });
			return retval;
		}
	}

	nel_debug ({ nel_trace (eng, "] exiting nel_file_parse()\nerror_ct = %d\n\n", eng->parser->error_ct); });

	return 0;
	
}
