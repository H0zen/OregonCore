# This file is part of the OregonCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

set(sources_Debugging "")

if( SERVERS )
  add_subdirectory(genrev)
  add_subdirectory(shared)
  add_subdirectory(oregonrealm)
  add_subdirectory(collision)
  add_subdirectory(game)
  add_subdirectory(scripts)
  add_subdirectory(oregoncore)
endif()

if(TOOLS)
  add_subdirectory(tools)
endif(TOOLS)
