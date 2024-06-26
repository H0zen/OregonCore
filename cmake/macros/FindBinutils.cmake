
FIND_PATH(BFD_INCLUDE_DIR
          NAMES
            bfd.h
          PATHS
            /usr
            /usr/local
            /usr/local/include
          PATH_SUFFIXES
            libbfd
)

FIND_LIBRARY(BFD_LIBRARY
    NAMES
        bfd
    PATHS
      /usr
      /usr/local/
    PATH_SUFFIXES lib lib/opt
)

FIND_LIBRARY(IBERTY_LIBRARY
    NAMES
        iberty
    PATHS
      /usr
      /usr/local/
    PATH_SUFFIXES lib lib/opt
)

IF (IBERTY_LIBRARY AND BFD_LIBRARY AND BFD_INCLUDE_DIR)
    SET(BINUTILS_FOUND TRUE)
    MESSAGE(STATUS "Found Binutils - ${BFD_LIBRARY}, ${IBERTY_LIBRARY}")
    INCLUDE_DIRECTORIES(${BFD_INCLUDE_DIR})
    INCLUDE_DIRECTORIES(${IBERTY_INCLUDE_DIR})
ELSE ()
    SET(BINUTILS_FOUND FALSE)
    MESSAGE(FATAL_ERROR "** Binutils were not found!\n** Your distro may provide a package for binutils e.g. for ubuntu try apt-get install binutils-dev")
ENDIF ()
