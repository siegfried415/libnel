

/*****************************************************************************/
/* This file, "lex.c", contains the lex rules and actions for the            */
/* lexical analyzer used by the application executive interpreter.           */
/*****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <io.h>
#include <elf.h>
#include <setjmp.h>

#include "type.h"		/* type descriptor definitions		*/
#include "intp.h"		/* decs for ops (needed by nel_stack.h)	*/
#include "lex.h"		/* decs for this file			*/
#include "err.h"		/* decs for error handlers		*/
#include "engine.h"
#include "parser.h"
#include "errors.h"
#include "mem.h"


static char nel_nextc(struct nel_eng *eng);
/********************************************************/
/* save the last input token, in case the parser clears */
/* it before we can use it for error recovery.          */
/********************************************************/
#define ret(_retval) \
        {                                                               \
           eng->parser->last_char = (_retval);                           \
           return (_retval);                                            \
        }


struct keyinfo {char *name; int id;};
struct keyinfo keywordtable[] =
{
	{"asm", 	nel_T_ASM},
	{"atom", 	nel_T_ATOM},
	{"auto", 	nel_T_AUTO},
	{"break", 	nel_T_BREAK},
	{"case", 	nel_T_CASE},
	{"char", 	nel_T_CHAR},
	{"complex", 	nel_T_COMPLEX},
	{"const", 	nel_T_CONST},
	{"continue", 	nel_T_CONTINUE},
	{"default", 	nel_T_DEFAULT},
	{"do", 	nel_T_DO},
	{"double", 	nel_T_DOUBLE},
	{"else", 	nel_T_ELSE},
	{"enum", 	nel_T_ENUM},
	{"event", 	nel_T_EVENT},
	{"extern", 	nel_T_EXTERN},
	{"float", 	nel_T_FLOAT},
	{"for", 	nel_T_FOR},
	{"fortran", 	nel_T_FORTRAN},
	/*{"goto", 	nel_T_GOTO},*/
	{"if", 	nel_T_IF},
	{"int", 	nel_T_INT},
	{"init", 	nel_T_INIT},
	{"fini", 	nel_T_FINI},
	{"main", 	nel_T_MAIN},
	{"exit", 	nel_T_EXIT},
	{"long", 	nel_T_LONG},
	{"nodelay", 	nel_T_NODELAY},
	{"register", 	nel_T_REGISTER},
	{"return", 	nel_T_RETURN},
	{"short", 	nel_T_SHORT},
	{"signed", 	nel_T_SIGNED},
	{"sizeof", 	nel_T_SIZEOF},
	{"static", 	nel_T_STATIC},
	{"struct", 	nel_T_STRUCT},
	{"switch", 	nel_T_SWITCH},
	//{"timeout", 	nel_T_TIMEOUT},
	{"typedec", 	nel_T_TYPEDEC},
	{"typedef", 	nel_T_TYPEDEF},
	{"typeof", 	nel_T_TYPEOF},
	{"union", 	nel_T_UNION},
	{"unsigned", 	nel_T_UNSIGNED},
	{"void", 	nel_T_VOID},
	{"volatile", 	nel_T_VOLATILE},
	{"while", 	nel_T_WHILE}
};	

struct keyinfo pkeywordtable[] =
{
	{"define", 	nel_T_P_DEFINE},
	{"elif", 	nel_T_P_ELIF},
	{"else", 	nel_T_P_ELSE},
	{"endif", 	nel_T_P_ENDIF},
	{"error", 	nel_T_P_ERROR},
	{"if", 	nel_T_P_IF},
	{"ifdef", 	nel_T_P_IFDEF},
	{"ifndef", 	nel_T_P_IFNDEF},
	{"include", 	nel_T_P_INCLUDE},
	{"line", 	nel_T_P_LINE},
	{"pragma", 	nel_T_P_PRAGMA},
	{"pounds", 	nel_T_P_POUNDS},
	{"undef", 	nel_T_P_UNDEF}
};	


#define keywordtablesize (sizeof(keywordtable)/sizeof(struct keyinfo))
#define pkeywordtablesize (sizeof(pkeywordtable)/sizeof(struct keyinfo))

#if 0
void yyerror(struct nel_eng *eng, char *fmt, ...)
{

	va_list args;
	eng->parser->error_ct++;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	//exit(2);
}

#else

void yyerror(struct nel_eng *eng, char *message, ...)
{
	va_list args;

	/* output error message first */
	if(eng != NULL && eng->parser_verbose_level >= NEL_ERROR_LEVEL ){
		va_start (args, message); 

		if ((eng->parser->type!= nel_R_NULL) && (eng->parser->filename!= NULL)){
			fprintf (stderr, "\"%s\", line %d: ", eng->parser->filename, eng->parser->line);
		}
                nel_do_print (stderr, message, args);
		//fprintf (stderr, "\n");
		va_end (args);
	}

	eng->parser->error_ct++;

	/*******************************************************/
	/* now longjmp back to the parser, which will continue */
	/* with the next statement (or the next symbol table   */
	/* string), if the stmt_err_jmp is set.                */
	/*******************************************************/
	/*
	if ((eng != NULL) 
		&& (eng->parser->type != nel_R_NULL) 
		&& (eng->parser->stmt_err_jmp != NULL)) {
		nel_longjmp (eng, *(eng->parser->stmt_err_jmp), 1);
	}
	else {
		parser_fatal_error (eng, "(nel_stmt_error #1): stmt error jump not set",NULL);
	}
	*/
	
	return ;

}

#endif

void pushback(struct nel_eng *eng)
{
	if( eng->parser->lexptr 
	&& eng->parser->lexptr > eng->parser->lexptr_begin ) {
		eng->parser->lexptr--;
		if( *eng->parser->lexptr == '\n' ) {
			eng->parser->line--;
		}
	}
}

static void tokexpand(struct nel_eng *eng)
{
	static int toksize = 60;
	int tokoffset;

	tokoffset = eng->parser->tok - eng->parser->text;
	//toksize *= 2;
	{
		toksize = 2 * tokoffset;
		if(toksize < 120 )
			toksize = 120;
	}

	if (eng->parser->text) {
		nel_realloc(eng->parser->text, eng->parser->text, toksize);
	}
	else{
		nel_malloc(eng->parser->text, toksize, char);
	}

	eng->parser->tokend = eng->parser->text + toksize;
	eng->parser->tok =  eng->parser->text + tokoffset;

	return ;
}

static int squoted_sync(struct nel_eng *eng) 
{
	int retval = 0;
	char c;

	for ( c= nel_nextc(eng); c != '\'' && c != ';' && c != '}' && c != '\0' ;
		c = nel_nextc(eng)) {
		retval ++;	
	}

	if (c != 0 && c != '\'') pushback(eng);
	return retval;
}


static void tokadd(struct nel_eng *eng, char c)
{
	if (eng->parser->tok == NULL || eng->parser->tok == eng->parser->tokend)
		tokexpand(eng);	
	*eng->parser->tok++ = c;
}

static int get_src_buf(struct nel_eng *eng)
{
	char *retval;
	int ret = 1;

	if (!eng->parser->infile){
		eng->parser->infile = fopen(eng->parser->filename, /*O_RDONLY*/ "r"); 
		if (eng->parser->infile == /*-1*/ NULL ){
			yyerror(eng, "can't open source file %s\n", eng->parser->filename); 
		}
		eng->parser->line = 1;
		eng->parser->lexptr_begin = eng->parser->buf ;
	} 

	if ((retval=fgets( eng->parser->buf, nel_YY_READ_BUF_SIZE, eng->parser->infile)) == NULL ){
		//fclose(eng->parser->infile);
		ret = 0;	
	}

	eng->parser->lexptr = eng->parser->buf;
	eng->parser->lexend = eng->parser->lexptr + strlen(eng->parser->lexptr);

	return ret;
}

static char nel_nextc(struct nel_eng *eng) 
{
        int c;

        if (eng->parser->lexptr 
		&& eng->parser->lexptr < eng->parser->lexend){
		//if(*eng->parser->lexptr == '\0')
		//	eng->parser->lexptr++;
                c = *eng->parser->lexptr++;
	}
        else if (get_src_buf(eng)) {
                c = *eng->parser->lexptr++;
	}
        else {
                c = *(eng->parser->lexptr++) = '\0';
                //eng->parser->lexptr++;
                //c = '\0';
	}

	if(c == '\n'){
		eng->parser->line++;
	}

        return c;
}

int nel_yylex(struct nel_eng *eng)
{
	register int c, c1;
	int esc_seen;		/* for literal strings */
	//char *tokkey;
	//int shouldback = 0;
	//int paren_cnt =0;	/*control BRACED_CODE*/
	int i;	
	int overflow = 0;
	register nel_symbol *symbol;
	long long n;
	int quoted_cnt = 0;
	int hex = 0;

	if (!nel_nextc(eng)) {
		//eng->parser->lasttok = 0;
		ret(0);
	}
	pushback(eng);

retry:
	while ((c = nel_nextc(eng)) == ' ' || c == '\t')
		continue;

	/*NOTE,NOTE,NOTE, review the following two lines */
	eng->parser->tok = eng->parser->text;
	//yylval.nodetypeval = Node_illegal;

	switch (c) {
	case 0:
		//eng->parser->lasttok = 0;
		ret(0);

	case '\r':
	case '\n':
		goto retry;


	case '#':		
		for(c  = nel_nextc(eng) ; isalpha(c); c = nel_nextc(eng)) {
			tokadd(eng, c);
		}
		tokadd(eng, '\0');
		
		if (c != 0)
			pushback(eng);

		for(i =0; i< pkeywordtablesize; i++){
			if(!strcmp(pkeywordtable[i].name, eng->parser->text)){
				eng->parser->want_assign = 1;
				ret(pkeywordtable[i].id);
			}
		}		

		yyerror(eng, "unknown pre-process keyword!\n");
		break;
		
	case '\\':
		if (nel_nextc(eng) == '\n') {
			goto retry;
		} else
			yyerror(eng, "backslash not last character on line\n");
		break;


	case '\'':
		c = nel_nextc(eng);
		switch(c){
		case'\\':
			c = nel_nextc(eng);
			switch(c) {
			case 'a':
				eng->parser->char_lval = '\a';
				quoted_cnt = 1;
				break;
			case 'b':
				eng->parser->char_lval = '\b';
				quoted_cnt = 1;
				break;
			case 'f':
				eng->parser->char_lval = '\f';
				quoted_cnt = 1;
				break;
			case 'n':
				eng->parser->char_lval = '\n';
				quoted_cnt = 1;
				break;
			case 'r':
				eng->parser->char_lval = '\r';
				quoted_cnt = 1;
				break;
			case 't':
				eng->parser->char_lval = '\t';
				quoted_cnt = 1;
				break;
			case 'v':
				eng->parser->char_lval = '\v';
				quoted_cnt = 1;
				break;
			case '\\':
				eng->parser->char_lval = '\\';
				quoted_cnt = 1;
				break;
			case '\?':
				eng->parser->char_lval = '\?';
				quoted_cnt = 1;
				break;
			case '\'':
				eng->parser->char_lval = '\'';
				quoted_cnt = 1;
				break;
			case '\"':
				eng->parser->char_lval = '\"';
				quoted_cnt = 1;
				break;

			case 'X':
			case 'x':
				hex = 1;
				c = nel_nextc(eng);
				/* pass through */
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '0':
				{
					int val = 0;
					int i = 0;
					int power = (hex) ? 16 : 8; 

					for( ; c != '\'' ; c = nel_nextc(eng)){
						int d = 0;
						switch(c) {
							case '0': 
							case '1': 
							case '2': 
							case '3': 
							case '4':
							case '5': 
							case '6':
							case '7': 
								d = c - '0'; 
								break;

							case '8': 
							case '9': 
								if(!hex) 
									goto out_of_squoted;
								d = c - '0';
								break;


							case 'a':
							case 'b':
							case 'c':
							case 'd':
							case 'e':
							case 'f':
								if(!hex) 
									goto out_of_squoted;
								d = c -'a'+ 10;
								break;


							case 'A': 
							case 'B': 
							case 'C': 
							case 'D': 
							case 'E': 
							case 'F': 
								if(!hex) 
									goto out_of_squoted;
								d = c -'A'+ 10;
								break;
							
							default:
								goto out_of_squoted;
						}

						i++;
						if ( ((hex)? i > 2 : i > 3) 
							|| val > ( UCHAR_MAX -d ) / power ) {

out_of_squoted:
							parser_stmt_error(eng, "bad %s in single quoted", hex ? "hex" : "octal");
						}

						val *= power ;
						val += d;

					}

squoted_finish:
					if( i == 0 ){
						goto out_of_squoted;
					}

					if (c != 0)
						pushback(eng);
			
					eng->parser->char_lval = val;
					quoted_cnt = 1;
				}
				break;
		
			default:
				eng->parser->char_lval = c;	
				quoted_cnt = 1;
				break;
			}
			break;

		default: 
			if( c != '\'' ) {
				eng->parser->char_lval = c;
				quoted_cnt = 1;
			}
			break;
		}
		
		if ( quoted_cnt == 0 ) {
			parser_stmt_error(eng, "no byte in single quoted");
		}


do_sync:
		quoted_cnt += squoted_sync(eng);
		if ( quoted_cnt > 1 ) {
			parser_stmt_error(eng, "multibyte in single quoted");
		}
		
		ret(nel_T_C_CHAR);


	case '$':
		tokadd(eng, c);
		c = nel_nextc(eng);
		if (isdigit(c)) {
			for (; isdigit(c); c = nel_nextc(eng)){
				tokadd(eng, c);
			}
			tokadd(eng, '\0');
			pushback(eng);
			ret(nel_T_DOLLAR_IDENT);

		} 
		/* 
		else if (c == '$') {
			ret(nel_T_DOLLAR_IDENT);
		}
		*/
	
		parser_stmt_error(eng, "unrecognized $\n");
		break;

	case ')':	ret(nel_T_RPAREN);
	case ']':	ret(nel_T_RBRACK);
	case '}':	ret(nel_T_RBRACE);
	case '(':	ret(nel_T_LPAREN);	
	case '[':	ret(nel_T_LBRACK);
	case '{':	ret(nel_T_LBRACE);
	case ';':	ret(nel_T_SEMI);
	case ':':	ret(nel_T_COLON);
	case '?':	ret(nel_T_QUESTION);
	case ',':	ret(nel_T_COMMA);

	case '*':
		if ((c = nel_nextc(eng)) == '=') {
			//yylval.nodetypeval = Node_assign_times;
			ret(nel_T_STAREQ);
		} 
		//else if (c == '*') {
		//	/* make ** and **= aliases for ^ and ^= */
		//	if (nextc() == '=') {
		//		yylval.nodetypeval = Node_assign_exp;
		//		return lasttok = ASSIGNOP;
		//	} else {
		//		pushback();
		//		return lasttok = '^';
		//	}
		//}
		pushback(eng);
		ret(nel_T_STAR); 

	case '/':
		if( (c = nel_nextc(eng)) == '/' ){
			while(nel_nextc(eng) != '\n');	
			goto retry;	
		}
		else if( c == '*' ) {
findstar:	
			do{ ; } while (nel_nextc(eng) != '*' );/* skip comment body */
			if( nel_nextc(eng) == '/' )
				goto retry;
			else {
				pushback(eng);
				goto findstar;	
			}
		}	
			
		else if (eng->parser->want_regexp == 1) {
			int in_brack = 0;	/* count brackets, [[:alnum:]] allowed */
			/*
			 * Counting brackets is non-trivial. [[] is ok,
			 * and so is [\]], with a point being that /[/]/ as a regexp
			 * constant has to work.
			 *
			 * Do not count [ or ] if either one is preceded by a \.
			 * A `[' should be counted if
			 *  a) it is the first one so far (in_brack == 0)
			 *  b) it is the `[' in `[:'
			 * A ']' should be counted if not preceded by a \, since
			 * it is either closing `:]' or just a plain list.
			 * According to POSIX, []] is how you put a ] into a set.
			 * Try to handle that too.
			 *
			 * The code for \ handles \[ and \].
			 */

			eng->parser->want_regexp = 2;
			eng->parser->tok = eng->parser->text;
			do {
				switch (c) {
				case '[':
					/* one day check for `.' and `=' too */
					if (nel_nextc(eng) == ':' || in_brack == 0)
						in_brack++;
					pushback(eng);
					break;
				case ']':
					if (eng->parser->text[0] == '['
					    && (eng->parser->tok == eng->parser->text + 1
						|| (eng->parser->tok == eng->parser->text + 2
							&& eng->parser->text[1] == '^')))
						/* do nothing */;
					else
						in_brack--;
					break;
				case '\\':
					if ((c = nel_nextc(eng)) == EOF) {
						yyerror( eng, "unterminated regexp ends with `\\' at end of file");
						ret (nel_T_REGEXP); /* kludge */
					} else if (c == '\n') {
						eng->parser->line++;
						continue;
					} else {
						tokadd(eng, '\\');
						tokadd(eng, c);
						continue;
					}
					break;
				
				case '/':	/* end of the regexp */
					if (in_brack > 0)
						break;

					pushback(eng);
					tokadd(eng, '\0');
					//yylval.sval = tokstart;
					ret(nel_T_REGEXP);

				case '\n':
					pushback(eng);
					yyerror(eng, "unterminated regexp");
					ret (nel_T_REGEXP);	/* kludge */

				case EOF:
					yyerror(eng, "unterminated regexp at end of file");
					ret (nel_T_REGEXP);	/* kludge */
				}
				tokadd(eng, c);
			} while( c = nel_nextc(eng));	

			if(c = '\0') {
				yyerror(eng, "unterminated regexp at end of file");
				ret (nel_T_REGEXP);	/* kludge */
			}

		}else if(eng->parser->want_regexp == 2 ) {
			int len =0;
			for (;;){	
				switch (c) {
				case 'i': tokadd(eng, c); len ++; break;
				case 's': tokadd(eng, c); len ++; break;	
				case 'm': tokadd(eng, c); len ++; break;
				case '/': break;
				default:
					pushback(eng);
					goto return_REGEXT;
				}

				c = nel_nextc(eng);
			}		
return_REGEXT:
			if(len > 0 ) {
				eng->parser->want_regexp = 0;
				tokadd(eng, '\0' ); ret(nel_T_REGEXT);
			}
			
			eng->parser->want_regexp = 0;
			goto retry;

		}

		if (eng->parser->want_assign) {
			if ( c  == '=' ) {
				//yylval.nodetypeval = Node_assign_quotient;
				ret(nel_T_SLASHEQ);
			}
		}

		pushback(eng);
		ret(nel_T_SLASH);

	case '%':
		c = nel_nextc(eng); 
		//shouldback++;
		if (c == '=') {
			//yylval.nodetypeval = Node_assign_mod;
			ret(nel_T_PERCENTEQ);
		}
		//else if (c=='{'){
		//	int c1, c2;
		//	if ((c1 = nextc()) && (c2 = nextc())){
		//		while(  c2!=0    
		//			&& (c1 != '%' || c2 !='}')) {
		//			c = c1;
		//			tokadd(c); 
		//			c1 = c2;
		//			c2 = nextc();
		//		}	
		//	}
		//
		//	if(c1 == 0) {
		//		fatal(" %{ has not %} with it\n");
		//		return lasttok = 0;
		//	}
		//
		//	// yeah, we got an LOGUE
		//	tokadd('\0');
		//	tokkey = (char *)malloc(tok - tokstart);
		//	memcpy(tokkey, tokstart, tok - tokstart);
		//
		//	yylval.sval = tokkey;
		//	return lasttok = LOGUE;
		//	
		//}

		//else{
		//	int i;
		//	tok = tokstart;
		//	while ( isalnum(c) || c == '_' || c == '%') {
		//		tokadd(c);
		//		c = nextc(); 
		//		shouldback++;
		//	}
		//
		//	tokadd('\0');
		//	tokkey = (char *)malloc(tok - tokstart);
		//	memcpy(tokkey, tokstart, tok - tokstart);
		//	if (c != 0){
		//		pushback();
		//		shouldback--;
		//	}
		//
		//	// if it is a keyword start with % 
		//	for(i =0; i< dtablesize; i++){
		//		if(!strcmp(__dtable[i].name, tokkey)){
		//			shouldback = 0;
		//			return lasttok = __dtable[i].id;
		//		}
		//	}		
		//}

		//while(shouldback--)
		//	pushback();
		
		pushback(eng);
		ret(nel_T_PERCENT);

	case '^':
		if (nel_nextc(eng) == '=') {
			//yylval.nodetypeval = Node_assign_exp;
			ret(nel_T_UPARROWEQ);
		}
		pushback(eng);
		ret(nel_T_UPARROW);

	case '+':
		if ((c = nel_nextc(eng)) == '=') {
			//yylval.nodetypeval = Node_assign_plus;
			ret(nel_T_PLUSEQ);
		}
		if (c == '+')
			ret(nel_T_DPLUS);
		pushback(eng);
		ret(nel_T_PLUS); 

	case '!':
		if ((c = nel_nextc(eng)) == '=') {
			ret (nel_T_EXCLAMEQ);
		}
		else if (c == '~') {
			ret (nel_T_EXCLAM_TILDE);
		}
		pushback(eng);
		ret(nel_T_EXCLAM); 

	case '<':
		c = nel_nextc(eng);
		//shouldback++;
		if (c == '=') {
			//yylval.nodetypeval = Node_leq;
			ret(nel_T_LARROWEQ);
		}
		else if (c == '<') {
			c = nel_nextc(eng);
			if(c == '=') {
				ret (nel_T_DLARROWEQ);
			}
			pushback(eng);
			ret(nel_T_DLARROW);
		}
		//else{
		//	tok = tokstart;
		//	while ( isalnum(c) || c == '_') {
		//		tokadd(c);
		//		c = nextc(); 
		//		shouldback++;
		//	}
		//	
#ifndef NON_COMPILE
		//	if(c == '>'){
		//		shouldback = 0;
		//		tokadd('\0');
		//		tokkey = (char *)malloc(tok - tokstart);
		//		memcpy(tokkey, tokstart, tok - tokstart);
		//		yylval.sval = tokkey;				
		//		return lasttok = TAG;
		//	}
#endif
		//}

		//yylval.nodetypeval = Node_less;
		//while(shouldback--)
		pushback(eng);
		ret(nel_T_LARROW);

	case '=':
		if (nel_nextc(eng) == '=') {
			//yylval.nodetypeval = Node_equal;
			ret(nel_T_DEQ);
		}
		//yylval.nodetypeval = Node_assign;
		pushback(eng);
		ret(nel_T_EQ);

	case '>':
		if ((c = nel_nextc(eng)) == '=') {
			//yylval.nodetypeval = Node_geq;
			ret(nel_T_RARROWEQ);
		} 
		else if (c == '>' ){
			c = nel_nextc(eng);
			if(c == '='){
				ret(nel_T_DRARROWEQ);
			}
			pushback(eng);
			ret(nel_T_DRARROW);
		}

		//yylval.nodetypeval = Node_greater;
		pushback(eng);
		ret(nel_T_RARROW); 

	case '~':
		//yylval.nodetypeval = Node_match;
		//eng->parser->want_assign = 0;
		if ((c = nel_nextc(eng)) == '=') {
			eng->parser->want_assign = 0;
			ret(nel_T_TILDEEQ);
		}
		pushback(eng);
		ret(nel_T_TILDE);

	case '"':
		esc_seen = 0;
		while ((c = nel_nextc(eng)) != '"') {
			if (c == '\n') {
				pushback(eng);
				parser_stmt_error(eng, "unterminated string\n");
			}		

			if(c == '\\') {
				int hex = 0;
				c = nel_nextc(eng);
				switch(c) {
				case '\n':
					/* skip '\\' and '\n' */
					continue;
	
				case 'a':
					tokadd(eng, '\a');
					continue;
				case 'b':
					tokadd(eng, '\b');
					continue;
				case 'f':
					tokadd(eng, '\f');
					continue;
				case 'n':
					tokadd(eng, '\n');
					continue;
				case 'r':
					tokadd(eng, '\r');
					continue;
				case 't':
					tokadd(eng, '\t');
					continue;
				case 'v':
					tokadd(eng, '\v');
					continue;
				case '\\':
					tokadd(eng, '\\');
					continue;
				case '\?':
					tokadd(eng, '\?');
					continue;
				case '\'':
					tokadd(eng, '\'');
					continue;
				case '\"':
					tokadd(eng, '\"');
					continue;

				case 'X':
				case 'x':
					hex = 1;
					c = nel_nextc(eng);
					/* pass through */

				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
					{
					int val = 0;
					int i = 0;
					int power = (hex) ? 16 : 8; 

					for( ; ; c = nel_nextc(eng)){
						int d = 0;
						switch(c) {
							case '0': 
							case '1': 
							case '2': 
							case '3': 
							case '4':
							case '5': 
							case '6':
							case '7': 
								d = c - '0'; 
								break;

							case '8': 
							case '9': 
								if(!hex) 
									goto out_of_dquoted;
								d = c - '0';
								break;


							case 'a':
							case 'b':
							case 'c':
							case 'd':
							case 'e':
							case 'f':
								if(!hex) 
									goto out_of_dquoted;
								d = c -'a'+ 10;
								break;


							case 'A': 
							case 'B': 
							case 'C': 
							case 'D': 
							case 'E': 
							case 'F': 
								if(!hex) 
									goto out_of_dquoted;
								d = c -'A'+ 10;
								break;
							
							default :
								goto dquoted_finish;
						}

						i++;
						if ( ((hex)? i > 2 : i > 3) 
							|| val > ( UCHAR_MAX -d ) / power ) {

out_of_dquoted:
							parser_stmt_error(eng, "%s escape sequence out of range", hex ? "hex" : "octal");
								
						}
						val *= power ;
						val += d;
					}

dquoted_finish:
					if (c != 0)
						pushback(eng);
			
					tokadd(eng, val);
					}

					continue;
			
				default:
					tokadd(eng, '\\');
					break;
				}
				

			}

			if (c == '\0') {
				pushback(eng);
				parser_stmt_error(eng, "unterminated string\n");
			}

			tokadd(eng, c);
		}

		//yylval.nodeval = make_str_node(tokstart,
		//	tok - tokstart, esc_seen ? 1 : 0);
		tokadd(eng, '\0');	
		ret(nel_T_C_STRING);

	case '-':
		if ((c = nel_nextc(eng)) == '=') {
			//yylval.nodetypeval = Node_assign_minus;
			ret(nel_T_MINUSEQ);
		}

		if (c == '-'){
			ret(nel_T_DMINUS);
		}	

		if (c == '>'){
			ret(nel_T_MINUSRARROW);
		}
		pushback(eng);
		ret(nel_T_MINUS);

	case '.':
		overflow = 0;
		c = nel_nextc(eng);

		if(c == '.') {
			c = nel_nextc(eng);
			if(c == '.')
				ret(nel_T_ELLIPSIS);
			
			pushback(eng);
		}
		else if (isdigit(c)) {
			tokadd(eng, '.');
			goto found_f;
		}
		
		pushback(eng);
		ret(nel_T_DOT);

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		overflow = 0;
		n = 0;
		c1 = nel_nextc(eng);
		pushback(eng);
		if(c == '0' && (c1 == 'x' || c1 == 'X')) {
			int d;

			c = nel_nextc(eng);
			c = nel_nextc(eng);

			while (c) {
				if(isdigit(c))
					d = c - '0';
				else if (c >= 'a' && c <= 'f')
					d = c - 'a' + 10;
				else if (c >= 'A' && c <= 'F')
					d = c - 'A' + 10;
				else 
					break;
				if (n & ~(~0UL >> 4))
					overflow = 1;
				else 
					n = (n<<4) + d;
				c = nel_nextc(eng);
			}

			if(c != '\0')
				pushback(eng);

			goto found_i;

		}
		else if (c == '0') {
			for(; isdigit(c); c = nel_nextc(eng)) {
				if ( c == '8' || c == '9')
					parser_error(eng, "invalid octal constant"); 
				if (n & ~(~0UL >> 3))
					overflow = 1;
				else 
					n = ( n<< 3) + (c - '0');
			}
			
			pushback(eng);
			goto found_i;

		} else {
			for( ; isdigit(c); c = nel_nextc(eng)){
				int d = c - '0';
				if (n > ( ULONG_MAX  -d ) / 10)
					overflow = 1;
				n = 10 * n + d; 
				tokadd(eng, c);
			}
		
			tokadd(eng, '\0');	
			pushback(eng);	
			if(c == '.' || c == 'e' || c == 'E') {
found_f:
				/* this if a floating point number */
				if (c == '.'){
					do {
						c = nel_nextc(eng);
						tokadd(eng, c);
					} while (isdigit(c));
				}

				if(c == 'e' || c == 'E') {
					c = nel_nextc(eng);
					tokadd(eng, c);
					if (c == '-' || c == '+') {
						c = nel_nextc(eng);
						tokadd(eng, c);
					}
					if(isdigit(c)) {
						do {
							c = nel_nextc(eng);
							tokadd(eng, c);
						}while (isdigit(c));

					}else {
						parser_error(eng, "invalid octal constant"); 
					}

				}	

				if(c == 'f' || c == 'F') {
//found_float:
					sscanf(eng->parser->text, "%f", 
							&(eng->parser->float_lval));
					ret(nel_T_C_FLOAT);
				}
				else if (c == 'l' || c == 'L') {
//found_long_double:
					sscanf(eng->parser->text, "%Lf", 
						&(eng->parser->long_double_lval));
					ret(nel_T_C_LONG_DOUBLE);
				}else {
//found_double:
					sscanf(eng->parser->text, "%lf", 
							&(eng->parser->double_lval));
					ret(nel_T_C_DOUBLE);
				}
				
			}

found_i:
			c = nel_nextc(eng);
			c1 = nel_nextc(eng);

			if (((c == 'u' || c == 'U') && (c1 =='l' || c1 == 'L'))
				|| ((c == 'l' || c == 'L') && (c1 == 'u' || c1 == 'U' ))) {
				goto found_unsigned_long_int;	
			}
			else if (c == 'u' || c == 'U') {
				if(c1 != '\0')
					pushback(eng);
				if (overflow || n > UINT_MAX ) {
					goto found_unsigned_long_int;
				}else {
					goto found_unsigned_int;
				}

			}else if (c == 'l' || c == 'L') {
				if(c1 != '\0')
					pushback(eng);
				if (overflow || n > LONG_MAX ) {
					goto found_unsigned_long_int;
				}else {
					goto found_signed_long_int;
				}

			}else if(overflow || n > LONG_MAX ){
				if(c1 != '\0')
					pushback(eng);
				if(c != '\0')
					pushback(eng);
found_unsigned_long_int:
				/*
				sscanf(eng->parser->text, "%ul",
					&(eng->parser->unsigned_long_int_lval));
				*/
				eng->parser->unsigned_long_int_lval = n;
				ret(nel_T_C_UNSIGNED_LONG_INT);

			}else if( n > UINT_MAX ){
				if(c1 != '\0')
					pushback(eng);
				if(c != '\0')
					pushback(eng);
found_signed_long_int:
				eng->parser->long_int_lval = n;
				ret(nel_T_C_SIGNED_LONG_INT);

			}else if ( n > INT_MAX ) {
				if(c1 != '\0')
					pushback(eng);
				if(c != '\0')
					pushback(eng);
found_unsigned_int:
				eng->parser->unsigned_int_lval = n;	
				ret(nel_T_C_UNSIGNED_INT);

			}else {
				/*every number is int , even it can be stored in char */
				if(c1 != '\0')
					pushback(eng);
				if(c != '\0')
					pushback(eng);
//found_signed_int:
				eng->parser->int_lval = n;	
				ret(nel_T_C_SIGNED_INT);
			}

		}
		break;

	case '&':
		if ((c = nel_nextc(eng)) == '&') {
			//yylval.nodetypeval = Node_and;
			//for (;;) {
			//	c = nextc();
			//	if (c == '\0')
			//		break;
			//	if (c == '#') {
			//		while ((c = nextc()) != '\n' && c != '\0')
			//			continue;
			//		if (c == '\0')
			//			break;
			//	}
			//	if (! isspace(c)) {
			//		pushback();
			//		break;
			//	}
			//}
			//eng->parser->want_assign = 0;
			ret(nel_T_DAMPER);
		}
		else if (c == '=') {
			ret(nel_T_AMPEREQ);
		}

		pushback(eng);
		ret(nel_T_AMPER); 

	case '|':
		if ((c = nel_nextc(eng)) == '|') {
			//yylval.nodetypeval = Node_or;
			//for (;;) {
			//	c = nextc();
			//	if (c == '\0')
			//		break;
			//	if (c == '#') {
			//		while ((c = nextc()) != '\n' && c != '\0')
			//			continue;
			//		if (c == '\0')
			//			break;
			//	}
			//	if (! isspace(c)) {
			//		pushback();
			//		break;
			//	}
			//}
			eng->parser->want_assign = 0;
			ret(nel_T_DBAR);
		}
		else if (c == '=') {
			ret(nel_T_BAREQ);
		}
		pushback(eng);
		ret(nel_T_BAR); 
	}


	if ( c != '_' && ! isalpha(c))
		parser_stmt_error(eng, "Invalid char '%c' in expression\n", c);

	/* it's some type of name-type-thing.  Find its length */
	while ( isalnum(c) || c == '_' ) {
		tokadd(eng, c);
		c = nel_nextc(eng);
	}
	tokadd(eng, '\0');
	
	if (c != 0)
		pushback(eng);

	/* keyword */ 
	for(i =0; i< keywordtablesize; i++){
		if(!strcmp(keywordtable[i].name, eng->parser->text)){
			eng->parser->want_assign = 1;
			ret(keywordtable[i].id);
		}
	}		
	
	eng->parser->want_assign = 1;
	/* we have to look up a symbol to see if it is a typedef name. */

	symbol = parser_lookup_ident (eng, eng->parser->text);
	if ( symbol && (symbol->type != NULL) 
		&& ((symbol->type->simple.type == nel_D_TYPEDEF_NAME)
		|| (symbol->type->simple.type == nel_D_TYPE_NAME))) {
		ret (nel_T_TYPEDEF_NAME);
	}

	ret (nel_T_IDENT);

}



/*****************************************************************************/
/* nel_token_string () returns the character string associated with      */
/* <token>.                                                                  */
/*****************************************************************************/
char *nel_token_string (register int token)
{
      switch (token) {
         case nel_T_ASM:			return ("asm");
         case nel_T_AUTO:			return ("auto");
         case nel_T_BREAK:			return ("break");
         case nel_T_CASE:			return ("case");
         case nel_T_CHAR:			return ("char");
         case nel_T_COMPLEX:		return ("complex");
         case nel_T_CONST:			return ("const");
         case nel_T_CONTINUE:		return ("continue");
         case nel_T_DEFAULT:		return ("default");
         case nel_T_DO:			return ("do");
         case nel_T_DOUBLE:			return ("double");
         case nel_T_ELSE:			return ("else");
         case nel_T_ENUM:			return ("enum");
         case nel_T_EXTERN:			return ("extern");
         case nel_T_FLOAT:			return ("float");
         case nel_T_FOR:			return ("for");
         case nel_T_FORTRAN:		return ("fortran");
         /*case nel_T_GOTO:			return ("goto");*/
         case nel_T_IF:			return ("if");
         case nel_T_INT:			return ("int");
         case nel_T_LONG:			return ("long");
         case nel_T_REGISTER:		return ("register");
         case nel_T_RETURN:			return ("return");
         case nel_T_SHORT:			return ("short");
         case nel_T_SIGNED:			return ("signed");
         case nel_T_SIZEOF:			return ("sizeof");
         case nel_T_STATIC:			return ("static");
         case nel_T_STRUCT:			return ("struct");
         case nel_T_SWITCH:			return ("switch");
         case nel_T_TYPEDEC:		return ("typedec");
         case nel_T_TYPEDEF:		return ("typedef");
         case nel_T_TYPEOF:			return ("typeof");
         case nel_T_UNION:			return ("union");
         case nel_T_UNSIGNED:		return ("unsigned");
         case nel_T_VOID:			return ("void");
         case nel_T_VOLATILE:		return ("volatile");
         case nel_T_WHILE:			return ("while");
         case nel_T_P_DEFINE:		return ("#define");
         case nel_T_P_ELIF:			return ("#elif");
         case nel_T_P_ELSE:			return ("#else");
         case nel_T_P_ENDIF:		return ("#endif");
         case nel_T_P_ERROR:		return ("#error");
         case nel_T_P_IF:			return ("#if");
         case nel_T_P_IFDEF:		return ("#ifdef");
         case nel_T_P_IFNDEF:		return ("#ifndef");
         case nel_T_P_INCLUDE:		return ("#include");
         case nel_T_P_LINE:			return ("#line");
         case nel_T_P_PRAGMA:		return ("#pragma");
         case nel_T_P_POUNDS:		return ("#pounds");
         case nel_T_P_UNDEF:		return ("#undef");
         case nel_T_AMPER:			return ("&");
         case nel_T_AMPEREQ:		return ("&=");
         case nel_T_BAR:			return ("|");
         case nel_T_BAREQ:			return ("|=");
         case nel_T_COLON:			return (":");
         case nel_T_COMMA:			return (",");
         case nel_T_DAMPER:			return ("&&");
         case nel_T_DBAR:			return ("||");
         case nel_T_DEQ:			return ("==");
         case nel_T_DLARROW:		return ("<<");
         case nel_T_DLARROWEQ:		return ("<<=");
         case nel_T_DMINUS:			return ("--");
         case nel_T_DOT:			return (".");
         case nel_T_DPLUS:			return ("++");
         case nel_T_DRARROW:		return (">>");
         case nel_T_DRARROWEQ:		return (">>=");
         case nel_T_ELLIPSIS:		return ("...");
         case nel_T_EQ:			return ("=");
         case nel_T_EXCLAM:			return ("!");
         case nel_T_EXCLAMEQ:		return ("!=");
         case nel_T_LARROW:			return ("<");
         case nel_T_LARROWEQ:		return ("<=");
         case nel_T_LBRACE:			return ("{");
         case nel_T_LBRACK:			return ("[");
         case nel_T_LPAREN:			return ("(");
         case nel_T_MINUS:			return ("-");
         case nel_T_MINUSRARROW:		return ("->");
         case nel_T_MINUSEQ:		return ("-=");
         case nel_T_PERCENT:		return ("%");
         case nel_T_PERCENTEQ:		return ("%=");
         case nel_T_PLUS:			return ("+");
         case nel_T_PLUSEQ:			return ("+=");
         case nel_T_QUESTION:		return ("?");
         case nel_T_RARROW:			return (">");
         case nel_T_RARROWEQ:		return (">=");
         case nel_T_RBRACE:			return ("}");
         case nel_T_RBRACK:			return ("]");
         case nel_T_RPAREN:			return (")");
         case nel_T_SEMI:			return (";");
         case nel_T_SLASH:			return ("/");
         case nel_T_SLASHEQ:		return ("/=");
         case nel_T_STAR:			return ("*");
         case nel_T_STAREQ:			return ("*=");
         case nel_T_TILDE:			return ("~");
         case nel_T_TILDEEQ:			return ("~=");
         case nel_T_UPARROW:		return ("^");
         case nel_T_UPARROWEQ:		return ("^=");
         case nel_T_C_CHAR:			return ("nel_T_C_CHAR");
         case nel_T_C_DOUBLE:		return ("nel_T_C_DOUBLE");
         case nel_T_C_FLOAT:		return ("nel_T_C_FLOAT");
         case nel_T_C_LONG_DOUBLE:		return ("nel_T_C_LONG_DOUBLE");
         case nel_T_C_SIGNED_INT:		return ("nel_T_C_SIGNED_INT");
         case nel_T_C_SIGNED_LONG_INT:	return ("nel_T_C_SIGNED_LONG_INT");
         case nel_T_C_STRING:		return ("nel_T_C_STRING");
         case nel_T_C_UNSIGNED_INT:		return ("nel_T_C_UNSIGNED_INT");
         case nel_T_C_UNSIGNED_LONG_INT:	return ("nel_T_C_UNSIGNED_LONG_INT");
         case nel_T_IDENT:			return ("nel_T_IDENT");
         case nel_T_TYPEDEF_NAME:		return ("nel_T_TYPEDEF_NAME");
         case nel_T_NULL:			return ("nel_T_NULL");
         case nel_T_MAX:			return ("nel_T_MAX");
	 case nel_T_NODELAY:		return ("nel_T_NODELAY");
         default:				return ("nel_T_BAD_TOKEN");
      }
   }

void nel_save_lex_context(struct nel_eng *eng)
{
	/* save the old current_buffer */
	//parser_push_value(eng, eng->parser->current_buffer, struct yy_buffer_state *);
	//parser_push_value(eng, eng->parser->hold_char, char);
	//parser_push_value(eng, eng->parser->n_chars, int);

	parser_push_value(eng, eng->parser->text, char *);
	eng->parser->text = NULL;

	//parser_push_value(eng, eng->parser->leng, int);
	parser_push_value(eng, eng->parser->infile, FILE *);

	//parser_push_value(eng, eng->parser->c_buf_p, char *);
	//parser_push_value(eng, eng->parser->init, int);

	//parser_push_value(eng, eng->parser->start, int);
	//parser_push_value(eng, eng->parser->did_buffer_switch_on_eof, int);
	//parser_push_value(eng, eng->parser->last_accepting_state,int);
	//parser_push_value(eng, eng->parser->last_accepting_cpos, char *);
	//parser_push_value(eng, eng->parser->more_flag, int);
	//parser_push_value(eng, eng->parser->doing_more, int);
	//parser_push_value(eng, eng->parser->more_len, int);

	parser_push_value(eng, eng->parser->lexptr, char *);
	eng->parser->lexptr = NULL;

	parser_push_value(eng, eng->parser->lexptr_begin, char *);
	eng->parser->lexptr_begin = NULL;

	parser_push_value(eng, eng->parser->lexend, char *);
	eng->parser->lexend = NULL;

	parser_push_value(eng, eng->parser->tok, char *);
	eng->parser->tok = NULL;

	parser_push_value(eng, eng->parser->tokend, char *);
	eng->parser->tokend = NULL;

	parser_push_value(eng, eng->parser->buf, char *);
	nel_alloca(eng->parser->buf, nel_YY_BUF_SIZE + 2 , char );

	parser_push_value(eng, eng->parser->line, int);
	eng->parser->line = 1;

	parser_push_value(eng, eng->parser->want_assign, int);
	eng->parser->want_assign = 1;
}

void nel_restore_lex_context(struct nel_eng *eng)
{
	parser_pop_value(eng, eng->parser->want_assign, int);
	parser_pop_value(eng, eng->parser->line, int);
	parser_pop_value(eng, eng->parser->buf, char *);
	parser_pop_value(eng, eng->parser->tokend, char *);
	parser_pop_value(eng, eng->parser->tok, char *);
	parser_pop_value(eng, eng->parser->lexend, char *);
	parser_pop_value(eng, eng->parser->lexptr_begin, char *);
	parser_pop_value(eng, eng->parser->lexptr, char *);

	//parser_pop_value(eng, eng->parser->more_len, int);	
	//parser_pop_value(eng, eng->parser->doing_more, int);	
	//parser_pop_value(eng, eng->parser->more_flag, int);	
	//parser_pop_value(eng, eng->parser->last_accepting_cpos, char * );	
	//parser_pop_value(eng, eng->parser->last_accepting_state,int );	
	//parser_pop_value(eng, eng->parser->did_buffer_switch_on_eof, int );	
	//parser_pop_value(eng, eng->parser->start, int );	
	//parser_pop_value(eng, eng->parser->init, int );	
	//parser_pop_value(eng, eng->parser->c_buf_p, char * );	
	parser_pop_value(eng, eng->parser->infile, FILE * );	
	//parser_pop_value(eng, eng->parser->leng, int );	
	parser_pop_value(eng, eng->parser->text, char * );	
	//parser_pop_value(eng, eng->parser->n_chars, int );	
	//parser_pop_value(eng, eng->parser->hold_char, char );	
	//parser_pop_value (eng, eng->parser->current_buffer, struct yy_buffer_state *);


}

/*************************************************************/
/* use the following macro to initialize the nel_eng fields */
/* used by the lexer in order to read input from <_infile>.  */
/*************************************************************/
void nel_parser_lex_init(struct nel_eng *_eng, FILE *_infile) 
{								
	nel_debug ({							
	if ((_eng) == NULL) {					
		parser_fatal_error (NULL, "(nel_parser_lex_rec_init #1): eng == NULL"); 
	}								
	});							

	(_eng)->parser->lexptr = NULL;
	(_eng)->parser->lexptr_begin = NULL;
	(_eng)->parser->lexend = NULL;
	(_eng)->parser->line = 1;
	(_eng)->parser->tok = NULL;
	(_eng)->parser->tokend = NULL;
	(_eng)->parser->want_assign = 1;
	(_eng)->parser->infile = (_infile);				

	//nel_alloca ((_eng)->parser->current_buffer, 1, struct yy_buffer_state); 
	//(_eng)->parser->current_buffer->buf_size = nel_YY_BUF_SIZE;
	//nel_alloca ((_eng)->parser->current_buffer->ch_buf, nel_YY_BUF_SIZE + 2, char); 
	//(_eng)->parser->hold_char = '\0';	   		
	//(_eng)->parser->n_chars = 0;	  
	(_eng)->parser->text = NULL;
	nel_alloca ((_eng)->parser->buf, nel_YY_BUF_SIZE + 2, char);
	(_eng)->parser->lexptr_begin = (_eng)->parser->buf;
	//(_eng)->parser->leng = 0;
	//(_eng)->parser->c_buf_p = NULL;
	//(_eng)->parser->init = 1;
	//(_eng)->parser->start = 0;
	//(_eng)->parser->did_buffer_switch_on_eof = 0;
	//(_eng)->parser->last_accepting_state = 0;
	//(_eng)->parser->last_accepting_cpos = NULL;	
	//(_eng)->parser->more_flag = 0;		
	//(_eng)->parser->doing_more = 0;	
	//(_eng)->parser->more_len = 0;
}

void nel_parser_lex_dealloc(struct nel_eng *_eng) 
{								
	nel_debug ({
	if ((_eng) == NULL) {	
		parser_fatal_error (NULL, "(nel_parser_lex_dealloc #1): eng== NULL"); 
	}
	if ((_eng)->parser->infile == NULL) {
		parser_fatal_error ((_eng), "(nel_parser_lex_dealloc #2): eng->parser->infile == NULL"); 
	}
	//if ((_eng)->parser->current_buffer == NULL) {
	//	parser_fatal_error ((_eng), "(nel_parser_lex_dealloc #3): eng->parser->current_buffer == NULL"); 
	//}
	});

	//nel_dealloca ((_eng)->parser->current_buffer->ch_buf);
	//nel_dealloca ((_eng)->parser->current_buffer);

	nel_dealloca ((_eng)->parser->buf);
	(_eng)->parser->infile = NULL;
	
	if((_eng)->parser->text)
		nel_dealloca((_eng)->parser->text);
	
	//(_eng)->parser->current_buffer = NULL;
}
