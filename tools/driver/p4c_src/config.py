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

    The common configuration includes the argument parser and the path to the
    backend configuration file.

    """

    def __init__(self, config_prefix):
        self.config_prefix = config_prefix or 'p4c'
        self.target = []

    def load_from_config(self, path, argParser):
        cfg_globals = dict(globals())
        cfg_globals['config'] = self
        cfg_globals['__file__'] = path
        cfg_globals['argParser'] = argParser

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
