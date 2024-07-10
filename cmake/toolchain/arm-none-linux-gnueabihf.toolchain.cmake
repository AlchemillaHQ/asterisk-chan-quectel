#
# arm-none-linux-gnueabihf.toolchain.cmake
#
# Download arm-none-linux-gnueabihf toolchain from
# https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
# and extract to /build/arm-gnu-toolchain/
#
set(CMAKE_SYSTEM_NAME               Linux)
set(CMAKE_SYSTEM_PROCESSOR          arm)

set(triple                          arm-none-linux-gnueabihf)
set(btriple                         arm-linux-gnueabihf)
set(gccbase                          /build/arm-gnu-toolchain/${triple})

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

function(internal_set_rpi_cxx_init_flags)
    set_cxx_init_flags(${ARGV} -mtls-dialect=gnu)
endfunction()

function(set_rpi_cxx_init_flags rpi)
    if (${rpi} EQUAL 1)
        internal_set_rpi_cxx_init_flags(-marm -march=armv6+fp -mfpu=vfp -mfloat-abi=hard -mtune=arm1176jzf-s)
    elseif(${rpi} EQUAL 2)
        internal_set_rpi_cxx_init_flags(-march=armv7-a+fp+neon-vfpv4 -mfloat-abi=hard -mtune=cortex-a7)
    elseif(${rpi} EQUAL 3)
        internal_set_rpi_cxx_init_flags(-march=armv8-a+crc -mfloat-abi=hard -mfpu=crypto-neon-fp-armv8 -mtune=cortex-a53)
    elseif(${rpi} EQUAL 4)
        internal_set_rpi_cxx_init_flags(-march=armv8-a+crc -mfloat-abi=hard -mfpu=crypto-neon-fp-armv8 -mtune=cortex-a72)
    elseif(${rpi} EQUAL 5)
        internal_set_rpi_cxx_init_flags(-march=armv8-a+crc+crypto -mfloat-abi=hard -mfpu=crypto-neon-fp-armv8 -mtune=cortex-a76)
    elseif(${rpi} GREATER 5)
        message(FATAL_ERROR "Version ${rpi} of Raspberry Pi is not supported.")
    else()
        message(FATAL_ERROR "Wrong Raspberry Pi version specified: ${rpi}.")
    endif()
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

function(set_sysroot_cxx_standard_include_directories)
    list(TRANSFORM ARGV PREPEND "=" OUTPUT_VARIABLE incs)
    set_cxx_standard_include_directories(${incs})
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

function(init_sysroot_linker_search_paths)
    list(TRANSFORM ARGV PREPEND "-L=" OUTPUT_VARIABLE lopts)
    set_linker_init_flags(${lopts})
endfunction()

if(DEFINED ENV{TOOLSET_TARGET_RPI})
    set_rpi_cxx_init_flags($ENV{TOOLSET_TARGET_RPI})

    set(sysroot /build/sysroot)
    set(CMAKE_SYSROOT ${sysroot})
    set(CMAKE_STAGING_PREFIX ${sysroot})

    # paths relative to SYSROOT
    set_sysroot_cxx_standard_include_directories(
        /usr/local/include/${btriple}
        /usr/local/include
        /usr/include/${btriple}
        /usr/include
        /include/${btriple}
        /include
    )
    init_sysroot_linker_search_paths(
        /usr/local/lib
        /usr/lib
        /usr/lib/gcc
        /lib
    )
    set(CMAKE_CROSSCOMPILING_EMULATOR /usr/bin/qemu-arm-static -L ${sysroot} CACHE INTERNAL "")
else()
    set_cxx_init_flags(-march=armv7-a+fp+neon -mfloat-abi=hard -Wl,--trace)

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
    set(CMAKE_CROSSCOMPILING_EMULATOR /usr/bin/qemu-arm-static -L /usr/lib/${btriple} CACHE INTERNAL "")
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CPACK_PACKAGE_ARCHITECTURE armhf)
