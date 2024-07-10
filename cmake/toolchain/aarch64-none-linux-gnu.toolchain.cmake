#
# aarch64-none-linux-gnu.toolchain.cmake
#
# Download aarch64-none-linux-gnu toolchain from
# https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
# and extract to /build/arm-gnu-toolchain/
#
set(CMAKE_SYSTEM_NAME               Linux)
set(CMAKE_SYSTEM_PROCESSOR          aarch64)

set(triple                          aarch64-none-linux-gnu)
set(btriple                         aarch64-linux-gnu)
set(gccbase                         /build/arm-gnu-toolchain/${triple})

set(CMAKE_AR                        ${gccbase}/bin/${triple}-ar${CMAKE_EXECUTABLE_SUFFIX})
set(CMAKE_ASM_COMPILER              ${gccbase}/bin/${triple}-gcc${CMAKE_EXECUTABLE_SUFFIX})
set(CMAKE_C_COMPILER                ${gccbase}/bin/${triple}-gcc${CMAKE_EXECUTABLE_SUFFIX})
set(CMAKE_C_COMPILER_TARGET         ${triple})
set(CMAKE_C_LIBRARY_ARCHITECTURE    ${btriple})
set(CMAKE_CXX_COMPILER              ${gccbase}/bin/${triple}-g++${CMAKE_EXECUTABLE_SUFFIX})
set(CMAKE_CXX_COMPILER_TARGET       ${triple})
set(CMAKE_CXX_LIBRARY_ARCHITECTURE  ${btriple})

function(set_cxx_init_flags)
    list(JOIN ARGV " " C_FLAGS_INIT)
    set(CMAKE_C_FLAGS_INIT "${C_FLAGS_INIT}" CACHE INTERNAL "")
    set(CMAKE_CXX_FLAGS_INIT "${C_FLAGS_INIT}" CACHE INTERNAL "")
endfunction()

function(set_cxx_standard_include_directories)
    foreach(lang C CXX)
        SET("CMAKE_${lang}_STANDARD_INCLUDE_DIRECTORIES" ${ARGV} CACHE INTERNAL "")
    endforeach()

    set(cflags)
    foreach(inc ${ARGV})
        list(APPEND cflags "-isystem")
        list(APPEND cflags ${inc})
    endforeach()
    string(JOIN " " C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT}" ${cflags})
    string(JOIN " " CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT}" ${cflags})
    set(CMAKE_C_FLAGS_INIT "${C_FLAGS_INIT}" CACHE INTERNAL "")
    set(CMAKE_CXX_FLAGS_INIT "${CXX_FLAGS_INIT}" CACHE INTERNAL "")
endfunction()

function(set_linker_init_flags)
    list(JOIN ARGV " " LINKER_FLAGS_INIT)
    foreach(target SHARED STATIC MODULE EXE)
        set("CMAKE_${target}_LINKER_FLAGS_INIT" "${LINKER_FLAGS_INIT}" CACHE INTERNAL "")
    endforeach()
endfunction()

function(init_linker_search_paths)
    list(TRANSFORM ARGV PREPEND "-L" OUTPUT_VARIABLE lopts)
    set_linker_init_flags(${lopts})
endfunction()

set_cxx_init_flags(-march=armv8-a -mabi=lp64)
set_cxx_standard_include_directories(
    /usr/${btriple}/include
    /usr/local/include/${btriple}
    /usr/local/include
    /usr/include/${btriple}
    /usr/include
)
file(GLOB GCC_CROSS LIST_DIRECTORIES true /usr/lib/gcc-cross/${btriple}/*)
init_linker_search_paths(
    /usr/${btriple}/lib
    ${GCC_CROSS}
    /usr/local/lib/${btriple}
    /usr/local/lib
    /usr/lib/${btriple}
    /usr/lib
    /lib/${btriple}
    /lib
)
set(CMAKE_CROSSCOMPILING_EMULATOR /usr/bin/qemu-aarch64-static -L /usr/lib/${btriple} CACHE INTERNAL "")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CPACK_PACKAGE_ARCHITECTURE arm64)

