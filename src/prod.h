#ifndef PROD_H
#define PROD_H

struct nel_eng;
union nel_TYPE;
struct nel_SYMBOL;

typedef enum {
        REL_EX = 0,		/* '!'  */
        REL_ON,			/* ':'	*/
        REL_OR,			/* '|'  */
        REL_OT,			/* ': others '  */
        REL_AT,			/* '->' */
        REL_UN			/* UNKNOWN */
} Relation;


typedef int NtIndex;

struct 	nel_RHS
{
        struct 	nel_SYMBOL *symbol;
        struct 	nel_RHS *next;
        struct 	nel_RHS *prev;
        struct 	nel_RHS *last;
        int 	offset;
        //union  	nel_STMT *stmts;
        //union  	nel_STMT *last_stmt;
        //int 	timeout;
        //union  	nel_STMT *timeout_stmts;
	char *filename;
	int line;
};


typedef struct prodinfo
{
        unsigned int rhsLen;                // # of RHS symbols
        NtIndex lhsIndex;                    // 'ntIndex' of LHS
        Relation rel;			
        struct nel_SYMBOL *action;
}
ProdInfo;

extern struct nel_RHS *rhs_copy_except(struct nel_eng *eng, struct nel_RHS *, struct nel_RHS *);
extern struct nel_SYMBOL *lookup_prod(struct nel_eng *eng, struct nel_SYMBOL *,  int offset);
extern struct nel_SYMBOL *prod_symbol_alloc(struct nel_eng *, char *, union nel_TYPE *);
extern void prod_symbol_dealloc(struct nel_eng *, struct nel_SYMBOL *);
extern int remove_empty_prods(struct nel_eng *eng);
extern void emit_prods(struct nel_eng *eng /*,FILE *fp*/);
struct nel_SYMBOL *pseudo_prod_alloc(struct nel_eng *eng);
struct nel_SYMBOL *others_prod_alloc(struct nel_eng *eng, int rhs_number);

#endif
