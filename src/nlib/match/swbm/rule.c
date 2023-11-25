#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nlib/match/swbm.h"
#include "strm.h" 


#if 1
int is_hex_digit(char c)
{
	if (c >= '0' && c <= '9')
		return 1;
	if (c >= 'A' && c <= 'F')
		return 1;
	if (c >= 'a' && c <= 'f')
		return 1;
	return 0;
}

/* translate strings, which includeing '\x' into asc string */
int hex2asc(char *hexpat, char **ascpat)
{
	int i, j, flag=0;
	int newlen, len=strlen(hexpat);
	char *newpat;
	char trans_tbl[] = {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,		
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,
		0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0, 
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 
		0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	};

	/* Calculate the new length */
	newlen = len;
	for (i=0; i<len; i++) {
		switch(hexpat[i]) {
			case '\\':
				if (i>0) {
					if (hexpat[i-1] == '\\') {
						if (flag) {
							flag = 0;
						} else {
							flag = 1;
							if (i == len-1)
								return -1;
						}
					} else {
						flag = 1;
						if (i == len-1)
							return -1;
					}

				} else {
					flag = 1;
					if (i == len-1)
						return -1;
				}
				break;
			case 'x':
				if (i>0) {
					if (hexpat[i-1] == '\\' && flag) {
						if (!is_hex_digit(hexpat[i+1])
						  || !is_hex_digit(hexpat[i+2]))
						{
							/* invalid "\xDD" */
							return -1;
						}
						newlen -= 3;	
						i += 2;
					}
				} else {
					i++;
				}
				break;
			default:
				break;
		}
	}

	newpat= (char *)calloc(newlen, sizeof(char));
	if (!newpat) {
		fprintf(stderr, "calloc error in 'pattrans()'.\n");
		return -1;
	}

	if (newlen == len) {
		/* Only dup the */
		memcpy(newpat, hexpat, len);
	} else {
		/* Do the translation */
		flag = 0;
		j = 0; 
		for (i=0; i<len; i++) {
			switch(hexpat[i]) {
				case '\\':
					if (i>0) {
						if (hexpat[i-1] == '\\') {
							if (flag) {
								flag = 0;
								newpat[j++] = hexpat[i];
							} else {
								flag = 1;
							}

						} else {
							flag = 1;
						}

					} else { 
						flag = 1;
					}
					break;
				case 'x':
					if (i>0 && hexpat[i-1] == '\\' && flag) {
						newpat[j++] = (char)(((trans_tbl[hexpat[i+1]] & 0x0f) << 4) + (trans_tbl[hexpat[i+2]] & 0x0f));
						i += 2;
					} else {
						newpat[j++] = hexpat[i];
					}
					break;
				default:
					newpat[j++] = hexpat[i];
					break;
			}
		}
	}	

	*ascpat = newpat;
	return newlen;
}
#else
int hex2asc(char *hexpat, char **ascpat)
{
	int i, j, flag=0;
	int newlen, len=strlen(hexpat);
	char trans_tbl[] = {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,		
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,
		0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0, 
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 
		0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	};
	
	/* Calculate the new length */
	newlen = len;
	for (i=0; i<len; i++) {
		switch(hexpat[i]) {
			case '\\':
				if (i>0) {
					if (hexpat[i-1] == '\\') {
						if (flag) {
							flag = 0;
						} else {
							flag = 1;
						}
					}
				}
				break;
			case 'x':
				if (i>0) {
					if (hexpat[i-1] == '\\' && !flag) {
						newlen -= 3;	
						i += 2;
					}
				}
				break;
			default:
				break;
		}
	}

	*ascpat= (char *)calloc(newlen, sizeof(char));
	if (!*ascpat) {
		fprintf(stderr, "calloc error in 'pattrans()'.\n");
		return -1;
	}

	if (newlen == len) {
		/* Only dup the */
		memcpy(*ascpat, hexpat, len);
	} else {
		/* Do the translation */
		flag = 0;
		j = 0; 
		for (i=0; i<len; i++) {
			switch(hexpat[i]) {
				case '\\':
					if (i>0) {
						if (hexpat[i-1] == '\\') {
							if (flag) {
								flag = 0;
							} else {
								flag = 1;
							}
						}
					}
					(*ascpat)[j++] = hexpat[i];
					break;
				case 'x':
					if (i>0 && hexpat[i-1] == '\\' && !flag) {
						(*ascpat)[j-1] = ((trans_tbl[hexpat[i+2]] & 0x0f) << 4) 
							 + (trans_tbl[hexpat[i+3]] & 0x0f);
						i += 2;
					} else {
						(*ascpat)[j++] = hexpat[i];
					}
					break;
				default:
					(*ascpat)[j++] = hexpat[i];
					break;
			}
		}
	}	
	return newlen;
}
#endif

char *insert_backslash(char *pat)
{
	char *newpat;
	int i, j, len, newlen;
	int bscount=0;

	len = strlen(pat);
	for (i=0; i<len; i++) {
		if (pat[i] == '\\') {
			++bscount;
		}
	}
	newlen = len + bscount;

	newpat = (char *)calloc(newlen+1, sizeof(char));
	if (!newpat) {
		fprintf(stderr, "calloc error in 'add_backslash()'.\n");
		return NULL;
	}

	j = 0;
	for (i=0; i<len; i++) {
		if (pat[i] == '\\') {
			newpat[j] = newpat[j+1] = '\\';
			j += 2;
		} else {
			newpat[j++] = pat[i];
		}
	}
	return newpat;
}

struct rule_item *new_rule_item(int_list_t *id_list, char *pat, int pat_len, int start, int end, int nocase, int charset)
{
	struct rule_item *ri;
	ri = (struct rule_item *)calloc(1, sizeof(struct rule_item));
	if (!ri) { 
		fprintf(stderr, "calloc error in 'new_rule_item()'.\n");
		return NULL;
	}
	ri->id_list = id_list;
	ri->pat_len = pat_len;
	//ri->pat = pat;
	ri->pat = (char *)malloc(pat_len * sizeof(char));
	if (!ri->pat) {
		fprintf(stderr, "malloc error in 'new_rule_item()'.\n");
		free(ri);
		return NULL;
	}
	memcpy(ri->pat, pat, pat_len);
	ri->start = start;
	ri->end = end;
	ri->nocase = nocase;
	ri->charset = charset;
	return ri;
}

struct rule_list *new_rule_list(int count)
{
	struct rule_list *rlist;
	rlist = (struct rule_list *)calloc(1, sizeof(struct rule_list));
	if (!rlist) {
		fprintf(stderr, "calloc error in 'new_rule_list()'.\n");
		return NULL;
	}
	rlist->rule_num = count;
	rlist->rules = (struct rule_item **)calloc(count, sizeof(struct rule_item *));
	if (!rlist->rules) {
		fprintf(stderr, "calloc error in 'new_rule_list()'.\n");
		free(rlist);
		return NULL;
	}
	return rlist;
}


void free_rule_item(struct rule_item *ritem)
{
	if (!ritem) return;
	free_int_list(ritem->id_list);
	if (ritem->pat)
		free(ritem->pat);
	free(ritem);
	return;
}

void free_rule_list(struct rule_list *rlist)
{
	struct rule_item **rules;
	int i;

	if (!rlist) return;

	if ((rules = rlist->rules)) {
		for (i=0; i<rlist->rule_num; i++) {
			free_rule_item(rules[i]);
		}
		free(rlist->rules);
	}

	free_trieobj(rlist->rear_pre_tree);
	free_trieobj(rlist->case_pre_tree);

	if (rlist->sub_rears) {
		for (i=0; i<rlist->subrear_num; i++) {
			free_subrear(rlist->sub_rears[i]);
		}
		free(rlist->sub_rears);
	}

	free(rlist);
	return;
}


