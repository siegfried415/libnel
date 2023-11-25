
#ifndef ITEM_H
#define ITEM_H

#include "termset.h"
#include "dprod.h"

struct nel_eng ;
/************************************************************************/ 
/* a dotted production with a lookahead; whereas each production	*/
/* has a fixed number of dotted versions of that production, there	*/
/* can be lots of items, because of the differing lookahead sets	*/	
/* (I prefer the name "struct lritem" to simply "Item" because the 	*/
/* latter easily collides with other uses)				*/
/************************************************************************/
struct lritem{
	struct dprod *dprod;  		// (serf) production and dot position
	struct termset lookahead;       // lookahead symbols

//#define LRITEM_DR 	0x1
//#define LRITEM_SR	0x2
//#define LRITEM_RR	0x4 
	//int reduceType;		// if it is a reduce item, 
					// the flag shows the way to reduce. 

	struct lritem *next;		// use to chain all item belong to
					// the same itemset
	int kernel;
	int offset;			// 0 if kernel struct lritem, 
					// >0 when this item isn't start from
					// kernel 's first; 
};


int item_merge(struct lritem *src, struct lritem *dst);
int item_equal(struct lritem *left, struct lritem *right);
struct lritem *lookup_item(struct nel_LIST **list, struct dprod *dp);
struct lritem *item_alloc(struct nel_eng *,struct dprod *, struct termset *);
void emit_item(struct nel_eng *eng,FILE *fp, struct lritem *lri);

#endif
