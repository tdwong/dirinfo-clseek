//
// mygetopt.c
//
// Module Name:
//   Get Option
//
// Description:
//
// Revision History:
//   T. David Wong		07-19-2002    Original Author
//   T. David Wong		04-19-2003    Included BufDump function
//   T. David Wong		05-09-2003    Fixed in fds_getopt - Unix shell only takes "-" as option indicator
//   T. David Wong		05-31-2003    Add support for multiple options in a single argument
//   T. David Wong		06-26-2003    Changed unknown option code to OPT_UNKNOWN
//   T. David Wong		12-23-2003    Fixed multiple option flag option
//
// -- MSVC command
// cl -Zi -W3 -D_TESTDRIVER_ mygetopt.c
// -- GCC command
// gcc -g -o mygetopt -D_TESTDRIVER_ mygetopt.c
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mygetopt.h"

/*
 * IN:
 *		optarg	- address of a char pointer
 *		optstr  - option string
 *		argc    - main's argc
 *		argv    - main's argv
 *
 *	optionh string example: "hvd:irm:"
 *		h, v, i, r    are options without argument
 *		d, m          are options with ONE argument
 *
 * OUT:
 *		_return_value_             _optarg_content_
 *
 *		char in optstr
 *		* option w/o :             ptr to the option itself
 *		* option w/  :             ptr to the option argument
 *		'\003' (unknown option)    ptr to the option itself
 *		':' (special option)       ptr to the option itself
 *		EOF (no more option)	   index to next argument in argv
 */
int fds_getopt(char **optarg, char *optstr, int argc, char **argv)
{
	/* static variable */
	static int idx = 0;
	static int shell = -1;		/* what kind of SHELL we're running under? */
	static char *multiOption = NULL;	/* multiple options in one argument */
	char *nextOpt;				/* next option to be checked */
	char *ptr;

#ifdef	_debug_
	{ int ix;
	for (ix = 0; ix < argc; ix++) { fprintf(stderr, "[%d]=%s; ", ix, argv[ix]); }
	fprintf(stderr, "\nargc=%d, idx=%d, multiOption=%s\n", argc, idx, multiOption);
	}
#endif	/* _debug_ */

	if (*optarg == NULL) {
		/* this indicates a new start (argc & argv) */
		idx = 0;
	}
	if (shell <  0) {
		shell = (getenv("SHLVL") == NULL) ? 0 : 1;
	}

	/* compound multiple state */
	if (multiOption != NULL)
	{
		/* continue on compound multiple option */
		nextOpt = multiOption;
	}
	else
	{
		/* normal option processing state */

		/* advance index pointer */
			/* first call will move idx to the 2nd argument */
		++idx;

		/* do we still have unprocessed argument */
		if (idx >= argc) {
			*optarg = (char *)idx;
			return (int)EOF;
		}

		/* check for '-' (or '/') */
#if (_BUILD_==MSVC)
			/* when SHLVL is undefined (i.e. Command.com), we will honor both "-" and "/" as option indicator */
		if ((shell == 0 && (argv[idx][0] != '-') && (argv[idx][0] != '/')) ||
			/* when SHLVL is defined (i.e. Unix-like shell), we only recognize "-" as option indicator */
		    (shell == 1 && argv[idx][0] != '-'))
#else
		if (argv[idx][0] != '-')
#endif
		{
#ifdef	_no_mix_of_options_and_arguments_
            /* any non option (not starts with -) terminates option parsing */
			*optarg = (char *)idx;
			return (int)EOF;
#else	/* _no_mix_of_options_and_arguments_ */
			*optarg = (char *)argv[idx];
			return (int)OPT_NONOPT;
#endif	/* _no_mix_of_options_and_arguments_ */
		}

		/* special case to end option list */
		if (strcmp(argv[idx], "--") == 0) {
			*optarg = (char *)(idx+1);
			return (int)EOF;
		}

		/* default option string */
		*optarg = argv[idx];

		/* special option */
#ifdef	_original_implementation_
		/* original implementation only recognizes "--help"
		 * */
		if (strcmp(argv[idx], "--help") == 0) {
			return (int)OPT_SPECIAL;
		}
#else	/* _original_implementation_ */
		/* current implementation (2003-05-03) recognizes options start with "--"
		 * and returns whatever after "--"
		 * */
		if (strncmp(argv[idx], "--", 2) == 0) {
			*optarg = &argv[idx][2];
			return (int)OPT_SPECIAL;
		}
#endif	/* _original_implementation_ */

		nextOpt = &argv[idx][1];

	}

#ifdef	_debug_
	ptr = strchr(optstr, (int)(*nextOpt));
	fprintf(stderr, "nextOpt=%s, ptr=%s\n", nextOpt, ptr);
#endif	/* _debug_ */

	/* match option */
	if ((ptr = strchr(optstr, (int)(*nextOpt))) == NULL)
	{
		/* compound multiple option must be reset */
		multiOption = NULL;
		return (int)OPT_UNKNOWN;
	}

	/* is extra argument needed? */
	if (*(ptr+1) == ':') {
		/* look into next character */
		++nextOpt;
		if (*nextOpt == 0) {
			/* take next argument */
			++idx;
			*optarg = argv[idx];
			/* handle boundary condition */
			if (idx >= argc) {
				*optarg = (char *)idx;
				return (int)EOF;
				// this will enable option to take 0 or 1 argument, but could
				// crash the program if the return value is not properly handled.
				//-- *optarg = NULL;
			}
		}
		else {
			/* take rest of the argument */
			*optarg = nextOpt;
		}

		/* compound multiple option must be reset */
		multiOption = NULL;
	}
	else {
		/* could be compound multiple option */
		if (*++nextOpt != 0) multiOption = nextOpt;
		else multiOption = NULL;
	}

	return (int)*ptr;
}

/* ------------------------------------------------------------
 */
#ifdef	_TESTDRIVER_
static void version()
{ fprintf(stderr, "version: %s built at %s %s\n", __FILE__, __DATE__, __TIME__); }
static int usage(char *progname)
{
	fprintf(stderr, "Usage: %s [options] [--] [arg [arg...]]\n", progname);
	fprintf(stderr, "  -h               help\n");
	fprintf(stderr, "  -v               version\n");
	fprintf(stderr, "  -k               known option w/o argument\n");
	fprintf(stderr, "  -o               known option w/o argument\n");
	fprintf(stderr, "  -i <ip-address>  IP address\n");
	fprintf(stderr, "  -p <port>        port #\n");
	fprintf(stderr, "  -d <#>           debug level\n");
	fprintf(stderr, "  -r <multiple tokens in double quote>\n");
	fprintf(stderr, "  -u               unimplemented\n");
	fprintf(stderr, "\nsample test cases:\n");
	/* use stdout so that test cases can be redirected into a file */
	fprintf(stdout, "  %s\n", progname);
	fprintf(stdout, "  %s -v --help\n", progname);
	fprintf(stdout, "  %s -Z\n", progname);
	fprintf(stdout, "  %s -h -v -p9899 -i addr\n", progname);
	fprintf(stdout, "  %s -p 9899 -iaddr -d33 arg1 arg2\n", progname);
	fprintf(stdout, "  %s -u -rtest -k -v arg1 arg2\n", progname);
	fprintf(stdout, "  %s -d23 -ok -korat -kor bs -kro -rok\n", progname);
	return 0;
}

int main (int argc, char **argv)
{
	char *progname = argv[0];
	int optcode;
	char *optptr;
//	extern char *optarg;
//	extern int optind;
	int errflg = 0;
	int optidx;

	/* no arguments */
	if (argc == 1) {
		usage(progname);
		return 0;
	}

#ifdef	_debug_
	{ int ix;
	for (ix = 0; ix < argc; ix++) { fprintf(stderr, "[%d]=%s; ", ix, argv[ix]); }
	fprintf(stderr, "\nargc=%d\n", argc);
	}
#endif	/* _debug_ */

	// while ((c = getopt(argc, argv, "abo:")) != EOF)
	while ((optcode = fds_getopt(&optptr, "hvkoi:p:d:r:u", argc, argv)) != EOF)
	{
		fprintf(stderr, "optcode=%c *optptr=%c\n", optcode, *optptr);

		switch (optcode) {
			case 'h':
					usage(progname);
					break;
			case 'v':
                    version();
					break;
			case 'k':
			case 'o':
					printf("known option (w/o arg) --> %c\n", (char)optcode);
					break;
			case 'i':
					fprintf(stderr, "option %c, IP address is: %s\n", (char)optcode, (char*)optptr);
					break;
			case 'p':
					fprintf(stderr, "option %c, port # is: %d\n", (char)optcode, atoi((char*)optptr));
					break;
			case 'd':
					{ int debug = atoi((char *)optptr);
					fprintf(stderr, "option %c, debug level set to: %d\n", (char)optcode, debug);
					}
					break;
			case 'r':
					printf("known option --> %c\n", (char)optcode);
//					if (optptr != NULL)
						printf("option argument: %s\n", optptr);
					break;
			case OPT_SPECIAL:
					printf("special -- option: %s\n", optptr);
                    if (strcmp(optptr, "help")==0) { usage(progname); }
                    if (strcmp(optptr, "version")==0) { version(); }
					break;
			case OPT_UNKNOWN:
					printf("unknown option: %s\n", optptr);
					errflg++;
					break;
			case OPT_NONOPT:
					printf("non-opt -- not an option, but an argument: %s\n", optptr);
					break;
			default:
					printf("unimplemented option: %c\n", (char)optcode);
					errflg++;
					break;
		}
	}	/* end-of-while */

	/* any more arguments? */
	if ((int)optptr >= argc) return 0;

	/* rest of arguments */
	printf("rest of the arguments:\n");
	for (optidx = (int)optptr; optidx < argc; optidx++) {
		printf("argv[%d] = %s\n", optidx, argv[optidx]);
	}
	return 0;
}
#endif	/* _TESTDRIVER_ */
/*
 * ------------------------------------------------------------ */

/* BufDump
 *    dump buffer content in an easy to read format
 *
 * input parameters:
 *    buf       - starting address of the buffer
 *    size      - # of bytes to be dump out
 *    startaddr - logical address of the buffer (can be 0)
 *    ofd       - output file pointer (can be NULL)
 */
#define COLS 256	/* change here, if you ever need more columns */
#define ADWIDTH 10
#define LLEN (ADWIDTH + (5*COLS-1)/2 + 2 + COLS)
static char hexxa[] = "0123456789abcdef0123456789ABCDEF", *hexx = hexxa;
void BufDump(char *buf, unsigned int size, unsigned int startaddr, FILE *ofd)
{
  int ix;
  FILE *fp = stdout;
  int chr, elnt, pos = 0;
  int cols = 16, nonzero = 0;
  long saddr = startaddr, seekoff = 0;
  char line[LLEN+1];
  char adformat[12];

  /* write to given file pointer if available */
  if (ofd != NULL) fp = ofd;

  /* width (i.e. 9) must be ADWIDTH-2 to accommodate ": " */
  sprintf(adformat, "%%0%dlx: ", (ADWIDTH-2));

  for (ix = 0; ix < (int)size; ix++)
	{
	  elnt = buf[ix];

	  if (pos == 0)
		{
		  sprintf(line, adformat, saddr + seekoff);
		  for (chr = ADWIDTH; chr < LLEN; line[chr++] = ' ');
		}

	  line[chr = (ADWIDTH + (5*pos) / 2)] = hexx[(elnt >> 4) & 0xf];
	  line[++chr]                   = hexx[ elnt       & 0xf];

	  line[(ADWIDTH+2) + (5*cols - 1)/2 + pos] = (elnt > 31 && elnt < 127) ? elnt : '.';
	  if (elnt)
		nonzero++;
	  saddr++;

	  if (++pos == cols)
		{
		  line[chr = ((ADWIDTH+2) + (5*cols - 1)/2 + pos)] = '\n';
		  line[++chr] = '\0';
		  fputs(line, fp);
		  // if (ofd > 0) { write(ofd, line, strlen(line)); }
		  nonzero = 0;
		  pos = 0;
		}
	} // end of while

  if (pos)
	{
	  line[chr = ((ADWIDTH+2) + (5*cols - 1)/2 + pos)] = '\n';
	  line[++chr] = '\0';
	  fputs(line, fp);
	  // if (ofd > 0) { write(ofd, line, strlen(line)); }
	}

  /* flush buffered data */
  fflush(fp);
  return;
}

#ifdef	_MSC_VER
	// mygetopt.c(405) : warning C4996: 'GetVersionExA': was declared deprecated
	// C:\Program Files\Windows Kits\8.1\include\um\sysinfoapi.h(433) : see declaration of 'GetVersionExA'
#pragma warning(disable: 4996)		// to disable C4996 warning
boolean GetWindowsVersion(unsigned int *major, unsigned int *minor)
{
/*
typedef struct _OSVERSIONINFO {
  DWORD dwOSVersionInfoSize;
  DWORD dwMajorVersion;
  DWORD dwMinorVersion;
  DWORD dwBuildNumber;
  DWORD dwPlatformId;
  TCHAR szCSDVersion[128];
} OSVERSIONINFO;

dwMajorVersion - Major version number of the operating system.
    Operating System      Meaning
    Windows Server 2003     5
    Windows XP              5
    Windows 2000            5
    Windows NT 4.0          4
    Windows Me              4
    Windows 98              4
    Windows 95              4
    Windows NT 3.51         3
dwMinorVersion - Minor version number of the operating system.
    Operating System      Meaning
    Windows Server 2003     2
    Windows XP              1
    Windows 2000            0
    Windows NT 4.0          0
    Windows Me             90
    Windows 98             10
    Windows 95              0
    Windows NT 3.51        51
*/
	OSVERSIONINFO osver;

	memset(&osver, 0, sizeof(osver));
	//
	osver.dwOSVersionInfoSize = sizeof(osver);
	if (GetVersionEx(&osver) == TRUE) {
		*major = (unsigned int) osver.dwMajorVersion;
		*minor = (unsigned int) osver.dwMinorVersion;
		return TRUE;
	}
/*
 * another approach
 *
{
	unsigned int _osver = GetVersion();
	unsigned _winver, _winmajor, _winminor;
	_winminor = (_osver >> 8) & 0x00FF;
	_winmajor = (_osver >> 0) & 0x00FF;
	_winver = (_winmajor << 8) + _winminor;
	_osver = (_osver >> 16) & 0x00FFFF;
	//
	*major = _winmajor;
	*minor = _winminor;
	return TRUE;
}
 *
 * another approach
 * */

	return FALSE;
}
#endif	/* _MSC_VER */

/* fdsout
 *    debug output function
 */
#include <stdarg.h>			/* for var_list, etc. */
static int gOutLevel = 0;
int fdsout(const int level, const char * format, ...)
{
	va_list argptr;
	int rc;

	/* control by global debug level */
	if (level > gOutLevel) return -1;

	va_start(argptr, format);
	rc = vprintf(format, argptr);
	va_end(argptr);
	return rc;
}
int fdsSetLevel(int level)
{
	gOutLevel = level;
	return gOutLevel;
}
int  fdsGetLevel(void)
{
	return gOutLevel;
}

