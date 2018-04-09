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

import inspect
import os
import sys

# get the directory the python program is running from
def get_script_dir(follow_symlinks=True):
    if getattr(sys, 'frozen', False): # py2exe, PyInstaller, cx_Freeze
        path = os.path.abspath(sys.executable)
    else:
        path = inspect.getabsfile(get_script_dir)
    if follow_symlinks:
        path = os.path.realpath(path)
    return os.path.dirname(path)

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

def find_file(directory, filename, binary=True):
    """
    Searches up the directory hierarchy for filename with prefix directory
    starting from the current working directory.
    If directory is an absolute path, just check for the file.
    If binary == true, then check permissions that the file is executable
    """
    def check_file(f):
        if os.path.isfile(f):
            if binary:
                if os.access(f, os.X_OK): return True
            else:
                return True
        return False

    executable = ""
    if directory.startswith('/'):
        executable = os.path.normpath(os.path.join(directory, filename))
        if check_file(executable): return executable

    # find the file up the hierarchy
    dir	= os.path.abspath(get_script_dir())
    if os.path.basename(filename) != filename:
        directory = os.path.join(directory, os.path.dirname(filename))
        filename = os.path.basename(filename)
    while dir != "/":
        path_to_file = os.path.join(dir, directory)
        if os.path.isdir(path_to_file):
	    files = os.listdir(path_to_file)
            if filename in files:
                executable = os.path.join(path_to_file, filename)
                if check_file(executable): return executable

	dir = os.path.dirname(dir)
    print 'File', filename, 'Not found'
    sys.exit(1)
