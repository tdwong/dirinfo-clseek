//
// ifTest.c
//
// Revision History:
//   T. David Wong		03-17-2004    Initial Version
//
// TODO:
//   - use PATHEXT for executable extension
/*
	E:>echo %PATHEXT%
	.COM;.EXE;.BAT;.CMD;.VBS;.VBE;.JS;.JSE;.WSF;.WSH
*/
//

#define _COPYRIGHT_	"(c) 2004, Tzunghsing David <wong>"
#define	_DESCRIPTION_	"Test File/Dir/Drive Existence"
#define _PROGRAMNAME_	"ifTest"	/* program name */
#define _PROGRAMVERSION_	"0.2b"	/* program version */

#include <stdio.h>
// #include <stdlib.h>	// getenv
#include <string.h>	// strstr
#include <direct.h>	// getcwd
#include <sys/types.h>	// stat
#include <sys/stat.h>	// stat

/* macros
 */
#ifdef	_MSC_VER
#include <windows.h>
/* local MSC-specific macros */
#define	S_ISDIR(mode)	(mode & S_IFDIR)
#define	S_ISREG(mode)	(mode & S_IFREG)
#define	S_ISCHR(mode)	(mode & S_IFCHR)
#define	S_ISFIFO(mode)	(mode & _S_IFIFO)
#define	S_ISREAD(mode)	(mode & S_IREAD)
#define	S_ISWRITE(mode)	(mode & S_IWRITE)
#define	S_ISEXEC(mode)	(mode & S_IEXEC)
#endif	/* _MSC_VER */

/* define constants
 */
#ifndef	PATH_MAX
#define	PATH_MAX	512
#endif
// the ERRORLEVELs for the drive types
#define	RC_ERROR	0
#define	RC_DRIVE_UNKNOWN		1
#define	RC_DRIVE_NO_ROOT_DIR		8
#define	RC_DRIVE_REMOVABLE		2
#define	RC_DRIVE_FIXED			3
#define	RC_DRIVE_REMOTE			4
#define	RC_DRIVE_CDROM			5
#define	RC_DRIVE_RAMDISK		6
#define	RC_ITEM_DOESNT_EXIST		7	// ERRORLEVEL of 7 indicates that the item does not exist

/* global variables
 */
unsigned int gQuiet = 0;			/* quiet mode */
unsigned int gDebug = 0;

/* static functions
 */
static char *extendToFullPath(char *testItem, char iFullPath[]);

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
	fprintf(stderr, "Usage: %s [option] <name>\n", progname);
	fprintf(stderr, "Only one option is allowed.\n");
	fprintf(stderr, "  -h   show this help\n");
	fprintf(stderr, "  -v   show version\n");
	fprintf(stderr, "  -D   check existence of a DRIVE (no UNC path)\n");
	fprintf(stderr, "  -d   check existence of a DIRECTORY\n");
	fprintf(stderr, "  -e   check existence of a FILE or DIRECTORY\n");
	fprintf(stderr, "  -f   check existence of a regular FILE\n");
	fprintf(stderr, "  -r   check existence of a readable FILE\n");
	fprintf(stderr, "  -w   check existence of a writable FILE\n");
	fprintf(stderr, "  -x   check existence of a executable FILE\n");
	fprintf(stderr, "  -s   check existence of a size greater than zero FILE\n");
	fprintf(stderr, "  -z   check existence of a size is zero FILE\n");
	fprintf(stderr, "  -c   check existence of a character special FILE\n");
	fprintf(stderr, "  -p   check existence of a named pipe\n");
	fprintf(stderr, "  -q   quiet mode\n");
	if (detail)
	{
	fprintf(stderr, "  -g#  set debug level\n");
	fprintf(stderr, "Example:\n");
	fprintf(stderr, "  %s -d c:\\somedir\n", progname);
	fprintf(stderr, "  %s -f c:\\somedir\\somefile.txt\n", progname);
	fprintf(stderr, "  %s -D c:\\\n", progname);
	}
	return;
}


/* main program
 */
int main(int argc, char** argv, char **envp)
{
	char *progname = argv[0];
	int  rc;
	char *testOpt = NULL, *testItem = NULL;
	char iFullPath[PATH_MAX];
	struct stat     sb;

	/* check # of arguments
	 */
	if (argc < 2) {
		usage(progname, 0); return(RC_ERROR);
	}

	/* parse arguments
	 */
	while (argc > 1)
	{
		/* handle help request
		 */
		if ((strncmp(argv[1], "-?", 2) == 0) || (strncmp(argv[1], "-h", 2) == 0) ||
			(strncmp(argv[1], "/?", 2) == 0) || (strncmp(argv[1], "/h", 2) == 0))
		{
			usage(progname, 0); return(RC_ERROR);
		}
		else if (stricmp(argv[1], "--help") == 0)
		{
			usage(progname, 1); return(RC_ERROR);
		}
		else if ((strncmp(argv[1], "-v", 2) == 0) || (strncmp(argv[1], "/v", 2) == 0))
		{
			version(argv[0]); return(RC_ERROR);
		}

		/* handle special control request
		 */
		else if ((strncmp(argv[1], "-q", 2) == 0) || (strncmp(argv[1], "/q", 2) == 0))
		{
			gQuiet++;
			++argv; --argc; continue;
		}
		else if ((strncmp(argv[1], "-g", 2) == 0) || (strncmp(argv[1], "/g", 2) == 0))
		{
			gDebug = atoi(&argv[1][2]);
			++argv; --argc; continue;
		}

		/* handle normal option
		 */
		else {
			//-dbg- fprintf(stderr, "argv[1]=%s, argv[2]=%s\n", argv[1], argv[2]);
			/* we can check only one item at a time */
			if (testOpt != NULL) {
				usage(progname, 0); return(RC_ERROR);
			}
			/* remember the option and target item */
			if (argc == 2) {
				/* one argument left. no option provided, use default */
				testOpt = "-f";
				testItem = argv[1];
				break;
			}
			else {
				testOpt = argv[1];
				testItem = argv[2];
				argv += 2; argc -= 2;
				continue;
			}
		}
	}	/* while (argc > 1) */

	//-dbg- fprintf(stderr, "testOpt=%s, testItem=%s\n", testOpt, testItem);

	/* validate provided option */
	if (testOpt[0] == '-'
#if (_BUILD_==MSVC)
		|| testOpt[0] == '/'
#endif
	) { ++testOpt; }
	else {
		usage(progname, 0); return(RC_ERROR);
	}

	/* convert target into internal format */
	if (extendToFullPath(testItem, iFullPath) == NULL) {
		if (!gQuiet) fprintf(stdout, "%s does not exist (no full path)\n", testItem);
		return RC_ITEM_DOESNT_EXIST;
	}

	/* handle drive query */
	if (strcmp(testOpt, "D") == 0) {
		//* UNC path such as \\remoteserver\share format doesn't have a drive
		if (strncmp(iFullPath, "\\\\", 2) == 0) {
			if (!gQuiet) fprintf(stdout, "%s - drive unknown for UNC path\n", testItem);
			return RC_DRIVE_UNKNOWN;
		}
		if (!gQuiet) fprintf(stdout, "%s", testItem);
		testItem = iFullPath;
	}
	/* handle other options */
	else {
		/* retrieve item state
		 */
		//-dbg- fprintf(stderr, "testOpt=%s, testItem=%s, iFullPath=%s\n", testOpt, testItem, iFullPath);
		if ((strcmp(testOpt, "d") == 0) && (strncmp(iFullPath, "\\\\", 2) == 0)) {
			/* trailing backslash is a must for UNC directory */
			if (iFullPath[strlen(iFullPath)-1] != '\\') { strcat(iFullPath, "\\"); }
			//-dbg- fprintf(stderr, "iFullPath=%s\n", iFullPath);
		}
		if (stat(iFullPath, &sb) != 0)
		{
			if (!gQuiet) fprintf(stdout, "%s does not exist (stat failed)\n", testItem);
			return RC_ITEM_DOESNT_EXIST;
		}
		//-dbg- fprintf(stderr, "mode=0x%X\n", sb.st_mode);
//
#if	0	/* found in sys/stat.h */
#define _S_IFMT         0170000         /* file type mask */
#define _S_IFDIR        0040000         /* directory */
#define _S_IFCHR        0020000         /* character special */
#define _S_IFIFO        0010000         /* pipe */
#define _S_IFREG        0100000         /* regular */
#define _S_IREAD        0000400         /* read permission, owner */
#define _S_IWRITE       0000200         /* write permission, owner */
#define _S_IEXEC        0000100         /* execute/search permission, owner */
#endif

		/* handle meaning option
		 */
		if (strcmp(testOpt, "e") == 0) {
			if (!gQuiet) fprintf(stdout, "%s", testItem);
		}
		else if (strcmp(testOpt, "f") == 0) {
			if (S_ISREG(sb.st_mode)) {
				if (!gQuiet) fprintf(stdout, "%s: File", testItem);
			}
			else {
				if (!gQuiet) fprintf(stdout, "%s: File doesn't exist\n", testItem);
				return RC_ITEM_DOESNT_EXIST;
			}
		}
		else if (strcmp(testOpt, "r") == 0) {
			if (S_ISREAD(sb.st_mode)) {
				if (!gQuiet) fprintf(stdout, "%s: File", testItem);
			}
			else {
				if (!gQuiet) fprintf(stdout, "%s: File is not readable\n", testItem);
				return RC_ITEM_DOESNT_EXIST;
			}
		}
		else if (strcmp(testOpt, "w") == 0) {
			if (S_ISWRITE(sb.st_mode)) {
				if (!gQuiet) fprintf(stdout, "%s: File", testItem);
			}
			else {
				if (!gQuiet) fprintf(stdout, "%s: File is not writable\n", testItem);
				return RC_ITEM_DOESNT_EXIST;
			}
		}
		else if (strcmp(testOpt, "x") == 0) {
			/* alternative: match extension with $PATHEXT
			 */
			if (S_ISEXEC(sb.st_mode)) {
				if (!gQuiet) fprintf(stdout, "%s: File", testItem);
			}
			else {
				if (!gQuiet) fprintf(stdout, "%s: File is not an executable\n", testItem);
				return RC_ITEM_DOESNT_EXIST;
			}
		}
		else if (strcmp(testOpt, "s") == 0) {
			if (S_ISREG(sb.st_mode) && (sb.st_size != 0)) {
				if (!gQuiet) fprintf(stdout, "%s: Non-zero size File", testItem);
			}
			else {
				if (!gQuiet) fprintf(stdout, "%s: File is not non-zero size\n", testItem);
				return RC_ITEM_DOESNT_EXIST;
			}
		}
		else if (strcmp(testOpt, "z") == 0) {
			if (S_ISREG(sb.st_mode) && (sb.st_size == 0)) {
				if (!gQuiet) fprintf(stdout, "%s: Zero size File", testItem);
			}
			else {
				if (!gQuiet) fprintf(stdout, "%s: File is not zero size\n", testItem);
				return RC_ITEM_DOESNT_EXIST;
			}
		}
		else if (strcmp(testOpt, "c") == 0) {
			if (S_ISCHR(sb.st_mode)) {
				if (!gQuiet) fprintf(stdout, "%s: File", testItem);
			}
			else {
				if (!gQuiet) fprintf(stdout, "%s: File is not a character special\n", testItem);
				return RC_ITEM_DOESNT_EXIST;
			}
		}
		else if (strcmp(testOpt, "p") == 0) {
			if (S_ISFIFO(sb.st_mode)) {
				if (!gQuiet) fprintf(stdout, "%s: File", testItem);
			}
			else {
				if (!gQuiet) fprintf(stdout, "%s: File is not a named pipe\n", testItem);
				return RC_ITEM_DOESNT_EXIST;
			}
		}
		else if (strcmp(testOpt, "d") == 0) {
			if (S_ISDIR(sb.st_mode)) {
				if (!gQuiet) fprintf(stdout, "%s: Directory", testItem);
			}
			else {
				if (!gQuiet) fprintf(stdout, "%s: Direcotry doesn't exist\n", testItem);
				return RC_ITEM_DOESNT_EXIST;
			}
		}
		else if (strcmp(testOpt, "D") == 0) {
		}
		else {
			/* unknown option */
			usage(progname, 0); return RC_ERROR;
		}
	}	/* else */

	/* ensure target ends with backslash (\)
	 */
	if (iFullPath[1] == ':') iFullPath[3] = 0;
	else {
		/* assuming an UNC path */
		char *lc = strrchr(iFullPath, (int)'\\');
		*++lc = 0;
	}

	/* identify driver type
	 */
	//-dbg- fprintf(stderr, "\ndbg: testItem=<%s> iFullPath=<%s>\n", testItem, iFullPath);
	switch (GetDriveType(iFullPath)) {
		case DRIVE_UNKNOWN:
					if (!gQuiet) fprintf(stdout, " exists on an Unknown Drive Type\n");
					rc = RC_DRIVE_UNKNOWN;
					break;
		case DRIVE_NO_ROOT_DIR:
					if (!gQuiet) fprintf(stdout, " exists with an Invalid Root Path\n");
					rc = RC_DRIVE_NO_ROOT_DIR;
					break;
		case DRIVE_REMOVABLE:
					if (!gQuiet) fprintf(stdout, " exists on a Removable Media\n");
					rc = RC_DRIVE_REMOVABLE;
					break;
		case DRIVE_FIXED:
					if (!gQuiet) fprintf(stdout, " exists on a Fixed Drive\n");
					rc = RC_DRIVE_FIXED;
					break;
		case DRIVE_REMOTE:
					if (!gQuiet) fprintf(stdout, " exists on a Remote Drive\n");
					rc = RC_DRIVE_REMOTE;
					break;
		case DRIVE_CDROM:
					if (!gQuiet) fprintf(stdout, " exists on a CDROM Drive\n");
					rc = RC_DRIVE_CDROM;
					break;
		case DRIVE_RAMDISK:
					if (!gQuiet) fprintf(stdout, " exists on a RAM Disk\n");
					rc = RC_DRIVE_RAMDISK;
					break;
		default:
					if (!gQuiet) fprintf(stdout, " does not exist (unknown GetDriveType return)\n");
					rc = RC_ITEM_DOESNT_EXIST;
					break;
	}
	return rc;
}

/* convert given path into fullpath
 * - change slash to backslash
 * - absolute fullpath include driver letter
 * - UNC path will have TWO '\0' at the end
 */
static char *extendToFullPath(char *item, char fullPath[])
{
	char *cp, iItem[MAX_PATH];

	/* change slash to backslash */
	strcpy(iItem, item);
	for (cp = iItem; *cp; cp++) {
		if (*cp == '/') *cp = '\\';
	}

	/* UNC path */
	if (strncmp(iItem, "\\\\", 2) == 0) {
		strcpy(fullPath, iItem);
	}

	/* relative path at current drive */
	else if (iItem[1] != ':') {
		if (getcwd(fullPath, MAX_PATH) == 0) {
			return NULL;
		}
		sprintf(fullPath, "%s\\%s", fullPath, iItem);
	}

	/* relative path at other drive */
	else if (iItem[1] == ':' && iItem[2] != '\\') {
		int curdrive = _getdrive();
		if (_chdrive(toupper((int)iItem[0]) - 'A' + 1) != 0) {
			return NULL;
		}
		if (getcwd(fullPath, MAX_PATH) == 0) {
			return NULL;
		}
		sprintf(fullPath, "%s\\%s", fullPath, &iItem[2]);
		_chdrive(curdrive);
	}

	/* absolute path already provided */
	else {
		strcpy(fullPath, iItem);
	}

	//-dbg- fprintf(stderr, "item=%s iItem=%s, fullPath=%s\n", item, iItem, fullPath);
	return fullPath;
}

/*
 * reference program
 *
IfExist.EXE : File/Dir/Drive Existence Utility : by Steven Wettberg

Usage: IfExist [/ACTION] [ITEM TO TEST]

/ACTION may be one of the following parameters
     /DRIVE      : Checks for the existence of a drive and reports the type.
     /FILE       : Checks for the existence of a file and reports the type
                   of drive on which it exists.
     /DIRECTORY  : Checks for the existence of a directory and reports the type
                   of drive on which it exists.
"ITEM TO TEST" may be a drive,file or directory depending upon the action.

The program checks for the existence the file,directory or drive, if the
item exists an ERRORLEVEL indicating the drive type is returned.
An ERRORLEVEL of 7 indicates that the item does not exist

The ERRORLEVELs for the drive types are shown below.
ERRORLEVEL 1 = Unknown
ERRORLEVEL 2 = Removable
ERRORLEVEL 3 = Fixed
ERRORLEVEL 4 = Remote
ERRORLEVEL 5 = CDROM
ERRORLEVEL 6 = RamDisk

Examples
Usage: IfExist /DIRECTORY C:\SOMEDIR
Usage: IfExist /FILE C:\SOMEDIR\Test.Dat
Usage: IfExist /DRIVE C:\

Note: A UNC path may be used for directory and file existence.
 *
 */
