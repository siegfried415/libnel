
#include <stdio.h>
#include <stdlib.h>

#include "engine.h"
#include "errors.h"
#include "itemset.h"
#include "item.h"
#include "gen.h"
#include "mem.h"

extern nel_lock_type nel_malloc_lock;

int item_merge(struct lritem *src, struct lritem *dst)
{
	termset_merge(&src->lookahead, &dst->lookahead);
	return 0;
}

int item_equal(struct lritem *left, struct lritem *right)
{
	return (left->dprod == right->dprod);
}

struct lritem *lookup_item(struct nel_LIST **list, struct dprod *dp)
{
	struct nel_LIST *itl;
	for(itl = *list; itl; itl = itl->next) {
		struct lritem *item = (struct lritem *)itl->symbol;
		if (item->dprod == dp ) {
			return item;
		}
	}

	return (struct lritem *)0;
}


struct lritem *item_alloc(struct nel_eng *eng, struct dprod *dp, struct termset *ts)
{
	struct lritem *it;
	nel_malloc(it, 1, struct lritem);

	if(!it) {
		gen_error(eng, "nel_item_alloc: malloc error!\n");	
		return NULL;
	}

	nel_zero(sizeof(struct lritem), it);
	it->dprod = dp;
	termset_init(&it->lookahead, eng->numTerminals);
	if(ts) {
		termset_copy(&(it->lookahead), ts);
	}

	return it ; 
}

void emit_item(struct nel_eng *eng,FILE *fp, struct lritem *lri)
{
	if(lri){
		emit_dprod(eng, fp, lri->dprod, lri->offset); fprintf(fp, "\n");
		//putputLookahead(eng, fp, &lri->lookahead);

		//emit_termset(eng, fp, &lri->lookahead );
	}

}



