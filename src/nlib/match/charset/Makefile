LIB_CFLAGS	= $(DEFS) -fPIC -g 
STA_LIB		= libcharset.a 

LIB_SRCS	= char_set.c conv_base64.c conv_gzip.c conv_ungzip.c conv_hz.c conv_qp.c conv_utf8.c \
		conv_uuencode.c b2g.c b2u.c g2b.c g2u.c u2b.c u2g.c bits.c \
		deflate.c trees.c util_gzip.c inflate.c util_ungzip.c jis2sjis.c

STA_LIB_OBJS    =  $(LIB_SRCS:.c=.ao)
%.ao:           %.c
		$(CC) -c -o $@ $(LIB_CFLAGS) -I. $<

${STA_LIB}      : ${LIB_SRCS} ${STA_LIB_OBJS} 
		ar -r $@ ${STA_LIB_OBJS}

all:${STA_LIB}
clean:
		rm -f core ${STA_LIB} ${STA_LIB_OBJS}
