# Makefile of nel compiler  
# $Id: Makefile,v 1.24 2006/10/14 15:51:25 wyong Exp $


NLIB_INSTALL_DIR=/usr/local/lib
NEL_INC_INSTALL_DIR=/usr/local/include/nel
NLIB_INC_INSTALL_DIR=${NEL_INC_INSTALL_DIR}/nlib
NLIB_MATCH_INC_INSTALL_DIR=${NLIB_INC_INSTALL_DIR}/match

NEL_SRC_DIR	= ..
NEL_INC_DIR	= ../../include
NLIB_INC_DIR	= ${NEL_INC_DIR}/nlib
NLIB_MATCH_INC_DIR = ${NLIB_INC_DIR}/match

NLIB_MATCH_SRC_DIR= ./match
NLIB_DFA_SRC_DIR= ${NLIB_MATCH_SRC_DIR}/dfa
NLIB_SWBM_SRC_DIR= ${NLIB_MATCH_SRC_DIR}/swbm

NLIB_INC_FILE= ${NEL_INC_DIR}/nlib.h 

NLIB_INC_FILES= ${NLIB_INC_DIR}/hashtab.h ${NLIB_INC_DIR}/string.h \
		${NLIB_INC_DIR}/match.h ${NLIB_INC_DIR}/stream.h \
		${NLIB_INC_DIR}/shellcode.h 

NLIB_MATCH_INC_FILES=${NLIB_MATCH_INC_DIR}/conv_char.h \
		${NLIB_MATCH_INC_DIR}/dfa.h \
		${NLIB_MATCH_INC_DIR}/swbm.h 

STA_LIB		= libnlib.a
DYN_LIB		= libnlib.so
STA_SWBM_LIB	= libswbm.a 
STA_DFA_LIB	= libdfa.a 

LIB_CFLAGS	= -I. -I${NEL_SRC_DIR} -I${NEL_INC_DIR} \
		-DUSE_DFA -DBACKREF_AS_RULEID -DUSE_SWBM -fPIC -g
		
LIB_SRCS	= nlib.c  shellcode.c hashtab.c match.c stream.c string.c 
$(STA_SWBM_LIB):
		cd $(NLIB_SWBM_SRC_DIR) && $(MAKE) 
$(STA_DFA_LIB):
		cd $(NLIB_DFA_SRC_DIR) && $(MAKE) 

################################# libnlib.a #################################
STA_LIB_OBJS 	=  $(LIB_SRCS:.c=.ao)
%.ao:		%.c
		$(CC) -c -o $@ $(LIB_CFLAGS) $<

$(STA_LIB)	: $(LIB_SRCS) $(STA_LIB_OBJS) 
		ar -r $@ ${STA_LIB_OBJS}	


################################# libnlib.so ##############################
libnlib		: ${STA_LIB} $(STA_SWBM_LIB) $(STA_DFA_LIB) 
		$(CC) -shared -o $(DYN_LIB) -gstabs+ -Wl,--whole-archive $(STA_LIB) \
		$(NLIB_SWBM_SRC_DIR)/$(STA_SWBM_LIB) \
		$(NLIB_DFA_SRC_DIR)/$(STA_DFA_LIB) -Wl,--no-whole-archive 

#DYN_NLIB_OBJS	=  $(LIB_SRCS:.c=.so)
#%.so:           %.c
#		$(CC) -c -fpic -o $@ $(LIB_CFLAGS) -I. $<
#libnlib		: $(DYN_NLIB_OBJS)
#		$(CC) $(LIB_CFLAGS) -shared -o $@ $(DYN_NLIB_OBJS)

install-libnlib	: libnlib
		@[ -d $(NLIB_INSTALL_DIR) ] || mkdir $(NLIB_INSTALL_DIR)
		cp -f $(DYN_LIB) ${NLIB_INSTALL_DIR}
		@[ -d ${NEL_INC_INSTALL_DIR} ] || mkdir ${NEL_INC_INSTALL_DIR}
		cp -f ${NLIB_INC_FILE} ${NEL_INC_INSTALL_DIR}
		@[ -d ${NLIB_INC_INSTALL_DIR} ] || mkdir ${NLIB_INC_INSTALL_DIR}
		cp -f ${NLIB_INC_FILES} ${NLIB_INC_INSTALL_DIR}
		@[ -d ${NLIB_MATCH_INC_INSTALL_DIR} ] || mkdir ${NLIB_MATCH_INC_INSTALL_DIR}
		cp -f ${NLIB_MATCH_INC_FILES} ${NLIB_MATCH_INC_INSTALL_DIR}

uninstall-libnlib:
		rm -f ${NLIB_INSTALL_DIR}/${DYN_LIB} 
		rm -f ${NLIB_INC_INSTALL_DIR}/*.*


################################## all  ######################################
install		: install-libnlib 
uninstall	: uninstall-libnlib 
all		: libnlib 
clean		:		
		cd $(NLIB_SWBM_SRC_DIR) && make clean
		cd $(NLIB_DFA_SRC_DIR) && make clean
		rm -f ${DYN_LIB} ${STA_LIB} 
		rm -f ${STA_LIB_OBJS} 
		rm -f *.o 
		rm -f core

