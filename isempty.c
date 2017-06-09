//
// isempty.c
//
// Revision History:
//   T. David Wong		02-07-2004    Initial Version
//   T. David Wong		02-13-2004    Re-designed command options
//   T. David Wong		03-10-2004    Fixed crash when there is no directory to check
//   T. David Wong		01-17-2005    added icon to to executable (change was actually made in isempty.nmak)
//   T. David Wong		03-22-2005    used extened functioality in dirinfo module
//   T. David Wong		08-25-2007    added is empty file handling
//
// TODO:
//   - accept multiple arguments - This may not be a good idea (02/18/04)

#define _COPYRIGHT_	"(c) 2004,2005 Tzunghsing David <wong>"
#define	_DESCRIPTION_	"Check if the target directory/file is an empty directory tree or file"
#define _PROGRAMNAME_	"isempty"	/* program name */
#define _PROGRAMVERSION_	"0.60d"	/* program version */

#include <stdio.h>
#include <sys/stat.h>

#include "dirinfo.h"
#include "mygetopt.h"

#define	NONEMPTY_DIR	0
#define	IS_EMPTY_DIR	1
#define	NONEMPTY_FILE	NONEMPTY_DIR
#define	IS_EMPTY_FILE	IS_EMPTY_DIR

/* global variables
 */
unsigned int gVerbose = 0;
unsigned int gQuiet = 0;
char *gTargetDir = NULL;

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
	fprintf(stderr, "Usage: %s [options] <target directory>\n", progname);
	fprintf(stderr, "  -h  help\n");
	fprintf(stderr, "  -v  version\n");
	fprintf(stderr, "  -q  quiet mode\n");
	fprintf(stderr, "  -V  verbose - show what return value will be\n");
	if (detail)
	{
	fprintf(stderr, "\nReturn value in quiet mode (i.e. %%errorlevel%%):\n");
	fprintf(stderr, "  0   one or more file(s) exists within the target directory\n");
	fprintf(stderr, "  1   the target directory is an empty directory tree\n");
	fprintf(stderr, "Example:\n");
	fprintf(stderr, "  %s %%TEMP%%\n", progname);
	fprintf(stderr, "  %s -v %%TEMP%%\n", progname);
	}
	return;
}

int attrCallback(const char *filename, const char *fullpath, struct stat *statp, void *opaquep)
{
	if (! S_ISDIR(statp->st_mode)) {
		/* NON-directory entry found... PUNT */
		if (gQuiet) exit(NONEMPTY_DIR);
		if (gVerbose) fprintf(stdout, "%s: non-empty directory.\n", gTargetDir);
		else fprintf(stdout, "false\n");
		exit(0);
	}
	return (0);
}


/* main program
 */
int 
main(int argc, char **argv)
{
	char *progname = argv[0];
	int optcode;
	char *optptr;
	dirInfo_t dibuf = { 0 };	/* summary information */
	matchCriteria_t mcbuf = { 0 };

	if (argc <= 1) {
		usage(progname, 0); return(-1);
	}

	// parameter parsing
	while ((optcode = fds_getopt(&optptr, "?hvVq", argc, argv)) != EOF)
	{
		switch (optcode) {
			case '?':
			case 'h':
					usage(progname, 0); return(-1);
					break;
			case 'v':
					version(progname); return(-1);
					break;

			case 'V':
					gVerbose++;
					break;
			case 'q':
					gQuiet++;
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
					fprintf(stderr, "unimplemented option: %c\n", (char)optcode);
					return(-1);
		}
	}	/* end-of-while */

	mcbuf.proc = attrCallback;	/* callback routine */

	if ((int) optptr >= argc) {
		usage(progname, 0); return(-1);
	}

	/* argv[1] is the target directory */
	gTargetDir = argv[(int) optptr];
	/* debug
	fprintf(stderr, "argc=%d, gTargetDir=%s\n", argc, gTargetDir);
	*/

	if (IsFile(gTargetDir))
	{
		int eStatus;
		struct stat sb;
		stat(gTargetDir, &sb);
		/* sb.st_size is the file size */
		eStatus = (sb.st_size == 0) ? IS_EMPTY_FILE : NONEMPTY_FILE;

		if (gQuiet) exit(eStatus);
		if (gVerbose) fprintf(stdout, "%s:%s an empty file.\n", gTargetDir, (eStatus==IS_EMPTY_FILE)?"":" not");
		else fprintf(stdout, "%s\n", (eStatus==IS_EMPTY_FILE)?"true":"false");
	}
	else if (IsDirectory(gTargetDir))
	{
		/* traverse the target directory */
		dirinfo_Find(gTargetDir, &dibuf, &mcbuf, (int) 1);
		/* should the funtion return, we know there was NO file found in this directory tree */

		if (gQuiet) exit(IS_EMPTY_DIR);
		if (gVerbose) fprintf(stdout, "%s: an empty directory.\n", gTargetDir);
		else fprintf(stdout, "true\n");
	}
	else {
		/* this entry is not a file, nor a directory */
		return(-1);
	}

	exit (0);
}

