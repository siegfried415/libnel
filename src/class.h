

#ifndef CLASS_H
#define CLASS_H

#include "item.h" 
#include "stmt.h" 

struct classInfo {
	nel_symbol *classifier;
	int 	   length;
};

union nel_TYPE *nel_class_functype_alloc(struct nel_eng *eng, struct lritem *it, nel_symbol **result);
nel_symbol *nel_class_func_alloc(struct nel_eng *, FILE *fp, char *name, union nel_TYPE *type, union nel_STMT *head, union nel_STMT *tail);

union nel_TYPE *nel_1_class_functype_alloc(struct nel_eng *, nel_symbol *sym, nel_symbol **result);

int class_alloc(struct nel_eng *eng);
void emit_term_class(struct nel_eng *eng );
void emit_nonterm_class(struct nel_eng *eng );

int create_classify_func (struct nel_eng *eng, nel_symbol *symbol);
nel_stmt *class_output_stmt_alloc(struct nel_eng *eng, nel_symbol *result, nel_symbol *out_symbol); 

#endif
