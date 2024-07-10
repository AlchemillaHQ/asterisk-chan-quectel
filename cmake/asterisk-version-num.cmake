#
# asterisk-version-num
#

FUNCTION(GetAsteriskVersionFromPkgConfig OutVar)
    FIND_PACKAGE(PkgConfig)
    IF(NOT PKG_CONFIG_FOUND)
        SET("${OutVar}" PARENT_SCOPE)
        RETURN()
    ENDIF()

    MESSAGE(DEBUG "Obtaining Asterisk version from pkg-info")
    pkg_check_modules(AST QUIET asterisk)
    IF(NOT AST_FOUND)
        SET("${OutVar}" PARENT_SCOPE)
        RETURN()    
    ENDIF()

    MESSAGE(DEBUG "pkg-info: asterisk module found")
    pkg_get_variable(ASTERISK_VERSION_NUM asterisk version_number)
    MESSAGE(VERBOSE "Asterisk version (from pkg-info): ${ASTERISK_VERSION_NUM}")
    SET("${OutVar}" "${ASTERISK_VERSION_NUM}" PARENT_SCOPE)
ENDFUNCTION()

FUNCTION(CheckAsteriskVersion)
    SET(VER_SOURCE cached)
    
    IF(NOT ASTERISK_VERSION_NUM)
        GetAsteriskVersionFromPkgConfig(AST_VERSION_NUM)
        IF(AST_VERSION_NUM)
            SET(ASTERISK_VERSION_NUM "${AST_VERSION_NUM}" CACHE STRING "Asterisk version" FORCE)
            SET(VER_SOURCE pkg-config)
        ENDIF()
    ENDIF()

    IF(NOT ASTERISK_VERSION_NUM)
        SET(ASTERISK_VERSION_NUM 180000 CACHE STRING "Asterisk version" FORCE)
        SET(VER_SOURCE default)
    ENDIF()

    MESSAGE(STATUS "Asterisk version: ${ASTERISK_VERSION_NUM} [${VER_SOURCE}]")
ENDFUNCTION()
