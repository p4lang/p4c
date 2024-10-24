# Copyright (C) 2024 Intel Corporation
# 
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy
# of the License at
# 
# http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software distributed
# under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations under the License.
# 
# SPDX-License-Identifier: Apache-2.0

# vi: ft=python
set print static-members off
set print object
set unwindonsignal on
set unwind-on-terminating-exception on
set python print-stack full

if $_isvoid($bpnum)
    break ErrorReporter::emit_message
    disable
    break Util::P4CExceptionBase::P4CExceptionBase<>
    break Util::P4CExceptionBase::traceCreation
    break BaseCompileContext::getDefaultErrorDiagnosticAction
end

define pn
    if $argc == 2
        call ::dbprint($arg0 $arg1)
    else
        call ::dbprint($arg0)
    end
end
document pn
        print a IR::Node pointer
end
define d
    if $argc == 2
        call ::dump($arg0 $arg1)
    else
        call ::dump($arg0)
    end
end
document d
        dump IR::Node tree or Visitor::Context
end
define dnt
    if $argc == 2
        call ::dump_notype($arg0 $arg1)
    else
        call ::dump_notype($arg0)
    end
end
document dnt
        dump IR::Node tree, skipping 'type' fields
end
define dsrc
    if $argc == 2
        call ::dump_src($arg0 $arg1)
    else
        call ::dump_src($arg0)
    end
end
document dsrc
        dump IR::Node tree, skipping 'type' fields, and printing source info
end

python
import sys
import re

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

TYPE_CACHE = {}

def lookup_type(typename):
    """Return a gdb.Type object represents given `typename`.
    For example, x.cast(ty('Buffer'))"""
    if typename in TYPE_CACHE:
        return TYPE_CACHE[typename]

    m0 = re.match(r"^(.*\S)\s*const$", typename)
    m1 = re.match(r"^(.*\S)\s*[*|&]$", typename)
    if m0 is not None:
        tp = lookup_type(m0.group(1))
    elif m1 is None:
        tp = gdb.lookup_type(typename)
    else:
        if m1.group(0).endswith('*'):
            tp = lookup_type(m1.group(1)).pointer()
        else:
            tp = lookup_type(m1.group(1)).reference()
    TYPE_CACHE[typename] = tp
    return tp

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

class ordered_set_Printer:
    "Print an ordered_set<>"
    def __init__(self, val):
        self.val = val
        self.args = template_split(val.type.tag)
        self.eltypestr = self.args[0]
        self.eltype = lookup_type(self.args[0])
        self.isptr = True if re.match(r"^.*[*]$", self.args[0]) else False
    def to_string(self):
        return "ordered_set<..>"
    class _iter:
        def __init__(self, eltype, eltypestr, isptr, it, e):
            self.eltype = eltype
            self.eltypestr = eltypestr
            self.isptr = isptr
            self.it = it
            self.e = e
            self.idx = 0
        def __iter__(self):
            return self
        def __next__(self):
            if self.it == self.e:
                raise StopIteration
            el = (self.it + 1).cast(self.eltype.pointer()).dereference()
            self.it = self.it.dereference()['_M_next']
            idx = self.idx
            self.idx = idx + 1
            if self.isptr:
                return ("[%d]" % idx, "(" + self.eltypestr + ")" + str(el))
            return ("[%d]" % idx, el);
        def next(self): return self.__next__()
    def children(self):
        return self._iter(self.eltype, self.eltypestr, self.isptr,
                          self.val['data']['_M_impl']['_M_node']['_M_next'],
                          self.val['data']['_M_impl']['_M_node'].address)

class ActionFormatUsePrinter(object):
    "Print an ActionFormat::Use"
    def __init__(self, val):
        self.val = val
    def to_string(self):
        rv = ""
        try:
            rv = "ActionFormat::Use"
        except Exception as e:
            rv += "{crash: "+str(e)+"}\n"
        return rv

class cstringPrinter(object):
    "Print a cstring"
    def __init__(self, val):
        self.val = val
    def to_string(self):
        if self.val['str']:
            return str(self.val['str'])
        else:
            return "nullptr"

class SourceInfoPrinter(object):
    "Print a Util::SourceInfo"
    def __init__(self, val):
        self.val = val
    def to_string(self):
        return (str(self.val['start']['lineNumber']) + ':' +
                str(self.val['start']['columnNumber']) + '-' +
                str(self.val['end']['lineNumber']) + ':' +
                str(self.val['end']['columnNumber']))

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

class MemoriesPrinter(object):
    "Print a Memories object"
    def __init__(self, val):
        self.val = val
    def to_string(self):
        rv = "\ntc  sb  rb  tib ab  st  srams       mapram  ov  gw pay 2p\n"
        tables = {}
        alloc_names = [ 'tcam_use', 'sram_print_search_bus', 'sram_print_result_bus',
            'tind_bus', 'action_data_bus', 'stash_use', 'sram_use', 'mapram_use',
            'overflow_bus', 'gateway_use', 'payload_use' ]
        ptrs = [ self.val[arr]['data'] for arr in alloc_names ]
        rows = [ self.val[arr]['nrows'] for arr in alloc_names ]
        cols = [ self.val[arr]['ncols'] for arr in alloc_names ]
        ptrs.append(self.val['twoport_bus']['data'])
        rows.append(self.val['twoport_bus']['size'])
        cols.append(1)

        for r in range(0, max(rows)):
            for t in range(0, len(ptrs)) :
                if r >= rows[t]:
                    break
                for c in range(0, cols[t]):
                    tbl = ptrs[t].dereference()['str']
                    if tbl:
                        name = tbl.string()
                        if name not in tables:
                            tables[name] = chr(ord('A') + len(tables))
                        rv += tables[name]
                    else:
                        rv += '.'
                    ptrs[t] += 1
                rv += '  '
            rv += "\n"
        for tbl,k in tables.items():
            rv += "   " + k + " " + tbl + "\n"
        return rv

class TofinoIXBarPrinter(object):
    "Print a Tofino::IXBar object"
    def __init__(self, val):
        self.val = val
    def to_string(self):
        rv = "\nexact ixbar groups                  ternary ixbar groups\n"
        fields = {}
        ptrs = [ self.val['exact_use']['data'],
                 self.val['ternary_use']['data'],
                 self.val['byte_group_use']['data'] ]
        cols = [ int(self.val['exact_use']['ncols']),
                 int(self.val['ternary_use']['ncols']), 1 ]
        indir = [ 0, 1, 2, 1 ]
        pfx = [ "", "   ", " ", " " ]
        for r in range(0, 8):
            for t in range(0, len(indir)):
                if r > 5 and t > 0:
                    break
                rv += pfx[t]
                for c in range(0, cols[indir[t]]):
                    field = ptrs[indir[t]].dereference()
                    if c == 8:
                        rv += ' '
                    if field['first']['type_']['size_'] != 0:
                        name = str(field['first'])
                        if name not in fields:
                            if (len(fields) > 26):
                                fields[name] = chr(ord('A') + len(fields) - 26)
                            else:
                                fields[name] = chr(ord('a') + len(fields))
                        rv += fields[name]
                        rv += hex(int(field['second'])//8)[2]
                    else:
                        rv += '..'
                    ptrs[indir[t]] += 1
            rv += "\n"
        for field,k in fields.items():
            rv += "   " + k + " " + field + "\n"

        rv += "hash select bits   G  hd  /- hash dist bits ---\n"
        tables = {}
        ptrs = [ self.val['hash_index_use']['data'],
                 self.val['hash_single_bit_use']['data'],
                 self.val['hash_group_print_use']['data'],
                 self.val['hash_dist_use']['data'],
                 self.val['hash_dist_bit_use']['data'] ]
        cols = [ int(self.val['hash_index_use']['ncols']),
                 int(self.val['hash_single_bit_use']['ncols']),
                 1, int(self.val['hash_dist_use']['ncols']),
                 int(self.val['hash_dist_bit_use']['ncols']) ]
        pfx = [ "", " ", "  ", "  ", " " ]
        for r in range(0, 16):
            for t in range(0, len(ptrs)):
                rv += pfx[t]
                if r > 8 and t == 2:
                    rv += ' '
                    continue
                for c in range(0, cols[t]):
                    tbl = ptrs[t].dereference()
                    if tbl['str']:
                        name = tbl['str'].string()
                        if name not in tables:
                            tables[name] = chr(ord('A') + len(tables))
                        rv += tables[name]
                    else:
                        rv += '.'
                    ptrs[t] += 1
            rv += "\n"
        for tbl,k in tables.items():
            rv += "   " + k + " " + tbl + "\n"
        return rv;

def vec_begin(vec):
    return vec['_M_impl']['_M_start']
def vec_end(vec):
    return vec['_M_impl']['_M_finish']
def vec_size(vec):
    return int(vec_end(vec) - vec_begin(vec))
def vec_at(vec, i):
    return (vec_begin(vec) + i).dereference()

class safe_vector_Printer:
    "Print a safe_vector<> or dyn_vector"
    def __init__(self, val):
        self.val = val
    def to_string(self):
        return None if vec_size(self.val) > 0 else "[]"
    def display_hint(self):
        return 'array'
    class _iter:
        def __init__(self, val):
            self.val = val
            self.size = vec_size(val)
            self.idx = 0
        def __iter__(self):
            return self
        def __next__(self):
            if self.idx >= self.size:
                raise StopIteration
            idx = self.idx
            self.idx = idx + 1
            return ("[%d]" % idx, vec_at(self.val, idx));
        def next(self): return self.__next__()
    def children(self):
        return self._iter(self.val)

def bvec_size(vec):
    sz = int(vec_end(vec)['_M_p'] - vec_begin(vec)['_M_p'])
    sz = sz * vec_begin(vec)['_M_p'].type.target().sizeof * 8
    sz = sz + int(vec_end(vec)['_M_offset'] - vec_begin(vec)['_M_offset'])
    return sz

class safe_vector_bool_Printer:
    "Print a safe_vector<bool> or dyn_vector<bool>"
    def __init__(self, val):
        self.val = val
    def to_string(self):
        return "" if bvec_size(self.val) > 0 else "[]"
    class _iter:
        def __init__(self, val):
            self.val = val
            self.end = vec_end(val)
            self.ptr = vec_begin(val)['_M_p']
            self.offset = vec_begin(val)['_M_offset']
            self.idx = 0
        def __iter__(self):
            return self
        def __next__(self):
            if self.ptr == self.end['_M_p'] and self.offset >= self.end['_M_offset']:
                raise StopIteration
            ptr = self.ptr
            offset = self.offset
            idx = self.idx
            self.offset = offset + 1
            self.idx = idx + 1
            if self.offset == self.ptr.type.target().sizeof * 8:
                self.offset = 0
                self.ptr = self.ptr + 1
            return ("[%d]" % idx, (self.ptr.dereference() >> self.offset) & 1);
        def next(self): return self.__next__()
    def children(self):
        return self._iter(self.val)

class MemoriesUsePrinter(object):
    "Print a Memories::Use object"
    def __init__(self, val):
        self.val = val
    def to_string(self):
        types = [ 'EXACT', 'ATCAM', 'TERNARY', 'GATEWAY', 'TIND', 'IDLETIME', 'COUNTER',
                  'METER', 'SELECTOR', 'STATEFUL', 'ACTIONDATA' ]
        t = int(self.val['type'])
        if t >= 0 and t < len(types):
            t = types[t]
        else:
            t = "<type " + str(t) + ">"
        rv = "MemUse " + t + "("
        rows = vec_size(self.val['row'])
        for i in range(0, rows):
            if i > 0:
                rv += "; "
            row = vec_at(self.val['row'], i)
            rv += str(row['row'])
            if row['bus'] >= 0:
                rv += "," + str(row['bus'])
            if row['result_bus'] >= 0:
                rv += ",rb" + str(row['result_bus'])
            rv += ": "
            for j in range(0, vec_size(row['col'])):
                if j > 0:
                    rv += ','
                rv += str(vec_at(row['col'], j))
            for j in range(0, vec_size(row['mapcol'])):
                if j > 0:
                    rv += ','
                else:
                    rv += " +"
                rv += str(vec_at(row['mapcol'], j))
        if self.val['gateway']['unit'] >= 0:
            rv += " payload " + str(self.val['gateway']['payload_row']) + ":"
            rv += str(self.val['gateway']['payload_unit']) + ":"
            rv += str(self.val['gateway']['unit']) + " = "
            rv += str(self.val['gateway']['payload_value'])
            if self.val['gateway']['payload_match_address'] >= 0:
                rv += ", match_address = " + str(self.val['gateway']['payload_match_address'])
        rv += ')'
        return rv

class IXBarUseBytePrinter(object):
    "Print an IXBar::Use::Byte object"
    def __init__(self, val):
        self.val = val
    def to_string(self):
        rv = ""
        rv += str(self.val['container'])
        rv += "[" + str(self.val['lo']) + "]("
        rv += str(self.val['loc']['group']) + ','
        rv += str(self.val['loc']['byte']) + ')'
        if int(self.val['flags']) != 0:
            rv += " flags=" + hex(int(self.val['flags']))
        if int(self.val['bit_use']['data']) != 255:
            rv += " bit_use=" + str(self.val['bit_use']);
        fields = self.val['field_bytes']
        for i in range(0, vec_size(fields)):
            field_info = vec_at(fields, i)
            rv += " " + field_info['field']['str'].string()
            rv += "(" + str(field_info['lo']) + ".." + str(field_info['hi']) + ")"
        if int(self.val['search_bus']) >= 0:
            rv += " search_bus=" + str(int(self.val['search_bus']))
        if int(self.val['match_index']) >= 0:
            rv += " match_index=" + str(int(self.val['match_index']))
        return rv

class IXBarUsePrinter(object):
    "Print an IXBar::Use object"
    use_types = [ "Exact", "ATCam", "Ternary", "Trie", "Gateway", "Action",
        "ProxyHash", "Selector", "Meter", "StatefulAlu", "HashDist" ]
    hash_dist_use_types = [ "CounterAdr", "MeterAdr", "MeterAdrAndImmed", "ActionAdr",
                            "Immed", "PreColor", "HashMod" ]
    def __init__(self, val):
        self.val = val
    def use_array(self, arr, indent):
        rv = ""
        for i in range(0, vec_size(arr)):
            rv += "\n" + indent + "use[" + str(i) +"]: "
            byte = vec_at(arr, i)
            rv += IXBarUseBytePrinter(byte).to_string()
        return rv;
    def to_string(self):
        rv = ""
        try:
            type_tag = int(self.val['type'])
            rv = "<type %d>" % type_tag
            if type_tag >= 0 and type_tag < len(self.use_types):
                rv = self.use_types[type_tag]
            if any(field.name == 'hash_dist_type' for field in self.val.type.fields()):
                type_tag = int(self.val['hash_dist_type'])
                if type_tag >= 0 and type_tag < len(self.hash_dist_use_types):
                    rv += "/" + self.hash_dist_use_types[type_tag]
            rv += self.use_array(self.val['use'], '   ')
            if any(field.name == 'hash_table_inputs' for field in self.val.type.fields()):
                for i in range(0, vec_size(self.val['hash_table_inputs'])):
                    hti = vec_at(self.val['hash_table_inputs'], i)
                    if hti != 0:
                        rv += "\n   hash_group[%d]: " % i
                        j = 0
                        while hti > 0:
                            if hti % 2 != 0:
                                rv += "%d " % j
                            j += 1
                            hti /= 2
            if any(field.name == 'bit_use' for field in self.val.type.fields()):
                for i in range(0, vec_size(self.val['bit_use'])):
                    rv += "\n   bit_use[" + str(i) +"]: "
                    bits = vec_at(self.val['bit_use'], i)
                    rv += str(bits['group']) + ':' + str(bits['bit']+40) + ': '
                    rv += bits['field']['str'].string()
                    rv += "(" + str(bits['lo'])
                    if bits['width'] > 1:
                        rv += ".." + str(bits['lo'] + bits['width'] - 1)
                    rv += ")"
            if any(field.name == 'way_use' for field in self.val.type.fields()):
                for i in range(0, vec_size(self.val['way_use'])):
                    rv += "\n   way_use[" + str(i) +"]: "
                    way = vec_at(self.val['way_use'], i)
                    rv += "%d,[%d:%d],[%d:%d],%x" % (int(way['source']),
                        int(way['index']['hi']), int(way['index']['lo']),
                        int(way['select']['hi']), int(way['select']['lo']),
                        int(way['select_mask']))
            if any(field.name == 'xme_units' for field in self.val.type.fields()):
                rv += "\n  xme_units = 0x%x" % int(self.val['xme_units'])
            if any(field.name == 'num_gw_rows' for field in self.val.type.fields()):
                if int(self.val['num_gw_rows']) > 0:
                    rv += " gw = %d..%d" % ( int(self.val['first_gw_row']),
                        int(self.val['first_gw_row']) + int(self.val['num_gw_rows']) - 1)
            #for i in range(0, vec_size(self.val['select_use'])):
            #    rv += "\n   sel_use[" + str(i) +"]: "
            #    sel = vec_at(self.val['select_use'], i)
            #    rv += "%d:%x - %s" % (int(sel['group']), int(sel['bit_mask']), str(sel['alg']))
            if any(field.name == 'meter_alu_hash' for field in self.val.type.fields()):
                mah = self.val['meter_alu_hash']
                if mah['allocated']:
                    rv += "\n   meter_alu_hash: group=" + str(mah['group'])
            if any(field.name == 'hash_dist_hash' for field in self.val.type.fields()):
                hdh = self.val['hash_dist_hash']
                if hdh['allocated']:
                    rv += "\n   hash_dist_hash: "
                    rv += " group=" + str(hdh['group'])
                    rv += " gm_bits=" + str(hdh['galois_matrix_bits'])
                    rv += " algorithm=" + str(hdh['algorithm'])
            if type_tag == 9 and any(field.name == 'salu_input_source' for field in self.val.type.fields()):
                sis = self.val['salu_input_source']
                rv += "\n   data_bytemask=%x hash_bytemask=%x" % (
                            sis['data_bytemask'], sis['hash_bytemask'])
            rv += "\n"
        except Exception as e:
            rv += "{crash: "+str(e)+"}\n"
        return rv;

class PHVBitPrinter(object):
    "Print a PHV::Bit object"
    def __init__(self, val):
        self.val = val
    def to_string(self):
        return self.val['first']['str'].string() + "[" + str(self.val['second']) + "]"


class PHVContainerPrinter(object):
    "Print a PHV::Container object"
    container_kinds = [ "T", "D", "M", "" ]
    container_sizes = { 8: "B", 16: "H", 32: "W" }
    def __init__(self, val):
        self.val = val
    def to_string(self):
        tk = int(self.val['type_']['kind_'])
        ts = int(self.val['type_']['size_'])
        if ts == 0:
            return "<U>";  # unused in an ixbar alloc, elsewhere?
        elif tk < 0 or tk >= len(self.container_kinds) or not ts in self.container_sizes:
            return "<invalid PHV::Container>"
        rv = self.container_kinds[tk] + self.container_sizes[ts] + str(self.val['index_'])
        return rv;

class PHVFieldSlicePrinter(object):
    "Print a PHV::FieldSlice object"
    def __init__(self, val):
        self.val = val
    def to_string(self):
        field = self.val['field_i']
        lo = self.val['range_i']['lo']
        hi = self.val['range_i']['hi']
        if field == 0:
            return "<invalid>"
        rv = str(field['name']['str'])
        if lo > 0 or hi != -1:
            rv += '['
            if hi != -1: rv += str(hi)
            rv += ':'
            if lo != -1: rv += str(lo)
            rv += ']'
        return rv

class SlicePrinter(object):
    "Print a Slice object"
    def __init__(self, val):
        self.val = val
    def to_string(self):
        field = self.val['field']
        if field == 0:
            name = PHVContainerPrinter(self.val['reg']).to_string()
        else:
            name = str(field['name']['str'])
        return name + '(' + str(self.val['lo']) + ".." + str(self.val['hi']) + ')'

class UniqueIdPrinter(object):
    "Print a UniqueId object"
    speciality = [ "", "$stcam", "$dleft" ]
    a_id_type = [ "", "tind", "idletime", "stats", "meter", "selector", "salu", "action_data" ]
    def __init__(self, val):
        self.val = val
    def to_string(self):
        rv = self.val['name']['str'].string()
        if self.val['speciality'] >= 0 and self.val['speciality'] < len(self.speciality):
            rv += self.speciality[self.val['speciality']]
        else:
            rv += "$spec<" + str(self.val['speciality']) + ">";
        if self.val['logical_table'] != -1:
            rv += "$lt" + str(self.val['logical_table'])
        if self.val['stage_table'] != -1:
            rv += "$st" + str(self.val['stage_table'])
        if self.val['is_gw'] != 0:
            rv += "$gw"
        a_id = self.val['a_id']
        if a_id['type'] != 0:
            if a_id['type'] > 0 and a_id['type'] < len(self.a_id_type):
                rv += "$" + self.a_id_type[a_id['type']]
            else:
                rv += "$a<" + str(a_id['type']) + ">"
            if a_id['name']['str']:
                rv += "." + a_id['name']['str'].string()
        return rv

def bfp4c_pp(val):
    if str(val.type.tag).startswith('ordered_map<'):
        return ordered_map_Printer(val)
    if str(val.type.tag).startswith('ordered_set<'):
        return ordered_set_Printer(val)
    if str(val.type.tag).startswith('safe_vector<bool'):
        return safe_vector_bool_Printer(val)
    if str(val.type.tag).startswith('dyn_vector<bool'):
        return safe_vector_bool_Printer(val)
    if str(val.type.tag).startswith('safe_vector<'):
        return safe_vector_Printer(val)
    if str(val.type.tag).startswith('dyn_vector<'):
        return safe_vector_Printer(val)
    if val.type.tag == 'ActionFormat::Use':
        return ActionFormatUsePrinter(val)
    if val.type.tag == 'bitvec':
        return bitvecPrinter(val)
    if val.type.tag == 'cstring':
        return cstringPrinter(val)
    if val.type.tag == 'Tofino::IXBar':
        return TofinoIXBarPrinter(val)
    if val.type.tag == 'Tofino::IXBar::Use':
        return IXBarUsePrinter(val)
    if str(val.type.tag).endswith('IXBar::Use'):
        return IXBarUsePrinter(val)
    if str(val.type.tag).endswith('IXBar::Use::Byte'):
        return IXBarUseBytePrinter(val)
    if str(val.type.tag).endswith('Memories'):
        return MemoriesPrinter(val)
    if str(val.type.tag).endswith('Memories::Use'):
        return MemoriesUsePrinter(val)
    if val.type.tag == 'Util::SourceInfo':
        return SourceInfoPrinter(val)
    if val.type.tag == 'PHV::Bit':
        return PHVBitPrinter(val)
    if val.type.tag == 'PHV::Container':
        return PHVContainerPrinter(val)
    if val.type.tag == 'PHV::FieldSlice':
        return PHVFieldSlicePrinter(val)
    if val.type.tag == 'Slice':
        return SlicePrinter(val)
    if val.type.tag == 'UniqueId':
        return UniqueIdPrinter(val)
    if val.type.tag == 'attached_entries_t':  # is an ordered_map
        return ordered_map_Printer(val)
    return None

try:
    found = False
    for i in range(len(gdb.pretty_printers)):
        try:
            if gdb.pretty_printers[i].__name__ == "bfp4c_pp":
                gdb.pretty_printers[i] = bfp4c_pp
                found = True
        except:
            pass
    if not found:
        gdb.pretty_printers.append(bfp4c_pp)
except:
    pass

end
