SHELL := /bin/bash

## Supported values: gcc (and gcc-*), clang (and clang-*)
## Ensure that if you update this, you correctly change the TOOLCHAIN variable, as failing to do so leads to incorrect arguments
CC=gcc
LD=gcc
## Possible values: clang, gcc
## This affects what parameters are passed
TOOLCHAIN=gcc

## Replace this if you are using a different cURL
INCLUDES   =-lcurl
INCLUDES   =-L/mingw64/lib -lcurl -lmingw32 -lgcc -lmingwex -lkernel32 -lpthread -ladvapi32 -lshell32 -luser32 -lkernel32 -lmingw32 -lgcc -lmingwex -lkernel32 -lidn2 -lws2_32 -lbcrypt -lz -lbrotlidec -lbrotlicommon -lzstd -lwldap32 -lpsl -lssh2 -ladvapi32 -lcrypt32 -lbrotlidec -lbrotlicommon -lzstd -lpsl -lunistring -lws2_32 -liconv -lidn2 -liconv -lunistring -lssh2 -lws2_32 -lcrypt32 -lbcrypt -lz

COREFLAGS  =-Isrc
## Replace these for non-GCC compilers
COREFLAGS +=-fPIC -Wall
COREFLAGS += -std=c23
ifeq ($(TOOLCHAIN), gcc)
## GCC supports LTO, we want that
COREFLAGS += -flto
endif

CFLAGS       =$(COREFLAGS) -Wall -Wextra -Wpedantic -Wformat=2
HFLAGS       =$(COREFLAGS) -Wall -Wextra -Wpedantic -Wformat=2

LINKFLAGS    =-fPIC
RELEASEFLAGS =-DRELEASE -O3
DEBUGFLAGS   =-DDEBUG -Og -ggdb3

ifeq ($(TOOLCHAIN), gcc)
RELEASEFLAGS += -fgcse-sm -fgcse-las
DEBUGFLAGS   += -fanalyzer
else ifeq ($(TOOLCHAIN), clang)
CFLAGS += -Wno-gnu-zero-variadic-macro-arguments
HFLAGS += -Wno-gnu-zero-variadic-macro-arguments
endif

ifeq ($(TOOLCHAIN), gcc)
PCH               =common.h.gch
RELEASE_PCH_FLAGS =-Winvalid-pch -include $(OBJDIR)/release/common.h
DEBUG_PCH_FLAGS   =-Winvalid-pch -include $(OBJDIR)/debug/common.h
else ifeq ($(TOOLCHAIN), clang)
PCH               =common.h.pch
RELEASE_PCH_FLAGS =-Winvalid-pch -include-pch $(OBJDIR)/release/$(PCH)
DEBUG_PCH_FLAGS   =-Winvalid-pch -include-pch $(OBJDIR)/debug/$(PCH)
else
## If no compiler is specified, fake PCH as "common.h"
PCH               =common.h
RELEASE_PCH_FLAGS =-include src/common.h
DEBUG_PCH_FLAGS   =-include src/common.h
endif


## These should only be changed in CI/CD builds
OBJDIR=obj
DISTDIR=dist
OUT_RELEASE=$(DISTDIR)/massget
OUT_DEBUG=$(DISTDIR)/massget_debug

ifneq ($(MSYSTEM),)
OUT_RELEASE:=$(OUT_RELEASE).exe
OUT_DEBUG:=$(OUT_DEBUG).exe
endif

## Automatic file detection
_C=$(wildcard src/*.c)
_H=$(wildcard src/*.h)
_OBJ=$(patsubst src/%.c,%.o,$(_C))
_OBJ_RELEASE=$(patsubst %,$(OBJDIR)/release/%,$(_OBJ))
_OBJ_DEBUG=$(patsubst %,$(OBJDIR)/debug/%,$(_OBJ))

## Runnable commands
all: debug release
debug: $(OUT_DEBUG)
release: $(OUT_RELEASE)
clean:
	rm -rf $(OBJDIR) $(DISTDIR)
configure:
	@python ./configure.py

.PHONY: all debug release clean configure


### RELEASE ###
$(OUT_RELEASE): $(_OBJ_RELEASE) $(DISTDIR)/.marker
	echo $()
	$(LD) $(LINKFLAGS) $(RELEASEFLAGS) -o $@ $(_OBJ_RELEASE) $(INCLUDES)

$(OBJDIR)/release/%.o: src/%.c $(OBJDIR)/release/$(PCH) $(_H) $(OBJDIR)/.marker
	$(CC) $(CFLAGS) $(RELEASEFLAGS) $(RELEASE_PCH_FLAGS) -c -o $@ $<


### DEBUG ###
$(OUT_DEBUG): $(_OBJ_DEBUG) $(DISTDIR)/.marker
	$(LD) $(LINKFLAGS) $(DEBUGFLAGS) -o $@ $(_OBJ_DEBUG) $(INCLUDES)

$(OBJDIR)/debug/%.o: src/%.c $(OBJDIR)/debug/$(PCH) $(_H) $(OBJDIR)/.marker
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(DEBUG_PCH_FLAGS) -c -o $@ $<


## These ensure our output folders exist, else GCC/Clang will complain the output folders don't exist
## We suppress the output of the touch commands to prevent console spam
$(OBJDIR)/.marker: $(OBJDIR)/debug/.marker $(OBJDIR)/release/.marker
	mkdir $(OBJDIR) -p
	@touch $(OBJDIR)/.marker

$(OBJDIR)/release/.marker:
	mkdir $(OBJDIR)/release -p
	@touch $(OBJDIR)/release/.marker

$(OBJDIR)/debug/.marker:
	mkdir $(OBJDIR)/debug -p
	@touch $(OBJDIR)/debug/.marker

$(DISTDIR)/.marker:
	mkdir $(DISTDIR) -p
	@touch $(DISTDIR)/.marker

ifeq ($(TOOLCHAIN), gcc)

## Pre-compiled headers for GCC
$(OBJDIR)/release/common.h.gch: src/common.h $(_H) Makefile $(OBJDIR)/.marker
	@cp src/common.h obj/release/common.h
	$(CC) -c -x c-header $(HFLAGS) $(RELEASEFLAGS) obj/release/common.h -o obj/release/common.h.gch
$(OBJDIR)/debug/common.h.gch: src/common.h $(_H) Makefile $(OBJDIR)/.marker $(OBJDIR)/debug/.marker
	@cp src/common.h obj/debug/common.h
	$(CC) -c -x c-header $(HFLAGS) $(DEBUGFLAGS) obj/debug/common.h -o obj/debug/common.h.gch

else ifeq ($(TOOLCHAIN),clang)

## Pre-compiled headers for Clang
$(OBJDIR)/release/common.h.pch: src/common.h $(_H) Makefile $(OBJDIR)/.marker
	$(CC) -c -x c-header $(HFLAGS) $(RELEASEFLAGS) src/common.h -o obj/release/common.h.pch
$(OBJDIR)/debug/common.h.pch: src/common.h $(_H) Makefile $(OBJDIR)/.marker
	$(CC) -c -x c-header $(HFLAGS) $(DEBUGFLAGS) src/common.h -o obj/debug/common.h.pch

else

$(OBJDIR)/release/common.h: src/common.h $(_H) Makefile $(OBJDIR)/.marker
	@cp src/common.h obj/release/common.h

$(OBJDIR)/debug/common.h: src/common.h $(_H) Makefile $(OBJDIR)/.marker
	@cp src/common.h obj/debug/common.h

endif

## Ignore other .h files
%.h: