#!/bin/bash

BOOST_VERSION=1.76.0
BOOST_LIBS=test

SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )"

build_apple() {
    cd $SCRIPT_DIR/../depts/Apple-Boost-BuildScript/
    ./boost.sh --boost-version $BOOST_VERSION --boost-libs $BOOST_LIBS -macos -ios

    TARGET_DIR=$SCRIPT_DIR/../libs/boost

    mkdir -p $TARGET_DIR/apple
    cd $SCRIPT_DIR/../depts/Apple-Boost-BuildScript/build/boost/$BOOST_VERSION
    cp ./macos-combined/release/build/libboost.a $TARGET_DIR/apple/

    echo Copy header files ...
    mkdir -p $TARGET_DIR/include
    cp -r $SCRIPT_DIR/../depts/Apple-Boost-BuildScript/src/boost_${BOOST_VERSION//./_}/boost $TARGET_DIR/include/
}

repack_android_lib() {
    arbin=$NDK_ROOT/toolchains/llvm/prebuilt/darwin-x86_64/bin/llvm-ar
    abis=(arm64-v8a armeabi-v7a x86 x86_64)
    for abi in ${abis[@]}
    do
        echo Repack $abi
        cd $SCRIPT_DIR/../depts/Boost-for-Android/build/out/$abi
        mkdir -p objects
        mkdir -p objects_renamed
        rm ./objects/*.o
        rm ./objects_renamed/*.o
        
        cd objects
        for FILE in ../lib/libboost_*.a; do            
            BNAME=$(basename ${FILE})
            ANAME=${BNAME:9:10}
            
            $arbin -x "$FILE";

            for OFILE in *.o; do
                NEW_FILE="../objects_renamed/${ANAME}_${OFILE}"
                mv -i "$OFILE" "$NEW_FILE"
            done
        done

        cd $SCRIPT_DIR/../depts/Boost-for-Android/build/out/$abi
        $arbin crus libboost.a ./objects_renamed/*.o

        TARGET_DIR=$SCRIPT_DIR/../libs/boost/android/$abi
        mkdir -p $TARGET_DIR
        mv ./libboost.a $TARGET_DIR
        echo "Android Boost Lib move to $TARGET_DIR"
    done
}

build_android() {
    if [ -z "$NDK_ROOT" ]; then
        echo "Need NDK_ROOT"
        exit
    fi
    cd $SCRIPT_DIR/../depts/Boost-for-Android/
    rm -rf ./build
    ./build-android.sh "$NDK_ROOT" --boost=$BOOST_VERSION --with-libraries=$BOOST_LIBS

    repack_android_lib
}


# Uncomment the lines to build for Android
# build_android

# Uncomment the lines to build for iOS
# build_apple

