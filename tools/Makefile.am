ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

EXTRA_DIST = \
get_version.sh

EXTRA_DIST += \
run_valgrind.sh \
veth_setup.sh \
veth_teardown.sh

if COND_THRIFT

python_PYTHON = \
p4dbg.py \
runtime_CLI.py \
nanomsg_client.py \
bmpy_utils.py

# See
# http://www.gnu.org/software/autoconf/manual/autoconf-2.69/html_node/Installation-Directory-Variables.html
edit = sed \
	-e 's|@pythondir[@]|$(pythondir)|g'

bm_p4dbg bm_CLI bm_nanomsg_events: Makefile
	rm -f $@ $@.tmp
	$(edit) $(srcdir)/$@.in >$@.tmp
	chmod +x $@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

bm_p4dbg: bm_p4dbg.in
bm_CLI: bm_CLI.in
bm_nanomsg_events: bm_nanomsg_events.in

bin_SCRIPTS = \
bm_p4dbg \
bm_CLI \
bm_nanomsg_events

EXTRA_DIST += \
bm_p4dbg.in \
bm_CLI.in \
bm_nanomsg_events.in

CLEANFILES = $(bin_SCRIPTS)

endif  # COND_THRIFT
