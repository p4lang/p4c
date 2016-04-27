ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

if COND_TARGETS
    MAYBE_TARGETS = targets
endif

if COND_PDFIXED
    MAYBE_PDFIXED = pdfixed
endif

SUBDIRS = thrift_src third_party src include tests $(MAYBE_TARGETS) tools \
$(MAYBE_PDFIXED)

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
mininet/p4_mininet.py \
mininet/stress_test_ipv4.py.in \
mininet/simple_router.p4 \
mininet/simple_router.json \
mininet/stress_test_commands.txt

# docs directory
EXTRA_DIST += \
docs/JSON_format.md
