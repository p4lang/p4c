# SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
# Copyright 2013-present Barefoot Networks, Inc.
#
# SPDX-License-Identifier: Apache-2.0
""" 'p4c' compiler driver """

__author__ = "Barefoot Networks"
__email__ = "p4c@barefootnetworks.com"
__versioninfo__ = (0, 0, 1)
__version__ = ".".join(str(v) for v in __versioninfo__) + "dev"

__all__ = []

from .main import main
