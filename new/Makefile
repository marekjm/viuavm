################################################################################
# Base directories
BUILD=./build
BUILD_LIBDIR=$(BUILD)/lib
BUILD_EXEDIR=$(BUILD)/libexec/viua
BUILD_BINDIR=$(BUILD)/bin

PREFIX=~/.local
LIBDIR=$(PREFIX)/lib
EXEDIR=$(PREFIX)/libexec/viua
BINDIR=$(PREFIX)/bin
SHAREDIR=$(PREFIX)/share
MANDIR=$(SHAREDIR)/man

VIUAVM_VERSION=$(shell sh ./scripts/make_version.sh short)
VIUAVM_VERSION_FULL=$(shell sh ./scripts/make_version.sh full)

OPTNAME=$(VIUAVM_VERSION)
VIUA_OPT_PREFIX=$(PREFIX)/opt/viua
VIUA_OPT_ROOT=$(VIUA_OPT_PREFIX)/$(OPTNAME)
VIUA_OPT_LIB_DIR=$(VIUA_OPT_ROOT)/lib
VIUA_OPT_CORE_DIR=$(VIUA_OPT_LIB_DIR)/viua-core
VIUA_OPT_ETC_DIR=$(VIUA_OPT_ROOT)/etc


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
all: dist
	@echo "Built version $(VIUAVM_VERSION_FULL)"
	@$(BUILD_EXEDIR)/vm --built-with

dist: \
	$(BUILD_EXEDIR)/readelf \
	$(BUILD_EXEDIR)/dis \
	$(BUILD_EXEDIR)/asm \
	$(BUILD_EXEDIR)/ld \
	$(BUILD_EXEDIR)/vm \
	$(BUILD_BINDIR)/viua

test: dist
	@python3 ./tests/suite.py $(TESTS)

clean-bin:
	find $(BUILD) -type f -delete 2>/dev/null || true
	find $(BUILD) -mindepth 1 -type d -delete 2>/dev/null || true

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
	rm -rf $(EXEDIR)
	rm -rf $(BINDIR)/viua
	rm -rf $(MANDIR)/man1/viua*
	rm -rf $(MANDIR)/man7/viua*

install: dist
	mkdir -p $(EXEDIR)
	install \
		--mode 755 \
		--target-directory $(EXEDIR) \
		$(BUILD_EXEDIR)/*
	mkdir -p $(BINDIR)
	install \
		--mode 755 \
		--target-directory $(BINDIR) \
		$(BUILD_BINDIR)/viua
	sed -i -e "s#^PREFIX=.*#PREFIX=$(PREFIX)#" $(BINDIR)/viua
	mkdir -p \
		$(MANDIR)/man1 \
		$(MANDIR)/man7
	install \
		--mode 644 \
		--target-directory $(MANDIR)/man1 \
		docs/man/*.1
	install \
		--mode 644 \
		--target-directory $(MANDIR)/man7 \
		docs/man/*.7
	find $(MANDIR) -type f -name 'viua*' -print0 |\
		xargs -P 0 -0 sed -i \
		-e "s#PUT_VIUAVM_TAG_DATESTAMP_HERE#$(VIUA_TAG_DATESTAMP)#g" \
		-e "s#PUT_VIUAVM_VERSION_HERE#$(VIUA_TAG_VERSION)#g"

erase:
	rm $(BINDIR)/viua
	rm -r $(VIUA_LIB_DIR)
	rm -v $(MANDIR)/man*/viua*

install-opt:
	make PREFIX=$(VIUA_OPT_ROOT) install
	mkdir -p $(VIUA_OPT_ETC_DIR)/viua
	echo "$(VIUAVM_VERSION_FULL)" > $(VIUA_OPT_ETC_DIR)/viua/version

erase-opt:
	rm -r $(VIUA_OPT_ROOT)

install-binfmt:
	mkdir -p $(PREFIX)/lib/binfmt.d
	cp -v ./binfmt.d/viua-exec.conf $(PREFIX)/lib/binfmt.d
	if [[ -f /proc/sys/fs/binfmt_misc/viua-exec ]]; then \
		echo -1 > /proc/sys/fs/binfmt_misc/viua-exec ; \
	fi
	cat ./binfmt.d/viua-exec.conf > /proc/sys/fs/binfmt_misc/register

format:
	@(\
		find ./include -type f -name '*.h' ;\
		find ./src -type f -name '*.cpp'\
	) | xargs --max-args 1 --max-procs 0 clang-format -i
	@black tests/suite.py


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
$(BUILD)/%: $(BUILD)/%.o
ifeq ($(VERBOSE),true)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(CXXLIBS)
else
	@echo $(CXX) $@ '<=' $<
	@$(CXX) $(CXXFLAGS) -o $@ $^ $(CXXLIBS)
endif

$(BUILD_EXEDIR)/%: $(BUILD)/tools/exec/%.o
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

$(BUILD)/%.o: ./src/%.cpp
	@mkdir -p $(shell dirname $@)
ifeq ($(VERBOSE),true)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
else
	@echo $(CXX) $@ '<=' $<
	@$(CXX) $(CXXFLAGS) -c -o $@ $<
endif

$(BUILD)/%.o: ./src/%.cc
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
	$(BUILD)/vm/elf.o \
	$(BUILD)/support/errno.o \
	$(BUILD)/support/fdio.o \
	$(BUILD)/support/tty.o

VM_OPERATION_TYPE_IMPL=\
	$(BUILD)/arch/ra.o \
	$(BUILD)/arch/ops/n.o \
	$(BUILD)/arch/ops/t.o \
	$(BUILD)/arch/ops/d.o \
	$(BUILD)/arch/ops/s.o \
	$(BUILD)/arch/ops/f.o \
	$(BUILD)/arch/ops/e.o \
	$(BUILD)/arch/ops/r.o \
	$(BUILD)/arch/ops/m.o \
	$(BUILD)/arch/ops.o

$(BUILD_EXEDIR)/dis: \
	$(BUILD)/tools/libs/assembler.o \
	$(BUILD)/support/errno.o \
	$(BUILD)/support/fdio.o \
	$(BUILD)/support/string.o \
	$(BUILD)/support/tty.o \
	$(BUILD)/vm/elf.o \
	$(VM_OPERATION_TYPE_IMPL)

$(BUILD_EXEDIR)/asm: \
	$(BUILD)/support/errno.o \
	$(BUILD)/support/fdio.o \
	$(BUILD)/support/string.o \
	$(BUILD)/support/tty.o \
	$(BUILD)/tools/libs/errors/compile_time.o \
	$(BUILD)/tools/libs/lexer.o \
	$(BUILD)/tools/libs/stage/error.o \
	$(BUILD)/tools/libs/stage/save.o \
	$(BUILD)/tools/libs/assembler.o \
	$(VM_OPERATION_TYPE_IMPL)

$(BUILD_EXEDIR)/ld: \
	$(BUILD)/arch/ra.o \
	$(BUILD)/arch/ops.o \
	$(BUILD)/arch/ops/f.o \
	$(BUILD)/arch/ops/e.o \
	$(BUILD)/vm/elf.o \
	$(BUILD)/tools/libs/errors/compile_time.o \
	$(BUILD)/tools/libs/stage/save.o \
	$(BUILD)/support/errno.o \
	$(BUILD)/support/fdio.o \
	$(BUILD)/support/string.o \
	$(BUILD)/support/tty.o

VM_INSTRUCTION_IMPL=\
	$(BUILD)/vm/ins/impl/aa.o \
	$(BUILD)/vm/ins/impl/ebreak.o \
	$(BUILD)/vm/ins/impl/ecall.o \
	$(BUILD)/vm/ins/impl/io.o

$(BUILD_EXEDIR)/vm: \
	$(BUILD)/support/errno.o \
	$(BUILD)/support/fdio.o \
	$(BUILD)/support/tty.o \
	$(BUILD)/support/string.o \
	$(BUILD)/vm/backtrace.o \
	$(BUILD)/vm/core.o \
	$(BUILD)/vm/elf.o \
	$(BUILD)/vm/ins.o \
	$(BUILD)/vm/ins/proxy.o \
	$(BUILD)/vm/ins/exec.o \
	$(VM_INSTRUCTION_IMPL) \
	$(BUILD)/runtime/pid.o \
	$(VM_OPERATION_TYPE_IMPL)

$(BUILD_EXEDIR)/repl: \
	$(BUILD)/arch/ra.o \
	$(BUILD)/arch/ops.o \
	$(BUILD)/vm/backtrace.o \
	$(BUILD)/vm/core.o \
	$(BUILD)/vm/elf.o \
	$(BUILD)/vm/ins.o \
	$(VM_INSTRUCTION_IMPL) \
	$(BUILD)/support/fdio.o \
	$(BUILD)/support/string.o \
	$(BUILD)/support/tty.o \
	$(BUILD)/tools/libs/errors/compile_time.o \
	$(BUILD)/tools/libs/stage.o \
	$(BUILD)/tools/libs/assembler.o \
	$(BUILD)/tools/libs/lexer.o \
	$(BUILD)/tools/libs/parser.o \
	$(BUILD)/runtime/pid.o \
	3rd-party/linenoise/linenoise.o \
	3rd-party/linenoise/encodings/utf8.o
