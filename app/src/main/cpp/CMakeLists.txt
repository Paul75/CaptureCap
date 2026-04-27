cmake_minimum_required(VERSION 3.18.1)
project("rtmp_encoder")

include(${CMAKE_CURRENT_SOURCE_DIR}/librtmp.cmake)

add_library(rtmp-jni SHARED
    native_rtmp.cpp
)

find_library(log-lib log)

target_link_libraries(rtmp-jni
    rtmp
    ${log-lib}
    android
    jnigraphics)
