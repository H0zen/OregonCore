# This file is part of the OregonCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


file(GLOB_RECURSE scripts_custom Custom/*.cpp Custom/*.h)
file(GLOB_RECURSE scripts_easternplaguelands EasternKingdoms/*.cpp EasternKingdoms/*.h)
file(GLOB_RECURSE scripts_examples Examples/*.cpp Examples/*.h)
file(GLOB_RECURSE scripts_kalimdor Kalimdor/*.cpp Kalimdor/*.h)
file(GLOB_RECURSE scripts_outland Outland/*.cpp Outland/*.h)
file(GLOB_RECURSE scripts_world World/*.cpp World/*.h)

if(SCRIPTS)
  set(scripts_STAT_SRCS
    ${scripts_STAT_SRCS}
    ${scripts_custom}
    ${scripts_easternplaguelands}
    ${scripts_examples}
    ${scripts_kalimdor}
    ${scripts_outland}
    ${scripts_world}
  )
endif()

add_library(scripts STATIC ${scripts_STAT_SRCS})
target_link_libraries(scripts
    PRIVATE
        gameintf
        collision
        RecastNavigation::Detour
        shared
)
target_precompile_headers(scripts PRIVATE ScriptPCH.h)




