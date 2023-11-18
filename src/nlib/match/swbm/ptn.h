/*
 * swbm_ptn.h
 * $Id: swbm_ptn.h,v 1.2 2006/03/17 05:50:16 wyong Exp $
 */

#ifndef __SWBM_PTN_H
#define __SWBM_PTN_H

/************************ INCLUDE *****************************/
#include "nlib/match/swbm.h"
#include "intlist.h"

#include "trie.h"
#include "strm.h"

/******************* DATA STRUCT *********************/


extern struct pat_tree_node *new_ptn(struct pat_tree_node *ltree, struct pat_tree_node *rtree, int minlen, int maxlen, int len, char *str, int_list_t *pat_id_list, int_list_t *match_id_list);
extern void free_ptn(struct pat_tree_node *ptn);
extern int dup_ptn(struct pat_tree_node **newptn, struct pat_tree_node *oldptn);
extern struct pat_tree_node *merge_ptn(struct pat_tree_node *pl, struct pat_tree_node *pr, char *str, int len, int_list_t *pat_id_list, int_list_t *match_id_list);
extern struct pat_tree_node *merge_new_ptn(struct pat_tree_node *pa, struct pat_tree_node *pb, char *str, int len, int_list_t *pat_id_list, int_list_t *match_id_list);
extern int cleanup_ptn(struct ptn_list *ptnlist, int newcount);

#endif
