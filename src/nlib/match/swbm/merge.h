/*
 * swbm_merge.h
 * $Id: swbm_merge.h,v 1.2 2006/03/17 05:50:16 wyong Exp $
 */

#ifndef __SWBM_MERGE_H
#define __SWBM_MERGE_H

#include "nlib/match/swbm.h"
#include "ptn.h"

int patsame(struct pat_tree_node **ptn, int *same, int *rcutflag, int *icutnum, int *jcutnum, int shortest, struct pat_tree_node *pa, struct pat_tree_node *pb);

#endif
