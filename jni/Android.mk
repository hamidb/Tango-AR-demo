#
# Copyright 2014 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)
TANGO_ROOT:= /hamidb/software/applications/tango-examples-c

include $(CLEAR_VARS)
LOCAL_MODULE := lib_cudaCode
LOCAL_SRC_FILES := ../cuda/lib_cudaCode.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libcudart_static
LOCAL_LIB_PATH   += $(CUDA_TOOLKIT_ROOT)/targets/armv7-linux-androideabi/lib/
LOCAL_SRC_FILES  := $(LOCAL_LIB_PATH)/libcudart_static.a 
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
INSTALL_CUDA_LIBRARIES:=on
OPENCV_INSTALL_MODULES:=on
CUDA_TOOLKIT_DIR=$(CUDA_TOOLKIT_ROOT)
NDK_TOOLCHAIN_VERSION = 4.8
include ../../OpenCV-2.4.8.2-Tegra-sdk/sdk/native/jni/OpenCV-tegra3.mk

LOCAL_MODULE    		:= myTangoProject
LOCAL_STATIC_LIBRARIES 	:= lib_cudaCode libcudart_static
LOCAL_SHARED_LIBRARIES 	+= tango_client_api
LOCAL_CFLAGS    		:= -Werror -std=c++11 -fno-short-enums
LOCAL_LDFLAGS 	+= -Os
LOCAL_SRC_FILES := tango_native.cc \
                   tango_handler.cc \
                   yuv_drawable.cc \
                   frame_processor.cc \
                   rhorefc.cc \
                   $(TANGO_ROOT)/tango-gl/camera.cpp \
                   $(TANGO_ROOT)/tango-gl/line.cpp \
                   $(TANGO_ROOT)/tango-gl/util.cpp \
                   $(TANGO_ROOT)/tango-gl/transform.cpp \
                   $(TANGO_ROOT)/tango-gl/drawable_object.cpp \
                   $(TANGO_ROOT)/tango-gl/shaders.cpp \
                   $(TANGO_ROOT)/tango-gl/video_overlay.cpp

LOCAL_C_INCLUDES += $(TANGO_ROOT)/tango-gl/include \
                    $(TANGO_ROOT)/third-party/glm \
                    tango-video-handler
LOCAL_C_INCLUDES += $(LOCAL_PATH)/..
LOCAL_C_INCLUDES += $(CUDA_TOOLKIT_ROOT)/targets/armv7-linux-androideabi/include
LOCAL_LDLIBS     := -llog -landroid -lGLESv2 -L$(SYSROOT)/usr/lib

include $(BUILD_SHARED_LIBRARY)

$(call import-add-path, $(TANGO_ROOT))
$(call import-module,tango_client_api)
