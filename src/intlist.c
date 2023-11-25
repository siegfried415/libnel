
#include <stdio.h>
#include <stdlib.h>

#include "intlist.h"



/*****************************************************************
 * Function: count_int_list(int_list_t *list) 
 *
 * Purpose: count how many int nodes are there in the given list.
 * 		
 * Arguments: list => the given int list
 *
 * Returns: the counting result. 
 *****************************************************************/
int count_int_list(int_list_t *list)
{
	int_list_t *plist;
	int count = 0;

	if (!list) {
		return 0;
	}
	plist = list;
	while (plist != NULL) {
		count++;
		plist = plist->next;
	}
	return count;
}

int_list_t *new_int_list(int val, int_list_t *next)
{
	int_list_t *plist;
	plist = (int_list_t *)calloc(1, sizeof(int_list_t));
	if (!plist) {
		//nel_error(NULL, "nel_malloc error \n");
		return NULL;
	}
	plist->val = val;
	plist->next = next;
	return plist;
}

int free_int_list(int_list_t *list)
{
	int_list_t *plist;
	if (!list) return 0;
	plist = list;
	while (plist) {
		list = list->next;
		free(plist);
		plist = list;
	}
	return 0;
}

int_list_t *dup_int_list(int_list_t *list)
{	
	int_list_t *newlist, *oldlist, *plist;

	if (!list) {
		//nel_error(NULL, "input error \n");
		return NULL;
	}
	oldlist = list;

	/* the head int_list_t node */
	newlist = new_int_list(oldlist->val, NULL);
	if (!newlist) {
		return NULL;
	}
	plist = newlist;
	oldlist = oldlist->next;
	while (oldlist != NULL) {
		plist->next = new_int_list(oldlist->val, NULL);
		if (!plist->next) {
			free_int_list(newlist);
			return NULL;
		}
		plist = plist->next;
		oldlist = oldlist->next;	
	}
	return newlist;
}



int check_int_list(int_list_t *list, int val)
{
	int_list_t *plist;
	if (list == NULL)
		return -1; /* Not Found */
	plist =list;
	while (plist) {
		if(plist->val == val)
			return 0; /* Found! */
		plist = plist -> next;
	}
	return -1; /* Not Found */
}


int check_between_int_list(int_list_t *list1, int_list_t *list2)
{
	int_list_t *plist = list2;
	while (plist) {
		if (check_int_list(list1, plist->val) == 0)
			break;
		plist = plist->next;
	} 

	return (plist !=NULL) ;
}


int_list_t *remove_int_list(int_list_t *list, int val)
{
	int_list_t *plist, *prev_plist=NULL;
	if (list == NULL)
		return NULL;

	plist = list;
	while(plist) {
		if(plist->val == val){
			if(prev_plist) {
				prev_plist->next = plist->next;
				return list;	
			}else {	/* the first one */
				return plist->next;
			}
		}	
		prev_plist = plist;
		plist = plist->next;
	}
	
	return NULL;
}

int insert_int_list(int_list_t **list, int val)
{
	int_list_t *plist;
	if (!list) {
		//nel_error(NULL, "input error \n");
		return -1;
	}
	plist = *list;
	if (check_int_list(plist, val) != 0) {
		plist = new_int_list(val, plist);
		if (plist == NULL)
			return -1;
	} 
	*list = plist;
	return 0;
}

int_list_t *cat_int_list(int_list_t *list1, int_list_t *list2)
{
	int_list_t *plist1, *plist2;
	if (!list1 && !list2) 
		return NULL;
	if (!list1) {
		plist1 = dup_int_list(list2);
		if (!plist1)
			return NULL;
	} else if (!list2) {
		plist1 = dup_int_list(list1);
		if (!plist1)
			return NULL;
	} else {
		plist1 = dup_int_list(list1);
		if (!plist1)
			return NULL;
		plist2 = list2;
		while (plist2) {
			if (check_int_list(plist1, plist2->val) == 0) {
				plist2 = plist2->next;
				continue;
			}
			plist1 = new_int_list(plist2->val, plist1);
			if (plist1 == NULL) {
				free_int_list(plist2);
				return NULL;
			}
			plist2 = plist2->next;
		}
	}
	return plist1;
}

int import_int_list(int_list_t **pplist, int_list_t *list)
{
	int_list_t *plist;
	plist = list;
	while (plist) {
		if (insert_int_list(pplist, plist->val) == -1)
			return -1;
		plist = plist->next;
	}
	return 0;
}


int_list_t *append_int_list(int_list_t *list1, int_list_t *list2)
{
	int_list_t *plist1, *plist2,
		*plist =NULL, *next =NULL ;

	if (!list1 && !list2 ) 
		return NULL;
	if (!list1) {
		return list2;
	} else if (!list2) {
		return list1; 
	} else {
		plist1 = list1; 
		plist2 = list2;
		while(plist1) { 
			plist = plist1; 
			plist1 = plist1->next;
		}
			
		while (plist2) {
			next = plist2->next;
			if (check_int_list(plist1, plist2->val) == 0) {
				plist2 = next;
				continue;
			}
			if(plist) 
				plist->next = plist2;	
			plist = plist2;

			/* this is important, otherwise nothing can be append 
			   because every plist2 is in plist1 when 
			   check_int_list(plist1, plist2->val) */
			plist2->next = NULL;

			plist2 = next;
		}
	}
	return plist1;
}

int_list_t *append_dup_list(int_list_t *list1, int_list_t *list2)
{
	return cat_int_list(list1, list2);
}

int_list_t *dup_list(int_list_t *list)
{
	return dup_int_list(list);
}

