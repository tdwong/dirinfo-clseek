//
// mygetopt.h
//
// Module Name:
//   Library functions
//
// Description:
//
// Revision History:
//   T. David Wong		06-26-2003    Original Author
//   T. David Wong		03-30-2012    Compiled on Mac OS/X
//   T. David Wong		06-09-2017    Exported strlwr and strupr for non-Windows system
//

#ifndef	_MYGETOPT_H_
#define	_MYGETOPT_H_

#ifdef	__cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <stdlib.h>
#ifdef	_MSC_VER
#include <winsock.h>	/* TRUE, FALSE, boolean */
	//https://sourceforge.net/p/predef/wiki/Compilers/
	#if	(_MSC_VER >= 1800)
	// mygetopt.c(405) : warning C4996: 'GetVersionExA': was declared deprecated
    //	C:\Program Files\Windows Kits\8.1\include\um\sysinfoapi.h(433) : see declaration of 'GetVersionExA'
	#endif
#else
	// 2012-03-30
#define	stricmp		strcasecmp
#define	strnicmp	strncasecmp
#undef	TRUE
#define	TRUE	1
#define	FALSE	(!TRUE)
#endif

/* */
#ifndef	_MSC_VER
typedef unsigned int	boolean;
#endif

/*
 * public definitions
 */
	/* fdsoutput */
#define	OUT_CRITIAL	1
#define	OUT_WARN	3
#define	OUT_INFO	5
#define	OUT_NOISE	9
	/* fds_getopt */
#define	OPT_NONOPT		'\002'
#define	OPT_UNKNOWN		'\003'
#define	OPT_SPECIAL		':'


/* public functions
 */
	/* fdsoutput */
extern int fdsout(const int level, const char * format, ...);
extern int fdsSetLevel(int level);
extern int fdsGetLevel(void);
	/* fds_getopt */
extern int fds_getopt(char **optarg, char *optstr, int argc, char **argv);
	/* bufdump */
extern void BufDump(char *buf, unsigned int size, unsigned int startaddr, FILE *ofd);
	/* Get windows's version */
#ifdef	_MSC_VER
extern boolean GetWindowsVersion(unsigned int *major, unsigned int *minor);
#endif
#ifndef	_WIN32
extern char *strlwr(char *str);
extern char *strupr(char *str);
#endif


#ifdef	__cplusplus
}
#endif

#endif	/* _MYGETOPT_H_ */

