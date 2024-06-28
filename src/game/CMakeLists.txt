# This file is part of the OregonCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

if (USE_COREPCH)
  include_directories(${CMAKE_CURRENT_BINARY_DIR})
endif()

file(GLOB sources_localdir *.cpp)
file(GLOB headers_localdir *.h)

# Create game-libary


set(game_STAT_SRCS
  ${game_STAT_SRCS}
  ${sources_localdir}
)

add_library(game STATIC ${game_STAT_SRCS})
add_library(gameintf INTERFACE)


target_include_directories(game PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(gameintf INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(game
    PUBLIC
        collision
        shared
        RecastNavigation::Detour
		ZLIB::ZLIB
)

target_precompile_headers(game PRIVATE gamePCH.h)
