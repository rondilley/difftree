dnl Process this file with autoconf to produce a configure script
m4_include([version.m4])
AC_PREREQ([2.59])
AC_INIT([DiffTree], VERSION_NUMBER, [ron.dilley@uberadmin.com], [difftree], [http://www.uberadmin.com/Projects/difftree/])
AC_CONFIG_SRCDIR(src/dt.c)
AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE

dnl Checks for programs
AC_PROG_CC

dnl make /usr/local as the default install dir
AC_PREFIX_DEFAULT(/usr/local)

CFLAGS="${CFLAGS} -I. -I.. -I../include"
LDFLAGS="${LDFLAGS}"

SPLINT="no"
AC_ARG_ENABLE(splint,
    [  --enable-splint         Debug the code with splint],
	SPLINT="yes"      
	CC="splint -D -I"
    ,)

DEBUG="no"
AC_ARG_ENABLE(debug,
    [  --enable-debug          Enable debugging],
    [ if test "$GCC" = yes; then
          DEBUG="yes"
          CFLAGS="${CFLAGS} -DDEBUG -ggdb"
      else
          DEBUG="yes"
          CFLAGS="${CFLAGS} -DDEBUG"
      fi
    ],)

MEM_DEBUG="no"
AC_ARG_ENABLE(memdebug,
    [  --enable-memdebug       Enable memory debugging],
      MEM_DEBUG="yes"
      CFLAGS="${CFLAGS} -DMEM_DEBUG"
    ,)

SHOW_MEM_DEBUG="no"
AC_ARG_ENABLE(showmemdebug,
    [  --enable-showmemdebug   Enable verbose memory debugging],
      SHOW_MEM_DEBUG="yes"
      MEM_DEBUG="yes"
      CFLAGS="${CFLAGS} -DMEM_DEBUG -DSHOW_MEM_DEBUG"
    ,)

GPROF="no"
AC_ARG_ENABLE(gprof,
    [  --enable-gprof          Enable profiler],
    [ if test "$GCC" = yes; then
          GPROF="yes"
          CFLAGS="${CFLAGS} -DGPROF -pg"
      else
          GPROF="yes"
          CFLAGS="${CFLAGS} -DGPROF"
      fi
    ],)

dnl ############# System Dependencies

AC_MSG_CHECKING([for special system dependencies])

AC_CANONICAL_HOST
AC_C_CONST
AC_PROG_GCC_TRADITIONAL

dnl Checks for OS type.
case "$host" in
    *-freebsd*)
        AC_DEFINE([BSD_DERIVED],1,BSD_DERIVED)
        AC_DEFINE([FREEBSD],1,FREEBSD)
        AC_DEFINE([_BSD_SOURCE],1,_BSD_SOURCE)
	LDFLAGS="${LDFLAGS}"
        ;;
    *-netbsd*)
        AC_DEFINE([BSD_DERIVED],1,BSD_DERIVED)
        AC_DEFINE([NETBSD],1,NETBSD)
	AC_DEFINE([__USE_BSD],1,__USE_BSD)
	AC_DEFINE([__FAVOR_BSD],1,__FAVOR_BSD)
        ;;
    *-openbsd*)
        AC_DEFINE([BSD_DERIVED],1,BSD_DERIVED)
        AC_DEFINE([OPENBSD],1,OPENBSD)
	AC_DEFINE([__USE_BSD],1,__USE_BSD)
	AC_DEFINE([__FAVOR_BSD],1,__FAVOR_BSD)
	AC_DEFINE([_BSD_SOURCE],1,_BSD_SOURCE)
	LDFLAGS="${LDFLAGS}"
        ;;
    *-apple*)
        AC_DEFINE([BSD_DERIVED],1,BSD_DERIVED)
        AC_DEFINE([MACOS],1,MACOS)
	AC_DEFINE([_GNU_SOURCE],1,_GNU_SOURCE)
	AC_DEFINE([FTW_STOP],1,FTW_STOP)
	AC_DEFINE([FTW_CONTINUE],0,FTW_CONTINUE)
	AC_DEFINE([FTW_ACTIONRETVAL],16,FTW_ACTIONRETVAL)
        LDFLAGS="${LDFLAGS}"
        ;;
    *-linux*)
        AC_DEFINE([SYSV_DERIVED],1,SYSV_DERIVED)
        AC_DEFINE([LINUX],1,LINUX)
	AC_DEFINE([__USE_BSD],1,__USE_BSD)
	AC_DEFINE([_GNU_SOURCE],1,_GNU_SOURCE)
	AC_DEFINE([__FAVOR_BSD],1,__FAVOR_BSD)
	CFLAGS="${CFLAGS} `getconf LFS64_CFLAGS`"
	LDFLAGS="${LDFLAGS} `getconf LFS64_LDFLAGS` `getconf LFS64_LIBS`"
        ;;
    *-cygwin)
        AC_DEFINE([SYSV_DERIVED],1,SYSV_DERIVED)
        AC_DEFINE([CYGWIN],1,CYGWIN)
	;;
    *-solaris2.10)
        AC_DEFINE([SYSV_DERIVED],1,SYSV_DERIVED)
        AC_DEFINE([SOLARIS],1,SOLARIS)
	AC_DEFINE([SOLARIS10],1,SOLARIS10)
	LDFLAGS="${LDFLAGS}"
        ;;
    *-solaris*)
        AC_DEFINE([SYSV_DERIVED],1,SYSV_DERIVED)
        AC_DEFINE([SOLARIS],1,SOLARIS)
	LDFLAGS="${LDFLAGS}"
        ;;
    *-mingw*)
        AC_DEFINE([SYSV_DERIVED],1,SYSV_DERIVED)
        AC_DEFINE([MINGW],1,MINGW)
	LDFLAGS="${LDFLAGS} -L/c/MinGW/lib -L/c/MinGW/lib/gcc/mingw32/4.6.2 -lmingw32 -lntdll"
        CFLAGS="${CFLAGS} -I/c/MinGW/include -I/c/MinGW/lib/gcc/mingw32/4.6.2/include"
        ;;
    *-cygwin)
        AC_DEFINE([SYSV_DERIVED],1,SYSV_DERIVED)
        AC_DEFINE([CYGWIN],1,CYGWIN)
        ;;
    *-aix*)
        AC_DEFINE([SYSV_DERIVED],1,SYSV_DERIVED)
        AC_DEFINE([AIX],1,AIX)
        ;;
    *-hpux*)
        AC_DEFINE([SYSV_DERIVED],1,SYSV_DERIVED)
        AC_DEFINE([HPUX],1,HPUX)
        ;;
esac

dnl ############## Header Checks

AC_CHECK_HEADERS([arpa/inet.h])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([inttypes.h])
AC_CHECK_HEADERS([libintl.h])
AC_CHECK_HEADERS([memory.h])
AC_CHECK_HEADERS([netdb.h])
AC_CHECK_HEADERS([netinet/in.h])
AC_CHECK_HEADERS([paths.h])
AC_CHECK_HEADERS([stdint.h])
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([string.h])
AC_CHECK_HEADERS([strings.h])
AC_CHECK_HEADERS([sys/ioctl.h])
AC_CHECK_HEADERS([sys/param.h])
AC_CHECK_HEADERS([sys/socket.h])
AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_HEADERS([sys/types.h])
AC_CHECK_HEADERS([sys/resource.h])
AC_CHECK_HEADERS([sys/stat.h])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([getopt.h])
AC_CHECK_HEADERS([stdbool.h])
AC_CHECK_HEADERS([syslog.h])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([wchar.h])
AC_CHECK_HEADERS([ftw.h])
AC_CHECK_HEADERS([pwd.h])
AC_CHECK_HEADERS([signal.h])
AC_CHECK_HEADERS([utime.h])
AC_HEADER_DIRENT
AC_HEADER_STDBOOL


dnl ############## Function checks

AC_FUNC_CLOSEDIR_VOID
AC_FUNC_LSTAT
AC_FUNC_MEMCMP
AC_CHECK_FUNCS([strtoul])
AC_CHECK_FUNCS([bzero])
AC_CHECK_FUNCS([getpwuid])
AC_CHECK_FUNCS([endpwent])
AC_CHECK_FUNCS([fchdir])
AC_CHECK_FUNCS([getcwd])
AC_CHECK_FUNCS([memmove])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([strdup])
AC_CHECK_FUNCS([gethostname])
AC_CHECK_FUNCS([opendir])
AC_CHECK_FUNCS([fstat])
AC_CHECK_FUNCS([ftw])
AC_CHECK_FUNCS([nftw],echo "nftw() found",echo "nftw() not found - using noftw()")
AC_CHECK_FUNCS([strncat])
AC_CHECK_FUNCS([strlcat])
AC_CHECK_FUNCS([snprintf])
AC_CHECK_FUNCS([setrlimit])
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([realpath])
AC_CHECK_FUNCS([strstr])
AC_CHECK_FUNCS([utime])
AC_CHECK_FUNCS([getopt_long])
AC_CHECK_FUNCS([strerror])
AC_CHECK_FUNCS([strrchr])

dnl ############## Type checks

AC_TYPE_INT16_T
AC_TYPE_INT64_T
AC_TYPE_INT32_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UID_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_UINT8_T
AC_STRUCT_TM
AC_STRUCT_ST_BLOCKS
AC_TYPE_OFF_T
AC_TYPE_SIGNAL

AM_CONFIG_HEADER(include/config.h)
AC_PROG_INSTALL
AC_OUTPUT(Makefile src/Makefile dt.1)

BINDIR=`eval echo ${bindir}`; BINDIR=`eval echo ${BINDIR}`;
MANDIR=`eval echo ${mandir}/${mansubdir}`; MANDIR=`eval echo ${MANDIR}`;

echo ""
echo "-----------------------------------------------"
echo "Host type             : ${host}"
echo "Compiler              : ${CC}"
echo "Compiler flags        : ${CFLAGS}"
echo "Linker flags          : ${LDFLAGS}"
echo "Libraries             : ${LIBS}"
echo ""
echo "Binary                : ${BINDIR}"
echo "Manual pages          : ${MANDIR}"
echo ""
echo "Enable debugging      : ${DEBUG}"
echo "Enable mem debugging  : ${MEM_DEBUG}"
echo "Show mem debugging    : ${SHOW_MEM_DEBUG}"
echo "-----------------------------------------------"
echo ""
