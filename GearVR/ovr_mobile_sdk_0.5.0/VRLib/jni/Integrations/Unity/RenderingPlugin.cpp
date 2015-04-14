/************************************************************************************

Filename    :   RenderingPlugin.cpp
Content     :   DLL Plugin; Expose rendering functionality to applications
Created     :   January 2, 2013
Authors     :   Peter Giokaris

Copyright   :   Copyright 2014 Oculus, Inc. All Rights reserved.

*************************************************************************************/
#include <jni.h>
#include <unistd.h>						// usleep, etc
#include <sys/syscall.h>

#include "Android/LogUtils.h"

#define EXPORT_API
