/*
 * mem_pool.c
 *
 */

#include <pthread.h>
#include <stdlib.h>

#include "mem_pool.h"
#include "mem.h"

typedef char nel_lock_type;
extern nel_lock_type nel_malloc_lock;


static  void __objPool_init(ObjPool_t *objPool)
{
	objPool->head = NULL;
	objPool->num = 0;
}

int __objPool_create(ObjPool_t *objPool, int elmLen, int maxLen)
{
	PoolNode_t *tmp;
	int i;
	
	__objPool_init(objPool);
	objPool->size = elmLen;
	
	for (i = 0; i < maxLen; i ++) {
		char *p;
		nel_malloc(p, elmLen + sizeof(PoolNode_t *), char);
		tmp = (PoolNode_t *)p;
		if(!tmp) {
			break;
		}

		tmp->next = objPool->head;
		objPool->head = tmp;
		objPool->num ++;
		tmp = NULL;
	} 	 	

	pthread_mutex_init(&objPool->lock, NULL);
	return objPool->num;
}

int __objPool_alloc_more(ObjPool_t *objPool, int elmLen, int max)
{
	PoolNode_t *tmp;
	int i;

	pthread_mutex_lock(&objPool->lock);
	for (i = 0; i < max; i ++) {
		char *p;
		nel_malloc(p, elmLen + sizeof(PoolNode_t *), char);
		tmp = (PoolNode_t *)p;
		if(!tmp) {
			break;
		}

		tmp->next = objPool->head;
		objPool->head = tmp;
		objPool->num ++;
		tmp = NULL;
	} 	 	

	pthread_mutex_unlock(&objPool->lock);
	return objPool->num;
}

int __objPool_destroy(ObjPool_t *objPool)
{
	PoolNode_t * tmp;	

	pthread_mutex_lock(&objPool->lock);
	while(objPool->num) {
		tmp = objPool->head;
		objPool->head = objPool->head->next;
		objPool->num --;
		nel_free(tmp);	
		tmp = NULL;	
	}	
	pthread_mutex_unlock(&objPool->lock);

	return 1;
}


void* __obj_alloc_fromPool(ObjPool_t *objPool)
{
	PoolNode_t * tmp = NULL;	

	pthread_mutex_lock(&objPool->lock);

	if(objPool->num > 0) {
		tmp = objPool->head;
		objPool->head = objPool->head->next;
		objPool->num --;
	}

	pthread_mutex_unlock(&objPool->lock);
	return tmp;	
}

int __obj_free_toPool(ObjPool_t *objPool, void * obj)
{
	pthread_mutex_lock(&objPool->lock);

	if(objPool && obj) {	
		((PoolNode_t*)obj)->next = objPool->head;
		objPool->head = (PoolNode_t*)obj;
		objPool->num ++; 	
	
		pthread_mutex_unlock(&objPool->lock);
		return 1;
	}
	else {
		pthread_mutex_unlock(&objPool->lock);
		return 0;
	}
}


int create_mem_pool (ObjPool_t *objPool, int elmLen, int maxLen)
{
	return __objPool_create(objPool, elmLen, maxLen);
}

int destroy_mem_pool (ObjPool_t *objPool)
{
	return __objPool_destroy(objPool);
}


void *alloc_mem (ObjPool_t *objPool)
{
	void *tmp;
	if( !(tmp = __obj_alloc_fromPool(objPool)) ) {
		if( __objPool_alloc_more(objPool, objPool->size, MEM_POOL_REALLOC_LEN) > 0 ) {
			tmp = __obj_alloc_fromPool(objPool);
		}
	}

	return tmp;
}

int free_mem (ObjPool_t *objPool, void *obj)
{
	return __obj_free_toPool(objPool, obj);
}
