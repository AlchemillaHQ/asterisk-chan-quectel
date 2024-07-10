#
# CPack - post build script - package info
#
# Links:
#   - https://manpages.debian.org/bookworm/dpkg/dpkg-deb.1.en.html
#

CMAKE_MINIMUM_REQUIRED(VERSION 3.20)

FOREACH(p IN LISTS CPACK_PACKAGE_FILES)
    CMAKE_PATH(GET p EXTENSION LAST_ONLY pext)
    IF(NOT ${pext} STREQUAL .deb)
        CONTINUE()
    ENDIF()

    MESSAGE(STATUS "[deb-info] Package: ${p}")
    EXECUTE_PROCESS(
        COMMAND dpkg-deb -I ${p}
        TIMEOUT 300
        COMMAND_ERROR_IS_FATAL ANY
    )
    EXECUTE_PROCESS(
        COMMAND dpkg-deb -c ${p}
        TIMEOUT 300
        COMMAND_ERROR_IS_FATAL ANY
    )
ENDFOREACH()
