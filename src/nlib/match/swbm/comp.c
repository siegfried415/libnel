#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nlib/match/swbm.h"
#include "trie.h"


#define MAX_PAT_LEN 256



int compile_prefix_tree(unsigned char *pat, int len, int *state, struct trieobj *to, int_list_t *target_id_list, int *state_num)
{
	struct trie *trie, *sub_trie=NULL;
	unsigned char c;
	int i, curr_state=0;

	/* here's the head */
	if (len-1 > 0) {
		c = pat[0];
		if (to->mtries[c] == NULL) {
			to->mtries[c] = new_trie(to, NULL, NULL, NULL, NOT_ACCEPT, sub_trie, ++(*state));
			if (!to->mtries[c])
				return -1;
			(*state_num)++;
		} else {
			sub_trie = to->mtries[c];
		}
		trie = to->mtries[c];

		for (i=1; i<len-1; i++) {
			c = pat[i];
			if (trie->branch[c] == NULL) {
				trie->branch[c] = new_trie(to, NULL, NULL, NULL, NOT_ACCEPT, sub_trie, ++(*state));
				if (!trie->branch[c])
					return -1;
				(*state_num)++;
			} else {
				sub_trie = trie->branch[c];
			}
			trie = trie->branch[c];
		}
	}
	
	/* here's the tail */
	c = pat[len-1];
	if (len-1 == 0) {
		if (to->mtries[c] == NULL) {
			to->mtries[c] = new_trie(to, NULL, target_id_list, NULL, ACCEPTABLE, sub_trie, ++(*state));
			if (!to->mtries[c])
				return -1;
			(*state_num)++;
		} else {
			to->mtries[c]->accept_flag = 1;
			if (import_int_list(&to->mtries[c]->target_id_list, target_id_list) == -1)
				return -1;
			sub_trie = to->mtries[c];
		}
		curr_state = to->mtries[c]->state;
	} else { 
		if (trie->branch[c] == NULL) {
			trie->branch[c] = new_trie(to, NULL, target_id_list, NULL, ACCEPTABLE, sub_trie, ++(*state));
			if (!trie->branch[c])
				return -1;
			(*state_num)++;
		} else {
			trie->branch[c]->accept_flag = 1;
			if (!import_int_list(&trie->branch[c]->target_id_list, target_id_list) == -1)
				return -1;
			sub_trie = trie->branch[c];
		}
		curr_state = trie->branch[c]->state;
	}

	return curr_state;
}


int compile_prefix_tree_ex(unsigned char *pat, int len, struct trieobj *to, int_list_t *target_id_list)
{
	struct trie *trie;
	unsigned char c;
	int i;

	/* here's the head */
	if (len-1 > 0) {
		c = pat[0];
		if (to->mtries[c] == NULL) {
			to->mtries[c] = new_trie(to, NULL, NULL, NULL, NOT_ACCEPT, NULL, -1);
			if (!to->mtries[c])
				return -1;
		}
		trie = to->mtries[c];

		for (i=1; i<len-1; i++) {
			c = pat[i];
			if (trie->branch[c] == NULL) {
				trie->branch[c] = new_trie(to, NULL, NULL, NULL, NOT_ACCEPT, NULL, -1);
				if (!trie->branch[c])
					return -1;
			}
			trie = trie->branch[c];
		}
	}
	
	/* here's the tail */
	c = pat[len-1];
	if (len-1 == 0) {
		if (to->mtries[c] == NULL) {
			to->mtries[c] = new_trie(to, NULL, target_id_list, NULL, ACCEPTABLE, NULL, -1);
			if (!to->mtries[c])
				return -1;
		} else {
			to->mtries[c]->accept_flag = 1;
			if (!import_int_list(&to->mtries[c]->target_id_list, target_id_list) == -1)
				return -1;
		}
	} else { 
		if (trie->branch[c] == NULL) {
			trie->branch[c] = new_trie(to, NULL, target_id_list, NULL, ACCEPTABLE, NULL, -1);
			if (!trie->branch[c])
				return -1;
		} else {
			trie->branch[c]->accept_flag = 1;
			if (import_int_list(&trie->branch[c]->target_id_list, target_id_list) == -1)
				return -1;
		}
	}
	return 0;
}

int insert_IDlist(struct rule_list *rlist, int size, struct pat_tree_node *ptn, struct ptn_list *ptnlist)
{
	int_list_t *pid;
	struct rule_item **rules;
	
	rules = rlist->rules;
	if (ptn->accept_flag) {
		if (ptn->rear_id_list) {
			ptn->IDlist = (int_list_t **)calloc(size, sizeof(int_list_t *));
			if (!ptn->IDlist) {
				fprintf(stderr, "calloc error in 'insert_IDlist()'.\n");
				return -1;
			}
			
			pid = ptn->rear_id_list;
			while (pid)	 {		
				if (import_int_list(&(ptn->IDlist[rules[pid->val]->state-1]), rules[pid->val]->id_list) == -1)
					return -1;
				pid = pid->next;
			}
		}
	}
	if (ptn->ltree) {
		if (insert_IDlist(rlist, size, ptn->ltree, ptnlist) == -1)
			return -1;
	} 
	if (ptn->rtree) {
		if (insert_IDlist(rlist, size, ptn->rtree, ptnlist) == -1)
			return -1;
	} 
	return 0;	
}


int sub_compile_suffix_tree(struct pat_tree_node *ptn, struct trie *sub_trie, struct trie *trie, int level, struct trieobj *obj)
{
	struct trie *pt;
	unsigned char *pat, c;
	int len, i;

	if (ptn->str == NULL) {
		if (ptn->ltree) {
			if (sub_compile_suffix_tree(ptn->ltree, sub_trie, trie, level, obj) == -1)
				return -1;
		}
		if (ptn->rtree) {
			if (sub_compile_suffix_tree(ptn->rtree, sub_trie, trie, level, obj) == -1)
				return -1;
		}
	} else {
		pat = ptn->str;
		len = ptn->len;

		/* Fill in the bad-shift table */
		for (i=0; i<len; i++) {
			c = pat[i];
			if ((! obj->bad[c]) || (obj->bad[c] > i+level)) {
				obj->bad[c] = i+level;
				obj->bad[toupper(c)] = i+level;
			}
		}
		
		/* Fill in the struct-trie */
		for (i=0; i<len-1; i++) {
			c = pat[i];
			if (trie->branch[c] == NULL) {
				trie->branch[c] = new_trie(obj, NULL, NULL, NULL, NOT_ACCEPT, NULL, 0);
				if (trie->branch[c] == NULL)
					return -1;
			}
			trie = trie->branch[c];
		}
		
		/* here's the tail */
		c = pat[len-1];
		trie->branch[c] = new_trie(obj, ptn->IDlist, ptn->target_id_list, ptn->rear_id_list, ptn->accept_flag, sub_trie, 0);
		if (!trie->branch[c])
			return -1;
		trie = trie->branch[c];	

		if (ptn->rear_id_list) {
			pt = trie;
		} else {
			pt = sub_trie;
		}
		
		if (ptn->ltree) {
			if (sub_compile_suffix_tree(ptn->ltree, pt, trie, level+len, obj) == -1)
				return -1;
		}
		if (ptn->rtree) {
			if (sub_compile_suffix_tree(ptn->rtree, pt, trie, level+len, obj) == -1)
				return -1;
		}
	}
	return 0;
}

int compile_suffix_tree(struct pat_tree_node *ptn, struct trieobj *obj)
{
	struct trie *trie=NULL, *sub_trie=NULL;
	unsigned char *pat, c;
	int len, i;

	/* The begin str of patterns are sure not NULL! */
	if (ptn->str == NULL)
		return -1;
		
	pat = ptn->str;
	len = ptn->len;
	
	/* Fill in the bad-shift table */
	for (i=1; i<len; i++) {
		c = pat[i];
		if ((! obj->bad[c]) || (obj->bad[c] > i)) {
			obj->bad[c] = i;
			obj->bad[toupper(c)] = i;
		}
	}
	
	/* Fill in the struct-trie */
	/* here's the head */
	if (len-1 > 0) {
		c = pat[0];
		if (obj->mtries[c] == NULL) {
			obj->mtries[c] = new_trie(obj, NULL, NULL, NULL, NOT_ACCEPT, NULL, 0);
			if (obj->mtries[c] == NULL)
				return -1;
		}
		trie = obj->mtries[c];

		for (i=1; i<len-1; i++) {
			c = pat[i];
			if (trie->branch[c] == NULL) {
				trie->branch[c] = new_trie(obj, NULL, NULL, NULL, NOT_ACCEPT, NULL, 0);
				if (trie->branch[c] == NULL)
					return -1;
			}
			trie = trie->branch[c];
		}
	}
	
	/* here's the tail */
	c = pat[len-1];
	if (len-1 == 0) {
		if (obj->mtries[c] == NULL) {
			obj->mtries[c] = new_trie(obj, ptn->IDlist, ptn->target_id_list, ptn->rear_id_list, ptn->accept_flag, NULL, 0);
			if (obj->mtries[c] == NULL) 
				return -1;
		}
		trie = obj->mtries[c];
	} else { 
		if (trie->branch[c] == NULL) {
			trie->branch[c] = new_trie(obj, ptn->IDlist, ptn->target_id_list, ptn->rear_id_list, ptn->accept_flag, NULL, 0);
			if (trie->branch[c] == NULL) 
				return -1;
		}
		trie = trie->branch[c];
	}
	
	if (ptn->rear_id_list) {
		sub_trie = trie;
	} else {
		sub_trie = 0;
	}
	
	if (ptn->ltree) {
		if (sub_compile_suffix_tree(ptn->ltree, sub_trie, trie, len, obj) == -1)
			return -1;
	}
	if (ptn->rtree) {
		if (sub_compile_suffix_tree(ptn->rtree, sub_trie, trie, len, obj) == -1)
			return -1;
	}
	
	return 0;
}

int build_search_tree(struct ptn_list *ptnlist, int nocase) 
{
	int i, shortest;
	shortest = ptnlist->ptns[0]->minlen;
	for (i=1; i<ptnlist->ptn_num; i++) {
		SETMIN(shortest, ptnlist->ptns[i]->minlen);
	}
	ptnlist->search_suf_tree = new_trieobj(ALPHABETSIZE, shortest, nocase, HAS_BAD);
	if (!ptnlist->search_suf_tree)
		return -1;

	for (i=0; i<ptnlist->ptn_num; i++) {
		if (compile_suffix_tree(ptnlist->ptns[i], ptnlist->search_suf_tree) == -1) {
			free_trieobj(ptnlist->search_suf_tree);
			return -1;
		}
	}
	if (nocase)
		trie_nocase(ptnlist->search_suf_tree); 
	return 0;
}

int build_rear_tree(struct rule_list *rlist, struct ptn_list *ptnlist, int nocase)
{
	struct rule_item **rules;
	unsigned char pat[MAX_PAT_LEN];
	int i, j, len, shortest=65535, state=0, state_num=0;

	rules = rlist->rules;
	for (i=0; i<rlist->rule_num; i++) {
		SETMIN(shortest, rules[i]->rearcut); /* used in 'frag_swbm_match()' */
	}
	rlist->rear_pre_tree = new_trieobj(ALPHABETSIZE, shortest, nocase, NO_BAD);
	if (!rlist->rear_pre_tree)
		return -1;

	for (i=0; i<rlist->rule_num; i++) {
		if ((len = rules[i]->rearcut)) {
			if (nocase) {
				/* create a non-case-sensitive prefix tree */
				memcpy(pat, rules[i]->pat+rules[i]->pat_len-len, len*sizeof(unsigned char));
				for (j=0; j<len; j++) 
					pat[j] = tolower(pat[j]);
				rules[i]->state = compile_prefix_tree(pat, len,
					 &state, rlist->rear_pre_tree, NULL, &state_num);
				if (!rules[i]->state) {
					free_trieobj(rlist->rear_pre_tree);
					return -1;
				}
			} else {
				/* create a case-sensitive prefix tree */
				rules[i]->state = compile_prefix_tree(rules[i]->pat+rules[i]->pat_len-len, len,
					 &state, rlist->rear_pre_tree, NULL, &state_num);
				if (!rules[i]->state) {
					free_trieobj(rlist->rear_pre_tree);
					return -1;
				}
			}
		}
	}

	for (i=0; i<ptnlist->ptn_num; i++) {
		if (insert_IDlist(rlist, state_num, ptnlist->ptns[i], ptnlist) == -1) {
			free_trieobj(rlist->rear_pre_tree);
			return -1;
		}
	}
	return 0;
}


/* create a prefix tree for case-sensitive check. */
int build_case_tree(struct rule_list *rlist)
{
	struct trieobj *case_tree;
	struct rule_item **rules;
	int i;
	
	case_tree = new_trieobj(ALPHABETSIZE, 0, CASE_SENS, NO_BAD);
	if (!case_tree) {
		return -1;
	}
	
	rules = rlist->rules;
	for (i=0; i<rlist->rule_num; i++) {
	   	/* NOTE: we only compile case-sensitive ones into it. */
		if (!rules[i]->nocase) {
			if (compile_prefix_tree_ex(rules[i]->pat, rules[i]->pat_len, case_tree, rules[i]->id_list) == -1)
			{
				free_trieobj(case_tree);
				return -1;
			}
		}
	}

	rlist->case_pre_tree = case_tree;
	rlist->case_check_flag = 1;
	return 0;
}


void init_swbm_mem_pools (void)
{
	init_swbm_tree_mem_pool();
}
