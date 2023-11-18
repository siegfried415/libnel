/*
 * stream.c
 * $Id: stream.c,v 1.13 2006/12/06 06:42:34 zhangb Exp $
 */

#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "errors.h"

//wyong, 20230810 
//#include "stream.h"
#include "nlib/stream.h"


#include "sym.h"

//added by zhangbin, 2006-7-17
#include "mem.h"
extern nel_lock_type nel_malloc_lock;
//end

struct stream_state *alloc_stream_state(void)    
{	
	struct stream_state *state;	
	state = (struct stream_state *)calloc(1, sizeof(struct stream_state));	
	if (!state) {		
		gen_error(NULL, "calloc error \n");		
		return NULL;	
	}	
	//fprintf(stderr, "alloc_stream_state: calloc pointer=%p\n", state);	
	return state;
}

void dealloc_stream_state(struct stream_state *state)
{	
	struct stream_state *pn, *ps;	
	ps = state;	
	while (ps) {		
		pn=ps->next;		

#ifdef	USE_PCRE
		if (ps->state_buf != NULL) {
			nel_free(ps->state_buf);
		}
#endif
		nel_free(ps);
		ps = pn; 	
	}	
	return;
}

struct stream_state *insert_stream_state(int caller_id, struct stream_state *state)
{	
	struct stream_state *s;	
	s = alloc_stream_state();	
	if (!s) 		
		return NULL;	
	s->caller_id = caller_id;	
	s->next = state;	
	return s;	
}

struct stream_state *lookup_stream_state(struct stream *stream, int caller_id)
{	
	struct stream_state *s;	
	s = stream->state;	
	while (s) {		
		if (s->caller_id == caller_id) {
			//printf("s->caller_id:%d   caller_id:%d\n", s->caller_id, caller_id);
			break;		
		}
		s = s->next;	
	}	
	if (!s) {		
		s = insert_stream_state(caller_id, stream->state);
		if (!s) 			
			return NULL;		
		stream->state = s; 	
	}	
	return s;
}



int nel_stream_put(struct stream *stream, char *data, int len )
{
	int copy_len = 0;
	if (!stream) {
		gen_error(NULL, "stream hasn't been allocated yet \n");
		return -1;
	}

	if (stream->buf == NULL ) {
		gen_error(NULL, "stream hasn't allocated buf\n");
		return -1;	
	}

	copy_len = (len < stream->buf_size) ? len : stream->buf_size;
	memcpy(stream->buf, data, copy_len );
	stream->buf_len = copy_len;
	
	return 0;	
}

nel_symbol *nel_stream_put_func_alloc(struct nel_eng *eng)
{
	nel_type *type;
	nel_symbol *symbol;
	nel_list *args;

	symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,"len"), nel_int_type, NULL, nel_C_FORMAL, nel_lhs_type(nel_int_type), nel_L_C, 1);
	args = nel_list_alloc(eng, 0, symbol, NULL);


	type = nel_type_alloc(eng, nel_D_POINTER, sizeof(char *), nel_alignment_of(char *), 0,0, nel_char_type );
	symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,"data"), type, NULL, nel_C_FORMAL, nel_lhs_type(type), nel_L_C, 1);
	args = nel_list_alloc(eng, 0, symbol, args);


	symbol = nel_lookup_symbol("stream", eng->nel_static_tag_hash,NULL);
	type = symbol->type->typedef_name.descriptor;
	type = nel_type_alloc(eng, nel_D_POINTER, sizeof(struct stream *), nel_alignment_of(struct stream *), 0,0,type);
	symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,"stream"), type, NULL, nel_C_FORMAL, nel_lhs_type(type), nel_L_C, 1);

	args = nel_list_alloc(eng, 0, symbol, args);


	type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 0, nel_int_type, args, NULL, NULL);

	
	symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "nel_stream_put"), type, (char *) nel_stream_put, nel_C_COMPILED_FUNCTION, nel_lhs_type(type), nel_L_C, 0);

	nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);
	return symbol;
}

void nel_stream_free(struct stream *stream)
{
	if (stream) {
		if (stream->buf) 
			nel_free(stream->buf);	
		if (stream->state)  
			dealloc_stream_state(stream->state); 
		nel_free(stream);
	}	
	return;
}

nel_symbol *nel_stream_free_func_alloc(struct nel_eng *eng)
{
	nel_type *type;
	nel_symbol *symbol;
	nel_list *args;

	symbol = nel_lookup_symbol("stream", eng->nel_static_tag_hash,NULL);
	type = symbol->type->typedef_name.descriptor;
	type = nel_type_alloc(eng, nel_D_POINTER, sizeof(struct stream *), nel_alignment_of(struct stream *), 0,0,type);
	symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,"stream"), type, NULL, nel_C_FORMAL, nel_lhs_type(type), nel_L_C, 1);
	args = nel_list_alloc(eng, 0, symbol, NULL);


	type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 0, nel_void_type, args, NULL, NULL);

	
	symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "nel_stream_free"), type, (char *) nel_stream_free, nel_C_COMPILED_FUNCTION, nel_lhs_type(type), nel_L_C, 0);

	nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);
	return symbol;
}


struct stream *nel_stream_alloc(int size)
{
	struct stream *s = NULL;	

	nel_calloc(s, 1, struct stream);
	if ( s	== NULL){
		gen_error(NULL, "nel_malloc error \n");
		return NULL;
	}

	if(size > MAX_STREAM_BUF_SIZE || size <= 0 )
		size = MAX_STREAM_BUF_SIZE;

	nel_calloc(s->buf, size, char);
	if(!s->buf){
		gen_error(NULL, "malloc error\n");
		return NULL;
	}
	
	s->buf_len = s->nel_count = 0;
	s->buf[0] = '\0'; s->buf_size = size;

	return s;

}

nel_symbol *nel_stream_alloc_func_alloc(struct nel_eng *eng)
{
	nel_type *type;
	nel_symbol *symbol;
	nel_list *args;

	//wyong, 20230809 
	//printf("nel_stream_alloc_func_alloc(10) ...\n" ); 

	symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,"size"), nel_int_type, NULL, nel_C_FORMAL, nel_lhs_type(nel_int_type), nel_L_C, 1);

	//wyong, 20230809 
	//printf("nel_stream_alloc_func_alloc(20) ...\n" ); 

	args = nel_list_alloc(eng, 0, symbol, /* args */ NULL); /* wyong, 2006.10.14 */

	//printf("nel_stream_alloc_func_alloc(30) ...\n" ); 

	symbol = nel_lookup_symbol("stream", eng->nel_static_tag_hash,NULL);

	//if ( symbol == NULL ) { 
	//	printf("nel_stream_alloc_func_alloc(40-1), symbol == NULL ...\n" ); 
	//} else {
	//	printf("nel_stream_alloc_func_alloc(40-2)...\n" ); 
	//}

	type = symbol->type->typedef_name.descriptor;

	//printf("nel_stream_alloc_func_alloc(50) ...\n" ); 
	type = nel_type_alloc(eng, nel_D_POINTER, sizeof(struct stream *), nel_alignment_of(struct stream *), 0,0,type);

	//printf("nel_stream_alloc_func_alloc(60) ...\n" ); 
	type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 0, type, args, NULL, NULL);
	//printf("nel_stream_alloc_func_alloc(70) ...\n" ); 

	symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "nel_stream_alloc"), type, (char *) nel_stream_alloc, nel_C_COMPILED_FUNCTION, nel_lhs_type(type), nel_L_C, 0);
	//printf("nel_stream_alloc_func_alloc(80) ...\n" ); 

	nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);

	//printf("nel_stream_alloc_func_alloc(90) ...\n" ); 
	return symbol;
}


int stream_init(struct nel_eng *eng)
{
	nel_symbol *symbol;

	/* nel_stream_alloc stuff */
	symbol = nel_lookup_symbol("nel_stream_alloc", eng->nel_static_ident_hash, eng->nel_static_location_hash, NULL);
	if(symbol == NULL) {
		symbol = nel_stream_alloc_func_alloc(eng);	
		//wyong, 20230809 
		//printf("stream_init, nel_stream_alloc successfully alloced !\n" ); 
	}else if(symbol->value == NULL) {
		symbol->value = (char *) nel_stream_alloc;
		//wyong, 20230809 
		//printf("stream_init, set earily inserted symbol value with nel_stream_alloc !\n" ); 

	}else if(symbol->value != (char *)nel_stream_alloc) {
		//nel_warning(eng, "the earily inserted symbol value have difference value with nel_stream_alloc !\n");
		//wyong, 20230809 
		//printf("stream_init,the earily inserted symbol value have difference value with nel_stream_alloc !\n" ); 
	}
	else {
		/* nel_stream_alloc was successfully inserted */
		//wyong, 20230809 
		//printf("stream_init,nel_stream_alloc was inserted already \n" ); 
	}


	/* nel_stream_put stuff */
	symbol = nel_lookup_symbol("nel_stream_put", eng->nel_static_ident_hash, eng->nel_static_location_hash, NULL);
	if(symbol == NULL) {
		symbol = nel_stream_put_func_alloc(eng);
	}else if(symbol->value == NULL) {
		symbol->value = (char *) nel_stream_put;

	}else if(symbol->value != (char *)nel_stream_put) {
		//nel_warning(eng, "the earily inserted symbol value have difference value with nel_stream_put!\n");
	}
	else {
		/* nel_stream_put was successfully inserted */
	}

	//wyong, 20230809 
	//printf("stream_init,nel_stream_put was successfully inserted \n" ); 

	/* nel_stream_free stuff */
	symbol = nel_lookup_symbol("nel_stream_free", eng->nel_static_ident_hash, eng->nel_static_location_hash, NULL);
	if(symbol == NULL) {
		symbol  = nel_stream_free_func_alloc(eng);
	}else if(symbol->value == NULL) {
		symbol->value = (char *) nel_stream_free;

	}else if(symbol->value != (char *)nel_stream_free) {
		//printf("the earily inserted symbol value have difference value with nel_stream_free!\n");

	}
	else {
		//printf( "nel_stream_free was successfully inserted !\n" );
	}

	//wyong, 20230809 
	//printf("stream_init,nel_stream_free was successfully inserted \n" ); 

	return 0;

}

