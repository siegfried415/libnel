#ifndef __SWBM_EXEC_H
#define __SWBM_EXEC_H



#define RET_FOUND_MASK		0x01 /* must be set as 0x01, otherwise the return value, which would be '0' when no patterns were found, won't be '1' when patterns were found. */
#define REAR_TREE_SCANED_MASK	0x02


#define NEWEND endp+=bad[*endp]; hp = endp; t=mtries[*hp]; 


#endif
