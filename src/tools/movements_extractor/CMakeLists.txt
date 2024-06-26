# This file is part of the OregonCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

set(sources
    IntermediateValues.cpp
    IntermediateValues.h
    MapBuilder.cpp
    MapBuilder.h
    MMapCommon.h
    generator.cpp
    TerrainBuilder.cpp
    TerrainBuilder.h
    VMapExtensions.cpp
)

# definitions
add_definitions(-DNO_CORE_FUNCS)
add_definitions(-DDEBUG)
add_definitions(-DNO_vsnprintf)

add_executable(movements_extractor
    ${sources}
)

if( UNIX )
  set(mv_LINK_FLAGS "-pthread -rdynamic ${mv_LINK_FLAGS}")
endif()

set_target_properties(movements_extractor PROPERTIES LINK_FLAGS "${mv_LINK_FLAGS}")

target_link_libraries(movements_extractor PRIVATE
  Recast
  Detour
  collision
)

if( UNIX )
  target_link_libraries(oregon-core
    dl
    rt
  )
endif()


if( UNIX )
  install(TARGETS movements_extractor DESTINATION bin)
elseif( WIN32 )
  install(TARGETS movements_extractor DESTINATION "${CMAKE_INSTALL_PREFIX}")
endif()
