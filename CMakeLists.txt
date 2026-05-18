cmake_minimum_required(VERSION 3.20)
project(ConfigurableFirmata NONE)

find_program(ARDUINO_CLI arduino-cli REQUIRED)
find_program(GIT_EXECUTABLE git REQUIRED)

set(SKETCH         "${CMAKE_SOURCE_DIR}/ConfigurableFirmata.ino")
set(LIBRARIES_PATH "${CMAKE_SOURCE_DIR}/libraries")
set(PATCH_DIR      "${CMAKE_SOURCE_DIR}/patch")
set(DIST_DIR       "${CMAKE_SOURCE_DIR}/dist")
set(INTERMEDIATE_DIR "${CMAKE_BINARY_DIR}/intermediate")

option(VERBOSE_BUILD "Pass -v to arduino-cli" OFF)
set(VERBOSE_FLAG)
if(VERBOSE_BUILD)
    set(VERBOSE_FLAG "-v")
endif()

# Add a board build target.
#
# Usage:
#   add_board_target(<name> <fqbn> [PATCH <file>] [EXTRA_FLAGS <args...>])
#
# Each target compiles the sketch with arduino-cli:
#   - intermediates go to ${CMAKE_BINARY_DIR}/intermediate/<name>/
#   - flash artifacts (.hex/.bin/.elf/...) go to ${DIST_DIR}/<name>/
#
# PATCH applies patch/<file> to the source tree before compiling.
# Application is idempotent (cmake/apply_patch.cmake handles already-applied
# state), so it's safe to re-run even after the build dir was wiped.
function(add_board_target NAME FQBN)
    cmake_parse_arguments(B "" "PATCH" "EXTRA_FLAGS" ${ARGN})

    set(intermediate "${INTERMEDIATE_DIR}/${NAME}")
    set(dist         "${DIST_DIR}/${NAME}")

    set(patch_command)
    if(B_PATCH)
        set(patch_command
            COMMAND ${CMAKE_COMMAND}
                    -DPATCH=${PATCH_DIR}/${B_PATCH}
                    -DWORKING_DIR=${CMAKE_SOURCE_DIR}
                    -P ${CMAKE_SOURCE_DIR}/cmake/apply_patch.cmake
        )
    endif()

    add_custom_target(${NAME} ALL
        ${patch_command}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${dist}
        COMMAND ${ARDUINO_CLI} compile ${VERBOSE_FLAG}
                --libraries ${LIBRARIES_PATH}
                --build-path ${intermediate}
                --output-dir ${dist}
                -b ${FQBN}
                ${B_EXTRA_FLAGS}
                ${SKETCH}
        COMMENT "[${NAME}] arduino-cli compile -> dist/${NAME}/"
        VERBATIM
        USES_TERMINAL
    )
endfunction()

add_board_target(arduinoUno       "arduino:avr:uno")
add_board_target(arduinoUnoUltra  "arduino:avr:uno"
    PATCH       arduinoUnoUltra.patch
    EXTRA_FLAGS --build-property "build.extra_flags=-DARDUINO_UNO_ULTRA"
)
add_board_target(arduinoMega2560  "arduino:avr:mega:cpu=atmega2560")
add_board_target(arduinoEsp32     "esp32:esp32:esp32"
    PATCH arduinoEsp32.patch
)
add_board_target(arduinoRP2040    "rp2040:rp2040:rpipico")
add_board_target(arduinoEsp8266   "esp8266:esp8266:generic"
    PATCH arduinoEsp8266.patch
)

# Wipe outputs and revert submodule patches.
add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${DIST_DIR}
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${INTERMEDIATE_DIR}
    COMMAND ${GIT_EXECUTABLE} -C "${LIBRARIES_PATH}/ConfigurableFirmata" checkout .
    COMMENT "Removing dist/, intermediates, and reverting submodule patches"
    VERBATIM
)
