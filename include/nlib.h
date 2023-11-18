/*
 * stream.h
 * $Id: stream.h,v 1.6 2006/12/06 07:00:59 yanf Exp $
 */
 
#ifndef __NLIB_H
#define __NLIB_H


#if 0
#define MAX_STREAM_BUF_SIZE 		4096
struct stream_state {    		
	int caller_id;			// caller 's id 
#ifdef	USE_PCRE
	int *state_buf;     
	int state_buf_size;  
#elif	USE_DFA
	int state;
#endif
	struct stream_state *next;	
};

struct stream {
	int nel_count;
	unsigned char *buf;
	int buf_len;
	int buf_size ; 
	struct stream_state *state;			
};

int nel_stream_put(struct stream *stream, char *data, int len);
struct stream *nel_stream_alloc(int size);
void nel_stream_free(struct stream *stream);

struct stream_state *alloc_stream_state(void); 
void dealloc_stream_state(struct stream_state *state); 
struct stream_state *insert_stream_state(int caller_id, struct stream_state *state); 
struct stream_state *lookup_stream_state(struct stream *stream, int caller_id); 
#endif

//wyong, 20230823
#include "./nlib/stream.h" 
#include "./nlib/shellcode.h" 
#include "./nlib/hashtab.h" 


#if 0 
#ifdef	USE_PCRE
#include "pcre.h"

#elif	USE_DFA
#include "./nlib/match/dfa.h"
struct hashtab;
#endif

#ifdef	USE_ACBM
#include "ac_bm.h"
#elif	USE_SWBM
#include "./nlib/match/swbm.h"
#endif

struct exact_match_info {
	int id;      //yanf, 2006.12.1
       	struct rule_list *rlist;
#ifdef	USE_ACBM
	struct ac_bm_tree *acbm;
#elif	USE_SWBM
	struct ptn_list *ptnlist;
#endif
	struct ret_sym **retsyms;       /* used in compile mode */
	int retnum;
};

struct match_info {
	int id;      //yanf, 2006.12.1
#ifdef	USE_PCRE
	pcre *re;
	pcre_extra *extra;
#elif	USE_DFA
        struct dfa *dfa;  
#elif	USE_RDFA
	struct ch_dfa *rdfa;
#endif
	int *retsyms;
	int retnum;
};
#endif

#include "./nlib/match.h"

#endif
