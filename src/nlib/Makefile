# Makefile of nel compiler  
# $Id: Makefile,v 1.24 2006/10/14 15:51:25 wyong Exp $


INSTALL_LIBDIR	= /usr/local/lib
INSTALL_INCDIR	= /usr/local/include/nel

NEL_SRCDIR	= ..
NEL_INCDIR	= ../../include
NLIB_MATCH_DIR	= ./match
NLIB_CHARSET_DIR	= ${NLIB_MATCH_DIR}/charset
NLIB_DFA_DIR 	= ${NLIB_MATCH_DIR}/dfa
NLIB_SWBM_DIR	= ${NLIB_MATCH_DIR}/swbm

STA_LIB		= libnlib.a
DYN_LIB		= libnlib.so
STA_CHARSET_LIB = libcharset.a 
STA_SWBM_LIB	= libswbm.a 
STA_DFA_LIB	= libdfa.a 

LIB_CFLAGS	= -I. -I${NEL_SRCDIR} -I${NEL_INCDIR} \
		-DUSE_DFA -DBACKREF_AS_RULEID \
		-DUSE_SWBM \
		-fPIC \
		-g
		
LIB_SRCS	= nlib.c  shellcode.c hashtab.c match.c stream.c string.c 
$(STA_CHARSET_LIB) : 
		cd $(NLIB_CHARSET_DIR) && $(MAKE) 
$(STA_SWBM_LIB):
		cd $(NLIB_SWBM_DIR) && $(MAKE) 
$(STA_DFA_LIB):
		cd $(NLIB_DFA_DIR) && $(MAKE) 

################################# libnlib.a #################################
STA_LIB_OBJS 	=  $(LIB_SRCS:.c=.ao)
%.ao:		%.c
		$(CC) -c -o $@ $(LIB_CFLAGS) $<

$(STA_LIB)	: $(LIB_SRCS) $(STA_LIB_OBJS) 
		ar -r $@ ${STA_LIB_OBJS}	


################################# libnlib.so ##############################
libnlib		: ${STA_LIB} $(STA_SWBM_LIB) $(STA_DFA_LIB) 
		$(CC) -shared -o $(DYN_LIB) -gstabs+ -Wl,--whole-archive $(STA_LIB) \
		$(NLIB_SWBM_DIR)/$(STA_SWBM_LIB) \
		$(NLIB_DFA_DIR)/$(STA_DFA_LIB) -Wl,--no-whole-archive 
#DYN_NLIB_OBJS	=  $(LIB_SRCS:.c=.so)
#%.so:           %.c
#		$(CC) -c -fpic -o $@ $(LIB_CFLAGS) -I. $<
#libnlib		: $(DYN_NLIB_OBJS)
#		$(CC) $(LIB_CFLAGS) -shared -o $@ $(DYN_NLIB_OBJS)

install-libnlib	: libnlib
		@[ -d $(INSTALL_LIBDIR) ] || mkdir $(INSTALL_LIBDIR)
		cp -f $(DYN_LIB) ${INSTALL_LIBDIR}

uninstall-libnlib:
		rm -f ${INSTALL_LIBDIR}/${DYN_LIB} 



################################## all  ######################################
install		: install-libnlib 
uninstall	: uninstall-libnlib 
all		: libnlib 
clean		:		
		cd $(NLIB_CHARSET_DIR) && make clean
		cd $(NLIB_SWBM_DIR) && make clean
		cd $(NLIB_DFA_DIR) && make clean
		rm -f ${DYN_LIB} ${STA_LIB} 
		rm -f ${STA_LIB_OBJS} 
		rm -f *.o 
		rm -f core

