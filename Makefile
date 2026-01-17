LOCALAPPDATA := $(shell powershell -NoProfile -Command "[Environment]::GetFolderPath('LocalApplicationData')")
DIALUP_ROOT       := $(LOCALAPPDATA)/DialUp
include $(DIALUP_ROOT)/build-tools/common.mk
include $(DIALUP_ROOT)/build-tools/shell.mk
include $(DIALUP_ROOT)/build-tools/rocketleague.mk

BUILD_DIR        := build
DLL              := DialUp-SDKGen.dll
SDK_OUTPUT_DIR   := ../sdk-plugin/generated

.PHONY: configure build clean inject

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

build:
	$(call run_with_vcvars, cmake --build build --config RelWithDebInfo $(ARGS))

clean:
	@rm -rf build

inject: check-shell
	# copy to RL bin dir, run, remove
	@bash -lc 'cp -v "$(BUILD_DIR)/$(DLL)" "$(binary_dir)/$(DLL)"'
	@powershell -NoLogo -NoProfile -Command "cmd /C 'call \"$(INJECTOR)\" \"${target}\" \"${DLL}\"'"
	@bash -lc 'rm "$(binary_dir)/$(DLL)"'

ls-procs: check-shell
	powershell -Command "tasklist /m DialUp-SDKGen.dll"
