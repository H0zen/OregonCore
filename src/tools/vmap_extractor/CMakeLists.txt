# This file is part of the OregonCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

file(GLOB_RECURSE sources *.cpp *.h)

# uncomment next line to disable debug mode
add_definitions("-DIOMAP_DEBUG")

# build setup currently only supports libmpq 0.4.x
add_definitions("-DUSE_LIBMPQ04")


add_executable(vmap_extractor ${sources})

target_link_libraries(vmap_extractor
  mpq
  G3D::G3D
)

if( UNIX )
  install(TARGETS vmap_extractor DESTINATION bin)
elseif( WIN32 )
  install(TARGETS vmap_extractor DESTINATION "${CMAKE_INSTALL_PREFIX}")
endif()
