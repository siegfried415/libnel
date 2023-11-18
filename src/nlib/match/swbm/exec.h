/*
 * swbm_exec.h
 * $Id: swbm_exec.h,v 1.2 2006/03/17 05:50:16 wyong Exp $
 */
 
#ifndef __SWBM_EXEC_H
#define __SWBM_EXEC_H


//#include "stream.h"

#define RET_FOUND_MASK		0x01 /* must be set as 0x01, otherwise the return value, which would be '0' when no patterns were found, won't be '1' when patterns were found. */
#define REAR_TREE_SCANED_MASK	0x02


#define NEWEND endp+=bad[*endp]; hp = endp; t=mtries[*hp]; 


#endif
