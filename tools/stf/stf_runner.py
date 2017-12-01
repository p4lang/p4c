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

# Template class for writing tests that use STF as input
# The intent is to subclass the STFRunner class and implement the methods
# for specific actions. See an example in stf_test.py which just prints
# the STF statements.

import logging
from string import maketrans

class STFNamedEntry:
    """
    A class that encapsulates a named table entry stored in an 'env var'
    used mainly for counters.
    """
    def __init__(self, tableName, fieldName, value, mask, priority):
        self._tableName = tableName
        self._fieldName = fieldName
        self._value = value
        self._mask = mask
        self._priority = priority

class STFRunner(object):
    """
    An abstract class that processes an STF parsed tree
    """

    def __init__(self, ast, testname):
        testNameEscapeChars = "-"
        testNameEscapes = "_"*len(testNameEscapeChars)
        self._transTable = maketrans(testNameEscapeChars,testNameEscapes)
        self._testname = testname.translate(self._transTable)
        logging.basicConfig(level=logging.INFO)
        self._logger = logging.getLogger(self._testname)
        self._ast = ast
        self._namedEntries = {}

    def runTest(self):
        self._logger.info("Starting STF test")
        try:
            pkt_expects = []
            for s in self._ast:
                self._logger.info("Processing STF statement: %s", s)
                if s[0] == 'packet':
                    self.genSendPacket(s)
                elif s[0] == 'expect':
                    pkt_expects.append(s)
                elif s[0] == 'add':
                    self.genAddTableEntry(s)
                elif s[0] == 'wait':
                    self.genWaitStmt()
                elif s[0] == 'setdefault':
                    self.genAddDefaultAction(s)
                elif s[0] == 'check_counter':
                    self.genCheckCounter(s)
                elif s[0] == 'remove':
                    self._logger.info("Flushing table entries")
                    self.undo_write_requests(self._requests)
                else:
                    assert False, "Unknown statement %s" % s[0]

        except Exception as e:
            self._logger.exception(e)
            raise
        else:
            # flush the queue of expects
            for e in pkt_expects:
                 self.genExpectPacket(e, None)
            pkt_expects = []
            self.testEnd()
        finally:
            self.testCleanup()
            self._logger.info("End STF Test")


    def genAddTableEntry(self, entry):
        """
            Generate a table entry
        """
        return


    def genAddDefaultAction(self, set_default):
        """
            Generate a default action
        """
        return

    def genSendPacket(self, packet):
        """
            Generate a send_packet statement
        """
        return

    def genExpectPacket(self, expect, orig_packet):
        """
            Generate a check_packet statement
        """
        return

    def genProcessPacket(self, pkt_pair):
        """
            Generate a send_packet/check_packet pair.
        """
        if pkt_pair[0] is not None:
            self.genSendPacket(pkt_pair[0])
            if pkt_pair[1] is not None: self.genExpectPacket(pkt_pair[1], pkt_pair[0])

    def genCheckCounter(self, chk):
        """
           Generate a check counter request and verify
        """
        return

    def genWaitStmt(self):
        """
        Generate a wait stmt
        """
        return

    def testEnd(self):
        """
        invoked at the end of the test when successful
        """
        return

    def testCleanup(self):
        """
        invoked at the end of the test, in the finally clause
        """
        self._logger.info("Cleanup")
        return

    def setExpectTern(self, expect, packet):
        """
            Set the expect packet don't care to packet data since
            the packet comparison looks byte-by-byte
        """
        val = ''
        for e, p in zip(expect, packet):
            if e == '*': val += p
            else: val += e
        return val
