# SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
# Copyright 2013-present Barefoot Networks, Inc.
#
# SPDX-License-Identifier: Apache-2.0

import os
import sys


class Config(object):
    """
    Config - Configuration data for a 'p4c' tool chain.

    The common configuration includes the argument parser and the path to the
    backend configuration file.

    """

    def __init__(self, config_prefix):
        self.config_prefix = config_prefix or "p4c"
        self.target = []

    def load_from_config(self, path, argParser):
        cfg_globals = dict(globals())
        cfg_globals["config"] = self
        cfg_globals["__file__"] = path
        cfg_globals["argParser"] = argParser

        data = None
        f = open(path)
        try:
            data = f.read()
        except:
            print("error", path)
        f.close()

        try:
            exec(compile(data, path, "exec"), cfg_globals, None)
        except SystemExit:
            e = sys.exc_info()[1]
            if e.args:
                raise
        except:
            import traceback

            print(traceback.format_exc())
