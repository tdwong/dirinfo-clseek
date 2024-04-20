#
#  Unix Makefile
#
#	native build:	make clseek
#	cross-compile:	make CC=arm-arago-linux-gnueabi-gcc AR=arm-arago-linux-gnueabi-ar clseek
#

# .PHONY: all default
.PHONY: lib

# ~~~

# Ubuntu 16.04 gcc (Ubuntu 5.4.0-6ubuntu1~16.04.12) 5.4.0 20160609
GCC_CFLAGS= -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast
USR_CFLAGS= -DSTDC_HEADERS=1 -DHAVE_STRING_H=1
CFLAGS    = -g $(USR_CFLAGS) $(GCC_CFLAGS)

# ~~~

SRC_DIRINFO	=	\
	mygetoptV2.c	\
	dirinfo_drv.c	\
	dirinfo.c
#	finddir.c
SRC_WHICH	=	\
	which.c		\
	mygetoptV2.c	\
	dirinfo.c
SRC_ISEMPTY	=	\
	isempty.c	\
	mygetoptV2.c	\
	dirinfo.c
SRC_LIBTD	=	\
	mygetoptV2.c	\
	mystropt.c	\
	regex.c		\
	dirinfo.c
SRC_CLSEEK	=	\
	CLSeek.c
#
ALL_SRCS=	\
	$(SRC_CLSEEK)	\
	$(SRC_LIBTD)	\
	$(SRC_ISEMPTY)	\
	$(SRC_WHICH)	\
	$(SRC_DIRINFO)

LIBtd	= libtd.a

OBJ_DIRINFO	=$(SRC_DIRINFO:.c=.o)
OBJ_WHICH	=$(patsubst %.c,%.o,$(SRC_WHICH))
OBJ_ISEMPTY	=$(patsubst %.c,%.o,$(SRC_ISEMPTY))
OBJ_LIBTD	=$(SRC_LIBTD:.c=.o)
OBJ_CLSEEK	=$(SRC_CLSEEK:.c=.o)


#
OBJS=	\
	$(OBJ_DIRINFO)	\
	$(OBJ_WHICH)	\
	$(OBJ_ISEMPTY)	\
	$(OBJ_LIBTD)	\
	$(OBJ_CLSEEK)
LIBS=	$(LIBtd)

# ~~~
CLANG_FORMAT=.clang-format
GIT_RELATED=.git/config _set_git_commit_dates.sh

# ~~~

default: clseek

#
# $@	: the target
# $<	: the dependent
# $^	: ALL dependents
#

dirinfo:	$(OBJ_DIRINFO)
	@echo building $@
	$(CC) $(CFLAGS) -o $@ $^

which:	$(OBJ_WHICH)
	@echo building $@
	$(CC) $(CFLAGS) -o $@ $^

isempty:	$(OBJ_ISEMPTY)
	@echo building $@
	$(CC) $(CFLAGS) -o $@ $^

lib:	$(LIBtd)
$(LIBtd):	$(OBJ_LIBTD)
	@echo building $@
	$(AR) -rc $@ $^

clseek:	$(OBJ_CLSEEK) $(LIBtd)
	@echo building $@
	$(CC) $(CFLAGS) -o $@ $^

# ~~~
mygetopt: mygetoptV2.c mygetopt.h
	$(CC) -g -o $@ -D_TESTDRIVER_ $(GCC_CFLAGS) mygetoptV2.c

# ~~~
.PHONY: clean clean-all distclean
clean:
	$(RM) -v $(OBJS)

clean-all distclean:	clean
	$(RM) -v $(LIBS)
#	$(RM) -v $(EXES)
	$(RM) -rv *.dSYM *.exe
	$(RM) -v cscope.out tags

# ---
#ARCHIVE=CLSeek-$(VERSION).zip
ARCHIVE=CLSeek-$(shell grep "define.*PROGRAMVERSION" CLSeek.c | cut -d\" -f2).zip

.PHONY: distribute tarball echo help
distribute tarball:
	@echo "creating archive: $(ARCHIVE)..."
#	@zip -qu CLSeek-`grep "define.*PROGRAMVERSION" CLSeek.c | cut -d\" -f2`.zip $(ALL_SRCS) *.h *nmak* *.bat *.ico Makefile
	@zip -qur $(ARCHIVE) $(ALL_SRCS) *.h *nmak* *.bat *.ico Makefile *.rc testdirs.zip testdirs.tgz msvc \
		$(CLANG_FORMAT) $(GIT_RELATED)

help:
	@echo "	make clseek"
	@echo "	make isempty"
	@echo "	make which"
	@echo "	make dirinfo"
	@echo "	make distribute | tarball"

VERSION := $(shell grep "define.*PROGRAMVERSION" CLSeek.c | cut -d\" -f2)
echo:
	@echo VERSION=$(VERSION)
	@echo VER= $(shell grep "define.*PROGRAMVERSION" CLSeek.c | cut -d\" -f2)
	@echo \
		clang=$(CLANG_FORMAT) \
		git=$(GIT_RELATED)

# ~~~

# dependency
mygetoptV2.o:	mygetoptV2.c mygetopt.h
mystropt.o:	mystropt.c mystropt.h
regex.o:	regex.c
dirinfo.o:	dirinfo.c dirinfo.h mygetopt.h
CLSeek.o:	CLSeek.c
CLSync.o:	CLSync.c

