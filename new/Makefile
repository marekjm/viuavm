################################################################################
# Base directories
#
# See https://www.gnu.org/prep/standards/html_node/Directory-Variables.html for
# more information.
#
# See https://www.gnu.org/prep/standards/html_node/DESTDIR.html for explanation
# of how DESTDIR is used in this Makefile.
prefix=/usr/local
exec_prefix=$(prefix)
libdir=$(exec_prefix)/lib
libexecdir=$(exec_prefix)/libexec
bindir=$(exec_prefix)/bin
sharedir=$(prefix)/share
mandir=$(sharedir)/man

# exedir is a non-standard variable. It is used to avoid having to repeat the
#
# 	$(libexecdir)/viua
#
# path everywhere. Instead, one can just write $(exedir) for the internal
# executables. The basic idea for a Viua installation is this:
#
#  - the frontend script ie, the "viua" command, is installed in a directory
#    which exists on PATH and is the entry point for everything else (think
#    something like Git, with the main command driving all the subcommands).
#
#  - the internal executables (asm, vm, and others) are installed in libexec,
#    which tends not to be present in PATH. Users can still access them directly
#    by specifying the full path to the commands.
#
# This way we do not have to pollute the main "namespace" with viua-foos and
# viua-bars. See https://refspecs.linuxfoundation.org/FHS_3.0/fhs/ch04s07.html
# for more information about the libexec directory.
exedir=$(libexecdir)/viua


################################################################################
# Build directories
BUILD_PREFIX=./build
BUILD_LIBDIR=$(BUILD_PREFIX)/lib
BUILD_EXEDIR=$(BUILD_PREFIX)/libexec/viua
BUILD_BINDIR=$(BUILD_PREFIX)/bin
BUILD_SHAREDIR=$(BUILD_PREFIX)/share
BUILD_MANDIR=$(BUILD_SHAREDIR)/man


################################################################################
# Source and header directories
sourcedir=src
headerdir=include

################################################################################
# Platform selection
#
# Platforms are not created equal (even if somewhat compatible) and some need
# special treatment. This is the first place where this becomes relevant.
platform?=$(shell uname -s)
include Makefile.d/Platform/$(platform)


################################################################################
# Compilation flags
#
# This is mostly platform-independent. We select the compiler, the standard, and
# some basic presets like optimisation level, debugging symbols, and sanitisers.
CXX?=g++
CXXSTD?=c++23
PRESET?=default
include Makefile.d/Preset/$(PRESET)

# Only include extra warnings if there is no BUILD_SUBMITTER ie, when the build
# is running in local environment. The CI runs with a basic set of common
# warning flags that should work on any version of GCC and Clang.
ifeq ($(BUILD_SUBMITTER),)
-include Makefile.d/Warnings/$(CXX)
endif

CXXFLAGS_INCLUDE=\
				 $(PLATFORM_INCLUDE_FLAGS) \
				 -I$(headerdir) \
				 -I./3rd-party


################################################################################
#
# All Feature/FOO files export several variables if enabled:
#
#  - FEATURE_FOO: a -D... flag defining a C preprocessor macro enabling the
#    feature
#  - LIBS_FOO: a set of -l... flags linking to necessary libraries
include Makefile.d/Feature/BLAKE3
include Makefile.d/Feature/STRERRORNAME_NP
include Makefile.d/Feature/IO_IMPL


FEATURE_DETECT_FLAGS::=\
	$(FEATURE_STRERRORNAME_NP) \
	$(FEATURE_BLAKE3) \
	$(FEATURE_IO_IMPL)


# To change version, edit the VERSION file in the repository's root directory.
VIUAVM_VERSION:=$(shell sh ./scripts/make_version.sh short)
VIUAVM_VERSION_FULL:=$(shell sh ./scripts/make_version.sh full)
VERSION_FLAGS=\
			  -DVIUAVM_PLATFORM='"$(platform)"' \
			  -DVIUAVM_VERSION='"$(VIUAVM_VERSION)"' \
			  -DVIUAVM_VERSION_FULL='"$(VIUAVM_VERSION_FULL)"' \
			  -DVIUAVM_IO_IMPL_HUMAN_READABLE='"$(io_impl)"' \
			  -DVIUAVM_CXX_PRESET='"$(PRESET)"' \
			  -DVIUAVM_CXX_OPTIONS='"$(CXXFLAGS_OPTIMISATION) $(CXXFLAGS_DEBUG) $(CXXFLAGS_SANITISER) $(CXXFLAGS_OPTION)"' \
			  -DCXX='"$(CXX)"' \
			  -DCXXVERSION='"$(shell $(CXX) -dumpversion)"' \
			  -DCXXSTD='"$(CXXSTD)"'


################################################################################
# Automatic prerequisites (header dependencies)
#
# https://www.gnu.org/software/make/manual/html_node/Automatic-Prerequisites.html
# contains more information about generating automatic prerequisites.
#
# One difference between what the manual says and what we do is that we keep the
# dependency files in the build, not source, directory. This makes cleaning the
# repository much easier--just nuke the build directory and you are done--and
# does not pollute the source directory with automatically generated files.
#
# See https://make.mad-scientist.net/papers/advanced-auto-dependency-generation/
# for a modern, fully-automated and efficient take on automatic dependency
# tracking. What is used here is based on the solution presented in the
# aforementioned article. If you are reading this Makefile, I would suggest
# taking a brief detour to read the article.
DEPFLAGS=-MT "$@ $(@:.o=.d)" -MMD

CXXFLAGS=\
		 $(VERSION_FLAGS) \
		 -std=$(CXXSTD) \
		 -DINSTALL_PREFIX='"$(prefix)"' \
		 $(FEATURE_DETECT_FLAGS) \
		 $(CXXFLAGS_OPTIMISATION) \
		 $(CXXFLAGS_DEBUG) \
		 $(CXXFLAGS_SANITISER) \
		 $(CXXFLAGS_OPTION) \
		 $(CXXFLAGS_WARNING) \
		 $(DEPFLAGS) \
		 -Wall \
		 -Wextra \
		 -Wpedantic \
		 -Werror \
		 -Wfatal-errors \
		 $(CXXFLAGS_NOERROR) \
		 $(CXXFLAGS_INCLUDE)

CXXLIBS=\
		$(PLATFORM_LIBS) \
		-lb2 \
		$(LIBS_BLAKE3) \
		-lmd \
		-luuid \
		$(LIBS_IO_IMPL)


################################################################################
# For reproducible builds
export SOURCE_DATE_EPOCH=$(shell ./scripts/make_source_date_epoch.sh)


################################################################################
# High-level targets
#
# See https://www.gnu.org/prep/standards/html_node/Standard-Targets.html for
# more information about the standard targets intended for user consumption.
#
# This Makefile deviates from the standard by having a "test" target instead of
# a "check" target.
all: all-silent
	@echo "version: viua $(VIUAVM_VERSION_FULL)"
	@$(BUILD_EXEDIR)/vm --built-with

all-silent: \
	man \
	$(BUILD_EXEDIR)/readelf \
	$(BUILD_EXEDIR)/dis \
	$(BUILD_EXEDIR)/asm \
	$(BUILD_EXEDIR)/ld \
	$(BUILD_EXEDIR)/vm \
	$(BUILD_BINDIR)/viua

dist: all
	mkdir -p viua-$(VIUAVM_VERSION)
	make DESTDIR=viua-$(VIUAVM_VERSION) install
	tar -cf viua-$(VIUAVM_VERSION).tar viua-$(VIUAVM_VERSION)
	zstd \
		--compress \
		--threads 0 \
		-9 \
		--force \
		--quiet \
		viua-$(VIUAVM_VERSION).tar \
		-o viua-$(VIUAVM_VERSION).tar.zstd

test_prefix?=$(BUILD_PREFIX)
test_exec_prefix?=$(test_prefix)
test: all
	test_prefix=$(test_prefix) test_exec_prefix=$(test_exec_prefix) \
		$(python3) ./tests/suite.py $(TESTS)

.PHONY: clean-test
clean-test:
	find ./tests -type f -name '*.elf' -delete
	find ./tests -type f -name '*.o' -delete
	find ./tests -type f -name '*.asm~' -delete
	find ./tests -type f -name '*.log' -delete
	find ./tests -type d -name '.cache' -print0 |\
		xargs -0 -I CACHE_DIR find CACHE_DIR -type f -delete
	find ./tests -type d -name '.cache' -delete

# For clean and mostlyclean use
#
# 	find $(BUILD_PREFIX) ... || true
#
# to avoid these targets failing in fresh clones when "make clean" is executed
# before the ./build directory has been created.
.PHONY: clean
clean: clean-test
	find $(BUILD_PREFIX) -type f -delete || true
	find $(BUILD_PREFIX) -mindepth 1 -type d -delete || true

.PHONY: mostlyclean
mostlyclean: clean-test
	find $(BUILD_PREFIX) -type f -not -path '*/share/man/*' -and -not -name '*.d' -delete || true

distclean: clean

VIUA_TAG_DATESTAMP::=$(shell git show --date=short HEAD |\
				   awk '/^Date:/ { print $$2 }')
VIUA_TAG_VERSION=$(VIUAVM_VERSION)

uninstall:
	rm -r $(DESTDIR)$(exedir)
	rm -r $(DESTDIR)$(bindir)/viua
	rm -v $(DESTDIR)$(mandir)/man*/viua*

install: all-silent $(prefix)
	$(install) \
		-m 755 \
		-D --target-directory $(DESTDIR)$(exedir) \
		$(BUILD_EXEDIR)/*
	$(install) \
		-m 755 \
		-D --target-directory $(DESTDIR)$(bindir) \
		$(BUILD_BINDIR)/viua
	sed -i -e "s#^prefix=.*#prefix=$(shell realpath $(prefix))#" $(DESTDIR)$(bindir)/viua
	$(install) \
		-m 644 \
		-D --target-directory $(DESTDIR)$(mandir)/man1 \
		$(BUILD_MANDIR)/man1/*.1
	ln -sf viua.1 $(DESTDIR)$(mandir)/man1/viua-viua.1
	$(install) \
		-m 644 \
		-D --target-directory $(DESTDIR)$(mandir)/man5 \
		$(BUILD_MANDIR)/man5/*.5

# This is to fix CI on Sourcehut. For some reason, Make evaluates the
#
# 	$(shell realpath $(prefix))
#
# expression before the first install(1) command is executed, and, predictably,
# realpath(1) complains that its argument path does not exist. Making sure that
# the path DOES exist solves the problem.
.PHONY: $(prefix)
$(prefix):
	mkdir -p $(prefix)

install-binfmt:
	mkdir -p $(libdir)/binfmt.d
	cp -v ./binfmt.d/viua-exec.conf $(libdir)/binfmt.d
	if [ -f /proc/sys/fs/binfmt_misc/viua-exec ]; then \
		echo -1 > /proc/sys/fs/binfmt_misc/viua-exec ; \
	fi
	cat ./binfmt.d/viua-exec.conf > /proc/sys/fs/binfmt_misc/register

format:
	(\
		find $(headerdir) -type f -name '*.h' -or -name '*.hh' ;\
		find $(sourcedir) -type f -name '*.cpp' -or -name '*.cc' \
	) | xargs --max-args 1 --max-procs 0 clang-format -i
	black --quiet tests/suite.py
	yamlfmt $(shell git rev-parse --show-toplevel)/.builds/*

man: \
	$(BUILD_MANDIR)/man1/viua-asm.1 \
	$(BUILD_MANDIR)/man1/viua-ld.1 \
	$(BUILD_MANDIR)/man1/viua-dis.1 \
	$(BUILD_MANDIR)/man1/viua-readelf.1 \
	$(BUILD_MANDIR)/man1/viua-vm.1 \
	$(BUILD_MANDIR)/man5/viua-asm.5 \
	$(BUILD_MANDIR)/man1/viua.1

$(BUILD_MANDIR)/%: docs/man/%.scdoc docs/man/common/*.scdoc docs/man/viua.m4
	@mkdir -p $(shell dirname $@)
	m4 -I docs/man -D "PUT_VIUAVM_VERSION_HERE=$(VIUA_TAG_VERSION)" $< |\
		scdoc > $@


################################################################################
# Development quality-of-life targets
watch:
	make --no-print-directory watch-impl |\
		entr -c make --shuffle=random -j

.PHONY: watch-impl
watch-impl:
	@(\
		ls -1 Makefile ;\
		find Makefile.d -mindepth 1 ;\
		find . -mindepth 2 -name '*.asm' ;\
		find ./docs/man -type f -name '*.scdoc' ;\
		find ./docs/man -type f -name '*.m4' ;\
		find $(sourcedir) -type f ;\
		find $(headerdir) -type f\
	)

watch-tests:
	(\
		find $(BUILD_EXEDIR) -type f ;\
		find ./tests/asm -type f -name '*.deps' ;\
		find ./tests/asm -type f -name '*.asm' ;\
		find ./tests/asm -type f -name '*.ebreak' ;\
		find ./tests/asm -type f -name '*.abort' ;\
		find ./tests/asm -type f -name '*.stdin' ;\
		find ./tests/asm -type f -name '*.stdout' ;\
		ls -1 tests/suite.py \
	) | entr -c $(python3) ./tests/suite.py


################################################################################
# Template rules
$(BUILD_PREFIX)/%: $(BUILD_PREFIX)/%.o
ifeq ($(VERBOSE),true)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(CXXLIBS)
else
	@echo $(CXX) $@ '<=' $<
	@$(CXX) $(CXXFLAGS) -o $@ $^ $(CXXLIBS)
endif

$(BUILD_BINDIR)/%: $(sourcedir)/libexec/viua/%.sh
	@mkdir -p $(BUILD_BINDIR)
	@cp -v $^ $@

$(BUILD_PREFIX)/%.o: $(sourcedir)/%.cpp
	@mkdir -p $(shell dirname $@)
ifeq ($(VERBOSE),true)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
else
	@echo $(CXX) $@ '<=' $<
	@$(CXX) $(CXXFLAGS) -c -o $@ $<
endif

$(BUILD_PREFIX)/%.o: $(sourcedir)/%.cc
	@mkdir -p $(shell dirname $@)
ifeq ($(VERBOSE),true)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
else
	@echo $(CXX) $@ '<=' $<
	@$(CXX) $(CXXFLAGS) -c -o $@ $<
endif


################################################################################
# Explicit dependencies
#
# Also, since the object files with the main() function of each executable in
# the Viua toolset are not really mentioned anywhere in the Makefile, GNU Make
# will consider them to be "intermediate" and delete them after it is done.
# Mentioning the files as prerequisites of the magic .NOTINTERMEDIATE: target
# prevents GNU Make from deleting these files.
#
# See https://www.gnu.org/software/make/manual/html_node/Chained-Rules.html for
# more information.
.NOTINTERMEDIATE: \
	$(BUILD_EXEDIR)/readelf.o \
	$(BUILD_EXEDIR)/dis.o \
	$(BUILD_EXEDIR)/asm.o \
	$(BUILD_EXEDIR)/ld.o \
	$(BUILD_EXEDIR)/vm.o \
	$(BUILD_EXEDIR)/repl.o

$(BUILD_EXEDIR)/readelf: \
	$(BUILD_PREFIX)/libexec/common.o \
	$(BUILD_PREFIX)/vm/elf.o \
	$(BUILD_PREFIX)/support/elf.o \
	$(BUILD_PREFIX)/support/errno.o \
	$(BUILD_PREFIX)/support/fdio.o \
	$(BUILD_PREFIX)/support/tty.o

VM_OPERATION_TYPE_IMPL=\
	$(BUILD_PREFIX)/arch/ra.o \
	$(BUILD_PREFIX)/arch/ops/n.o \
	$(BUILD_PREFIX)/arch/ops/t.o \
	$(BUILD_PREFIX)/arch/ops/d.o \
	$(BUILD_PREFIX)/arch/ops/s.o \
	$(BUILD_PREFIX)/arch/ops/f.o \
	$(BUILD_PREFIX)/arch/ops/e.o \
	$(BUILD_PREFIX)/arch/ops/r.o \
	$(BUILD_PREFIX)/arch/ops/m.o \
	$(BUILD_PREFIX)/arch/ops.o

$(BUILD_EXEDIR)/dis: \
	$(BUILD_PREFIX)/libexec/common.o \
	$(BUILD_PREFIX)/tools/libs/assembler.o \
	$(BUILD_PREFIX)/support/errno.o \
	$(BUILD_PREFIX)/support/fdio.o \
	$(BUILD_PREFIX)/support/string.o \
	$(BUILD_PREFIX)/support/tty.o \
	$(BUILD_PREFIX)/vm/elf.o \
	$(VM_OPERATION_TYPE_IMPL)

$(BUILD_EXEDIR)/asm: \
	$(BUILD_PREFIX)/libexec/common.o \
	$(BUILD_PREFIX)/support/errno.o \
	$(BUILD_PREFIX)/support/fdio.o \
	$(BUILD_PREFIX)/support/string.o \
	$(BUILD_PREFIX)/support/tty.o \
	$(BUILD_PREFIX)/tools/libs/errors/compile_time.o \
	$(BUILD_PREFIX)/tools/libs/lexer.o \
	$(BUILD_PREFIX)/tools/libs/stage/error.o \
	$(BUILD_PREFIX)/tools/libs/stage/save.o \
	$(BUILD_PREFIX)/tools/libs/assembler.o \
	$(VM_OPERATION_TYPE_IMPL)

$(BUILD_EXEDIR)/ld: \
	$(BUILD_PREFIX)/libexec/common.o \
	$(BUILD_PREFIX)/arch/ra.o \
	$(BUILD_PREFIX)/arch/ops.o \
	$(BUILD_PREFIX)/arch/ops/e.o \
	$(BUILD_PREFIX)/arch/ops/f.o \
	$(BUILD_PREFIX)/arch/ops/r.o \
	$(BUILD_PREFIX)/vm/elf.o \
	$(BUILD_PREFIX)/tools/libs/errors/compile_time.o \
	$(BUILD_PREFIX)/tools/libs/stage/save.o \
	$(BUILD_PREFIX)/support/elf.o \
	$(BUILD_PREFIX)/support/errno.o \
	$(BUILD_PREFIX)/support/fdio.o \
	$(BUILD_PREFIX)/support/string.o \
	$(BUILD_PREFIX)/support/tty.o

VM_INSTRUCTION_IMPL=\
	$(BUILD_PREFIX)/vm/ins/impl/memory.o \
	$(BUILD_PREFIX)/vm/ins/impl/concurrency.o \
	$(BUILD_PREFIX)/vm/ins/impl/globaltable.o \
	$(BUILD_PREFIX)/vm/ins/impl/bitwise.o \
	$(BUILD_PREFIX)/vm/ins/impl/registers.o \
	$(BUILD_PREFIX)/vm/ins/impl/logic.o \
	$(BUILD_PREFIX)/vm/ins/impl/arithmetic.o \
	$(BUILD_PREFIX)/vm/ins/impl/constructors.o \
	$(BUILD_PREFIX)/vm/ins/impl/flow.o \
	$(BUILD_PREFIX)/vm/ins/impl/ebreak.o \
	$(BUILD_PREFIX)/vm/ins/impl/ecall.o \
	$(BUILD_PREFIX)/vm/ins/impl/io.o

$(BUILD_EXEDIR)/vm: \
	$(BUILD_PREFIX)/libexec/common.o \
	$(BUILD_PREFIX)/support/errno.o \
	$(BUILD_PREFIX)/support/fdio.o \
	$(BUILD_PREFIX)/support/tty.o \
	$(BUILD_PREFIX)/support/string.o \
	$(BUILD_PREFIX)/vm/backtrace.o \
	$(BUILD_PREFIX)/vm/io/impl/$(io_impl).o \
	$(BUILD_PREFIX)/vm/io/sched.o \
	$(BUILD_PREFIX)/vm/core.o \
	$(BUILD_PREFIX)/vm/elf.o \
	$(BUILD_PREFIX)/vm/ins/proxy.o \
	$(BUILD_PREFIX)/vm/ins/exec.o \
	$(VM_INSTRUCTION_IMPL) \
	$(BUILD_PREFIX)/runtime/pid.o \
	$(VM_OPERATION_TYPE_IMPL)

$(BUILD_EXEDIR)/repl: \
	$(BUILD_PREFIX)/libexec/common.o \
	$(BUILD_PREFIX)/arch/ra.o \
	$(BUILD_PREFIX)/arch/ops.o \
	$(BUILD_PREFIX)/vm/backtrace.o \
	$(BUILD_PREFIX)/vm/core.o \
	$(BUILD_PREFIX)/vm/elf.o \
	$(VM_INSTRUCTION_IMPL) \
	$(BUILD_PREFIX)/support/fdio.o \
	$(BUILD_PREFIX)/support/string.o \
	$(BUILD_PREFIX)/support/tty.o \
	$(BUILD_PREFIX)/tools/libs/errors/compile_time.o \
	$(BUILD_PREFIX)/tools/libs/stage.o \
	$(BUILD_PREFIX)/tools/libs/assembler.o \
	$(BUILD_PREFIX)/tools/libs/lexer.o \
	$(BUILD_PREFIX)/tools/libs/parser.o \
	$(BUILD_PREFIX)/runtime/pid.o \
	3rd-party/linenoise/linenoise.o \
	3rd-party/linenoise/encodings/utf8.o

# FIXME dependencies are broken
include $(shell find ./build -type f -name '*.d')
