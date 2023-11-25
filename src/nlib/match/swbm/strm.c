#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "match_rule.h"
#include "ptn.h"
#include "strm.h"
#include "intlist.h"


void free_got_list(got_list_t *got_list)
{
	got_list_t *pre_pg, *pg;
	if (!got_list) return;
	pre_pg = pg = got_list;
	while (pre_pg) {
		pg = pg->next;	
		free_int_list(pre_pg->id_list);
		free(pre_pg);
		pre_pg = pg;
	}
	return;
}

got_list_t *insert_got_list(int offset, int_list_t *id_list, got_list_t *next)
{
	got_list_t *got;
	int_list_t *pid=id_list;
	got = (got_list_t *)calloc(1, sizeof(got_list_t));
	if (!got) {
		fprintf(stderr, "insert_got_list: malloc failed\n");
		return NULL;
	}
	got->offset = offset;
	while (pid) {
		if (insert_int_list(&got->id_list, pid->val) == -1) {
			free_got_list(got);
			return NULL;
		}
		pid = pid->next;
	}
	got->next = next;
	return got;
}

sub_pat *new_subpat(unsigned char *pat, int len)
{
	sub_pat *sp;
	sp = (sub_pat *)calloc(1, sizeof(sub_pat));
	if (!sp) {
		fprintf(stderr, "calloc error in 'new_subpat()'.\n");
		return NULL;
	}
	sp->pat = (unsigned char *)calloc(len, sizeof(unsigned char));
	if (!sp->pat) {
		fprintf(stderr, "calloc error in 'new_subpat()'.\n");
		free(sp);
		return NULL;
	}
	memcpy(sp->pat, pat, len);
	sp->len = len;
	return sp;
}


void free_subpat_list(sub_pat **sub_pats, int subpat_num)
{
	int i;
	if (!sub_pats) return;
	for (i=0; i<subpat_num; i++) {
		if (sub_pats[i]) {
			if (sub_pats[i]->pat)
				free(sub_pats[i]->pat);
			free(sub_pats[i]);
		}
	}
	free(sub_pats);
	return;
}


int add_subpat(unsigned char *pat, int len, sub_pat ***sub_pats, int *subpat_num)
{
	if (*subpat_num == 0) {
		*sub_pats = (sub_pat **)calloc(1, sizeof(sub_pat*));
		if (!*sub_pats) {
			fprintf(stderr, "calloc error in 'add_subpat()'.\n");
			return -1;
		}
	} else {
		*sub_pats = (sub_pat **)realloc(*sub_pats, ((*subpat_num)+1)*sizeof(sub_pat*));
		if (!*sub_pats) {
			fprintf(stderr, "realloc error in 'add_subpat()'.\n");
			return -1;
		}
	}
	(*sub_pats)[*subpat_num] = new_subpat(pat, len);
	if ((*sub_pats)[*subpat_num] == NULL) {
		free_subpat_list(*sub_pats, *subpat_num);
		*sub_pats = NULL;
		return -1;
	}
	++(*subpat_num);
	return 0;
}


sub_pat **new_subpat_list(int subpat_num)
{
	sub_pat **sub_pats;
	sub_pats = (sub_pat **)malloc(subpat_num * sizeof(sub_pat));
	if (!sub_pats) {
		fprintf(stderr, "malloc error in 'new_subpat_list()'.\n");
		return NULL;
	}
	return sub_pats;	
}


int show_subpat(sub_pat *sp)
{
	int i;
	for (i=0; i<sp->len; i++) {
		printf("%c ", sp->pat[i]);
	}
	return 0;
}

void free_subrule(sub_rule *sr)
{
	if (!sr) return;
	free_got_list(sr->got_list);
	sr->rule_trie = NULL;
	free(sr);
	return;
}

void free_subrule_list(sub_rule **sub_rules, int subrule_num)
{
	int i;
	if (!sub_rules) return;
	for (i=0; i<subrule_num; i++) {
		free_subrule(sub_rules[i]);
	}
	free(sub_rules);
	return;
}

sub_rule **new_subrule_list(int subrule_num)
{
	sub_rule **sr;
	int i;
	sr = (sub_rule **)malloc(subrule_num * sizeof(sub_rule*));	
	if (!sr) {
		fprintf(stderr, "malloc error in 'new_subrule_list()'.\n");
		return NULL;
	}
	for (i=0; i<subrule_num; i++) {
		sr[i] = (sub_rule *)calloc(1, sizeof(sub_rule)); 
		if (!sr[i]) {
			fprintf(stderr, "calloc error in 'new_sub_rule()'.\n");
			free_subrule_list(sr, subrule_num);
			return NULL;
		}
	}
	return sr;
}


void free_subrear(sub_rear *sr)
{
	if (!sr) return;
	free_got_list(sr->got_list);
	sr->search_trie = sr->rear_trie = sr->rule_trie = 0;
	free(sr);
	return;
}

void free_subrear_list(sub_rear **sub_rears, int subrear_num)
{
	int i;
	if (!sub_rears) return;
	for (i=0; i<subrear_num; i++) {
		free_subrear(sub_rears[i]);
	}
	free(sub_rears);
	return;
}

sub_rear **new_subrear_list(int subrear_num)
{
	sub_rear **sr;
	int i;
	sr = (sub_rear **)malloc(subrear_num * sizeof(sub_rear *));
	if (!sr) {
		fprintf(stderr, "malloc error in 'new_subrear_list()'.\n");
		return NULL;
	}
	for (i=0; i<subrear_num; i++) {
		sr[i] = (sub_rear *)calloc(1, sizeof(sub_rear));
		if (!sr[i]) {
			fprintf(stderr, "calloc error in 'new_sub_rear()'.\n");
			free_subrear_list(sr, subrear_num);
			return NULL;
		}
	}
	return sr;
}


int show_subrear(sub_rear *sr)
{
	int_list_t *pid;
	got_list_t *pg;
	printf("(gotIDs)");
	pg = sr->got_list;
	while (pg) {
		pid = pg->id_list;
		while (pid) {
			printf("%d ", pid->val);
			pid = pid->next;
		}
		pg = pg->next;
	}
	printf("(resume_type) %d ", sr->resume_type);
	if (sr->search_trie) {
		printf("(search_trie) %p ", sr->search_trie);
	} else {
		printf("(search_trie) 0 ");
	}
	if (sr->rear_trie) {
		printf("(rear_trie) %d", sr->rear_trie->state);
	} else {
		printf("(rear_trie) 0 ");
	}
	if (sr->rule_trie) {
		printf("(rule_trie) %d", sr->rule_trie->state);
	} else {
		printf("(rule_trie) 0 ");
	}
	return 0;
}

int show_subrule(sub_rule *sr)
{
	int_list_t *pid;
	got_list_t *pg;
	printf("(gotIDs)");
	pg = sr->got_list;
	while (pg) {
		pid = pg->id_list;
		while (pid) {
			printf("%d ", pid->val);
			pid = pid->next;
		}
		pg = pg->next;
	}
	if (sr->rule_trie) {
		printf("(rule_trie) %d", sr->rule_trie->state);
	} else {
		printf("(rule_trie) 0 ");
	}
	return 0;
}

#if 0
struct trie *put_remain_text_ex(unsigned char *hp, struct trie *level1_trie, unsigned char *stop, struct trie *level2_trie, struct match_info *ms, int *ret)
{
	struct trie *pt, *t1, *t2;
	int_list_t *pid;
	pt = t1= level1_trie; /* at least return the 'level1_trie' */
	while (hp >= stop) {
		t1 = t1->branch[*hp];
		if (t1) {
			hp--;
			if (t1->accept_flag) {
				pt = t1;
				if ((pid = t1->targetIDs)) {
					*ret = 1;
					while (pid) {
#if 0						
						if (check_symbolid(pid->val, ms)) {
							/* All rules found. */
							return 1;
						}
#else
						check_symbolid(pid->val, ms);
#endif
						pid = pid->next;
					}
				}
				if (t1->rearIDs) {
					t2 = level2_trie;
					while (t2) {
						if ((pid=t1->IDlist[level2_trie->state-1])) {
							*ret = 1;
							while (pid) {
#if 0						
								if (check_symbolid(pid->val, ms)) {
									/* All rules found. */
									return 1;
								}
#else
								check_symbolid(pid->val, ms);
#endif
								pid = pid->next;
							}
							t2 = t2->sub_trie;
						}
					}
				}
			}
		} else {
			break;
		}
	}
	return pt;
}

/* still can be optimized !!!!!! remember to do it when got time! */
struct trie *put_remain_text(unsigned char *buffer, int buf_len, struct trieobj *rule_tree, struct match_info *ms, int *ret)
{
	register unsigned char *stop, *hhp;
	register struct trie *ht, *pre_ht=NULL;
	int_list_t *pid;
	int i;
	/* should make sure whether 's' is set '0' */
	stop = buffer + buf_len -1;
	for (i=0; i<buf_len; i++) {
		hhp = buffer + i;
		ht = rule_tree->mtries[*hhp];
		while (hhp <= stop) {
			if (ht) {
				pre_ht = ht;
				if (ht->accept_flag) {
					if ((pid = ht->targetIDs)) {
						*ret = 1;
						while (pid) {
#if 0						
							if (check_symbolid(pid->val, ms)) {
								/* All rules found. */
								return 1;
							}
#else
							check_symbolid(pid->val, ms);
#endif
							pid = pid->next;
						}
					}
				}
				hhp++;
				ht = ht->branch[*hhp];
			} else {
				break;
			}
		}
		if (hhp > stop) {
			if (pre_ht) {
				return pre_ht;
			} else {
				return NULL;
			}
		}
	}
	return NULL;
}
#endif
