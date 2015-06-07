LOCAL_PATH := $(call my-dir)

CORE_PATH       := ../../core
SHELLSPACE_PATH := ../../shellspace
PLUGINS_PATH    := ../../plugins
EXTERNAL_PATH   := ../../external

# Prebuilt VLC 

include $(CLEAR_VARS)
LOCAL_MODULE    := libvlc
LOCAL_SRC_FILES := $(EXTERNAL_PATH)/vlc/libvlc.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libvlccore
LOCAL_SRC_FILES := $(EXTERNAL_PATH)/vlc/libvlccore.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libcompat
LOCAL_SRC_FILES := $(EXTERNAL_PATH)/vlc/libcompat.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libiconv
LOCAL_SRC_FILES := $(EXTERNAL_PATH)/vlc/libiconv.a
include $(PREBUILT_STATIC_LIBRARY)

VLC_PLUGINS += liba52_plugin
VLC_PLUGINS += liba52tofloat32_plugin
VLC_PLUGINS += liba52tospdif_plugin
VLC_PLUGINS += libaccess_mms_plugin
VLC_PLUGINS += libaccess_realrtsp_plugin
VLC_PLUGINS += libadjust_plugin
VLC_PLUGINS += libadpcm_plugin
VLC_PLUGINS += libaes3_plugin
VLC_PLUGINS += libafile_plugin
VLC_PLUGINS += libaiff_plugin
VLC_PLUGINS += libamem_plugin
VLC_PLUGINS += libanaglyph_plugin
#VLC_PLUGINS += libandroid_audiotrack_plugin
#VLC_PLUGINS += libandroid_logger_plugin
#VLC_PLUGINS += libandroid_native_window_plugin
#VLC_PLUGINS += libandroid_surface_plugin
#VLC_PLUGINS += libandroid_window_plugin
VLC_PLUGINS += libantiflicker_plugin
VLC_PLUGINS += libaraw_plugin
VLC_PLUGINS += libasf_plugin
VLC_PLUGINS += libattachment_plugin
VLC_PLUGINS += libau_plugin
VLC_PLUGINS += libaudio_format_plugin
VLC_PLUGINS += libavcodec_plugin
VLC_PLUGINS += libavformat_plugin
VLC_PLUGINS += libavi_plugin
VLC_PLUGINS += libavio_plugin
VLC_PLUGINS += libblend_plugin
VLC_PLUGINS += libcaf_plugin
VLC_PLUGINS += libcanvas_plugin
VLC_PLUGINS += libcc_plugin
VLC_PLUGINS += libcdg_plugin
VLC_PLUGINS += libchain_plugin
VLC_PLUGINS += libchorus_flanger_plugin
VLC_PLUGINS += libchroma_yuv_neon_plugin
VLC_PLUGINS += libcolorthres_plugin
VLC_PLUGINS += libcompressor_plugin
VLC_PLUGINS += libconsole_logger_plugin
VLC_PLUGINS += libcroppadd_plugin
VLC_PLUGINS += libcvdsub_plugin
VLC_PLUGINS += libdash_plugin
VLC_PLUGINS += libdecomp_plugin
VLC_PLUGINS += libdeinterlace_plugin
VLC_PLUGINS += libdemux_cdg_plugin
VLC_PLUGINS += libdemux_stl_plugin
VLC_PLUGINS += libdemuxdump_plugin
VLC_PLUGINS += libdiracsys_plugin
VLC_PLUGINS += libdolby_surround_decoder_plugin
VLC_PLUGINS += libdsm_plugin
VLC_PLUGINS += libdts_plugin
VLC_PLUGINS += libdtstospdif_plugin
VLC_PLUGINS += libdummy_plugin
VLC_PLUGINS += libdvbsub_plugin
VLC_PLUGINS += libdvdnav_plugin
VLC_PLUGINS += libdvdread_plugin
#VLC_PLUGINS += libegl_android_plugin
VLC_PLUGINS += libequalizer_plugin
VLC_PLUGINS += libes_plugin
VLC_PLUGINS += libextract_plugin
VLC_PLUGINS += libfile_logger_plugin
VLC_PLUGINS += libfilesystem_plugin
VLC_PLUGINS += libfingerprinter_plugin
VLC_PLUGINS += libflac_plugin
VLC_PLUGINS += libflacsys_plugin
VLC_PLUGINS += libfloat_mixer_plugin
VLC_PLUGINS += libfolder_plugin
VLC_PLUGINS += libfps_plugin
VLC_PLUGINS += libfreetype_plugin
VLC_PLUGINS += libfreeze_plugin
VLC_PLUGINS += libftp_plugin
VLC_PLUGINS += libg711_plugin
VLC_PLUGINS += libgain_plugin
VLC_PLUGINS += libgaussianblur_plugin
#VLC_PLUGINS += libgles2_plugin
VLC_PLUGINS += libgnutls_plugin
VLC_PLUGINS += libgradfun_plugin
VLC_PLUGINS += libgrey_yuv_plugin
VLC_PLUGINS += libh264_plugin
VLC_PLUGINS += libhds_plugin
VLC_PLUGINS += libheadphone_channel_mixer_plugin
VLC_PLUGINS += libhevc_plugin
VLC_PLUGINS += libhqdn3d_plugin
VLC_PLUGINS += libhttp_plugin
VLC_PLUGINS += libhttplive_plugin
VLC_PLUGINS += libi420_rgb_plugin
VLC_PLUGINS += libi420_yuy2_plugin
VLC_PLUGINS += libi422_i420_plugin
VLC_PLUGINS += libi422_yuy2_plugin
VLC_PLUGINS += libimage_plugin
VLC_PLUGINS += libimem_plugin
VLC_PLUGINS += libinteger_mixer_plugin
VLC_PLUGINS += libinvert_plugin
#VLC_PLUGINS += libiomx_plugin 				# C++ code requires RTTI (not supported by stlport)
VLC_PLUGINS += libjpeg_plugin
VLC_PLUGINS += libkaraoke_plugin
VLC_PLUGINS += liblibass_plugin
VLC_PLUGINS += liblibmpeg2_plugin
VLC_PLUGINS += liblive555_plugin
VLC_PLUGINS += liblogo_plugin
VLC_PLUGINS += liblpcm_plugin
VLC_PLUGINS += libmad_plugin
VLC_PLUGINS += libmarq_plugin
#VLC_PLUGINS += libmediacodec_plugin
VLC_PLUGINS += libmjpeg_plugin
VLC_PLUGINS += libmkv_plugin
VLC_PLUGINS += libmod_plugin
VLC_PLUGINS += libmono_plugin
VLC_PLUGINS += libmp4_plugin
VLC_PLUGINS += libmpeg_audio_plugin
VLC_PLUGINS += libmpgv_plugin
VLC_PLUGINS += libnormvol_plugin
VLC_PLUGINS += libnsc_plugin
VLC_PLUGINS += libnsv_plugin
VLC_PLUGINS += libnuv_plugin
VLC_PLUGINS += libogg_plugin
VLC_PLUGINS += liboldmovie_plugin
#VLC_PLUGINS += libopensles_android_plugin
VLC_PLUGINS += libopus_plugin
VLC_PLUGINS += libpacketizer_avparser_plugin
VLC_PLUGINS += libpacketizer_dirac_plugin
VLC_PLUGINS += libpacketizer_flac_plugin
VLC_PLUGINS += libpacketizer_h264_plugin
VLC_PLUGINS += libpacketizer_hevc_plugin
VLC_PLUGINS += libpacketizer_mlp_plugin
VLC_PLUGINS += libpacketizer_mpeg4audio_plugin
VLC_PLUGINS += libpacketizer_mpeg4video_plugin
VLC_PLUGINS += libpacketizer_mpegvideo_plugin
VLC_PLUGINS += libpacketizer_vc1_plugin
VLC_PLUGINS += libparam_eq_plugin
VLC_PLUGINS += libplaylist_plugin
VLC_PLUGINS += libpng_plugin
VLC_PLUGINS += libpostproc_plugin
VLC_PLUGINS += libps_plugin
VLC_PLUGINS += libpva_plugin
VLC_PLUGINS += librar_plugin
VLC_PLUGINS += librawaud_plugin
VLC_PLUGINS += librawdv_plugin
VLC_PLUGINS += librawvid_plugin
VLC_PLUGINS += librawvideo_plugin
VLC_PLUGINS += librecord_plugin
VLC_PLUGINS += libremap_plugin
VLC_PLUGINS += librotate_plugin
VLC_PLUGINS += librtp_plugin
VLC_PLUGINS += librv32_plugin
VLC_PLUGINS += libscale_plugin
VLC_PLUGINS += libscaletempo_plugin
VLC_PLUGINS += libscte27_plugin
VLC_PLUGINS += libsdp_plugin
VLC_PLUGINS += libsepia_plugin
VLC_PLUGINS += libsftp_plugin
VLC_PLUGINS += libshm_plugin
VLC_PLUGINS += libsimple_channel_mixer_neon_plugin
VLC_PLUGINS += libsimple_channel_mixer_plugin
VLC_PLUGINS += libsmooth_plugin
VLC_PLUGINS += libspatializer_plugin
VLC_PLUGINS += libspeex_plugin
VLC_PLUGINS += libspudec_plugin
VLC_PLUGINS += libstereo_widen_plugin
VLC_PLUGINS += libstl_plugin
VLC_PLUGINS += libsubsdec_plugin
VLC_PLUGINS += libsubsdelay_plugin
VLC_PLUGINS += libsubsttml_plugin
VLC_PLUGINS += libsubstx3g_plugin
VLC_PLUGINS += libsubsusf_plugin
VLC_PLUGINS += libsubtitle_plugin
VLC_PLUGINS += libsvcdsub_plugin
VLC_PLUGINS += libswscale_plugin
VLC_PLUGINS += libsyslog_plugin
VLC_PLUGINS += libtaglib_plugin
VLC_PLUGINS += libtcp_plugin
VLC_PLUGINS += libtelx_plugin
VLC_PLUGINS += libtheora_plugin
VLC_PLUGINS += libtimecode_plugin
VLC_PLUGINS += libtransform_plugin
VLC_PLUGINS += libtrivial_channel_mixer_plugin
VLC_PLUGINS += libts_plugin
VLC_PLUGINS += libtta_plugin
VLC_PLUGINS += libttml_plugin
VLC_PLUGINS += libty_plugin
VLC_PLUGINS += libudp_plugin
VLC_PLUGINS += libugly_resampler_plugin
VLC_PLUGINS += libuleaddvaudio_plugin
VLC_PLUGINS += libupnp_plugin
VLC_PLUGINS += libvc1_plugin
VLC_PLUGINS += libvdr_plugin
VLC_PLUGINS += libvhs_plugin
VLC_PLUGINS += libvmem_plugin
VLC_PLUGINS += libvobsub_plugin
VLC_PLUGINS += libvoc_plugin
VLC_PLUGINS += libvolume_neon_plugin
VLC_PLUGINS += libvorbis_plugin
VLC_PLUGINS += libwav_plugin
VLC_PLUGINS += libwave_plugin
VLC_PLUGINS += libxa_plugin
VLC_PLUGINS += libxml_plugin
VLC_PLUGINS += libyuv_rgb_neon_plugin
VLC_PLUGINS += libyuvp_plugin
VLC_PLUGINS += libyuy2_i420_plugin
VLC_PLUGINS += libyuy2_i422_plugin
VLC_PLUGINS += libzip_plugin
VLC_PLUGINS += libzvbi_plugin

define vlc-plugin-static-library
include $$(CLEAR_VARS)
LOCAL_MODULE    := $1
LOCAL_SRC_FILES := $$(EXTERNAL_PATH)/vlc/modules/$1.a
include $$(PREBUILT_STATIC_LIBRARY)
endef

$(foreach p,$(VLC_PLUGINS),$(eval $(call vlc-plugin-static-library,$p)))

# Prebuilt V8

include $(CLEAR_VARS)
LOCAL_MODULE    := libv8_base
LOCAL_SRC_FILES := $(EXTERNAL_PATH)/v8/libv8_base.a
LOCAL_LDLIBS    := -lstdc++
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libv8_libbase
LOCAL_SRC_FILES := $(EXTERNAL_PATH)/v8/libv8_libbase.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libv8_libplatform
LOCAL_SRC_FILES := $(EXTERNAL_PATH)/v8/libv8_libplatform.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libv8_snapshot
LOCAL_SRC_FILES := $(EXTERNAL_PATH)/v8/libv8_snapshot.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libv8_nosnapshot
LOCAL_SRC_FILES := $(EXTERNAL_PATH)/v8/libv8_nosnapshot.a
include $(PREBUILT_STATIC_LIBRARY)

# Prebuilt Skia 

include $(CLEAR_VARS)
LOCAL_MODULE    := libskia_android
LOCAL_SRC_FILES := $(EXTERNAL_PATH)/skia/libskia_android.so
include $(PREBUILT_SHARED_LIBRARY)

# Shellspace Shared Object
include $(CLEAR_VARS)

include $(OVR_MOBILE_SDK)/VRLib/import_vrlib.mk	

PLUGIN_SRC_FILES := \
	$(PLUGINS_PATH)/v8/v8plugin.cpp \
	$(PLUGINS_PATH)/v8/v8skia.cpp \
	$(PLUGINS_PATH)/vlc/vlcplugin.cpp \
	$(PLUGINS_PATH)/vnc/vncplugin.cpp

EXTERNAL_SRC_FILES := \
	$(EXTERNAL_PATH)/gason/gason.cpp \
	$(EXTERNAL_PATH)/libvncserver/common/minilzo.c \
	$(EXTERNAL_PATH)/libvncserver/libvncclient/cursor.c \
	$(EXTERNAL_PATH)/libvncserver/libvncclient/listen.c \
	$(EXTERNAL_PATH)/libvncserver/libvncclient/rfbproto.c \
	$(EXTERNAL_PATH)/libvncserver/libvncclient/sockets.c \
	$(EXTERNAL_PATH)/libvncserver/libvncclient/tls_none.c \
	$(EXTERNAL_PATH)/libvncserver/libvncclient/vncviewer.c \
	$(EXTERNAL_PATH)/std_logger/std_logger.c 

#	$(EXTERNAL_PATH)/coffeecatch/coffeecatch.c 
#	$(EXTERNAL_PATH)/coffeecatch/coffeejni.c 

CORE_SRC_FILES := \
	$(CORE_PATH)/message.cpp \
	$(CORE_PATH)/profile.cpp \
	$(CORE_PATH)/thread.cpp 

SHELLSPACE_SRC_FILES := \
	$(SHELLSPACE_PATH)/api.cpp \
	$(SHELLSPACE_PATH)/command.cpp \
	$(SHELLSPACE_PATH)/entity.cpp \
	$(SHELLSPACE_PATH)/file.cpp \
	$(SHELLSPACE_PATH)/geometry.cpp \
	$(SHELLSPACE_PATH)/inqueue.cpp \
	$(SHELLSPACE_PATH)/registry.cpp \
	$(SHELLSPACE_PATH)/texture.cpp \

GEARVR_SRC_FILES := \
	OvrApp.cpp

LOCAL_ARM_MODE   := arm

# Try these: 
#LOCAL_ARM_NEON  := true				# compile with neon support enabled
#LOCAL_CFLAGS += -O3 -funroll-loops -ftree-vectorize -ffast-math -fpermissive

LOCAL_STATIC_LIBRARIES += libcompat libvlccore libvlc libiconv $(foreach p,$(VLC_PLUGINS),$p )) 
LOCAL_STATIC_LIBRARIES += libv8_base libv8_nosnapshot libv8_libplatform libv8_libbase
LOCAL_SHARED_LIBRARIES += libskia_android

LOCAL_MODULE     := shellspace

LOCAL_SRC_FILES  := $(CORE_SRC_FILES) $(PLUGIN_SRC_FILES) $(SHELLSPACE_SRC_FILES) $(GEARVR_SRC_FILES) $(EXTERNAL_SRC_FILES)

LOCAL_LDLIBS += \
	-L../external/vlc/contrib \
	-ldl -lz -lm -llog \
	-ldvbpsi -lmatroska -lebml -ltag \
	-logg -lFLAC -ltheora -lvorbis \
	-lmpeg2 -la52 \
	-lavformat -lavcodec -lswscale -lavutil -lpostproc -lgsm -lopenjpeg \
	-lliveMedia -lUsageEnvironment -lBasicUsageEnvironment -lgroupsock \
	-lspeex -lspeexdsp \
	-lxml2 -lpng -lgnutls -lgcrypt -lgpg-error \
	-lnettle -lhogweed -lgmp \
	-lfreetype -liconv -lass -lfribidi -lopus \
	-lEGL -lGLESv2 -ljpeg \
	-ldvdnav -ldvdread -ldvdcss \
	-ldsm -ltasn1 \
	-lmad \
	-lzvbi \
	-lssh2 \
	-lmodplug \
	-lupnp -lthreadutil -lixml \
	$(EXTRA_LDFLAGS)

ABSOLUTE_ROOT_PATH     = $(LOCAL_PATH)/../..

LOCAL_CFLAGS	 += -Wall -x c++ -std=c++11 
LOCAL_CFLAGS     += -I$(ABSOLUTE_ROOT_PATH)/core -I$(ABSOLUTE_ROOT_PATH)/shellspace
LOCAL_CFLAGS     += -isystem $(ABSOLUTE_ROOT_PATH)/external/libvncserver -isystem $(ABSOLUTE_ROOT_PATH)/external/libvncserver/common 
LOCAL_CFLAGS     += -isystem $(ABSOLUTE_ROOT_PATH)/external
LOCAL_CFLAGS     += -isystem $(ABSOLUTE_ROOT_PATH)/external/v8
LOCAL_CFLAGS     += -isystem $(ABSOLUTE_ROOT_PATH)/external/skia/include
LOCAL_CFLAGS     += -isystem $(ABSOLUTE_ROOT_PATH)/external/skia/include/config

include $(BUILD_SHARED_LIBRARY)
