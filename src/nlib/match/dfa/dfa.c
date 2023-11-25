/* dfa.c - deterministic extended regexp routines for GNU
   Copyright 1988, 1998, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA */

/* Written June, 1988 by Mike Haertel
   Modified July, 1988 by Arthur David Olson to assist BMG speedups  */

//#define DEBUG 1
//#define DEBUG_0531 1


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <ctype.h>
#include <stdio.h>

#include <sys/types.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#else
extern char *calloc(), *malloc(), *realloc();
extern void free();
#endif

#if defined(HAVE_STRING_H) || defined(STDC_HEADERS)
#include <string.h>
#else
#include <strings.h>
#endif

#if HAVE_SETLOCALE
# include <locale.h>
#endif

#include <regex.h>

#ifndef DEBUG	/* use the same approach as regex.c */
#undef assert
#define assert(e)
#endif /* DEBUG */

#ifndef isgraph
#define isgraph(C) (isprint(C) && !isspace(C))
#endif

#if defined (STDC_HEADERS) || (!defined (isascii) && !defined (HAVE_ISASCII))
#define ISALPHA(C) isalpha(C)
#define ISUPPER(C) isupper(C)
#define ISLOWER(C) islower(C)
#define ISDIGIT(C) isdigit(C)
#define ISXDIGIT(C) isxdigit(C)
#define ISSPACE(C) isspace(C)
#define ISPUNCT(C) ispunct(C)
#define ISALNUM(C) isalnum(C)
#define ISPRINT(C) isprint(C)
#define ISGRAPH(C) isgraph(C)
#define ISCNTRL(C) iscntrl(C)
#else
#define ISALPHA(C) (isascii(C) && isalpha(C))
#define ISUPPER(C) (isascii(C) && isupper(C))
#define ISLOWER(C) (isascii(C) && islower(C))
#define ISDIGIT(C) (isascii(C) && isdigit(C))
#define ISXDIGIT(C) (isascii(C) && isxdigit(C))
#define ISSPACE(C) (isascii(C) && isspace(C))
#define ISPUNCT(C) (isascii(C) && ispunct(C))
#define ISALNUM(C) (isascii(C) && isalnum(C))
#define ISPRINT(C) (isascii(C) && isprint(C))
#define ISGRAPH(C) (isascii(C) && isgraph(C))
#define ISCNTRL(C) (isascii(C) && iscntrl(C))
#endif


/* ISASCIIDIGIT differs from ISDIGIT, as follows:
   - Its arg may be any int or unsigned int; it need not be an unsigned char.
   - It's guaranteed to evaluate its argument exactly once.
   - It's typically faster.
   Posix 1003.2-1992 section 2.5.2.1 page 50 lines 1556-1558 says that
   only '0' through '9' are digits.  Prefer ISASCIIDIGIT to ISDIGIT unless
   it's important to use the locale's definition of `digit' even when the
   host does not conform to Posix.  */
#define ISASCIIDIGIT(c) ((unsigned) (c) - '0' <= 9)


/* If we (don't) have I18N.  */
/* glibc defines _ */
#ifndef _
# ifdef HAVE_LIBINTL_H
#  include <libintl.h>
#  ifndef _
#   define _(Str) gettext (Str)
#  endif
# else
#  define _(Str) (Str)
# endif
#endif

#include "nlib/match/dfa.h"

/* HPUX, define those as macros in sys/param.h */
#ifdef setbit
# undef setbit
#endif
#ifdef clrbit
# undef clrbit
#endif

//static void dfamust PARAMS ((struct dfa *dfa));
static void regexp PARAMS ((int toplevel));

void
dfaerror (char const *mesg)
{
  //error (2, 0, mesg);
  fprintf(stderr, "%s", mesg); 
}


static ptr_t
xcalloc (size_t n, size_t s)
{
  ptr_t r = calloc(n, s);

  if (!r)
    dfaerror(_("Memory exhausted"));
  return r;
}

static ptr_t
xmalloc (size_t n)
{
  ptr_t r = malloc(n);

  assert(n != 0);
  if (!r)
    dfaerror(_("Memory exhausted"));
  return r;
}

//////////
#if 0
static ptr_t
realloc(ptr_t p, size_t n) {
	register ptr_t __p;
	
	if(p == NULL){
		__p = (ptr_t)malloc(n);
	}
	else{
		__p = (ptr_t)malloc(n);
		
		memcpy(__p, p, n);	
		if(p < MapAddress || p > (MapAddress + MapSize * 1024 * 1024)) {
			free(p);	
		}
		else {
			p = NULL;
		}
	}
	
	return __p;
}
#endif
/////////

static ptr_t
xrealloc (ptr_t p, size_t n)
{
  ptr_t r = realloc(p, n);

  assert(n != 0);
  if (!r)
    dfaerror(_("Memory exhausted"));
  return r;
}

#define CALLOC(p, t, n) ((p) = (t *) xcalloc((size_t)(n), sizeof (t)))
#define MALLOC(p, t, n) ((p) = (t *) xmalloc((n) * sizeof (t)))
#define REALLOC(p, t, n) ((p) = (t *) xrealloc((ptr_t) (p), (n) * sizeof (t)))

/* Reallocate an array of type t if nalloc is too small for index. */
#define REALLOC_IF_NECESSARY(p, t, nalloc, index) \
  if ((index) >= (nalloc))			  \
    {						  \
      do					  \
	(nalloc) *= 2;				  \
      while ((index) >= (nalloc));		  \
      REALLOC(p, t, nalloc);			  \
    }

#ifdef DEBUG

static void
prtok (token t)
{
  char const *s;

  if (t < 0)
    fprintf(stderr, "END");
  else if (t < NOTCHAR)
    fprintf(stderr, "%c", t);
  else
    {
      switch (t)
	{
	case EMPTY: s = "EMPTY"; break;
	case BACKREF: s = "BACKREF"; break;
	case BEGLINE: s = "BEGLINE"; break;
	case ENDLINE: s = "ENDLINE"; break;
	case BEGWORD: s = "BEGWORD"; break;
	case ENDWORD: s = "ENDWORD"; break;
	case LIMWORD: s = "LIMWORD"; break;
	case NOTLIMWORD: s = "NOTLIMWORD"; break;
	case QMARK: s = "QMARK"; break;
	case STAR: s = "STAR"; break;
	case PLUS: s = "PLUS"; break;
	case CAT: s = "CAT"; break;
	case OR: s = "OR"; break;
	case ORTOP: s = "ORTOP"; break;
	case LPAREN: s = "LPAREN"; break;
	case RPAREN: s = "RPAREN"; break;
	case CRANGE: s = "CRANGE"; break;
	default: s = "CSET"; break;
	}
      fprintf(stderr, "%s", s);
    }
}
#endif /* DEBUG */

/* Stuff pertaining to charclasses. */

static int
tstbit (unsigned b, charclass c)
{
  return c[b / INTBITS] & 1 << b % INTBITS;
}

static void
setbit (unsigned b, charclass c)
{
  c[b / INTBITS] |= 1 << b % INTBITS;
}

static void
clrbit (unsigned b, charclass c)
{
  c[b / INTBITS] &= ~(1 << b % INTBITS);
}

static void
copyset (charclass src, charclass dst)
{
  memcpy (dst, src, sizeof (charclass));
}

static void
zeroset (charclass s)
{
  memset (s, 0, sizeof (charclass));
}

static void
notset (charclass s)
{
  int i;

  for (i = 0; i < CHARCLASS_INTS; ++i)
    s[i] = ~s[i];
}

static int
equal (charclass s1, charclass s2)
{
  return memcmp (s1, s2, sizeof (charclass)) == 0;
}

/* A pointer to the current dfa is kept here during parsing. */
//static struct dfa *dfa;

//static struct anchor anc[100];
//static int num = 0;

/* Find the index of charclass s in dfa->charclasses, or allocate a new charclass. */
static int
charclass_index (struct dfa *dfa, charclass s)  
{
  int i;

  for (i = 0; i < dfa->cindex; ++i)
    if (equal(s, dfa->charclasses[i]))
      return i;
  REALLOC_IF_NECESSARY(dfa->charclasses, charclass, dfa->calloc, dfa->cindex);
  ++dfa->cindex;
  copyset(s, dfa->charclasses[i]);
  return i;
}

/* Syntax bits controlling the behavior of the lexical analyzer. */
//static reg_syntax_t syntax_bits, syntax_bits_set;

/* Flag for case-folding letters into sets. */
//static int case_fold;

/* End-of-line byte in data.  */
//static unsigned char eolbyte;

/* Entry point to set syntax options. */
void
dfasyntax (struct dfa *dfa, reg_syntax_t bits, int fold, unsigned char eol
#ifdef	DFA_LAZY
, int lazy
#endif
)
{
  dfa->syntax_bits_set = 1;
  dfa->syntax_bits = bits;
  dfa->case_fold = fold;
  dfa->eolbyte = eol;
#ifdef	DFA_LAZY
  dfa->lazy_flag = lazy;
#endif
}

/* Like setbit, but if case is folded, set both cases of a letter.  */
static void
setbit_case_fold (struct dfa *dfa, unsigned b, charclass c)
{
  setbit (b, c);
  if (dfa->case_fold)
    {
      if (ISUPPER (b))
	setbit (tolower (b), c);
      else if (ISLOWER (b))
	setbit (toupper (b), c);
    }
}

/* Lexical analyzer.  All the dross that deals with the obnoxious
   GNU Regex syntax bits is located here.  The poor, suffering
   reader is referred to the GNU Regex documentation for the
   meaning of the @#%!@#%^!@ syntax bits. */

//static char const *lexstart;	/* Pointer to beginning of input string. */
//static char const *lexptr;	/* Pointer to next input character. */
//static int lexleft;		/* Number of characters remaining. */
//static token lasttok;		/* Previous token returned; initially END. */
//static int laststart;		/* True if we're separated from beginning or (, |
//				   only by zero-width characters. */
//static int parens;		/* Count of outstanding left parens. */
//static int minrep, maxrep;	/* Repeat counts for {m,n}. */
//static int hard_LC_COLLATE;	/* Nonzero if LC_COLLATE is hard.  */



/* Note that characters become unsigned here. */
# define FETCH(dfa, c, eoferr)   	      \
  {			   	      \
    if (! dfa->lexleft)	   	      \
      {				      \
	if (eoferr != 0)	      \
	  dfaerror (eoferr);	      \
	else		   	      \
	  return dfa->lasttok = END;	      \
      }				      \
    (c) = (unsigned char) *dfa->lexptr++;  \
    --dfa->lexleft;		   	      \
  }


#ifdef __STDC__
#define FUNC(F, P) static int F(int c) { return P(c); }
#else
#define FUNC(F, P) static int F(c) int c; { return P(c); }
#endif

FUNC(is_alpha, ISALPHA)
FUNC(is_upper, ISUPPER)
FUNC(is_lower, ISLOWER)
FUNC(is_digit, ISDIGIT)
FUNC(is_xdigit, ISXDIGIT)
FUNC(is_space, ISSPACE)
FUNC(is_punct, ISPUNCT)
FUNC(is_alnum, ISALNUM)
FUNC(is_print, ISPRINT)
FUNC(is_graph, ISGRAPH)
FUNC(is_cntrl, ISCNTRL)

static int
is_blank (int c)
{
   return (c == ' ' || c == '\t');
}

/* The following list maps the names of the Posix named character classes
   to predicate functions that determine whether a given character is in
   the class.  The leading [ has already been eaten by the lexical analyzer. */
static struct {
  const char *name;
  int (*pred) PARAMS ((int));
} const prednames[] = {
  { ":alpha:]", is_alpha },
  { ":upper:]", is_upper },
  { ":lower:]", is_lower },
  { ":digit:]", is_digit },
  { ":xdigit:]", is_xdigit },
  { ":space:]", is_space },
  { ":punct:]", is_punct },
  { ":alnum:]", is_alnum },
  { ":print:]", is_print },
  { ":graph:]", is_graph },
  { ":cntrl:]", is_cntrl },
  { ":blank:]", is_blank },
  { 0 }
};


/* Return non-zero if C is a `word-constituent' byte; zero otherwise.  */
#define IS_WORD_CONSTITUENT(C) (ISALNUM(C) || (C) == '_')

static int
looking_at (struct dfa *dfa, char const *s)
{
  size_t len;

  len = strlen(s);
  if (dfa->lexleft < len)
    return 0;
  return strncmp(s, dfa->lexptr, len) == 0;
}

static token
lex (struct dfa *dfa)
{
	unsigned c, c1, c2;
	int backslash = 0, invert;
	charclass ccl;
	int i;

	/* Basic plan: We fetch a character.  If it's a backslash,
	we set the backslash flag and go through the loop again.
	On the plus side, this avoids having a duplicate of the
	main switch inside the backslash case.  On the minus side,
	it means that just about every case begins with
	"if (backslash) ...".  */
	for (i = 0; i < 200; ++i)
	{
		FETCH(dfa, c, 0);
		switch (c)
		{
			case '\\':
				if (backslash)
					goto normal_char;
				if (dfa->lexleft == 0)
					dfaerror(_("Unfinished \\ escape"));
				backslash = 1;
				break;

			case '^':
				if (backslash)
					goto normal_char;
				if (dfa->syntax_bits & RE_CONTEXT_INDEP_ANCHORS
				|| dfa->lasttok == END
				|| dfa->lasttok == LPAREN
				|| dfa->lasttok == OR)
					return dfa->lasttok = BEGLINE;
				goto normal_char;

			case '$':
				if (backslash)
					goto normal_char;
				if (dfa->syntax_bits & RE_CONTEXT_INDEP_ANCHORS
				|| dfa->lexleft == 0
				|| (dfa->syntax_bits & RE_NO_BK_PARENS
					? dfa->lexleft > 0 && *(dfa->lexptr) == ')'
					: dfa->lexleft > 1 && dfa->lexptr[0] == '\\' && dfa->lexptr[1] == ')')
				|| (dfa->syntax_bits & RE_NO_BK_VBAR
					? dfa->lexleft > 0 && *(dfa->lexptr) == '|'
					: dfa->lexleft > 1 && dfa->lexptr[0] == '\\' && dfa->lexptr[1] == '|')
				|| ((dfa->syntax_bits & RE_NEWLINE_ALT)
					&& dfa->lexleft > 0 && *(dfa->lexptr) == '\n'))
					return dfa->lasttok = ENDLINE;
				goto normal_char;

			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				if (backslash && !(dfa->syntax_bits & RE_NO_BK_REFS)) {
#ifdef 	BACKREF_AS_RULEID	
					/* save ruleid with backref */
					dfa->ruleid = c - '0';
					for (;;) {
						if (dfa->lexleft) {
							FETCH(dfa, c, 0);
							if (! ISASCIIDIGIT (c))
								break;
							dfa->ruleid = 10 * dfa->ruleid + c - '0';
						} else  
							goto rule_id_end;
					}
					dfa->lexleft++;
					dfa->lexptr--;
				rule_id_end:
#endif
					dfa->laststart = 0;
					return dfa->lasttok = BACKREF;
				}
				goto normal_char;

			case '`':
				if (backslash && !(dfa->syntax_bits & RE_NO_GNU_OPS))
				//return dfa->lasttok = BEGLINE;	/* FIXME: should be beginning of string */
				return dfa->lasttok = 39;
				goto normal_char;

			case '\'':
				if (backslash && !(dfa->syntax_bits & RE_NO_GNU_OPS))
				//return dfa->lasttok = ENDLINE;	/* FIXME: should be end of string */
						return dfa->lasttok = 39;
				goto normal_char;

			case '<':
				if (backslash && !(dfa->syntax_bits & RE_NO_GNU_OPS))
				//return dfa->lasttok = BEGWORD;
						return dfa->lasttok = 60;
				goto normal_char;

			case '>':
				if (backslash && !(dfa->syntax_bits & RE_NO_GNU_OPS))
				//return dfa->lasttok = ENDWORD;
						return dfa->lasttok = 62;
				goto normal_char;

			case 'b':
				if (backslash && !(dfa->syntax_bits & RE_NO_GNU_OPS))
				return dfa->lasttok = LIMWORD;
				goto normal_char;

			case 'B':
				if (backslash && !(dfa->syntax_bits & RE_NO_GNU_OPS))
				return dfa->lasttok = NOTLIMWORD;
				goto normal_char;

			case '?':
				if (dfa->syntax_bits & RE_LIMITED_OPS)
				goto normal_char;
				if (backslash != ((dfa->syntax_bits & RE_BK_PLUS_QM) != 0))
				goto normal_char;
				if (!(dfa->syntax_bits & RE_CONTEXT_INDEP_OPS) && dfa->laststart)
				goto normal_char;
				return dfa->lasttok = QMARK;

			case '*':
				if (backslash)
				goto normal_char;
				if (!(dfa->syntax_bits & RE_CONTEXT_INDEP_OPS) && dfa->laststart)
				goto normal_char;
				return dfa->lasttok = STAR;

			case '+':
				if (dfa->syntax_bits & RE_LIMITED_OPS)
				goto normal_char;
				if (backslash != ((dfa->syntax_bits & RE_BK_PLUS_QM) != 0))
				goto normal_char;
				if (!(dfa->syntax_bits & RE_CONTEXT_INDEP_OPS) && dfa->laststart)
				goto normal_char;
				return dfa->lasttok = PLUS;

			case '{':
				if (!(dfa->syntax_bits & RE_INTERVALS))
				goto normal_char;
				if (backslash != ((dfa->syntax_bits & RE_NO_BK_BRACES) == 0))
				goto normal_char;
				if (!(dfa->syntax_bits & RE_CONTEXT_INDEP_OPS) && dfa->laststart)
				goto normal_char;

				if (dfa->syntax_bits & RE_NO_BK_BRACES)
				{
					/* Scan ahead for a valid interval; if it's not valid,
					 treat it as a literal '{'.  */
					int lo = -1, hi = -1;
					char const *p = dfa->lexptr;
					char const *lim = p + dfa->lexleft;
					for (;  p != lim && ISASCIIDIGIT (*p);  p++)
						lo = (lo < 0 ? 0 : lo * 10) + *p - '0';
					if (p != lim && *p == ',')
						while (++p != lim && ISASCIIDIGIT (*p))
							hi = (hi < 0 ? 0 : hi * 10) + *p - '0';
					else
						hi = lo;

					if (p == lim || *p != '}'
						|| lo < 0 || RE_DUP_MAX < hi || (0 <= hi && hi < lo))
						goto normal_char;
				}

				dfa->minrep = 0;
				/* Cases:
				{M} - exact count
				{M,} - minimum count, maximum is infinity
				{M,N} - M through N */
				FETCH(dfa, c, _("unfinished repeat count"));
				if (ISASCIIDIGIT (c))
				{
					dfa->minrep = c - '0';
					for (;;)
					{
						FETCH(dfa, c, _("unfinished repeat count"));
						if (! ISASCIIDIGIT (c))
							break;
						dfa->minrep = 10 * dfa->minrep + c - '0';
					}
				}
				else
					dfaerror(_("malformed repeat count"));
				if (c == ',')
				{
					FETCH (dfa, c, _("unfinished repeat count"));
					if (! ISASCIIDIGIT (c))
						dfa->maxrep = -1;
					else
					{
						dfa->maxrep = c - '0';
						for (;;)
						{
							FETCH (dfa, c, _("unfinished repeat count"));
							if (! ISASCIIDIGIT (c))
								break;
							dfa->maxrep = 10 * dfa->maxrep + c - '0';
						}

						if (0 <= dfa->maxrep && dfa->maxrep < dfa->minrep)
						dfaerror (_("malformed repeat count"));
					}
	    			}
	 			else
					dfa->maxrep = dfa->minrep;

				if (!(dfa->syntax_bits & RE_NO_BK_BRACES))
	  		  	{
					if (c != '\\')
						dfaerror(_("malformed repeat count"));
					FETCH(dfa, c, _("unfinished repeat count"));
				}
				if (c != '}')
					dfaerror(_("malformed repeat count"));
				dfa->laststart = 0;
				return dfa->lasttok = REPMN;

			case '|':
				if (dfa->syntax_bits & RE_LIMITED_OPS)
				goto normal_char;
				if (backslash != ((dfa->syntax_bits & RE_NO_BK_VBAR) == 0))
				goto normal_char;
				dfa->laststart = 1;
				return dfa->lasttok = OR;

			/*  
			case '\n':
				if (syntax_bits & RE_LIMITED_OPS
				|| backslash
				|| !(syntax_bits & RE_NEWLINE_ALT))
				goto normal_char;
				laststart = 1;
				return lasttok = OR;
			*/

			case '(':
				if (backslash != ((dfa->syntax_bits & RE_NO_BK_PARENS) == 0))
					goto normal_char;
				++dfa->parens;
				dfa->laststart = 1;
				return dfa->lasttok = LPAREN;

			case ')':
				if (backslash != ((dfa->syntax_bits & RE_NO_BK_PARENS) == 0))
					goto normal_char;
				if (dfa->parens == 0 && dfa->syntax_bits & RE_UNMATCHED_RIGHT_PAREN_ORD)
					goto normal_char;
				--dfa->parens;
				dfa->laststart = 0;
				return dfa->lasttok = RPAREN;

			case '.':
				if (backslash)
				goto normal_char;
				zeroset(ccl);
				notset(ccl);
				if (!(dfa->syntax_bits & RE_DOT_NEWLINE))
					//clrbit(dfa->eolbyte, ccl);
				clrbit('\n', ccl);
				if (dfa->syntax_bits & RE_DOT_NOT_NULL)
					clrbit('\0', ccl);
				dfa->laststart = 0;
				return dfa->lasttok = CSET + charclass_index(dfa, ccl);

			case 'f':
				if (!backslash || (dfa->syntax_bits & RE_NO_GNU_OPS))
				goto normal_char;
				return '\f';
					
			case 'v':
				if (!backslash || (dfa->syntax_bits & RE_NO_GNU_OPS))
				goto normal_char;
					return '\v';
					
			case 't':
				if (!backslash || (dfa->syntax_bits & RE_NO_GNU_OPS))
				goto normal_char;
					return '\t';
					
			case 'r':
				if (!backslash || (dfa->syntax_bits & RE_NO_GNU_OPS))
				goto normal_char;
					return '\r';
					
			case 'n':
				if (!backslash || (dfa->syntax_bits & RE_NO_GNU_OPS))
				goto normal_char;
					return '\n';
				//end

			case 'd':  
			case 'D':
				if (!backslash || (dfa->syntax_bits & RE_NO_GNU_OPS))
				goto normal_char;
				zeroset(ccl);
				for (c2 = 48; c2 < 58; ++c2)
				if (IS_WORD_CONSTITUENT(c2))
				setbit(c2, ccl);
				if (c == 'D')
				notset(ccl);
				dfa->laststart = 0;
				return dfa->lasttok = CSET + charclass_index(dfa, ccl);
					
			case 's': 
			case 'S':
				if (!backslash || (dfa->syntax_bits & RE_NO_GNU_OPS))
				goto normal_char;
				zeroset(ccl);
				for (c2 = 9; c2 < 14; ++c2)
				//if (IS_WORD_CONSTITUENT(c2)) 
				setbit(c2, ccl);
					setbit(32, ccl);
				if (c == 'S')
				notset(ccl);
				dfa->laststart = 0;
				return dfa->lasttok = CSET + charclass_index(dfa, ccl);
				
			case 'w':
			case 'W':
				if (!backslash || (dfa->syntax_bits & RE_NO_GNU_OPS))
				goto normal_char;
				zeroset(ccl);
				for (c2 = 0; c2 < NOTCHAR; ++c2)
				if (IS_WORD_CONSTITUENT(c2))
				setbit(c2, ccl);
				if (c == 'W')
				notset(ccl);
				dfa->laststart = 0;
				return dfa->lasttok = CSET + charclass_index(dfa, ccl);

			case '[':
				if (backslash)
					goto normal_char;
				dfa->laststart = 0;
				zeroset(ccl);
				FETCH(dfa, c, _("Unbalanced ["));
				if (c == '^')
				{
					FETCH(dfa, c, _("Unbalanced ["));
					invert = 1;
				}
				else
					invert = 0;

				do
				{
					/* Nobody ever said this had to be fast. :-)
					Note that if we're looking at some other [:...:]
					construct, we just treat it as a bunch of ordinary
					characters.  We can do this because we assume
					regex has checked for syntax errors before
					dfa is ever called. */
					if (c == '[' && (dfa->syntax_bits & RE_CHAR_CLASSES))
						for (c1 = 0; prednames[c1].name; ++c1)
							if (looking_at(dfa, prednames[c1].name))
							{
								int (*pred) PARAMS ((int)) = prednames[c1].pred;

								for (c2 = 0; c2 < NOTCHAR; ++c2)
									if ((*pred)(c2))
										setbit_case_fold (dfa, c2, ccl);
								dfa->lexptr += strlen(prednames[c1].name);
								dfa->lexleft -= strlen(prednames[c1].name);
								FETCH(dfa, c1, _("Unbalanced ["));
								goto skip;
							}

					if (c == '\\' && (dfa->syntax_bits & RE_BACKSLASH_ESCAPE_IN_LISTS))
					{
						FETCH(dfa, c, _("Unbalanced ["));
						if (c == 'x')
						{
							int dec_num = 0;   //\xA9   dec_num = A9
							int j = 0;
							while (j++ < 2)
							{
								FETCH(dfa, c,0);                       //fetch a char
								if (dfa->lexleft==0 && j==1)
									dfaerror(_("Unfinished \xA9 "));
								if (!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')))
									dfaerror(_("Error \xAG  "));
								if (c >= 'a') c -= 32;              // Convert to upper case
								dec_num = dec_num * 16 + c - ((c < 'A')? '0' : ('A' - 10));
							}
							c = dec_num;
						}
						else if (c == 'd')
						{
							int t1;
							for (t1 = 48; t1 < 57; ++t1)
							{
								setbit_case_fold (dfa, t1, ccl);
							}
							c = 57;
						}
						else if (c == 's')
						{
							int t1;
							for (t1 = 9; t1 < 14; ++t1)
								setbit_case_fold (dfa, t1, ccl);
							c = 32;
						}
						else if (c == 'w')
						{
							int t1;
							for (t1 = 0; t1 < NOTCHAR; ++t1)
								if (IS_WORD_CONSTITUENT(t1))
									setbit_case_fold (dfa, t1, ccl);
							c = 'a';
						}
						else if (c == 'n') 
						{
						  	c = 10;
						}
						else
						{
							 
						}
						
						//end
					}

					FETCH(dfa, c1, _("Unbalanced ["));
					if (c1 == '-')
					{
						FETCH(dfa, c2, _("Unbalanced ["));
						if (c2 == ']')
						{
							/* In the case [x-], the - is an ordinary hyphen,
								which is left in c1, the lookahead character. */
							--dfa->lexptr;
							++dfa->lexleft;
						}
						else
						{
							if (c2 == '\\'
			  					&& (dfa->syntax_bits & RE_BACKSLASH_ESCAPE_IN_LISTS))
							{
								FETCH(dfa, c2, _("Unbalanced ["));
								if (c2 == 'x')
								{
									int dec_num = 0;   //\xA9   dec_num = A9
									int j = 0;
									while (j++ < 2)
									{
										FETCH(dfa, c2,0);                       //fetch a char
										if (dfa->lexleft==0 && j==1)
											dfaerror(_("Unfinished \xA9 "));
										if (!((c2>='0'&&c2<='9')||(c2>='a'&&c2<='f')||(c2>='A'&&c2<='F')))
											dfaerror(_("Error \xAG  "));
										if (c2 >= 'a') 
											c2 -= 32;              // Convert to upper case
										dec_num = dec_num * 16 + c2 - ((c < 'A')? '0' : ('A' - 10));
									}
									c2 = dec_num;
								}
								//end
							}

							FETCH(dfa, c1, _("Unbalanced ["));
							if (! dfa->hard_LC_COLLATE) {
								for (; c <= c2; c++)
									setbit_case_fold (dfa, c, ccl);
							} else {
								/* POSIX locales are painful - leave the decision to libc */
								char expr[6] = { '[', c, '-', c2, ']', '\0' };
								regex_t re;
								if (regcomp (&re, expr, dfa->case_fold ? REG_ICASE : 0) == REG_NOERROR) {
									for (c = 0; c < NOTCHAR; ++c) {
										char buf[2] = { c, '\0' };
										regmatch_t mat;
										if (regexec (&re, buf, 1, &mat, 0) == REG_NOERROR
											 && mat.rm_so == 0 && mat.rm_eo == 1)
											setbit_case_fold (dfa, c, ccl);
			  						}
			 				 		regfree (&re);
								}

		      					}

		     					continue;

		   				}

					}

					setbit_case_fold (dfa, c, ccl);

skip:
	      				;
	    			}

				while ((c = c1) != ']');
	  			if (invert)
				{
					notset(ccl);
					if (dfa->syntax_bits & RE_HAT_LISTS_NOT_NEWLINE)
						clrbit(dfa->eolbyte, ccl);
				}

				return dfa->lasttok = CSET + charclass_index(dfa, ccl);

			case 'x':
	  			if (!backslash)
					goto normal_char;
				int dec_num = 0;   //\xA9   dec_num = A9
				int j = 0;
				while (j++ < 2)
     				{
					FETCH(dfa, c,0);                       //fetch a char
					if (dfa->lexleft==0 && j==1)
						dfaerror(_("Unfinished \xA9 "));
					if (!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')))
						dfaerror(_("Error \xAG  "));
					if (c >= 'a') c -= 32;              // Convert to upper case
						dec_num = dec_num * 16 + c - ((c < 'A')? '0' : ('A' - 10));
				}
				c = dec_num;

				dfa->laststart = 0;
				if (dfa->case_fold && ISALPHA(c))
				{
					zeroset(ccl);
					setbit_case_fold (dfa, c, ccl);
					return dfa->lasttok = CSET + charclass_index(dfa, ccl);
				}
				return c;
	
			default:
normal_char:			dfa->laststart = 0;
				if (dfa->case_fold && ISALPHA(c)) {
					zeroset(ccl);
					setbit_case_fold (dfa, c, ccl);
					return dfa->lasttok = CSET + charclass_index(dfa, ccl);
				}

				return c;

		}


	}

	/* The above loop should consume at most a backslash
	and some other character. */
	abort();
	return END;	/* keeps pedantic compilers happy. */
}

/* Recursive descent parser for regular expressions. */

//static token tok;		/* Lookahead token. */
/*static int depth;		 Current depth of a hypothetical stack
				   holding deferred productions.  This is
				   used to determine the depth that will be
				   required of the real stack later on in
				   dfaanalyze(). */

/* Add the given token to the parse tree, maintaining the depth count and
   updating the maximum depth if necessary. */
static void
addtok (struct dfa *dfa, token t)
{
  REALLOC_IF_NECESSARY(dfa->tokens, token, dfa->talloc, dfa->tindex);
  dfa->tokens[dfa->tindex++] = t;

  switch (t)
    {
    case QMARK:
    case STAR:
    case PLUS:
      break;

    case CAT:
    case OR:
    case ORTOP:
      --dfa->stack_depth;
      break;

#ifdef	BACKREF_AS_RULEID
    case BACKREF:
	if(dfa->talloc > dfa->ralloc) {	
		REALLOC(dfa->rules,  int, dfa->talloc);
		while (dfa->ralloc < dfa->talloc) {
		    dfa->rules[dfa->ralloc++] = 0;
		}
	}
	dfa->rules[dfa->tindex - 1] = dfa->ruleid;
	dfa->ruleid  = 0;
	/* pass through */
#endif

    default:
      ++dfa->nleaves;
    case EMPTY:
      ++dfa->stack_depth;
      break;
    }
  if (dfa->stack_depth > dfa->depth)
    dfa->depth = dfa->stack_depth;
}

/* The grammar understood by the parser is as follows.

   regexp:
     regexp OR branch
     branch

   branch:
     branch closure
     closure

   closure:
     closure QMARK
     closure STAR
     closure PLUS
     closure REPMN
     atom

   atom:
     <normal character>
     <multibyte character>
     ANYCHAR
     MBCSET
     CSET
     BACKREF
     BEGLINE
     ENDLINE
     BEGWORD
     ENDWORD
     LIMWORD
     NOTLIMWORD
     CRANGE
     LPAREN regexp RPAREN
     <empty>

   The parser builds a parse tree in postfix form in an array of tokens. */

static void
atom (struct dfa *dfa)
{
  if ((dfa->tok >= 0 && dfa->tok < NOTCHAR) || dfa->tok >= CSET || dfa->tok == BACKREF
      || dfa->tok == BEGLINE || dfa->tok == ENDLINE || dfa->tok == BEGWORD
      || dfa->tok == ENDWORD || dfa->tok == LIMWORD || dfa->tok == NOTLIMWORD)
    {
      addtok(dfa, dfa->tok);
      dfa->tok = lex(dfa);
    }
  else if (dfa->tok == CRANGE)
    {
      /* A character range like "[a-z]" in a locale other than "C" or
	 "POSIX".  This range might any sequence of one or more
	 characters.  Unfortunately the POSIX locale primitives give
	 us no practical way to find what character sequences might be
	 matched.  Treat this approximately like "(.\1)" -- i.e. match
	 one character, and then punt to the full matcher.  */
      charclass ccl;
      zeroset (ccl);
      notset (ccl);
      addtok (dfa, CSET + charclass_index (dfa, ccl));
      addtok (dfa, BACKREF);
      addtok (dfa, CAT);
      dfa->tok = lex (dfa);
    }
  else if (dfa->tok == LPAREN)
    {
      dfa->tok = lex(dfa);
      regexp(dfa, 0);
      if (dfa->tok != RPAREN)
	dfaerror(_("Unbalanced ("));
      dfa->tok = lex(dfa);
    }
  else
    addtok(dfa, EMPTY);
}

/* Return the number of tokens in the given subexpression. */
static int
nsubtoks (struct dfa *dfa, int tindex)
{
  int ntoks1;

  switch (dfa->tokens[tindex - 1])
    {
    default:
      return 1;
    case QMARK:
    case STAR:
    case PLUS:
      return 1 + nsubtoks(dfa, tindex - 1);
    case CAT:
    case OR:
    case ORTOP:
      ntoks1 = nsubtoks(dfa, tindex - 1);
      return 1 + ntoks1 + nsubtoks(dfa, tindex - 1 - ntoks1);
    }
}

/* Copy the given subexpression to the top of the tree. */
static void
copytoks (struct dfa *dfa, int tindex, int ntokens)
{
  int i;

  for (i = 0; i < ntokens; ++i)
    addtok(dfa, dfa->tokens[tindex + i]);
}

static void
closure (struct dfa *dfa)
{
  int tindex, ntokens, i;

  atom(dfa);
  while (dfa->tok == QMARK || dfa->tok == STAR || dfa->tok == PLUS || dfa->tok == REPMN)
    if (dfa->tok == REPMN)
      {
	ntokens = nsubtoks(dfa, dfa->tindex);
	tindex = dfa->tindex - ntokens;
	if (dfa->maxrep < 0)
	  addtok(dfa, PLUS);
	if (dfa->minrep == 0)
	  addtok(dfa, QMARK);
	for (i = 1; i < dfa->minrep; ++i)
	  {
	    copytoks(dfa, tindex, ntokens);
	    addtok(dfa, CAT);
	  }
	for (; i < dfa->maxrep; ++i)
	  {
	    copytoks(dfa, tindex, ntokens);
	    addtok(dfa, QMARK);
	    addtok(dfa, CAT);
	  }
	dfa->tok = lex(dfa);
      }
    else
      {
	addtok(dfa, dfa->tok);
	dfa->tok = lex(dfa);
      }
}

static void
branch (struct dfa *dfa)
{
  closure(dfa);
  while (dfa->tok != RPAREN && dfa->tok != OR && dfa->tok >= 0)
    {
      closure(dfa);
      addtok(dfa, CAT);
    }
}

static void
regexp (struct dfa *dfa, int toplevel)
{
  branch(dfa);
  while (dfa->tok == OR)
    {
      dfa->tok = lex(dfa);
      branch(dfa);
      if (toplevel)
				addtok(dfa, ORTOP);
      else
				addtok(dfa, OR);
    }
}

/* Main entry point for the parser.  S is a string to be parsed, len is the
   length of the string, so s can include NUL characters.  D is a pointer to
   the struct dfa to parse into. */
void
dfaparse (struct dfa *dfa, char const *s, size_t len)
{
  dfa->lexstart = dfa->lexptr = s;
  dfa->lexleft = len;
  dfa->lasttok = END;
  dfa->laststart = 1;
  dfa->parens = 0;
#if ENABLE_NLS
  dfa->hard_LC_COLLATE = hard_locale (LC_COLLATE);
#endif

  if (! dfa->syntax_bits_set)
    dfaerror(_("No syntax specified"));

  dfa->tok = lex(dfa);
  dfa->stack_depth = dfa->depth;

  regexp(dfa, 1);

  if (dfa->tok != END)
    dfaerror(_("Unbalanced )"));

  addtok(dfa, END - dfa->nregexps);
  addtok(dfa, CAT);

  if (dfa->nregexps)
    addtok(dfa, ORTOP);

  ++dfa->nregexps;
}

/* Some primitives for operating on sets of positions. */

/* Copy one set to another; the destination must be large enough. */
static void
copy (position_set const *src, position_set *dst)
{
  int i;

  for (i = 0; i < src->nelem; ++i)
    dst->elems[i] = src->elems[i];
  dst->nelem = src->nelem;
}

/* Insert a position in a set.  Position sets are maintained in sorted
   order according to index.  If position already exists in the set with
   the same index then their constraints are logically or'd together.
   S->elems must point to an array large enough to hold the resulting set. */
static void
insert (position p, position_set *s)
{
  int i;
  position t1, t2;

  for (i = 0; i < s->nelem && p.index < s->elems[i].index; ++i)
    continue;
  if (i < s->nelem && p.index == s->elems[i].index)
    s->elems[i].constraint |= p.constraint;
  else
    {
      t1 = p;
      ++s->nelem;
      while (i < s->nelem)
	{
	  t2 = s->elems[i];
	  s->elems[i++] = t1;
	  t1 = t2;
	}
    }
}

/* Merge two sets of positions into a third.  The result is exactly as if
   the positions of both sets were inserted into an initially empty set. */
static void
merge (position_set const *s1, position_set const *s2, position_set *m)
{
  int i = 0, j = 0;

  m->nelem = 0;
  while (i < s1->nelem && j < s2->nelem)
    if (s1->elems[i].index > s2->elems[j].index)
      m->elems[m->nelem++] = s1->elems[i++];
    else if (s1->elems[i].index < s2->elems[j].index)
      m->elems[m->nelem++] = s2->elems[j++];
    else
      {
	m->elems[m->nelem] = s1->elems[i++];
	m->elems[m->nelem++].constraint |= s2->elems[j++].constraint;
      }
  while (i < s1->nelem)
    m->elems[m->nelem++] = s1->elems[i++];
  while (j < s2->nelem)
    m->elems[m->nelem++] = s2->elems[j++];
}

/* Delete a position from a set. */
static void
delete (position p, position_set *s)
{
  int i;

  for (i = 0; i < s->nelem; ++i)
    if (p.index == s->elems[i].index)
      break;
  if (i < s->nelem)
    for (--s->nelem; i < s->nelem; ++i)
      s->elems[i] = s->elems[i + 1];
}

/* Find the index of the state corresponding to the given position set with
   the given preceding context, or create a new state if there is no such
   state.  Newline and letter tell whether we got here on a newline or
   letter, respectively. */
static int
state_index (struct dfa *dfa, position_set const *s, int newline, int letter)
{
  int hash = 0;
  int constraint;
  int i, j;

  newline = newline ? 1 : 0;
  letter = letter ? 1 : 0;

  for (i = 0; i < s->nelem; ++i)
    hash ^= s->elems[i].index + s->elems[i].constraint;

  /* Try to find a state that exactly matches the proposed one. */
  for (i = 0; i < dfa->sindex; ++i)
    {
      if (hash != dfa->states[i].hash || s->nelem != dfa->states[i].elems.nelem
	  || newline != dfa->states[i].newline || letter != dfa->states[i].letter)
	continue;
      for (j = 0; j < s->nelem; ++j)
	if (s->elems[j].constraint
	    != dfa->states[i].elems.elems[j].constraint
	    || s->elems[j].index != dfa->states[i].elems.elems[j].index)
	  break;
      if (j == s->nelem)
	return i;
    }

  /* We'll have to create a new state. */
  //printf("state_index start!\n");
  REALLOC_IF_NECESSARY(dfa->states, dfa_state, dfa->salloc, dfa->sindex);

  //add by miaocs, 20090518
  memset(&(dfa->states[dfa->sindex]),0,(dfa->salloc - dfa->sindex) * sizeof(dfa_state));

  dfa->states[i].hash = hash;
  MALLOC(dfa->states[i].elems.elems, position, s->nelem);
  //printf("MALLOC %d byte\n", s->nelem*sizeof(position)); 
  //printf("state_index end!\n");
  copy(s, &dfa->states[i].elems);
  dfa->states[i].newline = newline;
  dfa->states[i].letter = letter;

#ifdef	BACKREF_AS_RULEID
  dfa->states[i].backref.nelem= 0;
  dfa->states[i].backref.elems= NULL;
#else
  dfa->states[i].backref = 0;
#endif

  dfa->states[i].constraint = 0;
  dfa->states[i].first_end = 0;
 
  for (j = 0; j < s->nelem; ++j)
    if (dfa->tokens[s->elems[j].index] < 0)
      {
	constraint = s->elems[j].constraint;
	if (SUCCEEDS_IN_CONTEXT(constraint, newline, 0, letter, 0)
	    || SUCCEEDS_IN_CONTEXT(constraint, newline, 0, letter, 1)
	    || SUCCEEDS_IN_CONTEXT(constraint, newline, 1, letter, 0)
	    || SUCCEEDS_IN_CONTEXT(constraint, newline, 1, letter, 1))
	  dfa->states[i].constraint |= constraint;
	if (! dfa->states[i].first_end)
	  dfa->states[i].first_end = dfa->tokens[s->elems[j].index];
      }
    else if (dfa->tokens[s->elems[j].index] == BACKREF)
      {
	dfa->states[i].constraint = NO_CONSTRAINT;

#ifdef 	BACKREF_AS_RULEID	
	if(dfa->states[i].backref.elems == NULL)
		MALLOC(dfa->states[i].backref.elems, position, s->nelem);
	dfa->states[i].backref.elems[dfa->states[i].backref.nelem].index  = 
					dfa->rules[s->elems[j].index];
	dfa->states[i].backref.elems[dfa->states[i].backref.nelem].constraint = 
					NO_CONSTRAINT;
	dfa->states[i].backref.nelem++;
#else
	dfa->states[i].backref = 1;
#endif

      }

  ++dfa->sindex;

  return i;
}

/* Find the epsilon closure of a set of positions.  If any position of the set
   contains a symbol that matches the empty string in some context, replace
   that position with the elements of its follow labeled with an appropriate
   constraint.  Repeat exhaustively until no funny positions are left.
   S->elems must be large enough to hold the result. */
static void
epsclosure (position_set *s, struct dfa const *d)
{
  int i, j;
  int *visited;
  position p, old;

  MALLOC(visited, int, d->tindex);
  for (i = 0; i < d->tindex; ++i)
    visited[i] = 0;

  for (i = 0; i < s->nelem; ++i)
    if (d->tokens[s->elems[i].index] >= NOTCHAR
	&& d->tokens[s->elems[i].index] != BACKREF
	&& d->tokens[s->elems[i].index] < CSET)
      {
	old = s->elems[i];
	p.constraint = old.constraint;
	delete(s->elems[i], s);
	if (visited[old.index])
	  {
	    --i;
	    continue;
	  }
	visited[old.index] = 1;
	switch (d->tokens[old.index])
	  {
	  case BEGLINE:
	    p.constraint &= BEGLINE_CONSTRAINT;
	    break;
	  case ENDLINE:
	    p.constraint &= ENDLINE_CONSTRAINT;
	    break;
	  case BEGWORD:
	    p.constraint &= BEGWORD_CONSTRAINT;
	    break;
	  case ENDWORD:
	    p.constraint &= ENDWORD_CONSTRAINT;
	    break;
	  case LIMWORD:
	    p.constraint &= LIMWORD_CONSTRAINT;
	    break;
	  case NOTLIMWORD:
	    p.constraint &= NOTLIMWORD_CONSTRAINT;
	    break;
	  default:
	    break;
	  }
	for (j = 0; j < d->follows[old.index].nelem; ++j)
	  {
	    p.index = d->follows[old.index].elems[j].index;
	    insert(p, s);
	  }
	/* Force rescan to start at the beginning. */
	i = -1;
      }

  free(visited);
}

/* Perform bottom-up analysis on the parse tree, computing various functions.
   Note that at this point, we're pretending constructs like \< are real
   characters rather than constraints on what can follow them.

   Nullable:  A node is nullable if it is at the root of a regexp that can
   match the empty string.
   *  EMPTY leaves are nullable.
   * No other leaf is nullable.
   * A QMARK or STAR node is nullable.
   * A PLUS node is nullable if its argument is nullable.
   * A CAT node is nullable if both its arguments are nullable.
   * An OR node is nullable if either argument is nullable.

   Firstpos:  The firstpos of a node is the set of positions (nonempty leaves)
   that could correspond to the first character of a string matching the
   regexp rooted at the given node.
   * EMPTY leaves have empty firstpos.
   * The firstpos of a nonempty leaf is that leaf itself.
   * The firstpos of a QMARK, STAR, or PLUS node is the firstpos of its
     argument.
   * The firstpos of a CAT node is the firstpos of the left argument, union
     the firstpos of the right if the left argument is nullable.
   * The firstpos of an OR node is the union of firstpos of each argument.

   Lastpos:  The lastpos of a node is the set of positions that could
   correspond to the last character of a string matching the regexp at
   the given node.
   * EMPTY leaves have empty lastpos.
   * The lastpos of a nonempty leaf is that leaf itself.
   * The lastpos of a QMARK, STAR, or PLUS node is the lastpos of its
     argument.
   * The lastpos of a CAT node is the lastpos of its right argument, union
     the lastpos of the left if the right argument is nullable.
   * The lastpos of an OR node is the union of the lastpos of each argument.

   Follow:  The follow of a position is the set of positions that could
   correspond to the character following a character matching the node in
   a string matching the regexp.  At this point we consider special symbols
   that match the empty string in some context to be just normal characters.
   Later, if we find that a special symbol is in a follow set, we will
   replace it with the elements of its follow, labeled with an appropriate
   constraint.
   * Every node in the firstpos of the argument of a STAR or PLUS node is in
     the follow of every node in the lastpos.
   * Every node in the firstpos of the second argument of a CAT node is in
     the follow of every node in the lastpos of the first argument.

   Because of the postfix representation of the parse tree, the depth-first
   analysis is conveniently done by a linear scan with the aid of a stack.
   Sets are stored as arrays of the elements, obeying a stack-like allocation
   scheme; the number of elements in each set deeper in the stack can be
   used to determine the address of a particular set's array. */
void
dfaanalyze (struct dfa *dfa, int searchflag)
{
  int *nullable;		/* Nullable stack. */
  int *nfirstpos;		/* Element count stack for firstpos sets. */
  position *firstpos;		/* Array where firstpos elements are stored. */
  int *nlastpos;		/* Element count stack for lastpos sets. */
  position *lastpos;		/* Array where lastpos elements are stored. */
  int *nalloc;			/* Sizes of arrays allocated to follow sets. */
  position_set tmp;		/* Temporary set for merging sets. */
  position_set merged;		/* Result of merging sets. */
  int wants_newline;		/* True if some position wants newline info. */
  int *o_nullable;
  int *o_nfirst, *o_nlast;
  position *o_firstpos, *o_lastpos;
  int i, j;
  position *pos;


  dfa->searchflag = searchflag;

  MALLOC(nullable, int, dfa->depth);
  o_nullable = nullable;
  MALLOC(nfirstpos, int, dfa->depth);
  o_nfirst = nfirstpos;
  MALLOC(firstpos, position, dfa->nleaves);
  o_firstpos = firstpos, firstpos += dfa->nleaves;
  MALLOC(nlastpos, int, dfa->depth);
  o_nlast = nlastpos;
  MALLOC(lastpos, position, dfa->nleaves);
  o_lastpos = lastpos, lastpos += dfa->nleaves;
  MALLOC(nalloc, int, dfa->tindex);
  for (i = 0; i < dfa->tindex; ++i)
    nalloc[i] = 0;
  MALLOC(merged.elems, position, dfa->nleaves);

  CALLOC(dfa->follows, position_set, dfa->tindex);

  for (i = 0; i < dfa->tindex; ++i)
    switch (dfa->tokens[i])
      {
      case EMPTY:
	/* The empty set is nullable. */
	*nullable++ = 1;

	/* The firstpos and lastpos of the empty leaf are both empty. */
	*nfirstpos++ = *nlastpos++ = 0;
	break;

      case STAR:
      case PLUS:
	/* Every element in the firstpos of the argument is in the follow
	   of every element in the lastpos. */
	tmp.nelem = nfirstpos[-1];
	tmp.elems = firstpos;
	pos = lastpos;
	for (j = 0; j < nlastpos[-1]; ++j)
	  {
	    merge(&tmp, &dfa->follows[pos[j].index], &merged);
	    REALLOC_IF_NECESSARY(dfa->follows[pos[j].index].elems, position,
				 nalloc[pos[j].index], merged.nelem - 1);
	    copy(&merged, &dfa->follows[pos[j].index]);
	  }

      case QMARK:
	/* A QMARK or STAR node is automatically nullable. */
	if (dfa->tokens[i] != PLUS)
	  nullable[-1] = 1;
	break;

      case CAT:
	/* Every element in the firstpos of the second argument is in the
	   follow of every element in the lastpos of the first argument. */
	tmp.nelem = nfirstpos[-1];
	tmp.elems = firstpos;
	pos = lastpos + nlastpos[-1];
	for (j = 0; j < nlastpos[-2]; ++j)
	  {
	    merge(&tmp, &dfa->follows[pos[j].index], &merged);
	    REALLOC_IF_NECESSARY(dfa->follows[pos[j].index].elems, position,
				 nalloc[pos[j].index], merged.nelem - 1);
	    copy(&merged, &dfa->follows[pos[j].index]);
	  }

	/* The firstpos of a CAT node is the firstpos of the first argument,
	   union that of the second argument if the first is nullable. */
	if (nullable[-2])
	  nfirstpos[-2] += nfirstpos[-1];
	else
	  firstpos += nfirstpos[-1];
	--nfirstpos;

	/* The lastpos of a CAT node is the lastpos of the second argument,
	   union that of the first argument if the second is nullable. */
	if (nullable[-1])
	  nlastpos[-2] += nlastpos[-1];
	else
	  {
	    pos = lastpos + nlastpos[-2];
	    for (j = nlastpos[-1] - 1; j >= 0; --j)
	      pos[j] = lastpos[j];
	    lastpos += nlastpos[-2];
	    nlastpos[-2] = nlastpos[-1];
	  }
	--nlastpos;

	/* A CAT node is nullable if both arguments are nullable. */
	nullable[-2] = nullable[-1] && nullable[-2];
	--nullable;
	break;

      case OR:
      case ORTOP:
	/* The firstpos is the union of the firstpos of each argument. */
	nfirstpos[-2] += nfirstpos[-1];
	--nfirstpos;

	/* The lastpos is the union of the lastpos of each argument. */
	nlastpos[-2] += nlastpos[-1];
	--nlastpos;

	/* An OR node is nullable if either argument is nullable. */
	nullable[-2] = nullable[-1] || nullable[-2];
	--nullable;
	break;

      default:
	/* Anything else is a nonempty position.  (Note that special
	   constructs like \< are treated as nonempty strings here;
	   an "epsilon closure" effectively makes them nullable later.
	   Backreferences have to get a real position so we can detect
	   transitions on them later.  But they are nullable. */
	*nullable++ = dfa->tokens[i] == BACKREF;

	/* This position is in its own firstpos and lastpos. */
	*nfirstpos++ = *nlastpos++ = 1;
	--firstpos, --lastpos;
	firstpos->index = lastpos->index = i;
	firstpos->constraint = lastpos->constraint = NO_CONSTRAINT;

	/* Allocate the follow set for this position. */
	nalloc[i] = 1;
	MALLOC(dfa->follows[i].elems, position, nalloc[i]);
	break;
      }

  /* For each follow set that is the follow set of a real position, replace
     it with its epsilon closure. */
  for (i = 0; i < dfa->tindex; ++i)
    if (dfa->tokens[i] < NOTCHAR || dfa->tokens[i] == BACKREF
	|| dfa->tokens[i] >= CSET)
      {
#ifdef DEBUG
	//printf("i = %d\n", i);
	fprintf(stderr, "follows(%d:", i);
	prtok(dfa->tokens[i]);
	fprintf(stderr, "):");
	for (j = dfa->follows[i].nelem - 1; j >= 0; --j)
	  {
	    fprintf(stderr, " %d:", dfa->follows[i].elems[j].index);
	    prtok(dfa->tokens[dfa->follows[i].elems[j].index]);
	  }
	putc('\n', stderr);
#endif
	copy(&dfa->follows[i], &merged);
	epsclosure(&merged, dfa);
	if (dfa->follows[i].nelem < merged.nelem)
	  REALLOC(dfa->follows[i].elems, position, merged.nelem);
	copy(&merged, &dfa->follows[i]);
      }

  /* Get the epsilon closure of the firstpos of the regexp.  The result will
     be the set of positions of state 0. */
  merged.nelem = 0;
  for (i = 0; i < nfirstpos[-1]; ++i)
    insert(firstpos[i], &merged);
  epsclosure(&merged, dfa);

  /* Check if any of the positions of state 0 will want newline context. */
  wants_newline = 0;
  for (i = 0; i < merged.nelem; ++i)
    if (PREV_NEWLINE_DEPENDENT(merged.elems[i].constraint))
      wants_newline = 1;

  /* Build the initial state. */
  dfa->salloc = 1;
  dfa->sindex = 0;
  MALLOC(dfa->states, dfa_state, dfa->salloc);
  state_index(dfa, &merged, wants_newline, 0);

#if 1
  free(o_nullable);
  free(o_nfirst);
  free(o_firstpos);
  free(o_nlast);
  free(o_lastpos);
  free(nalloc);
  free(merged.elems);
#endif

}

/* Find, for each character, the transition out of state s of d, and store
   it in the appropriate slot of trans.

   We divide the positions of s into groups (positions can appear in more
   than one group).  Each group is labeled with a set of characters that
   every position in the group matches (taking into account, if necessary,
   preceding context information of s).  For each group, find the union
   of the its elements' follows.  This set is the set of positions of the
   new state.  For each character in the group's label, set the transition
   on this character to be to a state corresponding to the set's positions,
   and its associated backward context information, if necessary.

   If we are building a searching matcher, we include the positions of state
   0 in every state.

   The collection of groups is constructed by building an equivalence-class
   partition of the positions of s.

   For each position, find the set of characters C that it matches.  Eliminate
   any characters from C that fail on grounds of backward context.

   Search through the groups, looking for a group whose label L has nonempty
   intersection with C.  If L - C is nonempty, create a new group labeled
   L - C and having the same positions as the current group, and set L to
   the intersection of L and C.  Insert the position in this group, set
   C = C - L, and resume scanning.

   If after comparing with every group there are characters remaining in C,
   create a new group labeled with the characters of C and insert this
   position in that group. */
void
dfastate (struct dfa *dfa, int s, int trans[])
{
  position_set grps[NOTCHAR];	/* As many as will ever be needed. */
  charclass labels[NOTCHAR];	/* Labels corresponding to the groups. */
  int ngrps = 0;		/* Number of groups actually used. */
  position pos;			/* Current position being considered. */
  charclass matches;		/* Set of matching characters. */
  int matchesf;			/* True if matches is nonempty. */
  charclass intersect;		/* Intersection with some label set. */
  int intersectf;		/* True if intersect is nonempty. */
  charclass leftovers;		/* Stuff in the label that didn't match. */
  int leftoversf;		/* True if leftovers is nonempty. */
  //static charclass letters;	/* Set of characters considered letters. */
  //static charclass newline;	/* Set of characters that aren't newline. */
  position_set follows;		/* Union of the follows of some group. */
  position_set tmp;		/* Temporary space for merging sets. */
  int state;			/* New state. */
  int wants_newline;		/* New state wants to know newline context. */
  int state_newline;		/* New state on a newline transition. */
  int wants_letter;		/* New state wants to know letter context. */
  int state_letter;		/* New state on a letter transition. */
  //static int initialized;	/* Flag for static initialization. */
  int i, j, k;

  /* Initialize the set of letters, if necessary. */
  if (! dfa->initialized)
    {
      dfa->initialized = 1;
      for (i = 0; i < NOTCHAR; ++i)
	if (IS_WORD_CONSTITUENT(i))
	  setbit(i, dfa->letters);
#if DEBUG_0531
      setbit(dfa->eolbyte, dfa->newline);
#else
			if (dfa->eolbyte != '\0')
      	setbit(dfa->eolbyte, dfa->newline);
#endif
    }

  zeroset(matches);

  for (i = 0; i < dfa->states[s].elems.nelem; ++i)
    {
      pos = dfa->states[s].elems.elems[i];
      if (dfa->tokens[pos.index] >= 0 && dfa->tokens[pos.index] < NOTCHAR)
	setbit(dfa->tokens[pos.index], matches);
      else if (dfa->tokens[pos.index] >= CSET)
	copyset(dfa->charclasses[dfa->tokens[pos.index] - CSET], matches);
      else
	continue;

      /* Some characters may need to be eliminated from matches because
	 they fail in the current context. */
      if (pos.constraint != 0xFF)
	{
	  if (! MATCHES_NEWLINE_CONTEXT(pos.constraint,
					 dfa->states[s].newline, 1))
	    clrbit(dfa->eolbyte, matches);
	  if (! MATCHES_NEWLINE_CONTEXT(pos.constraint,
					 dfa->states[s].newline, 0))
	    for (j = 0; j < CHARCLASS_INTS; ++j)
	      matches[j] &= dfa->newline[j];
	  if (! MATCHES_LETTER_CONTEXT(pos.constraint,
					dfa->states[s].letter, 1))
	    for (j = 0; j < CHARCLASS_INTS; ++j)
	      matches[j] &= ~dfa->letters[j];
	  if (! MATCHES_LETTER_CONTEXT(pos.constraint,
					dfa->states[s].letter, 0))
	    for (j = 0; j < CHARCLASS_INTS; ++j)
	      matches[j] &= dfa->letters[j];

	  /* If there are no characters left, there's no point in going on. */
	  for (j = 0; j < CHARCLASS_INTS && !matches[j]; ++j)
	    continue;
	  if (j == CHARCLASS_INTS)
	    continue;
	}

      for (j = 0; j < ngrps; ++j)
	{
	  /* If matches contains a single character only, and the current
	     group's label doesn't contain that character, go on to the
	     next group. */
	  if (dfa->tokens[pos.index] >= 0 && dfa->tokens[pos.index] < NOTCHAR
	      && !tstbit(dfa->tokens[pos.index], labels[j]))
	    continue;

	  /* Check if this group's label has a nonempty intersection with
	     matches. */
	  intersectf = 0;
	  for (k = 0; k < CHARCLASS_INTS; ++k)
	    (intersect[k] = matches[k] & labels[j][k]) ? (intersectf = 1) : 0;
	  if (! intersectf)
	    continue;

	  /* It does; now find the set differences both ways. */
	  leftoversf = matchesf = 0;
	  for (k = 0; k < CHARCLASS_INTS; ++k)
	    {
	      /* Even an optimizing compiler can't know this for sure. */
	      int match = matches[k], label = labels[j][k];

	      (leftovers[k] = ~match & label) ? (leftoversf = 1) : 0;
	      (matches[k] = match & ~label) ? (matchesf = 1) : 0;
	    }

	  /* If there were leftovers, create a new group labeled with them. */
	  if (leftoversf)
	    {
	      copyset(leftovers, labels[ngrps]);
	      copyset(intersect, labels[j]);
	      MALLOC(grps[ngrps].elems, position, dfa->nleaves);
	      copy(&grps[j], &grps[ngrps]);
	      ++ngrps;
	    }

	  /* Put the position in the current group.  Note that there is no
	     reason to call insert() here. */
	  grps[j].elems[grps[j].nelem++] = pos;

	  /* If every character matching the current position has been
	     accounted for, we're done. */
	  if (! matchesf)
	    break;
	}

      /* If we've passed the last group, and there are still characters
	 unaccounted for, then we'll have to create a new group. */
      if (j == ngrps)
	{
	  copyset(matches, labels[ngrps]);
	  zeroset(matches);
	  MALLOC(grps[ngrps].elems, position, dfa->nleaves);
	  grps[ngrps].nelem = 1;
	  grps[ngrps].elems[0] = pos;
	  ++ngrps;
	}
    }

  MALLOC(follows.elems, position, dfa->nleaves);
  MALLOC(tmp.elems, position, dfa->nleaves);

  /* If we are a searching matcher, the default transition is to a state
     containing the positions of state 0, otherwise the default transition
     is to fail miserably. */
  if (dfa->searchflag)
    {
      wants_newline = 0;
      wants_letter = 0;
      for (i = 0; i < dfa->states[0].elems.nelem; ++i)
	{
	  if (PREV_NEWLINE_DEPENDENT(dfa->states[0].elems.elems[i].constraint))
	    wants_newline = 1;
	  if (PREV_LETTER_DEPENDENT(dfa->states[0].elems.elems[i].constraint))
	    wants_letter = 1;
	}
      copy(&dfa->states[0].elems, &follows);
      state = state_index(dfa, &follows, 0, 0);
      if (wants_newline)
	state_newline = state_index(dfa, &follows, 1, 0);
      else
	state_newline = state;
      if (wants_letter)
	state_letter = state_index(dfa, &follows, 0, 1);
      else
	state_letter = state;
      for (i = 0; i < NOTCHAR; ++i)
	trans[i] = (IS_WORD_CONSTITUENT(i)) ? state_letter : state;
#if DEBUG_0531
			trans[dfa->eolbyte] = state_newline;
#else
			//if (dfa->eolbyte != '\0')
				trans[dfa->eolbyte] = state_newline;
#endif
				
    }
  else
    for (i = 0; i < NOTCHAR; ++i)
      trans[i] = -1;

  for (i = 0; i < ngrps; ++i)
    {
      follows.nelem = 0;

      /* Find the union of the follows of the positions of the group.
	 This is a hideously inefficient loop.  Fix it someday. */
      for (j = 0; j < grps[i].nelem; ++j)
	for (k = 0; k < dfa->follows[grps[i].elems[j].index].nelem; ++k)
	{
	  insert(dfa->follows[grps[i].elems[j].index].elems[k], &follows);
	}


      /* If we are building a searching matcher, throw in the positions
	 of state 0 as well. */
      if (dfa->searchflag)
	for (j = 0; j < dfa->states[0].elems.nelem; ++j)
	{
	  insert(dfa->states[0].elems.elems[j], &follows);
	}

      /* Find out if the new state will want any context information. */
      wants_newline = 0;
      if (tstbit(dfa->eolbyte, labels[i]))
	for (j = 0; j < follows.nelem; ++j)
	  if (PREV_NEWLINE_DEPENDENT(follows.elems[j].constraint))
	    wants_newline = 1;

      wants_letter = 0;
      for (j = 0; j < CHARCLASS_INTS; ++j)
	if (labels[i][j] & dfa->letters[j])
	  break;
      if (j < CHARCLASS_INTS)
	for (j = 0; j < follows.nelem; ++j)
	  if (PREV_LETTER_DEPENDENT(follows.elems[j].constraint))
	    wants_letter = 1;

      /* Find the state(s) corresponding to the union of the follows. */
      state = state_index(dfa, &follows, 0, 0);
      if (wants_newline)
	state_newline = state_index(dfa, &follows, 1, 0);
      else
	state_newline = state;
      if (wants_letter)
	state_letter = state_index(dfa, &follows, 0, 1);
      else
	state_letter = state;

      /* Set the transitions for each character in the current label. */
      for (j = 0; j < CHARCLASS_INTS; ++j)
	for (k = 0; k < INTBITS; ++k)
	  if (labels[i][j] & 1 << k)
	    {
	      int c = j * INTBITS + k;

#if DEBUG_0531
	      if (c == dfa->eolbyte)
					trans[c] = state_newline;
	      else if (IS_WORD_CONSTITUENT(c))
					trans[c] = state_letter;
	      else if (c < NOTCHAR)
					trans[c] = state;
#else
	      if (c == dfa->eolbyte/* && c != '\0'*/)
					trans[c] = state_newline;
	      else if (IS_WORD_CONSTITUENT(c))
					trans[c] = state_letter;
	      else if (c < NOTCHAR/* || c == '\0'*/)
					trans[c] = state;
#endif
	    }
    }

  for (i = 0; i < ngrps; ++i)
    free(grps[i].elems);
  free(follows.elems);
  free(tmp.elems);
}

/* Some routines for manipulating a compiled dfa's transition tables.
   Each state may or may not have a transition table; if it does, and it
   is a non-accepting state, then d->trans[state] points to its table.
   If it is an accepting state then d->fails[state] points to its table.
   If it has no table at all, then d->trans[state] is NULL.
   TODO: Improve this comment, get rid of the unnecessary redundancy. */

static void
build_state (struct dfa *dfa, int s)
{
  int *trans;			/* The new transition table. */
  int i;

  //added by miaocs, 20090518
  if (s >= dfa->sindex){
      dfaerror(_("s >= dfa->sindex in build_state(struct dfa *dfa, int s);\n"));
      return;
  }

#ifdef	DFA_LAZY
  /* Set an upper limit on the number of transition tables that will ever
     exist at once.  1024 is arbitrary.  The idea is that the frequently
     used transition tables will be quickly rebuilt, whereas the ones that
     were only needed once or twice will be cleared away. */
if(dfa->lazy_flag > 0 )
  if (dfa->trcount >= 1024)
    {
      for (i = 0; i < dfa->tralloc; ++i)
	if (dfa->trans[i])
	  {
	    free((ptr_t) dfa->trans[i]);
	    dfa->trans[i] = NULL;
	  }
	else if (dfa->fails[i])
	  {
	    free((ptr_t) dfa->fails[i]);
	    dfa->fails[i] = NULL;
	  }
      dfa->trcount = 0;
    }
#else
	if (dfa->trcount > 1024 * 256 )
		dfaerror(_("Too much of states alloced, bugfix!!!"));
#endif

  ++dfa->trcount;

  /* Set up the success bits for this state. */
  dfa->success[s] = 0;
  if (ACCEPTS_IN_CONTEXT(dfa->states[s].newline, 1, dfa->states[s].letter, 0,
      s, *dfa))
    dfa->success[s] |= 4;
  if (ACCEPTS_IN_CONTEXT(dfa->states[s].newline, 0, dfa->states[s].letter, 1,
      s, *dfa))
    dfa->success[s] |= 2;
  if (ACCEPTS_IN_CONTEXT(dfa->states[s].newline, 0, dfa->states[s].letter, 0,
      s, *dfa))
    dfa->success[s] |= 1;

  MALLOC(trans, int, NOTCHAR);
  dfastate(dfa, s, trans);

  /* Now go through the new transition table, and make sure that the trans
     and fail arrays are allocated large enough to hold a pointer for the
     largest state mentioned in the table. */
  for (i = 0; i < NOTCHAR; ++i)
    if (trans[i] >= dfa->tralloc)
      {
	int oldalloc = dfa->tralloc;

	while (trans[i] >= dfa->tralloc)
	  dfa->tralloc *= 2;
	REALLOC(dfa->realtrans, int *, dfa->tralloc + 1);
	dfa->trans = dfa->realtrans + 1;
	REALLOC(dfa->fails, int *, dfa->tralloc);
	REALLOC(dfa->success, int, dfa->tralloc);
	while (oldalloc < dfa->tralloc)
	  {
	    dfa->trans[oldalloc] = NULL;
	    dfa->fails[oldalloc++] = NULL;
	  }
      }

  /* Newline is a sentinel.  */
#if DEBUG_0531
  //trans[dfa->eolbyte] = -1;
  trans['\0'] = -1;
#else
	if (dfa->eolbyte != '\0')
	{
		//trans[dfa->eolbyte] = -1; 
	}
#endif

  if (ACCEPTING(s, *dfa))
    dfa->fails[s] = trans;
  else
    dfa->trans[s] = trans;
}

static void
build_state_zero (struct dfa *dfa)
{
  dfa->tralloc = 1;
  dfa->trcount = 0;
  CALLOC(dfa->realtrans, int *, dfa->tralloc + 1);
  dfa->trans = dfa->realtrans + 1;
  CALLOC(dfa->fails, int *, dfa->tralloc);
  MALLOC(dfa->success, int, dfa->tralloc);
  build_state(dfa, 0);
}


/* add by miaocs, 20090518 */
static void build_state_all (struct dfa *dfa)
{
	int state;
	//printf("dfa build_state_all(%p): begin... \n", dfa);
	build_state_zero(dfa);
	state = 1;
	while( state < dfa->sindex) {
		build_state(dfa, state);
		state++;
	}
	//printf("dfa build_state_all(%p): %d states of %d state alloced!\n",dfa, dfa->sindex, dfa->salloc );
}

/* Search through a buffer looking for a match to the given struct dfa.
   Find the first occurrence of a string matching the regexp in the buffer,
   and the shortest possible version thereof.  Return the offset of the first
   character after the match, or (size_t) -1 if none is found.  BEGIN points to
   the beginning of the buffer, and SIZE is the size of the buffer.  If SIZE
   is nonzero, BEGIN[SIZE - 1] must be a newline.  BACKREF points to a place
   where we're supposed to store a 1 if backreferencing happened and the
   match needs to be verified by a backtracking matcher.  Otherwise
   we store a 0 in *backref. */
size_t
dfaexec (struct dfa *dfa, char const *begin, size_t size, int *backref, int *state)
{
  register int s;	/* Current state. */
  register int itmp = 0;
  register unsigned char const *p; /* Current input character. */
  register unsigned char const *end; /* One past the last input character.  */
  register int **trans, *t;	/* Copy of d->trans so it can be optimized
				   into a register. */
  register unsigned char eol = dfa->eolbyte;	/* Likewise for eolbyte.  */
  int j, k, l;
  int t1;
  int psize = 0;

  if (!dfa->sbit_init)
  {
    int i;

    dfa->sbit_init = 1;
    for (i = 0; i < NOTCHAR; ++i)
	dfa->sbit[i] = (IS_WORD_CONSTITUENT(i)) ? 2 : 1;

	if (eol != '\0')
		dfa->sbit[eol] = 4;
  }

#ifdef	DFA_LAZY
  if(dfa->lazy_flag > 0 )
    if (!dfa->tralloc)
    {
	build_state_zero(dfa);
    }
#endif

  s = *state; 
  //printf("dfaexec: s = %d\n", s);
  p = (unsigned char const *) begin;
  end = p + size;
  trans = dfa->trans;


  for (;;) {
	while ((t = trans[s])) {
		itmp = s;
		if (p < end) {
			s = t[*p++];
		}
		else {
			//*state = itmp;
			s = t[*p++];
			s = -1;
			break;
		}
	}

    	if (s < 0) {
	  	if (p > end) {
		  	*state = itmp;
	      		return (size_t) -1;
	    	}
	  	s = 0;
	}

    	else if ((t = dfa->fails[s])) {
	  	if (dfa->success[s] & ((p>=end)?4:dfa->sbit[*p])) {
#ifdef	BACKREF_AS_RULEID
	      		if (backref) {
				*backref = (long) &dfa->states[s].backref;
			}
#else
			match_rule(dfa, s, p);	
#endif

			*state = 0;  //clear states
	      		return (char const *) p - begin;
	    	}
		itmp = s;
	  	s = t[*p++];
	}
#ifdef	DFA_LAZY
    	else {
		if (dfa->lazy_flag > 0 ) {
			build_state(dfa, s);
			trans = dfa->trans;
		}
	}
#endif	
  
  }

}

/* Initialize the components of a dfa that the other routines don't
   initialize for themselves. */
void
dfainit (struct dfa *dfa)
{
  dfa->calloc = 1;
  MALLOC(dfa->charclasses, charclass, dfa->calloc);
  dfa->cindex = 0;

  dfa->talloc = 1;
  MALLOC(dfa->tokens, token, dfa->talloc);
  dfa->tindex = dfa->depth = dfa->nleaves = dfa->nregexps = 0;

#ifdef	BACKREF_AS_RULEID
  dfa->ralloc = dfa->talloc;
  MALLOC(dfa->rules, int, dfa->talloc);
  if (dfa->rules != NULL)
  {
	int i;
	for (i = 0; i < dfa->ralloc; i++) {
		dfa->rules[i] = 0;
	}
  }
#endif

  dfa->searchflag = 0;
  dfa->tralloc = 0;

  //d->musts = 0;
  dfa->musts = NULL; 
	
	//printf("dfainit: dfa->retnum = %d\n", dfa->retnum);
	dfa->rule_id = 0;
	//dfa->retnum = 0;

	//CALLOC(dfa->retsyms, int, dfa->retnum);
	dfa->empty_string = "";

  //dfa->offset_table = hash_alloc(1);	

#ifdef	DFA_LAZY
  dfa->lazy_flag = 0;
#endif

}

/* Parse and analyze a single string of the given length. */
void dfacomp (struct dfa *dfa, char const *s, size_t len, int searchflag)
{
  if (dfa->case_fold)	// dummy folding in service of dfamust()
    {
      char *lcopy;
      int i;

      lcopy = malloc(len);
      if (!lcopy)
				dfaerror(_("out of memory"));

      // This is a kludge.
      dfa->case_fold = 0;
      for (i = 0; i < len; ++i)
				if (ISUPPER ((unsigned char) s[i]))
	  			lcopy[i] = tolower ((unsigned char) s[i]);
				else
	  			lcopy[i] = s[i];

      dfainit(dfa);
      dfaparse(dfa, lcopy, len);
      free(lcopy);

      //dfamust(dfa);

      dfa->cindex = dfa->tindex = dfa->depth = dfa->nleaves = dfa->nregexps = 0;
      dfa->case_fold = 1;
      dfaparse(dfa, s, len);
      dfaanalyze(dfa, searchflag);
#ifdef	DFA_LAZY
      if (dfa->lazy_flag == 0)
#endif
	build_state_all(dfa);
    }
  else
    {
      dfainit(dfa);
      dfaparse(dfa, s, len);
		   // dfamust(dfa);  
      dfaanalyze(dfa, searchflag);
#ifdef	DFA_LAZY
      if (dfa->lazy_flag == 0)
#endif
	build_state_all(dfa);
    }
}

/* Free the storage held by the components of a dfa. */
void
dfafree (struct dfa *dfa)
{
  int i;
  struct dfamust *dm, *ndm;

  free((ptr_t) dfa->charclasses);
  free((ptr_t) dfa->tokens);

#ifdef	BACKREF_AS_RULEID
  free((ptr_t) dfa->rules);
#endif

  for (i = 0; i < dfa->sindex; ++i)
    free((ptr_t) dfa->states[i].elems.elems);
  free((ptr_t) dfa->states);
  for (i = 0; i < dfa->tindex; ++i)
    if (dfa->follows[i].elems)
      free((ptr_t) dfa->follows[i].elems);
  free((ptr_t) dfa->follows);
  for (i = 0; i < dfa->tralloc; ++i)
    if (dfa->trans[i])
      free((ptr_t) dfa->trans[i]);
    else if (dfa->fails[i])
      free((ptr_t) dfa->fails[i]);
  if (dfa->realtrans) free((ptr_t) dfa->realtrans);
  if (dfa->fails) free((ptr_t) dfa->fails);
  if (dfa->success) free((ptr_t) dfa->success);

#if 0
  for (dm = dfa->musts; dm; dm = ndm)
    {
      ndm = dm->next;
      free(dm->must);
      free((ptr_t) dm);
    }
#endif

}


#if 0
/* Having found the postfix representation of the regular expression,
   try to find a long sequence of characters that must appear in any line
   containing the r.e.
   Finding a "longest" sequence is beyond the scope here;
   we take an easy way out and hope for the best.
   (Take "(ab|a)b"--please.)

   We do a bottom-up calculation of sequences of characters that must appear
   in matches of r.e.'s represented by trees rooted at the nodes of the postfix
   representation:
	sequences that must appear at the left of the match ("left")
	sequences that must appear at the right of the match ("right")
	lists of sequences that must appear somewhere in the match ("in")
	sequences that must constitute the match ("is")

   When we get to the root of the tree, we use one of the longest of its
   calculated "in" sequences as our answer.  The sequence we find is returned in
   d->must (where "d" is the single argument passed to "dfamust");
   the length of the sequence is returned in d->mustn.

   The sequences calculated for the various types of node (in pseudo ANSI c)
   are shown below.  "p" is the operand of unary operators (and the left-hand
   operand of binary operators); "q" is the right-hand operand of binary
   operators.

   "ZERO" means "a zero-length sequence" below.

	Type	left		right		is		in
	----	----		-----		--		--
	char c	# c		# c		# c		# c

	ANYCHAR	ZERO		ZERO		ZERO		ZERO

	MBCSET	ZERO		ZERO		ZERO		ZERO

	CSET	ZERO		ZERO		ZERO		ZERO

	STAR	ZERO		ZERO		ZERO		ZERO

	QMARK	ZERO		ZERO		ZERO		ZERO

	PLUS	p->left		p->right	ZERO		p->in

	CAT	(p->is==ZERO)?	(q->is==ZERO)?	(p->is!=ZERO &&	p->in plus
		p->left :	q->right :	q->is!=ZERO) ?	q->in plus
		p->is##q->left	p->right##q->is	p->is##q->is :	p->right##q->left
						ZERO

	OR	longest common	longest common	(do p->is and	substrings common to
		leading		trailing	q->is have same	p->in and q->in
		(sub)sequence	(sub)sequence	length and
		of p->left	of p->right	content) ?
		and q->left	and q->right	p->is : NULL

   If there's anything else we recognize in the tree, all four sequences get set
   to zero-length sequences.  If there's something we don't recognize in the tree,
   we just return a zero-length sequence.

   Break ties in favor of infrequent letters (choosing 'zzz' in preference to
   'aaa')?

   And. . .is it here or someplace that we might ponder "optimizations" such as
	egrep 'psi|epsilon'	->	egrep 'psi'
	egrep 'pepsi|epsilon'	->	egrep 'epsi'
					(Yes, we now find "epsi" as a "string
					that must occur", but we might also
					simplify the *entire* r.e. being sought)
	grep '[c]'		->	grep 'c'
	grep '(ab|a)b'		->	grep 'ab'
	grep 'ab*'		->	grep 'a'
	grep 'a*b'		->	grep 'b'

   There are several issues:

   Is optimization easy (enough)?

   Does optimization actually accomplish anything,
   or is the automaton you get from "psi|epsilon" (for example)
   the same as the one you get from "psi" (for example)?

   Are optimizable r.e.'s likely to be used in real-life situations
   (something like 'ab*' is probably unlikely; something like is
   'psi|epsilon' is likelier)? */

static char *
icatalloc (char *old, char *new)
{
  char *result;
  size_t oldsize, newsize;

  newsize = (new == NULL) ? 0 : strlen(new);
  if (old == NULL)
    oldsize = 0;
  else if (newsize == 0)
    return old;
  else	oldsize = strlen(old);
  if (old == NULL)
    result = (char *) malloc(newsize + 1);
  else
    result = (char *) realloc((void *) old, oldsize + newsize + 1);
  if (result != NULL && new != NULL)
    (void) strcpy(result + oldsize, new);
  return result;
}

static char *
icpyalloc (char *string)
{
  return icatalloc((char *) NULL, string);
}

static char *
istrstr (char *lookin, char *lookfor)
{
  char *cp;
  size_t len;

  len = strlen(lookfor);
  for (cp = lookin; *cp != '\0'; ++cp)
    if (strncmp(cp, lookfor, len) == 0)
      return cp;
  return NULL;
}

static void
ifree (char *cp)
{
  if (cp != NULL)
    free(cp);
}

static void
freelist (char **cpp)
{
  int i;

  if (cpp == NULL)
    return;
  for (i = 0; cpp[i] != NULL; ++i)
    {
      free(cpp[i]);
      cpp[i] = NULL;
    }
}

static char **
enlist (char **cpp, char *new, size_t len)
{
  int i, j;

  if (cpp == NULL)
    return NULL;
  if ((new = icpyalloc(new)) == NULL)
    {
      freelist(cpp);
      return NULL;
    }
  new[len] = '\0';
  /* Is there already something in the list that's new (or longer)? */
  for (i = 0; cpp[i] != NULL; ++i)
    if (istrstr(cpp[i], new) != NULL)
      {
	free(new);
	return cpp;
      }
  /* Eliminate any obsoleted strings. */
  j = 0;
  while (cpp[j] != NULL)
    if (istrstr(new, cpp[j]) == NULL)
      ++j;
    else
      {
	free(cpp[j]);
	if (--i == j)
	  break;
	cpp[j] = cpp[i];
	cpp[i] = NULL;
      }
  /* Add the new string. */
  cpp = (char **) realloc((char *) cpp, (i + 2) * sizeof *cpp);
  if (cpp == NULL)
    return NULL;
  cpp[i] = new;
  cpp[i + 1] = NULL;
  return cpp;
}

/* Given pointers to two strings, return a pointer to an allocated
   list of their distinct common substrings. Return NULL if something
   seems wild. */
static char **
comsubs (char *left, char *right)
{
  char **cpp;
  char *lcp;
  char *rcp;
  size_t i, len;

  if (left == NULL || right == NULL)
    return NULL;
  cpp = (char **) malloc(sizeof *cpp);
  if (cpp == NULL)
    return NULL;
  cpp[0] = NULL;
  for (lcp = left; *lcp != '\0'; ++lcp)
    {
      len = 0;
      rcp = strchr (right, *lcp);
      while (rcp != NULL)
	{
	  for (i = 1; lcp[i] != '\0' && lcp[i] == rcp[i]; ++i)
	    continue;
	  if (i > len)
	    len = i;
	  rcp = strchr (rcp + 1, *lcp);
	}
      if (len == 0)
	continue;
      if ((cpp = enlist(cpp, lcp, len)) == NULL)
	break;
    }
  return cpp;
}

static char **
addlists (char **old, char **new)
{
  int i;

  if (old == NULL || new == NULL)
    return NULL;
  for (i = 0; new[i] != NULL; ++i)
    {
      old = enlist(old, new[i], strlen(new[i]));
      if (old == NULL)
	break;
    }
  return old;
}

/* Given two lists of substrings, return a new list giving substrings
   common to both. */
static char **
inboth (char **left, char **right)
{
  char **both;
  char **temp;
  int lnum, rnum;

  if (left == NULL || right == NULL)
    return NULL;
  both = (char **) malloc(sizeof *both);
  if (both == NULL)
    return NULL;
  both[0] = NULL;
  for (lnum = 0; left[lnum] != NULL; ++lnum)
    {
      for (rnum = 0; right[rnum] != NULL; ++rnum)
	{
	  temp = comsubs(left[lnum], right[rnum]);
	  if (temp == NULL)
	    {
	      freelist(both);
	      return NULL;
	    }
	  both = addlists(both, temp);
	  freelist(temp);
	  free(temp);
	  if (both == NULL)
	    return NULL;
	}
    }
  return both;
}

static void
resetmust (struct dfamust *mp)
{
  mp->left[0] = mp->right[0] = mp->is[0] = '\0';
  freelist(mp->in);
}

static void
dfamust (struct dfa *dfa)
{
  struct dfamust *musts;
  struct dfamust *mp;
  char *result;
  int ri;
  int i;
  int exact;
  token t;
  //static must must0;
  struct dfamust *dm;
  //static char empty_string[] = "";

  result = dfa->empty_string;
  exact = 0;
  musts = (must *) malloc((dfa->tindex + 1) * sizeof *musts);
  if (musts == NULL)
    return;
  mp = musts;
  for (i = 0; i <= dfa->tindex; ++i)
    mp[i] = dfa->must0;
  for (i = 0; i <= dfa->tindex; ++i)
    {
      mp[i].in = (char **) malloc(sizeof *mp[i].in);
      mp[i].left = malloc(2);
      mp[i].right = malloc(2);
      mp[i].is = malloc(2);
      if (mp[i].in == NULL || mp[i].left == NULL ||
	  mp[i].right == NULL || mp[i].is == NULL)
	goto done;
      mp[i].left[0] = mp[i].right[0] = mp[i].is[0] = '\0';
      mp[i].in[0] = NULL;
    }
#ifdef DEBUG
  fprintf(stderr, "dfamust:\n");
  for (i = 0; i < dfa->tindex; ++i)
    {
      fprintf(stderr, " %d:", i);
      prtok(dfa->tokens[i]);
    }
  putc('\n', stderr);
#endif
  for (ri = 0; ri < dfa->tindex; ++ri)
    {
      switch (t = dfa->tokens[ri])
	{
	case LPAREN:
	case RPAREN:
	  goto done;		/* "cannot happen" */
	case EMPTY:
	case BEGLINE:
	case ENDLINE:
	case BEGWORD:
	case ENDWORD:
	case LIMWORD:
	case NOTLIMWORD:
	case BACKREF:
	  resetmust(mp);
	  break;
	case STAR:
	case QMARK:
	  if (mp <= musts)
	    goto done;		/* "cannot happen" */
	  --mp;
	  resetmust(mp);
	  break;
	case OR:
	case ORTOP:
	  if (mp < &musts[2])
	    goto done;		/* "cannot happen" */
	  {
	    char **new;
	    must *lmp;
	    must *rmp;
	    int j, ln, rn, n;

	    rmp = --mp;
	    lmp = --mp;
	    /* Guaranteed to be.  Unlikely, but. . . */
	    if (strcmp(lmp->is, rmp->is) != 0)
	      lmp->is[0] = '\0';
	    /* Left side--easy */
	    i = 0;
	    while (lmp->left[i] != '\0' && lmp->left[i] == rmp->left[i])
	      ++i;
	    lmp->left[i] = '\0';
	    /* Right side */
	    ln = strlen(lmp->right);
	    rn = strlen(rmp->right);
	    n = ln;
	    if (n > rn)
	      n = rn;
	    for (i = 0; i < n; ++i)
	      if (lmp->right[ln - i - 1] != rmp->right[rn - i - 1])
		break;
	    for (j = 0; j < i; ++j)
	      lmp->right[j] = lmp->right[(ln - i) + j];
	    lmp->right[j] = '\0';
	    new = inboth(lmp->in, rmp->in);
	    if (new == NULL)
	      goto done;
	    freelist(lmp->in);
	    free((char *) lmp->in);
	    lmp->in = new;
	  }
	  break;
	case PLUS:
	  if (mp <= musts)
	    goto done;		/* "cannot happen" */
	  --mp;
	  mp->is[0] = '\0';
	  break;
	case END:
	  if (mp != &musts[1])
	    goto done;		/* "cannot happen" */
	  for (i = 0; musts[0].in[i] != NULL; ++i)
	    if (strlen(musts[0].in[i]) > strlen(result))
	      result = musts[0].in[i];
	  if (strcmp(result, musts[0].is) == 0)
	    exact = 1;
	  goto done;
	case CAT:
	  if (mp < &musts[2])
	    goto done;		/* "cannot happen" */
	  {
	    must *lmp;
	    must *rmp;

	    rmp = --mp;
	    lmp = --mp;
	    /* In.  Everything in left, plus everything in
	       right, plus catenation of
	       left's right and right's left. */
	    lmp->in = addlists(lmp->in, rmp->in);
	    if (lmp->in == NULL)
	      goto done;
	    if (lmp->right[0] != '\0' &&
		rmp->left[0] != '\0')
	      {
		char *tp;

		tp = icpyalloc(lmp->right);
		if (tp == NULL)
		  goto done;
		tp = icatalloc(tp, rmp->left);
		if (tp == NULL)
		  goto done;
		lmp->in = enlist(lmp->in, tp,
				 strlen(tp));
		free(tp);
		if (lmp->in == NULL)
		  goto done;
	      }
	    /* Left-hand */
	    if (lmp->is[0] != '\0')
	      {
		lmp->left = icatalloc(lmp->left,
				      rmp->left);
		if (lmp->left == NULL)
		  goto done;
	      }
	    /* Right-hand */
	    if (rmp->is[0] == '\0')
	      lmp->right[0] = '\0';
	    lmp->right = icatalloc(lmp->right, rmp->right);
	    if (lmp->right == NULL)
	      goto done;
	    /* Guaranteed to be */
	    if (lmp->is[0] != '\0' && rmp->is[0] != '\0')
	      {
		lmp->is = icatalloc(lmp->is, rmp->is);
		if (lmp->is == NULL)
		  goto done;
	      }
	    else
	      lmp->is[0] = '\0';
	  }
	  break;
	default:
	  if (t < END)
	    {
	      /* "cannot happen" */
	      goto done;
	    }
	  else if (t == '\0')
	    {
	      /* not on *my* shift */
	      goto done;
	    }
	  else if (t >= CSET
		   )
	    {
	      /* easy enough */
	      resetmust(mp);
	    }
	  else
	    {
	      /* plain character */
	      resetmust(mp);
	      mp->is[0] = mp->left[0] = mp->right[0] = t;
	      mp->is[1] = mp->left[1] = mp->right[1] = '\0';
	      mp->in = enlist(mp->in, mp->is, (size_t)1);
	      if (mp->in == NULL)
		goto done;
	    }
	  break;
	}
#ifdef DEBUG
      fprintf(stderr, " node: %d:", ri);
      prtok(dfa->tokens[ri]);
      fprintf(stderr, "\n  in:");
      for (i = 0; mp->in[i]; ++i)
	fprintf(stderr, " \"%s\"", mp->in[i]);
      fprintf(stderr, "\n  is: \"%s\"\n", mp->is);
      fprintf(stderr, "  left: \"%s\"\n", mp->left);
      fprintf(stderr, "  right: \"%s\"\n", mp->right);
#endif
      ++mp;
    }
 done:
  if (strlen(result))
    {
      dm = (struct dfamust *) malloc(sizeof (struct dfamust));
      dm->exact = exact;
      dm->must = malloc(strlen(result) + 1);
      strcpy(dm->must, result);
      dm->next = dfa->musts;
      dfa->musts = dm;
    }
  mp = musts;
  for (i = 0; i <= dfa->tindex; ++i)
    {
      freelist(mp[i].in);
      ifree((char *) mp[i].in);
      ifree(mp[i].left);
      ifree(mp[i].right);
      ifree(mp[i].is);
    }
  free((char *) mp);
}
/* vim:set shiftwidth=2: */
#endif


#if 0
main()
{
	struct dfa *dfa1, *dfa2;
	int i=0;
	int s=0;
	int backref = 0;
	int state = 0;

	
	char *p  = "(abc)|(Apache)";
	char *c  = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n<HTML>\n <HEAD>\n  <TITLE>Test Page for the SSL/TLS-aware Apache Installation on Web Site</TITLE>\n  <STYLE TYPE=\"text/css\">\n  H1 {\n";
	//char *c1 = "ardfafafafafdfpass wh00";
	char *c2 = "tazpassforward wh00txdfadddefakyfafafaa1ii3ccfzirstposklastpbadosnulla(blefollowsbeanmisterbean";
	char *c3 = "fpassssss w\n00tor\rwardakknibc1xxaiciloveweiweibutsheisgoodandagoodgirlilikiboys";
	//char *p = "n.*\x61|kk\x62\x68ni";
	//char *p = "ab|b.*a|(a|b)*a";
	//char *p = "(a|b)*abb";
	//char *p = "((abc|def))!9|(bad)!34|(ni.*k)!1000|adadfadf!1001|jyy.*k!1002|h.*h!90|dfdf!87|3.z!98|zzz!7|dddd!6|(df.*(a|b))!765|a.*zzz!3|a.n!2";
	//char *p = "a.*33!1|d!2|xxb!3|dfafaf!4|n.*z!5|(ab|b.*a)!6";
	//char *p = "a\\(b\\x6c!98|ddddd\\x78xai!23|w\\n00!1|kkkkfdf!2";
	//char *p = "(\\x61 \\x62!|\\x63\\x64\\x65\\x66!k!1212ad\\x67\\x68\\x693\\x70f\\x71\\x72)!9a!988|k[^x\t\v]bos!1";
	//char *p = "wh!6";
	//char *p = "a.*33!1|d!2|xxb!3";
	//char *p1 = "^USER +w0rm!1|.forward!2|.rhosts!3|^CWD +~root!4|^CEL [^\n]{100}!5|PASS ddd@!6|pass -iss@iss!7|pass wh00t!8|[rR][eE][tT][rR].*passwd!9|pass -cklaus!10|pass -saint!11|pass -satan!12|.%20.!13|^SITE +EXEC!14| --use-compress-program !15|^PASS *\n!16|^530 +(Login|User)!17|STOR.{1,}1MB!18|RETR.{1,}1MB!19|CWD.{1,}/ !20|CWD  !21|MKD  !22|MKD.{1,}/ !23|^CWD [^\n]*?...!24|~.*\[!25|~.*\{!26|^STAT [^\n]{100}!27|[rR][eE][tT][rR].{1,}file_id.diz!28|[uU][sS][eE][rR].* ftp!29|^SITE [^\n]{100}!30";
	//char *p2 = "USER +w0rm!1|forward!2|rhosts!3|CWD +~root!4|CEL {100}!5|PASS ddd@!6|pass -iss@iss!7|pass wh00t!80|[rR][eE][tT][rR].*passwd!9|pass -cklaus!10|pass -saint!11|pass -satan!12|%20.!13|SITE +EXEC!14| --use-compress-program !15|PASS *!16|530 +(Login|User)!17|STOR.{1,}1MB!18|RETR.{1,}1MB!19|CWD.{1,}/ !20|CWD  !21|MKD  !22|MKD.{1,}/ !23|CWD .*?...!24|~.*!25|~.*!26|STAT .{100}!27|[rR][eE][tT][rR].{1,}file_id.diz!28|[uU][sS][eE][rR].* ftp!29|SITE .{100}!30|%p!31|SITE +CHOWN .{100}!32|CMD .{100}!33|[rR][nN][fF][rR].* ././!34|MODE +[ABSC]{1}!35|PWD!36|SYST!37|CWD +~!38|CWD .* ~!39|USER .{100}!40|STAT +.*!41|STAT +!42|CWD .* ....!43|SITE +NEWER!44|SITE +CPWD !45|CWD 0!46|SITE +NEWER .{100}!47|SITE +ZIPCHK !48|authorized_keys!49|[rR][eE][tT][rR].*shadow!50|RMDIR !51|SITE +EXEC .*?%.*?%!52|REST !53|DELE !54|RMD !55|[lL][iI][sS][tT].{1}...{1}..!56|[cC][wW][dD].{1}C!57|USER .*?%.*?%!58|PASS .*?%.*?%!59|LIST +-W +[0123456789]+!60|MKDIR .*?%.*?%!61|RENAME .*?%.*?%!62|LIST .{100,}!63|SITE +CHMOD .{100}!64|STOR .{100}!65|XCWD .{100}!66|XMKD .{100}!67|NLST .{100}!68|RNTO .{100}!69|STOU .{100}!70|APPE .{100}!70|RETR .{100}!71|MDTM [0123456789]+[-+][0123456789]!72|ALLO .{100}!73|MDTM .{100}!74|RETR .*?%.*?%!75|RNFR .{100}!76";
	

char *p1 = "(\\/\\*.*\\*\\/)|"
"([^-_0-9a-zA-Z]javascript:)|"
"([^-_0-9a-zA-Z]ecmascript[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]jscript:)|"
"([^-_0-9a-zA-Z]vbscript:)|"
"([^-_0-9a-zA-Z]alert\\s*\\()|"
"([^-_0-9a-zA-Z]eval\\s*\\()|"
"([^-_0-9a-zA-Z]fromCharCode\\s*\\()|"
"([^-_0-9a-zA-Z]expression\\s*\\()|"
"([^-_0-9a-zA-Z]url\\s*\\()|"
"([^-_0-9a-zA-Z]innerHTML[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]document\\.body[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]document\\.cookie[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]document\\.location[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]document\\.write)|"
"([^-_0-9a-zA-Z]document\\.URL[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]document\\.referrer[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]window\\.location[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]document\\.forms[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]document\\.attachEvent)|"
"([^-_0-9a-zA-Z]document\\.create)|"
"([^-_0-9a-zA-Z]document\\.execCommand)|"
"([^-_0-9a-zA-Z]window\\.attachEvent)|"
"([^-_0-9a-zA-Z]window\\.navigate)|"
"([^-_0-9a-zA-Z]document\\.open)|"
"([^-_0-9a-zA-Z]window\\.open)|"
"([^-_0-9a-zA-Z]window\\.execScript)|"
"([^-_0-9a-zA-Z]window\\.setInterval)|"
"([^-_0-9a-zA-Z]window\\.setTimeout)|"
"([^-_0-9a-zA-Z]xmlHttp[^-_0-9a-zA-Z])|"
"(\\s+http-equiv\\s*=)|"
"(\\s+style\\s*=)|"
"(\\s+type\\s*=)|"
"(\\s+dynsrc\\s*=)|"
"([^-_0-9a-zA-Z]jsessionid[^=-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]phpsessid[^=-_0-9a-zA-Z])|"
"(<applet[^-_0-9a-zA-Z])|"
"(<div[^-_0-9a-zA-Z])|"
"(<embed[^-_0-9a-zA-Z])|"
"(<iframe[^-_0-9a-zA-Z])|"
"(<img[^-_0-9a-zA-Z])|"
"(<meta[^-_0-9a-zA-Z])|"
"(<object[^-_0-9a-zA-Z])|"
"(<script[^-_0-9a-zA-Z])|"
"(<about[^-_0-9a-zA-Z])|"
"(<activex[^-_0-9a-zA-Z])|"
"(<chrome[^-_0-9a-zA-Z])|"
"(<textarea[^-_0-9a-zA-Z])|"
"(<body[^-_0-9a-zA-Z])|"
"(<xml[^-_0-9a-zA-Z])|"
"(<blink[^-_0-9a-zA-Z])|"
"(<link[^-_0-9a-zA-Z])|"
"(<frame[^-_0-9a-zA-Z])|"
"(<frameset[^-_0-9a-zA-Z])|"
"(<ilayer[^-_0-9a-zA-Z])|"
"(<layer[^-_0-9a-zA-Z])|"
"(<bgsound[^-_0-9a-zA-Z])|"
"(<title[^-_0-9a-zA-Z])|"
"(<base[^-_0-9a-zA-Z])|"
"(\\s+onabort\\s*=)|"
"(\\s+onactivate\\s*=)|"
"(\\s+onafterprint\\s*=)|"
"(\\s+onafterupdate\\s*=)|"
"(\\s+onbeforeactivate\\s*=)|"
"(\\s+onbeforecopy\\s*=)|"
"(\\s+onbeforecut\\s*=)|"
"(\\s+onbeforedeactivate\\s*=)|"
"(\\s+onbeforeeditfocus\\s*=)|"
"(\\s+onbeforepaste\\s*=)|"
"(\\s+onbeforeprint\\s*=)|"
"(\\s+onbeforeunload\\s*=)|"
"(\\s+onbeforeupdate\\s*=)|"
"(\\s+onblur\\s*=)|"
"(\\s+onbounce\\s*=)|"
"(\\s+oncellchange\\s*=)|"
"(\\s+onchange\\s*=)|"
"(\\s+onclick\\s*=)|"
"(\\s+oncontextmenu\\s*=)|"
"(\\s+oncontrolselect\\s*=)|"
"(\\s+oncopy\\s*=)|"
"(\\s+oncut\\s*=)|"
"(\\s+ondataavailable\\s*=)|"
"(\\s+ondatasetchanged\\s*=)|"
"(\\s+ondatasetcomplete\\s*=)|"
"(\\s+ondblclick\\s*=)|"
"(\\s+ondeactivate\\s*=)|"
"(\\s+ondrag\\s*=)|"
"(\\s+ondragend\\s*=)|"
"(\\s+ondragenter\\s*=)|"
"(\\s+ondragleave\\s*=)|"
"(\\s+ondragover\\s*=)|"
"(\\s+ondragstart\\s*=)|"
"(\\s+ondrop\\s*=)|"
"(\\s+onerror\\s*=)|"
"(\\s+onerrorupdate\\s*=)|"
"(\\s+onfilterchange\\s*=)|"
"(\\s+onfinish\\s*=)|"
"(\\s+onfocus\\s*=)|"
"(\\s+onfocusin\\s*=)|"
"(\\s+onfocusout\\s*=)|"
"(\\s+onhelp\\s*=)|"
"(\\s+onkeydown\\s*=)|"
"(\\s+onkeypress\\s*=)|"
"(\\s+onkeyup\\s*=)|"
"(\\s+onlayoutcomplete\\s*=)|"
"(\\s+onload\\s*=)|"
"(\\s+onlosecapture\\s*=)|"
"(\\s+onmousedown\\s*=)|"
"(\\s+onmouseenter\\s*=)|"
"(\\s+onmouseleave\\s*=)|"
"(\\s+onmousemove\\s*=)|"
"(\\s+onmouseout\\s*=)|"
"(\\s+onmouseover\\s*=)|"
"(\\s+onmouseup\\s*=)|"
"(\\s+onmousewheel\\s*=)|"
"(\\s+onmove\\s*=)|"
"(\\s+onmoveend\\s*=)|"
"(\\s+onmovestart\\s*=)|"
"(\\s+onpaste\\s*=)|"
"(\\s+onpropertychange\\s*=)|"
"(\\s+onreadystatechange\\s*=)|"
"(\\s+onreset\\s*=)|"
"(\\s+onresize\\s*=)|"
"(\\s+onresizeend\\s*=)|"
"(\\s+onresizestart\\s*=)|"
"(\\s+onrowenter\\s*=)|"
"(\\s+onrowexit\\s*=)|"
"(\\s+onrowsdelete\\s*=)|"
"(\\s+onrowsinserted\\s*=)|"
"(\\s+onscroll\\s*=)|"
"(\\s+onselect\\s*=)|"
"(\\s+onselectionchange\\s*=)|"
"(\\s+onselectstart\\s*=)|"
"(\\s+onstart\\s*=)|"
"(\\s+onstop\\s*=)|"
"(\\s+onsubmit\\s*=)|"
"(\\s+onunload\\s*=)|"
"(\\s+FSCommand\\s*=)|"
"(Apache)" ;


char *p2=
"((\\'\\s*--|\\'\\s*#))|"
"(\\'\\s*;)|"
"((\\s|\\'|\\))like\\s*\\(*\\'.*\\%.*\\')|"
"([^-_0-9a-zA-Z](system_user|substring\\s*\\(.*\\)|unicode\\s*\\(.*\\)|len\\s*\\(.*\\)|is_srvrolemember\\s*\\(.*\\)|db_name\\s*\\(.*\\)|right\\s*\\(.*\\)|left\\s*\\(.*\\)|count\\s*\\(.*\\)))|"
"(\\'\\s*\\+\\s*\\')|"
"(\\\"\\s*\\,\\s*\\\")|"
"(\\\"\\s*\\&\\s*\\\")|"
"(\\'\\s*\\|\\|\\s*\\')|"
"(\\\"\\s*\\+\\s*\\\")|"
"((\\'\\s*|[0-9]\\s+)(or|and|having)[^\\&-_0-9a-zA-Z][^\\&]*(>|<|=|[^-_0-9a-zA-Z]between[^-_0-9a-zA-Z].*[^-_0-9a-zA-Z]and[^-_0-9a-zA-Z]|[^-_0-9a-zA-Z]like[^-_0-9a-zA-Z]|[^-_0-9a-zA-Z]in[^-_0-9a-zA-Z]|[^-_0-9a-zA-Z]is\\s+null[^-_0-9a-zA-Z]|[^-_0-9a-zA-Z]is\\s+not\\s+null[^-_0-9a-zA-Z]))|"
"([^-_0-9a-zA-Z]delete\\s+from[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]drop\\s+database[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]drop\\s+table[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]drop\\s+column[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]drop\\s+procedure[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]create\\s+table[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]update[^-_0-9a-zA-Z].*[^-_0-9a-zA-Z]set[^-_0-9a-zA-Z].*=)|"
"([^-_0-9a-zA-Z]insert\\s+into[^-_0-9a-zA-Z].*[^-_0-9a-zA-Z]values[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]select[^-_0-9a-zA-Z].*[^-_0-9a-zA-Z]from[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]bulk\\s+insert[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]union\\s+select[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]alter\\s+table[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]create\\s+database[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]create\\s+procedure[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]alter\\s+database[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]alter\\s+procedure[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]create\\s+index[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]create\\s+view[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]alter\\s+index[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]alter\\s+view[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]drop\\s+index[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]drop\\s+view[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]grant[^-_0-9a-zA-Z].*[^-_0-9a-zA-Z]on[^-_0-9a-zA-Z].*[^-_0-9a-zA-Z]to[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]exec.*[^-_0-9a-zA-Z]xp_)|"
"([^-_0-9a-zA-Z]exec.*[^-_0-9a-zA-Z]sp_)|"
"([^-_0-9a-zA-Z]exec\\s*\\()|"
"([^-_0-9a-zA-Z]openquery[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z](openrowset|opendatasource)[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]msdasql[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]sqloledb[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]sysobjects[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]syscolumns[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]syslogins[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]sysxlogins[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]waitfor\\s+delay[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z](char\\s*\\(.*\\)|DATABASE\\s*\\(.*\\)|USER\\s*\\(.*\\)|SYSTEM_USER\\s*\\(.*\\)|SESSION_USER\\s*\\(.*\\)|CURRENT_USER\\s*\\(.*\\)))|"
"([^-_0-9a-zA-Z]into[^-_0-9a-zA-Z].*[^-_0-9a-zA-Z]outfile[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]load_file[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]load[^-_0-9a-zA-Z].*[^-_0-9a-zA-Z]data[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]MySQL\\.user[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]MySQL\\.host[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]MySQL\\.db[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]MSysACEs[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]MSysObjects[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]MSysQueries[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]MSysRelationships[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]SYS\\.USER_OBJECTS[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]SYS\\.TAB[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]SYS\\.USER_TABLES[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]SYS\\.USER_VIEWS[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]SYS\\.ALL_TABLES[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]SYS\\.USER_TAB_COLUMNS[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]SYS\\.USER_CATALOG[^-_0-9a-zA-Z])|"
"(\\;\\s*cmd=)|"
"(\\;\\s*chr\\(.*\\))|"
"(\\;\\s*fwrite\\(.*\\))|"
"(\\;\\s*fopen\\(.*\\))|"
"(\\;\\s*system\\(.*\\))|"
"(\\;\\s*echr\\(.*\\))|"
"(\\;\\s*passthru\\(.*\\))|"
"(\\;\\s*popen\\(.*\\))|"
"(\\;\\s*proc_open\\(.*\\))|"
"(\\;\\s*shell_exec\\(.*\\))|"
"(\\;\\s*exec\\(.*\\))|"
"(\\;\\s*proc_nice\\(.*\\))|"
"(\\;\\s*proc_terminate\\(.*\\))|"
"(\\;\\s*proc_get_status\\(.*\\))|"
"(\\;\\s*proc_close\\(.*\\))|"
"(\\;\\s*pfsockopen\\(.*\\))|"
"(\\;\\s*leak\\(.*\\))|"
"(\\;\\s*apache_child_terminate\\(.*\\))|"
"(\\;\\s*posix_kill\\(.*\\))|"
"(\\;\\s*posix_mkfifo\\(.*\\))|"
"(\\;\\s*posix_setpgid\\(.*\\))|"
"(\\;\\s*posix_setsid\\(.*\\))|"
"(\\;\\s*posix_setuid\\(.*\\))|"
"(\\;\\s*phpinfo\\(.*\\))|";
/*
"(<!--\\s*#\\s*exec[^-_0-9a-zA-Z])|"
"(<!--\\s*#\\s*config[^-_0-9a-zA-Z])|"
"(<!--\\s*#\\s*echo[^-_0-9a-zA-Z])|"
"(<!--\\s*#\\s*include[^-_0-9a-zA-Z])|"
"(<!--\\s*#\\s*printenv[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]objectClass[^-_0-9a-zA-Z])|"
"([^-_0-9a-zA-Z]objectCategory[^-_0-9a-zA-Z])|"
"(\\)\\(\\|)|"
"(\\)\\(!)" ;
*/









	dfa1 = (struct dfa *)malloc(sizeof(struct dfa));
 	dfasyntax (dfa1, RE_SYNTAX_POSIX_EGREP , 0, '\n');
 	dfacomp(dfa1, p1, strlen(p1), 0);
	i = dfaexec(dfa1, c, strlen(c), &backref, &state);
	printf("i1 = %d\n", i);

	dfa2 = (struct dfa *)malloc(sizeof(struct dfa));
 	dfasyntax (dfa2, RE_SYNTAX_POSIX_EGREP , 0, '\n');
 	dfacomp(dfa2, p2, strlen(p2), 0);
	i = dfaexec(dfa2, c, strlen(c), &backref, &state);
	printf("i1 = %d\n", i);
	/*
	printf("s = %d\n", s);
	i = dfaexec(dfa, c2, strlen(c2), 0, 1);
	printf("i2 = %d\n", i);
	printf("s = %d\n", s);
	i = dfaexec(dfa, c3, strlen(c3), 0, 1);
	printf("i3 = %d\n", i);
	printf("s = %d\n", s);
	*/

	//i = dfa_buf_exec(d, c1, strlen(c1), 0);
	//printf("i = %d\n", i);

}
#endif
