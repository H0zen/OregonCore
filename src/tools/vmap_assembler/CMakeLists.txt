# This file is part of the OregonCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

add_definitions(-DNO_CORE_FUNCS)

add_executable(vmap_assembler VMapAssembler.cpp)

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set_target_properties(vmap_assembler PROPERTIES LINK_FLAGS "-framework Carbon")
endif()

target_link_libraries(vmap_assembler
    PRIVATE
      collision
)

if( UNIX )
  install(TARGETS vmap_assembler DESTINATION bin)
elseif( WIN32 )
  install(TARGETS vmap_assembler DESTINATION "${CMAKE_INSTALL_PREFIX}")
endif()
