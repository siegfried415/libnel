#include <stdio.h>
#include <stdlib.h>

#include "nlib/match/swbm.h"
#include "ptn.h"
#include "same.h"
#include "merge.h"


void free_max_item(struct max_item *mi)
{
	if (!mi) return;
	free(mi);
	return;
}

void free_max_item_list(struct max_item **milist, int mi_num)
{
	int i;
	if (!milist) return;
	for (i=0; i<mi_num; i++) {
		free_max_item(milist[i]);
	}
	free(milist);
	return;
}

struct max_item *new_max_item(int same, int idx)
{
	struct max_item *mi;
	mi = (struct max_item *)calloc(1, sizeof(struct max_item));
	if (mi == NULL) {
		fprintf(stderr, "calloc error \n");
		return NULL;
	}
	mi->same = same;
	mi->idx = idx;
	return mi;
}

struct max_item **new_max_item_list(int count)
{
	struct max_item **milist;
	int i;
	milist = (struct max_item **)calloc(count, sizeof(struct max_item *));
	if (milist == NULL) {
		fprintf(stderr, "calloc error \n");
		return NULL;
	}
		
	for (i=0; i<count; i++) {
		milist[i] = new_max_item(0, -1);
		if (milist[i] == NULL) {
			free_max_item_list(milist, count);
			return NULL;
		}
	}
	return milist;
}


struct same_bit *new_same_bit(int rcutflag, int icutnum, int jcutnum, int same, double E, struct pat_tree_node *ptn)
{
	struct same_bit *sb;
	sb = (struct same_bit *)calloc(1, sizeof(struct same_bit));
	if (sb == NULL) {
		fprintf(stderr, "calloc error \n");
		return NULL;
	}
	sb->rcutflag = rcutflag;
	sb->icutnum = icutnum;
	sb->jcutnum = jcutnum;
	sb->same = same;
	sb->E = E;
	sb->ptn = ptn;
	return sb;
}

void free_same_bit(struct same_bit *sb)
{
	if (!sb) return;
	if (sb->ptn) {
		free_ptn(sb->ptn);
		sb->ptn = NULL;
	}
	free(sb);
	return;
}

void free_same_bit_map(struct same_bit ***Similarity, int size)
{
	int i, j;
	if (!Similarity) return;
	for (i=0; i<size; i++) {
		if (Similarity[i]) {
			for (j=i+1; j<size; j++) {
				free_same_bit(Similarity[i][j]);
			}
			free(Similarity[i]);
		}
	}
	free(Similarity);
	return;
}


struct same_bit ***new_same_bit_map(int count)
{
	struct same_bit ***sbmap;
	int i;
	sbmap = (struct same_bit ***)calloc(count, sizeof(struct same_bit **));
	if (sbmap == NULL) {
		fprintf(stderr, "calloc error \n");
		return NULL;
	}
	for (i=0; i<count; i++) {
		sbmap[i] = (struct same_bit **)calloc(count, sizeof(struct same_bit *));
		if (sbmap[i] == NULL) {
			fprintf(stderr, "calloc error \n");
			free_same_bit_map(sbmap, count);
			return NULL;
		}
	}
	return sbmap;
}

void fillShift1(int *Shift, int *Count, int *sum, struct pat_tree_node *ptn, int level)
{
	int i;
	if (ptn->str) {
		for (i=0; i<ptn->len; i++) {
			if (level+i < Shift[*ptn->str+i]) {
				Shift[*ptn->str+i] = level+i;
				++Count[level+i];
				++*sum;
			}
		}
	}
	level += ptn->len;
	if (ptn->ltree) {
		fillShift1(Shift, Count, sum, ptn->ltree, level);
	}
	if (ptn->rtree) {
		fillShift1(Shift, Count, sum, ptn->rtree, level);
	}
	
	return;
}

void fillShift(int *Shift, int *Count, int *sum, struct pat_tree_node *ptn)
{
	int i;
	if (ptn->str) {
		for (i=1; i<ptn->len; i++) {
			if (i < Shift[*ptn->str+i]) {
				Shift[*ptn->str+i] = i;
				++Count[i];
				++*sum;
			}
		}
	}
	if (ptn->ltree) {
		fillShift1(Shift, Count, sum, ptn->ltree, ptn->len);
	}
	if (ptn->rtree) {
		fillShift1(Shift, Count, sum, ptn->rtree, ptn->len);
	}
	return;
}

double expect(struct pat_tree_node *ptn)
{
	int Shift[256], *Count;
	int i, j, sum;
	double E, E1, P, P1, Pc;
	P1= 1.0;
	E = E1 = P = Pc = 0;
	
	if (ptn == NULL)
		return 0;
		
	for (i=0; i<256; i++) {
		Shift[i] = ptn->minlen;
	}

	Count = (int *)calloc(ptn->minlen, sizeof(int));
	if (!Count) {
		fprintf(stderr, "calloc error in 'expect()'.\n");
		return -1;
	}
	
	fillShift(Shift, Count, &sum, ptn);
	
	if (Shift[(int)(ptn->str[0])] < ptn->minlen) {
		--Count[Shift[(int)(*ptn->str)]];
	}
	--sum;
	Pc = ((float) 1) / MAXCHARNUM;

	for (i=1; i<ptn->minlen; i++) {
		E1 = i * Count[i];
		E += E1;
	}
	free(Count);

	E += ptn->minlen * (MAXCHARNUM - sum);
	E *= Pc;

	for (i=2; i<=ptn->maxlen; i++) {
		for (j=0; j<i-1; j++) {
			P1 *= Pc;
		}
		P += P1/ i;
	}

	E += Shift[(int)(*ptn->str)] * (1-Pc) * P;

	return E;
}

int maxexpect(struct same_bit *sb1, struct same_bit *sb2)
{
	double e1, e2;
#if 0	
	if (!sb1->ptn || !sb2->ptn)
		return 0;	/* be considered as 'e1 == e2' */
#endif
	if (sb1->E != -1){
		e1 = sb1->E;
	} else {
		e1 = expect(sb1->ptn);
		sb1->E = e1;
	}

	if (sb2->E != -1) {
		e2 = sb2->E;
	} else {
		e2 = expect(sb2->ptn);
		sb2->E = e2;
	}
	
	if (e1 > e2)
		return 1;
	else if (e1 == e2)
		return 0;
	else
		return -1;
}


/* Create and fillin the 'Similarity' Map */
struct same_bit ***create_samemap(struct pat_tree_node **patterns, int count, struct max_item ***Max, int shortest)
{
	struct same_bit ***Similarity;
	struct pat_tree_node *ptn=NULL;
	int_list_t *pid=NULL;
	int same=0, rcutflag=0, icutnum=0, jcutnum=0;
	int i, j;

#if 0
	fprintf(stderr, "\n");
	for (i=0; i<count; i++) {
		fprintf(stderr, "ptns[%d]\n", i);
		show_ptn(patterns[i], 1, -1, NULL);
	}
#endif

	Similarity = new_same_bit_map(count);
	if (Similarity == NULL)
		return NULL;
	
	*Max = new_max_item_list(count - 1);
	if (*Max == NULL) {
		free_same_bit_map(Similarity, count);
		return NULL;
	}
	
	for (i=0; i<count; i++) { /* scan each line */
		for (j=i+1; j<count; j++) { /* scan each same bit of the current line */
			/* If same ID found we continue */
			pid = patterns[i]->pat_id_list;
			while (pid != NULL) {
				if (check_int_list(patterns[j]->pat_id_list, pid->val) == 0){
					Similarity[i][j] = NULL;
					break;
				}	
				pid = pid->next;
			}
			if (pid != NULL) {
				Similarity[i][j] = new_same_bit(0,0,0,0,-1,NULL);
				if (!Similarity[i][j]) {
					free_same_bit_map(Similarity, count);
					free_max_item_list(*Max, count-1);
					return NULL;
				}
				continue;
			}

			/* No same ID found, we merge */
			if (patsame(&ptn, &same, &rcutflag, &icutnum, &jcutnum, shortest,
					patterns[i], patterns[j]) == -1)
			{
				free_same_bit_map(Similarity, count);
				free_max_item_list(*Max, count-1);
				return NULL;
			}
			
			Similarity[i][j] = new_same_bit(rcutflag, icutnum, jcutnum, same, -1, ptn);
			if (!Similarity[i][j]) {
				free_same_bit_map(Similarity, count);
				free_max_item_list(*Max, count-1);
				return NULL;
			}

			/* don't free ptn, it's refered by Similarity */
			ptn = NULL;

			/* Update '*Max[i]' */
			if (same > (*Max)[i]->same) {
				(*Max)[i]->idx = j;
				(*Max)[i]->same = same;
			} else if (same && same == (*Max)[i]->same) {
				if (maxexpect(Similarity[i][j], 
					  Similarity[i][(*Max)[i]->idx]) > 0)
				{	/* Greater or equal to */
					(*Max)[i]->idx = j;
					(*Max)[i]->same = same; 
				}				
			}
		} /* scan each same bit of the current line */
	} /* scan each line */

#if 0
	for (i=0; i<count; i++) {
		for (j=0; j<count; j++) {
			if (Similarity[i][j])
				fprintf(stderr, "%d", Similarity[i][j]->same);
			else
				fprintf(stderr, "N");

		}
		fprintf(stderr, "\n");
	}
#endif

	return Similarity;
}

/* Choose the the Merge-pair */
int calcu_maxsame(int count, struct same_bit ***Similarity, struct max_item **Max, int *maxi, int *maxj)
{
	int i, maxsame = 0;
	*maxi = *maxj = -1;

	for (i=0; i<count-1; i++) {
		if (Max[i]->same > maxsame) {
			maxsame = Max[i]->same;
			*maxi = i;
			*maxj = Max[i]->idx;
		} else if (Max[i]->same == maxsame && maxsame) {
			if (maxexpect(Similarity[i][Max[i]->idx],
						  Similarity[*maxi][*maxj]) > 0)
			{	/* Greater or equal to */
				maxsame = Max[i]->same;
				*maxi = i;
				*maxj = Max[i]->idx;
			}
		}				
	}
#if 0 
	fprintf(stderr, "ptns[%d] %s\t ptns[%d] %s\t maxsame %d\n", *maxi, patterns[*maxi]->str, *maxj, patterns[*maxj]->str, maxsame);
#endif
	return maxsame;
}
