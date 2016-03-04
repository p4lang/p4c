ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

if COND_TARGETS
    MAYBE_TARGETS = targets
endif

SUBDIRS = thrift_src third_party modules tests $(MAYBE_TARGETS)

bm_p4dbg: tools/p4dbg.py
	cp $< $@
	chmod +x $@

bm_CLI: tools/runtime_CLI.py
	cp $< $@
	chmod +x $@

bm_nanomsg_events: tools/nanomsg_client.py
	cp $< $@
	chmod +x $@

EXTRA_DIST = \
tools/p4dbg.py \
tools/runtime_CLI.py \
tools/nanomsg_client.py

dist_bin_SCRIPTS = \
bm_p4dbg \
bm_CLI \
bm_nanomsg_events

CLEANFILES = $(dist_bin_SCRIPTS)

# needs to be installed because it is needed by bm_p4dbg
python_PYTHON = \
tools/runtime_CLI.py

# I am leaving all style-related files (cpplint) out of dist on purpose, maybe
# will add them later if needed

EXTRA_DIST += \
Doxyfile \
Doxymain.md \
LICENSE \
README.md \
install_deps.sh

# tools directory
EXTRA_DIST += \
tools/run_valgrind.sh \
tools/veth_setup.sh \
tools/veth_teardown.sh

# mininet directory
EXTRA_DIST += \
mininet/1sw_demo.py \
mininet/p4_mininet.py

# docs directory
EXTRA_DIST += \
docs/JSON_format.md
