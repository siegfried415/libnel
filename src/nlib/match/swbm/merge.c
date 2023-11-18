/*
 * swbm_merge.c
 * $Id: swbm_merge.c,v 1.2 2006/03/17 05:50:16 wyong Exp $
 */


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "nlib/match/swbm.h"

#if 0
#include "ptn.h"
#include "merge.h"
#endif
#include "same.h"


int compPatlen(void *a, void *b)
{
	struct pat_tree_node **ppa, **ppb;
	ppa = (struct pat_tree_node **)a;
	ppb = (struct pat_tree_node **)b;

	if ((*ppa)->len > (*ppb)->len) {
		return 1;
	} else if ((*ppa)->len == (*ppb)->len) {
		return 0;
	} else
		return -1;
}

#if 0
int premerge_pat(struct ptn_list *ptnlist, struct rule_list *rlist)
{
	int_list_t *pid, *pat_id_list, *match_id_list;
	int i, j, count, len, ptnlen;
	char temp; 
	struct pat_tree_node *tmpptn, *tmpsub; 
	struct pat_tree_node **ptns = ptnlist->ptns;
	struct rule_item **rules = rlist->rules;
	
	/* Sort the pattern list according to string length, 
		for the following merging. */
	qsort(ptns, ptnlist->ptn_num, sizeof(struct pat_tree_node *), (void *)compPatlen);
	
	/* Delete some of the origin patterns, which r too much short
		also won't lead to "false negtive"(wu4 bao4). */
	count = ptnlist->ptn_num;
	for (i=0; i<ptnlist->ptn_num; i++) {
		if (!ptns[i]) continue;
		if (ptns[i]->len > MINORIGINPATLEN) {
			/* when no pats shorter than 'MINORIGINPATLEN', we break */
			break;
		}
		for (j=i+1; j<ptnlist->ptn_num; j++) {
			if (ptns[j]->pat_id_list->val == ptns[i]->pat_id_list->val) {
				if (ptns[j]->len > MINORIGINPATLEN) {
					free_ptn(ptns[i]);
					ptns[i] = NULL;
					--count;
					break;
				}
			}
		}
	}
	/* Do some clean up things here */
	if (count < ptnlist->ptn_num) {
		cleanup_ptn(ptnlist, count);
	}

	/* Reverse the char order. */	
	for (i=0; i<count; i++) {
		len = ptns[i]->len;
		for (j=0; j<len/2; j++) {
			temp = ptns[i]->str[j];
			ptns[i]->str[j] = ptns[i]->str[len-j-1];
			ptns[i]->str[len-j-1] = temp;
		}
	}

	/* Sort the pattern list according to string length, 
		for the following merging. */
	qsort(ptns, count, sizeof(struct pat_tree_node *), (void *)compPatlen);

	/* Pre-merge the ptns with the same suffix, 
	   including totally the same. */
	count = ptnlist->ptn_num;
	for (i=0; i<ptnlist->ptn_num; i++) {
		if (ptns[i] == NULL)
			continue;
		for (j=i+1; j<ptnlist->ptn_num; j++) {
			if (ptns[j] == NULL)
				continue;
			if (memcmp(ptns[i]->str, ptns[j]->str, ptns[i]->len) == 0) {
				if (ptns[i]->len == ptns[j]->len) {
					insert_int_list(&ptns[i]->pat_id_list, ptns[j]->pat_id_list->val);
					insert_int_list(&ptns[i]->match_id_list, ptns[j]->match_id_list->val);
					pid = ptns[j]->match_id_list;
					while (pid) {
						rules[pid->val]->headcut = ptns[j]->len - ptns[i]->len;
						pid = pid->next;
					}
				} else {
					pat_id_list = dup_int_list(ptns[j]->pat_id_list);
					match_id_list = dup_int_list(ptns[j]->match_id_list);
					ptnlen = ptns[j]->len - ptns[i]->len;
					tmpsub = new_ptn(NULL, NULL, ptnlen, ptnlen, ptnlen, 
								ptns[j]->str+ptns[i]->len, pat_id_list, match_id_list);
							
					pat_id_list = cat_int_list(ptns[i]->pat_id_list, ptns[j]->pat_id_list);
					match_id_list = cat_int_list(ptns[i]->match_id_list, ptns[j]->match_id_list);
					tmpptn = new_ptn(tmpsub, NULL, ptns[i]->len, ptns[j]->len+tmpsub->maxlen, ptns[i]->len, ptns[i]->str, pat_id_list, match_id_list);
				
					free_ptn(ptns[i]);
					ptns[i] = tmpptn;

				}
				free_ptn(ptns[j]);
				ptns[j] = NULL;
				--count;
			}
		}
	}
	/* Do some clean up things here */
	if (count < ptnlist->ptn_num) {
		cleanup_ptn(ptnlist, count);
	}
	return 0;
}
#endif

struct ptn_list *prepare_pat(struct rule_list *rlist, int nocase)
{
	struct ptn_list	*ptnlist;
	struct pat_tree_node **ptns;
	int_list_t *match_id_list=NULL, *pat_id_list=NULL;
	struct rule_item **rules;
	unsigned char pat[256];
	int i, j, len, rule_num;
	char temp;
	
	rule_num = rlist->rule_num;
	rules = rlist->rules;
	ptnlist = (struct ptn_list *)calloc(1, sizeof(struct ptn_list));
	if (!ptnlist) {
		fprintf(stderr, "calloc error in 'prepare_pat()'.\n");
		return NULL;
	}
	ptnlist->ptn_num = rule_num;
	ptnlist->ptns = (struct pat_tree_node **)calloc(rule_num, sizeof(struct pat_tree_node *));
	if (!ptnlist->ptns) {
		fprintf(stderr, "calloc error in 'prepare_pat()'.\n");
		free_ptn_list(ptnlist);
		return NULL;
	}
	for (i=0; i<rule_num; i++) {
		match_id_list = new_int_list(i, NULL);
		if (match_id_list == NULL) {
			free_ptn_list(ptnlist);
			return NULL;
		}

		/* NOTE, NOTE, NOTE. xiayu: felt already no need to reserve 'pat_id_list' */
		pat_id_list = new_int_list(i, NULL);
		if (pat_id_list == NULL) {
			free_int_list(match_id_list);
			free_ptn_list(ptnlist);
			return NULL;
		}
			
		len = rules[i]->pat_len;
		if (nocase) {
			memcpy(pat, rules[i]->pat, len * sizeof(unsigned char));
			for (j=0; j<len; j++)
				pat[j] = tolower(pat[j]);
			ptnlist->ptns[i] = new_ptn(NULL, NULL, len, len, len, pat, pat_id_list, match_id_list);
			if (ptnlist->ptns[i] == NULL) {
				free_int_list(match_id_list);
				free_int_list(pat_id_list);
				free_ptn_list(ptnlist);
				return NULL;
			}
		} else {
			ptnlist->ptns[i] = new_ptn(NULL, NULL, len, len, len, rules[i]->pat, pat_id_list, match_id_list);
			if (ptnlist->ptns[i] == NULL) {
				free_int_list(match_id_list);
				free_int_list(pat_id_list);
				free_ptn_list(ptnlist);
				return NULL;		
			}
		}

	}
	//premerge_pat(ptnlist, rlist);

	/* Reverse the char order. */	
	ptns = ptnlist->ptns;
	for (i=0; i<rule_num; i++) {
		len = ptns[i]->len;
		for (j=0; j<len/2; j++) {
			temp = ptns[i]->str[j];
			ptns[i]->str[j] = ptns[i]->str[len-j-1];
			ptns[i]->str[len-j-1] = temp;
		}
	}
	return ptnlist;
}

int submerge(struct pat_tree_node **newptn, 
			 int *same, 
			 struct pat_tree_node *pa, 
			 struct pat_tree_node *pb, 
			 char *str,
			 int len);

int mergekern(struct pat_tree_node **ptn, 
			  int *same,
			  struct pat_tree_node *pa, 
			  struct pat_tree_node *pb, 
			  int ahead, 
			  int bhead, 
			  int len)
{
	struct pat_tree_node *pl, *pr;
	int_list_t *pat_id_list, *match_id_list;
	int retval, ptnlen;

	pat_id_list = match_id_list = NULL;
	*ptn = pl = pr = NULL;

	if (ahead+len < pa->len && bhead+len < pb->len) {

		/* The most simple one */
		if (pa->ltree || pa->rtree) {

			pat_id_list = dup_int_list(pa->pat_id_list);
			if (!pat_id_list)
				goto Error;
			match_id_list = dup_int_list(pa->match_id_list);
			if (!match_id_list)
				goto Error;

			pl = merge_new_ptn(pa->ltree, pa->rtree, 
					pa->str+ahead+len, 
					pa->len-ahead-len,
					pat_id_list, match_id_list);
			if (pl == NULL)
				goto Error;

			pat_id_list = match_id_list = NULL;

		} else {

			pat_id_list = dup_int_list(pa->pat_id_list);
			if (!pat_id_list)
				goto Error;
			match_id_list = dup_int_list(pa->match_id_list);
			if (!match_id_list)
				goto Error;
			ptnlen = pa->len - ahead - len;
			
			pl = new_ptn(NULL, NULL, ptnlen, ptnlen, 
				ptnlen, pa->str+ahead+len, 
				pat_id_list, match_id_list);
			if (!pl)
				goto Error;

			pat_id_list = match_id_list = NULL;
		}
		
		if (pb->ltree || pb->rtree) {

			pat_id_list = dup_int_list(pb->pat_id_list);
			if (!pat_id_list)
				goto Error;
			match_id_list = dup_int_list(pb->match_id_list);
			if (!match_id_list)
				goto Error;

			pr = merge_new_ptn(pb->ltree, pb->rtree, 
					pb->str+bhead+len, 
					pb->len-bhead-len,
					pat_id_list, match_id_list);
			if (pr == NULL)
				goto Error;

			pat_id_list = match_id_list = NULL;

		} else {

			pat_id_list = dup_int_list(pb->pat_id_list);
			if (!pat_id_list)
				goto Error;
			match_id_list = dup_int_list(pb->match_id_list);
			if (!match_id_list)
				goto Error;

			ptnlen = pb->len - bhead - len;
			pr = new_ptn(NULL, NULL, ptnlen, ptnlen, 
				ptnlen, pb->str+bhead+len, 
				pat_id_list, match_id_list);
			if (pr == NULL)
				goto Error;

			pat_id_list = match_id_list = NULL;
		}

		*ptn = merge_ptn(pl, pr, pa->str+ahead, len, NULL, NULL);
		if (*ptn == NULL)
			goto Error;

	} else if (ahead+len == pa->len && bhead+len < pb->len) {

		ptnlen = pb->len - bhead - len;
		if (pa->ltree || pa->rtree) {

			if (pa->ltree && pa->rtree) {

				retval = submerge(&pl, same, pa->ltree, pb, pb->str+bhead+len, ptnlen);
				if (retval == -1)
					goto Error;

				if (pl) { /* Merged in ltree */ 
					if (dup_ptn(&pr, pa->rtree) == -1)
						goto Error;

				} else {

					retval = submerge(&pr, same, pa->rtree, pb, pb->str+bhead+len, ptnlen);
					if (retval == -1)
						goto Error;

					if (pr) { /* Merged in rtree */ 
						if (dup_ptn(&pl, pa->ltree) == -1)
							goto Error;
					} else {
						/* Neither merged */
						pl = merge_new_ptn(pa->ltree, pa->rtree,
								NULL, 0, NULL, NULL);
						if (pl == NULL)
							goto Error;

						if (pb->ltree || pb->rtree) {

							pat_id_list = dup_int_list(pb->pat_id_list);
							if (!pat_id_list)
								goto Error;
							match_id_list = dup_int_list(pb->match_id_list);
							if (!match_id_list)
								goto Error;

							pr = merge_new_ptn(pb->ltree, pb->rtree, pb->str+bhead+len, ptnlen, pat_id_list, match_id_list);
							if (!pr)
								goto Error;

							pat_id_list = match_id_list = NULL;

						} else {

							pat_id_list = dup_int_list(pb->pat_id_list);
							if (!pat_id_list)
								goto Error;
							match_id_list = dup_int_list(pb->match_id_list);
							if (!match_id_list)
								goto Error;

							pr = new_ptn(NULL, NULL, ptnlen, ptnlen, ptnlen, pb->str+bhead+len, pat_id_list, match_id_list); 
							if (!pr)
								goto Error;

							pat_id_list = match_id_list = NULL;

						}
					}
				}

			} else if (pa->ltree && !pa->rtree) {

				retval = submerge(&pl, same, pa->ltree, pb, pb->str+bhead+len, ptnlen);
				if (retval == -1)
					goto Error;

				if (pl) {
					pr = NULL;
				} else {
					if (dup_ptn(&pl, pa->ltree) == -1)
						goto Error;
					if (pb->ltree || pb->rtree) {

						pat_id_list = dup_int_list(pb->pat_id_list);
						if (!pat_id_list)
							goto Error;
						match_id_list = dup_int_list(pb->match_id_list);
						if (!match_id_list)
							goto Error;

						pr = merge_new_ptn(pb->ltree, pb->rtree, pb->str+bhead+len, ptnlen, pat_id_list, match_id_list);
						if (!pr)
							goto Error;

						pat_id_list = match_id_list = NULL;

					} else {
						pat_id_list = dup_int_list(pb->pat_id_list);
						if (!pat_id_list)
							goto Error;
						match_id_list = dup_int_list(pb->match_id_list);
						if (!match_id_list)
							goto Error;

						pr = new_ptn(NULL, NULL, ptnlen, ptnlen, ptnlen, pb->str+bhead+len, pat_id_list, match_id_list); 
						if (!pr)
							goto Error;

						pat_id_list = match_id_list = NULL;
					}
				}				

			} else { /* !pa->ltree && pa->rtree */

				retval = submerge(&pr, same, pa->rtree, pb, pb->str+bhead+len, ptnlen);
				if (retval == -1)
					goto Error;
				if (pr) {
					pl = NULL;
				} else {
					if (dup_ptn(&pr, pa->rtree) == -1)
						goto Error;
					if (pb->ltree || pb->rtree) {
						pat_id_list = dup_int_list(pb->pat_id_list);
						if (!pat_id_list)
							goto Error;
						match_id_list = dup_int_list(pb->match_id_list);
						if (!match_id_list)
							goto Error;

						pl = merge_new_ptn(pb->ltree, pb->rtree, pb->str+bhead+len, ptnlen, pat_id_list, match_id_list);
						if (!pl)
							goto Error;

						pat_id_list = match_id_list = NULL;

					} else {
						pat_id_list = dup_int_list(pb->pat_id_list);
						if (!pat_id_list)
							goto Error;
						match_id_list = dup_int_list(pb->match_id_list);
						if (!match_id_list)
							goto Error;

						pl = new_ptn(NULL, NULL, ptnlen, ptnlen, ptnlen, pb->str+bhead+len, pat_id_list, match_id_list); 
						if (!pl)
							goto Error;

						pat_id_list = match_id_list = NULL;
					}
				}				
			} 
			*ptn = merge_ptn(pl, pr, pa->str+ahead, pa->len-ahead, NULL, NULL);
			if (*ptn == NULL)
				goto Error;
		} else {
			pat_id_list = dup_int_list(pb->pat_id_list);
			if (!pat_id_list)
				goto Error;
			match_id_list = dup_int_list(pb->match_id_list);
			if (!match_id_list)
				goto Error;

			pr = merge_new_ptn(pb->ltree, pb->rtree,
					pb->str+bhead+len, ptnlen,
					pat_id_list, match_id_list);
			if (!pr)
				goto Error;
				
			pat_id_list = match_id_list = NULL;

			pat_id_list = cat_int_list(pa->pat_id_list, pb->pat_id_list);
			if (!pat_id_list)
				goto Error;
			match_id_list = cat_int_list(pa->match_id_list, pb->match_id_list);
			if (!match_id_list)
				goto Error;

			*ptn = new_ptn(NULL, pr, len, len+pr->maxlen, len, pa->str+ahead,
					pat_id_list, match_id_list);
			if (*ptn == NULL)
				goto Error;

			pat_id_list = match_id_list = NULL;

			(*ptn)->minlen = len; /* since 'pa' has reached the end */	
			if (import_int_list(&(*ptn)->pat_id_list, pa->pat_id_list) == -1)
				goto Error;
			if (import_int_list(&(*ptn)->match_id_list, pa->match_id_list) == -1)
				goto Error;
		}

	} else if (ahead+len < pa->len && bhead+len == pb->len) {

		ptnlen = pa->len - ahead - len;
		if (pb->ltree || pb->rtree) {

			if (pb->ltree && pb->rtree) {
				retval = submerge(&pl, same,
						pb->ltree, pa,
						pa->str+ahead+len,
						ptnlen);
				if (retval == -1)
					goto Error;

				if (pl) { /* Merged in ltree */ 
					if (dup_ptn(&pr, pb->rtree) == -1)
						goto Error;
				} else {
					retval = submerge(&pr, same, pb->rtree, pa, pa->str+ahead+len, ptnlen);
					if (retval == -1)
						goto Error;

					if (pr) { /* Merged in rtree */ 
						if (dup_ptn(&pl, pb->ltree) == -1)
							goto Error;
					} else { /* Neither merged */
						pl = merge_new_ptn(pb->ltree, pb->rtree, NULL, 0, NULL, NULL);
						if (pl == NULL)
							goto Error;

						if (pa->ltree || pa->rtree) {

							pat_id_list = dup_int_list(pa->pat_id_list);
							if (!pat_id_list)
								goto Error;
							match_id_list = dup_int_list(pa->match_id_list);
							if (!match_id_list)
								goto Error;

							pr = merge_new_ptn(pa->ltree, pa->rtree, pa->str+ahead+len, ptnlen, pat_id_list, match_id_list);
							if (!pr)
								goto Error;

							pat_id_list = match_id_list = NULL;

						} else {
							pat_id_list = dup_int_list(pa->pat_id_list);
							if (!pat_id_list)
								goto Error;
							match_id_list = dup_int_list(pa->match_id_list);
							if (!match_id_list)
								goto Error;

							pr = new_ptn(NULL, NULL, ptnlen, ptnlen, ptnlen, pa->str+ahead+len, pat_id_list, match_id_list); 
							if (!pr)
								goto Error;

							pat_id_list = match_id_list = NULL;
						}
					}
				}
			} else if (pb->ltree && !pb->rtree) {
				retval = submerge(&pl, same,
						pb->ltree, pa,
						pa->str+ahead+len,
						ptnlen);
				if (retval == -1)
					goto Error;
				if (pl) {
					pr = NULL;
				} else {
					if (dup_ptn(&pl, pb->ltree) == -1)
						goto Error;
					if (pa->ltree || pa->rtree) {
						pat_id_list = dup_int_list(pa->pat_id_list);
						if (!pat_id_list)
							goto Error;
						match_id_list = dup_int_list(pa->match_id_list);
						if (!match_id_list)
							goto Error;
						pr = merge_new_ptn(pa->ltree, pa->rtree, pa->str+ahead+len, ptnlen, pat_id_list, match_id_list);
						if (!pr)
							goto Error;
						pat_id_list = match_id_list = NULL;

					} else {
						pat_id_list = dup_int_list(pa->pat_id_list);
						if (!pat_id_list)
							goto Error;
						match_id_list = dup_int_list(pa->match_id_list);
						if (!match_id_list)
							goto Error;

						pr = new_ptn(NULL, NULL, ptnlen, ptnlen, ptnlen, pa->str+ahead+len, pat_id_list, match_id_list); 
						if (!pr)
							goto Error;

						pat_id_list = match_id_list = NULL;
					}
				}				

			} else { /* !pb->ltree && pb->rtree */
				retval = submerge(&pr, same, pb->rtree, pa, pa->str+ahead+len, ptnlen);
				if (retval == -1)
					goto Error;

				if (pr) {
					pl = NULL;
				} else {
					if (dup_ptn(&pr, pb->rtree) == -1)
						goto Error;

					if (pa->ltree || pa->rtree) {
						pat_id_list = dup_int_list(pa->pat_id_list);
						if (!pat_id_list)
							goto Error;
						match_id_list = dup_int_list(pa->match_id_list);
						if (!match_id_list)
							goto Error;

						pl = merge_new_ptn(pa->ltree, pa->rtree, pa->str+ahead+len, ptnlen, pat_id_list, match_id_list);
						if (!pl)
							goto Error;

						pat_id_list = match_id_list = NULL;

					} else {
						pat_id_list = dup_int_list(pa->pat_id_list);
						if (!pat_id_list)
							goto Error;
						match_id_list = dup_int_list(pa->match_id_list);
						if (!match_id_list)
							goto Error;

						pl = new_ptn(NULL, NULL, ptnlen, ptnlen, ptnlen, pa->str+ahead+len, pat_id_list, match_id_list);
						if (!pl)
							goto Error;

						pat_id_list = match_id_list = NULL;
					}
				}				
			} 
			*ptn = merge_ptn(pl, pr, pb->str+bhead, pb->len-bhead, NULL, NULL);
			if (*ptn == NULL)
				goto Error;

			pat_id_list = match_id_list = NULL;

			(*ptn)->minlen = len; /* since 'pb' has reached the end */	
			if (import_int_list(&(*ptn)->pat_id_list, pb->pat_id_list) == -1)
				goto Error;
			if (import_int_list(&(*ptn)->match_id_list, pb->match_id_list) == -1)
				goto Error;
		} else {
			pat_id_list = dup_int_list(pa->pat_id_list);
			if (!pat_id_list)
				goto Error;
			match_id_list = dup_int_list(pa->match_id_list);
			if (!match_id_list)
				goto Error;

			pl = merge_new_ptn(pa->ltree, pa->rtree,
					pa->str+ahead+len, ptnlen,
					pat_id_list, match_id_list);
			if (!pl)
				goto Error;

			pat_id_list = match_id_list = NULL;

			pat_id_list = cat_int_list(pa->pat_id_list, pb->pat_id_list);
			if (!pat_id_list)
				goto Error;
			match_id_list = cat_int_list(pa->match_id_list, pb->match_id_list);
			if (!match_id_list)
				goto Error;

			*ptn = new_ptn(pl, NULL, len, len+pl->maxlen,
					len, pa->str+ahead,
					pat_id_list, match_id_list);
			if (*ptn == NULL)
				goto Error;
		}

	} else if (ahead+len == pa->len && bhead+len == pb->len) {

		if ((!pa->ltree && !pa->rtree) || (!pb->ltree && !pb->rtree)) {
			pl = pr = NULL;
			if (!pa->ltree && !pa->rtree) {
				if (pb->ltree) {
					if (dup_ptn(&pl, pb->ltree) == -1)
						goto Error;
				}
				if (pb->rtree) {
					if (dup_ptn(&pr, pb->rtree) == -1)
						goto Error;
				}
			} else if (!pb->ltree && !pb->rtree) {
				if (pa->ltree) {
					if (dup_ptn(&pl, pa->ltree) == -1)
						goto Error;
				}
				if (pa->rtree) {
					if (dup_ptn(&pr, pa->rtree) == -1)
						goto Error;
				}
			}

			pat_id_list = cat_int_list(pa->pat_id_list, pb->pat_id_list);
			if (pat_id_list == NULL)
				goto Error;
			match_id_list = cat_int_list(pa->match_id_list, pb->match_id_list);
			if (match_id_list == NULL)
				goto Error;

			*ptn = merge_ptn(pl, pr, pa->str+ahead, len, pat_id_list, match_id_list);
			(*ptn)->minlen = len; /* this minlen should not be missed. */
					
		} else {
			if (pa->ltree && pa->rtree) {
				if (pb->ltree && pb->rtree) {
					retval = submerge(&pl, same, pa->ltree, pb->ltree, NULL, 0);
					if (retval == -1)
						goto Error;
					if (!pl) {
						retval = submerge(&pl, same, pa->ltree, pb->rtree, NULL, 0);
						if (retval == -1)
							goto Error;

						if (pl == NULL) {
							retval = submerge(&pr, same, pa->rtree, pb->rtree, NULL, 0);
							if (retval == -1)
								goto Error;

							if (pr == NULL) {
								retval = submerge(&pr, same, pa->rtree, pb->ltree, NULL, 0);
								if (retval == -1)
									goto Error;

								if (pr == NULL) {
									pl = merge_new_ptn(pa->ltree, pa->rtree, NULL, 0, NULL, NULL);
									if (pl == NULL)
										goto Error;
										
									pr = merge_new_ptn( pb->ltree, pb->rtree,	NULL, 0, NULL, NULL);
									if (pr == NULL)
										goto Error;
								} else {
									pl = merge_new_ptn( pa->ltree, pb->rtree, NULL, 0, NULL, NULL);
									if (pl == NULL)
										goto Error;
								} 
							} else {
								pl = merge_new_ptn(pa->ltree, pb->ltree, NULL, 0, NULL, NULL);
								if (pl == NULL)
									goto Error;
							}
						} else {
							retval = submerge(&pr, same, pa->rtree, pb->rtree, NULL, 0);
							if (retval == -1)
								goto Error;

							if (pr == NULL) {
								pr = merge_new_ptn(pa->rtree, pb->ltree, NULL, 0, NULL, NULL);
								if (pr == NULL)
									goto Error;
							}
						} 

					} else {
						retval = submerge(&pr, same, pa->rtree, pb->rtree, NULL, 0);
						if (retval == -1)
							goto Error;

						if (pr == NULL) {
							pr = merge_new_ptn(pa->rtree, pb->rtree, NULL, 0, NULL, NULL);
							if (pl == NULL)
								goto Error;
						}
					}
					pat_id_list = cat_int_list(pa->pat_id_list, pb->pat_id_list);
					if (pat_id_list == NULL)
						goto Error;
					match_id_list = cat_int_list(pa->match_id_list, pb->match_id_list);
					if (match_id_list == NULL)
						goto Error;

					*ptn = merge_ptn(pl, pr, pa->str+ahead, pa->len-ahead, pat_id_list, match_id_list);
					if (*ptn == NULL)
						goto Error;

				} else if (pb->ltree && !pb->rtree) {
					retval = submerge(&pl, same, pa->ltree, pb->ltree, NULL, 0);
					if (retval == -1)
						goto Error;
					if (!pl) {
						retval = submerge(&pr, same, pa->rtree, pb->ltree, NULL, 0);
						if (retval == -1)
							goto Error;

						if (pr == NULL) {
							pl = merge_new_ptn(pa->ltree, pa->rtree, NULL, 0, NULL, NULL);
							if (pl == NULL)
								goto Error;
								
							if (dup_ptn(&pr, pb->ltree) == -1)
								goto Error;
						} else {
							if (dup_ptn(&pl, pa->ltree) == -1)
								goto Error;
						} 
					}  
					pat_id_list = cat_int_list(pa->pat_id_list, pb->pat_id_list);
					if (pat_id_list == NULL)
						goto Error;
					match_id_list = cat_int_list(pa->match_id_list, pb->match_id_list);
					if (match_id_list == NULL)
						goto Error;

					*ptn = merge_ptn(pl, pr, pa->str+ahead, pa->len-ahead, pat_id_list, match_id_list);
					if (*ptn == NULL)
						goto Error;
						
				} else { /* !pb->ltree && pb->rtree */
					retval = submerge(&pl, same, pa->ltree, pb->rtree, NULL, 0);
					if (retval == -1)
						goto Error;

					if (pl == NULL) {
						retval = submerge(&pr, same, pa->rtree, pb->rtree, NULL, 0);
						if (retval == -1)
							goto Error;

						if (pr == NULL) {
							pl = merge_new_ptn( pa->ltree, pa->rtree, NULL, 0, NULL, NULL);
							if (pl == NULL)
								goto Error;
								
							if (dup_ptn(&pr, pb->ltree) == -1)
								goto Error;
						} else {
							if (dup_ptn(&pl, pa->ltree) == -1)
								goto Error;
						} 
					} else {
						retval = submerge(&pr, same, pa->rtree, pb->rtree, NULL, 0);
						if (retval == -1)
							goto Error;

						if (pr == NULL) {
							pr = merge_new_ptn(pa->rtree, pb->ltree, NULL, 0, NULL, NULL);
							if (pr == NULL)
								goto Error;
						}
					}

					pat_id_list = cat_int_list(pa->pat_id_list, pb->pat_id_list);
					if (pat_id_list == NULL)
						goto Error;
					match_id_list = cat_int_list(pa->match_id_list, pb->match_id_list);
					if (match_id_list == NULL)
						goto Error;

					*ptn = merge_ptn(pl, pr, pa->str+ahead, pa->len-ahead, pat_id_list, match_id_list);
					if (*ptn == NULL)
						goto Error;
				}

			} else if (pa->ltree && !pa->rtree) {
				if (pb->ltree && pb->rtree) {
					retval = submerge(&pl, same, pa->ltree, pb->ltree, NULL, 0);
					if (retval == -1)
						goto Error;
					if (!pl) {
						retval = submerge(&pl, same, pa->ltree, pb->rtree, NULL, 0);
						if (retval == -1)
							goto Error;

						if (pl == NULL) {
							if (dup_ptn(&pl, pa->ltree) == -1)
								goto Error;
							pr = merge_new_ptn( pb->ltree, pb->rtree,	NULL, 0, NULL, NULL);
							if (pr == NULL)
								goto Error;
						} else {
							if (dup_ptn(&pr, pb->ltree) == -1)
								goto Error;
						} 
					} else {
						if (dup_ptn(&pr, pb->rtree) == -1)
							goto Error;
					}

					pat_id_list = cat_int_list(pa->pat_id_list, pb->pat_id_list);
					if (pat_id_list == NULL)
						goto Error;
					match_id_list = cat_int_list(pa->match_id_list, pb->match_id_list);
					if (match_id_list == NULL)
						goto Error;

					*ptn = merge_ptn(pl, pr, pa->str+ahead, pa->len-ahead, pat_id_list, match_id_list);
					if (*ptn == NULL)
						goto Error;
				
				} else if (pb->ltree && !pb->rtree) {
					retval = submerge(&pl, same, pa->ltree, pb->ltree, NULL, 0);
					if (retval == -1)
						goto Error;
					if (!pl) {
						if (dup_ptn(&pl, pa->ltree) == -1)
							goto Error;
						if (dup_ptn(&pr, pb->ltree) == -1)
							goto Error;
					} else {
						pr = NULL;
					}

					pat_id_list = cat_int_list(pa->pat_id_list, pb->pat_id_list);
					if (pat_id_list == NULL)
						goto Error;
					match_id_list = cat_int_list(pa->match_id_list, pb->match_id_list);
					if (match_id_list == NULL)
						goto Error;

					*ptn = merge_ptn(pl, pr, pa->str+ahead, pa->len-ahead, pat_id_list, match_id_list);
					if (*ptn == NULL)
						goto Error;
				
				} else { /* !pb->ltree && pb->rtree */
					retval = submerge(&pl, same, pa->ltree, pb->rtree, NULL, 0);
					if (retval == -1)
						goto Error;

					if (pl == NULL) {
						if (dup_ptn(&pl, pa->ltree) == -1)
							goto Error;
						if (dup_ptn(&pr, pb->rtree) == -1)
							goto Error;
					} else {
						pr = NULL;
					} 

					pat_id_list = cat_int_list(pa->pat_id_list, pb->pat_id_list);
					if (pat_id_list == NULL)
						goto Error;
					match_id_list = cat_int_list(pa->match_id_list, pb->match_id_list);
					if (match_id_list == NULL)
						goto Error;

					*ptn = merge_ptn(pl, pr, pa->str+ahead, pa->len-ahead, pat_id_list, match_id_list);
					if (*ptn == NULL)
						goto Error;
				}
			} else { /* !pa->ltree && pa->rtree */
				if (pb->ltree && pb->rtree) {
					retval = submerge(&pr, same, pa->rtree, pb->rtree, NULL, 0);
					if (retval == -1)
						goto Error;

					if (pr == NULL) {
						retval = submerge(&pr, same, pa->rtree, pb->ltree, NULL, 0);
						if (retval == -1)
							goto Error;

						if (pr == NULL) {
							if (dup_ptn(&pl, pa->rtree) == -1)
								goto Error;
							pr = merge_new_ptn( pb->ltree, pb->rtree,	NULL, 0, NULL, NULL);
							if (!pr)
								goto Error;
						} else {
							if (dup_ptn(&pl, pb->rtree) == -1)
								goto Error;
						} 
					} else {
						if (dup_ptn(&pl, pb->rtree) == -1)
							goto Error;
					}

					pat_id_list = cat_int_list(pa->pat_id_list, pb->pat_id_list);
					if (pat_id_list == NULL)
						goto Error;
					match_id_list = cat_int_list(pa->match_id_list, pb->match_id_list);
					if (match_id_list == NULL)
						goto Error;
					*ptn = merge_ptn(pl, pr, pa->str+ahead, pa->len-ahead, pat_id_list, match_id_list);
					if (*ptn == NULL)
						goto Error;

				} else if (pb->ltree && !pb->rtree) {
					retval = submerge(&pr, same, pa->rtree, pb->ltree, NULL, 0);
					if (retval == -1)
						goto Error;

					if (!pr) {
						if (dup_ptn(&pl, pa->rtree) == -1)
							goto Error;
						if (dup_ptn(&pr, pb->ltree) == -1)
							goto Error;
					} else {
						pl = NULL;
					} 

					pat_id_list = cat_int_list(pa->pat_id_list, pb->pat_id_list);
					if (pat_id_list == NULL)
						goto Error;
					match_id_list = cat_int_list(pa->match_id_list, pb->match_id_list);
					if (match_id_list == NULL)
						goto Error;

					*ptn = merge_ptn(pl, pr, pa->str+ahead, pa->len-ahead, pat_id_list, match_id_list);
					if (*ptn == NULL)
						goto Error;
				
				} else { /* !pb->ltree && pb->rtree */
					retval = submerge(&pr, same, pa->rtree, pb->rtree, NULL, 0);
					if (retval == -1)
						goto Error;

					if (pr == NULL) {
						if (dup_ptn(&pl, pa->rtree) == -1)
							goto Error;
						if (dup_ptn(&pr, pb->rtree) == -1)
							goto Error;
					} else {
						pl = NULL;
					}
					pat_id_list = cat_int_list(pa->pat_id_list, pb->pat_id_list);
					if (pat_id_list == NULL)
						goto Error;
					match_id_list = cat_int_list(pa->match_id_list, pb->match_id_list);
					if (match_id_list == NULL)
						goto Error;
					*ptn = merge_ptn(pl, pr, pa->str+ahead, pa->len-ahead, pat_id_list, match_id_list);
					if (*ptn == NULL)
						goto Error;
				}
			}
				
		} 	/* End of both have subtrees */
		
	} 	/* End of ahead[len] == '\0' && bhead[len] == '\0' */

	return 0;

Error:
	free_int_list(pat_id_list);
	free_int_list(match_id_list);
	free(pl);
	free(pr);
	free(*ptn);
	return -1;
}

int submerge(struct pat_tree_node **newptn, 
			 int *same, 
			 struct pat_tree_node *pa, 
			 struct pat_tree_node *pb, 
			 char *str,
			 int len)
{
	int subsame, i;
	if (str == NULL) {
		str = pb->str;
		len = pb->len;
	}
	for (i=0; i<pa->len && i<len && pa->str[i] == str[i]; i++)
			continue;

	subsame = i;
	if (subsame == 0) {
		*newptn = NULL;
	} else {
		*same += subsame;
		if (mergekern(newptn, same, pa, pb, 0, str-pb->str, subsame) == -1)
			return -1;
	}
	
	return 0;
}

int patsame(struct pat_tree_node **ptn, int *same, 
		int *rcutflag,
		int *icutnum,	/* for horison pattern */
		int *jcutnum,	/* for vertical pattern */
		int shortest,	/* shortest pattern length */
		struct pat_tree_node *pa, 
		struct pat_tree_node *pb)
{
	int len, i, j, k;
	
	*ptn = NULL;
	*same = *rcutflag = *jcutnum = *icutnum = 0;
   	len = 0;
	for (i=0; i<pa->len; i++) {
		for (j=0; j<pb->len; j++) {
			if (pb->str[j] == pa->str[i])
				break;
		}
   		while (j < pb->len) {
			for (k=1; k+i<pa->len && k+j<pb->len
					&& pa->str[i+k]==pb->str[j+k]; ++k)
				continue;
			if (k > len) {
	    		len = k;
				*icutnum = i;
    			*jcutnum = j;
  		  	}
			++j;
			for ( ;j<pb->len; j++) {
				if (pb->str[j] == pa->str[i])
					break;
			}
		}
	}

	if (*icutnum != 0 || *jcutnum != 0)
		*rcutflag = 1;
		
	*same = len;

	if (len > 0) {
		if (mergekern(ptn, same, pa, pb, *icutnum, *jcutnum, len) == -1)
			return -1;
	}
	if ((*ptn) && (*ptn)->minlen < shortest) {
		free_ptn(*ptn);
		*ptn = NULL;
		*same = *rcutflag = *jcutnum = *icutnum = 0;
	}

	return 0;
}

int merge_ptnlist(int maxi, int maxj, int count, struct same_bit ***Similarity, struct max_item **Max, struct pat_tree_node **patterns)
{
	struct pat_tree_node *tmpptn;
	int_list_t *pid, *tmppid;
	int i, j, newcount=count;
	int oldi, oldj;
	
	/*** Save the merged ptn first of all ***/
	tmpptn = Similarity[maxi][maxj]->ptn;
	Similarity[maxi][maxj]->ptn = NULL;
		
	free(Similarity[maxi][maxj]);
	Similarity[maxi][maxj] = NULL;

	/*** Delete the redundant item of 'patterns' ***/
	for (i=0; i<count; i++) {
		if (i == maxi || i == maxj) {
			for (j=i+1; j<count; j++){
				/* Delete the horizon line */
				if (Similarity[i][j] != NULL) {
					free_same_bit(Similarity[i][j]);
					Similarity[i][j] = NULL;
				}
			}
			for (j=0; j<i; j++) {	
				/* Delete the vertical line */
				if (Similarity[j][i] != NULL) {
					free_same_bit(Similarity[j][i]);
					Similarity[j][i] = NULL;
				}
			}
			free_ptn(patterns[i]);
			patterns[i] = NULL;
			--newcount;
		} else {
			/* when id_list in 'pid' r all the same, 
			   or included in id_list of 'tmppid',
			   we delete the 'patterns[i]' */
			pid = patterns[i]->pat_id_list;
			while (pid != NULL) {
				tmppid = tmpptn->pat_id_list;
				while (tmppid) {
					if (tmppid->val == pid->val) {
						break;
					}
					tmppid = tmppid->next;
				}	
				if (tmppid == NULL)
					/* 'pid' has id_list, which is not 
					included in id_list of 'tmppid',
					so we cannot delete 'patterns[i]' */
					break;
				pid = pid->next;
			}					
			if (pid != NULL)
				/* not all id_list of 'pid' were found in 'tmppid' */
				continue;
			
			/* when 'tmppid' includes 'pid' 
			we should delete 'patterns[i]' */
			for (j=i+1; j<count; j++) {	
				/* Delete the horizon line */
				if (Similarity[i][j] != NULL) {
					free_same_bit(Similarity[i][j]);
					Similarity[i][j] = NULL;
				}
			}
			for (j=0; j<i; j++) {	
				/* Delete the vertical line */
				if (Similarity[j][i] != NULL) {
					free_same_bit(Similarity[j][i]);
					Similarity[j][i] = NULL;
				}
			}
			free_ptn(patterns[i]);
			patterns[i] = NULL;
			--newcount;
		}
	}
#if 0
	fprintf(stderr, "BEFORE\n");
	for (i=0; i<count; i++) {
		fprintf(stderr, "ptns[%d]\n", i);
		show_ptn(patterns[i], 1, -1, NULL);
	}
#endif

	/*** Refresh the 'Similarity' ***/
	for (i=0; i<newcount-1; i++) {
		if (patterns[i] == NULL) {
			for (oldi=i+1; oldi<count-1; oldi++) {
				if (patterns[oldi] == NULL) 
					continue;

				/* Fill in the NULL line */
				oldj = oldi + 1;
				for (j=i+1; j<newcount; j++) {
					for ( ; oldj<count; oldj++) {
						if (Similarity[oldi][oldj] == NULL)
							continue;
						
						Similarity[i][j] = Similarity[oldi][oldj];
						Similarity[oldi][oldj] = NULL;
						break;
					}
				}
				
				/* Refresh the 'patterns' */
				patterns[i] = patterns[oldi];
				patterns[oldi] = NULL;
				
				break; /* the line finished */
			}
			
		} else {
			for (j=i+1; j<newcount; j++) {
				if (Similarity[i][j] != NULL)
					continue;
					
				for (oldj=j+1; oldj<count; oldj++) {
					if (Similarity[i][oldj] == NULL)
						continue;
					
					Similarity[i][j] = Similarity[i][oldj];
					Similarity[i][oldj] = NULL;
					break;
				}
			}
		}			
	}

	if (patterns[newcount-1] == NULL) {
		/* sth. special here */
		for (i=newcount; i<count; i++) {
			if (patterns[i] != NULL) {
				patterns[newcount-1] = patterns[i];
				patterns[i] = NULL;
			}
		}
	}

	/* Add the new merged suffix pat-pair */
	patterns[newcount] = tmpptn;
	tmpptn = NULL;
		
	++newcount;
#if 0
	fprintf(stderr, "\nAFTER\n");
	for (i=0; i<count; i++) {
		fprintf(stderr, "ptns[%d]\n", i);
		show_ptn(patterns[i], 1, -1, NULL);
	}
#endif
	return newcount;
}

/* Fill in the new vertical line */
int fillin_samemapline(int newcount, int oldcount, int shortest, struct pat_tree_node **patterns, struct same_bit ***Similarity, struct max_item **Max)
{
	struct pat_tree_node *ptn=NULL;
	int_list_t *pid;
	int same, rcutflag, icutnum, jcutnum;
	int i, j;
	
	for (i=0; i<newcount-1; i++) {
		/* If same ID found we continue */
		pid = patterns[newcount-1]->pat_id_list;
		while (pid != NULL) {
			if (check_int_list(patterns[i]->pat_id_list, pid->val) == 0)		
				break;
			pid = pid->next;
		}
		if (pid != NULL) {
			Similarity[i][newcount-1] = new_same_bit(0,0,0,0,-1,NULL);
			if (!Similarity[i][newcount-1]) return -1;
			continue;
		}

		/* No same ID found, we merge */
		if (patsame(&ptn, &same, &rcutflag, &icutnum, &jcutnum, shortest, patterns[i], patterns[newcount-1]) == -1)
			return -1;
			
		Similarity[i][newcount-1] = new_same_bit(rcutflag, icutnum, jcutnum, same, 0, ptn);
		if (!Similarity[i][newcount-1])
			return -1;
		//xiayu
		Similarity[i][newcount-1]->ptn = ptn;
		/* don't free ptn, for it's refered by 'Similarity' */	
		ptn = NULL;
	}
#if 0
{
	for (i=0; i< newcount; i++) {
		for (j=0; j< newcount; j++)
			if (Similarity[i][j])
				fprintf(stderr, "%d", Similarity[i][j]->same);
			else
				fprintf(stderr, "N");
		fprintf(stderr, "\n");
	}
}
#endif
	/* Re-calculate the Max[i] for the PTNmap */
	for (i=0; i<newcount-1; i++) {
		Max[i]->same = 0;
		Max[i]->idx = -1;
	}
	for (i=newcount-1; i<oldcount-1; i++) {
		free(Max[i]);
		Max[i] = NULL;
	}
		
	for (i=0; i<newcount-1; i++) { 		
		for (j=i+1; j<newcount; j++) {
			if (Similarity[i][j]->same > Max[i]->same) {
				Max[i]->idx = j;
				Max[i]->same = Similarity[i][j]->same;
			} else if (Similarity[i][j]->same == Max[i]->same && Max[i]->same) {
				if (maxexpect(Similarity[i][j], 
						 	  Similarity[i][Max[i]->idx]) > 0) {
					Max[i]->idx = j;
					Max[i]->same = Similarity[i][j]->same;
				}						
			}
		}
	}
	return 0;
}

void assign_accept(struct rule_item **rules, struct pat_tree_node *ptn)
{
	int_list_t *pid;
	if (ptn->ltree && ptn->rtree) {
		if (count_int_list(ptn->match_id_list) > count_int_list(ptn->ltree->match_id_list)+count_int_list(ptn->rtree->match_id_list))
		{
			pid = ptn->match_id_list;
			while (pid) {
				if (check_int_list(ptn->ltree->match_id_list, pid->val) == 0 || 
					check_int_list(ptn->rtree->match_id_list, pid->val) == 0) 
				{
					pid = pid->next;
					continue;
				}
				if (rules[pid->val]->rearcut) {
					insert_int_list(&ptn->rear_id_list, pid->val);
				} else {
					insert_int_list(&ptn->target_id_list, pid->val);
				}
				ptn->accept_flag = 1;
				pid = pid->next;
			}
		}
	} else if (ptn->ltree && !ptn->rtree) {
		if (count_int_list(ptn->match_id_list) > count_int_list(ptn->ltree->match_id_list)) {
			pid = ptn->match_id_list;
			while (pid) {
				if (check_int_list(ptn->ltree->match_id_list, pid->val) == 0) {
					pid = pid->next;
					continue;
				}
				if (rules[pid->val]->rearcut) {
					insert_int_list(&ptn->rear_id_list, pid->val);
				} else {
					insert_int_list(&ptn->target_id_list, pid->val);
				}
				ptn->accept_flag = 1;
				pid = pid->next;
			}
		}
	} else if (!ptn->ltree && ptn->rtree) {
		if (count_int_list(ptn->match_id_list) > count_int_list(ptn->rtree->match_id_list)) {
			pid = ptn->match_id_list;
			while (pid) {
				if (check_int_list(ptn->rtree->match_id_list, pid->val) == 0) {
					pid = pid->next;
					continue;
				}
				if (rules[pid->val]->rearcut) {
					insert_int_list(&ptn->rear_id_list, pid->val);
				} else {
					insert_int_list(&ptn->target_id_list, pid->val);
				}
				ptn->accept_flag = 1;
				pid = pid->next;
			}
		}
	} else { /* !ptn->ltree && !ptn->rtree */
		pid = ptn->match_id_list;
		while (pid) {
			if (rules[pid->val]->rearcut) {
				insert_int_list(&ptn->rear_id_list, pid->val);
			} else {
				insert_int_list(&ptn->target_id_list, pid->val);
			}
			pid = pid->next;
		}
		ptn->accept_flag = 1;
	}

	if (ptn->ltree) {
		assign_accept(rules, ptn->ltree);
	}
	if (ptn->rtree) {
		assign_accept(rules, ptn->rtree);
	}
	return;
}

struct ptn_list *merge_pat(struct rule_list *rlist, int shortest, int nocase)
{
	struct ptn_list *ptnlist;
	struct rule_item **rules;
	int count;
	
	struct same_bit ***Similarity;
	struct max_item **Max=NULL;
	int map_size;
	struct pat_tree_node **patterns;
	
	int_list_t *pid=NULL;
	int maxi, maxj, maxsame, newcount=0;
	int i, j, id, maxlen=0;

	ptnlist = prepare_pat(rlist, nocase);
	if (ptnlist->ptn_num > 1) {

		patterns = ptnlist->ptns;
		rules = rlist->rules;
		map_size = count = ptnlist->ptn_num;

		Similarity = create_samemap(patterns, map_size, &Max, shortest);
		if (!Similarity) {
			return NULL;
		}
		maxsame = calcu_maxsame(count, Similarity, Max, &maxi, &maxj);
		newcount = count;
		
		/* Enter the Merging Loop */
		while (maxsame >= shortest && count > 1){

			/* Sign the 'rules' whose origin patterns have been cutted */
			if (Similarity[maxi][maxj]->rcutflag == 1) {
				/* Horison pattern */
				if (Similarity[maxi][maxj]->icutnum != 0) {
					pid = patterns[maxi]->match_id_list;
					while (pid) {
						rules[pid->val]->rearcut += Similarity[maxi][maxj]->icutnum;
						pid = pid->next;
					}
				}
			
				/* Vertical pattern */
				if (Similarity[maxi][maxj]->jcutnum != 0) {
					pid = patterns[maxj]->match_id_list;
					while (pid) {
						rules[pid->val]->rearcut += Similarity[maxi][maxj]->jcutnum;
						pid = pid->next;
					}
				}
			}

			newcount = merge_ptnlist(maxi, maxj, count, Similarity, Max, patterns);
			fillin_samemapline(newcount, count, shortest, patterns, Similarity, Max);		
			maxsame = calcu_maxsame(newcount, Similarity, Max, &maxi, &maxj);

			/* Refresh the the 'count' */
			count = ptnlist->ptn_num = newcount;
		}
		free_same_bit_map(Similarity, map_size);
		free_max_item_list(Max, map_size-1);

		/* Delete redundent 'patterns' */
		for (i=0; i<newcount; i++) {
			if (!patterns[i] || patterns[i]->pat_id_list->next != NULL)
				/* if the count of 'pat_id_list' is more than 1, 
				there must be no redundent items for these 'id's */
				continue;

			id = patterns[i]->pat_id_list->val;
			maxlen = patterns[i]->maxlen;
			for (j=i+1; j<newcount; j++) {
				if (!patterns[j] || patterns[j]->pat_id_list->next != NULL)
				/* if the count of 'pat_id_list' is more than 1, 
				there must be no redundent items for these 'id's */
					continue;
					
				if (patterns[j]->pat_id_list->val == id) {
					if (patterns[j]->maxlen > patterns[i]->maxlen) {
						maxlen = patterns[j]->maxlen;
						free_ptn(patterns[i]);
						patterns[i] = NULL;
						--count;
						break; /* the patterns[i] were freed so we break */
					} else {
						free_ptn(patterns[j]);
						patterns[j] = NULL;
						--count;
					}
				}
			}
		}
		if (count < newcount) {
			if (cleanup_ptn(ptnlist, count)==-1)
				return NULL;
		}
	}
	for (i=0; i<ptnlist->ptn_num; i++) {
		assign_accept(rlist->rules, ptnlist->ptns[i]);
	}
	return ptnlist;
}
