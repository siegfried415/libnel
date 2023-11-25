
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "type.h"
#include "sym.h"
#include "nlib/string.h" 


int nel_strcmp(char *data1, char *data2)
{
	if(data1 == NULL || data2 == NULL )
		return -1;
	return strcmp(data1, data2);
}

char *nel_strstr(char *data1, char *data2 )
{
	if(data1 == NULL || data2 == NULL ) {
		return NULL;
	}
	return strstr(data1, data2);
}

char *nel_strchr(char *s, char c)
{
	return  strchr(s, c );
}


int string_init(struct nel_eng *eng)
{
	nel_type *type;
	nel_symbol *symbol;
	nel_list *args;

	/* 
	 * nel_strcmp 
	 */

	symbol = nel_lookup_symbol("nel_strcmp", eng->nel_static_ident_hash, eng->nel_static_location_hash,  NULL);
	if(symbol == NULL) {
		type = nel_type_alloc(eng, nel_D_POINTER, sizeof(char *), nel_alignment_of(char *), 0, 0, nel_char_type);
		symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,"data1"), type, NULL, nel_C_FORMAL, nel_lhs_type(type), nel_L_C, 1);
		args = nel_list_alloc(eng, 0, symbol, NULL);

		symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,"data2"), type, NULL, nel_C_FORMAL, nel_lhs_type(type), nel_L_C, 1);
		args = nel_list_alloc(eng, 0, symbol, args);

		
		/* return type */
		type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 0, nel_int_type, args, NULL, NULL);
		
		/* create symbol for function 'nel_has_shellcode' */
		symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "nel_strcmp"), type, (char *) nel_strcmp, nel_C_COMPILED_FUNCTION, nel_lhs_type(type), nel_L_C, 0);

		nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);
		
	}else if(symbol->value == NULL) {
		symbol->value = (char *) nel_strcmp;

	}else if(symbol->value != (char *)nel_strcmp) {
		fprintf(stderr, "the earily inserted symbol value have difference value with nel_strcmp!\n");

	}
	else {
		/* strstr was successfully inserted */
	}

	/*
	 * nel_strstr
	 */

	symbol = nel_lookup_symbol("nel_strstr", eng->nel_static_ident_hash, eng->nel_static_location_hash,  NULL);
	if(symbol == NULL) {
		type = nel_type_alloc(eng, nel_D_POINTER, sizeof(void *), nel_alignment_of(char *), 0, 0, nel_char_type);
		symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,"data1"), type, NULL, nel_C_FORMAL, nel_lhs_type(type), nel_L_C, 1);
		args = nel_list_alloc(eng, 0, symbol, NULL);


		symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,"data2"), type, NULL, nel_C_FORMAL, nel_lhs_type(type), nel_L_C, 1);
		args = nel_list_alloc(eng, 0, symbol, NULL);


		type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 0, type, args, NULL, NULL);
		
		symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "nel_strstr"), type, (char *) nel_strstr, nel_C_COMPILED_FUNCTION, nel_lhs_type(type), nel_L_C, 0);

		nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);

	}else if(symbol->value == NULL) {
		symbol->value = (char *) nel_strstr;

	}else if(symbol->value != (char *)nel_strstr) {
		fprintf(stderr, "the earily inserted symbol value have difference value with nel_strstr!\n");
	}else {
		/* strstr was successfully inserted */
	}

	/*
	 * nel_strchr
	 */
	symbol = nel_lookup_symbol("nel_strchr", eng->nel_static_ident_hash, eng->nel_static_location_hash,  NULL);
	if(symbol == NULL) {
		//type = nel_type_alloc(eng, nel_D_POINTER, sizeof(void *), nel_alignment_of(char *), 0, 0, nel_char_type);
		//symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,"s"), type, NULL, nel_C_FORMAL, nel_lhs_type(type), nel_L_C, 1);
		//args = nel_list_alloc(eng, 0, symbol, NULL);

		//symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,"c"), nel_char_type, NULL, nel_C_FORMAL, nel_lhs_type(nel_char_type), nel_L_C, 1);
		//args = nel_list_alloc(eng, 0, symbol, args);

		// c
		symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,"c"), nel_char_type, NULL, nel_C_FORMAL, nel_lhs_type(nel_char_type), nel_L_C, 1);
		args = nel_list_alloc(eng, 0, symbol, NULL);
		

		// s 
		type = nel_type_alloc(eng, nel_D_POINTER, sizeof(void *), nel_alignment_of(char *), 0, 0, nel_char_type);
		symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,"s"), type, NULL, nel_C_FORMAL, nel_lhs_type(type), nel_L_C, 1);
		args = nel_list_alloc(eng, 0, symbol, args);


		/* return type */
		type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 0, type, args, NULL, NULL);
		
		/* create symbol for function 'nel_has_shellcode' */
		symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "nel_strchr"), type, (char *) nel_strchr, nel_C_COMPILED_FUNCTION, nel_lhs_type(type), nel_L_C, 0);


		nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);

	}else if(symbol->value == NULL) {
		symbol->value = (char *) nel_strchr;

	}else if(symbol->value != (char *)nel_strchr) {
		fprintf(stderr, "the earily inserted symbol value have difference value with nel_strchr!\n");
	}else {
		/* strchr was successfully inserted */
	}


	return 0;

}

