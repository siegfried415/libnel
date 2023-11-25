
/*****************************************************************************/
/* This file, "debug.c", contains the routines for reading in characters    */
/* from the input stream and printing out any trace or error messages from   */
/* the NetEye Engine.                                                */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include <stdarg.h>
#include <setjmp.h>

#include "engine.h"
#include "errors.h"
#include "io.h"
#include "type.h"
#include "intp.h"
#include "lex.h"
#include "mem.h"
#include "_stab.h" 

//int verbose_level = 0;
//unsigned_int nel_silent = 0;	/* repress diagnostic messages		*/
int nel_stderr_backspaces;	/* # chars to backspace before printing	*/

//int debug_level = 0;
#ifdef NEL_DEBUG
//unsigned_int nel_tracing = 1;	/* tracing flag 			*/
int nel_stdout_backspaces;	/* # char to backspace before printing	*/
#endif /* NEL_DEBUG */


/********************************************************************/
/* when nel_print_brief_types is nonzero, types descriptors (printed */
/* with " %T") for structures and unions do not have their members  */
/* listed, type descriptors for enumerated types do not have the    */
/* constant list printed, and type descriptors for functions do not */
/* have the lists of arguments and local variables printed.         */
/********************************************************************/
unsigned_int nel_print_brief_types = 1;

#define tab(_indent); \
	{								\
	   register int __indent = (_indent);				\
	   int __i;							\
	   for (__i = 0; (__i < __indent); __i++) {			\
	      fprintf (file, "   ");					\
	   }								\
	}



/*****************************************************************************/
/* nel_pretty_print () prints out <*obj> to <file> in a pretty format, by     */
/* traversing its type descriptor, <type>.  <indent> is the indentation      */
/* level.  if it is negative, we do not indent the first line, but we do     */
/* indent any subsequent lines, as would exist in the case of a structure.   */
/*****************************************************************************/
void nel_pretty_print (FILE *file, register nel_type *type, char *obj, int indent)
{
	if (type == NULL) {
		fprintf (file, "illegally_typed_object");
	} else if (obj == NULL) {
		fprintf (file, "NULL_valued_object");
	} else {
		switch (type->simple.type) {

		case nel_D_CHAR:
		case nel_D_SIGNED_CHAR:
		case nel_D_UNSIGNED_CHAR:
			tab (indent);
			fprintf (file, "'%c'", *((char *) obj));
			break;

		case nel_D_SHORT:
		case nel_D_SHORT_INT:
		case nel_D_SIGNED:
		case nel_D_SIGNED_SHORT:
		case nel_D_SIGNED_SHORT_INT:
		case nel_D_UNSIGNED_SHORT:
		case nel_D_UNSIGNED_SHORT_INT:
			tab (indent);
			fprintf (file, "%d", (int) (*((short *) obj)));
			break;

		case nel_D_INT:
		case nel_D_SIGNED_INT:
		case nel_D_UNSIGNED:
		case nel_D_UNSIGNED_INT:
			tab (indent);
			fprintf (file, "%d", *((int *) obj));
			break;

		case nel_D_LONG:
		case nel_D_LONG_INT:
		case nel_D_SIGNED_LONG:
		case nel_D_SIGNED_LONG_INT:
		case nel_D_UNSIGNED_LONG:
		case nel_D_UNSIGNED_LONG_INT:
			tab (indent);
			fprintf (file, "%ld", *((long *) obj));
			break;

		case nel_D_FLOAT:
			tab (indent);
			fprintf (file, "%f", (double) (*((float *) obj)));
			break;

		case nel_D_DOUBLE:
			tab (indent);
			fprintf (file, "%f", *((double *) obj));
			break;

		case nel_D_LONG_DOUBLE:
			tab (indent);
			fprintf (file, "%Lf", *((long_double *) obj));
			break;

		case nel_D_FUNCTION:
			/*****************************************/
			/* just print its address of a function. */
			/*****************************************/
			tab (indent);
			fprintf (file, "0x%p\n", (void *) obj);
			break;

		case nel_D_VOID:
			/***************************************/
			/* assume a void type is just a word,  */
			/* so print it in hexadecimal.         */
			/***************************************/
			tab (indent);
			fprintf (file, "0x%p", (void *) obj);
			break;

		case nel_D_ENUM:
			tab (indent);
			{
				nel_list *list = type->enumed.consts;
				for (; (list != NULL); list = list->next) {
					if ((list->value == *((int *) obj)) && (list != NULL) &&
							(list->symbol != NULL) && (list->symbol->name != NULL)) {
						fprintf (file, "%s", list->symbol->name);
						break;
					}
				}
				if (list == NULL) {
					fprintf (file, "%d", *((int *) obj));
				}
			}
			break;

		case nel_D_STRUCT:
			/*************************************************/
			/* print out the srtuct in the following format: */
			/*                                               */
			/*    struct <tag_name> {                        */
			/*       <member_name>: <member>                 */
			/*       <member_name>: <member>                 */
			/*               .                               */
			/*               .                               */
			/*               .                               */
			/*    }                                          */
			/*     ^ no newline here                         */
			/*************************************************/
			tab (indent);
			fprintf (file, "struct ");
			if (type->s_u.tag != NULL) {
				nel_print_symbol_name (file, type->s_u.tag, 0);
				fprintf (file, " ");
			}
			fprintf (file, "{\n");
			if (indent < 0) {
				indent = (- indent);
			}
			{
				nel_member *members = type->s_u.members;
				for (; (members != NULL); members = members->next) {
					if ((members->symbol == NULL) || (members->symbol->type == NULL)
							|| (members->symbol->type->simple.size <= 0)) {
						continue;
					}
					nel_print_symbol_name (file, members->symbol, indent + 1);
					fprintf (file, ": ");
					if (! members->bit_field) {
						nel_pretty_print (file, members->symbol->type, obj + members->offset, - (indent + 1));
					} else {
						register nel_type *type = members->symbol->type;
						unsigned_int word;
						char *value;
						nel_copy (sizeof (unsigned_int), &word, obj + members->offset);
						nel_alloca (value, members->symbol->type->simple.size, char);
						nel_extract_bit_field (NULL, type, value, word, members->bit_lb, members->bit_size);
						nel_pretty_print (file, type, value, - (indent + 1));
						nel_dealloca (value);
					}
					fprintf (file, "\n");
				}
			}

			/***********************************************/
			/* don't print a newline after the closing '}' */
			/***********************************************/
			tab (indent);
			fprintf (file, "}");
			break;

		case nel_D_UNION:
			/*********************************/
			/* no way to know what member is */
			/* currently valid in the union. */
			/*********************************/
			tab (indent);
			fprintf (file, "union ");
			if (type->s_u.tag != NULL) {
				nel_print_symbol_name (file, type->s_u.tag, 0);
			}
			break;

		case nel_D_POINTER:
			tab (indent);
			if (*((char **) obj) == NULL) {
				fprintf (file, "NULL");
			} else {
				fprintf (file, "0x%p", *((char **) obj));
			}
			break;

		case nel_D_ARRAY:
			tab (indent);
			if ((type->array.size <= 0) || (! type->array.known_lb) ||
					(! type->array.known_ub) || (type->array.base_type == NULL)) {
				fprintf (stderr, "0x%p", obj);
			}
			if ((type->array.base_type != NULL) &&
					((type->array.base_type->simple.type == nel_D_CHAR) ||
					 (type->array.base_type->simple.type == nel_D_SIGNED_CHAR) ||
					 (type->array.base_type->simple.type == nel_D_UNSIGNED_CHAR))) {
				/**************************************************/
				/* if the base type is char and we know the size, */
				/* print out the string.  maximum length is 999.  */
				/* we know sizeof (char) == 1, so use the size    */
				/* field of the descriptor and don't worry about  */
				/* nonzero lower bounds.                          */
				/**************************************************/
				if (type->array.size < 999) {
					char format[7];  /* 7 == strlen ("%-nnns") + 1 */
					sprintf (format, "%%.%-ds", type->array.size);
					fprintf (file, format, obj);
				} else {
					fprintf (file, "%999s", obj);
				}
			} else {
				/************************************************/
				/* print out the array in the following format: */
				/*                                              */
				/*    {                                         */
				/*       <lb>:   <value>                        */
				/*       <lb+1>: <value>                        */
				/*       <lb+2>: <value>                        */
				/*    }                                         */
				/*     ^ no newline here                        */
				/************************************************/
				register int i;
				if (indent < 0) {
					indent = (- indent);
				}
				fprintf (file, "{\n");
				for (i = type->array.lb.value; (i <= type->array.ub.value); i++) {
					tab (indent + 1);
					fprintf (file, "%d: ", i);
					nel_pretty_print (file, type->array.base_type, obj + ((i - type->array.lb.value)
									  * type->array.base_type->simple.size), - (indent + 1));
					fprintf (file, "\n");
				}
				tab (indent);
				fprintf (file, "}");
			}
			break;

		case nel_D_ENUM_TAG:
		case nel_D_FILE:
		case nel_D_STAB_UNDEF:
		case nel_D_STRUCT_TAG:
		case nel_D_TYPE_NAME:
		case nel_D_TYPEDEF_NAME:
		case nel_D_UNION_TAG:
		default:
			fprintf (file, "illegally_typed_object");
			break;
		}
	}
}


/*****************************************************************************/
/* nel_do_print () prints out a message message to the screen.  It takes 3    */
/* arguments, the file where the output should go, a pointer to the control  */
/* string, and a pointer to the start of the argument list, which is         */
/* accessed using the routines defined in "stdarg.h"                         */
/*                                                                           */
/* the following extra print formats are added in addition of %d, %f, etc.:  */
/* some formats are only supported in versions of the library that include   */
/* the interpreter; others formats are only supported in versions that       */
/* include the stab scanner.                                                 */
/*                                                                           */
/* %A print out the list of line/address structures (nel_line *arg)           */
/* %B print out the list of block structures (nel_block *arg)                 */
/* %C print out the nel_C_token (nel_C_token arg)                              */
/* %D print out the nel_D_token (nel_D_token arg)                              */
/* %H print out the the nel_L_token (nel_L_token arg)                          */
/* %I print out the symbol's name, an Identifier (nel_symbol *arg)            */
/* %J print out the nel_S_token (nel_S_token arg)                    */
/* %K print out the statement structure (nel_stmt *arg)                  */
/* %L print out the list of symbols (nel_list *arg)                           */
/* %M print out the list of member structures (nel_member *arg)               */
/* %N print out the string representation of the token (int arg)             */
/* %O print out the nel_O_token (nel_O_token arg)                    */
/* %P print out the string representation of the nel_O_token             */
/*      (nel_O_token arg)                                                */
/* %R print out the the nel_R_token (nel_R_token arg)                          */
/* %S print out the symbol structure (nel_symbol *arg)                        */
/* %T print out the type descriptor (nel_type *arg)                           */
/* %U print out the single nel_stab_type structure (nel_stab_type *arg)        */
/* %V print out the list of nel_stab_type structures (nel_stab_type *arg)      */
/* %X print out the expression tree (nel_expr *arg)                      */
/* %Y print out the list of expression trees (nel_expr *arg)             */
/* %Z pretty-print the object (nel_type *arg_type, void *arg_address)         */
/*                                                                           */
/* for all formats except %C and %D, an integer number may appear after the  */
/* '%', designating the number of tabs to indent when printing.              */
/*                                                                           */
/*****************************************************************************/
void nel_do_print (FILE *file, char *control_string, va_list args)
{
	register char *scan;/* scans control_string 			*/

	/********************************************************************/
	/* backspace over a overwritable message if file == stderr. */
	/* we then have to print as many blanks, and backspace again, or    */
	/* part of the overwritable message will still show if it was       */
	/* longer than the new message.                                     */
	/********************************************************************/
	if (file == stderr) {
		register int i;
		for (i = nel_stderr_backspaces; (i > 0); i--) {
			fprintf (file, "\b");
		}
		for (i = nel_stderr_backspaces; (i > 0); i--) {
			fprintf (file, " ");
		}
		for (i = nel_stderr_backspaces; (i > 0); i--) {
			fprintf (file, "\b");
		}
		nel_stderr_backspaces = 0;
	}

#ifdef NEL_DEBUG

	if (file == stdout) {
		register int i = nel_stdout_backspaces;
		for (; (i > 0); i--) {
			fprintf (file, "\b");
		}
		nel_stdout_backspaces = 0;
	}

#endif /* NEL_DEBUG */

	/*************************/
	/* now print the message */
	/*************************/
	scan = control_string;
	while (*scan != '\0') {

		/* 
		       if (*scan != '%') {
		          fprintf (file, "%c", *(scan++));
		       }
		*/

		if (*scan == '\\' ) {
			switch (*(++ scan)) {
			case 'n' :
				fprintf(file, "%c", '\n');
				scan++;
				break;
			case 't' :
				fprintf(file, "%c", '\t');
				scan++;
				break;
				//case 'o' :
				//	fprintf(file, "%c", '\o'); scan++; break;
			case 'r' :
				fprintf(file, "%c", '\r');
				scan++;
				break;
			case 'b' :
				fprintf(file, "%c", '\b');
				scan++;
				break;
			case 'f' :
				fprintf(file, "%c", '\f');
				scan++;
				break;
			case '\'' :
				fprintf(file, "%c", '\'');
				scan++;
				break;
			case '\"' :
				fprintf(file, "%c", '\"');
				scan++;
				break;
			case '\\' :
				fprintf(file, "%c", '\\');
				scan++;
				break;
			default:
				/* for those are not ilegal pair, just
				 output '\\' and scan , so go back */
				fprintf(file, "%c", '\\');
				break;
			}
		}
		else if (*scan == '%') {
			register char *scan2;/* scan control_string for conv char	*/
			register char *length;/* long or short? points to 'h' or 'l'.*/
			char *control;	/* stores conversion chars		*/
			register int clen;	/* length of control			*/
			int indent;		/* indentation level			*/

			/********************************************************/
			/* advance scan2 to the format descriptor.  make length */
			/* point to an 'h' or an 'l', if one is found.          */
			/********************************************************/
			length = NULL;
			for (scan2 = scan + 1, clen = 2;
					((*scan2 == '-') || (*scan2 == '.') ||
					 ((*scan2 >= '0') && (*scan2 <= '9')) ||
					 ((*scan2 == 'h') ? ((length = scan2), 1) : 0) ||
					 ((*scan2 == 'l') ? ((length = scan2), 1) : 0)); scan2++, clen++)
				;

			/***********************************************/
			/* copy portion of *control_string to *control */
			/***********************************************/
			nel_alloca (control, clen + 1, char);
			nel_copy (clen * sizeof (char), control, scan);
			*(control + clen) = '\0';

			/************************************************/
			/* if the *control contians a number, read it's */
			/* value.  otherwise, indent is 0 by default.   */
			/************************************************/
			if (sscanf (scan + 1, "%d", &indent) == 0) {
				indent = 0;
			}

			switch (*scan2) {

				/********************************/
				/* print out a list of nel_lines */
				/********************************/
			case 'A':
				nel_print_lines (file, va_arg (args, nel_line *), indent);
				break;

				/*********************************/
				/* print out a list of nel_blocks */
				/*********************************/
			case 'B':
				nel_print_blocks (file, va_arg (args, nel_block *), indent);
				break;

				/***************************/
				/* print out an nel_C_token */
				/***************************/
			case 'C':
				fprintf (file, "%s", nel_C_name (va_arg (args, nel_C_token)));
				break;

				/***************************/
				/* print out an nel_D_token */
				/***************************/
			case 'D':
				fprintf (file, "%s", nel_D_name (va_arg (args, nel_D_token)));
				break;

				/***************************/
				/* print out an nel_L_token */
				/***************************/
			case 'H':
				fprintf (file, "%s", nel_L_name (va_arg (args, nel_L_token)));
				break;

				/*********************************************/
				/* print out a symbol's name, and Identifier */
				/*********************************************/
			case 'I':
				nel_print_symbol_name (file, va_arg (args, nel_symbol *), indent);
				break;

#ifdef NEL_NTRP

				/*************************************/
				/* print out the statement structure */
				/*************************************/
			case 'J':
				fprintf (file, "%s", nel_S_name (va_arg (args, nel_S_token)));
				break;

				/*************************************/
				/* print out the statement structure */
				/*************************************/
			case 'K':
				nel_print_stmt (file, va_arg (args, nel_stmt *), indent);
				break;

#endif /* NEL_NTRP */

				/*******************************/
				/* print out a list of symbols */
				/*******************************/
			case 'L':
				nel_print_list (file, va_arg (args, nel_list *), indent);
				break;

				/**********************************/
				/* print out a list of nel_members */
				/**********************************/
			case 'M':
				nel_print_members (file, va_arg (args, nel_member *), indent);
				break;

#ifdef NEL_NTRP

				/**************************************************/
				/* print out the string representation of a token */
				/**************************************************/
			case 'N':
				fprintf (file, "%s", nel_token_string (va_arg (args, int)));
				break;

				/********************************/
				/* print out an nel_O_token */
				/********************************/
			case 'O':
				fprintf (file, "%s", nel_O_name (va_arg (args, nel_O_token)));
				break;

				/*************************************************************/
				/* print out the string representation of an nel_O_token */
				/*************************************************************/
			case 'P':
				fprintf (file, "%s", nel_O_string (va_arg (args, nel_O_token)));
				break;

#endif /* NEL_NTRP */

				/***************************/
				/* print out an nel_R_token */
				/***************************/
			case 'R':
				fprintf (file, "%s", nel_R_name (va_arg (args, nel_R_token)));
				break;

				/********************************/
				/* print out a symbol structure */
				/********************************/
			case 'S':
				nel_print_symbol (file, va_arg (args, nel_symbol *), indent);
				break;

				/*******************************/
				/* print out a type descriptor */
				/*******************************/
			case 'T':
				nel_print_type (file, va_arg (args, nel_type *), indent);
				break;

				/*********************************************/
				/* print out a single nel_stab_type structure */
				/*********************************************/
			case 'U':
				stab_print_type (file, va_arg (args, nel_stab_type *), indent);
				break;

				/*******************************************************/
				/* print out an entire list of nel_stab_type structures */
				/*******************************************************/
			case 'V':
				stab_print_types (file, va_arg (args, nel_stab_type *), indent);
				break;

#ifdef NEL_NTRP

				/*********************************/
				/* print out the expression tree */
				/*********************************/
			case 'X':
				nel_print_expr (file, va_arg (args, nel_expr *), indent);
				break;

				/****************************************/
				/* print out a list of expression trees */
				/****************************************/
			case 'Y':
				nel_print_expr_list (file, va_arg (args, nel_expr_list *), indent);
				break;

#endif /* NEL_NTRP */

				/***********************************/
				/* pretty-print out the object by  */
				/* traversing its type descriptor. */
				/************************************/
			case 'Z': {
					register nel_type *type;
					register char *obj;
					type = va_arg (args, nel_type *);
					obj = va_arg (args, char*);
					nel_pretty_print (file, type, obj, indent);
				}
				break;

				/***********************************************************/
				/* other arguments - %d, %o, etc.  advance arg pointer,    */
				/* overwrite the character after the conversion char with  */
				/* '\0', call fprintf () to do the printing, then restore */
				/* the overwritten char.                                   */
				/***********************************************************/
				/********************/
				/* print out a char */
				/********************/
			case 'c':
				/****************************************************/
				/* chars are coerced to ints in the subroutine call */
				/****************************************************/
				fprintf (file, control, va_arg (args, int));
				break;

				/****************************************/
				/* print out an int, in various formats */
				/****************************************/
			case 'd':
			case 'i':
			case 'o':
			case 'x':
			case 'u':
				if (length == NULL) {
					fprintf (file, control, va_arg (args, int));
				} else if (*length == 'l') {
					fprintf (file, control, va_arg (args, long));
				} else { 	/* length == 'h' */
					/*****************************************************/
					/* shorts are coerced to ints in the subroutine call */
					/*****************************************************/
					fprintf (file, control, va_arg (args, int));
				}
				break;

				/**********************/
				/* print out a string */
				/**********************/
			case 's': {
					char *string = va_arg (args, char *);
					if (string == NULL) {
						fprintf (file, "NULL");
					} else {
						fprintf (file, control, string);
					}
				}
				break;

				/**********************/
				/* print out a double */
				/**********************/
			case 'e':
			case 'E':
			case 'f':
			case 'g':
			case 'G':
				fprintf (file, control, va_arg (args, double));
				break;

				/***********************/
				/* print out a pointer */
				/***********************/
			case 'p':
				fprintf (file, control, va_arg (args, char *));
				break;

				/********************************************************/
				/* don't specify args or advance arg pointer on default */
				/********************************************************/
			default:
				fprintf (file, "%s", control);
				break;
			}
			nel_dealloca (control);
			scan = scan2 + 1;
		} else {
			fprintf (file, "%c", *(scan++));
		}
	}
	fflush (file);
}



/*****************************************************************************/
/* nel_make_file_prefix () converts <filename> to a legal C identifier by     */
/* replacing all illegal identifier characters with _'s.  If <filename>      */
/* begins with a digit, an '_' is preprended to the entire name.             */
/* no more than <n> - 1 character (plus a '\0') will be stored in <buffer>.  */
/* nel_read_name () returns strlen (prefix).                                  */
/*****************************************************************************/
int nel_make_file_prefix (struct nel_eng *eng, register char *prefix, register char *filename, register int n)
{
	register char *source = filename;
	register char *scan = prefix;
	register char ch = '_';	/* anything but '\0', ':', ';' */
	register int i = 0;

	/*********************************************/
	/* if the filename has a path in front, only */
	/* use the portion after the last '/'.       */
	/* scan to the end, then back to the '/', or */
	/* to the start.                             */
	/*********************************************/
	for (; (*source != '\0'); source++)
		;
	for (; ((*source != '/') && (source != filename)); source--)
		;
	if (*source == '/') {
		source++;
	}

	/**************************************************/
	/* prepend an '_' if the name starts with a digit */
	/**************************************************/
	if ((*source >= '0') && (*source <= '9') && (n > 1)) {
		*(scan++) = '_';
		i++;
	}

	/****************************************/
	/* now copy the file name to prefix[],  */
	/* replacing weird characters with '_'. */
	/****************************************/
	for (;(((ch = *(source++)) != '\0') && (i < n - 1)); i++, *(scan++) = ch) {
		if (! (((ch >= 'A') && (ch <= 'Z')) ||
				((ch >= 'a') && (ch <= 'z')) ||
				((ch >= '0') && (ch <= '9')) ||
				(ch == '`'))) {
			ch = '_';
		}
	}

	/**********************************/
	/* terminate the result with '\0' */
	/**********************************/
	*scan = '\0';

	/**********************/
	/* check for overflow */
	/**********************/
	if (ch != '\0') {
		*scan = '\0';
		//nel_warning (eng, "file prefix truncated to %s", prefix);
	}

	/************************************/
	/* return the length of the string. */
	/************************************/
	return (++i);
}

/*
 * rel2abs: convert an relative path name into absolute.
 *
 *	i)	path	relative path
 *	i)	base	base directory (must be absolute path)
 *	o)	result	result buffer
 *	i)	size	size of result buffer
 *	r)		!= NULL: absolute path
 *			== NULL: error
 */
char *rel2abs(const char *path, const char *base, char *result, int size)
{
	const char *pp, *bp;
	/*
	 * endp points the last position which is safe in the result buffer.
	 */
	const char *endp = result + size - 1;
	char *rp;
	int length;

	if (*path == '/') {
		if (strlen(path) >= size)
			goto erange;
		strcpy(result, path);
		goto finish;
	} else if (*base != '/' || !size) {
		//errno = EINVAL;
		return (NULL);
	} else if (size == 1)
		goto erange;

	length = strlen(base);

	if (!strcmp(path, ".") || !strcmp(path, "./")) {
		if (length >= size)
			goto erange;
		strcpy(result, base);
		/*
		 * rp points the last char.
		 */
		rp = result + length - 1;
		/*
		 * remove the last '/'.
		 */
		if (*rp == '/') {
			if (length > 1)
				*rp = 0;
		} else
			rp++;
		/* rp point NULL char */
		if (*++path == '/') {
			/*
			 * Append '/' to the tail of path name.
			 */
			*rp++ = '/';
			if (rp > endp)
				goto erange;
			*rp = 0;
		}
		goto finish;
	}
	bp = base + length;

	/* if be end with a nel filename, strip it  */
	if(/*!strcmp(bp-4, ".nel") */ 1 ) {
		while(*(bp-1) != '/') {
			bp--;
		}
	}

	/* this must be an directory, if a '/' at the end, strip it */
	if (*(bp - 1) == '/')
		--bp;

	/*
	 * up to root.
	 */
	for (pp = path; *pp && *pp == '.'; ) {
		if (!strncmp(pp, "../", 3)) {
			pp += 3;
			while (bp > base && *--bp != '/')
				;
		} else if (!strncmp(pp, "./", 2)) {
			pp += 2;
		} else if (!strncmp(pp, "..\0", 3)) {
			pp += 2;
			while (bp > base && *--bp != '/')
				;
		} else
			break;
	}
	/*
	 * down to leaf.
	 */
	length = bp - base;
	if (length >= size)
		goto erange;
	strncpy(result, base, length);
	rp = result + length;
	if (*pp || *(pp - 1) == '/' || length == 0)
		*rp++ = '/';
	if (rp + strlen(pp) > endp)
		goto erange;
	strcpy(rp, pp);
finish:
	return result;
erange:
	//errno = ERANGE;
	return (NULL);
}
