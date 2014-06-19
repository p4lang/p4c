#!/usr/bin/python 
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

################################################################
#
# This script updates all local submodules if they haven't
# been initialized. 
#
################################################################
import os
import sys
import subprocess
import argparse

ap = argparse.ArgumentParser(description="Submodule Management.")

#
# The root of the repository.
# We assume by default that this script resides in 
# the $(ROOT)/submodules directory. 
#
root_default = os.path.abspath("%s/../" % os.path.dirname(__file__))

ap.add_argument("--root", help="The root of the respository.", 
                default=root_default)

ap.add_argument("--update", help="Update the named submodules.", 
                nargs='+', default=None, metavar='SUBMODULE')

ap.add_argument("--list", help="List all submodules.", 
                action='store_true')

ops = ap.parse_args(); 

# Move to the root of the repository
os.chdir(ops.root)

# Get the status of all submodules
submodule_status = {}
try:
    for entry in subprocess.check_output(['git', 'submodule', 'status']).split("\n"):
        data = entry.split()
        if len(data) >= 2:
            submodule_status[data[1].replace("submodules/", "")] = data[0]
except Exception as e:
    print repr(e)
    raise

if ops.list:
    for (module, status) in submodule_status.iteritems():
        print module

if ops.update:
    for (module, status) in submodule_status.iteritems():
        if module in ops.update or "all" in ops.update:
            if status[0] == '-':
                # This submodule has not yet been updated
                print "Updating submodule %s" % module
                if subprocess.check_call(['git', 'submodule', 'update', '--init', 'submodules/%s' % module]) != 0:
                    print "git error updating module '%s'." % (module, switchlight_root, module)
                    sys.exit(1)
            else:
                print "Submodule %s is already checked out." % module
    print "submodules:ok."




