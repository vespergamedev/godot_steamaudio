**Godot Steam&reg;Audio**

This is a Godot 4.x module that adds support for Valve's Steam&reg; Audio SDK/Library for spatialization of sound in a 3D space. This module provides an integration with the Godot (4.x) game engine and is MIT-licensed (MODULELICENSE.md), but the Steam&reg; Audio SDK/Library itself is licensed under Valve's own license (VALVELICENCE.md) and thus if you use this module you are bound by Valve's terms.

You can find the Steam&reg; Audio Github page here: https://github.com/ValveSoftware/steam-audio
You can also find the latest release here under "C API": https://valvesoftware.github.io/steam-audio/downloads.html

This repository itself (godot\_steamaudio) does not contain or host the Steam&reg; Audio SDK/Library. You will need to acquire this yourself via the above release link.

***Building***

As of this time this is a module, which means it must be compiled as you are building the Godot engine. Work is being done to move this module to the GDExtension framework to avoid the need to recompile the engine, however at this time there are several blockers related to the audio components exposed by https://github.com/godotengine/godot-cpp.

At this time this repository only supports Linux builds. I will also work on Mac support. Contributions to get Windows support working would be greatly appreciated as I do not use Windows.
If you are unfamiliar with compiling Godot from source, please read this guide first from the official docs: https://docs.godotengine.org/en/stable/contributing/development/compiling/index.html
To get a Linux build going, clone or copy this repository into the Godot source's modules folder:
```
pushd godot/modules
git clone git@github.com:ai/vespergamedev/godot_steamaudio.git .
popd
```

Acquire the Steam&reg; Audio SDK/Library and place it in godot/modules/godot_steamaudio/external like so (replace the URL/version as needed):
```
wget https://github.com/ValveSoftware/steam-audio/releases/download/v4.1.4/steamaudio_4.1.4.zip
mkdir -p godot/modules/godot_steamaudio/external/
unzip steamaudio_4.1.4.zip -d godot/modules/godot_steamaudio/external/
```

Personally, I like to create a build directory to build the engine in that is separate from the main source directory. Here is a Makefile that builds the engine (it assumes you are executing it in a directoy above "godot"), release templates, and then places the Steam&reg; Audio Library in the final "bin" location. I've included it in the misc directory of this repository as reference. This uses clang/llvm but you can use gcc if you prefer.
```
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

```

With a Makefile like this, you can simply run "make" in the directory above your godot source location. In this case, the final output will be in $(BUILD_DIR)/godot/bin. Feel free to adapt however you wish, this is a complete engine build of course and your own needs may require further customization.

***Sample Project***

A sample project with multiple test scenes is available at https://github.com/vespergamedev/godot\_steamaudio\_sample\_project

***Current Status***

At this time there is support for rendering 3D ambisonics, volumetric occlusion by geometry, transmission through geometry, and distance attenuation. 

***Road Map***

1. Real-time Reflections (echoes caused by geometry) support via GPU or CPU ray-tracing
2. Non-static geometry (moving objects) influencing the simulation
3. Exposing controls in the Godot Editor through inspect settings and project settings
4. Pathing baking - offline computation of paths to spatialize sound. This requires a lot of Editor work!
5. More as they come!
