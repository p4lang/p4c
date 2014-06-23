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
# This script generates a new code module and unit test build
# for this repository.
#
################################################################

import sys
import os

# The root of the repository
ROOT = os.path.realpath("%s/.." % (os.path.dirname(__file__)))

import infra

from modulegen import *

#
# Import uCli support
#
from uclimod import *

################################################################################
#
# Instantiate the unit test for each new module in utests
#
###############################################################################

class GModuleUnitTestTargetMake(ModuleUnitTestTargetMake):
    def finalize(self):
        self.MODULE_DIRS = "$(ROOT)/modules"
        self.MODULE_UNIT_TEST_DIR = "%s/targets/utests/%s/%s" % (
            ROOT, self.MODULE_RELATIVE_PATH, self.MODULE_NAME)




if __name__ == "__main__":

    ############################################################################
    #
    # Set the module base directory to bigcode/modules
    #
    ############################################################################
    ModuleGenerator.modulesBaseDir = "%s/modules" % ROOT

    #
    # Make it happen.
    #
    ModuleGenerator.main(globals().copy())

    # Rebuild manifest
    os.system("make -C %s/modules manifest" % ROOT)







