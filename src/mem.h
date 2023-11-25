
#ifndef MEM_H
#define MEM_H


/*************************************************************/
/* we define macros on top of alloca and malloc that contain */
/* a critical section check for memory overflow.             */
/*************************************************************/
#ifndef alloca
#include <alloca.h>
#endif /* !alloca */

void mem_checkAll();
unsigned char *Realloc(unsigned char *buf, int size, char *file, int line);
unsigned char *Malloc(int size, char *file, int line);
unsigned char *Calloc(int nobj, int objsize, char *file, int line);
void Free(unsigned char *buf, char *file, int line);

#define PATCH_SIZE 4
#define MEM_PATCH 0

#define NOTRACE
//#define DEBUG_MALLOC
//#define DEBUG_CALLOC
//#define DEBUG_REALLOC
//#define DEBUG_FREE

#ifndef NOTRACE
#define MEM_TRACE fprintf
#else
#define MEM_TRACE if(0) fprintf
#endif

#define nel_lock(x)
#define nel_unlock(x)

#ifdef DEBUG_MALLOC
#define nel_malloc(_loc,_nobj,_type) \
	{								\
	   register char *__tmp;					\
	   nel_lock (&nel_malloc_lock);					\
	   __tmp = (char *) Malloc ((_nobj) * sizeof (_type), __FILE__, __LINE__); 		\
	   nel_unlock (&nel_malloc_lock);				\
	   (_loc) = (_type *) __tmp;					\
	}
#else
#define nel_malloc(_loc,_nobj,_type) \
	{								\
	   register char *__tmp;					\
	   nel_lock (&nel_malloc_lock);					\
	   __tmp = (char *) malloc ((_nobj) * sizeof (_type)); 		\
	   nel_unlock (&nel_malloc_lock);				\
	   (_loc) = (_type *) __tmp;					\
	}
#endif

#ifdef DEBUG_CALLOC
#define nel_calloc(_loc,_nobj,_type)				\
	{												\
	   register char *__tmp;						\
	   nel_lock (&nel_calloc_lock);					\
	   __tmp = (char *)Calloc((_nobj), sizeof(_type), __FILE__, __LINE__); 		\
	   nel_unlock (&nel_calloc_lock);				\
	   (_loc) = (_type *) __tmp;					\
	}
#else
#define nel_calloc(_loc,_nobj,_type) 				\
	{												\
	   register char *__tmp;						\
	   nel_lock (&nel_calloc_lock);					\
	   __tmp = (char *)calloc((_nobj), sizeof(_type)); 		\
	   nel_unlock (&nel_calloc_lock);				\
	   (_loc) = (_type *) __tmp;					\
	}
#endif

#ifdef DEBUG_REALLOC
#define nel_realloc(_loc,_oldptr,_size)				\
	{												\
	   register char *__tmp;						\
	   nel_lock (&nel_realloc_lock);				\
	   __tmp = (char *)Realloc((_oldptr), (_size), __FILE__, __LINE__); \
	   nel_unlock (&nel_realloc_lock);				\
	   (_loc) = __tmp;								\
	}
#else
#define nel_realloc(_loc,_oldptr,_size)				\
	{												\
	   register char *__tmp;						\
	   nel_lock (&nel_realloc_lock);				\
	   __tmp = (char *)realloc((_oldptr), (_size));	\
	   nel_unlock (&nel_realloc_lock);				\
	   (_loc) = __tmp;								\
	}
#endif

#ifdef DEBUG_MALLOC
#define checksum() mem_checkAll()
#else
#define checksum()
#endif

#ifdef DEBUG_FREE
#define nel_free(_loc)						\
{								\
	if(_loc)						\
		Free ((void *) _loc, __FILE__, __LINE__);	\
}
#else
#define nel_free(_loc)						\
{								\
	if(_loc)						\
		free ((void *) _loc);				\
}
#endif


/********************************************************************/
/* on CRAY_YMP, alloca () does not exists, so we must use malloc () */
/* and then free () the memory before nel returns.  this means one   */
/* should not use setjmp () to exit the interpreter, or else the    */
/* space will not be reclaimed.                                     */
/********************************************************************/
#define nel_alloca(_loc,_nobj,_type)	nel_malloc ((_loc), (_nobj), _type)
#define nel_dealloca(_loc) 	nel_free(_loc)


/**********************************************************************/
/* copy _bytes bytes starting at *_dest to area starting at *_source  */
/* don't worry about doing word-length transfers - this macro is used */
/* mostly on character strings, data that does not necessarily have   */
/* the same alignment, or the amount of data transferred is so small  */
/* that the overhead of the address arithmetic would be prohibitive.  */
/**********************************************************************/
#define nel_copy(_bytes,_dest,_source); \
	{								\
	   register int __bytes = (_bytes);				\
	   register unsigned_char *__dest = (unsigned_char *) (_dest);	\
	   register unsigned_char *__source = (unsigned_char *) (_source); \
	   register int __i;						\
	   if (__dest < __source) {					\
	      for (__i = 0; (__i < __bytes); __i++) {			\
	         *(__dest++) = *(__source++);				\
	      }								\
	   }								\
	   else if (__dest > __source) {				\
	      __dest += __bytes - 1;					\
	      __source += __bytes - 1;					\
	      for (__i = 0; (__i < __bytes); __i++) {			\
	         *(__dest--) = *(__source--);				\
	      }								\
	   }								\
	}


/*****************************************************/
/* zero out _bytes bytes starting at *_dest          */
/* try to do as many word-length writes as possible, */
/* for efficiency (especially important on the       */
/* CRAY_YMP, as it is not byte-addressed)            */
/*****************************************************/
#define nel_zero(_bytes,_dest); \
	{								\
	   register unsigned_char *__dest = (unsigned_char *) (_dest);	\
	   register int __bytes = (_bytes);				\
	   register int __offset = ((unsigned_long) _dest) % (sizeof (int)); \
	   register int *__ip;						\
	   if (__offset > 0) {						\
	      __offset = sizeof (int) - __offset;			\
	      for (; (__offset-- > 0); __bytes--) {			\
	         *(__dest++) = '\0';					\
	      }								\
	   }								\
	   __ip = (int *) __dest;					\
	   for (; ((__offset = (__bytes - sizeof (int))) > 0); __bytes = __offset) { \
	      *__ip++ = 0;						\
	   }								\
	   __dest = (unsigned_char *) __ip;				\
	   for (; (__bytes-- > 0); ) {					\
	      *(__dest++) = '\0';					\
	   }								\
	}


#endif
