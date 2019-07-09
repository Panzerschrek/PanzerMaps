LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL2

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include

PM_SOURCES_ROOT= source
LOCAL_SRC_FILES := \
	$(PM_SOURCES_ROOT)/common/coordinates_conversion.cpp \
	$(PM_SOURCES_ROOT)/common/data_file.cpp \
	$(PM_SOURCES_ROOT)/common/log.cpp \
	$(PM_SOURCES_ROOT)/common/memory_mapped_file.cpp \
	$(PM_SOURCES_ROOT)/maps/android/gps_service.cpp \
	$(PM_SOURCES_ROOT)/maps/main.cpp \
	$(PM_SOURCES_ROOT)/maps/main_loop.cpp \
	$(PM_SOURCES_ROOT)/maps/map_drawer.cpp \
	$(PM_SOURCES_ROOT)/maps/mouse_map_controller.cpp \
	$(PM_SOURCES_ROOT)/maps/shaders.cpp \
	$(PM_SOURCES_ROOT)/maps/system_window.cpp \
	$(PM_SOURCES_ROOT)/maps/textures_generation.cpp \
	$(PM_SOURCES_ROOT)/maps/touch_map_controller.cpp \
	$(PM_SOURCES_ROOT)/maps/ui_drawer.cpp \
	$(PM_SOURCES_ROOT)/maps/zoom_controller.cpp \
	$(PM_SOURCES_ROOT)/panzer_ogl_lib/func_addresses.cpp \
	$(PM_SOURCES_ROOT)/panzer_ogl_lib/glsl_program.cpp \
	$(PM_SOURCES_ROOT)/panzer_ogl_lib/matrix.cpp \
	$(PM_SOURCES_ROOT)/panzer_ogl_lib/polygon_buffer.cpp \
	$(PM_SOURCES_ROOT)/panzer_ogl_lib/shaders_loading.cpp \
	$(PM_SOURCES_ROOT)/panzer_ogl_lib/texture.cpp \

LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)
