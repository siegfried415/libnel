
#include <stdio.h>
#include <stdlib.h>
#include "nlib/hashtab.h"

typedef char nel_lock_type;
extern nel_lock_type nel_malloc_lock;

/*
(c) 2002, 2004 James D. Allen
 * We support several sizes,
 *      ranging from tiny (or zero) up to almost 60 million
 * Each entry in psize[] (except first few)
 *      is about 1.125 times preceding entry.
 * NUMSIZE must be one more than power-of-two; first size should be 0.
 * Each entry should be prime, though code will work (in a degraded
 *  fashion) as long as table size is odd.
 */

#define NUMSIZE 129
static long     psize[NUMSIZE] = {
	0, 3, 7, 11, 17, 23, 29, 37, 47, 53, 59, 67, 73, 79, 89, 101, 113, 127,
	139, 157, 179, 199, 223, 251, 283, 317, 359, 401, 449, 503, 569, 641, 
	719, 809, 911, 1021, 1151, 1297, 1459, 1637, 1847, 2081, 2341, 2633, 
	2963, 3331, 3739, 4211, 4733, 5323, 5987, 6733, 7573, 8521, 9587, 10781,
	12119, 13633, 15331, 17239, 19391, 21817, 24547, 27617, 31069, 34949, 
	39317, 44221, 49747, 55967, 62969, 70841, 79697, 89659, 100853, 113467,
	127649, 143609, 161561, 181757, 204481, 230047, 258803, 291143, 327529,
	368471, 414539, 466357, 524669, 590251, 664043, 747049, 840439, 945481,
	1063661, 1196609, 1346183, 1514459, 1703773, 1916741, 2156339, 2425889,
	2729119, 3070273, 3454081, 3885841, 4371569, 4918019, 5532763, 6224359,
	7002409, 7877711, 8862421, 9970223, 11216501, 12618569, 14195887, 
	15970369, 17966659, 20212483, 22739033, 25581407, 28779073, 32376469, 
	36423533, 40976471, 46098527, 51860839, 58343459,
};

static int setsiz(siz)
        long    siz;
{
        int     cumi, inci;

        cumi = 0;
        for (inci = NUMSIZE / 2; inci; inci >>= 1)
                if (psize[cumi + inci] < siz)
                        cumi += inci;
        return cumi + 1;
}

/* Caller must inspect return code (rc) to determine result:
 *      rc  == NULL          -->  not found, table unbuilt
 *      MTSLOT(rc->te_key)   -->  not found, can insert at rc
 *      DELETED(rc->te_key)  -->  not found, can insert at rc
 *      VALID(rc->te_key)    -->  found, at rc
 */
Tab_entry *hashfind(hashval, key, tabp)
        unsigned int    hashval;
        Te_ktype        key;
        struct hashtab *tabp;
{
        int     hk, siz, incr;
        Tab_entry *base, *hp, *delp;
        Te_ktype kp;

        siz = tabp->htsize;
        base = tabp->httab;
        if (base == 0)
                return 0;
        hk = hashval % siz;
        hp = base + hk;
        kp = hp->te_key;
#ifdef  ALLOWDEL
        if (VALID(kp)) {
                if (KEYEQ(kp, key))
                        return hp;
                delp = 0;
        } else if (MTSLOT(kp))
                return hp;
        else
                delp = hp;
#else
        if (MTSLOT(kp) || KEYEQ(kp, key))
                return hp;
#endif
        incr = 1;
        while (1) {
                hk += incr;
                if (incr != 2)
                        incr += 2;
                if (hk >= siz) {
                        hk -= siz;
                        if (incr >= siz)
                                incr = 2;
                }
                hp = base + hk;
                kp = hp->te_key;
#ifdef  ALLOWDEL
                if (VALID(kp)) {
                        if (KEYEQ(kp, key))
                                return hp;
                } else if (MTSLOT(kp))
                        return delp ? delp : hp;
                else if (!delp)
                        delp = hp;
#else
                if (MTSLOT(kp) || KEYEQ(kp, key))
                        return hp;
#endif
        }
}

void hashreorg(tabp, siz)
        struct hashtab *tabp;
	int siz; 
{
        int     i, oldsiz;
        Tab_entry       *oohp, *ohp, *nhp;
        Te_ktype        key;
        //void    *malloc();

        oldsiz = tabp->htsize;
        ohp = tabp->httab;
#ifdef  ALLOWDEL
        tabp->htnumi -= tabp->htnumd;
        tabp->htnumd = 0;
#endif
        tabp->htsize = siz;
        //printf("Reorganizing from %d to %d\n", oldsiz, siz);

        tabp->htmaxalloc = siz - (siz >> TPARM1) - 1;
        tabp->htpmoved = 1;
		
        nhp = tabp->httab = malloc(siz * sizeof(Tab_entry));
        if (!nhp) {
                //printf("Malloc(%d) failed\n", siz * sizeof(Tab_entry));
		int size = siz * sizeof(Tab_entry); 
                printf("Malloc(%d) failed\n", size );

                /* Yes, i know msg "should" go to stderr */
                exit(1);
        }
        for (i = 0; i < siz; i++, nhp++) {
                MTSET(nhp->te_key);
        }
        if (oohp = ohp) {
                for (i = 0; i < oldsiz; i++, ohp++) {
                        key = ohp->te_key;
                        if (VALID(key))
                                *(hashfind(HASHER(KEY(key)), key, tabp)) = *ohp;
                }
                free(oohp);
        }
}

#ifdef  ALLOWDEL
hashdelete(key, tabp)
        Te_ktype key;
        struct hashtab *tabp;
{
        Tab_entry *hp;

        hp = hashfind(HASHER(key), key, tabp);
        if (hp && VALID(hp->te_key)) {
                DELSET(hp->te_key);
                tabp->htnumd++;
                return 1;
        }
        return 0;
}
#endif

/* cover for hashreorg(); bypass for personal control */
void hashmreorg(tabp)
        struct hashtab *tabp;
{
        int     siz;

        if ((siz = tabp->htsziz) >= NUMSIZE - 1) {
                printf(" (maxsize) ... abort\n");
                exit(99);
        }
#ifdef  ALLOWDEL
        siz += tabp->htnumd > tabp->htnumi >> 2
                ? TPARM3 / 2 : TPARM3;
#else
        siz += TPARM3;
#endif
        if (siz >= NUMSIZE)
                siz = NUMSIZE - 1;
        tabp->htsziz = siz;
        siz = psize[siz];
        hashreorg(tabp, siz);
}

/*
 * Sets a pointer pointed to by argument `tepp' to a valid Table entry
 *  for argument `key', creating it if necessary.
 * Return 1 if the entry is new, 0 otherwise.
 */
int hashinsert(key, tabp, tepp)
        Te_ktype key;
        struct hashtab *tabp;
        Tab_entry **tepp;
{
        register Tab_entry      *hp;

        *tepp = hp = hashfind(HASHER(key), key, tabp);
        if (hp && VALID(hp->te_key))
                return 0;
        if (tabp->htnumi >= tabp->htmaxalloc) {
                hashmreorg(tabp);
                *tepp = hp = hashfind(HASHER(key), key, tabp);
        }
#ifdef  ALLOWDEL
        if (DELETED(hp->te_key))
                tabp->htnumd -= 1;
        else
                tabp->htnumi += 1;
#else
        tabp->htnumi += 1;
#endif
        hp->te_key = key;
        return 1;
}


void hash_free(struct hashtab *ht)
{
	free(ht);
}

struct hashtab *hash_alloc(int size)
{
	struct hashtab *ht;
	if( (ht = calloc(sizeof(struct hashtab ),1 )) != NULL ){
		Tab_entry *hp;
		ht->httab = calloc(sizeof(Tab_entry), 1);
		ht->htsize = 1;
	}
	return ht; 
}
