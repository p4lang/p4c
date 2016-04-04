irgenerator_SOURCES = \
	tools/ir-generator/ir-generator.ypp \
	tools/ir-generator/generator.cpp \
	tools/ir-generator/irclass.cpp \
	tools/ir-generator/irclass.h \
	tools/ir-generator/type.cpp \
	tools/ir-generator/type.h
irgenerator_LDADD = -lgc libp4ctoolkit.a

BUILT_SOURCES += \
	tools/ir-generator/ir-generator-lex.c

bin_PROGRAMS += irgenerator

cpplint_FILES += $(irgenerator_SOURCES)
