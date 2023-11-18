#ifndef ITEMSET_H
#define ITEMSET_H

#include "item.h"
//#include "sym.h"
//typedef unsigned int StateId;

// ---------------- struct itemset -------------------
// a set of dotted productions, and the transitions between
// item sets, as in LR(0) set-of-items construction

// integer id for an item-set DFA state; I'm using an 'enum' to
// prevent any other integers from silently flowing into it
//typedef enum { STATE_INVALID=-1 } StateId;

struct itemset{
	// numerical state id, should be unique among item sets
	// in a particular grammar's sets
	StateId id;

	// kernel items: the items that define the set; except for
	// the special case of the initial item in the initial state,
	// the kernel items are distinguished by having the dot *not*
	// at the left edge
	struct lritem *kernelItems;


	// nonkernel items: those derived as the closure of the kernel
	// items by expanding symbols to the right of dots; here I am
	// making the choice to materialize them, rather than derive
	// them on the spot as needed (and may change this decision)
	struct lritem *nonkernelItems;

	// transition function (where we go on shifts); NULL means no transition
	// Map : (Terminal id or Nonterminal id)  -> struct itemset*
	//struct pathseg **nontermTransition;	
	//struct pathseg **termTransition;	
	struct nel_LIST **nontermTransition;	
	struct nel_LIST **termTransition;	

	// bounds for above
	int terms;
	int nonterms;

	/* timeout value, wyong, 2004.11.27 */
	int timeout;
	int priority;

	// need to store this, because I can't compute it once I throw
	// away the items
	//struct nel_SYMBOL *stateSymbol;

	// for traversal through all the transition accroding to a given 
	// terminal , thus changed every time when called by getTransitions
	struct itemset *next_transition;

	struct itemset *next;
	
} ;

struct stateInfo {
	int id;
	int has_reduce;
	int has_shift;
	int priority;
	int reduce; 	//wyong, 2006.3.2 
	int dot_pos;	//wyong, 2006.9.25
};


void sort_itemsets(struct itemset *is);
int add_item_to_itemset_kernel(struct itemset *is, struct lritem *item);
void add_item_to_itemset_nonkernel(struct itemset *is, struct lritem *item);
struct itemset *others_itemset_alloc(struct nel_eng *eng, int dot, TerminalSet *ts);
struct itemset *timeout_itemset_alloc(struct nel_eng *eng, int dot, TerminalSet *ts);
struct itemset *not_itemset_alloc(struct nel_eng *eng, int dot, TerminalSet *ts);
int itemset_equal(struct itemset *left, struct itemset *right);
struct itemset *lookup_itemset(struct nel_LIST **list, struct itemset *is);
void insert_itemset(struct nel_eng *eng, struct itemset *is);
struct itemset *itemset_alloc(struct nel_eng *eng);
void itemset_dealloc(struct nel_eng *eng, struct itemset *is);
void emit_itemset(struct nel_eng *eng,FILE *fp, struct itemset *is);
void  emit_itemsets(struct nel_eng *eng /*,FILE *fp*/ );

#endif
