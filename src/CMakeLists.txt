cmake_minimum_required (VERSION 3.21)

project (ttwwam CXX)

add_subdirectory(lib)

add_executable(ttwwam main.cpp)
target_link_libraries(ttwwam libttwwam)

# add_custom_command(TARGET ttwwam POST_BUILD
#   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:ttwwam> $<TARGET_FILE_DIR:ttwwam>
#   COMMAND_EXPAND_LISTS
# )

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
