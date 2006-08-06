//
// regex_drv.c
//
// Module Name:
//
// Description:
//   Regular Expression Driver
//
//
// Revision History:
//   T. David Wong		05-01-2003    Original Author
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

#ifdef _MSC_VER
#include "MSConfig.h"
#endif

#include "mygetopt.h"
#include "regex.h"


/* defined constants
 */
#define SYNTAX_GNU    0x01
#define SYNTAX_POSIX  0x02
#define SYNTAX_BSD    0x04

/* global variables
 */
int gDebug = 0;
unsigned int gRegexSyntax = 0;
const char *gRegexStr = "ab.*";
char *gStrTable[16] = { 0 };
int   gTableSize = 0;
/* *****
char *gStrTable[] = {
	"abtester",
	"testabes",
	"ateab",
	"ttestabc",
};
int gTableSize=sizeof(gStrTable)/sizeof(char *);
***** */

/* subroutines
 */
static void regex_GNU(void);
static void regex_POSIX(void);
static void regex_BSD(void);


/* main program
 */
int
main(int argc, char **argv)
{
	int idx;
	int optcode;
	char *optptr;

	/* ***
	 * PARAMETER PARSING
	 */
	while ((optcode = fds_getopt(&optptr, "gpbm:", argc, argv)) != EOF)
	{
		switch (optcode) {
			case 'g':  gRegexSyntax |= SYNTAX_GNU; break;
			case 'p':  gRegexSyntax |= SYNTAX_POSIX; break;
			case 'b':  gRegexSyntax |= SYNTAX_BSD; break;

			/* set matchihng pattern */
			case 'm':  gRegexStr = optptr; break;

			/* special option - option starts with "--" */
			case ':':
					printf("unknown option: --%s\n", optptr);
					return 0;
			case '\003':
					printf("usage: %s [-g|-p|-b] -m regex string ...\n", argv[0]);
					return 0;

			default:
					printf("unimplemented option: %c\n", (char)optcode);
					return 0;
		}
	}	/* end-of-while */
	/*
	 *** */
	if (gRegexSyntax == 0) gRegexSyntax = SYNTAX_GNU;

	/* rest of arguments are strings to be matched */
	for (idx = (int)optptr; idx < argc; idx++) {
		// fprintf(stderr, "argv[%d] = %s\n", idx, argv[idx]);
		gStrTable[gTableSize] = argv[idx];
		gTableSize++;
	}

	// printf("gTableSize=%d\n", gTableSize);

	re_syntax_options = RE_SYNTAX_GREP;

	/* do regex matching */
	if (gRegexSyntax & SYNTAX_GNU) regex_GNU();
	if (gRegexSyntax & SYNTAX_POSIX) regex_POSIX();
	if (gRegexSyntax & SYNTAX_BSD) regex_BSD();

	return 0;
}

void regex_GNU()
{
	char *rc;
	int  ix;
	struct re_pattern_buffer pattern_buffer;

	/* initialize pattern buffer */
	pattern_buffer.translate = NULL;
	pattern_buffer.fastmap = NULL;
	pattern_buffer.buffer = NULL;
	pattern_buffer.allocated = 0L;

	/* compile pattern */
	rc = (char *)re_compile_pattern(gRegexStr, strlen(gRegexStr), &pattern_buffer);
	printf("re_compile_pattern returns %s\n", (rc==NULL)? "OK": rc);
	if (rc) return;

	/* perform matching */
	printf("GNU Regex - \"%s\"\n", gRegexStr);
	for (ix = 0; ix < gTableSize; ix++) {
		char *str = gStrTable[ix];
  			int  ri = re_match(&pattern_buffer, str, strlen(str), 0, NULL);
		printf("matching  \"%s\" against \"%s\" - returns ", str, gRegexStr);
		switch (ri) {
			case -1: printf("-1: no part is matched.\n"); break;
			case -2: printf("-2: internal error.\n"); break;
			default: printf("%d bytes are matched.\n", ri); break;
		}
	}
	/* perform searching */
	for (ix = 0; ix < gTableSize; ix++) {
		char *str = gStrTable[ix];
  			int  ri = re_search(&pattern_buffer, str, strlen(str), 0, strlen(str), NULL);
		printf("searching \"%s\" against \"%s\" - returns ", str, gRegexStr);
		switch (ri) {
			case -1: printf("-1: no part is matched.\n"); break;
			case -2: printf("-2: internal error.\n"); break;
			default: printf("matched (starts at %d) string is \"%s\".\n", ri, &str[ri]); break;
		}
	}

	regfree(&pattern_buffer);
	return;
}

void regex_POSIX()
{
	int  ri;
	int  ix;
	regex_t pattern_buffer;

	ri = regcomp(&pattern_buffer, gRegexStr, 0 /*REG_EXTENDED|REG_ICASE|REG_NOSUB|REG_NEWLINE*/);
	printf("regcomp returns %s\n", (ri==0)? "OK": "Err");
	if (ri) {
		switch (ri) {
			case REG_BADRPT: printf("bad repetition operators\n"); break;
			case REG_BADBR: printf("bad braces\n"); break;
			case REG_EBRACE: printf("bad braces count\n"); break;
			case REG_EBRACK: printf("bad bracket\n"); break;
			case REG_ERANGE: printf("bad range\n"); break;
			case REG_ECTYPE: printf("invalid character class name\n"); break;
			case REG_EPAREN: printf("bad parentheis\n"); break;
			case REG_ESUBREG: printf("bad subexpression\n"); break;
			case REG_EEND: printf("the regular expression causes no other more specific error\n"); break;
			case REG_EESCAPE: printf("bad escape sequence\n"); break;
			case REG_BADPAT: printf("bad extended regular expression syntax\n"); break;
			case REG_ESIZE: printf("regex needs a pattern buffer larger than 65536 bytes\n"); break; 
			case REG_ESPACE: printf("regex makes Regex to run out of memory\n"); break;
			default: printf("unknown error - %d\n", ri); break;
		}
		return;
	}

	/* perform matching */
	printf("POSIX Regex - \"%s\"\n", gRegexStr);
	for (ix = 0; ix < gTableSize; ix++) {
		char *str = gStrTable[ix];
		int  ri = regexec(&pattern_buffer, str, 0, NULL, 0);
		printf("matching  \"%s\" against \"%s\" - returns ", str, gRegexStr);
		switch (ri) {
			case 0: printf("0: match found.\n"); break;
			case REG_NOMATCH: printf("%d: no part is matched.\n", ri); break;
			default: printf("%d unknown return value.\n", ri); break;
		}
	}

	return;
}

void regex_BSD()
{
	char *rc;
	int  ix;

	rc = re_comp(gRegexStr);
	printf("re_comp returns %s\n", (rc==NULL)? "OK": rc);
	if (rc) return;

	/* perform matching */
	printf("BSD Search - \"%s\"\n", gRegexStr);
	for (ix = 0; ix < gTableSize; ix++) {
		char *str = gStrTable[ix];
  			int  ri = re_exec(str);
		printf("matching  \"%s\" against \"%s\" - returns ", str, gRegexStr);
		switch (ri) {
			case 0: printf("0: no part is matched.\n"); break;
			case 1: printf("1: match found.\n"); break;
			default: printf("%d unknown return value.\n", ri); break;
		}
	}

	return;
}

