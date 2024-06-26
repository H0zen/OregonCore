# This file is part of the OregonCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

file(GLOB sources_localdir
  *.cpp
  RegressionTests/*.cpp
)

set(oregon-core_SRCS
  ${oregon-core_SRCS}
  ${sources_localdir}
)

if( WIN32 )
  set(oregon-core_SRCS
    ${oregon-core_SRCS}
    OregonCore.rc
  )
endif()

set(oregon-core_LINK_FLAGS "")

add_executable(oregon-core ${oregon-core_SRCS})

if( NOT WIN32 )
  add_definitions(-D_OREGON_CORE_CONFIG="${CONF_DIR}/oregoncore.conf")
endif()

if( UNIX )
  set(oregon-core_LINK_FLAGS "-pthread -rdynamic ${oregon-core_LINK_FLAGS}")
endif()

set_target_properties(oregon-core PROPERTIES LINK_FLAGS "${oregon-core_LINK_FLAGS}")

target_link_libraries(oregon-core
  game
  shared
  scripts
  gsoap
  Recast
  Detour
  ${TERMCAP_LIBRARY}
  ${CURSES_LIBRARY}
  ${OSX_LIBS}
)

if( UNIX )
  target_link_libraries(oregon-core
    ${BFD_LIBRARY}
    dl
    ${IBERTY_LIBRARY}
    rt
    ${SYSINFO_LIBRARY}
    execinfo
  )
endif()

if( WIN32 )
  add_custom_command(TARGET oregon-core
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/oregoncore.conf.dist ${CMAKE_BINARY_DIR}/bin/$(ConfigurationName)/
  )
endif()

if( UNIX )
  install(TARGETS oregon-core DESTINATION bin)
  install(FILES oregoncore.conf.dist DESTINATION ${CONF_DIR})
elseif( WIN32 )
  install(TARGETS oregon-core DESTINATION "${CMAKE_INSTALL_PREFIX}")
  install(FILES oregoncore.conf.dist DESTINATION "${CMAKE_INSTALL_PREFIX}")
endif()
