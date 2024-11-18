#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )"

build_android() {
    cd $SCRIPT_DIR/../depts/ffmpeg-kit/

    export ANDROID_SDK_ROOT=/Users/lewisou/Library/Android/sdk
    export ANDROID_NDK_ROOT=/Users/lewisou/Library/Android/sdk/ndk/22.0.7026061

    ./android.sh --lts --speed --no-archive

    TARGET_DIR=$SCRIPT_DIR/../libs/ffmpeg/android/armeabi-v7a/
    # rm -rf $TARGET_DIR
    mkdir -p $TARGET_DIR
    cp ./prebuilt/android-arm-lts/ffmpeg/lib/*.a $TARGET_DIR

    TARGET_DIR=$SCRIPT_DIR/../libs/ffmpeg/android/arm64-v8a/
    # rm -rf $TARGET_DIR
    mkdir -p $TARGET_DIR
    cp ./prebuilt/android-arm64-lts/ffmpeg/lib/*.a $TARGET_DIR
    
    TARGET_DIR=$SCRIPT_DIR/../libs/ffmpeg/android/x86/
    # rm -rf $TARGET_DIR
    mkdir -p $TARGET_DIR
    cp ./prebuilt/android-x86-lts/ffmpeg/lib/*.a $TARGET_DIR

    TARGET_DIR=$SCRIPT_DIR/../libs/ffmpeg/android/x86_64/
    # rm -rf $TARGET_DIR
    mkdir -p $TARGET_DIR
    echo mkdir -p $TARGET_DIR
    cp ./prebuilt/android-x86_64-lts/ffmpeg/lib/*.a $TARGET_DIR

    TARGET_INCLUDE_DIR=$SCRIPT_DIR/../libs/ffmpeg/
    # rm -rf $TARGET_DIR
    mkdir -p $TARGET_INCLUDE_DIR
    cp -r ./prebuilt/android-arm-lts/ffmpeg/include $TARGET_INCLUDE_DIR
}

build_ios() {
    cd $SCRIPT_DIR/../depts/ffmpeg-kit/

    ./ios.sh --lts --speed

    TARGET_DIR=$SCRIPT_DIR/../libs/ffmpeg/ios
    rm -rf $TARGET_DIR
    mkdir -p $TARGET_DIR

    lib_array=()
    FILES=(prebuilt/apple-ios-arm64-lts/ffmpeg*/lib/*.a)
    for f in "${FILES[@]}"
    do
        REPLY=$(basename "$f")
        lib_array+=("$REPLY")
    done

    for liba in "${lib_array[@]}"
    do
        echo Generate Fat: $liba
        files_array=()
        while IFS=  read -r -d $'\0'; do
            files_array+=("$REPLY")
        done < <(find prebuilt/apple-ios*lts/ffmpeg*/lib/$liba -print0)

        files=${files_array[@]}
        lipo -create $files -output $TARGET_DIR/$liba
    done
}

build_macos() {
    cd $SCRIPT_DIR/../depts/ffmpeg-kit/

    ./macos.sh --lts --speed

    TARGET_DIR=$SCRIPT_DIR/../libs/ffmpeg/macos/
    rm -rf $TARGET_DIR
    mkdir -p $TARGET_DIR/x86_64/
    mkdir -p $TARGET_DIR/arm64/

    cp ./prebuilt/apple-macos-x86_64-lts/ffmpeg/lib/*.a $TARGET_DIR/x86_64/
    cp ./prebuilt/apple-macos-arm64-lts/ffmpeg/lib/*.a $TARGET_DIR/arm64/
}

# build_ios
# build_android
# build_macos