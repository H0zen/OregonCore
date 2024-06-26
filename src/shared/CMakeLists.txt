# This file is part of the OregonCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

file(GLOB_RECURSE sources_auth Auth/*.cpp Auth/*.h)
file(GLOB_RECURSE sources_config Config/*.cpp Config/*.h)
file(GLOB_RECURSE sources_database Database/*.cpp Database/*.h)
file(GLOB_RECURSE sources_utilities Utilities/*.cpp Utilities/*.h)
file(GLOB_RECURSE sources_policies Policies/*.cpp Policies/*.h)
file(GLOB sources_localdir *.cpp *.h)

if (WIN32)
      set(sources_Debugging
        ${CMAKE_CURRENT_SOURCE_DIR}/Debugging/WheatyExceptionReport.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Debugging/WheatyExceptionReport.h
      )
else(WIN32)
    if (UNIX)
        set(sources_Debugging ${CMAKE_CURRENT_SOURCE_DIR}/Debugging/UnixDebugger.cpp)
    endif (UNIX)
endif(WIN32)


#
# Build shared sourcelist
#

SET(shared_STAT_SRCS
  ${shared_STAT_SRCS}
  ${sources_auth}
  ${sources_config}
  ${sources_database}
  ${sources_utilities}
  ${sources_policies}
  ${sources_Debugging}
  ${sources_localdir}
)

add_library(shared STATIC ${shared_STAT_SRCS})
add_dependencies(shared revision.h)

target_include_directories(shared
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
)

target_link_libraries(shared
  PUBLIC
        ace
        utf8
        sfmt
        MySQL::MySQL
        ${OPENSSL_LIBRARIES}
#if (defined(OPENSSL_VERSION_MAJOR) && OPENSSL_VERSION_MAJOR >= 3)
        OpenSSL::Crypto
#endif()
)
