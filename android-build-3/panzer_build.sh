#!/bin/bash

#set your pathes here
PM_SDL_DIR=/home/panzerschrek/Projects/SDL2-2.0.9/
export ANDROID_HOME=/home/panzerschrek/Projects/android-sdk/
export ANDROID_NDK_HOME=/home/panzerschrek/Projects/android-ndk-r20/

# Create symlinks to SDL android project files
ln -sf $PM_SDL_DIR/android-project/gradle gradle
ln -sf $PM_SDL_DIR/android-project/build.gradle build.gradle
ln -sf $PM_SDL_DIR/android-project/gradlew gradlew
ln -sf $PM_SDL_DIR/android-project/gradle.properties gradle.properties
ln -sf $PM_SDL_DIR/android-project/settings.gradle settings.gradle
cd app
ln -sf $PM_SDL_DIR/android-project/app/src src
ln -sf $PM_SDL_DIR/android-project/app/build.gradle build.gradle
ln -sf $PM_SDL_DIR/android-project/app/proguard-rules.pro proguard-rules.pro
cd jni
ln -sf $PM_SDL_DIR SDL2

cd ../..

./gradlew
./gradlew build