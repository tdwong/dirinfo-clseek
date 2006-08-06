//
// CLsync.c
//
//	Command-line sync utility
//
// Description:
//   main module
//
// Revision History:
//   T. David Wong		03-11-2005    Original Author
//   T. David Wong		05-16-2005    utilized newly implemented mystropt library in -c, -C, -x, -X options
//   T. David Wong		05-16-2005    implemented -k (keep target entry) option
//   T. David Wong		05-16-2005    implemented -y (skip empty source directory) option
//   T. David Wong		05-17-2005    fixed bugs in copyFile
//   T. David Wong		05-17-2005    enhanced copyFile to ensure permission and timestamp are carried over
//   T. David Wong		05-17-2005    *first* real exercise
//   T. David Wong		05-20-2005    fixed problem in updating read-only entry
//   T. David Wong		05-31-2005    implemented -u (update only newer entry) option
//   T. David Wong		05-31-2005    handled source file entry properly
//   T. David Wong		06-15-2005    added prompt for deletion if -f option not specified
//   T. David Wong		07-03-2005    added -p (same as -k which will be deprecated)
//   T. David Wong		07-03-2005    added checking for -p & -f conflict
//   T. David Wong		11-18-2005    added report on deletion error
//
// TODO:
//	1. better handle dry-run and debug messages
//	2. consolidate sync_src_file() with syncCallback()
//	*. MBCS- (wide-)character support
//

/*
 *	DEFINES FOR PROGRAM
 */
#define _COPYRIGHT_	"(c) 2005 Tzunghsing David <wong>"
#define _DESCRIPTION_	"Command-line sync utility"
#define _PROGRAMNAME_	"CLsync"	/* program name */
#define _PROGRAMVERSION_	"0.20c"	/* program version */

#define	_use_str_opt_lib_	1	/* use mystropt library */

/*
 *	Includes
 */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#if	defined(unix) || defined(__STDC__)
#include <dirent.h>
#include <unistd.h>
#include <utime.h>
#else
#include <io.h>			// read, write
#include <sys/utime.h>	// utime
#endif

#include "mygetopt.h"
#include "mystropt.h"
#include "dirinfo.h"
#include "regex.h"

/*
 *	Defines
 */
/* directory entity type */
#define ENTITY_FILE       0x04
#define ENTITY_DIRECTORY  0x0200
#define ENTITY_OTHER      0x0400
//
#define	TREE_SRC	0x01
#define	TREE_DST	0x02

/*
 */
#ifndef	_MSC_VER
typedef	unsigned int	boolean;
#endif	/* _MSC_VER */

/*
 *	Global variables
 */
#ifdef	_MSC_VER
unsigned int  gMajorVersion = 0;
unsigned int  gMinorVersion = 0;
#endif	/* _MSC_VER */
static char gPathDelimiter =
#ifdef	_MSC_VER
	'\\';	/* may be changed later if env("SHLVL") is set */
#else
	'/';	/* non-MSDOS */
#endif	/* _MSC_VER */
/**/
unsigned char *gSrcDirectory = NULL;
unsigned char *gDstDirectory = NULL;
unsigned int   gSrcDirLen = 0, gDstDirLen = 0;
/**/
unsigned int   gInclusiveNameCriteria = 0;
unsigned char *gNameBegins = NULL;
unsigned char *gNameEnds = NULL;
#ifndef	_use_str_opt_lib_
unsigned char **gPathContainsTbl = NULL;
unsigned int    gPathContainsCnt = 0;
unsigned char **gPathExcludesTbl = NULL;
unsigned int    gPathExcludesCnt = 0;
#else
void        *gPathContainsTbl = NULL;
void        *gPathExcludesTbl = NULL;
#endif	/* _use_str_opt_lib_ */
/**/
unsigned int gRecursive = 1;
unsigned int gDryRun = 0;
unsigned int gKeepTargetEntry = 0;
unsigned int gUpdateOnlyNewerEntry = 0;
	#define	ONLY_EMPTY_ENTRY	0x01
	#define	ENTIRE_EMPTY_TREE	0x02	/* directory tree contains no file */
unsigned int gSkipEmptySource = 0;
unsigned int gForceOverwrite = 0;
unsigned int gQuietMode = 0;
unsigned int gDebug = 0;
unsigned int gIgnoreCase = 0;

/*
 *	Local variables
 */
/* statistics */
unsigned int sFileCheckCount = 0;
unsigned int sFileCreateCount = 0;
unsigned int sFileDeleteCount = 0;
unsigned int sDirCheckCount = 0;
unsigned int sDirCreateCount = 0;
unsigned int sDirDeleteCount = 0;
unsigned int sFileDeletionFailCount = 0;
unsigned int sDirDeletionFailCount = 0;

/*
 *	Local functions
 */
/* parameter handlers
 */
static void	show_Setting(int optptr, int argc, char **argv);
static int  parse_Parameter(char *progname, int argc, char **argv);
static void cacheContainsFromFile(char *name);
static void cacheExcludesFromFile(char *name);
#ifndef	_use_str_opt_lib_
static void listContainedPatterns(void);
static void listExcludedPatterns(void);
#endif	/* _use_str_opt_lib_ */
static boolean matchContainedPatterns(char *path, char *name);
static boolean matchExcludedPatterns(char *path, char *name);
/* directory tree traverser
 */
static void	sync_src_file(char *srcfile, char *dstdir);
static void traverse_sync_trees(char *srcdir, char *dstdir);
static void reverse_check_tree(char *srcdir, char *dstdir);
static boolean isEmptyDirectory(const char *dirpath);
/* callback functions
 */
static void syncCallback(const char *name, const char *path, struct stat *statp, void *opaquep);
static void reverseCallback(const char *name, const char *path, struct stat *statp, void *opaquep);
static void rmdirCallback(const char *name, const char *path, struct stat *statp, void *opaquep);
/* sync. support functions
 */
static boolean confirmDeletion(char *src, int overwrite);
static boolean matchFiles(char *src, char *dst);
static int  copyFile(char *src, char *dst, int overwrite);
static int  makeDir(char *dstdir, int overwrite);
static int  deleteFile(char *src);
static int  deleteDir(char *src, int recursive);
static int  deleteEntry(char *src, int overwrite);


/* program options
 */
static void version(char *progname)
{
	fprintf(stderr, "%s %s, %s\n", _PROGRAMNAME_, _PROGRAMVERSION_, _COPYRIGHT_);
	fprintf(stderr, "built at %s %s\n", __DATE__, __TIME__);
	return;
}
static void usage(char *progname, int detail)
{
	fprintf(stderr, "%s - %s\n", _PROGRAMNAME_, _DESCRIPTION_);
	fprintf(stderr, "Usage: %s [options] <source directory> <destination directory>\n", progname);
	fprintf(stderr, "  -h               help\n");
	fprintf(stderr, "  -v               version\n");
	fprintf(stderr, "  -t               trim (no recursive)\n");
	fprintf(stderr, "  -b<pattern>      match name begins with <pattern>\n");
	fprintf(stderr, "  -e<pattern>      match name ends with <pattern>\n");
	fprintf(stderr, "  -c<pattern>      match path contains <pattern>\n");
	fprintf(stderr, "  -C<file>         match path contains pattern from <file>\n");
	fprintf(stderr, "     -b, -e, -c, -C are matched inclusively (match ALL to identify the entry)\n");
	fprintf(stderr, "  -x<pattern>      match path without <pattern>\n");
	fprintf(stderr, "  -X<file>         match path without pattern from <file>\n");
	fprintf(stderr, "     -x, -X are matched exclusively (ANY match excludes the entry)\n");
	fprintf(stderr, "  -n               don't actually run any commands; just print them\n");
	fprintf(stderr, "  -f               force overwrite/delete\n");
	fprintf(stderr, "  -p               keep target entry (no deletion)\n");
	fprintf(stderr, "  -u               update only newer entry (newer timestamp)\n");
	fprintf(stderr, "  -s               skip empty source directory entry which contains nothing\n");
	fprintf(stderr, "  -S               skip empty source directory tree which contains no file\n");
	fprintf(stderr, "  -i               ignore case distinctions\n");
	fprintf(stderr, "  -q               quiet mode\n");
	fprintf(stderr, "  -d#              debug level\n");
	if (detail)
	{
	fprintf(stderr, "\nExample:\n");
	fprintf(stderr, "  %s -v\n", progname);
	fprintf(stderr, "  %s -r -X.tmp %%TEMP%%\n", progname);
	}
	return;
}

/* main program
 */
int 
main(int argc, char **argv)
{
	char *progname = argv[0];
	int  nextarg;

	/* initialization */
#ifdef	_MSC_VER
	if (getenv("SHLVL") != NULL) { gPathDelimiter = '/'; }
#endif	/* _MSC_VER */

	/* parse parameters */
	if ((nextarg = parse_Parameter(progname, argc, argv)) <= 0) {
		return 0;
	}

	/* we need at least two more arguments */
	//-debug- fprintf(stderr, "argc=%d, nextarg=%d\n", argc, nextarg);
	if ((argc - nextarg) == 0) {
		usage(progname, 0);
		return 0;
	}
	else if ((argc - nextarg) == 1) {
		fprintf(stderr, "%s: Error: missing destination directory\n", progname);
		// usage(progname, 0);
		return 0;
	}
	gSrcDirectory = argv[nextarg];
	gDstDirectory = argv[nextarg+1];
	gSrcDirLen = strlen(gSrcDirectory);
	gDstDirLen = strlen(gDstDirectory);

#ifdef	_MSC_VER
	/* retrieve OS version */
	if (GetWindowsVersion(&gMajorVersion, &gMinorVersion) != TRUE) {
		fprintf(stderr, "%s: Fatal Error: Failed retrieve OS version\n", progname);
		return 0;
	}
#endif	/* _MSC_VER */

	/*-special-case-
	 *	2005-05-17 As I am so used to add the /L option at the end of the XCOPY command, this needs to be handled too!!!
	 */
	if ((argc - nextarg) > 2) {
		if ((strcmp(argv[nextarg+2], "/L") == 0) || (strcmp(argv[nextarg+2], "-L") == 0))
		{
			gDryRun++;
		}
	}
	/*-end-of-special-case*/

	if (gDebug & 0x10)
	{
		printf("=== arg ==============\n");
		show_Setting((int)nextarg, argc, argv);
		printf("======================\n");
	}
	//-dbg- printf("argc=%d nextarg=%d argv[nextarg]=<%s>\n", argc, nextarg, argv[nextarg]);

	/* clean up "contains" and "excludes" tables */
	str_ResolveConflicts(gPathContainsTbl, gPathExcludesTbl);
	if (gDebug & 0x08)
	{
		str_ListTable("resolved contains:", gPathContainsTbl);
		str_ListTable("resolved excludes:", gPathExcludesTbl);
	}
	//-dbg- printf("argc=%d nextarg=%d argv[nextarg]=<%s>\n", argc, nextarg, argv[nextarg]);

	/* tidy given paths */
	condense_path(gSrcDirectory);
	condense_path(gDstDirectory);

	/* convert to lowercase if necessary */
	if (gIgnoreCase)
	{
		strlwr(gSrcDirectory);
		strlwr(gDstDirectory);
	}

	/* 2005-05-31 handle the special case where gSrcDirectory is actually a file
	 */
	if (IsFile(gSrcDirectory) && IsDirectory(gDstDirectory))
	{
		sync_src_file(gSrcDirectory, gDstDirectory);
	}
	else
	{
		/* ***
		 */
		traverse_sync_trees(gSrcDirectory, gDstDirectory);
		/*
		 *** */
	}

	/* clean up allocated tables
	 */
#ifndef	_use_str_opt_lib_
	if (gPathContainsTbl) free(gPathContainsTbl);
	if (gPathExcludesTbl) free(gPathExcludesTbl);
#else
	if (gPathContainsTbl) str_FreeTable(gPathContainsTbl);
	if (gPathExcludesTbl) str_FreeTable(gPathExcludesTbl);
#endif	/* _use_str_opt_lib_ */

	/* statistics report */
	if (gDebug) {
		fprintf(stdout, "[summary] %d file(s) %scopied.\n",    sFileCreateCount, gDryRun ? "would be " : "");
		fprintf(stdout, "[summary] %d file(s) %sdeleted.\n",   sFileDeleteCount, gDryRun ? "would be " : "");
		fprintf(stdout, "[summary] %d file(s) checked.\n",     sFileCheckCount);
		fprintf(stdout, "[summary] %d directory %screated.\n", sDirCreateCount,  gDryRun ? "would be " : "");
		fprintf(stdout, "[summary] %d directory %sdeleted.\n", sDirDeleteCount,  gDryRun ? "would be " : "");
		fprintf(stdout, "[summary] %d directory checked.\n",   sDirCheckCount);
		if (!gDryRun) {
			if (sFileDeletionFailCount) fprintf(stdout, "[warning] %d file deletion attempt(s) failed.\n", sFileDeletionFailCount);
			if (sDirDeletionFailCount)  fprintf(stdout, "[warning] %d directory deletion attempt(s) failed.\n", sDirDeletionFailCount);
		}
	}

	return 0;
}

/* ----------------------
 *	CALL BACK FUNCTIONS
 * ----------------------
 */
//
static void syncCallback(const char *filename, const char *fullpath, struct stat *statp, void *opaquep)
{
	char dstPath[PATH_MAX];

	/* Implemented function logic:
	 *
	 *	- (src) match criteria
	 *	- (src)
	 *		[DIR]
	 *			if (!exists dstDIR) mkdir(dstDIR)
	 *			if ( exists dstDIR) reverseCheck(dstDIR)
	 *		[FILE]
	 *			if (!exists dstFILE) copyFile(srcFILE, dstFILE)
	 *			if ( exists dstFILE) matchFiles(srcFILE, dstFILE)
	 *				if (!same or newer?(srcFILE)) copyFile(srcFILE, dstFILE)
	 */

	//
	//	check for excluded pattern(s)
	//
	if (matchExcludedPatterns((char*)fullpath, (char*)filename)) {
		/* do nothing if this entry is excluded
		 */
		if (gDebug & 0x04) fprintf(stderr, "(syncCB-Excluded) %s [rel:%s]\n", fullpath, &fullpath[gSrcDirLen]);
		return;
	}
	//
	//	check for inclusive pattern(s)
	//
	if (gInclusiveNameCriteria &&
		!matchContainedPatterns((char*)fullpath, (char*)filename))
	{
		/* inclusive pattern(s) specified
		 */
		if (gDebug & 0x04) fprintf(stderr, "(syncCB-Contained-NO-match) %s [rel:%s]\n", fullpath, &fullpath[gSrcDirLen]);
		return;
	}

	/* destination path */
	strcpy(dstPath, gDstDirectory);
	strcat(dstPath, &fullpath[gSrcDirLen]);

	//-debug-
	if (gDebug & 0x02) fprintf(stderr, "(syncCB%s) %s [rel:%s] [dstPath=%s]\n", gInclusiveNameCriteria ? "-Contained" : "", fullpath, &fullpath[gSrcDirLen], dstPath);

	/* identify entity type */
	if (S_ISDIR(statp->st_mode))
	{
		sDirCheckCount++;

		if (gSkipEmptySource && isEmptyDirectory(fullpath)) {
			/* empty source directory will NOT be copied */
			return;
		}

		/* srcDIR */
		if (IsFile(dstPath) && deleteEntry(dstPath, gForceOverwrite) < 0) {
			// cannot remove existing entry
			return;
		}
		/* check dstDIR */
		if ((_access(dstPath, 0)) == -1) {
			makeDir(dstPath, gForceOverwrite);
		}
		else {
			/* dstDIR exists */
			reverse_check_tree((char*)fullpath, dstPath);
		}
	}
	else if (S_ISREG(statp->st_mode))
	{
		/* srcFILE */
		sFileCheckCount++;
		if (IsDirectory(dstPath) && deleteEntry(dstPath, gForceOverwrite) < 0) {
			// cannot remove existing entry
			return;
		}
		/* check dstFILE */
		if ((_access(dstPath, 0)) == -1) {
			copyFile((char*)fullpath, dstPath, gForceOverwrite);
		}
		else {
			/* dstFILE exists */
			if (matchFiles((char*)fullpath, dstPath) == FALSE) {
				/* files do not match */
					/* this dry-run message covers two operations */
				fprintf(stderr, "[%s-update] copy file %s into %s\n", gDryRun?"would":"sync", fullpath, dstPath);
				if (gDryRun == 0) {
					// 2005-05-20 deletion will be taken care of in copyFile
					// deleteFile(dstPath);
					int bytes = copyFile((char*)fullpath, dstPath, gForceOverwrite);
				}
			}
			else {
				/* no-op for identical files */
				if (gDebug & 0x20) fprintf(stderr, "[matched files] %s & %s\n", fullpath, dstPath);
			}
		}
	}
	return;
}

//
static void reverseCallback(const char *dstfile, const char *dstpath, struct stat *statp, void *opaquep)
{
	int srcdir = (int)((matchCriteria_t*)opaquep)->iblock;
	char srcPath[256];

	/* identify entity type */
	// if (S_ISDIR(statp->st_mode))      { attr = ENTITY_DIRECTORY; }
	// else if (S_ISREG(statp->st_mode)) { attr = ENTITY_FILE; }
	// // else                              { attr = ENTITY_OTHER; }

	// call back function
	/* process logic:
	 *
	 *	- (dst) match criteria
	 *	- (dst)
	 *		[DIR]
	 *			if (recursive)
	 *				if (!exists srcDIR) deleteDIR(dstDIR, recursive=1)
	 *				if ( exists srcDIR) no-op
	 *			else no-op // when non-recursive request
	 *		[FILE]
	 *			if (!exists srcFILE) deleteFile(dstFILE)
	 *			if ( exists srcFILE) no-op
	 */

	//
	//	check for excluded pattern(s)
	//
	if (matchExcludedPatterns((char*)dstpath, (char*)dstfile)) {
		/* do nothing if this entry is excluded
		 */
		if (gDebug & 0x04) fprintf(stderr, "(reverseCB-Excluded) %s [rel:%s]\n", dstpath, &dstpath[gDstDirLen]);
		return;
	}
	//
	//	check for inclusive pattern(s)
	//
	if (gInclusiveNameCriteria &&
		!matchContainedPatterns((char*)dstpath, (char*)dstfile))
	{
		/* inclusive pattern(s) specified
		 */
		if (gDebug & 0x04) fprintf(stderr, "(reverseCB-Contained-NO-match) %s [rel:%s]\n", dstpath, &dstpath[gDstDirLen]);
		return;
	}

	/* source path */
	strcpy(srcPath, (char*)gSrcDirectory);
	strcat(srcPath, &dstpath[gDstDirLen]);

	//-debug-
	if (gDebug & 0x02) fprintf(stderr, "(reverseCB%s) %s [rel:%s] [srcdir:%s srcPath:%s]\n", gInclusiveNameCriteria ? "-Contained" : "", dstpath, &dstpath[gDstDirLen], srcdir, srcPath);

	/* identify entity type */
	if (S_ISDIR(statp->st_mode))
	{
		/* dstDIR */
		sDirCheckCount++;
		/* check srcDIR only when recursive mode */
		if (gRecursive) {
			if ((_access(srcPath, 0)) == -1) {
				/* delete directory doesNOT exist in source directory */
				if (gKeepTargetEntry == 0) { deleteDir((char*)dstpath, 1 /*recursive*/); }
			}
		}
	}
	else if (S_ISREG(statp->st_mode))
	{
		/* dstFILE */
		sFileCheckCount++;
		/* check srcFILE */
		if ((_access(srcPath, 0)) == -1) {
			/* delete file doesNOT exist in source directory */
			if (gKeepTargetEntry == 0) { deleteFile((char*)dstpath); }
		}
	}
	return;
}


/* -------------------------
 *	OTHER SUPPORT FUNCTIONS
 * -------------------------
 */
static void	show_Setting(int optptr, int argc, char **argv)
{
	fprintf(stdout, "--- settings ---\n");
	fprintf(stdout, "recursive mode:        %s\n", gRecursive ? "TRUE" : "FALSE");
	fprintf(stdout, "quiet mode:            %s\n", gQuietMode ? "TRUE" : "FALSE");
	fprintf(stdout, "ignore case:           %s\n", gIgnoreCase ? "TRUE" : "FALSE");
	fprintf(stdout, "force overwrite:       %s\n", gForceOverwrite ? "TRUE" : "FALSE");
	fprintf(stdout, "dry-run mode:          %s\n", gDryRun ? "TRUE" : "FALSE");
	fprintf(stdout, "debug level set to:    %d (0X%0x)\n", gDebug, gDebug);
	fprintf(stdout, "source directory:      %s\n", gSrcDirectory ? gSrcDirectory : "[none]");
	fprintf(stdout, "destination directory: %s\n", gDstDirectory ? gDstDirectory : "[none]");
#ifndef	_use_str_opt_lib_
	listContainedPatterns();
	listExcludedPatterns();
#else
	fprintf(stdout, "gInclusiveNameCriteria= %d\n", gInclusiveNameCriteria);
	fprintf(stdout, "gNameBegins:            %s\n", gNameBegins ? gNameBegins : "[NULL]");
	fprintf(stdout, "gNameEnds:              %s\n", gNameEnds ? gNameEnds : "[NULL]");
	str_ListTable("match pattern(s) set: ", gPathContainsTbl);
	str_ListTable("exclude pattern(s) set: ", gPathExcludesTbl);
#endif	/* _use_str_opt_lib_ */
	fprintf(stdout, "--- end of settings ---\n");
	return;
}

/* Parsing parameters in (argc, argv)
 */
static int parse_Parameter(char *progname, int argc, char **argv)
{
	int optcode;
	char *optptr;
//	extern char *optarg;
//	extern int optind;
	int errflags = 0;

#ifdef	_debug_
	{	int ix;
		printf("parsing parameter...\n");
		for (ix = 0; ix < argc; ix++)
			printf("argv[%d] = %s\n", ix, argv[ix]);
	}
#endif	/* _debug_ */

	/* ***
	 * PARAMETER PARSING
	 */
	optptr = NULL;
	while ((optcode = fds_getopt(&optptr, "?hvtb:e:c:C:x:X:nLfkpsSuiqd:", argc, argv)) != EOF)
	{
		//-dbg- printf("optcode=%c *optptr=%c\n", optcode, *optptr);
		switch (optcode) {
			case '?':
			case 'h':  usage(progname, 0);	return 0;
			case 'v':  version(progname);	return 0;

			/* recursive mode */
			case 't':  gRecursive = 0;	break;

			/* ignore case */
			case 'i':  gIgnoreCase++;	break;

			/* enable dry-run mode */
			case 'L':	/* similar to XCOPY /L */
			case 'n':  gDryRun++;	break;

			/* force overwrite/delete existing entry */
			case 'f':
					if (gKeepTargetEntry) {
						/* conflict exists */
						fprintf(stderr, "Option '%c' conflicts with preserve (keep) option.\n", optcode);
						errflags++;
					}
					else {
						gForceOverwrite++;
					}
					break;

			/* keep target entry */
			case 'k':
			case 'p':
					if (gForceOverwrite) {
						/* conflict exists */
						fprintf(stderr, "Option '%c' conflicts with preserve (keep) option.\n", optcode);
						errflags++;
					}
					else {
						gKeepTargetEntry++;
					}
					break;
			/* update only newer entry */
			case 'u':  gUpdateOnlyNewerEntry++;	break;

			/* skip empty source directory */
			case 's':  gSkipEmptySource = ONLY_EMPTY_ENTRY;	break;

			/* skip empty source directory */
			case 'S':  gSkipEmptySource = ENTIRE_EMPTY_TREE;	break;

			/* disable verbose mode */
			case 'q':  gQuietMode++;	break;

			/* set debug level */
			case 'd':
					gDebug = atoi((char *)optptr);
					break;

			/* matched pattern */
			case 'b':
					if (gNameBegins == NULL) gInclusiveNameCriteria++;
					gNameBegins   = optptr;
					break;
			case 'e':
					if (gNameEnds   == NULL) gInclusiveNameCriteria++;
					gNameEnds     = optptr;
					break;
			case 'c':  /* reallocate the table */
#ifndef	_use_str_opt_lib_
					gPathContainsTbl = (char **) realloc(gPathContainsTbl, (++gPathContainsCnt)*sizeof(char*));
					gPathContainsTbl[(gPathContainsCnt-1)] = optptr;
					gInclusiveNameCriteria++;
#else
					if ((gPathContainsTbl = str_InsertToTable(optptr, gPathContainsTbl, STR_KEEP_LONGER_STRING)) != NULL)
						gInclusiveNameCriteria++;
#endif	/* _use_str_opt_lib_ */
					break;
			/* matched pattern in file */
			case 'C':
					if (!IsFile(optptr)) {
						fprintf(stderr, "Option %c ignored: %s: not a file.\n", optcode, optptr);
						break;
					}
					cacheContainsFromFile(optptr);
					break;

			/* excluded pattern */
			case 'x':  /* reallocate the table */
#ifndef	_use_str_opt_lib_
					gPathExcludesTbl = (char **) realloc(gPathExcludesTbl, (++gPathExcludesCnt)*sizeof(char*));
					gPathExcludesTbl[(gPathExcludesCnt-1)] = optptr;
#else
					gPathExcludesTbl = str_InsertToTable(optptr, gPathExcludesTbl, STR_KEEP_SHORTER_STRING);
#endif	/* _use_str_opt_lib_ */
					break;
			/* excluded pattern in file */
			case 'X':
					if (!IsFile(optptr)) {
						fprintf(stderr, "Option '%c' ignored: %s: not a file.\n", optcode, optptr);
						break;
					}
					cacheExcludesFromFile(optptr);
					break;

			/* special option - option starts with "--" */
			case OPT_SPECIAL:
					if (strcmp(optptr, "help") == 0) {
						version(progname);	usage(progname, 1);
					}
					else if (strcmp(optptr, "version") == 0) version(progname);
					else fprintf(stderr, "Unknown option: --%s\n", optptr);
					return 0;

			case OPT_UNKNOWN:
					fprintf(stderr, "Unknown option: %s\n", optptr);
					errflags++;
					break;
			default:
					fprintf(stderr, "Option '%c' not yet implemented.\n", (char)optcode);
					errflags++;
					break;
		}
	}	/* end-of-while */
	/*
	 *** */

	/* error handling */
	if (errflags) {
		// version(progname);
		return -1;
	}
	return (int) optptr;
}

/*
 * sync_src_file - traverse source directory tree and sync the destination directory tree along the way
 */
static void	sync_src_file(char *srcfile, char *dstdir)
{
	int  srcFullLen;
	char srcPath[PATH_MAX], *srcFilePart;
	char dstPath[PATH_MAX];

	/* extend full source pathname */
	srcFullLen = GetFullPathName(srcfile, sizeof(srcPath), srcPath, &srcFilePart);

	/* destination path */
	strcpy(dstPath, gDstDirectory);
	strcat(dstPath, (srcFilePart-1) /*include path delimiter*/);

	/* --- SAME operation logic as defined syncCallback ---
	 */
	/* check dstFILE */
	if ((_access(dstPath, 0)) == -1) {
		copyFile((char*)srcPath, dstPath, gForceOverwrite);
	}
	else {
		/* dstFILE exists */
		if (matchFiles((char*)srcPath, dstPath) == FALSE) {
			/* files do not match */
				/* this dry-run message covers two operations */
			fprintf(stderr, "[%s-update] copy file %s into %s\n", gDryRun?"would":"sync", srcPath, dstPath);
			if (gDryRun == 0) {
				int bytes = copyFile((char*)srcPath, dstPath, gForceOverwrite);
			}
		}
		else {
			/* no-op for identical files */
			if (gDebug & 0x20) fprintf(stderr, "[matched files] %s & %s\n", srcPath, dstPath);
		}
	}
	return;
}

/*
 * traverse_sync_trees - traverse source directory tree and sync the destination directory tree along the way
 */
static void traverse_sync_trees(char *srcdir, char *dstdir)
{
	dirInfo_t dibuf = { 0 };	/* summary information */
	matchCriteria_t mcbuf = { 0 };

	//
	// sanity check
	//
	/* cannot handle NULL entries */
	if (srcdir == NULL || dstdir == NULL) {
		fprintf(stderr, "Error: NULL path found (src=%s, dst=%s)\n", srcdir, dstdir);
		return;
	}
	/* check source directory
	 *	source directory must exist and is a directory.
	 */
	if ((_access(srcdir, 0)) == -1) {
		fprintf(stderr, "Error: Directory not found - %s\n", srcdir);
		return;
	}
	else if (!IsDirectory(srcdir)) {
		fprintf(stderr, "Error: Not a directory - %s\n", srcdir);
		return;
	}
	/* check destination directory
	 *	- if destination directory does not exist, create it.
	 *	- if destination directory          exist, it must be a directory.
	 */
	if ((_access(dstdir, 0)) == -1) {
		//* non-exist entry will be created.
		if (makeDir(dstdir, 0) != 0) {
			fprintf(stderr, "Error: Cannot create directory - %s\n", dstdir);
			return;
		}
	}
	else if (!IsDirectory(dstdir)) {
		fprintf(stderr, "Error: Not a directory - %s\n", dstdir);
		return;
	}
	else {
		/* check first-level entries in destination directory */
		reverse_check_tree(srcdir, dstdir);
	}
	//-debug-
	if (gDebug & 0x40) fprintf(stderr, "traverse_sync_trees (src=%s, dst=%s)\n", srcdir, dstdir);

	//
	// traverse the source directory tree
	//
	mcbuf.proc = (FILEPROC) syncCallback;	/* callback routine */
	//
	dirinfo_Find(srcdir, &dibuf, &mcbuf, (int) gRecursive);

	/* closing report */
	if (gDebug & 0x40) {
		dirinfo_Report(&dibuf, "source");
	}
	return;
}

/*
 * reverse_check_tree - check destination directory against source directory (always NON-recursive)
 */
static void reverse_check_tree(char *srcdir, char *dstdir)
{
	matchCriteria_t mcbuf = { 0 };

	//-debug- fprintf(stderr, "reverse_check_tree: srcdir=%s, dstdir=%s\n", srcdir, dstdir);

	// sanity check
	//
	/* cannot handle NULL entries */
	if (srcdir == NULL || dstdir == NULL) {
		fprintf(stderr, "Error: NULL path found (src=%s, dst=%s)\n", srcdir, dstdir);
		return;
	}
	if (!IsDirectory(srcdir)) {
		fprintf(stderr, "Warning: Not a directory - %s\n", srcdir);
		return;
	}
	if (!IsDirectory(dstdir)) {
		fprintf(stderr, "Warning: Not a directory - %s\n", dstdir);
		return;
	}

	//
	// check the destination directory tree (NO recursive)
	//
	mcbuf.proc = (FILEPROC) reverseCallback;	/* callback routine */
	mcbuf.iblock = (void*)srcdir;	/* opaque information */
	//
	dirinfo_Find(dstdir, NULL, &mcbuf, 0 /*non-recursive*/);
	return;
}

/*
 * isEmptyDirectory - check if an empty directory
 */
static boolean isEmptyDirectory(const char *dirpath)
{
	dirInfo_t dibuf = { 0 };	/* summary information */

	/* NULL entry is considered empty directory */
	if (dirpath == NULL) { return TRUE; }

	/* file entry is NOT an empty directory */
	if (!IsDirectory(dirpath)) { return FALSE; }

	// perform directory find
	dirinfo_Find(dirpath, &dibuf, NULL, 0 /*non-recursive*/);

	if (gSkipEmptySource == ONLY_EMPTY_ENTRY)
		return ((dibuf.num_of_directories + dibuf.num_of_files + dibuf.num_of_others) == 0);
	else if (gSkipEmptySource == ENTIRE_EMPTY_TREE)
		return ((dibuf.num_of_files + dibuf.num_of_others) == 0);
	else
		return FALSE;
}

/*
 * cacheContainsFromFile - read and append each line to global contains table (also update global count)
 */
static void cacheContainsFromFile(char *name)
{
	FILE *fp;
	char lbuf[80];

	/* open file and read each line... */
	if ((fp = fopen(name, "rt")) == NULL) return;
	while (fscanf(fp, "%s", lbuf) != EOF)
	{
#ifndef	_use_str_opt_lib_
		gPathContainsTbl = (char **) realloc(gPathContainsTbl, (++gPathContainsCnt)*sizeof(char*));
			/** TODO: strdup() will introduce a memory leak **/
		gPathContainsTbl[(gPathContainsCnt-1)] = strdup(lbuf);
		++gInclusiveNameCriteria;
#else
		if ((gPathContainsTbl = str_InsertToTable(lbuf, gPathContainsTbl, STR_KEEP_LONGER_STRING)) != NULL)
			++gInclusiveNameCriteria;
#endif	/* _use_str_opt_lib_ */
	}
	fclose(fp);
}

/*
 * cacheExcludesFromFile - read and append each line to global excludes table
 */
static void cacheExcludesFromFile(char *name)
{
	FILE *fp;
	char lbuf[80];

	/* open file and read each line... */
	if ((fp = fopen(name, "rt")) == NULL) return;
	while (fscanf(fp, "%s", lbuf) != EOF)
	{
#ifndef	_use_str_opt_lib_
		gPathExcludesTbl = (char **) realloc(gPathExcludesTbl, (++gPathExcludesCnt)*sizeof(char*));
			/** TODO: strdup() will introduce a memory leak **/
		gPathExcludesTbl[(gPathExcludesCnt-1)] = strdup(lbuf);
#else
		if ((gPathExcludesTbl = str_InsertToTable(lbuf, gPathExcludesTbl, STR_KEEP_SHORTER_STRING)) == NULL)
			break;
#endif	/* _use_str_opt_lib_ */
	}
	fclose(fp);
}

#ifndef	_use_str_opt_lib_
static void listContainedPatterns(void)
{
	char **tbl = gPathContainsTbl;
	unsigned int  ix;

	fprintf(stdout, "gInclusiveNameCriteria= %d\n", gInclusiveNameCriteria);
	fprintf(stdout, "gNameBegins:            %s\n", gNameBegins ? gNameBegins : "[NULL]");
	fprintf(stdout, "gNameEnds:              %s\n", gNameEnds ? gNameEnds : "[NULL]");
	fprintf(stdout, "%d match patther(s) set: ", gPathContainsCnt);
	for (ix = 0; ix < gPathContainsCnt; ix++) {
		fprintf(stdout, "%s ", tbl[ix]);
	}
	fprintf(stdout, "\n");
	return;
}
static void listExcludedPatterns(void)
{
	char **tbl = gPathExcludesTbl;
	unsigned int  ix;

	fprintf(stdout, "%d exclude patther(s) set: ", gPathExcludesCnt);
	for (ix = 0; ix < gPathExcludesCnt; ix++) {
		fprintf(stdout, "%s ", tbl[ix]);
	}
	fprintf(stdout, "\n");
	return;
}
#endif	/* _use_str_opt_lib_ */

/*
 * matchContainedPatterns - check if ALL "contain" patterns in the provided path
 */
static boolean matchContainedPatterns(char *path, char *name)
{
	unsigned int match = 0;
	int (*cmpfunc)();
	//
#ifndef	_use_str_opt_lib_
	char **tbl = gPathContainsTbl;
	unsigned int  ix;
#endif	/* _use_str_opt_lib_ */

	/* select appropriate compare function */
	cmpfunc = gIgnoreCase ? strnicmp : strncmp;

	//
	// match patterns in filename
	//
	if (gNameBegins) {
		if (cmpfunc(name, gNameBegins, strlen(gNameBegins)) == 0) {
			if (gDebug & 0x40) fprintf(stderr, "*** %s begins [%s]\n", path, gNameBegins);
			match++;
		}
	}
	if (gNameEnds) {
		int flen = strlen(name);
		int slen = strlen(gNameEnds);
		if (cmpfunc(&name[flen-slen], gNameEnds, slen) == 0) {
			if (gDebug & 0x40) fprintf(stderr, "*** %s ends [%s]\n", path, gNameEnds);
			match++;
		}
	}

	//
	// match patterns in pathname
	//
#ifndef	_use_str_opt_lib_
	for (ix = 0; ix < (int) gPathContainsCnt; ix++) {
		/* match matched substring */
		if (strstr(path, tbl[ix]) != NULL) { match++; }
	}
#else
	if (str_FindInTable(path, gPathContainsTbl, gIgnoreCase) != NULL) { match++; }
#endif	/* _use_str_opt_lib_ */
	return (gInclusiveNameCriteria == match);
}

/*
 * matchExcludedPatterns - check if ANY "exclude" patterns in the provided path
 */
static boolean matchExcludedPatterns(char *path, char *name)
{
#ifndef	_use_str_opt_lib_
	char **tbl = gPathExcludesTbl;
	unsigned int  ix;
	//
	for (ix = 0; ix < gPathExcludesCnt; ix++) {
		/* match excluded substring */
		if (strstr(path, tbl[ix]) != NULL) { return TRUE; }
	}
	return FALSE;
#else
	return (str_FindInTable(path, gPathExcludesTbl, gIgnoreCase) != NULL) ? TRUE : FALSE;
#endif	/* _use_str_opt_lib_ */
}

/*
 * directory entry handling functions
 */
/*
 * matchFiles - compare two provided files
 *
 * Return:
 *		FALSE - files do NOT match
 *		TRUE  - files do match
 */
static boolean matchFiles(char *src, char *dst)
{
	struct _stat fsrc, fdst;

	/* sanity check */
	if (!IsFile(src)) {
		fprintf(stderr, "Error: Not a file - %s\n", src);
		return FALSE;
	}
	if (!IsFile(dst)) {
		fprintf(stderr, "Error: Not a file - %s\n", dst);
		return FALSE;
	}

	/* retrieve file status */
	if (_stat(src, &fsrc) != 0) {
		fprintf(stderr, "Fatal Error: File not found - %s\n", src);
		return FALSE;
	}
	if (_stat(dst, &fdst) != 0) {
		fprintf(stderr, "Fatal Error: File not found - %s\n", dst);
		return FALSE;
	}

	/* compare following attributes
	 * 1. file size
	 * 2. timestamp (last modification time?)
	 * 3. [option] CRC32 value
	 */
	if ((fsrc.st_size == fdst.st_size) && (fsrc.st_mtime == fdst.st_mtime)) {
		if (gDebug & 0x20) fprintf(stderr, "matched: %s & %s (size=%d; mtime=%d)\n", src, dst, fsrc.st_size, fsrc.st_mtime);
		return TRUE;
	}

	/* if we want to update ONLY NEWER entry, older timestamp (i.e.
	 * smaller mtime) will trigger a match to prevent the update.
	 */
	if (gUpdateOnlyNewerEntry) {
		if (fsrc.st_mtime < fdst.st_mtime) {
			if (gDebug & 0x20) fprintf(stderr, "older src: %s [mtime=%d] & newer dst: %s [mtime=%d]\n", src, fsrc.st_mtime, dst, fdst.st_mtime);
			return TRUE;
		}
	}

	/* files may not match */
	return FALSE;
}

/*
 * copyFile - copy src file into dst file
 *
 * Return:
 *		 0 - file copied successfully
 *		~0 - error
 */
#define	BLKSIZE	32*1024		/* 32-K buffer */
static char sCopyBuffer[BLKSIZE]; 
static int copyFile(char *src, char *dst, int overwrite)
{
	char        *bp;
	int          srcfd, dstfd;
	unsigned int bytesread;
	unsigned int byteswritten = 0;
	unsigned int totalbytes = 0;
	struct _stat    fsrc;
	struct _utimbuf tdst;

	/* make sure src file exists */
	if (!IsFile(src)) {
		fprintf(stderr, "Error: Not a file - %s\n", src);
		return -1;
	}

	/* check if dst exists */
	if (deleteEntry(dst, overwrite) < 0) {
		/* cannot overwrite existing entry */
		return -1;
	}

	/* do the copying... */
	fprintf(stderr, "[%s-create] copy file %s into %s\n", gDryRun?"would":"sync", src, dst);
	if (gDryRun) {
		sFileCreateCount++;
		return 0;
	}

	/* retrieve file status */
	if (_stat(src, &fsrc) != 0) { return -1; }
	/* open */
	srcfd = _open(src, _O_BINARY|_O_RDONLY);
    dstfd = _open(dst, _O_BINARY|_O_CREAT|_O_WRONLY);
	if ((srcfd < 0) || (dstfd < 0))
	{
		if (gDebug & 0x08) {
			if (srcfd < 0) fprintf(stderr, "%s: cannot open file for reading (errno=%d/0x%X)\n", src, errno, errno);
			if (dstfd < 0) fprintf(stderr, "%s: cannot open file for writing (errno=%d/0x%X)\n", dst, errno, errno);
		}
		return -1;
	}

	/*
	 * Copying...
	 */
	while ((bytesread = _read(srcfd, sCopyBuffer, BLKSIZE)) > 0)
	{
		if (gDebug & 0x08) fprintf(stderr, "bytesread=%d\n", bytesread);
		for (bp = sCopyBuffer; bytesread > 0; /*no-op*/)
		{
			while ((byteswritten = _write(dstfd, bp, bytesread)) > 0)
			{
				if (gDebug & 0x08) fprintf(stderr, "byteswritten=%d\n", byteswritten);
				if (byteswritten <= 0) break;
				/* more to copy? */
				totalbytes += byteswritten;
				bytesread -= byteswritten;
				bp += byteswritten;
			}
			if (byteswritten == -1) { break; }	/* real error on dstfd */
		}
	}
	if (gDebug & 0x08) fprintf(stderr, "totalbytes=%d\n", totalbytes);
	/* close */
	_close(srcfd);
	_close(dstfd);

	// TODO: if (fsrc.st_size != totalbytes) { return -1; }
	if (byteswritten == -1) {
		/* real error on dstfd */
		deleteFile(dst);
		return -1;
	}

	/* properly set up permission and timestamp
	 */
	if (gDebug & 0x08) fprintf(stderr, "%s: access permission and timestamp updated\n", dst);
	/* NOTE:
	 *	Set timestamp first as the access permission may be read-only and prohibit any change
	 */
		// timestamp - access & modification times
	tdst.actime = fsrc.st_atime;
	tdst.modtime = fsrc.st_mtime;
	/* enable write permission */
	chmod(dst, (S_IREAD|S_IWRITE|S_IEXEC));
	utime(dst, &tdst);
		// access permission
	chmod(dst, (fsrc.st_mode & (S_IREAD|S_IWRITE|S_IEXEC)));

	sFileCreateCount++;
	return totalbytes;
}

/*
 * makeDir - make directory
 *
 * Return:
 *		 0 - file copied successfully
 *		~0 - error
 */
static int makeDir(char *dstdir, int overwrite)
{
	/* check if dstdir exists */
	if (deleteEntry(dstdir, overwrite) < 0) {
		/* cannot overwrite existing entry */
		return -1;
	}

	/* make directory... */
	fprintf(stderr, "[%s-create] mkdir %s\n", gDryRun?"would":"sync", dstdir); sDirCreateCount++;
	if (gDryRun == 0) {
		if (mkdir(dstdir, 0777) != 0) {
			fprintf(stderr, "Fatal Error: Cannot create directory - %s\n", dstdir);
			return -1;
		}
	}
	return 0;
}

/*
 * deleteFile - delete the specified file
 *
 * Return:
 *		 0 - file copied successfully
 *		~0 - error
 */
static int deleteFile(char *src)
{
	if (!IsFile(src)) {
		fprintf(stderr, "Error: Not a file - %s\n", src);
		return -1;
	}
	fprintf(stderr, "[%s-remove] delete file %s\n", gDryRun?"would":"sync", src); sFileDeleteCount++;
	if (gDryRun == 0) {
		if (confirmDeletion(src, gForceOverwrite) == FALSE) { return -1; }
		// unlink() should do the same thing
		if (remove(src) != 0) {
			fprintf(stderr, "%s: cannot delete destination file\n", src);
			sFileDeletionFailCount++;
			return -1;
		}
	}
	return 0;
}

/* callback used in deleteDir()
 */
static void rmdirCallback(const char *filename, const char *rdpath, struct stat *statp, void *opaquep)
{
	int recursive = (int)((matchCriteria_t*)opaquep)->iblock;

	/* identify entity type */
	if (S_ISDIR(statp->st_mode))
	{
		//-debug- fprintf(stderr, "_dir_: %s\n", rdpath);
		deleteDir((char*)rdpath, recursive);
	}
	else if (S_ISREG(statp->st_mode))
	{
		//-debug- fprintf(stderr, "_file_: %s\n", rdpath);
		deleteFile((char*)rdpath);
	}
	else {
		//-debug- fprintf(stderr, "_unknown_: %s\n", rdpath);
		deleteFile((char*)rdpath);
	}
	return;
}
/*
 * deleteDir - delete the specified directory (recursively if requested)
 *
 * Return:
 *		 0 - file copied successfully
 *		~0 - error
 */
static int deleteDir(char *src, int recursive)
{
	matchCriteria_t mcbuf = { 0 };

	if (!IsDirectory(src)) {
		fprintf(stderr, "Error: Not a directory - %s\n", src);
		return -1;
	}

	/* prepare to delete the directory [tree] */
	fprintf(stderr, "[%s-remove] %sdelete %s\n", gDryRun?"would":"sync", recursive?"recursively ":"", src); sDirDeleteCount++;
	mcbuf.proc = (FILEPROC) rmdirCallback;
	mcbuf.iblock = (void*)recursive;
	/* search directory info */
	dirinfo_Find(src, NULL, &mcbuf, (int) recursive);

	/* delete myself */
	if (gDryRun == 0) {
		if (confirmDeletion(src, gForceOverwrite) == FALSE) { return -1; }
		if (rmdir(src) != 0) {
			fprintf(stderr, "Fatal Error: Cannot delete directory - %s\n", src);
			sDirDeletionFailCount++;
			return -1;
		}
	}
	return 0;
}

/*
 * deleteEntry - attempt to delete existing entry if allowed
 *
 * Return:
 *		 0 - non-exist entry or successfully removed
 *		-1 - error
 */
static int deleteEntry(char *src, int overwrite)
{
	/* is src a valid path */
	if ((_access(src, 0)) != -1) {
		if (!overwrite) {
			/* cannot overwrite existing entry */
			return -1;
		}
		else {
			/* make sure we have proper permission */
			chmod(src, (S_IREAD|S_IWRITE|S_IEXEC));
			if (IsDirectory(src)) { return deleteDir(src, 1); }
			else if (IsFile(src)) { return deleteFile(src); }
			// else { /* handle other entry type */; }
		}
	}
	/* non-exist entry is okay */
	return 0;
}

/*
 * confirmDeletion - confirm deletion operation
 *
 * Return:
 *		FALSE - don't delete
 *		TRUE  - okay to delete
 */
static boolean confirmDeletion(char *src, int overwrite)
{
	char ans[2];
	if (overwrite) {
		/* make sure we have proper permission */
		chmod(src, (S_IREAD|S_IWRITE|S_IEXEC));
		return TRUE;
	}
	fprintf(stderr, "Warning: Do you really want to delete %s? ", src);
	ans[0] = fgetc(stdin);
	if (ans[0] == 'Y' || ans[0] == 'y') {
		/* explicit positive answer is expected */
		return TRUE;
	}
	return FALSE;
}

