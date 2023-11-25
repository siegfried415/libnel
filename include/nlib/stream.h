#ifndef __STREAM_H
#define __STREAM_H

#define MAX_STREAM_BUF_SIZE 		4096
struct stream_state {    		
	int caller_id;			/* caller 's id */
#ifdef	USE_PCRE
	int *state_buf;     
	int state_buf_size;  
#elif	USE_DFA
	int state;
#endif
	struct stream_state *next;	
};

struct stream {
	int nel_count;
	unsigned char *buf;
	int buf_len;
	int buf_size ; 
	struct stream_state *state;			
};

int nel_stream_put(struct stream *stream, char *data, int len);
struct stream *nel_stream_alloc(int size);
void nel_stream_free(struct stream *stream);

struct stream_state *alloc_stream_state(void); 
void dealloc_stream_state(struct stream_state *state); 
struct stream_state *insert_stream_state(int caller_id, struct stream_state *state); 
struct stream_state *lookup_stream_state(struct stream *stream, int caller_id); 
int stream_init(struct nel_eng *eng);

#endif
