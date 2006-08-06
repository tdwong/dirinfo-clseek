//
// mystropt.h
//
// Module Name:
//   Library functions
//
// Description:
//
// Revision History:
//   T. David Wong		06-26-2003    Original Author
//
//

#ifndef	_MYSTROPT_H_
#define	_MYSTROPT_H_

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#ifdef	_MSC_VER
#include <winsock.h>	/* TRUE, FALSE, boolean */
#endif

/* */
#ifndef	_MSC_VER
typedef unsigned int	boolean;
#endif

/*
 * public definitions
 */
#define STR_KEEP_LONGER_STRING	1
#define STR_KEEP_SHORTER_STRING	2

/* public functions
 */
extern void str_FreeTable(void *ptr);
extern void *str_InsertToTable(char *str, void *ptr, int kflag);
extern char *str_FindInTable(char *str, void *ptr, int icase);
extern void str_ResolveConflicts(void *cptr, void *eptr);
extern void str_ListTable(char *desc, void *ptr);

#ifdef	__cplusplus
}
#endif

#endif	/* _MYSTROPT_H_ */

