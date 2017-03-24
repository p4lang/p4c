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

from __future__ import absolute_import
import sys
import os

class Config(object):
    """
    Config - Configuration data for a 'p4c' tool chain.

    The common configuration data include compiler/assembler name,
    compiler/assembler installation path.
    """

    def __init__(self, config_prefix):
        self.config_prefix = config_prefix or 'p4c'
        self.target = []
        self.steps = {}
        self.init_func = {}
        self.options = {}
        self.toolchain = {}

    def load_from_config(self, path, output_dir, source_file):
        cfg_globals = dict(globals())
        cfg_globals['config'] = self
        cfg_globals['__file__'] = path
        cfg_globals['output_dir'] = output_dir
        cfg_globals['source_fullname'] = source_file
        cfg_globals['source_basename'] = os.path.splitext(os.path.basename(source_file))[0]

        data = None
        f = open(path)
        try:
            data = f.read()
        except:
            print "error", path
        f.close()

        try:
            exec(compile(data, path, 'exec'), cfg_globals, None)
        except SystemExit:
            e = sys.exc_info()[1]
            if e.args:
                raise
        except:
            import traceback
            print traceback.format_exc()

    def add_preprocessor_option(self, backend, option):
        if backend not in self.options:
            self.options[backend] = {}
        if 'preprocessor' not in self.options[backend]:
            self.options[backend]['preprocessor'] = []
        self.options[backend]['preprocessor'].append(option)

    def add_compiler_option(self, backend, option):
        if backend not in self.options:
            self.options[backend] = {}
        if 'compiler' not in self.options[backend]:
            self.options[backend]['compiler'] = []
        self.options[backend]['compiler'].append(option)

    def add_assembler_option(self, backend, option):
        if backend not in self.options:
            self.options[backend] = {}
        if 'assembler' not in self.options[backend]:
            self.options[backend]['assembler'] = []
        self.options[backend]['assembler'].append(option)

    def add_linker_option(self, backend, option):
        if backend not in self.options:
            self.options[backend] = {}
        if 'linker' not in self.options[backend]:
            self.options[backend]['linker'] = []
        self.options[backend]['linker'].append(option)

    def add_toolchain(self, backend, toolchain):
        if backend not in self.toolchain:
            self.toolchain[backend] = {}
        if not isinstance(toolchain, dict):
            print "variable toolchain is not type dict"
            sys.exit(1)
        for k, v in toolchain.iteritems():
            self.toolchain[backend][k] = v

    def add_compilation_steps(self, backend, step):
        self.steps[backend] = step


    def get_compiler(self, backend):
        if 'compiler' in self.toolchain[backend]:
            return self.toolchain[backend]['compiler']
        else:
            return None

    def get_preprocessor(self, backend):
        if 'preprocessor' in self.toolchain[backend]:
            return self.toolchain[backend]['preprocessor']
        else:
            return None

    def get_assembler(self, backend):
        if 'assembler' in self.toolchain[backend]:
            return self.toolchain[backend]['assembler']
        else:
            return None

    def get_linker(self, backend):
        if 'linker' in self.toolchain[backend]:
            return self.toolchain[backend]['linker']
        else:
            return None


