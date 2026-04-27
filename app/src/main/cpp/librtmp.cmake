include(FetchContent)

option(RTMP_USE_OPENSSL "Use OpenSSL for crypto" ON)
option(RTMP_USE_GNUTLS   "Use GnuTLS for crypto" OFF)
option(RTMP_USE_NO_CRYPTO "Disable crypto" OFF)
option(RTMP_BUILD_SHARED "Build librtmp as shared library" ON)

if (RTMP_USE_GNUTLS AND RTMP_USE_OPENSSL)
  message(FATAL_ERROR "Only one of RTMP_USE_OPENSSL or RTMP_USE_GNUTLS can be enabled")
endif()

FetchContent_Declare(
  android_openssl
  DOWNLOAD_EXTRACT_TIMESTAMP true
  URL https://github.com/KDAB/android_openssl/archive/refs/heads/master.zip
)
FetchContent_MakeAvailable(android_openssl)
include(${android_openssl_SOURCE_DIR}/android_openssl.cmake)

FetchContent_Declare(
  ZLIB
  GIT_REPOSITORY https://github.com/madler/zlib.git
  GIT_TAG        v1.3.2
)

FetchContent_Declare(
  librtmp
  GIT_REPOSITORY https://git.ffmpeg.org/rtmpdump.git
  GIT_TAG        v2.6
)

FetchContent_MakeAvailable(ZLIB librtmp)

FetchContent_GetProperties(librtmp)
set(LIBRTMP_SRC_DIR ${librtmp_SOURCE_DIR}/librtmp)

set(LIBRTMP_SOURCES
  ${LIBRTMP_SRC_DIR}/rtmp.c
  ${LIBRTMP_SRC_DIR}/log.c
  ${LIBRTMP_SRC_DIR}/amf.c
  ${LIBRTMP_SRC_DIR}/hashswf.c
  ${LIBRTMP_SRC_DIR}/parseurl.c
)

set(LIBRTMP_DEFS "")
set(LIBRTMP_LINK_LIBS "")

if (RTMP_USE_OPENSSL)
  list(APPEND LIBRTMP_DEFS -DUSE_OPENSSL)
  list(APPEND LIBRTMP_LINK_LIBS OpenSSL::SSL OpenSSL::Crypto ZLIB::ZLIB)
elseif (RTMP_USE_GNUTLS)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GNUTLS REQUIRED gnutls)
  list(APPEND LIBRTMP_DEFS -DUSE_GNUTLS)
  list(APPEND LIBRTMP_LINK_LIBS ${GNUTLS_LIBRARIES})
elseif (RTMP_USE_NO_CRYPTO)
  list(APPEND LIBRTMP_DEFS -DNO_CRYPTO)
else()
  message(FATAL_ERROR "No crypto backend specified (RTMP_USE_OPENSSL/RTMP_USE_GNUTLS/RTMP_USE_NO_CRYPTO)")
endif()

if (RTMP_BUILD_SHARED)
  add_library(rtmp SHARED ${LIBRTMP_SOURCES})
  set_target_properties(rtmp PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    SOVERSION 1
    VERSION 1.0.0
)
else()
  add_library(rtmp STATIC ${LIBRTMP_SOURCES})
endif()

target_include_directories(rtmp
  PUBLIC
    $<BUILD_INTERFACE:${LIBRTMP_SRC_DIR}>
    $<INSTALL_INTERFACE:include/librtmp>
  PRIVATE
    ${LIBRTMP_SRC_DIR}
    ${android_openssl_SOURCE_DIR}/include
    ${ZLIB_SOURCE_DIR}
)

target_compile_definitions(rtmp PRIVATE ${LIBRTMP_DEFS})

target_link_libraries(rtmp PRIVATE ${LIBRTMP_LINK_LIBS})

install(TARGETS rtmp
  EXPORT rtmpTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include/librtmp
)

install(FILES
  ${LIBRTMP_SRC_DIR}/amf.h
  ${LIBRTMP_SRC_DIR}/http.h
  ${LIBRTMP_SRC_DIR}/log.h
  ${LIBRTMP_SRC_DIR}/rtmp.h
  DESTINATION include/librtmp
)