LOCAL_PATH := $(call my-dir)

# Prebuilt V8
include $(CLEAR_VARS)
LOCAL_MODULE    := libv8_base
LOCAL_SRC_FILES := v8/libv8_base.a
LOCAL_LDLIBS    := -lstdc++
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libv8_libbase
LOCAL_SRC_FILES := v8/libv8_libbase.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libv8_libplatform
LOCAL_SRC_FILES := v8/libv8_libplatform.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libv8_snapshot
LOCAL_SRC_FILES := v8/libv8_snapshot.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libv8_nosnapshot
LOCAL_SRC_FILES := v8/libv8_nosnapshot.a
include $(PREBUILT_STATIC_LIBRARY)

# Shellspace Shared Object
include $(CLEAR_VARS)

include $(OVR_MOBILE_SDK)/VRLib/import_vrlib.mk	

### XXX tls_openssl or tls_gnutls

PLUGIN_SRC_FILES := \
	vncplugin.cpp \
	libvncserver/common/minilzo.c \
	libvncserver/libvncclient/cursor.c \
	libvncserver/libvncclient/listen.c \
	libvncserver/libvncclient/rfbproto.c \
	libvncserver/libvncclient/sockets.c \
	libvncserver/libvncclient/tls_none.c \
	libvncserver/libvncclient/vncviewer.c \
	shellplugin.cpp 

PLUGIN_SRC_FILES += v8plugin.cpp 

COMMON_SRC_FILES := \
	api.cpp \
	command.cpp \
	entity.cpp \
	file.cpp \
	gason/gason.cpp \
	geometry.cpp \
	inqueue.cpp \
	keyboard.cpp \
	message.cpp \
	OvrApp.cpp \
	profile.cpp \
	registry.cpp \
	texture.cpp \
	thread.cpp 

#	coffeecatch/coffeecatch.c 
#	coffeecatch/coffeejni.c 

LOCAL_ARM_MODE   := arm
# Try this: 
#LOCAL_ARM_NEON  := true				# compile with neon support enabled
#LOCAL+CFLAGS += -O3 -funroll-loops -ftree-vectorize -ffast-math -fpermissive

LOCAL_STATIC_LIBRARIES	 += libv8_base libv8_nosnapshot libv8_libplatform libv8_libbase

LOCAL_MODULE     := ovrapp
LOCAL_SRC_FILES  := $(COMMON_SRC_FILES) $(PLUGIN_SRC_FILES)
LOCAL_CFLAGS	 += -Wall -x c++ -std=c++11 
LOCAL_CFLAGS     += -isystem $(LOCAL_PATH)/libvncserver -isystem $(LOCAL_PATH)/libvncserver/common 
LOCAL_CFLAGS     += -isystem $(LOCAL_PATH)/v8
LOCAL_CFLAGS     += -funwind-tables -Wl,--no-merge-exidx-entries
LOCAL_C_INCLUDES += 

include $(BUILD_SHARED_LIBRARY)
