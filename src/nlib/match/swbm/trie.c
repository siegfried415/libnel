#include <stdio.h> 
#include <stdlib.h> 
#include <ctype.h>

#include "nlib/match/swbm.h"
#include "trie.h"
#include "intlist.h"

/* 
#include "mem_pool.h"
ObjPool_t	swbm_trie_pool;
ObjPool_t	swbm_branch_pool;
ObjPool_t	swbm_trieobj_pool;
ObjPool_t	swbm_bad_shift_pool;
*/


struct trie *new_trie(struct trieobj *self, int_list_t **IDlist, int_list_t *target_id_list, int_list_t *rear_id_list, int accept_flag, struct trie *sub_trie, int state)
{
	struct trie *trie;

	trie = (struct trie *)calloc(1, sizeof(struct trie));
	if (!trie) {
		fprintf(stderr, "calloc error in 'new_trie()'.\n");
		return NULL;
	}
		
	trie->branch = (struct trie **)calloc(self->alphabet * sizeof(struct trie *), 1);
	if (!trie->branch) {
		fprintf(stderr, "calloc error in 'new_trie()'.\n");
		free(trie);
		return NULL;
	}

 	/* will be freed in free_ptn */ 
	trie->self = self;
	trie->IDlist = IDlist;
	trie->target_id_list = target_id_list;
	trie->rear_id_list = rear_id_list;
	trie->accept_flag = accept_flag;
	trie->sub_trie = sub_trie;
	trie->state = state;

	return trie;
}

struct trieobj *new_trieobj(int alphabet, int shortest, int nocase, int bad_flag)
{
	struct trieobj *to;
	int i;
	to = (struct trieobj *)calloc(1, sizeof(struct trieobj));
	if (!to) {
		fprintf(stderr, "calloc error in 'new_trieobj()'.\n");
		return NULL;
	}

	to->alphabet = alphabet;
	to->nocase = nocase;
	to->mtries = (struct trie**)calloc(alphabet, sizeof(struct trie *));
	if (!to->mtries) {
		fprintf(stderr, "calloc error in 'new_trieobj()'.\n");
		free(to);
		return NULL;
	}

	to->shortest = shortest;
	if (bad_flag) {
		if (!to->shortest) { 
			fprintf(stderr, "invaild 'shortest' in 'new_trieobj()'.\n");
			return NULL;
		}
		to->bad = (unsigned int *)malloc(alphabet * sizeof(unsigned int));
		if (!to->bad) {
			fprintf(stderr, "malloc error in 'new_trieobj()'.\n");
			return NULL;
		}
		for (i=0; i<alphabet; i++) 
			to->bad[i] = shortest;
	}
	return to;
}

/* flag: 1-nocase 0-case sensitive */
void free_trie(struct trie *trie, int alphabet, int nocase)
{
	int i, offset;
	if (!trie) return;
	
	/* NOTE: no need to free the 'trie->target',
		'coz here is only a reference not a new alloc */
	offset = 'a' - 'A';

	if (trie->branch) { /* they finally pointed to the same 'trie',
				 so we could only free each of them ONCE */	
		for (i=0; i<alphabet; i++) {
			if (trie->branch[i]) {
				free_trie(trie->branch[i], alphabet, nocase);
				trie->branch[i] = NULL;
				if (nocase && i>=65 && i<=90) {
					trie->branch[i+offset] = NULL;
				}
			}
		}
		free(trie->branch);
		trie->branch = NULL;
	}
	free(trie);
	return;
}

void free_trieobj(struct trieobj *obj)
{
	int i, offset;
	if (!obj) return;
	offset = 'a' - 'A';
	for (i=0; i<obj->alphabet; i++) {
		if (obj->mtries[i]) {
			free_trie(obj->mtries[i], obj->alphabet, obj->nocase);
			obj->mtries[i] = NULL;
		}
		if (obj->nocase && i>=65 && i<=90) {
			obj->mtries[i+offset] = NULL;
		}
	}
	free(obj->mtries);
	obj->mtries = NULL;
	if (obj->bad) {
		free(obj->bad);
		obj->bad = NULL;
	}
	free(obj);
	return;
}

void trie_nocase_branch(struct trieobj *self, struct trie *t)
{
	unsigned long ch;
	for (ch = 0; ch < self->alphabet; ch++) {   
		if (!t->branch[ch]) continue;
		t->branch[toupper(ch)] = t->branch[ch];
		t->branch[tolower(ch)] = t->branch[ch];
		trie_nocase_branch(self, t->branch[ch]);
	}
	return;
}

void trie_nocase(struct trieobj * self)
{
	unsigned long ch;
	for (ch = 0; ch < self->alphabet; ch++) {   
		if (!self->mtries[ch]) 
			continue;
		self->mtries[toupper(ch)] = self->mtries[ch];
		self->mtries[tolower(ch)] = self->mtries[ch];
		trie_nocase_branch(self, self->mtries[ch]);
	}
	return;
}

void init_swbm_tree_mem_pool (void)
{
	/* 
	create_mem_pool (&swbm_trie_pool, sizeof(struct trie), 2048);
	create_mem_pool (&swbm_branch_pool, ALPHABETSIZE*sizeof(struct trie *), 1024);
	create_mem_pool (&swbm_trieobj_pool, sizeof(struct trieobj), 512);
	create_mem_pool (&swbm_bad_shift_pool,  ALPHABETSIZE*sizeof(unsigned int), 512);
	*/
}
