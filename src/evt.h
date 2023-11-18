#ifndef EVENT_H
#define EVENT_H
#include <engine.h>
#include <termset.h>
#include "sym.h"

#if 0
struct nel_EVENT
{
        int pid;                /* wyong, 2004.6.1 */
        int cyclic;             /* */
        TerminalSet first;      /* */
        TerminalSet follow;     /* */

        /* event 's extend field, wyong, 2005.3.23 */
        int state;              /* */
        int deep;               /* */
        int flag;
        //union nel_EXPR *expr;
        //union nel_EXPR *expr2;        /* wyong, 2004.5.23 */
        //struct nel_SYMBOL *at_class;
        //struct nel_SYMBOL *default_class;
        //int nodelay;          /* */
        int isEmptyString;      /* */
        int reachable;          /* */

        struct nel_SYMBOL *parent;

} ;
#endif

extern nel_symbol *lookup_event_symbol(struct nel_eng *, char *name);
extern nel_symbol *event_symbol_copy(struct nel_eng *, nel_symbol *);
extern nel_symbol *event_symbol_alloc(struct nel_eng *eng, char *name, union nel_TYPE *,int, int , union nel_EXPR *);
extern void event_symbol_dealloc(struct nel_eng *eng, nel_symbol *); 
extern void emit_symbol(FILE *, nel_symbol *);

extern int evt_expr_resolve(struct nel_eng *eng, register union nel_EXPR * /*,int flag*/, int offset, int count, union nel_TYPE *type/*, union nel_TYPE *type0*/);

extern void emit_terminals(struct nel_eng *eng /*,FILE *fp*/);
extern void emit_nonterminals(struct nel_eng *eng /*,FILE *fp*/ );

//added by zhangbin, 2006-7-21
struct nel_EVENT *nel_event_alloc(struct nel_eng *eng);
void nel_event_dealloc(struct nel_EVENT *evt);
void event_dealloc(struct nel_eng *eng);
//end

#endif
