#ifndef GEN_H
#define GEN_H

#include "itemset.h"
#include "type.h"
#include "dprod.h"


#define TOK_NUMS(eng) (eng->numTerminals + eng->numNonterminals + 1 )
#define SYM_OF_TERM_CLASS(eng, sid, tid ) (eng->classTable[sid * TOK_NUMS(eng) + eng->numNonterminals + tid].classifier)
#define LEN_OF_TERM_CLASS(eng, sid, tid ) (eng->classTable[sid * TOK_NUMS(eng) + eng->numNonterminals + tid].length)
#define SYM_OF_NONTERM_CLASS(eng, sid, nid) (eng->classTable[sid * TOK_NUMS(eng) + eng->numNonterminals + nid].classifier)
#define LEN_OF_NONTERM_CLASS(eng, sid, nid) (eng->classTable[sid * TOK_NUMS(eng) + eng->numNonterminals + nid].length)
#define SYM_OF_CLASS(eng,sid, id) ((id > 0) ? SYM_OF_TERM_CLASS(eng, sid, id) :			\
						((id < 0 ) ? SYM_OF_NONTERM_CLASS(eng, sid, id)	\
							  : NULL ))

#define LEN_OF_CLASS(eng,sid, id) ((id > 0) ? LEN_OF_TERM_CLASS(eng, sid, id) : 		\
						((id < 0 ) ? LEN_OF_NONTERM_CLASS(eng, sid, id)	\
							  : 0))

#define actionEntry(eng, state, termId) eng->actionTable[ (state) * eng->numTerminals+(termId)]
#define defaultActionEntry(eng, state) eng->default_action_tbl[state]

#define reduceEntry(eng, state) eng->stateTable[state].reduce
#define gotoEntry(eng, state, nonTermId) eng->gotoTable[(state) * eng->numNonterminals+(nonTermId)]
#define encodeSymbolId(eng, id, type) ( ((type)== nel_C_TERMINAL) ? \
					((id)+1) : \
					( ((type)== nel_C_NONTERMINAL) ? \
						(-(id)-1) : \
						0 \
					) \
				 )
#define decodeSymbolId(eng, id) ( ((id)>0) ? \
				((id)-1) : \
				( ((id)<0) ? \
					 (-(id)-1) : \
						 0 \
				) \
			   ) \


#define isAmbigAction(eng, code) ( (code)> eng->numStates )
#define decodeAmbig(eng, code) ( (code)-1- eng->numStates )
#define encodeAmbig(eng, ambigId) ( (ActionEntry)(eng->numStates+(ambigId)+1) )
#define isShiftAction(eng, code) ( ((code)>0) && ((code)<=eng->numStates) )
#define decodeShift(eng, code) ( (StateId)((code)-1) )
#define encodeShift(eng, stateId) ( (ActionEntry)((stateId)+1) )

#define isReduceAction(eng, code) ((code)<0)
#define decodeReduce(eng, code) (-(code)-1)
#define encodeReduce(eng, prodId) (-(prodId)-1)

#define encodeGoto(eng, stateId) ((GotoEntry)(stateId))
#define encodeGotoError( eng ) ((GotoEntry)eng->numStates )
#define decodeGoto(eng, code) ((StateId)(code))
#define encodeError(eng) ((ActionEntry)0)

typedef enum{
        LR0 = 0,// LR(0) does all reductions,
        // regardless of what the next token is

        SLR1,	// SLR(1) looks at a production's LHS's Follow

        LR1,	// LR(1) computes context-sensitive follow for each item,
        // depending on how that item arises in the item-set DFA

        LALR1	// LALR(1) is like LR(1), except two states are merged if
        // they only differ in their items' lookaheads (so it has
        // the same # of states as SLR(1), while having some of the
        // context-sensitivity of LR(1))
} LRlevel;

typedef enum{
        LR = 0,	// LR use one stack, regardless of confilicit

        OPLR,	// same with LR, but optimized for less shift and reduction.
        // implenmented by pengxn.

        TGLR,	// Tree Stack GLR parser, which have multiple stack at same
        // time, almost as efficient as LR. by wang.yong

        GGLR	// Graph Stack GLR parser, can handle more complicated sytanx.
        // but not as efficient as TGLR.
} ParseMethod;


typedef enum{
        SWBM = 0,	// set wise BM
        RBM,		// regulr expression via SWBM
        ACBM,		// ACBM
        AGREP		// agrep
} PatternMethod;


struct eng_gen {

	/********************************************************/
	/*      true if any nonterminal can derive itself	*/
	/*      (with no extra symbols surrounding it) 		*/
	/*      in 1 or more steps				*/
	/********************************************************/
	int cycle;

	/********************************************************/
	/*      incremented each time we encounter an error 	*/
	/*      that we can recover from			*/
	/********************************************************/
	int errors ;
	
	/********************************************************/
	/* if entry i,j is true, then nonterminal i can 	*/
	/* derive nonterminal j (this is a graph, represented 	*/
	/* (for now) as an adjacency matrix)			*/
	/********************************************************/
	char *derivable; 

	int nextItemSetId;

	/************************************************************/
	/*      map of production x dotPosition -> DottedProduction;*/
	/*      each element of the 'dottedProds' array is a pointer*/
	/*      to an array of DottedProduction objects		    */
	/************************************************************/
	struct dprod **dottedProds;

	struct itemset *itemSets;
	int **itemSetBefore;
	struct itemset *startState;
};


//----------------- Transition -----------------
#define LINE_SIZE 1024*8
#define ITEMS_PER_LINE 10

/* when grammar is built, this runs all analyses and stores
 the results in this object's data fields; write the LR item
 sets to the given file (or don't, if NULL) */
extern int nel_generator(struct nel_eng *);
extern int nel_gen_init(struct nel_eng *eng);

extern int gen_warning(struct nel_eng *, char *, ...);
extern int gen_error (struct nel_eng *, char *, ...);

void gen_dealloc(struct nel_eng *eng);
int gen_init(struct nel_eng *eng);

#endif
