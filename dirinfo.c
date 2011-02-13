//
// dirinfo.c
//
// Description:
//   Traverse directory tree and call the callback routine for every found entry
//
// Copyright (c) 2002 by PowerTV, Inc.  All Rights Reserved.
//
// Revision History:
//   T. David Wong		07-03-2002    Original Author
//   T. David Wong		05-10-2003    Fixed path delimiter, Unix shell will now use "/" instead of "\\"
//   T. David Wong		02-07-2004    Added output function to matchCriteria
//   T. David Wong		02-12-2011    Prevented double delimiter (e.g. F:\\Software\clseek.exe)
//

////////////
#ifdef	_local_inc_
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef	_MSC_VER
#include <windows.h>
/* local MSC-specific macros */
#define	S_ISDIR(mode)	(mode & S_IFDIR)
#define	S_ISREG(mode)	(mode & S_IFREG)
#endif	/* _MSC_VER */

#ifndef	PATH_MAX
#define	PATH_MAX	512
#endif
#endif	/* _local_inc_ */
////////////

#if	defined(unix) || defined(__STDC__)
#include <dirent.h>		/* DIR structure */
#endif

#include "mygetopt.h"
#include "dirinfo.h"

/* public functions
 */
/*
 * Traverse provided path
 *
 * - Callback function will be called if provided in matchCriteria structure.
 * - Collected statistics will be recorded in dirInfo structure.
 * - Depends on the recursive flag, this function could recursively calls itself.
 */
int dirinfo_Find(const char *dirname, dirInfo_t *dip, matchCriteria_t *mcbuf, int recursive)
{
#ifdef	_MSC_VER
	HANDLE          hFile = NULL;	/* Find file handle */
	WIN32_FIND_DATA FileData;       /* Find file info structure */
	BOOL            fDone = FALSE;
#else
	DIR             *dirp;			/* directory stream */
	struct dirent   *direntp;		/* pointer to directory entry structure */
#endif	/* _MSC_VER */
	char            fullname[PATH_MAX+1];		/* plus "\0" */
	struct stat     sb;
	int             count = 0;		/* for debug purpose */
	static char     sDelimiter = 0;
	OutputFunc      dbgOutput = NULL;

	if (sDelimiter == 0)
	{
#ifdef	_MSC_VER
		sDelimiter = (getenv("SHLVL") == NULL) ? '\\' : '/';
#else	/* non-MSDOS */
		sDelimiter = '/';
#endif	/* _MSC_VER */
	}

	if (mcbuf && mcbuf->printf) {
		dbgOutput = mcbuf->printf;
	}

#ifdef	_MSC_VER
	if (IsDirectory(dirname) == 0) {
		if (mcbuf && mcbuf->printf)
			mcbuf->printf(OUT_INFO, "%s: not a directory\n", dirname);
		return -1;
	}
#else
	/* open the directory entry */
	if ((dirp = opendir(dirname)) == NULL) {
		if (dbgOutput)
			dbgOutput(OUT_WARN, "%s: error opening directory\n", dirname);
		return -1;
	}
#endif	/* _MSC_VER */

	/* debug */
	if (dbgOutput)
		dbgOutput(OUT_INFO, "Entering [%s]\n", dirname);

#ifdef	_MSC_VER
	{
		char extFileName[MAX_PATH+5];	/* plus "\\*.*\0" */

	/* create a extended pathname ends with "*.*" which is
	 * required by FindFirstFile() call
	 */
		// strcpy(extFileName, dirname);
		// strcat(extFileName, "\\*.*");
		sprintf(extFileName, "%s%c*.*", dirname, sDelimiter);
		// swprintf((wchar_t*)extFileName, (const wchar_t*)"%s%c*.*", dirname, sDelimiter);

	/* start a search on all the files within the directory
	 */
		hFile = FindFirstFile(extFileName, &FileData);
		if (hFile == INVALID_HANDLE_VALUE) {
			if (dbgOutput)
				dbgOutput(OUT_WARN, "%s: FindFirstFile failed (%#x)\n", dirname, hFile);
			return -1;
		}
		if (dbgOutput)
			dbgOutput(OUT_NOISE, "FindFirst[cFileName]=%s%c[%s]\n", dirname, sDelimiter, FileData.cFileName);

	/* handle first entry.  but don't count in '.' or '..'
	 */
		if ((strcmp(FileData.cFileName, ".") != 0) &&
			(strcmp(FileData.cFileName,"..") != 0)) {
			/* statistics */
			if (dip) {
				sprintf(fullname, "%s%c%s", dirname, sDelimiter, FileData.cFileName);
				stat(fullname, &sb);
				if (S_ISDIR(sb.st_mode)) dip->num_of_directories++;
				else if (S_ISREG(sb.st_mode)) dip->num_of_files++;
				else dip->num_of_others++;
			}
			count++;		/* for debug purpose */
		}
	}
#endif	/* _MSC_VER */

	/*
	// walk all the entries in the directory
	// (i.e. read all directory entries
	*/
#ifdef	_MSC_VER
	while (!fDone)
#else
	while ((direntp = readdir(dirp)) != NULL)
#endif	/* _MSC_VER */
	{
#ifdef	_MSC_VER
		char *direntName = FileData.cFileName;
#else
		char *direntName = direntp->d_name;
#endif	/* _MSC_VER */

		/* skip '.' and '..' */
		if ((direntName[0] == '.' && direntName[1] == 0) ||
		    (direntName[0] == '.' && direntName[1] == '.' &&
		     direntName[2] == 0))
		{
			if (dbgOutput)
				dbgOutput(OUT_NOISE, "Special DIRECTORY: %s\n", direntName);
#ifdef	_MSC_VER
			/* find next file entry */
			fDone = !FindNextFile(hFile, &FileData);
#endif	/* _MSC_VER */
			continue;
		}

		count++;		/* for debug purpose */

		/* let's compose the FULL pathname */
		/* and acquire the file status */
		// 2011-02-12 prevent double delimiter
		if (dirname[(strlen(dirname)-1)] == sDelimiter) {
			sprintf(fullname, "%s%s", dirname, direntName);
		}
		else {
			sprintf(fullname, "%s%c%s", dirname, sDelimiter, direntName);
		}
		stat(fullname, &sb);

		/* ***
		 * call provided function ...
		 * */
		if (mcbuf && mcbuf->proc) {
			int rc = (mcbuf->proc) (direntName, fullname, &sb, mcbuf); 
		}

		/* brief file information */
		if (dbgOutput)
			dbgOutput(OUT_INFO, "  %s [%s]\n", fullname,
				S_ISREG(sb.st_mode) ? "REG" :
				S_ISDIR(sb.st_mode) ? "DIR" :
#ifndef	_WIN32
				S_ISLNK(sb.st_mode) ? "LNK" :
				S_ISSOCK(sb.st_mode) ? "SOCK" :
#endif	/* !_WIN32 */
				"other"
				);

#ifndef	_MSC_VER
		/* direct entry information */
		if (dbgOutput)
			dbgOutput(OUT_NOISE, "(ino=%d) name=%s [reclen=%d/off=%d]\n",
				(unsigned int) direntp->d_ino,
				direntp->d_name,
#ifndef	_WIN32
				/* these fields don't exist in Windows environment */
				direntp->d_reclen,
				(unsigned int) direntp->d_off
#else	/* WIN32 */
				0, 0
#endif	/* !_WIN32 */
				 );
#endif	/* !_MSC_VER */
		/* file stat information */
		if (dbgOutput)
			dbgOutput(OUT_NOISE, "(ino=%d) mode=%#x size=%d ctime=%#x mtime=%#x atime=%#x (uid=%d/gid=%d)\n",
				(unsigned int)sb.st_ino,
				(unsigned int)sb.st_mode,
				(unsigned int)sb.st_size,
				(unsigned int)sb.st_ctime,
				(unsigned int)sb.st_mtime,
				(unsigned int)sb.st_atime,
				(unsigned int)sb.st_uid,
				(unsigned int)sb.st_gid
				);

		/* statistics */
		if (dip) {
			if (S_ISDIR(sb.st_mode)) dip->num_of_directories++;
			else if (S_ISREG(sb.st_mode)) dip->num_of_files++;
			else dip->num_of_others++;
		}

		/* recursive ... */
		if (recursive && S_ISDIR(sb.st_mode))
		{
			/* depth first */
			/* the directory could have been removed by the callback
			 * but the call will return with error when opendir() fails
			 */
			int rc = dirinfo_Find(fullname, dip, mcbuf, recursive);
		}

#ifdef	_MSC_VER
		/* find next file entry */
		fDone = !FindNextFile(hFile, &FileData);
#endif	/* _MSC_VER */

	}
	/* !_MSC_VER  while ((direntp = readdir(dirp)) != NULL) */
	/* _MSC_VER   while (!fDone) */

	if (dbgOutput) {
		dbgOutput(OUT_NOISE, "dirent count=%d\n", count);
		dbgOutput(OUT_INFO, "Leaving [%s]\n", dirname);
	}

#ifndef	_MSC_VER
	/* close the directory entry */
	closedir(dirp);
#endif	/* !_MSC_VER */

	/* ***
	 * call provided callback function for this directory
	 * */
	if (mcbuf && mcbuf->postFunc) {
		int rc;
		stat(dirname, &sb);
		rc = (mcbuf->postFunc) (dirname, dirname, &sb, mcbuf); 
	}

	return count;	/* # of entries found */
}

/* public functions
 */
void dirinfo_Report(dirInfo_t *dip, char *name)
{
	fprintf(stdout, "# of %s directories %d\n", name, dip->num_of_directories);
	fprintf(stdout, "# of %s files       %d\n", name, dip->num_of_files);
	fprintf(stdout, "# of %s other types %d\n", name, dip->num_of_others);
}

/* ***********************************
 * support functions
 */
/* condense the path, i.e. remove redundent components */
int condense_path(char *dirpath)
{
	// char *cp;
	int  len = strlen(dirpath);
	struct stat sb;

	if (stat(dirpath, &sb) < 0) return -1;

	/* remove trailing slash (/) */
	if (dirpath[len-1] == '/') dirpath[len-1] = 0;
	if (dirpath[len-1] == '\\') dirpath[len-1] = 0;

/* TODO:  may not need these function */
	/* remove meaningless "/./" components */
	/* adjust overlapping "/../" components */

	return 0;
}

/* public functions
 */
#ifdef	_MSC_VER
#ifndef INVALID_FILE_ATTRIBUTES
#define	INVALID_FILE_ATTRIBUTES	0xFFFFFFFF
#endif
/*
 * Check if provided path is valid.
 *
 * Return:
 *         0	Not a valid path
 *         1	Is  a valid path
 */
int IsValidPath(const char *path)
{
	/* check file attribue */
	return (GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES) ? 0 : 1;
}
/*
 * Check if provided path is a directory.
 *
 * Return:
 *         0	Not a directory
 *         1	Is  a directory
 */
int IsDirectory(const char *path)
{
	int isDir = 0;
	/* check file attribue */
	DWORD dwAttrib = GetFileAttributes(path);
	// fprintf(stdout, "FileAttributes> [%s] attrib=%#x\n", path, dwAttrib);
	if ((dwAttrib != (DWORD) INVALID_FILE_ATTRIBUTES)
		&& (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) isDir++;
	//
	return isDir;
}
/*
 * Check if provided path is a file.
 *
 * Return:
 *         0	Not a file
 *         1	Is  a file
 */
int IsFile(const char *path)
{
	int isFile = 0;
	/* check file attribue */
	DWORD dwAttrib = GetFileAttributes(path);
	// fprintf(stdout, "FileAttributes> [%s] attrib=%#x\n", path, dwAttrib);
	if ((dwAttrib != (DWORD) INVALID_FILE_ATTRIBUTES)
		&& !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) isFile++;
	//
	return isFile;
}
#endif	/* _MSC_VER */

/*
DWORD GetFileAttributes(
	LPCTSTR lpFileName
);

Parameters
	lpFileName 

	[in] Pointer to a null-terminated string that specifies the name
	of a file or directory.

	In the ANSI version of this function, the name is limited to
	MAX_PATH characters.

		Windows Me/98/95:  This string must not exceed MAX_PATH characters.

Return Values

	If the function succeeds, the return value contains the
	attributes of the specified file or directory.

	If the function fails, the return value is INVALID_FILE_ATTRIBUTES.
	To get extended error information, call GetLastError.

	The attributes can be one or more of the following values.

WINNT.H:#define FILE_ATTRIBUTE_READONLY             0x00000001
WINNT.H:#define FILE_ATTRIBUTE_HIDDEN               0x00000002
WINNT.H:#define FILE_ATTRIBUTE_SYSTEM               0x00000004
WINNT.H:#define FILE_ATTRIBUTE_DIRECTORY            0x00000010
WINNT.H:#define FILE_ATTRIBUTE_ARCHIVE              0x00000020
WINNT.H:#define FILE_ATTRIBUTE_ENCRYPTED            0x00000040
WINNT.H:#define FILE_ATTRIBUTE_NORMAL               0x00000080
WINNT.H:#define FILE_ATTRIBUTE_TEMPORARY            0x00000100
WINNT.H:#define FILE_ATTRIBUTE_SPARSE_FILE          0x00000200
WINNT.H:#define FILE_ATTRIBUTE_REPARSE_POINT        0x00000400
WINNT.H:#define FILE_ATTRIBUTE_COMPRESSED           0x00000800
WINNT.H:#define FILE_ATTRIBUTE_OFFLINE              0x00001000
WINNT.H:#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED  0x00002000
*/
