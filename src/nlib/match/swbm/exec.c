/*
 * swbm_exec.c
 * $Id: swbm_exec.c,v 1.2 2006/03/17 05:50:16 wyong Exp $
 */

#include <stdio.h>

//#include "stream.h"
//#include "memwatch.h"
#include "nlib/match/swbm.h"
#include "intlist.h"

#if 0
#include "ptn.h"
#endif
#include "exec.h"

#define DEBUGING 0


#define CHECK_SID(ID) \
	id = ID;\
	if (!rlist->rules[id]->val) {\
		rlist->rules[id]->val = 1;\
		flag |= RET_FOUND_MASK;\
		/* Check each FOUND id */\
		mid = rlist->rules[id]->id_list;\
		while (mid) {\
			for (j=0; j<retnum; j++){\
				if (retsyms[j]->symbol == mid->val) {\
					retsyms[j]->val = 1;\
					break;\
				}\
			}\
			if (j == retnum) {\
				/* could not found this id */\
				fprintf(stderr, "Symbol ID stored in Matchstruct were corrupted.\n ");\
				return -1;\
			}\
			mid = mid->next;\
		}\
		value = 1;\
		for (i=0; i<rlist->rule_num; i++) {\
			value *= rlist->rules[i]->val;\
		}\
		if (value) {\
			return 1; /* when all rules found */\
		}\
	}

#define CHECK_SID_FRAG(offset, ID) \
	id = ID;\
	if (!rlist->rules[id]->val) {\
		if (rlist->rules[id]->end == -1) {\
			if (offset-begin>=rlist->rules[id]->start \
					&& offset+rlist->rules[id]->pat_len-1 <= stop) {\
				rlist->rules[id]->val = 1;\
				flag |= RET_FOUND_MASK;\
				/* Check each FOUND id */\
				mid = rlist->rules[id]->id_list;\
				while (mid) {\
					for (j=0; j<retnum; j++){\
						if (retsyms[j]->symbol == mid->val) {\
							retsyms[j]->val = 1;\
							break;\
						}\
					}\
					if (j == retnum) {\
						/* could not found this id */\
						fprintf(stderr, "Symbol ID stored in Matchstruct were corrupted.\n ");\
						return -1;\
					}\
					mid = mid->next;\
				}\
				value = 1;\
				for (i=0; i<rlist->rule_num; i++) {\
					value *= rlist->rules[i]->val;\
				}\
				if (value) {\
					return 1; /* when all rules found */\
				}\
			}\
		} else {\
			if (offset-begin>=rlist->rules[id]->start \
					&& offset-begin+rlist->rules[id]->pat_len <= rlist->rules[id]->end) {\
				rlist->rules[id]->val = 1;\
				flag |= RET_FOUND_MASK;\
				/* Check each FOUND id */\
				mid = rlist->rules[id]->id_list;\
				while (mid) {\
					fprintf(stderr, "%d found.\n", mid->val);\
					mid = mid->next;\
				}\
				value = 1;\
				for (i=0; i<rlist->rule_num; i++) {\
					value *= rlist->rules[i]->val;\
				}\
				if (value) {\
					return 2; /* when all rules found */\
				}\
			}\
		}\
	}


#define CHECK_SYMBOLID_LIST(id_list) \
	cpid = id_list;\
	while (cpid) {\
		CHECK_SID(cpid->val);\
		cpid = cpid->next;\
	}

#define CHECK_SYMBOLID_LIST_FRAG(start, id_list) \
	cpid = id_list;\
	while (cpid) {\
		CHECK_SID_FRAG(start, cpid->val);\
		cpid = cpid->next;\
	}


/* for 'swbm_match()' */
#define CHECK_SYMBOL_AND_CASE_FRAG(start, id_list) \
	chp = start;\
	pid = id_list;\
	while (pid) {\
		id = pid->val;\
		if (!rlist->rules[id]->val) {\
			if (rlist->rules[id]->nocase) {\
				CHECK_SID_FRAG(start, id);\
			} else {\
				if (chp >= begin) {\
					pre_ct = NULL;\
					ct = rlist->case_pre_tree->mtries[*chp];\
					while (chp <= stop) {\
						if (ct) {\
							pre_ct = ct;\
							if (ct->accept_flag) {\
								CHECK_SYMBOLID_LIST_FRAG(start, ct->target_id_list);\
							}\
							chp++;\
							ct = ct->branch[*chp];\
						} else {\
							break;\
						}\
					} /* 'chp>stop' never gonna happen. */\
				} /* else no need check case here, coz this would be done when first enter this packet.\
				     read codes where 'resume_type == 1'. */\
			}\
		}\
		pid = pid->next;\
	}



int swbm_match(			unsigned char *text, 
				int text_len, 
						struct ptn_list *ptnlist,  
					struct rule_list *rlist,
					int retnum, 
					 struct ret_sym **retsyms)
{
	/* for ID checking */
	register int id, value;
	register int_list_t *mid, *cpid, *pid;
	register int i, j;

	/* for SEARCH tree scaning */
	register unsigned char *hp, *endp;
	register struct trie *t;

	/* for REAR tree scaning */
	register unsigned char *rhp;
	register struct trie *rt, *pre_rt;

	/* for CASE tree scaning */
	register unsigned char *chp;
	register struct trie *ct, *pre_ct;

	/* keep a tmperal situation */
	register struct trie *tmp_trie;
	register struct trie *latest_rear_trie=NULL;
	register int len, flag=0;

	/* for efficiency */

	register struct trie **mtries;
	register unsigned int *bad;

	register unsigned char *begin, *stop, *tail;

	register struct trieobj *search_tree;
	register struct trieobj *rear_tree;
	//register struct rule_list *rlist;

	/*NOTE,NOTE,NOTE, wyong, 20090609 */	
	//xiayu 2005.12.2 when text is null
	if (text == NULL || text_len == 0) {
		return 0;
	}

	for (i=0; i<rlist->rule_num; i++) {
		rlist->rules[i]->val = 0;
	}


	search_tree = ptnlist->search_suf_tree;
	rear_tree = rlist->rear_pre_tree;
	
	mtries = search_tree->mtries;
	bad = search_tree->bad;
	
	/* initiation for SEARCH tree scaning */
	begin = text + rlist->min_start;
	if (-1 == rlist->max_end) {
		stop = begin + text_len - 1;
	} else {
		stop = begin + rlist->max_end;
	}
	len = stop - begin + 1;

	if (rear_tree->shortest >= len) {
		tail = begin;
	} else {
		tail = stop - rear_tree->shortest;
	}

	hp = endp = begin + search_tree->shortest - 1;
	t = mtries[*endp];

	while (endp <= tail) {
		if (!t) {
			flag &= ~REAR_TREE_SCANED_MASK;
			NEWEND;
		} else {
			hp--;

			if (t->accept_flag) {
				/* check the matched SymbolID */
				if (t->target_id_list) {
					if (rlist->case_check_flag) {
						CHECK_SYMBOL_AND_CASE_FRAG(hp+1, t->target_id_list);
					} else {
						CHECK_SYMBOLID_LIST(t->target_id_list);
					}
				}

				if (t->rear_id_list) {
					if (!(flag & REAR_TREE_SCANED_MASK)) {
						pre_rt = NULL; 
						rhp = endp + 1;
						rt = rear_tree->mtries[*rhp];

						while (rhp <= stop) {
							if (rt) {
								pre_rt = rt;
								if (rt->accept_flag) {
									if (t->IDlist[rt->state-1]) {
										if (rlist->case_check_flag) {
											CHECK_SYMBOL_AND_CASE_FRAG(hp+1, t->IDlist[rt->state-1]);
										} else {
											//CHECK_SYMBOLID_LIST(t->IDlist[rt->state-1]);
										}
									}
								}
								rhp++;
								rt = rt->branch[*rhp];
							} else {
								if (pre_rt) {
									latest_rear_trie = pre_rt;
								} else {
									latest_rear_trie = NULL;
								}
								flag |= REAR_TREE_SCANED_MASK;
								break;
							}
						}

					} else {
						/* check the found states */
						tmp_trie = latest_rear_trie;
						while (tmp_trie) {
							if (tmp_trie->IDlist[rt->state-1]) {
								if (rlist->case_check_flag) {
									CHECK_SYMBOL_AND_CASE_FRAG(hp+1, tmp_trie->IDlist[latest_rear_trie->state-1]);
								} else {
									CHECK_SYMBOLID_LIST(tmp_trie->IDlist[latest_rear_trie->state-1]);
								}
							}
							tmp_trie = tmp_trie->sub_trie;
						}
						
					}
				}
			}


			if (hp < begin) {
				flag &= ~REAR_TREE_SCANED_MASK;
				NEWEND;
			} else {
				t = t->branch[*hp];
			}
		}
	}

	return (flag & RET_FOUND_MASK);

}
