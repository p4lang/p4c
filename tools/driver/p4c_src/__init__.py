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

""" 'p4c' compiler driver """

__author__ = "Barefoot Networks"
__email__ = "p4c@barefootnetworks.com"
__versioninfo__ = (0, 0, 1)
__version__ = '.'.join(str(v) for v in __versioninfo__) + 'dev'

__all__ = []

from .main import main
