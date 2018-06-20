export BUILDTYPE ?= Debug
export WITH_CXX11ABI ?= $(shell scripts/check-cxx11abi.sh)

ifeq ($(BUILDTYPE), Release)
else ifeq ($(BUILDTYPE), RelWithDebInfo)
else ifeq ($(BUILDTYPE), Sanitize)
else ifeq ($(BUILDTYPE), Debug)
else
  $(error BUILDTYPE must be Debug, Sanitize, Release or RelWithDebInfo)
endif

buildtype := $(shell echo "$(BUILDTYPE)" | tr "[A-Z]" "[a-z]")

ifeq ($(shell uname -s), Linux)
  HOST_PLATFORM = linux
  HOST_PLATFORM_VERSION = $(shell uname -m)
  export NINJA = platform/linux/ninja
  export JOBS ?= $(shell grep --count processor /proc/cpuinfo)
else
  $(error Cannot determine host platform)
endif

ifeq ($(MASON_PLATFORM),)
  BUILD_PLATFORM = $(HOST_PLATFORM)
else
  BUILD_PLATFORM = $(MASON_PLATFORM)
endif

ifeq ($(MASON_PLATFORM_VERSION),)
  BUILD_PLATFORM_VERSION = $(HOST_PLATFORM_VERSION)
else
  BUILD_PLATFORM_VERSION = $(MASON_PLATFORM_VERSION)
endif

ifeq ($(V), 1)
  export XCPRETTY
  NINJA_ARGS ?= -v
else
  export XCPRETTY ?= | tee '$(shell pwd)/build/xcodebuild-$(shell date +"%Y-%m-%d_%H%M%S").log' | xcpretty
  NINJA_ARGS ?=
endif

.PHONY: default
default: test

BUILD_DEPS += Makefile
BUILD_DEPS += CMakeLists.txt

#### Linux targets #####################################################

ifeq ($(HOST_PLATFORM), linux)

export PATH := $(shell pwd)/platform/linux:$(PATH)
export LINUX_OUTPUT_PATH = build/linux-$(shell uname -m)/$(BUILDTYPE)
LINUX_BUILD = $(LINUX_OUTPUT_PATH)/build.ninja

$(LINUX_BUILD): $(BUILD_DEPS)
	mkdir -p $(LINUX_OUTPUT_PATH)
	(cd $(LINUX_OUTPUT_PATH) && cmake -G Ninja ../../.. \
		-DCMAKE_BUILD_TYPE=$(BUILDTYPE) \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DWITH_CXX11ABI=${WITH_CXX11ABI} \
		-DWITH_COVERAGE=${WITH_COVERAGE} \
		-DWITH_OSMESA=${WITH_OSMESA} \
		-DWITH_EGL=${WITH_EGL})

.PHONY: linux
linux: glfw-app render offline

.PHONY: linux-core
linux-core: $(LINUX_BUILD)
	$(NINJA) $(NINJA_ARGS) -j$(JOBS) -C $(LINUX_OUTPUT_PATH) mbgl-core mbgl-loop-uv mbgl-filesource

ifneq (,$(shell command -v gdb 2> /dev/null))
  GDB ?= $(shell scripts/mason.sh PREFIX gdb VERSION 2017-04-08-aebcde5)/bin/gdb \
        	-batch -return-child-result \
        	-ex 'set print thread-events off' \
        	-ex 'set disable-randomization off' \
        	-ex 'run' \
        	-ex 'thread apply all bt' --args
endif

.PHONY: render
render: $(LINUX_BUILD)
	$(NINJA) $(NINJA_ARGS) -j$(JOBS) -C $(LINUX_OUTPUT_PATH) mbgl-render

.PHONY: offline
offline: $(LINUX_BUILD)
	$(NINJA) $(NINJA_ARGS) -j$(JOBS) -C $(LINUX_OUTPUT_PATH) mbgl-offline

.PHONY: compdb
compdb: $(LINUX_BUILD)
	# Ninja generator already outputs the file at the right location

.PHONY: tidy
tidy: compdb
	scripts/clang-tools.sh $(LINUX_OUTPUT_PATH)

.PHONY: check
check: compdb
	scripts/clang-tools.sh $(LINUX_OUTPUT_PATH) --diff

endif

#### Miscellaneous targets #####################################################

.PHONY: style-code
style-code:
	node scripts/generate-style-code.js
	node scripts/generate-shaders.js

.PHONY: codestyle
codestyle:
	scripts/codestyle.sh

.PHONY: clean
clean:
	-rm -rf ./build \
	        ./lib/*.node \

.PHONY: distclean
distclean: clean
	-rm -rf ./mason_packages
	-rm -rf ./node_modules
