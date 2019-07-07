#!/bin/bash

#set your pathes here
PM_SDL_DIR=/home/panzerschrek/Projects/SDL2-2.0.9/
export ANDROID_HOME=/home/panzerschrek/Projects/android-sdk/
export ANDROID_NDK_HOME=/home/panzerschrek/Projects/android-ndk-r20/

# Create symlinks to SDL android project files
ln -sfT $PM_SDL_DIR/android-project/gradle gradle
ln -sfT $PM_SDL_DIR/android-project/build.gradle build.gradle
ln -sfT $PM_SDL_DIR/android-project/gradlew gradlew
ln -sfT $PM_SDL_DIR/android-project/gradle.properties gradle.properties
ln -sfT $PM_SDL_DIR/android-project/settings.gradle settings.gradle
cd app
ln -sfT $PM_SDL_DIR/android-project/app/proguard-rules.pro proguard-rules.pro
cd src/main/java
ln -sfT $PM_SDL_DIR/android-project/app/src/main/java/org org
cd ../../../jni
ln -sfT $PM_SDL_DIR SDL2
cd src
ln -sfT ../../../../source source
cd ../../..

./gradlew
./gradlew build