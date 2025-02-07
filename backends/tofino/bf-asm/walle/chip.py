# Copyright (C) 2024 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License.  You may obtain
# a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
#
# SPDX-License-Identifier: Apache-2.0

"""
TODO: document this file
"""
import struct
from copy import copy

class chip_object(object):
    """
    TODO: docstring
    """
    def __init__(self, addr, src_key):
        self.addr = addr
        self.src_key = src_key

    def add_offset (self, offset):
        self.addr += offset

class direct_reg(chip_object):
    """
    A single register write operation, of the format:

    4 bytes: "\0\0\0R"
    4 bytes: 32-bit PCIe address
    4 bytes: Data

    All fields little-endian
    """

    def __init__(self, addr, value, src_key=None):
        chip_object.__init__(self, addr, src_key)
        self.value=struct.pack("<I",value)
        self.orig_value = value

    def __str__(self):
        # TODO
        return hex(self.addr) + ": " + hex(self.orig_value)

    def deepcopy(self):
        return copy(self)

    def bytes(self):
        return "\0\0\0R" + struct.pack("<I",self.addr) + self.value

class indirect_reg(chip_object):
    """
    A single indirect register write operation, of the format:

    4 bytes: "\0\0\0I"
    8 bytes: 42-bit chip address
    4 bytes: Bit-length of word
    Following: Data

    All fields little-endian
    """
    def __init__(self, addr, value, width, src_key=None):
        chip_object.__init__(self, addr, src_key)
        self.width=width

        hexstr = hex(value).replace('0x','').replace('L','')
        if len(hexstr) % 2 != 0:
            hexstr = '0' + hexstr
        self.value=bytearray.fromhex(hexstr)
        self.value = self.value.rjust(self.width//8,chr(0))
        self.value.reverse()
        self.orig_value = value

    def __str__(self):
        # TODO
        return hex(self.addr) + ": " + hex(self.orig_value)

    def deepcopy(self):
        return copy(self)

    def bytes(self):
        # TODO: for now implement as a sequence of direct register writes
        #return "\0\0\0I" + struct.pack("<Q",self.addr) + struct.pack("<I",self.width) + self.value

        # Make sure to write pieces of the register starting from the
        # most-significant end, to ensure atomicity
        offset = (self.width//8) - 4
        byte_str = ""
        while offset >= 0:
            byte_str += "\0\0\0R" + struct.pack("<I",self.addr+offset) + self.value[offset:offset+4].ljust(4,chr(0))
            offset -= 4

        return byte_str

class dma_block(chip_object):
    """
    A single DMA block write operation in one of these two formats:

    4 bytes: "\0\0\0D"
    8 bytes: 42-bit chip address
    4 bytes: Bit-length of word
    4 bytes: Number of words
    Following: Data, in 32-bit word chunks

    4 bytes: "\0\0\0B"
    8 bytes: 32-bit PCIe address
    4 bytes: Bit-length of word
    4 bytes: Number of words
    Following: Data, in 32-bit word chunks

    All fields little-endian
    """
    def __init__(self, addr, width, src_key=None, is_reg=False):
        chip_object.__init__(self, addr, src_key)
        self.width=width
        self.values=[]
        self.is_reg = is_reg

    def add_word(self, value):
        hexstr = hex(value).replace('0x','').replace('L','')

        if len(hexstr) % 2 != 0:
            hexstr = '0' + hexstr
        self.values.append(bytearray.fromhex(hexstr))
        self.values[-1] = self.values[-1].rjust(self.width//8,chr(0))
        self.values[-1].reverse()

    def __str__(self):
        # TODO
        return hex(self.addr) + ": <TODO>"
    
    def deepcopy(self):
        new = copy(self)
        new.values = new.values[:]
        return new

    def bytes(self):
        if self.width > 128:
            # FIXME: this only works cleanly if width is a multiple of 128, can it be otherwise?
            if self.width % 128 != 0:
                sys.stderr.write("ERROR: register width %d not a multiple of 128" % self.width);
                sys.exit(1)
            new_values = []
            for value in self.values:
                for chunk in range(0, self.width//8, 16):
                    new_values.append(value[chunk:chunk+16].rjust(128//8,chr(0)))
            self.values = new_values
            self.width = 128

        if self.is_reg:
            op_type = "\0\0\0B"
        else:
            op_type = "\0\0\0D"
        bytestr = op_type + struct.pack("<Q",self.addr) + struct.pack("<I",self.width) + struct.pack("<I",len(self.values))
        for value in self.values:
            bytestr += value
        return bytestr
