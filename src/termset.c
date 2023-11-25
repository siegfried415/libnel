
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "type.h"
#include "termset.h"
#include "sym.h"
#include "mem.h"

extern nel_lock_type nel_malloc_lock;

void termset_clear(TerminalSet *ts)
{
	if(ts) {
		memset(ts->bitmap, 0, ts->bitmapLen);
	}
}

void termset_reset(TerminalSet *ts, int numTerms)
{
	if (ts->bitmap) {
		nel_free(ts->bitmap);	
	}
	termset_init(ts, numTerms);
}


int termset_unset(TerminalSet *ts, int id)
{
	unsigned char *p = &(ts->bitmap[(unsigned)id/8]);
	if(p) {
		unsigned char old = *p;
		*p &= (unsigned char ) ~(1 << ((unsigned)id % 8)) ;
		return (*p != old);
	}
	return 0;
}

int termset_set(TerminalSet *ts, int id)
{
	unsigned char *p = &(ts->bitmap[(unsigned)id/8]);
	if(p){
		unsigned char old = *p;
		*p |= (unsigned char ) (1 << ((unsigned)id % 8)) ;
		return (*p != old);
	}
	return 0;
}

int termset_set_2(struct nel_eng *eng, TerminalSet *ts, int id)
{
	if(id != eng->otherSymbol->id && id != eng->timeoutSymbol->id&& id != eng->notSymbol->id)  {
		return termset_set(ts, id);
	}
	return 0;
}


void termset_copy(TerminalSet *ts, TerminalSet *dst)
{
	memcpy(ts->bitmap, dst->bitmap, ts->bitmapLen);
}

int termset_merge(TerminalSet *src, TerminalSet *des)
{
	int changed = 0, i;
	for (i=0; i< src->bitmapLen; i++) {
		unsigned before = src->bitmap[i];
		unsigned after = before | des->bitmap[i];
		if (after != before) {
			changed = 1;
			src->bitmap[i] = after;
		}
	}

	return changed;
}


int termset_merge_2(struct nel_eng *eng,TerminalSet *src, TerminalSet *des)
{
	int changed = 0, id;
	for(id = 0;  id < eng->numTerminals;  id++ ) {
		if(id != eng->otherSymbol->id && id != eng->timeoutSymbol->id && id != eng->notSymbol->id) {
			if(termset_test(des, id)) {
				changed += termset_set(src, id);
			}
		}
	}

	return changed;

}

int termset_test(TerminalSet *ts, int id)
{
	unsigned char p, q;
	if(ts && ts->bitmap) {
		p = ts->bitmap[(unsigned)id/8];
		if(p) {
			q = ( p >> ((unsigned)id % 8)) & 1;
			return (q == 1);
		}
	}
	return 0;
}

int termset_equal(TerminalSet *ts, TerminalSet *des) 
{
	int i;	
	if(!ts || !des) { return -1;}
	if(ts->numbers != des->numbers) {return 0;}
	for(i=0; i< ts->numbers; i++) {
		if(termset_test(ts, i) != termset_test(des, i)) {
			return 0;
		}
	}
	return 1;
}

int termset_iszero(struct nel_eng *eng, TerminalSet *ts)
{
	TerminalSet tmp;
	termset_init(&tmp, eng->numTerminals);
	int i = termset_equal(ts, &tmp);
	nel_dealloca(tmp.bitmap);
	return i;
}

void termset_init(TerminalSet *ts, int numTerms)
{
	if (numTerms != 0) 
	{
		/*---------------------------------------------
			Allocate enough space for one bit 
			per terminal; I assume 8 bits per byte
		 ---------------------------------------------*/
		ts->numbers = numTerms;
		ts->bitmapLen = (numTerms + 7) / 8;
		nel_malloc(ts->bitmap, ts->bitmapLen, unsigned char);
		/*---------------------------------------------
			Initially the set will be empty
		 ---------------------------------------------*/
		memset(ts->bitmap, 0, ts->bitmapLen);
	}
	else 
	{
		/*----------------------------------------------
			Intended for situations where reset() 
			will be called later to allocate some space
		 ----------------------------------------------*/
		ts->numbers = numTerms;
		ts->bitmapLen = 0;
		ts->bitmap = NULL;
	}

}

void emit_termset(struct nel_eng *eng,FILE *fp, TerminalSet *ts)
{
	nel_symbol *t;
	fprintf(fp, "\tLookahead:\n");
	for(t = eng->terminals; t; t = t->next) {
		if(termset_test(ts, t->id)) {
			fprintf(fp, "\t\t%d\t", t->id);
			emit_symbol(fp, t);
			fprintf(fp, "\t\t\n");
		}
	}

	fprintf(fp, "\n");

}
