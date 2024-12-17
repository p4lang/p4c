project(libdynhash VERSION 0.1 LANGUAGES C)
add_library(dynhash_o OBJECT
  dynamic_hash.cpp
  bfn_hash_algorithm.cpp
)

add_library(dynhash SHARED EXCLUDE_FROM_ALL $<TARGET_OBJECTS:dynhash_o>)

# static lib for compiler with no syslibs
add_library(tofino_dynhash STATIC dynamic_hash.cpp bfn_hash_algorithm.cpp)
set_target_properties(tofino_dynhash PROPERTIES OUTPUT_NAME dynhash)
target_compile_definitions(tofino_dynhash PUBLIC NO_BFSYSLIBS)

