LOCALAPPDATA := $(shell powershell -NoProfile -Command "[Environment]::GetFolderPath('LocalApplicationData')")
DIALUP_ROOT       := $(LOCALAPPDATA)/DialUp
include $(DIALUP_ROOT)/build-tools/common.mk
include $(DIALUP_ROOT)/build-tools/shell.mk
include $(DIALUP_ROOT)/build-tools/rocketleague.mk

BUILD_DIR        := build
DLL              := DialUp-SDKGen.dll
SDK_OUTPUT_DIR   := plugin/generated

LOCK             := $(DIALUP_ROOT)/.lock-sdkgen
DLL_RAND_NAME    := $(shell shuf -i 8-16 -n 1 | xargs openssl rand -hex)

BANNER_DIRS := \
	plugin/imported/model \
	plugin/imported/runtime

.PHONY: configure build clean run configure-plugin build-plugin install-plugin

configure: check-shell
	@echo "🛠️ Configuring CMake..."
	$(call run_with_vcvars, \
		cmake -S . -B build -G $(GENERATOR)  \
		-DCMAKE_BUILD_TYPE=RelWithDebInfo    \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON   \
		"-DGAME=$(game)"                     \
		"-DLOG_DIR=$(log_dir)"               \
		"-DGAME_BINARY_PATH=$(binary_dir)"   \
		$(ARGS))

build: check-shell
	$(call run_with_vcvars, cmake --build build --config RelWithDebInfo $(ARGS))

clean: check-shell
	@rm -rf build

run: check-shell
	# copy to RL bin dir, run, remove
	@bash -lc '\
		if [ -f dll_filename ]; then \
			rm "$(binary_dir)/$$(cat dll_filename).dll"; \
			rm "$(binary_dir)/$$(cat dll_filename).pdb"; \
		fi ; \
		touch dll_filename ; \
		echo $(DLL_RAND_NAME) > dll_filename; \
		cp -v "$(BUILD_DIR)/$(DLL)" "$(binary_dir)/$(DLL_RAND_NAME).dll"; \
		cp -v "$(BUILD_DIR)/$(DLL)" "$(binary_dir)/$(DLL_RAND_NAME).pdb"; \
		touch "$(LOCK)" \
		'
	"$(INJECTOR)" "$(target)" "$(binary_dir)/$(DLL_RAND_NAME).dll"
	@bash -lc '\
		until [[ ! -f "$(LOCK)" ]]; do \
		  echo "Generating..."; \
		  sleep 1; \
		done \
		'
	@bash -lc '\
		until [[ ! -f "$(LOCK)" ]]; do \
		  echo "Generating..."; \
		  sleep 1; \
		done \
		'

ls-procs: check-shell
	powershell -Command "tasklist /m DialUp-SDKGen.dll"

configure-plugin: check-shell
	$(call run_with_vcvars, cmake -S ./plugin -B plugin/build -G $(GENERATOR) -DCMAKE_BUILD_TYPE=RelWithDebInfo)

build-plugin: check-shell
	$(call run_with_vcvars, cmake --build plugin/build --config RelWithDebInfo)

install-plugin: check-shell
	$(call run_with_vcvars, cmake --install plugin/build --config RelWithDebInfo)

clean-plugin: check-shell
	@rm -rf plugin/build
	@rm -rf plugin/.build-artifacts
