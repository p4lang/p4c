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

import os

# recursive find, good for developer
def rec_find_bin(cwd, exe):
    found = None
    for root, dirs, files in os.walk(cwd):
        for f in files:
            if f == exe:
                return os.path.join(root, f)
    cwd = os.path.abspath(os.path.join(cwd, os.pardir))
    if cwd != "/":
        found = rec_find_bin(cwd, exe)
    return found

def use_rec_find(exe):
    cwd = os.getcwd()
    return rec_find_bin(cwd, exe)

def getLocalCfg(config):
    """
    Search bottom-up for p4c.site.cfg
    """
    return use_rec_find(config)

# top-down find, good for deployment
def find_bin(exe):
    found_path = None
    for pp in os.environ['PATH'].split(':'):
        for root, dirs, files in os.walk(pp):
            for ff in files:
                if ff == exe:
                    found_path = os.path.join(pp, ff)
    return found_path

