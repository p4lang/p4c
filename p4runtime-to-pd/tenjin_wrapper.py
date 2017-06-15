#!/usr/bin/python

# Copyright 2013-present Barefoot Networks, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

#
# Antonin Bas (antonin@barefootnetworks.com)
#
#

# -*- coding: utf-8 -*-

import sys
import tenjin
import re


# Preprocess a Tenjin template using a macro like system
# This was written as a utility to avoid a lot of code duplication in the PD
# For example, the following:
#
# //:: #define METER_STUFF
# //:: for i in xrange(10):
# //::   if True:
# print "YES!"
# //::   #endif
# //:: #endfor
# //:: #enddefine

# //::   #expand METER_STUFF 2
#
# will become:
#
# //::   for i in xrange(10):
# //::     if True:
#   print "YES!"
# //::     #endif
# //::   #endfor
#
# decided not to use it after all, because error line reporting gets screwed up
# in Tenjin, keeping it around for now
class MacroPreprocessor(object):

    def __init__(self):
        self.regexp_define = re.compile(
            r'//::(\s*)#define\s*([^\s]*)\s*(.*)//::\s*#enddefine', re.S)
        self.regexp_expand = re.compile(
            r'//::(\s*)#expand\s*([^\s]*)\s*([0-9]*)\s*$', re.M)
        self.regexp_define_instruction = re.compile(r'\s*//::(\s*)(.*)')

    # name imposed by Tenjin
    def __call__(self, input, **kwargs):
        macros = {}
        macros_indent = {}
        buf_1 = []
        append = buf_1.append
        pos = 0
        for m in self.regexp_define.finditer(input):
            base_indent = len(m.group(1))
            text = input[pos:m.start()]
            append(text)
            macro = m.group(2)
            stmt = m.group(3)
            macros[macro] = []
            macros_indent[macro] = base_indent
            for line in stmt.splitlines():
                m2 = self.regexp_define_instruction.match(line)
                if m2:
                    indent = len(m2.group(1))
                    macros[macro].append(('instruction', indent, m2.group(2)))
                else:
                    macros[macro].append(('code', line))
            pos = m.end()
        rest = input[pos:]
        append(rest)

        input_2 = "".join(buf_1)
        buf_2 = []
        append = buf_2.append
        pos = 0

        for m in self.regexp_expand.finditer(input_2):
            nb_spaces_instruction = len(m.group(1))
            macro = m.group(2)
            nb_spaces_code = int(m.group(3))
            spaces_code = " " * nb_spaces_code
            if macro not in macros:
                continue
            text = input_2[pos:m.start()]
            append(text)
            for t in macros[macro]:
                if t[0] == "instruction":
                    indent = t[1] - macros_indent[macro] + nb_spaces_instruction
                    spaces_instruction = " " * indent
                    append("//::")
                    append(spaces_instruction)
                    append(t[2])
                else:
                    assert(t[0] == "code")
                    append(spaces_code)
                    append(t[1])
                append("\n")
            pos = m.end()
        rest = input_2[pos:]
        append(rest)

        return "".join(buf_2)


def render_template(out, name, context, templates_dir, prefix=None):
    """
    Render a template using tenjin.
    out: a file-like object
    name: name of the template
    context: dictionary of variables to pass to the template
    prefix: optional prefix for embedding (for other languages than python)
    """

    # support "::" syntax
    pp = [MacroPreprocessor()]
    pp += [tenjin.PrefixedLinePreprocessor(prefix=prefix)
           if prefix else tenjin.PrefixedLinePreprocessor()]
    # disable HTML escaping
    template_globals = {"to_str": str, "escape": str}
    engine = TemplateEngine(path=[templates_dir], pp=pp, cache=False)
    out.write(engine.render(name, context, template_globals))


# We have not found a use for this yet, so excude it from cov report
class TemplateEngine(tenjin.Engine):  # pragma: no cover
    def include(self, template_name, **kwargs):
        """
        Tenjin has an issue with nested includes that use the same local
        variable names, because it uses the same context dict for each level of
        nesting.  The fix is to copy the context.
        """
        frame = sys._getframe(1)
        locals = frame.f_locals
        globals = frame.f_globals
        context = locals["_context"].copy()
        context.update(kwargs)
        template = self.get_template(template_name, context, globals)
        return template.render(context, globals, _buf=locals["_buf"])
