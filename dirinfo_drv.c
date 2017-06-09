//
// dirinfo_drv.c
//
// Module Name:
//
// Description:
//   Directory Information Retrieval Driver
//
//
// Copyright (c) 2002 by PowerTV, Inc.  All Rights Reserved.
//
// Revision History:
//   T. David Wong		07-03-2002    Original Author
//   T. David Wong		04-17-2012    Added 'f' to list all files
//
/*
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#if	defined(unix) || defined(__STDC__)
#include <dirent.h>
#include <unistd.h>
#endif

#include "dirinfo.h"
#include "mygetopt.h"

/* defined constants for matchCriteria */
#define	WILDCARD         0
#define	STRING_CONTAINS 11
#define STRING_BEGINS   12
#define STRING_ENDS     13
#define	LATER_FILE      21
#define	EARLIER_FILE    22

int gDebug = 0;

/* local functions
 */
static void show_dirinfo(char *dirpath, int recursive);


// typedef int (*FILEPROC)(char *, char *, struct stat *, void *);
int matchString(const char *filename, const char *dirname, struct stat *statp, void *voidp)
{
	matchCriteria_t *mcbuf = (matchCriteria_t *)voidp;
	int flen = strlen(filename);
	int slen = strlen(mcbuf->cparam.str);
	//
	if (mcbuf->type == WILDCARD) {
			fprintf(stderr, "***");
	}
	else if (mcbuf->type == STRING_CONTAINS &&
		strstr(filename, mcbuf->cparam.str)) {
		fprintf(stderr, "*** contains [%s]", mcbuf->cparam.str);
	}
	else if (mcbuf->type == STRING_BEGINS &&
		(strncmp(filename, mcbuf->cparam.str, slen) == 0)) {
		fprintf(stderr, "*** begins [%s]", mcbuf->cparam.str);
	}
	else if (mcbuf->type == STRING_ENDS &&
		(strncmp(&filename[flen-slen], mcbuf->cparam.str, slen) == 0)) {
		fprintf(stderr, "*** ends [%s]", mcbuf->cparam.str);
	}
	else {
		return -1;
	}
	fprintf(stderr, " name=%s [%#x] ***\n",
				filename, (unsigned int)statp->st_mtime);
	fflush(stderr);
	return 0;
}

int compareTime(const char *filename, const char *dirname, struct stat *statp, void *voidp)
{
	matchCriteria_t *mcbuf = (matchCriteria_t *)voidp;
	int rc = 0;
	//
	if (mcbuf->type == WILDCARD) {
			fprintf(stderr, "*** name=%s [%#x] ***\n",
				filename, (unsigned int)statp->st_mtime);
	}
	else if (mcbuf->type == LATER_FILE && statp->st_mtime > mcbuf->cparam.time) {
		fprintf(stderr, "*** later than [%#x] name=%s [%#x] ***\n",
				(unsigned int)mcbuf->cparam.time, filename, (unsigned int)statp->st_mtime);
	}
	else if (mcbuf->type == EARLIER_FILE && statp->st_mtime < mcbuf->cparam.time) {
		fprintf(stderr, "*** earlier than [%#x] name=%s [%#x] ***\n",
				(unsigned int)mcbuf->cparam.time, filename, (unsigned int)statp->st_mtime);
	}
	else {
		rc = -1;
	}
	return rc;
}

// print this entry if it is a File
int showFileEntry(const char *filename, const char *dirname, struct stat *statp, void *voidp)
{
	if (statp->st_mode & S_IFREG) {
		fprintf(stderr, "*** [REG] %s\n", filename);
		fflush(stderr);
	}
	return 0;
}

// print this entry if it is a Directory
int showDirEntry(const char *filename, const char *dirname, struct stat *statp, void *voidp)
{
	if (statp->st_mode & S_IFDIR) {
		fprintf(stderr, "*** [DIR] %s\n", filename);
		fflush(stderr);
	}
	return 0;
}

// show program version & build time
inline void showVersion(char *name)
{
	printf("%s (built at %s %s)\n", name, __DATE__, __TIME__);  
}

/* main program
 */
int 
main(int argc, char **argv)
{
	int   loop;
	int   recursive;
	char  resource;
	char  rootdir[128];
	char *destdirp = NULL;

	/* parameter parsing */
	while (argc > 1) {
		if ((strncmp(argv[1], "-d", 2)) == 0) {
			sscanf(&argv[1][2], "%d", &gDebug);
			argc--; argv++;
		}
		else if ((strncmp(argv[1], "-v", 2)) == 0) {
			showVersion(argv[0]);
			return 0;
		}
		else { /* unrecognized option */
			break;
		}
	}	/* while (argc > 1) */

	if (argc <= 1) {
		fprintf(stderr, "usage: %s _directory_\n", argv[0]);
		return -1;
	}

	/* default root directory */
	strcpy(rootdir, argv[1]);
	if (condense_path(rootdir) < 0) {
		fprintf(stderr, "%s: invalid directory\n", rootdir);
		return -2;
	}
	destdirp = rootdir;

	resource = '?';
	loop = 1;
	recursive = 1;
	do {
		static char *prompt = "[l|F|D|b|e|c|n|o|r|d|R|x]";
		char ans[80];

		//fprintf(stderr, "-2-resource=%c (0x%02x)\n", resource, (int)resource);

		/* what's next? */
		if (resource != 0)
			fprintf(stderr, "Your selection %s: ", prompt); fflush(stderr);
		if (fgets(ans, sizeof(ans), stdin) == NULL) { break; }
		resource = ans[0];

		//fprintf(stderr, "-1-resource=%c (0x%02x)\n", resource, (int)resource);

		switch (resource)
		{
			case '?':
			case 'h':
				{
				fprintf(stderr, "Current root directory: %s\n", destdirp);
				fprintf(stderr, "Available options:\n");
				fprintf(stderr, " h - show this menu\n");
				fprintf(stderr, " v - show version\n");
				fprintf(stderr, " l - show directory information\n");
				fprintf(stderr, " F - show all files\n");
				fprintf(stderr, " D - show all directories\n");
				// fprintf(stderr, " m - search files contain specified pattern\n");
				fprintf(stderr, " b - search files begin with specified pattern\n");
				fprintf(stderr, " e - search files end with specified pattern\n");
				fprintf(stderr, " c - search files contains specified pattern\n");
				fprintf(stderr, " n - search files with later mtime\n");
				fprintf(stderr, " o - search files with earlier mtime\n");
				fprintf(stderr, " r - change root directory\n");
				fprintf(stderr, " d - change debug level\n");
				fprintf(stderr, " R - toggle recursive\n");
				fprintf(stderr, " x - quit\n");
				}
				break;

			case 'v':
				{
				showVersion(argv[0]);
				}
				break;

			case 0:
			case '\n':
			case '\r':
				break;

			case 'r':	/* change root directory */
				{
				int  rc;
				char basedir[128];
				fprintf(stderr, "[%s] new root? ", destdirp); fflush(stderr);
				rc = scanf("%s", basedir);
				if (condense_path(basedir) < 0)
					fprintf(stderr, "error: %s: directory not found.\n", basedir);
				else
					strcpy(rootdir, basedir);
				}
				break;

			case 'c':	/* search files contain specified pattern */
			case 'b':	/* search files begin with specified pattern */
			case 'e':	/* search files end with specified pattern */
				{
				// dirInfo_t dibuf = { 0 };
				matchCriteria_t mcbuf = { 0 };
				char pattern[80];
				fprintf(stderr, "pattern? "); fflush(stderr);
				if (scanf("%s", pattern) < 0)
					break;
				if (resource == 'c') mcbuf.type = STRING_CONTAINS;
				else if (resource == 'b') mcbuf.type = STRING_BEGINS;
				else if (resource == 'e') mcbuf.type = STRING_ENDS;
				else if (strcmp(pattern, "*") == 0) mcbuf.type = WILDCARD;
				else break;
				mcbuf.cparam.str = pattern;
				mcbuf.proc = matchString;
				dirinfo_Find(destdirp, NULL /*&dibuf*/, &mcbuf, recursive);
				}
				break;

			case 'n':	/* search files with later mtime */
			case 'o':	/* search files with earlier mtime */
				{
				// dirInfo_t dibuf = { 0 };
				matchCriteria_t mcbuf = { 0 };
				char basefile[80];
				fprintf(stderr, "basefile? "); fflush(stderr);
				if (scanf("%s", basefile) < 0)
					break;
				if (strcmp(basefile, "*") == 0)
					{ mcbuf.type = WILDCARD; }
				else {
					struct stat sb;
					if (stat(basefile, &sb) < 0) {
						fprintf(stderr, "error: %s: file not found.\n", basefile);
						break;
					}
					if (resource == 'n') mcbuf.type = LATER_FILE;
					else if (resource == 'o') mcbuf.type = EARLIER_FILE;
					else break;
					mcbuf.cparam.time = sb.st_mtime;
				}
				mcbuf.proc = compareTime;
				dirinfo_Find(destdirp, NULL /*&dibuf*/, &mcbuf, recursive);
				}
				break;

			case 'd':	/* show directory information */
				{
				fprintf(stderr, "[was %d] new debug level? ", gDebug); fflush(stderr);
				scanf("%d", &gDebug);
				fdsSetLevel(gDebug);
				}
				break;

			case 'R':	/* toggle recursive */
				{
				int nRecursive = recursive ? 0 : 1;
				fprintf(stderr, "[was %d] new recursive setting = %d\n", recursive, nRecursive); fflush(stderr);
				recursive = nRecursive;
				}
				break;

			case 'l':	/* show directory information */
				{
				show_dirinfo(destdirp, recursive);
				}
				break;

			case 'F':	/* show files */
			case 'D':	/* show directories */
				{
				// dirInfo_t dibuf = { 0 };
				matchCriteria_t mcbuf = { 0 };
				char  path[80];
				char *pathp = &path[0];
				if (destdirp == NULL) {
					printf("destination dir: ");
					scanf("%s", path);
				}
				else {
					pathp = destdirp;
				}
				if (resource == 'F') mcbuf.proc = showFileEntry;
				else if (resource == 'D') mcbuf.proc = showDirEntry;
				else break;
				dirinfo_Find(destdirp, NULL, &mcbuf, recursive);
				}
				break;

			case 'q':	/* exit */
			case 'x':
			case '.':
				{
				loop = 0;
				break;
				}

			default:
				{
				fprintf(stderr, "%c (0x%02x): unknonw option\n", resource, resource);
				}
		} /* end-of-switch */

		//fprintf(stderr, "-3-resource=%c (0x%02x)\n", resource, (int)resource);

		if (resource == '\n' || resource == '\r') {
			resource = 0;
		}

	} while (loop);

	return 0;
}

static void show_dirinfo(char *dirpath, int recursive)
{
	dirInfo_t dibuf = { 0 };
	char  path[80];
	char  *pathp = &path[0];

	if (dirpath == NULL) {
		printf("destination dir: ");
		scanf("%s", path);
	}
	else {
		pathp = dirpath;
	}

	/* search directory info */
	dirinfo_Find(pathp, &dibuf, NULL, recursive);

	/* closing report */
	dirinfo_Report(&dibuf, dirpath);
}

