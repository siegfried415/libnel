#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nlib/match/swbm.h"
#include "intlist.h"
#include "ptn.h"


/* 0-left branch, 1-right branch, -1-root */
void show_ptn(struct pat_tree_node *ptn, int tabnum, int flag, struct rule_item **rules)
{
	int_list_t *pid;
	int i;

	for (i=0; i<tabnum; i++) {
		fprintf(stderr, "\t");
	}
	if (flag == 1) {
		fprintf(stderr, "R ");
	} else if (flag == 0) {
		fprintf(stderr, "L ");
	}
	if (ptn) {
		if (ptn->str) {
			for (i=0; i<ptn->len; i++) {
				fprintf(stderr, "%c", ptn->str[i]);
			}
		} else {
			fprintf(stderr, "NULL");
		} 

		//if (ptn->accept_flag) 
		{
			fprintf(stderr, " * ");
			if (ptn->target_id_list) {
				fprintf(stderr, "(targetID) ");
				pid = ptn->target_id_list;
				while (pid) {
					fprintf(stderr, "%d ", pid->val);
					pid = pid->next;
				}
			}
			if (ptn->rear_id_list) {
				fprintf(stderr, "(rearID) ");
				pid = ptn->rear_id_list;
				while (pid) {
					fprintf(stderr, " %d", pid->val);
					pid = pid->next;
				}
				fprintf(stderr, "\n");

#if 0
				if (rules) {
					pid = ptn->rear_id_list;
					while (pid) {
						for (i=0; i<tabnum+2; i++) {
							fprintf(stderr, "\t");
						}
						state = rules[pid->val]->state;
						fprintf(stderr, "%d %d %p\n", state-1, ptn->resume_items[state-1]->level1_state, ptn->resume_items[state-1]->level2_trie);
						pid = pid->next;
					}
				}
#endif				
			} else {
				fprintf(stderr, "\n");
			}
		}
		fprintf(stderr, "\n");

		if (ptn->ltree) {
			show_ptn(ptn->ltree, tabnum+1, 0, rules);
		} else {
			for (i=0; i<tabnum+1; i++) {
				fprintf(stderr, "\t");
			}
			fprintf(stderr, "L none");
		}
		fprintf(stderr, "\n");
		if (ptn->rtree) {
			show_ptn(ptn->rtree, tabnum+1, 1, rules);
		} else {
			for (i=0; i<tabnum+1; i++) {
				fprintf(stderr, "\t");
			}
			fprintf(stderr, "R none");
		}	
		fprintf(stderr, "\n");
	} else {
		fprintf(stderr, "none\n");
	}
	return;
}


void free_ptn(struct pat_tree_node *ptn)
{
	if (!ptn) return;

	if (ptn->ltree)
		free_ptn(ptn->ltree);
	if (ptn->rtree)
		free_ptn(ptn->rtree);

	if (ptn->pat_id_list)
		free_int_list(ptn->pat_id_list);
	if (ptn->match_id_list)
		free_int_list(ptn->match_id_list);

	free(ptn->str);

	if (ptn->target_id_list)
		free_int_list(ptn->target_id_list);
	if (ptn->rear_id_list)
		free_int_list(ptn->rear_id_list);
	if (ptn->IDlist)
		free(ptn->IDlist);

	free(ptn);
	return;
}

void free_ptn_list(struct ptn_list *ptnlist)
{
	int i;
	if (!ptnlist) return;

	for (i=0; i<ptnlist->ptn_num; i++) {
		if (ptnlist->ptns[i])
			free_ptn(ptnlist->ptns[i]);
	}
	free(ptnlist->ptns);

	if (ptnlist->search_suf_tree)
		free_trieobj(ptnlist->search_suf_tree);

	free(ptnlist);
	return;
}


struct pat_tree_node *new_ptn(struct pat_tree_node *ltree, struct pat_tree_node *rtree, int minlen, int maxlen, int len, char *str, int_list_t *pat_id_list, int_list_t *match_id_list)
{
	struct pat_tree_node *ptn;
	ptn = (struct pat_tree_node *)calloc(1, sizeof(struct pat_tree_node));
	if (ptn == NULL) {
		fprintf(stderr, "calloc error in 'new_ptn()'.\n");
		return NULL;
	}
	ptn->ltree = ltree;
	ptn->rtree = rtree;
	ptn->minlen = minlen;
	ptn->maxlen = maxlen;
	ptn->len = len;
	if (len != 0) {
		ptn->str = (char *)malloc(len * sizeof(char));
		if (ptn->str == NULL) {
			fprintf(stderr, "malloc error in 'new_ptn()'.\n");
			free_ptn(ptn);
			return NULL;
		}
		memcpy(ptn->str, str, len);
	}
	ptn->pat_id_list = pat_id_list;	
	ptn->match_id_list = match_id_list;	
	return ptn;
}

int dup_ptn(struct pat_tree_node **newptn, struct pat_tree_node *oldptn)
{
	struct pat_tree_node *pl, *pr;
	int_list_t *pat_id_list, *match_id_list;

	if (oldptn == NULL) {
		*newptn = NULL;
		return 0;
	}
	
	if (dup_ptn(&pl, oldptn->ltree) == -1)
		return -1;
		
	if (dup_ptn(&pr, oldptn->rtree) == -1)
		goto Error;

	pat_id_list = dup_int_list(oldptn->pat_id_list);
	if (!pat_id_list)
		goto Error;

	match_id_list = dup_int_list(oldptn->match_id_list);
	if (!match_id_list)
		goto Error;

	*newptn = new_ptn(pl, pr, oldptn->minlen, oldptn->maxlen, oldptn->len, oldptn->str, pat_id_list, match_id_list);
	if (*newptn == NULL)
		goto Error;

	return 0;

Error:
	free_ptn(pl);
	free_ptn(pr);
	free_int_list(pat_id_list);
	free_int_list(match_id_list);

	return -1;
}

struct pat_tree_node *merge_ptn(struct pat_tree_node *pl, struct pat_tree_node *pr, char *str, int len, int_list_t *pat_id_list, int_list_t *match_id_list)
{
	struct pat_tree_node *ptn;
	int_list_t *plist, *mlist;
	int minlen, maxlen;

	minlen = maxlen = len; 
	if (pl && pr) {		
		if (pl->minlen < pr->minlen)
			minlen += pl->minlen;
		else 
			minlen += pr->minlen;

		if (pl->maxlen > pr->maxlen)
			maxlen += pl->maxlen;
		else 
			maxlen += pr->maxlen;

		if (!pat_id_list) {
			if (pl->pat_id_list && pr->pat_id_list) {
				plist = cat_int_list(pl->pat_id_list, pr->pat_id_list);
				if (plist == NULL)
					return NULL;
			}
		} else {
			plist = pat_id_list;
		}

		if (!match_id_list) {
			mlist = cat_int_list(pl->match_id_list, pr->match_id_list);
			if (mlist == NULL) {
				if (!pat_id_list) /* make sure 'plist' was allocated inside this func */
					free_int_list(plist);
				return NULL;
			}
		} else {
			mlist = match_id_list;
		}

	} else if (pl) {
		maxlen += pl->maxlen;
		if (!pat_id_list) {
			plist = dup_int_list(pl->pat_id_list);
			if (plist == NULL)
				return NULL;
		} else {
			plist = pat_id_list;
		}
		if (!mlist) {
			mlist = dup_int_list(pl->match_id_list);
			if (mlist == NULL) {
				if (!pat_id_list) /* make sure 'plist' was allocated inside this func */
					free_int_list(plist);
				return NULL;
			}
		} else {
			mlist = match_id_list;
		}
	} else if (pr) {
		maxlen += pr->maxlen;
		if (!pat_id_list) {
			plist = dup_int_list(pr->pat_id_list);
			if (plist == NULL)
				return NULL;
		} else {
			plist = pat_id_list;
		}
		if (!match_id_list) {
			mlist = dup_int_list(pr->match_id_list);
			if (mlist == NULL) {
				if (!pat_id_list) /* make sure 'plist' was allocated inside this func */
					free_int_list(plist);
				return NULL;
			}
		} else {
			mlist = match_id_list;
		}
	} else {
		plist = pat_id_list;
		mlist = match_id_list;
	}
		
	ptn = new_ptn(pl, pr, minlen, maxlen, len, str, plist, mlist);
	if (ptn == NULL) {
		if (!pat_id_list) /* make sure 'plist' was allocated inside this func */
			free_int_list(plist);
		if (!match_id_list) /* make sure 'mlist' was allocated inside this func */
			free_int_list(mlist);
		return NULL;
	}

	return ptn;
}


struct pat_tree_node *merge_new_ptn(struct pat_tree_node *pa, struct pat_tree_node *pb, char *str, int len, int_list_t *pat_id_list, int_list_t *match_id_list)
{
	struct pat_tree_node *pl, *pr, *ptn;
	if (pa == NULL) {
		pl = NULL;
		if (dup_ptn(&pr, pb) == -1)
			return NULL;
	} else if (pb == NULL) {
		if (dup_ptn(&pl, pa) == -1)
			return NULL;
		pr = NULL;
	} else {
		if (dup_ptn(&pl, pa) == -1)
			return NULL;
		if (dup_ptn(&pr, pb) == -1)
			return NULL;
	}

	ptn = merge_ptn(pl, pr, str, len, pat_id_list, match_id_list);
	if (ptn == NULL) {
		free_ptn(pl);
		free_ptn(pr);
		return NULL;
	}
	return ptn;
}


/* if success, return value is equal to 'newcount' 
   if failed, return -1 */
int cleanup_ptn(struct ptn_list *ptnlist, int newcount)
{
	int i, j;
	struct pat_tree_node **ptns;
	if (newcount > ptnlist->ptn_num) {
		fprintf(stderr, "input error in 'cleanup_ptn()'.\n");
		return -1;
	}
	ptns = ptnlist->ptns;
	j = (ptnlist->ptn_num)-1;
	for (i=0; i<j; i++) {
		if (ptns[i] != NULL)
			continue;
		for ( ; j>i; j--) {
			if (ptns[j] == NULL)
				continue;
			ptns[i] = ptns[j];
			ptns[j] = NULL;
			break;
		}
	}
	ptnlist->ptn_num = newcount;
	ptnlist->ptns = (struct pat_tree_node **)realloc(ptnlist->ptns, newcount * sizeof(struct pat_tree_node *));
	if (!ptnlist->ptns) {
		fprintf(stderr, "realloc error in 'cleanup_ptn()'.\n");
		return -1;
	}
	return newcount;
}
