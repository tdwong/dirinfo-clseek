//
// mystropt.c
//
// Module Name:
//   Manage String Options
//
// Description:
//
// Revision History:
//   T. David Wong		05-05-2005    Original Author
//   T. David Wong		05-15-2005    Implemented and tested TESTDRIVER
//   T. David Wong		04-02-2012    Compiled on Mac OS/X
//
// TODO:
//	1. better string handle in case insensitive condition
//
// -- MSVC command
// cl -Zi -W3 -D_TESTDRIVER_ mystropt.c
// -- GCC command
// gcc -g -o mystropt -D_TESTDRIVER_ mystropt.c
//

#include <stdio.h>
#include <stdlib.h>			// malloc
#include <string.h>

#include "mystropt.h"

/* internal defines
 */
#define LIST_ITEM_PER_LINE	6

/* internal data structure
 */
typedef struct TableOfStrings
{
	unsigned int count;		/* # of strings in the table */
	char **tbl;
} strTbl;

/*
 * str_InitTable
 *
 *	IN:
 *		kflag - keep flag
 *
 *	OUT:
 *		ptr   - pointer to table structure
 */
static void *str_InitTable(void)
{
	strTbl *tbl = malloc(sizeof(strTbl));
	if (tbl == NULL) return NULL;
	tbl->count = 0;
	tbl->tbl = NULL;
	// tbl->flag = kflag;
	return tbl;
}

/*
 * str_FreeTable - release table
 *
 *	IN:
 *		ptr   - pointer to table structure
 */
void str_FreeTable(void *ptr)
{
	strTbl *tbl = (strTbl*)ptr;
	unsigned int ix;

	/* sanity check */
	if (tbl == NULL) return;

	for (ix = 0; ix < tbl->count; ix++) {
		/* free table entry */
		if (tbl->tbl[ix]) free(tbl->tbl[ix]);
	}
	/* free table itself */
	free(tbl->tbl);
	free(tbl);
	return;
}

/*
 * str_InsertToTable - insert entry to table
 *
 *	IN:
 *		str   - string to be inserted
 *		ptr   - pointer to table structure
 *		kflag - keep flag (when match occurs which, longer or shorter string, to keep)
 *	OUT:
 *		opaque pointer to table structure
 */
void *str_InsertToTable(char *str, void *ptr, int kflag)
{
	strTbl *tbl = (strTbl *)ptr;
	unsigned int icase =
#ifdef	_MSC_VER
		1
#else
		0
#endif
		;
	unsigned int ix;
	unsigned int count;
	unsigned int slen = strlen(str);
	int (*cmpfunc)();

	/* sanity check */
	if (tbl == NULL) {
		/* there is NO table yet */
		tbl = str_InitTable();
	}

	/* use only lowercase if case is insensitive */
	if (icase) strlwr(str);

	/* search for sub-string match
	 */
	count = tbl->count;
	for (ix = 0; ix < count; ix++)
	{
		unsigned int elen = strlen(tbl->tbl[ix]);
		/*
		 * - check exact match
		 */
		if (strlen(str) == strlen(tbl->tbl[ix]))
		{
			/* select appropriate compare function */
			cmpfunc = icase ? stricmp : strcmp;
			if (cmpfunc(str, tbl->tbl[ix]) == 0) {
				/* no insertion needed */
				return (void*)tbl;
			}
		}
		else if (kflag == STR_KEEP_LONGER_STRING)
		{
			if (elen > slen && strstr(tbl->tbl[ix], str) != NULL) {
			/* longer string already exists */
				return (void*)tbl;
			}
			else
			if (slen > elen && strstr(str, tbl->tbl[ix]) != NULL) {
			/* replace with newer longer string */
				free(tbl->tbl[ix]);
				tbl->tbl[ix] = strdup(str);
				return (void*)tbl;
			}
		}
		else /* if (kflag == STR_KEEP_SHORTER_STRING) */
		{
			if (slen > elen && strstr(str, tbl->tbl[ix]) != NULL) {
			/* shorter string already exists */
				return (void*)tbl;
			}
			else
			if (elen > slen && strstr(tbl->tbl[ix], str) != NULL) {
			/* replace with newer shorter string */
				free(tbl->tbl[ix]);
				tbl->tbl[ix] = strdup(str);
				return (void*)tbl;
			}
		}
	}	/* for (ix = 0; ix < count; ix++) */
	/* no match found in the table
	 */
	(tbl->count)++;
	tbl->tbl = (char **)realloc(tbl->tbl, ((tbl->count)+1)*sizeof(char*));
	tbl->tbl[count] = strdup(str);
	tbl->tbl[tbl->count] = NULL;	/* make sure last entry is NULL */
	return (void*)tbl;
}


/*
 * str_FindInTable - find matched entry in table
 *
 *	IN:
 *		str   - string to search
 *		ptr   - pointer to table structure
 *		icase - ignore case if icase != 0
 */
char *str_FindInTable(char *str, void *ptr, int icase)
{
	strTbl *tbl = (strTbl *)ptr;
	unsigned int ix;
	char *cStr = str;

	/* sanity check */
	if (tbl == NULL) return NULL;

	if (icase) { cStr = strlwr(strdup(str)); }
	//
	for (ix = 0; ix <= tbl->count; ix++) {
		if (tbl->tbl[ix] == NULL) continue;
		/* match excluded substring */
		if (icase == 0) {
			if (strstr(str, tbl->tbl[ix]) != NULL) { return tbl->tbl[ix]; }
		}
		else {
			/* handle case insensitive condition */
			char *cTblStr = strlwr(strdup(tbl->tbl[ix]));
			char *sStr = strstr(cStr, cTblStr);
			free(cTblStr);
			if (sStr != NULL) {
				free(cStr);
				return tbl->tbl[ix];
			}
		}
	}
	return NULL;
}

/*
 * str_ResolveConflicts - resolve con conflict patterns in "contain" and "exclude" tables
 *
 *	IN:
 *		cptr  - pointer to "contain" table structure
 *		xptr  - pointer to "exclude" table structure
 */
void str_ResolveConflicts(void *cptr, void *eptr)
{
	strTbl *ctbl = (strTbl *)cptr;
	strTbl *etbl = (strTbl *)eptr;
	unsigned int cix, eix;

	/* sanity check */
	if (ctbl == NULL || etbl == NULL) return;
	if (ctbl->count == 0 || etbl->count == 0) return;

	for (cix = 0; cix < ctbl->count; cix++)
	{
		if (ctbl->tbl[cix] == NULL) continue;
		for (eix = 0; eix < etbl->count; eix++)
		{
			/* "contain" pattern takes precedence!!!
			 */
			if (etbl->tbl[eix] == NULL) continue;
			if (strstr(ctbl->tbl[cix], etbl->tbl[eix]) != 0) {
				//-dbg- fprintf(stderr, "contain=%s exclude=%s\n", ctbl->tbl[cix], etbl->tbl[eix]);
				/* move last entry to current slot */
				free(etbl->tbl[eix]);	// free current entry
				--(etbl->count);		// decrease entry count
				etbl->tbl[eix] = (etbl->count) ? etbl->tbl[etbl->count] : NULL;
				--eix;				// allow current slot to be checked again
				break;
			}
		}	/* for (eix = 0; eix < etbl->count; eix++) */
	}	/* for (cix = 0; cix < ctbl->count; cix++) */
	return;
}

/*
 * str_ListTable - list table content
 *
 *	IN:
 *		desc  - description string
 *		ptr   - pointer to table structure
 */
void str_ListTable(char *desc, void *ptr)
{
	strTbl *tbl = (strTbl *)ptr;
	unsigned int ix;

	/* sanity check */
	if (tbl == NULL) return;

	//
	fprintf(stdout, "%s\n", desc);
	if (tbl->count == 0) {
		// no entry available
		fprintf(stdout, "<none>\n");
		return;
	}
	for (ix = 0; ix <= tbl->count; ix++) {
		if (tbl->tbl[ix] == NULL) continue;
		fprintf(stdout, "[%d] %s ", ix, tbl->tbl[ix]);
		if (ix && (ix % LIST_ITEM_PER_LINE) == 0) { fprintf(stdout, "\n"); }
	}
	fprintf(stdout, "\n");
	return;
}

#ifndef	_WIN32
char *strlwr(char *str)
{
	char *ptr = str;
	while (*ptr) {
		if ('A' <= *ptr && *ptr <= 'Z') {
			*ptr += ('a' - 'A');
		}
		ptr++;
	}
	return str;
}
char *strupr(char *str)
{
	char *ptr = str;
	while (*ptr) {
		if ('a' <= *ptr && *ptr <= 'z') {
			*ptr -= ('a' - 'A');
		}
		ptr++;
	}
	return str;
}
#endif

/* ------------------------------------------------------------
 */
#ifdef	_TESTDRIVER_
void *gContainsStr = NULL,
     *gExcludedStr = NULL;
int   gIgnoreCase = 0;
//
static int usage(char *progname)
{
	fprintf(stderr, "Usage: %s [options] str...\n", progname);
	fprintf(stderr, "  -c<incstr>   string to include\n");
	fprintf(stderr, "  -x<excstr>   string to exclude\n");
	fprintf(stderr, "  -i           ignore case\n");
	return 0;
}

int main (int argc, char **argv)
{
	char *progname = argv[0];
	int ix;

	/* no arguments */
	if (argc == 1) {
		usage(progname);
		return 0;
	}

	/* parse options */
	for (ix = 1; ix < argc; ix++) {
//		fprintf(stderr, "[%d] %s\n", ix, argv[ix]);
		if (strcmp(argv[ix], "-i") == 0) {
			gIgnoreCase++;
			continue;
		}
		else if (strncmp(argv[ix], "-c", 2) == 0) {
			gContainsStr = str_InsertToTable(&argv[ix][2], gContainsStr, STR_KEEP_LONGER_STRING);
			if (gContainsStr == NULL) {
				fprintf(stderr, "error: str_InsertToTable to gContainsStr failed\n");
			}
			continue;
		}
		else if (strncmp(argv[ix], "-x", 2) == 0) {
			gExcludedStr = str_InsertToTable(&argv[ix][2], gExcludedStr, STR_KEEP_SHORTER_STRING);
			if (gExcludedStr == NULL) {
				fprintf(stderr, "error: str_InsertToTable to gExcludedStr failed\n");
			}
			continue;
		}
		else if (argv[ix][0] != '-') {
			/* no more option */
			break;
		}
	}

	/* list all */
	fprintf(stderr, "-- before RESOLVE CONFILCTS:\n");
	str_ListTable("string contains:", gContainsStr);
	str_ListTable("string excluded:", gExcludedStr);

	str_ResolveConflicts(gContainsStr, gExcludedStr);

	/* list all */
	fprintf(stderr, "-- after RESOLVE CONFILCTS:\n");
	str_ListTable("string contains:", gContainsStr);
	str_ListTable("string excluded:", gExcludedStr);

	/* handle arguments */
	for (/*no-op*/; ix < argc; ix++) {
		char *str;
		fprintf(stderr, "-%d- %s\n", ix, argv[ix]);
		if (str = str_FindInTable(argv[ix], gContainsStr, gIgnoreCase)) {
			fprintf(stderr, "-%d- %s - contains WANTED sub-string (%s)\n", ix, argv[ix], str);
		}
		if (str = str_FindInTable(argv[ix], gExcludedStr, gIgnoreCase)) {
			fprintf(stderr, "-%d- %s - should be EXCLUDED due to (%s)\n", ix, argv[ix], str);
		}
	}

	/* release all */
	str_FreeTable(gContainsStr);
	str_FreeTable(gExcludedStr);

	return 0;
}
#endif	/* _TESTDRIVER_ */
/*
 * ------------------------------------------------------------ */

