#ifndef _SWBM_H
#define _SWBM_H

#include "regex.h"
#include "intlist.h"
//#include "rulelist.h"

#define ALPHABETSIZE		256
#define ACCEPTABLE		1
#define NOT_ACCEPT		0			

typedef struct _got_list_t {
        int offset;
        int_list_t *id_list;
        struct _got_list_t *next;
} got_list_t;

typedef struct _sub_rear {
        got_list_t *got_list;
        int resume_type;
        struct trie *search_trie;
        struct trie *rear_trie;
        struct trie *rule_trie;
} sub_rear;


struct rule_item {
        int_list_t      *id_list;       /* the same with ExpNode->id_list, Symbol Id */

        int             pat_len;        /* length of the pattern, including '\' */
        unsigned char   *pat;   /* the original pattern */

        int             start;  /* start of the search range */
        int             end;    /* end of the search range */
        int             nocase; /* 1 - IS NOT case sensitive, 0 - IS case sensitive */
        int             charset; /* refer from "conv_char.h" */

        int rearcut;    /* the merging cut from the rear of the pattern */
        int state;
        int val;
};


struct rule_list {
        struct rule_item        **rules;
        int             rule_num;

        /* for 'start', 'end' parametres */
        int             min_start;
        int             max_end;

        /* for 'nocase' parametres */
        int             case_check_flag; /* 0 - check, 1 - NOT check. */
        struct trieobj  *case_pre_tree;  /* not NULL when 'case_check_flag == 1' */

        /* for real-time running */
        /* REAR tree, confirming complete matching after SEARCH tree matched. */
        struct trieobj *rear_pre_tree;
        sub_rear **sub_rears;
        int subrear_num;
};

struct rule_list *new_rule_list(int count);
struct rule_item *new_rule_item(        int_list_t *id_list,
                                                char *pat,
                                                int pat_len,
                                                int start,
                                        int end,
                                        int nocase,
                                                int charset);

struct trieobj;
struct trie {
        struct trieobj *self;
        struct trie **branch;

        int state; /* level1_state in search tree
                                  level2_state in rear tree */

        int accept_flag;        /* 0-unacceptable, 1-acceptable */
        int_list_t *target_id_list;

        /* following fields r for the suffix searching tree,
                which r the same with the ones in leaves of PTN struct */
        int_list_t *rear_id_list;
        int_list_t **IDlist; /* indexed with 'level2_state' */
        struct trie *sub_trie;
};


extern struct trie *new_trie (  struct trieobj *self,
                                int_list_t **IDlist,
                                int_list_t *target_id_list,
                                int_list_t *rear_id_list,
                                int accept_flag,
                                struct trie *sub_trie,
                                                int state);
extern void trie_nocase(struct trieobj * self);


struct trieobj {
        int alphabet;
        struct trie **mtries;
        int shortest;
        int longest;
        int nocase; /* 1-nocase 0-case sensitive */
        int *bad;
};

extern struct trieobj *new_trieobj(     int alphabet,
                                        int shortest,
                                int nocase,
                                        int bad_flag);
extern void free_trieobj(struct trieobj *obj);



struct pat_tree_node {
	struct pat_tree_node	*ltree;
	struct pat_tree_node	*rtree;
	/* following fields used in merging time */
	int minlen;	/* minimum len of patterns in the suffix-tree */
	int maxlen;	/* maximum len of patterns in the suffix-tree */
	int len; 	/* length of the 'str' itself */
	char *str;	/* the whole or only part of a pattern */
	int_list_t *pat_id_list; 	/* for merging */
	int_list_t *match_id_list;	/* for searching */

	int accept_flag;
	int_list_t *target_id_list;
	int_list_t *rear_id_list;
	int_list_t **IDlist;
	
	//int scaned;			/* 0-unsearched, 1-searched */
};

struct ptn_list{
	struct pat_tree_node **ptns;
	int ptn_num;

	/* for real-time running */
	/* SEARCH tree, used to efficiently detect a possible match, 
	   after which a RULE tree would be used to confirm the 
	   complete matching. */
	struct trieobj *search_suf_tree;
};

extern void free_ptn_list(struct ptn_list *ptnlist);
struct ret_sym {
        int symbol;
        int val;
};


#define SETMIN(a,b) if ( (a) > (b) ) { (a) = (b); } 
#define SETMAX(a,b) if ( (a) < (b) ) { (a) = (b); } 

#define NO_CASE			1
#define CASE_SENS		0

struct ptn_list *merge_pat(struct rule_list *rlist, int shortest, int nocase);
int build_search_tree(struct ptn_list *ptnlist, int nocase);
int build_rear_tree(struct rule_list *rlist, struct ptn_list *ptnlist, int nocase);
int swbm_match(       unsigned char *text,
                              	int text_len,
				struct ptn_list *ptnlist,
                         	struct rule_list *rlist,
				int retnum,
				struct ret_sym **retsyms );

void free_rule_list(struct rule_list *rlist);

#endif
