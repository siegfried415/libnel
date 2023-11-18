/*
 * match.c
 * $Id: match.c,v 1.15 2006/12/07 08:47:32 zhangb Exp $
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "engine.h"
#include "errors.h"
#include "expr.h"
#include "stmt.h"
#include "opt.h"

//wyong, 20230823
#include "nlib/match/conv_char.h"
#include "nlib/match/dfa.h" 
#include "nlib/match.h"
#include "nlib/stream.h"
#include "nlib/hashtab.h"


#include "mem.h"


#define SETMIN(a,b) if ( (a) > (b) ) { (a) = (b); }
#define SETMAX(a,b) if ( (a) < (b) ) { (a) = (b); }


#include "intlist.h"
#ifdef	USE_ACBM
#include "ac_bm.h"
#elif	USE_SWBM
#include "nlib/match/swbm.h"
#endif

extern nel_lock_type nel_malloc_lock;
int stream_match(struct stream *stream, struct match_info *ms);

/* for 'main()' */
/*
#define TEXTSIZE		1379
#define LINESIZE		2048	
#define DEBUG_STREAM		0
*/

#ifdef	USE_PCRE
#define OVECCOUNT 30    /* should be a multiple of 3 */
#define PCRE_PATTERN_SIZE (65535*2)   
#define PCRE_DFA_WORKSPACE_SIZE 1000 * 10 /* change from 1000 -> 1000 * 10 
					     bugfix for liuj, 
					     wyong, 2006.8.30 */
#endif

struct match_info *match_init(unsigned char **, int , int );
int delete_backslash(struct rule_item *ri)
{
        char *pat, *newpat;
        int i, j, len, newlen, num, is_plain_char;

        num = is_plain_char = 0;
        len = ri->pat_len;
        pat = ri->pat;

        for (i=0; i<len; i++) {
                if (pat[i] == '\\') {
                        if (is_plain_char) {
                                num++;
                                is_plain_char = 0;
                        } else {
                                is_plain_char = 1;
                        }
                } else {
                        if (is_plain_char) {
                                num++;
                                is_plain_char = 0;
                        }
                }
        }
        newlen = len - num;
        //modified by zhangbin, 2006-7-17, malloc=>nel_malloc
#if 1
        nel_malloc(newpat, newlen, char);
#else
        newpat = (char *)malloc(newlen * sizeof(char));
#endif
        //end, 2006-7-17
        if (!newpat) {
                gen_error(NULL, "malloc error \n");
                return -1;
        }
        j = 0;
        for (i=0; i<len; i++) {
                if (pat[i] == '\\') {
                        if (is_plain_char == 1) {
                                newpat[j++] = '\\';
                                is_plain_char = 0;
                        } else {
                                is_plain_char = 1;
                        }
                } else {
                        newpat[j++] = pat[i];
                        if (is_plain_char) is_plain_char = 0;
                }
        }
        if (j > newlen) {
                gen_error(NULL, "memory overflow \n");
                return -1;
        }

        nel_free(ri->pat);      

        ri->pat = newpat;
        ri->pat_len = newlen;

        return 0;
}


void free_retsym_list(struct ret_sym **retsyms, int retnum)
{
        int i;
        for (i=0; i<retnum; i++) {
                free(retsyms[i]);
        }
        free(retsyms);
        return;
}


struct ret_sym *new_retsym(int id)
{
        struct ret_sym *rs;
        rs = (struct ret_sym *)calloc(1, sizeof(struct ret_sym));
        if (!rs) {
                fprintf(stderr, "calloc() error in 'new_retsym()'.\n");
                return NULL;
        }
        rs->symbol = id;
        return rs;
}

struct ret_sym **new_retsym_list(int retnum, int_list_t *id_list)
{
        struct ret_sym **retsyms;
        int_list_t *pid;
        int i;
        retsyms = (struct ret_sym **)malloc(retnum * sizeof(struct ret_sym *));
        if (!retsyms) {
                fprintf(stderr, "malloc() error in 'new_retsym_list()'.\n");
                return NULL;
        }
        pid = id_list;
        for (i=0; i<retnum; i++) {
                retsyms[i] = new_retsym(pid->val);
                if (!retsyms[i]) {
                        free_retsym_list(retsyms, retnum);
                        return NULL;
                }
                pid = pid->next;
        }
        return retsyms;
}

struct ret_sym **init_retsyms(struct rule_list *rlist, int *retnum)
{
        struct ret_sym **retsyms;
        int_list_t *pid, *id_list;
        int i;

        id_list = dup_int_list(rlist->rules[0]->id_list);
        if (!id_list)
                return NULL;

        for (i=1; i<rlist->rule_num; i++) {
                pid = rlist->rules[i]->id_list;
                while (pid) {
                        if (check_int_list(id_list, pid->val) != 0) {
                                if (insert_int_list(&id_list, pid->val) == -1) {
                                        free_int_list(id_list);
                                        return NULL;
                                }
                        }
                        pid = pid->next;
                }
        }

        pid = id_list;
        *retnum = 0;
        while (pid) {
                ++(*retnum);
                pid = pid->next;
        }

        retsyms = new_retsym_list(*retnum, id_list);
        if (!retsyms) {
                free_int_list(id_list);
                return NULL;
        }

        free_int_list(id_list);
        return retsyms;
}

void match_info_free(struct match_info *ms)
{
	if(ms) {
#ifdef	USE_PCRE
		nel_free(ms->re);	
		nel_free(ms->extra);	
#elif	USE_DFA
                dfafree(ms->dfa);
#endif
		nel_free(ms->retsyms);	
		nel_free(ms);		
	}
}

struct exact_match_info *exact_match_info_alloc (
			struct rule_list *rlist, 
#ifdef	USE_ACBM
			struct ac_bm_tree *acbm, 
#elif	USE_SWBM
				struct ptn_list *ptnlist, 
#endif
			struct ret_sym **retsyms, 
			int retnum)
{
        struct exact_match_info *ms;
        static int id=	3;	//MIN_MS_ID;
        ms = (struct exact_match_info *)calloc(1, sizeof(struct match_info));
        if (!ms) {
                fprintf(stderr, "calloc() error in 'new_match_info'.\n");
                return NULL;
        }
        ms->id = id++;
        ms->rlist = rlist;

#ifdef	USE_ACBM
        ms->acbm = acbm;
#elif	USE_SWBM
        ms->ptnlist = ptnlist;
#endif
        ms->retsyms = retsyms;
        ms->retnum = retnum;
        return ms;
}


void exact_match_info_free(struct exact_match_info *ms)
{
        if (!ms) return;

        free_rule_list(ms->rlist);

#ifdef	USE_ACBM
        if (ms->acbm)
                free_acbm_tree(ms->acbm);
#elif	USE_SWBM
        free_ptn_list(ms->ptnlist);
#endif

        free_retsym_list(ms->retsyms, ms->retnum);
        free(ms);
        return;
}

struct match_info *match_info_alloc( 
#ifdef	USE_PCRE
				pcre *re, 
			pcre_extra *extra, 
#elif	USE_DFA
			struct dfa *dfa,
#endif
		int *retsyms, 
		int retnum, 
		struct hashtab *offset_table)
{
	static unsigned int id = 1;    
	struct match_info *ms = NULL;
	
	nel_malloc(ms, 1, struct match_info);
	if(ms){
		ms->id = id;    
		ms->retsyms = retsyms;
		ms->retnum = retnum;
#ifdef	USE_PCRE
		ms->re = re;
		ms->extra = extra;
#elif	USE_DFA
			ms->dfa  = dfa;	
#endif
	}

	id++; 
	return ms;	
}


static struct rule_list *array_to_rlist(unsigned char **pattern_array, 
				int pattern_array_size) 
{
        struct rule_list *rlist;
        int i;

        if(pattern_array == NULL || pattern_array_size == 0){
                return NULL;
        }

        rlist = new_rule_list(pattern_array_size);
        if (!rlist)
                return NULL;

        for(i = 0; i< pattern_array_size; i++) {
                int_list_t *pid = NULL;
                insert_int_list(&pid, i);
                rlist->rules[i] = new_rule_item(pid, 
						pattern_array[i], 
					strlen(pattern_array[i]), 
						0, 
						-1, 
						0, //CASE_SENS, 
					CHARSET_UNICODE);

                if (!rlist->rules[i]) {
                        free_rule_list(rlist);
                        return NULL;
                }
        }

        rlist->min_start = 0;
        rlist->max_end = -1;

	return rlist;

}



static int array_to_pattern(unsigned char **pattern_array, 
				int pattern_array_size, 
				char *pattern, 
				int *pattern_len, 
				int pattern_size ,
				struct hashtab *offset_table)
{
	struct hashtab *table;
	struct exp_node *exp;	
	int retval=0;	
	int i, offset = 0;

#ifdef	USE_PCRE
	/* wondering why DFA needn't do the following,it 's said that  
	Lazy DFA do this automatically,is that true ?  wyong, 20090512 */
	offset += sprintf(pattern + offset, ".*(");
#endif

	for(i=0; i < pattern_array_size ; i++) {	
		Tab_entry *hp;
		nel_expr *expr;		// 
		char *pat ;		// sub pattern 

		if( ( pat = pattern_array[i] ) == NULL ) {
			//NOTE,NOTE,NOTE, need add error handing code
			return -1;
		}

#ifdef	USE_PCRE
		offset += sprintf(pattern+offset, (i==0)?"%s":"|%s", pat);
		offset += sprintf(pattern + offset, "(?C1)");
		hashinsert(offset, offset_table, &hp);
		hp->val = i;
#elif	USE_DFA
		
                offset += sprintf(pattern+offset, (i==0)?"(%s)":"|(%s)", pat);
		
		//wyong, 20090605
                offset += sprintf(pattern+offset, "%c%d", '\\',i + 1 );

		//hashinsert(start, offset_table, &hp);
		//hp->val = i;
#endif
                retval++;
		if( offset > pattern_size  - 1 ) {
			//NOTE,NOTE,NOTE, do error handing 
			return -1;
		}

	}

#ifdef	USE_PCRE
	offset += sprintf(pattern + offset, ")");
#endif

	*pattern_len = offset;
	return retval;

}

#ifdef	USE_PCRE
static int pcre_callout_func(pcre_callout_block *cb)
{
	struct match_info *ms;
	if (cb != NULL && 
	(ms=(struct match_info *)(cb->callout_data)) != NULL ) {
		int offset = cb->pattern_position;
		Tab_entry *hp = hashfind( HASHER(offset), offset, ms->offset_table); 
		if(hp && VALID(hp->te_key)) {
			ms->retsyms[hp->val] = 1;
		}
	}
	return 0;
}
#endif


int do_buf_exact_match(unsigned char *text, int len, struct exact_match_info *ms)
{
        int ret;
        int resume=0; //must be zero

        //printf("Yeah! we are in frag_match, text=%s, len=%d, ms=%p\n", text, len, ms);
        if(text == NULL || len==0 || ms == NULL) {
                ret = -1;
        }

#ifdef	USE_ACBM
	/* xiayu 2005.11.5 will support charset in the future
	   (otherwise should get the stream's charset here) */
	ret = acbm_match(	text, 
				len, 
					&resume, /*&s->state*/
					ms->acbm, 
				ms->rlist,
				ms->retnum,
				ms->retsyms,
				CHARSET_MODE_UNICODE);
	//printf("after acbm_match, ret = %d\n", ret);

#elif	USE_SWBM
	ret = swbm_match(text, 
				len, 
				ms->ptnlist, //->search_suf_tree,/*ms*/ 
					ms->rlist,
				ms->retnum, 
				ms->retsyms);
	//printf("frag_match: after swbm_match, ret=%d\n", ret);
#endif

        return ret;

}

int do_buf_match(unsigned char *text, int len, struct match_info *ms)
{
	int rc;
#ifdef	USE_PCRE
	int ovector[OVECCOUNT];
#elif 	USE_DFA
        int backref;
        int tmpstate = 0;
#endif
	if( text == NULL || ms == NULL || ms->retsyms == NULL) {
		return 0;
	}
	
#ifdef	USE_PCRE
	//memset(ms->retsyms, 0, ms->retnum);/* wyong, 2006.7.6 */
	rc = pcre_exec(
		ms->re,   	/* the compiled pattern */
		ms->extra,	/* extra data - we didn't study the pattern */
		text,	/* the subject string */
		len,       /* the length of the subject */
		0, 	/* start at offset 0 in the subject */
		0, 	/* default options */
		ovector, 	/* output vector for substring information */
		OVECCOUNT);  /* number of elements in the output vector */
#elif	USE_DFA
        rc = dfaexec(ms->dfa, 
			text, 
			len, 
			&backref, 
			&tmpstate);   // 0 is buf_match

	/* wyong, 20090607 */	
	if (rc >= 0 ) {
		position_set *ps = (position_set *)backref; 
		if (ps != NULL){
			int rindex, ruleid;
			for(rindex = 0; rindex < ps->nelem ; rindex++) {
				/* 'ps->elems[rindex].index - 1' is ruleid of matched */
				ruleid = ps->elems[rindex].index - 1;
				if (ruleid >= 0  && ruleid < ms->retnum){ 
					ms->retsyms[ ruleid ] = 1;
				}
			}
		}	
	}

	else 
#endif

	/* Matching failed: handle error cases */
	if (rc < 0) {
#ifdef	USE_PCRE
		switch(rc) {
			case PCRE_ERROR_NOMATCH: 
				//printf("No match\n"); 
				break;
			default: 
				/* Handle other special cases if you like */
				//printf("Matching error %d\n", rc); 
				break;
		}
#endif

		return 0;
	}

	/* Match succeded */
	return 1;

}

int do_stream_exact_match(struct stream *stream, struct exact_match_info *ms)
{
        struct stream_state *ss;
        int ret;

        if(stream == NULL || ms == NULL) {
                return -1;
        }

        //printf("stream_match: stream->buf=%s, ms=%p\n", stream->buf, ms);
        ss = lookup_stream_state(stream, ms->id);
        if (!ss)
                return -1;

#ifdef	USE_ACBM
	/* xiayu 2005.11.5 will support charset in the future
	   (otherwise should get the stream's charset here) */
	ret = acbm_match(stream->buf, 
				stream->buf_len, 
				&ss->state, 
				ms->acbm, //ms, 
					ms->rlist,
					ms->retnum,
					ms->retsyms,
				CHARSET_MODE_UNICODE);
	//printf("after acbm_match, ret = %d\n", ret);
#elif	USE_SWBM
	//#error "SWBM can't support stream match"	
	ret = swbm_match(stream->buf, 
				stream->buf_len, 
				ms->ptnlist, //->search_suf_tree,/*ms*/ 
					ms->rlist,
				ms->retnum, 
				ms->retsyms);
	//printf("frag_match: after swbm_match, ret=%d\n", ret);
#endif

        return ret;

}

int do_stream_match(struct stream *stream, struct match_info *ms)
{

	int rc;
        struct stream_state *ss;
#ifdef	USE_PCRE
	int ovector[OVECCOUNT];
	int first = 0;
#elif	USE_DFA
        int backref;
#endif
	
	if(stream == NULL || stream->buf == NULL || ms == NULL) {
		return 0;//wyong, 2006.7.6 
	}

	/* get stream_state of this ms from stream by ms->id */
	if ((ss = lookup_stream_state(stream, ms->id)) == NULL ) {
		return -1;
	}

#ifdef	USE_PCRE	
	if ( ss->state_buf == NULL ) {
		nel_calloc(ss->state_buf, PCRE_DFA_WORKSPACE_SIZE, int);
		if (ss->state_buf == NULL)
		{
		  return -1;
		}
		first = 1;
	}

	
	//memset(ms->retsyms, 0, ms->retnum);/* wyong, 2006.7.6 */
	rc = pcre_dfa_exec(
		ms->re,   	/* the compiled pattern */
		ms->extra,	/* extra data - we didn't study the pattern */
		stream->buf,	/* the subject string */
		stream->buf_len,       /* the length of the subject */
		0, 	/* start at offset 0 in the subject */
		(first) ? PCRE_PARTIAL : 	/* default options */
			  PCRE_PARTIAL | PCRE_DFA_RESTART, 
		ovector, 	/* output vector for substring information */
		OVECCOUNT,  /* number of elements in the output vector */
		ss->state_buf,
		PCRE_DFA_WORKSPACE_SIZE);	
#elif	USE_DFA
	rc = dfaexec(	ms->dfa, 
			stream->buf, 
			stream->buf_len, 
			&backref, 
			&ss->state);

	/* wyong, 20090607 */	
	if (rc >= 0 ) {
		position_set *ps = (position_set *)backref; 
		if (ps != NULL){
			int rindex, ruleid;
			for(rindex = 0; rindex < ps->nelem ; rindex++) {
				/* 'ps->elems[rindex].index - 1' is ruleid of matched */
				ruleid = ps->elems[rindex].index - 1;
				if (ruleid >= 0  && ruleid < ms->retnum){ 
					ms->retsyms[ ruleid ] = 1;
				}
			}
		}	
	}

	else 
#endif	

	/* Matching failed: handle error cases */
	if (rc < 0) {
#ifdef	USE_PCRE
		switch(rc) {
			case PCRE_ERROR_NOMATCH: 
				//printf("No match\n"); 
				break;
			case PCRE_ERROR_PARTIAL:
				//printf("Partial matched\n"); 
				break;
			default: 
				/* Handle other special cases if you like */
				//printf("Matching error %d\n", rc); 
				break;
		}
		/* Release memory used for the compiled pattern */
		//free(re);     
#endif

		return 0;
	}

	/* Match succeded */
	return 1;
}

int is_exact_match(struct exact_match_info *ms, int id)
{
        int i;

        if(ms == NULL) {
                /* wyong, 2005.5.30 */
                return 0;
        }

        for(i=0; i< ms->retnum; i++  ){
                if(ms->retsyms[i]->symbol == id && ms->retsyms[i]->val == 1 ){
                        ms->retsyms[i]->val = 0;
                        return 1;
                }
        }
        return 0;
}


static nel_symbol *is_exact_match_func_init(struct nel_eng *eng)
{	
	nel_type *type;
	nel_symbol *symbol;
	nel_list *args;

	symbol = nel_lookup_symbol("is_exact_match", 
					eng->nel_static_ident_hash, 
			eng->nel_static_location_hash,  NULL);
	if( symbol == NULL) {

		symbol = nel_static_symbol_alloc(eng, 
					nel_insert_name(eng,"id"), 
				nel_int_type, NULL, nel_C_FORMAL, 
				nel_lhs_type(nel_int_type), nel_L_C, 1);
		args = nel_list_alloc(eng, 0, symbol, NULL);

		symbol = nel_lookup_symbol("exact_match_info", 
					eng->nel_static_tag_hash,NULL);
		type = symbol->type->typedef_name.descriptor;
		type = nel_type_alloc(eng, nel_D_POINTER, 
				sizeof(struct exact_match_info *), 
				nel_alignment_of(struct exact_match_info *), 
					0,0,type);
		symbol = nel_static_symbol_alloc(eng, 
					nel_insert_name(eng,"ms"), 
					type, NULL, nel_C_FORMAL, 
					nel_lhs_type(type), nel_L_C, 1);
		args = nel_list_alloc(eng, 0, symbol, args);


		type = nel_type_alloc (eng, nel_D_FUNCTION, 
					0, 0, 0, 0, 0, 0, nel_int_type, 
					args, NULL, NULL);

		
		symbol = nel_static_symbol_alloc (eng, 
					nel_insert_name(eng, "is_exact_match"), 
					type, (char *) is_exact_match, 
					nel_C_COMPILED_FUNCTION,
					nel_lhs_type(type), nel_L_C, 0);

		nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);

	}else if( symbol->value == NULL) {
		symbol->value = (char *) is_exact_match;
	}else if( symbol->value != (char *)is_exact_match) {
		//printf(eng, "the earily inserted symbol have difference"
		//" value with is_exact_match!\n");
	}
	else {
		/* is_exact_match was successfully inserted */
	}
	
	return symbol;

}


int is_match(struct match_info *ms, int id)
{
	int retval = 0;
	if( ms != NULL) {
		retval = ms->retsyms[id];
		ms->retsyms[id]=0;
	}
	return retval;
}


static nel_symbol *is_match_func_init(struct nel_eng *eng)
{	
	nel_type *type;
	nel_symbol *symbol;
	nel_list *args;

	//printf("is_match_func_init(10)\n");
	symbol = nel_lookup_symbol("is_match", eng->nel_static_ident_hash, 
		eng->nel_static_location_hash,  NULL);
	if( symbol == NULL) {

		//printf("is_match_func_init(20)\n");
		symbol = nel_static_symbol_alloc(eng, 
					nel_insert_name(eng,"id"), 
				nel_int_type, NULL, nel_C_FORMAL, 
				nel_lhs_type(nel_int_type), nel_L_C, 1);
		//printf("is_match_func_init(30)\n");
		args = nel_list_alloc(eng, 0, symbol, NULL);

		//printf("is_match_func_init(40)\n");
		symbol = nel_lookup_symbol("match_info", 
					eng->nel_static_tag_hash,NULL);
		//printf("is_match_func_init(50)\n");
		type = symbol->type->typedef_name.descriptor;
		//printf("is_match_func_init(60)\n");
		type = nel_type_alloc(eng, nel_D_POINTER, 
					sizeof(struct match_info *), 
					nel_alignment_of(struct match_info *), 
					0,0,type);
		//printf("is_match_func_init(70)\n");
		symbol = nel_static_symbol_alloc(eng, 
					nel_insert_name(eng,"ms"), 
					type, NULL, nel_C_FORMAL, 
					nel_lhs_type(type), nel_L_C, 1);
		//printf("is_match_func_init(80)\n");
		args = nel_list_alloc(eng, 0, symbol, args);


		//printf("is_match_func_init(90)\n");
		type = nel_type_alloc (eng, nel_D_FUNCTION, 
					0, 0, 0, 0, 0, 0, nel_int_type, 
					args, NULL, NULL);

		
		//printf("is_match_func_init(100)\n");
		symbol = nel_static_symbol_alloc (eng, 
					nel_insert_name(eng, "is_match"), 
					type, (char *) is_match, 
					nel_C_COMPILED_FUNCTION,
					nel_lhs_type(type), nel_L_C, 0);

		//printf("is_match_func_init(110)\n");
		nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);

	}else if( symbol->value == NULL) {
		//printf("is_match_func_init(120)\n");
		symbol->value = (char *) is_match;
	}else if( symbol->value != (char *)is_match) {
		//printf("is_match_func_init(130)\n");
		//printf(eng, "the earily inserted symbol have difference"
		//" value with is_match!\n");
	}
	else {
		//printf("is_match_func_init(140)\n");
		/* is_match was successfully inserted */
	}
	
	//printf("is_match_func_init(150)\n");
	return symbol;

}


static nel_expr *clus_post_exact_match(struct nel_eng *eng, struct prior_node *pnode,int id)
{
	nel_expr *func, *fun_call;
	nel_expr *expr;
	nel_type *type;
	char *value;
	nel_symbol *symbol;
	nel_expr_list *ms_expr_list, *Symbol_expr_list;

	func = pnode->sbj;
	value = nel_static_value_alloc(eng, sizeof(int), nel_alignment_of(int));
	symbol =  nel_static_symbol_alloc(eng, "id", nel_int_type,value, 
							nel_C_CONST,0,0,0);
	*((int *)symbol->value) = id; 
	expr = nel_expr_alloc(eng, nel_O_SYMBOL, symbol);
	Symbol_expr_list = nel_expr_list_alloc(eng, expr, NULL);	


	symbol = nel_lookup_symbol("exact_match_info", eng->nel_static_tag_hash,
							NULL);
	type = symbol->type->typedef_name.descriptor;
	type = nel_type_alloc(eng, nel_D_POINTER, 
				sizeof(struct exact_match_info *), 
				nel_alignment_of(struct exact_match_info *),
				0,0,type);
	value = nel_static_value_alloc(eng, sizeof(struct exact_match_info *), 
				nel_alignment_of(struct exact_match_info *));


	symbol = nel_static_symbol_alloc(eng, "ms", type, value , nel_C_CONST, 0, 0,0);
	*((struct exact_match_info **)symbol->value) = (struct exact_match_info *)pnode->data;
	expr = nel_expr_alloc(eng, nel_O_SYMBOL, symbol);


	ms_expr_list  = nel_expr_list_alloc(eng, expr, Symbol_expr_list);	
	symbol = nel_lookup_symbol("is_match", eng->nel_static_ident_hash, 
					eng->nel_static_location_hash,  NULL);
	func = nel_expr_alloc(eng, nel_O_SYMBOL, symbol); 
	fun_call = nel_expr_alloc(eng, nel_O_FUNCALL, func, 2, ms_expr_list);

	return fun_call;

}


static nel_expr *clus_post_match(struct nel_eng *eng, struct prior_node *pnode,int id)
{
	nel_expr *func, *fun_call;
	nel_expr *expr;
	nel_type *type;
	char *value;
	nel_symbol *symbol;
	nel_expr_list *ms_expr_list, *Symbol_expr_list;

	func = pnode->sbj;
	value = nel_static_value_alloc(eng, sizeof(int), nel_alignment_of(int));
	symbol =  nel_static_symbol_alloc(eng, "id", nel_int_type,value, 
							nel_C_CONST,0,0,0);
	*((int *)symbol->value) = id; 
	expr = nel_expr_alloc(eng, nel_O_SYMBOL, symbol);
	Symbol_expr_list = nel_expr_list_alloc(eng, expr, NULL);	


	symbol = nel_lookup_symbol("match_info", eng->nel_static_tag_hash,
							NULL);
	type = symbol->type->typedef_name.descriptor;
	type = nel_type_alloc(eng, nel_D_POINTER, sizeof(struct match_info *), 
					nel_alignment_of(struct match_info *),
					0,0,type);
	value = nel_static_value_alloc(eng, sizeof(struct match_info *), 
					nel_alignment_of(struct match_info *));


	symbol = nel_static_symbol_alloc(eng, "ms", type, value , nel_C_CONST, 0, 0,0);
	*((struct match_info **)symbol->value) = (struct match_info *)pnode->data;
	expr = nel_expr_alloc(eng, nel_O_SYMBOL, symbol);


	ms_expr_list  = nel_expr_list_alloc(eng, expr, Symbol_expr_list);	
	symbol = nel_lookup_symbol("is_match", eng->nel_static_ident_hash, 
					eng->nel_static_location_hash,  NULL);
	func = nel_expr_alloc(eng, nel_O_SYMBOL, symbol); 
	fun_call = nel_expr_alloc(eng, nel_O_FUNCALL, func, 2, ms_expr_list);

	return fun_call;
}


int is_no_match(struct match_info *ms, int id)
{
	int retval = 0;
	if( ms != NULL) {
		retval = ms->retsyms[id];
		ms->retsyms[id]=0;
	}
	return !retval;
}


static nel_symbol *is_no_match_func_init(struct nel_eng *eng)
{	
	nel_type *type;
	nel_symbol *symbol;
	nel_list *args;

	symbol = nel_lookup_symbol("is_no_match", eng->nel_static_ident_hash, 
		eng->nel_static_location_hash,  NULL);
	if( symbol == NULL) {

		symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,"id"), 
						nel_int_type, NULL, nel_C_FORMAL, 
						nel_lhs_type(nel_int_type), 
						nel_L_C, 1);
		args = nel_list_alloc(eng, 0, symbol, NULL);

		symbol = nel_lookup_symbol("match_info", eng->nel_static_tag_hash,
									NULL);
		type = symbol->type->typedef_name.descriptor;
		type = nel_type_alloc(eng, nel_D_POINTER, 
					sizeof(struct match_info *), 
					nel_alignment_of(struct match_info *), 
					0,0,type);
		symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,"ms"), 
					type, NULL, nel_C_FORMAL, nel_lhs_type(type), 
					nel_L_C, 1);
		args = nel_list_alloc(eng, 0, symbol, args);


		type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 0, 
					nel_int_type, args, NULL, NULL);

		
		symbol = nel_static_symbol_alloc (eng, 
						nel_insert_name(eng, "is_no_match"), 
						type, (char *) is_no_match, 
						nel_C_COMPILED_FUNCTION, 
						nel_lhs_type(type), nel_L_C, 0);

		nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);

	}else if( symbol->value == NULL) {
		symbol->value = (char *) is_no_match;
	}else if( symbol->value != (char *)is_no_match) {
		//printf(eng, "the earily inserted symbol have difference "
		//" value with is_match!\n");
	}
	else {
		/* is_no_match was successfully inserted */
	}
	
	return symbol;

}


static nel_expr *clus_post_no_match(struct nel_eng *eng, 
					struct prior_node *pnode,
					int id)
{
	nel_expr *func, *fun_call;
	nel_expr *expr;
	nel_type *type;
	char *value;
	nel_symbol *symbol;
	nel_expr_list *ms_expr_list, 
			*Symbol_expr_list;

	//func = pnode->sbj;
	value = nel_static_value_alloc(eng, sizeof(int), nel_alignment_of(int));
	symbol =  nel_static_symbol_alloc(eng, "id", nel_int_type,value, 
							nel_C_CONST,0,0,0);
	*((int *)symbol->value) = id; 
	expr = nel_expr_alloc(eng, nel_O_SYMBOL, symbol);
	Symbol_expr_list = nel_expr_list_alloc(eng, expr, NULL);	


	symbol = nel_lookup_symbol("match_info", eng->nel_static_tag_hash,NULL);
	type = symbol->type->typedef_name.descriptor;
	type = nel_type_alloc(eng, nel_D_POINTER, sizeof(struct match_info *), 
				nel_alignment_of(struct match_info *), 0,0,type);
	value = nel_static_value_alloc(eng, sizeof(struct match_info *), 
					nel_alignment_of(struct match_info *));


	symbol = nel_static_symbol_alloc(eng, "ms", type, value , nel_C_CONST, 0, 0,0);
	*((struct match_info **)symbol->value) = (struct match_info *)pnode->data;
	expr = nel_expr_alloc(eng, nel_O_SYMBOL, symbol);


	ms_expr_list  = nel_expr_list_alloc(eng, expr, Symbol_expr_list);	
	symbol = nel_lookup_symbol("is_no_match", eng->nel_static_ident_hash, 
					eng->nel_static_location_hash,  NULL);
	func = nel_expr_alloc(eng, nel_O_SYMBOL, symbol); 
	fun_call = nel_expr_alloc(eng, nel_O_FUNCALL, func, 2, ms_expr_list);

	return fun_call;
}


#if 0
static nel_symbol *clus_post_no_match_func_init(struct nel_eng *eng)
{
	nel_symbol *symbol;

	symbol = nel_lookup_symbol("clus_post_no_match", eng->nel_static_ident_hash, 
		eng->nel_static_location_hash, NULL);
	if(symbol == NULL ) {
		symbol = nel_static_symbol_alloc (eng, 
					nel_insert_name(eng, "clus_post_no_match"), 
					NULL, (char *) clus_post_no_match, 
					nel_C_COMPILED_FUNCTION, 0, nel_L_C, 0);
		nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);
	}
	return symbol;
}
#endif


static nel_expr *stream_exact_match_funcall_alloc(struct nel_eng *eng, 
						struct prior_node *pnode, 
						struct exact_match_info *ms)
{
	nel_expr *func, *fun_call;
	nel_expr *expr;
	nel_type *type;
	char *value;
	nel_symbol *symbol;
	nel_expr_list *data_expr_list, 
			*ms_expr_list;

	symbol = nel_lookup_symbol("stream_exact_match", 
						eng->nel_static_ident_hash, 
					eng->nel_static_location_hash, NULL);
	func = nel_expr_alloc(eng, nel_O_SYMBOL, symbol);

	//bugfix, wyong,2005.6.2 
	symbol = nel_lookup_symbol("exact_match_info", 
				eng->nel_static_tag_hash,NULL);	
	type = symbol->type->typedef_name.descriptor;

	type = nel_type_alloc(eng, nel_D_POINTER, 
				sizeof(struct exact_match_info *), 
			nel_alignment_of(struct exact_match_info *), 
						0,0,type);
	value = nel_static_value_alloc(eng, sizeof(struct exact_match_info *), 
				nel_alignment_of(struct exact_match_info *));


	symbol = nel_static_symbol_alloc(eng, "ms", type, value, nel_C_CONST, 0, 0,0);
	*((struct exact_match_info **)symbol->value) = ms;
	expr = nel_expr_alloc(eng, nel_O_SYMBOL, symbol);


	ms_expr_list  = nel_expr_list_alloc(eng, expr, NULL);	

#if 1
	data_expr_list  = nel_expr_list_alloc(eng, pnode->varlist[0], ms_expr_list);
#else
	//NOTE,NOTE,NOTE, need check this. wyong, 20090516
	data_expr_list  = nel_expr_list_alloc(eng, pnode->exp->opr.arglist[0], ms_expr_list);
#endif
	fun_call = nel_expr_alloc(eng, nel_O_FUNCALL, func, 2, data_expr_list);


	return fun_call;
}


static nel_expr *stream_match_funcall_alloc(	struct nel_eng *eng, 
						struct prior_node *pnode, 
						struct match_info *ms)
{
	nel_expr *func, *fun_call;
	nel_expr *expr;
	nel_type *type;
	char *value;
	nel_symbol *symbol;
	nel_expr_list *data_expr_list, 
			*ms_expr_list;

	symbol = nel_lookup_symbol("stream_match", eng->nel_static_ident_hash, 
					eng->nel_static_location_hash, NULL);
	func = nel_expr_alloc(eng, nel_O_SYMBOL, symbol);

	//bugfix, wyong,2005.6.2 
	symbol = nel_lookup_symbol("match_info", eng->nel_static_tag_hash,NULL);	
	type = symbol->type->typedef_name.descriptor;

	type = nel_type_alloc(eng, nel_D_POINTER, sizeof(struct match_info *), 
			nel_alignment_of(struct match_info *), 0,0,type);
	value = nel_static_value_alloc(eng, sizeof(struct match_info *), 
					nel_alignment_of(struct match_info *));


	symbol = nel_static_symbol_alloc(eng, "ms", type, value, nel_C_CONST, 0, 0,0);
	*((struct match_info **)symbol->value) = ms;
	expr = nel_expr_alloc(eng, nel_O_SYMBOL, symbol);


	ms_expr_list  = nel_expr_list_alloc(eng, expr, NULL);	

#if 1
	data_expr_list  = nel_expr_list_alloc(eng, pnode->varlist[0], ms_expr_list);
#else
	//NOTE,NOTE,NOTE, need check this. wyong, 20090516
	data_expr_list  = nel_expr_list_alloc(eng, pnode->exp->opr.arglist[0], ms_expr_list);
#endif
	fun_call = nel_expr_alloc(eng, nel_O_FUNCALL, func, 2, data_expr_list);


	return fun_call;
}


static nel_expr *buf_no_match_funcall_alloc(	struct nel_eng *eng, 
						struct prior_node *pnode, 
						struct match_info *ms)
{
	nel_expr *func, *fun_call;
	nel_expr *expr;
	nel_type *type;
	char *value;
	nel_symbol *symbol;
	nel_expr_list *data_expr_list, 
			*len_expr_list, 
			*ms_expr_list;


	symbol = nel_lookup_symbol("buf_no_match", eng->nel_static_ident_hash, 
					eng->nel_static_location_hash, NULL);
	func = nel_expr_alloc(eng, nel_O_SYMBOL, symbol);

	symbol = nel_lookup_symbol("match_info", eng->nel_static_tag_hash,NULL);
	type = symbol->type->typedef_name.descriptor;

	type = nel_type_alloc(eng, nel_D_POINTER, sizeof(struct match_info *), 
				nel_alignment_of(struct match_info *), 0,0,type);
	value = nel_static_value_alloc(eng, sizeof(struct match_info *), 
					nel_alignment_of(struct match_info *));


	symbol = nel_static_symbol_alloc(eng, "ms", type, value, nel_C_CONST, 0, 0,0);
	*((struct match_info **)symbol->value) = ms;
	expr = nel_expr_alloc(eng, nel_O_SYMBOL, symbol);


	ms_expr_list  = nel_expr_list_alloc(eng, expr, NULL );	
	len_expr_list = nel_expr_list_alloc(eng, pnode->varlist[1], ms_expr_list );
	data_expr_list  = nel_expr_list_alloc(eng, pnode->varlist[0], len_expr_list);
	fun_call = nel_expr_alloc(eng, nel_O_FUNCALL, func, 3, data_expr_list);

	return fun_call;
}

static nel_expr *buf_exact_match_funcall_alloc(	struct nel_eng *eng, 
						struct prior_node *pnode, 
						struct exact_match_info *ms)
{
	nel_expr *func, *fun_call;
	nel_expr *expr;
	nel_type *type;
	char *value;
	nel_symbol *symbol;
	nel_expr_list *data_expr_list, 
			*len_expr_list, 
			*ms_expr_list;

	symbol = nel_lookup_symbol("buf_exact_match", 
						eng->nel_static_ident_hash, 
					eng->nel_static_location_hash, NULL);
	func = nel_expr_alloc(eng, nel_O_SYMBOL, symbol);

	symbol = nel_lookup_symbol("exact_match_info", 
					eng->nel_static_tag_hash,NULL);

	type = symbol->type->typedef_name.descriptor;

	type = nel_type_alloc(eng, nel_D_POINTER, 
				sizeof(struct exact_match_info *), 
				nel_alignment_of(struct exact_match_info *), 
							0,0,type);
	value = nel_static_value_alloc(eng, sizeof(struct exact_match_info *), 
				nel_alignment_of(struct exact_match_info *));


	symbol = nel_static_symbol_alloc(eng, "ms", type, value, nel_C_CONST, 0, 0,0);
	*((struct exact_match_info **)symbol->value) = ms;
	expr = nel_expr_alloc(eng, nel_O_SYMBOL, symbol);


	ms_expr_list  = nel_expr_list_alloc(eng, expr, NULL );	
	len_expr_list = nel_expr_list_alloc(eng, pnode->varlist[1], ms_expr_list );
	data_expr_list  = nel_expr_list_alloc(eng, pnode->varlist[0], len_expr_list);
	fun_call = nel_expr_alloc(eng, nel_O_FUNCALL, func, 3, data_expr_list);

	return fun_call;
}

static nel_expr *buf_match_funcall_alloc(	struct nel_eng *eng, 
						struct prior_node *pnode, 
						struct match_info *ms)
{
	nel_expr *func, *fun_call;
	nel_expr *expr;
	nel_type *type;
	char *value;
	nel_symbol *symbol;
	nel_expr_list *data_expr_list, 
			*len_expr_list, 
			*ms_expr_list;

	symbol = nel_lookup_symbol("buf_match", eng->nel_static_ident_hash, 
					eng->nel_static_location_hash, NULL);
	func = nel_expr_alloc(eng, nel_O_SYMBOL, symbol);

	symbol = nel_lookup_symbol("match_info", eng->nel_static_tag_hash,NULL);
	type = symbol->type->typedef_name.descriptor;

	type = nel_type_alloc(eng, nel_D_POINTER, sizeof(struct match_info *), 
				nel_alignment_of(struct match_info *), 0,0,type);
	value = nel_static_value_alloc(eng, sizeof(struct match_info *), 
					nel_alignment_of(struct match_info *));


	symbol = nel_static_symbol_alloc(eng, "ms", type, value, nel_C_CONST, 0, 0,0);
	*((struct match_info **)symbol->value) = ms;
	expr = nel_expr_alloc(eng, nel_O_SYMBOL, symbol);


	ms_expr_list  = nel_expr_list_alloc(eng, expr, NULL );	
	len_expr_list = nel_expr_list_alloc(eng, pnode->varlist[1], ms_expr_list );
	data_expr_list  = nel_expr_list_alloc(eng, pnode->varlist[0], len_expr_list);
	fun_call = nel_expr_alloc(eng, nel_O_FUNCALL, func, 3, data_expr_list);

	return fun_call;
}

static struct rule_list *pnode_to_rlist(struct prior_node *pnode)
{
	struct rule_list *rlist;
        int_list_t *pid;
        char *pat;
        int pat_len;
        int i;

        struct exp_node *exp;
        nel_expr *expr;
        nel_expr *fun_call;

        rlist = new_rule_list(pnode->numcount);
        if (!rlist)
                return NULL;

        for(i=0, exp=pnode->exp; exp; i++,exp=exp->next){
                pid = NULL;

                /* NOTE, NOTE, NOTE, I wonder */
                //insert_int_list(&pid, exp->id_list->val);
                pid = dup_int_list(exp->id_list);
                if (!pid) {
                        free_rule_list(rlist);
                        return NULL;
                }

                /* pattern */
                expr = exp->opr.arglist[0];
                pat = (char *)expr->symbol.symbol->value;
                if (pat == NULL) {
                        return NULL;
		}

                pat_len = strlen(pat);
                rlist->rules[i] = new_rule_item(pid, 
						pat, 
						pat_len, 
						0, 
						-1, 
						0, //CASE_SENS, 
					CHARSET_UNICODE);

                if (!rlist->rules[i]) {
                        free_int_list(pid);
                        free_rule_list(rlist);
                        return NULL;
                }
        }

        rlist->min_start = 0;
        rlist->max_end = -1;

	return rlist;

}


static int pnode_to_pattern(struct prior_node *pnode, 
				char *pattern, 
				int *pattern_len,
				int pattern_size, 
				struct hashtab *offset_table)
{
	struct hashtab *table;
	struct exp_node *exp;	
	int retval = 0;	
	int i, offset = 0;

#ifdef	USE_PCRE
	offset += sprintf(pattern + offset, ".*(");
#endif

	for(i=0, exp=pnode->exp; exp; i++,exp=exp->next) {	
		int_list_t *pid;	// pid list 
		nel_expr *expr;		// 
		char *pat;		// sub pattern 

		if((pid = exp->id_list) == NULL){
			continue;
		}
		
		expr = exp->opr.arglist[0];	
		if((pat=(char *)expr->symbol.symbol->value) == NULL) {
			//NOTE,NOTE,NOTE, need add error handing code
			return -1;
		}

		if( offset+strlen(pat)+2 > pattern_size - 1 ) {
			//NOTE,NOTE,NOTE, do error handing 
			return -1;
		}
#ifdef	USE_PCRE
		offset += sprintf(pattern+offset, (i==0)?"%s":"|%s", pat);
#elif	USE_DFA
		offset += sprintf(pattern+offset, 
				//(i==0)?"(%s)!%d":"|(%s)!%d", pat, i+1);
				(i==0)?"(%s)":"|(%s)", pat);
#endif

		for(;pid; pid = pid->next) {
#ifdef	USE_PCRE
			Tab_entry *hp;
			// add (?Cj) at the end of pattern 
		
			if( offset+6 > pattern_size - 1 ) {
				//NOTE,NOTE,NOTE, do error handing 
				return -1;
			}

			offset += sprintf(pattern + offset, "(?C1)");
			hashinsert(offset, offset_table, &hp);
			hp->val = pid->val;

#elif	USE_DFA
			//Tab_entry *hp; // add (!) at the end of pattern 
			//int start = offset;

			if( offset+6 > pattern_size - 1 ) {
				//NOTE,NOTE,NOTE, do error handing 
				return -1;
			}

			//wyong, 20090605
			offset += sprintf(pattern + offset, "%c%d", '\\',pid->val + 1);

			//hashinsert(/*offset*/ start, offset_table, &hp);
			//hp->val = pid->val;
#endif
			//retval++;
			//bugfix, wyong, 2006.7.11 
			if( pid->val >= retval)
				retval = pid->val + 1 ;
		}

		if( offset > pattern_size - 1 ) {
			//NOTE,NOTE,NOTE, do error handing 
			return -1;
		}

	}

#ifdef	USE_PCRE
	offset += sprintf(pattern + offset, ")");
#endif

	*pattern_len = offset;
	return retval;

}

/* Generated by re2c 0.5 on Wed May 13 05:08:31 2009 */
//#define NULL            ((char*) 0)
int get_flag_from_name(		char *name, 
				int *stream_flag, 
				int *nocase_flag, 
				int *match_flag )
{
char *p = name;
char *q;
#define YYCTYPE         unsigned char
#define YYCURSOR        p
#define YYLIMIT         p
#define YYMARKER        q
#define YYFILL(n)

	if(name == NULL) {
		return -1;
	}else {
{
	YYCTYPE yych;
	unsigned int yyaccept;
	goto yy0;
yy1:	++YYCURSOR;
yy0:
	if((YYLIMIT - YYCURSOR) < 41) YYFILL(41);
	yych = *YYCURSOR;
	if(yych != 'c')	goto yy4;
yy2:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych == 'l')	goto yy5;
yy3:

	{
			return -1;

		}
yy4:	yych = *++YYCURSOR;
	goto yy3;
yy5:	yych = *++YYCURSOR;
	if(yych == 'u')	goto yy7;
yy6:	YYCURSOR = YYMARKER;
	switch(yyaccept){
	case 0:	goto yy3;
	}
yy7:	yych = *++YYCURSOR;
	if(yych != 's')	goto yy6;
yy8:	yych = *++YYCURSOR;
	if(yych != '_')	goto yy6;
yy9:	yych = *++YYCURSOR;
	if(yych == 'b')	goto yy11;
	if(yych != 's')	goto yy6;
yy10:	yych = *++YYCURSOR;
	if(yych == 't')	goto yy143;
	goto yy6;
yy11:	yych = *++YYCURSOR;
	if(yych != 'u')	goto yy6;
yy12:	yych = *++YYCURSOR;
	if(yych != 'f')	goto yy6;
yy13:	yych = *++YYCURSOR;
	if(yych != '_')	goto yy6;
yy14:	yych = *++YYCURSOR;
	if(yych <= 'd'){
		if(yych <= 'b')	goto yy6;
		if(yych <= 'c')	goto yy16;
	} else {
		if(yych == 'n')	goto yy17;
		goto yy6;
	}
yy15:	yych = *++YYCURSOR;
	if(yych == 'o')	goto yy58;
	goto yy6;
yy16:	yych = *++YYCURSOR;
	if(yych == 'a')	goto yy39;
	goto yy6;
yy17:	yych = *++YYCURSOR;
	if(yych != 'o')	goto yy6;
yy18:	yych = *++YYCURSOR;
	if(yych != 'c')	goto yy6;
yy19:	yych = *++YYCURSOR;
	if(yych != 'a')	goto yy6;
yy20:	yych = *++YYCURSOR;
	if(yych != 's')	goto yy6;
yy21:	yych = *++YYCURSOR;
	if(yych != 'e')	goto yy6;
yy22:	yych = *++YYCURSOR;
	if(yych != '_')	goto yy6;
yy23:	yych = *++YYCURSOR;
	if(yych <= 'l')	goto yy6;
	if(yych <= 'm')	goto yy24;
	if(yych <= 'n')	goto yy25;
	goto yy6;
yy24:	yych = *++YYCURSOR;
	if(yych == 'a')	goto yy34;
	goto yy6;
yy25:	yych = *++YYCURSOR;
	if(yych != 'o')	goto yy6;
yy26:	yych = *++YYCURSOR;
	if(yych != '_')	goto yy6;
yy27:	yych = *++YYCURSOR;
	if(yych != 'm')	goto yy6;
yy28:	yych = *++YYCURSOR;
	if(yych != 'a')	goto yy6;
yy29:	yych = *++YYCURSOR;
	if(yych != 't')	goto yy6;
yy30:	yych = *++YYCURSOR;
	if(yych != 'c')	goto yy6;
yy31:	yych = *++YYCURSOR;
	if(yych != 'h')	goto yy6;
yy32:	yych = *++YYCURSOR;
yy33:

	{
			*stream_flag = 0; 
			*nocase_flag = 1; 
			*match_flag  = 0; 
			return 0;
		}
yy34:	yych = *++YYCURSOR;
	if(yych != 't')	goto yy6;
yy35:	yych = *++YYCURSOR;
	if(yych != 'c')	goto yy6;
yy36:	yych = *++YYCURSOR;
	if(yych != 'h')	goto yy6;
yy37:	yych = *++YYCURSOR;
yy38:

	{
			*stream_flag = 0; 
			*nocase_flag = 1; 
			*match_flag  = 1; 
			return 0;
		}
yy39:	yych = *++YYCURSOR;
	if(yych != 's')	goto yy6;
yy40:	yych = *++YYCURSOR;
	if(yych != 'e')	goto yy6;
yy41:	yych = *++YYCURSOR;
	if(yych != '_')	goto yy6;
yy42:	yych = *++YYCURSOR;
	if(yych <= 'l')	goto yy6;
	if(yych <= 'm')	goto yy43;
	if(yych <= 'n')	goto yy44;
	goto yy6;
yy43:	yych = *++YYCURSOR;
	if(yych == 'a')	goto yy53;
	goto yy6;
yy44:	yych = *++YYCURSOR;
	if(yych != 'o')	goto yy6;
yy45:	yych = *++YYCURSOR;
	if(yych != '_')	goto yy6;
yy46:	yych = *++YYCURSOR;
	if(yych != 'm')	goto yy6;
yy47:	yych = *++YYCURSOR;
	if(yych != 'a')	goto yy6;
yy48:	yych = *++YYCURSOR;
	if(yych != 't')	goto yy6;
yy49:	yych = *++YYCURSOR;
	if(yych != 'c')	goto yy6;
yy50:	yych = *++YYCURSOR;
	if(yych != 'h')	goto yy6;
yy51:	yych = *++YYCURSOR;
yy52:

	{
			*stream_flag = 0; 
			*nocase_flag = 0; 
			*match_flag  = 0; 
			return 0;
		}
yy53:	yych = *++YYCURSOR;
	if(yych != 't')	goto yy6;
yy54:	yych = *++YYCURSOR;
	if(yych != 'c')	goto yy6;
yy55:	yych = *++YYCURSOR;
	if(yych != 'h')	goto yy6;
yy56:	yych = *++YYCURSOR;
yy57:

	{
			*stream_flag = 0; 
			*nocase_flag = 0;
			*match_flag = 1;
			return 0;
		}
yy58:	yych = *++YYCURSOR;
	if(yych != 't')	goto yy6;
yy59:	yych = *++YYCURSOR;
	if(yych != 'a')	goto yy6;
yy60:	yych = *++YYCURSOR;
	if(yych != 'l')	goto yy6;
yy61:	yych = *++YYCURSOR;
	if(yych != 'l')	goto yy6;
yy62:	yych = *++YYCURSOR;
	if(yych != '_')	goto yy6;
yy63:	yych = *++YYCURSOR;
	if(yych <= 'l')	goto yy6;
	if(yych <= 'm')	goto yy64;
	if(yych <= 'n')	goto yy65;
	goto yy6;
yy64:	yych = *++YYCURSOR;
	if(yych == 'a')	goto yy95;
	if(yych == 'u')	goto yy94;
	goto yy6;
yy65:	yych = *++YYCURSOR;
	if(yych != 'o')	goto yy6;
yy66:	yych = *++YYCURSOR;
	if(yych == '_')	goto yy67;
	if(yych == 'c')	goto yy68;
	goto yy6;
yy67:	yych = *++YYCURSOR;
	if(yych == 'm')	goto yy88;
	goto yy6;
yy68:	yych = *++YYCURSOR;
	if(yych != 'a')	goto yy6;
yy69:	yych = *++YYCURSOR;
	if(yych != 's')	goto yy6;
yy70:	yych = *++YYCURSOR;
	if(yych != 'e')	goto yy6;
yy71:	yych = *++YYCURSOR;
	if(yych != '_')	goto yy6;
yy72:	yych = *++YYCURSOR;
	if(yych <= 'l')	goto yy6;
	if(yych <= 'm')	goto yy74;
	if(yych >= 'o')	goto yy6;
yy73:	yych = *++YYCURSOR;
	if(yych == 'o')	goto yy80;
	goto yy6;
yy74:	yych = *++YYCURSOR;
	if(yych != 'a')	goto yy6;
yy75:	yych = *++YYCURSOR;
	if(yych != 't')	goto yy6;
yy76:	yych = *++YYCURSOR;
	if(yych != 'c')	goto yy6;
yy77:	yych = *++YYCURSOR;
	if(yych != 'h')	goto yy6;
yy78:	yych = *++YYCURSOR;
yy79:

	{
			*stream_flag = 0; 
			*nocase_flag = 5;
			*match_flag = 1;
			return 0;
		}
yy80:	yych = *++YYCURSOR;
	if(yych != '_')	goto yy6;
yy81:	yych = *++YYCURSOR;
	if(yych != 'm')	goto yy6;
yy82:	yych = *++YYCURSOR;
	if(yych != 'a')	goto yy6;
yy83:	yych = *++YYCURSOR;
	if(yych != 't')	goto yy6;
yy84:	yych = *++YYCURSOR;
	if(yych != 'c')	goto yy6;
yy85:	yych = *++YYCURSOR;
	if(yych != 'h')	goto yy6;
yy86:	yych = *++YYCURSOR;
yy87:

	{
			*stream_flag = 0; 
			*nocase_flag = 5;
			*match_flag = 0;
			return 0;
		}
yy88:	yych = *++YYCURSOR;
	if(yych != 'a')	goto yy6;
yy89:	yych = *++YYCURSOR;
	if(yych != 't')	goto yy6;
yy90:	yych = *++YYCURSOR;
	if(yych != 'c')	goto yy6;
yy91:	yych = *++YYCURSOR;
	if(yych != 'h')	goto yy6;
yy92:	yych = *++YYCURSOR;
yy93:

	{
			*stream_flag = 0; 
			*nocase_flag = 4;
			*match_flag = 0;
			return 0;
		}
yy94:	yych = *++YYCURSOR;
	if(yych == 'l')	goto yy100;
	goto yy6;
yy95:	yych = *++YYCURSOR;
	if(yych != 't')	goto yy6;
yy96:	yych = *++YYCURSOR;
	if(yych != 'c')	goto yy6;
yy97:	yych = *++YYCURSOR;
	if(yych != 'h')	goto yy6;
yy98:	yych = *++YYCURSOR;
yy99:

	{
			*stream_flag = 0; 
			*nocase_flag = 4;
			*match_flag = 1;
			return 0;
		}
yy100:	yych = *++YYCURSOR;
	if(yych != 't')	goto yy6;
yy101:	yych = *++YYCURSOR;
	if(yych != 'i')	goto yy6;
yy102:	yych = *++YYCURSOR;
	if(yych != 'l')	goto yy6;
yy103:	yych = *++YYCURSOR;
	if(yych != 'i')	goto yy6;
yy104:	yych = *++YYCURSOR;
	if(yych != 'n')	goto yy6;
yy105:	yych = *++YYCURSOR;
	if(yych != 'e')	goto yy6;
yy106:	yych = *++YYCURSOR;
	if(yych != '_')	goto yy6;
yy107:	yych = *++YYCURSOR;
	if(yych <= 'l')	goto yy6;
	if(yych <= 'm')	goto yy108;
	if(yych <= 'n')	goto yy109;
	goto yy6;
yy108:	yych = *++YYCURSOR;
	if(yych == 'a')	goto yy138;
	goto yy6;
yy109:	yych = *++YYCURSOR;
	if(yych != 'o')	goto yy6;
yy110:	yych = *++YYCURSOR;
	if(yych == '_')	goto yy111;
	if(yych == 'c')	goto yy112;
	goto yy6;
yy111:	yych = *++YYCURSOR;
	if(yych == 'm')	goto yy132;
	goto yy6;
yy112:	yych = *++YYCURSOR;
	if(yych != 'a')	goto yy6;
yy113:	yych = *++YYCURSOR;
	if(yych != 's')	goto yy6;
yy114:	yych = *++YYCURSOR;
	if(yych != 'e')	goto yy6;
yy115:	yych = *++YYCURSOR;
	if(yych != '_')	goto yy6;
yy116:	yych = *++YYCURSOR;
	if(yych <= 'l')	goto yy6;
	if(yych <= 'm')	goto yy118;
	if(yych >= 'o')	goto yy6;
yy117:	yych = *++YYCURSOR;
	if(yych == 'o')	goto yy124;
	goto yy6;
yy118:	yych = *++YYCURSOR;
	if(yych != 'a')	goto yy6;
yy119:	yych = *++YYCURSOR;
	if(yych != 't')	goto yy6;
yy120:	yych = *++YYCURSOR;
	if(yych != 'c')	goto yy6;
yy121:	yych = *++YYCURSOR;
	if(yych != 'h')	goto yy6;
yy122:	yych = *++YYCURSOR;
yy123:

	{
			*stream_flag = 0; 
			*nocase_flag = 7;
			*match_flag = 1;
			return 0;
		}
yy124:	yych = *++YYCURSOR;
	if(yych != '_')	goto yy6;
yy125:	yych = *++YYCURSOR;
	if(yych != 'm')	goto yy6;
yy126:	yych = *++YYCURSOR;
	if(yych != 'a')	goto yy6;
yy127:	yych = *++YYCURSOR;
	if(yych != 't')	goto yy6;
yy128:	yych = *++YYCURSOR;
	if(yych != 'c')	goto yy6;
yy129:	yych = *++YYCURSOR;
	if(yych != 'h')	goto yy6;
yy130:	yych = *++YYCURSOR;
yy131:

	{
			*stream_flag = 0; 
			*nocase_flag = 7;
			*match_flag = 0;
			return 0;
		}
yy132:	yych = *++YYCURSOR;
	if(yych != 'a')	goto yy6;
yy133:	yych = *++YYCURSOR;
	if(yych != 't')	goto yy6;
yy134:	yych = *++YYCURSOR;
	if(yych != 'c')	goto yy6;
yy135:	yych = *++YYCURSOR;
	if(yych != 'h')	goto yy6;
yy136:	yych = *++YYCURSOR;
yy137:

	{
			*stream_flag = 0; 
			*nocase_flag = 6;
			*match_flag = 0;
			return 0;
		}
yy138:	yych = *++YYCURSOR;
	if(yych != 't')	goto yy6;
yy139:	yych = *++YYCURSOR;
	if(yych != 'c')	goto yy6;
yy140:	yych = *++YYCURSOR;
	if(yych != 'h')	goto yy6;
yy141:	yych = *++YYCURSOR;
yy142:

	{
			*stream_flag = 0; 
			*nocase_flag = 6;
			*match_flag = 1;
			return 0;
		}
yy143:	yych = *++YYCURSOR;
	if(yych != 'r')	goto yy6;
yy144:	yych = *++YYCURSOR;
	if(yych != 'e')	goto yy6;
yy145:	yych = *++YYCURSOR;
	if(yych != 'a')	goto yy6;
yy146:	yych = *++YYCURSOR;
	if(yych != 'm')	goto yy6;
yy147:	yych = *++YYCURSOR;
	if(yych != '_')	goto yy6;
yy148:	yych = *++YYCURSOR;
	if(yych <= 'd'){
		if(yych <= 'b')	goto yy6;
		if(yych <= 'c')	goto yy150;
		goto yy151;
	} else {
		if(yych != 'n')	goto yy6;
	}
yy149:	yych = *++YYCURSOR;
	if(yych == 'o')	goto yy204;
	goto yy6;
yy150:	yych = *++YYCURSOR;
	if(yych == 'a')	goto yy194;
	goto yy6;
yy151:	yych = *++YYCURSOR;
	if(yych != 'o')	goto yy6;
yy152:	yych = *++YYCURSOR;
	if(yych != 't')	goto yy6;
yy153:	yych = *++YYCURSOR;
	if(yych != 'a')	goto yy6;
yy154:	yych = *++YYCURSOR;
	if(yych != 'l')	goto yy6;
yy155:	yych = *++YYCURSOR;
	if(yych != 'l')	goto yy6;
yy156:	yych = *++YYCURSOR;
	if(yych != '_')	goto yy6;
yy157:	yych = *++YYCURSOR;
	if(yych <= 'l')	goto yy6;
	if(yych <= 'm')	goto yy158;
	if(yych <= 'n')	goto yy159;
	goto yy6;
yy158:	yych = *++YYCURSOR;
	if(yych == 'u')	goto yy172;
	goto yy6;
yy159:	yych = *++YYCURSOR;
	if(yych != 'o')	goto yy6;
yy160:	yych = *++YYCURSOR;
	if(yych != 'c')	goto yy6;
yy161:	yych = *++YYCURSOR;
	if(yych != 'a')	goto yy6;
yy162:	yych = *++YYCURSOR;
	if(yych != 's')	goto yy6;
yy163:	yych = *++YYCURSOR;
	if(yych != 'e')	goto yy6;
yy164:	yych = *++YYCURSOR;
	if(yych != '_')	goto yy6;
yy165:	yych = *++YYCURSOR;
	if(yych != 'm')	goto yy6;
yy166:	yych = *++YYCURSOR;
	if(yych != 'a')	goto yy6;
yy167:	yych = *++YYCURSOR;
	if(yych != 't')	goto yy6;
yy168:	yych = *++YYCURSOR;
	if(yych != 'c')	goto yy6;
yy169:	yych = *++YYCURSOR;
	if(yych != 'h')	goto yy6;
yy170:	yych = *++YYCURSOR;
yy171:

	{
			*stream_flag = 1; 
			*nocase_flag = 5; 
			*match_flag  = 1; 
			return 0;
		}
yy172:	yych = *++YYCURSOR;
	if(yych != 'l')	goto yy6;
yy173:	yych = *++YYCURSOR;
	if(yych != 't')	goto yy6;
yy174:	yych = *++YYCURSOR;
	if(yych != 'i')	goto yy6;
yy175:	yych = *++YYCURSOR;
	if(yych != 'l')	goto yy6;
yy176:	yych = *++YYCURSOR;
	if(yych != 'i')	goto yy6;
yy177:	yych = *++YYCURSOR;
	if(yych != 'n')	goto yy6;
yy178:	yych = *++YYCURSOR;
	if(yych != 'e')	goto yy6;
yy179:	yych = *++YYCURSOR;
	if(yych != '_')	goto yy6;
yy180:	yych = *++YYCURSOR;
	if(yych != 'n')	goto yy6;
yy181:	yych = *++YYCURSOR;
	if(yych != 'o')	goto yy6;
yy182:	yych = *++YYCURSOR;
	if(yych != 'c')	goto yy6;
yy183:	yych = *++YYCURSOR;
	if(yych != 'a')	goto yy6;
yy184:	yych = *++YYCURSOR;
	if(yych != 's')	goto yy6;
yy185:	yych = *++YYCURSOR;
	if(yych != 'e')	goto yy6;
yy186:	yych = *++YYCURSOR;
	if(yych != '_')	goto yy6;
yy187:	yych = *++YYCURSOR;
	if(yych != 'm')	goto yy6;
yy188:	yych = *++YYCURSOR;
	if(yych != 'a')	goto yy6;
yy189:	yych = *++YYCURSOR;
	if(yych != 't')	goto yy6;
yy190:	yych = *++YYCURSOR;
	if(yych != 'c')	goto yy6;
yy191:	yych = *++YYCURSOR;
	if(yych != 'h')	goto yy6;
yy192:	yych = *++YYCURSOR;
yy193:

	{
			*stream_flag = 1; 
			*nocase_flag = 7; 
			*match_flag  = 1; 
			return 0;
		}
yy194:	yych = *++YYCURSOR;
	if(yych != 's')	goto yy6;
yy195:	yych = *++YYCURSOR;
	if(yych != 'e')	goto yy6;
yy196:	yych = *++YYCURSOR;
	if(yych != '_')	goto yy6;
yy197:	yych = *++YYCURSOR;
	if(yych != 'm')	goto yy6;
yy198:	yych = *++YYCURSOR;
	if(yych != 'a')	goto yy6;
yy199:	yych = *++YYCURSOR;
	if(yych != 't')	goto yy6;
yy200:	yych = *++YYCURSOR;
	if(yych != 'c')	goto yy6;
yy201:	yych = *++YYCURSOR;
	if(yych != 'h')	goto yy6;
yy202:	yych = *++YYCURSOR;
yy203:

	{
			*stream_flag = 1; 
			*nocase_flag = 0;
			*match_flag = 1;
			return 0;
		}
yy204:	yych = *++YYCURSOR;
	if(yych != 'c')	goto yy6;
yy205:	yych = *++YYCURSOR;
	if(yych != 'a')	goto yy6;
yy206:	yych = *++YYCURSOR;
	if(yych != 's')	goto yy6;
yy207:	yych = *++YYCURSOR;
	if(yych != 'e')	goto yy6;
yy208:	yych = *++YYCURSOR;
	if(yych != '_')	goto yy6;
yy209:	yych = *++YYCURSOR;
	if(yych != 'm')	goto yy6;
yy210:	yych = *++YYCURSOR;
	if(yych != 'a')	goto yy6;
yy211:	yych = *++YYCURSOR;
	if(yych != 't')	goto yy6;
yy212:	yych = *++YYCURSOR;
	if(yych != 'c')	goto yy6;
yy213:	yych = *++YYCURSOR;
	if(yych != 'h')	goto yy6;
yy214:	yych = *++YYCURSOR;
yy215:

	{
			*stream_flag = 1; 
			*nocase_flag = 1; 
			*match_flag  = 1; 
			return 0;
		}
}


	}

	return 0;

}



static nel_expr *clus_pre_exact_match(struct nel_eng *eng, struct prior_node *pnode)
{
        struct exact_match_info *ms;
	struct rule_list *rlist;
#ifdef	USE_ACBM
        struct ac_bm_tree *acbm_tree;
#elif	USE_SWBM
	struct ptn_list *ptnlist;
#endif

        struct ret_sym **retsyms;
        int i, retnum, shortest=65535;
	int stream_flag;
        int case_flag=0, nocase_flag=0;
	nel_expr *fun_call = NULL;
	

	rlist = pnode_to_rlist(pnode);
	if(rlist == NULL) {
		return NULL;
	}

        for (i=0; i<rlist->rule_num; i++) {
                delete_backslash(rlist->rules[i]);

                SETMIN(shortest, rlist->rules[i]->pat_len);

                nocase_flag |= rlist->rules[i]->nocase;
                case_flag |= !(rlist->rules[i]->nocase);
        }

	/*
        if (nocase_flag && case_flag) {
                if (build_case_tree(rlist) == -1)
                        return NULL;
        }
	*/

	
#ifdef	USE_ACBM
        acbm_tree = build_acbm_tree(rlist);
        if (!acbm_tree)
                return NULL;
#elif	USE_SWBM
        ptnlist = merge_pat(rlist, shortest, nocase_flag);
        if (!ptnlist)
                return NULL;

        if (build_rear_tree(rlist, ptnlist, nocase_flag) == -1) {
                free_ptn_list(ptnlist);
                return NULL;
        }

        if (build_search_tree(ptnlist, nocase_flag) == -1) {
                free_ptn_list(ptnlist);
                return NULL;
        }
#endif	

        retsyms = init_retsyms(rlist, &retnum);
        if (!retsyms) {
                //return NULL;
		goto error;
        }

#ifdef	USE_ACBM
        ms = exact_match_info_alloc(rlist, 	
					acbm_tree, 
					retsyms, 
					retnum);
        if (!ms) {
                //free_acbm_tree(acbm_tree);
                //free_retsym_list(retsyms,retnum);
                //return NULL;
		goto error;
        }

#elif	USE_SWBM
        ms = exact_match_info_alloc (rlist, 	
					ptnlist, 
					retsyms, 
					retnum);
        if (!ms) {
                //free_ptn_list(ptnlist);
                //free_retsym_list(retsyms, retnum);
                //return NULL;
		goto error;
        }
#endif

        pnode->data = (char *)ms;
	if (stream_flag == 1) {
		fun_call = stream_exact_match_funcall_alloc(eng, pnode, ms);
	}
	else {
		fun_call = buf_exact_match_funcall_alloc(eng, pnode, ms);
	}
	      	
	if (!fun_call) {
		goto error;	
	}	


	return fun_call;

error:
#ifdef	USE_ACBM
	if(acbm_tree)
		free_acbm_tree(acbm_tree);
	if(retsyms)
		free_retsym_list(retsyms, retnum);
	
#elif	USE_SWBM
	if(ptnlist)
		free_ptn_list(ptnlist);

	if(retsyms)
                free_retsym_list(retsyms, retnum);
#endif

	return NULL;

}

static nel_expr *clus_pre_match(struct nel_eng *eng, struct prior_node *pnode)
{
	int retnum = 0;
	int *retsyms;

	struct hashtab *offset_table;
	nel_expr *fun_call = NULL;

	struct match_info *ms;
	const char *error;
	int erroffset;
	nel_symbol *func_sym;

	int stream_flag = 0;
	int nocase_flag = 0;
	int match_flag = 0; 

#ifdef	USE_PCRE	
        char pattern[PCRE_PATTERN_SIZE]; 
	pcre_extra *extra;
	pcre *re;
#elif	USE_DFA
        char pattern[MAX_PATTERN_SIZE];

	//wyong, 20230731 
        //int s_flag = RE_SYNTAX_POSIX_EGREP;

	//bugfix, wyong, 20230825 
	int s_flag = 	RE_INTERVALS |  
			RE_NO_BK_BRACES |
				RE_NO_BK_PARENS |
				RE_NO_BK_VBAR ; 

	int i_flag = 0;
	int m_flag = '\0';
	int l_flag = 0;
	struct dfa *dfa;
#endif

	int pattern_len;
	int options = 0;

	if(pnode == NULL || pnode->numcount == 0 || pnode->exp == NULL ) {
		goto error;
	}

	/*set stream_flag and nocasae_flag, wyong, 2006.6.25 */
	if (!pnode->sbj || !(func_sym = pnode->sbj->symbol.symbol)) {
		goto error;
	}

	if (get_flag_from_name(func_sym->name, &stream_flag, 
				&nocase_flag, &match_flag ) < 0 ) {
		goto error;
	}

#ifdef	USE_PCRE
	if( nocase_flag & 0x01)
		options |= PCRE_CASELESS;
	if (nocase_flag & 0x02)
                options |= PCRE_MULTILINE;
	if (nocase_flag & 0x04)
                options |= PCRE_DOTALL;
#elif	USE_DFA
	if( nocase_flag & 0x01)
                i_flag = 1;
	if (nocase_flag & 0x02)
                m_flag = '\n';

	// wyong, 20230731
	if( nocase_flag & 0x04)
                s_flag |= RE_DOT_NEWLINE;
	

#endif

	offset_table = hash_alloc(1);
	if ((retnum = pnode_to_pattern(pnode, pattern, &pattern_len,  
#ifdef	USE_PCRE
		PCRE_PATTERN_SIZE, 
#elif	USE_DFA
		MAX_PATTERN_SIZE,
#endif
		offset_table)) <= 0 ) {
		goto pattern_error;
	}

	// malloc result array (retnum)
	nel_calloc(retsyms, retnum, int);
	if(!retsyms)
	{
		goto result_array_error;
	}

#ifdef	USE_PCRE
	/* malloc extra data with set pcre_data with result */
	nel_malloc(extra, 1, struct pcre_extra);
	if(!extra){
		goto pcre_extra_error;
	}

	extra->flags = 0;
	extra->flags |= PCRE_EXTRA_CALLOUT_DATA;


	// then compile the pattern with pcre_compile  
	if ((re = pcre_compile( pattern,	/* the pattern */
			options,		/* default options */
			&error,			/* for error message */
		&erroffset,			/* for error offset */
		NULL)) ==NULL){	/* use default character tables */
		goto pcre_re_error;
	}

	
	// create match_info for those patterns 
	if ((ms = match_info_alloc(re, 	/* pcre */
		extra, 			/* study data for pcre */
		retsyms,		/* return symbols */ 
		retnum,			/* return symbols number */
		offset_table)) ==NULL){	/* offset->retsyms index */
		goto ms_error;
	}

	extra->callout_data = (void *)ms;
	ms->extra = extra;

#elif 	USE_DFA
	nel_calloc(dfa, 1, struct dfa);
	if (!dfa) {
		goto result_array_error;
	}

        // then compile the pattern with dfacomp
        dfasyntax(dfa, s_flag , i_flag, m_flag, l_flag); 
	dfacomp(dfa, pattern, pattern_len, 1);

	// create match_info for those patterns, wyong, 20090516
        if ((ms = match_info_alloc(/*map,*/dfa,  
				retsyms, 
				retnum, 
				offset_table)) ==NULL ) {
                goto dfa_error;
        }

	dfa->ext_data = ms;		// wyong, 20090516 

#endif

        pnode->data = (char *)ms;
	if (stream_flag == 1) {
		fun_call = stream_match_funcall_alloc(eng, pnode, ms);
	}
	else {
		fun_call = match_flag ? buf_match_funcall_alloc(eng, pnode, ms)
	      : buf_no_match_funcall_alloc(eng, pnode, ms);
	}
	      	
	if (!fun_call) {
		goto ms_error;	
	}	


	return fun_call;

ms_error:
	if(ms){
		match_info_free(ms);
		pnode->data = NULL;
		return NULL;
	}

#ifdef	USE_PCRE
pcre_re_error:
	if(re){
		nel_free(re);	
	}

pcre_extra_error:
	if(extra){
		nel_free(extra);
	}

#elif	USE_DFA
dfa_error:
	if(dfa){
                dfafree(dfa);
		nel_free(dfa);
	}
#endif


result_array_error:
	if(retsyms){
		nel_free(retsyms);	
	}

pattern_error:
	if(offset_table){
		hash_free(offset_table);
	}

error:
	return NULL;

}

struct exact_match_info *exact_match_init(unsigned char **array, int array_size, int options)
{
        struct exact_match_info *ms;
	struct rule_list *rlist;
#ifdef	USE_ACBM
        struct ac_bm_tree *acbm_tree;
#elif	USE_SWBM
	struct ptn_list *ptnlist;
#endif

        struct ret_sym **retsyms;
        int i, retnum, shortest=65535;
	int stream_flag;
        int case_flag=0, nocase_flag=0;
	nel_expr *fun_call = NULL;
	

	rlist = array_to_rlist(array, array_size);
	if(rlist == NULL) {
		return NULL;
	}

        for (i=0; i<rlist->rule_num; i++) {
                delete_backslash(rlist->rules[i]);

                SETMIN(shortest, rlist->rules[i]->pat_len);

                nocase_flag |= rlist->rules[i]->nocase;
                case_flag |= !(rlist->rules[i]->nocase);
        }

	/*
        if (nocase_flag && case_flag) {
                if (build_case_tree(rlist) == -1)
                        return NULL;
        }
	*/

	
#ifdef	USE_ACBM
        acbm_tree = build_acbm_tree(rlist);
        if (!acbm_tree)
                return NULL;
#elif	USE_SWBM
        ptnlist = merge_pat(rlist, shortest, nocase_flag);
        if (!ptnlist)
                return NULL;

        if (build_rear_tree(rlist, ptnlist, nocase_flag) == -1) {
                free_ptn_list(ptnlist);
                return NULL;
        }

        if (build_search_tree(ptnlist, nocase_flag) == -1) {
                free_ptn_list(ptnlist);
                return NULL;
        }
#endif	

        retsyms = init_retsyms(rlist, &retnum);
        if (!retsyms) {
                //return NULL;
		goto error;
        }

#ifdef	USE_ACBM
        ms = exact_match_info_alloc(rlist, 	
					acbm_tree, 
					retsyms, 
					retnum);
        if (!ms) {
                //free_acbm_tree(acbm_tree);
                //free_retsym_list(retsyms,retnum);
                //return NULL;
		goto error;
        }

#elif	USE_SWBM
        ms = exact_match_info_alloc (rlist, 	
					ptnlist, 
					retsyms, 
					retnum);
        if (!ms) {
                //free_ptn_list(ptnlist);
                //free_retsym_list(retsyms, retnum);
                //return NULL;
		goto error;
        }
#endif

	return ms;

error:
	if(ms){
		exact_match_info_free(ms);
	}

#ifdef	USE_ACBM
	if(acbm_tree)
		free_acbm_tree(acbm_tree);
	if(retsyms)
		free_retsym_list(retsyms, retnum);
	
#elif	USE_SWBM
	if(ptnlist)
		free_ptn_list(ptnlist);

	if(retsyms)
                free_retsym_list(retsyms, retnum);
#endif

	return NULL;

}


struct match_info *match_init(unsigned char **array, int array_size, int options)
{
#ifdef	USE_PCRE	
        char pattern[PCRE_PATTERN_SIZE]; 
	pcre_extra *extra;
	pcre *re;
#elif	USE_DFA
        char pattern[MAX_PATTERN_SIZE];

	//wyong, 20230731
        //int s_flag = RE_SYNTAX_POSIX_EGREP;

	//bugfix, wyong, 20230825 
	int s_flag = 	RE_INTERVALS |  
			RE_NO_BK_BRACES |
				RE_NO_BK_PARENS |
				RE_NO_BK_VBAR ; 

	int i_flag = 0;
	int m_flag = '\0';
	int l_flag = 0;	/*lazy ? */
	struct dfa *dfa;
#endif
	int retnum = 0;
	int *retsyms = NULL;
	struct hashtab *offset_table = NULL;
	struct match_info *ms = NULL;
	int erroffset;
	const char *error;
	int pattern_len;

	if( array == NULL || array_size == 0 ) {
		goto error;
	}

	offset_table = hash_alloc(1);
	if ((retnum = array_to_pattern(array, array_size, pattern, 
		&pattern_len, 
#ifdef	USE_PCRE
		PCRE_PATTERN_SIZE, 
#elif	USE_DFA
		MAX_PATTERN_SIZE,
#endif
		offset_table)) <= 0 ) {
		goto error;
	}

	// malloc result array (retnum)
	nel_calloc(retsyms, retnum, int);
	if(!retsyms) {
		goto error;
	}

#ifdef	USE_PCRE
	// malloc extra data with set pcre_data with result
	nel_malloc(extra, 1, pcre_extra);
	if(!extra){
		goto error;
	}

	extra->flags = 0;
	extra->flags |= PCRE_EXTRA_CALLOUT_DATA;
	

	// then compile the pattern with pcre_compile  
	if ((re = pcre_compile(pattern,		/* the pattern */
			options,	/* default options */
			&error,		/* for error message */
		&erroffset,		/* for error offset */
		NULL)) ==NULL){	/* use default character tables */
		goto error;
	}

	
	// create match_info for those patterns 
	if ((ms = match_info_alloc(re, 	/* pcre */
		extra, 			/* study data for pcre */
		retsyms,		/* return symbols */ 
		retnum,			/* return symbols number */
		offset_table)) ==NULL){	/* offset->retsyms index */
		goto error;
	}

	extra->callout_data = (void *)ms;
	ms->extra = extra;

#elif 	USE_DFA
	if(options & 0x01)
		i_flag = 1;
	if(options & 0x02)
                m_flag = '\n';
	if(options & 0x04)
                s_flag |= RE_DOT_NEWLINE;
	if(options & 0x08)
		l_flag = 1;
                
	nel_calloc(dfa, 1, struct dfa);
	if (!dfa) {
		goto error;
	}

	dfasyntax (dfa, s_flag, i_flag, m_flag, l_flag);
        dfacomp(dfa, pattern, pattern_len, 1);

        if ((ms = match_info_alloc(dfa,
				retsyms, 
				retnum, 
				offset_table )) ==NULL ) {
                goto error;
        }

	dfa->ext_data = ms;
#endif

	return ms;

error:
	if(ms){
		match_info_free(ms);
	}

#ifdef	USE_PCRE
	if(re) nel_free(re);
	if(extra) nel_free(extra);	

#elif	USE_DFA
     	if (dfa) {
                dfafree(dfa);
		nel_free(dfa);
        }
#endif

	if(retsyms) nel_free(retsyms);	
	//if(offset_table) {
	//	hash_free(offset_table);
	//}

	return NULL;
}

static nel_symbol *exact_match_init_func_init(struct nel_eng *eng)
{
	nel_type *type;
	nel_symbol *symbol;
	nel_list *args;

	symbol = nel_lookup_symbol("exact_match_init", 
					eng->nel_static_tag_hash, 
				eng->nel_static_location_hash, NULL);
	if( symbol == NULL ) {

		symbol = nel_static_symbol_alloc(eng, 
					nel_insert_name(eng,"options"), 
					nel_int_type, NULL, nel_C_FORMAL, 
						nel_lhs_type(nel_int_type), 
							nel_L_C, 1);
		args = nel_list_alloc(eng, 0, symbol, NULL);

		symbol = nel_static_symbol_alloc(eng, 
					nel_insert_name(eng,"array_size"), 
					nel_int_type, NULL, nel_C_FORMAL, 
					nel_lhs_type(nel_int_type), nel_L_C, 1);
		args = nel_list_alloc(eng, 0, symbol, args);


		type = nel_type_alloc(eng, nel_D_POINTER, 
					sizeof(unsigned char *), 
					nel_alignment_of(unsigned char *), 
					0,0, nel_unsigned_char_type );
		type = nel_type_alloc(eng, nel_D_POINTER, 
					sizeof(unsigned char **), 
					nel_alignment_of(unsigned char *), 
					0,0, type );
		symbol = nel_static_symbol_alloc(eng, 
					nel_insert_name(eng,"array"), 
					type, NULL, nel_C_FORMAL, 
						nel_lhs_type(type), 
					nel_L_C, 1);
		args = nel_list_alloc(eng, 0, symbol, args);


		symbol = nel_lookup_symbol("exact_match_info", 
					eng->nel_static_tag_hash, NULL);
		type = symbol->type->typedef_name.descriptor;
		type = nel_type_alloc(eng, nel_D_POINTER, 
					sizeof(struct exact_match_info *),
					nel_alignment_of(struct exact_match_info *), 
					0,0,type);
		
		type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 0, 
							type, args, NULL, NULL);

		symbol = nel_static_symbol_alloc (eng, 
				nel_insert_name(eng, "exact_match_init"), 
					type, (char *) exact_match_init, 
						nel_C_COMPILED_FUNCTION, 
						nel_lhs_type(type), nel_L_C, 0);

		nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);

	}else if(symbol->value == NULL) {
		symbol->value = (char *) exact_match_init;
	}else if(symbol->value != (char *)exact_match_init) {
		printf("the earily inserted symbol have difference value with"
		" exact_match_init!\n");
	}
	else {
		printf("exact_match_init was successfully inserted!\n");
	}

	symbol->type->function.prev_hander = NULL;	
	symbol->type->function.post_hander = NULL;
}

static nel_symbol *match_init_func_init(struct nel_eng *eng)
{
	nel_type *type;
	nel_symbol *symbol;
	nel_list *args;

	symbol = nel_lookup_symbol("match_init", eng->nel_static_tag_hash, 
		eng->nel_static_location_hash, NULL);
	if( symbol == NULL ) {

		symbol = nel_static_symbol_alloc(eng, 
						nel_insert_name(eng,"options"), 
					nel_int_type, NULL, nel_C_FORMAL, 
					nel_lhs_type(nel_int_type), nel_L_C, 1);
		args = nel_list_alloc(eng, 0, symbol, NULL);


		symbol = nel_static_symbol_alloc(eng, 
						nel_insert_name(eng,"array_size"), 
					nel_int_type, NULL, nel_C_FORMAL, 
					nel_lhs_type(nel_int_type), nel_L_C, 1);
		args = nel_list_alloc(eng, 0, symbol, args);


		type = nel_type_alloc(eng, nel_D_POINTER, 
					sizeof(unsigned char *), 
					nel_alignment_of(unsigned char *), 
					0,0, nel_unsigned_char_type );
		type = nel_type_alloc(eng, nel_D_POINTER, 
					sizeof(unsigned char **), 
					nel_alignment_of(unsigned char *), 
					0,0, type );
		symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,"array"), 
					type, NULL, nel_C_FORMAL, nel_lhs_type(type), 
					nel_L_C, 1);
		args = nel_list_alloc(eng, 0, symbol, args);


		symbol = nel_lookup_symbol("match_info", eng->nel_static_tag_hash,
									NULL);
		type = symbol->type->typedef_name.descriptor;
		type = nel_type_alloc(eng, nel_D_POINTER, 
						sizeof(struct match_info *),
					nel_alignment_of(struct match_info *), 
					0,0,type);
		
		type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 0, 
							type, args, NULL, NULL);

		symbol = nel_static_symbol_alloc (eng, 
						nel_insert_name(eng, "match_init"), 
						type, (char *) match_init, 
						nel_C_COMPILED_FUNCTION, 
						nel_lhs_type(type), nel_L_C, 0);

		nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);

	}else if(symbol->value == NULL) {
		symbol->value = (char *) match_init;
	}else if(symbol->value != (char *)match_init) {
		printf("the earily inserted symbol have difference value with"
		" match_init!\n");
	}
	else {
		printf("match_init was successfully inserted!\n");
	}

	symbol->type->function.prev_hander = NULL;	
	symbol->type->function.post_hander = NULL;

}

void match_free(struct match_info *ms)
{
	match_info_free(ms);
}

void exact_match_free(struct exact_match_info *ms)
{
	exact_match_info_free(ms);
}

static nel_symbol *exact_match_free_func_init(struct nel_eng *eng)
{
	nel_type *type;
	nel_symbol *symbol;
	nel_list *args;

	symbol = nel_lookup_symbol("exact_match_free", 
						eng->nel_static_tag_hash, 
				eng->nel_static_location_hash, NULL);
	if( symbol == NULL ) {

		symbol = nel_lookup_symbol("exact_match_info", 
					eng->nel_static_tag_hash, NULL);
		type = symbol->type->typedef_name.descriptor;
		type = nel_type_alloc(eng, nel_D_POINTER, 
				sizeof(struct exact_match_info *), 
			nel_alignment_of(struct exact_match_info *), 0,0,type);
		symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,"ms"), 
				type, NULL, nel_C_FORMAL, nel_lhs_type(type), 
				nel_L_C, 1);
		args = nel_list_alloc(eng, 0, symbol, NULL);
		
		type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 0, 
				nel_void_type, args, NULL, NULL);

		symbol = nel_static_symbol_alloc (eng, 
					nel_insert_name(eng,"exact_match_free"),
					type, 
					(char *) exact_match_free, 
					nel_C_COMPILED_FUNCTION, 
						nel_lhs_type(type), 
							nel_L_C, 0);

		nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);

	}else if(symbol->value == NULL) {
		symbol->value = (char *) exact_match_free;
	}else if(symbol->value != (char *)exact_match_free) {
		printf("the earily inserted symbol have difference value with"
		" exact_match_free!\n");
	}
	else {
		printf("exact_match_free was successfully inserted!\n");
	}

	symbol->type->function.prev_hander = NULL;	
	symbol->type->function.post_hander = NULL;
}

static nel_symbol *match_free_func_init(struct nel_eng *eng)
{
	nel_type *type;
	nel_symbol *symbol;
	nel_list *args;

	symbol = nel_lookup_symbol("match_free", eng->nel_static_tag_hash, 
		eng->nel_static_location_hash, NULL);
	if( symbol == NULL ) {

		symbol = nel_lookup_symbol("match_info", 
					eng->nel_static_tag_hash, NULL);
		type = symbol->type->typedef_name.descriptor;
		type = nel_type_alloc(eng, nel_D_POINTER, 
				sizeof(struct match_info *), 
			nel_alignment_of(struct match_info *), 0,0,type);
		symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,"ms"), 
				type, NULL, nel_C_FORMAL, nel_lhs_type(type), 
				nel_L_C, 1);
		args = nel_list_alloc(eng, 0, symbol, NULL);
		
		type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 0, 
				nel_void_type, args, NULL, NULL);

		symbol = nel_static_symbol_alloc (eng, 
				nel_insert_name(eng, "match_free"), type, 
				(char *) match_free, nel_C_COMPILED_FUNCTION, 
				nel_lhs_type(type), nel_L_C, 0);

		nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);

	}else if(symbol->value == NULL) {
		symbol->value = (char *) match_free;
	}else if(symbol->value != (char *)match_free) {
		printf("the earily inserted symbol have difference value with"
		" match_free!\n");
	}
	else {
		printf("match_free was successfully inserted!\n");
	}

	symbol->type->function.prev_hander = NULL;	
	symbol->type->function.post_hander = NULL;

}


int stream_exact_match(struct stream *stream, struct exact_match_info *ms)
{
	int i;
	if (do_stream_exact_match(stream, ms) > 0 ) {
		for(i=0; i< ms->retnum; i++  ){
			if( ms->retsyms[i]->val == 1 ){
				return 1;  //ms->retsyms[i] = 0;
			}
		}
	}
	return 0;

}

nel_symbol *INIT_STREAM_EXACT_MATCH_FUNC( struct nel_eng *eng,
				char *name, 
				int (* func)(struct stream *stream, 
						struct exact_match_info *ms),
				nel_symbol *pre, 
				nel_symbol *post)		
{									
	nel_type *type;							
	nel_symbol *symbol;							
	nel_list *args;								
										
	symbol = nel_lookup_symbol(name,
	eng->nel_static_ident_hash, eng->nel_static_location_hash, NULL);	
	if( symbol == NULL ) {							
										
		symbol = nel_lookup_symbol("exact_match_info", 			
					eng->nel_static_tag_hash,NULL);		
		type = symbol->type->typedef_name.descriptor;			
		type = nel_type_alloc(eng, nel_D_POINTER,			
				sizeof(struct exact_match_info *), 		
				nel_alignment_of(struct exact_match_info *), 	
					0,0,type);				
		symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,"ms"),
					type, NULL, nel_C_FORMAL,		
					nel_lhs_type(type), nel_L_C, 1);	
		args = nel_list_alloc(eng, 0, symbol, NULL);			
										
		symbol = nel_lookup_symbol("stream",				
					eng->nel_static_tag_hash,NULL);		
		type = symbol->type->typedef_name.descriptor;			
		type = nel_type_alloc(eng, nel_D_POINTER,			
					sizeof(struct stream *),		
					nel_alignment_of(struct stream *), 	
					0,0,type);				
		symbol = nel_static_symbol_alloc(eng,				
					nel_insert_name(eng,"stream"),		
					type, 					
					NULL,					
					nel_C_FORMAL, nel_lhs_type(type), 	
					nel_L_C, 1);				
		args = nel_list_alloc(eng, 0, symbol, args);			
										
		type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 0, 	
					nel_int_type, args, NULL, NULL);	
										
		symbol = nel_static_symbol_alloc (eng, 				
					nel_insert_name(eng, name),	
					type, (char *) func, 		
					nel_C_COMPILED_FUNCTION, 	
					nel_lhs_type(type), nel_L_C, 0);	
			
		nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);

	}else if(symbol->value == NULL) {
		symbol->value = (char *) func;
	}else if(symbol->value != (char *)func) {	
		/*printf("the earily symbol have difference value with"	*/
		/*			## (__func__) ## "!\n");	*/
	}
	else {
		/*printf(__name__# "was successfully inserted!\n");*/
	}

	symbol->type->function.prev_hander = pre;
	symbol->type->function.post_hander = post;
	symbol->type->function.key_nums    = 1 ;   //wyong,20090516

	return symbol;
}


int stream_match(struct stream *stream, struct match_info *ms)
{
	int i;
	if (do_stream_match(stream, ms) > 0 ) {
		for(i=0; i< ms->retnum; i++  ){
			if( ms->retsyms[i] == 1 ){
				return 1;  //ms->retsyms[i] = 0;
			}
		}
	}
	return 0;

}


nel_symbol *INIT_STREAM_MATCH_FUNC( struct nel_eng *eng,
				char *name, 
				int (* func)(struct stream *stream, 
						struct match_info *ms),
				nel_symbol *pre, 
				nel_symbol *post)		
{									
	nel_type *type;							
	nel_symbol *symbol;							
	nel_list *args;								
										
	symbol = nel_lookup_symbol(name,
	eng->nel_static_ident_hash, eng->nel_static_location_hash, NULL);	
	if( symbol == NULL ) {							
										
		symbol = nel_lookup_symbol("match_info", 			
					eng->nel_static_tag_hash,NULL);		
		type = symbol->type->typedef_name.descriptor;			
		type = nel_type_alloc(eng, nel_D_POINTER,			
					sizeof(struct match_info *), 		
					nel_alignment_of(struct match_info *), 	
					0,0,type);				
		symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,"ms"),
					type, NULL, nel_C_FORMAL,		
					nel_lhs_type(type), nel_L_C, 1);	
		args = nel_list_alloc(eng, 0, symbol, NULL);			
										
		symbol = nel_lookup_symbol("stream",				
					eng->nel_static_tag_hash,NULL);		
		type = symbol->type->typedef_name.descriptor;			
		type = nel_type_alloc(eng, nel_D_POINTER,			
					sizeof(struct stream *),		
					nel_alignment_of(struct stream *), 	
					0,0,type);				
		symbol = nel_static_symbol_alloc(eng,				
					nel_insert_name(eng,"stream"),		
					type, 					
					NULL,					
					nel_C_FORMAL, nel_lhs_type(type), 	
					nel_L_C, 1);				
		args = nel_list_alloc(eng, 0, symbol, args);			
										
		type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 0, 	
					nel_int_type, args, NULL, NULL);	
										
		symbol = nel_static_symbol_alloc (eng, 				
					nel_insert_name(eng, name),	
					type, (char *) func, 		
					nel_C_COMPILED_FUNCTION, 	
					nel_lhs_type(type), nel_L_C, 0);	
			
		nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);

	}else if(symbol->value == NULL) {
		symbol->value = (char *) func;
	}else if(symbol->value != (char *)func) {	
		/*printf("the earily symbol have difference value with"	*/
		/*			## (__func__) ## "!\n");	*/
	}
	else {
		/*printf(__name__# "was successfully inserted!\n");*/
	}

	symbol->type->function.prev_hander = pre;
	symbol->type->function.post_hander = post;
	symbol->type->function.key_nums    = 1 ;   //wyong,20090516

	return symbol;
}



int buf_exact_match(unsigned char *data, int length, struct exact_match_info *ms)
{
	int i;
	if (do_buf_exact_match(data, length, ms) > 0 ) {
		for(i=0; i< ms->retnum; i++  ){
			if( ms->retsyms[i]->val == 1 ){
				return 1;  //ms->retsyms[i] = 0;
			}
		}
	}
	return 0;
}

nel_symbol *INIT_BUFFER_EXACT_MATCH_FUNC(struct nel_eng *eng,
				char *name, 
				int (*func)(unsigned char *data, 
						int length, 
						struct exact_match_info *ms),
				nel_symbol *pre, 
				nel_symbol *post)
{										
	nel_type *type;								
	nel_symbol *symbol;							
	nel_list *args;								

	symbol = nel_lookup_symbol(name, eng->nel_static_tag_hash,	
		eng->nel_static_location_hash, NULL);				
	if( symbol == NULL ) {							
										
		symbol = nel_lookup_symbol("exact_match_info", 			
					eng->nel_static_tag_hash,NULL);		
		type = symbol->type->typedef_name.descriptor;			
		type = nel_type_alloc(eng, nel_D_POINTER, 			
				sizeof(struct exact_match_info *), 		
				nel_alignment_of(struct exact_match_info *), 	
							0,0,type);				
		symbol = nel_static_symbol_alloc(eng, 				
					nel_insert_name(eng,"ms"), 		
					type, NULL, nel_C_FORMAL, 		
					nel_lhs_type(type), nel_L_C, 1);	
		args = nel_list_alloc(eng, 0, symbol, NULL);			
										
										
		symbol = nel_static_symbol_alloc(eng, 				
					nel_insert_name(eng,"len"), 		
					nel_int_type, NULL, nel_C_FORMAL,	
					nel_lhs_type(nel_int_type), 		
					nel_L_C, 1);				
		args = nel_list_alloc(eng, 0, symbol, args);			
										
										
		type = nel_type_alloc(eng, nel_D_POINTER, 			
					sizeof(unsigned char *), 		
					nel_alignment_of(unsigned char *), 	
					0,0, nel_unsigned_char_type );		
		symbol = nel_static_symbol_alloc(eng, 				
					nel_insert_name(eng,"data"), 		
					type, NULL, nel_C_FORMAL, 		
					nel_lhs_type(type), nel_L_C, 1);	
		args = nel_list_alloc(eng, 0, symbol, args);			
										
										
		type = nel_type_alloc (eng, nel_D_FUNCTION, 			
					0, 0, 0, 0, 0, 0, nel_int_type, 	
					args, NULL, NULL);			
										
		symbol = nel_static_symbol_alloc (eng, 				
					nel_insert_name(eng, name), 	
					type, (char *) func, 		
					nel_C_COMPILED_FUNCTION,		
					nel_lhs_type(type), nel_L_C, 0);	
										
		nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);	
										
										
	}else if(symbol->value == NULL) {					
		symbol->value = (char *) func;				
	}else if(symbol->value != (char *)func) {				
		/*printf("the earily symbol have difference value with !\n");*/	
	}									
	else {									
		/*printf("buf_match was successfully inserted!\n");*/		
	}									
										
	symbol->type->function.prev_hander = pre;				
	symbol->type->function.post_hander = post;			
	symbol->type->function.key_nums	   = 2;			
										
	return symbol;								
}


int buf_match(unsigned char *data, int length, struct match_info *ms)
{
	int i;
	if (do_buf_match(data, length, ms) > 0 ) {
		for(i=0; i< ms->retnum; i++  ){
			if( ms->retsyms[i] == 1 ){
				return 1;  //ms->retsyms[i] = 0;
			}
		}
	}

	return 0;
}

int buf_no_match(unsigned char *text, int len, struct match_info *ms)
{	
	int i;
	do_buf_match(text, len, ms);
	{
		for(i=0; i< ms->retnum; i++  ){
			if( ms->retsyms[i] == 0 ){
				return 1;  //ms->retsyms[i] = 0;
			}
		}
	}
	return 0;
}


nel_symbol *INIT_BUFFER_MATCH_FUNC(struct nel_eng *eng,
				char *name, 
				int (*func)(unsigned char *data, 
						int length, 
						struct match_info *ms),
				nel_symbol *pre, 
				nel_symbol *post)
{										
	nel_type *type;								
	nel_symbol *symbol;							
	nel_list *args;								

	symbol = nel_lookup_symbol(name, eng->nel_static_tag_hash,	
		eng->nel_static_location_hash, NULL);				
	if( symbol == NULL ) {							
										
		symbol = nel_lookup_symbol("match_info", 			
					eng->nel_static_tag_hash,NULL);		
		type = symbol->type->typedef_name.descriptor;			
		type = nel_type_alloc(eng, nel_D_POINTER, 			
					sizeof(struct match_info *), 		
					nel_alignment_of(struct match_info *), 	
					0,0,type);				
		symbol = nel_static_symbol_alloc(eng, 				
					nel_insert_name(eng,"ms"), 		
					type, NULL, nel_C_FORMAL, 		
					nel_lhs_type(type), nel_L_C, 1);	
		args = nel_list_alloc(eng, 0, symbol, NULL);			
										
										
		symbol = nel_static_symbol_alloc(eng, 				
					nel_insert_name(eng,"len"), 		
					nel_int_type, NULL, nel_C_FORMAL,	
					nel_lhs_type(nel_int_type), 		
					nel_L_C, 1);				
		args = nel_list_alloc(eng, 0, symbol, args);			
										
										
		type = nel_type_alloc(eng, nel_D_POINTER, 			
					sizeof(unsigned char *), 		
					nel_alignment_of(unsigned char *), 	
					0,0, nel_unsigned_char_type );		
		symbol = nel_static_symbol_alloc(eng, 				
					nel_insert_name(eng,"data"), 		
					type, NULL, nel_C_FORMAL, 		
					nel_lhs_type(type), nel_L_C, 1);	
		args = nel_list_alloc(eng, 0, symbol, args);			
										
										
		type = nel_type_alloc (eng, nel_D_FUNCTION, 			
					0, 0, 0, 0, 0, 0, nel_int_type, 	
					args, NULL, NULL);			
										
		symbol = nel_static_symbol_alloc (eng, 				
					nel_insert_name(eng, name), 	
					type, (char *) func, 		
					nel_C_COMPILED_FUNCTION,		
					nel_lhs_type(type), nel_L_C, 0);	
										
		nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);	
										
										
	}else if(symbol->value == NULL) {					
		symbol->value = (char *) func;				
	}else if(symbol->value != (char *)func) {				
		/*printf("the earily symbol have difference value with !\n");*/	
	}									
	else {									
		/*printf("buf_match was successfully inserted!\n");*/		
	}									
										
	symbol->type->function.prev_hander = pre;				
	symbol->type->function.post_hander = post;			
	symbol->type->function.key_nums	   = 2;			
										
	return symbol;								
}





static nel_symbol *INIT_PREV_MATCH_FUNC(struct nel_eng *eng, char *name, 
				nel_expr * (* func)(struct nel_eng *eng, 
						struct prior_node *pnode))
{
	nel_symbol *symbol;
	symbol = nel_lookup_symbol(name, eng->nel_static_ident_hash, 
		eng->nel_static_location_hash, NULL);
	if( symbol == NULL ) {
		symbol = nel_static_symbol_alloc(eng, 
						nel_insert_name(eng, name), 
						NULL, 
						(char *) func, 
						nel_C_COMPILED_FUNCTION, 
						0, 
						nel_L_C, 
							0);
		nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);
	}
	
	return symbol;
}

static nel_symbol *INIT_POST_MATCH_FUNC(struct nel_eng *eng, char *name, 
				nel_expr * (* func)(struct nel_eng *eng, 
						struct prior_node *pnode,
						int id))
{
	nel_symbol *symbol;
	symbol = nel_lookup_symbol(name, eng->nel_static_ident_hash, 
		eng->nel_static_location_hash, NULL);
	if( symbol == NULL ) {
		symbol = nel_static_symbol_alloc(eng, 
						nel_insert_name(eng, name), 
						NULL, 
						(char *) func, 
						nel_C_COMPILED_FUNCTION, 
							0, 
							nel_L_C, 
						0);
		nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);
	}
	
	return symbol;
}

int nel_match_init(struct nel_eng *eng)
{

	nel_symbol *clus_pre_match_symbol; 
	nel_symbol *clus_post_match_symbol;
	nel_symbol *clus_post_no_match_symbol;
	nel_symbol *clus_pre_exact_match_symbol;
	nel_symbol *clus_post_exact_match_symbol;

	//printf("nel_match_init(10)\n");
	is_match_func_init(eng);
	//printf("nel_match_init(20)\n");
	is_exact_match_func_init(eng);
	//printf("nel_match_init(30)\n");
	is_no_match_func_init(eng);
	//printf("nel_match_init(40)\n");

	match_init_func_init(eng);
	//printf("nel_match_init(50)\n");
	match_free_func_init(eng);
	//printf("nel_match_init(60)\n");
	exact_match_init_func_init(eng);
	//printf("nel_match_init(70)\n");
	exact_match_free_func_init(eng);
	//printf("nel_match_init(80)\n");

	/*
	clus_pre_match_symbol = clus_pre_match_func_init(eng);
	clus_post_match_symbol = clus_post_match_func_init(eng);
	clus_post_no_match_symbol = clus_post_no_match_func_init(eng);
	clus_pre_exact_match_symbol = clus_pre_exact_match_func_init(eng);
	clus_post_exact_match_symbol= clus_post_exact_match_func_init(eng);
	*/

	clus_pre_match_symbol  = INIT_PREV_MATCH_FUNC(eng, 
						"clus_pre_match", 
						clus_pre_match );
	//printf("nel_match_init(90)\n");
	clus_post_match_symbol = INIT_POST_MATCH_FUNC(eng, 
							"clus_post_match", 
							clus_post_match );
	//printf("nel_match_init(100)\n");
	clus_post_no_match_symbol = INIT_POST_MATCH_FUNC(eng, 
						"clus_post_no_match", 
						clus_post_no_match );

	//printf("nel_match_init(110)\n");

	clus_pre_exact_match_symbol = INIT_PREV_MATCH_FUNC(eng, 
						"clus_pre_exact_match", 
						clus_pre_exact_match);
	//printf("nel_match_init(120)\n");
	clus_post_exact_match_symbol = INIT_POST_MATCH_FUNC(eng, 
						"clus_post_exact_match",
						clus_post_exact_match);

	//printf("nel_match_init(130)\n");
	/* wyong, 20090609 */
	INIT_STREAM_EXACT_MATCH_FUNC(eng, "stream_exact_match", 
				     stream_exact_match, 
				     (nel_symbol *)NULL, 
				     (nel_symbol *)NULL);
	//printf("nel_match_init(140)\n");
	INIT_STREAM_EXACT_MATCH_FUNC(eng, "clus_stream_exact_match", 
				     stream_exact_match, 
				     clus_pre_exact_match_symbol, 
				     clus_post_exact_match_symbol );
	//printf("nel_match_init(150)\n");
	INIT_BUFFER_EXACT_MATCH_FUNC(eng, "buf_exact_match", 
				     buf_exact_match, 
				     (nel_symbol *)NULL, 
				     (nel_symbol *)NULL);
	//printf("nel_match_init(160)\n");
	INIT_BUFFER_EXACT_MATCH_FUNC(eng, "clus_buf_exact_match", 
				     buf_exact_match, 
				     clus_pre_exact_match_symbol, 
				     clus_post_exact_match_symbol );


	//printf("nel_match_init(170)\n");
	INIT_STREAM_MATCH_FUNC(eng, "stream_match", 
					stream_match, 
					(nel_symbol *)NULL, 
					(nel_symbol *)NULL);
	//printf("nel_match_init(180)\n");
	INIT_STREAM_MATCH_FUNC(eng, "clus_stream_case_match", 
				stream_match, 
				clus_pre_match_symbol, 
				clus_post_match_symbol);		
	//printf("nel_match_init(190)\n");
	INIT_STREAM_MATCH_FUNC(eng, "clus_stream_nocase_match", 
				stream_match, 
				clus_pre_match_symbol, 
				clus_post_match_symbol);		
	//printf("nel_match_init(200)\n");
	INIT_STREAM_MATCH_FUNC(eng, "clus_stream_dotall_nocase_match", 
				stream_match, 
				clus_pre_match_symbol, 
				clus_post_match_symbol);		
	//printf("nel_match_init(210)\n");
	INIT_STREAM_MATCH_FUNC(eng, "clus_stream_dotall_multiline_nocase_match", 
				stream_match, 
				clus_pre_match_symbol, 
				clus_post_match_symbol);		

	//printf("nel_match_init(220)\n");

	INIT_BUFFER_MATCH_FUNC(eng, "buf_match", 
					buf_match, 
					(nel_symbol *)NULL, 
					(nel_symbol *)NULL);

	//printf("nel_match_init(230)\n");
	INIT_BUFFER_MATCH_FUNC(eng, "buf_no_match", 
					buf_no_match, 
					(nel_symbol *)NULL, 
					(nel_symbol *)NULL);
	//printf("nel_match_init(240)\n");
	INIT_BUFFER_MATCH_FUNC(eng, "clus_buf_case_match", 
				buf_match, 
				clus_pre_match_symbol, 
				clus_post_match_symbol);
	//printf("nel_match_init(250)\n");
	INIT_BUFFER_MATCH_FUNC(eng, "clus_buf_nocase_match", 
				buf_match, 
				clus_pre_match_symbol, 
				clus_post_match_symbol);
	//printf("nel_match_init(260)\n");
	INIT_BUFFER_MATCH_FUNC(eng, "clus_buf_dotall_match", 
				buf_match, 
				clus_pre_match_symbol, 
				clus_post_match_symbol);
	//printf("nel_match_init(270)\n");
	INIT_BUFFER_MATCH_FUNC(eng, "clus_buf_dotall_nocase_match", 
				buf_match, 
				clus_pre_match_symbol, 
				clus_post_match_symbol);
	//printf("nel_match_init(280)\n");
	INIT_BUFFER_MATCH_FUNC(eng, "clus_buf_case_no_match", 
				buf_no_match, 
					clus_pre_match_symbol, 
				clus_post_no_match_symbol);
	//printf("nel_match_init(290)\n");
	INIT_BUFFER_MATCH_FUNC(eng, "clus_buf_nocase_no_match", 
				buf_no_match, 
				clus_pre_match_symbol, 
				clus_post_no_match_symbol);
	//printf("nel_match_init(300)\n");
	INIT_BUFFER_MATCH_FUNC(eng, "clus_buf_dotall_no_match", 
				buf_no_match, 
				clus_pre_match_symbol, 
				clus_post_no_match_symbol);
	//printf("nel_match_init(310)\n");
	INIT_BUFFER_MATCH_FUNC(eng, "clus_buf_dotall_nocase_no_match", 
				buf_no_match, 
				clus_pre_match_symbol, 
				clus_post_no_match_symbol);
	//printf("nel_match_init(320)\n");
	INIT_BUFFER_MATCH_FUNC(eng, "clus_buf_dotall_multiline_match", 
				buf_match, 
				clus_pre_match_symbol, 
				clus_post_match_symbol);
	//printf("nel_match_init(330)\n");
	INIT_BUFFER_MATCH_FUNC(eng, "clus_buf_dotall_multiline_no_match", 
				buf_no_match, 
				clus_pre_match_symbol, 
				clus_post_no_match_symbol);
	//printf("nel_match_init(340)\n");
	INIT_BUFFER_MATCH_FUNC(eng, "clus_buf_dotall_multiline_nocase_match", 
				buf_match, 
				clus_pre_match_symbol, 
				clus_post_match_symbol);
	//printf("nel_match_init(350)\n");
	INIT_BUFFER_MATCH_FUNC(eng, "clus_buf_dotall_multiline_nocase_no_match", 
				buf_no_match, 
				clus_pre_match_symbol, 
				clus_post_no_match_symbol);

	//printf("nel_match_init(360)\n");
	/* set callout as pcre_callout callback function */
#ifdef	USE_PCRE
	pcre_callout = pcre_callout_func;
#endif

	return 0;
}

