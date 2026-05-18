cmake_minimum_required(VERSION 3.20)
project(ConfigurableFirmata NONE)

find_program(ARDUINO_CLI arduino-cli REQUIRED)
find_program(GIT_EXECUTABLE git REQUIRED)

set(SKETCH         "${CMAKE_SOURCE_DIR}/ConfigurableFirmata.ino")
set(LIBRARIES_PATH "${CMAKE_SOURCE_DIR}/libraries")
set(PATCH_DIR      "${CMAKE_SOURCE_DIR}/patch")
set(DIST_DIR       "${CMAKE_SOURCE_DIR}/dist")
set(INTERMEDIATE_DIR "${CMAKE_BINARY_DIR}/intermediate")

option(VERBOSE_BUILD "Pass -v to arduino-cli" ON)
set(VERBOSE_FLAG)
if(VERBOSE_BUILD)
    set(VERBOSE_FLAG "-v")
endif()

# Parallel compile jobs passed to arduino-cli (-j N).
# Board targets stay serialized at the ninja level (USES_TERMINAL); parallelism
# happens inside each arduino-cli call across the sketch's .cpp files.
include(ProcessorCount)
ProcessorCount(_DETECTED_JOBS)
if(_DETECTED_JOBS EQUAL 0)
    set(_DETECTED_JOBS 1)
endif()
set(ARDUINO_JOBS "${_DETECTED_JOBS}" CACHE STRING
    "Value of -j passed to arduino-cli compile (default: detected CPU cores)")

# Source files that, when changed, should invalidate every board's stamp.
# CONFIGURE_DEPENDS makes Ninja re-run CMake if files are added/removed.
file(GLOB_RECURSE FIRMATA_SOURCE_DEPS CONFIGURE_DEPENDS
    "${LIBRARIES_PATH}/*.cpp"
    "${LIBRARIES_PATH}/*.c"
    "${LIBRARIES_PATH}/*.h"
    "${LIBRARIES_PATH}/*.hpp"
    "${LIBRARIES_PATH}/*.S"
)

set(STAMP_DIR "${CMAKE_BINARY_DIR}/stamps")
file(MAKE_DIRECTORY "${STAMP_DIR}")

# Firmata feature set shared by every board. Each token becomes -D<token>
# and is passed to arduino-cli via compiler.cpp.extra_flags.
# Per-board adjustments use add_board_target's EXTRA_FEATURES / DISABLE_FEATURES.
set(FIRMATA_BASE_FEATURES
    # ENABLE_ONE_WIRE
    ENABLE_SERVO
    # ENABLE_ACCELSTEPPER
    # ENABLE_BASIC_SCHEDULER
    ENABLE_SERIAL
    ENABLE_I2C
    ENABLE_SPI
    ENABLE_ANALOG
    ENABLE_DIGITAL
    ENABLE_DHT
    ENABLE_FREQUENCY
    ENABLE_SONAR
    ENABLE_BUZZER
    # ENABLE_SLEEP
)

# Add a board build target.
#
# Usage:
#   add_board_target(<name> <fqbn>
#                    [PATCH <file>]
#                    [EXTRA_FLAGS <args...>]
#                    [EXTRA_FEATURES <ENABLE_X> ...]
#                    [DISABLE_FEATURES <ENABLE_X> ...]
#                    [KEEP <glob> ...])
#
# Feature set is computed as (FIRMATA_BASE_FEATURES + EXTRA_FEATURES) - DISABLE_FEATURES
# and injected as -D macros into the sketch compilation.
#
# Each target compiles the sketch with arduino-cli:
#   - intermediates go to ${CMAKE_BINARY_DIR}/intermediate/<name>/
#   - flash artifacts (.hex/.bin/.elf/...) go to ${DIST_DIR}/<name>/
#
# PATCH applies patch/<file> to the source tree before compiling.
# Application is idempotent (cmake/apply_patch.cmake handles already-applied
# state), so it's safe to re-run even after the build dir was wiped.
function(add_board_target NAME FQBN)
    cmake_parse_arguments(B "" "PATCH" "EXTRA_FLAGS;KEEP;EXTRA_FEATURES;DISABLE_FEATURES" ${ARGN})

    set(intermediate "${INTERMEDIATE_DIR}/${NAME}")
    set(dist         "${DIST_DIR}/${NAME}")

    # Compose per-board feature set.
    set(features ${FIRMATA_BASE_FEATURES})
    if(B_EXTRA_FEATURES)
        list(APPEND features ${B_EXTRA_FEATURES})
    endif()
    if(B_DISABLE_FEATURES)
        list(REMOVE_ITEM features ${B_DISABLE_FEATURES})
    endif()
    list(REMOVE_DUPLICATES features)

    set(feature_defines)
    foreach(f IN LISTS features)
        list(APPEND feature_defines "-D${f}")
    endforeach()
    list(JOIN feature_defines " " feature_defines_str)
    # arduino-cli overrides this property wholesale; it's empty by default in
    # the AVR/ESP32/ESP8266/RP2040 cores we target, so override is safe.
    set(extra_flags_prop "compiler.cpp.extra_flags=${feature_defines_str}")

    set(patch_command)
    if(B_PATCH)
        set(patch_command
            COMMAND ${CMAKE_COMMAND}
                    -DPATCH=${PATCH_DIR}/${B_PATCH}
                    -DWORKING_DIR=${CMAKE_SOURCE_DIR}
                    -P ${CMAKE_SOURCE_DIR}/cmake/apply_patch.cmake
        )
    endif()

    set(prune_command)
    if(B_KEEP)
        string(REPLACE ";" "\;" keep_arg "${B_KEEP}")
        set(prune_command
            COMMAND ${CMAKE_COMMAND}
                    -DDIR=${dist}
                    "-DKEEP=${keep_arg}"
                    -P ${CMAKE_SOURCE_DIR}/cmake/prune_dist.cmake
        )
    endif()

    # Stamp-file-driven build: ninja only re-runs arduino-cli when one of
    # the listed DEPENDS is newer than the stamp. arduino-cli still does its
    # own incremental work inside ${intermediate} when it does run.
    set(stamp "${STAMP_DIR}/${NAME}.stamp")
    set(deps ${SKETCH} ${FIRMATA_SOURCE_DEPS} ${CMAKE_CURRENT_LIST_FILE})
    if(B_PATCH)
        list(APPEND deps "${PATCH_DIR}/${B_PATCH}")
    endif()

    add_custom_command(
        OUTPUT ${stamp}
        ${patch_command}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${dist}
        COMMAND ${ARDUINO_CLI} compile ${VERBOSE_FLAG}
                --jobs ${ARDUINO_JOBS}
                --libraries ${LIBRARIES_PATH}
                --build-path ${intermediate}
                --output-dir ${dist}
                -b ${FQBN}
                --build-property ${extra_flags_prop}
                ${B_EXTRA_FLAGS}
                ${SKETCH}
        ${prune_command}
        COMMAND ${CMAKE_COMMAND} -E touch ${stamp}
        DEPENDS ${deps}
        COMMENT "[${NAME}] arduino-cli compile -> dist/${NAME}/"
        VERBATIM
        USES_TERMINAL
    )

    add_custom_target(${NAME} ALL DEPENDS ${stamp})
endfunction()

add_board_target(arduinoUno       "arduino:avr:uno"
    KEEP "*.ino.hex"
)
add_board_target(arduinoNano  "arduino:avr:nano:cpu=atmega328"
    KEEP "*.ino.hex"
)
add_board_target(arduinoMega2560  "arduino:avr:mega:cpu=atmega2560"
    KEEP "*.ino.hex"
)
add_board_target(arduinoEsp32     "esp32:esp32:esp32"
    PATCH arduinoEsp32.patch
    KEEP  "*.merged.bin"
)
add_board_target(arduinoEsp32S3   "esp32:esp32:esp32s3"
    PATCH arduinoEsp32.patch
    KEEP  "*.merged.bin"
)
add_board_target(arduinoEsp32C3   "esp32:esp32:esp32c3"
    PATCH arduinoEsp32.patch
    KEEP  "*.merged.bin"
)
add_board_target(arduinoEsp32C6   "esp32:esp32:esp32c6"
    PATCH arduinoEsp32.patch
    KEEP  "*.merged.bin"
)
add_board_target(arduinoPico      "rp2040:rp2040:rpipico"
    KEEP "*.uf2"
)
add_board_target(arduinoPicoW     "rp2040:rp2040:rpipicow"
    KEEP "*.uf2"
)
# Pico 2 / Pico 2W are RP2350. ConfigurableFirmata's Boards.h only handles
# TARGET_RP2040 / TARGET_RASPBERRY_PI_PICO, so the build hits the catch-all
# #error in Boards.h. Re-enable after adding an RP2350 branch via patch.
# add_board_target(arduinoPico2     "rp2040:rp2040:rpipico2"
#     KEEP "*.uf2"
# )
# add_board_target(arduinoPico2W    "rp2040:rp2040:rpipico2w"
#     KEEP "*.uf2"
# )
add_board_target(arduinoEsp8266   "esp8266:esp8266:generic"
    PATCH arduinoEsp8266.patch
    KEEP  "*.bin"
)

# Wipe outputs and revert submodule patches.
add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${DIST_DIR}
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${INTERMEDIATE_DIR}
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${STAMP_DIR}
    COMMAND ${GIT_EXECUTABLE} -C "${LIBRARIES_PATH}/ConfigurableFirmata" checkout .
    COMMENT "Removing dist/, intermediates, stamps, and reverting submodule patches"
    VERBATIM
)
