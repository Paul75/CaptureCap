if(NOT DEFINED NDK_ROOT)
    message(FATAL_ERROR "NDK_ROOT must be defined!")
endif()

if(CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
  set(HOST_TAG "linux-x86_64")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL Darwin)
  set(HOST_TAG "darwin-x86_64")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
  set(HOST_TAG "windows-x86_64")
  set(WIN_EXT ".exe")
endif()

set(NDK_OBJCOPY "${NDK_ROOT}/toolchains/llvm/prebuilt/${HOST_TAG}/bin/llvm-objcopy${WIN_EXT}")

if(NOT EXISTS "${NDK_OBJCOPY}")
    message(WARNING "llvm-objcopy not found at ${NDK_OBJCOPY}. Fallback to system's objcopy")
    set(NDK_OBJCOPY "objcopy")
endif()