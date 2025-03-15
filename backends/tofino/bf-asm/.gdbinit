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

# vim: ft=python
set print object
set unwindonsignal on
set unwind-on-terminating-exception on

if $_isvoid($bpnum)
    break __assert_fail
    break error
    break bug
end

define d
    call ::dump($arg0)
end


python
def template_split(s):
    parts = []
    bracket_level = 0
    current = []
    for c in (s):
        if c == "," and bracket_level == 1:
            parts.append("".join(current))
            current = []
        else:
            if c == '>':
                bracket_level -= 1
            if bracket_level > 0:
                current.append(c)
            if c == '<':
                bracket_level += 1
    parts.append("".join(current))
    return parts

def vec_begin(vec):
    return vec['_M_impl']['_M_start']
def vec_end(vec):
    return vec['_M_impl']['_M_finish']
def vec_size(vec):
    return int(vec_end(vec) - vec_begin(vec))
def vec_at(vec, i):
    return (vec_begin(vec) + i).dereference()

class bitvecPrinter(object):
    "Print a bitvec"
    def __init__(self, val):
        self.val = val
    def to_string(self):
        data = self.val['data']
        rv = ""
        size = self.val['size']
        ptr = self.val['ptr']
        unitsize = ptr.type.target().sizeof * 8
        while size > 1:
            data = ptr.dereference()
            i = 0
            while i < unitsize:
                if (rv.__len__() % 120 == 119): rv += ':'
                elif (rv.__len__() % 30 == 29): rv += ' '
                elif (rv.__len__() % 6 == 5): rv += '_'
                if (data & 1) == 0:
                    rv += "0"
                else:
                    rv += "1"
                data >>= 1
                i += 1
            ptr += 1
            size -= 1
            data = ptr.dereference()
        while rv == "" or data > 0:
            if (rv.__len__() % 120 == 119): rv += ':'
            elif (rv.__len__() % 30 == 29): rv += ' '
            elif (rv.__len__() % 6 == 5): rv += '_'
            if (data & 1) == 0:
                rv += "0"
            else:
                rv += "1"
            data >>= 1
        return rv
class value_t_Printer(object):
    "Print a value_t"
    def __init__(self, val):
        self.val = val
    def to_string(self):
        typ = self.val['type']
        if typ == 0:  # tINT
            return str(self.val['i'])
        elif typ == 1:  # tBIGINT
            v = self.val['bigi']
            data = v['data']
            size = v['size']
            val = 0
            while size > 0:
                val <<= 64
                val += data.dereference()
                size -= 1
                data += 1
            return str(val)
        elif typ == 2:  # tRANGE
            return str(self.val['lo']) + '..' + str(self.val['hi'])
        elif typ == 3:  # tSTR
            return self.val['s']
        elif typ == 4:  # tMATCH
            return self.val['m']
        elif typ == 5:  # tBIGMATCH
            return self.val['bigm']
        elif typ == 6:  # tVEC
            return "vector of %d elements" % self.val['vec']['size']
        elif typ == 7:  # tMAP
            return "map of %d elements" % self.val['map']['size']
        elif typ == 8:  # tCMD
            cmd = self.val['vec']['data']
            count = self.val['vec']['size']
            rv = str(cmd.dereference())
            rv += "("
            while count > 1:
                count -= 1
                cmd += 1
                rv += str(cmd.dereference())
                if count > 1:
                    rv += ", "
            rv += ")"
            return rv;
        else:
            return "<value_t type " + hex(typ) + ">"
    class _vec_iter:
        def __init__(self, data, size):
            self.data = data
            self.size = size
            self.counter = -1
        def __iter__(self):
            return self
        def __next__(self):
            self.counter += 1
            if self.counter >= self.size:
                raise StopIteration
            item = self.data.dereference()
            self.data += 1
            return ("[%d]" % self.counter, item)
        def next(self): return self.__next__()
    class _map_iter:
        def __init__(self, data, size):
            self.data = data
            self.size = size
        def __iter__(self):
            return self
        def __next__(self):
            self.size -= 1
            if self.size < 0:
                raise StopIteration
            item = self.data.dereference()
            self.data += 1
            return ("[" + str(item['key']) + "]", item['value'])
        def next(self): return self.__next__()

    class _not_iter:
        def __init__(self):
            pass
        def __iter__(self):
            return self
        def __next__(self):
            raise StopIteration
        def next(self): return self.__next__()
    def children(self):
        typ = self.val['type']
        if typ == 6:
            vec = self.val['vec']
            return self._vec_iter(vec['data'], vec['size'])
        elif typ == 7:
            map = self.val['map']
            return self._map_iter(map['data'], map['size'])
        else:
            return self._not_iter()
class value_t_VECTOR_Printer(object):
    "Print a VECTOR(value_t)"
    def __init__(self, val):
        self.val = val
    def to_string(self):
        return "vector of %d elements" % self.val['size']
    class _iter:
        def __init__(self, data, size):
            self.data = data
            self.size = size
            self.counter = -1
        def __iter__(self):
            return self
        def __next__(self):
            self.counter += 1
            if self.counter >= self.size:
                raise StopIteration
            item = self.data.dereference()
            self.data += 1
            return ("[%d]" % self.counter, item)
        def next(self): return self.__next__()
    def children(self):
        return self._iter(self.val['data'], self.val['size'])
class pair_t_VECTOR_Printer(object):
    "Print a VECTOR(pair_t)"
    def __init__(self, val):
        self.val = val
    def to_string(self):
        return "map of %d elements" % self.val['size']
    class _iter:
        def __init__(self, data, size):
            self.data = data
            self.size = size
        def __iter__(self):
            return self
        def __next__(self):
            self.size -= 1
            if self.size < 0:
                raise StopIteration
            item = self.data.dereference()
            self.data += 1
            return ("[" + str(item['key']) + "]", item['value'])
        def next(self): return self.__next__()
    def children(self):
        return self._iter(self.val['data'], self.val['size'])
class ordered_map_Printer:
    "Print an ordered_map<>"
    def __init__(self, val):
        self.val = val
        self.args = template_split(val.type.tag)
        self.eltype = gdb.lookup_type('std::pair<' + self.args[0] + ' const,' + self.args[1] + '>')
    def to_string(self):
        it = self.val['data']['_M_impl']['_M_node']['_M_next']
        e = self.val['data']['_M_impl']['_M_node'].address
        if it == e:  # empty map
            return "{}"
        else:
            return None
    class _iter:
        def __init__(self, eltype, it, e):
            self.eltype = eltype
            self.it = it
            self.e = e
        def __iter__(self):
            return self
        def __next__(self):
            if self.it == self.e:
                raise StopIteration
            el = (self.it + 1).cast(self.eltype.pointer()).dereference()
            self.it = self.it.dereference()['_M_next']
            return ("[" + str(el['first']) + "]", el['second']);
        def next(self): return self.__next__()
    def children(self):
        return self._iter(self.eltype, self.val['data']['_M_impl']['_M_node']['_M_next'],
                          self.val['data']['_M_impl']['_M_node'].address)
class InputXbar_Group_Printer:
    "Print an InputXbar::Group"
    def __init__(self, val):
        self.val = val
    def to_string(self):
        types = [ 'invalid', 'exact', 'ternary', 'byte', 'gateway', 'xcmp' ]
        t = int(self.val['type'])
        if t >= 0 and t < len(types):
            rv = types[t]
        else:
            rv = '<bad type 0x%x>' % int(self.val['type'])
        rv += ' group ' + str(self.val['index'])
        return rv
class ActionBusSource_Printer:
    "Print an ActionBusSource"
    def __init__(self, val):
        self.val = val
    def to_string(self):
        try:
            types = [ "None", "Field", "HashDist", "HashDistPair", "RandomGen",
                      "TableOutput", "TableColor", "TableAddress", "Ealu", "XCmp",
                      "NameRef", "ColorRef", "AddressRef" ]
            t = int(self.val['type'])
            if t >= 0 and t < len(types):
                rv = types[t]
            else:
                rv = '<bad type 0x%x>' % int(self.val['type'])
            if t == 9:  # XCMP on one line without children
                rv += "[" + str(self.val['xcmp_group']) + ":" + str(self.val['xcmp_byte']) + "]"
        except Exception as e:
            rv += "{crash: "+str(e)+"}"
        return rv
    class _iter:
        def __init__(self, val, type):
            self.val = val
            self.type = type
            self.count = 0
        def __iter__(self):
            return self
        def __next__(self):
            self.count = self.count + 1
            if self.type == 3:
                if self.count == 1:
                    return ("hd1", self.val['hd1'])
                elif self.count == 2:
                    return ("hd2", self.val['hd2'])
                else:
                    raise StopIteration
            #elif self.type == 9:
            #    XCmp on one line without children
            #    if self.count == 1:
            #        return ("group", self.val['xcmp_group'])
            #    elif self.count == 2:
            #        return ("byte", self.val['xcmp_byte'])
            elif self.count > 1:
                raise StopIteration
            elif self.type == 1:
                return ("field", self.val['field'].dereference())
            elif self.type == 2:
                return ("hd", self.val['hd'])
            elif self.type == 4:
                return ("rng", self.val['rng'])
            elif self.type == 5 or self.type == 6 or self.type == 7:
                return ("table", self.val['table'])
            elif self.type == 10 or self.type == 11 or self.type == 12:
                return ("name_ref", self.val['name_ref'])
            raise StopIteration
        def next(self): return self.__next__()
    def children(self):
        return self._iter(self.val, int(self.val['type']))

class PhvRef_Printer:
    "Print a Phv::Ref"
    def __init__(self, val):
        self.val = val
    def to_string(self):
        threads = [ "ig::", "eg::", "gh::" ]
        rv = threads[self.val['gress_']] + str(self.val['name_'])
        if self.val['lo'] >= 0:
            rv += '(' + str(self.val['lo'])
            if self.val['hi'] >= 0:
                rv += '..' + str(self.val['hi'])
            rv += ')'
        return rv

class Mem_Printer:
    "Print a MemUnit or subclass"
    def __init__(self, val, big, small):
        self.val = val
        self.big = big
        self.small = small
    def to_string(self):
        if self.val['stage'] > -32768:
            return "%s(%d,%d,%d)" % (self.big, self.val['stage'], self.val['row'], self.val['col'])
        if self.val['row'] >= 0:
            return "%s(%d,%d)" % (self.big, self.val['row'], self.val['col'])
        return "%s(%d)" % (self.small, self.val['col'])

def bfas_pp(val):
    if val.type.tag == 'bitvec':
        return bitvecPrinter(val)
    if val.type.tag == 'value_t':
        return value_t_Printer(val)
    if val.type.tag == 'value_t_VECTOR':
        return value_t_VECTOR_Printer(val)
    if val.type.tag == 'pair_t_VECTOR':
        return pair_t_VECTOR_Printer(val)
    if str(val.type.tag).startswith('ordered_map<'):
        return ordered_map_Printer(val)
    if val.type.tag == 'InputXbar::Group':
        return InputXbar_Group_Printer(val)
    if val.type.tag == 'ActionBusSource':
        return ActionBusSource_Printer(val)
    if val.type.tag == 'Phv::Ref':
        return PhvRef_Printer(val)
    if val.type.tag == 'SRamMatchTable::Ram':
        return Mem_Printer(val, 'Ram', 'Lamb')
    if val.type.tag == 'MemUnit':
        return Mem_Printer(val, 'Mem', 'Mem')
    return None

try:
    found = False
    for i in range(len(gdb.pretty_printers)):
        try:
            if gdb.pretty_printers[i].__name__ == "bfas_pp":
                gdb.pretty_printers[i] = bfas_pp
                found = True
        except:
            pass
    if not found:
        gdb.pretty_printers.append(bfas_pp)
except:
    pass

end
