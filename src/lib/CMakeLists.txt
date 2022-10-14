cmake_minimum_required (VERSION 3.21)

project (libttwwam CXX)

set(_sources lib.cpp
             ttwwam.h)

add_library(libttwwam STATIC ${_sources})
