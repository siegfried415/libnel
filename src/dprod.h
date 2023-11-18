#ifndef DPROD_H
#define DPROD_H

#include "termset.h"
/************************************************************************/ 
/* a production, with an indicator that says how much of this		*/
/* production has been matched by some part of the input string		*/
/* (exactly which part of the input depends on where this appears	*/
/* in the algorithm's data structures)					*/
/************************************************************************/
struct dprod {
	struct nel_SYMBOL *prod;
	int dot;	// 0 means it's before all RHS symbols, 
			// 1 means after first, etc.

	int dotAtEnd;

	/* performance optimization: NULL if dot at end, or else pointer*/
	/* to the symbol right after the dot				*/
	struct nel_SYMBOL *afterDot;

	/* First of the sentential form that follows the dot; this set	*/
	/* is computed by GrammarAnalysis::computeDProdFirsts		*/
	struct termset firstSet;

	/* also computed by computeDProdFirsts, this is true if the	*/
	/* sentential form can derive epsilon (the empty string)	*/
	int canDeriveEmpty;

	/* timeout value on this dotted production, wyong, 2004.11.27 	*/
	int timeout;

};

struct nel_eng;
struct nel_SYMBOL;
struct nel_SYMBOL *sym_before_dot(struct dprod *);
struct nel_SYMBOL *sym_after_dot(struct dprod *);

int is_dot_at_start(struct dprod *);
int is_dot_at_end(struct dprod *);

struct dprod *get_dprod(struct nel_eng *eng, struct nel_SYMBOL *prod, int posn);
struct dprod *get_next_dprod(struct dprod *);


int dprod_alloc(struct nel_eng *, struct nel_SYMBOL *);
int dprods_alloc(struct nel_eng *);
void emit_dprod(struct nel_eng *,FILE * , struct dprod * , int);

#endif
