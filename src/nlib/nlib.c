

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <dlfcn.h>

#include "engine.h"
#include "nlib/shellcode.h"
#include "nlib/string.h"
#include "nlib/stream.h" 
#include "nlib/match.h"

int nel_lib_init(struct nel_eng *eng)
{
	//void *h;
	//h = dlopen("/lib/libc.so.6", RTLD_GLOBAL | RTLD_LAZY);

	stream_init(eng); //must before match_init
	nel_match_init(eng);
	shellcode_init(eng);
	string_init(eng);

	return 0;
}
