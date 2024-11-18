#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )"

build_android_abi() {
    ABI=$1
    NDK=$HOME/Library/Android/sdk/ndk/22.0.7026061
    MINSDKVERSION=23

    cd $SCRIPT_DIR/../
    BUILD_FOLDER=./build/android/$ABI
    rm -rf $BUILD_FOLDER
    mkdir -p $BUILD_FOLDER
    cd $BUILD_FOLDER
    cmake \
        -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
        -DANDROID_ABI=$ABI \
        -DANDROID_PLATFORM=android-$MINSDKVERSION \
        ../../..

        # -DCMAKE_BUILD_TYPE=Debug \

    make
    cmake --install . --prefix ./output
    # make
    
    # VERBOSE=1
}

build_android() {
    abis=(arm64-v8a armeabi-v7a x86 x86_64)
    for abi in ${abis[@]}
    do
        echo build $abi
        build_android_abi $abi
    done

    # cd $SCRIPT_DIR/../build/android/
    # mkdir -p ./include/vcamshare
    # cp $SCRIPT_DIR/
}

build_ios() {
    BUILD_DIR=$SCRIPT_DIR/../build/ios/

    rm -rf $BUILD_DIR
    mkdir -p $BUILD_DIR
    cd $BUILD_DIR
    cmake ../.. -G Xcode -DCMAKE_TOOLCHAIN_FILE=../../depts/ios-cmake/ios.toolchain.cmake -DPLATFORM=OS64COMBINED
    # cmake ../.. -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DPLATFORM=ios

    # xcodebuild clean build \
    #     -project vcamshare.xcodeproj \
    #     -scheme vcamshare \
    #     -configuration Release \
    #     -sdk iphoneos \
    #     -derivedDataPath derived_data \
    #     BUILD_LIBRARY_FOR_DISTRIBUTION=YES

    # xcodebuild clean build \
    #   -project vcamshare.xcodeproj \
    #   -scheme vcamshare \
    #   -configuration Release \
    #   -sdk iphonesimulator \
    #   -derivedDataPath derived_data \
    #   BUILD_LIBRARY_FOR_DISTRIBUTION=YES
}

# build_ios
build_android
