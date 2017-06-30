set (TOOLSDIR, "${P4C_SOURCE_DIR}/tools")
set (GENTESTS, "${TOOLSDIR}/gen-tests.py")

set (IRGENERATOR_SRC_FILES
  generator.cpp
  irclass.cpp
  methods.cpp
  type.cpp
  )

set (IRGENERATOR_HEADERS
  ir-generator.h
  irclass.h
  type.h
  )

BISON_TARGET(IRgenParser ir-generator.ypp ${CMAKE_CURRENT_BINARY_DIR}/ir-generator.cpp)
FLEX_TARGET(IRgenLexer ir-generator-lex.l  ${CMAKE_CURRENT_BINARY_DIR}/ir-generator-lex.c)
ADD_FLEX_BISON_DEPENDENCY(IRgenLexer IRgenParser)

set (IRGENERATOR_GEN_FILES
    ${BISON_IRgenParser_OUTPUTS}
  )

include_directories (${CMAKE_CURRENT_BINARY_DIR})
add_executable (irgenerator ${IRGENERATOR_SRC_FILES} ${IRGENERATOR_GEN_FILES})
target_link_libraries (irgenerator p4ctoolkit gc gmp)
