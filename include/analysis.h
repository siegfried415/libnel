
/*
 * analysis.h
 * $Id: analysis.h,v 1.12 2006/09/26 10:33:02 wyong Exp $
 */

#ifndef ANALYSIS_H 
#define ANALYSIS_H 
#include <engine.h>

#define list_del(sh)							\
{									\
	if((sh)->next) (sh)->next->prev = (sh)->prev;			\
	if((sh)->prev) (sh)->prev->next = (sh)->next;			\
	(sh)->next = (sh)->prev = NULL;				\
}

#define list_add_tail(list, sh)					\
{									\
	(sh)->prev = (list)->prev;					\
	(list)->prev->next = (sh);					\
	(list)->prev = (sh);						\
	(sh)->next = (list);						\
}	

#define list_add_after(list, sh)					\
{									\
	(sh)->next = (list->next);					\
	(list)->next->prev  = (sh);					\
	(list)->next = (sh);					\
	(sh)->prev = (list);						\
}

#define list_entry(ptr, type, member) 					\
	((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

#define list_empty(list) ((list)->next == list)
#define list_until(list, sh) ((list)->next == sh )

struct token {
	int 	nel_count;
};

/* rewrite the following macro, wyong, 2006.6.1 */
#define NEL_REF_DEF  	 	int nel_count;
#define NEL_REF_SYM(eng, i)	(eng->symbolInfo[i])
#define NEL_REF_TYP(eng, i)	(NEL_REF_SYM(eng, i)->type)
#define NEL_REF_CLS(eng, i)	(NEL_REF_TYP(eng, i)->simple.type)
#define NEL_REF_CHK(eng, i, v)	( v != NULL				\
				  && NEL_REF_SYM(eng, i) != NULL	\
				  && NEL_REF_TYP(eng, i) != NULL	\
				)

#define NEL_REF_VAL(eng, i,v)	( NEL_REF_CHK(eng, i, v) 		\
				  ? ((struct token *)v)->nel_count 	\
				  : 0 					\
				)

#define NEL_REF_ZERO(eng,i,v)	( NEL_REF_CHK(eng, i, v) 		\
				  && ((struct token *)v)->nel_count==0  \
				)


#define NEL_REF_INIT(v)		do{ 					\
				    ((struct token *)v)->nel_count = 0;	\
				} while(0)	


#define NEL_REF_ADD(eng, i,v)	do{ if(NEL_REF_CHK(eng, i, v))		\
				    ((struct token *)v)->nel_count++;	\
				} while(0)

#define NEL_REF_SUB(eng, i,v)	do{ if(NEL_REF_CHK(eng, i, v))		\
				    ((struct token *)v)->nel_count--; 	\
				} while(0)


#define SYM_SET_NUM 16 
struct SymbolSet
{
	int num;
	struct token originToken;
	SymbolId symbolIds[SYM_SET_NUM];
};

#define initSymbolSet(symbolSetPtr) \
	memset((symbolSetPtr), 0, sizeof(struct SymbolSet))


struct StackNode
{
	StateId stateId;
	SymbolId symbolId;
	void *sval;
	struct Stack *foot;
	struct Stack *root;
};

#define STACKMAXNUM 256 
struct StackHead{
	struct StackHead *next;
	struct StackHead *prev;
	SymbolId id;		
	void *sval;
	int  need_external_token;
	int  priority;
};

struct Stack
{
	struct StackHead head;
	int num;
	struct StackNode stackNodes[STACKMAXNUM];
	int used;
	int recovery_mode;
};


struct nel_env 
{
	struct StackHead wait_queue[2];	
	struct StackHead work_queue;	
	
	// 0 or 1, indicate which wait_queue is in use
	int flag ;	

	int finished;			
	int recovery_mode;		
};

extern int nel_env_init(struct nel_eng *, struct nel_env *, SymbolId , void *);
extern int nel_env_analysis(struct nel_eng *, struct nel_env *, SymbolId , void *);
extern void nel_env_cleanup(struct nel_eng *, struct nel_env *);


#endif
