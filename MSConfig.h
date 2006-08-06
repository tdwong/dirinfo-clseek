/*
 * MSConfig.h
 *
 * Includes definitions and macros specific to using MS Visual C++
 */

#ifndef	_MSCONFIG_H_
#define	_MSCONFIG_H_

/* Following settings will be valid only when using MSVC
 */

/* Define to off_t `long' if <sys/types.h> doesn't define. */
#ifndef _OFF_T_DEFINED
typedef long off_t;
typedef long _off_t;
#define _OFF_T_DEFINED
#endif

/* Define to size_t `unsigned' if <sys/types.h> doesn't define. */
/* #undef size_t */
#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif

/* _MSC_VER is defined by all Microsoft C products
 */
/* in IO.H */
#define access    _access
#define chmod     _chmod
#define close     _close
#define creat     _creat
#define dup       _dup
#define dup2      _dup2
#define eof       _eof
#define lseek     _lseek
#define mktemp    _mktemp
#define open      _open
#define read      _read
#define tell      _tell
#define umask     _umask
#define unlink    _unlink
#define utime     _utime
#define write     _write
//#define fstat     _fstat
/* in MALLOC.H */
#define alloca    _alloca	/* allocate memory on the stack */
/* direct.h */
#define chdir     _chdir
#define getcwd     _getcwd
#define mkdir(path,mask)     _mkdir(path)
#define rmdir     _rmdir
/**/
#define O_RDONLY  _O_RDONLY
#define O_WRONLY  _O_WRONLY
#define O_RDWR    _O_RDWR
#define O_APPEND  _O_APPEND
#define O_CREAT   _O_CREAT
#define O_TRUNC   _O_TRUNC
#define O_EXCL    _O_EXCL
#define O_TEXT    _O_TEXT
#define O_BINARY  _O_BINARY
/**/
#define S_ISFIFO(mode)  ((mode)&_S_IFIFO)
#define S_ISCHR(mode)   ((mode)&_S_IFCHR)
#define S_ISDIR(mode)   ((mode)&_S_IFDIR)
#define S_ISBLK(mode)   ((mode)&(_S_IFCHR|_S_IFDIR))
#define S_ISREG(mode)   ((mode)&_S_IFREG)
#define S_ISLNK(mode)   na-in-msvc
#define S_ISSOCK(mode)  na-in-msvc
/**/
// extern int open(), read(), write(), close(), lseek();

#ifndef	PATH_MAX
#define	PATH_MAX	512
#endif

#endif	/* _MSCONFIG_H_ */
