# This file is part of the OregonCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

file(GLOB sources_mpq libmpq/*.c libmpq/*.h)

set(mpq_STAT_SRCS
  ${sources_mpq}
)

add_library(mpq STATIC ${mpq_STAT_SRCS})
set_target_properties(mpq PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(mpq
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(mpq
    PUBLIC
        ZLIB::ZLIB
        BZip2::BZip2
)