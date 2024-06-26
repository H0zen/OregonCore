
FIND_PATH(SYSINFO_INCLUDE_DIR
          NAMES
            sysinfo.h
          PATHS
            /usr
            /usr/local
          PATH_SUFFIXES
            include
            include/sys
)

FIND_LIBRARY(SYSINFO_LIBRARY
    NAMES
        sysinfo
    PATHS
      /usr
      /usr/local/
    PATH_SUFFIXES lib
)

IF (SYSINFO_LIBRARY AND SYSINFO_INCLUDE_DIR)
    SET(SYSINFO_FOUND TRUE)
    MESSAGE(STATUS "Found sysinfo - ${SYSINFO_LIBRARY}")
    INCLUDE_DIRECTORIES(${SYSINFO_INCLUDE_DIR})
ELSE ()
    SET(SYSINFO_FOUND FALSE)
    MESSAGE(FATAL_ERROR "** sysinfo were not found!\n** Your distro may provide a package for sysinfo e.g. for FreeBSD try pkg install sysinfo")
ENDIF ()
