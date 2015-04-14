LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(OVR_MOBILE_SDK)/VRLib/import_vrlib.mk		# import VRLib for this module.  Do NOT call $(CLEAR_VARS) until after building your module.
										# use += instead of := when defining the following variables: LOCAL_LDLIBS, LOCAL_CFLAGS, LOCAL_C_INCLUDES, LOCAL_STATIC_LIBRARIES 

### XXX tls_openssl or tls_gnutls

LIBVNCCLIENT_SRC_FILES := \
	3rdParty/libvncserver/common/minilzo.c \
	3rdParty/libvncserver/libvncclient/cursor.c \
	3rdParty/libvncserver/libvncclient/listen.c \
	3rdParty/libvncserver/libvncclient/rfbproto.c \
	3rdParty/libvncserver/libvncclient/sockets.c \
	3rdParty/libvncserver/libvncclient/tls_none.c \
	3rdParty/libvncserver/libvncclient/vncviewer.c \
	3rdParty/gason/gason.cpp

LIBVNCCLIENT_INCLUDE_DIRS := \
	-Ijni/3rdParty/libvncserver \
	-Ijni/3rdParty/libvncserver/common \

COMMON_SRC_FILES := \
	OvrApp.cpp \
	command.cpp \
	keyboard.cpp \
	profile.cpp

WIDGET_SRC_FILES := \
	vncwidget.cpp

LOCAL_ARM_MODE := arm

LOCAL_MODULE    := ovrapp
LOCAL_SRC_FILES := $(COMMON_SRC_FILES) $(WIDGET_SRC_FILES) $(LIBVNCCLIENT_SRC_FILES)
LOCAL_LDLIBS	+=
LOCAL_CFLAGS	+= -std=c++11 -Ijni/3rdParty $(LIBVNCCLIENT_INCLUDE_DIRS)
LOCAL_C_INCLUDES += 

include $(BUILD_SHARED_LIBRARY)

# native activities need this, regular java projects don't
# $(call import-module,android/native_app_glue)
