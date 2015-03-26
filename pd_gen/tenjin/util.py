#!/usr/bin/python
#
# These are utility functions, some taken from bigcode/infra/sourcegen/util.py
#

import copy
import types
import sys
import os
import tenjin
import json
import yaml
# import globals as gl

class DotDict(dict):
    """ Access keys in a nested dictionary using dot notation """

    def __getattr__(self, attr):
        item = self.get(attr, None)
        if type(item) == types.DictType:
            item = DotDict(item)
        return item

    __setattr__= dict.__setitem__
    __delattr__= dict.__delitem__

# From loxi_utils.py
def render_template(out, name, context, templates_dir, prefix = None):
    """
    Render a template using tenjin.
    out: a file-like object
    name: name of the template
    context: dictionary of variables to pass to the template
    prefix: optional prefix for embedding (for other languages than python)
    """

    # support "::" syntax
    pp = [ tenjin.PrefixedLinePreprocessor(prefix=prefix)
           if prefix else tenjin.PrefixedLinePreprocessor() ]
    # disable HTML escaping
    template_globals = { "to_str": str, "escape": str }
    engine = TemplateEngine(path=[templates_dir], pp=pp, cache = False)
    out.write(engine.render(name, context, template_globals))

class TemplateEngine(tenjin.Engine):
    def include(self, template_name, **kwargs):
        """
        Tenjin has an issue with nested includes that use the same local
        variable names, because it uses the same context dict for each level of
        nesting.  The fix is to copy the context.
        """
        frame = sys._getframe(1)
        locals  = frame.f_locals
        globals = frame.f_globals
        context = locals["_context"].copy()
        context.update(kwargs)
        template = self.get_template(template_name, context, globals)
        return template.render(context, globals, _buf=locals["_buf"])

def dump_render_dict(render_dict, out = sys.stdout):
    # output = json.dumps(render_dict)
    output =  yaml.dump(render_dict)
    out.write(output)

# _verbose = 1
# def verbose_set(verbose):
#     # @fixme ensure verbose is a number
#     gl.args.verbosity = verbose

# def log_msg(str):
#     if gl.args.verbosity:
#         sys.stderr.write(str)

# def log_verbose(str):
#     if gl.args.verbosity > 1:
#         sys.stderr.write(str)

def error(str):
    sys.stderr.write(str)
    os._exit(1)
