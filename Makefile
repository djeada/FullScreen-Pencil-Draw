# FullScreen Pencil Draw - Advanced Makefile
# Mirrors the workflow style used in Standard-of-Iron, adapted for this repo.

SHELL := /bin/bash
.DEFAULT_GOAL := help

# Core configuration
PROJECT_NAME := FullScreen-Pencil-Draw
BUILD_DIR ?= build
BUILD_TIDY_DIR ?= build-tidy
BUILD_DEBUG_DIR ?= build-debug
BUILD_RELEASE_DIR ?= build-release
BUILD_TYPE ?= Release
BUILD_TESTING ?= ON
GENERATOR ?= Unix Makefiles

# Tooling
CMAKE ?= cmake
CTEST ?= ctest
CLANG_FORMAT ?= clang-format
CLANG_TIDY ?= clang-tidy
RUN_CLANG_TIDY ?= run-clang-tidy
QMLFORMAT ?= $(shell command -v qmlformat 2>/dev/null || echo qmlformat)
CLANG_TIDY_GIT_BASE ?= origin/main
CLANG_TIDY_CHECKS ?=
CLANG_TIDY_JOBS ?=

# Dependency helper for Debian/Ubuntu
APT_DEPS := build-essential cmake qt6-base-dev qt6-tools-dev qt6-tools-dev-tools libgl1-mesa-dev xvfb

# File globs for style/lint tasks
CXX_GLOBS := -name "*.c" -o -name "*.cc" -o -name "*.cpp" -o -name "*.cxx" -o -name "*.h" -o -name "*.hh" -o -name "*.hpp" -o -name "*.hxx"
QML_GLOBS := -name "*.qml"
EXCLUDE_DIRS := ./$(BUILD_DIR) ./$(BUILD_TIDY_DIR) ./$(BUILD_DEBUG_DIR) ./$(BUILD_RELEASE_DIR) ./.git
EXCLUDE_FIND := $(foreach d,$(EXCLUDE_DIRS),-not -path "$(d)/*")

# Colors for readable output
BOLD := \033[1m
GREEN := \033[32m
BLUE := \033[34m
YELLOW := \033[33m
RED := \033[31m
RESET := \033[0m

NPROC := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

.PHONY: help
help:
	@echo "$(BOLD)FullScreen Pencil Draw - Build System$(RESET)"
	@echo ""
	@echo "$(BOLD)Available targets:$(RESET)"
	@echo "  $(GREEN)install$(RESET)        - Install dependencies (Ubuntu/Debian)"
	@echo "  $(GREEN)check-deps$(RESET)     - Verify build/runtime dependencies"
	@echo "  $(GREEN)configure$(RESET)      - Configure CMake in $(BUILD_DIR)"
	@echo "  $(GREEN)build$(RESET)          - Build project in $(BUILD_DIR)"
	@echo "  $(GREEN)build-tidy$(RESET)     - Build with clang-tidy via CMAKE_CXX_CLANG_TIDY"
	@echo "  $(GREEN)debug$(RESET)          - Configure and build debug profile"
	@echo "  $(GREEN)release$(RESET)        - Configure and build release profile"
	@echo "  $(GREEN)run$(RESET)            - Build and run application"
	@echo "  $(GREEN)run-headless$(RESET)   - Run app under xvfb"
	@echo "  $(GREEN)test$(RESET)           - Build and run ctest"
	@echo "  $(GREEN)format$(RESET)         - Format C/C++ and QML sources"
	@echo "  $(GREEN)format-check$(RESET)   - Check formatting (CI-friendly)"
	@echo "  $(GREEN)tidy$(RESET)           - clang-tidy on changed files vs origin/main"
	@echo "  $(GREEN)tidy-all$(RESET)       - clang-tidy across all sources"
	@echo "  $(GREEN)clean$(RESET)          - Remove all build directories"
	@echo "  $(GREEN)rebuild$(RESET)        - Clean and build"
	@echo "  $(GREEN)dev$(RESET)            - install + build"
	@echo "  $(GREEN)info$(RESET)           - Show toolchain and build info"
	@echo "  $(GREEN)quickstart$(RESET)     - Print quickstart commands"
	@echo ""
	@echo "$(BOLD)Examples:$(RESET)"
	@echo "  make install"
	@echo "  make build"
	@echo "  make run"
	@echo "  make test BUILD_TESTING=ON"

.PHONY: build-dir tidy-build-dir debug-build-dir release-build-dir
build-dir:
	@mkdir -p $(BUILD_DIR)

tidy-build-dir:
	@mkdir -p $(BUILD_TIDY_DIR)

debug-build-dir:
	@mkdir -p $(BUILD_DEBUG_DIR)

release-build-dir:
	@mkdir -p $(BUILD_RELEASE_DIR)

.PHONY: install
install:
	@echo "$(BOLD)$(BLUE)Installing dependencies...$(RESET)"
	@if command -v apt-get >/dev/null 2>&1; then \
		sudo apt-get update && sudo apt-get install -y $(APT_DEPS); \
		echo "$(GREEN)✓ Dependencies installed$(RESET)"; \
	else \
		echo "$(YELLOW)Automatic install only supports apt-get.$(RESET)"; \
		echo "$(YELLOW)Install manually: $(APT_DEPS)$(RESET)"; \
	fi

.PHONY: check-deps
check-deps:
	@echo "$(BOLD)$(BLUE)Checking dependencies...$(RESET)"
	@MISSING=0; \
	for cmd in $(CMAKE) make c++ pkg-config; do \
		if command -v $$cmd >/dev/null 2>&1; then \
			echo "$(GREEN)✓$$cmd$(RESET)"; \
		else \
			echo "$(RED)✗$$cmd$(RESET)"; \
			MISSING=1; \
		fi; \
	done; \
	if pkg-config --exists Qt6Widgets 2>/dev/null || pkg-config --exists Qt5Widgets 2>/dev/null; then \
		echo "$(GREEN)✓Qt Widgets pkg-config entry$(RESET)"; \
	else \
		echo "$(YELLOW)⚠ Qt pkg-config entry not found (CMake may still detect Qt via toolchain).$(RESET)"; \
	fi; \
	if command -v xvfb-run >/dev/null 2>&1; then \
		echo "$(GREEN)✓xvfb-run$(RESET)"; \
	else \
		echo "$(YELLOW)⚠ xvfb-run not found (only needed for run-headless).$(RESET)"; \
	fi; \
	if [ $$MISSING -eq 0 ]; then \
		echo "$(GREEN)✓ Basic dependency checks passed$(RESET)"; \
	else \
		echo "$(RED)✗ Missing required commands$(RESET)"; \
		exit 1; \
	fi

.PHONY: configure
configure: build-dir
	@echo "$(BOLD)$(BLUE)Configuring CMake in $(BUILD_DIR)...$(RESET)"
	@cd $(BUILD_DIR) && $(CMAKE) \
		-G "$(GENERATOR)" \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DBUILD_TESTING=$(BUILD_TESTING) \
		..
	@echo "$(GREEN)✓ Configuration complete$(RESET)"

.PHONY: build
build: configure
	@echo "$(BOLD)$(BLUE)Building $(PROJECT_NAME)...$(RESET)"
	@$(CMAKE) --build $(BUILD_DIR) --parallel $(NPROC)
	@echo "$(GREEN)✓ Build complete$(RESET)"

.PHONY: build-tidy
build-tidy: tidy-build-dir
	@echo "$(BOLD)$(BLUE)Building with clang-tidy enabled...$(RESET)"
	@if ! command -v $(CLANG_TIDY) >/dev/null 2>&1; then \
		echo "$(RED)clang-tidy not found.$(RESET)"; \
		exit 1; \
	fi
	@cd $(BUILD_TIDY_DIR) && $(CMAKE) \
		-G "$(GENERATOR)" \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DBUILD_TESTING=$(BUILD_TESTING) \
		-DCMAKE_CXX_CLANG_TIDY="$(CLANG_TIDY)" \
		..
	@$(CMAKE) --build $(BUILD_TIDY_DIR) --parallel $(NPROC)
	@echo "$(GREEN)✓ Tidy build complete$(RESET)"

.PHONY: debug
debug: debug-build-dir
	@echo "$(BOLD)$(BLUE)Configuring debug build...$(RESET)"
	@cd $(BUILD_DEBUG_DIR) && $(CMAKE) \
		-G "$(GENERATOR)" \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DBUILD_TESTING=ON \
		..
	@$(CMAKE) --build $(BUILD_DEBUG_DIR) --parallel $(NPROC)
	@echo "$(GREEN)✓ Debug build complete$(RESET)"

.PHONY: release
release: release-build-dir
	@echo "$(BOLD)$(BLUE)Configuring release build...$(RESET)"
	@cd $(BUILD_RELEASE_DIR) && $(CMAKE) \
		-G "$(GENERATOR)" \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DBUILD_TESTING=$(BUILD_TESTING) \
		..
	@$(CMAKE) --build $(BUILD_RELEASE_DIR) --parallel $(NPROC)
	@echo "$(GREEN)✓ Release build complete$(RESET)"

.PHONY: run
run: build
	@echo "$(BOLD)$(BLUE)Running $(PROJECT_NAME)...$(RESET)"
	@BIN="$(BUILD_DIR)/$(PROJECT_NAME)"; \
	APP_BUNDLE="$(BUILD_DIR)/$(PROJECT_NAME).app/Contents/MacOS/$(PROJECT_NAME)"; \
	if [ -x "$$BIN" ]; then \
		"$$BIN"; \
	elif [ -x "$$APP_BUNDLE" ]; then \
		"$$APP_BUNDLE"; \
	else \
		echo "$(RED)Executable not found in $(BUILD_DIR).$(RESET)"; \
		exit 127; \
	fi

.PHONY: run-headless
run-headless: build
	@echo "$(BOLD)$(BLUE)Running $(PROJECT_NAME) under xvfb...$(RESET)"
	@if ! command -v xvfb-run >/dev/null 2>&1; then \
		echo "$(RED)xvfb-run not found. Install xvfb first.$(RESET)"; \
		exit 1; \
	fi
	@BIN="$(BUILD_DIR)/$(PROJECT_NAME)"; \
	if [ ! -x "$$BIN" ]; then \
		echo "$(RED)Executable not found at $$BIN$(RESET)"; \
		exit 127; \
	fi; \
	xvfb-run -s "-screen 0 1920x1080x24" "$$BIN"

.PHONY: test
test: build
	@echo "$(BOLD)$(BLUE)Running tests with ctest...$(RESET)"
	@cd $(BUILD_DIR) && $(CTEST) --output-on-failure

.PHONY: format
format:
	@echo "$(BOLD)$(BLUE)Formatting C/C++ files...$(RESET)"
	@if command -v $(CLANG_FORMAT) >/dev/null 2>&1; then \
		find . -type f \( $(CXX_GLOBS) \) $(EXCLUDE_FIND) -print0 \
			| xargs -0 -r $(CLANG_FORMAT) -i --style=file; \
		echo "$(GREEN)✓ C/C++ formatting complete$(RESET)"; \
	else \
		echo "$(RED)clang-format not found.$(RESET)"; \
		exit 1; \
	fi
	@echo "$(BOLD)$(BLUE)Formatting QML files...$(RESET)"
	@if command -v $(QMLFORMAT) >/dev/null 2>&1 || [ -x "$(QMLFORMAT)" ]; then \
		find . -type f \( $(QML_GLOBS) \) $(EXCLUDE_FIND) -print0 \
			| xargs -0 -r $(QMLFORMAT) -i; \
		echo "$(GREEN)✓ QML formatting complete$(RESET)"; \
	else \
		echo "$(YELLOW)⚠ qmlformat not found. Skipping QML formatting.$(RESET)"; \
	fi
	@echo "$(GREEN)✓ Formatting complete$(RESET)"

.PHONY: format-check
format-check:
	@echo "$(BOLD)$(BLUE)Checking formatting...$(RESET)"
	@FAILED=0; \
	if command -v $(CLANG_FORMAT) >/dev/null 2>&1; then \
		find . -type f \( $(CXX_GLOBS) \) $(EXCLUDE_FIND) -print0 \
			| xargs -0 -r $(CLANG_FORMAT) --dry-run -Werror --style=file || FAILED=1; \
	else \
		echo "$(YELLOW)⚠ clang-format not found. Skipping C/C++ checks.$(RESET)"; \
	fi; \
	if command -v $(QMLFORMAT) >/dev/null 2>&1 || [ -x "$(QMLFORMAT)" ]; then \
		for file in $$(find . -type f \( $(QML_GLOBS) \) $(EXCLUDE_FIND)); do \
			$(QMLFORMAT) "$$file" > /tmp/qmlformat_check.tmp 2>/dev/null; \
			if ! diff -q "$$file" /tmp/qmlformat_check.tmp >/dev/null 2>&1; then \
				echo "$(RED)QML file needs formatting: $$file$(RESET)"; \
				FAILED=1; \
			fi; \
		done; \
		rm -f /tmp/qmlformat_check.tmp; \
	fi; \
	if [ $$FAILED -eq 0 ]; then \
		echo "$(GREEN)✓ Formatting checks passed$(RESET)"; \
	else \
		echo "$(RED)✗ Formatting check failed. Run 'make format'.$(RESET)"; \
		exit 1; \
	fi

.PHONY: tidy
tidy: configure
	@echo "$(BOLD)$(BLUE)Running clang-tidy on changed files...$(RESET)"
	@if ! command -v $(CLANG_TIDY) >/dev/null 2>&1; then \
		echo "$(RED)clang-tidy not found.$(RESET)"; \
		exit 1; \
	fi
	@FILES="$$(git diff --name-only --diff-filter=ACMR $(CLANG_TIDY_GIT_BASE)...HEAD -- '*.c' '*.cc' '*.cpp' '*.cxx' '*.h' '*.hh' '*.hpp' '*.hxx' 2>/dev/null || true)"; \
	if [ -z "$$FILES" ]; then \
		echo "$(GREEN)No changed C/C++ files to lint.$(RESET)"; \
		exit 0; \
	fi; \
	if command -v $(RUN_CLANG_TIDY) >/dev/null 2>&1; then \
		echo "$$FILES" | xargs -r $(RUN_CLANG_TIDY) -p $(BUILD_DIR) -fix \
			$(if $(CLANG_TIDY_JOBS),-j $(CLANG_TIDY_JOBS),) \
			$(if $(CLANG_TIDY_CHECKS),-checks=$(CLANG_TIDY_CHECKS),); \
	else \
		for file in $$FILES; do \
			$(CLANG_TIDY) "$$file" -p $(BUILD_DIR) --fix \
				$(if $(CLANG_TIDY_CHECKS),-checks=$(CLANG_TIDY_CHECKS),) || exit 1; \
		done; \
	fi
	@echo "$(GREEN)✓ clang-tidy complete$(RESET)"

.PHONY: tidy-all
tidy-all: configure
	@echo "$(BOLD)$(BLUE)Running clang-tidy on all files...$(RESET)"
	@if ! command -v $(CLANG_TIDY) >/dev/null 2>&1; then \
		echo "$(RED)clang-tidy not found.$(RESET)"; \
		exit 1; \
	fi
	@if command -v $(RUN_CLANG_TIDY) >/dev/null 2>&1; then \
		$(RUN_CLANG_TIDY) -p $(BUILD_DIR) -fix \
			$(if $(CLANG_TIDY_JOBS),-j $(CLANG_TIDY_JOBS),) \
			$(if $(CLANG_TIDY_CHECKS),-checks=$(CLANG_TIDY_CHECKS),); \
	else \
		find . -type f \( $(CXX_GLOBS) \) $(EXCLUDE_FIND) -print0 \
			| xargs -0 -r -I{} $(CLANG_TIDY) "{}" -p $(BUILD_DIR) --fix \
				$(if $(CLANG_TIDY_CHECKS),-checks=$(CLANG_TIDY_CHECKS),); \
	fi
	@echo "$(GREEN)✓ clang-tidy all complete$(RESET)"

.PHONY: clean
clean:
	@echo "$(BOLD)$(YELLOW)Cleaning build directories...$(RESET)"
	@rm -rf $(BUILD_DIR) $(BUILD_TIDY_DIR) $(BUILD_DEBUG_DIR) $(BUILD_RELEASE_DIR)
	@echo "$(GREEN)✓ Clean complete$(RESET)"

.PHONY: rebuild
rebuild: clean build

.PHONY: dev
dev: install build
	@echo "$(GREEN)✓ Development environment ready$(RESET)"

.PHONY: all
all: build

.PHONY: info
info:
	@echo "$(BOLD)Project Information$(RESET)"
	@echo "  Project name: $(PROJECT_NAME)"
	@echo "  Build dir: $(BUILD_DIR)"
	@echo "  CMake: $$($(CMAKE) --version | head -1)"
	@echo "  C++: $$(c++ --version | head -1)"
	@echo "  Generator: $(GENERATOR)"
	@if [ -f "$(BUILD_DIR)/compile_commands.json" ]; then \
		echo "  compile_commands.json: $(GREEN)present$(RESET)"; \
	else \
		echo "  compile_commands.json: $(YELLOW)missing (run make configure)$(RESET)"; \
	fi

.PHONY: quickstart
quickstart:
	@echo "$(BOLD)$(GREEN)Quick Start$(RESET)"
	@echo "  1) make install"
	@echo "  2) make build"
	@echo "  3) make run"
