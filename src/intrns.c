
/*****************************************************************************/
/* This file, "intrns.c", contains the intrinsic functions called by  */
/* the interpreter.  An intrinsic is declared as:                            */
/*                                                                           */
/* nel_symbol *nel_ntrn_<name> (eng, func, nargs, arg_start)              */
/*    struct nel_eng *eng;                                             */
/*    register nel_symbol *func;                                              */
/*    register int nargs;                                                    */
/*    register nel_stack *arg_start;                                          */
/*                                                                           */
/* The return value of an intrinsic should be the symbol for the result.     */
/* All other needed data is contained in *eng.                               */
/*****************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include <stdarg.h>
#include <setjmp.h>

#include "engine.h"
#include "errors.h"
#include "type.h"
#include "intp.h"
#include "lex.h"
#include "io.h"
#include "evt.h"
#include "intrns.h"
#include "gen.h"
#include "mem.h" 

#if 0
/*****************************************************************************/
/* nel_ntrn_getrec () returns (a pointer to) a symbol whose value field  */
/* contains a pointer to the current nel_eng , <rec>.                     */
/*****************************************************************************/
nel_symbol *nel_ntrn_getrec (struct nel_eng *eng, register nel_symbol *func, register int nargs, register nel_stack *arg_start)
{
	register nel_symbol *retval;

	nel_debug ({ nel_trace (eng, "entering nel_ntrn_getrec [\neng = 0x%x\narg_ct = %d\n\n", eng, nargs); });

	/***********************/
	/* check argument list */
	/***********************/
	if (nargs != 0) {
		intp_stmt_error (eng, "usage: nel_eng *eng = geteng()");
	}

	nel_debug ({
		if (func->type->simple.type != nel_D_FUNCTION) {
			intp_fatal_error (eng, "(nel_ntrn_getrec #1): bad func\n%S", func);
		}
	});

	/***********************************************/
	/* allocate a symbol for the result, and set   */
	/* its value to point to the current nel_eng . */
	/***********************************************/
	retval = intp_dyn_symbol_alloc (eng, NULL, 
		func->type->function.return_type,
		intp_dyn_value_alloc (eng, sizeof (char *), nel_alignment_of (char *)),
		nel_C_RESULT, 0, nel_L_NEL, eng->intp->level);

	*((struct nel_eng **) (retval->value)) = eng;
	nel_debug ({ nel_trace (eng, "] exiting nel_ntrn_getrec\nretval = \n%S\n", retval); });
	return (retval);
}
#endif


void call_nel_do_print (char *fmt, ... ) {
	va_list argptr;
	va_start(argptr,fmt);
	nel_do_print (stdout, fmt, argptr);
	va_end(argptr);
}

/*****************************************************************************/
/* nel_ntrn_print () pretty prints its arguments.                            */
/* wyong, 20230908 							     */
/*****************************************************************************/
nel_symbol *nel_ntrn_print (struct nel_eng *eng, register nel_symbol *func, register int nargs, register nel_stack *arg_start)
{
	nel_type *type ; 
        nel_symbol *symbol = nel_lookup_symbol("call_nel_do_print", eng->nel_static_ident_hash, eng->nel_static_location_hash,  NULL);
	if ( (symbol == NULL ) 
		|| (symbol->name == NULL)
		|| (strcmp(symbol->name, "call_nel_do_print" ) != 0 )
		|| (symbol->class != nel_C_COMPILED_FUNCTION ) 
		|| ((type=symbol->type) == NULL) 
		|| (type->simple.type != nel_D_FUNCTION )
			|| (symbol->value == NULL ) 
			|| (symbol->value != (char *) call_nel_do_print)) {
		intp_error (eng, "call_nel_do_print not found or not properly initialized!");
	}

	return intp_eval_call(eng, symbol, nargs, arg_start ) ; 
}


nel_symbol *nel_ntrn_ridof (struct nel_eng *eng, register nel_symbol *func, register int nargs, register nel_stack *arg_start)
{
	register nel_symbol *retval;
	register nel_symbol *symbol;
	int rid = 0;

	nel_debug ({ nel_trace (eng, "entering nel_ntrn_idof[\neng = 0x%x\narg_ct = %d\n\n", eng, nargs); });

	/***********************/
	/* check argument list */
	/***********************/
	if (nargs != 0) {
		intp_stmt_error (eng, "usage: int rid = ridof()");
	}

	nel_debug ({
	if (func->type->simple.type != nel_D_FUNCTION) {
		intp_fatal_error (eng, "(nel_ntrn_idof#1): bad func\n%S", func);
	}
	});

	/**************************************************/
	/* allocate a symbol for the result, and set its  */
	/* value to point to the symbol for the argument. */
	/**************************************************/
	retval= intp_dyn_symbol_alloc (eng, NULL, func->type->function.return_type, intp_dyn_value_alloc (eng, sizeof (char *), nel_alignment_of (char *)), nel_C_RESULT, 0, nel_L_NEL, eng->intp->level);


#if 0
	symbol = arg_start->symbol;

	//symbol = lookup_event_symbol(eng, (char *)arg_start->symbol->value);
	if(symbol) {
		*((int *) (retval->value)) =
		(symbol->class == nel_C_TERMINAL 
			|| symbol->class == nel_C_NONTERMINAL) 
			? ((symbol->_reachable == 0) ? 0 
				:encodeSymbolId(eng, symbol->id,symbol->class))
			: symbol->id;
	} else {
		*((int *) (retval->value)) = 0;
	}
#else
	if(eng->intp->filename != NULL) {
		char *name;
		if((name = rindex(eng->intp->filename, '/')) != NULL){
			name ++;	
			rid = atoi(name);
		}
	}
	*((int *) (retval->value)) = rid;

#endif

	nel_debug ({ nel_trace (eng, "] exiting nel_ntrn_idof\nretval = \n%S\n", retval); });
	return (retval);
}

/*****************************************************************************/
/* nel_ntrn_symof () returns (a pointer to) the symbol representing   */
/* its argument.                                                             */
/*****************************************************************************/
nel_symbol *nel_ntrn_symof (struct nel_eng *eng, register nel_symbol *func, register int nargs, register nel_stack *arg_start)
{
	register nel_symbol *retval;

	nel_debug ({ nel_trace (eng, "entering nel_ntrn_symof [\neng = 0x%x\narg_ct = %d\n\n", eng, nargs); });

	/***********************/
	/* check argument list */
	/***********************/
	if (nargs != 1) {
		intp_stmt_error (eng, "usage: nel_symbol *symbol = symof(<expr>)");
	}

	nel_debug ({
	if (func->type->simple.type != nel_D_FUNCTION) {
		intp_fatal_error (eng, "(nel_ntrn_symof #1): bad func\n%S", func);
	}
	});

	/**************************************************/
	/* allocate a symbol for the result, and set its  */
	/* value to point to the symbol for the argument. */
	/**************************************************/
	retval = intp_dyn_symbol_alloc (eng, NULL, 
		func->type->function.return_type,
		intp_dyn_value_alloc (eng, sizeof (char *), nel_alignment_of (char *)),
		nel_C_RESULT, 0, nel_L_NEL, eng->intp->level);
	*((nel_symbol **) (retval->value)) = arg_start->symbol;
	nel_debug ({ nel_trace (eng, "] exiting nel_ntrn_symof\nretval = \n%S\n", retval); });

	return (retval);
}


nel_symbol *nel_ntrn_idof (struct nel_eng *eng, register nel_symbol *func, register int nargs, register nel_stack *arg_start)
{
	register nel_symbol *retval;
	register nel_symbol *symbol;

	nel_debug ({ nel_trace (eng, "entering nel_ntrn_idof[\neng = 0x%x\narg_ct = %d\n\n", eng, nargs); });

	/***********************/
	/* check argument list */
	/***********************/
	if (nargs != 1) {
		intp_stmt_error (eng, "usage: int id = idof((nel_symbol *)symbol)");
	}

	nel_debug ({
	if (func->type->simple.type != nel_D_FUNCTION) {
		intp_fatal_error (eng, "(nel_ntrn_idof#1): bad func\n%S", func);
	}
	if(arg_start->symbol->name == NULL ){
		intp_fatal_error (eng, "(nel_ntrn_idof#2): null name\n" );
	}
	});


	/**************************************************/
	/* allocate a symbol for the result, and set its  */
	/* value to point to the symbol for the argument. */
	/**************************************************/
	retval= intp_dyn_symbol_alloc (eng, NULL, func->type->function.return_type, intp_dyn_value_alloc (eng, sizeof (char *), nel_alignment_of (char *)), nel_C_RESULT, 0, nel_L_NEL, eng->intp->level);

	/*bugfix for event/event-decl-10.nel, wyong, 2006.6.1 */
	symbol = lookup_event_symbol(eng, (char *)arg_start->symbol->name);
	if(symbol) {
		*((int *) (retval->value)) =
		(symbol->class == nel_C_TERMINAL 
			|| symbol->class == nel_C_NONTERMINAL) 
			? ((symbol->_reachable == 0) ? 0 
				:encodeSymbolId(eng, symbol->id,symbol->class))
			: 0 /*symbol->id wyong, 2006.6.20 */ ;
	} else {
		intp_fatal_error (eng, "undeclaration of event %s", (char *)arg_start->symbol->name);
		*((int *) (retval->value)) = 0;
	}


	nel_debug ({ nel_trace (eng, "] exiting nel_ntrn_idof\nretval = \n%S\n", retval); });
	return (retval);
}

#if 0
/*****************************************************************************/
/* nel_ntrn_new() call malloc to create memory,    wyong, 2005.10.27 */
/*****************************************************************************/
nel_symbol *nel_ntrn_new(struct nel_eng *eng, register nel_symbol *func, register int nargs, register nel_stack *arg_start)
{
	register nel_symbol *retval;
	int size ;

	
	nel_debug ({ 
		nel_trace(eng, "entering nel_ntrn_new[\neng = 0x%x\narg_ct = %d\n\n", eng, nargs); 
	});

	/***********************/
	/* check argument list */
	/***********************/
	if (nargs != 1) {
		intp_stmt_error (eng, "usage: void *p = new(size)");
	}

	nel_debug ({
	if (func->type->simple.type != nel_D_FUNCTION) {
		intp_fatal_error (eng, "(nel_ntrn_new#1): bad func\n%S", func);
	}
	});

	/**************************************************/
	/* allocate a symbol for the result, and set its  */
	/* value to point to the symbol for the argument. */
	/**************************************************/
	retval = intp_dyn_symbol_alloc (eng, NULL, 
		func->type->function.return_type,
		intp_dyn_value_alloc (eng, sizeof (char *), nel_alignment_of (char *)),
		nel_C_RESULT, 0, nel_L_NEL, eng->intp->level);
	size = *((int *)arg_start->symbol->value);
	if(size <= 0 ){
		intp_stmt_error (eng, "(nel_ntrn_new#2): size =%d\n", size);
	}else{
		//modified by zhangbin, 2006-7-17, malloc=>nel_malloc
#if 1
		nel_malloc(retval->value, size, char);
#else
		*((void **) (retval->value)) = malloc(size);
#endif
		//end modified, 2006-7-17
	}

	nel_debug ({ nel_trace (eng, "] exiting nel_ntrn_new\nretval = \n%S\n", retval); });
	return (retval);
}
#endif

#if 0
/*****************************************************************************/
/* nel_ntrn_delete() free memory created by nel_ntrn_new, wyong, 2005.10.27  */
/*****************************************************************************/
nel_symbol *nel_ntrn_delete(struct nel_eng *eng, register nel_symbol *func, register int nargs, register nel_stack *arg_start)
{
	register nel_symbol *retval;
	void *data;

	
	nel_debug ({ 
		nel_trace(eng, "entering nel_ntrn_delete[\neng = 0x%x\narg_ct = %d\n\n", eng, nargs); 
	});

	/***********************/
	/* check argument list */
	/***********************/
	if (nargs != 1) {
		intp_stmt_error (eng, "usage: delete(p)");
	}

	nel_debug ({
	if (func->type->simple.type != nel_D_FUNCTION) {
		intp_fatal_error (eng, "(nel_ntrn_new#1): bad func\n%S", func);
	}
	});

	/**************************************************/
	/* allocate a symbol for the result, and set its  */
	/* value to point to the symbol for the argument. */
	/**************************************************/
	retval = intp_dyn_symbol_alloc (eng, NULL, nel_void_type, NULL,
		nel_C_RESULT, 0, nel_L_NEL, eng->intp->level);

	data = *((void **)arg_start->symbol->value);
	if(data == NULL ){
		intp_stmt_error (eng, "(nel_ntrn_new#2): data=NULL\n");
	}else{
		free(data);
	}

	nel_debug ({ nel_trace (eng, "] exiting nel_ntrn_delete\nretval = \n%S\n", retval); });
	return (retval);
}
#endif


nel_symbol *nel_ntrn_register_init(struct nel_eng *eng, register nel_symbol *func, register int nargs, register nel_stack *arg_start)
{
	//FILE *file;
	//register int i;
	register nel_symbol *symbol;
	//register nel_type *type;
	register nel_symbol *retval;

	nel_debug ({ nel_trace (eng, "entering nel_ntrn_register_init[\neng = 0x%x\narg_ct = %d\n\n", eng, nargs); });

	/***********************/
	/* check argument list */
	/***********************/
	if (nargs != 2 ) {
		intp_stmt_error (eng, "usage: nel_register_init(symbol, init_hander)");
	}

	/*****************************************************/
	/* if the first argument has type (struct _iobuf *), */
	/* (or (struct __file_s) on IRIS), then assume it is */
	/* the file to print to                              */
	/*****************************************************/
	symbol = arg_start->symbol;
	//if ((symbol != NULL) && ((type = symbol->type) != NULL) &&
	//  (type->simple.type == nel_D_POINTER) &&
	//  ((type = type->pointer.deref_type) != NULL) &&
	//  (type->simple.type == nel_D_STRUCT) &&
	//  (type->s_u.tag != NULL) &&
	//  (type->s_u.tag->name != NULL) &&
	//  (! strcmp (nel_FILE_tag, type->s_u.tag->name))) {
	//   file = *((FILE **) (symbol->value));
	//   arg_start++;
	//   nargs--;
	//
	//   /**************************/
	//   /* re-check argument list */
	//   /**************************/
	//   if (nargs < 1) {
	//      intp_stmt_error (eng, "usage: print([<file>], <expr>,...)");
	//   }
	//}
	//else {
	//   file = stdout;
	//}


	/*********************************************************************/
	/* scan through the argument list and pretty-print the data objects. */
	/* nel_pretty_print () checks for NULL types and NULL values.         */
	/*********************************************************************/
	func = (arg_start + 1)->symbol;
	if(symbol && func) {
		nel_symbol *scan;
		scan= (symbol->class == nel_C_TERMINAL) ? eng->terminals:
			((symbol->class==nel_C_NONTERMINAL)? eng->nonterminals:
			NULL);
		while(scan) {
			if(symbol->id == scan->_pid ) {
				scan->type->event.init_hander = func;
			}
			scan = scan->next;
		}
	}

	//for (i = 0; (i < nargs); i++) {
	//   symbol = (arg_start + i)->symbol;
	//   nel_pretty_print (file, symbol->type, symbol->value, 0);
	//}

	/*******************************************/
	/* allocate a symbol for for the result,   */
	/* with void type, and a NULL value field. */
	/*******************************************/
	retval = intp_dyn_symbol_alloc (eng, NULL, nel_void_type, NULL,
		   nel_C_RESULT, 0, nel_L_NEL, eng->intp->level);

	nel_debug ({ nel_trace (eng, "] exiting nel_ntrn_register_init\nretval = \n%S\n", retval); });

	return (retval);

}

nel_symbol *nel_ntrn_register_free(struct nel_eng *eng, register nel_symbol *func, register int nargs, register nel_stack *arg_start)
{
	//FILE *file;
	//register int i;
	register nel_symbol *symbol;
	//register nel_type *type;
	register nel_symbol *retval;

	nel_debug ({ nel_trace (eng, "entering nel_ntrn_register_free[\neng = 0x%x\narg_ct = %d\n\n", eng, nargs); });

	/***********************/
	/* check argument list */
	/***********************/
	if (nargs != 2 ) {
		intp_stmt_error (eng, "usage: register_free(symbol, free_hander)");
	}

	/*****************************************************/
	/* if the first argument has type (struct _iobuf *), */
	/* (or (struct __file_s) on IRIS), then assume it is */
	/* the file to print to                              */
	/*****************************************************/
	symbol = arg_start->symbol;
	//if ((symbol != NULL) && ((type = symbol->type) != NULL) &&
	//  (type->simple.type == nel_D_POINTER) &&
	//  ((type = type->pointer.deref_type) != NULL) &&
	//  (type->simple.type == nel_D_STRUCT) &&
	//  (type->s_u.tag != NULL) &&
	//  (type->s_u.tag->name != NULL) &&
	//  (! strcmp (nel_FILE_tag, type->s_u.tag->name))) {
	//   file = *((FILE **) (symbol->value));
	//   arg_start++;
	//   nargs--;
	//
	//   /**************************/
	//   /* re-check argument list */
	//   /**************************/
	//   if (nargs < 1) {
	//      intp_stmt_error (eng, "usage: print([<file>], <expr>,...)");
	//   }
	//}
	//else {
	//   file = stdout;
	//}


	/*********************************************************************/
	/* scan through the argument list and pretty-print the data objects. */
	/* nel_pretty_print () checks for NULL types and NULL values.         */
	/*********************************************************************/
	func = (arg_start + 1)->symbol;
	if(symbol && func) {
		nel_symbol *scan;
		scan= (symbol->class == nel_C_TERMINAL) ? eng->terminals:
			((symbol->class==nel_C_NONTERMINAL)? eng->nonterminals:
			NULL);
		while(scan) {
			if(symbol->id == scan->_pid ) {
				scan->type->event.free_hander = func;
			}
			scan = scan->next;
		}
	}

	//for (i = 0; (i < nargs); i++) {
	//   symbol = (arg_start + i)->symbol;
	//   nel_pretty_print (file, symbol->type, symbol->value, 0);
	//}

	/*******************************************/
	/* allocate a symbol for for the result,   */
	/* with void type, and a NULL value field. */
	/*******************************************/
	retval = intp_dyn_symbol_alloc (eng, NULL, nel_void_type, NULL,
		   nel_C_RESULT, 0, nel_L_NEL, eng->intp->level);

	nel_debug ({ nel_trace (eng, "] exiting nel_ntrn_register_free\nretval = \n%S\n", retval); });

	return (retval);

}

/*****************************************************************************/
/* nel_ntrn_init () creates symbols for each of the intrinsic functions  */
/* and inserts them in the static ident hash table at level -1, so that the  */
/* identifier names for the intrinsics may be used if the corresponding      */
/* intrinsic is not needed.                                                  */
/*****************************************************************************/
void nel_ntrn_init (struct nel_eng *eng)
{
	register nel_type *type;
	register nel_type *tag_type_d;
	register nel_symbol *tag;
	register nel_symbol *symbol;
	nel_list *args;

	nel_debug ({ nel_trace (eng, "entering nel_ntrn_init [\n\n"); });

	/**********************************************************/
	/* allocate symbols for all of the intrinsic functions.   */
	/* insert them in the static ident hash table at level -1 */
	/* so they don't conflict with global identifiers.        */
	/**********************************************************/

#if 0
	/*************/
	/* getrec () */
	/*************/
	/************************************************************/
	/* the return type of getrec () is (struct nel_eng *).   */
	/* lookup nel_eng in the static tag table (it should be */
	/* defined at level 0; ignore any definition at an inner    */
	/* scope) and create the type descriptor for an incomplete  */
	/* union if it isn't found.                                 */
	/************************************************************/
	if ((tag = nel_lookup_symbol ("nel_eng", eng->nel_static_tag_hash, NULL)) == NULL) {
		type = nel_type_alloc (eng, nel_D_UNION, 0, 0, 0, 0, NULL, NULL);
		tag_type_d = nel_type_alloc (eng, nel_D_UNION_TAG, 0, 0, 0, 0, type);
		tag = nel_static_symbol_alloc (eng, nel_insert_name(eng, "nel_eng"),
									   tag_type_d, (char *) type, nel_C_TYPE, 0, nel_L_NEL, 0);
		type->s_u.tag = tag;
		nel_insert_symbol (eng, tag, eng->nel_static_tag_hash);
	} else {
		nel_debug ({
			if (tag->type == NULL) {
				intp_fatal_error (NULL, "(nel_ntrn_init #1): bad tag\n%S", tag);
			}
		});
		if (tag->type->simple.type != nel_D_UNION_TAG) {
			intp_error (NULL, "redeclaration of nel_eng : getrec() intrinsic disabled");
			goto post_getrec;
		}
		nel_debug ({
			if (tag->type->tag_name.descriptor == NULL) {
				intp_fatal_error (NULL, "(nel_ntrn_init #2): bad symbol\n%S", tag);
			}
		});
		type = tag->type->tag_name.descriptor;
	}

	/***********************************************************/
	/* we have the type descriptor for (struct nel_eng).    */
	/* now create the descriptor for (struct nel_eng * ()), */
	/* and the symbol for getrec ().                           */
	/***********************************************************/
	type = nel_type_alloc (eng, nel_D_POINTER, sizeof (char *), nel_alignment_of (char *), 0, 0, type);
	type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 0, type, NULL, NULL, NULL);
	symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "getrec"), type, (char *) nel_ntrn_getrec, nel_C_INTRINSIC_FUNCTION, 0, nel_L_NEL, -1);
	nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);
post_getrec:
	;


#endif

	/************/
	/* print () */
	/************/
	/***********************************/
	/* print () has a void return type */
	/***********************************/

        /*
         * create symbol of call_nel_do_print  for nel_ntrn_print 
	 * wyong, 20230908 
         */

        symbol = nel_lookup_symbol("call_nel_do_print", eng->nel_static_ident_hash, eng->nel_static_location_hash,  NULL);
        if(symbol == NULL) {
                //type = nel_type_alloc(eng, nel_D_POINTER, sizeof(FILE *), nel_alignment_of(FILE *), 0, 0, nel_char_type);
                //symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,"file"), type, NULL, nel_C_FORMAL, nel_lhs_type(type), nel_L_C, 1);
                //args = nel_list_alloc(eng, 0, symbol, NULL);

                type = nel_type_alloc(eng, nel_D_POINTER, sizeof(char *), nel_alignment_of(char *), 0, 0, nel_char_type);
                symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,"fmt"), type, NULL, nel_C_FORMAL, nel_lhs_type(type), nel_L_C, 1);
                args = nel_list_alloc(eng, 0, symbol, args);

                /* return type */
                type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 0, nel_void_type, args, NULL, NULL);

                /* create symbol for function 'call_nel_do_print' */
                symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "call_nel_do_print"), type, (char *) call_nel_do_print, nel_C_COMPILED_FUNCTION, nel_lhs_type(type), nel_L_C, 0);

                nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);

        }else if(symbol->value == NULL) {
                symbol->value = (char *) call_nel_do_print;

        }else if(symbol->value != (char *)call_nel_do_print) {
                intp_error (NULL, "the earily inserted symbol value have difference value with call_nel_do_print!\n");

        }
        else {
                /* call_nel_do_print was successfully inserted */
        }


	//wyong, 20230817 

	/************/
	/* print () */
	/************/
	/***********************************/
	/* print () has a void return type */
	/***********************************/


	type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 1, nel_void_type, NULL, NULL, NULL);
	symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "print"), type,
									  (char *) nel_ntrn_print, nel_C_INTRINSIC_FUNCTION, 0, nel_L_NEL, -1);
	nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);


#if 0 
	/***************/
	/* new() */
	/***************/
	type = nel_type_alloc (eng, nel_D_POINTER, sizeof (char *), nel_alignment_of (char *), 0, 0, nel_void_type);
	type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 0, type, NULL, NULL, NULL);
	symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "new"), type, (char *) nel_ntrn_new, nel_C_INTRINSIC_FUNCTION, 0, nel_L_NEL, -1);
	nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);


	/***************/
	/* delete() */
	/***************/
	type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 1, nel_void_type, NULL, NULL, NULL);
	symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "delete"), type, (char *) nel_ntrn_delete, nel_C_INTRINSIC_FUNCTION, 0, nel_L_NEL, -1);
	nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);
#endif


	/***************/
	/* symof () */
	/***************/
	/***********************************************************/
	/* the return type of symof () is (struct nel_SYMBOL *). */
	/* lookup nel_SYMBOL in the static tag table (it should be  */
	/* defined at level 0; ignore any definition at an inner   */
	/* scope) and create the type descriptor for an incomplete */
	/* struct if it isn't found.                               */
	/***********************************************************/
	if ((tag = nel_lookup_symbol ("nel_SYMBOL", eng->nel_static_tag_hash, NULL)) == NULL) {
		type = nel_type_alloc (eng, nel_D_STRUCT, 0, 0, 0, 0, NULL, NULL);
		tag_type_d = nel_type_alloc (eng, nel_D_STRUCT_TAG, 0, 0, 0, 0, type);
		tag = nel_static_symbol_alloc (eng, nel_insert_name(eng, "nel_SYMBOL"),
									   tag_type_d, (char *) type, nel_C_TYPE, 0, nel_L_NEL, 0);
		type->s_u.tag = tag;
		nel_insert_symbol (eng, tag, eng->nel_static_tag_hash);
	} else {
		nel_debug ({
		   if (tag->type == NULL) {
			   intp_fatal_error (NULL, "(nel_ntrn_init #3): bad tag\n%S", tag) ;
		   }
	   	});
		if (tag->type->simple.type != nel_D_STRUCT_TAG) {
			intp_error (NULL, "redeclaration of nel_SYMBOL: symof() intrinsic disabled");
			goto post_symof;
		}
		nel_debug ({
			if (tag->type->tag_name.descriptor == NULL) {
				intp_fatal_error (NULL, "(nel_ntrn_init #4): bad symbol\n%S", tag);
			}
		});
		type = tag->type->tag_name.descriptor;
	}

	/********************************************************/
	/* we have the type descriptor for struct nel_SYMBOL.    */
	/* now create the descriptor for struct nel_SYMBOL * (), */
	/* and the symbol for symof ().                      */
	/********************************************************/
	type = nel_type_alloc (eng, nel_D_POINTER, sizeof (char *), nel_alignment_of (char *), 0, 0, type);
	type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 0, type, NULL, NULL, NULL);
	symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "symof"), type,
									  (char *) nel_ntrn_symof, nel_C_INTRINSIC_FUNCTION, 0, nel_L_NEL, -1);
	nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);
post_symof:
	;


	/***************/
	/* idof() */
	/***************/
	type = nel_type_alloc (eng, nel_D_POINTER, sizeof (char *), nel_alignment_of (char *), 0, 0, nel_char_type);
	symbol = event_symbol_alloc(eng, nel_insert_name(eng,"name"), type, 0, nel_C_TERMINAL, NULL);
	args = nel_list_alloc(eng, 0, symbol, NULL);


	type = nel_type_alloc (eng, nel_D_INT, sizeof (int), nel_alignment_of (int), 0, 0);
	type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 0, type, args, NULL, NULL);
	symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "idof"), type, (char *) nel_ntrn_idof, nel_C_INTRINSIC_FUNCTION, 0, nel_L_NEL, -1);
	nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);



	/***************/
	/* ridof() */
	/***************/

	type = nel_type_alloc (eng, nel_D_INT, sizeof (int), nel_alignment_of (int), 0, 0);
	type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 0, type, NULL, NULL, NULL);
	symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "ridof"), type,
									  (char *) nel_ntrn_ridof, nel_C_INTRINSIC_FUNCTION, 0, nel_L_NEL, -1);
	nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);



	/***********************/
	/* register_init() */
	/***********************/
	type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 1, nel_void_type, NULL, NULL, NULL);
	symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "register_init"), type, (char *) nel_ntrn_register_init, nel_C_INTRINSIC_FUNCTION, 0, nel_L_NEL, -1);
	nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);



	/***********************/
	/* register_free() */
	/***********************/
	type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 1, nel_void_type, NULL, NULL, NULL);
	symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "register_free"), type, (char *) nel_ntrn_register_free, nel_C_INTRINSIC_FUNCTION, 0, nel_L_NEL, -1);
	nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);

	/*************/
	/* typeof () */
	/*************/
	/***********************************************************/
	/* the return type of a typeof () op is (union nel_TYPE *). */
	/* lookup nel_TYPE in the static tag table (it should be    */
	/* defined at level 0; ignore any definition at an inner   */
	/* scope) and create the type descriptor for an incomplete */
	/* struct if it isn't found.                               */
	/***********************************************************/
	if ((tag = nel_lookup_symbol ("nel_TYPE", eng->nel_static_tag_hash, NULL)) == NULL) {
		type = nel_type_alloc (eng, nel_D_UNION, 0, 0, 0, 0, NULL, NULL);
		tag_type_d = nel_type_alloc (eng, nel_D_UNION_TAG, 0, 0, 0, 0, type);
		tag = nel_static_symbol_alloc (eng, nel_insert_name(eng, "nel_TYPE"),
									   tag_type_d, (char *) type, nel_C_TYPE, 0, nel_L_NEL, 0);
		type->s_u.tag = tag;
		nel_insert_symbol (eng, tag, eng->nel_static_tag_hash);
	}

	/************************************************/
	/* do not create a symbol for typeof, as it is  */
	/* an operator, not an intrinsic function. when */
	/* typeof is invoked, it will search the static */
	/* tag table for the descriptor (union nel_TYPE) */
	/************************************************/

	nel_debug ({ nel_trace (eng, "] exiting nel_ntrn_init\n\n"); });
}


