# Makefile of nel compiler  
# $Id: Makefile,v 1.24 2006/10/14 15:51:25 wyong Exp $


PUB_INC_DIR	= ../include
NLIB_DIR 	= ./nlib
TEST_DIR	= ./testsuite
INSTALL_LIBDIR	= /usr/local/lib

DYN_NEL		= libnel.so
DYN_NLIB	= libnlib.so
RUNTEST		= /opt/dejagnu-1.4.4/runtest

CFLAGS	      	= -I. -I${PUB_INC_DIR} -DNEL_NTRP -DNEL_STAB -DSTAB_DEBUG -DNEL_DEBUG -ldl -g 
NEL_SRCS	=analysis.c nel.yacc.c gen.c lex.c stab.c mem_pool.c \
		type.c  engine.c sym.c intrns.c class.c dec.c opt.c io.c \
		prod.c evt.c expr.c stmt.c intp.c errors.c parser.c item.c \
		itemset.c termset.c dprod.c mem.c \
		free_func.c action.c intlist.c 

nel.yacc.c:     nel.y
		bison $< 
		sed -e 's/yyparse (.*)/parser_eval_yy(struct nel_eng *eng)/g'\
		    -e 's/\define YYLEX.*yylex (&yylval.*)/define YYLEX nel_yylex (eng)/g' \
        	< nel.tab.c > $@ 


########################## libnel.so & libnlib.so ##############################
DYN_NEL_OBJS 	=  $(NEL_SRCS:.c=.so) 
%.so:		%.c
		$(CC) -c -fpic -o $@ $(CFLAGS) -I. $<

$(DYN_NEL)	: $(NEL_SRCS) $(DYN_NEL_OBJS)
		$(CC) $(CFLAGS) -shared -o $@ $(DYN_NEL_OBJS) 
$(DYN_NLIB)	: 
		cd $(NLIB_DIR) && $(MAKE) libnlib 
libnel		: $(DYN_NEL)
libnlib		: $(DYN_NLIB)
install-libnel	: ${DYN_NEL}
		@[ -d $(INSTALL_LIBDIR) ] || mkdir $(INSTALL_LIBDIR)
		cp -f ${DYN_NEL} ${INSTALL_LIBDIR}
install-libnlib	: ${DYN_NLIB}
		cd $(NLIB_DIR) && $(MAKE) install-libnlib 

uninstall-libnel:
		rm -f ${INSTALL_LIBDIR}/${DYN_NEL}
uninstall-libnlib:
		cd $(NLIB_DIR) && $(MAKE) uninstall-libnlib 



###############################  TEST ##########################################
stab-test	: install-libnel install-libnlib
		cp -f $(TEST_DIR)/config/stab $(TEST_DIR)/config/default.exp
		${RUNTEST} -tool gcc -srcdir $(TEST_DIR)
nel-test	: install-libnel install-libnlib
		cp -f  $(TEST_DIR)/config/intp $(TEST_DIR)/config/default.exp
		${RUNTEST} -tool nel -srcdir $(TEST_DIR) 
test		: nel-test stab-test
		echo "test over..."	


############################## all  ############################################
install		: install-libnel install-libnlib
uninstall	: uninstall-libnel uninstall-libnlib
all		: libnel libnlib 
clean		:		
		cd $(NLIB_DIR) && $(MAKE) clean 
		rm -f ${DYN_NEL} ${STA_NEL} ${NEL}
		rm -f ${DYN_NEL_OBJS} nel.yacc.*
		rm -f ${DYN_NEL_OBJS}
		rm -f *.o 
		rm -f core

