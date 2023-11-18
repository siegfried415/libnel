

#ifndef CLASS_H
#define CLASS_H


struct classInfo {
	nel_symbol *classifier;
	int 	   length;
};

union nel_TYPE *nel_class_functype_alloc(struct nel_eng *eng, struct lritem *it, nel_symbol **result);
nel_symbol *nel_class_func_alloc(struct nel_eng *, FILE *fp, char *name, union nel_TYPE *type, union nel_STMT *head, union nel_STMT *tail);


union nel_TYPE *nel_1_class_functype_alloc(struct nel_eng *, nel_symbol *sym, nel_symbol **result);

/*
nel_symbol *nel_at_class_func_alloc(struct nel_eng *, char *name, union nel_TYPE *type, union nel_STMT *head, union nel_STMT *tail);
*/


extern int class_alloc(struct nel_eng *eng);
extern void emit_term_class(struct nel_eng *eng /*,FILE *fp*/);
extern void emit_nonterm_class(struct nel_eng *eng /*,FILE *fp*/);
//extern void emit_prod_action(struct nel_eng *eng/*,FILE *fp*/);





#endif
