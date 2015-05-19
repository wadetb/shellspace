# Common build settings for all VR apps

# This needs to be defined to get the right header directories for egl / etc
APP_PLATFORM := android-19

# This needs to be defined to avoid compile errors like:
# Error: selected processor does not support ARM mode `ldrex r0,[r3]'
APP_ABI := armeabi-v7a

# Statically link the GNU STL. This may not be safe for multi-so libraries but
# we don't know of any problems yet.
APP_STL := gnustl_static

# Make sure every shared lib includes a .note.gnu.build-id header, for crash reporting
APP_LDFLAGS := -Wl,--build-id

# Explicitly use GCC 4.8 as our toolchain. This is the 32-bit default as of
# r10d but versions as far back as r9d have 4.8. The previous default, 4.6, is
# deprecated as of r10c.
NDK_TOOLCHAIN_VERSION := 4.8

# Define the directories for $(import-module, ...) to look in
ROOT_DIR := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))
NDK_MODULE_PATH := $(ROOT_DIR)/3rdParty/libjpeg-turbo$(HOST_DIRSEP)$(ROOT_DIR)/3rdParty/breakpad/android$(HOST_DIRSEP)$(ROOT_DIR)/Tools$(HOST_DIRSEP)$(ROOT_DIR)
