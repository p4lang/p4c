ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

EXTRA_DIST = \
get_version.sh

EXTRA_DIST += \
run_valgrind.sh \
veth_setup.sh \
veth_teardown.sh

python_PYTHON =
bin_SCRIPTS =

if COND_THRIFT

python_PYTHON += bmpy_utils.py runtime_CLI.py

bm_CLI: bm_CLI.in

bin_SCRIPTS += bm_CLI

EXTRA_DIST += bm_CLI.in

endif  # COND_THRIF

if COND_NANOMSG

python_PYTHON += p4dbg.py nanomsg_client.py

bm_p4dbg: bm_p4dbg.in
bm_nanomsg_events: bm_nanomsg_events.in

bin_SCRIPTS += bm_p4dbg bm_nanomsg_events

EXTRA_DIST += bm_p4dbg.in bm_nanomsg_events.in

endif  # COND_NANOMSG

# See
# http://www.gnu.org/software/autoconf/manual/autoconf-2.69/html_node/Installation-Directory-Variables.html
edit = sed \
	-e 's|@pythondir[@]|$(pythondir)|g'

$(bin_SCRIPTS): Makefile
	rm -f $@ $@.tmp
	$(edit) $(srcdir)/$@.in >$@.tmp
	chmod +x $@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

CLEANFILES = $(bin_SCRIPTS)
