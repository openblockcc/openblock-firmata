# Idempotently apply a git patch.
#
# Usage:
#   cmake -DPATCH=<file> -DWORKING_DIR=<dir> -P apply_patch.cmake
#
# Behavior:
#   - If the patch reverses cleanly, it's already applied -> no-op.
#   - Else if it applies cleanly forward, apply it.
#   - Else fail with a clear error (real conflict).

if(NOT PATCH OR NOT WORKING_DIR)
    message(FATAL_ERROR "apply_patch.cmake requires -DPATCH and -DWORKING_DIR")
endif()

find_program(GIT git REQUIRED)

execute_process(
    COMMAND ${GIT} apply --reverse --check ${PATCH}
    WORKING_DIRECTORY ${WORKING_DIR}
    RESULT_VARIABLE reverse_rc
    OUTPUT_QUIET ERROR_QUIET
)
if(reverse_rc EQUAL 0)
    message(STATUS "Patch already applied: ${PATCH}")
    return()
endif()

execute_process(
    COMMAND ${GIT} apply --check ${PATCH}
    WORKING_DIRECTORY ${WORKING_DIR}
    RESULT_VARIABLE forward_rc
    OUTPUT_VARIABLE forward_out
    ERROR_VARIABLE forward_err
)
if(NOT forward_rc EQUAL 0)
    message(FATAL_ERROR
        "Patch does not apply cleanly: ${PATCH}\n${forward_err}")
endif()

execute_process(
    COMMAND ${GIT} apply ${PATCH}
    WORKING_DIRECTORY ${WORKING_DIR}
    RESULT_VARIABLE apply_rc
    COMMAND_ERROR_IS_FATAL ANY
)
message(STATUS "Applied patch: ${PATCH}")
