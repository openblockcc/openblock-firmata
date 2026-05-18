# Keep only files in DIR matching one of the KEEP globs; remove the rest.
#
# Usage:
#   cmake -DDIR=<path> -DKEEP="*.hex;*.uf2" -P prune_dist.cmake

if(NOT DIR OR NOT DEFINED KEEP)
    message(FATAL_ERROR "prune_dist.cmake requires -DDIR and -DKEEP")
endif()

file(GLOB all_files "${DIR}/*")

set(keep_files)
foreach(pattern IN LISTS KEEP)
    file(GLOB matched "${DIR}/${pattern}")
    list(APPEND keep_files ${matched})
endforeach()

foreach(f IN LISTS all_files)
    if(IS_DIRECTORY ${f})
        continue()
    endif()
    list(FIND keep_files "${f}" idx)
    if(idx EQUAL -1)
        file(REMOVE ${f})
    endif()
endforeach()
