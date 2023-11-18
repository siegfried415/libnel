/*
 * swbm_strm.h
 * $Id: swbm_strm.h,v 1.2 2006/03/17 05:50:16 wyong Exp $
 */
 

#ifndef __SWBM_STRM_H
#define __SWBM_STRM_H

#include "nlib/match/swbm.h"
//#include "intlist.h"

#define MAX_REMAIN_TEXT_LEN	256

/* max length of 'ptn->subpats' */
#define MAXPATLEN		1024

#define MAX_SUBPAT_NUM	1024


typedef struct _sub_pat {
	char *pat;
	int len;
} sub_pat;

typedef struct _sub_rule {
	got_list_t *got_list;
	struct trie *rule_trie;
} sub_rule;


extern sub_pat *new_subpat(unsigned char *pat, int len);
extern sub_pat **new_subpat_list(int subpat_num);
extern int show_subpat(sub_pat *sp);
extern void free_subpat(sub_pat **sub_pats, int subpat_num);
extern sub_rule **new_subrule_list(int subrule_num);
extern sub_rear **new_subrear_list(int subrear_num);
extern int show_subrear(sub_rear *sr);
extern void free_subrear(sub_rear *sr);
int show_subrule(sub_rule *sr);

got_list_t *insert_got_list(int offset, int_list_t *id_list, got_list_t *next);
void free_got_list(got_list_t *got_list);

#endif
