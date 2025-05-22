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


################################################################################
# Compilation flags
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
				 -I./include \
				 -I./3rd-party

FEATURE_DETECT_FLAGS=\
					 $(shell CXX=$(CXX) bash ./feature-tests/test.sh ./feature-tests/strerrorname_np.c)

# To change version, edit the VERSION file in the repository's root directory.
VIUAVM_VERSION=$(shell sh ./scripts/make_version.sh short)
VIUAVM_VERSION_FULL=$(shell sh ./scripts/make_version.sh full)
VERSION_FLAGS=\
			  -DVIUAVM_VERSION='"$(VIUAVM_VERSION)"' \
			  -DVIUAVM_VERSION_FULL='"$(VIUAVM_VERSION_FULL)"' \
			  -DVIUAVM_CXX_PRESET='"$(PRESET)"' \
			  -DVIUAVM_CXX_OPTIONS='"$(CXXFLAGS_OPTIMISATION) $(CXXFLAGS_DEBUG) $(CXXFLAGS_SANITISER) $(CXXFLAGS_OPTION)"' \
			  -DCXX='"$(CXX)"' \
			  -DCXXVERSION='"$(shell $(CXX) -dumpversion)"' \
			  -DCXXSTD='"$(CXXSTD)"'

CXXFLAGS=\
		 $(VERSION_FLAGS) \
		 -std=$(CXXSTD) \
		 $(FEATURE_DETECT_FLAGS) \
		 $(CXXFLAGS_OPTIMISATION) \
		 $(CXXFLAGS_DEBUG) \
		 $(CXXFLAGS_SANITISER) \
		 $(CXXFLAGS_OPTION) \
		 $(CXXFLAGS_WARNING) \
		 -Wall \
		 -Wextra \
		 -Wpedantic \
		 -Werror \
		 -Wfatal-errors \
		 $(CXXFLAGS_NOERROR) \
		 $(CXXFLAGS_INCLUDE)

CXXLIBS=\
		-luring


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
		python3 ./tests/suite.py $(TESTS)

clean-bin:
	find $(BUILD_PREFIX) -type f -delete 2>/dev/null || true
	find $(BUILD_PREFIX) -mindepth 1 -type d -delete 2>/dev/null || true

clean-test:
	find ./tests -type f -name '*.elf' -delete
	find ./tests -type f -name '*.o' -delete
	find ./tests -type f -name '*.asm~' -delete
	find ./tests -type f -name '*.log' -delete
	find ./tests -type d -name '.cache' -print0 |\
		xargs --null -I CACHE_DIR find CACHE_DIR -type f -delete
	find ./tests -type d -name '.cache' -delete

clean: clean-test clean-bin

VIUA_TAG_DATESTAMP=$(shell git show --date=short HEAD |\
				   awk '/^Date:/ { print $$2 }')
VIUA_TAG_VERSION=$(VIUAVM_VERSION)

uninstall:
	rm -r $(DESTDIR)$(exedir)
	rm -r $(DESTDIR)$(bindir)/viua
	rm -v $(DESTDIR)$(mandir)/man*/viua*

install: all-silent
	install \
		--mode 755 \
		-D --target-directory $(DESTDIR)$(exedir) \
		$(BUILD_EXEDIR)/*
	install \
		--mode 755 \
		-D --target-directory $(DESTDIR)$(bindir) \
		$(BUILD_BINDIR)/viua
	sed -i -e "s#^prefix=.*#prefix=$(prefix)#" $(DESTDIR)$(bindir)/viua
	install \
		--mode 644 \
		-D --target-directory $(DESTDIR)$(mandir)/man1 \
		docs/man/*.1
	ln -sf $(mandir)/man1/viua.1 $(DESTDIR)$(mandir)/man1/viua-viua.1
	install \
		--mode 644 \
		-D --target-directory $(DESTDIR)$(mandir)/man7 \
		docs/man/*.7
	find $(DESTDIR)$(mandir) -type f -name 'viua*' -print0 |\
		xargs -P 0 -0 sed -i \
		-e "s#PUT_VIUAVM_TAG_DATESTAMP_HERE#$(VIUA_TAG_DATESTAMP)#g" \
		-e "s#PUT_VIUAVM_VERSION_HERE#$(VIUA_TAG_VERSION)#g"

install-binfmt:
	mkdir -p $(libdir)/binfmt.d
	cp -v ./binfmt.d/viua-exec.conf $(libdir)/binfmt.d
	if [ -f /proc/sys/fs/binfmt_misc/viua-exec ]; then \
		echo -1 > /proc/sys/fs/binfmt_misc/viua-exec ; \
	fi
	cat ./binfmt.d/viua-exec.conf > /proc/sys/fs/binfmt_misc/register

format:
	(\
		find ./include -type f -name '*.h' ;\
		find ./src -type f -name '*.cpp'\
	) | xargs --max-args 1 --max-procs 0 clang-format -i
	black --quiet tests/suite.py
	yamlfmt $(shell git rev-parse --show-toplevel)/.builds/*


################################################################################
# Development quality-of-life targets
watch:
	make watch-impl | entr -c make -j

.PHONY: watch-impl
watch-impl:
	(\
		ls -1 Makefile ;\
		find Makefile.d -mindepth 1 ;\
		find . -mindepth 2 -name '*.asm' ;\
		find ./src -type f ;\
		find ./include -type f\
	)

watch-tests:
	(\
		find $(BUILD_EXEDIR) -type f ;\
		find ./tests/asm -type f ;\
		ls -1 tests/suite.py \
	) | entr -c python3 ./tests/suite.py


################################################################################
# Template rules
$(BUILD_PREFIX)/%: $(BUILD_PREFIX)/%.o
ifeq ($(VERBOSE),true)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(CXXLIBS)
else
	@echo $(CXX) $@ '<=' $<
	@$(CXX) $(CXXFLAGS) -o $@ $^ $(CXXLIBS)
endif

$(BUILD_EXEDIR)/%: $(BUILD_PREFIX)/tools/exec/%.o
	@mkdir -p $(shell dirname $@)
ifeq ($(VERBOSE),true)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(CXXLIBS)
else
	@echo $(CXX) $@ '<=' $<
	@$(CXX) $(CXXFLAGS) -o $@ $^ $(CXXLIBS)
endif

$(BUILD_BINDIR)/%: ./src/tools/exec/%.sh
	@mkdir -p $(BUILD_BINDIR)
	@cp -v $^ $@

$(BUILD_PREFIX)/%.o: ./src/%.cpp
	@mkdir -p $(shell dirname $@)
ifeq ($(VERBOSE),true)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
else
	@echo $(CXX) $@ '<=' $<
	@$(CXX) $(CXXFLAGS) -c -o $@ $<
endif

$(BUILD_PREFIX)/%.o: ./src/%.cc
	@mkdir -p $(shell dirname $@)
ifeq ($(VERBOSE),true)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
else
	@echo $(CXX) $@ '<=' $<
	@$(CXX) $(CXXFLAGS) -c -o $@ $<
endif


################################################################################
# Explicit dependencies
$(BUILD_EXEDIR)/readelf: \
	$(BUILD_PREFIX)/vm/elf.o \
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
	$(BUILD_PREFIX)/tools/libs/assembler.o \
	$(BUILD_PREFIX)/support/errno.o \
	$(BUILD_PREFIX)/support/fdio.o \
	$(BUILD_PREFIX)/support/string.o \
	$(BUILD_PREFIX)/support/tty.o \
	$(BUILD_PREFIX)/vm/elf.o \
	$(VM_OPERATION_TYPE_IMPL)

$(BUILD_EXEDIR)/asm: \
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
	$(BUILD_PREFIX)/arch/ra.o \
	$(BUILD_PREFIX)/arch/ops.o \
	$(BUILD_PREFIX)/arch/ops/f.o \
	$(BUILD_PREFIX)/arch/ops/e.o \
	$(BUILD_PREFIX)/vm/elf.o \
	$(BUILD_PREFIX)/tools/libs/errors/compile_time.o \
	$(BUILD_PREFIX)/tools/libs/stage/save.o \
	$(BUILD_PREFIX)/support/errno.o \
	$(BUILD_PREFIX)/support/fdio.o \
	$(BUILD_PREFIX)/support/string.o \
	$(BUILD_PREFIX)/support/tty.o

VM_INSTRUCTION_IMPL=\
	$(BUILD_PREFIX)/vm/ins/impl/aa.o \
	$(BUILD_PREFIX)/vm/ins/impl/ebreak.o \
	$(BUILD_PREFIX)/vm/ins/impl/ecall.o \
	$(BUILD_PREFIX)/vm/ins/impl/io.o

$(BUILD_EXEDIR)/vm: \
	$(BUILD_PREFIX)/support/errno.o \
	$(BUILD_PREFIX)/support/fdio.o \
	$(BUILD_PREFIX)/support/tty.o \
	$(BUILD_PREFIX)/support/string.o \
	$(BUILD_PREFIX)/vm/backtrace.o \
	$(BUILD_PREFIX)/vm/core.o \
	$(BUILD_PREFIX)/vm/elf.o \
	$(BUILD_PREFIX)/vm/ins.o \
	$(BUILD_PREFIX)/vm/ins/proxy.o \
	$(BUILD_PREFIX)/vm/ins/exec.o \
	$(VM_INSTRUCTION_IMPL) \
	$(BUILD_PREFIX)/runtime/pid.o \
	$(VM_OPERATION_TYPE_IMPL)

$(BUILD_EXEDIR)/repl: \
	$(BUILD_PREFIX)/arch/ra.o \
	$(BUILD_PREFIX)/arch/ops.o \
	$(BUILD_PREFIX)/vm/backtrace.o \
	$(BUILD_PREFIX)/vm/core.o \
	$(BUILD_PREFIX)/vm/elf.o \
	$(BUILD_PREFIX)/vm/ins.o \
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
