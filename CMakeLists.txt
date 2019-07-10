cmake_minimum_required (VERSION 3.14)
project (jsmn_dreem)

add_library(jsmn_dreem STATIC jsmn.c json_parser.c)

target_compile_options(jsmn_dreem PRIVATE 
  -fPIC
  )
target_include_directories(jsmn_dreem INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
