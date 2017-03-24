# Copyright 2013-present Barefoot Networks, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

bin_SCRIPTS += p4c
CLEANFILES += $(bin_SCRIPTS)
EXTRA_DIST += p4c.in

do_substitution = sed -e 's,[@]pkgdatadir[@],$(pkgdatadir),g'

# generate the p4c for installation
# this runs before install-data-hook
p4c: $(srcdir)/tools/driver/p4c.in
	@$(do_substitution) < $(srcdir)/tools/driver/p4c.in > $(builddir)/p4c
	@chmod +x $(builddir)/p4c

include $(srcdir)/tools/driver/p4c_src/Makefile.am
