

#ifndef STMT_H
#define STMT_H

/*****************************************************/
/* #define a macro to append a stmt to the stmt list */
/*****************************************************/
#define append_stmt(_stmt,_next_append_pt) \
	{								\
	   if (eng->parser->append_pt == NULL) {			\
	      parser_warning (eng, "statement not reached");		\
	   }								\
	   else {							\
	      *(eng->parser->append_pt) = (_stmt);			\
	   }								\
	   eng->parser->append_pt = (_next_append_pt);			\
	}

/*************************************************************/
/* statment type constants.  they begin with an "nel_S_" */
/* for statement, and are called "nel_S_tokens.          */
/*************************************************************/
typedef enum nel_S_TOKEN {
        nel_S_NULL = 0,
        nel_S_DEC,
        nel_S_EXPR,
        nel_S_RETURN,
        nel_S_BRANCH,
        nel_S_WHILE,
        nel_S_FOR,
        nel_S_TARGET,
		nel_S_GOTO,
		nel_S_BREAK,
		nel_S_CONTINUE,
        nel_S_MAX
} nel_S_token;

/*********************************************************/
/* structures which form statement lists.                */
/* nel_S_tokens are the discriminator for the union. */
/*********************************************************/
typedef union nel_STMT {

        /*********************/
        /* generic structure */
        /*********************/
        struct {
                nel_S_token type;
                char *filename;			/* keep source file name and	*/
                int line;				/* line # for error messages.	*/
				int visited;	//andded by zhangbin, 2006-6-20, to avoid looping infinetely
				
				//added by zhangbin, 2006-6-13
				int compiled;
				int addr;
				int patch_addr;
				//end
				
                union nel_STMT *next;		/* for use in free list		*/
        }
        gen;

        /*******************************/
        /* declaration of a new symbol */
        /*******************************/
        struct {
                nel_S_token type;
                char *filename;
                int line;
				int visited;	//andded by zhangbin, 2006-6-20, to avoid looping infinetely				
				
				//added by zhangbin, 2006-6-13
				int compiled;
				int addr;
				int patch_addr;
				//end
				
                struct nel_SYMBOL *symbol;
                union nel_STMT *next;
				
        }
        dec;

        /************************/
        /* expression statement */
        /************************/
        struct {
                nel_S_token type;
                char *filename;
                int line;
				int visited;	//andded by zhangbin, 2006-6-20, to avoid looping infinetely
				
				//added by zhangbin, 2006-6-13
				int compiled;
				int addr;
				int patch_addr;
				//end
				
                union nel_EXPR *expr;
                union nel_STMT *next;
				
        }
        expr;

        /********************/
        /* return statement */
        /********************/
        struct {
                nel_S_token type;
                char *filename;
                int line;
				int visited;	//andded by zhangbin, 2006-6-20, to avoid looping infinetely
                				
				//added by zhangbin, 2006-6-13
				int compiled;
				int addr;
				int patch_addr;
				//end
				
				union nel_EXPR *retval;
                union nel_STMT *next;
				
        }
        ret;

        /**********************/
        /* conditional branch */
        /**********************/
        struct {
                nel_S_token type;
                char *filename;
                int line;
				int visited;	//andded by zhangbin, 2006-6-20, to avoid looping infinetely
				
				//added by zhangbin, 2006-6-13
				int compiled;
				int addr;
				int patch_addr;
				//end
				
				int level;
                union nel_EXPR *cond;
                union nel_STMT *true_branch;
                union nel_STMT *false_branch;

                union nel_STMT *next;	/* wyong, 2004.10.17 */
				
        }
        branch;

        struct {
                nel_S_token type;
                char *filename;
                int line;
				int visited;	//andded by zhangbin, 2006-6-20, to avoid looping infinetely
				
				//added by zhangbin, 2006-6-13
				int compiled;
				int addr;
				int patch_addr;
				//end
				
				int level;
                union nel_EXPR *cond;
                union nel_STMT *body;
                union nel_STMT *next;
				//added by zhangbin, 2006-6-15
				union nel_STMT *break_target;
				union nel_STMT *continue_target;
				int break_patch;
				int continue_addr;
				//end
				
        }
        while_stmt;


        struct {
                nel_S_token type;
                char *filename;
                int line;
				int visited;	//andded by zhangbin, 2006-6-20, to avoid looping infinetely
				
				//added by zhangbin, 2006-6-13
				int compiled;
				int addr;
				int patch_addr;
				//end
				
				int level;
                union nel_EXPR *init;
                union nel_EXPR *cond;
                union nel_EXPR *inc;
                union nel_STMT *body;
                union nel_STMT *next;
				//added by zhangbin, 2006-6-15
				union nel_STMT *break_target;
				union nel_STMT *continue_target;
				int break_patch;
				int continue_addr;
				int continue_patch;
				//end
				
        }
        for_stmt;

        /****************************************************/
        /* a target contains scoping information.  a target */
        /* appears at the beginning and end of each block,  */
        /* as well as at a goto target, and any other place */
        /* that is needed for the parser's convenience.     */
        /****************************************************/
        struct {
                nel_S_token type;
                char *filename;
                int line;
				int visited;	//andded by zhangbin, 2006-6-20, to avoid looping infinetely
				
				//added by zhangbin, 2006-6-13
				int compiled;
				int addr;
				int patch_addr;
				//end
				
                int level;
                union nel_STMT *next;
				
        }
        target;

		//added by zhangbin, 2006-5-20
		struct
		{
			nel_S_token type;
			char *filename;
			int line;
			int visited;
				
			//added by zhangbin, 2006-6-13
			int compiled;
			int addr;
			int patch_addr;
			//end
			
			int level;	//add by zhangbin, 2006-11-22
			union nel_STMT *goto_target;
			union nel_STMT *next;
		}goto_stmt;
		//end

} nel_stmt;



/***************************************************************/
/* linked lists are made of the following structures so that   */
/* we may find the start of all the chunks of memory allocated */
/* to hold nel_stmt structures at garbage collection time. */
/***************************************************************/
typedef struct nel_STMT_CHUNK
{
        unsigned_int size;
        nel_stmt *start;
        struct nel_STMT_CHUNK *next;
}
nel_stmt_chunk;



/*********************************************************/
/* declarations for the nel_stmt structure allocator */
/*********************************************************/
extern unsigned_int nel_stmts_max;
extern nel_lock_type nel_stmts_lock;



/**********************************************************************/
/* when the flag nel_const_addresses is nonzero, the one may take */
/* the address of a constant this is useful when calling Fortran      */
/* routines, where arguments are passed by reference.                 */
/**********************************************************************/
extern unsigned_int nel_const_addresses;



/*********************************************************************/
/* declarations for expressions for (which point to the symbols for) */
/* the integer values 0 and 1, which are boolean return values.      */
/*********************************************************************/
extern struct nel_SYMBOL *nel_zero_symbol;
extern struct nel_SYMBOL *nel_one_symbol;


/*************************************************************/
/* declarations for the routines defined in "stmt.c" */
/*************************************************************/
extern char *nel_S_name (register nel_S_token);
extern union nel_STMT *nel_stmt_alloc (struct nel_eng *, register nel_S_token, register char *, int,...);
extern void emit_stmt(FILE *, union nel_STMT *, int indent);
extern void nel_stmt_dealloc (register union nel_STMT *);
extern void nel_print_stmt (FILE *, register union nel_STMT *, int);
extern int nel_stmt_link(nel_stmt * tail , nel_stmt *head);
int nel_stmt_dup( struct nel_eng *, nel_stmt *, nel_stmt **, nel_stmt **);
int ast_to_intp( struct nel_eng *, nel_stmt *, nel_stmt **, nel_stmt **);
int evt_stmt_update_dollar(struct nel_eng *, nel_stmt *, int, int );

//added by zhangbin, 2006-7-19
void stmt_dealloc(struct nel_eng *eng);
//end
#endif
