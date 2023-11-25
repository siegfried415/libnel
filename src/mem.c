#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "mem.h"

struct mem_info {
	unsigned char *pAlloced;
	unsigned char *pPointer;
	size_t size;
	char *file;
	int line;
	int flag;
};
static struct mem_info *mInfo = NULL;
static int recNum = 0;
static pthread_mutex_t mlock;
static int MaxEnt = 100000;
static char mLab[PATCH_SIZE * 2];
static int inited = 0;

void mem_checkAll()
{
	int i;

	if (!inited) {
		inited = 1;
		memset(mLab, MEM_PATCH, sizeof(mLab));
	}


	for (i = 0; i < recNum; i++) {
		if (mInfo[i].pAlloced != NULL) {
			assert(mInfo[i].pPointer == (mInfo[i].pAlloced - PATCH_SIZE));
			if (memcmp(mInfo[i].pPointer, mLab, PATCH_SIZE) != 0) {
				fprintf(stderr,
						"memCheck: Pointer overflow, malloc at <%s:%d>\n",
						mInfo[i].file, mInfo[i].line);
				abort();
			}
			if (memcmp(mInfo[i].pPointer + PATCH_SIZE + mInfo[i].size,
					   mLab, PATCH_SIZE) != 0) {
				fprintf(stderr,
						"memCheck: Pointer underflow, malloc at <%s:%d>\n",
						mInfo[i].file, mInfo[i].line);
				abort();
			}
		}
	}
}

static unsigned int total_alloc = 0;
static unsigned int total_free = 0;

unsigned char *Realloc(unsigned char *buf, int size,  char *file, int line)
{
	unsigned char *s = NULL;
	if(buf){
		s = Malloc(size, file, line);
		memcpy(s, buf, size);	
		Free(buf, file, line);	
	}
	else {
		s = Malloc(size, file, line);
	}

	return s;
}

unsigned char *Calloc(int nobj, int objsize, char *file, int line)
{
	unsigned char *s;
	s = Malloc(nobj*objsize, file, line);
	memset(s, 0, nobj*objsize);
	return s;
}

unsigned char *Malloc(int size, char *file, int line)
{
	unsigned char *buf;

	if (!inited) {
		inited = 1;
		memset(mLab, MEM_PATCH, sizeof(mLab));
	}

	MEM_TRACE(stderr, "BEGIN MALLOC, size=%d\n", size);
	total_alloc += size;
	MEM_TRACE(stderr, "TOTAL_ALLOC: %u, TOTAL_FREE: %u, MEM_USED: %u\n",
		total_alloc, total_free, total_alloc - total_free);
	if (mInfo == NULL) {
		mInfo =
			(struct mem_info *) malloc(MaxEnt * sizeof(struct mem_info));
		if (mInfo == NULL) {
			fprintf(stderr, "Can not alloc memory for pAlloced.\n");
		} else {
			memset(mInfo, 0, sizeof(struct mem_info) * MaxEnt);
			recNum = MaxEnt;
		}
		pthread_mutex_init(&mlock, NULL);
	}
	pthread_mutex_lock(&mlock);
	mem_checkAll();
	buf = (char *) malloc(size + PATCH_SIZE * 2);
	MEM_TRACE(stderr, "MALLOC : [%s] [%d] [%d] [%lX]\n", file, line, size,
		  (unsigned long) buf);
	{
		int i;
		for (i = 0; i < recNum; i++) {
			if (mInfo[i].pAlloced == NULL) {
				mInfo[i].pPointer = buf;
				/*
				int j=0;
				for(j=0; j<i; j++)
				{
					if(mInfo[j].pPointer >= mInfo[i].pPointer && mInfo[j].pPointer <= mInfo[i].pPointer ||
						(char*)(mInfo[j].pPointer)+PATCH_SIZE*2 >=mInfo[i].pPointer &&(char*)(mInfo[j].pPointer)+PATCH_SIZE*2 <=mInfo[i].pPointer)
					{
						printf("malloc overlapping, old index=%d, new index=%d\n", j, i);
						exit(-1);
					}
				}
				*/

				mInfo[i].pAlloced = buf + PATCH_SIZE;
				mInfo[i].size = size;
				mInfo[i].file = file;
				mInfo[i].line = line;
				mInfo[i].flag = 1;
				memset(buf, 0, size + PATCH_SIZE * 2);
				buf += PATCH_SIZE;
				break;
			}
		}
		if (i == recNum) {
			fprintf(stderr, "memAlloc handle exceed the max number %d\n",
					recNum);
		}
	}

	pthread_mutex_unlock(&mlock);
	MEM_TRACE(stderr, "MALLOC OK.\n\n");
	return buf;
}


void Free(unsigned char *buf, char *file, int line)
{
	if (!inited) {
		inited = 1;
		memset(mLab, MEM_PATCH, sizeof(mLab));
	}

	pthread_mutex_lock(&mlock);
	MEM_TRACE(stderr, "FREE: [%s] [%d] [%lX]\n", file, line,
		  (unsigned long) buf);
	{
		int i;
		for (i = 0; i < recNum; i++) {
			if (mInfo[i].pAlloced == buf) {
				if(!mInfo[i].flag)
				{
					fprintf(stderr, "re-free!\n");
					assert(0);
				}
				assert(mInfo[i].pPointer ==
					   (mInfo[i].pAlloced - PATCH_SIZE));
				mInfo[i].pAlloced = NULL;
				total_free += mInfo[i].size;
				if (memcmp(mInfo[i].pPointer, mLab, PATCH_SIZE) != 0) {
					fprintf(stderr,
							"memFree: Pointer overflow at <%s:%d>,malloc <%s:%d>\n",
							file, line, mInfo[i].file, mInfo[i].line);
				}
				if (memcmp
					(mInfo[i].pPointer + PATCH_SIZE + mInfo[i].size, mLab,
					 PATCH_SIZE) != 0) {
					fprintf(stderr,
							"memFree: Pointer underflow at <%s:%d>,malloc <%s:%d>\n",
							file, line, mInfo[i].file, mInfo[i].line);
				}
				free(mInfo[i].pPointer); 
				mInfo[i].flag = 0;
				break;
			}
		}
		MEM_TRACE(stderr, "TOTAL_ALLOC: %u, TOTAL_FREE: %u, MEM_USED: %u\n",
			total_alloc, total_free, total_alloc - total_free);
		if (i == recNum) {
			fprintf(stderr, "memFree handle not found <%s,%d>\n", file,
					line);
		}
	}

	MEM_TRACE(stderr, "FREE [%lX] OK\n\n", (unsigned long) buf);
	pthread_mutex_unlock(&mlock);
}

void PrintNotFree()
{
	int i;
	int flag=0;
	printf("\n=====================NOT FREE==========================\n");
	for(i=0; i<MaxEnt; i++)
	{
		if(mInfo[i].flag)
		{
			printf("%s, %d(line), %lu(size), %d(i)\n", mInfo[i].file, mInfo[i].line, mInfo[i].size, i);
			flag=1;
		}
	}
	if(!flag)
		printf("haha, all memmory are dealloc!");
	printf("\n=====================NOT FREE==========================\n");
}
