
#wyong, 20230823 
#LIB_CFLAGS 	= -DDFA_LAZY -DBACKREF_AS_RULEID -fPIC -g 
LIB_CFLAGS 	= -DBACKREF_AS_RULEID -fPIC -g 

STA_LIB		= libdfa.a
LIB_SRCS        = dfa.c 

STA_LIB_OBJS    =  $(LIB_SRCS:.c=.ao)
%.ao:           %.c
		$(CC) -c -o $@ $(LIB_CFLAGS) -I../../../../include $<

${STA_LIB}      : ${LIB_SRCS} ${STA_LIB_OBJS}
		ar -r $@ ${STA_LIB_OBJS}

all:${STA_LIB}
clean:
		rm -f core ${STA_LIB}  ${STA_LIB_OBJS}

test:		test.c dfa.c 
		$(CC) -g -o test -DUSE_DFA -DBACKREF_AS_RULEID -I../../../../include test.c dfa.c 
