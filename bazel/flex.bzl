"""Build rule for generating C or C++ sources with Flex."""

# Expects to find flex in the path.
def genlex(name, src, out, prefix, includes = [], visibility = None):
    """Generate a C++ lexer from a lex file using Flex.

    Args:
      name: The name of the rule.
      src: The .lex source file.
      out: The generated source file.
      includes: A list of headers included by the .lex file.
      prefix: Passed to flex as the -P option.
      visibility: visibility of this rule to other packages.
    """
    cmd = "flex -o $(location %s) -P %s $(location %s)" % (out, prefix, src)
    native.genrule(
        name = name,
        outs = [out],
        srcs = [src] + includes,
        cmd = cmd,
        visibility = visibility,
    )
