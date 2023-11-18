/*
 * mem_pool.h
 */


#ifndef _MEM_POOL_H
#define _MEM_POOL_H 

#include <pthread.h>

#define MEM_POOL_REALLOC_LEN	10


struct PoolNode
{
	struct PoolNode *next;
	char data[0];
};
typedef struct PoolNode PoolNode_t;

struct ObjPool
{
	PoolNode_t *head;
	pthread_mutex_t lock;
	int size;
	int num;
};
typedef struct ObjPool ObjPool_t;

#define TO_DATA(p) ((void *)p+sizeof(PoolNode_t *))
#define TO_OBJ(p) ((void *)p - sizeof(PoolNode_t *))
 

inline void *alloc_mem (ObjPool_t *objPool);
inline int free_mem (ObjPool_t *objPool, void *obj);
inline int create_mem_pool (ObjPool_t *objPool, int elmLen, int maxLen);
inline int destroy_mem_pool (ObjPool_t *objPool);

#endif
