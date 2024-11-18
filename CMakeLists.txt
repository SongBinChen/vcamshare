cmake_minimum_required(VERSION 3.15)

# set the project name
project(vcamshare VERSION 0.1)

configure_file(./src/config.h.in config.h)

set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)

# set(CMAKE_CXX_FLAGS_DEBUG  "-g")
# set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
# set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

# src folder
set(main_sources
    vcamshare.h
    video_muxer.cpp
    utils.cpp
    vcamshare.cpp
)

set(public_headers
    vcamshare.h
)

list(TRANSFORM main_sources PREPEND "src/main/")
list(TRANSFORM public_headers PREPEND "src/main/")

add_library(${CMAKE_PROJECT_NAME} SHARED ${main_sources})

# Find library
if(APPLE)
    if(IOS)
        # enable_language(Swift)
        message(IOS="${IOS}")
        find_library(AVUTIL avutil ${PROJECT_SOURCE_DIR}/libs/ffmpeg/ios NO_DEFAULT_PATH)
        find_library(AVFORMAT2 avformat ${PROJECT_SOURCE_DIR}/libs/ffmpeg/ios NO_DEFAULT_PATH)
        find_library(AVCODEC avcodec ${PROJECT_SOURCE_DIR}/libs/ffmpeg/ios NO_DEFAULT_PATH)
        find_library(SWSCALE swscale ${PROJECT_SOURCE_DIR}/libs/ffmpeg/ios NO_DEFAULT_PATH)
        find_library(SWRESAMPLE swresample ${PROJECT_SOURCE_DIR}/libs/ffmpeg/ios NO_DEFAULT_PATH)
    else()
        message(CMAKE_SYSTEM_PROCESSOR="${CMAKE_SYSTEM_PROCESSOR}")
        find_library(AVFORMAT2 avformat ${PROJECT_SOURCE_DIR}/libs/ffmpeg/macos/${CMAKE_SYSTEM_PROCESSOR} NO_DEFAULT_PATH)
        find_library(AVUTIL avutil ${PROJECT_SOURCE_DIR}/libs/ffmpeg/macos/${CMAKE_SYSTEM_PROCESSOR} NO_DEFAULT_PATH)
        find_library(AVCODEC avcodec ${PROJECT_SOURCE_DIR}/libs/ffmpeg/macos/${CMAKE_SYSTEM_PROCESSOR} NO_DEFAULT_PATH)
        find_library(SWSCALE swscale ${PROJECT_SOURCE_DIR}/libs/ffmpeg/macos/${CMAKE_SYSTEM_PROCESSOR} NO_DEFAULT_PATH)
        find_library(SWRESAMPLE swresample ${PROJECT_SOURCE_DIR}/libs/ffmpeg/macos/${CMAKE_SYSTEM_PROCESSOR} NO_DEFAULT_PATH)
    endif()
elseif(ANDROID)
    find_library(AVFORMAT2 avformat ${PROJECT_SOURCE_DIR}/libs/ffmpeg/android/${ANDROID_ABI} NO_DEFAULT_PATH)
    find_library(AVUTIL avutil ${PROJECT_SOURCE_DIR}/libs/ffmpeg/android/${ANDROID_ABI} NO_DEFAULT_PATH)
    find_library(AVCODEC avcodec ${PROJECT_SOURCE_DIR}/libs/ffmpeg/android/${ANDROID_ABI} NO_DEFAULT_PATH)
    find_library(SWSCALE swscale ${PROJECT_SOURCE_DIR}/libs/ffmpeg/android/${ANDROID_ABI} NO_DEFAULT_PATH)
    find_library(SWRESAMPLE swresample ${PROJECT_SOURCE_DIR}/libs/ffmpeg/android/${ANDROID_ABI} NO_DEFAULT_PATH)

    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-Bsymbolic" )
    install(TARGETS ${CMAKE_PROJECT_NAME})
    install(FILES ${public_headers} DESTINATION include)
endif()

# The Target Properties

if(IOS)
    set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY FRAMEWORK TRUE)
    # set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY MACOSX_BUNDLE_GUI_IDENTIFIER "com.vxmt.vcamshare")
endif()

set (CMAKE_CXX_STANDARD 11)
set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_STANDARD 11)
set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY PUBLIC_HEADER "${public_headers}")



# include folders
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/boost/include/"
    "${PROJECT_SOURCE_DIR}/libs/FFmpeg/include/"
)

# link for the main lib
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE 
                        ${AVFORMAT2}
                        ${AVUTIL}
                        ${SWSCALE}
                        ${SWRESAMPLE}
                        ${AVCODEC}
                        )


# Test executable ======================================

if(APPLE)
if(NOT IOS)

set(test_sources
    test_video_muxer.cpp
)

list(TRANSFORM test_sources PREPEND "src/test/")

add_executable(test ${test_sources})
set_property(TARGET test PROPERTY CXX_STANDARD 11)

target_include_directories(test PUBLIC
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}/libs/boost/include/"
    "${PROJECT_SOURCE_DIR}/libs/FFmpeg/include/"
)

target_link_libraries(test PRIVATE ${CMAKE_PROJECT_NAME})

# Copy Test Files ==============
file(COPY ${PROJECT_SOURCE_DIR}/src/test/mt.h264 DESTINATION ${PROJECT_BINARY_DIR})
file(COPY ${PROJECT_SOURCE_DIR}/src/test/mx_local.h264 DESTINATION ${PROJECT_BINARY_DIR})
file(COPY ${PROJECT_SOURCE_DIR}/src/test/drain.h264 DESTINATION ${PROJECT_BINARY_DIR})
file(COPY ${PROJECT_SOURCE_DIR}/src/test/hdpro.h264 DESTINATION ${PROJECT_BINARY_DIR})
file(COPY ${PROJECT_SOURCE_DIR}/src/test/android_audio.raw DESTINATION ${PROJECT_BINARY_DIR})


endif()
endif()
