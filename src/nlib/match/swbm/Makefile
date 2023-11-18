STA_LIB		= libswbm.a
LIB_SRCS	= trie.c ptn.c strm.c same.c comp.c merge.c exec.c rule.c
		
STA_LIB_OBJS    =  $(LIB_SRCS:.c=.ao)
%.ao:           %.c
		$(CC) -c -o $@ -fPIC -g -I../../../../include -I.  $<

${STA_LIB}      : ${LIB_SRCS} ${STA_LIB_OBJS} 
		ar -r $@ ${STA_LIB_OBJS}

all:${STA_LIB}
clean:
		rm -f core ${STA_LIB}  ${STA_LIB_OBJS}
