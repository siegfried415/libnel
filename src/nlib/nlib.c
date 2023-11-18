

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//wyong, 20230803 
//#include <dlfcn.h>

//#include "type.h"
#include "engine.h"
//#include "sym.h"

int nel_lib_init(struct nel_eng *eng)
{
	//printf("nel_lib_init begin...\n");

	//wyong, 20230803 
	//void *h;
	//h = dlopen("/lib/libc.so.6", RTLD_GLOBAL | RTLD_LAZY);

	//printf("nel_lib_init before add_symbol_id_init ...\n");
	add_symbol_id_init(eng);
	//printf("nel_lib_init before stream_init ...\n");
	stream_init(eng); //must before match_init
	//printf("nel_lib_init before nel_match_init ...\n");
	nel_match_init(eng);
	//printf("nel_lib_init before shellcode_init ...\n");
	shellcode_init(eng);

	//wyong, 20231025 
	string_init(eng);

	//printf_init(eng);

	//printf("nel_lib_init finished ...\n");
	return 0;
}
