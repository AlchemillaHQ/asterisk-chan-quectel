#!/bin/bash -e

echoerr() { echo "$@" 1>&2; }

# apt install dpkg-dev
readonly JQ_DEB_SCRIPT='
{
    version: 3, cmakeMinimumRequired: {major: 3,minor: 22, patch: 1},
    configurePresets: [
        {
            name: "deb-internal",
            hidden: true,
            cacheVariables: {
                "BUILD_TESTING": {
                  type: "BOOL",
                  value: true
                },
                "CHECK_SOURCE_DATE_EPOCH": {
                    "type": "BOOL",
                    "value": true
                }
            },
            environment: {
                CFLAGS: (if env.CPPFLAGS then ([ env.CFLAGS, env.CPPFLAGS] | join(" ")) else env.CFLAGS // "" end),
                CXXFLAGS: (if env.CPPFLAGS then ([ env.CXXFLAGS, env.CPPFLAGS] | join(" ")) else env.CXXFLAGS // "" end),
                LDFLAGS: env.LDFLAGS
            }
        },
        {
            name: "deb",
            inherits: ["deb-internal", "default"]
        }
    ],
    buildPresets: [
        {
            name: "deb",
            inherits: "default",
            configurePreset: "deb"
        },
        {
            name: "package-deb",
            configurePreset: "deb",
            jobs: 0,
            cleanFirst: true,
            targets: "package"
        }
    ],
    testPresets: [
        {
            name: "deb",
            inherits: "default",
            configurePreset: "deb"
        }
    ]
}'

readonly JQ_RPI_SCRIPT='
{
    version: 3, cmakeMinimumRequired: {major: 3,minor: 22, patch: 1},
    configurePresets: [
        {
            name: "rpi-internal",
            hidden: true,
            cacheVariables: {
                "BUILD_TESTING": {
                  type: "BOOL",
                  value: true
                },                
                "CHECK_SOURCE_DATE_EPOCH": {
                    "type": "BOOL",
                    "value": true
                },
                "CPACK_DEBIAN_PACKAGE_SHLIBDEPS": {
                    type: "BOOL",
                    value: true
                }
            },
            environment: {
                CFLAGS: (if env.CPPFLAGS then ([ env.CFLAGS, env.CPPFLAGS] | join(" ")) else env.CFLAGS // "" end),
                CXXFLAGS: (if env.CPPFLAGS then ([ env.CXXFLAGS, env.CPPFLAGS] | join(" ")) else env.CXXFLAGS // "" end),
                LDFLAGS: env.LDFLAGS
            }
        },
        {
            name: "rpi",
            inherits: ["rpi-internal", "default"]
        }
    ],
    buildPresets: [
        {
            name: "rpi",
            inherits: "default",
            configurePreset: "rpi"
        },
        {
            name: "package-rpi",
            configurePreset: "rpi",
            jobs: 0,
            cleanFirst: true,
            targets: "package"
        }
    ],
    testPresets: [
        {
            name: "rpi",
            inherits: "default",
            configurePreset: "rpi"
        }
  ]
}'

# yum install rpm-build
readonly JQ_RPM_SCRIPT='
{
    version: 3, cmakeMinimumRequired: {major: 3,minor: 22, patch: 1},
    configurePresets: [
        {
            name: "rpm-internal",
            hidden: true,
            cacheVariables: {
                "BUILD_TESTING": {
                  type: "BOOL",
                  value: true
                },
                "CHECK_SOURCE_DATE_EPOCH": {
                    "type": "BOOL",
                    "value": true
                },
                "CPACK_GENERATOR": {
                    type: "STRING",
                    value: "TGZ;7Z"
                }
            },
            environment: {
                CFLAGS: (env.CFLAGS // ""),
                CXXFLAGS: (env.CFLAGS // ""),
                LDFLAGS: (env.LDFLAGS // "")
            }
        },
        {
            name: "rpm",
            inherits: ["rpm-internal", "default"]
        }
    ],
    buildPresets: [
        {
            name: "rpm",
            inherits: "default",
            configurePreset: "rpm"
        },
        {
            name: "package-rpm",
            configurePreset: "rpm",
            jobs: 0,
            cleanFirst: true,
            targets: "package"
        }
    ],
    testPresets: [
        {
            name: "rpm",
            inherits: "default",
            configurePreset: "rpm"
        }
  ]
}'

case $1 in
    deb)
    eval "$(env 'DEB_BUILD_OPTIONS=reproducible=-fixfilepath,-fixdebugpath' dpkg-buildflags --export=sh)"
    jq -n "$JQ_DEB_SCRIPT"
    ;;

    deb-env)
    jq -n "$JQ_DEB_SCRIPT"
    ;;    

    rpi)
    eval "$(env 'DEB_BUILD_OPTIONS=reproducible=-fixfilepath,-fixdebugpath' dpkg-buildflags --export=sh)"
    jq -n "$JQ_RPI_SCRIPT"    
    ;;

    rpi-env)
    jq -n "$JQ_RPI_SCRIPT"    
    ;;    

    rpm)
    env "CFLAGS=$(rpm -E '%optflags')" "CXXFLAGS=$(rpm -E '%optflags')" "LDFLAGS=$(rpm -E '%__global_ldflags')" jq -n "$JQ_RPM_SCRIPT"
    ;;

    rpm-env)
    jq -n "$JQ_RPM_SCRIPT"
    ;;    

    *)
    echoerr Unknown package manager
    exit 1;;
esac
