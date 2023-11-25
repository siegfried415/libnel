#ifndef __MATCH_RULE_H
#define __MATCH_RULE_H


/* for the Global Identification of struct match_info */
#define MIN_MS_ID		3


#if 0 
#if 1
#define CHECK_SYMBOLID(id, ms) \
	if (!ms->rlist->rules[id]->val) {\
		ms->rlist->rules[id]->val = 1;\
		/* Check each FOUND id */\
		mid = ms->rlist->rules[id]->id_list;\
		while (mid) {\
			for (j=0; j<ms->retnum; j++){\
				if (ms->retsyms[j]->symbol == mid->val) {\
					ms->retsyms[j]->val = 1;\
					break;\
				}\
			}\
			if (j == ms->retnum) {\
				/* could not found this id */\
				fprintf(stderr, "Symbol ID stored in Matchstruct were corrupted.\n ");\
				return -1;\
			}\
			mid = mid->next;\
		}\
		value = 1;\
		for (i=0; i<ms->rlist->rule_num; i++) {\
			value *= ms->rlist->rules[i]->val;\
		}\
		if (value) return 1; /* when all rules found */\
	}
#else
#define CHECK_SYMBOLID() \
	if (!ms->rlist->rules[id]->val) {\
		ms->rlist->rules[id]->val = 1;\
		/* Check each FOUND id */\
		mid = ms->rlist->rules[id]->id_list;\
		while (mid) {\
			fprintf(stderr, "%d found.\n", mid->val);\
			mid = mid->next;\
		}\
		value = 1;\
		for (i=0; i<ms->rlist->rule_num; i++) {\
			value *= ms->rlist->rules[i]->val;\
		}\
		if (value) return 1; /* when all rules found */\
	}
#endif
#endif




#if 0

/* for the size  of field 'id_list' in struct 'originpats' */
#define	MAXORIGINIDNUM	5	/* max num of 'id_list' of 'originpats' */

/* for the 'swbm_match' type, which make different things in 'level2_match' */
#define NONCOMP_MODE		0
#define FUNC_MODE		1
#define COMPILE_MODE		2

#endif

#endif
