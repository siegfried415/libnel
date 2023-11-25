#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>


#include "engine.h"
#include "type.h"
#include "analysis.h"
#include "mem_pool.h"
#include "gen.h"
#include "errors.h"
#include "intp.h"
#include "class.h" 
#include "itemset.h"
#include "prod.h"
#include "io.h" 
#include "mem.h"

typedef void(* FREE_FUNC)(void *); 
#define REALLOC_LEN 10
#define PRIORITY(s) ((s!=NULL)? (eng->stateTable[(s)->stackNodes[(s)->num-1].stateId].priority): -1)
ObjPool_t stack_pool;


/*****************************************************************************/
/* nel_trace behaves like nel_printf (), but only when the "nel_tracing" flag*/
/* is set.                                                                   */
/*****************************************************************************/
void analysis_trace (struct nel_eng *eng, char *message, ...)
{
        if (eng && eng->analysis_verbose_level  >= 3 ) {
                va_list args;
                va_start (args, message);

		/*******************************************/
		/* error message i/o is a critical section */
		/*******************************************/
		nel_lock (&nel_error_lock);

                nel_do_print (stderr, message, args);

		/*************************/
		/* exit critical section */
		/*************************/
		nel_unlock (&nel_error_lock);

                va_end (args);
        }
}



/*****************************************************************************/
/* analysis_error () prints out an error message, together with the current input*/
/* filename and line number, increments eng->intp->error_ct, and returns.    */
/*****************************************************************************/
int analysis_error (struct nel_eng *eng, char *message, ...)
{
	va_list args;

	if ( eng && (eng->analysis_verbose_level >= 0 )) {
		va_start (args, message);

		/*******************************************/
		/* error message i/o is a critical section */
		/*******************************************/
		nel_lock (&nel_error_lock);

		nel_do_print (stderr, message, args);
		fprintf (stderr, "\n");

		/******************************************/
		/* exit critical section                  */
		/******************************************/
		nel_unlock (&nel_error_lock);
		va_end (args);
	}

	return (0);
}

inline int createStackPool(ObjPool_t *objPool, int max)
{
	return create_mem_pool(objPool, sizeof(struct Stack), max);
}

inline int destroyStackPool(ObjPool_t *objPool)
{
	return destroy_mem_pool(objPool);
}



static inline struct Stack *del_from_foot_list(struct nel_eng *eng, struct nel_env *env, struct Stack *s, int count)
{
	struct Stack *p, *c;
	struct Stack *ret = (struct Stack *)0;

	if(s->stackNodes[count].foot != s ) {
		/* first get previous stack 'p', it may be 's' itself */
		c = s->stackNodes[count].foot;		
		do {
			p = c; 
			c = c->stackNodes[ c->num - 1 ].foot; 
			if(PRIORITY(p) > PRIORITY(ret)){
				ret = p;
			}
		} while( c != s ) ;

		/*then get current stack 's' out from 'root' list */
		p->stackNodes[ p->num - 1 ].foot = s->stackNodes[count].foot;
		s->stackNodes[count].foot = s;
	}

	return ret;
}


static inline void release( struct nel_eng *eng, int symbolId, void *sval)
{
	nel_symbol *sym, *func ;
	char ret;

	//nel_debug({analysis_trace(eng, "--->release: sval =%p, count=%d!\n", sval, NEL_REF_VAL(eng, symbolId, sval));});
	if(!sval){
		nel_debug({analysis_trace(eng, "--->release: sval == NULL!\n", sval);});
		return;
	}

	sym = (eng->symbolInfo)[symbolId];
	if(sym == NULL) {
		nel_debug({analysis_trace(eng, "--->free semantic value (%p), sym =NULL!\n", sval);});
		return;
	}

	NEL_REF_SUB(eng, symbolId, sval);
	if(NEL_REF_ZERO(eng, symbolId, sval)){
		func= sym->type->event.free_hander;
		if(func){
			if(func->class == nel_C_COMPILED_FUNCTION 
				|| func->class == nel_C_LOCATION ){
				(*((FREE_FUNC)func->value)) ( sval );
			}
			else{
				nel_func_call(eng, &ret, func, sval);
			}
			nel_debug({analysis_trace(eng, "--->free semantic value (%p) !\n", sval);});
		}

		else {
			/* it 's better to do nothing */
		}

	}

	return;
}

static inline int free_stack(struct nel_eng *eng, struct nel_env *env, struct Stack *s, int pos)
{
	struct Stack *c; 
	int i;
	nel_debug({analysis_trace(eng, "--->free_stack(%p)\n", s);});

	/*if other stack inked in, fork them and release recursively */
	for(i = pos ; i >= 0; i--){
		c = del_from_foot_list(eng, env, s, i ); 
		if( c != (struct Stack *)0 ) {
			nel_debug({analysis_trace(eng, "--->linked stack(%p) forked\n", c);});
			free_stack(eng, env, c, c->num - 1);
		}

		release(eng, s->stackNodes[i].symbolId,s->stackNodes[i].sval);
	}

	return free_mem(&stack_pool, TO_OBJ(s));

}

static inline struct Stack * alloc_stack(void)
{
	struct Stack *tmp;
	ObjPool_t *objPool = &stack_pool;

	objPool->size  = sizeof(struct Stack); 
	tmp = alloc_mem(objPool) ; 
	return tmp?TO_DATA(tmp):0;
}

static inline int merge_stack(struct nel_eng *eng, struct nel_env *env, struct Stack *s, SymbolId id, void *sval, int new_state_id)
{
	struct StackHead *sh1, *sh2;
	int i;
	int flag = 0;

	sh1 = &env->work_queue;
search_again:
	sh2 = sh1->next;
	while( sh2 != sh1 )
	{
		struct Stack *t = list_entry(sh2, struct Stack, head);
		StateId state = t->stackNodes[t->num-1].stateId;

		if(state == new_state_id 
		&& t->stackNodes[t->num-1].sval == sval){

			/* found a stack can be merged */
			int off = eng->stateTable[state].dot_pos;

			if( off > 0 && off <= s->num && off <= (t->num-1)){
				nel_debug({analysis_trace(eng, "--->stack(%p) have same state with this stack(%p), merge it!\n", t, s);});

				for(i = s->num - 1; i > s->num - off; i-- ) {
					release(eng, s->stackNodes[i].symbolId,  s->stackNodes[i].sval );	
				}
				s->stackNodes[s->num - off].foot
					= t->stackNodes[t->num - off - 1].foot; 
				t->stackNodes[t->num - off - 1 ].foot= s;

				s->num = s->num - off + 1; 
				s->used = 1;	
				return 1;
			}
			
			/* can't merge, continue to search  */

		}
		sh2 = sh2->next;
	}

	if( flag ++ == 0 ) {
		sh1 = &env->wait_queue[!env->flag];
		goto search_again;
	}

	/* didn't found stack can be merged */
	return 0;

}


static inline struct Stack *copy_stack(struct nel_eng *eng, struct Stack *s, int count)
{		
	int i;
	struct Stack *n = (struct Stack *)0;

	if(( n = alloc_stack() )){

		/* copy every stack node info from s to n, set root and 
		foot information when necessary. */
		for(i=0; i < count; i++){

			/* setting root */
			n->stackNodes[i].root= s->stackNodes[i].root;
			s->stackNodes[i].root = n;

			/* setting foot */
			n->stackNodes[i].foot = n;
			if ( s != s->stackNodes[i].foot 
			&& s->num > ((struct Stack *)s->stackNodes[i].foot)->num ) {
				/* which means the stack s is an proxy stack 
				for some others stack, create duplicated stack 
				for those stack too */
				struct Stack *nn = copy_stack(eng, s->stackNodes[i].foot, i + 1);
				n->stackNodes[i].foot = nn->stackNodes[i].foot;
				nn->stackNodes[i].foot  = n;	
			}

			n->stackNodes[i].stateId =s->stackNodes[i].stateId;
			n->stackNodes[i].symbolId=s->stackNodes[i].symbolId;
			n->stackNodes[i].sval =s->stackNodes[i].sval;
			if(s->stackNodes[i].sval){
				NEL_REF_ADD(eng, s->stackNodes[i].symbolId,
						s->stackNodes[i].sval);
			}
		}

		n->num  = count; 
		n->used   = 0;
	}
	return n; 
}


int addSymbolId(struct SymbolSet *result, int id)
{
	if(result) {
		if(result->num < SYM_SET_NUM  ) {
			result->symbolIds[result->num++] = id; 
		}
	}
	return 0;
}

int add_symbol_id_init(struct nel_eng *eng)
{
	nel_type *type;
	nel_symbol *symbol;
	nel_list *args;


	symbol = nel_lookup_symbol("addSymbolId", eng->nel_static_ident_hash, eng->nel_static_location_hash,  NULL);
	if(symbol == NULL) {
		/* bytes */
		symbol = nel_lookup_symbol("SymbolSet", eng->nel_static_tag_hash,NULL);
		type = symbol->type->typedef_name.descriptor;
		type = nel_type_alloc(eng, nel_D_POINTER, sizeof(struct SymbolSet *), nel_alignment_of(struct SymbolSet *), 0,0,type);
		symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,"result"), type, NULL, nel_C_FORMAL, nel_lhs_type(type), nel_L_C, 1);
		args = nel_list_alloc(eng, 0, symbol, NULL);


		symbol = nel_static_symbol_alloc(eng, nel_insert_name(eng,"id"), nel_int_type, NULL, nel_C_FORMAL, nel_lhs_type(nel_int_type), nel_L_C, 1);
		args = nel_list_alloc(eng, 0, symbol, args);


		type = nel_type_alloc (eng, nel_D_FUNCTION, 0, 0, 0, 0, 0, 0, nel_int_type, args, NULL, NULL);
		symbol = nel_static_symbol_alloc (eng, nel_insert_name(eng, "addSymbolId"), type, (char *) addSymbolId, nel_C_COMPILED_FUNCTION, nel_lhs_type(type), nel_L_C, 0);

		nel_insert_symbol (eng, symbol, eng->nel_static_ident_hash);

	}else if(symbol->value == NULL) {
		symbol->value = (char *) addSymbolId;

	}else if(symbol->value != (char *)addSymbolId) {
		//analysis_warning(eng, "the earily inserted symbol value have difference value with addSymbolId!\n");

	}
	else {
		/* addSymbolId was successfully inserted */
	}

	return 0;

}

int eval_class_func(struct nel_eng *eng, nel_symbol *func, int len,  struct Stack *s, void *sval, struct SymbolSet *set )
{
        /********************************************/
	/* register variables are not guaranteed to */
	/* survive a longjmp () on some systems     */
	/********************************************/
        nel_jmp_buf return_pt;
        struct eng_intp *old_intp = NULL;
        int val;
        nel_symbol *retval;
	char name[16];
	int i; 
	nel_symbol *symbol ; 

	if(eng->intp != NULL){
		old_intp = eng->intp;
	}

	/*********************************************/
	/* initialize the engine record that is passed */
	/* thoughout the entire call sequence.       */
	/* nel_intp_eval () will set the filename and */
	/* line numbers from the stmt structures     */
	/*********************************************/
	nel_malloc(eng->intp, 1, struct eng_intp );
	intp_init (eng, "", NULL, NULL, NULL, 0);

	//add symbol for "set", 
	symbol = nel_static_symbol_alloc(eng, "result", nel_pointer_type, intp_dyn_value_alloc (eng, sizeof(char *), nel_alignment_of(char *)), nel_C_LOCAL, 0, nel_L_NEL, 0 );
	*((char **) (symbol->value)) = (char *)set; 
	intp_push_symbol(eng, symbol);

	//add symbol for input term/nonterm
	symbol = nel_static_symbol_alloc(eng, "input", nel_pointer_type, intp_dyn_value_alloc (eng, sizeof(char *), nel_alignment_of(char *)) , nel_C_LOCAL, 0, nel_L_NEL, 0 );
	*((char **) (symbol->value)) = (char *)sval; 
	intp_push_symbol(eng, symbol);

	for (i =1; i<=len; i++ ) {
		sprintf(name, "$%d", i ); 
		symbol = nel_static_symbol_alloc(eng, name, nel_pointer_type, intp_dyn_value_alloc (eng, sizeof(char *), nel_alignment_of(char *)) , nel_C_LOCAL, 0, nel_L_NEL, 0 );
		*((char **) (symbol->value)) = (char *)s->stackNodes[s->num-i].sval; 
		intp_push_symbol(eng, symbol);
	}

	//add symbol for "$0", 
	symbol = nel_static_symbol_alloc(eng, "$0", nel_pointer_type, intp_dyn_value_alloc (eng, sizeof(char *), nel_alignment_of(char *)) , nel_C_LOCAL, 0, nel_L_NEL, 0 );
	*((char **) (symbol->value)) = (char *)s->stackNodes[0].sval; 
	intp_push_symbol(eng, symbol);

	eng->intp->ret_type = func->type->function.return_type;

	/**********************/
	/* call the evaluator */
	/**********************/
	eng->intp->return_pt = &return_pt;
	intp_setjmp (eng, val, &return_pt);
	if (val == 0) {
		/*******************************************************/
		/* retreive a pointer to the start of the pushed args. */
		/* and call intp_eval_call() to perform the call.        */
		/*******************************************************/
		nel_stack *arg_start ; 
		int nargs = len + 3;

		intp_top_pointer (eng, arg_start, nargs - 1);
		retval = intp_eval_call(eng, func, nargs, arg_start );
	}


	/*******************************************************/
	/* deallocate any dynamically allocated substructs     */
	/* withing the engine (if they were stack allocated) */
	/* and return.                                         */
	/*******************************************************/
	retval = nel_symbol_dup(eng, retval);  
	intp_dealloc (eng);

	nel_debug ({ analysis_trace (eng, "] exiting eval_class_func ()\nerror_ct = %d\n\n", eng->intp->error_ct); });
	
	nel_dealloca(eng->intp);
	if(old_intp != NULL){
		eng->intp = old_intp;
	}

	return *((int *)retval->value); 
}


static inline int do_class(struct nel_eng *eng, struct nel_env *env, struct Stack *s, SymbolId id, void *sval, struct SymbolSet *set)			
{
	//struct classInfo *class;
	nel_symbol *func;
	int ret; 
	int len;
	StateId topState = s->stackNodes[s->num-1].stateId;
	set->num = 0;

	nel_debug({analysis_trace(eng, "--->do_class, stack=%p, topState=%d, id=%d, sval=%p!\n", s, topState, id, sval);});
	if(id != 0) {

		int toknum, index;
		struct classInfo *class;

		toknum = eng->numTerminals + eng->numNonterminals + 1;
		index = topState * toknum + eng->numNonterminals + id;
		class = &eng->classTable[index];
		if(class == NULL){
			nel_debug({analysis_trace(eng, "--->do_class, class=NULL!\n");});
			return 0; 
		}

		func = class->classifier;
		len  = LEN_OF_CLASS(eng, topState, id);

	}
	else if(sval == NULL ){  
		return 0;
	}

	if(!func) {
		return 0;
	}

	nel_debug({analysis_trace(eng, "--->do_class, func=%s, func len =%d!\n", func->name, len);});

	ret = eval_class_func(eng, func, len, s, sval, set);
	return ret;
}

#if 0
int print_queue(struct nel_eng *eng, struct nel_env *env)
{
	struct StackHead *scan, *end;
	nel_debug({analysis_trace(eng, "--->print_work_queue of env(%p)!\n", env);});
	end = &env->work_queue; 
	for( scan = end->next; scan != end;  scan = scan->next){
		nel_debug({analysis_trace(eng, "--->stackHead(%p)!\n", scan);});
	}

	nel_debug({analysis_trace(eng, "--->print_wait_queue of env(%p)!\n", env);});
	end = &env->wait_queue[!env->flag]; printf("scan=%p\n", end);
	for( scan = end->next; scan != end;  scan = scan->next){
		nel_debug({analysis_trace(eng, "--->stackHead(%p)!\n", scan);});
	}

	return 0;

}
#endif

static inline void add_to_work_queue(struct nel_eng *eng, struct nel_env *env,  struct Stack *stack, SymbolId id,  void *sval)
{
	struct StackHead *list, *next, *prev, *sh;
	StateId topState;

	nel_debug({analysis_trace(eng, "--->add stack(%p) to work queue!\n", stack);});
	list = &env->work_queue;
	sh = &stack->head;	
	topState = stack->stackNodes[stack->num-1].stateId;
	sh->priority = eng->stateTable[topState].priority;
			
	sh->need_external_token = 0;
	sh->id = id; 
	sh->sval = sval; 
	/* sh->type = type; */
	prev = list;
	next= list->next;

	while( next != list && next->priority < sh->priority ) {
		prev = next;
		next = next->next;
	}
	list_add_after(prev, sh);

	/* Originally we add lhs 's count at next do_stack_shift, 
	but zzh found the cases which another stack will incorrectly 
	free the sval. */
	if(sval){
		NEL_REF_ADD(eng, id, sval);
	}

	return;

}

static inline void add_to_wait_queue(struct nel_eng *eng, struct nel_env *env, struct Stack *stack)
{
	struct StackHead *list, *next, *prev, *sh;
	StateId topState;

	nel_debug({analysis_trace(eng, "--->add stack(%p) to wait queue!\n", stack);});
	list = &env->wait_queue[!env->flag];
	sh = &stack->head;	
	topState = stack->stackNodes[stack->num-1].stateId;
	sh->priority = eng->stateTable[topState].priority;
			
	sh->need_external_token = 1;
	prev = list;
	next= list->next;

	while( next != list && next->priority < sh->priority ) {
		prev = next;
		next = next->next;
	}
	list_add_after(prev, sh);

	return;
}


static inline int do_stack_shift(struct nel_eng *eng, struct nel_env *env, struct Stack *s, SymbolId id, void *sval, StateId new_state_id)
{
	StateId topState;

	nel_debug({analysis_trace(eng, "--->do_stack_shift for stack(%p), symbolid=%d, sval=%p, count=%d\n",s, id, sval, NEL_REF_VAL(eng, id, sval));});
	if( new_state_id <= 0 ||  new_state_id > eng->numStates )
	{
		return -1;
	}

	/* if new stack can merge on other stack, merge it and return */
	if(merge_stack(eng, env, s, id, sval, new_state_id ) > 0 ){
		return 0;
	}

	/* procet the stack from overflow */
	if( s->num >= STACKMAXNUM ){	
		analysis_error(eng, "--->stack(%p) 's number is overflowed(%d) !\n", s, s->num);
		return -1;
	}
					
	/* we can't get stack which have same state with s from 0 to
	s->num, so simple set root with itself. */
	s->stackNodes[s->num].root = s;

	/* there is no stack which can be proxy of stack s , so set foot with itself. */
	s->stackNodes[s->num].foot = s ;

	s->stackNodes[s->num].stateId = new_state_id; 
	s->stackNodes[s->num].symbolId = id; 
	s->stackNodes[s->num].sval = sval;


	if(s->stackNodes[s->num].sval ) {
		NEL_REF_ADD(eng, id, s->stackNodes[s->num].sval);
	}

#if 0
	/* deal with timeout */
	timeout = eng->state_timeout_tbl[new_state_id];
	if(timeout < 0 ) {
		/* stop the current timer, when state timeout == 0; */
		timux_deljob(eng->timemux, s->stackNodes[s->num-1].timer );
		if(++timeout < 0 ) {
			timeout =  -timeout;
		}
	}
	
	if( timeout > 0 ) {
		/* start an timer, when state timeout > 0; */
		s->stackNodes[s->num].timer = 
			timux_addjob(eng->timemux, 
			(duty)env_timeout_handler,  /* timeout_stmts*/
			eng, env, s,  		   /* parameter    */
			timeout);
	}
#endif

	s->num++;
	//s->used = 1;	
	return 1;

}


static inline int stack_sr_shift(struct nel_eng *eng, struct nel_env *env, struct Stack *c, SymbolId id, void *sval, int new_state_id)
{
	StateId topState;
	int ret;

	nel_debug({analysis_trace(eng, "--->stack_sr_shift for stack(%p), id = %d, new_state_id =%d!\n", c, id, new_state_id );});
	//c->used = 0;
	if ((ret = do_stack_shift(eng, env, c, id, sval, new_state_id)) < 0 ){
		/* if no noterm acceptable, release the stack*/
		nel_debug({analysis_trace(eng, "--->stack(%p) be freed\n", c);});
		free_stack(eng, env, c, c->num - 1 );
		return -1;
	}
	else if (ret == 0) {
		/* c has merage on other stack, so skip the next process */
		return ret;
	}

	/* if stack c is in recovery_mode, change to normal*/ 
	if( c->recovery_mode > 0 ) {
		c->recovery_mode = 0;
	}

	/* put the stack and un-consumed event into work queue, 
	use priority to determine the position be inserted */
	topState = c->stackNodes[c->num-1].stateId;

	c->used = 0;
	if(eng->stateTable[topState].has_shift > 0 ) {	
		add_to_wait_queue(eng, env, c);
		c->used = 1;
	}

	/* if SR/RR conflicit, copy the stack and continue the process. */
	if (eng->stateTable[topState].has_reduce > 0 ){
		if(c->used ==1){
			c = copy_stack(eng, c, c->num ); 
		}

		add_to_work_queue(eng, env, c, 0, NULL);
		c->used = 1;

	}

	return ret;

}

/* handle process SS shift */
static inline int stack_ss_shift(struct nel_eng *eng, struct nel_env *env, struct Stack *s, SymbolId id, void *sval)
{	
	struct Stack *c, *b;
	StateId topState;
	StateId new_state_id;
	int entry;
	int id_acpt_cnt=0;
	int ret;
	nel_debug({analysis_trace(eng, "--->stack_ss_shift for stack(%p), id=%d,sval=%p!\n", s,id,sval);});

	/* use action table at a terminal, and gotoTable at a noneterminal */
	topState = s->stackNodes[s->num-1].stateId;
	entry = (id>0)? actionEntry(eng, topState, decodeSymbolId(eng,id))
			: gotoEntry(eng, topState, decodeSymbolId(eng,id));
			/* unsigned int  ->  int */

	if(isAmbigAction(eng, entry)) {
		int start = decodeAmbig(eng, entry);
		int total = eng->ambigAction[start];
		int i;

		nel_debug({analysis_trace(eng, "--->stack_ss_shift for stack(%p), ambigAction = %d, total =%d!\n", s, entry, total);});

		for(i = 1, c = s; i <= total; i++) {
			int shift = eng->ambigAction[start + i];
			if ((new_state_id = ( id > 0 ) ? decodeShift(eng,shift) 
				: decodeGoto(eng, shift)) > 0 ){
				b = (i < total) ? copy_stack(eng, c, c->num ) 
						: NULL;

				if ((ret = stack_sr_shift(eng, env, c, id, sval, new_state_id)) >= 0 ) {
					id_acpt_cnt++;
				}
				c = b;
				
			}
		}
		return 1;
	}

	new_state_id = (id>0) ? decodeShift(eng,entry) : decodeGoto(eng, entry);
	nel_debug({analysis_trace(eng, "--->stack_ss_shift for stack(%p), id = %d, new_state_id =%d!\n", s, id, new_state_id );});
	if ((ret = stack_sr_shift(eng, env, s, id, sval, new_state_id )) >= 0 ){
		id_acpt_cnt++;
	}
	
	if (id_acpt_cnt == 0){
		if(sval){
			NEL_REF_ADD(eng, id, sval);
			release(eng, id, sval);	
		}
	}	
	
	return 1;

}





static inline int stack_shift(struct nel_eng *eng, struct nel_env *env, struct Stack *s, SymbolId id, void *sval)
{
	StateId topState;
	struct SymbolSet set;
	int ret = -1;
	int i; 
	struct Stack	*c, *b, *nc; 
	int id_acpt_cnt = 0;

	nel_debug({analysis_trace(eng, "--->stack_shift for stack(%p)!\n", s);});
	/* get the most left rhs of the rule */ 
	c = s;
	c->used = 0;
	nc = del_from_foot_list(eng, env, c, c->num - 1 ); 
	
	do {
		/* do terminal or nonterminal classification */ 
		set.num = 0;
		if(id==encodeSymbolId(eng,eng->timeoutSymbol->id,nel_C_TERMINAL)
			|| id==encodeSymbolId(eng, eng->notSymbol->id,nel_C_TERMINAL)
			|| id==encodeSymbolId(eng,eng->otherSymbol->id,nel_C_TERMINAL)){
			set.symbolIds[set.num++] = id; 
		}
		else {
			do_class(eng, env, c, id, sval, &set);
		}

		/* we have't got an expect event,fake an 'others' event,
		and feed it to analiazer. */
		if( set.num == 0) {

#if 0
			set.symbolIds[set.num++] = encodeSymbolId(eng,
				eng->otherSymbol->id,nel_C_TERMINAL);
			release(eng, id, sval);
			sval = NULL;
			if( c->recovery_mode == 1 ) {
				nel_debug({analysis_trace(eng, "--->put stack(%p) into wait queue!\n", c);});
				add_to_wait_queue(eng, env, c);
				/* in recovery mode, we consume all input 
				event that we can't recognized, 2004.11.26 */
				goto do_next_c;
			}

			/* stack now is in the recovery mode, in which only 
			process those event we can recognized */
			c->recovery_mode = 1;

#else

			nel_debug({analysis_trace(eng, "--->unexpected input event!\n");});
			free_stack(eng, env, c, c->num - 1 );
#endif

		}

		nel_debug({
			analysis_trace(eng, "--->stack(%p) Event(%p) classified to %d noneTerms:",c, sval, set.num);
			for(i=0; i< set.num ; i++){
				analysis_trace(eng, "%d,",set.symbolIds[i]);
			}
			analysis_trace(eng, "\n");
		});

		for ( i = 0; i < set.num; i ++) {
			//if(c->used ==1){
			if( i < ( set.num -1 ) ) {
				b = copy_stack(eng, c, c->num );	
			}

			id_acpt_cnt++;

			if (( ret = stack_ss_shift(eng, env, c, set.symbolIds[i], sval)) == 0 ) {
				/* the stack is merged, so needn't do anything*/
				continue;
			}

			c = b;

		}
do_next_c:
		c = nc;
		if( nc != (struct Stack *)0 ) {
			nel_debug({analysis_trace(eng, "--->linked stack(%p) forked\n", nc);});
			nc = del_from_foot_list(eng, env, c, c->num - 1);
			c->used = 0;
		}

	} while(c != (struct Stack *)0);	
	
	
	if(id_acpt_cnt == 0 ){
		nel_debug({analysis_trace(eng, "--->stack_shift, id not accept, free it(%d,%p)!\n", id, sval); });
		if(sval){
			NEL_REF_ADD(eng, id, sval);
			release(eng, id, sval);	
		}
	}	

	return 1;

}


int eval_reduce_func(struct nel_eng *eng, nel_symbol *func, char**psval,  int len,  struct Stack *s )
{
        /********************************************/
	/* register variables are not guaranteed to */
	/* survive a longjmp () on some systems     */
	/********************************************/
        nel_jmp_buf return_pt;
        struct eng_intp *old_intp = NULL;
        int val;
        nel_symbol *retval;

	char name[16];
	int i; 
	nel_symbol *symbol ; 

	if(eng->intp != NULL){
		old_intp = eng->intp;
	}

	/*********************************************/
	/* initialize the engine record that is passed */
	/* thoughout the entire call sequence.       */
	/* nel_intp_eval () will set the filename and */
	/* line numbers from the stmt structures     */
	/*********************************************/
	nel_malloc(eng->intp, 1, struct eng_intp );
	intp_init (eng, "", NULL, NULL, NULL, 0);


	for (i =1; i<= len; i++ ) {
		sprintf(name, "$%d", i ); 
		symbol = nel_static_symbol_alloc(eng, name, nel_pointer_type, intp_dyn_value_alloc (eng, sizeof(char *), nel_alignment_of(char *)), nel_C_LOCAL, 0, nel_L_NEL, 0 );
		*((char **) (symbol->value)) = (char *)s->stackNodes[s->num-i].sval; 
		intp_push_symbol(eng, symbol);
	}

	//add symbol for "$0", 
	symbol = nel_static_symbol_alloc(eng, "$0", nel_pointer_type, intp_dyn_value_alloc (eng, sizeof(char *),nel_alignment_of(char *)), nel_C_LOCAL, 0, nel_L_NEL, 0 );
	*((char **) (symbol->value)) = (char *)s->stackNodes[0].sval;
	intp_push_symbol(eng, symbol);


	eng->intp->ret_type = func->type->function.return_type;

	/**********************/
	/* call the evaluator */
	/**********************/
	eng->intp->return_pt = &return_pt;
	intp_setjmp (eng, val, &return_pt);
	if (val == 0) {
		/*******************************************************/
		/* retreive a pointer to the start of the pushed args. */
		/* and call intp_eval_call() to perform the call.        */
		/*******************************************************/
		nel_stack *arg_start ; 
		int nargs = len + 1;

		intp_top_pointer (eng, arg_start, nargs - 1);
		retval = intp_eval_call(eng, func, nargs, arg_start );
	}


	/*******************************************************/
	/* deallocate any dynamically allocated substructs     */
	/* withing the engine (if they were stack allocated) */
	/* and return.                                         */
	/*******************************************************/
	retval = nel_symbol_dup(eng, retval);  
	intp_dealloc (eng);

	nel_debug ({ analysis_trace (eng, "] exiting eval_reduce_func ()\nerror_ct = %d\n\n", eng->intp->error_ct); });
	
	nel_dealloca(eng->intp);
	if(old_intp != NULL){
		eng->intp = old_intp;
	}

	*((char **)psval) = *((char **)(retval->value));
	return 0; 
}


static inline int do_stack_reduce(struct nel_eng *eng, struct nel_env *env, struct Stack *s, int prodId)
{
	void *sval=NULL;
	int symbolId;
	nel_symbol *func;
	int i;
	int ret = 0;
	int len; 

	nel_debug({analysis_trace(eng, "--->do_stack_reduce for stack(%p)!\n", s);});
	s->used = 0;


	//call reduce function
	func = eng->prodInfo[prodId].action;
	if(!func) {
		analysis_error(eng, "no action for production(%d)\n", prodId);
		return -1;
	}

	nel_debug({analysis_trace(eng, "--->do_stack_reduce for stack(%p) before call_reduce_func(%s)!\n", s, func->name);});


	len = eng->prodInfo[prodId].rhsLen; 
	ret = eval_reduce_func( eng, func, (char **)&sval, len, s );	
	s->num -= len ;

	//nel_debug({analysis_trace(eng, "--->do_stack_reduce for stack(%p) call_reduce_func over!\n", s);});

	if(ret < 0) {
		analysis_error(eng, "call reduce function(%s) failed\n", func->name);
		return -1;
	}

	if((symbolId = eng->prodInfo[prodId].lhsIndex) == eng->target_id) {
		/* set lrParse finish flag */
		env->finished = 1;
	}

	/* 
	the code here is subtle, if we release $1 whose count equal 1 at first, 
	then we will get a junk pointer as $$.
	AS 	: AS a 
		{
			$$ = $1;	
		}
		;
	so we here add the count first, and then sub.(we will realy add count until
	we have the need and we have done), this way, we can solve the problem. 
	*/
	if(sval){
		NEL_REF_ADD(eng, symbolId, sval);
	}

	/* release all rhs 's value */ 
	for(i=0; i< eng->prodInfo[prodId].rhsLen; i++){
		release(eng, s->stackNodes[s->num+i].symbolId, s->stackNodes[s->num+i].sval);
		/* if s->stackNodes[s->num+i] in some root list, pick it up  */
	}


	if(sval){
		NEL_REF_SUB(eng, symbolId, sval);
	}

	nel_debug({analysis_trace(eng, "--->stack(%p) reduce with prod(%d), get noneTerminal(id=%d,sval=%p,count=%d)\n", s, prodId, symbolId, sval, NEL_REF_VAL(eng, symbolId, sval));});


#if 0
	if(eng->prodInfo[prodId].rel == REL_EX ){
		struct Stack *ns = del_from_root_list(eng, env, s, s->num -1 );
		env_not_handler(eng, env, ns, s->num - 1 );

		/* put the stack and un-consumed event into work queue, 
		use priority to determine the insert position */
		add_to_work_queue(eng,env, s,
			/* return 'other' rather than 'not' */
			encodeSymbolId(eng,eng->otherSymbol->id,nel_C_TERMINAL), sval );

		return 0;
	}
#endif

	add_to_work_queue(eng,env,s,symbolId,sval);	

	return 0;
}

int stack_reduce(struct nel_eng *eng, struct nel_env *env, struct Stack *s, ActionEntry entry)
{
	ActionEntry reduction;		
	int prodId;
	struct Stack *c, *b;

	nel_debug({analysis_trace(eng, "--->stack_reduce for stack(%p)!\n", s);});
	if(isAmbigAction(eng, entry)) {
		int start, total, i;
		start = decodeAmbig(eng, entry);
		total = eng->ambigAction[start];

		nel_debug({analysis_trace(eng, "--->stack_reduce for stack(%p), ambigAction = %d, total =%d!\n", s, entry, total);});

		for(i = 1, c = s; i <= total; i++) {
		
			reduction = eng->ambigAction[start + i];
			if ( (prodId=decodeReduce(eng, reduction)) >= 0 ){
				if(i < total){
					nel_debug({analysis_trace(eng, "--->stack_reduce for stack(%p), backup_stack(%p) !\n", c, b);});
					b = copy_stack(eng, c, c->num  );	
				}

				nel_debug({analysis_trace(eng, "--->stack_reduce for stack(%p), reduction= %d, prodId =%d!\n", c, reduction, prodId);});
				do_stack_reduce(eng, env, c, prodId); 
			}

			c = b;
	
		}
		return 0;
	}

	nel_debug({analysis_trace(eng, "--->stack_reduce for stack(%p), entry = %d!\n", s, entry);});
	/* otherwise, must be an reduction entry */
	prodId = decodeReduce(eng, entry);
	do_stack_reduce(eng, env, s, prodId); 

	return 0;

}


/* if input token consumed,  return 1  */
static inline int do_env_analysis(struct nel_eng *eng, struct nel_env *env, struct Stack *s, SymbolId id, void *sval)
{
	StateId topState;
	ActionEntry entry ;
	int prodId;

	nel_debug({analysis_trace(eng, "\n\n");});
	topState = s->stackNodes[s->num-1].stateId;

	/* use action table at a terminal, and gotoTable at a noneterminal */
	if(id > 0) { /* Termnial */
		entry = actionEntry(eng, topState, decodeSymbolId(eng,id));
		nel_debug({analysis_trace(eng, "--->do_env_analysis for stack(%p), Terminal= %d, entry=%d!\n", s, id, entry);});
	}
	else if( id < 0 ){ /* NoneTerminal*/
		entry = gotoEntry(eng, topState, decodeSymbolId(eng, id));
		nel_debug({analysis_trace(eng, "--->do_env_analysis for stack(%p), NonTerminal= %d, entry=%d!\n", s, id, entry);});
	}
	else {
		/*get entry from reduce table */
		nel_debug({analysis_trace(eng, "--->do_env_analysis for stack(%p), topsState=%d!\n", s, topState);});
		entry = reduceEntry(eng, topState);	
		nel_debug({analysis_trace(eng, "--->do_env_analysis for stack(%p), entry=%d!\n", s, entry);});
	
	}
	

	if(entry == 0 ){
		if(id != 0 /* when it is a reduce */
			&& id == encodeSymbolId(eng, eng->otherSymbol->id, nel_C_TERMINAL)){	
			stack_shift(eng,env, s, encodeSymbolId(eng,eng->otherSymbol->id,nel_C_TERMINAL), sval );
		}else {
			nel_debug({analysis_trace(eng, "--->unexpected input event!\n");});
			free_stack(eng, env, s, s->num - 1 );
		}

		return 0;
	}

	/* in case of state has both reduce and shift entry, check whether it is an shift */
	if (isShiftAction(eng, entry) 
		|| ( isAmbigAction(eng, entry) && id != 0 ) ){	
		nel_debug({analysis_trace(eng, "--->do_env_analysis for stack(%p), call stack_shift!\n", s);});
		stack_shift(eng, env, s, id, sval);
		//nel_debug({analysis_trace(eng, "--->do_env_analysis for stack(%p), call stack_sr_shift over !\n", s);});
		return 1;
	}


	/* OK, we have an certifiable reduction, call stack_reduce do the dirty things */
	nel_debug({analysis_trace(eng, "--->do_env_analysis for stack(%p), call stack_reduce!\n", s);});
	stack_reduce(eng, env, s, entry );
	//nel_debug({analysis_trace(eng, "--->do_env_analysis for stack(%p), call stack_reduce over !\n", s);});

	return 0;
}


int nel_env_analysis( struct nel_eng *eng, struct nel_env *env, SymbolId id, void *sval)
{
	struct Stack *stack; 
	struct StackHead *sh;
	sigset_t mask, oldmask;
	int consumed = 0;
	nel_symbol *sym;

	nel_debug({analysis_trace(eng, "\n\n--->start nel_env_analysis, eng=%p, id=%d, sval=%p\n", eng, id, sval);});

	/* if the symbol was unreachable, don't analysis it any more, and relase it ASAP */
	if( id <= 0 ) {  /*  id == 0 , unreachable, < 0 nonterminal */
		if(id<0) {
			nel_debug({analysis_trace(eng, "--->nonterminal input event detected!\n\n");});
		} else {
			nel_debug({analysis_trace(eng, "--->unreachable input event detected!\n\n");});
		}
		return 1;
	}else if(id == 
		encodeSymbolId(eng,eng->endSymbol->id,nel_C_TERMINAL)){
		/* force to free the env  */
		nel_env_cleanup(eng, env);
		
		/* go on */
	}
	
	if(id > eng->numTerminals) {
		nel_debug({analysis_trace(eng, "--->illegal input terminal id detected!\n\n");});
		return -1;
	}
	
	sigemptyset(&mask);
	sigaddset(&mask, SIGALRM);
	sigprocmask(SIG_BLOCK, &mask, &oldmask);


	env->flag = !env->flag;

	/* do reduce caused by the input event */	
	while(!list_empty(&env->wait_queue[env->flag])){
		stack=list_entry(env->wait_queue[env->flag].next,struct Stack,head);
		list_del(&stack->head);
		/* consumed += */ 
		do_env_analysis(eng, env, stack, id, sval);
		if(env->finished) break;
	}

	/* get the highest stack from env->work queue */
	while(!list_empty(&env->work_queue)) {
		sh = env->work_queue.next;

		stack =list_entry(sh, struct Stack, head);
		list_del(sh);

		if(env->finished == 1){
			nel_debug({analysis_trace(eng, "--->stack(%p) be freed\n", stack);});
			free_stack(eng, env, stack, stack->num - 1 );
			continue;
		}
		
		if( sh->need_external_token ) {
			consumed += do_env_analysis(eng, env, stack, id, sval );
		}else {
			/* sval have been owned by work queue, own the sval 
			again by  sub the count.  */
			NEL_REF_SUB(eng, sh->id, sh->sval);

			do_env_analysis(eng, env, stack, sh->id, sh->sval);

		}

	}

	if(list_empty(&env->work_queue) && list_empty(&env->wait_queue[!env->flag])) {
		/* all the stack are freed, there must have an uncovery error
		happend, return -1 indicate caller to free the LR stuff. */
		sigprocmask(SIG_SETMASK, &oldmask, NULL);
		return -1;
	}

	if(env->finished == 1){
		/* Yeah, we have got an successful match, tell the caller.*/
		sigprocmask(SIG_SETMASK, &oldmask, NULL);
		return 0;
	}

	/* on the halfway */
	nel_debug({analysis_trace(eng, "--->exit nel_env_analysis\n\n");});
	//print_queue(eng, env);
	sigprocmask(SIG_SETMASK, &oldmask, NULL);

	return 1;
}

#if 0
int env_timeout_handler(struct nel_eng *eng, struct nel_env *env, struct Stack *s)
{
	SymbolId id;
	void *sval;
	struct StackHead *sh = &s->head;

	nel_debug({analysis_trace(eng, "--->env_timeout_handler(%p)\n", s);});
	while(list_search(&env->wait_queue, sh)) {
		id = encodeSymbolId(eng,eng->timeoutSymbol->id,nel_C_TERMINAL);
		sval = NULL;  /* type=TOKEN_UNKNOWN; */
			list_del(sh);
		do_env_analysis(eng, env, s, id, sval );
	}

	while(list_search(&env->work_queue, sh)) {
		id = sh->id; sval = sh->sval; 
			list_del(sh);
		do_env_analysis(eng, env, s, id, sval );
	}

	return 1;
}
#endif

#if 0
/* return value:
	1: everything were freed, free the rest of caller stack;
	0: something can't be freed, the caller stack must exist
	   as proxy stack;
       -1: something wrong;
*/
int env_not_handler(struct nel_eng *eng, struct nel_env *env, struct Stack *s, int count)
{
	//struct StackHead *sh = &s->head;
	int i;
	struct Stack *ns;

	nel_debug({analysis_trace(eng, "--->env_not_handler(%p)\n", s);});
	if( s == (struct Stack *)0) {
		//analysis_error(eng, "env_not_handler: NULL stack !\n");
		return -1;
 	}

	/* we call env_not_handler recusively to free the stack started with nc */
	ns = del_from_root_list(eng, env, s, count);		
	env_not_handler(eng, env, ns, count); 

	for (i = 0; i< s->num; i++) {
	
		if( s->stackNodes[i].foot != s ) {
			/* it means 's' is still the proxy of some stack, so we 
			can't free the rest of the stack. */
			if(((struct Stack *)s->stackNodes[i].foot)->num < s->num) {
				return 0;
			}
		} 	

		/* free this stackNode */	
		release(eng, s->stackNodes[i].symbolId, s->stackNodes[i].sval);
		del_from_root_list(eng, env, s, i);	
		del_from_foot_list(eng, env, s, i);
	}

	nel_debug({analysis_trace(eng, "--->stack(%p) be freed\n", s);});
	list_del(&s->head);

	free_mem(&stack_pool, TO_OBJ(s));
	return 1;
}
#endif


void nel_env_cleanup(struct nel_eng *eng, struct nel_env *env)
{
	nel_symbol *sym, *func;
	void *sval;
	struct Stack *s;
	char ret;

	while(!list_empty(&env->work_queue)){
		s=list_entry(env->work_queue.next, struct Stack, head);
		list_del(&s->head);
		nel_debug({analysis_trace(eng, "--->stack(%p) be freed\n", s);});
		free_stack(eng, env, s, s->num -1 );
	}

	while(!list_empty(&env->wait_queue[0])){
		s=list_entry(env->wait_queue[0].next, struct Stack, head);
		list_del(&s->head);
		nel_debug({analysis_trace(eng, "--->stack(%p) be freed\n", s);});
		free_stack(eng, env, s, s->num -1 );

	}


	while(!list_empty(&env->wait_queue[1])){
		s=list_entry(env->wait_queue[1].next, struct Stack, head);
		list_del(&s->head);
		nel_debug({analysis_trace(eng, "--->stack(%p) be freed\n", s);});
		free_stack(eng, env, s, s->num -1 );
	}
	
	return;

}

int nel_env_init(struct nel_eng *eng, struct nel_env *env, SymbolId id, void *sval)
{
	nel_symbol *sym, *func; 
	char ret;
	struct Stack *s;

	if(!env) { 
		analysis_error(eng, "nel_env_init: not an avaliable env!\n");
		return -1;		
	}

	memset(env, 0, sizeof(struct nel_env));
	env->work_queue.next=env->work_queue.prev = &env->work_queue; 
	env->wait_queue[0].next=env->wait_queue[0].prev = &env->wait_queue[0];
	env->wait_queue[1].next=env->wait_queue[1].prev = &env->wait_queue[1];
	env->flag = 0;
	env->finished = 0;
	/*
	env->dollar0_value = sval;
	env->dollar0_id = id;
	*/

	/*create stackNode 0, and insert it into to be shift list */ 
	if (!(s = alloc_stack() )){
		/*NOTE,NOTE,NOTE, need free env */
		analysis_error(eng, "nel_env_init: can't alloc an stack !\n");
		return -1;		
	}

	nel_debug({analysis_trace(eng, "--->stack(%p) be created\n", s);});
	s->num = 0;
	s->recovery_mode = 0;
	s->stackNodes[s->num].stateId = 0; 
	s->stackNodes[s->num].symbolId = id; 
	s->stackNodes[s->num].sval = sval;

	/* root and foot are cycly linked list, initialize them by itself. */
	s->stackNodes[s->num].root= s;
	s->stackNodes[s->num].foot= s;

	if(s->stackNodes[s->num].sval){
		NEL_REF_ADD(eng,id, s->stackNodes[s->num].sval);
	}

	sym = eng->symbolInfo[id];
	nel_debug({analysis_trace(eng, "--->id=%d, sym = %p\n", id, sym);});
	if( sym != NULL && ( func = sym->type->event.init_hander ) != NULL){
		nel_debug({analysis_trace(eng, "--->sym = %p, func =%p \n", sym, sym->type->event.init_hander);});
		if(func->class == nel_C_COMPILED_FUNCTION)
			(*((FREE_FUNC)func->value)) ( sval );
		else
			nel_func_call(eng, &ret, func, sval);
	}

	
	s->num++;
	add_to_work_queue(eng, env, s, 0, NULL /*,TOKEN_UNKNOWN */);
	s->head.need_external_token = 1;

	return 1;
}

