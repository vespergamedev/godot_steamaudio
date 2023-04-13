BUILD_DIR=build
GODOT_DIR=$(BUILD_DIR)/godot
BUILD_TARGET=$(BUILD_DIR)/godot/bin/godot.linuxbsd.tools.64.llvm
RELEASE_EXPORT_TEMPLATE=$(BUILD_DIR)/godot/export_templates/linux_x11_64_release
DEBUG_EXPORT_TEMPLATE=$(BUILD_DIR)/godot/export_templates/linux_x11_64_debug
NUM_PAR_JOBS=$(shell nproc)
GODOT_BUILD_CMD=scons platform=linuxbsd use_llvm=yes use_lld=yes use_static_cpp=no -j$(NUM_PAR_JOBS) -Q VERBOSE=1

default: $(BUILD_TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(GODOT_DIR): $(BUILD_DIR)
	cp -rfP godot $(BUILD_DIR)/.
	mkdir -p $(GODOT_DIR)/export_templates

$(RELEASE_EXPORT_TEMPLATE): $(GODOT_DIR)
	cd $(GODOT_DIR) && $(GODOT_BUILD_CMD) tools=no target=template_release bits=64
	mv $(GODOT_DIR)/bin/godot.linuxbsd.template_release.x86_64.llvm $(GODOT_DIR)/export_templates/linux_x11_64_release

$(DEBUG_EXPORT_TEMPLATE): $(GODOT_DIR)
	cd $(GODOT_DIR) && $(GODOT_BUILD_CMD) tools=no target=template_debug bits=64
	mv $(GODOT_DIR)/bin/godot.linuxbsd.template_debug.x86_64.llvm $(GODOT_DIR)/export_templates/linux_x11_64_debug

$(BUILD_TARGET): $(GODOT_DIR) $(RELEASE_EXPORT_TEMPLATE) $(DEBUG_EXPORT_TEMPLATE)
	cd $(GODOT_DIR) && $(GODOT_BUILD_CMD)
	cp -rfP $(GODOT_DIR)/modules/godot_steamaudio/external/steamaudio/lib/linux-x64/* $(BUILD_DIR)/godot/bin/.

clean:
	rm -rf $(BUILD_DIR)
