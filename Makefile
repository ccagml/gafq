# makefile for building Gafq
# see ../INSTALL for installation instructions
# see ../Makefile and gafqconf.h for further customization

# == CHANGE THE SETTINGS BELOW TO SUIT YOUR ENVIRONMENT =======================

# Your platform. See PLATS for possible values.
PLAT= macosx

CC= gcc

# CFLAGS= -O2 -Wall $(MYCFLAGS)
# 调试
CFLAGS= -O0 -Wall -g $(MYCFLAGS)
AR= ar rcu
RANLIB= ranlib
RM= rm -f
LIBS= -lm $(MYLIBS)

MYCFLAGS=
MYLDFLAGS=
MYLIBS=

# == END OF USER SETTINGS. NO NEED TO CHANGE ANYTHING BELOW THIS LINE =========

PLATS= aix ansi bsd freebsd generic linux macosx mingw posix solaris

GAFQ_A=	libgafq.a
CORE_O=	lapi.o lcode.o ldebug.o ldo.o ldump.o lfunc.o lgc.o llex.o lmem.o \
	lobject.o lopcodes.o lparser.o lstate.o lstring.o ltable.o ltm.o  \
	lundump.o lvm.o lzio.o
LIB_O=	lauxlib.o lbaselib.o ldblib.o liolib.o lmathlib.o loslib.o ltablib.o \
	lstrlib.o loadlib.o linit.o

GAFQ_T=	gafq
GAFQ_O=	gafq.o

GAFQC_T=	gafqc
GAFQC_O=	gafqc.o print.o

ALL_O= $(CORE_O) $(LIB_O) $(GAFQ_O) $(GAFQC_O)
ALL_T= $(GAFQ_A) $(GAFQ_T) $(GAFQC_T)
ALL_A= $(GAFQ_A)

default: $(PLAT)

all:	$(ALL_T)

o:	$(ALL_O)

a:	$(ALL_A)

$(GAFQ_A): $(CORE_O) $(LIB_O)
	$(AR) $@ $(CORE_O) $(LIB_O)	# DLL needs all object files
	$(RANLIB) $@

$(GAFQ_T): $(GAFQ_O) $(GAFQ_A)
	$(CC) -o $@ $(MYLDFLAGS) $(GAFQ_O) $(GAFQ_A) $(LIBS)

$(GAFQC_T): $(GAFQC_O) $(GAFQ_A)
	$(CC) -o $@ $(MYLDFLAGS) $(GAFQC_O) $(GAFQ_A) $(LIBS)

clean:
	$(RM) $(ALL_T) $(ALL_O)

depend:
	@$(CC) $(CFLAGS) -MM l*.c print.c

echo:
	@echo "PLAT = $(PLAT)"
	@echo "CC = $(CC)"
	@echo "CFLAGS = $(CFLAGS)"
	@echo "AR = $(AR)"
	@echo "RANLIB = $(RANLIB)"
	@echo "RM = $(RM)"
	@echo "MYCFLAGS = $(MYCFLAGS)"
	@echo "MYLDFLAGS = $(MYLDFLAGS)"
	@echo "MYLIBS = $(MYLIBS)"

# convenience targets for popular platforms

none:
	@echo "Please choose a platform:"
	@echo "   $(PLATS)"

aix:
	$(MAKE) all CC="xlc" CFLAGS="-O2 -DGAFQ_USE_POSIX -DGAFQ_USE_DLOPEN" MYLIBS="-ldl" MYLDFLAGS="-brtl -bexpall"

ansi:
	$(MAKE) all MYCFLAGS=-DGAFQ_ANSI

bsd:
	$(MAKE) all MYCFLAGS="-DGAFQ_USE_POSIX -DGAFQ_USE_DLOPEN" MYLIBS="-Wl,-E"

freebsd:
	$(MAKE) all MYCFLAGS="-DGAFQ_USE_LINUX" MYLIBS="-Wl,-E -lreadline"

generic:
	$(MAKE) all MYCFLAGS=

linux:
	$(MAKE) all MYCFLAGS=-DGAFQ_USE_LINUX MYLIBS="-Wl,-E -ldl -lreadline -lhistory -lncurses"

macosx:
	$(MAKE) all MYCFLAGS=-DGAFQ_USE_LINUX MYLIBS="-lreadline"
# use this on Mac OS X 10.3-
#	$(MAKE) all MYCFLAGS=-DGAFQ_USE_MACOSX

mingw:
	$(MAKE) "GAFQ_A=gafq51.dll" "GAFQ_T=gafq.exe" \
	"AR=$(CC) -shared -o" "RANLIB=strip --strip-unneeded" \
	"MYCFLAGS=-DGAFQ_BUILD_AS_DLL" "MYLIBS=" "MYLDFLAGS=-s" gafq.exe
	$(MAKE) "GAFQC_T=gafqc.exe" gafqc.exe

posix:
	$(MAKE) all MYCFLAGS=-DGAFQ_USE_POSIX

solaris:
	$(MAKE) all MYCFLAGS="-DGAFQ_USE_POSIX -DGAFQ_USE_DLOPEN" MYLIBS="-ldl"

# list targets that do not create files (but not all makes understand .PHONY)
.PHONY: all $(PLATS) default o a clean depend echo none

# DO NOT DELETE

lapi.o: lapi.c gafq.h gafqconf.h lapi.h lobject.h llimits.h ldebug.h \
  lstate.h ltm.h lzio.h lmem.h ldo.h lfunc.h lgc.h lstring.h ltable.h \
  lundump.h lvm.h
lauxlib.o: lauxlib.c gafq.h gafqconf.h lauxlib.h
lbaselib.o: lbaselib.c gafq.h gafqconf.h lauxlib.h gafqlib.h
lcode.o: lcode.c gafq.h gafqconf.h lcode.h llex.h lobject.h llimits.h \
  lzio.h lmem.h lopcodes.h lparser.h ldebug.h lstate.h ltm.h ldo.h lgc.h \
  ltable.h
ldblib.o: ldblib.c gafq.h gafqconf.h lauxlib.h gafqlib.h
ldebug.o: ldebug.c gafq.h gafqconf.h lapi.h lobject.h llimits.h lcode.h \
  llex.h lzio.h lmem.h lopcodes.h lparser.h ldebug.h lstate.h ltm.h ldo.h \
  lfunc.h lstring.h lgc.h ltable.h lvm.h
ldo.o: ldo.c gafq.h gafqconf.h ldebug.h lstate.h lobject.h llimits.h ltm.h \
  lzio.h lmem.h ldo.h lfunc.h lgc.h lopcodes.h lparser.h lstring.h \
  ltable.h lundump.h lvm.h
ldump.o: ldump.c gafq.h gafqconf.h lobject.h llimits.h lstate.h ltm.h \
  lzio.h lmem.h lundump.h
lfunc.o: lfunc.c gafq.h gafqconf.h lfunc.h lobject.h llimits.h lgc.h lmem.h \
  lstate.h ltm.h lzio.h
lgc.o: lgc.c gafq.h gafqconf.h ldebug.h lstate.h lobject.h llimits.h ltm.h \
  lzio.h lmem.h ldo.h lfunc.h lgc.h lstring.h ltable.h
linit.o: linit.c gafq.h gafqconf.h gafqlib.h lauxlib.h
liolib.o: liolib.c gafq.h gafqconf.h lauxlib.h gafqlib.h
llex.o: llex.c gafq.h gafqconf.h ldo.h lobject.h llimits.h lstate.h ltm.h \
  lzio.h lmem.h llex.h lparser.h lstring.h lgc.h ltable.h
lmathlib.o: lmathlib.c gafq.h gafqconf.h lauxlib.h gafqlib.h
lmem.o: lmem.c gafq.h gafqconf.h ldebug.h lstate.h lobject.h llimits.h \
  ltm.h lzio.h lmem.h ldo.h
loadlib.o: loadlib.c gafq.h gafqconf.h lauxlib.h gafqlib.h
lobject.o: lobject.c gafq.h gafqconf.h ldo.h lobject.h llimits.h lstate.h \
  ltm.h lzio.h lmem.h lstring.h lgc.h lvm.h
lopcodes.o: lopcodes.c lopcodes.h llimits.h gafq.h gafqconf.h
loslib.o: loslib.c gafq.h gafqconf.h lauxlib.h gafqlib.h
lparser.o: lparser.c gafq.h gafqconf.h lcode.h llex.h lobject.h llimits.h \
  lzio.h lmem.h lopcodes.h lparser.h ldebug.h lstate.h ltm.h ldo.h \
  lfunc.h lstring.h lgc.h ltable.h
lstate.o: lstate.c gafq.h gafqconf.h ldebug.h lstate.h lobject.h llimits.h \
  ltm.h lzio.h lmem.h ldo.h lfunc.h lgc.h llex.h lstring.h ltable.h
lstring.o: lstring.c gafq.h gafqconf.h lmem.h llimits.h lobject.h lstate.h \
  ltm.h lzio.h lstring.h lgc.h
lstrlib.o: lstrlib.c gafq.h gafqconf.h lauxlib.h gafqlib.h
ltable.o: ltable.c gafq.h gafqconf.h ldebug.h lstate.h lobject.h llimits.h \
  ltm.h lzio.h lmem.h ldo.h lgc.h ltable.h
ltablib.o: ltablib.c gafq.h gafqconf.h lauxlib.h gafqlib.h
ltm.o: ltm.c gafq.h gafqconf.h lobject.h llimits.h lstate.h ltm.h lzio.h \
  lmem.h lstring.h lgc.h ltable.h
gafq.o: gafq.c gafq.h gafqconf.h lauxlib.h gafqlib.h
gafqc.o: gafqc.c gafq.h gafqconf.h lauxlib.h ldo.h lobject.h llimits.h \
  lstate.h ltm.h lzio.h lmem.h lfunc.h lopcodes.h lstring.h lgc.h \
  lundump.h
lundump.o: lundump.c gafq.h gafqconf.h ldebug.h lstate.h lobject.h \
  llimits.h ltm.h lzio.h lmem.h ldo.h lfunc.h lstring.h lgc.h lundump.h
lvm.o: lvm.c gafq.h gafqconf.h ldebug.h lstate.h lobject.h llimits.h ltm.h \
  lzio.h lmem.h ldo.h lfunc.h lgc.h lopcodes.h lstring.h ltable.h lvm.h
lzio.o: lzio.c gafq.h gafqconf.h llimits.h lmem.h lstate.h lobject.h ltm.h \
  lzio.h
print.o: print.c ldebug.h lstate.h gafq.h gafqconf.h lobject.h llimits.h \
  ltm.h lzio.h lmem.h lopcodes.h lundump.h

# (end of Makefile)
