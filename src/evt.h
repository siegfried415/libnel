#ifndef EVENT_H
#define EVENT_H
#include "engine.h"
#include "termset.h"
#include "sym.h"

nel_symbol *lookup_event_symbol(struct nel_eng *, char *name);
nel_symbol *event_symbol_copy(struct nel_eng *, nel_symbol *);
nel_symbol *event_symbol_alloc(struct nel_eng *eng, char *name, union nel_TYPE *,int, int , union nel_EXPR *);
void event_symbol_dealloc(struct nel_eng *eng, nel_symbol *); 
void emit_symbol(FILE *, nel_symbol *);

int evt_expr_resolve(struct nel_eng *eng, register union nel_EXPR *, int offset, int count, union nel_TYPE *type );

extern void emit_terminals(struct nel_eng *eng );
extern void emit_nonterminals(struct nel_eng *eng );

struct nel_EVENT *nel_event_alloc(struct nel_eng *eng);
void nel_event_dealloc(struct nel_EVENT *evt);
void event_dealloc(struct nel_eng *eng);

#endif
