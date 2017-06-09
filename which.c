//
// which.c
//
// Revision History:
//   T. David Wong		02-17-2004    Initial Version
//   T. David Wong		10-22-2004    Enabled searching non-standard extension; Added more comments
//   T. David Wong		01-17-2005    Added icon to to executable (change was actually made in which.nmak)
//   T. David Wong		03-13-2005    Added -A, -P options, deleted -1 option
//   T. David Wong		03-23-2005    Extented extension list to include $PATHEXT
//   T. David Wong		03-30-2012    Compiled on Mac OS/X
//

#define _COPYRIGHT_	"(c) 2004,2005 Tzunghsing David <wong>"
#define	_DESCRIPTION_	"Find matched executable in the search path"
#define _PROGRAMNAME_	"which"	/* program name */
#define _PROGRAMVERSION_	"0.32c"	/* program version */

#include <stdio.h>
#include <stdlib.h>	// getenv
#include <string.h>	// strstr

#include "mygetopt.h"
#include "dirinfo.h"

#ifdef	_MSC_VER
#define	PATH_DELIMITER	';'
#else
#define	PATH_DELIMITER	':'
#endif

/* global variables
 */
unsigned int gFound = 0;			/* total number of matches found */
unsigned int gDebug = 0;
unsigned int gIgnoreExtension =		/* whether excluding directory entry's extension during comparing or not */
#ifdef	_WIN32
						0;			/* extension in Microsft Win32 environment */
#else
						1;
#endif
#define	EXACT_MATCH		0x01
#define	PARTIAL_MATCH	0x02
unsigned int gListAllMatches = 0;
unsigned int gHasKnownExtension = 0;		/* indicates if given target has known extension */

/* known executable extension
 */
static char *sExtensions = NULL;			/* allocated space for $PATHEXT & built-in extensions */
static char *sBuiltInExts = ".exe;.com;.bat;.sh;.zsh;.pl";

/* static functions
 */
static int matchString_Callback(const char *filename, const char *fullpath, struct stat *statp, void *voidp);
static int searchForProgram(char *where, char *what);
static void showMatch(const char *path);
static int compareStrings(const char *strA, const char *strB, int type);
static int compareExtension(char *target, char* extList);
static void showOptions(void);

/*
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
	fprintf(stderr, "Usage: %s [options] <name>\n", progname);
	fprintf(stderr, "  -h  help\n");
	fprintf(stderr, "  -v  version\n");
	fprintf(stderr, "  -i  ignore extension when searching matches\n");
	fprintf(stderr, "  -a  list all matches\n");
	fprintf(stderr, "  -A  list all matches (incl. partial matches)\n");
	if (detail)
	{
	fprintf(stderr, "  -V  list all search paths while searching\n");
	fprintf(stderr, "  -P  set artificial PATH\n");
	if (sExtensions) fprintf(stderr, "known extensions: %s\n", sExtensions);
	fprintf(stderr, "Example:\n");
	fprintf(stderr, "  %s -i sort\n", progname);
	fprintf(stderr, "  %s -ia sort.exe\n", progname);
	}
	return;
}


/* main program
 */
int main(int argc, char** argv, char **envp)
{
	char *progname = argv[0];
	int optcode;
	char *optptr;
	char *pathENV = "PATH";
	unsigned int listPaths = 0;
	char *pathp, *nextp;
	char *target;

	if (argc <= 1) {
		usage(progname, 0); return(-1);
	}

	// parameter parsing
	while ((optcode = fds_getopt(&optptr, "?hviaAVP:", argc, argv)) != EOF)
	{
		switch (optcode) {
			case '?':
			case 'h':
					usage(progname, 0); return(-1);
					break;
			case 'v':
					version(progname); return(-1);
					break;

			case 'i':
					gIgnoreExtension++;
					break;
			case 'A':
					gListAllMatches |= (EXACT_MATCH | PARTIAL_MATCH);
					break;
			case 'a':
					gListAllMatches |= EXACT_MATCH;
					break;
			case 'V':
					listPaths++;
					break;
			case 'P':
					pathENV = optptr;
					break;

			case OPT_SPECIAL:
					if (strcmp(optptr, "help") == 0) {
						version(progname); usage(progname, 1);
					}
					else if (strcmp(optptr, "version") == 0) {
						version(progname);
					}
					return(-1);
			case OPT_UNKNOWN:
					fprintf(stderr, "unknown option: %s\n", optptr);
					return(-1);
			default:
					fprintf(stderr, "-%c: NOT YET IMPLEMENTED!\n", (char) optcode);
					return(-1);
		}
	}	/* end-of-while */
	
	/* argv[1] is the target directory */
	target = argv[(int) optptr];

	if (target == NULL) {
		usage(progname, 0); return(-1);
	}

	/* set up list of executable extensions */
	{	char *extp;
		sExtensions = strdup(sBuiltInExts);
		if ((extp = getenv("PATHEXT")) != NULL) {
			/* concatenate built-in with $PATHEXT */
			sExtensions = realloc(sExtensions, (strlen(sExtensions)+strlen(extp)+1));
			strcat(sExtensions, extp);
		}
	}

	//-dbg- showOptions();

	/* check if the target already has extension
	 */
	//-dbg- fprintf(stderr, "target=%s (sExtensions=%s)\n", target, sExtensions);
	gHasKnownExtension = compareExtension(target, sExtensions);

	//-dbg-
	// { /* dump ALL environment variables */
	// int ix;
	// for (ix = 0; envp[ix] != 0; ix++)
	//	fprintf(stderr, "env[%d]: %s\n", ix, envp[ix]);
	// }

	// "path" contains exact same thing as "PATH"
	pathp = getenv(pathENV);
	//-dbg- if (pathp) printf("PATH=%s\n", pathp);

	/* dissect the entire PATH and search for the target
	 */
	while (pathp)
	{
		/* find next search path element */
		if ((nextp = strchr(pathp, (int)PATH_DELIMITER)) != 0) {
			*nextp = 0;
			++nextp;
		}
		if (listPaths) fprintf(stdout, "Path= %s\n", pathp);
		gFound += searchForProgram(pathp, target);
		/* search no more if
		 * 1. at least one match is found
		 * 2. no "list all" request
		 */
		//-dbg- fprintf(stderr, "(so far) gFound= %d (next=%s)\n", gFound, nextp);
		if (gFound && (gListAllMatches == 0))
			return 0;
		pathp = nextp;
	}
	/* last element */
	//-dbg- printf("PathElement= %s\n", pathp);

	/* clean up */
	if (sExtensions) free(sExtensions);

	return 0;
}

//* this is the callback routine that will be passed to dirinfo_Find
//*
static unsigned int sMatchedItem = 0;	/* shared between matchString_Callback() and searchForProgram() */
static int matchString_Callback(const char *filename, const char *fullpath, struct stat *statp, void *voidp)
{
	matchCriteria_t *mcbuf = (matchCriteria_t *)voidp;
	char *sFile = mcbuf->cparam.str;	/* target file in search */
	int  match = 0;

	//-dbg- fprintf(stderr, "filename=%s cparam.str=%s\n", filename, sFile);
	if (gHasKnownExtension)
	{
		/* exact match is required when the target has known extension */
		if (compareStrings(filename, sFile, EXACT_MATCH) == 1) match++;
	}
	else
	{
		/* many possibilities when the target has no extension
		 */
		char *dotp = NULL;
		if (filename[0] != '.') {
			/* filename with leading dot will not be changed */
			dotp = strrchr(filename, (int)'.');
		}
		
		if (gIgnoreExtension) {
			/* ignore the extension when comparing
			 */
			if (dotp) { *dotp = 0; }
			/* now, filename has no extension */

			if (compareStrings(filename, sFile,
					(gListAllMatches & PARTIAL_MATCH) ? PARTIAL_MATCH : EXACT_MATCH) == 1)
				match++;
		}
		else if (dotp) {
			/* handle only known extension
			 */
			if (compareExtension((char *)filename, sExtensions) != 0) {
				*dotp = 0;
				if (compareStrings(filename, sFile,
						(gListAllMatches & PARTIAL_MATCH) ? PARTIAL_MATCH : EXACT_MATCH) == 1)
					match++;
			}
		}	/* else */
	}

	/* do we have a match? */
	if (match) {
//		if (sMatchedItem) {
//			/* are we asked to show only first match? */
//			return 0;
//		}
		showMatch(fullpath);
		sMatchedItem += match;
		return 0;
	}
	else return -1;
}

static int searchForProgram(char *where, char *what)
{
	matchCriteria_t mcbuf = { 0 };

	mcbuf.proc = matchString_Callback;
	mcbuf.cparam.str = what;
	sMatchedItem = 0;
	dirinfo_Find(where, NULL /*collect no directory information*/, &mcbuf, 0 /*non-recursive*/);
	return sMatchedItem;
}

static void showMatch(const char *path)
{
	/* dont print if
	 * 1. one or more match has been found
	 * 2. no "list all" request
	 */
	if (gFound && (gListAllMatches == 0)) {
		//-dbg-
		fprintf(stdout, "~ %s\n", path);
		return;
	}
	//
	fprintf(stdout, "%s\n", path);
}

static int compareStrings(const char *strA, const char *strB, int type)
{
	unsigned int lenA = strlen(strA);
	unsigned int lenB = strlen(strB);

	//-dbg- fprintf(stderr, "strA=%s (%d) strB=%s (%d) type=%d\n", strA, lenA, strB, lenB, type);

	if (type == EXACT_MATCH) {
		if ((lenA == lenB) && strnicmp(strA, strB, lenB) == 0) return 1;
	}
	else if (type == PARTIAL_MATCH) {
		if (strnicmp(strA, strB, lenB) == 0) return 1;
	}
	return 0;
}

int compareExtension(char *target, char* extList)
{
	char *dotp;
	if ((dotp = strrchr(target, (int)'.')) == 0)
		return 0;
	/* search list of extensions */
	return strstr(extList, dotp) ? 1 : 0;
}

static void showOptions(void)
{
	fprintf(stderr, "--- options ---\n");
	fprintf(stderr, "gFound = %d\n", gFound);
	fprintf(stderr, "gDebug = %d\n", gDebug);
	fprintf(stderr, "gIgnoreExtension = %s\n", gIgnoreExtension ? "TRUE" : "FALSE");
	fprintf(stderr, "gListAllMatches  = 0x%x\n", gListAllMatches); 
	fprintf(stderr, "---------------\n\n");
}

