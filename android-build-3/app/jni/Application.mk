# You can find more information here:
# https://developer.android.com/ndk/guides/cpp-support

APP_ABI := armeabi-v7a arm64-v8a x86 x86_64
APP_STL := c++_shared
APP_CPPFLAGS := -std=c++14 -frtti -DPM_OPENGL_ES

# Min runtime API level
APP_PLATFORM=android-14
