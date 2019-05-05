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
        return "ordered_map<..>"
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

def vec_begin(vec):
    return vec['_M_impl']['_M_start']
def vec_end(vec):
    return vec['_M_impl']['_M_finish']
def vec_size(vec):
    return int(vec_end(vec) - vec_begin(vec))
def vec_at(vec, i):
    return (vec_begin(vec) + i).dereference()

class safe_vector_Printer:
    "Print a safe_vector<>"
    def __init__(self, val):
        self.val = val
    def to_string(self):
        return "" if vec_size(self.val) > 0 else "[]"
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
    "Print a safe_vector<bool>"
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

def find_pp(val):
    if str(val.type.tag).startswith('ordered_map<'):
        return ordered_map_Printer(val)
    if str(val.type.tag).startswith('ordered_set<'):
        return ordered_set_Printer(val)
    if str(val.type.tag).startswith('safe_vector<bool'):
        return safe_vector_bool_Printer(val)
    if str(val.type.tag).startswith('safe_vector<'):
        return safe_vector_Printer(val)
    if val.type.tag == 'bitvec':
        return bitvecPrinter(val)
    if val.type.tag == 'cstring':
        return cstringPrinter(val)
    if val.type.tag == 'Util::SourceInfo':
        return SourceInfoPrinter(val)
    return None

try:
    while gdb.pretty_printers[-1].__name__ == "find_pp":
        gdb.pretty_printers = gdb.pretty_printers[:-1]
except:
    pass

gdb.pretty_printers.append(find_pp)
end
