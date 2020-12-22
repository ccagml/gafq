/*
** $Id: gafqconf.h,v 1.82.1.7 2008/02/11 16:25:08 roberto Exp $
** Configuration file for Gafq
** See Copyright Notice in gafq.h
*/


#ifndef lconfig_h
#define lconfig_h

#include <limits.h>
#include <stddef.h>


/*
** ==================================================================
** Search for "@@" to find all configurable definitions.
** ===================================================================
*/


/*
@@ GAFQ_ANSI controls the use of non-ansi features.
** CHANGE it (define it) if you want Gafq to avoid the use of any
** non-ansi feature or library.
*/
#if defined(__STRICT_ANSI__)
#define GAFQ_ANSI
#endif


#if !defined(GAFQ_ANSI) && defined(_WIN32)
#define GAFQ_WIN
#endif

#if defined(GAFQ_USE_LINUX)
#define GAFQ_USE_POSIX
#define GAFQ_USE_DLOPEN		/* needs an extra library: -ldl */
#define GAFQ_USE_READLINE	/* needs some extra libraries */
#endif

#if defined(GAFQ_USE_MACOSX)
#define GAFQ_USE_POSIX
#define GAFQ_DL_DYLD		/* does not need extra library */
#endif



/*
@@ GAFQ_USE_POSIX includes all functionallity listed as X/Open System
@* Interfaces Extension (XSI).
** CHANGE it (define it) if your system is XSI compatible.
*/
#if defined(GAFQ_USE_POSIX)
#define GAFQ_USE_MKSTEMP
#define GAFQ_USE_ISATTY
#define GAFQ_USE_POPEN
#define GAFQ_USE_ULONGJMP
#endif


/*
@@ GAFQ_PATH and GAFQ_CPATH are the names of the environment variables that
@* Gafq check to set its paths.
@@ GAFQ_INIT is the name of the environment variable that Gafq
@* checks for initialization code.
** CHANGE them if you want different names.
*/
#define GAFQ_PATH        "GAFQ_PATH"
#define GAFQ_CPATH       "GAFQ_CPATH"
#define GAFQ_INIT	"GAFQ_INIT"


/*
@@ GAFQ_PATH_DEFAULT is the default path that Gafq uses to look for
@* Gafq libraries.
@@ GAFQ_CPATH_DEFAULT is the default path that Gafq uses to look for
@* C libraries.
** CHANGE them if your machine has a non-conventional directory
** hierarchy or if you want to install your libraries in
** non-conventional directories.
*/
#if defined(_WIN32)
/*
** In Windows, any exclamation mark ('!') in the path is replaced by the
** path of the directory of the executable file of the current process.
*/
#define GAFQ_LDIR	"!\\gafq\\"
#define GAFQ_CDIR	"!\\"
#define GAFQ_PATH_DEFAULT  \
		".\\?.gafq;"  GAFQ_LDIR"?.gafq;"  GAFQ_LDIR"?\\init.gafq;" \
		             GAFQ_CDIR"?.gafq;"  GAFQ_CDIR"?\\init.gafq"
#define GAFQ_CPATH_DEFAULT \
	".\\?.dll;"  GAFQ_CDIR"?.dll;" GAFQ_CDIR"loadall.dll"

#else
#define GAFQ_ROOT	"/usr/local/"
#define GAFQ_LDIR	GAFQ_ROOT "share/gafq/5.1/"
#define GAFQ_CDIR	GAFQ_ROOT "lib/gafq/5.1/"
#define GAFQ_PATH_DEFAULT  \
		"./?.gafq;"  GAFQ_LDIR"?.gafq;"  GAFQ_LDIR"?/init.gafq;" \
		            GAFQ_CDIR"?.gafq;"  GAFQ_CDIR"?/init.gafq"
#define GAFQ_CPATH_DEFAULT \
	"./?.so;"  GAFQ_CDIR"?.so;" GAFQ_CDIR"loadall.so"
#endif


/*
@@ GAFQ_DIRSEP is the directory separator (for submodules).
** CHANGE it if your machine does not use "/" as the directory separator
** and is not Windows. (On Windows Gafq automatically uses "\".)
*/
#if defined(_WIN32)
#define GAFQ_DIRSEP	"\\"
#else
#define GAFQ_DIRSEP	"/"
#endif


/*
@@ GAFQ_PATHSEP is the character that separates templates in a path.
@@ GAFQ_PATH_MARK is the string that marks the substitution points in a
@* template.
@@ GAFQ_EXECDIR in a Windows path is replaced by the executable's
@* directory.
@@ GAFQ_IGMARK is a mark to ignore all before it when bulding the
@* gafqopen_ function name.
** CHANGE them if for some reason your system cannot use those
** characters. (E.g., if one of those characters is a common character
** in file/directory names.) Probably you do not need to change them.
*/
#define GAFQ_PATHSEP	";"
#define GAFQ_PATH_MARK	"?"
#define GAFQ_EXECDIR	"!"
#define GAFQ_IGMARK	"-"


/*
@@ GAFQ_INTEGER is the integral type used by gafq_pushinteger/gafq_tointeger.
** CHANGE that if ptrdiff_t is not adequate on your machine. (On most
** machines, ptrdiff_t gives a good choice between int or long.)
*/
#define GAFQ_INTEGER	ptrdiff_t


/*
@@ GAFQ_API is a mark for all core API functions.
@@ GAFQLIB_API is a mark for all standard library functions.
** CHANGE them if you need to define those functions in some special way.
** For instance, if you want to create one Windows DLL with the core and
** the libraries, you may want to use the following definition (define
** GAFQ_BUILD_AS_DLL to get it).
*/
#if defined(GAFQ_BUILD_AS_DLL)

#if defined(GAFQ_CORE) || defined(GAFQ_LIB)
#define GAFQ_API __declspec(dglexport)
#else
#define GAFQ_API __declspec(dllimport)
#endif

#else

#define GAFQ_API		extern

#endif

/* more often than not the libs go together with the core */
#define GAFQLIB_API	GAFQ_API


/*
@@ GAFQI_FUNC is a mark for all extern functions that are not to be
@* exported to outside modules.
@@ GAFQI_DATA is a mark for all extern (const) variables that are not to
@* be exported to outside modules.
** CHANGE them if you need to mark them in some special way. Elf/gcc
** (versions 3.2 and later) mark them as "hidden" to optimize access
** when Gafq is compiled as a shared library.
*/
#if defined(gafqall_c)
#define GAFQI_FUNC	static
#define GAFQI_DATA	/* empty */

#elif defined(__GNUC__) && ((__GNUC__*100 + __GNUC_MINOR__) >= 302) && \
      defined(__ELF__)
#define GAFQI_FUNC	__attribute__((visibility("hidden"))) extern
#define GAFQI_DATA	GAFQI_FUNC

#else
#define GAFQI_FUNC	extern
#define GAFQI_DATA	extern
#endif



/*
@@ GAFQ_QL describes how error messages quote program elements.
** CHANGE it if you want a different appearance.
*/
#define GAFQ_QL(x)	"'" x "'"
#define GAFQ_QS		GAFQ_QL("%s")


/*
@@ GAFQ_IDSIZE gives the maximum size for the description of the source
@* of a function in debug information.
** CHANGE it if you want a different size.
*/
#define GAFQ_IDSIZE	60


/*
** {==================================================================
** Stand-alone configuration
** ===================================================================
*/

#if defined(gafq_c) || defined(gafqall_c)

/*
@@ gafq_stdin_is_tty detects whether the standard input is a 'tty' (that
@* is, whether we're running gafq interactively).
** CHANGE it if you have a better definition for non-POSIX/non-Windows
** systems.
*/
#if defined(GAFQ_USE_ISATTY)
#include <unistd.h>
#define gafq_stdin_is_tty()	isatty(0)
#elif defined(GAFQ_WIN)
#include <io.h>
#include <stdio.h>
#define gafq_stdin_is_tty()	_isatty(_fileno(stdin))
#else
#define gafq_stdin_is_tty()	1  /* assume stdin is a tty */
#endif


/*
@@ GAFQ_PROMPT is the default prompt used by stand-alone Gafq.
@@ GAFQ_PROMPT2 is the default continuation prompt used by stand-alone Gafq.
** CHANGE them if you want different prompts. (You can also change the
** prompts dynamically, assigning to globals _PROMPT/_PROMPT2.)
*/
#define GAFQ_PROMPT		"> "
#define GAFQ_PROMPT2		">> "


/*
@@ GAFQ_PROGNAME is the default name for the stand-alone Gafq program.
** CHANGE it if your stand-alone interpreter has a different name and
** your system is not able to detect that name automatically.
*/
#define GAFQ_PROGNAME		"gafq"


/*
@@ GAFQ_MAXINPUT is the maximum length for an input line in the
@* stand-alone interpreter.
** CHANGE it if you need longer lines.
*/
#define GAFQ_MAXINPUT	512


/*
@@ gafq_readline defines how to show a prompt and then read a line from
@* the standard input.
@@ gafq_saveline defines how to "save" a read line in a "history".
@@ gafq_freeline defines how to free a line read by gafq_readline.
** CHANGE them if you want to improve this functionality (e.g., by using
** GNU readline and history facilities).
*/
#if defined(GAFQ_USE_READLINE)
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#define gafq_readline(L,b,p)	((void)L, ((b)=readline(p)) != NULL)
#define gafq_saveline(L,idx) \
	if (gafq_strlen(L,idx) > 0)  /* non-empty line? */ \
	  add_history(gafq_tostring(L, idx));  /* add it to history */
#define gafq_freeline(L,b)	((void)L, free(b))
#else
#define gafq_readline(L,b,p)	\
	((void)L, fputs(p, stdout), fflush(stdout),  /* show prompt */ \
	fgets(b, GAFQ_MAXINPUT, stdin) != NULL)  /* get line */
#define gafq_saveline(L,idx)	{ (void)L; (void)idx; }
#define gafq_freeline(L,b)	{ (void)L; (void)b; }
#endif

#endif

/* }================================================================== */


/*
@@ GAFQI_GCPAUSE defines the default pause between garbage-collector cycles
@* as a percentage.
** CHANGE it if you want the GC to run faster or slower (higher values
** mean larger pauses which mean slower collection.) You can also change
** this value dynamically.
*/
#define GAFQI_GCPAUSE	200  /* 200% (wait memory to double before next GC) */


/*
@@ GAFQI_GCMUL defines the default speed of garbage collection relative to
@* memory allocation as a percentage.
** CHANGE it if you want to change the granularity of the garbage
** collection. (Higher values mean coarser collections. 0 represents
** infinity, where each step performs a full collection.) You can also
** change this value dynamically.
*/
#define GAFQI_GCMUL	200 /* GC runs 'twice the speed' of memory allocation */



/*
@@ GAFQ_COMPAT_GETN controls compatibility with old getn behavior.
** CHANGE it (define it) if you want exact compatibility with the
** behavior of setn/getn in Gafq 5.0.
*/
#undef GAFQ_COMPAT_GETN

/*
@@ GAFQ_COMPAT_LOADLIB controls compatibility about global loadlib.
** CHANGE it to undefined as soon as you do not need a global 'loadlib'
** function (the function is still available as 'package.loadlib').
*/
#undef GAFQ_COMPAT_LOADLIB

/*
@@ GAFQ_COMPAT_VARARG controls compatibility with old vararg feature.
** CHANGE it to undefined as soon as your programs use only '...' to
** access vararg parameters (instead of the old 'arg' table).
*/
#define GAFQ_COMPAT_VARARG

/*
@@ GAFQ_COMPAT_MOD controls compatibility with old math.mod function.
** CHANGE it to undefined as soon as your programs use 'math.fmod' or
** the new '%' operator instead of 'math.mod'.
*/
#define GAFQ_COMPAT_MOD

/*
@@ GAFQ_COMPAT_LSTR controls compatibility with old long string nesting
@* facility.
** CHANGE it to 2 if you want the old behaviour, or undefine it to turn
** off the advisory error when nesting [[...]].
*/
#define GAFQ_COMPAT_LSTR		1

/*
@@ GAFQ_COMPAT_GFIND controls compatibility with old 'string.gfind' name.
** CHANGE it to undefined as soon as you rename 'string.gfind' to
** 'string.gmatch'.
*/
#define GAFQ_COMPAT_GFIND

/*
@@ GAFQ_COMPAT_OPENLIB controls compatibility with old 'gafqL_openlib'
@* behavior.
** CHANGE it to undefined as soon as you replace to 'gafqL_register'
** your uses of 'gafqL_openlib'
*/
#define GAFQ_COMPAT_OPENLIB

/*
@@ gafqi_apicheck是Gafq-C API使用的断言宏。
**更改gafqi_apicheck，是否要让Gafq在
**参数来自API调用。这可能会使翻译器变慢
**，但在调试接口的C代码时可能非常有用
**与Gafq。一个有用的重新定义是使用assert.h。
*/
#if defined(GAFQ_USE_APICHECK)
#include <assert.h>
#define gafqi_apicheck(L,o)	{ (void)L; assert(o); }
#else
#define gafqi_apicheck(L,o)	{ (void)L; }
#endif

/*
@@ GAFQI_BITSINT定义int的位数。
**如果Gafq无法自动检测到的位数，请在此处更改
**您的机器。可能您不需要更改此设置。
*/
/*避免比较时溢出*/ 
#if INT_MAX-20 < 32760
#define GAFQI_BITSINT	16
#elif INT_MAX > 2147483640L
/* int has at least 32 bits */
#define GAFQI_BITSINT	32
#else
#error "you must define GAFQ_BITSINT with number of bits in an integer"
#endif


/*
@@ GAFQI_UINT32 is an unsigned integer with at least 32 bits.
@@ GAFQI_INT32 is an signed integer with at least 32 bits.
@@ GAFQI_UMEM is an unsigned integer big enough to count the total
@* memory used by Gafq.
@@ GAFQI_MEM is a signed integer big enough to count the total memory
@* used by Gafq.
** CHANGE here if for some weird reason the default definitions are not
** good enough for your machine. (The definitions in the 'else'
** part always works, but may waste space on machines with 64-bit
** longs.) Probably you do not need to change this.
*/
#if GAFQI_BITSINT >= 32
#define GAFQI_UINT32	unsigned int
#define GAFQI_INT32	int
#define GAFQI_MAXINT32	INT_MAX
#define GAFQI_UMEM	size_t
#define GAFQI_MEM	ptrdiff_t
#else
/* 16-bit ints */
#define GAFQI_UINT32	unsigned long
#define GAFQI_INT32	long
#define GAFQI_MAXINT32	LONG_MAX
#define GAFQI_UMEM	unsigned long
#define GAFQI_MEM	long
#endif


/*
@@ GAFQI_MAXCALLS limits the number of nested calls.
** CHANGE it if you need really deep recursive calls. This limit is
** arbitrary; its only purpose is to stop infinite recursion before
** exhausting memory.
*/
#define GAFQI_MAXCALLS	20000


/*
@@ GAFQI_MAXCSTACK limits the number of Gafq stack slots that a C function
@* can use.
** CHANGE it if you need lots of (Gafq) stack space for your C
** functions. This limit is arbitrary; its only purpose is to stop C
** functions to consume unlimited stack space. (must be smaller than
** -GAFQ_REGISTRYINDEX)
*/
//栈的最大值？
#define GAFQI_MAXCSTACK	8000



/*
** {==================================================================
** CHANGE (to smaller values) the following definitions if your system
** has a small C stack. (Or you may want to change them to larger
** values if your system has a large C stack and these limits are
** too rigid for you.) Some of these constants control the size of
** stack-allocated arrays used by the compiler or the interpreter, while
** others limit the maximum number of recursive calls that the compiler
** or the interpreter can perform. Values too large may cause a C stack
** overflow for some forms of deep constructs.
** ===================================================================
*/


/*
@@ GAFQI_MAXCCALLS is the maximum depth for nested C calls (short) and
@* syntactical nested non-terminals in a program.
*/
#define GAFQI_MAXCCALLS		200


/*
@@ GAFQI_MAXVARS is the maximum number of local variables per function
@* (must be smaller than 250).
*/
#define GAFQI_MAXVARS		200


/*
@@ GAFQI_MAXUPVALUES is the maximum number of upvalues per function
@* (must be smaller than 250).
*/
#define GAFQI_MAXUPVALUES	60


/*
@@ GAFQL_BUFFERSIZE is the buffer size used by the gauxlib buffer system.
*/
#define GAFQL_BUFFERSIZE		BUFSIZ

/* }================================================================== */




/*
** {==================================================================
@@ GAFQ_NUMBER is the type of numbers in Gafq.
** CHANGE the following definitions only if you want to build Gafq
** with a number type different from double. You may also need to
** change gafq_number2int & gafq_number2integer.
** ===================================================================
*/

#define GAFQ_NUMBER_DOUBLE
#define GAFQ_NUMBER	double

/*
@@ GAFQI_UACNUMBER is the result of an 'usual argument conversion'
@* over a number.
*/
#define GAFQI_UACNUMBER	double


/*
@@ GAFQ_NUMBER_SCAN is the format for reading numbers.
@@ GAFQ_NUMBER_FMT is the format for writing numbers.
@@ gafq_number2str converts a number to a string.
@@ GAFQI_MAXNUMBER2STR is maximum size of previous conversion.
@@ gafq_str2number converts a string to a number.
*/
#define GAFQ_NUMBER_SCAN		"%lf"
#define GAFQ_NUMBER_FMT		"%.14g"
#define gafq_number2str(s,n)	sprintf((s), GAFQ_NUMBER_FMT, (n))
#define GAFQI_MAXNUMBER2STR	32 /* 16 digits, sign, point, and \0 */
#define gafq_str2number(s,p)	strtod((s), (p))


/*
@@ The gafqi_num* macros define the primitive operations over numbers.
*/
#if defined(GAFQ_CORE)
#include <math.h>
#define gafqi_numadd(a,b)	((a)+(b))
#define gafqi_numsub(a,b)	((a)-(b))
#define gafqi_nummul(a,b)	((a)*(b))
#define gafqi_numdiv(a,b)	((a)/(b))
#define gafqi_nummod(a,b)	((a) - floor((a)/(b))*(b))
#define gafqi_numpow(a,b)	(pow(a,b))
#define gafqi_numunm(a)		(-(a))
#define gafqi_numeq(a,b)		((a)==(b))
#define gafqi_numlt(a,b)		((a)<(b))
#define gafqi_numle(a,b)		((a)<=(b))
#define gafqi_numisnan(a)	(!gafqi_numeq((a), (a)))
#endif


/*
@@ gafq_number2int is a macro to convert gafq_Number to int.
@@ gafq_number2integer is a macro to convert gafq_Number to gafq_Integer.
** CHANGE them if you know a faster way to convert a gafq_Number to
** int (with any rounding method and without throwing errors) in your
** system. In Pentium machines, a naive typecast from double to int
** in C is extremely slow, so any alternative is worth trying.
*/

/* On a Pentium, resort to a trick */
#if defined(GAFQ_NUMBER_DOUBLE) && !defined(GAFQ_ANSI) && !defined(__SSE2__) && \
    (defined(__i386) || defined (_M_IX86) || defined(__i386__))

/* On a Microsoft compiler, use assembler */
#if defined(_MSC_VER)

#define gafq_number2int(i,d)   __asm fld d   __asm fistp i
#define gafq_number2integer(i,n)		gafq_number2int(i, n)

/* the next trick should work on any Pentium, but sometimes clashes
   with a DirectX idiosyncrasy */
#else

union gafqi_Cast { double l_d; long l_l; };
#define gafq_number2int(i,d) \
  { volatile union gafqi_Cast u; u.l_d = (d) + 6755399441055744.0; (i) = u.l_l; }
#define gafq_number2integer(i,n)		gafq_number2int(i, n)

#endif


/* this option always works, but may be slow */
#else
#define gafq_number2int(i,d)	((i)=(int)(d))
#define gafq_number2integer(i,d)	((i)=(gafq_Integer)(d))

#endif

/* }================================================================== */


/*
@@ GAFQI_USER_ALIGNMENT_T is a type that requires maximum alignment.
** CHANGE it if your system requires alignments larger than double. (For
** instance, if your system supports long doubles and they must be
** aligned in 16-byte boundaries, then you should add long double in the
** union.) Probably you do not need to change this.
*/
#define GAFQI_USER_ALIGNMENT_T	union { double u; void *s; long l; }


/*
@@ GAFQI_THROW/GAFQI_TRY define how Gafq does exception handling.
** CHANGE them if you prefer to use longjmp/setjmp even with C++
** or if want/don't to use _longjmp/_setjmp instead of regular
** longjmp/setjmp. By default, Gafq handles errors with exceptions when
** compiling as C++ code, with _longjmp/_setjmp when asked to use them,
** and with longjmp/setjmp otherwise.
*/
#if defined(__cplusplus)
/* C++ exceptions */
#define GAFQI_THROW(L,c)	throw(c)
#define GAFQI_TRY(L,c,a)	try { a } catch(...) \
	{ if ((c)->status == 0) (c)->status = -1; }
#define gafqi_jmpbuf	int  /* dummy variable */

#elif defined(GAFQ_USE_ULONGJMP)
/* in Unix, try _longjmp/_setjmp (more efficient) */
#define GAFQI_THROW(L,c)	_longjmp((c)->b, 1)
#define GAFQI_TRY(L,c,a)	if (_setjmp((c)->b) == 0) { a }
#define gafqi_jmpbuf	jmp_buf

#else
/* default handling with long jumps */
#define GAFQI_THROW(L,c)	longjmp((c)->b, 1)
#define GAFQI_TRY(L,c,a)	if (setjmp((c)->b) == 0) { a }
#define gafqi_jmpbuf	jmp_buf

#endif


/*
@@ GAFQ_MAXCAPTURES is the maximum number of captures that a pattern
@* can do during pattern-matching.
** CHANGE it if you need more captures. This limit is arbitrary.
*/
#define GAFQ_MAXCAPTURES		32


/*
@@ gafq_tmpnam is the function that the OS library uses to create a
@* temporary name.
@@ GAFQ_TMPNAMBUFSIZE is the maximum size of a name created by gafq_tmpnam.
** CHANGE them if you have an alternative to tmpnam (which is considered
** insecure) or if you want the original tmpnam anyway.  By default, Gafq
** uses tmpnam except when POSIX is available, where it uses mkstemp.
*/
#if defined(goslib_c) || defined(gafqall_c)

#if defined(GAFQ_USE_MKSTEMP)
#include <unistd.h>
#define GAFQ_TMPNAMBUFSIZE	32
#define gafq_tmpnam(b,e)	{ \
	strcpy(b, "/tmp/gafq_XXXXXX"); \
	e = mkstemp(b); \
	if (e != -1) close(e); \
	e = (e == -1); }

#else
#define GAFQ_TMPNAMBUFSIZE	L_tmpnam
#define gafq_tmpnam(b,e)		{ e = (tmpnam(b) == NULL); }
#endif

#endif


/*
@@ gafq_popen spawns a new process connected to the current one through
@* the file streams.
** CHANGE it if you have a way to implement it in your system.
*/
#if defined(GAFQ_USE_POPEN)

#define gafq_popen(L,c,m)	((void)L, fflush(NULL), popen(c,m))
#define gafq_pclose(L,file)	((void)L, (pclose(file) != -1))

#elif defined(GAFQ_WIN)

#define gafq_popen(L,c,m)	((void)L, _popen(c,m))
#define gafq_pclose(L,file)	((void)L, (_pclose(file) != -1))

#else

#define gafq_popen(L,c,m)	((void)((void)c, m),  \
		gafqL_error(L, GAFQ_QL("popen") " not supported"), (FILE*)0)
#define gafq_pclose(L,file)		((void)((void)L, file), 0)

#endif

/*
@@ GAFQ_DL_* define which dynamic-library system Gafq should use.
** CHANGE here if Gafq has problems choosing the appropriate
** dynamic-library system for your platform (either Windows' DLL, Mac's
** dyld, or Unix's dlopen). If your system is some kind of Unix, there
** is a good chance that it has dlopen, so GAFQ_DL_DLOPEN will work for
** it.  To use dlopen you also need to adapt the src/Makefile (probably
** adding -ldl to the linker options), so Gafq does not select it
** automatically.  (When you change the makefile to add -ldl, you must
** also add -DGAFQ_USE_DLOPEN.)
** If you do not want any kind of dynamic library, undefine all these
** options.
** By default, _WIN32 gets GAFQ_DL_DLL and MAC OS X gets GAFQ_DL_DYLD.
*/
#if defined(GAFQ_USE_DLOPEN)
#define GAFQ_DL_DLOPEN
#endif

#if defined(GAFQ_WIN)
#define GAFQ_DL_DLL
#endif


/*
@@ GAFQI_EXTRASPACE allows you to add user-specific data in a gafq_State
@* (the data goes just *before* the gafq_State pointer).
** CHANGE (define) this if you really need that. This value must be
** a multiple of the maximum alignment required for your machine.
*/
#define GAFQI_EXTRASPACE		0


/*
@@ gafqi_userstate* allow user-specific actions on threads.
** CHANGE them if you defined GAFQI_EXTRASPACE and need to do something
** extra when a thread is created/deleted/resumed/yielded.
*/
#define gafqi_userstateopen(L)		((void)L)
#define gafqi_userstateclose(L)		((void)L)
#define gafqi_userstatethread(L,L1)	((void)L)
#define gafqi_userstatefree(L)		((void)L)
#define gafqi_userstateresume(L,n)	((void)L)
#define gafqi_userstateyield(L,n)	((void)L)


/*
@@ GAFQ_INTFRMLEN is the length modifier for integer conversions
@* in 'string.format'.
@@ GAFQ_INTFRM_T is the integer type correspoding to the previous length
@* modifier.
** CHANGE them if your system supports long long or does not support long.
*/

#if defined(GAFQ_USELONGLONG)

#define GAFQ_INTFRMLEN		"ll"
#define GAFQ_INTFRM_T		long long

#else

#define GAFQ_INTFRMLEN		"l"
#define GAFQ_INTFRM_T		long

#endif



/* =================================================================== */

/*
** Local configuration. You can use this space to add your redefinitions
** without modifying the main part of the file.
*/



#endif

