/*
 * int_list.h
 */

#ifndef __INT_LIST_H
#define __INT_LIST_H

typedef struct _int_list_t {
	int			val;
	struct _int_list_t	*next;
} int_list_t;

extern int count_int_list(int_list_t *list);
extern int_list_t *new_int_list(int val, int_list_t *next);
extern int_list_t *dup_int_list(int_list_t *list);
extern int free_int_list(int_list_t *list);
extern int check_int_list(int_list_t *list, int val);
extern int insert_int_list(int_list_t **list, int val);
extern int_list_t *cat_int_list(int_list_t *list1, int_list_t *list2);
extern int import_int_list(int_list_t **pplist, int_list_t *list);
extern int_list_t *remove_int_list(int_list_t *list, int val);
extern int_list_t *append_int_list(int_list_t *list1, int_list_t *list2);


#endif
