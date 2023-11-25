#ifndef __SWBM_MERGE_H
#define __SWBM_MERGE_H

#include "nlib/match/swbm.h"
#include "ptn.h"

int patsame(struct pat_tree_node **ptn, int *same, int *rcutflag, int *icutnum, int *jcutnum, int shortest, struct pat_tree_node *pa, struct pat_tree_node *pb);

#endif
