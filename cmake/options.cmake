# Copyright (C) 2008-2018 OregonCore <https://oregon-core.net/>
# Copyright (C) 2008-2012 TrinityCore <https://www.trinitycore.org/>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

option(SERVERS          "Build worldserver and authserver"                            ON)
option(SCRIPTS          "Build core with scripts included"                            ON)
option(TOOLS            "Build map/vmap extraction/assembler tools"                   ON)
option(USE_SCRIPTPCH    "Use precompiled headers when compiling scripts"              ON)
option(USE_COREPCH      "Use precompiled headers when compiling servers"              ON)
option(WITH_WARNINGS    "Show all warnings during compile"                            OFF)
option(WITH_COREDEBUG   "Include additional debug-code in core"                       0)
option(WITH_DOCS        "Build Doxygen Documentation"                                 ON)
