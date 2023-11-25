#ifndef __SWBM_SAME_H
#define __SWBM_SAME_H


/************************ INCLUDE *****************************/
#include "ptn.h"


/************************ DEFINE *****************************/
/* for 'premerge_pat()' */
#define MINORIGINPATLEN		11	

#define MINSAME		11
#define MAXCHARNUM	256
#define MAXPTREELEN	1024

/******************* DATA STRUCT *********************/
/* For merge_pat() */
struct same_bit {
	int rcutflag;	/* cut in the rear, 0 - uncut, 1 - cut */
	int icutnum;	/* the num horison pattern cutted in the head */
	int jcutnum;	/* the num vertical pattern cutted in the head */
	
	int hcutflag;	/* cut in the head, 0 - uncut, 1 - cut */
	
	int same;	/* max Similarity of the horizon line */
	double E;	/* be '-1', unless once calculated */
	struct pat_tree_node *ptn;	/* the merged pattern tree */
};

struct max_item {
	int same;
	int idx;
};

/******************* FUNCTION INTERFACE *********************/
extern void free_same_bit(struct same_bit *sb);
extern void free_same_bit_map(struct same_bit ***Similarity, int size);
extern struct same_bit *new_same_bit(int rcutflag, int icutnum, int jcutnum, int same, double E, struct pat_tree_node *ptn);
extern void free_max_item_list(struct max_item **milist, int mi_num);
extern int maxexpect(struct same_bit *sb1, struct same_bit *sb2);
extern int calcu_maxsame(int count, struct same_bit ***Similarity, struct max_item **Max, int *maxi, int *maxj);
extern struct same_bit ***create_samemap(struct pat_tree_node **patterns, int count, struct max_item ***Max, int shortest);

#endif
