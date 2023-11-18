
/*
 * match.h
 */

#ifndef _MATCH_H
#define _MATCH_H

#ifdef	USE_PCRE
#include "pcre.h"

#elif	USE_DFA
#include "./match/dfa.h"
struct hashtab;
#endif

#ifdef	USE_ACBM
#include "ac_bm.h"
#elif	USE_SWBM
#include "./match/swbm.h"
#endif

/*
struct ret_sym {
        int symbol;
        int val;
};
*/


struct exact_match_info {
	int id;      //yanf, 2006.12.1
       	struct rule_list *rlist;
#ifdef	USE_ACBM
	struct ac_bm_tree *acbm;
#elif	USE_SWBM
	struct ptn_list *ptnlist;
#endif
	//int *retsyms;
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
	//struct hashtab *offset_table;	
};

#endif	//#ifndef MATCH_H


