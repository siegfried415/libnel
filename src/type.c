
/****************************************************************************/
/* This file, "nel_types.c", contains routines for allocating the data type  */
/* descriptors for the application executive.                               */
/****************************************************************************/



#include <stdio.h>
#include <elf.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <setjmp.h>
#include <time.h>
#include <dlfcn.h>
#include <ar.h>
#include <ucontext.h>
#include <sys/reg.h>


#include "engine.h"
#include <errors.h>
#include "type.h"
#include "intp.h"
#include "lex.h"

//wyong, 20230801 
//#include "comp_elf.h"
//#include "comp_io.h"

#include "io.h"
#include "prod.h"
#include "mem.h"

/*****************************************************************************/
/* nel_D_name () returns the string that is the identifier for <D_token>, an  */
/* nel_D_token used as the discriminator in the type descriptors.             */
/*****************************************************************************/
char *nel_D_name (register nel_D_token D_token)
{
	switch (D_token) {
	case  nel_D_NULL:
		return ("nel_D_NULL");
	case  nel_D_ARRAY:
		return ("nel_D_ARRAY");
	case  nel_D_CHAR:
		return ("nel_D_CHAR");
	case  nel_D_COMMON:
		return ("nel_D_COMMON");
	case  nel_D_COMPLEX:
		return ("nel_D_COMPLEX");
	case  nel_D_COMPLEX_DOUBLE:
		return ("nel_D_COMPLEX_DOUBLE");
	case  nel_D_COMPLEX_FLOAT:
		return ("nel_D_COMPLEX_FLOAT");
	case  nel_D_DOUBLE:
		return ("nel_D_DOUBLE");
	case  nel_D_ENUM:
		return ("nel_D_ENUM");
	case  nel_D_ENUM_TAG:
		return ("nel_D_ENUM_TAG");
	case  nel_D_FLOAT:
		return ("nel_D_FLOAT");
	case  nel_D_FILE:
		return ("nel_D_FILE");
	case  nel_D_FUNCTION:
		return ("nel_D_FUNCTION");
	case  nel_D_INT:
		return ("nel_D_INT");
	case  nel_D_LABEL:
		return ("nel_D_LABEL");
	case  nel_D_LOCATION:
		return ("nel_D_LOCATION");
	case  nel_D_LONG:
		return ("nel_D_LONG");
	case  nel_D_LONG_COMPLEX:
		return ("nel_D_LONG_COMPLEX");
	case  nel_D_LONG_COMPLEX_DOUBLE:
		return ("nel_D_LONG_COMPLEX_DOUBLE");
	case  nel_D_LONG_COMPLEX_FLOAT:
		return ("nel_D_LONG_COMPLEX_FLOAT");
	case  nel_D_LONG_DOUBLE:
		return ("nel_D_LONG_DOUBLE");
	case  nel_D_LONG_INT:
		return ("nel_D_LONG_INT");
	case  nel_D_LONG_FLOAT:
		return ("nel_D_LONG_FLOAT");
	case  nel_D_POINTER:
		return ("nel_D_POINTER");
	case  nel_D_SHORT:
		return ("nel_D_SHORT");
	case  nel_D_SHORT_INT:
		return ("nel_D_SHORT_INT");
	case  nel_D_SIGNED:
		return ("nel_D_SIGNED");
	case  nel_D_SIGNED_CHAR:
		return ("nel_D_SIGNED_CHAR");
	case  nel_D_SIGNED_INT:
		return ("nel_D_SIGNED_INT");
	case  nel_D_SIGNED_LONG:
		return ("nel_D_SIGNED_LONG");
	case  nel_D_SIGNED_LONG_INT:
		return ("nel_D_SIGNED_LONG_INT");
	case  nel_D_SIGNED_SHORT:
		return ("nel_D_SIGNED_SHORT");
	case  nel_D_SIGNED_SHORT_INT:
		return ("nel_D_SIGNED_SHORT_INT");
	case  nel_D_STAB_UNDEF:
		return ("nel_D_STAB_UNDEF");
	case  nel_D_STRUCT:
		return ("nel_D_STRUCT");
	case  nel_D_STRUCT_TAG:
		return ("nel_D_STRUCT_TAG");
	case  nel_D_TYPE_NAME:
		return ("nel_D_TYPE_NAME");
	case  nel_D_TYPEDEF_NAME:
		return ("nel_D_TYPEDEF_NAME");
	case  nel_D_UNION:
		return ("nel_D_UNION");
	case  nel_D_UNION_TAG:
		return ("nel_D_UNION_TAG");
	case  nel_D_UNSIGNED:
		return ("nel_D_UNSIGNED");
	case  nel_D_UNSIGNED_CHAR:
		return ("nel_D_UNSIGNED_CHAR");
	case  nel_D_UNSIGNED_INT:
		return ("nel_D_UNSIGNED_INT");
	case  nel_D_UNSIGNED_LONG:
		return ("nel_D_UNSIGNED_LONG");
	case  nel_D_UNSIGNED_LONG_INT:
		return ("nel_D_UNSIGNED_LONG_INT");
	case  nel_D_UNSIGNED_SHORT:
		return ("nel_D_UNSIGNED_SHORT");
	case  nel_D_UNSIGNED_SHORT_INT:
		return ("nel_D_UNSIGNED_SHORT_INT");
	case  nel_D_VOID:
		return ("nel_D_VOID");
	case  nel_D_MAX:
		return ("nel_D_MAX");
	case nel_D_LONG_LONG:
		return "nel_D_LONG_LONG_INT";
	case nel_D_UNSIGNED_LONG_LONG:
		return "nel_D_UNSIGNED_LONG_LONG_INT";
	case nel_D_SIGNED_LONG_LONG:
		return "nel_D_SIGNED_LONG_LONG";
	case nel_D_EVENT:
		return "nel_D_EVENT";
	default:
		return ("nel_D_BAD_TOKEN");
	}
}


/*****************************************************************************/
/* nel_TC_name () returns the string that is the identifier for <D_token>, an  */
/* nel_D_token used as the discriminator in the type descriptors.             */
/*****************************************************************************/
char *nel_TC_name (register nel_D_token D_token)
{
	switch (D_token) {
	case  nel_D_NULL:
		return ("void");
		//case  nel_D_ARRAY:		return ("nel_D_ARRAY");
	case  nel_D_CHAR:
		return ("char ");
		//case  nel_D_COMMON:		return ("nel_D_COMMON");
		//case  nel_D_COMPLEX:		return ("nel_D_COMPLEX");
		//case  nel_D_COMPLEX_DOUBLE:	return ("nel_D_COMPLEX_DOUBLE");
		//case  nel_D_COMPLEX_FLOAT:	return ("nel_D_COMPLEX_FLOAT");
		//case  nel_D_DOUBLE:		return ("nel_D_DOUBLE");
		//case  nel_D_ENUM:		return ("nel_D_ENUM");
		//case  nel_D_ENUM_TAG:		return ("nel_D_ENUM_TAG");
		//case  nel_D_FLOAT:		return ("nel_D_FLOAT");
		//case  nel_D_FILE:		return ("nel_D_FILE");
		//case  nel_D_FUNCTION:		return ("nel_D_FUNCTION");
	case  nel_D_INT:
		return ("int");
		//case  nel_D_LABEL:		return ("nel_D_LABEL");
		//case  nel_D_LOCATION:		return ("nel_D_LOCATION");
	case  nel_D_LONG:
		return ("long");
		//case  nel_D_LONG_COMPLEX:	return ("nel_D_LONG_COMPLEX");
		//case  nel_D_LONG_COMPLEX_DOUBLE:return ("nel_D_LONG_COMPLEX_DOUBLE");
		//case  nel_D_LONG_COMPLEX_FLOAT:	return ("nel_D_LONG_COMPLEX_FLOAT");
		//case  nel_D_LONG_DOUBLE:	return ("nel_D_LONG_DOUBLE");
	case  nel_D_LONG_INT:
		return ("long int");
		//case  nel_D_LONG_FLOAT:		return ("nel_D_LONG_FLOAT");
	case  nel_D_POINTER:
		return (" *");
	case  nel_D_SHORT:
		return ("short");
	case  nel_D_SHORT_INT:
		return ("short int");
	case  nel_D_SIGNED:
		return ("signed");
	case  nel_D_SIGNED_CHAR:
		return ("signed char");
	case  nel_D_SIGNED_INT:
		return ("signed int");
	case  nel_D_SIGNED_LONG:
		return ("signed long");
		//case  nel_D_SIGNED_LONG_INT:	return ("nel_D_SIGNED_LONG_INT");
		//case  nel_D_SIGNED_SHORT:	return ("nel_D_SIGNED_SHORT");
		//case  nel_D_SIGNED_SHORT_INT:	return ("nel_D_SIGNED_SHORT_INT");
		//case  nel_D_STAB_UNDEF:		return ("nel_D_STAB_UNDEF");
	case  nel_D_STRUCT:
		return ("struct ");
		//case  nel_D_STRUCT_TAG:		return ("nel_D_STRUCT_TAG");
		//case  nel_D_TYPE_NAME:		return ("nel_D_TYPE_NAME");
		//case  nel_D_TYPEDEF_NAME:	return ("nel_D_TYPEDEF_NAME");
		//case  nel_D_UNION:		return ("nel_D_UNION");
		//case  nel_D_UNION_TAG:		return ("nel_D_UNION_TAG");
	case  nel_D_UNSIGNED:
		return ("unsigned");
	case  nel_D_UNSIGNED_CHAR:
		return ("unsigned char");
	case  nel_D_UNSIGNED_INT:
		return ("unsigned int");
	case  nel_D_UNSIGNED_LONG:
		return ("unsigned long");
	case  nel_D_UNSIGNED_LONG_INT:
		return ("unsigned long int");
	case  nel_D_UNSIGNED_SHORT:
		return ("unsigned short");
	case  nel_D_UNSIGNED_SHORT_INT:
		return ("unsigned short int");
	case  nel_D_VOID:
		return ("void");
		//case  nel_D_MAX:		return ("nel_D_MAX");
	default:
		return ("");
	}
}

/*****************************************************************************/
/* intp_coerce coerces a data object of type <old_type> pointed to by          */
/* <old_value>, and coerces it to type <new_type>, storing the result in     */
/* <new_value>.                                                              */
/*****************************************************************************/
int nel_coerce (struct nel_eng *eng, nel_type *new_type, char *new_value, nel_type *old_type, char *old_value)
{
	register nel_D_token old_D_token;
	register nel_D_token new_D_token;

	nel_debug ({ nel_trace (eng, "entering nel_coerce [\neng = 0x%x\nnew_type = \n%1Tnew_value = 0x%x\nold_type =\n%1Told_value = 0x%x\n\n", eng, new_type, new_value, old_type, old_value); });

	nel_debug ({
		if ((new_type == NULL) || (new_value == NULL) || (old_type == NULL) || (old_value == NULL)) {
			//intp_fatal_error (eng, "(intp_coerce #1): bad arguments for intp_coerce\nnew_type = \n%1Tnew_value = 0x%x\nold_type = \n%1Told_value = 0x%x", new_type, new_value, old_type, old_value);
			return -2;
		}
	});

	/*********************************************/
	/* find the standardized equivalent D_tokens */
	/*********************************************/
	new_D_token = nel_D_token_equiv (new_type->simple.type);
	old_D_token = nel_D_token_equiv (old_type->simple.type);

	switch (new_D_token) {
		/****************************************/
		/* include "intp_coerce_incl.h to perform */
		/* the coercion between simple types    */
		/****************************************/

	case nel_D_SIGNED_CHAR:
		switch (old_D_token) {
		case nel_D_SIGNED_CHAR:
			*((signed_char *) new_value) = *((signed_char *) old_value);
			break;
		case nel_D_COMPLEX_FLOAT:
			*((signed_char *) new_value) = *(nel_real_part (old_value, float));
			break;
		case nel_D_COMPLEX_DOUBLE:
			*((signed_char *) new_value) = *(nel_real_part (old_value, double));
			break;
		case nel_D_DOUBLE:
			*((signed_char *) new_value) = *((double *) old_value);
			break;
		case nel_D_SIGNED_INT:
			*((signed_char *) new_value) = *((signed_int *) old_value);
			break;
		case nel_D_FLOAT:
			*((signed_char *) new_value) = *((float *) old_value);
			break;
		case nel_D_LONG_COMPLEX_DOUBLE:
			*((signed_char *) new_value) = *(nel_real_part (old_value, long_double));
			break;
		case nel_D_LONG_DOUBLE:
			*((signed_char *) new_value) = *((long_double *) old_value);
			break;
		case nel_D_SIGNED_SHORT_INT:
			*((signed_char *) new_value) = *((signed_short_int *) old_value);
			break;
		case nel_D_UNSIGNED_INT:
			*((signed_char *) new_value) = *((unsigned_int *) old_value);
			break;
		case nel_D_UNSIGNED_CHAR:
			*((signed_char *) new_value) = *((unsigned_char *) old_value);
			break;
		case nel_D_UNSIGNED_SHORT_INT:
			*((signed_char *) new_value) = *((unsigned_short_int *) old_value);
			break;
		case nel_D_SIGNED_LONG_INT:
			*((signed_char *) new_value) = *((signed_long_int *) old_value);
			break;
		case nel_D_UNSIGNED_LONG_INT:
			*((signed_char *) new_value) = *((unsigned_long_int *) old_value);
			break;
		default:
			//intp_stmt_error (eng, "illegal type coercion");
			return -1;
		}
		break;

	case nel_D_COMPLEX_FLOAT:
		switch (old_D_token) {
		case nel_D_SIGNED_CHAR:
			*(nel_real_part (new_value, float)) = *((signed_char *) old_value);
			break;
		case nel_D_COMPLEX_FLOAT:
			*(nel_real_part (new_value, float)) = *(nel_real_part (old_value, float));
			*(nel_imaginary_part (new_value, float)) = *(nel_imaginary_part (old_value, float));
			break;
		case nel_D_COMPLEX_DOUBLE:
			*(nel_real_part (new_value, float)) = *(nel_real_part (old_value, double));
			*(nel_imaginary_part (new_value, float)) = *(nel_imaginary_part (old_value, double));
			break;
		case nel_D_DOUBLE:
			*(nel_real_part (new_value, float)) = *((double *) old_value);
			break;
		case nel_D_SIGNED_INT:
			*(nel_real_part (new_value, float)) = *((signed_int *) old_value);
			break;
		case nel_D_FLOAT:
			*(nel_real_part (new_value, float)) = *((float *) old_value);
			break;
		case nel_D_LONG_COMPLEX_DOUBLE:
			*(nel_real_part (new_value, float)) = *(nel_real_part (old_value, long_double));
			*(nel_imaginary_part (new_value, float)) = *(nel_imaginary_part (old_value, long_double));
			break;
		case nel_D_LONG_DOUBLE:
			*(nel_real_part (new_value, float)) = *((long_double *) old_value);
			break;
		case nel_D_SIGNED_SHORT_INT:
			*(nel_real_part (new_value, float)) = *((signed_short_int *) old_value);
			break;
		case nel_D_UNSIGNED_INT:
			*(nel_real_part (new_value, float)) = *((unsigned_int *) old_value);
			break;
		case nel_D_UNSIGNED_CHAR:
			*(nel_real_part (new_value, float)) = *((unsigned_char *) old_value);
			break;
		case nel_D_UNSIGNED_SHORT_INT:
			*(nel_real_part (new_value, float)) = *((unsigned_short_int *) old_value);
			break;
		default:
			//intp_stmt_error (eng, "illegal type coercion");
			return -1;
		}
		break;

	case nel_D_COMPLEX_DOUBLE:
		switch (old_D_token) {
		case nel_D_SIGNED_CHAR:
			*(nel_real_part (new_value, double)) = *((signed_char *) old_value);
			break;
		case nel_D_COMPLEX_FLOAT:
			*(nel_real_part (new_value, double)) = *(nel_real_part (old_value, float));
			*(nel_imaginary_part (new_value, double)) = *(nel_imaginary_part (old_value, float));
			break;
		case nel_D_COMPLEX_DOUBLE:
			*(nel_real_part (new_value, double)) = *(nel_real_part (old_value, double));
			*(nel_imaginary_part (new_value, double)) = *(nel_imaginary_part (old_value, double));
			break;
		case nel_D_DOUBLE:
			*(nel_real_part (new_value, double)) = *((double *) old_value);
			break;
		case nel_D_SIGNED_INT:
			*(nel_real_part (new_value, double)) = *((signed_int *) old_value);
			break;
		case nel_D_FLOAT:
			*(nel_real_part (new_value, double)) = *((float *) old_value);
			break;
		case nel_D_LONG_COMPLEX_DOUBLE:
			*(nel_real_part (new_value, double)) = *(nel_real_part (old_value, long_double));
			*(nel_imaginary_part (new_value, double)) = *(nel_imaginary_part (old_value, long_double));
			break;
		case nel_D_LONG_DOUBLE:
			*(nel_real_part (new_value, double)) = *((long_double *) old_value);
			break;
		case nel_D_SIGNED_SHORT_INT:
			*(nel_real_part (new_value, double)) = *((signed_short_int *) old_value);
			break;
		case nel_D_UNSIGNED_INT:
			*(nel_real_part (new_value, double)) = *((unsigned_int *) old_value);
			break;
		case nel_D_UNSIGNED_CHAR:
			*(nel_real_part (new_value, double)) = *((unsigned_char *) old_value);
			break;
		case nel_D_UNSIGNED_SHORT_INT:
			*(nel_real_part (new_value, double)) = *((unsigned_short_int *) old_value);
			break;
		default:
			//intp_stmt_error (eng, "illegal type coercion");
			return -1;
		}
		break;

	case nel_D_DOUBLE:
		switch (old_D_token) {
		case nel_D_SIGNED_CHAR:
			*((double *) new_value) = *((signed_char *) old_value);
			break;
		case nel_D_COMPLEX_FLOAT:
			*((double *) new_value) = *(nel_real_part (old_value, float));
			break;
		case nel_D_COMPLEX_DOUBLE:
			*((double *) new_value) = *(nel_real_part (old_value, double));
			break;
		case nel_D_DOUBLE:
			*((double *) new_value) = *((double *) old_value);
			break;
		case nel_D_SIGNED_INT:
			*((double *) new_value) = *((signed_int *) old_value);
			break;
		case nel_D_FLOAT:
			*((double *) new_value) = *((float *) old_value);
			break;
		case nel_D_LONG_COMPLEX_DOUBLE:
			*((double *) new_value) = *(nel_real_part (old_value, long_double));
			break;
		case nel_D_LONG_DOUBLE:
			*((double *) new_value) = *((long_double *) old_value);
			break;
		case nel_D_SIGNED_SHORT_INT:
			*((double *) new_value) = *((signed_short_int *) old_value);
			break;
		case nel_D_UNSIGNED_INT:
			*((double *) new_value) = *((unsigned_int *) old_value);
			break;
		case nel_D_UNSIGNED_CHAR:
			*((double *) new_value) = *((unsigned_char *) old_value);
			break;
		case nel_D_UNSIGNED_SHORT_INT:
			*((double *) new_value) = *((unsigned_short_int *) old_value);
			break;
		default:
			//intp_stmt_error (eng, "illegal type coercion");
			return -1;
		}
		break;

	case nel_D_SIGNED_INT:
		switch (old_D_token) {
		case nel_D_SIGNED_CHAR:
			*((signed_int *) new_value) = *((signed_char *) old_value);
			break;
		case nel_D_COMPLEX_FLOAT:
			*((signed_int *) new_value) = *(nel_real_part (old_value, float));
			break;
		case nel_D_COMPLEX_DOUBLE:
			*((signed_int *) new_value) = *(nel_real_part (old_value, double));
			break;
		case nel_D_DOUBLE:
			*((signed_int *) new_value) = *((double *) old_value);
			break;
		case nel_D_SIGNED_INT:
			*((signed_int *) new_value) = *((signed_int *) old_value);
			break;
		case nel_D_FLOAT:
			*((signed_int *) new_value) = *((float *) old_value);
			break;
		case nel_D_LONG_COMPLEX_DOUBLE:
			*((signed_int *) new_value) = *(nel_real_part (old_value, long_double));
			break;
		case nel_D_LONG_DOUBLE:
			*((signed_int *) new_value) = *((long_double *) old_value);
			break;
		case nel_D_SIGNED_SHORT_INT:
			*((signed_int *) new_value) = *((signed_short_int *) old_value);
			break;
		case nel_D_UNSIGNED_INT:
			*((signed_int *) new_value) = *((unsigned_int *) old_value);
			break;
		case nel_D_UNSIGNED_CHAR:
			*((signed_int *) new_value) = *((unsigned_char *) old_value);
			break;
		case nel_D_UNSIGNED_SHORT_INT:
			*((signed_int *) new_value) = *((unsigned_short_int *) old_value);
			break;
		case nel_D_SIGNED_LONG_INT:
			*((signed_int *) new_value) = *((signed_long_int *) old_value);
			break;
		case nel_D_UNSIGNED_LONG_INT:
			*((signed_int *) new_value) = *((unsigned_long_int *) old_value);
			break;
		default:
			//intp_stmt_error (eng, "illegal type coercion");
			return -1;
		}
		break;

	case nel_D_FLOAT:
		switch (old_D_token) {
		case nel_D_SIGNED_CHAR:
			*((float *) new_value) = *((signed_char *) old_value);
			break;
		case nel_D_COMPLEX_FLOAT:
			*((float *) new_value) = *(nel_real_part (old_value, float));
			break;
		case nel_D_COMPLEX_DOUBLE:
			*((float *) new_value) = *(nel_real_part (old_value, double));
			break;
		case nel_D_DOUBLE:
			*((float *) new_value) = *((double *) old_value);
			break;
		case nel_D_SIGNED_INT:
			*((float *) new_value) = *((signed_int *) old_value);
			break;
		case nel_D_FLOAT:
			*((float *) new_value) = *((float *) old_value);
			break;
		case nel_D_LONG_COMPLEX_DOUBLE:
			*((float *) new_value) = *(nel_real_part (old_value, long_double));
			break;
		case nel_D_LONG_DOUBLE:
			*((float *) new_value) = *((long_double *) old_value);
			break;
		case nel_D_SIGNED_SHORT_INT:
			*((float *) new_value) = *((signed_short_int *) old_value);
			break;
		case nel_D_UNSIGNED_INT:
			*((float *) new_value) = *((unsigned_int *) old_value);
			break;
		case nel_D_UNSIGNED_CHAR:
			*((float *) new_value) = *((unsigned_char *) old_value);
			break;
		case nel_D_UNSIGNED_SHORT_INT:
			*((float *) new_value) = *((unsigned_short_int *) old_value);
			break;
		default:
			//intp_stmt_error (eng, "illegal type coercion");
			return -1;
		}
		break;

	case nel_D_LONG_COMPLEX_DOUBLE:
		switch (old_D_token) {
		case nel_D_SIGNED_CHAR:
			*(nel_real_part (new_value, long_double)) = *((signed_char *) old_value);
			break;
		case nel_D_COMPLEX_FLOAT:
			*(nel_real_part (new_value, long_double)) = *(nel_real_part (old_value, float));
			*(nel_imaginary_part (new_value, long_double)) = *(nel_imaginary_part (old_value, float));
			break;
		case nel_D_COMPLEX_DOUBLE:
			*(nel_real_part (new_value, long_double)) = *(nel_real_part (old_value, double));
			*(nel_imaginary_part (new_value, long_double)) = *(nel_imaginary_part (old_value, double));
			break;
		case nel_D_DOUBLE:
			*(nel_real_part (new_value, long_double)) = *((double *) old_value);
			break;
		case nel_D_SIGNED_INT:
			*(nel_real_part (new_value, long_double)) = *((signed_int *) old_value);
			break;
		case nel_D_FLOAT:
			*(nel_real_part (new_value, long_double)) = *((float *) old_value);
			break;
		case nel_D_LONG_COMPLEX_DOUBLE:
			*(nel_real_part (new_value, long_double)) = *(nel_real_part (old_value, long_double));
			*(nel_imaginary_part (new_value, long_double)) = *(nel_imaginary_part (old_value, long_double));
			break;
		case nel_D_LONG_DOUBLE:
			*(nel_real_part (new_value, long_double)) = *((long_double *) old_value);
			break;
		case nel_D_SIGNED_SHORT_INT:
			*(nel_real_part (new_value, long_double)) = *((signed_short_int *) old_value);
			break;
		case nel_D_UNSIGNED_INT:
			*(nel_real_part (new_value, long_double)) = *((unsigned_int *) old_value);
			break;
		case nel_D_UNSIGNED_CHAR:
			*(nel_real_part (new_value, long_double)) = *((unsigned_char *) old_value);
			break;
		case nel_D_UNSIGNED_SHORT_INT:
			*(nel_real_part (new_value, long_double)) = *((unsigned_short_int *) old_value);
			break;
		default:
			//intp_stmt_error (eng, "illegal type coercion");
			return -1;
		}
		break;

	case nel_D_LONG_DOUBLE:
		switch (old_D_token) {
		case nel_D_SIGNED_CHAR:
			*((long_double *) new_value) = *((signed_char *) old_value);
			break;
		case nel_D_COMPLEX_FLOAT:
			*((long_double *) new_value) = *(nel_real_part (old_value, float));
			break;
		case nel_D_COMPLEX_DOUBLE:
			*((long_double *) new_value) = *(nel_real_part (old_value, double));
			break;
		case nel_D_DOUBLE:
			*((long_double *) new_value) = *((double *) old_value);
			break;
		case nel_D_SIGNED_INT:
			*((long_double *) new_value) = *((signed_int *) old_value);
			break;
		case nel_D_FLOAT:
			*((long_double *) new_value) = *((float *) old_value);
			break;
		case nel_D_LONG_COMPLEX_DOUBLE:
			*((long_double *) new_value) = *(nel_real_part (old_value, long_double));
			break;
		case nel_D_LONG_DOUBLE:
			*((long_double *) new_value) = *((long_double *) old_value);
			break;
		case nel_D_SIGNED_SHORT_INT:
			*((long_double *) new_value) = *((signed_short_int *) old_value);
			break;
		case nel_D_UNSIGNED_INT:
			*((long_double *) new_value) = *((unsigned_int *) old_value);
			break;
		case nel_D_UNSIGNED_CHAR:
			*((long_double *) new_value) = *((unsigned_char *) old_value);
			break;
		case nel_D_UNSIGNED_SHORT_INT:
			*((long_double *) new_value) = *((unsigned_short_int *) old_value);
			break;
		default:
			//intp_stmt_error (eng, "illegal type coercion");
			return -1;
		}
		break;

	case nel_D_SIGNED_SHORT_INT:
		switch (old_D_token) {
		case nel_D_SIGNED_CHAR:
			*((signed_short_int *) new_value) = *((signed_char *) old_value);
			break;
		case nel_D_COMPLEX_FLOAT:
			*((signed_short_int *) new_value) = *(nel_real_part (old_value, float));
			break;
		case nel_D_COMPLEX_DOUBLE:
			*((signed_short_int *) new_value) = *(nel_real_part (old_value, double));
			break;
		case nel_D_DOUBLE:
			*((signed_short_int *) new_value) = *((double *) old_value);
			break;
		case nel_D_SIGNED_INT:
			*((signed_short_int *) new_value) = *((signed_int *) old_value);
			break;
		case nel_D_FLOAT:
			*((signed_short_int *) new_value) = *((float *) old_value);
			break;
		case nel_D_LONG_COMPLEX_DOUBLE:
			*((signed_short_int *) new_value) = *(nel_real_part (old_value, long_double));
			break;
		case nel_D_LONG_DOUBLE:
			*((signed_short_int *) new_value) = *((long_double *) old_value);
			break;
		case nel_D_SIGNED_SHORT_INT:
			*((signed_short_int *) new_value) = *((signed_short_int *) old_value);
			break;
		case nel_D_UNSIGNED_INT:
			*((signed_short_int *) new_value) = *((unsigned_int *) old_value);
			break;
		case nel_D_UNSIGNED_CHAR:
			*((signed_short_int *) new_value) = *((unsigned_char *) old_value);
			break;
		case nel_D_UNSIGNED_SHORT_INT:
			*((signed_short_int *) new_value) = *((unsigned_short_int *) old_value);
			break;
		case nel_D_SIGNED_LONG_INT:
			*((signed_short_int *) new_value) = *((signed_long_int *) old_value);
			break;
		case nel_D_UNSIGNED_LONG_INT:
			*((signed_short_int *) new_value) = *((unsigned_long_int *) old_value);
			break;
		default:
			//intp_stmt_error (eng, "illegal type coercion");
			return -1;
		}
		break;

	//bugfix, wyong, 20230817 
	case nel_D_SIGNED_LONG_INT:
		switch (old_D_token) {
		case nel_D_SIGNED_CHAR:
			*((signed_long_int *) new_value) = *((signed_char *) old_value);
			break;
		case nel_D_COMPLEX_FLOAT:
			*((signed_long_int *) new_value) = *(nel_real_part (old_value, float));
			break;
		case nel_D_COMPLEX_DOUBLE:
			*((signed_long_int *) new_value) = *(nel_real_part (old_value, double));
			break;
		case nel_D_DOUBLE:
			*((signed_long_int *) new_value) = *((double *) old_value);
			break;
		case nel_D_SIGNED_INT:
			*((signed_long_int *) new_value) = *((signed_int *) old_value);
			break;
		case nel_D_FLOAT:
			*((signed_long_int *) new_value) = *((float *) old_value);
			break;
		case nel_D_LONG_COMPLEX_DOUBLE:
			*((signed_long_int *) new_value) = *(nel_real_part (old_value, long_double));
			break;
		case nel_D_LONG_DOUBLE:
			*((signed_long_int *) new_value) = *((long_double *) old_value);
			break;
		case nel_D_SIGNED_SHORT_INT:
			*((signed_long_int *) new_value) = *((signed_short_int *) old_value);
			break;
		case nel_D_UNSIGNED_INT:
			*((signed_long_int *) new_value) = *((unsigned_int *) old_value);
			break;
		case nel_D_UNSIGNED_CHAR:
			*((signed_long_int *) new_value) = *((unsigned_char *) old_value);
			break;
		case nel_D_UNSIGNED_SHORT_INT:
			*((signed_long_int *) new_value) = *((unsigned_short_int *) old_value);
			break;
		case nel_D_SIGNED_LONG_INT:
			*((signed_long_int *) new_value) = *((signed_long_int *) old_value);
			break;
		case nel_D_UNSIGNED_LONG_INT:
			*((signed_long_int *) new_value) = *((unsigned_long_int *) old_value);
			break;
		default:
			//intp_stmt_error (eng, "illegal type coercion");
			return -1;
		}
		break;

	//bugfix, wyong, 20230821
	case nel_D_UNSIGNED_LONG_INT:
		switch (old_D_token) {
		case nel_D_SIGNED_CHAR:
			*((unsigned_long_int *) new_value) = *((signed_char *) old_value);
			break;
		case nel_D_COMPLEX_FLOAT:
			*((unsigned_long_int *) new_value) = *(nel_real_part (old_value, float));
			break;
		case nel_D_COMPLEX_DOUBLE:
			*((unsigned_long_int *) new_value) = *(nel_real_part (old_value, double));
			break;
		case nel_D_DOUBLE:
			*((unsigned_long_int *) new_value) = *((double *) old_value);
			break;
		case nel_D_SIGNED_INT:
			*((unsigned_long_int *) new_value) = *((signed_int *) old_value);
			break;
		case nel_D_FLOAT:
			*((unsigned_long_int *) new_value) = *((float *) old_value);
			break;
		case nel_D_LONG_COMPLEX_DOUBLE:
			*((unsigned_long_int *) new_value) = *(nel_real_part (old_value, long_double));
			break;
		case nel_D_LONG_DOUBLE:
			*((unsigned_long_int *) new_value) = *((long_double *) old_value);
			break;
		case nel_D_SIGNED_SHORT_INT:
			*((unsigned_long_int *) new_value) = *((signed_short_int *) old_value);
			break;
		case nel_D_UNSIGNED_INT:
			*((unsigned_long_int *) new_value) = *((unsigned_int *) old_value);
			break;
		case nel_D_UNSIGNED_CHAR:
			*((unsigned_long_int *) new_value) = *((unsigned_char *) old_value);
			break;
		case nel_D_UNSIGNED_SHORT_INT:
			*((unsigned_long_int *) new_value) = *((unsigned_short_int *) old_value);
			break;
		case nel_D_SIGNED_LONG_INT:
			*((unsigned_long_int *) new_value) = *((signed_long_int *) old_value);
			break;
		case nel_D_UNSIGNED_LONG_INT:
			*((unsigned_long_int *) new_value) = *((unsigned_long_int *) old_value);
			break;
		default:
			//intp_stmt_error (eng, "illegal type coercion");
			return -1;
		}
		break;

	case nel_D_UNSIGNED_INT:
		switch (old_D_token) {
		case nel_D_SIGNED_CHAR:
			*((unsigned_int *) new_value) = *((signed_char *) old_value);
			break;
		case nel_D_COMPLEX_FLOAT:
			*((unsigned_int *) new_value) = *(nel_real_part (old_value, float));
			break;
		case nel_D_COMPLEX_DOUBLE:
			*((unsigned_int *) new_value) = *(nel_real_part (old_value, double));
			break;
		case nel_D_DOUBLE:
			*((unsigned_int *) new_value) = *((double *) old_value);
			break;
		case nel_D_SIGNED_INT:
			*((unsigned_int *) new_value) = *((signed_int *) old_value);
			break;
		case nel_D_FLOAT:
			*((unsigned_int *) new_value) = *((float *) old_value);
			break;
		case nel_D_LONG_COMPLEX_DOUBLE:
			*((unsigned_int *) new_value) = *(nel_real_part (old_value, long_double));
			break;
		case nel_D_LONG_DOUBLE:
			*((unsigned_int *) new_value) = *((long_double *) old_value);
			break;
		case nel_D_SIGNED_SHORT_INT:
			*((unsigned_int *) new_value) = *((signed_short_int *) old_value);
			break;
		case nel_D_UNSIGNED_INT:
			*((unsigned_int *) new_value) = *((unsigned_int *) old_value);
			break;
		case nel_D_UNSIGNED_CHAR:
			*((unsigned_int *) new_value) = *((unsigned_char *) old_value);
			break;
		case nel_D_UNSIGNED_SHORT_INT:
			*((unsigned_int *) new_value) = *((unsigned_short_int *) old_value);
			break;
		case nel_D_SIGNED_LONG_INT:
			*((unsigned_int *) new_value) = *((signed_long_int *) old_value);
			break;
		case nel_D_UNSIGNED_LONG_INT:
			*((unsigned_int *) new_value) = *((unsigned_long_int *) old_value);
			break;
		default:
			//intp_stmt_error (eng, "illegal type coercion");
			return -1;
		}
		break;

	case nel_D_UNSIGNED_CHAR:
		switch (old_D_token) {
		case nel_D_SIGNED_CHAR:
			*((unsigned_char *) new_value) = *((signed_char *) old_value);
			break;
		case nel_D_COMPLEX_FLOAT:
			*((unsigned_char *) new_value) = *(nel_real_part (old_value, float));
			break;
		case nel_D_COMPLEX_DOUBLE:
			*((unsigned_char *) new_value) = *(nel_real_part (old_value, double));
			break;
		case nel_D_DOUBLE:
			*((unsigned_char *) new_value) = *((double *) old_value);
			break;
		case nel_D_SIGNED_INT:
			*((unsigned_char *) new_value) = *((signed_int *) old_value);
			break;
		case nel_D_FLOAT:
			*((unsigned_char *) new_value) = *((float *) old_value);
			break;
		case nel_D_LONG_COMPLEX_DOUBLE:
			*((unsigned_char *) new_value) = *(nel_real_part (old_value, long_double));
			break;
		case nel_D_LONG_DOUBLE:
			*((unsigned_char *) new_value) = *((long_double *) old_value);
			break;
		case nel_D_SIGNED_SHORT_INT:
			*((unsigned_char *) new_value) = *((signed_short_int *) old_value);
			break;
		case nel_D_UNSIGNED_INT:
			*((unsigned_char *) new_value) = *((unsigned_int *) old_value);
			break;
		case nel_D_UNSIGNED_CHAR:
			*((unsigned_char *) new_value) = *((unsigned_char *) old_value);
			break;
		case nel_D_UNSIGNED_SHORT_INT:
			*((unsigned_char *) new_value) = *((unsigned_short_int *) old_value);
			break;
		case nel_D_SIGNED_LONG_INT:
			*((unsigned_char *) new_value) = *((signed_long_int *) old_value);
			break;
		case nel_D_UNSIGNED_LONG_INT:
			*((unsigned_char *) new_value) = *((unsigned_long_int *) old_value);
			break;
		default:
			//intp_stmt_error (eng, "illegal type coercion");
			return -1;
		}
		break;

	case nel_D_UNSIGNED_SHORT_INT:
		switch (old_D_token) {
		case nel_D_SIGNED_CHAR:
			*((unsigned_short_int *) new_value) = *((signed_char *) old_value);
			break;
		case nel_D_COMPLEX_FLOAT:
			*((unsigned_short_int *) new_value) = *(nel_real_part (old_value, float));
			break;
		case nel_D_COMPLEX_DOUBLE:
			*((unsigned_short_int *) new_value) = *(nel_real_part (old_value, double));
			break;
		case nel_D_DOUBLE:
			*((unsigned_short_int *) new_value) = *((double *) old_value);
			break;
		case nel_D_SIGNED_INT:
			*((unsigned_short_int *) new_value) = *((signed_int *) old_value);
			break;
		case nel_D_FLOAT:
			*((unsigned_short_int *) new_value) = *((float *) old_value);
			break;
		case nel_D_LONG_COMPLEX_DOUBLE:
			*((unsigned_short_int *) new_value) = *(nel_real_part (old_value, long_double));
			break;
		case nel_D_LONG_DOUBLE:
			*((unsigned_short_int *) new_value) = *((long_double *) old_value);
			break;
		case nel_D_SIGNED_SHORT_INT:
			*((unsigned_short_int *) new_value) = *((signed_short_int *) old_value);
			break;
		case nel_D_UNSIGNED_INT:
			*((unsigned_short_int *) new_value) = *((unsigned_int *) old_value);
			break;
		case nel_D_UNSIGNED_CHAR:
			*((unsigned_short_int *) new_value) = *((unsigned_char *) old_value);
			break;
		case nel_D_UNSIGNED_SHORT_INT:
			*((unsigned_short_int *) new_value) = *((unsigned_short_int *) old_value);
			break;
		case nel_D_SIGNED_LONG_INT:
			*((unsigned_short_int *) new_value) = *((signed_long_int *) old_value);
			break;
		case nel_D_UNSIGNED_LONG_INT:
			*((unsigned_short_int *) new_value) = *((unsigned_long_int *) old_value);
			break;
		default:
			//intp_stmt_error (eng, "illegal type coercion");
			return -1;
		}
		break;

		/*****************************************/
		/* handle structures and unions manually */
		/*****************************************/
	case nel_D_UNION:
		if ((old_D_token == nel_D_UNION) && (new_type->s_u.tag != NULL) && (new_type->s_u.tag == old_type->s_u.tag)) {
			nel_copy (new_type->simple.size, new_value, old_value);
		} else {
			//intp_stmt_error (eng, "(intp_coerce #1): illegal type coercion (union)");
			return -1;
		}
		break;

	case nel_D_STRUCT:
		if ((old_D_token == nel_D_STRUCT) && (new_type->s_u.tag != NULL) && (new_type->s_u.tag == old_type->s_u.tag)) {
			nel_copy (new_type->simple.size, new_value, old_value);
		} else {
			//intp_stmt_error (eng, "(intp_coerce #2): illegal type coercion (struct)");
			return -1;
		}
		break;

	default:
		//intp_stmt_error (eng, "(intp_coerce #3): illegal type coercion (default)");
		//break;
		return -1;
	}

	nel_debug ({ nel_trace (eng, "] exiting intp_coerce\n\n"); });
	return 0;

}


/*****************************************************************************/
/* intp_extract_bit_field () extracts a sequence of bits from <word> and     */
/* copies them to *result.                                                   */
/*****************************************************************************/
int nel_extract_bit_field (struct nel_eng *eng, nel_type *type, char *result, unsigned_int word, register unsigned_int lb, register unsigned_int size)
{
	int retval = 0;
	nel_debug ({ nel_trace (eng, "entering intp_extract_bit_field [\neng = 0x%x\ntype =\n%1Tresult = 0x%x\nword = 0x%x\nlb = %d\nsize = %d\n\n", eng, type, result, word, lb, size); });

	nel_debug ({
	if (type == NULL) {
		//intp_fatal_error (eng, "(intp_extract_bit_field #1): NULL type");
		return -2;
	}
	if (result == NULL) {
		//intp_fatal_error (eng, "(intp_extract_bit_field #2): NULL result");
		return -2;
	}
	});

	/*******************************************************************/
	/* for big endian machines:                                        */
	/* the high-order bits appear in the lowest addresses,             */
	/* so shift left to discard <lb> bits, then shift right to discard */
	/* unneeded bits on the other side, leaving the significant bits   */
	/* in the low-order position.                                      */
	/*******************************************************************/
	word <<= (CHAR_BIT * sizeof(unsigned_int) - lb - size);		//modified by zhangbin, 2006-5-24
	word >>= (CHAR_BIT * sizeof (unsigned_int)) - size;

	switch (type->simple.type) {
	case nel_D_UNSIGNED:
	case nel_D_UNSIGNED_CHAR:
	case nel_D_UNSIGNED_INT:
	case nel_D_UNSIGNED_LONG:
	case nel_D_UNSIGNED_LONG_INT:
	case nel_D_UNSIGNED_SHORT:
	case nel_D_UNSIGNED_SHORT_INT:
		retval = nel_coerce (eng, type, result, nel_unsigned_int_type, (char *) (&word));
		break;


		/*********************************/
		/* architectures where int bit   */
		/* fields are treated as signed. */
		/*********************************/
	case nel_D_ENUM:
	case nel_D_INT:
	case nel_D_LONG:
	case nel_D_LONG_INT:
	case nel_D_SHORT:
	case nel_D_SHORT_INT:


	case nel_D_SIGNED:
	case nel_D_SIGNED_CHAR:
	case nel_D_SIGNED_INT:
	case nel_D_SIGNED_LONG:
	case nel_D_SIGNED_LONG_INT:
	case nel_D_SIGNED_SHORT:
	case nel_D_SIGNED_SHORT_INT:

		/**************************/
		/* sign extend the result */
		/**************************/
		{
			register unsigned_int mask = 1 << (size - 1);
			if (word & mask)
			{
				register unsigned_int i;
				mask <<= 1;
				for (i = size; (i < CHAR_BIT * sizeof (int)); i++, mask <<= 1) {
					word |= mask;
				}
			}
		}
		retval = nel_coerce (eng, type, result, nel_int_type, (char *) (&word));
		break;

	default:
		//intp_fatal_error (eng, "(intp_extract_bit_field #3): bad type for bit_field\n%1T", type);
		return -1;
	}

	nel_debug ({ nel_trace (eng, "] exiting intp_extract_bit_field\n*result = 0x%x\n\n", *result); });
	return retval;
}
/*****************************************************************************/
/* nel_set_struct_offsets () sets the offset field of each member of a struct */
/* and deletes any unnamed members whose sole purpose was to align the next  */
/* field.  The size and alignment of the struct are set it its type desc.    */
/* For bit field members, the bit_field flag and the appropriate bit_size    */
/* should already be set.                                                    */
/*****************************************************************************/
void nel_set_struct_offsets (struct nel_eng *eng, nel_type *type)
{
	register nel_member *member;
	register nel_member *prev_member;
	register unsigned_int offset = 0;	/* byte offset from struct base	*/
	register unsigned_int bit_offset = 0; /* bit offset within datum	*/
	register unsigned_int alignment = 0; /* alignment of struct	*/
	register unsigned_int size = 0;	/* size of struct		*/

	nel_debug ({ nel_trace (eng, "entering nel_set_struct_offsets () [\neng = 0x%x\ntype =\n%1T\n", eng, type); });

	nel_debug ({
	if (type->simple.type != nel_D_STRUCT) {
		parser_fatal_error (eng, "(nel_set_struct_offsets #1): bad type\n%T", type);
	}
	});

	for (prev_member=NULL, member = type->s_u.members; (member != NULL);) {
		register unsigned_int member_size;
		register unsigned_int member_alignment;

		nel_debug ({
		if ((member->symbol == NULL) || (member->symbol->type == NULL)){
			parser_fatal_error (eng, "(nel_set_struct_offsets #1): bad member\n%1M", member);
		}
		});

		member_size = member->symbol->type->simple.size;
		member_alignment = member->symbol->type->simple.alignment;
		if (! member->bit_field) {
			/******************************************/
			/* if there was a preceeding bit field,   */
			/* advance to the next free byte before   */
			/* doing calculations for a non-bit field */
			/******************************************/
			if (bit_offset != 0) {
				offset += ((bit_offset - 1) / CHAR_BIT + 1);
				bit_offset = 0;
			}

			/*******************************************/
			/* align the member correctly, and set its */
			/* offset, and advance to the next member  */
			/*******************************************/
			offset = nel_align_offset (offset, member_alignment);
			member->offset = offset;
			offset += member_size;
			member->bit_lb = member->bit_size = 0;
			prev_member = member;
			member = member->next;

			/****************************************/
			/* the struct must be large enough to   */
			/* include the entire size of the datum */
			/****************************************/
			if (offset > size) {
				size = offset;
			}

			/******************************************/
			/* on these machines, structs/unions/ are */
			/* aligned on a boundary that is the max  */
			/* of the alignments of its members.      */
			/******************************************/
			if (member_alignment > alignment) {
				alignment = member_alignment;
			}

		} else {
			register unsigned_int next_bdry;
			nel_debug ({
			if (member->bit_size > (CHAR_BIT * member_size)) {
				parser_fatal_error (eng, "(nel_set_struct_offsets #2): field too big\n%M", member);
			}
			});

			/*******************************************************/
			/* find the next alignment boundary for this data type */
			/*******************************************************/
			if (bit_offset > 0) {
				next_bdry = nel_align_offset ((offset + ((bit_offset - 1) / CHAR_BIT + 1)), member_alignment);
			} else {
				next_bdry = nel_align_offset (offset, member_alignment);
			}

			/***************************************************************/
			/* see if there is room for the field before the next boundary */
			/***************************************************************/
			if ((((next_bdry * CHAR_BIT) - (bit_offset + (CHAR_BIT * offset))) >= member->bit_size) && (member->bit_size > 0)) {
				/********************************************************/
				/* there is room.  subtract one datum from next_bdry to */
				/* get the btye offset (which could possibly put offset */
				/* before the previous member, if its base type had a   */
				/* smaller size), and recalculate bit_offset.           */
				/********************************************************/
				bit_offset += (offset - next_bdry + member_alignment) * CHAR_BIT;
				offset = next_bdry - member_alignment;
			} else {
				/***********************************************************/
				/* there wasn't room for the new bit field in the previous */
				/* datum, or else a field size of 0 was specified.         */
				/***********************************************************/
				offset = next_bdry;
				bit_offset = 0;
			}

			/**********************************************/
			/* delete a member from the list if its only  */
			/* purpose was align the next field correctly */
			/* advance the bit offset first.              */
			/**********************************************/
			if (member->symbol->name == NULL) {
				bit_offset += member->bit_size;
				if (prev_member == NULL) {
					type->s_u.members = member->next;
				} else {
					prev_member->next = member->next;
				}
				member = member->next;

				/*********************************************************/
				/* the struct must be large enough to include the entire */
				/* size of the current datum, even if a only the first   */
				/* portion of it is used in for bit field padding.       */
				/*********************************************************/
				if (offset + member_size > size) {
					size = offset + member_size;
				}

			}

			/***************************************************/
			/* otherwise, set the appropriate struct nel_member */
			/* fields and advance to the next member           */
			/***************************************************/
			else {
				member->bit_lb = bit_offset;
				member->offset = offset;
				bit_offset += member->bit_size;
				prev_member = member;
				member = member->next;

				/*********************************************************/
				/* the struct must be large enough to include the entire */
				/* size of the current datum, even if a only the first   */
				/* portion of it is used in a bit field.                 */
				/*********************************************************/
				if (offset + member_size > size) {
					size = offset + member_size;
				}

				/******************************************/
				/* on these machines, structs/unions/ are */
				/* aligned on a boundary that is the max  */
				/* of the alignments of its members.      */
				/******************************************/
				if (member_alignment > alignment) {
					alignment = member_alignment;
				}

			}
		}
	}

	/****************************************/
	/* set the struct's size and alignment. */
	/* the entire size of the struct must   */
	/* be a multiple of its alignment.      */
	/****************************************/
	type->s_u.size = nel_align_offset (size, alignment);
	type->s_u.alignment = alignment;

	nel_debug ({ nel_trace (eng, "] exiting nel_set_struct_offsets ()\ntype =\n%T\ntype->s_u.members =\n%1M\n", type, type->s_u.members); });
}


int s_u_has_member(struct nel_eng *eng, nel_type *type, nel_symbol *member)
{
	register nel_member *scan;
	if(type->s_u.members != NULL ) {
		for (scan=type->s_u.members; (scan != NULL); scan = scan->next)
		{
			register nel_symbol *symbol = scan->symbol;
			if(! strcmp(symbol->name, member->name)) {
				member->type = symbol->type; /* wyong, 2006.4.14 */
				return 1;
			}
		}
	}

	return 0;

}

/*****************************************************************************/
/* nel_s_u_size () scans the members of a struct/union and returns the total  */
/* size of the object, and its alignment in *alignment.  if the members are  */
/* elements of a struct, then their offsets should already be set, and any   */
/* members with a NULL symbol used to align the fields should be deleted.    */
/* if the members are elements of a union, then their offsets should be 0.   */
/*****************************************************************************/
unsigned_int nel_s_u_size (struct nel_eng *eng, unsigned_int *alignment, register nel_member *members)
{
	register unsigned_int size = 0;
	register unsigned_int max_alignment = 0;

	nel_debug ({ nel_trace (eng, "entering nel_s_u_size () [\neng = 0x%x\nalignment = 0x%x\nmembers =\n%1M\n", eng, alignment, members); });

	for (; (members != NULL); members = members->next) {
		nel_debug ({
			if ((members->symbol == NULL) || (members->symbol->type == NULL)) {
				//nel_fatal_error (eng, "(nel_s_u_size #1): illegal nel_member structure:\n%1M", members);
				return (-1);
			}
		});
		if (members->offset + members->symbol->type->simple.size > size) {
			size = members->offset + members->symbol->type->simple.size;
		}

		/*********************************************************/
		/* assume that the alignment of the entire struct/union  */
		/* is the max of the alignments of its members.          */
		/*********************************************************/
		if (members->symbol->type->simple.alignment > max_alignment) {
			max_alignment = members->symbol->type->simple.alignment;
		}

	}

	/***************************************************/
	/* assume that the entire size of the struct/union */
	/* must be a multiple of its aligment.             */
	/***************************************************/
	size = nel_align_offset (size, max_alignment);

	*alignment = max_alignment;
	nel_debug ({ nel_trace (eng, "] exiting nel_s_u_size ()\nsize = %d\n*alignment = %d\n\n", size, *alignment); });
	return (size);
}


/*****************************************************************************/
/* nel_bit_field_member () scans the member list and returns 1 if any member  */
/* is marked as a bit field.  *symbol is set to the symbol for the first     */
/* bit field member.                                                         */
/*****************************************************************************/
unsigned_int nel_bit_field_member (register nel_member *members, nel_symbol **symbol)
{
	register unsigned_int retval = 0;

	//nel_debug ({ nel_trace (eng, "entering nel_bit_field_member () [\nmembers =\n%1M\n", members); });

	*symbol = NULL;
	for (; (members != NULL); members = members->next) {
		if (members->bit_field) {
			retval = 1;
			*symbol = members->symbol;
			goto end;
		}
	}

end:
	//nel_debug ({ nel_trace (eng, "] exiting nel_bit_field_member ()\nretval = %d\n\n", retval); });
	return (retval);
}



/*****************************************************************************/
/* nel_D_token_equiv () returns the nel_D_token that is equivalent to D_token, */
/* but has a consistent type names specified.  i.e., the signed or unsigned  */
/* qualifier is always specified, and long or short is only specified if the */
/* type differs with it deleted.  an implicit int is included where          */
/* appropriate.  e.g, for long, we return signed int if longs have the same  */
/* size as ints.                                                             */
/*****************************************************************************/
nel_D_token nel_D_token_equiv (register nel_D_token D_token)
{
	switch (D_token) {

	case  nel_D_CHAR: {
			register char ch = -1;
			if (ch < 0) {
				return (nel_D_SIGNED_CHAR);
			} else {
				return (nel_D_UNSIGNED_CHAR);
			}
		}

	case  nel_D_SIGNED_CHAR:
		return (nel_D_SIGNED_CHAR);

	case  nel_D_UNSIGNED_CHAR:
		return (nel_D_UNSIGNED_CHAR);

	case  nel_D_SHORT:
	case  nel_D_SHORT_INT:
	case  nel_D_SIGNED_SHORT:
	case  nel_D_SIGNED_SHORT_INT:
		if (sizeof (short) == sizeof (int)) {
			return (nel_D_SIGNED_INT);
		} else {
			return (nel_D_SIGNED_SHORT_INT);
		}

	case  nel_D_UNSIGNED_SHORT:
	case  nel_D_UNSIGNED_SHORT_INT:
		if (sizeof (short) == sizeof (int)) {
			return (nel_D_UNSIGNED_INT);
		} else {
			return (nel_D_UNSIGNED_SHORT_INT);
		}

	case  nel_D_ENUM:
	case  nel_D_INT:
	case  nel_D_SIGNED:
	case  nel_D_SIGNED_INT:
		return (nel_D_SIGNED_INT);

	case  nel_D_UNSIGNED:
	case  nel_D_UNSIGNED_INT:
		return (nel_D_UNSIGNED_INT);

	case  nel_D_LONG:
	case  nel_D_LONG_INT:
	case  nel_D_SIGNED_LONG:
	case  nel_D_SIGNED_LONG_INT:
		if (sizeof (long) == sizeof (int)) {
			return (nel_D_SIGNED_INT);
		} else {
			return (nel_D_SIGNED_LONG_INT);
		}

	case  nel_D_UNSIGNED_LONG:
	case  nel_D_UNSIGNED_LONG_INT:
		if (sizeof (long) == sizeof (int)) {
			return (nel_D_UNSIGNED_INT);
		} else {
			return (nel_D_UNSIGNED_LONG_INT);
		}

	case  nel_D_FLOAT:
		if (sizeof (float) == sizeof (double)) {
			return (nel_D_DOUBLE);
		} else {
			return (nel_D_FLOAT);
		}

	case  nel_D_DOUBLE:
	case  nel_D_LONG_FLOAT:
		return (nel_D_DOUBLE);

	case  nel_D_LONG_DOUBLE:
		if (sizeof (long_double) == sizeof (double)) {
			return (nel_D_DOUBLE);
		} else {
			return (nel_D_LONG_DOUBLE);
		}

	case  nel_D_COMPLEX:
	case  nel_D_COMPLEX_FLOAT:
		if (sizeof (float) == sizeof (double)) {
			return (nel_D_COMPLEX_DOUBLE);
		} else {
			return (nel_D_COMPLEX_FLOAT);
		}

	case  nel_D_COMPLEX_DOUBLE:
	case  nel_D_LONG_COMPLEX:
	case  nel_D_LONG_COMPLEX_FLOAT:
		return (nel_D_COMPLEX_DOUBLE);

	case  nel_D_LONG_COMPLEX_DOUBLE:
		if (sizeof (long_double) == sizeof (double)) {
			return (nel_D_COMPLEX_DOUBLE);
		} else {
			return (nel_D_LONG_COMPLEX_DOUBLE);
		}

	default:
		return (D_token);
	}
}


/**************************************************/
/* declarations for the type descriptor allocator */
/**************************************************/
unsigned_int nel_types_max = 0x1000;
nel_lock_type nel_types_lock = nel_unlocked_state;
static nel_type *nel_types_next = NULL;
static nel_type *nel_types_end = NULL;
static nel_type *nel_free_types = NULL;

/***********************************************************/
/* make a linked list of nel_type_chunk structures so that  */
/* we may find the start of all chunks of memory allocated */
/* to hold type descriptors at garbage collection time.    */
/***********************************************************/
static nel_type_chunk *nel_type_chunks = NULL;


/* log max rhs number , wyong, 2006.4.13 */
extern int rhs_num_max;

/*****************************************************************************/
/* nel_type_alloc (eng, ) allocates space for an nel_type structure, 	     */
/* initializes  its fields, and returns a pointer to the structure.          */
/*****************************************************************************/
nel_type *nel_type_alloc (struct nel_eng *eng, register nel_D_token type, register unsigned_int size, register unsigned_int alignment, register unsigned_int _const, unsigned_int _volatile, ...)
{
	va_list args;		/* scans arguments after size		*/
	register nel_type *retval;	/* return value			*/

	nel_debug ({ nel_trace (eng, "entering nel_type_alloc [\ntype = %D\nsize = %d\nalignment = %d\n_const = %d\n_volatile = %d\n\n", type, size, alignment, _const, _volatile); });

	va_start (args, _volatile);

	/*******************************************/
	/* static allocators are critical sections */
	/*******************************************/
	nel_lock (&nel_types_lock);

	if (nel_free_types != NULL) {
		/*****************************************************/
		/* first, try to re-use deallocated type descriptors */
		/*****************************************************/
		retval = nel_free_types;
		nel_free_types = (nel_type *) nel_free_types->simple.size;
	} else {
		/**************************************************************/
		/* check for overflow of space allocated for type descriptors.*/
		/* on overflow, allocate another chunk of memory.             */
		/**************************************************************/
		if (nel_types_next >= nel_types_end) {
			register nel_type_chunk *chunk;

			nel_debug ({ nel_trace (eng, "nel_type structure overflow - allocating more space\n\n"); });
			nel_malloc (nel_types_next, nel_types_max, nel_type);
			nel_types_end = nel_types_next + nel_types_max;

			/*************************************************/
			/* make an entry for this chunk of memory in the */
			/* list of nel_type_chunks.                       */
			/*************************************************/
			nel_malloc (chunk, 1, nel_type_chunk);
			chunk->start = nel_types_next;
			chunk->size = nel_types_max;
			chunk->next = nel_type_chunks;
			nel_type_chunks = chunk;
		}
		retval = nel_types_next++;
	}
	
	//added by zhangbin, 2006-7-26
	nel_zero(sizeof(nel_type), retval);
	//end
	
	/*************************/
	/* exit critical section */
	/*************************/
	nel_unlock (&nel_types_lock);

	/****************************************/
	/* initialize the appropriate members.  */
	/****************************************/
	retval->simple.type = type;
	retval->simple.alignment = alignment;
	retval->simple._const = _const;
	retval->simple._volatile = _volatile;
	retval->simple.garbage = 0;
	retval->simple.traversed = 0;
	retval->simple.size = size;

	switch (type) {
	case nel_D_ARRAY:
		retval->array.base_type = va_arg (args, nel_type *);
		retval->array.index_type = va_arg (args, nel_type *);
		if ((retval->array.known_lb = va_arg (args, unsigned_int))){
			retval->array.lb.value = va_arg (args, unsigned_int);
		} else {
			retval->array.lb.expr= va_arg (args, nel_expr *);
		}
		if ((retval->array.known_ub = va_arg (args, unsigned_int))){
			retval->array.ub.value = va_arg (args, unsigned_int);
		} else {
			retval->array.ub.expr= va_arg (args, nel_expr *);
		}
		retval->array.val_len = 0;	//added by zhangbin, 2006-6-23
		break;

	case nel_D_ENUM:
		retval->enumed.tag = va_arg (args, nel_symbol *);
		retval->enumed.nconsts = va_arg (args, unsigned_int);
		retval->enumed.consts = va_arg (args, nel_list *);
		break;
	case nel_D_FILE:
		retval->file.stab_types = va_arg (args, nel_stab_type *);
		retval->file.routines = va_arg (args, nel_list *);
		retval->file.static_globals = va_arg (args, nel_list *);
		retval->file.static_locations = va_arg (args, nel_list *);
		retval->file.includes = va_arg (args, nel_list *);
		retval->file.lines = va_arg (args, nel_line *);
		break;
	case nel_D_FUNCTION:
		retval->function.new_style = va_arg (args, unsigned_int);
		retval->function.var_args = va_arg (args, unsigned_int);
		retval->function.return_type = va_arg (args, nel_type *);
		retval->function.args = va_arg (args, nel_list *);
		retval->function.blocks = va_arg (args, nel_block *);
		retval->function.file = va_arg (args, nel_symbol *);

		/* wyong, 2004.6.21 */
		retval->function.key_nums = 0;
		break;
	case nel_D_POINTER:
		retval->pointer.deref_type = va_arg (args, nel_type *);
		break;
	case nel_D_STAB_UNDEF:
		retval->stab_undef.stab_type = va_arg (args, nel_stab_type *);
		break;
	case nel_D_STRUCT:
	case nel_D_UNION:
		retval->s_u.tag = va_arg (args, nel_symbol *);
		retval->s_u.members = va_arg (args, nel_member *);
		break;
	case nel_D_ENUM_TAG:
	case nel_D_STRUCT_TAG:
	case nel_D_UNION_TAG:
		retval->tag_name.descriptor = va_arg (args, nel_type *);
		break;
	case nel_D_TYPE_NAME:
	case nel_D_TYPEDEF_NAME:
		retval->typedef_name.descriptor = va_arg (args, nel_type *);
		break;

	case nel_D_EVENT:
		retval->event.descriptor = va_arg(args, nel_type *);
		retval->event.free_hander = va_arg(args, nel_symbol *);
		retval->event.prec = 0;
		retval->event.assoc = NOASSOC;
		break;

	case nel_D_PRODUCTION: 
		{
			struct nel_RHS *scan;

			retval->prod.lhs = va_arg(args, struct nel_SYMBOL *);
			retval->prod.rel = va_arg(args, int);
			retval->prod.rhs = va_arg(args, struct nel_RHS *);
			retval->prod.stmts = va_arg(args, union nel_STMT *);
			
			for(scan = retval->prod.rhs ; scan; scan = scan->next) {
				retval->prod.rhs_num ++;
			}

			if(retval->prod.rhs_num > rhs_num_max )	/* wyong, 2006.4.13 */
				rhs_num_max = retval->prod.rhs_num;

			if(strcmp(retval->prod.lhs->name, "others")
					&& !strcmp(retval->prod.rhs->symbol->name, "others")) {
				retval->prod.rel = REL_OT;
			}

			switch(retval->prod.rel) {
			case REL_OT:
			case REL_AT:
				if(retval->prod.rhs_num != 1)
					break;
			case REL_EX:
			case REL_ON:
			case REL_OR:
				if( retval->prod.rhs_num <= 0
						|| retval->prod.rhs_num > MAX_RHS)
					break;
				retval->prod.stop = retval->prod.rhs ;
				break;

			default:
				break;

			}
		}
		break;

	default:
		/************************************************/
		/* simple types, nel_D_COMMON, and nel_D_LOCATION */
		/************************************************/
		break;
	}

	va_end (args);
	nel_debug ({ nel_trace (eng, "] exiting nel_type_alloc\nretval =\n%1T\n", retval); });
	return (retval);
}



/*****************************************************************************/
/* nel_type_dealloc () returns the nel_type structure pointed to by <type>     */
/* back to the free list (nel_free_types), so that the space may be re-used.  */
/*****************************************************************************/
void nel_type_dealloc (register nel_type *type)
{
	nel_debug ({
	if (type == NULL) {
		//nel_fatal_error (NULL, "(nel_type_dealloc #1): type == NULL");
	}
	});

	/*******************************************/
	/* static allocators are critical sections */
	/*******************************************/
	nel_lock (&nel_types_lock);

	type->simple.size = (unsigned_int) nel_free_types;
	nel_free_types = type;

	/*************************/
	/* exit critical section */
	/*************************/
	nel_unlock (&nel_types_lock);
}



#define tab(_indent); \
	{								\
	   register unsigned_int __indent = (_indent);			\
	   int __i;							\
	   for (__i = 0; (__i < __indent); __i++) {			\
	      fprintf (file, "   ");					\
	   }								\
	}


/*****************************************************************************/
/* nel_print_type () prints out a type descriptor in a pretty format.         */
/*****************************************************************************/
void nel_print_type (FILE *file, register nel_type *desc, int indent)
{
	if (desc == NULL) {
		tab (indent);
		fprintf (file, "NULL_type\n");
	} else if (desc->simple.traversed) {
		/****************************************************/
		/* check the traversed bit to keep out of an        */
		/* infinite loop when printing circular structures. */
		/****************************************************/
		tab (indent);
		fprintf (file, "0x%p\n", desc);
		tab (indent);
		fprintf (file, "circular_type\n");
	} else {
		desc->simple.traversed = 1;
		tab (indent);
		fprintf (file, "0x%p\n", desc);
		tab (indent);
		fprintf (file, "type: %s\n", nel_D_name (desc->simple.type));
		tab (indent);
		fprintf (file, "size: %d\n", desc->simple.size);
		tab (indent);
		fprintf (file, "alignment: %d\n", desc->simple.alignment);
		tab (indent);
		fprintf (file, "_const: %d\n", desc->simple._const);
		tab (indent);
		fprintf (file, "_volatile: %d\n", desc->simple._volatile);
		switch (desc->simple.type) {

		case nel_D_ARRAY:
			tab (indent);
			fprintf (file, "base_type:\n");
			nel_print_type (file, desc->array.base_type, indent + 1);
			tab (indent);
			fprintf (file, "index_type:\n");
			nel_print_type (file, desc->array.index_type, indent + 1);
			tab (indent);
			fprintf (file, "known_lb: %d\n", desc->array.known_lb);
			if (desc->array.known_lb) {
				tab (indent);
				fprintf (file, "lb.value: %d\n", desc->array.lb.value);
			} else {
				tab (indent);

//#ifdef NEL_NTRP

				fprintf (file, "lb.expr:\n");
				nel_print_expr (file, desc->array.lb.expr, indent + 1);
//#else
//
//				fprintf (file, "lb.symbol:\n");
//				nel_print_symbol (file, desc->array.lb.symbol, indent + 1);
//#endif /* NEL_NTRP */

			}
			tab (indent);
			fprintf (file, "known_ub: %d\n", desc->array.known_ub);
			if (desc->array.known_ub) {
				tab (indent);
				fprintf (file, "ub.value: %d\n", desc->array.ub.value);
			} else {
				tab (indent);

//#ifdef NEL_NTRP

				fprintf (file, "ub.expr:\n");
				nel_print_expr (file, desc->array.ub.expr, indent + 1);
//#else
//
//				fprintf (file, "ub.symbol:\n");
//				nel_print_symbol (file, desc->array.ub.symbol, indent + 1);
//#endif /* NEL_NTRP */

			}
			break;

		case nel_D_ENUM:
			tab (indent);
			fprintf (file, "tag:\n");
			nel_print_symbol (file, desc->enumed.tag, indent + 1);
			tab (indent);
			fprintf (file, "nconsts: %d\n", desc->enumed.nconsts);
			if (! nel_print_brief_types) {
				tab (indent);
				fprintf (file, "consts:\n");
				nel_print_list (file, desc->enumed.consts, indent + 1);
			} else {
				tab (indent);
				fprintf (file, "consts: 0x%p\n", desc->enumed.consts);
			}
			break;

		case nel_D_FILE:
			if (! nel_print_brief_types) {
				tab (indent);
				fprintf (file, "stab_types:\n");
				stab_print_types (file, desc->file.stab_types, indent + 1);
				tab (indent);
				fprintf (file, "routines:\n");
				nel_print_list (file, desc->file.routines, indent + 1);
				tab (indent);
				fprintf (file, "static_globals:\n");
				nel_print_list (file, desc->file.static_globals, indent + 1);
				tab (indent);
				fprintf (file, "includes:\n");
				nel_print_list (file, desc->file.includes, indent + 1);
				tab (indent);
				fprintf (file, "lines:\n");
				nel_print_lines (file, desc->file.lines, indent + 1);
			} else {
				tab (indent);
				fprintf (file, "stab_types: 0x%p\n", desc->file.stab_types);
				tab (indent);
				fprintf (file, "routines: 0x%p\n", desc->file.routines);
				tab (indent);
				fprintf (file, "static_globals: 0x%p\n", desc->file.static_globals);
				tab (indent);
				fprintf (file, "includes: 0x%p\n", desc->file.includes);
				tab (indent);
				fprintf (file, "lines: 0x%p\n", desc->file.lines);
			}
			break;

		case nel_D_FUNCTION:
			tab (indent);
			fprintf (file, "new_style: %d\n", desc->function.new_style);
			tab (indent);
			fprintf (file, "var_args: %d\n", desc->function.var_args);
			tab (indent);
			fprintf (file, "return_type:\n");
			nel_print_type (file, desc->function.return_type, indent + 1);
			if (! nel_print_brief_types) {
				tab (indent);
				fprintf (file, "args:\n");
				nel_print_list (file, desc->function.args, indent + 1);
				tab (indent);
				fprintf (file, "blocks:\n");
				nel_print_blocks (file, desc->function.blocks, indent + 1);
				tab (indent);
				fprintf (file, "file:\n");
				nel_print_symbol (file, desc->function.file, indent + 1);
			} else {
				tab (indent);
				fprintf (file, "args: 0x%p\n", desc->function.args);
				tab (indent);
				fprintf (file, "blocks: 0x%p\n", desc->function.blocks);
				tab (indent);
				fprintf (file, "file: 0x%p\n", desc->function.file);
			}
			break;

		case nel_D_POINTER:
			tab (indent);
			fprintf (file, "deref_type:\n");
			nel_print_type (file, desc->pointer.deref_type, indent + 1);
			break;

		case nel_D_STAB_UNDEF:
			tab (indent);
			fprintf (file, "stab_type:\n");
			stab_print_type (file, desc->stab_undef.stab_type, indent + 1);
			break;

		case nel_D_STRUCT:
		case nel_D_UNION:
			tab (indent);
			fprintf (file, "tag:\n");
			nel_print_symbol (file, desc->s_u.tag, indent + 1);
			if (! nel_print_brief_types) {
				tab (indent);
				fprintf (file, "members:\n");
				nel_print_members (file, desc->s_u.members, indent + 1);
			} else {
				tab (indent);
				fprintf (file, "members: 0x%p\n", desc->s_u.members);
			}
			break;

		case nel_D_ENUM_TAG:
		case nel_D_STRUCT_TAG:
		case nel_D_UNION_TAG:
			tab (indent);
			fprintf (file, "descriptor:\n");
			nel_print_type (file, desc->tag_name.descriptor, indent + 1);
			break;

		case nel_D_TYPE_NAME:
		case nel_D_TYPEDEF_NAME:
			tab (indent);
			fprintf (file, "descriptor:\n");
			nel_print_type (file, desc->typedef_name.descriptor, indent + 1);
			break;

		default:
			break;

		}
		desc->simple.traversed = 0;
	}
}



/*****************************************************/
/* declarations for the nel_block structure allocator */
/*****************************************************/
unsigned_int nel_blocks_max = 0x200;
nel_lock_type nel_blocks_lock = nel_unlocked_state;
static nel_block *nel_blocks_next = NULL;
static nel_block *nel_blocks_end = NULL;
static nel_block *nel_free_blocks = NULL;

/***********************************************************/
/* make a linked list of nel_block_chunk structures so that */
/* we may find the start of all chunks of memory allocated */
/* to hold nel_block structures at garbage collection time. */
/***********************************************************/
static nel_block_chunk *nel_block_chunks = NULL;



/*****************************************************************************/
/* nel_block_alloc () allocates space for an nel_block structure, initializes  */
/* its fields, and returns a pointer to the structure.                       */
/*****************************************************************************/
nel_block *nel_block_alloc (struct nel_eng *eng, int block_no, char *start_address, char *end_address, nel_list *locals, nel_block *blocks, nel_block *next)
{
	register nel_block *retval;

	nel_debug ({ nel_trace (eng, "entering nel_block_alloc [\nblock_no = %d\nstart_address = 0x%x\nend_address = 0x%x\nlocals =\n%1Lblocks =\n%1Bnext =\n%1B\n", block_no, start_address, end_address, locals, blocks, next); });

	/*******************************************/
	/* static allocators are critical sections */
	/*******************************************/
	nel_lock (&nel_blocks_lock);

	if (nel_free_blocks != NULL) {
		/*****************************************************/
		/* first, try to re-use deallocated block structures */
		/*****************************************************/
		retval = nel_free_blocks;
		nel_free_blocks = nel_free_blocks->next;
	} else {
		/******************************************************************/
		/* check for overflow of space allocated for nel_block structures. */
		/* on overflow, allocate another chunk of memory.                 */
		/******************************************************************/
		if (nel_blocks_next >= nel_blocks_end) {
			register nel_block_chunk *chunk;

			nel_debug ({ nel_trace (eng, "nel_block structure overflow - allocating more space\n\n"); });
			nel_malloc (nel_blocks_next, nel_blocks_max, nel_block);
			nel_blocks_end = nel_blocks_next + nel_blocks_max;

			/*************************************************/
			/* make an entry for this chunk of memory in the */
			/* list of nel_block_chunks.                       */
			/*************************************************/
			nel_malloc (chunk, 1, nel_block_chunk);
			chunk->start = nel_blocks_next;
			chunk->next = nel_block_chunks;
			nel_block_chunks = chunk;
		}
		retval = nel_blocks_next++;
	}

	/*************************/
	/* exit critical section */
	/*************************/
	nel_unlock (&nel_blocks_lock);

	/**************************************/
	/* initialize the appropriate members */
	/**************************************/
	retval->garbage = 0;
	retval->block_no = block_no;
	retval->start_address = start_address;
	retval->end_address = end_address;
	retval->locals = locals;
	retval->blocks = blocks;
	retval->next = next;

	nel_debug ({ nel_trace (eng, "] exiting nel_block_alloc\nretval =\n%1B\n", retval); });
	return (retval);
}



/*****************************************************************************/
/* nel_block_dealloc () returns the nel_block structure pointed to by <block>  */
/* back to the free list (nel_free_blocks), so that the space may be re-used. */
/*****************************************************************************/
void nel_block_dealloc (register nel_block *block)
{
	nel_debug ({
	if (block == NULL) {
		//nel_fatal_error (NULL, "(nel_block_dealloc #1): block == NULL");
	}
	});

	/*******************************************/
	/* static allocators are critical sections */
	/*******************************************/
	nel_lock (&nel_blocks_lock);

	block->next = nel_free_blocks;
	nel_free_blocks = block;

	/*************************/
	/* exit critical section */
	/*************************/
	nel_unlock (&nel_blocks_lock);
}


/*****************************************************************************/
/* nel_print_blocks () prints out the blocks list of a function type          */
/* descriptor.                                                               */
/*****************************************************************************/
void nel_print_blocks (FILE *file, register nel_block *blocks, int indent)
{
	if (blocks == NULL) {
		tab (indent);
		fprintf (file, "NULL\n");
	} else {
		for (; (blocks != NULL); blocks = blocks->next) {
			tab (indent);
			fprintf (file, "block:\n");
			tab (indent + 1);
			fprintf (file, "block_no: %d\n", blocks->block_no);
			tab (indent + 1);
			fprintf (file, "start_address: 0x%p\n", blocks->start_address);
			tab (indent + 1);
			fprintf (file, "end_address: 0x%p\n", blocks->end_address);
			tab (indent + 1);
			fprintf (file, "locals:\n");
			nel_print_list (file, blocks->locals, indent + 2);
			tab (indent + 1);
			fprintf (file, "blocks:\n");
			nel_print_blocks (file, blocks->blocks, indent + 2);
		}
	}
}



/****************************************************/
/* declarations for the nel_line structure allocator */
/****************************************************/
unsigned_int nel_lines_max = 0x800;
nel_lock_type nel_lines_lock = nel_unlocked_state;
static nel_line *nel_lines_next = NULL;
static nel_line *nel_lines_end = NULL;
static nel_line *nel_free_lines = NULL;

/***********************************************************/
/* make a linked list of nel_line_chunk structures so that  */
/* we may find the start of all chunks of memory allocated */
/* to hold line structures at garbage collection time.     */
/***********************************************************/
static nel_line_chunk *nel_line_chunks = NULL;



/*****************************************************************************/
/* nel_line_alloc () allocates space for an nel_line structure, initializes    */
/* its fields, and returns a pointer to the structure.                       */
/*****************************************************************************/
nel_line *nel_line_alloc (struct nel_eng *eng, int line_no, char *address, nel_line *next)
{
	register nel_line *retval;

	nel_debug ({ nel_trace (eng, "entering nel_line_alloc [\nline_no = %d\naddress = 0x%x\nnext =\n%1A\n", line_no, address, next); });

	/*******************************************/
	/* static allocators are critical sections */
	/*******************************************/
	nel_lock (&nel_lines_lock);

	if (nel_free_lines != NULL) {
		/****************************************************/
		/* first, try to re-use deallocated line structures */
		/****************************************************/
		retval = nel_free_lines;
		nel_free_lines = nel_free_lines->next;
	} else {
		/*****************************************************************/
		/* check for overflow of space allocated for nel_line structures. */
		/* on overflow, allocate another chunk of memory.                */
		/*****************************************************************/
		if (nel_lines_next >= nel_lines_end) {
			register nel_line_chunk *chunk;

			nel_debug ({ nel_trace (eng, "nel_line structure overflow - allocating more space\n\n"); });
			nel_malloc (nel_lines_next, nel_lines_max, nel_line);
			nel_lines_end = nel_lines_next + nel_lines_max;

			/*************************************************/
			/* make an entry for this chunk of memory in the */
			/* list of nel_line_chunks.                       */
			/*************************************************/
			nel_malloc (chunk, 1, nel_line_chunk);
			chunk->start = nel_lines_next;
			chunk->next = nel_line_chunks;
			nel_line_chunks = chunk;
		}
		retval = nel_lines_next++;
	}

	/*************************/
	/* exit critical section */
	/*************************/
	nel_unlock (&nel_lines_lock);

	/**************************************/
	/* initialize the appropriate members */
	/**************************************/
	retval->garbage = 0;
	retval->line_no = line_no;
	retval->address = address;
	retval->next = next;

	nel_debug ({ nel_trace (eng, "] exiting nel_line_alloc\nretval =\n%1A\n", retval); });
	return (retval);
}



/*****************************************************************************/
/* nel_line_dealloc () returns the nel_line structure pointed to by <line>     */
/* back to the free list (nel_free_lines), so that the space may be re-used.  */
/*****************************************************************************/
void nel_line_dealloc (register nel_line *line)
{
	nel_debug ({
	if (line == NULL) {
		//nel_fatal_error (NULL, "(nel_line_dealloc #1): line == NULL");
	}
	});

	/*******************************************/
	/* static allocators are critical sections */
	/*******************************************/
	nel_lock (&nel_lines_lock);

	line->next = nel_free_lines;
	nel_free_lines = line;

	/*************************/
	/* exit critical section */
	/*************************/
	nel_unlock (&nel_lines_lock);
}


/*****************************************************************************/
/* nel_print_line () prints out the list of line # / address pair structures  */
/* pointer to by <line>.                                                     */
/*****************************************************************************/
void nel_print_lines (FILE *file, register nel_line *line, int indent)
{
	if (line == NULL) {
		tab (indent);
		fprintf (file, "NULL_line\n");
	} else {
		for (; (line != NULL); line = line->next) {
			tab (indent);
			fprintf (file, "line_no: %-8d  address: 0x%p\n", line->line_no, line->address);
		}
	}
}




/****************************************************/
/* declarations for the nel_list structure allocator */
/****************************************************/
unsigned_int nel_lists_max = 0x800;
nel_lock_type nel_lists_lock = nel_unlocked_state;
static nel_list *nel_lists_next = NULL;
static nel_list *nel_lists_end = NULL;
static nel_list *nel_free_lists = NULL;

/***********************************************************/
/* make a linked list of nel_list_chunk structures so that  */
/* we may find the start of all chunks of memory allocated */
/* to hold list structures at garbage collection time.     */
/***********************************************************/
static nel_list_chunk *nel_list_chunks = NULL;


struct nel_LIST *__make_entry(struct nel_SYMBOL *symbol)
{
	//modified by zhangbin, 2006-7-17, malloc=>nel_malloc
#if 1
	struct nel_LIST *list;
	list = nel_list_alloc(NULL, 0, symbol, NULL);
	//nel_malloc(list, 1, struct nel_LIST); //modified by zhangbin, 2006-7-20
#else
	struct nel_LIST *list =(struct nel_LIST *)malloc(sizeof(struct nel_LIST ));
#endif
	//end, 2006-7-17
	if(list)
	{
		list->next = (struct nel_LIST *)0;
		list->symbol= symbol;
	}

	return list;
}

void __append_entry_to(struct nel_LIST **plist, struct nel_LIST *list )
{
	struct nel_LIST *this, *prev;

	if (*plist != (struct nel_LIST *)0)
	{
		prev = (struct nel_LIST *)0;
		this= *plist;
		while(this)
		{
			prev=this;
			this=this->next;
		}

		if(prev)
		{
			prev->next = list;
		}
	} else
	{
		*plist = list;
	}

}

int __lookup_item_from(struct nel_LIST **plist, struct nel_SYMBOL *symbol)
{
	struct nel_LIST *scan;
	for( scan=*plist; scan; scan=scan->next ) {
		if (scan->symbol == symbol) {
			return 1;
		}
	}
	return 0;
}


void __get_item_out_from(struct nel_LIST **plist, struct nel_SYMBOL *symbol)
{
	struct nel_LIST *scan, *prev;
	for( prev=(struct nel_LIST *)0, scan=*plist; scan; prev=scan, scan=scan->next )
	{
		if (scan->symbol == symbol) {
			if(prev) {
				prev->next = scan->next;
			} else {
				*plist = scan->next;
			}
			nel_list_dealloc(scan);	//modified by zhangbin, 2006-7-20
			//nel_dealloca(scan); //free(scan); zhangbin, 2006-7-17
			break;
		}
	}
}

struct nel_SYMBOL * __get_first_from(struct nel_LIST **plist)
{
	struct nel_SYMBOL *symbol; struct nel_LIST *fst;
	if( (fst = (*plist)) ) {
		symbol = fst->symbol;
	}

	return symbol;
}

struct nel_SYMBOL * __get_first_out_from(struct nel_LIST **plist)
{
	struct nel_SYMBOL *symbol;
	struct nel_LIST *fst;
	if( (fst = (*plist)) )
	{
		symbol = fst->symbol;
		*plist = fst->next;
		nel_list_dealloc(fst);	//modified by zhangbin, 2006-7-20
		//nel_dealloca(fst);	//free(fst); zhangbin, 2006-7-17
	}

	return symbol;
}

int __get_count_of(struct nel_LIST **plist)
{
	struct nel_LIST *scan ;
	int count =0;

	for(scan = *plist; scan; scan =scan->next)
	{
		count++;
	}

	return count;
}


/*****************************************************************************/
/* nel_list_alloc () allocates space for an nel_list structure, initializes    */
/* its fields, and returns a pointer to the structure.                       */
/*****************************************************************************/
nel_list *nel_list_alloc (struct nel_eng *eng, int value, nel_symbol *symbol, nel_list *next)
{
	register nel_list *retval;

	//nel_debug ({ nel_trace (eng, "entering nel_list_alloc [\nvalue = %d\nsymbol =\n%1Snext =\n%1L\n", value, symbol, next); });

	/*******************************************/
	/* static allocators are critical sections */
	/*******************************************/
	nel_lock (&nel_lists_lock);

	if (nel_free_lists != NULL) {
		/****************************************************/
		/* first, try to re-use deallocated list structures */
		/****************************************************/
		retval = nel_free_lists;
		nel_free_lists = nel_free_lists->next;
	} else {
		/*****************************************************************/
		/* check for overflow of space allocated for nel_list structures. */
		/* on overflow, allocate another chunk of memory.                */
		/*****************************************************************/
		if (nel_lists_next >= nel_lists_end) {
			register nel_list_chunk *chunk;

			//nel_debug ({ nel_trace (eng, "nel_list structure overflow - allocating more space\n\n"); });
			nel_malloc (nel_lists_next, nel_lists_max, nel_list);
			nel_lists_end = nel_lists_next + nel_lists_max;

			/*************************************************/
			/* make an entry for this chunk of memory in the */
			/* list of nel_list_chunks.                       */
			/*************************************************/
			nel_malloc (chunk, 1, nel_list_chunk);
			chunk->start = nel_lists_next;
			chunk->next = nel_list_chunks;
			nel_list_chunks = chunk;
		}
		retval = nel_lists_next++;
	}

	/*************************/
	/* exit critical section */
	/*************************/
	nel_unlock (&nel_lists_lock);

	/**************************************/
	/* initialize the appropriate members */
	/**************************************/
	retval->garbage = 0;
	retval->value = value;
	retval->symbol = symbol;
	retval->next = next;

	//nel_debug ({ nel_trace (eng, "] exiting nel_list_alloc\nretval =\n%1L\n", retval); });
	return (retval);
}



/*****************************************************************************/
/* nel_list_dealloc () returns the nel_list structure pointed to by <list>     */
/* back to the free list (nel_free_lists), so that the space may be re-used.  */
/*****************************************************************************/
void nel_list_dealloc (register nel_list *list)
{
	nel_debug ({
	if (list == NULL) {
		//nel_fatal_error (NULL, "(nel_list_dealloc #1): list == NULL");
	}
	});

	/*******************************************/
	/* static allocators are critical sections */
	/*******************************************/
	nel_lock (&nel_lists_lock);

	list->next = nel_free_lists;
	nel_free_lists = list;

	/*************************/
	/* exit critical section */
	/*************************/
	nel_unlock (&nel_lists_lock);
}


/*****************************************************************************/
/* nel_print_list () prints out the list of symbols pointed to by <list>.     */
/*****************************************************************************/
void nel_print_list (FILE *file, register nel_list *list, int indent)
{
	if (list == NULL) {
		tab (indent);
		fprintf (file, "NULL_list\n");
	} else {
		for (; (list != NULL); list = list->next) {
			tab (indent);
			fprintf (file, "list:\n");
			tab (indent + 1);
			fprintf (file, "value: %d\n", list->value);
			tab (indent + 1);
			fprintf (file, "symbol:\n");
			nel_print_symbol (file, list->symbol, indent + 2);
		}
	}
}




/******************************************************/
/* declarations for the nel_member structure allocator */
/******************************************************/
unsigned_int nel_members_max = 0x200;
nel_lock_type nel_members_lock = nel_unlocked_state;
static nel_member *nel_members_next = NULL;
static nel_member *nel_members_end = NULL;
static nel_member *nel_free_members = NULL;

/************************************************************/
/* make a linked list of nel_member_chunk structures so that */
/* we may find the start of all chunks of memory allocated  */
/* to hold nel_member structures at garbage collection time. */
/************************************************************/
static nel_member_chunk *nel_member_chunks = NULL;



/*****************************************************************************/
/* nel_member_alloc () allocates space for an nel_member structure,            */
/* initializes its fields, and returns a pointer to the structure.           */
/*****************************************************************************/
nel_member *nel_member_alloc (struct nel_eng *eng, nel_symbol *symbol, unsigned_int bit_field, unsigned_int bit_size, unsigned_int bit_lb, unsigned_int offset, nel_member *next)
{
	register nel_member *retval;

	nel_debug ({ nel_trace (eng, "entering nel_member_alloc [\nsymbol =\n%1Sbit_field = %d\nbit_size = %d\nbit_lb = %d\noffset = %d\nnext =\n%1M\n", symbol, bit_field, bit_size, bit_lb, offset, next); });

	/*******************************************/
	/* static allocators are critical sections */
	/*******************************************/
	nel_lock (&nel_members_lock);

	if (nel_free_members != NULL) {
		/******************************************************/
		/* first, try to re-use deallocated member structures */
		/******************************************************/
		retval = nel_free_members;
		nel_free_members = nel_free_members->next;
	} else {
		/****************************************************************/
		/* check for overflow of space allocated for member structures. */
		/* on overflow, allocate another chunk of memory.               */
		/****************************************************************/
		if (nel_members_next >= nel_members_end) {
			register nel_member_chunk *chunk;

			nel_debug ({ nel_trace (eng, "nel_member structure overflow - allocating more space\n\n"); });
			nel_malloc (nel_members_next, nel_members_max, nel_member);
			nel_members_end = nel_members_next + nel_members_max;

			/*************************************************/
			/* make an entry for this chunk of memory in the */
			/* list of nel_member_chunks.                     */
			/*************************************************/
			nel_malloc (chunk, 1, nel_member_chunk);
			chunk->start = nel_members_next;
			chunk->next = nel_member_chunks;
			nel_member_chunks = chunk;
		}
		retval = nel_members_next++;
	}

	/*************************/
	/* exit critical section */
	/*************************/
	nel_unlock (&nel_members_lock);

	/**************************************/
	/* initialize the appropriate members */
	/**************************************/
	retval->symbol = symbol;
	retval->garbage = 0;
	retval->bit_field = bit_field;
	retval->bit_size = bit_size;
	retval->bit_lb = bit_lb;
	retval->offset = offset;
	retval->next = next;

	nel_debug ({ nel_trace (eng, "] exiting nel_member_alloc\nretval =\n%1M\n", retval); });
	return (retval);
}



/*****************************************************************************/
/* nel_member_dealloc () returns the nel_member structure pointed to by        */
/* <member> back to the free list (nel_free_members), so that the space may   */
/* be re-used.                                                               */
/*****************************************************************************/
void nel_member_dealloc (register nel_member *member)
{
	nel_debug ({
	if (member == NULL) {
		//nel_fatal_error (NULL, "(nel_member_dealloc #1): member == NULL");
	}
	});

	/*******************************************/
	/* static allocators are critical sections */
	/*******************************************/
	nel_lock (&nel_members_lock);

	member->next = nel_free_members;
	nel_free_members = member;

	/*************************/
	/* exit critical section */
	/*************************/
	nel_unlock (&nel_members_lock);
}


/*****************************************************************************/
/* nel_print_members () prints out the members list of a struct or union type */
/* descriptor.                                                               */
/*****************************************************************************/
void nel_print_members (FILE *file, register nel_member *members, int indent)
{
	if (members == NULL) {
		tab (indent);
		fprintf (file, "NULL_member\n");
	} else {
		for (; (members != NULL); members = members->next) {
			tab (indent);
			fprintf (file, "member:\n");
			tab (indent + 1);
			fprintf (file, "symbol:\n");
			nel_print_symbol (file, members->symbol, indent + 2);
			tab (indent + 1);
			fprintf (file, "bit_field: %d\n", members->bit_field);
			tab (indent + 1);
			fprintf (file, "bit_size: %d\n", members->bit_size);
			tab (indent + 1);
			fprintf (file, "bit_lb: %d\n", members->bit_lb);
			tab (indent + 1);
			fprintf (file, "offset: %d\n", members->offset);
		}
	}
}

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
nel_type *nel_char_type;
nel_type *nel_complex_type;
nel_type *nel_complex_double_type;
nel_type *nel_complex_float_type;
nel_type *nel_double_type;
nel_type *nel_float_type;
nel_type *nel_int_type;
nel_type *nel_long_type;
nel_type *nel_long_complex_type;
nel_type *nel_long_complex_double_type;
nel_type *nel_long_complex_float_type;
nel_type *nel_long_double_type;
nel_type *nel_long_float_type;
nel_type *nel_long_int_type;
nel_type *nel_long_long_type;
nel_type *nel_short_type;
nel_type *nel_short_int_type;
nel_type *nel_signed_type;
nel_type *nel_signed_char_type;
nel_type *nel_signed_int_type;
nel_type *nel_signed_long_type;
nel_type *nel_signed_long_long_type;
nel_type *nel_signed_long_int_type;
nel_type *nel_signed_short_type;
nel_type *nel_signed_short_int_type;
nel_type *nel_unsigned_type;
nel_type *nel_unsigned_char_type;
nel_type *nel_unsigned_int_type;
nel_type *nel_unsigned_long_type;
nel_type *nel_unsigned_long_int_type;
nel_type *nel_unsigned_long_long_type;
nel_type *nel_unsigned_short_type;
nel_type *nel_unsigned_short_int_type;
nel_type *nel_void_type;

/************************************/
/* the symbols for the C type names */
/************************************/
nel_symbol *nel_char_symbol;
nel_symbol *nel_complex_symbol;
nel_symbol *nel_complex_double_symbol;
nel_symbol *nel_complex_float_symbol;
nel_symbol *nel_double_symbol;
nel_symbol *nel_float_symbol;
nel_symbol *nel_int_symbol;
nel_symbol *nel_long_symbol;
nel_symbol *nel_long_complex_symbol;
nel_symbol *nel_long_complex_double_symbol;
nel_symbol *nel_long_complex_float_symbol;
nel_symbol *nel_long_double_symbol;
nel_symbol *nel_long_float_symbol;
nel_symbol *nel_long_int_symbol;
nel_symbol *nel_long_double_symbol;
nel_symbol *nel_short_symbol;
nel_symbol *nel_short_int_symbol;
nel_symbol *nel_signed_symbol;
nel_symbol *nel_signed_char_symbol;
nel_symbol *nel_signed_int_symbol;
nel_symbol *nel_signed_long_symbol;
nel_symbol *nel_long_signed_symbol;
nel_symbol *nel_signed_long_int_symbol;
nel_symbol *nel_long_signed_int_symbol;
nel_symbol *nel_signed_short_symbol;
nel_symbol *nel_short_signed_symbol;
nel_symbol *nel_signed_short_int_symbol;
nel_symbol *nel_short_signed_int_symbol;
nel_symbol *nel_unsigned_symbol;
nel_symbol *nel_unsigned_char_symbol;
nel_symbol *nel_unsigned_int_symbol;
nel_symbol *nel_unsigned_long_symbol;
nel_symbol *nel_long_unsigned_symbol;
nel_symbol *nel_unsigned_long_int_symbol;
nel_symbol *nel_long_unsigned_int_symbol;
nel_symbol *nel_unsigned_short_symbol;
nel_symbol *nel_short_unsigned_symbol;
nel_symbol *nel_unsigned_short_int_symbol;
nel_symbol *nel_short_unsigned_int_symbol;
nel_symbol *nel_void_symbol;
//added by zhangbin, 2006-4-27
nel_symbol *nel_signed_long_long_symbol;
nel_symbol *nel_long_long_symbol;
nel_symbol *nel_unsigned_long_long_symbol;
//end add



/*************************************************************/
/* we might as well have a location type so we don't keep    */
/* allocating hunreds of identical location type descriptors */
/* when scanning the ld symbol table of the executable file. */
/* a generic pointer type comes in handy, also.              */
/*************************************************************/
nel_type *nel_pointer_type;
nel_type *nel_location_type;



/*****************************************************************************/
/* nel_type_init () performs whatever initialization is necessary of the type */
/* structures.   currently, this consists of allocating space for the        */
/* constant C and fortran simple type descriptors.                           */
/*****************************************************************************/
void nel_type_init (struct nel_eng *eng)
{
	nel_type *type_type;

	nel_debug ({ nel_trace (eng, "entering nel_type_init [\n\n"); });

	/****************************************************************/
	/* create a type descirptor for the type and assign it to the   */
	/* appropriate global variable, then create a TYPE_NAME type    */
	/* descriptor for the symbol representing the type, then create */
	/* the symbol and assign it to the appropriate global var.      */
	/****************************************************************/

	nel_char_type = nel_type_alloc (eng, nel_D_CHAR, sizeof (char),
									nel_alignment_of (char), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0, nel_char_type);
	nel_char_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "char"),
					  type_type, (char *) nel_char_type, nel_C_TYPE, 0, nel_L_C, 0);

	nel_complex_type = nel_type_alloc (eng, nel_D_COMPLEX, 2 * sizeof (double),	//modified by zhangbin, sizeof(float)=>sizeof(double)
									   nel_alignment_of (float), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0, nel_complex_type);
	nel_complex_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "complex"),
						 type_type, (char *) nel_complex_type, nel_C_TYPE, 0, nel_L_C, 0);

	nel_complex_double_type = nel_type_alloc (eng, nel_D_COMPLEX_DOUBLE,
							  2 * sizeof (double), nel_alignment_of (double), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0,
								nel_complex_double_type);
	nel_complex_double_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
								"complex double"), type_type, (char *) nel_complex_double_type,
								nel_C_TYPE, 0, nel_L_C, 0);

	nel_complex_float_type = nel_type_alloc (eng, nel_D_COMPLEX_FLOAT,
							 2 * sizeof (float), nel_alignment_of (float), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0,
								nel_complex_float_type);
	nel_complex_float_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
							   "complex float"), type_type, (char *) nel_complex_float_type, nel_C_TYPE,
							   0, nel_L_C, 0);

	nel_double_type = nel_type_alloc (eng, nel_D_DOUBLE, sizeof (double),
									  nel_alignment_of (double), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0, nel_double_type);
	nel_double_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "double"),
						type_type, (char *) nel_double_type, nel_C_TYPE, 0, nel_L_C, 0);

	nel_float_type = nel_type_alloc (eng, nel_D_FLOAT, sizeof (float),
									 nel_alignment_of (float), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0, nel_float_type);
	nel_float_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "float"),
					   type_type, (char *) nel_float_type, nel_C_TYPE, 0, nel_L_C, 0);

	nel_int_type = nel_type_alloc (eng, nel_D_INT, sizeof (int),
								   nel_alignment_of (int), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0, nel_int_type);
	nel_int_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "int"),
					 type_type, (char *) nel_int_type, nel_C_TYPE, 0, nel_L_C, 0);

	nel_long_type = nel_type_alloc (eng, nel_D_LONG, sizeof (long),
									nel_alignment_of (long), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0, nel_long_type);
	nel_long_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "long"),
					  type_type, (char *) nel_long_type, nel_C_TYPE, 0, nel_L_C, 0);


	/* wyong, 2005.4.15 */  //modified by zhangbin, 2006-4-27
	nel_long_long_type = nel_type_alloc (eng, nel_D_LONG_LONG, sizeof (long long),nel_alignment_of (long long ), 0, 0);
	type_type = nel_type_alloc(eng, nel_D_TYPE_NAME, 0, 0, 0, 0, nel_long_long_type);
	nel_long_long_symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng, "long long int"), type_type, 
													(char*)nel_long_long_type, nel_C_TYPE, 0, nel_L_C, 0);
	
	nel_signed_long_long_type = nel_type_alloc (eng, nel_D_SIGNED_LONG_LONG, sizeof (signed long long),
												nel_alignment_of (signed long long ), 0, 0);
	type_type = nel_type_alloc(eng, nel_D_TYPE_NAME, 0, 0, 0, 0, nel_signed_long_long_type);
	nel_signed_long_long_symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng, "signed long long int"), type_type, 
														(char*)nel_signed_long_long_type, nel_C_TYPE, 0, nel_L_C, 0);

	
	nel_unsigned_long_long_type = nel_type_alloc (eng, nel_D_UNSIGNED_LONG_LONG, sizeof (unsigned long long),
													nel_alignment_of (unsigned long long ), 0, 0);
	type_type = nel_type_alloc(eng, nel_D_TYPE_NAME, 0, 0, 0, 0, nel_unsigned_long_long_type);
	nel_unsigned_long_long_symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng, "long long unsigned int"), type_type, 
														(char*)nel_unsigned_long_long_type, nel_C_TYPE, 0, nel_L_C, 0);
	//end add, 2006-4-27

	//type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0, nel_long_long_type);
	//nel_long_long_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "long"),type_type, (char *) nel_long_type, nel_C_TYPE, 0, nel_L_C, 0);



	nel_long_complex_type = nel_type_alloc (eng, nel_D_LONG_COMPLEX,
											2 * sizeof (long_float), nel_alignment_of (long_float), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0,
								nel_long_complex_type);
	nel_long_complex_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
							  "long complex"), type_type, (char *) nel_long_complex_type, nel_C_TYPE,
							  0, nel_L_C, 0);

	nel_long_complex_double_type = nel_type_alloc (eng, nel_D_LONG_COMPLEX_DOUBLE,
								   2 * sizeof (long_double), nel_alignment_of (long_double), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0,
								nel_long_complex_double_type);
	nel_long_complex_double_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
									 "long complex double"), type_type, (char *) nel_long_complex_double_type,
									 nel_C_TYPE, 0, nel_L_C, 0);

	nel_long_complex_float_type = nel_type_alloc (eng, nel_D_LONG_COMPLEX_FLOAT,
								  2 * sizeof (long_float), nel_alignment_of (long_float), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0,
								nel_long_complex_float_type);
	nel_long_complex_float_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
									"long complex float"), type_type, (char *) nel_long_complex_float_type,
									nel_C_TYPE, 0, nel_L_C, 0);

	nel_long_double_type = nel_type_alloc (eng, nel_D_LONG_DOUBLE,
										   sizeof (long_double), nel_alignment_of (long_double), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0,
								nel_long_double_type);
	nel_long_double_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
							 "long double"), type_type, (char *) nel_long_double_type, nel_C_TYPE,
							 0, nel_L_C, 0);

	nel_long_float_type = nel_type_alloc (eng, nel_D_LONG_FLOAT, sizeof (long_float),
										  nel_alignment_of (long_float), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0,
								nel_long_float_type);
	nel_long_float_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
							"long float"), type_type, (char *) nel_long_float_type, nel_C_TYPE,
							0, nel_L_C, 0);

	nel_long_int_type = nel_type_alloc (eng, nel_D_LONG_INT, sizeof (long_int),
										nel_alignment_of (long_int), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0, nel_long_int_type);
	nel_long_int_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "long int"),
						  type_type, (char *) nel_long_int_type, nel_C_TYPE, 0, nel_L_C, 0);

	nel_short_type = nel_type_alloc (eng, nel_D_SHORT, sizeof (short),
									 nel_alignment_of (short), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0, nel_short_type);
	nel_short_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "short"),
					   type_type, (char *) nel_short_type, nel_C_TYPE, 0, nel_L_C, 0);

	nel_short_int_type = nel_type_alloc (eng, nel_D_SHORT_INT, sizeof (short_int),
										 nel_alignment_of (short_int), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0, nel_short_int_type);
	nel_short_int_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
						   "short int"), type_type, (char *) nel_short_int_type, nel_C_TYPE,
						   0, nel_L_C, 0);

	nel_signed_type = nel_type_alloc (eng, nel_D_SIGNED, sizeof (signed),
									  nel_alignment_of (signed), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0, nel_signed_type);
	nel_signed_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "signed"),
						type_type, (char *) nel_signed_type, nel_C_TYPE, 0, nel_L_C, 0);

	nel_signed_char_type = nel_type_alloc (eng, nel_D_SIGNED_CHAR,
										   sizeof (signed_char), nel_alignment_of (signed_char), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0,
								nel_signed_char_type);
	nel_signed_char_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
							 "signed char"), type_type, (char *) nel_signed_char_type, nel_C_TYPE,
							 0, nel_L_C, 0);

	nel_signed_int_type = nel_type_alloc (eng, nel_D_SIGNED_INT,
										  sizeof (signed_int), nel_alignment_of (signed_int), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0,
								nel_signed_int_type);
	nel_signed_int_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
							"signed int"), type_type, (char *) nel_signed_int_type, nel_C_TYPE,
							0, nel_L_C, 0);

	/***********************************************/
	/* "signed long" may be called "long unsigned" */
	/***********************************************/
	nel_signed_long_type = nel_type_alloc (eng, nel_D_SIGNED_LONG, sizeof (
											   signed_long), nel_alignment_of (signed_long), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0,
								nel_signed_long_type);
	nel_signed_long_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
							 "signed long"), type_type, (char *) nel_signed_long_type, nel_C_TYPE,
							 0, nel_L_C, 0);
	nel_long_signed_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
							 "long signed"), type_type, (char *) nel_signed_long_type, nel_C_TYPE,
							 0, nel_L_C, 0);

	/*****************************************************/
	/* "signed long int" may be called "long signed int" */
	/*****************************************************/
	nel_signed_long_int_type = nel_type_alloc (eng, nel_D_SIGNED_LONG_INT,
							   sizeof (signed_long_int), nel_alignment_of (signed_long_int), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0,
								nel_signed_long_int_type);
	nel_signed_long_int_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
								 "signed long int"), type_type, (char *) nel_signed_long_int_type,
								 nel_C_TYPE, 0, nel_L_C, 0);
	nel_long_signed_int_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
								 "long signed int"), type_type, (char *) nel_signed_long_int_type,
								 nel_C_TYPE, 0, nel_L_C, 0);

	/***********************************************/
	/* "signed short" may be called "short signed" */
	/***********************************************/
	nel_signed_short_type = nel_type_alloc (eng, nel_D_SIGNED_SHORT,
											sizeof (signed_short), nel_alignment_of (signed_short), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0,
								nel_signed_short_type);
	nel_signed_short_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
							  "signed short"), type_type, (char *) nel_signed_short_type, nel_C_TYPE,
							  0, nel_L_C, 0);
	nel_short_signed_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
							  "short signed"), type_type, (char *) nel_signed_short_type, nel_C_TYPE,
							  0, nel_L_C, 0);

	/*******************************************************/
	/* "signed short int" may be called "short signed int" */
	/*******************************************************/
	nel_signed_short_int_type = nel_type_alloc (eng, nel_D_SIGNED_SHORT_INT,
								sizeof (signed_short_int), nel_alignment_of (signed_short_int), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0,
								nel_signed_short_int_type);
	nel_signed_short_int_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
								  "signed short int"), type_type, (char *) nel_signed_short_int_type,
								  nel_C_TYPE, 0, nel_L_C, 0);
	nel_short_signed_int_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
								  "short signed int"), type_type, (char *) nel_signed_short_int_type,
								  nel_C_TYPE, 0, nel_L_C, 0);

	nel_unsigned_type = nel_type_alloc (eng, nel_D_UNSIGNED, sizeof (unsigned),
										nel_alignment_of (unsigned), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0, nel_unsigned_type);
	nel_unsigned_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "unsigned"),
						  type_type, (char *) nel_unsigned_type, nel_C_TYPE, 0, nel_L_C, 0);

	nel_unsigned_char_type = nel_type_alloc (eng, nel_D_UNSIGNED_CHAR,
							 sizeof (unsigned_char), nel_alignment_of (unsigned_char), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0,
								nel_unsigned_char_type);
	nel_unsigned_char_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
							   "unsigned char"), type_type, (char *) nel_unsigned_char_type, nel_C_TYPE,
							   0, nel_L_C, 0);

	nel_unsigned_int_type = nel_type_alloc (eng, nel_D_UNSIGNED_INT,
											sizeof (unsigned_int), nel_alignment_of (unsigned_int), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0,
								nel_unsigned_int_type);
	nel_unsigned_int_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
							  "unsigned int"), type_type, (char *) nel_unsigned_int_type, nel_C_TYPE,
							  0, nel_L_C, 0);

	/*************************************************/
	/* "unsigned long" may be called "long unsigned" */
	/*************************************************/
	nel_unsigned_long_type = nel_type_alloc (eng, nel_D_UNSIGNED_LONG,
							 sizeof (unsigned_long), nel_alignment_of (unsigned_long), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0,
								nel_unsigned_long_type);
	nel_unsigned_long_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
							   "unsigned long"), type_type, (char *) nel_unsigned_long_type, nel_C_TYPE,
							   0, nel_L_C, 0);
	nel_long_unsigned_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
							   "long unsigned"), type_type, (char *) nel_unsigned_long_type, nel_C_TYPE,
							   0, nel_L_C, 0);

	/*********************************************************/
	/* "unsigned long int" may be called "long unsigned int" */
	/*********************************************************/
	nel_unsigned_long_int_type = nel_type_alloc (eng, nel_D_UNSIGNED_LONG_INT,
								 sizeof (unsigned_long_int), nel_alignment_of (unsigned_long_int), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0,
								nel_unsigned_long_int_type);
	nel_unsigned_long_int_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
								   "unsigned long int"), type_type, (char *) nel_unsigned_long_int_type,
								   nel_C_TYPE, 0, nel_L_C, 0);
	nel_long_unsigned_int_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
								   "long unsigned int"), type_type, (char *) nel_unsigned_long_int_type,
								   nel_C_TYPE, 0, nel_L_C, 0);

	/***************************************************/
	/* "unsigned short" may be called "short unsigned" */
	/***************************************************/
	nel_unsigned_short_type = nel_type_alloc (eng, nel_D_UNSIGNED_SHORT,
							  sizeof (unsigned_short), nel_alignment_of (unsigned_short), 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0,
								nel_unsigned_short_type);
	nel_unsigned_short_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
								"unsigned short"), type_type, (char *) nel_unsigned_short_type,
								nel_C_TYPE, 0, nel_L_C, 0);
	nel_short_unsigned_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
								"short unsigned"), type_type, (char *) nel_unsigned_short_type,
								nel_C_TYPE, 0, nel_L_C, 0);

	/***********************************************************/
	/* "unsigned short int" may be called "short unsigned int" */
	/***********************************************************/
	nel_unsigned_short_int_type = nel_type_alloc (eng, nel_D_UNSIGNED_SHORT_INT,
								  sizeof (unsigned_short_int), nel_alignment_of (unsigned_short_int), 0,
								  0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0,
								nel_unsigned_short_int_type);
	nel_unsigned_short_int_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
									"unsigned short int"), type_type, (char *) nel_unsigned_short_int_type,
									nel_C_TYPE, 0, nel_L_C, 0);
	nel_short_unsigned_int_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng,
									"short unsigned int"), type_type, (char *) nel_unsigned_short_int_type,
									nel_C_TYPE, 0, nel_L_C, 0);

	/******************************************************/
	/* give the void type descriptor size 1, so that      */
	/* address calculations on void * pointers will work. */
	/******************************************************/
	nel_void_type = nel_type_alloc (eng, nel_D_VOID, 1, 1, 0, 0);
	type_type = nel_type_alloc (eng, nel_D_TYPE_NAME, 0, 0, 0, 0, nel_void_type);
	nel_void_symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "void"),
					  type_type, (char *) nel_void_type, nel_C_TYPE, 0, nel_L_C, 0);


	/*********************************************************/
	/* initialize nel_pointer_type and nel_location_type, also */
	/*********************************************************/
	nel_pointer_type = nel_type_alloc (eng, nel_D_POINTER, sizeof (char *), nel_alignment_of (char *), 0, 0, nel_void_type);
	nel_location_type = nel_type_alloc (eng, nel_D_LOCATION, 0, 0, 0, 0);

	nel_debug ({ nel_trace (eng, "] exiting nel_type_init\n\n"); });
}



#ifdef NEL_STAB
/************************************************************/
/* we only need to insert the type descriptors into the     */
/* symbol tables if we are compiling with the stab scanner. */
/************************************************************/



#define insert(_eng, _symbol) \
	{								\
	   register nel_symbol *__symbol = (_symbol);			\
	   if ((__symbol != NULL) && (__symbol->table == NULL)) {	\
	      nel_insert_symbol ((_eng), (__symbol), (_eng)->nel_static_ident_hash);	\
	   }								\
	}



/*****************************************************************************/
/* nel_insert_C_types () inserts symbols for the simple C types into the      */
/* ident hash tables (nel_static_ident_hash).                                 */
/*****************************************************************************/
void nel_insert_C_types (struct nel_eng *eng)
{
	nel_debug ({ nel_trace (eng, "entering nel_insert_C_types [\n\n"); });

	insert(eng, nel_char_symbol);
	insert(eng, nel_complex_symbol);
	insert(eng, nel_complex_double_symbol);
	insert(eng, nel_complex_float_symbol);
	insert(eng, nel_double_symbol);
	insert(eng, nel_float_symbol);
	insert(eng, nel_int_symbol);
	insert(eng, nel_long_symbol);
	insert(eng, nel_long_complex_symbol);
	insert(eng, nel_long_complex_double_symbol);
	insert(eng, nel_long_complex_float_symbol);
	insert(eng, nel_long_double_symbol);
	insert(eng, nel_long_float_symbol);
	insert(eng, nel_long_int_symbol);
	insert(eng, nel_short_symbol);
	insert(eng, nel_short_int_symbol);
	insert(eng, nel_signed_symbol);
	insert(eng, nel_signed_char_symbol);
	insert(eng, nel_signed_int_symbol);
	insert(eng, nel_signed_long_symbol);
	insert(eng, nel_long_signed_symbol);
	insert(eng, nel_signed_long_int_symbol);
	insert(eng, nel_long_signed_int_symbol);
	insert(eng, nel_signed_short_symbol);
	insert(eng, nel_short_signed_symbol);
	insert(eng, nel_signed_short_int_symbol);
	insert(eng, nel_short_signed_int_symbol);
	insert(eng, nel_unsigned_symbol);
	insert(eng, nel_unsigned_char_symbol);
	insert(eng, nel_unsigned_int_symbol);
	insert(eng, nel_unsigned_long_symbol);
	insert(eng, nel_long_unsigned_symbol);
	insert(eng, nel_unsigned_long_int_symbol);
	insert(eng, nel_long_unsigned_int_symbol);
	insert(eng, nel_unsigned_short_symbol);
	insert(eng, nel_short_unsigned_symbol);
	insert(eng, nel_unsigned_short_int_symbol);
	insert(eng, nel_short_unsigned_int_symbol);
	insert(eng, nel_void_symbol);
	//added by zhangbin, 2006-4-26
	insert(eng, nel_signed_long_long_symbol);
	insert(eng, nel_long_long_symbol);
	insert(eng, nel_unsigned_long_long_symbol);
	//end

	nel_debug ({ nel_trace (eng, "] exiting nel_insert_C_types\n\n"); });
}



/*****************************************************************************/
/* nel_remove_C_types () removes symbols for the simple C types from the      */
/* ident hash tables (nel_static_ident_hash).                                 */
/*****************************************************************************/
void nel_remove_C_types (struct nel_eng *eng)
{
	nel_debug ({ nel_trace (eng, "entering nel_remove_C_types [\n\n"); });

	nel_remove_symbol (eng, nel_char_symbol);
	nel_remove_symbol (eng, nel_complex_symbol);
	nel_remove_symbol (eng, nel_complex_double_symbol);
	nel_remove_symbol (eng, nel_complex_float_symbol);
	nel_remove_symbol (eng, nel_double_symbol);
	nel_remove_symbol (eng, nel_float_symbol);
	nel_remove_symbol (eng, nel_int_symbol);
	nel_remove_symbol (eng, nel_long_symbol);
	nel_remove_symbol (eng, nel_long_complex_symbol);
	nel_remove_symbol (eng, nel_long_complex_double_symbol);
	nel_remove_symbol (eng, nel_long_complex_float_symbol);
	nel_remove_symbol (eng, nel_long_double_symbol);
	nel_remove_symbol (eng, nel_long_float_symbol);
	nel_remove_symbol (eng, nel_long_int_symbol);
	nel_remove_symbol (eng, nel_short_symbol);
	nel_remove_symbol (eng, nel_short_int_symbol);
	nel_remove_symbol (eng, nel_signed_symbol);
	nel_remove_symbol (eng, nel_signed_char_symbol);
	nel_remove_symbol (eng, nel_signed_int_symbol);
	nel_remove_symbol (eng, nel_signed_long_symbol);
	nel_remove_symbol (eng, nel_long_signed_symbol);
	nel_remove_symbol (eng, nel_signed_long_int_symbol);
	nel_remove_symbol (eng, nel_long_signed_int_symbol);
	nel_remove_symbol (eng, nel_signed_short_symbol);
	nel_remove_symbol (eng, nel_short_signed_symbol);
	nel_remove_symbol (eng, nel_signed_short_int_symbol);
	nel_remove_symbol (eng, nel_short_signed_int_symbol);
	nel_remove_symbol (eng, nel_unsigned_symbol);
	nel_remove_symbol (eng, nel_unsigned_char_symbol);
	nel_remove_symbol (eng, nel_unsigned_int_symbol);
	nel_remove_symbol (eng, nel_unsigned_long_symbol);
	nel_remove_symbol (eng, nel_long_unsigned_symbol);
	nel_remove_symbol (eng, nel_unsigned_long_int_symbol);
	nel_remove_symbol (eng, nel_long_unsigned_int_symbol);
	nel_remove_symbol (eng, nel_unsigned_short_symbol);
	nel_remove_symbol (eng, nel_short_unsigned_symbol);
	nel_remove_symbol (eng, nel_unsigned_short_int_symbol);
	nel_remove_symbol (eng, nel_short_unsigned_int_symbol);
	nel_remove_symbol (eng, nel_void_symbol);

//added by zhangbin
	nel_remove_symbol(eng, nel_signed_long_long_symbol);
	nel_remove_symbol(eng, nel_long_long_symbol);
	nel_remove_symbol(eng, nel_unsigned_long_long_symbol);
//end
	nel_debug ({ nel_trace (eng, "] exiting nel_remove_C_types\n\n"); });
}


#undef insert
#endif /* NEL_STAB */


/*****************************************************************************/
/* nel_merge_types () takes two type descriptors and merges them, returning   */
/* a pointer to the new descriptor.  for instance, if the first type         */
/* descriptor had type nel_D_UNSIGNED and the second had type nel_D_INT, the   */
/* result would have type nel_D_UNSIGNED_INT.                                 */
/*****************************************************************************/
nel_type *nel_merge_types (struct nel_eng *eng, register nel_type *type1, register nel_type *type2)
{
	register nel_type *retval = NULL;

	nel_debug ({ nel_trace (eng, "entering nel_merge_types [\ntype1 =\n%1Ttype2 =\n%1T\n", type1, type2); });

#define ret(_retval) \
	{								\
	   retval = (_retval);						\
	   goto end;							\
	}

	switch (type1->simple.type) {
	case nel_D_CHAR:
		switch (type2->simple.type) {
		case nel_D_SIGNED:
			ret (nel_signed_char_type);
		case nel_D_UNSIGNED:
			ret (nel_unsigned_char_type);
		default:
			ret (NULL);
		}
	case nel_D_COMPLEX:
		switch (type2->simple.type) {
		case nel_D_DOUBLE:
			ret (nel_complex_double_type);
		case nel_D_FLOAT:
			ret (nel_complex_float_type);
		case nel_D_LONG:
			ret (nel_long_complex_type);
		case nel_D_LONG_DOUBLE:
			ret (nel_long_complex_double_type);
		case nel_D_LONG_FLOAT:
			ret (nel_long_complex_float_type);
		default:
			ret (NULL);
		}
	case nel_D_COMPLEX_DOUBLE:
		switch (type2->simple.type) {
		case nel_D_LONG:
			ret (nel_long_complex_double_type);
		default:
			ret (NULL);
		}
	case nel_D_COMPLEX_FLOAT:
		switch (type2->simple.type) {
		case nel_D_LONG:
			ret (nel_long_complex_float_type);
		default:
			ret (NULL);
		}
	case nel_D_DOUBLE:
		switch (type2->simple.type) {
		case nel_D_COMPLEX:
			ret (nel_complex_double_type);
		case nel_D_LONG:
			ret (nel_long_double_type);
		case nel_D_LONG_COMPLEX:
			ret (nel_long_complex_double_type);
		default:
			ret (NULL);
		}
	case nel_D_FLOAT:
		switch (type2->simple.type) {
		case nel_D_COMPLEX:
			ret (nel_complex_float_type);
		case nel_D_LONG:
			ret (nel_long_float_type);
		case nel_D_LONG_COMPLEX:
			ret (nel_long_complex_float_type);
		default:
			ret (NULL);
		}
	case nel_D_INT:
		switch (type2->simple.type) {
		case nel_D_LONG:
			ret (nel_long_int_type);
		case nel_D_SHORT:
			ret (nel_short_int_type);
		case nel_D_SIGNED:
			ret (nel_signed_int_type);
		case nel_D_SIGNED_LONG:
			ret (nel_signed_long_int_type);
		case nel_D_SIGNED_SHORT:
			ret (nel_signed_short_int_type);
		case nel_D_UNSIGNED:
			ret (nel_unsigned_int_type);
		case nel_D_UNSIGNED_LONG:
			ret (nel_unsigned_long_int_type);
		case nel_D_UNSIGNED_SHORT:
			ret (nel_unsigned_short_int_type);
		default:
			ret (NULL);
		}
	case nel_D_LONG:
		switch (type2->simple.type) {
		case nel_D_COMPLEX:
			ret (nel_long_complex_type);
		case nel_D_COMPLEX_DOUBLE:
			ret (nel_long_complex_double_type);
		case nel_D_COMPLEX_FLOAT:
			ret (nel_long_complex_float_type);
		case nel_D_DOUBLE:
			ret (nel_long_double_type);
		case nel_D_FLOAT:
			ret (nel_long_float_type);
		case nel_D_INT:
			ret (nel_long_int_type);
		case nel_D_SIGNED:
			ret (nel_signed_long_type);
		case nel_D_SIGNED_INT:
			ret (nel_signed_long_int_type);
		case nel_D_UNSIGNED:
			ret (nel_unsigned_long_type);
		case nel_D_UNSIGNED_INT:
			ret (nel_unsigned_long_int_type);
		default:
			ret (NULL);
		}
	case nel_D_LONG_COMPLEX:
		switch (type2->simple.type) {
		case nel_D_DOUBLE:
			ret (nel_long_complex_double_type);
		case nel_D_FLOAT:
			ret (nel_long_complex_float_type);
		default:
			ret (NULL);
		}
	case nel_D_LONG_DOUBLE:
		switch (type2->simple.type) {
		case nel_D_COMPLEX:
			ret (nel_long_complex_double_type);
		default:
			ret (NULL);
		}
	case nel_D_LONG_FLOAT:
		switch (type2->simple.type) {
		case nel_D_COMPLEX:
			ret (nel_long_complex_float_type);
		default:
			ret (NULL);
		}
	case nel_D_LONG_INT:
		switch (type2->simple.type) {
		case nel_D_SIGNED:
			ret (nel_signed_long_int_type);
		case nel_D_UNSIGNED:
			ret (nel_unsigned_long_int_type);
		default:
			ret (NULL);
		}
	case nel_D_SHORT:
		switch (type2->simple.type) {
		case nel_D_INT:
			ret (nel_short_int_type);
		case nel_D_SIGNED:
			ret (nel_signed_short_type);
		case nel_D_SIGNED_INT:
			ret (nel_signed_short_int_type);
		case nel_D_UNSIGNED:
			ret (nel_unsigned_short_type);
		case nel_D_UNSIGNED_INT:
			ret (nel_unsigned_short_int_type);
		default:
			ret (NULL);
		}
	case nel_D_SHORT_INT:
		switch (type2->simple.type) {
		case nel_D_SIGNED:
			ret (nel_signed_short_int_type);
		case nel_D_UNSIGNED:
			ret (nel_unsigned_short_int_type);
		default:
			ret (NULL);
		}
	case nel_D_SIGNED:
		switch (type2->simple.type) {
		case nel_D_CHAR:
			ret (nel_signed_char_type);
		case nel_D_INT:
			ret (nel_signed_int_type);
		case nel_D_LONG:
			ret (nel_signed_long_type);
		case nel_D_LONG_INT:
			ret (nel_signed_long_int_type);
		case nel_D_SHORT:
			ret (nel_signed_short_type);
		case nel_D_SHORT_INT:
			ret (nel_signed_short_int_type);
		default:
			ret (NULL);
		}
	case nel_D_SIGNED_INT:
		switch (type2->simple.type) {
		case nel_D_LONG:
			ret (nel_signed_long_int_type);
		case nel_D_SHORT:
			ret (nel_signed_short_int_type);
		default:
			ret (NULL);
		}
	case nel_D_SIGNED_LONG:
		switch (type2->simple.type) {
		case nel_D_INT:
			ret (nel_signed_long_int_type);
		default:
			ret (NULL);
		}
	case nel_D_SIGNED_SHORT:
		switch (type2->simple.type) {
		case nel_D_INT:
			ret (nel_signed_short_int_type);
		default:
			ret (NULL);
		}
	case nel_D_UNSIGNED:
		switch (type2->simple.type) {
		case nel_D_CHAR:
			ret (nel_unsigned_char_type);
		case nel_D_INT:
			ret (nel_unsigned_int_type);
		case nel_D_LONG:
			ret (nel_unsigned_long_type);
		case nel_D_LONG_INT:
			ret (nel_unsigned_long_int_type);
		case nel_D_SHORT:
			ret (nel_unsigned_short_type);
		case nel_D_SHORT_INT:
			ret (nel_unsigned_short_int_type);
		default:
			ret (NULL);
		}
	case nel_D_UNSIGNED_INT:
		switch (type2->simple.type) {
		case nel_D_LONG:
			ret (nel_unsigned_long_int_type);
		case nel_D_SHORT:
			ret (nel_unsigned_short_int_type);
		default:
			ret (NULL);
		}
	case nel_D_UNSIGNED_LONG:
		switch (type2->simple.type) {
		case nel_D_INT:
			ret (nel_unsigned_long_int_type);
		default:
			ret (NULL);
		}
	case nel_D_UNSIGNED_SHORT:
		switch (type2->simple.type) {
		case nel_D_INT:
			ret (nel_unsigned_short_int_type);
		default:
			ret (NULL);
		}
	default:
		ret (NULL);
	}

#undef ret

end:
	nel_debug ({ nel_trace (eng, "] exiting nel_merge_types\nretval =\n%1T\n", retval); });
	return (retval);
}


/*****************************************************************************/
/* nel_arg_diff () is called by nel_type_diff () to see if the types of the    */
/* nel_symbols in two argument lists (nel_list *) differ.  see nel_type_diff () */
/* for an explanation of check_qual and the return codes.                    */
/*****************************************************************************/
unsigned_int nel_arg_diff (register nel_list *list1, register nel_list *list2, register unsigned_int check_qual)
{
	register unsigned_int retval = 0;

	for (;;) {
		if (list1 == list2) {
			return (retval);
		}
		if ((list1 == NULL) || (list2 == NULL)) {
			return (1);
		}
		if (list1->symbol == list2->symbol) {
			/********************/
			/* shouldn't happen */
			/********************/
			list1 = list1->next;
			list2 = list2->next;
			continue;
		}
		if ((list1->symbol == NULL) || (list2->symbol == NULL)) {
			/********************/
			/* shouldn't happen */
			/********************/
			return (1);
		}
		retval = retval & nel_type_diff (list1->symbol->type, list2->symbol->type, check_qual);
		if (retval & 1) {
			return (1);
		}
		list1 = list1->next;
		list2 = list2->next;
	}
}


/*****************************************************************************/
/* nel_enum_const_lt () returns true if list1 contains a const not found in   */
/* list2.                                                                    */
/*****************************************************************************/
unsigned_int nel_enum_const_lt (register nel_list *list1, nel_list *list2)
{
	register nel_list *scan2;
	register nel_symbol *symbol1;
	register nel_symbol *symbol2;

	for (; (list1 != NULL); list1 = list1->next) {
		if (((symbol1 = list1->symbol) == NULL) || (symbol1->name == NULL)) {
			continue;
		}
		for (scan2 = list2; (scan2 != NULL); scan2 = scan2->next) {
			if (scan2->value != list1->value) {
				continue;
			}
			if (((symbol2 = scan2->symbol) == NULL) || (symbol2->name == NULL)) {
				continue;
			}
			if (strcmp (symbol2->name, symbol1->name)) {
				continue;
			}
			/*********************************************************/
			/* don't compare the types - they just point back to the */
			/* enum type descriptor whose constants we are comparing */
			/*********************************************************/
			/************************************************/
			/* found a match -  goto next constant in list1 */
			/************************************************/
			break;
		}
		if (scan2 == NULL) {
			return (1);
		}
	}
	return (0);
}


/*****************************************************************************/
/* nel_enum_const_diff () is called by nel_type_diff () to see if two lists of */
/* enum constants differ.  we need to compare the names and values of the    */
/* symbols; we know they are all int type.   see nel_type_diff for an         */
/* explanation of the return codes.                                          */
/*****************************************************************************/
unsigned_int nel_enum_const_diff (register nel_list *list1, register nel_list *list2)
{
	return (nel_enum_const_lt (list1, list2) || (nel_enum_const_lt (list2, list1)));
}


/*****************************************************************************/
/* nel_member_diff () is called by nel_type_diff () to see if two lists of     */
/* nel_member structures differ.  see nel_type_diff () for an explanation of   */
/* check_qual and the return codes.                                          */
/*****************************************************************************/
unsigned_int nel_member_diff (register nel_member *member1, register nel_member *member2, register unsigned_int check_qual)
{
	register unsigned_int retval = 0;

	for (;;) {
		if (member1 == member2) {
			return (retval);
		}
		if ((member1 == NULL) || (member2 == NULL)) {
			return (1);
		}
		if (member1->symbol == member2->symbol) {
			/********************/
			/* shouldn't happen */
			/********************/
			member1 = member1->next;
			member2 = member2->next;
			continue;
		}
		if ((member1->symbol == NULL) || (member2->symbol == NULL)) {
			/********************/
			/* shouldn't happen */
			/********************/
			return (1);
		}
		if ((member1->symbol->name == NULL) || (member2->symbol->name == NULL)) {
			/********************/
			/* shouldn't happen */
			/********************/
			return (1);
		}
		if (strcmp (member1->symbol->name, member2->symbol->name)) {
			return (1);
		}
		if (member1->bit_field != member2->bit_field) {
			return (1);
		}
		if (member1->bit_field) {
			if (member1->bit_size != member2->bit_size) {
				return (1);
			}
			if ((member1->offset * CHAR_BIT + member1->bit_lb) !=
					(member2->offset * CHAR_BIT + member2->bit_lb)) {
				return (1);
			}
		}
		if (member1->offset != member2->offset) {
			return (1);
		}
		retval = retval |/*modified by zhangbin, 2006-5-17, '&'=>'|'*/ nel_type_diff (member1->symbol->type, member2->symbol->type, check_qual);
		if (retval & 1) {
			return (1);
		}
		member1 = member1->next;
		member2 = member2->next;
	}
}


/*****************************************************************************/
/* nel_type_diff compares type descriptors type1 and type2.                   */
/* we comprimise C's struct tag-name equivalence for structural equivalence, */
/* as the .stab strings in the executable code do not alway preserve the tag */
/* names completely.  should we be unable to compare two structs / unions    */
/* because one or both of them are incomplete, we return a '2'.  in the      */
/* interpreter, this is flagged as an error, as all occurrence of an         */
/* incomplete struct / union with the same tag result in the same type       */
/* descriptor (which is considered equal to itself, even if it's incomplete) */
/* not merely an equivalent type descriptor, as could happen scanning the    */
/* stab table.  when scanning the stab table, this could concievably happen, */
/* and it is not flagged as an error.  it is possible that we may miss       */
/* a redeclaration of a type when parsing the stab table.                    */
/*                                                                           */
/* if check_qual is true, the const/volatile type qualifiers are taken into  */
/* account when comparing type descriptors.  they are always ingored when    */
/* comparing types for which they make no sense (functions, etc)             */
/*                                                                           */
/* return value:                                                             */
/*                                                                           */
/*    0: type1 and type2 are equivalent                                      */
/*    1: type1 and type2 differ.                                             */
/*    2: the comparison failed because one or both of the type descriptors   */
/*          contained an incomplete struct / union.  the type descriptors    */
/*          are otherwise equivalent.  the comparison of an incomplete       */
/*          struct / union to itself (not a copy) returns 0, not 2.          */
/*                                                                           */
/*****************************************************************************/
unsigned_int nel_type_diff (register nel_type *type1, register nel_type *type2, register unsigned_int check_qual)
{
	register unsigned_int retval = 0;

	if (type1 == type2) {
		return (0);
	}
	if ((type1 == NULL) || (type2 == NULL)) {
		return (1);
	}
	if (nel_D_token_equiv (type1->simple.type) != nel_D_token_equiv (type2->simple.type)) {
		return (1);
	}


	switch (type1->simple.type) {
	case nel_D_ARRAY:
		/*************************************************/
		/* only compare sizes if both types are complete */
		/*************************************************/
		if (check_qual) {
			if ((type1->array._const != type2->array._const) ||
					(type1->array._volatile != type2->array._volatile)) {
				return (1);
			}
		}
		
		if ((type1->array.size > 0 ) && (type2->array.size > 0 )) {
			if (type1->array.size != type2->array.size) {
				return (1);
			}
		}
		retval = nel_type_diff (type1->array.base_type, type2->array.base_type, check_qual);
		if (retval & 1) {
			return (1);
		}
		retval = nel_type_diff (type1->array.index_type, type2->array.index_type, check_qual);
		if (retval & 1) {
			return (1);
		}

		/***************************************************/
		/* if both bounds were not integral and evaluated, */
		/* don't compare them - this type is incomplete,   */
		/* for C interpreter purposes.                     */
		/***************************************************/
		if ((type1->array.size <= 0) || (type2->array.size <= 0) ||
			(! type1->array.known_lb) || (! type2->array.known_lb)
			|| (! type1->array.known_ub) || (! type2->array.known_ub)) {
			return (2);
		}
#if 0
		if (type1->array.lb.value != type2->array.lb.value) {
			return (1);
		}
		if (type1->array.ub.value != type2->array.ub.value) {
			return (1);
		}
#else
		if(type1->array.size != type2->array.size)
			return 1;
#endif
		return (retval);

	case nel_D_ENUM:
		/*******************************************************/
		/* note that all enum constants have size sizeof (int) */
		/*******************************************************/
		if (check_qual) {
			if ((type1->array._const != type2->array._const) ||
					(type1->array._volatile != type2->array._volatile)) {
				return (1);
			}
		}
		if (type1->enumed.nconsts != type2->enumed.nconsts) {
			return (1);
		}
		return (nel_enum_const_diff (type1->enumed.consts, type2->enumed.consts));

	case nel_D_FUNCTION:
		/***************************************************/
		/* for functions, ignore size, const, and volatile */
		/***************************************************/
		retval = nel_type_diff (type1->function.return_type, type2->function.return_type, check_qual);
		if (retval & 1) {
			return (1);
		}
		if ((! type1->function.new_style) || (! type2->function.new_style)) {
			return (retval);
		}

		/********************************************************/
		/* only check to see if the argument types are diff if */
		/* both types descriptors are for new-style functions.  */
		/********************************************************/
		if (type1->function.var_args != type2->function.var_args) {
			return (1);
		}
		retval = retval & nel_arg_diff (type1->function.args, type2->function.args, check_qual);
		if (retval & 1) {
			return (1);
		}
		return (retval);

	case nel_D_POINTER:
		/***********************************/
		/* all pointers have the same size */
		/***********************************/
		if (check_qual) {
			if ((type1->pointer._const != type2->pointer._const) ||
					(type1->pointer._volatile != type2->pointer._volatile)) {
				return (1);
			}
		}
		return (nel_type_diff (type1->pointer.deref_type,
							   type2->pointer.deref_type, check_qual));

	case nel_D_STRUCT:
	case nel_D_UNION:
		/*********************************************************/
		/* we assume that BOTH structures are not circular.      */
		/* if the parser encounters a redeclaration of a struct  */
		/* or union type with pointers to itself, the deref_type */
		/* of the pointer for the second declaration is actually */
		/* the type descriptor for the first declaration.        */
		/* circular structures will be check as far as possible. */
		/*********************************************************/
		if (check_qual) {
			if ((type1->s_u._const != type2->s_u._const) ||
					(type1->s_u._volatile != type2->s_u._volatile)) {
				return (1);
			}
		}
		if (type1->simple.type != type2->simple.type) {
			/***************************************/
			/* could have one struct and one union */
			/***************************************/
			return (1);
		}
		if ((type1->s_u.members == NULL) || (type2->s_u.members == NULL)) {
			/***************************************/
			/* one structure / union is incomplete */
			/***************************************/
			return (2);
		}
		if (type1->simple.size != type2->simple.size) {
			/**************************************************************/
			/* check for unequal sizes after checking for incompleteness. */
			/* incomplete structs / unions have size 0.                   */
			/**************************************************************/
			return (1);
		}

		/************************************************************/
		/* structures and unions may form circular structures, so   */
		/* we mark them as traversed to prevent infinite recursion. */
		/* for robust type checking, we would create of a list of   */
		/* unified struct/unions.  if both types were in this list, */
		/* we return true.  otherwise, we merge the two lists, and  */
		/* recursively check the members.                           */
		/************************************************************/
		if (type1->simple.traversed && type2->simple.traversed) {
			return (0);
		}

		/***************************************************/
		/* looks like we have to traverse the member lists */
		/***************************************************/
		{
			unsigned_int trav1 = type1->simple.traversed;
			unsigned_int trav2 = type2->simple.traversed;
			type1->simple.traversed = 1;
			type2->simple.traversed = 1;
			retval = nel_member_diff (type1->s_u.members, type2->s_u.members, check_qual);
			type1->simple.traversed = trav1;
			type2->simple.traversed = trav2;
			return (retval);
		}

	case nel_D_ENUM_TAG:
	case nel_D_STRUCT_TAG:
	case nel_D_UNION_TAG:
		/***********************/
		/* should never happen */
		/***********************/
		return (nel_type_diff (type1->tag_name.descriptor,
							   type2->tag_name.descriptor, check_qual));

	case nel_D_TYPE_NAME:
	case nel_D_TYPEDEF_NAME:
		/***********************/
		/* should never happen */
		/***********************/
		return (nel_type_diff (type1->typedef_name.descriptor,
							   type2->typedef_name.descriptor, check_qual));


	case nel_D_EVENT:
		return (nel_type_diff(type1->event.descriptor,
							  type2->event.descriptor,check_qual));



	case nel_D_PRODUCTION: 
		{
			struct nel_RHS *rhs1, *rhs2;
			if( nel_symbol_diff(type1->prod.lhs, type2->prod.lhs)) {
				/* bugfix, wyong, 2005.6.10 */
				return 1;
			}

			for(rhs1=type1->prod.rhs,rhs2=type2->prod.rhs;rhs1 != NULL && rhs2 != NULL;rhs1=rhs1->next,rhs2=rhs2->next) {
				if(nel_symbol_diff(rhs1->symbol, rhs2->symbol)){
					return 1;
				}
			}

			if( rhs1 != NULL || rhs2 != NULL ) {
				/*bugfix, wyong, 2006.6.15 */
				return 1;
			}

		}
		return 0;

	default: /* simple types */
		/**********************************************/
		/* just check the type qualifiers;            */
		/* we already know the D tokens are equivlent */
		/**********************************************/
		if (check_qual) {
			if ((type1->simple._const != type2->simple._const) ||
					(type1->simple._volatile != type2->simple._volatile)) {
				return (1);
			}
		}
		return (0);

	}

	return 0;

}


/*****************************************************************************/
/* nel_type_incomplete () returns true if <type> has not been completely      */
/* specified.                                                                */
/*****************************************************************************/
unsigned_int nel_type_incomplete (register nel_type *type)
{
	register unsigned_int retval;

	if (type == NULL) {
		return (1);
	} else if (type->simple.traversed) {
		return (0);
	} else {
		type->simple.traversed = 1;
		switch (type->simple.type) {

		case nel_D_ARRAY:
			if (type->array.size <= 0) {
				retval = 1;
			} else {
				retval = nel_type_incomplete (type->array.base_type)
						 || nel_type_incomplete (type->array.index_type);
			}
			break;

		case nel_D_ENUM:
			/************************************************************/
			/* an incomplete enumed type may have (but not necessarily) */
			/* a dummy element in the consts list, with a NULL symbol.  */
			/* (the stab scanner does this, the interpreter does not)   */
			/* we allow incomplete enumed types even though they are    */
			/* illegal in ANSI C.                                       */
			/************************************************************/
			retval = ((type->enumed.consts == NULL) || ((type->enumed.consts->symbol == NULL) && (type->enumed.consts->next == NULL)));
			break;

		case nel_D_STRUCT:
		case nel_D_UNION:
			/*************************************************************/
			/* an incomplete struct/union may have (but not necessarily) */
			/* a dummy element in the members list, with a NULL symbol.  */
			/*************************************************************/
			retval = ((type->s_u.members == NULL) || ((type->s_u.members->symbol == NULL) && (type->s_u.members->next == NULL)));
			break;

		case nel_D_ENUM_TAG:
		case nel_D_STRUCT_TAG:
		case nel_D_UNION_TAG:
			retval = nel_type_incomplete (type->tag_name.descriptor);
			break;

		case nel_D_TYPEDEF_NAME:
			retval = nel_type_incomplete (type->typedef_name.descriptor);
			break;

		case nel_D_POINTER:
			/*******************************************************/
			/* it is legal to declare a pointer to an incompletely */
			/* specified type, so do not check that the deref_type */
			/* is complete.                                        */
			/*******************************************************/
			retval = 0;
			break;

		default: /* simple types are complete*/
			retval = 0;
			break;

		}
		type->simple.traversed = 0;
		return (retval);
	}
}



/*****************************************************************************/
/* nel_print_type () prints out a type descriptor in a pretty format.         */
/*****************************************************************************/
void emit_type (FILE *file, register nel_type *type/*, int indent*/)
{
	if (type == NULL) {
		//tab (indent);
		fprintf (file, "void");
	} else if (type->simple.traversed) {
		/****************************************************/
		/* check the traversed bit to keep out of an        */
		/* infinite loop when printing circular structures. */
		/****************************************************/
		//tab (indent);
		fprintf (file, "0x%p\n", type);
		//tab (indent);
		fprintf (file, "circular_type\n");
	} else {
		type->simple.traversed = 1;
		//tab (indent);
		//fprintf (file, "0x%x\n", type);
		//tab (indent);

#if 0
		//wyong, 2004.6.12
		fprintf (file, "%s", nel_TC_name (type->simple.type));
#endif

		//tab (indent);
		//fprintf (file, "size: %d\n", type->simple.size);
		//tab (indent);
		//fprintf (file, "alignment: %d\n", type->simple.alignment);
		//tab (indent);
		//fprintf (file, "_const: %d\n", type->simple._const);
		//tab (indent);
		//fprintf (file, "_volatile: %d\n", type->simple._volatile);
		switch (type->simple.type) {

#if 0
		case nel_D_ARRAY:
			tab (indent);
			fprintf (file, "base_type:\n");
			nel_print_type (file, type->array.base_type, indent + 1);
			tab (indent);
			fprintf (file, "index_type:\n");
			nel_print_type (file, type->array.index_type, indent + 1);
			tab (indent);
			fprintf (file, "known_lb: %d\n", type->array.known_lb);
			if (type->array.known_lb) {
				tab (indent);
				fprintf (file, "lb.value: %d\n", type->array.lb.value);
			} else {
				tab (indent);

#ifdef NEL_NTRP

				fprintf (file, "lb.expr:\n");
				nel_print_expr (file, type->array.lb.expr, indent + 1);
#else

				fprintf (file, "lb.symbol:\n");
				nel_print_symbol (file, type->array.lb.symbol, indent + 1);
#endif /* NEL_NTRP */

			}
			tab (indent);
			fprintf (file, "known_ub: %d\n", type->array.known_ub);
			if (type->array.known_ub) {
				tab (indent);
				fprintf (file, "ub.value: %d\n", type->array.ub.value);
			} else {
				tab (indent);

#ifdef NEL_NTRP

				fprintf (file, "ub.expr:\n");
				nel_print_expr (file, type->array.ub.expr, indent + 1);
#else

				fprintf (file, "ub.symbol:\n");
				nel_print_symbol (file, type->array.ub.symbol, indent + 1);
#endif /* NEL_NTRP */

			}
			break;

		case nel_D_ENUM:
			tab (indent);
			fprintf (file, "tag:\n");
			nel_print_symbol (file, type->enumed.tag, indent + 1);
			tab (indent);
			fprintf (file, "nconsts: %d\n", type->enumed.nconsts);
			if (! nel_print_brief_types) {
				tab (indent);
				fprintf (file, "consts:\n");
				nel_print_list (file, type->enumed.consts, indent + 1);
			} else {
				tab (indent);
				fprintf (file, "consts: 0x%x\n", type->enumed.consts);
			}
			break;

		case nel_D_FILE:
			if (! nel_print_brief_types) {
				tab (indent);
				fprintf (file, "stab_types:\n");
				nel_print_stab_types (file, type->file.stab_types, indent + 1);
				tab (indent);
				fprintf (file, "routines:\n");
				nel_print_list (file, type->file.routines, indent + 1);
				tab (indent);
				fprintf (file, "static_globals:\n");
				nel_print_list (file, type->file.static_globals, indent + 1);
				tab (indent);
				fprintf (file, "includes:\n");
				nel_print_list (file, type->file.includes, indent + 1);
				tab (indent);
				fprintf (file, "lines:\n");
				nel_print_lines (file, type->file.lines, indent + 1);
			} else {
				tab (indent);
				fprintf (file, "stab_types: 0x%x\n", type->file.stab_types);
				tab (indent);
				fprintf (file, "routines: 0x%x\n", type->file.routines);
				tab (indent);
				fprintf (file, "static_globals: 0x%x\n", type->file.static_globals);
				tab (indent);
				fprintf (file, "includes: 0x%x\n", type->file.includes);
				tab (indent);
				fprintf (file, "lines: 0x%x\n", type->file.lines);
			}
			break;

		case nel_D_FUNCTION:
			tab (indent);
			fprintf (file, "new_style: %d\n", type->function.new_style);
			tab (indent);
			fprintf (file, "var_args: %d\n", type->function.var_args);
			tab (indent);
			fprintf (file, "return_type:\n");
			nel_print_type (file, type->function.return_type, indent + 1);
			if (! nel_print_brief_types) {
				tab (indent);
				fprintf (file, "args:\n");
				nel_print_list (file, type->function.args, indent + 1);
				tab (indent);
				fprintf (file, "blocks:\n");
				nel_print_blocks (file, type->function.blocks, indent + 1);
				tab (indent);
				fprintf (file, "file:\n");
				nel_print_symbol (file, type->function.file, indent + 1);
			} else {
				tab (indent);
				fprintf (file, "args: 0x%x\n", type->function.args);
				tab (indent);
				fprintf (file, "blocks: 0x%x\n", type->function.blocks);
				tab (indent);
				fprintf (file, "file: 0x%x\n", type->function.file);
			}
			break;
#endif

		case nel_D_POINTER:
			//tab (indent);
			//fprintf (file, "deref_type:\n");
			emit_type (file, type->pointer.deref_type/*, indent + 1*/);
			fprintf(file, " *");
			break;

			/*
			          case nel_D_STAB_UNDEF:
			             tab (indent);
			             fprintf (file, "stab_type:\n");
			             nel_print_stab_type (file, type->stab_undef.stab_type, indent + 1);
			             break;
			*/

		case nel_D_STRUCT:
		case nel_D_UNION:
			//tab (indent);
			//fprintf (file, "tag:\n");
			fprintf (file, "%s ", nel_TC_name (type->simple.type));
			emit_symbol (file, type->s_u.tag /* , indent + 1*/);
			//if (! nel_print_brief_types) {
			//   tab (indent);
			//   fprintf (file, "members:\n");
			//   nel_print_members (file, type->s_u.members, indent + 1);
			//}
			//else {
			//   tab (indent);
			//   fprintf (file, "members: 0x%x\n", type->s_u.members);
			//}
			break;

		case nel_D_ENUM_TAG:
		case nel_D_STRUCT_TAG:
		case nel_D_UNION_TAG:
			//tab (indent);
			//fprintf (file, "typeriptor:\n");
			//emit_type (file, type->tag_name.typeriptor /*, indent + 1*/);
			break;

			/*
			          case nel_D_TYPE_NAME:
			          case nel_D_TYPEDEF_NAME:
			             tab (indent);
			             fprintf (file, "typeriptor:\n");
			             nel_print_type (file, type->typedef_name.typeriptor, indent + 1);
			             break;
			*/
		case nel_D_VOID:
		default:
			fprintf (file, "%s", nel_TC_name (type->simple.type));
			break;

		}
		type->simple.traversed = 0;
	}
}


/* return true if type1 and type2 are exactly the same (including qualifiers).
   - enums are not checked as gcc __builtin_types_compatible_p () */
int is_compatible_types(nel_type *type1, nel_type *type2)
{

	if (type1->simple.type == type2->simple.type) {

		switch(type1->simple.type) {
		case nel_D_POINTER:
			return is_compatible_types(type1->pointer.deref_type,
									   type2->pointer.deref_type);

			//case nel_D_STRUCT:
			//	return (type1->s_u.type == type2->s_u.type);
			//	break;

		case nel_D_FUNCTION: {
				struct nel_LIST *scan1, *scan2;
				if (!is_compatible_types(type1->function.return_type,
										 type2->function.return_type))
					return 0;

				for(scan1 = type1->function.args, scan2 = type2->function.args;
						scan1 != NULL && scan2 != NULL;
						scan1 = scan1->next, scan2 = scan2->next ) {
					if (!is_compatible_types(scan1->symbol->type,
											 scan2->symbol->type))
						return 0;
				}

				return ( scan1 != NULL || scan2 != NULL )
					   ? 0 : 1 ;
			}
		default:
			return 1;
		}
	}

	return 0;

}

//added by zhangbin, 2006-5-25
int is_asgn_compatible(struct nel_eng *eng, nel_type *type1, nel_type *type2)
{
	if(!type1 || !type2)
	{
		parser_stmt_error(eng, "NULL type");
		return 0;
	}

	//added by zhangbin, 2006-5-31
	if(type1==type2)
		return 1;
	//end
#if 0	//zhangbin, 2006-9-27
	//added by zhangbin, 2006-8-8
	if(type1->simple.type == nel_D_STAB_UNDEF && type2->simple.type != nel_D_STAB_UNDEF)
	{
		*type1 = *type2;
		return 1;
	}
	if(type2->simple.type == nel_D_STAB_UNDEF && type1->simple.type != nel_D_STAB_UNDEF)
	{
		*type2 = *type1;
		return 1;
	}
	if(type2->simple.type == nel_D_STAB_UNDEF && type1->simple.type == nel_D_STAB_UNDEF)
	{
		return 0;
	}
	//end
#endif
	//all numerical type is asignment compatible
	if(nel_numerical_D_token(type1->simple.type) && nel_numerical_D_token(type2->simple.type))
		return 1;

	if(type1->simple.type != type2->simple.type)
	{
		if(type1->simple.type == nel_D_POINTER && type2->simple.type == nel_D_ARRAY)
		{
			if(nel_type_diff(type1->pointer.deref_type, type2->array.base_type, 1))
				parser_warning(eng, "assign array to a incompatible pointer");
			return 1;
		}
		//added by zhangbin, 2006-6-2
		if(type1->simple.type == nel_D_POINTER && type2->simple.type == nel_D_FUNCTION)
		{
			if(nel_type_diff(type1->pointer.deref_type, type2, 1))
				parser_stmt_error(eng, "incompatible function and function pointer");
			return 1;
		}
		//end
		return 0;
	}
	else
	{
		if(type1->simple.type == nel_D_POINTER)
		{
			if(nel_type_diff(type1->pointer.deref_type, type2->pointer.deref_type, 1))
				parser_warning(eng, "assignment between two incompatible pointer");
			return 1;
		}
		return !nel_type_diff(type1, type2, 1);
	}
}

//added by zhangbin, 2006-7-19
//calling nel_dealloca to free line chunks,
//after doing this, all line pointer should be illegal
void line_dealloc(struct nel_eng *eng)
{
	while(nel_line_chunks)
	{
		nel_line_chunk *chunk = nel_line_chunks->next;
		nel_dealloca(nel_line_chunks->start);
		nel_dealloca(nel_line_chunks);
		nel_line_chunks = chunk;
	}
	nel_lines_next = nel_lines_end = nel_free_lines = NULL;
}
//end

//added by zhangbin, 2006-7-19
//calling nel_dealloca to free type chunks,
//after doing this, all type pointer should be illegal
void type_dealloc(struct nel_eng *eng)
{
	while(nel_type_chunks)
	{
		nel_type_chunk *chunk = nel_type_chunks->next;
		nel_dealloca(nel_type_chunks->start);
		nel_dealloca(nel_type_chunks);
		nel_type_chunks = chunk;
	}
	nel_types_next = nel_types_end = nel_free_types = NULL;
}
//end

//added by zhangbin, 2006-7-19
//calling nel_dealloca to free list chunks,
//after doing this, all list pointer should be illegal
void list_dealloc(struct nel_eng *eng)
{
	while(nel_list_chunks)
	{
		nel_list_chunk *chunk = nel_list_chunks->next;
		nel_dealloca(nel_list_chunks->start);
		nel_dealloca(nel_list_chunks);
		nel_list_chunks = chunk;
	}
	nel_lists_next = nel_lists_end = nel_free_lists = NULL;
}
//end


//added by zhangbin, 2006-7-19
//calling nel_dealloca to free block chunks,
//after doing this, all block pointer should be illegal
void block_dealloc(struct nel_eng *eng)
{
	while(nel_block_chunks)
	{
		nel_block_chunk *chunk = nel_block_chunks->next;
		nel_dealloca(nel_block_chunks->start);
		nel_dealloca(nel_block_chunks);
		nel_block_chunks = chunk;
	}
	nel_blocks_next = nel_blocks_end = nel_free_blocks = NULL;
}
//end


//added by zhangbin, 2006-7-19
//calling nel_dealloca to free member chunks,
//after doing this, all member pointer should be illegal
void member_dealloc(struct nel_eng *eng)
{
	while(nel_member_chunks)
	{
		nel_member_chunk *chunk = nel_member_chunks->next;
		nel_dealloca(nel_member_chunks->start);
		nel_dealloca(nel_member_chunks);
		nel_member_chunks = chunk;
	}
	nel_members_next = nel_members_end = nel_free_members = NULL;
}
//end
