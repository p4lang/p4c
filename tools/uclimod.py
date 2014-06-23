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

###############################################################################
#
# Add a custom makefile in the module's src directory
# which regenerates ucli handlers:
#
###############################################################################
from modulegen import *

class GModuleSrcMakefile(ModuleMakefile):
    def finit(self):
        self.BRIEF = "Local source generation targets."
        self.fname = "%(MODULE_SRC_DIR)s/Makefile"
        self.body = (
"""
ucli:
\t@../../../../tools/uclihandlers.py %(MODULE_NAME_LOWER)s_ucli.c

""")

###############################################################################
#
# Create a basic <module>_ucli.c file
#
###############################################################################
class GModuleUcliSource(ModuleCSourceFile):
    def finit(self):
        self.BRIEF = "%(MODULE_NAME)s Generic uCli Support."
        self.fname="%(MODULE_SRC_DIR)s/%(MODULE_NAME_LOWER)s_ucli.c"
        self.body=(
"""
#if %(MODULE_NAME_UPPER)s_CONFIG_INCLUDE_UCLI == 1

#include <uCli/ucli.h>
#include <uCli/ucli_argparse.h>
#include <uCli/ucli_handler_macros.h>

static ucli_status_t
%(MODULE_NAME_LOWER)s_ucli_ucli__config__(ucli_context_t* uc)
{
    UCLI_HANDLER_MACRO_MODULE_CONFIG(%(MODULE_NAME_LOWER)s)
}

/* <auto.ucli.handlers.start> */
/* <auto.ucli.handlers.end> */

static ucli_module_t
%(MODULE_NAME_LOWER)s_ucli_module__ =
    {
        "%(MODULE_NAME_LOWER)s_ucli",
        NULL,
        %(MODULE_NAME_LOWER)s_ucli_ucli_handlers__,
        NULL,
        NULL,
    };

ucli_node_t*
%(MODULE_NAME_LOWER)s_ucli_node_create(void)
{
    ucli_node_t* n;
    ucli_module_init(&%(MODULE_NAME_LOWER)s_ucli_module__);
    n = ucli_node_create("%(MODULE_NAME_LOWER)s", NULL, &%(MODULE_NAME_LOWER)s_ucli_module__);
    ucli_node_subnode_add(n, ucli_module_log_node_create("%(MODULE_NAME_LOWER)s"));
    return n;
}

#else
void*
%(MODULE_NAME_LOWER)s_ucli_node_create(void)
{
    return NULL;
}
#endif
""")


###############################################################################
#
# Add some extra CDEFS to each module's default autogen file
#
###############################################################################
GModuleAutoYml.CDEFS += (
"""- %(MODULE_NAME_UPPER)s_CONFIG_INCLUDE_UCLI:
    doc: "Include generic uCli support."
    default: 0
""")
