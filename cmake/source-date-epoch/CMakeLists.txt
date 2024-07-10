#
# Check existience of SOURCE_DATE_EPOCH environment variable
#

FUNCTION(CheckSourceDateEpoch)
    IF(DEFINED ENV{SOURCE_DATE_EPOCH})
        STRING(TIMESTAMP SOURCE_DATE_STR UTC)
        MESSAGE(STATUS "SOURCE_DATE_EPOCH=$ENV{SOURCE_DATE_EPOCH} # ${SOURCE_DATE_STR}")
        CONFIGURE_FILE(source-date-epoch.env.in source-date-epoch.env NO_SOURCE_PERMISSIONS)
    ELSE()
        MESSAGE(FATAL_ERROR "Environment variable SOURCE_DATE_EPOCH not defined")
    ENDIF()
ENDFUNCTION()

CheckSourceDateEpoch()

ADD_CUSTOM_TARGET(check-source-date-epoch
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/check-source-date-epoch.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    USES_TERMINAL
    SOURCES check-source-date-epoch.cmake
    COMMENT "Checking SOURCE_DATE_EPOCH environment variable"
)
