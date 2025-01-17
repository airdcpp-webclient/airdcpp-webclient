if(UNIX)
    find_package(PkgConfig QUIET)
    pkg_check_modules(_NATPMP QUIET libnatpmp)
endif()

find_path(NATPMP_INCLUDE_DIR
    NAMES natpmp.h
    HINTS ${_NATPMP_INCLUDEDIR})
find_library(NATPMP_LIBRARY
    NAMES natpmp
    HINTS ${_NATPMP_LIBDIR})

set(NATPMP_INCLUDE_DIRS ${NATPMP_INCLUDE_DIR})
set(NATPMP_LIBRARIES ${NATPMP_LIBRARY})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(natpmp
    REQUIRED_VARS
        NATPMP_LIBRARY
        NATPMP_INCLUDE_DIR)

mark_as_advanced(NATPMP_INCLUDE_DIR NATPMP_LIBRARY)

if(NATPMP_FOUND)
    # message(STATUS "Found libnatpmp  (include: ${NATPMP_INCLUDE_DIR}, library: ${NATPMP_LIBRARY})")
    if(NOT TARGET natpmp)
      add_library(natpmp UNKNOWN IMPORTED)
      set_target_properties(natpmp PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${NATPMP_INCLUDE_DIR}")

        set_property(TARGET natpmp APPEND PROPERTY
          IMPORTED_LOCATION "${NATPMP_LIBRARY}")
    endif()
endif()
