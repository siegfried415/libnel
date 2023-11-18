#ifndef ACTION_H
#define ACTION_H

union nel_TYPE *nel_reduction_functype_alloc(struct nel_eng *, struct nel_SYMBOL *prod);

nel_symbol *nel_reduction_func_alloc(struct nel_eng *, FILE *fp, char *name, union nel_TYPE *type, union nel_STMT *head);

#endif
