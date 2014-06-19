#################################################################
#
#################################################################

THISDIR := $(dir $(lastword $(MAKEFILE_LIST)))

testmod_INCLUDES := -I $(THISDIR)inc
testmod_INTERNAL_INCLUDES := -I $(THISDIR)src
testmod_DEPENDMODULE_ENTRIES += init:testmod ucli:testmod







