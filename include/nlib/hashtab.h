
#ifndef HASH_TBL_H
#define HASH_TBL_H

/*
(c) 2002, 2004 James D. Allen
 * This is a header to customize the UNALTERED
 * file hashtab.c for a specific application.
 *
 * To clarify this, we incorporate two versions of the
 * header, each used for real application.
 *
 * The code defines two data types: Tab_entry and Te_ktype.
 * Tab_entry must be a structure containing the field
        Te_ktype        te_key;
 * and Te_ktype must be lvaluable.  Otherwise hashtab.c
 * doesn't need anything about these except the macros defined
 * as follows,  In every case, the arguments are instances of
 * Te_ktype.
	KEY(t)		-- selects key info from t
			  (often the no-op  define KEY(t) t)
	KEYEQ(a, b)	-- compare two keys for equality
	HASHER(t)	-- develop the pseudo-random hash number
			  for a key.
	MTSET(t)	-- set the entry to denote empty.
	MTSLOT(t)	-- test the entry: is it empty?
	DELSET(t)	-- set the entry to denote deleted.
	DELETED(t)	-- test the entry: is it deleted?
 */

#ifdef	VERSION2

#define	MAXB	10

typedef struct {
	unsigned char	tecode;
	unsigned char xx[MAXB + 2];
} Te_ktype;

typedef struct {
        Te_ktype        te_key;
	unsigned char	te_resu;
} Tab_entry;

#define KEY(tekey)              (tekey)
#define KEYEQ(a, b)             (!strcmp(a.xx, b.xx))
#define HASHER(s)               hashstring(s.xx)
#define MTCODE                  (0)
#define MTSLOT(s)               ((s.tecode) == MTCODE)
#define MTSET(s)                ((s.tecode) = MTCODE)
#define VALID(tekey)            (tekey.tecode > 2)
#define DELETED(s)              (0)
#define DELSET(s)
#else

typedef unsigned short Te_ktype;

typedef struct {
        Te_ktype        te_key;
	unsigned /*char*/	short	val;	/*bugfix, wyong, 2006.8.30 */
} Tab_entry;

#define EPEND                   0x4000
#define OPEND                   0x8000
#define KEY(tekey)              ((tekey) & ~(EPEND | OPEND))
#define KEYEQ(a, b)             (!(KEY(a ^ b)))
#define HASHER(s)               ((s) + ((s) >> 1) - ((s) >> 3))
#define MTCODE                  (0)
#define MTSLOT(s)               ((s) == MTCODE)
#define	MTSET(s)		((s) = MTCODE)

#ifdef  ALLOWDEL
#define DELCODE                 0x4000
#define VALID(tekey)            KEY(tekey)
#define DELETED(s)              ((s) == DELCODE)
#define DELSET(s)               ((s) = DELCODE)
#else
#define VALID(tekey)            (tekey)
#define DELETED(s)              (0)
#define DELSET(s)
#endif

#endif


/* From here on is more-or-less independent of application specifics */
struct hashtab {
        Tab_entry       *httab;
        int     htsziz;
        int     htsize;
        int     htnumi; /* includes deleted items also */
        int     htmaxalloc;
        int     htpmoved;
#ifdef  ALLOWDEL
        int     htnumd;
#endif
};

Tab_entry *hashfind();

/*
 * Two parameters determine how full tables get.
 * Here are two example settings.
 */
#if     0
        /* Memory is scarce: Following parms reorg from 88% to 69% */
#define TPARM1  3
#define TPARM3  2
#else
        /* Moderate: Following parms reorg from 75% to 37% */
#define TPARM1  2
#define TPARM3  6
#endif


#if     0
/* Here is a synopsis of the interface */
//#include      "hashtab.h"

/* Find key, whose hash value is hashval, in tabp.
 * Usage:  rc = hashfind(...);
 *         if (rc && VALID(rc->te_key)) {
 *            // found, rc points to table entry
 *         }
 */
Tab_entry *hashfind(unsigned int hashval, Te_ktype key, struct hashtab *tabp);

#ifdef  ALLOWDEL
/* Delete key, if present, from tabp.  Return true if key was present.
 */
hashdelete(Te_ktype key, struct hashtab *tabp);
#endif

/* The remaining three functions may make old pointers to table entries
 *  stale.  Test (and then reset) tabp->htpmoved to see if pointers moved.
 * Hashinsert() will allocate initial memory for a table, and expand
 *  as needed.  Thus hashmreorg() and hashreorg() need not be called
 *  unless you want to exercise more control.
 */

/* Set *tepp to point to an entry for key in tabp, creating it
 *   if necessary.  Return true if the entry is new.
 * Caller will then need to set any other info into *tepp.
 */
hashinsert(Te_ktype key, struct hashtab *tabp, Tab_entry **tepp);

/* Replace tabp's memory with a replacement of size siz.
 */
hashreorg(struct hashtab *tabp, int siz);

/* Replace tabp's memory with a replacement of heuristic size.
 */
hashmreorg(struct hashtab *tabp)


#endif

void hash_free(struct hashtab *);
struct hashtab *hash_alloc(int );


#endif
