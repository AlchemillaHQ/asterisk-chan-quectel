#
# CPack - post build script - lintian
#
# Links:
#   - https://manpages.debian.org/bookworm/lintian/lintian.1.en.html
#   - https://manpages.debian.org/bookworm/lintian/lintian-explain-tags.1.en.html
#   - https://decovar.dev/blog/2021/09/23/cmake-cpack-package-deb-apt/
#

CMAKE_MINIMUM_REQUIRED(VERSION 3.20)

FUNCTION(CountDebPkgs)
    SET(CNT 0)
    FOREACH(p IN LISTS ARGV)
        CMAKE_PATH(GET p EXTENSION LAST_ONLY pext)
        IF(NOT ${pext} STREQUAL .deb)
            CONTINUE()
        ENDIF()
        MATH(EXPR CNT "${CNT}+1")
    ENDFOREACH()
    SET(DEBCNT ${CNT} PARENT_SCOPE)
ENDFUNCTION()

CountDebPkgs(${CPACK_PACKAGE_FILES})
IF(${DEBCNT} LESS_EQUAL 0)
    RETURN()
ENDIF()

FIND_PROGRAM(
    LINTIAN_SCRIPT
    NAME lintian
    NO_CACHE
    REQUIRED
)

EXECUTE_PROCESS(
    COMMAND ${LINTIAN_SCRIPT} --version
    OUTPUT_VARIABLE LINTIAN_VERSION_OUT
    OUTPUT_STRIP_TRAILING_WHITESPACE    
    COMMAND_ERROR_IS_FATAL ANY
)

IF(NOT "${LINTIAN_VERSION_OUT}" MATCHES "^Lintian v(.+)$")
    MESSAGE(FATAL_ERROR "[lintian] Unable to determine version from [${LINTAN_VERSION_OUT}]")
ENDIF()

SET(LINTAN_VERSION "${CMAKE_MATCH_1}")
IF("${LINTAN_VERSION}" VERSION_LESS_EQUAL 2.62)
    MESSAGE(WARNING "[lintian] Unsupported version: ${LINTAN_VERSION}")
    RETURN()
ENDIF()
MESSAGE(STATUS "[lintian] Version: ${LINTAN_VERSION}")

LIST(GET CPACK_BUILD_SOURCE_DIRS 0 PROJECT_DIR)
CMAKE_PATH(APPEND PROJECT_DIR cpack suppressed-tags.txt OUTPUT_VARIABLE TAGS_FILE)
MESSAGE(STATUS "[lintian] Tags: ${TAGS_FILE}")

SET(LINTIAN_ARGS --no-user-dirs --no-cfg
    --fail-on error,warning
    -q -i
    --color never
    --suppress-tags-from-file ${TAGS_FILE}
)

FOREACH(p IN LISTS CPACK_PACKAGE_FILES)
    CMAKE_PATH(GET p EXTENSION LAST_ONLY pext)
    IF(NOT ${pext} STREQUAL .deb)
        CONTINUE()
    ENDIF()

    MESSAGE(STATUS "[lintian] Package: ${p}")
    EXECUTE_PROCESS(
        COMMAND ${LINTIAN_SCRIPT} ${LINTIAN_ARGS} ${p}
        TIMEOUT 300
        COMMAND_ERROR_IS_FATAL ANY
    )
ENDFOREACH()
