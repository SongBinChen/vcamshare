# The C++ library shared by the iOS app and the Android app

## Build dependencies

### Build boost

    scripts/build_boost.sh``
    
    - Uncomment the line #72 to build for Android
    - Uncomment the line #75 to build for Android

### Build FFmpeg

    scripts/build_ffmpeg.sh

    - Uncomment the line #84 to build for iOS
    - Uncomment the line #85 to build for Android
    - Uncomment the line #86 to build for Macos


## Build the library

    scripts/build_mobile.sh
  
    - Uncomment the line #69 to build for iOS.
    - Uncomment the line #69 to build for Android.
    - Change the line #7 to your local NDK location for Android build only.

## Run Unit Tests
    scripts/test.sh
    - Modify line #11 to specify which part of tests to run.

## Interface
    Please check src/main/vcamshare.h
