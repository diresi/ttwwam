cmake_minimum_required (VERSION 3.12)

project (ttwwam CXX)

set (sources
             src/cmd.cpp
             src/log.cpp
             src/main.cpp
             src/wm.cpp
             src/ui.cpp
    )
add_executable(ttwwam ${sources})
IF (WIN32)
    set_target_properties(ttwwam PROPERTIES COMPILE_DEFINITIONS_DEBUG "_WINDOWS")
    set_target_properties(ttwwam PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS")
    set_target_properties(ttwwam PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "_WINDOWS")
    set_target_properties(ttwwam PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDWOS")
    set_target_properties(ttwwam PROPERTIES COMPILE_DEFINITIONS_RELEASE "_WINDOWS")
    set_target_properties(ttwwam PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
    set_target_properties(ttwwam PROPERTIES COMPILE_DEFINITIONS_MINSIZEREL "_WINDOWS")
    set_target_properties(ttwwam PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")
ENDIF (WIN32)
