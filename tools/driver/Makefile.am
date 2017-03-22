include $(srcdir)/tools/driver/p4c/Makefile.am

bin_SCRIPTS += p4c
CLEANFILES += $(bin_SCRIPTS)
EXTRA_DIST += p4c.in

do_substitution = sed -e 's,[@]pythondir[@],$(pythondir),g'

p4c: $(srcdir)/tools/driver/p4c.in
	$(do_substitution) < $(srcdir)/tools/driver/p4c.in > $(builddir)/p4c
	chmod +x $(builddir)/p4c
