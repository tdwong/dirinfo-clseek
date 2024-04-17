//
// CLSeek.c
//
//	Command-line seek utility
//
// Description:
//   main module
//
// Revision History:
//   T. David Wong		04-19-2003    Original Author
//   T. David Wong		04-28-2003    ready for my personal use
//   T. David Wong		05-05-2003    handled -x (name w/o pattern) option correctly
//   T. David Wong		05-06-2003    settled with -q (quiet) and -d (debug) options
//   T. David Wong		05-07-2003    enabled regexp matching
//   T. David Wong		05-20-2003    added -C (compound commad) option
//   T. David Wong		05-21-2003    removed -r option
//   T. David Wong		05-22-2003    implemented -M (path regexp) option.  made -r same as -R option
//   T. David Wong		05-23-2003    implemented -t (time constraint) option.
//   T. David Wong		06-26-2003    incorporated fds_getopt changes
//   T. David Wong		10-27-2003    handled pre-set parameters in environment variable "$CLSEEKOPT"
//   T. David Wong		11-26-2003    fixed problem in handling "C:/" as root directory
//   T. David Wong		11-30-2003    made -r the option for recursion & reserved -R for future use
//   T. David Wong		12-24-2003    merged -C (compound command) into -E (execute command)
//   T. David Wong		12-24-2003    implemented -C (path contains) and -X (path excludes) options
//   T. David Wong		01-05-2004    implemented -= (name equals) option
//   T. David Wong		01-11-2004    modified ignoreCase - true for Unix & false for Windows/DOS
//   T. David Wong		02-07-2004    implemented -l (limit entires) option
//   T. David Wong		02-13-2004    implemented multiple -c & -x options
//   T. David Wong		02-25-2004    excluded filename from matching -C and -X options
//   T. David Wong		03-24-2004    added a special case: "clseek -ir foo.c" searches foo.c in current tree
//   T. David Wong		03-28-2004    added "%:r" into compound command
//   T. David Wong		03-29-2004    implemented multiple -C & -X options
//   T. David Wong		04-16-2004    implemented -j options
//   T. David Wong		06-23-2004    fixed -s option (now supports +, -, =, +=, -=, <NIL>)
//   T. David Wong		09-02-2004    fixed -= option so that both length and string content are compared
//   T. David Wong		10-11-2004    made -t use "-" (time within) as default
//   T. David Wong		12-23-2004    added -D (target directory) option
//   T. David Wong		01-17-2005    added icon to to executable (change was actually made in clseek.nmak)
//   T. David Wong		03-22-2005    used extened functioality in dirinfo module
//   T. David Wong		03-22-2005    removed restriction on # of match/exclude patterns
//   T. David Wong		04-01-2005    removed not-so-useful -v option
//   T. David Wong		04-20-2005    replaced fdsout with fprintf(stderr)
//   T. David Wong		04-28-2005    implemented separate time constraint parsing routine
//   T. David Wong		05-16-2005    implemented separate file size constraint parsing routine
//   T. David Wong		05-18-2005    added -p (access permission) option
//   T. David Wong		05-18-2005    fixed -s=0 when using size parsing routine
//   T. David Wong		08-05-2006    added -w (filename length) option
//   T. David Wong		05-03-2011    enabled debug output in dirinfo when debug leven >= 10
//   T. David Wong		04-19-2012    enabled showing entity details (-a option)
//   T. David Wong		02-06-2014    fixed gcc compiler warnings
//   T. David Wong		06-08-2017    added -ah for symlink only (on non-Windows systems)
//   T. David Wong		06-09-2017    enhanced detail output
//   T. David Wong		06-21-2019    added -0 to terminate with NUL character
//   T. David Wong		10-01-2020    added -y & -z to extend -x functionality
//   T. David Wong		04-07-2022    added -L (limit directory level) option
//   T. David Wong		02-10-2023    made return value consistent (rc>=0 = # of matches)
//   T. David Wong		02-28-2023    added -V (verbose) for Unix-variant && code clean-up
//   T. David Wong		03-15-2023    (v1.71g)
//                                    WIP: attempt to manipulate output line position on the screen
//   T. David Wong		12-05-2023    (v1.8c)
//   T. David Wong		04-15-2023    (v1.9) merged v1.71g back to mainline with #ifdef/#endif
//
// TODO:
//  1. utilize mystropt library for -c, -C, -x, -X options
//	2. enable checking on permission (rwx)
//	3. enable ORing given options
//

#define _COPYRIGHT_	"(c) 2003-2023 Tzunghsing David <wong>"
#define _DESCRIPTION_	"Command-line seek utility"
#define _PROGRAMNAME_	"CLSeek"	/* program name */
#define _PROGRAMVERSION_	"1.9d"	/* program version */
#define	_ENVVARNAME_	"CLSEEKOPT"	/* environment variable name */

#include <stdio.h>
#ifdef	_MSC_VER
#include <wchar.h>
#endif	/* _MSC_VER */
#include <string.h>
#include <stdint.h>		// uint32_t
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>		// isspace
#if	defined(unix) || defined(__STDC__)
#include <dirent.h>
#include <unistd.h>
#include <sys/ioctl.h>	// ioctl for terminal window size
#ifdef  __1_71g__
/*
 * \033[J  - Erase Display: Clears the screen from the cursor to the end of the screen
 * \033[2J - Erase Display: Clears the screen and moves the cursor to the home position (line 0, column 0)
 * \033[K  - Erase Line: Clears all characters from the cursor position to the end of the line (including the character at the cursor position)
 * \033[2K - Erase Line: Clears all characters on the line
 */
#define CLEAR_LINE	"\33[2K"
#define CURSOR_UP	"\033[A"
#endif  // __1_71g__
#endif

#include "mygetopt.h"
#include "dirinfo.h"
#include "regex.h"

	// redefinition after all #include's
	//https://sourceforge.net/p/predef/wiki/Compilers/
	#if	(_MSC_VER >= 1800)
	#define	strdup		_strdup
	#define	strlwr		_strlwr
	#define	stricmp		_stricmp
	#define	strnicmp	_strnicmp
	#endif

/* defined constants for matchCriteria */
#define	WILDCARD          0x0000
#define	STRING_CONTAINS   0x0001
#define STRING_BEGINS     0x0002
#define STRING_ENDS       0x0004
#define	ENTITY_LATER      0x0010
#define	ENTITY_EARLIER    0x0020
#define ENTITY_FILE       0x0100
#define ENTITY_DIRECTORY  0x0200
#define ENTITY_OTHER      0x0400
#define ENTITY_DETAILS    0x0800
#define ENTITY_SYMLINK    0x1000
/**/
#define	PATH_CONTAINS	0x01
#define PATH_EXCLUDES	0x02

/* time constraint */
#define	TIME_WITHIN  0x1
#define	TIME_OVER    0x2

/* size constraint */
#define	ONE_BYTE	1
#define	KBYTE		1024*ONE_BYTE
#define	MBYTE		1024*KBYTE
#define	GBYTE		1024*MBYTE
/**/
#define	SIZE_IN_RANGE 0x00
#define	SIZE_LESS     0x01
#define	SIZE_LESS_EQ  0x02
#define	SIZE_OVER     0x04
#define	SIZE_OVER_EQ  0x08
#define	SIZE_EQUAL    0x10

/*
 */
#ifndef	_MSC_VER
//typedef	unsigned int	boolean;
#else	/* _MSC_VER */
typedef	unsigned int	uint;
#endif	/* _MSC_VER */

/* global variables
 */
static char gPathDelimiter =
#ifdef	_MSC_VER
	'\\';	/* may be changed later if env("SHLVL") is set */
#else
	'/';	/* non-MSDOS */
#endif	/* _MSC_VER */
/**/
uint gEntityAttribute = 0;
uint gLimitEntry = 0;
uint gLimitDirLevel = 0;
uint gTotalMatches = 0;
boolean      gJunkPaths = 0;
boolean      gRecursive = 0;
boolean      gQuietMode = 0;
boolean      gVerboseMode = 0;		// show entry is being examined
#ifdef  __1_71g__
static uint32_t gLastVerboseLen = 0;
struct winsize	gTerminalSize;
#endif  // __1_71g__
boolean      gNulTerminator = 0;
boolean      gIgnoreCase =			//* default is determined by OS type
#ifdef	_MSC_VER
			1;	// case insensitive
#else
			0;	// case sensitive
#endif	/* _MSC_VER */
uint gDebug = 0;
uint gShowSettings = 0;
uint gPathNameCriteria = 0;
	char *gNameEquals   = NULL;
	char *gNameBegins   = NULL;
	char **gNameContainsStr = NULL;
	uint gNameContainsCnt = 0;
	char *gNameEnds     = NULL;
	char **gNameExcludesStr = NULL;
	uint gNameExcludesCnt = 0;
	char **gNameExcludesBeginStr = NULL;
	uint gNameExcludesBeginCnt = 0;
	char **gNameExcludesEndStr = NULL;
	uint gNameExcludesEndCnt = 0;
	char **gPathContainsStr = NULL;
	uint gPathContainsCnt = 0;
	char **gPathExcludesStr = NULL;
	uint gPathExcludesCnt = 0;
	char *gFileRegExp   = NULL;
	struct re_pattern_buffer gFilePatternBuffer = {0};
	char *gPathRegExp   = NULL;
	struct re_pattern_buffer gPathPatternBuffer = {0};
uint gTimeStampCriteria = 0;
	char *gPathNewer    = NULL;
	char *gPathOlder    = NULL;
	struct stat gPathNewerStat;
	struct stat gPathOlderStat;
	int gTimeDirection	= 0;
	time_t gTimeRange = 0;
	time_t gCurrentTime = 0;
uint gFileSizeCriteria = 0;
	int gSizeComparisonSetting = SIZE_IN_RANGE;			// default
	int gSizeExact = 0;				/* when gSizeComparisonSetting != SIZE_IN_RANGE */
	int gSizeUpperLimit = 0;		/* when gSizeComparisonSetting == SIZE_IN_RANGE */
	int gSizeLowerLimit = 0;		/* when gSizeComparisonSetting == SIZE_IN_RANGE */
uint gFileNameLengthCriteria = 0;
	int gNameLengthComparisonSetting = SIZE_IN_RANGE;	// default
	int gNameLengthExact = 0;		/* when gNameLengthComparisonSetting != SIZE_IN_RANGE */
	int gNameLengthUpperLimit = 0;	/* when gNameLengthComparisonSetting == SIZE_IN_RANGE */
	int gNameLengthLowerLimit = 0;	/* when gNameLengthComparisonSetting == SIZE_IN_RANGE */
uint gPermissionCriteria = 0;
char *gPermissionConstraintStr = NULL;
#define	MODE_NOT_INCLUDE	0
#define	MODE_TO_INCLUDE		1
#define	MODE_TO_MATCH		2
	int gPermissionState = 0;
	int gPermissionMode = 0;		/* bitmask */
char *gCompoundCommand = NULL;
	char gCompoundSymbol = '%';		/* replacable symbol in compound command */
	int  gCompoundSymCount = 0;
char *gTargetDirectory = NULL;

/* local functions
 */
static void	show_Settings(int optptr, int argc, char **argv);
static void check_Settings(void);
static void traverse_DirTree(char *dirpath);
static int matchNameString(const char *filename, const char *fullpath);
static int matchTimeStamp(const char *filename, const char *fullpath, struct stat *statp);
static int matchFileSize(const char *filename, const char *fullpath, struct stat *statp);
static int matchPermission(const char *filename, const char *fullpath, struct stat *statp);
static int parseSizeExactSetting(char *str, int *toskip);
/*
static int check_IfDirectory(char *path);
static int check_IfFile(char *path);
*/
static int Str2ArgList(char *str, char **arlist, int arlistsize);
static int32_t parse_Parameter(char *progname, int argc, char **argv);
static int parseTimeConstraint(int option, char *str);
static int parseSizeConstraint(int option, char *str);
static int parseNameLengthConstraint(int option, char *str);
static int parsePermissionConstraint(int option, char *str);
static char *composeExternalCommand(const char* fullpath);
static char *keepLongerString(char *str, char **array, int count);
static char *keepShorterString(char *str, char **array, int count);
static char *keepShorterEndString(char *str, char **array, int count);
static void lowerStrings(char **array, int count);
static void dumpStrings(char *msg, char **array, int count);
static int matchStrings(char *str, char **array, int count);
static void cancelConflicts(char **containsStr, uint containsCnt, char **excludesStr, unsigned int excludesCnt);
static char *numericToString(size_t size, char *buffer, uint bufsize);

/* program options
 */
static void version(char *progname)
{
	fprintf(stdout, "%s %s, %s\n", _PROGRAMNAME_, _PROGRAMVERSION_, _COPYRIGHT_);
	fprintf(stdout, "built at %s %s\n", __DATE__, __TIME__);
	return;
}
static void usage(char *progname, int detail)
{
	fprintf(stdout, "%s - %s\n", _PROGRAMNAME_, _DESCRIPTION_);
	fprintf(stdout, "Usage: %s [options] <target directory...>\n", progname);
	fprintf(stdout, "  -h|--help        help (in details)\n");
	fprintf(stdout, "  -v               version\n");
#if	defined(_WIN32) || defined(__CYGWIN32__)
	fprintf(stdout, "  -a[d|f]          attribute\n");
#else
	fprintf(stdout, "  -a[d|f|h]        attribute\n");
#endif	/* _WIN32 || __CYGWIN32__ */
	if (detail)
	{
	fprintf(stdout, "    [D|F]          directory or file with details\n");
	}
//	fprintf(stdout, "  -f<cfg_file>     configuration file (not yet implemented)\n");
	fprintf(stdout, "  -=<pattern>      name equals <pattern>\n");
	fprintf(stdout, "  -b<pattern>      name begins with <pattern>\n");
	fprintf(stdout, "  -e<pattern>      name ends with <pattern>\n");
	fprintf(stdout, "  -c<pattern>      name contains <pattern>\n");
	fprintf(stdout, "  -C<pattern>      path contains <pattern>\n");
	fprintf(stdout, "     -b, -e, -c, -C are matched inclusively (match ALL to identify the entry)\n");
	fprintf(stdout, "  -x<pattern>      name without <pattern>\n");
	fprintf(stdout, "  -y<pattern>      name not begins with <pattern>\n");
	fprintf(stdout, "  -z<pattern>      name not ends  with <pattern>\n");
	fprintf(stdout, "  -X<pattern>      path without <pattern>\n");
	fprintf(stdout, "     -x, -X are matched exclusively (ANY match excludes the entry)\n");
	fprintf(stdout, "  -m<regexp>       name matchs <regexp>\n");
	fprintf(stdout, "  -M<regexp>       path matchs <regexp>\n");
	fprintf(stdout, "  -n<path>         newer than path\n");
	fprintf(stdout, "  -o<path>         older than path\n");
	fprintf(stdout, "  -t(+|-)<time>    modification time constraint (s|m|h|d|w)\n");
	if (detail)
	{
	fprintf(stdout, "     +             older than <time>\n");
	fprintf(stdout, "     -             within <time>\n");
	fprintf(stdout, "     <time>        in <NNN>(s|m|h|d|w) format\n");
	}
	fprintf(stdout, "  -s(+|-|=)<size>  file size constraint (b|k|m|g)\n");
	if (detail)
	{
	fprintf(stdout, "     +             greater than <size>\n");
	fprintf(stdout, "     -             smaller than <size>\n");
	fprintf(stdout, "     [=]           equal to <size> (= sign is optional)\n");
	fprintf(stdout, "     <size>        in <NNN>(b|k|m|g) format\n");
	fprintf(stdout, "     <smin>~<smax> in the range of <smin> to <smax>\n");
	}
	fprintf(stdout, "  -w(+|-|=)<width> file name length constraint\n");
	if (detail)
	{
	fprintf(stdout, "     +             greater than <width>\n");
	fprintf(stdout, "     -             smaller than <width>\n");
	fprintf(stdout, "     [=]           equal to <width> (= sign is optional)\n");
	fprintf(stdout, "     <size>        a numerical value\n");
	}
	fprintf(stdout, "  -p(+|-)<mode>    access permission mode\n");
	if (detail)
	{
	fprintf(stdout, "     +             any of <mode> permission set\n");
	fprintf(stdout, "     -             none of <mode> permission set\n");
	fprintf(stdout, "     [=]           match exactly <mode> permission\n");
	fprintf(stdout, "     <mode>        any combination of r,w,x\n");
	}
	fprintf(stdout, "  -D<path>         target directory path (ignore other target directory)\n");
	fprintf(stdout, "  -r               recursive\n");
	fprintf(stdout, "  -j               junk paths (do not show directory)\n");
	fprintf(stdout, "  -l#              limit # of found entires\n");
	fprintf(stdout, "  -L#              limit directory depth/level\n");
	fprintf(stdout, "  -i               ignore case distinctions\n");
	fprintf(stdout, "  -I               enable case distinctions\n");
	fprintf(stdout, "  -E<program>      execute program or command\n");
	fprintf(stdout, "  -q               quiet mode\n");
#if	defined(unix) || defined(__STDC__)
	fprintf(stdout, "  -V               verbose mode (disabled in debug mode or using NUL terminator\n");
#endif
	fprintf(stdout, "  -0               use NUL instead of NL as terminator\n");
	fprintf(stdout, "  -d#              debug level\n");
	if (detail)
	{
	fprintf(stdout, "  -S               show settings (all options) and exit\n");
	}
	fprintf(stdout, "\n");
	fprintf(stdout, "  Option set in environment variable \"%s\" will be parsed first\n", _ENVVARNAME_);
	if (detail)
	{
	fprintf(stdout, "\nExample:\n");
	fprintf(stdout, "  %s -v\n", progname);
	fprintf(stdout, "  %s -r -e.tmp %%TEMP%%\n", progname);
	fprintf(stdout, "  %s -ctmp -Els %%TEMP%%\n", progname);
	fprintf(stdout, "  %s -ctmp -E\"ls -l %%\" %%TEMP%%\n", progname);
	fprintf(stdout, "  %s -r -e.tmp -E\"move %% %%.junk\" %%TEMP%%\n", progname);
	fprintf(stdout, "  %s -r -e.tmp -E\"move %% %%:r.junk\" %%TEMP%%\n", progname);
	fprintf(stdout, "  %s -t+10d %%TEMP%%\n", progname);
	fprintf(stdout, "  %s -s-300K %%TEMP%%\n", progname);
	}
	return;
}
#ifdef	_MSC_VER
// #define	fprintf	win_fprintf
#include <stdarg.h>			/* for var_list, etc. */
int win_fprintf(FILE *stream, const char * format, ...)
{
	va_list argptr;
	int rc;

	va_start(argptr, format);
	rc = vfwprintf(stream, (const wchar_t*)format, argptr);
	va_end(argptr);
	return rc;
}
#endif	/* _MSC_VER */

/* debug output function */
int XOutput(const int level, const char * format, ...)
{
	va_list argptr;
	int rc;

	/* control by global debug level */
	if (level > (int) gDebug) return -1;

	va_start(argptr, format);
	rc = vprintf(format, argptr);
	va_end(argptr);
	return rc;
}

/* main program
 */
int 
main(int argc, char **argv)
{
	char *progname = argv[0];
	char *envp = getenv(_ENVVARNAME_);
	int32_t nextarg;

#ifdef  __1_71g__
#if	defined(unix) || defined(__STDC__)
	// assume unchanged terminal size during the run
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &gTerminalSize);
#endif
#endif  // __1_71g__

	/* process environment setting if exists */
	if (envp != NULL) {
	#define	ENV_OPT_COUNT	16
		int  eargc;
		char *eargv[ENV_OPT_COUNT];

		//-dbg- fprintf(stderr, "envp:>>%s<<\n", envp);
				/** TODO: strdup() will introduce a memory leak **/
		if ((eargc = Str2ArgList(strdup(envp), &eargv[1], ENV_OPT_COUNT))) {
			/* create a dummy argv[0] as fds_getopt() in parse_Parameter()
			 * SKIP the first argument...
			 */
			eargc++;
			eargv[0] = _ENVVARNAME_;
			/* evaluate all options */
			parse_Parameter(progname, eargc, eargv);
		}
		else {
			fprintf(stderr, "%s: environment variable contains NO valid options\n", _ENVVARNAME_);
			envp = NULL;
		}
	}	/* if (envp = != NULL) */

	/* no arguments */
	if (argc == 1) {
		/* and no environment setting */
		if (envp == NULL) {
			usage(progname, 0);
			return 0;	// gTotalMatches;
		}
		/* but environment setting available */
		else {
			fprintf(stderr, "Default option: %s\n", envp);
		}
	}

	/* initialization */
#ifdef	_MSC_VER
	if (getenv("SHLVL") != NULL) { gPathDelimiter = '/'; }
#endif	/* _MSC_VER */

	/* setting information */
	if (gDebug >= 10) { show_Settings((int)0, 0, NULL); }

	/* parse arguments in command line
	 */
	nextarg = parse_Parameter(progname, argc, argv);
	if (nextarg <= 0) {
		// version(progname);
		return -1;
	}
	//-dbg- printf("argc=%d nextarg=%d argv[nextarg]=<%s>\n", argc, nextarg, argv[nextarg]);

	/* setting information */
	if (gDebug >= 10) { show_Settings((int)nextarg, argc, argv); }
	/*
	 *** */

#ifdef  __1_71g__
	//
	if (gShowSettings) {
		show_Settings((int)nextarg, argc, argv);
		return 0;
	}
#endif  // __1_71g__

	/* ***
	 * CHECK THE SETTING
	 */
	check_Settings();
	/*
	 *** */

	/* ***
	 * DO THE JOB
	 */
	/* Special case: 2004-03-24
	 *
	 * If name constraint is set and the one and only one argument is a FILE,
	 * then search this FILE at current direectory tree.
	 */
	if ((gPathNameCriteria == 0) &&
		((argc - nextarg) == 1) && (IsFile(argv[nextarg]))
	   )
	{
		gPathNameCriteria++;
		gNameEquals = argv[nextarg];
		traverse_DirTree(".");
		return 0;
	}

	/* normal case:
	 */
	if (gTargetDirectory) {
		/* target directory path set in command line takes precedence
		 */
		if (IsDirectory(gTargetDirectory)) {
			traverse_DirTree(gTargetDirectory);
		}
	}
	else if (nextarg < argc) {
		/* visit every remained parameter as root working directory
		 */
		int optidx;
		for (optidx = nextarg; optidx < argc; optidx++) {
			/* make sure this is a valid directory entry */
			if (IsDirectory(argv[optidx])) {
				if (gDebug >= 10)
					fprintf(stderr, "[%s]: IsDirectory\n", argv[optidx]);
				traverse_DirTree(argv[optidx]);
			}
			else {
				if (gDebug >= 10)
					fprintf(stderr, "[%s]: IsNotDirectory\n", argv[optidx]);
			}
		}
	}
	else {
		/* look into the current directory as root working directory
		 */
		traverse_DirTree(".");
	}
	/*
	 *** */

#ifdef  __1_71g__
#if	defined(unix) || defined(__STDC__)
	// clean up
	if (gLastVerboseLen) {
		uint32_t fLines = (gLastVerboseLen / gTerminalSize.ws_col);
		while (fLines--) {
			printf("%s%s", CLEAR_LINE, CURSOR_UP);
		}
		printf("%s\r", CLEAR_LINE);
		fflush(stdout);
	}
#endif /* defined(unix) || defined(__STDC__) */
#endif  // __1_71g__

	/* clean up allocated tables
	 */
	if (gNameContainsStr) free(gNameContainsStr);
	if (gNameExcludesStr) free(gNameExcludesStr);
	if (gPathContainsStr) free(gPathContainsStr);
	if (gPathExcludesStr) free(gPathExcludesStr);

	return gTotalMatches;
}


/* ***********************************
 * support functions
 */
static int matchNameString(const char *filename, const char *fullpath)
{
	uint match = 0;
	uint exclude = 0;
	int flen = strlen(filename);
	int (*cmpfunc)(const char *s1, const char *s2, size_t n);

	/* select appropriate compare function */
	cmpfunc = gIgnoreCase ? strnicmp : strncmp;

	/* cases that may exclude this entry
	 * */
	if (gNameExcludesCnt) {
		char *dupname = strdup(filename);
		if (gIgnoreCase) strlwr(dupname);
		exclude = matchStrings(dupname, gNameExcludesStr, gNameExcludesCnt);
		if (exclude == 0) match++;
		// if (gDebug > 2) fprintf(stderr, "%s is excluded, exclude=%d\n", filename, exclude);
		free(dupname);
	}
	if (gNameExcludesBeginCnt) {
		uint32_t ix;
		for (ix = 0; ix < gNameExcludesBeginCnt; ix++) {
			if (cmpfunc(filename, gNameExcludesBeginStr[ix], strlen(gNameExcludesBeginStr[ix])) == 0) {
				// if (gDebug > 2) fprintf(stderr, "*** excludes %s begins [%s]\n", filename, gNameExcludesBeginStr[ix]);
				exclude++;
				break;
			}
		}
		if (exclude == 0) match++;
	}
	if (gNameExcludesEndCnt) {
		uint32_t ix;
		for (ix = 0; ix < gNameExcludesEndCnt; ix++) {
			int slen = strlen(gNameExcludesEndStr[ix]);
			if (cmpfunc(&filename[flen-slen], gNameExcludesEndStr[ix], slen) == 0) {
				// if (gDebug > 2) fprintf(stderr, "*** excludes %s ends [%s]\n", filename, gNameExcludesEndStr[ix]);
				exclude++;
				break;
			}
		}
		if (exclude == 0) match++;
	}
	if (exclude) {
		if (gDebug > 2) fprintf(stderr, "%s: excluded by -x or -y or -z\n", filename);
		return 0;
	}

	if (gPathExcludesCnt) {
		char *dupname = strdup(fullpath);
		char *ptr;
		// exclude the filename. i.e. keep ONLY the path
		if ((ptr = strrchr(dupname, (int)gPathDelimiter)) != NULL)
			*ptr = 0;
		if (gIgnoreCase) strlwr(dupname);
		if ((exclude = matchStrings(dupname, gPathExcludesStr, gPathExcludesCnt)) == 0) {
			/* indicating that this criteria has been checked */
			match++;
		}
		free(dupname);
	}
	if (exclude) return 0;

	if (gPathContainsCnt) {
		char *dupname = strdup(fullpath);
		char *ptr;
		int  mcount;
		// exclude the filename. i.e. keep ONLY the path
		if ((ptr = strrchr(dupname, (int)gPathDelimiter)) != NULL)
			*ptr = 0;
		if (gIgnoreCase) strlwr(dupname);
		/* mcount: how many matched string found */
		mcount = matchStrings(dupname, gPathContainsStr, gPathContainsCnt);
		match += mcount;
		if (mcount != (int) gPathContainsCnt) { exclude++; }
		free(dupname);
	}
	if (exclude) return 0;

	if (gNameEquals) {
		/* find any condition could exclude this entry */
		if (flen != (int)strlen(gNameEquals) ||
			cmpfunc(filename, gNameEquals, strlen(gNameEquals)))
		{
			// if (gDebug > 2) fprintf(stderr, "*** %s is excluded due to inequality to [%s]\n", filename, gNameEquals);
			exclude++;
		}
		match++;
	}
	if (exclude) return 0;

	if (gFileNameLengthCriteria) {
		/* find any condition could exclude this entry */
		if (
			/* range is set */
			((gNameLengthComparisonSetting == SIZE_IN_RANGE) &&
					 ((flen < gNameLengthLowerLimit) || (gNameLengthUpperLimit < flen)))   ||
			/* single limit set */
			((gNameLengthComparisonSetting == SIZE_EQUAL)   && (flen != gNameLengthExact)) ||
			((gNameLengthComparisonSetting == SIZE_LESS)    && (flen >= gNameLengthExact)) ||
			((gNameLengthComparisonSetting == SIZE_LESS_EQ) && (flen >  gNameLengthExact)) ||
			((gNameLengthComparisonSetting == SIZE_OVER)    && (flen <= gNameLengthExact)) ||
			((gNameLengthComparisonSetting == SIZE_OVER_EQ) && (flen <  gNameLengthExact))
		   )
		{
			// if (gDebug > 2) fprintf(stderr, "*** %s is excluded due to file length criteria [%d]\n", filename, flen);
			exclude++;
		}
		match++;
	}
	if (exclude) return 0;

	/* cases that may match this entry partially
	 * */
	/* match sub-string */
	if (gNameContainsCnt) {
		char *dupname = strdup(filename);
		if (gIgnoreCase) strlwr(dupname);
		match += matchStrings(dupname, gNameContainsStr, gNameContainsCnt);
		free(dupname);
	}

	/* select appropriate compare function */
	cmpfunc = gIgnoreCase ? strnicmp : strncmp;

	if (gNameBegins) {
		if (cmpfunc(filename, gNameBegins, strlen(gNameBegins)) == 0) {
			if (gDebug > 2) fprintf(stderr, "*** %s begins [%s]\n", fullpath, gNameBegins);
			match++;
		}
	}

	if (gNameEnds) {
		int slen = strlen(gNameEnds);
		if (cmpfunc(&filename[flen-slen], gNameEnds, slen) == 0) {
			if (gDebug > 2) fprintf(stderr, "*** %s ends [%s]\n", fullpath, gNameEnds);
			match++;
		}
	}

	/* regular expression matching */
	if (gFileRegExp) {
		if (re_search(&gFilePatternBuffer, filename, strlen(filename), 0, strlen(filename), NULL) >= 0) {
			if (gDebug > 2) fprintf(stderr, "*** %s matches File regex [%s]\n", fullpath, gFileRegExp);
			match++;
		}
	}

	if (gPathRegExp) {
		if (re_search(&gPathPatternBuffer, fullpath, strlen(fullpath), 0, strlen(fullpath), NULL) >= 0) {
			if (gDebug > 2) fprintf(stderr, "*** %s matches Path regex [%s]\n", fullpath, gPathRegExp);
			match++;
		}
	}

	if (gDebug > 3) fprintf(stderr, "matchNameString: %s, match=%d (NamsStringCriteria=%d)\n", fullpath, match, gPathNameCriteria);

	return (match == gPathNameCriteria);
}

/*
 * matchTimeStamp - check if provided entry matches set time criteria
 */
static int matchTimeStamp(const char *filename, const char *fullpath, struct stat *statp)
{
	uint match = 0;

	if (gPathNewer) {
		if (statp->st_mtime > gPathNewerStat.st_mtime) {
			if (gDebug > 2) fprintf(stderr, "*** %s (%#x) later than [%s] (%#x)\n",
				fullpath, (uint)statp->st_mtime, gPathNewer, (unsigned int)gPathNewerStat.st_mtime);
			match++;
		}
	}

	if (gPathOlder) {
		if (statp->st_mtime > gPathOlderStat.st_mtime) {
			if (gDebug > 2) fprintf(stderr, "*** %s (%#x) later than [%s] (%#x)\n",
				fullpath, (uint)statp->st_mtime, gPathOlder, (unsigned int)gPathOlderStat.st_mtime);
			match++;
		}
	}

	if (gTimeDirection && gTimeRange) {
		if (gDebug > 4) fprintf(stderr, "*** gCurrentTime= %d, gTimeRange= %d\n", (int)gCurrentTime, (int)gTimeRange);
		if (gDebug > 4) fprintf(stderr, "*** mtime (%s)= %d\n", filename, (int)statp->st_mtime);
		if ((gTimeDirection == TIME_WITHIN) &&
			((gCurrentTime - statp->st_mtime) <= (time_t)gTimeRange))
		{
			if (gDebug > 2) fprintf(stderr, "*** %s matched time_within\n", filename);
			match++;
		}
		else if ((gTimeDirection == TIME_OVER) &&
			((gCurrentTime - statp->st_mtime) >= (time_t)gTimeRange))
		{
			if (gDebug > 2) fprintf(stderr, "*** %s matched time_over\n", filename);
			match++;
		}
	}
	// 2005-05-16 - there is ONE criteria allowed
	// return (match == gTimeStampCriteria);
	return match;
}

/*
 * matchFileSize - check if provided entry matches set size criteria
 *
 *	if gSizeComparisonSetting != 0, the file size is compared with gSizeExact (GT, GE, EQ, LE, LT)
 *	if gSizeComparisonSetting == 0, the file size must be between gSizeLowerLimit and gSizeUpperLimit
 */
static int matchFileSize(const char *filename, const char *fullpath, struct stat *statp)
{
	int  fsize = statp->st_size;
	uint match = 0;

	/* size is only meaningful for non-directory entry
	 */
	if (S_ISDIR(statp->st_mode)) { return 0; }

	if (gDebug > 4) fprintf(stderr, "*** st_size (%s)= %d, gSizeComparisonSetting=%d\n", filename, fsize, gSizeComparisonSetting);
	if (gSizeComparisonSetting != SIZE_IN_RANGE) {
		if (gDebug > 4) fprintf(stderr, "*** gSizeExact= %d\n", gSizeExact);
		if ((gSizeComparisonSetting == SIZE_LESS) && (fsize < gSizeExact))
		{
			if (gDebug > 2) fprintf(stderr, "*** %s matched size_less\n", filename);
			match++;
		}
		else if ((gSizeComparisonSetting == SIZE_LESS_EQ) && (fsize <= gSizeExact))
		{
			if (gDebug > 2) fprintf(stderr, "*** %s matched size_less_eq\n", filename);
			match++;
		}
		else if ((gSizeComparisonSetting == SIZE_OVER) && (fsize > gSizeExact))
		{
			if (gDebug > 2) fprintf(stderr, "*** %s matched size_over\n", filename);
			match++;
		}
		else if ((gSizeComparisonSetting == SIZE_OVER_EQ) && (fsize >= gSizeExact))
		{
			if (gDebug > 2) fprintf(stderr, "*** %s matched size_over_eq\n", filename);
			match++;
		}
		else if ((gSizeComparisonSetting == SIZE_EQUAL) && (fsize == gSizeExact))
		{
			if (gDebug > 2) fprintf(stderr, "*** %s matched size_equal\n", filename);
			match++;
		}
	}
	else /* (gSizeComparisonSetting == SIZE_IN_RANGE) */ {
		if (gDebug > 4) fprintf(stderr, "*** gSizeLowerLimit= %d, gSizeUpperLimit= %d\n", gSizeLowerLimit, gSizeUpperLimit);
		if ((gSizeLowerLimit <= fsize) && (fsize <= gSizeUpperLimit)) {
			if (gDebug > 2) fprintf(stderr, "*** %s matched size_equal\n", filename);
			match++;
		}
	}
	// 2005-05-16 - there is ONE criteria allowed
	// return (match == gFileSizeCriteria);
	return match;
}

/*
 * matchPermission - check if provided entry matches set access permission criteria
 */
static int matchPermission(const char *filename, const char *fullpath, struct stat *statp)
{
	int  mode = statp->st_mode & (S_IREAD|S_IWRITE|S_IEXEC);

	/* access permission is only meaningful for non-directory entry
	 */
//	if (S_ISDIR(statp->st_mode)) { return 0; }

	/* does this entry has (or not has) the access permission
	 */
	if (gPermissionState == MODE_NOT_INCLUDE) {
		return (!(mode & gPermissionMode));
	}
	else if (gPermissionState == MODE_TO_INCLUDE) {
		return ((mode & gPermissionMode) == gPermissionMode);
	}
	else if (gPermissionState == MODE_TO_MATCH) {
		//-debug- fprintf(stderr, "mode=%d, gPermissionMode=%d\n", mode, gPermissionMode);
		return (mode == gPermissionMode);
	}
	return 0;
}

static int parseSizeExactSetting(char *str, int *toskip)
{
	int  rc = SIZE_EQUAL;
	char *ptr = str;

	/* make sure we have correct constraint marker */
	if (ptr[0] == '+' && ptr[1] == '=') {
		ptr += 2;
		rc = SIZE_OVER_EQ;
	}
	else if (ptr[0] == '-' && ptr[1] == '=') {
		ptr += 2;
		rc = SIZE_LESS_EQ;
	}
	else if (ptr[0] == '+') {
		ptr++;
		rc = SIZE_OVER;
	}
	else if (ptr[0] == '-') {
		ptr++;
		rc = SIZE_LESS;
	}
	else if (ptr[0] == '=') {
		ptr++;
		rc = SIZE_EQUAL;
	}

	/* make sure next token is a numeric number */
	if (!isdigit(*ptr)) return -1;

	/* tell caller how many character we consumed */
	*toskip = ptr - str;
	return rc;
}

dirInfo_t gMatchedBuffer = { 0 };	/* match information */
int seekCallback(const char *filename, const char *fullpath, struct stat *statp, void *opaquep)
{
	uint attr = ENTITY_OTHER;
	static uint realMatchedCount = 0;

	/* strip "./" or ".\\" prefix in fullpath */
	if (fullpath[0] == '.' &&
		((fullpath[1] == '/') || fullpath[1] == '\\'))
	{
		/* skip leading "./" */
		fullpath += 2;
	}
	if (gDebug > 3) fprintf(stderr, "seekCallback: %s\n", fullpath);

	/* identify entity type */
	if (S_ISDIR(statp->st_mode))      { attr = ENTITY_DIRECTORY; }
	else if (S_ISREG(statp->st_mode)) { attr = ENTITY_FILE; }
#if	!defined(_WIN32) && !defined(__CYGWIN32__)
	else if (S_ISLNK(statp->st_mode)) { attr = ENTITY_SYMLINK; }
#endif	/* !_WIN32 && !__CYGWIN32__ */

	/* match entity attribute */
	if ((attr & gEntityAttribute) == 0) {
		if (gDebug > 3) fprintf(stderr, "seekCallback: mismatched attribute %s\n",
			(attr==ENTITY_DIRECTORY)?"DIRECTORY":
#if	!defined(_WIN32) && !defined(__CYGWIN32__)
			(attr==ENTITY_SYMLINK)?"SYMLINK":
#endif	/* !_WIN32 && !__CYGWIN32__ */
			(attr==ENTITY_FILE)?"FILE":"other");
		return 0;
	}

	/* real match... */

#ifdef  __1_71g__
#if	defined(unix) || defined(__STDC__)
  #if  __1_71g__
	if (gVerboseMode) {
		if (gLastVerboseLen && (gLastVerboseLen > gTerminalSize.ws_col))
		{
            uint32_t fLines = (gLastVerboseLen / gTerminalSize.ws_col);
            while (fLines--) {
                printf("%s%s", CLEAR_LINE, CURSOR_UP);
		    }
		}
	}
  #else	// pre-1.71g
	static uint32_t lastLineLen = 0;
	if (gVerboseMode) {
		// last line is wider than current terminal width
		if (lastLineLen > gTerminalSize.ws_col) {
			printf("%s%s", CLEAR_LINE, CURSOR_UP);
		}
		printf("%s\r%s\r", CLEAR_LINE, fullpath);
//		printf("%s\r[%d:%d]-%s", CLEAR_LINE, gTerminalSize.ws_col, lastLineLen, fullpath);
		fflush(stdout);
        lastLineLen = (int)strlen(fullpath);
	}
  #endif
#endif
#endif  // __1_71g__

	if (
		/* matching [path]name with patterns */
		((gPathNameCriteria == 0) ||
		(gPathNameCriteria && matchNameString(filename, fullpath))) &&
		/* matching timestamp constraints */
		((gTimeStampCriteria == 0) ||
		(gTimeStampCriteria && matchTimeStamp(filename, fullpath, statp))) &&
		/* matching size constraints */
		((gFileSizeCriteria == 0) ||
		(gFileSizeCriteria && matchFileSize(filename, fullpath, statp))) &&
		/* matching permission constraints */
		((gPermissionCriteria == 0) ||
		(gPermissionCriteria && matchPermission(filename, fullpath, statp)))
	)
	{
		// execute command on the matched entitiy
		if (gCompoundCommand)
		{
			int  rc;
			char *ncCommand = composeExternalCommand(fullpath);
			if (gDebug > 6) fprintf(stderr, "seekCallback: execute command: %s\n", ncCommand);

			if (ncCommand != NULL) {
// ...........
				/* execute the external command */
				if (gDebug == 0 && gQuietMode == 0) printf("%s\n", ncCommand);
				rc = system(ncCommand);
				if (gDebug > 6) fprintf(stderr, "CompoundCommand rc=%d\n", rc);
				/* 06-20-2003 enable external command to be applied to future matched entires */
				if (rc && gCompoundSymCount == 0) {
					/* DISABLE the external command if error in executing the simple command */
					gCompoundCommand = NULL;
					/* command failed, show only the found entity */
					printf("?? %s\n", ncCommand);
				}
// ...........
				/* free compound command buffer */
				free(ncCommand);
			}	/* if (ncCommand != NULL) */
		}
		else
		{
			/* echo matched entitiy */
			if (gQuietMode == 0) {

#ifdef  __1_71g__
#if	defined(unix) || defined(__STDC__)
  #if pre-1.71g
	if (gVerboseMode) {
		// last line is wider than current terminal width
		if (lastLineLen > gTerminalSize.ws_col) {
			printf("%s%s", CLEAR_LINE, CURSOR_UP);
		}
		printf("%s\r", CLEAR_LINE);
		lastLineLen = 0;
	}
  #endif
#endif
#endif  // __1_71g__

				if (gJunkPaths) {
					char *lastp = strrchr(fullpath, (int)gPathDelimiter);
					printf("%s", (lastp == NULL) ? fullpath : ++lastp);
					putc(gNulTerminator ? (int)'\0' : (int)'\n', stdout);
				}
				else {
					if (gEntityAttribute & ENTITY_DETAILS) {
						// showing entity details
						struct tm  *tmptr = localtime(&(statp->st_mtime));
						//
#if	defined(_WIN32) || defined(__CYGWIN32__)
						if (S_ISDIR(statp->st_mode)) {
							// WIN32 directory entry has NO size
							printf("[%04d-%02d-%02d %02d:%02d:%02d].<dir> %s",
								// timestamp
								(tmptr->tm_year+1900), (tmptr->tm_mon+1), tmptr->tm_mday,
								tmptr->tm_hour, tmptr->tm_min, tmptr->tm_sec,
								fullpath);
							putc(gNulTerminator ? (int)'\0' : (int)'\n', stdout);
						}
						else
#endif	/* _WIN32 || __CYGWIN32__ */
						{
							char bufSize[16];
//									<--------- timestamp ---------> <FS> <FY> <name>
							printf("[%04d-%02d-%02d %02d:%02d:%02d].[%s].[%s] %s",
								// timestamp
								(tmptr->tm_year+1900), (tmptr->tm_mon+1), tmptr->tm_mday,
								tmptr->tm_hour, tmptr->tm_min, tmptr->tm_sec,
								// filesize
								numericToString(statp->st_size, bufSize, sizeof(bufSize)),
								// filetype
								S_ISREG(statp->st_mode) ? "REG" :
								S_ISDIR(statp->st_mode) ? "DIR" :
#ifndef	_WIN32
								S_ISLNK(statp->st_mode) ? "LNK" :
								S_ISSOCK(statp->st_mode) ? "SOCK" :
#endif	/* !_WIN32 */
								"OTH",
								fullpath);
							putc(gNulTerminator ? (int)'\0' : (int)'\n', stdout);
						}
					}
					else {
						printf("%s", fullpath);
						putc(gNulTerminator ? (int)'\0' : (int)'\n', stdout);
						fflush(stdout);
					}
// TODO: manage & print wide-characters
//					wprintf(L"-คJนา-%s\n", fullpath);
				}
			}	/* if (gQuietMode == 0) */
		}

		/* information collection */
		gTotalMatches++;
		realMatchedCount++;

		if (attr == ENTITY_DIRECTORY) { gMatchedBuffer.num_of_directories++; }
		else if (attr == ENTITY_FILE) { gMatchedBuffer.num_of_files++; }
		else                          { gMatchedBuffer.num_of_others++; }

		/* EXIT the program if have we found enough entries? */
		if (gLimitEntry == realMatchedCount)
			exit (gLimitEntry);
	}

#ifdef  __1_71g__
#if	defined(unix) || defined(__STDC__)
	if (gVerboseMode) {
		// print verbose line
		printf("%s\r%s", CLEAR_LINE, fullpath);
		fflush(stdout);
		gLastVerboseLen = (uint32_t)strlen(fullpath);
	}
#endif
#endif  // __1_71g__

	return 0;
}

static void dumpStrings(char *msg, char **array, int count)
{
	int ix;
	if (count == 0) {
		fprintf(stderr, "%s:        -none-\n", msg);
		return;
	}
	for (ix = 0; ix < count; ix++) {
		if (array[ix] != NULL)
			fprintf(stderr, "%s [%d]:    %s\n", msg, ix, array[ix]);
	}
	return;
}

static void	show_Settings(int optptr, int argc, char **argv)
{
	int optidx;
	fprintf(stderr, "--- program settings ---\n");
	fprintf(stderr, "attribute: %s %s %s\n",
			(gEntityAttribute&ENTITY_DIRECTORY)?"DIRECTORY":"",
#if	!defined(_WIN32) && !defined(__CYGWIN32__)
			(gEntityAttribute&ENTITY_SYMLINK)?"SYMLINK":"",
#endif	/* !_WIN32 && !__CYGWIN32__ */
			(gEntityAttribute&ENTITY_FILE)?"FILE":"");
	fprintf(stderr, "limit entries:        %d (0 = unlimited)\n", gLimitEntry);
	fprintf(stderr, "ignore case:          %s\n", gIgnoreCase ? "TRUE" : "FALSE");
	fprintf(stderr, "junk output path:     %s\n", gJunkPaths ? "TRUE" : "FALSE");
	fprintf(stderr, "recursive mode:       %s\n", gRecursive ? "TRUE" : "FALSE");
	fprintf(stderr, "directory level:      %d (0 = unlimited)\n", gLimitDirLevel);
	fprintf(stderr, "quiet mode:           %s\n", gQuietMode ? "TRUE" : "FALSE");
	fprintf(stderr, "verbose mode:         %s\n", gVerboseMode ? "TRUE" : "FALSE");
	fprintf(stderr, "nul terminator:       %s\n", gNulTerminator ? "TRUE" : "FALSE");
	fprintf(stderr, "name equals with:     %s\n", gNameEquals ? gNameEquals : "");
	fprintf(stderr, "name begins with:     %s\n", gNameBegins ? gNameBegins : "");
	fprintf(stderr, "name ends with:       %s\n", gNameEnds ? gNameEnds : "");
	      dumpStrings("name contains", gNameContainsStr, gNameContainsCnt);
	      dumpStrings("name excludes", gNameExcludesStr, gNameExcludesCnt);
	fprintf(stderr, "file regexp is:       %s\n", gFileRegExp ? gFileRegExp : "");
	      dumpStrings("path contains", gPathContainsStr, gPathContainsCnt);
	      dumpStrings("path excludes", gPathExcludesStr, gPathExcludesCnt);
	fprintf(stderr, "path regexp is:       %s\n", gPathRegExp ? gPathRegExp : "");
	fprintf(stderr, "# of name string criteria=%d\n", gPathNameCriteria);
	fprintf(stderr, "path must newer than: %s\n", gPathNewer ? gPathNewer : "");
	fprintf(stderr, "path must older than: %s\n", gPathOlder ? gPathOlder : "");
	if (gTimeStampCriteria) {
	fprintf(stderr, "path must be modified %s%d seconds%s\n",
					gTimeDirection==TIME_WITHIN ? "WITHIN " : "",
					(int)gTimeRange,
					gTimeDirection==TIME_WITHIN ? "" : " AGO");
	}
	if (gFileSizeCriteria) {
		if (gSizeComparisonSetting != SIZE_IN_RANGE)
	fprintf(stderr, "file size must be %s %d bytes\n",
					gSizeComparisonSetting==SIZE_LESS    ? "LESS THAN" :
					gSizeComparisonSetting==SIZE_LESS_EQ ? "LESS OR EQUAL" :
					gSizeComparisonSetting==SIZE_OVER    ? "OVER THAN" :
					gSizeComparisonSetting==SIZE_OVER_EQ ? "OVER OR EQUAL" :
												"EQUAL TO", gSizeExact);
		else /* (gSizeComparisonSetting == SIZE_IN_RANGE) */
	fprintf(stderr, "file size must be between %d and %d bytes\n", gSizeLowerLimit, gSizeUpperLimit);
	}
	if (gFileNameLengthCriteria) {
		if (gNameLengthComparisonSetting != SIZE_IN_RANGE)
	fprintf(stderr, "file name length must be %s %d bytes\n",
					gNameLengthComparisonSetting==SIZE_LESS    ? "LESS THAN" :
					gNameLengthComparisonSetting==SIZE_LESS_EQ ? "LESS OR EQUAL" :
					gNameLengthComparisonSetting==SIZE_OVER    ? "OVER THAN" :
					gNameLengthComparisonSetting==SIZE_OVER_EQ ? "OVER OR EQUAL" :
												"EQUAL TO", gNameLengthExact);
		else /* (gNameLengthComparisonSetting == SIZE_IN_RANGE) */
	fprintf(stderr, "file name length must be between %d and %d bytes\n", gNameLengthLowerLimit, gNameLengthUpperLimit);
	}
	if (gPermissionCriteria) {
	fprintf(stderr, "permission constraint: %s\n", gPermissionConstraintStr);
	}
	//
	fprintf(stderr, "compound command: %s\n", gCompoundCommand ? gCompoundCommand : "[none]");
	fprintf(stderr, "debug level set to: %d\n", gDebug);
	fprintf(stderr, "target directory: %s\n", gTargetDirectory ? gTargetDirectory : "[none]");
	//
	/* rest of arguments */
	fprintf(stderr, "directories to visit:\n");
	for (optidx = (int)optptr; optidx < argc; optidx++) {
		fprintf(stderr, "argv[%d] = %s\n", optidx, argv[optidx]);
	}
	//
#ifdef  __1_71g__
	fprintf(stderr, "terminal dimention:   lines: %d columns: %d\n",
		gTerminalSize.ws_row, gTerminalSize.ws_col);
#endif  // __1_71g__
	fprintf(stderr, "--- end of settings ---\n\n");

	return;
}

static void check_Settings()
{
//	int nclen = (gNameContains) ? strlen(gNameContains) : 0;
//	int nelen = (gNameExcludes) ? strlen(gNameExcludes) : 0;

	/* set default attribute to match */
	if (gEntityAttribute == 0) {
		gEntityAttribute = (ENTITY_FILE | ENTITY_DIRECTORY | ENTITY_SYMLINK);
	}

	/* cancel out gNameContains and gNameExcludes if they conflict to each other */
//	if (nclen && nelen && (nclen >= nelen)) {
//		if (strcmp(gNameContains, gNameExcludes) == 0) {
//			gNameContains = gNameExcludes = NULL;
//		}
//	}

	/* cancel out possible conflicts between "contains" and "excludes" */
	cancelConflicts(gNameContainsStr, gNameContainsCnt, gNameExcludesStr, gNameExcludesCnt);
	cancelConflicts(gPathContainsStr, gPathContainsCnt, gPathExcludesStr, gPathExcludesCnt);

	/* convert match strings to lower case if case doesn't matter */
	if (gIgnoreCase) {
		if (gNameEquals) strlwr(gNameEquals);
		if (gNameBegins) strlwr(gNameBegins);
		if (gNameEnds)   strlwr(gNameEnds);
		if (gNameContainsCnt) lowerStrings(gNameContainsStr, gNameContainsCnt);
		if (gNameExcludesCnt) lowerStrings(gNameExcludesStr, gNameExcludesCnt);
		if (gNameExcludesBeginCnt) lowerStrings(gNameExcludesBeginStr, gNameExcludesBeginCnt);
		if (gNameExcludesEndCnt)   lowerStrings(gNameExcludesEndStr,   gNameExcludesEndCnt);
		if (gPathContainsCnt) lowerStrings(gPathContainsStr, gPathContainsCnt);
		if (gPathExcludesCnt) lowerStrings(gPathExcludesStr, gPathExcludesCnt);
	}

	return;
}

static void traverse_DirTree(char *dirpath)
{
	dirInfo_t dibuf = { 0 };	/* summary information */
	matchCriteria_t mcbuf = { 0 };
	char  path[80];
	char  *pathp = &path[0];

	if (dirpath == NULL) {
		printf("destination dir: ");
		scanf("%s", path);
	}
	else {
		pathp = dirpath;
	}

	if (gDebug) {
		/* enable debug output */
		mcbuf.printf=(OutputFunc)XOutput;
	}

	mcbuf.proc = seekCallback;	/* callback routine */
	// mcbuf.printf = fdsout;			/* output routine */

	/* search directory info */
	dirinfo_Find(pathp, &dibuf, &mcbuf, (int) gRecursive, (int) gLimitDirLevel, 1 /*current dir level*/);

	/* closing report */
	if (gDebug > 4) {
		dirinfo_Report(&dibuf, "total");
		dirinfo_Report(&gMatchedBuffer, "matched");
	}
}

/* Convert a string into argument list
 */
static int Str2ArgList(char *str, char **arlist, int arlistsize)
{
	int cnt = 0;
	//-dbg- printf("str=>>%s<<\n", str);
	while (cnt < arlistsize)
	{
		/* skip whitespaces */
		while (isspace((int)*str)) str++;

		/* done? */
		if (*str == 0) break;

		/* found an argument */
		//-dbg- printf("ac=%d first-char=%c\n", cnt, *str);
		*arlist = str;
		arlist++;
		cnt++;

		/* find the end of argument */
		while (*str && !isspace((int)*str)) str++;

		/* done? */
		//-dbg- printf("ac=%d last-char=%c next-char=%c=(%x)\n", cnt, *(str-1), *str, *str);
		if ((*str) == 0) break;
		else *str++ = 0;
	}
	return cnt;
}

/* Parsing parameters in (argc, argv)
 */
static int32_t parse_Parameter(char *progname, int argc, char **argv)
{
	int optcode;
	char *optptr;
//	extern char *optarg;
//	extern int optind;
	int errflags = 0;

	if (gDebug > 10)
	{
		int ix;
		printf("parse_Parameter\n");
		for (ix = 0; ix < argc; ix++)
			printf("argv[%d] = %s\n", ix, argv[ix]);
	}

	/* ***
	 * PARAMETER PARSING
	 */
	optptr = NULL;
	// while ((c = getopt(argc, argv, "abo:")) != EOF)
	while ((optcode = fds_getopt(&optptr, "?hva:=:b:c:e:x:y:z:m:n:o:C:X:M:t:s:w:p:D:jl:L:rRiIE:qV0d:", argc, argv)) != EOF)
	{
		//-dbg- printf("optcode=%c *optptr=%c\n", optcode, *optptr);
		switch (optcode) {
			case '?':
			case 'h':  usage(progname, 0);	return 0;
			case 'v':  version(progname);	return 0;

			/* select entity attribute (default: FILE) */
			case 'a':
					{
						uint ix=0;
						while (optptr[ix] != 0)
						{
							if (optptr[ix] == 'd') {
								gEntityAttribute |= ENTITY_DIRECTORY;
							}
							else if (optptr[ix] == 'f') {
								gEntityAttribute |= ENTITY_FILE;
							}
							else if (optptr[ix] == 'D') {
								gEntityAttribute |= ENTITY_DIRECTORY | ENTITY_DETAILS;
							}
							else if (optptr[ix] == 'F') {
								gEntityAttribute |= ENTITY_FILE | ENTITY_DETAILS;
							}
							else if (optptr[ix] == 'A') {	/* both - default setting */
								gEntityAttribute |= (ENTITY_FILE | ENTITY_DIRECTORY | ENTITY_SYMLINK | ENTITY_DETAILS);
							}
#if	!defined(_WIN32) && !defined(__CYGWIN32__)
							else if (optptr[ix] == 'h') {
								gEntityAttribute |= ENTITY_SYMLINK;
							}
							else if (optptr[ix] == 'H') {
								gEntityAttribute |= ENTITY_SYMLINK | ENTITY_DETAILS;
							}
#endif	/* !_WIN32 && !__CYGWIN32__ */
							else {
								//fprintf(stderr, "Invalid attribute option: %s (default set to FILE)\n", (char*) optptr);
								fprintf(stderr, "Unrecognized attribute option: %c (default set to FILE)\n", optptr[ix]);
								gEntityAttribute |= ENTITY_FILE;
							}
							//
							ix++;
						}
					break;
					}
			/* name matchihng pattern */
			case '=':
					{
						if (gNameEquals == NULL) gPathNameCriteria++;
						gNameEquals   = optptr;
						break;
					}
			case 'b':
					{
						if (gNameBegins == NULL) gPathNameCriteria++;
						gNameBegins   = optptr;
						break;
					}
			case 'e':
					{
						if (gNameEnds   == NULL) gPathNameCriteria++;
						gNameEnds     = optptr;
						break;
					}
			case 'c':
					{
						/* check duplication */
						if (keepLongerString(optptr, gNameContainsStr, gNameContainsCnt) == NULL) {
							gNameContainsStr = (char **) realloc(gNameContainsStr, (++gNameContainsCnt)*sizeof(char*));
							gNameContainsStr[(gNameContainsCnt-1)] = optptr;
							gPathNameCriteria++;
						}
						else {
							fprintf(stderr, "-%c%s: redundant option ignored\n", optcode, optptr);
						}
						break;
					}
			case 'x':
					{
						/* check duplication */
						if (keepShorterString(optptr, gNameExcludesStr, gNameExcludesCnt) == NULL) {
							/* count as ONE criterion */
							if (gNameExcludesCnt == 0) gPathNameCriteria++;
							gNameExcludesStr = (char **) realloc(gNameExcludesStr, (++gNameExcludesCnt)*sizeof(char*));
							gNameExcludesStr[(gNameExcludesCnt-1)] = optptr;
						}
						else {
							fprintf(stderr, "-%c%s: redundant option ignored\n", optcode, optptr);
						}
						break;
					}
			case 'y':
					{
						/* check duplication */
						if (keepShorterString(optptr, gNameExcludesBeginStr, gNameExcludesBeginCnt) == NULL) {
							/* count as ONE criterion */
							if (gNameExcludesBeginCnt == 0) gPathNameCriteria++;
							gNameExcludesBeginStr = (char **) realloc(gNameExcludesBeginStr, (++gNameExcludesBeginCnt)*sizeof(char*));
							gNameExcludesBeginStr[(gNameExcludesBeginCnt-1)] = optptr;
						}
						else {
							fprintf(stderr, "-%c%s: redundant option ignored\n", optcode, optptr);
						}
						break;
					}
			case 'z':
					{
						/* check duplication */
						if (keepShorterEndString(optptr, gNameExcludesEndStr, gNameExcludesEndCnt) == NULL) {
							/* count as ONE criterion */
							if (gNameExcludesEndCnt == 0) gPathNameCriteria++;
							gNameExcludesEndStr = (char **) realloc(gNameExcludesEndStr, (++gNameExcludesEndCnt)*sizeof(char*));
							gNameExcludesEndStr[(gNameExcludesEndCnt-1)] = optptr;
						}
						else {
							fprintf(stderr, "-%c%s: redundant option ignored\n", optcode, optptr);
						}
						break;
					}
			case 'm':
					{
					/* match regex in found entry (basename only) */
						if (gFileRegExp == NULL) gPathNameCriteria++;
						gFileRegExp = optptr;
						/* compile the pattern */
						if ((char *)re_compile_pattern(gFileRegExp, strlen(gFileRegExp), &gFilePatternBuffer) != NULL) {
							fprintf(stderr, "error in compiling \"%s\"\n", gFileRegExp);
							gFileRegExp = NULL;
							gPathNameCriteria--;
						}
						break;
					}
			case 'M':
					{
					/* match regex in full path */
						if (gPathRegExp == NULL) gPathNameCriteria++;
						gPathRegExp = optptr;
						/* compile the pattern */
						if ((char *)re_compile_pattern(gPathRegExp, strlen(gPathRegExp), &gPathPatternBuffer) != NULL) {
							fprintf(stderr, "error in compiling \"%s\"\n", gPathRegExp);
							gPathRegExp = NULL;
							gPathNameCriteria--;
						}
						break;
					}

			/* target directory path */
			case 'D':
					{
						if (gTargetDirectory == NULL) gTargetDirectory = optptr;
						break;
					}

		#if 0			/* --- configuration file --- */ // not yet implemented
			case 'f':
					{
						/* e.g.
						 *      -fConfig.cfg
						 */
						break;
					}
		#endif

			/* specify what's included (or not part of) the pathname  */
			case 'C':
					{
						/* check duplication */
						if (keepLongerString(optptr, gPathContainsStr, gPathContainsCnt) == NULL) {
							gPathContainsStr = (char **) realloc(gPathContainsStr, (++gPathContainsCnt)*sizeof(char*));
							gPathContainsStr[(gPathContainsCnt-1)] = optptr;
							gPathNameCriteria++;
						}
						else {
							fprintf(stderr, "-%c%s: redundant option ignored\n", optcode, optptr);
						}
						break;
					}
			case 'X':
					{
						/* check duplication */
						if (keepShorterString(optptr, gPathExcludesStr, gPathExcludesCnt) == NULL) {
							/* count as ONE criterion */
							if (gPathExcludesCnt == 0) gPathNameCriteria++;
							gPathExcludesStr = (char **) realloc(gPathExcludesStr, (++gPathExcludesCnt)*sizeof(char*));
							gPathExcludesStr[(gPathExcludesCnt-1)] = optptr;
						}
						else {
							fprintf(stderr, "-%c%s: redundant option ignored\n", optcode, optptr);
						}
						break;
					}

			/* time matching criteria */
			case 'n':
					{	/* check if specified path exists */
						gPathNewer = optptr;
						if (stat((char *)gPathNewer, &gPathNewerStat) != 0) {
							// error - specified program not found
							gPathNewer = NULL;
						}
						else {
							gTimeStampCriteria++;
						}
						break;
					}
			case 'o':
					{	/* check if specified path exists */
						gPathOlder = optptr;
						if (stat((char *)gPathOlder, &gPathOlderStat) != 0) {
							// error - specified program not found
							gPathOlder = NULL;
						}
						else {
							gTimeStampCriteria++;
						}
						break;
					}

			case 't':
					/* --- modification time constraint --- */
					{
						if (gTimeStampCriteria) {
							gTimeStampCriteria++;
							fprintf(stderr, "-%c%s: redundant option ignored\n", optcode, optptr);
							break;
						}
						/* e.g.
						 *      -t+2h - modified over 2 hours ago
						 *      -t-2W - modified within 2 weeks
						 */
						if (parseTimeConstraint(optcode, optptr) < 0) {
							errflags++;
							break;
						}
						/* set global variables */
						gTimeStampCriteria++;
						gCurrentTime = time(&gCurrentTime);
						break;
					}

			case 's':
					/* --- file size constraint --- */
					{
						char *cp = (char*) strlwr(optptr);
						int  skip = 0;

						if (gFileSizeCriteria) {
							gFileSizeCriteria++;
							fprintf(stderr, "-%c%s: redundant option ignored\n", optcode, optptr);
							break;
						}
						/* e.g.
						 *      -s+2k - size over 2KB
						 *      -s-1M - size less than 1MB
						 *      -s1868b - size = 1868byte
						 */
						if (parseSizeConstraint(optcode, optptr) < 0) {
							errflags++;
							// reset file size constraint
							gFileSizeCriteria = 0;
							break;
						}
						/* set global variables */
						gFileSizeCriteria++;
						break;
					}

			case 'w':
					/* --- filename length constraint --- */
					{
						char *cp = (char*) strlwr(optptr);
						int  skip = 0;

						if (gFileNameLengthCriteria) {
							gFileNameLengthCriteria++;
							fprintf(stderr, "-%c%s: redundant option ignored\n", optcode, optptr);
							break;
						}
						/* e.g.
						 *      -w+12 - filename length longer than 12 bytes
						 *      -w-10 - filename length shorter than 10 bytes
						 *      -w=8  - filename length is 8 bytes
						 *      -w8   - filename length is 8 bytes
						 */
						if (parseNameLengthConstraint(optcode, optptr) < 0) {
							errflags++;
							// reset filename length constraint
							gFileNameLengthCriteria = 0;
							break;
						}
						/* set global variables */
						if (gFileNameLengthCriteria == 0) gPathNameCriteria++;
						gFileNameLengthCriteria++;
						break;
					}

			case 'p':
					/* --- access permission constraint --- */
					{
						if (gPermissionCriteria) {
							gPermissionCriteria++;
							fprintf(stderr, "-%c%s: redundant option ignored\n", optcode, optptr);
							break;
						}
						/* e.g.
						 *      -p+rw - read & write permission set
						 *      -p-x  - execution permission not set
						 *      -pr   - only read permission set
						 */
						//*	affects gPermissionState, gPermissionMode
						if (parsePermissionConstraint(optcode, optptr) >= 0) {
							/* set global variables */
							gPermissionCriteria++;
							gPermissionConstraintStr = optptr;
						}
						break;
					}

			/* junk paths (do not show directories) */
			case 'j':
					gJunkPaths++;
					break;

			/* set limit # of found entries */
			case 'l':
					gLimitEntry = atoi((char *)optptr);
					break;

			/* set directory depth/level limit */
			case 'L':
					gLimitDirLevel = atoi((char *)optptr);
					// implicit recursive
					if (gLimitDirLevel > 1) {
						gRecursive++;
					}
					break;

			/* recursive mode */
			case 'R':
			case 'r':  gRecursive++;	break;
			/* case insensitive mode */
			case 'i':  gIgnoreCase++;	break;
			case 'I':  gIgnoreCase=0;	break;

			/* enable quiet mode */
			case 'q':  gQuietMode++;	break;

#if	defined(unix) || defined(__STDC__)
			/* enable verbose mode
			 * only valid if no debug message
			 */
			case 'V': 
					if (gDebug == 0 && gNulTerminator == 0) { gVerboseMode++; }
					else {
						fprintf(stderr, "verbose mode disabled in debug mode or use NUL terminator\n");
					}
#ifdef  __1_71g__
  #if pre-1.71g
        			ioctl(STDOUT_FILENO, TIOCGWINSZ, &gTerminalSize);
  #endif
#endif  // __1_71g__
					break;
#endif

			/* enable NUL terminator */
			case '0':
					gNulTerminator++;
					// using NUL terminator disables verbose mode
					if (gNulTerminator) { gVerboseMode = 0; }
					break;

			/* set up external program */
			case 'E':
					{
					gCompoundCommand = (char *)optptr;
					gCompoundSymCount = 0;

					/* count number of to-be-substituted symbol */
						{
						char *cp;
						for (cp = gCompoundCommand; *cp; cp++)
							if (*cp == gCompoundSymbol) gCompoundSymCount++;
						}

					/* check if specified external program exists
					 * */
				#ifdef	_MSC_VER
				#if _CHECK_EXTERNAL_PROGRAM_
						if (optptr[1] == ':') {
							if (!IsFile(gCompoundCommand)) {
								gCompoundCommand = NULL;
							}
						}
				#endif	/* _CHECK_EXTERNAL_PROGRAM_ */
				#endif	/* _MSC_VER */
					}
					break;

			/* set debug level */
			case 'd':
					gDebug = atoi((char *)optptr);
					// debug mode disables verbose mode
					if (gDebug) { gVerboseMode = 0; }
					break;

			/* show settings only */
			case 'S':
					gShowSettings++;
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
					fprintf(stderr, "Option %c: not yet implemented.\n", (char)optcode);
					errflags++;
					break;
		}
	}	/* end-of-while */
	/*
	 *** */

	///NOTE: when fds_getopt() returns EOF, optptr is set to the INDEX of next argument
	//
	// e.g. -iraf -cname -Xnova- -V   target-dir
	//      ^[1]  ^[2]   ^[3]    ^[4] ^[5]
	//                                optptr is 5

	/* error handling */
	if (errflags) {
		// version(progname);
		return -1;
	}
	//
	return (int32_t) optptr;
}

static int timeInSeconds(char *str)
{
	uint trange;
	char tunit;
	/* find the time range
	 *
	 *	time := {digit}+(s|m|h|d|w)
	 *
	 *	s - second -
	 *	m - minute = 60 seconds
	 *	h - hour   = 60 minutes
	 *	d - day    = 24 hours
	 *	w - week   = 7 days
	 */
	//-dbg- fprintf(stderr, "timeInSeconds: str=%s\n", str);
	if (sscanf(str, "%d%c", &trange, &tunit) != 2) {
		return -1;
	}

#define	SECONDS_PER_MINUTE	60
#define	MINUTES_PER_HOUR	60
#define	HOURS_PER_DAY		24
#define	DAYS_PER_WEEK		7

	/*
	 * scale up the time range
	 * */
	switch (tunit) {
		case 'w':
		case 'W':
			trange *= DAYS_PER_WEEK;
		case 'd':
		case 'D':
			trange *= HOURS_PER_DAY;
		case 'h':
		case 'H':
			trange *= MINUTES_PER_HOUR;
		case 'm':
		case 'M':
			trange *= SECONDS_PER_MINUTE;
		case 's':
		case 'S':
			/* second is the lowest unit */
			break;
		default:
			return -2;
	}	/* switch */
	//-dbg- fprintf(stderr, "timeInSeconds: trange=%d, tunit=%c\n", trange, tunit);
	return (int) trange;
}

/* parse time constraint parameter
 *
 *	Global variables will be updated:	gTimeDirection & gTimeRange
 *
 */
static int parseTimeConstraint(int option, char *str)
{
	char *ptr = str;
	int rtime;

	/* syntax:
	 *		(+|-)<time>
	 */
	if (*ptr == '-') {
		gTimeDirection = TIME_WITHIN;
		ptr++;
	}
	else if (*ptr == '+')
	{
		gTimeDirection = TIME_OVER;
		ptr++;
	}
	else {
		/* default behaviour */
		gTimeDirection = TIME_WITHIN;
		/* -- error --
		fprintf(stderr, "unknown time direction: %c (expecting '-' or '+')\n", str[0]);
		return -1;
		*/
	}

	if ((rtime = timeInSeconds(ptr)) < 0)
	{
		fprintf(stderr, "(-%c) %s: %s\n", option,
			(rtime == -1) ? "invalid time unit" :
			(rtime == -2) ? "unknown time range" : "invalid option",
			str);
		return rtime;
	}
	gTimeRange = (uint)rtime;
	//-dbg- fprintf(stderr, "parseTimeConstraint: gTimeRange=%d, gTimeDirection=%d\n", gTimeRange, gTimeDirection);
	return 0;
}

static int sizeInBytes(char *str)
{
	uint srange;
	char sunit;
	/* find the size range
	 *
	 *	size := {digit}+(b|k|kb|m|mb|g|gb)
	 *
	 *	b      - byte
	 *	k [kb] - Kbytes
	 *	m [mb] - Mbytes
	 *	g [gb] - Gbytes
	 */
	//-dbg- fprintf(stderr, "sizeInBytes: str=%s\n", str);
#define	_MUST_HAVE_SIZE_UNIT_
#ifdef	_MUST_HAVE_SIZE_UNIT_
	if (sscanf(str, "%d%c", &srange, &sunit) != 2) {
		/* 2005-06-13 I would like to make an exception when the value is only zero */
		return (srange == 0) ? 0 : -1;
	}
#else
	{
		uint rc = sscanf(str, "%d%c", &srange, &sunit);
		if (rc <= 0) { return -1; }
		else if (rc == 1) {
			/* default unit: byte */
			sunit = 'b';
		}
		/* else (rc >= 2) */
	}
#endif	/* _MUST_HAVE_SIZE_UNIT_ */

	/*
	 * scale up the size range
	 * */
	switch (sunit) {
		case 'g':
		case 'G':
			srange *= GBYTE; break;
		case 'm':
		case 'M':
			srange *= MBYTE; break;
		case 'k':
		case 'K':
			srange *= KBYTE; break;
		case 'b':
		case 'B':
			/* byte is the lowest unit */
			break;
		default:
			return -2;
	}	/* switch */
	return (int) srange;
}

/* parse size constraint parameter
 *
 *	Global variables will be updated:	gSizeComparisonSetting
 *	Global variables may  be updated:	gSizeUpperLimit, gSizeLowerLimit, gSizeExact 
 *
 */
static int parseSizeConstraint(int option, char *str)
{
	char *ptr = str;
	char *psize;

	/* initialization */
	gSizeComparisonSetting = SIZE_IN_RANGE;

	/* syntax:
	 *		[+|+=|-|-=|=]<size>
	 *		<size>~<size>
	 */
	if (ptr[0] == '-' && ptr[1] == '=') {
		gSizeComparisonSetting = SIZE_LESS_EQ;
		ptr += 2;
	}
	else if (ptr[0] == '-') {
		gSizeComparisonSetting = SIZE_LESS;
		ptr++;
	}
	else if (ptr[0] == '+' && ptr[1] == '=') {
		gSizeComparisonSetting = SIZE_OVER_EQ;
		ptr += 2;
	}
	else if (ptr[0] == '+') {
		gSizeComparisonSetting = SIZE_OVER;
		ptr++;
	}
	else if (ptr[0] == '=') {
		gSizeComparisonSetting = SIZE_EQUAL;
		ptr++;
	}

	/* next token must be a digit */
	if (!isdigit((int)*ptr)) {
		fprintf(stderr, "(-%c) invalid option: %s\n", option, str);
		return -1;
	}

	/* two acceptable cases:
	 *	1.	single size - <size>
	 *	2.	size range  - <size1>~<size2>
	 */
	/* find '~' or end-of-string */
	for (psize = ptr; *ptr != '~' && *ptr != 0; ptr++) /*no-op*/;
	if (*ptr == 0) {
		//* case1 - only size specified	(same as =<size>)
		if (gSizeComparisonSetting == SIZE_IN_RANGE)
			gSizeComparisonSetting = SIZE_EQUAL;
	}
	if ((*ptr == '~') && (gSizeComparisonSetting != SIZE_IN_RANGE)) {
	    fprintf(stderr, "(-%c) invalid option: %s\n", option, ptr);
		return -1;
    }

	/* gSizeComparisonSetting != SIZE_IN_RANGE indicates a single size value is specified
	 */
	if (gSizeComparisonSetting != SIZE_IN_RANGE)
	{
		/* size has been set */
		int rsize;
		if ((rsize = sizeInBytes(psize)) < 0) {
			fprintf(stderr, "(-%c) invalid size unit: %s\n", option, psize);
			return rsize;
		}
		gSizeExact = (uint) rsize;
		return 0;
	}

	/* now let's deal with size range
	 */
	if (*ptr == '~') {
		//* case2 - lower and upper limits provided
		int rclower, rcupper;
		if ((rclower = sizeInBytes(psize)) < 0) {
			fprintf(stderr, "(-%c) invalid size unit: %s\n", option, psize);
			return rclower;
		}
		++ptr;
		if ((rcupper = sizeInBytes(ptr)) < 0) {
			fprintf(stderr, "(-%c) invalid size unit: %s\n", option, ptr);
			return rcupper;
		}
		//gSizeLowerLimit = (uint) rclower;
		//gSizeUpperLimit = (uint) rcupper;
		gSizeLowerLimit = ((uint) rclower < (uint) rcupper)  ? (uint) rclower : (uint) rcupper;
		gSizeUpperLimit = ((uint) rclower >= (uint) rcupper) ? (uint) rclower : (uint) rcupper;
		return 0;
	}

	/* error condition */
	fprintf(stderr, "(-%c) invalid option: %s\n", option, str);
	return -1;
}

/* parse filename length constraint parameter
 *
 *	Global variables will be updated:	gNameLengthComparisonSetting
 *	Global variables may  be updated:	gNameLengthUpperLimit, gNameLengthLowerLimit, gNameLengthExact 
 *
 */
static int parseNameLengthConstraint(int option, char *str)
{
	char *ptr = str;
	char *psize;

	/* initialization */
	gNameLengthComparisonSetting = SIZE_IN_RANGE;
	gNameLengthExact = 0;

	/* syntax:
	 *		[+|-|=]<length>
	 *		<length>~<length>
	 */
	if (ptr[0] == '-' && ptr[1] == '=') {
		gNameLengthComparisonSetting = SIZE_LESS_EQ;
		ptr += 2;
	}
	else if (ptr[0] == '-') {
		gNameLengthComparisonSetting = SIZE_LESS;
		ptr++;
	}
	else if (ptr[0] == '+' && ptr[1] == '=') {
		gNameLengthComparisonSetting = SIZE_OVER_EQ;
		ptr += 2;
	}
	else if (ptr[0] == '+') {
		gNameLengthComparisonSetting = SIZE_OVER;
		ptr++;
	}
	else if (ptr[0] == '=') {
		gNameLengthComparisonSetting = SIZE_EQUAL;
		ptr++;
	}

	/* next token must be a digit */
	if (!isdigit((int)*ptr)) {
		fprintf(stderr, "(-%c) invalid option: %s\n", option, str);
		return -1;
	}

	/* two acceptable cases:
	 *	1.	single length - <length>
	 *	2.	length range  - <length1>~<length2>
	 */
	/* find '~', '-', or end-of-string */
	for (psize = ptr; *ptr != '~' && *ptr != '-' && *ptr != 0; ptr++) /*no-op*/;
	if (*ptr == 0) {
		//* case1 - only size specified	(same as =<size>)
		if (gNameLengthComparisonSetting == SIZE_IN_RANGE)
			gNameLengthComparisonSetting = SIZE_EQUAL;
	}

	/* gNameLengthComparisonSetting indicates one size value is specified
	 */
	if (gNameLengthComparisonSetting)
	{
		/* length has been set */
		gNameLengthExact = (uint) atoi(psize);
		return 0;
	}

	/* now let's deal with length range
	 */
	if (*ptr == '~') {
		//* case2 - lower and upper limits provided
		gNameLengthLowerLimit = (uint) atoi(psize);
		++ptr;
		gNameLengthUpperLimit = (uint) atoi(ptr);
		return 0;
	}

	/* error condition */
	fprintf(stderr, "(-%c) invalid option: %s\n", option, str);
	return -1;
}

/* parse permission constraint parameter
 *
 *	Global variables will be updated:	gPermissionState, gPermissionMode
 */
static int parsePermissionConstraint(int option, char *str)
{
	char *ptr = str;

	/* syntax:
	 *		(+|-)<mode>
	 *
	 */
	if (*ptr == '-') {
		gPermissionState = MODE_NOT_INCLUDE;
		ptr++;
	}
	else if (*ptr == '+') {
		gPermissionState = MODE_TO_INCLUDE;
		ptr++;
	}
	else if (*ptr == '=') {
		gPermissionState = MODE_TO_MATCH;
		ptr++;
	}
	else {
		gPermissionState = MODE_TO_MATCH;
	}
	/* which access permission are of interest
	 */
	gPermissionMode = 0;
	for (/*no-op*/; *ptr != 0; ++ptr) {
		if (*ptr == 'r')      { gPermissionMode |= S_IREAD; }
		else if (*ptr == 'w') { gPermissionMode |= S_IWRITE; }
		else if (*ptr == 'x') { gPermissionMode |= S_IEXEC; }
		else {
			/* error */
			gPermissionMode = 0;
			fprintf(stderr, "(-%c) invalid option: %s\n", option, str);
			return -1;
		}
	}
	return 0;
}

/* compose the external command
 */
static char *composeExternalCommand(const char* fullpath)
{
	char *ncCommand = NULL;
	int   nbufsize;

	if (gDebug > 6) fprintf(stderr, "cCommand=%s fullpath=%s\n", gCompoundCommand, fullpath);

	/* simple execution command */
	if (gCompoundSymCount == 0)
	{
		/* allocate compound command buffer */
		nbufsize = strlen(gCompoundCommand) + strlen(fullpath) + 4;	/* space + 2 double-quotes (") + NULL */
		ncCommand = (char *)malloc(nbufsize);
		memset(ncCommand, 0, nbufsize);
		/* create new external command */
		sprintf(ncCommand, "%s \"%s\"", gCompoundCommand, fullpath);
	}
	/* compound command */
	else 
		/*
		 *	gCompoundSymbol will be replaced with the given path
		 *
		 *	TODO:
		 *		%:r - root of path (file.ext -> file)
		 */
	{
		// create new buffer and replace every % with fullpath 
		char *cp, *ncp;

		/* compute buffer size to hold expanded compound command (plus NULL)
		 * every pathname will be wrapped with double-quote (")
		 * */
		nbufsize = strlen(gCompoundCommand) + (gCompoundSymCount * strlen(fullpath)) + (gCompoundSymCount * 2) + 1;
		if (gDebug > 6) fprintf(stderr, "%d symbol (%c) found (size=%d)\n", gCompoundSymCount, gCompoundSymbol, nbufsize);

		/* allocate compound command buffer */
		ncp = ncCommand = (char *)malloc(nbufsize);
		memset(ncCommand, 0, nbufsize);

		/* create new command by parsing through the given compound command */
		for (cp = gCompoundCommand; *cp; cp++)
			{
			if (*cp == gCompoundSymbol) {
				if (gDebug > 6) fputs(fullpath, stdout);
				/* replaced fullpath will be enbraced in double-quote */
				*ncp++ = '"';
				if (*(cp+1) == ':' && *(cp+2) == 'r')
				{
					char *ecp, *xcp;
					int  elen;

					/* skip over ":r" */
					cp += 2;

						/* find last path element */
					ecp = strrchr(fullpath, (int)gPathDelimiter);
					if (ecp == NULL) { ecp = (char *)fullpath; }
						/* find the extension */
					xcp = strrchr(ecp, (int)'.');

					/* determine how many characters to copy */
					elen = (xcp == NULL) ? strlen(fullpath) : (xcp - fullpath);

					/* compound symbol + ":r" will be replaced with full path w/o extension
					 * */
					strncat(ncCommand, fullpath, elen);
					ncp += elen;

				}
				else {
					/* compound symbol will be simply replaced with found full path
					 * */
					strncat(ncCommand, fullpath, strlen(fullpath));
					ncp += strlen(fullpath);
				}
				/* replaced fullpath will be enbraced in double-quote */
				*ncp++ = '"';
			}
			else {
				/* other characters will be preserved */
				if (gDebug > 6) putc((int)*cp, stdout);
				*ncp++ = *cp;
			}
		}
		/* terminate with \0 */
		*ncp = 0;
		if (gDebug > 6) putchar((int)'\n');
	}

	return ncCommand;
}

/* Find a (partially) matched string in the "array".
 * If the "str" contains one of the elememnt in the "array" (longer), the element will be replaced.
 */
static char *keepLongerString(char *str, char **array, int count)
{
	int ix;
	uint slen = strlen(str);
	for (ix = 0; ix < count; ix++) {
		/* if str is a sub-string of the current element, we punt */
		//-dbg- printf("str=%s, array[%d]=%s\n", str, ix, array[ix]);
		if (strstr(array[ix], str)) return str;

		/* if the current element is a sub-string of the str, REPLACE it */
		if ((strlen(array[ix]) < slen) && strstr(str, array[ix])) {
			char *sptr;
			sptr = array[ix];
			array[ix] = str;
			//-dbg- printf("str=%s, array[%d]=%s\n", str, ix, array[ix]);
			return sptr;
		}
	}
	return NULL;
}

/* Find a (partially) matched string in the "array".
 * If the "str" is being contained by one of the elememnt in the "array" (shorter), the element will be replaced.
 *
 * return NULL indicates the "str" is an unique pattern
 *
 * In exclusion case, a shorter string should be used for matching,
 * e.g. when given excluding patterns are "string" & "str", using "str" can match all of the following but "string" matches only one
 *	- string_a
 *	- a_str
 *	- not_strong_case
 */
static char *keepShorterString(char *str, char **array, int count)
{
	int ix;
	uint slen = strlen(str);
	for (ix = 0; ix < count; ix++) {
		/* if str is a sub-string of the current element, we punt */
		//-dbg- printf("str=%s, array[%d]=%s\n", str, ix, array[ix]);
		if (strstr(str, array[ix])) return str;

		/* if the str is a sub-string of the current element, REPLACE the element */
		if (((strlen(array[ix]) > slen) && strstr(array[ix], str))) {
			char *sptr = array[ix];
			array[ix] = str;
			//-dbg- printf("str=%s, array[%d]=%s\n", str, ix, array[ix]);
			return sptr;
		}
	}
	// the given str is considered an *UNIQUE* pattern
	return NULL;
}

/* Find a (partially) matched-from-the-end string in the "array".
 * If the "str" is being contained by one of the elememnt in the "array" (shorter), the element will be replaced.
 *
 * return NULL indicates the "str" is an unique pattern
 *
 * In exclusion case, a shorter string should be used for matching,
 * e.g. if given excluding patterns ".cpp" and ".c", they must be treated differently
 #	  but "xx" will cover ".cxx" and ".hxx" patterns
 */
static char *keepShorterEndString(char *str, char **array, int count)
{
	int ix;
	uint slen = strlen(str);
	for (ix = 0; ix < count; ix++) {
		/* if str is a sub-string of the current element, we punt */
		//-dbg- printf("str=%s, array[%d]=%s\n", str, ix, array[ix]);
		uint alen = strlen(array[ix]);

		// ignore the str if
		// - the str is longer or of same length, and
		// - array element pattern matches the str from the end
		if ((slen >= alen) &&
			(strncmp(&str[slen-alen], array[ix], alen) == 0)) {
			return str;
		}

		// replace the str if
		// - the str is shorter and
		// - the str matches array element pattern from the end
		/* if the str is a sub-string of the current element, REPLACE the element */
		if ((slen < alen) &&
			(strncmp(str, &array[ix][alen-slen], slen) == 0)) {
			char *sptr = array[ix];
			array[ix] = str;
			//-dbg- printf("str=%s, array[%d]=%s\n", str, ix, array[ix]);
			return sptr;
		}
	}
	// the given str is considered an *UNIQUE* pattern
	return NULL;
}

/* Change every element in the "array" to lowercase string
 */
static void lowerStrings(char **array, int count)
{
	int ix;
	for (ix = 0; ix < count; ix++) {
		if (array[ix]) strlwr(array[ix]);
	}
	return;
}

/* Find how many (partially) matches can be found in the "array"
 */
static int matchStrings(char *str, char **array, int count)
{
	int ix, match;
	for (ix = 0, match = 0; ix < count; ix++) {
		if (array[ix] && strstr(str, array[ix])) { match++; }
	}
	return match;
}

/* Cancel conflict matching pattern in "containsStr" and "excludesStr" arrays
 */
static void cancelConflicts(char **containsStr, uint containsCnt, char **excludesStr, unsigned int excludesCnt)
{
	uint cix, eix;

	for (cix = 0; cix < containsCnt; cix++) {
		for (eix = 0; eix < excludesCnt; eix++) {
			/* CONTAINS pattern takes precedence!!!
			 */
			if (strstr(containsStr[cix], excludesStr[eix]) != 0) {
				excludesStr[eix] = NULL;
				break;
			}
		}
	}
	return;
}

/* Convert a numeric value to string representation with comma(s) inserted
 */
static char *numericToString(size_t value, char *buffer, uint bufsize)
{
	char sCompact[12];
	char *pCompact = sCompact, *pExtended = buffer;
	uint lenCompact;
	uint commas, leads;
	uint ix;

// binary to string
	sprintf(sCompact, "%zu", value);
	//-dbg- printf("value=%zu, sCompact=%s\n", value, sCompact);

// compute related parameters
	lenCompact = strlen(sCompact);
	commas = (lenCompact-1)/3;			// # of comma(s) to insert
	leads  = lenCompact % 3;
	leads += (leads==0) ? 3 : 0;		// # of digits before 1st comma
	//-dbg- printf("len=%u, commas=%u, leads=%u\n", lenCompact, commas, leads);

// prepare extended buffer
		// prefill with blanks
	memset(buffer, (int)' ', bufsize);
		// move ahead pointer
	pExtended += (bufsize - (lenCompact+commas+1));		// +1 for NUL-terminator

// copy sCompact to buffer with comma(s)
	for (ix = 0; ix < leads; ix++) {
		*pExtended++ = *pCompact++;
	}
	for (ix = 0; ix < commas; ix++) {
		*pExtended++ = ',';
		*pExtended++ = *pCompact++;
		*pExtended++ = *pCompact++;
		*pExtended++ = *pCompact++;
	}
		// NUL-terminate the buffer
	buffer[(bufsize-1)] = 0;
	//-dbg- printf("sExtended=%s\n", buffer);
	return buffer;
}

#if	0
// ---------------------------------------------------------------------------
//	replaced by IsDirectory, IsFile, and IsValidPath in dirinfo.c
//
static int check_IfDirectory(char *path)
{
	struct _stat fstat;
	int  len = strlen(path);
	
	/* condense the directory path */
	/* remove trailing slash (/) */
	if
#ifdef	_MSC_VER
		(path[len-1] == '/' || path[len-1] == '\\')
#else
		(path[len-1] == '/')
#endif
		{
			path[len-1] = 0;
		}
	
	if (gDebug > 9) fprintf(stderr, "root path= %s\n", path);
	
	/* handle drive root case - [C:] */
	if (strlen(path) == 2 && path[1] == ':')
		return 1;
	
	/* check if specified directory exists */
	if (_stat(path, &fstat) != 0) {
		// error - specified directory not found
		if (gDebug > 2) fprintf(stderr, "error - %s specified directory not found\n", path);
		return 0;
	}
	else if (!(fstat.st_mode & _S_IFDIR)) {
		// error - not a directory entry
		if (gDebug > 2) fprintf(stderr, "error - %s not a directory\n", path);
		return 0;
	}
	
	return 1;
}
static int check_IfFile(char *path)
{
	struct _stat fstat;
	/* check if specified file entry exists */
	if (_stat(path, &fstat) != 0) {
		// error - specified file entry not found
		if (gDebug > 2) fprintf(stderr, "error - %s specified file entry not found\n", path);
		return 0;
	}
	else if (!(fstat.st_mode & _S_IFREG)) {
		// error - not a file entry
		if (gDebug > 2) fprintf(stderr, "error - %s not a file entry\n", path);
		return 0;
	}
	return 1;
}
// ---------------------------------------------------------------------------
#endif

