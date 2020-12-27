# makefile for building Gafq
# see ../INSTALL for installation instructions
# see ../Makefile and gafqconf.h for further customization

# == CHANGE THE SETTINGS BELOW TO SUIT YOUR ENVIRONMENT =======================

# Your platform. See PLATS for possible values.
PLAT= linux

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
CORE_O=	gapi.o gcode.o gdebug.o gdo.o gdump.o gfunc.o ggc.o glex.o gmem.o \
	gobject.o gopcodes.o gparser.o gstate.o gstring.o gtable.o gtm.o  \
	gundump.o gvm.o gzio.o
LIB_O=	gauxlib.o gbaselib.o gdblib.o giolib.o gmathlib.o goslib.o gtablib.o \
	gstrlib.o loadlib.o ginit.o

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

gapi.o: gapi.c gafq.h gafqconf.h gapi.h gobject.h glimits.h gdebug.h \
  gstate.h gtm.h gzio.h gmem.h gdo.h gfunc.h ggc.h gstring.h gtable.h \
  gundump.h gvm.h
gauxlib.o: gauxlib.c gafq.h gafqconf.h gauxlib.h
gbaselib.o: gbaselib.c gafq.h gafqconf.h gauxlib.h gafqlib.h
gcode.o: gcode.c gafq.h gafqconf.h gcode.h glex.h gobject.h glimits.h \
  gzio.h gmem.h gopcodes.h gparser.h gdebug.h gstate.h gtm.h gdo.h ggc.h \
  gtable.h
gdblib.o: gdblib.c gafq.h gafqconf.h gauxlib.h gafqlib.h
gdebug.o: gdebug.c gafq.h gafqconf.h gapi.h gobject.h glimits.h gcode.h \
  glex.h gzio.h gmem.h gopcodes.h gparser.h gdebug.h gstate.h gtm.h gdo.h \
  gfunc.h gstring.h ggc.h gtable.h gvm.h
gdo.o: gdo.c gafq.h gafqconf.h gdebug.h gstate.h gobject.h glimits.h gtm.h \
  gzio.h gmem.h gdo.h gfunc.h ggc.h gopcodes.h gparser.h gstring.h \
  gtable.h gundump.h gvm.h
gdump.o: gdump.c gafq.h gafqconf.h gobject.h glimits.h gstate.h gtm.h \
  gzio.h gmem.h gundump.h
gfunc.o: gfunc.c gafq.h gafqconf.h gfunc.h gobject.h glimits.h ggc.h gmem.h \
  gstate.h gtm.h gzio.h
ggc.o: ggc.c gafq.h gafqconf.h gdebug.h gstate.h gobject.h glimits.h gtm.h \
  gzio.h gmem.h gdo.h gfunc.h ggc.h gstring.h gtable.h
ginit.o: ginit.c gafq.h gafqconf.h gafqlib.h gauxlib.h
giolib.o: giolib.c gafq.h gafqconf.h gauxlib.h gafqlib.h
glex.o: glex.c gafq.h gafqconf.h gdo.h gobject.h glimits.h gstate.h gtm.h \
  gzio.h gmem.h glex.h gparser.h gstring.h ggc.h gtable.h
gmathlib.o: gmathlib.c gafq.h gafqconf.h gauxlib.h gafqlib.h
gmem.o: gmem.c gafq.h gafqconf.h gdebug.h gstate.h gobject.h glimits.h \
  gtm.h gzio.h gmem.h gdo.h
loadlib.o: loadlib.c gafq.h gafqconf.h gauxlib.h gafqlib.h
gobject.o: gobject.c gafq.h gafqconf.h gdo.h gobject.h glimits.h gstate.h \
  gtm.h gzio.h gmem.h gstring.h ggc.h gvm.h
gopcodes.o: gopcodes.c gopcodes.h glimits.h gafq.h gafqconf.h
goslib.o: goslib.c gafq.h gafqconf.h gauxlib.h gafqlib.h
gparser.o: gparser.c gafq.h gafqconf.h gcode.h glex.h gobject.h glimits.h \
  gzio.h gmem.h gopcodes.h gparser.h gdebug.h gstate.h gtm.h gdo.h \
  gfunc.h gstring.h ggc.h gtable.h
gstate.o: gstate.c gafq.h gafqconf.h gdebug.h gstate.h gobject.h glimits.h \
  gtm.h gzio.h gmem.h gdo.h gfunc.h ggc.h glex.h gstring.h gtable.h
gstring.o: gstring.c gafq.h gafqconf.h gmem.h glimits.h gobject.h gstate.h \
  gtm.h gzio.h gstring.h ggc.h
gstrlib.o: gstrlib.c gafq.h gafqconf.h gauxlib.h gafqlib.h
gtable.o: gtable.c gafq.h gafqconf.h gdebug.h gstate.h gobject.h glimits.h \
  gtm.h gzio.h gmem.h gdo.h ggc.h gtable.h
gtablib.o: gtablib.c gafq.h gafqconf.h gauxlib.h gafqlib.h
gtm.o: gtm.c gafq.h gafqconf.h gobject.h glimits.h gstate.h gtm.h gzio.h \
  gmem.h gstring.h ggc.h gtable.h
gafq.o: gafq.c gafq.h gafqconf.h gauxlib.h gafqlib.h
gafqc.o: gafqc.c gafq.h gafqconf.h gauxlib.h gdo.h gobject.h glimits.h \
  gstate.h gtm.h gzio.h gmem.h gfunc.h gopcodes.h gstring.h ggc.h \
  gundump.h
gundump.o: gundump.c gafq.h gafqconf.h gdebug.h gstate.h gobject.h \
  glimits.h gtm.h gzio.h gmem.h gdo.h gfunc.h gstring.h ggc.h gundump.h
gvm.o: gvm.c gafq.h gafqconf.h gdebug.h gstate.h gobject.h glimits.h gtm.h \
  gzio.h gmem.h gdo.h gfunc.h ggc.h gopcodes.h gstring.h gtable.h gvm.h
gzio.o: gzio.c gafq.h gafqconf.h glimits.h gmem.h gstate.h gobject.h gtm.h \
  gzio.h
print.o: print.c gdebug.h gstate.h gafq.h gafqconf.h gobject.h glimits.h \
  gtm.h gzio.h gmem.h gopcodes.h gundump.h

# (end of Makefile)
