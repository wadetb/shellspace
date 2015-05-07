LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(OVR_MOBILE_SDK)/VRLib/import_vrlib.mk		# import VRLib for this module.  Do NOT call $(CLEAR_VARS) until after building your module.
										# use += instead of := when defining the following variables: LOCAL_LDLIBS, LOCAL_CFLAGS, LOCAL_C_INCLUDES, LOCAL_STATIC_LIBRARIES 

### XXX tls_openssl or tls_gnutls

PLUGIN_SRC_FILES := \
	v8.cpp \
	vncplugin.cpp \
	libvncserver/common/minilzo.c \
	libvncserver/libvncclient/cursor.c \
	libvncserver/libvncclient/listen.c \
	libvncserver/libvncclient/rfbproto.c \
	libvncserver/libvncclient/sockets.c \
	libvncserver/libvncclient/tls_none.c \
	libvncserver/libvncclient/vncviewer.c \
	shellplugin.cpp 

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

LOCAL_MODULE     := ovrapp
LOCAL_SRC_FILES  := $(COMMON_SRC_FILES) $(PLUGIN_SRC_FILES)
LOCAL_LDLIBS	 +=
LOCAL_CFLAGS	 += -Wall -x c++ -std=c++11 -isystem $(LOCAL_PATH)/libvncserver -isystem $(LOCAL_PATH)/libvncserver/common
LOCAL_CFLAGS     += -funwind-tables -Wl,--no-merge-exidx-entries
LOCAL_C_INCLUDES += 

include $(BUILD_SHARED_LIBRARY)

# native activities need this, regular java projects don't
# $(call import-module,android/native_app_glue)
