

#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "parser.h"
#include "errors.h"
#include "type.h"
#include "err.h"
#include "sym.h"
#include "lex.h"
#include "stmt.h"
#include "expr.h"
#include "intp.h"
#include "dec.h"
#include "evt.h"
#include "mem.h"

/***************************************************************************/
/* when nel_static_global_equivs is true and the parser encounters a   */
/* declaration for a static global (file-local) variable <var_name>, it    */
/* looks for a non-static global var by the name <file_name>`<var_name>,   */
/* where <file_name> has _'s substituted in for characters not allowed in  */
/* C identifiers, and prepended with a _ if the name begins with a digit.  */
/* if it is found (and has the same type), then the static var assumes the */
/* same storage location as the non-static var.  if the non-static var is  */
/* not found, then it is created, and both vars assume the same storage.   */
/***************************************************************************/
unsigned_int nel_static_global_equivs = 1;



/*************************************************************************/
/* when nel_static_local_equivs is true and the parser encounters a  */
/* declaration for a static local variable <var_name>, it looks for a    */
/* global var (or a static global var if the function we are in was also */
/* declared static) by the name <function_name>`<block_no>`<var_name>.   */
/* <block_no> is omitted if it is 0 (the outermost block).  if it is     */
/* found (and has the same type), then the local var assumes the same    */
/* storage location as the global var.  if the global var is not found,  */
/* then it is created, and both vars assume the same storage.            */
/*************************************************************************/
unsigned_int nel_static_local_equivs = 1;

int nel_s_u_init(struct nel_eng *eng, nel_symbol *sym1, nel_expr *expr2)
{
	if(!sym1 || !expr2 || !sym1->type || nel_type_incomplete(sym1->type) || expr2->gen.opcode != nel_O_LIST)
	{
		parser_error(eng, "illegal s_u init argument");
		return 0;
	}
	
	int size = 0;
	nel_symbol *temp_symbol = NULL;
	nel_expr_list *temp_expr_list = NULL;
	nel_member *temp_member = NULL;
	if(sym1->type->s_u.type == nel_D_STRUCT)	//struct var init
	{
		temp_member = sym1->type->s_u.members;
		temp_expr_list = expr2->list.list;
		while(temp_member)
		{	
			size = temp_member->offset;
			eng->parser->dyn_symbols_next++;
			temp_symbol = parser_dyn_symbol_alloc(eng, NULL, temp_member->symbol->type, 
													(char*)(sym1->value)+size, sym1->class, 1, sym1->source_lang, eng->parser->level);

			//bit field initialize
			if(temp_member->bit_field) {
				nel_symbol *expr_result;
				unsigned_int word;
				expr_result = intp_eval_expr_2(eng, temp_expr_list->expr);
				intp_coerce (eng, nel_unsigned_int_type, (char *) (&word), expr_result->type, expr_result->value);
				intp_insert_bit_field(eng, temp_member->symbol->type, sym1->value+size, word, temp_member->bit_lb, temp_member->bit_size);
			}
			else {
				nel_global_init(eng, temp_symbol, temp_expr_list->expr);
			}
			temp_member = temp_member->next;
			temp_expr_list = temp_expr_list->next;
			if(temp_member && !temp_expr_list)
			{
				parser_error(eng, "less value when init struct value");
				return 0;
			}
		}
		if(temp_expr_list)
		{
			parser_warning(eng, "more value when init struct var.");
		}
		return sym1->type->s_u.size;
	}
	else if(sym1->type->s_u.type == nel_D_UNION)	//union var init
	{
		//find the longest member
		temp_expr_list = expr2->list.list;
		temp_member = sym1->type->s_u.members;
		while(temp_member)
		{
			if(temp_member->symbol->type->simple.size == sym1->type->s_u.size)
				break;
			temp_member = temp_member->next;
		}
		if(temp_expr_list->next != NULL)
		{
			parser_warning(eng, "more value when init union var");
		}
		temp_symbol = parser_dyn_symbol_alloc(eng, NULL, temp_member->symbol->type, sym1->value,
												sym1->class, 1, sym1->source_lang, eng->parser->level);
		nel_global_init(eng, temp_symbol, temp_expr_list->expr);
		return sym1->type->s_u.size;
	}
	else
	{
		parser_stmt_error(eng, "illegal type, not struct/union type");
		return 0;
	}
}

int nel_array_elms_init(struct nel_eng *eng, nel_symbol *sym, nel_expr_list *expr_list)
{
	nel_symbol *tmp_symbol = NULL;
	int size = 0 ;
	nel_expr_list *scan;
	nel_expr *expr;
	nel_type *base_type = sym->type->array.base_type;

	for( scan=expr_list; scan != NULL; scan = scan->next){
		expr = scan->expr;
		if(size >= sym->type->simple.size) {
			if(!sym->type->array.val_len) {
				parser_stmt_error(eng, "init list too long");
				return -1;
			}

			char *value;
			sym->type->simple.size += base_type->simple.size;
			value = nel_static_value_alloc(eng, sym->type->simple.size, sym->type->simple.alignment);
			
			nel_copy(size, (char *)value, (char *)sym->value);
			sym->value = value;			
		
		}

		tmp_symbol = parser_dyn_symbol_alloc(eng, NULL,
				base_type, 
				((char *)sym->value + size), 
				sym->class, 
				1, /* every element in an array is lhs, except 
				      for multi-array which we don't support */
				sym->source_lang, 
				eng->parser->level);
	
		size += nel_global_init(eng, tmp_symbol, expr);

	}

	if (nel_static_C_token(sym->class)){
		sym->_global->value = sym->value;	

	}

	return (size);

}

int nel_array_init(struct nel_eng *eng, nel_symbol *sym1, nel_expr *expr2)
{
	register nel_symbol *sym2;
	int size = 0;
	nel_type *base_type;
	base_type = sym1->type->array.base_type;
	
	if(expr2 == NULL){
		parser_error(eng, "nel_array_init #1) NULL expr");
		return 0;
	}

	switch(expr2->gen.opcode){
	case nel_O_LIST: 
		size = nel_array_elms_init(eng, sym1, expr2->list.list);
		break;
		

	case nel_O_SYMBOL:
		/* just an string, simple copy expr to sym */
		sym2 = expr2->symbol.symbol;
		
		switch(base_type->simple.type)
		{
		case nel_D_CHAR:
		case nel_D_SIGNED_CHAR:
		case nel_D_UNSIGNED_CHAR:
			if(!sym1->type->array.val_len && sym1->type->array.size<sym2->type->array.size) {
				parser_stmt_error(eng, "init string too long");
				return -1;
			}

			if(sym1->_global) {
				sym1->_global->value =  sym1->value;
			}

			if(sym1->type->simple.size<sym2->type->simple.size)
			{
				sym1->value = nel_static_value_alloc(eng, sym2->type->simple.size, nel_alignment_of(char));
			}
			size = sym1->type->simple.size = sym2->type->simple.size ;
			nel_copy(size, (char *)sym1->value, (char *)sym2->value);
			break;

		default:
			parser_stmt_error(eng, "array of inappropriate type initialized from string constant");

		}
		break;
	default:	
		parser_stmt_error(eng, "nel_array_init #2) NULL opcode!");
		break;

	}

	return size; 
	
}


/*****************************************************************************/
/* nel_global_init() is called when a initialization for a global data 	     */
/* object 'symbol' with expression 'expr'.				     */
/*****************************************************************************/
int nel_global_init(struct nel_eng *eng, nel_symbol *sym1, nel_expr *expr2)
{
	union nel_TYPE *type1;
	nel_symbol *sym2;
	nel_expr *expr1, *expr;
	int size=0;
	union nel_TYPE *t1, *t2;

	if( sym1 && (type1 = sym1->type)) {
		switch(type1->simple.type){
		case nel_D_STRUCT:
		case nel_D_UNION:
			size = nel_s_u_init(eng, sym1, expr2);
			break;
		case nel_D_ARRAY :
			size = nel_array_init(eng, sym1, expr2);
			break;

		default:
			expr1 = nel_expr_alloc (eng, nel_O_SYMBOL, sym1);
			expr = nel_expr_alloc(eng, nel_O_ASGN, expr1, expr2); 
			t1 = sym1->type;
			t2 = eval_expr_type(eng, expr2);
			if(!t2) {
				parser_stmt_error(eng, "illegal expr");
				return -1;
			}
			if(!is_asgn_compatible(eng, t1, t2)) {
				parser_stmt_error(eng, "illegal expr: assignment incompatible");
				return -1;
			}
			intp_eval_expr_2(eng, expr);
			size = sym1->type->simple.size;
			break;
		}
	}

	sym1->initialized = 1;
	return (size);

}


int nel_dec_init(struct nel_eng *eng, nel_symbol *sym, nel_expr *expr)
{
	nel_type *type;
	int stor_class;
	int retval = 0;
	
	/******************************************************/
	/* for global (or file local) data objects, call      */
	/* nel_global_init() to initialize the symbol, */
	/******************************************************/
	if (eng->parser->level == 0) {
		/**********************************************/
		/* don't resolve type if it 's an array.      */	 
		/**********************************************/
		//if(type->simple.type != nel_D_ARRAY){
		//	type = parser_resolve_type (eng, type, name);
		//}


		switch (sym->class) {
#if 0
		case nel_T_AUTO:
		case nel_T_REGISTER:
			parser_warning (eng, "illegal class (%N) for %I", stor_class, sym);
			/* fall through */
		case nel_T_EXTERN:
		case nel_T_NULL:
			retval = nel_global_init(eng, sym, expr);
			break;
		case nel_T_STATIC:
			retval = nel_global_init(eng, sym, expr);
			break;
		
		default:
			parser_fatal_error (eng, "(nel_dec #9): bad class %N", stor_class);
			break;
#else
		case nel_C_GLOBAL:
		case nel_C_STATIC_GLOBAL:
		case nel_C_NULL:
			if(expr_contain_funcall(eng, expr)) {
				parser_stmt_error(eng, "cannot inititialize a global variable by function calling");
				retval = 0;	
			}
			retval = nel_global_init(eng, sym, expr);
			break;
		default:
			parser_fatal_error (eng, "(nel_dec #9): bad class %N", stor_class);
			break;
#endif
		}

	}

	/***************************************************/
	/* for routine-local data objects, call            */
	/* nel_local_dec () to create the symbol, */
	/* or to set the old symbol's value, then create   */
	/* a stmt structure around the symbol, and insert  */
	/* it in the stmt list.  do not resolve the type   */
	/* descriptor, as nel_local_dec () will   */
	/* do this if it needs to (for static vars).       */
	/***************************************************/
	else /* eng->parser->level > 0 */
	{
		//register nel_symbol *symbol;
		switch (sym->class) {
		case nel_C_LOCAL:
		case nel_C_REGISTER_LOCAL:
		case nel_C_NULL:
			/* create ASGN stmt and insert it into eng->parser->stmts list */
			{
				register nel_expr *expr1;		
				register nel_stmt *stmt;
				register nel_symbol *global;

				type = sym->type;
				if (type->simple.type == nel_D_ARRAY){
					char *global_name;
					if((eng->parser->unit_prefix != NULL) 
					&& (*(eng->parser->unit_prefix)!='\0')){
						/*********************************************/
						/* create/find a global variable by the name */
						/* <filename>`<var_name>, where filename     */
						/* has _`s substituted in for illegal chars. */
						/*********************************************/
						char *buffer;
						nel_alloca (buffer, strlen (eng->parser->unit_prefix) + strlen (sym->name) + 2, char);
						sprintf (buffer, "%s`%s", eng->parser->unit_prefix, sym->name);
						global_name = nel_insert_name (eng, buffer);
						nel_dealloca (buffer);
					}
	
					/*type = nel_type_alloc(eng, nel_D_ARRAY, 0, type->simple.alignment, 1, 0, type->array.base_type, nel_int_type, type->array.known_lb, (type->array.known_lb) ? type->array.lb.value : type->array.lb.expr, type->array.known_ub, (type->array.known_ub) ? type->array.ub.value : type->array.ub.expr);*/
					type->simple.traversed = 1;
					if((global = nel_global_def (eng, global_name,  type , 0, nel_static_value_alloc(eng, type->simple.size, type->simple.alignment), 0))){
						nel_global_init(eng, global, expr);
					}
					if(eng->compile_level==2) {
						expr1 = nel_expr_alloc(eng, nel_O_SYMBOL, sym);
						expr1 = nel_expr_alloc(eng, nel_O_ASGN, expr1, expr); 
						stmt = nel_stmt_alloc (eng, nel_S_EXPR, eng->parser->filename, eng->parser->line, expr1, NULL);
						append_stmt (stmt, &(stmt->expr.next));
						break;
					}
					else
						expr = nel_expr_alloc(eng, nel_O_SYMBOL, global);
				}
				expr1 = nel_expr_alloc(eng, nel_O_SYMBOL, sym);
				expr1 = nel_expr_alloc(eng, nel_O_ASGN, expr1, expr); 
				stmt = nel_stmt_alloc (eng, nel_S_EXPR, eng->parser->filename, eng->parser->line, expr1, NULL);
				append_stmt (stmt, &(stmt->expr.next));
			}
			break;

		case nel_C_STATIC_LOCAL:
			if(sym->_global)
				retval = nel_global_init (eng, sym->_global, expr);
			break;

		default:
			parser_fatal_error (eng, "(nel_dec #10): bad class %N", stor_class);
			break;
		}

	}

	nel_debug ({
		nel_trace (eng, "] exiting nel_dec\n\n");
	});

	return retval;

}

/*****************************************************************************/
/* nel_global_def () is called when a declaration for a   */
/* routine or data object is encountered in declare mode, or when an         */
/* interpreted routine is defined (i.e. the interpreted code is parsed).     */
/* it creates/modifies the appropriate symbol table entrie(s).               */
/* we assume that the type desciptor has already been resolved.              */
/*****************************************************************************/
nel_symbol *nel_global_def (struct nel_eng *eng, register char *name, register nel_type *type, register unsigned_int static_class, register char *address, unsigned_int compiled)
{
	register nel_symbol *old_sym;
	char *buffer;
	char *global_name;
	register nel_symbol *retval = NULL;

	nel_debug ({ nel_trace (eng, "entering nel_global_def [\neng = 0x%x\nname = %s\ntype =\n%1Tstatic_class = %d\naddress = 0x%x\ncompiled = %d\n\n", eng, name, type, static_class, address, compiled); });

	nel_debug ({
	if ((name == NULL) || (*name == '\0')) {
		parser_fatal_error (eng, "(nel_global_def #1): NULL or empty name");
	}
	
	if (type == NULL) {
		parser_fatal_error(eng, "(nel_global_def #2): NULL type");
	}
	});

	/*****************************************************/
	/* check for redeclaration of the var by looking for */
	/* another symbol with the same name at this level.  */
	/* first check the dynamic ident hash table for      */
	/* static globals, then check the static ident hash  */
	/* table for external (regular) globals.             */
	/*****************************************************/
	/* 
	old_sym = lookup_event_symbol(eng, name);	
	if(old_sym != NULL ){
		parser_stmt_error(eng, "%s has declaration as %s ignored",name, 
		 (old_sym->class == nel_C_TERMINAL) ? "atom":
                        ((old_sym->class==nel_C_NONTERMINAL)? "event":
                        NULL));
		return NULL;
	}
	*/

	old_sym = nel_lookup_symbol (name, eng->parser->dyn_ident_hash, NULL);
	if ((old_sym != NULL) && (old_sym->level > 0)) {
		old_sym = NULL;
	}
	if (old_sym == NULL) {
		old_sym = nel_lookup_symbol (name, eng->nel_static_ident_hash, NULL);
	}
	nel_debug ({
		nel_trace (eng, "old_sym =\n%1S\n", old_sym);
	});

	if (! static_class) {
		if (old_sym != NULL) {
			/*********************************/
			/* make sure that the old symbol */
			/* has the same type and value.  */
			/*********************************/
			if ( (nel_type_diff (old_sym->type, type, 1) == 1) ||
				((old_sym->value != NULL) && (old_sym->value != address)) ) {
				parser_stmt_error (eng, "redeclaration of %s", name);
			} else {
				if (old_sym->value == NULL) {
					/********************************************************/
					/* if the value field of the old symbol wasn't, set it. */
					/* set the old symbol's class to reflect this is an     */
					/* interpreted routine or a compiled routine.           */
					/********************************************************/
					old_sym->value = address;
					if (compiled) {
						if (old_sym->class == nel_C_NEL_FUNCTION) {
							old_sym->class = nel_C_COMPILED_FUNCTION;
						} else if (old_sym->class == nel_C_NEL_STATIC_FUNCTION) {
							old_sym->class = nel_C_COMPILED_STATIC_FUNCTION;
						}
					} else {
						if (old_sym->class == nel_C_COMPILED_FUNCTION) {
							old_sym->class = nel_C_NEL_FUNCTION;
						} else if (old_sym->class == nel_C_COMPILED_STATIC_FUNCTION) {
							old_sym->class = nel_C_NEL_STATIC_FUNCTION;
						}
					}
				}

				/***********************************/
				/* if the old symbol was declared  */
				/* static, emit a warning message. */
				/***********************************/
				if (nel_static_C_token (old_sym->class)) {
					parser_warning (eng, "non-static declaration of %s ignored", name);

					if (nel_static_global_equivs && (eng->parser->unit_prefix != NULL) && (*(eng->parser->unit_prefix) != '\0'))
					{
						/*****************************************************/
						/* since this was originally a static global symbol, */
						/* the flag nel_static_global_equivs is set,     */
						/* and we are in a named interpretation unit, call   */
						/* nel_global_def() recursively to   */
						/* make sure that the global symbol corresponding to */
						/* this exists and has its address set properly.     */
						/*****************************************************/
						nel_alloca (buffer, strlen (eng->parser->unit_prefix) + strlen (name) + 2, char);
						sprintf (buffer, "%s`%s", eng->parser->unit_prefix, name);
						global_name = nel_insert_name (eng,buffer);
						nel_dealloca (buffer);
						old_sym->_global = nel_global_def (eng, global_name, type, 0, address, 0);
					}
				}else {
					if(nel_compiled_C_token(old_sym->class) || nel_C_token(old_sym->class)) {
						old_sym->type = type;
					}

				}

				/********************************************/
				/* the old symbol becomes the return value. */
				/********************************************/
				retval = old_sym;
			}
		} else {
			/*******************************************************/
			/* we have not seen a declaration for this symbol yet. */
			/* check for void and incomplete types.                */
			/*******************************************************/
			if (type->simple.type == nel_D_VOID) {
				parser_error (eng, "void type for %s", name);
			} else if (nel_type_incomplete (type)) {
				parser_error (eng, "incomplete type for %s", name);
			} else {
				/****************************************************/
				/* allocate space for the symbol and set its value, */
				/* and mark it declared.  insert the symbol in the  */
				/* static ident hash table.                         */
				/****************************************************/
				retval = nel_static_symbol_alloc (eng, name, type, address,
						((type->simple.type != nel_D_FUNCTION) ? nel_C_GLOBAL :
						(compiled ? nel_C_COMPILED_FUNCTION : nel_C_NEL_FUNCTION)),
						nel_lhs_type (type), nel_L_NEL, 0);
				retval->declared = 1;
				parser_insert_ident (eng, retval);
			}
		}
	} else { /* static */
		if (old_sym != NULL) {
			/*********************************/
			/* make sure that the old symbol */
			/* has the same type and value.  */
			/*********************************/
			if ((nel_type_diff (old_sym->type, type, 1)) ||
				((old_sym->value != NULL) && (old_sym->value != address))) {
				parser_error (eng, "redeclaration of %s", name);
			} else {
				if (old_sym->value == NULL) {
					/********************************************************/
					/* if the value field of the old symbol wasn't, set it. */
					/* set the old symbol's class to reflect this is an     */
					/* interpreted routine or a compiled routine.           */
					/********************************************************/
					old_sym->value = address;
					if (compiled) {
						if (old_sym->class == nel_C_NEL_FUNCTION) {
							old_sym->class = nel_C_COMPILED_FUNCTION;
						} else if (old_sym->class == nel_C_NEL_STATIC_FUNCTION) {
							old_sym->class = nel_C_COMPILED_STATIC_FUNCTION;
						}
					} else {
						if (old_sym->class == nel_C_COMPILED_FUNCTION) {
							old_sym->class = nel_C_NEL_FUNCTION;
						} else if (old_sym->class == nel_C_COMPILED_STATIC_FUNCTION) {
							old_sym->class = nel_C_NEL_STATIC_FUNCTION;
						}
					}
				}

				/**************************************/
				/* if the old symbol was not declared */
				/* static, emit a warning message.    */
				/**************************************/
				if (! nel_static_C_token (old_sym->class)) {
					parser_warning (eng, "static declaration of %s ignored", name);
				} else if (nel_static_global_equivs && (eng->parser->unit_prefix != NULL) && (*(eng->parser->unit_prefix) != '\0')) {

					parser_warning (eng, "redeclaration of %s ignored", name);
					/*****************************************************/
					/* since this is a static global symbol,             */
					/* the flag nel_static_global_equivs is set,     */
					/* and we are in a named interpretation unit, call   */
					/* nel_global_def () recursively  */
					/* to make sure that the global symbol corresponding */
					/* to this exists and has its address set properly.  */
					/*****************************************************/
					nel_alloca (buffer, strlen (eng->parser->unit_prefix) + strlen (name) + 2, char);
					sprintf (buffer, "%s`%s", eng->parser->unit_prefix, name);
					global_name = nel_insert_name (eng,buffer);
					nel_dealloca (buffer);
					old_sym->_global = nel_global_def (eng, global_name, type, 0, address, compiled);
				}

				/********************************************/
				/* the old symbol becomes the return value. */
				/********************************************/
				retval = old_sym;
			}
		} else {
			/*******************************************************/
			/* we have not seen a declaration for this symbol yet. */
			/* check for void and incomplete types.                */
			/*******************************************************/
			if (type->simple.type == nel_D_VOID) {
				parser_error (eng, "void type for %s", name);
			} else if (nel_type_incomplete (type)) {
				parser_error (eng, "incomplete type for %s", name);
			} else {
				register nel_symbol *global_symbol;
				if (nel_static_global_equivs && (eng->parser->unit_prefix != NULL)
						&& (*(eng->parser->unit_prefix) != '\0')) {
					/*********************************************/
					/* create/find a global variable by the name */
					/* <filename>`<var_name>, where filename     */
					/* has _`s substituted in for illegal chars. */
					/*********************************************/
					nel_alloca (buffer, strlen (eng->parser->unit_prefix) + strlen (name) + 2, char);
					sprintf (buffer, "%s`%s", eng->parser->unit_prefix, name);
					global_name = nel_insert_name (eng,buffer);
					nel_dealloca (buffer);
					global_symbol = nel_global_def (eng, global_name, type, 0, address, compiled);
				}

				/**************************************************/
				/* regardless of the success of finding/creating  */
				/* the global symbol, create a local symbol with  */
				/* address as the storage location, and insert it */
				/* in the dynamic ident hash table.               */
				/**************************************************/
				retval = nel_static_symbol_alloc (eng, name, type, address,
					  ((type->simple.type != nel_D_FUNCTION) ? nel_C_STATIC_GLOBAL :
					   (compiled ? nel_C_COMPILED_STATIC_FUNCTION : nel_C_NEL_STATIC_FUNCTION)),
					  nel_lhs_type (type), nel_L_NEL, 0);
				retval->declared = 1;
				retval->_global = global_symbol;
				parser_insert_ident (eng, retval);
			}
		}
	}

	nel_debug ({ nel_trace (eng, "] exiting nel_global_def\nretval =\n%1S\n", retval); });
	return (retval);
}



/*****************************************************************************/
/* nel_global_dec () is called when a declaration for a data      */
/* object at level 0 is encountered.  it creates/modifies the appropriate    */
/* symbol table entrie(s).  declarations of level 0 symbols may appear       */
/* multiple times, so long as the type descriptors are equivalent.           */
/* we assume that the type desciptor has already been resolved.              */
/*****************************************************************************/
nel_symbol *nel_global_dec (struct nel_eng *eng, register char *name, register nel_type *type, register unsigned_int static_class)
{
	register nel_symbol *old_sym;
	register nel_symbol *global_sym = NULL;
	char *buffer;
	char *global_name;
	register nel_symbol *retval = NULL;

	nel_debug ({ nel_trace (eng, "entering nel_global_dec [\neng= 0x%x\nname = %s\ntype =\n%1Tstatic_class = %d\n\n", eng, name, type, static_class); });

	nel_debug ({
	if ((name == NULL) || (*name == '\0')) {
		parser_fatal_error (eng, "(nel_global_dec #1): NULL or empty name");
	}
	if (type == NULL) {
		parser_fatal_error (eng, "(nel_global_dec #2): NULL type");
	}
	});

	/*****************************************************/
	/* check for redeclaration of the var by looking for */
	/* another symbol with the same name at level 0.     */
	/* first check the dynamic ident hash table for      */
	/* static globals, then check the static ident hash  */
	/* table for external (regular) globals.             */
	/*****************************************************/
	/* 
	old_sym = lookup_event_symbol(eng, name);	
	if(old_sym != NULL ){
		parser_stmt_error(eng, "%s has declaration as %s ignored",name, 
		 (old_sym->class == nel_C_TERMINAL) ? "atom":
                        ((old_sym->class==nel_C_NONTERMINAL)? "event":
                        NULL));
		return NULL;
	}
	*/

	old_sym = nel_lookup_symbol (name, eng->parser->dyn_ident_hash, NULL);
	if ((old_sym != NULL) && (old_sym->level > 0)) {
		old_sym = NULL;
	}
	if (old_sym == NULL) {
		old_sym = nel_lookup_symbol (name, eng->nel_static_ident_hash, NULL);
	}
	nel_debug ({
		nel_trace (eng, "old_sym =\n%1S\n", old_sym);
	});

	if (! static_class) {
		if (old_sym != NULL) {
			/****************************************************/
			/* make sure that the old symbol has the same type. */
			/****************************************************/
			if (nel_type_diff (old_sym->type, type, 1)) {
				parser_error (eng, "redeclaration of %s", name);
			} else if (old_sym->value != NULL) {
				/***********************************/
				/* if the old symbol was declared  */
				/* static, emit a warning message. */
				/***********************************/
				if (nel_static_C_token (old_sym->class)) {
					parser_warning (eng, "non-static declaration of %s ignored", name);
					if (nel_static_global_equivs && (eng->parser->unit_prefix != NULL) && (*(eng->parser->unit_prefix) != '\0'))
					{
						/*****************************************************/
						/* since this was originally a static global symbol, */
						/* the flag nel_static_global_equivs is set,     */
						/* and we are in a named interpretation unit, call   */
						/* nel_global_def () recursively  */
						/* to make sure that the global symbol corresponding */
						/* to this exists and has its address set properly.  */
						/*****************************************************/
						nel_alloca (buffer, strlen (eng->parser->unit_prefix) + strlen (name) + 2, char);
						sprintf (buffer, "%s`%s", eng->parser->unit_prefix, name);
						global_name = nel_insert_name (eng, buffer);
						nel_dealloca (buffer);
						old_sym->_global = nel_global_def (eng, global_name, type, 0, old_sym->value, 0);

						/********************************************/
						/* don't worry about the compiled parameter */
						/* (6th) being 0. it is only used to set    */
						/* function classes, not data objs.         */
						/********************************************/
					}
				}else {
					parser_warning (eng, "redeclaration of %s ignored", name);
				}

				/********************************************/
				/* the old symbol becomes the return value. */
				/********************************************/
				retval = old_sym;
			} else {
				/***********************************/
				/* if the old symbol was declared  */
				/* static, emit a warning message. */
				/***********************************/
				if (nel_static_C_token (old_sym->class)) {
					parser_warning (eng, "non-static declaration of %s ignored", name);
					if (nel_static_global_equivs && (eng->parser->unit_prefix != NULL) && (*(eng->parser->unit_prefix) != '\0'))
					{
						/*****************************************************/
						/* since this was originally a static global symbol, */
						/* the flag nel_static_global_equivs is set,     */
						/* and we are in a named interpretation unit, call   */
						/* nel_global_def () recursively  */
						/* to make sure that the global symbol corresponding */
						/* to this exists and has its address set properly.  */
						/*****************************************************/
						nel_alloca (buffer, strlen (eng->parser->unit_prefix) + strlen (name) + 2, char);
						sprintf (buffer, "%s`%s", eng->parser->unit_prefix, name);
						global_name = nel_insert_name (eng, buffer);
						nel_dealloca (buffer);
						global_sym = nel_global_dec (eng, global_name, type, 0);
					}
				}

				/***************************************************/
				/* the old symbol becomes the return value.        */
				/* use the storage allocated for the global_sym if */
				/* it exists, or else allocate new storage.        */
				/* if the global sym exists but storage was not    */
				/* allocated for it, set its storage location to   */
				/* the newly-allocated memory.                     */
				/***************************************************/
				if ((global_sym != NULL) && (global_sym->value != NULL)) {
					old_sym->value = global_sym->value;
				} else {
					old_sym->value = nel_static_value_alloc (eng, type->simple.size, type->simple.alignment);
					if (global_sym != NULL) /* global_sym->value == NULL */
					{
						global_sym->value = old_sym->value;
					}
				}
				retval = old_sym;
				retval->_global = global_sym;
			}
		} else {
			/*******************************************************/
			/* we have not seen a declaration for this symbol yet. */
			/* check for void and incomplete types.                */
			/*******************************************************/
			if (type->simple.type == nel_D_VOID) {
				parser_error (eng, "void type for %s", name);
			} else if (nel_type_incomplete (type)) {
				//parser_error (eng, "incomplete type for %s", name);
				retval = nel_static_symbol_alloc (eng, name,type,  NULL, nel_C_GLOBAL, nel_lhs_type (type), nel_L_NEL, 0);

				retval->declared = 1;
				parser_insert_ident (eng, retval);

			} else {
				/************************************************/
				/* allocate space for the symbol and its value, */
				/* and mark it declared.  insert the symbol in  */
				/* the static ident hash table.                 */
				/************************************************/
				retval = nel_static_symbol_alloc (eng, name, type, nel_static_value_alloc (eng, type->simple.size, type->simple.alignment), nel_C_GLOBAL, nel_lhs_type (type), nel_L_NEL, 0);

				retval->declared = 1;
				parser_insert_ident (eng, retval);
			}
		}
	} else { /* static */
		if (old_sym != NULL) {
			/****************************************************/
			/* make sure that the old symbol has the same type. */
			/****************************************************/
			if (nel_type_diff (old_sym->type, type, 1)) {
				parser_error (eng, "redeclaration of %s", name);
			} else if (old_sym->value != NULL) {
				/***************************************/
				/* if the old symbol was not declared  */
				/* static, emit a warning message.     */
				/***************************************/
				if (! nel_static_C_token (old_sym->class)) {
					parser_warning (eng, "static declaration of %s ignored", name);
				} else if (nel_static_global_equivs && (eng->parser->unit_prefix != NULL) && (*(eng->parser->unit_prefix) != '\0')) {

					parser_warning(eng, "redeclaration of %s ignored", name);

					/*****************************************************/
					/* since this is a static global symbol,             */
					/* the flag nel_static_global_equivs is set,     */
					/* and we are in a named interpretation unit, call   */
					/* nel_global_def () recursively  */
					/* to make sure that the global symbol corresponding */
					/* to this exists and has its address set properly.  */
					/*****************************************************/
					nel_alloca (buffer, strlen (eng->parser->unit_prefix) + strlen (name) + 2, char);
					sprintf (buffer, "%s`%s", eng->parser->unit_prefix, name);
					global_name = nel_insert_name (eng, buffer);
					nel_dealloca (buffer);
					old_sym->_global = nel_global_def (eng, global_name, type, 0, old_sym->value, 0);

					/***********************************************************/
					/* don't worry about the compiled parameter (6th) being 0. */
					/* it is only used to set function classes, not data objs. */
					/***********************************************************/
				}

				/********************************************/
				/* the old symbol becomes the return value. */
				/********************************************/
				retval = old_sym;

			} else {
				/***************************************/
				/* if the old symbol was not declared  */
				/* static, emit a warning message.     */
				/***************************************/
				if (! nel_static_C_token (old_sym->class)) {
					parser_warning (eng, "static declaration of %s ignored", name);
				} else if (nel_static_global_equivs && (eng->parser->unit_prefix != NULL) && (*(eng->parser->unit_prefix) != '\0')) {
					/*****************************************************/
					/* since this is a static global symbol,             */
					/* the flag nel_static_global_equivs is set,     */
					/* and we are in a named interpretation unit, call   */
					/* nel_global_def () recursively  */
					/* to make sure that the global symbol corresponding */
					/* to this exists and has its address set properly.  */
					/*****************************************************/
					nel_alloca (buffer, strlen (eng->parser->unit_prefix) + strlen (name) + 2, char);
					sprintf (buffer, "%s`%s", eng->parser->unit_prefix, name);
					global_name = nel_insert_name (eng, buffer);
					nel_dealloca (buffer);
					global_sym = nel_global_dec (eng, global_name, type, 0);
				}

				/***************************************************/
				/* the old symbol becomes the return value.        */
				/* use the storage allocated for the global_sym if */
				/* it exists, or else allocate new storage.        */
				/* if the global sym exists but storage was not    */
				/* allocated for it, set its storage location to   */
				/* the newly-allocated memory.                     */
				/***************************************************/
				if ((global_sym != NULL) && (global_sym->value != NULL)) {
					old_sym->value = global_sym->value;
				} else {
					old_sym->value = nel_static_value_alloc (eng, type->simple.size, type->simple.alignment);
					if (global_sym != NULL) /* global_sym->value == NULL */
					{
						global_sym->value = old_sym->value;
					}
				}
				retval = old_sym;
				retval->_global = global_sym;
			}
		} else {
			/*******************************************************/
			/* we have not seen a declaration for this symbol yet. */
			/* check for void and incomplete types.                */
			/*******************************************************/
			if (type->simple.type == nel_D_VOID) {
				parser_error (eng, "void type for %s", name);
			} else if (nel_type_incomplete (type)) {
				parser_error (eng, "incomplete type for %s", name);
			} else {
				if (nel_static_global_equivs && (eng->parser->unit_prefix != NULL) && (*(eng->parser->unit_prefix) != '\0')) {
					/*********************************************/
					/* create/find a global variable by the name */
					/* <filename>`<var_name>, where filename     */
					/* has _`s substituted in for illegal chars. */
					/*********************************************/
					nel_alloca (buffer, strlen (eng->parser->unit_prefix) + strlen (name) + 2, char);
					sprintf (buffer, "%s`%s", eng->parser->unit_prefix, name);
					global_name = nel_insert_name (eng, buffer);
					nel_dealloca (buffer);
					global_sym = nel_global_dec (eng, global_name, type, 0);
					if (global_sym != NULL) {
						/**************************************/
						/* the local symbol takes on the same */
						/* location as the global symbol.     */
						/**************************************/
						retval = nel_static_symbol_alloc (eng, name, type, global_sym->value, nel_C_STATIC_GLOBAL, nel_lhs_type (type), nel_L_NEL, 0);
						retval->declared = 1;
						retval->_global = global_sym;
						parser_insert_ident (eng, retval);
					}
				}
				if (retval == NULL) {
					/***********************************************************/
					/* either we are in an unnamed interpretation unit, the    */
					/* flag nel_static_global_equivs isn't set, or we were */
					/* unable to create/find the global symbol.  allocate      */
					/* space for the local symbol and its value, and mark      */
					/* it declared.  insert the symbol in the dynamic          */
					/* ident hash table.                                       */
					/***********************************************************/
					retval = nel_static_symbol_alloc (eng, name, type, nel_static_value_alloc (eng, type->simple.size, type->simple.alignment), nel_C_STATIC_GLOBAL, nel_lhs_type (type), nel_L_NEL, 0);

					retval->declared = 1;
					parser_insert_ident (eng, retval);
				}
			}
		}
	}

	nel_debug ({ nel_trace (eng, "] exiting nel_global_dec\nretval =\n%1S\n", retval); });
	return (retval);
}


/*****************************************************************************/
/* nel_local_dec () is called when a declaration for a local data   */
/* object is encountered.  it creates/modifies the appropriate symbol table  */
/* entry(s).  the type desciptor should not have been resolved yet - this is */
/* done at run-time for dynamic local variables, or in this routine if the   */
/* var is declared static.                                                   */
/*****************************************************************************/
nel_symbol *nel_local_dec(struct nel_eng *eng, register char *name, register nel_type *type, register unsigned_int static_class)
{
	register nel_symbol *old_sym;
	register nel_symbol *retval = NULL;
	char *buffer;
	char *global_name;
	register nel_symbol *global_sym;

	nel_debug ({ nel_trace (eng, "entering nel_local_dec [\neng = 0x%x\nname = %s\ntype =\n%1Tstatic_class = %d\n\n", eng, name, type, static_class); });

	nel_debug ({
	if ((name == NULL) || (*name == '\0')) {
		parser_fatal_error (eng, "(nel_local_dec #1): NULL or empty name");
	}
	if (type == NULL) {
		parser_fatal_error (eng, "(nel_local_dec #2): NULL type");
	}
	});

	/*****************************************************/
	/* check for redeclaration of the var by looking for */
	/* another symbol with the same name at this level.  */
	/* check only the dynamic ident hash table.          */
	/*****************************************************/
	old_sym = nel_lookup_symbol (name, eng->parser->dyn_ident_hash, NULL);
	if ((old_sym != NULL) && (old_sym->level != eng->parser->level)) {
		old_sym = NULL;
	}
	nel_debug ({
		nel_trace (eng, "old_sym =\n%1S\n", old_sym);
	});

	if (! static_class) {
		if (old_sym != NULL) {
			/******************************************/
			/* since local symbols go into and out of */
			/* scope, we should never see multiple    */
			/* declaration of local variables         */
			/******************************************/
			parser_error (eng, "redeclaration of %s", name);
		} else {
			/******************************************************/
			/* we have not seen a declaration for this symbol yet.*/
			/* do not check for void and incomplete types, as the */
			/* type descriptor will be resolved at run-time.      */
			/* allocate the symbol and and mark it declared.      */
			/* insert the symbol in the dynamic ident hash table. */
			/* do not allocate the actual storage - this will be  */
			/* done at interpretation-time.                       */
			/******************************************************/
			retval = nel_static_symbol_alloc (eng, name, type, NULL, nel_C_LOCAL, nel_lhs_type (type), nel_L_NEL, eng->parser->level);
			retval->declared = 1;
			parser_insert_ident (eng, retval);
		}
	} else { /* static */
		if (old_sym != NULL) {
			/******************************************/
			/* since local symbols go into and out of */
			/* scope, we should never see multiple    */
			/* declaration of local variables         */
			/******************************************/
			parser_error (eng, "redeclaration of %s", name);
		} else {
			/*******************************************/
			/* since this var is statically allocated, */
			/* resolve the type descriptor now.        */
			/*******************************************/
			type = parser_resolve_type (eng, type, name);
			nel_debug ({
			if (type == NULL) {
				parser_fatal_error (eng, "(nel_local_dec #3): NULL resolved type");
			}
			});

			/******************************************************/
			/* we have not seen a declaration for this symbol yet.*/
			/* check for void and incomplete types, since we will */
			/* allocate the storage now, at parse-time.           */
			/******************************************************/
			if (type->simple.type == nel_D_VOID) {
				parser_error (eng, "void type for %s", name);
			} else if (nel_type_incomplete (type)) {
				parser_error (eng, "incomplete type for %s", name);
			} else {
				if (nel_static_local_equivs && (eng->parser->prog_unit != NULL)) {
					/*****************************************************/
					/* we are in a named program unit, and               */
					/* the flag nel_static_local_equivs is set.      */
					/* look for a global (or static global) variable by  */
					/* the name <prog_unit>`<block_no>`<var_name>.  omit */
					/* <block_no> if it is 0.                            */
					/*****************************************************/
					if (eng->parser->block_no == 0) {
						nel_alloca (buffer, strlen (eng->parser->prog_unit->name) + strlen (name) + 3, char);
						sprintf (buffer, "%s``%s", eng->parser->prog_unit->name, name);
					} else {
						/************************************/
						/* assume block_no has <= 20 digits */
						/************************************/
						nel_alloca (buffer, strlen (eng->parser->prog_unit->name) + strlen (name) + 23, char);
						sprintf (buffer, "%s`%d`%s", eng->parser->prog_unit->name, eng->parser->block_no, name);
					}
					global_name = nel_insert_name (eng, buffer);
					nel_dealloca (buffer);

					/****************************************************/
					/* call nel_global_dec () to find/create */
					/* the corresponding static global (if the program  */
					/* unit is declared static) or global var.          */
					/****************************************************/
					global_sym = nel_global_dec (eng, global_name, type, nel_static_C_token (eng->parser->prog_unit->class));
					if ((global_sym != NULL) && (global_sym->value != NULL)) {
						retval = nel_static_symbol_alloc (eng, name, type, global_sym->value, nel_C_STATIC_LOCAL, nel_lhs_type (type), nel_L_NEL, eng->parser->level);
						retval->declared = 1;
						retval->_global = global_sym;
						parser_insert_ident (eng, retval);
					}
				}
				if (retval == NULL) {
					/********************************************************/
					/* either we are in an unnamed program unit, the flag   */
					/* nel_static_local_equivs isn't set, or we were    */
					/* unable to create/find the global symbol. check for   */
					/* allocate the symbol and and mark it declared.        */
					/* insert the symbol in the dynamic ident hash table.   */
					/* the storage for it must be statically allocated even */
					/* though the symbol could soon be out of scope forever */
					/* (after an unnamed prog unit is evaluated).           */
					/********************************************************/
					parser_warning(eng, "declare %s as auto", name);
					return nel_local_dec(eng, name, type, 0);

					retval = nel_static_symbol_alloc (eng, name, type, nel_static_value_alloc (eng, type->simple.size, type->simple.alignment), nel_C_STATIC_LOCAL, nel_lhs_type (type), nel_L_NEL, eng->parser->level);
					retval->declared = 1;
					parser_insert_ident (eng, retval);
				}
			}
		}
	}

	nel_debug ({ nel_trace (eng, "] exiting nel_local_dec\nretval =\n%1S\n", retval); });
	return (retval);
}


/*****************************************************************************/
/* nel_func_dec () is called when a declaration for a routine is */
/* encountered.  it creates/modifies the appropriate symbol table entrie(s). */
/* this routine is only called when external declaration are encountered;    */
/* nel_global_def() is called where the actual definition     */
/* takes place. all routine symbols are entered into the symbol tables at    */
/* level 0.  we assume that the type desciptor has already been resolved.    */
/*****************************************************************************/
nel_symbol *nel_func_dec (struct nel_eng *eng, register char *name, register nel_type *type, register unsigned_int static_class)
{
	register nel_symbol *old_sym;
	register nel_symbol *retval = NULL;
	char *buffer;
	char *global_name;

	nel_debug ({ nel_trace (eng, "entering nel_func_dec [\neng = 0x%x\name = %s\ntype =\n%1Tstatic_class = %d\n\n", eng, name, type, static_class); });

	nel_debug ({
	if ((name == NULL) || (*name == '\0')) {
		parser_fatal_error (eng, "(nel_func_dec #1): NULL or empty name");
	}
	if ((type == NULL) || (type->simple.type != nel_D_FUNCTION) || (type->function.return_type == NULL)) {
		parser_fatal_error (eng, "(nel_func_dec #2): bad type\n%T", type);
	}
	});

	/*****************************************************/
	/* check for redeclaration of the routine by looking */
	/* for another symbol with the same name at level 0. */
	/* (all routine symbols are at level 0).             */
	/* first check the dynamic ident hash table for      */
	/* static globals, then check the static ident hash  */
	/* table for external (regular) globals.             */
	/*****************************************************/
	old_sym = nel_lookup_symbol (name, eng->parser->dyn_ident_hash, NULL);
	if ((old_sym != NULL) && (old_sym->level > 0)) {
		old_sym = NULL;
	}
	if (old_sym == NULL) {
		old_sym = nel_lookup_symbol (name, eng->nel_static_ident_hash, NULL);
	}
	nel_debug ({
		nel_trace (eng, "old_sym =\n%1S\n", old_sym);
	});

	if (! static_class) {
		if (old_sym != NULL) {
			/****************************************************/
			/* make sure that the old symbol has the same type. */
			/****************************************************/
			if (nel_type_diff (old_sym->type, type, 1)) {
				parser_error (eng, "redeclaration of %s", name);
			} else {
				/***********************************/
				/* if the old symbol was declared  */
				/* static, emit a warning message. */
				/***********************************/
				if (nel_static_C_token (old_sym->class)) {
					parser_warning (eng, "non-static declaration of %s ignored", name);
					if (nel_static_global_equivs && (eng->parser->unit_prefix != NULL) && (*(eng->parser->unit_prefix) != '\0'))
					{
						/*****************************************************/
						/* since this was originally a static global symbol, */
						/* the flag nel_static_global_equivs is set,     */
						/* and we are in a named interpretation unit, call   */
						/* nel_func_dec () recursively to make   */
						/* sure that the global symbol corresponding to this */
						/* exists and has its address set properly.          */
						/*****************************************************/
						nel_alloca (buffer, strlen (eng->parser->unit_prefix) + strlen (name) + 2, char);
						sprintf (buffer, "%s`%s", eng->parser->unit_prefix, name);
						global_name = nel_insert_name (eng, buffer);
						nel_dealloca (buffer);
						old_sym->_global = nel_func_dec (eng, global_name, type, 0);
					}
				}

				/********************************************/
				/* the old symbol becomes the return value. */
				/********************************************/
				retval = old_sym;
			}
		} else {
			/*******************************************************/
			/* we have not seen a declaration for this symbol yet. */
			/* check for an incomplete return type.                */
			/*******************************************************/
			if (nel_type_incomplete (type)) {
				parser_error (eng, "incomplete type for %s", name);
			} else {
				/*****************************************************/
				/* allocate the symbol and mark it declared.         */
				/* for now, assume this is an interpreted routine.   */
				/* insert the symbol in the static ident hash table. */
				/*****************************************************/
				retval = nel_static_symbol_alloc (eng, name, type, NULL, nel_C_NEL_FUNCTION, 0, nel_L_NEL, 0);
				retval->declared = 1;
				parser_insert_ident (eng, retval);
			}
		}
	} else /* static */
	{
		if (old_sym != NULL) {
			/****************************************************/
			/* make sure that the old symbol has the same type. */
			/****************************************************/
			if (nel_type_diff (old_sym->type, type, 1)) {
				parser_error (eng, "redeclaration of %s", name);
			} else {
				/**************************************/
				/* if the old symbol was not declared */
				/* static, emit a warning message.    */
				/**************************************/
				if (! nel_static_C_token (old_sym->class)) {
					parser_warning (eng, "static declaration of %s ignored", name);
				} else if (nel_static_global_equivs && (eng->parser->unit_prefix != NULL) && (*(eng->parser->unit_prefix) != '\0')) {
					/*****************************************************/
					/* since this is a static global symbol,             */
					/* the flag nel_static_global_equivs is set,     */
					/* and we are in a named interpretation unit, call   */
					/* nel_func_dec () recursively to make   */
					/* sure that the global symbol corresponding to this */
					/* exists and has its address set properly.          */
					/*****************************************************/
					nel_alloca (buffer, strlen (eng->parser->unit_prefix) + strlen (name) + 2, char);
					sprintf (buffer, "%s`%s", eng->parser->unit_prefix, name);
					global_name = nel_insert_name (eng, buffer);
					nel_dealloca (buffer);
					old_sym->_global = nel_func_dec (eng, global_name, type, 0);
				}

				/********************************************/
				/* the old symbol becomes the return value. */
				/********************************************/
				retval = old_sym;
			}
		} else {
			/*******************************************************/
			/* we have not seen a declaration for this symbol yet. */
			/* check for an incomplete return type.                */
			/*******************************************************/
			if (nel_type_incomplete (type)) {
				parser_error (eng, "incomplete type for %s", name);
			} else {
				nel_symbol *global_symbol;
				if (nel_static_global_equivs && (eng->parser->unit_prefix != NULL) && (*(eng->parser->unit_prefix) != '\0')) {
					/*********************************************/
					/* create/find a global variable by the name */
					/* <filename>`<var_name>, where filename     */
					/* has _`s substituted in for illegal chars. */
					/*********************************************/
					nel_alloca (buffer, strlen (eng->parser->unit_prefix) + strlen (name) + 2, char);
					sprintf (buffer, "%s`%s", eng->parser->unit_prefix, name);
					global_name = nel_insert_name (eng, buffer);
					nel_dealloca (buffer);
					global_symbol = nel_func_dec (eng, global_name, type, 0);
				}

				/*************************************************/
				/* regardless of the success of finding/creating */
				/* the global symbol, create a local symbol for  */
				/* the function, and insert it in the dynamic    */
				/* ident hash table.  assume the symbol          */
				/* represents an interpreted routine for now.    */
				/*************************************************/
				retval = nel_static_symbol_alloc (eng, name, type, NULL, nel_C_NEL_STATIC_FUNCTION, 0, nel_L_NEL, 0);
				retval->_global = global_symbol;
			}
		}
	}

	/* 
	the declared function will be an c static compiled function,
	a dynamic glibc function, or nel function. they are in the same 
	name space, it means you can't declare a nel function that exist
	in glibc or static c library.

	if(retval->value == NULL) {
		retval->value = dlsym(NULL,  retval->name);
		if(retval->value) {
			retval->class = nel_C_COMPILED_FUNCTION;
		}
	}
	*/


	nel_debug ({ nel_trace (eng, "] exiting nel_func_dec\nretval =\n%1S\n", retval); });
	return (retval);
}


/******************************************************************************/
/* nel_evt_dec () is called when a declaration for a event is encountered.    */
/* it creates/modifies the appropriate symbol table entrie(s). 		      */
/* the type desciptor here is the type of event instance , not the event      */
/* symbol itself so we need create an external type for event.		      */
/******************************************************************************/
nel_symbol *nel_evt_dec(struct nel_eng *eng, char *name, nel_type *type, unsigned_int class )
{
	nel_symbol *retval, *old_sym;
	nel_type *type1;
	register nel_symbol *symbol; 
	register nel_member *first;
	unsigned int _volatile;
	
	if( type == NULL 
		|| ( type->simple.type != nel_D_VOID 
		     && type->simple.type != nel_D_POINTER )) {
		parser_fatal_error(eng, "the type of event %s must be void or pointer", name);
	}

	if(type->simple.type == nel_D_POINTER ) {
		type1 = type->pointer.deref_type;
		if( type1 == NULL 
			|| ( type1->simple.type != nel_D_VOID 
				&& type1->simple.type != nel_D_STRUCT 
			&& type1->simple.type != nel_D_UNION)){
			parser_fatal_error(eng, "the type of event %s must be pointer to void or struct/union", name);
		}

		if ( type1->simple.type != nel_D_VOID ){
			if ( (first = type1->s_u.members) == NULL 
				|| (symbol = first->symbol ) == NULL
				|| strcmp(symbol->name, "nel_count") != 0 ) {
				parser_fatal_error(eng, "the first field of %s must be nel_count", name);
			}
		}

		/* correctly get _volatile */
		_volatile = type1->simple._volatile;	

	}else if ( type->simple.type == nel_D_VOID ) {
		_volatile = type->simple._volatile;	
	}

	if ((old_sym = lookup_event_symbol(eng, name ))) {
		if ( old_sym->class != class 
			|| nel_type_diff (old_sym->type->event.descriptor, type, 0)) {
			parser_error (eng, "redeclaration of %s", name);
		}else {
			parser_warning (eng, "redeclaration of %s", name);
		}
		return old_sym ;
	}
	

	if((retval = event_symbol_alloc(eng, name, nel_type_alloc (eng,nel_D_EVENT,0,0,0,0, type, NULL), /*type->simple.*/ _volatile, class, NULL)) == NULL ) {
		parser_warning (eng, "symbol(%s) alloc error \n ", name);
		return NULL;
	}

	if (class == nel_C_NONTERMINAL && !strcmp(name, "link")) {
		register nel_type * val_type;
		register nel_symbol *val;
		eng->startSymbol = retval ;
		eng->startSymbol->_reachable = 1; 

		val = nel_static_symbol_alloc(eng, nel_insert_name(eng,"$0"), type, NULL, nel_C_FORMAL, nel_lhs_type(type), nel_L_NEL, 0);


		/**********************************************************************/
		/* make a struct nel_EVENT data structure for the symbol, which saving*/
		/* the necessary information of EVENT, so that the normal symbol won't*/
		/* need malloc memory space for those unnecessary info,this way, we   */
		/* can save a lot of memory usage */
		/**********************************************************************/
		
		val->aux.event = nel_event_alloc(eng);	
		//nel_malloc(val->aux.event, 1, struct nel_EVENT);
		
		/*
		if(!val->aux.event ){
			event_symbol_dealloc(eng, val);	
			return NULL;
		}
		*/
		

		/**********************************************************************/
		/*pid is id of parent event , set pid equal id means the event has no */
		/*expression with it therefore need not an parent. */
		/**********************************************************************/
		val->_pid = val->id;

		val->_parent = val;	
		val->_cyclic = 0;
		val->_flag = 0;
		//val->_nodelay = 0;
		val->_isEmptyString = 0;
		val->_reachable = 0; 
		val->_deep = 0;
		val->_state = -1;

		val->_isolate = retval ->_isolate;	

		/**********************************************************************/
		/* push the retval to semantic stack, so this variable can be seen */
		/* within expr or stmt 					  	*/
		/**********************************************************************/
		parser_insert_ident (eng, val);
		retval->data = (char *)val;
	}


	return retval;

}

/*****************************************************************************/
/* nel_dec () is called when a declaration for a routine or    */
/* data object is encountered.  it creates/modifies the appropriate symbol   */
/* table entrie(s).                                                          */
/*****************************************************************************/
nel_symbol *nel_dec (struct nel_eng *eng, register char *name, register nel_type *type, register int stor_class)
{
	nel_symbol *retval=NULL;
	nel_debug ({
		nel_trace (eng, "entering nel_dec [\neng = 0x%x\nname = %s\ntype =\n%1Tstor_class = %N\n\n", eng, name, type, stor_class);
	});

	if ((name == NULL) || (*name == '\0') ){
		parser_fatal_error (eng, "(nel_dec #1): NULL or empty name");
	}

	if (name[0] == '$') {		
		/*can't declare an variable or event as $x */
		if (stor_class == nel_T_ATOM || stor_class == nel_T_EVENT){
			parser_fatal_error (eng, "event name can't start with $");
		}else {
			parser_fatal_error (eng, "variable name can't start with $");
		}
	}
	else if (name[0] == '_') {		
		/*can't declare an variable or event as _xxx */
		if (stor_class == nel_T_ATOM || stor_class == nel_T_EVENT){
			parser_fatal_error (eng, "event name can't start with _");
		}else {
			parser_fatal_error (eng, "variable name can't start with _");
		}
	}

	if (type == NULL) {
		parser_fatal_error (eng, "(nel_dec #2): NULL type");
	}

	/*******************************/
	/* first, handle typedef names */
	/*******************************/
	if (stor_class == nel_T_TYPEDEF) {
		/********************************************/
		/* check for a redeclaration of the symbol. */
		/********************************************/
		register nel_symbol *old_sym = parser_lookup_ident (eng, name);
		if ((old_sym != NULL) && (old_sym->level != eng->parser->level)) {
			old_sym = NULL;
		}
		nel_debug ({
			nel_trace (eng, "old_sym =\n%1S\n", old_sym);
		});
		if (old_sym != NULL) {
			nel_debug ({
				if (old_sym->type == NULL) {
					parser_fatal_error (eng, "(nel_dec #3): NULL type");
				}
			});
			if ((eng->parser->level > 0) 
			|| (old_sym->type->simple.type != nel_D_TYPEDEF_NAME)
			|| nel_type_diff (old_sym->type->typedef_name.descriptor, type, 1)) {
				parser_error (eng, "redeclaration of %s", name);
			}
		} else {
			/*******************************************/
			/* allocate a symbol for the typedef name, */
			/* and insert it in the ident hash tables. */
			/* allocate a TYPEDEF_NAME type descriptor */
			/* for the symbol's type, and its value    */
			/* field is the type descriptor itself.    */
			/*******************************************/
			register nel_symbol *symbol = nel_static_symbol_alloc (eng, name, nel_type_alloc (eng, nel_D_TYPEDEF_NAME, 0, 0, 0, 0, type), (char *) type, nel_C_TYPE, 0, nel_L_NEL, eng->parser->level);
			parser_insert_ident (eng, symbol);

			/**************************************************/
			/* if this is a typedef at an inner scope, create */
			/* a stmt and append it to the current list.      */
			/**************************************************/
			if (eng->parser->level > 0) {
				register nel_stmt *stmt = nel_stmt_alloc (eng, nel_S_DEC, eng->parser->filename, eng->parser->line, symbol, NULL);
				append_stmt (stmt, &(stmt->dec.next));
			}
		}
	}

	/********************************************************************/
	/* create a new symbol for the function if there isn't one in the   */
	/* tables already and we are not at level 0.  if we are at an inner */
	/* level, we just ignore the declaration - the function must be     */
	/* defined by run-time in order to call it, anyway.                 */
	/********************************************************************/
	else if (type->simple.type == nel_D_FUNCTION) {
		if (eng->parser->level == 0) {
			/***************************/
			/* first, resolve the type */
			/***************************/
			type = parser_resolve_type (eng, type, name);
			nel_debug ({
			if (type == NULL) {
				parser_fatal_error (eng, "(nel_dec #6): NULL resolved type");
			}
			});

			switch (stor_class) {
			case nel_T_AUTO:
			case nel_T_REGISTER:
				parser_warning (eng, "function %s has illegal storage class", name);
				/* fall through */
			case nel_T_EXTERN:
			case nel_T_NULL:
				nel_func_dec (eng, name, type, 0);
				break;
			case nel_T_STATIC:
				nel_func_dec (eng, name, type, 1);
				break;
			default:
				parser_fatal_error (eng, "(nel_dec #7): bad class %N", stor_class);
				break;
			}
		}
	}


	/******************************************************/
	/* for global (or file local) data objects, call      */
	/* nel_global_dec () to create the symbol, */
	/* to set the old symbol's value, or just to check    */
	/* for a redeclaration of the symbol.                 */
	/******************************************************/
	else if (eng->parser->level == 0) {
		/**********************************************/
		/* don't resolve type if it 's an array.      */	 
		/**********************************************/
		//if(type->simple.type != nel_D_ARRAY){
			type = parser_resolve_type (eng, type, name);
		//}

		nel_debug ({
		if (type == NULL) {
			parser_fatal_error (eng, "(nel_dec #8): NULL resolved type");
		}
		});

		switch (stor_class) {
		case nel_T_AUTO:
		case nel_T_REGISTER:
			parser_warning (eng, "illegal class (%N) for %s", stor_class, name);
			/* fall through */
		case nel_T_EXTERN:
		case nel_T_NULL:
			retval = nel_global_dec (eng, name, type, 0);
			break;
		case nel_T_STATIC:
			retval = nel_global_dec (eng, name, type, 1);
			break;
		case nel_T_ATOM:
			retval = nel_evt_dec(eng, name, type, nel_C_TERMINAL);
			break;
		case nel_T_EVENT:
			retval = nel_evt_dec(eng, name, type, nel_C_NONTERMINAL);
			break;

		default:
			parser_fatal_error (eng, "(nel_dec #9): bad class %N", stor_class);
			break;
		}
	}

	/***************************************************/
	/* for routine-local data objects, call            */
	/* nel_local_dec () to create the symbol, */
	/* or to set the old symbol's value, then create   */
	/* a stmt structure around the symbol, and insert  */
	/* it in the stmt list.  do not resolve the type   */
	/* descriptor, as nel_local_dec () will   */
	/* do this if it needs to (for static vars).       */
	/***************************************************/
	else /* eng->parser->level > 0 */
	{
		register nel_symbol *symbol;
		switch (stor_class) {
		case nel_T_EXTERN:
			parser_warning (eng, "illegal class (%N) for %s", stor_class, name);
			/* fall through */
		case nel_T_AUTO:
		case nel_T_REGISTER:
		case nel_T_NULL:

			/*NOTE, NOTE, NOTE, need check if the following resolve is OK   */
			type = parser_resolve_type (eng, type, name);
			symbol = nel_local_dec (eng, name, type, 0);
			break;

		case nel_T_STATIC:
			symbol = nel_local_dec (eng, name, type, 1);
			break;
		default:
			parser_fatal_error (eng, "(nel_dec #10): bad class %N", stor_class);
			break;
		}

		/****************************************/
		/* if there was no error, create a stmt */
		/* structure for the declaration.       */
		/****************************************/
		if (symbol != NULL) {
			register nel_stmt *stmt = nel_stmt_alloc (eng, nel_S_DEC, eng->parser->filename, eng->parser->line, symbol, NULL);
			append_stmt (stmt, &(stmt->dec.next));
		}

		retval = symbol;

	}

	nel_debug ({
		nel_trace (eng, "] exiting nel_dec\n\n");
	});
	return retval;
}



