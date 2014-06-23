#!/usr/bin/env python
################################################################
#
#        Copyright 2013, Big Switch Networks, Inc.
#
# Licensed under the Eclipse Public License, Version 1.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#        http://www.eclipse.org/legal/epl-v10.html
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
# either express or implied. See the License for the specific
# language governing permissions and limitations under the
# License.
#
################################################################
#
# Resolve dependencies on the infra repository.
#
################################################################
import os
import sys

# The root of the repository
ROOT = os.path.realpath("%s/.." % (os.path.dirname(__file__)))

SUBMODULE_INFRA = os.getenv("SUBMODULE_INFRA")
if SUBMODULE_INFRA is None:
    SUBMODULE_INFRA = "%s/submodules/infra" % ROOT

if not os.path.exists("%s/builder/unix/tools" % SUBMODULE_INFRA):
    raise Exception("This script requires the infra repository.")

sys.path.append("%s/builder/unix/tools" % SUBMODULE_INFRA)
