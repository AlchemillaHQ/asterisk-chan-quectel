#
# aarch64-linux-gnu.toolchain.cmake
#
# Required packages:
#   gcc-aarch64-linux-gnu
#   g++-aarch64-linux-gnu
#   binutils-aarch64-linux-gnu
#
set(CMAKE_SYSTEM_NAME               Linux)
set(CMAKE_SYSTEM_PROCESSOR          aarch64)

set(triple                          aarch64-linux-gnu)

set(CMAKE_AR                        ${triple}-ar${CMAKE_EXECUTABLE_SUFFIX})
set(CMAKE_ASM_COMPILER              ${triple}-gcc${CMAKE_EXECUTABLE_SUFFIX})
set(CMAKE_C_COMPILER                ${triple}-gcc${CMAKE_EXECUTABLE_SUFFIX})
set(CMAKE_C_COMPILER_TARGET         ${triple})
set(CMAKE_CXX_COMPILER              ${triple}-g++${CMAKE_EXECUTABLE_SUFFIX})
set(CMAKE_CXX_COMPILER_TARGET       ${triple})

set(CMAKE_C_FLAGS                   "-march=armv8-a" CACHE INTERNAL "")
set(CMAKE_CXX_FLAGS                 "-march=armv8-a" CACHE INTERNAL "")
set(CMAKE_CROSSCOMPILING_EMULATOR /usr/bin/qemu-aarch64-static -L /usr/lib/${triple} CACHE INTERNAL "")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CPACK_PACKAGE_ARCHITECTURE arm64)
