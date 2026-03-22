# build.cmake — Cross-platform build orchestrator for PrusaSlicer (BOSS)
#
# Usage:
#   cmake -P build.cmake                         # Build deps + slicer
#   cmake -P build.cmake -- --target deps        # Build deps only
#   cmake -P build.cmake -- --target slicer      # Build slicer only (deps must exist)
#   cmake -P build.cmake -- --target install     # Build slicer + assemble .app (macOS)
#   cmake -P build.cmake -- --arch x86_64        # Override architecture
#   cmake -P build.cmake -- --build-type Debug   # Override build type

cmake_minimum_required(VERSION 3.20)

# --- Parse arguments ---
set(BUILD_TARGET "all")
set(BUILD_ARCH "")
set(BUILD_TYPE "Release")

set(_i 0)
while(_i LESS ${CMAKE_ARGC})
    set(_arg "${CMAKE_ARGV${_i}}")
    if(_arg STREQUAL "--target")
        math(EXPR _i "${_i} + 1")
        set(BUILD_TARGET "${CMAKE_ARGV${_i}}")
    elseif(_arg STREQUAL "--arch")
        math(EXPR _i "${_i} + 1")
        set(BUILD_ARCH "${CMAKE_ARGV${_i}}")
    elseif(_arg STREQUAL "--build-type")
        math(EXPR _i "${_i} + 1")
        set(BUILD_TYPE "${CMAKE_ARGV${_i}}")
    endif()
    math(EXPR _i "${_i} + 1")
endwhile()

# --- Detect platform and architecture ---
set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(DEPS_DESTDIR "${SOURCE_DIR}/deps/build/destdir")
set(DEPS_PREFIX "${DEPS_DESTDIR}/usr/local")

cmake_host_system_information(RESULT HOST_OS QUERY OS_NAME)
cmake_host_system_information(RESULT HOST_ARCH QUERY OS_PLATFORM)

if(NOT BUILD_ARCH)
    set(BUILD_ARCH "${HOST_ARCH}")
endif()

# Determine preset name
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    if(BUILD_ARCH STREQUAL "aarch64")
        set(BUILD_ARCH "arm64")
    endif()
    set(PRESET "macos-${BUILD_ARCH}")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(PRESET "windows-x64")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    if(BUILD_ARCH STREQUAL "aarch64" OR BUILD_ARCH STREQUAL "arm64")
        set(PRESET "linux-aarch64")
    else()
        set(PRESET "linux-x86_64")
    endif()
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_HOST_SYSTEM_NAME}")
endif()

# Determine parallel job count
cmake_host_system_information(RESULT NCORES QUERY NUMBER_OF_LOGICAL_CORES)

message(STATUS "Platform:  ${CMAKE_HOST_SYSTEM_NAME}")
message(STATUS "Arch:      ${BUILD_ARCH}")
message(STATUS "Preset:    ${PRESET}")
message(STATUS "Target:    ${BUILD_TARGET}")
message(STATUS "Jobs:      ${NCORES}")

# --- Helper: run a command and abort on failure ---
macro(run_command)
    execute_process(
        COMMAND ${ARGN}
        RESULT_VARIABLE _rc
    )
    if(NOT _rc EQUAL 0)
        message(FATAL_ERROR "Command failed (exit ${_rc}): ${ARGN}")
    endif()
endmacro()

# --- Build deps ---
if(BUILD_TARGET STREQUAL "all" OR BUILD_TARGET STREQUAL "deps")
    message(STATUS "")
    message(STATUS "=== Building dependencies ===")

    set(DEPS_BUILD_DIR "${SOURCE_DIR}/deps/build")
    file(MAKE_DIRECTORY "${DEPS_BUILD_DIR}")

    # Deps cmake args
    set(DEPS_CMAKE_ARGS
        -DDESTDIR=${DEPS_DESTDIR}
        -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
    )

    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
        list(APPEND DEPS_CMAKE_ARGS -GNinja)
        list(APPEND DEPS_CMAKE_ARGS "-DCMAKE_TOOLCHAIN_FILE=${SOURCE_DIR}/cmake/toolchain-msvc-ninja.cmake")
    endif()

    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
        list(APPEND DEPS_CMAKE_ARGS
            -DOPENSSL_ARCH=darwin64-${BUILD_ARCH}-cc
            -DCMAKE_OSX_ARCHITECTURES:STRING=${BUILD_ARCH}
            -DCMAKE_OSX_DEPLOYMENT_TARGET=11.3
        )
    endif()

    # Configure deps
    message(STATUS "Configuring dependencies...")
    run_command(
        ${CMAKE_COMMAND} ${SOURCE_DIR}/deps ${DEPS_CMAKE_ARGS}
        WORKING_DIRECTORY "${DEPS_BUILD_DIR}"
    )

    # Build deps
    message(STATUS "Building dependencies...")
    run_command(
        ${CMAKE_COMMAND} --build "${DEPS_BUILD_DIR}" --parallel ${NCORES}
    )

    message(STATUS "Dependencies built successfully.")
endif()

# --- Build slicer ---
if(BUILD_TARGET STREQUAL "all" OR BUILD_TARGET STREQUAL "slicer" OR BUILD_TARGET STREQUAL "install")
    # Verify deps exist
    if(NOT EXISTS "${DEPS_PREFIX}")
        message(FATAL_ERROR
            "Dependencies not found at ${DEPS_PREFIX}\n"
            "Run: cmake -P build.cmake -- --target deps"
        )
    endif()

    message(STATUS "")
    message(STATUS "=== Building slicer ===")

    # Configure
    message(STATUS "Configuring slicer (preset: ${PRESET})...")
    run_command(${CMAKE_COMMAND} --preset ${PRESET})

    # Build
    message(STATUS "Building slicer...")
    run_command(${CMAKE_COMMAND} --build --preset ${PRESET})

    message(STATUS "Slicer built successfully.")
endif()

# --- Install (.app assembly on macOS, binary install elsewhere) ---
if(BUILD_TARGET STREQUAL "install")
    message(STATUS "")
    message(STATUS "=== Installing ===")

    set(BUILD_DIR "${SOURCE_DIR}/build")
    set(INSTALL_DIR "${BUILD_DIR}/PrusaSlicer")

    run_command(
        ${CMAKE_COMMAND} --install "${BUILD_DIR}" --prefix "${INSTALL_DIR}"
    )

    # Clean up .DS_Store files on macOS
    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
        file(GLOB_RECURSE _ds_store "${INSTALL_DIR}/.DS_Store")
        if(_ds_store)
            file(REMOVE ${_ds_store})
        endif()
    endif()

    message(STATUS "Installed to: ${INSTALL_DIR}")
endif()
