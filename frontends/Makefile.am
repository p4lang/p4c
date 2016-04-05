v12_frontend_SOURCES = \
	frontends/p4/p4-parse.ypp \
	frontends/p4/frontend.h \
	frontends/p4/frontend.cpp \
	frontends/p4/createBuiltins.h \
	frontends/p4/createBuiltins.cpp \
	frontends/p4/validateParsedProgram.h \
	frontends/p4/validateParsedProgram.cpp \
	frontends/p4/p4-parse.h \
	frontends/p4/symbol_table.cpp \
	frontends/p4/symbol_table.h \
	frontends/p4/toP4/toP4.cpp \
	frontends/p4/toP4/toP4.h \
	frontends/p4/callGraph.h \
	frontends/p4/typeChecking/typeChecker.cpp \
	frontends/p4/typeChecking/typeChecker.h \
	frontends/p4/typeChecking/typeConstraints.h \
	frontends/p4/typeChecking/typeConstraints.cpp \
	frontends/p4/typeChecking/typeUnification.h \
	frontends/p4/typeChecking/typeUnification.cpp \
	frontends/p4/strengthReduction.h \
	frontends/p4/strengthReduction.cpp \
	frontends/p4/unusedDeclarations.h \
	frontends/p4/unusedDeclarations.cpp \
	frontends/p4/evaluator/blockMap.h \
	frontends/p4/evaluator/blockMap.cpp \
	frontends/p4/evaluator/specializeBlocks.h \
	frontends/p4/evaluator/specializeBlocks.cpp \
	frontends/p4/evaluator/evaluator.h \
	frontends/p4/evaluator/evaluator.cpp \
	frontends/p4/evaluator/substituteParameters.h \
	frontends/p4/evaluator/substituteParameters.cpp \
	frontends/p4/cloner.h \
	frontends/p4/reservedWords.h \
	frontends/p4/reservedWords.cpp \
	frontends/p4/coreLibrary.h \
	frontends/p4/coreLibrary.cpp \
	frontends/p4/enumInstance.h \
	frontends/p4/methodInstance.h \
	frontends/p4/methodInstance.cpp \
	frontends/p4/tableApply.h \
	frontends/p4/tableApply.cpp \
	frontends/p4/simplify.h \
	frontends/p4/simplify.cpp \
	frontends/p4/fromv1.0/programStructure.h \
	frontends/p4/fromv1.0/programStructure.cpp \
	frontends/p4/fromv1.0/v1model.h \
	frontends/p4/fromv1.0/v1model.cpp \
	frontends/p4/fromv1.0/converters.h \
	frontends/p4/fromv1.0/converters.cpp

common_frontend_SOURCES = \
	frontends/common/options.h  \
	frontends/common/options.cpp \
	frontends/common/constantFolding.h \
	frontends/common/constantFolding.cpp \
	frontends/common/typeMap.h \
	frontends/common/typeMap.cpp \
	frontends/common/resolveReferences/referenceMap.cpp \
	frontends/common/resolveReferences/resolveReferences.cpp \
	frontends/common/resolveReferences/referenceMap.h \
	frontends/common/resolveReferences/resolveReferences.h \
	frontends/common/model.h \
	frontends/common/parseInput.h \
	frontends/common/parseInput.cpp \
	frontends/common/constantParsing.h \
	frontends/common/constantParsing.cpp

v1_frontend_SOURCES = \
	frontends/p4v1/intrinsic.cpp \
	frontends/p4v1/p4v1-parse.ypp \
	frontends/p4v1/typecheck.h \
	frontends/p4v1/typecheck.cpp \
	frontends/p4v1/header_type.h \
	frontends/p4v1/header_type.cpp

ir_DEF_FILES += $(srcdir)/frontends/p4v1/ir-v1.def

BUILT_SOURCES += \
        frontends/p4v1/p4v1-lex.c \
        frontends/p4/p4-lex.c \
	frontends/p4v1/p4v1-parse.cpp \
	frontends/p4/p4-parse.cpp 

frontend_SOURCES = \
	$(v12_frontend_SOURCES) \
	$(common_frontend_SOURCES) \
	$(v1_frontend_SOURCES)

cpplint_FILES += \
	$(frontend_SOURCES) \
	frontends/p4v1/p4v1-lex.l \
	frontends/p4/p4-lex.l
