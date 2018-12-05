#! /usr/bin/env python2
#
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

# Example of usign STFRunner that simply prints the STF statement

import argparse
import sys, os, os.path
import logging
import math
import random
import re
import time
import traceback
import unittest
from stf_parser import STFParser
from stf_runner import STFRunner

class STFTest (STFRunner):
    def __init__(self, ast, testname):
        super(STFTest, self).__init__(ast, testname)

    def genAddTableEntry(self, entry):
        table = entry[1].translate(self._transTable)
        priority = entry[2]
        match_list = entry[3]
        action = entry[4][0].translate(self._transTable)
        action_params = entry[4][1]

        self._logger.info("add entry: %s %s", table, str(match_list))

        # bookeeping for aliases (mainly used to name counters)
        if entry[5] is not None:
            match_name, match, mask, hasMask = self.match2spec(table, match_list[0][0], match_list[0][1])
            self._namedEntries[entry[5]] = STFNamedEntry(table, match_list[0][0], match_list[0][1], 0, priority)

    def genAddDefaultAction(self, set_default):
        """
            Generate a default action
        """
        table = set_default[1].translate(self._transTable)
        action = set_default[2][0].translate(self._transTable)
        action_params = set_default[2][1]

        self._logger.info("Set default action %s for table %s", action, table)

    def genSendPacket(self, packet):
        """
            Generate a send_packet statement
        """
        port = int(packet[1])
        payload = packet[2]
        self._logger.info("Sending packet on port %d", port)

    def genExpectPacket(self, expect, orig_packet):
        """
            Generate a check_packet statement
        """
        if (expect[1] is None):
             # expect no_packet
            self._logger.info("Expect no packet")
            return

        port = int(expect[1])
        payload = expect[2]
        if payload is None:
            self._logger.info("Expecting lots of packets on port {}".format(port))
        else:
            self._logger.info("Expecting {} packet on port {}".format(payload, port))

    def genCheckCounter(self, chk):
        """
           Generate a check counter request and verify
        """
        isDirect = True
        counterName = chk[1]
        if str(chk[2]).startswith('$'):
            varName = chk[2][1:]
            assert varName in self._namedEntries, "Invalid named entry " + chk[2]
            table = self._namedEntries[varName]._tableName
            counterIndex = self._namedEntries[varName]._value
            isDirect = True
        else:
            counterIndex = int(chk[2])
            isDirect = False

        self._logger.info("check_counter %s(%s)", counterName, counterIndex)

    def genWaitStmt(self):
        """
        Generate a wait stmt
        """
        self._logger.info("wait")

def get_arg_parser():
    parser = argparse.ArgumentParser(description='Tests STF parsing')
    parser.add_argument('stftest', help='name of STF input file',
                        type=str, action='store')
    return parser



def main():
    args = get_arg_parser().parse_args()
    parser = STFParser()
    stf, errs = parser.parse(filename=args.stftest)
    if errs != 0:
        sys.exit(1)
    testname, ext = os.path.splitext(os.path.basename(args.stftest))
    stftester = STFTest(stf, testname)
    stftester._testfile = args.stftest

    stftester.runTest()


if __name__ == '__main__':
    main()
