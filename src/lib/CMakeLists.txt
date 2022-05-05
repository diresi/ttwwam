cmake_minimum_required (VERSION 3.21)

project (libttwwam CXX)

set(_sources lib.cpp
             ttwwam.h
            "${PROJECT_BINARY_DIR}/libttwwam_export.h")
add_library(libttwwam SHARED ${_sources})
include(GenerateExportHeader)
generate_export_header(libttwwam)
target_include_directories(libttwwam PUBLIC ${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR})
