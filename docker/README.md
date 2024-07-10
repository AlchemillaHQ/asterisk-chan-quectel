# Building `chan_quectel` using *Docker*

In this directory you can find examples of building `chan_quectel` for various operating system using *Docker*.

Building is performed in two steps:

- Building *Docker* image with full *development environment*.

  Here all required tools (compiler, etc.) and files (libraries, header files, etc.) are collected (compiled).

- Building `chan_quectel` itself using previously creaded *development environment*.

> [!CAUTION]
> Building of *development environment* may take a long time.

## Supported operating systems

| Operating system | Versions | Archiectures |
| :--------------- | :------- | :----------- |
| Debain | 10, 11, 12 | amd64, armhf, arm64 |
| Ubuntu | 20.04, 22.04, 24.04 | amd64, armhf, arm64 |
| CentOS | 7.8 | amd64 |
| Raspberry Pi OS (Raspbian) | 10, 11, 12 | armhf |

## Usage

[(go-)task](http://taskfile.dev/) must be installed on your system.

On `docker` directory type:

```sh
task -l
```

to see all available options:

```
* clean:                        Delete build directories
* native:                       Build chan_quectel                    (aliases: default)
* centos:7.8:clean:             Remove package directory              (aliases: centos:freepbx:clean)
* centos:7.8:native:            Build chan_quectel on CentOS 7.8      (aliases: centos:7.8:default, centos:freepbx:native, centos:freepbx:default)
* centos:clean:                 Remove package directories
* centos:native:                Build chan_quectel on CentOS                                (aliases: centos:default)
* debian:10:arm64:              Build chan_quectel for ARM64 on Debian Buster               (aliases: debian:buster:arm64)
* debian:10:armhf:              Build chan_quectel for ARM on Debian Buster                 (aliases: debian:buster:armhf)
* debian:10:clean:              Remove package directory                                    (aliases: debian:buster:clean)
* debian:10:native:             Build chan_quectel on Debian Buster                         (aliases: debian:10:default, debian:buster:native, debian:buster:default)
* debian:10:rpi*:               Build chan_quectel for Raspberry Pi on Debian Buster        (aliases: debian:buster:rpi*)
* debian:11:arm64:              Build chan_quectel for ARM64 on Debian Bullseye             (aliases: debian:bullseye:arm64)
* debian:11:armhf:              Build chan_quectel for ARM on Debian Bullseye               (aliases: debian:bullseye:armhf)
* debian:11:clean:              Remove package directory                                    (aliases: debian:bullseye:clean)
* debian:11:native:             Build chan_quectel on Debian Bullseye                       (aliases: debian:11:default, debian:bullseye:native, debian:bullseye:default)
* debian:11:rpi*:               Build chan_quectel for Raspberry Pi on Debian Bullseye      (aliases: debian:bullseye:rpi*)
* debian:12:arm64:              Build chan_quectel for ARM64 on Debian Bookworm             (aliases: debian:bookworm:arm64)
* debian:12:armhf:              Build chan_quectel for ARM on Debian Bookworm               (aliases: debian:bookworm:armhf)
* debian:12:clean:              Remove package directory                                    (aliases: debian:bookworm:clean)
* debian:12:native:             Build chan_quectel on Debian Bookworm                       (aliases: debian:12:default, debian:bookworm:native, debian:bookworm:default)
* debian:12:rpi*:               Build chan_quectel for Raspberry Pi on Debian Bookworm      (aliases: debian:bookworm:rpi*)
* debian:arm64:                 Build chan_quectel for ARM64 on Debian
* debian:armhf:                 Build chan_quectel for ARM on Debian
* debian:clean:                 Remove package directories
* debian:native:                Build chan_quectel on Debian      (aliases: debian:default)
* debian:rpi*:                  Build chan_quectel for Raspberry Pi on Debian
* ext:debian:10:arm64:          Build chan_quectel for ARM64 on Debian Buster using ARM GNU Toolchain             (aliases: ext:debian:buster:arm64, arm-gnu-toolchain:debian:10:arm64, arm-gnu-toolchain:debian:buster:arm64)
* ext:debian:10:armhf:          Build chan_quectel for ARM on Debian Buster using ARM GNU Toolchain               (aliases: ext:debian:buster:armhf, arm-gnu-toolchain:debian:10:armhf, arm-gnu-toolchain:debian:buster:armhf)
* ext:debian:10:rpi*:           Build chan_quectel for Raspberry Pi on Debian Buster using ARM GNU Toolchain      (aliases: ext:debian:buster:rpi*, arm-gnu-toolchain:debian:10:rpi*, arm-gnu-toolchain:debian:buster:rpi*)
* ext:debian:11:arm64:          Build chan_quectel for ARM64 on Debian Bullseye using GNU Toolchain               (aliases: ext:debian:bullseye:arm64, arm-gnu-toolchain:debian:11:arm64, arm-gnu-toolchain:debian:bullseye:arm64)
* ext:debian:11:armhf:          Build chan_quectel for ARM on Debian Bullseye using GNU Toolchain                 (aliases: ext:debian:bullseye:armhf, arm-gnu-toolchain:debian:11:armhf, arm-gnu-toolchain:debian:bullseye:armhf)
* ext:debian:11:rpi*:           Build chan_quectel for Raspberry Pi on Debian Bullseye using GNU Toolchain        (aliases: ext:debian:bullseye:rpi*, arm-gnu-toolchain:debian:11:rpi*, arm-gnu-toolchain:debian:bullseye:rpi*)
* ext:debian:12:arm64:          Build chan_quectel for ARM64 on Debian Bookworm using GNU Toolchain               (aliases: ext:debian:bookworm:arm64, arm-gnu-toolchain:debian:12:arm64, arm-gnu-toolchain:debian:bookworm:arm64)
* ext:debian:12:armhf:          Build chan_quectel for ARM on Debian Bookworm using GNU Toolchain                 (aliases: ext:debian:bookworm:armhf, arm-gnu-toolchain:debian:12:armhf, arm-gnu-toolchain:debian:bookworm:armhf)
* ext:debian:12:rpi*:           Build chan_quectel for Raspberry Pi on Debian Bookworm using GNU Toolchain        (aliases: ext:debian:bookworm:rpi*, arm-gnu-toolchain:debian:12:rpi*, arm-gnu-toolchain:debian:bookworm:rpi*)
* ext:debian:arm64:             Build chan_quectel for ARM64 on Debian using GNU Toolchain                        (aliases: arm-gnu-toolchain:debian:arm64)
* ext:debian:armhf:             Build chan_quectel for ARM on Debian using GNU Toolchain                          (aliases: arm-gnu-toolchain:debian:armhf)
* ext:debian:rpi*:              Build chan_quectel for Raspberry Pi on Debian using GNU Toolchain                 (aliases: arm-gnu-toolchain:debian:rpi*)
* ext:ubuntu:20.04:arm64:       Build chan_quectel for ARM64 on Ubuntu 20.04 using GNU Toolchain                  (aliases: ext:ubuntu:focal:arm64, ext:ubuntu:focal-fossa:arm64, arm-gnu-toolchain:ubuntu:20.04:arm64, arm-gnu-toolchain:ubuntu:focal:arm64, arm-gnu-toolchain:ubuntu:focal-fossa:arm64)
* ext:ubuntu:20.04:armhf:       Build chan_quectel for ARM on Ubuntu 20.04 using GNU Toolchain                    (aliases: ext:ubuntu:focal:armhf, ext:ubuntu:focal-fossa:armhf, arm-gnu-toolchain:ubuntu:20.04:armhf, arm-gnu-toolchain:ubuntu:focal:armhf, arm-gnu-toolchain:ubuntu:focal-fossa:armhf)
* ext:ubuntu:22.04:arm64:       Build chan_quectel for ARM64 on Ubuntu 22.04 using GNU Toolchain                  (aliases: arm-gnu-toolchain:ubuntu:22.04:arm64)
* ext:ubuntu:22.04:armhf:       Build chan_quectel for ARM on Ubuntu 22.04 using GNU Toolchain                    (aliases: arm-gnu-toolchain:ubuntu:22.04:armhf)
* ext:ubuntu:24.04:arm64:       Build chan_quectel for ARM64 on Ubuntu 24.04 using GNU Toolchain                  (aliases: ext:ubuntu:noble:arm64, ext:ubuntu:noble-numbat:arm64, arm-gnu-toolchain:ubuntu:24.04:arm64, arm-gnu-toolchain:ubuntu:noble:arm64, arm-gnu-toolchain:ubuntu:noble-numbat:arm64)
* ext:ubuntu:24.04:armhf:       Build chan_quectel for ARM on Ubuntu 24.04 using GNU Toolchain                    (aliases: ext:ubuntu:noble:armhf, ext:ubuntu:noble-numbat:armhf, arm-gnu-toolchain:ubuntu:24.04:armhf, arm-gnu-toolchain:ubuntu:noble:armhf, arm-gnu-toolchain:ubuntu:noble-numbat:armhf)
* ext:ubuntu:arm64:             Build chan_quectel for ARM64 on Ubuntu using GNU Toolchain                        (aliases: arm-gnu-toolchain:ubuntu:arm64)
* ext:ubuntu:armhf:             Build chan_quectel for ARM on Ubuntu using GNU Toolchain                          (aliases: arm-gnu-toolchain:ubuntu:armhf)
* rpi-debian:10:clean:          Remove package directory                                                          (aliases: rpi-debian:buster:clean)
* rpi-debian:10:native:         Build chan_quectel on Raspberry Pi OS (Buster)                                    (aliases: rpi-debian:10:default, rpi-debian:buster:native, rpi-debian:buster:default)
* rpi-debian:11:clean:          Remove package directory                                                          (aliases: rpi-debian:bullseye:clean)
* rpi-debian:11:native:         Build chan_quectel on Raspberry Pi OS (Bullseye)                                  (aliases: rpi-debian:11:default, rpi-debian:bullseye:native, rpi-debian:bullseye:default)
* rpi-debian:12:clean:          Remove package directory                                                          (aliases: rpi-debian:bookworm:clean)
* rpi-debian:12:native:         Build chan_quectel on Raspberry Pi OS (Bookworm)                                  (aliases: rpi-debian:12:default, rpi-debian:bookworm:native, rpi-debian:bookworm:default)
* rpi-debian:clean:             Remove package directories
* rpi-debian:native:            Build chan_quectel on Raspberry Pi OS             (aliases: rpi-debian:default)
* ubuntu:20.04:arm64:           Build chan_quectel for ARM64 on Ubuntu 20.04      (aliases: ubuntu:focal:arm64, ubuntu:focal-fossa:arm64)
* ubuntu:20.04:armhf:           Build chan_quectel for ARM on Ubuntu 20.04        (aliases: ubuntu:focal:armhf, ubuntu:focal-fossa:armhf)
* ubuntu:20.04:clean:           Remove package directory                          (aliases: ubuntu:focal:clean, ubuntu:focal-fossa:clean)
* ubuntu:20.04:native:          Build chan_quectel on Ubuntu 20.04                (aliases: ubuntu:20.04:default, ubuntu:focal:native, ubuntu:focal:default, ubuntu:focal-fossa:native, ubuntu:focal-fossa:default)
* ubuntu:22.04:arm64:           Build chan_quectel for ARM64 on Ubuntu 22.04      (aliases: ubuntu:jammy:arm64, ubuntu:jammy-jellyfish:arm64)
* ubuntu:22.04:armhf:           Build chan_quectel for ARM on Ubuntu 22.04        (aliases: ubuntu:jammy:armhf, ubuntu:jammy-jellyfish:armhf)
* ubuntu:22.04:clean:           Remove package directory                          (aliases: ubuntu:jammy:clean, ubuntu:jammy-jellyfish:clean)
* ubuntu:22.04:native:          Build chan_quectel on Ubuntu 22.04                (aliases: ubuntu:22.04:default, ubuntu:jammy:native, ubuntu:jammy:default, ubuntu:jammy-jellyfish:native, ubuntu:jammy-jellyfish:default)
* ubuntu:24.04:arm64:           Build chan_quectel for ARM64 on Ubuntu 24.04      (aliases: ubuntu:noble:arm64, ubuntu:noble-numbat:arm64)
* ubuntu:24.04:armhf:           Build chan_quectel for ARM on Ubuntu 24.04        (aliases: ubuntu:noble:armhf, ubuntu:noble-numbat:armhf)
* ubuntu:24.04:clean:           Remove package directory                          (aliases: ubuntu:noble:clean, ubuntu:noble-numbat:clean)
* ubuntu:24.04:native:          Build chan_quectel on Ubuntu 24.04                (aliases: ubuntu:24.04:default, ubuntu:noble:native, ubuntu:noble:default, ubuntu:noble-numbat:native, ubuntu:noble-numbat:default)
* ubuntu:arm64:                 Build chan_quectel for ARM64 on Ubuntu
* ubuntu:armhf:                 Build chan_quectel for ARM on Ubuntu
* ubuntu:clean:                 Remove package directories
* ubuntu:native:                Build chan_quectel on Ubuntu      (aliases: ubuntu:default)
```

### Examples of usage

#### Building on *Debian Buster*

```sh
task debian:10:default
```

Packages are copied to `debian/10/package` directory.

#### Building on *CentOS*

```sh
task centos:default
```

Packages are copied to `centos/7.8/package` directory.

#### Building for *Debian Bookworm* ARM64 (cross compilation)

```sh
task debian:12:arm64
```

You may also cross compile using [ARM GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads):

```
task ext:debian:12:arm64
```

Packages are copied to `debian/12/package/arm64` directory.

#### Building for *Raspberry Pi OS* 32-bit (cross compilation)

Building on *Debian Buster* for *Raspbian*:

```sh
task debian:10:rpi4
```

or using [ARM GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads):

```sh
task ext:debian:12:rpi4
```

You may compile `chan_quectel` for specific board version:

| Task name | *Raspberry Pi* model |
| :-------: | :------------------- |
| `rpi`, `rpi1` | B, B+, A, Zero |
| `rpi2` | 2 |
| `rpi3` | 3B, 3B+, 3A+ |
| `rpi4` | 4B |
| `rpi5` | 5 |

> [!NOTE]
> Building module for *Raspberry Pi* requires `qemu-user-static` package to be installed.
> It is used during creation of *development environment* only.
> Compilation is performed by cross compiler.

## Environment variables

There are few environment variables used to control *Docker* usage.
Their default values are defined in `docker.env` file.
You may create `.env` file and change them.

- `DOCKER_QUIET` (**`0`**/`1`)

    Perform Docker operations quietly.

- `DOCKER_IMAGE_ENV_BUILD` (**`0`**/`1`/`2`)

    Specify when to build development environment image:

    | Value | Description |
    | :---: | :---------- |
    | `0` | build only if not exist |
    | `1` | always build (create or update) |
    | `2` | always build without cache (rebuild) |

- `DOCKER_IMAGE_ENV_VERBOSITY` (`none`,**`auto`**,`plain`)

    Verbosity of building process of build environment:

    | Value | Description |
    | :---: | :---------- |
    | `none` | be quiet |
    | `auto` | automatically selected |
    | `plain` | plain mode |

- `DOCKER_IMAGE_BUILDER_VERBOSITY` (`none`,`auto`,**`plain`**)

    Verbosity of `chan_quectel` building process:

    | Value | Description |
    | :---: | :---------- |
    | `none` | be quiet |
    | `auto` | automatically selected |
    | `plain` | plain mode |

- `SOURCE_LOCATION` (**`repo`**,`local`)

    Location of `chan_quectel` sources:

    | Value | Description |
    | :---: | :---------- |
    | `repo` | Use sources from GitHub repository (`git clone`) |
    | `local` | Use sources from your working directory |

- `CMAKE_VERBOSE` (irrelevant value)

    If this variable is defined then configuring and building of `chan_quectel` is performed in verbose mode.

## Useful links

- [Dokcerfile Reference](https://docs.docker.com/reference/dockerfile/)
- [Docker Blog: Dockerfiles now Support Multiple Build Contexts](https://www.docker.com/blog/dockerfiles-now-support-multiple-build-contexts/).
- [*Balena* base images](https://docs.balena.io/reference/base-images/base-images/)
- [ARM GNU Toolchain (previously known as *Linaro* Toolchain)](https://developer.arm.com/Tools%20and%20Software/GNU%20Toolchain)
- [*Baeldung*: What Is a Sysroot?](https://www.baeldung.com/linux/sysroot)
- [*Baeldung*: *gcc* Default include Directories in Linux](https://www.baeldung.com/linux/gcc-default-include-directories)
- [*Baeldung*: Exploring *ld* Linker Search Paths](https://www.baeldung.com/linux/gnu-linker-search-paths)
- [Library path in *gcc*](https://transang.me/library-path-in-gcc/)
- [*Earthly*: Cross-Compiling for Raspberry Pi: Getting Started and Troubleshooting](https://earthly.dev/blog/cross-compiling-raspberry-pi)
- [*Earthly*: Getting a Repeatable Build, Every Time](https://earthly.dev/blog/repeatable-builds-every-time/#tips-for-taming-bash-scripts)
