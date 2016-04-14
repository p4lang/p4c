ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

if COND_TARGETS
    MAYBE_TARGETS = targets
endif

SUBDIRS = thrift_src third_party modules tests $(MAYBE_TARGETS) tools

# I am leaving all style-related files (cpplint) out of dist on purpose, maybe
# will add them later if needed

EXTRA_DIST = \
Doxyfile \
Doxymain.md \
LICENSE \
README.md \
install_deps.sh

# mininet directory
EXTRA_DIST += \
mininet/1sw_demo.py \
mininet/p4_mininet.py

# docs directory
EXTRA_DIST += \
docs/JSON_format.md
