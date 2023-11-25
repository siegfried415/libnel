
#ifndef TERMSET_H
#define TERMSET_H

// ----------------- TerminalSet -------------------
// used for the lookahead sets of LR items, and for the First()
// sets of production RHSs
typedef struct termset{
	unsigned char *bitmap;	// (owner) bitmap of terminals, indexed by
				// terminal id; lsb of byte 0 is index 0
  	int bitmapLen; 		// # of bytes in 'bitmap'
	int numbers ;		// # of terminals 
}TerminalSet;


void termset_clear(TerminalSet *ts);
void termset_reset(TerminalSet *ts, int numTerms);
int termset_unset(TerminalSet *ts, int id);
int termset_set(TerminalSet *ts, int id);
int termset_set_2(struct nel_eng *eng, TerminalSet *ts, int id);

void termset_copy(TerminalSet *ts, TerminalSet *dst);
int termset_merge(TerminalSet *src, TerminalSet *des);
int termset_merge_2(struct nel_eng *eng,TerminalSet *src, TerminalSet *des);
int termset_test(TerminalSet *ts, int id);
int termset_equal(TerminalSet *ts, TerminalSet *des);
int termset_iszero(struct nel_eng *eng, TerminalSet *ts);
void termset_init(TerminalSet *ts, int numTerms);
void emit_termset(struct nel_eng *eng,FILE *fp, TerminalSet *ts);

#endif
