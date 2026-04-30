cmake_minimum_required(VERSION 3.18.1)
project("rtmp_encoder")

add_link_options("LINKER:--build-id=none")

include(${CMAKE_CURRENT_SOURCE_DIR}/ndk-objcopy.cmake)

include(${CMAKE_CURRENT_SOURCE_DIR}/librtmp.cmake)

add_library(rtmp-jni SHARED
    native_rtmp.cpp
)

add_custom_command(TARGET rtmp-jni POST_BUILD
    COMMAND ${NDK_OBJCOPY} --remove-section=.comment $<TARGET_FILE:rtmp-jni>
)

find_library(log-lib log)

target_link_libraries(rtmp-jni
    rtmp
    ${log-lib}
    android)
