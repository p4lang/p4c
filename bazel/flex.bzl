"""Build rules for generating C or C++ sources with Flex."""

load("@rules_cc//cc/common:cc_common.bzl", "cc_common")
load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")

def _genlex_impl(ctx):
    """Implementation for genlex rule."""
    flex_tc = ctx.toolchains["@rules_flex//flex:toolchain_type"].flex_toolchain

    args = ctx.actions.args()
    args.add("-o", ctx.outputs.out.path)
    if ctx.attr.prefix:
        args.add("-P")
        args.add(ctx.attr.prefix)
    args.add(ctx.file.src.path)

    ctx.actions.run(
        executable = flex_tc.flex_tool,
        arguments = [args],
        inputs = depset(direct = [ctx.file.src] + ctx.files.includes),
        outputs = [ctx.outputs.out],
        env = flex_tc.flex_env,
        mnemonic = "Flex",
        progress_message = "Generating %s from %s" % (
            ctx.outputs.out.short_path,
            ctx.file.src.short_path,
        ),
    )

genlex = rule(
    implementation = _genlex_impl,
    doc = "Generate a C or C++ lexer from a lex file using Flex.",
    attrs = {
        "includes": attr.label_list(
            allow_files = True,
            doc = "A list of headers included by the .lex file.",
        ),
        "out": attr.output(
            mandatory = True,
            doc = "The generated source file.",
        ),
        "prefix": attr.string(
            doc = "Passed to flex as the -P option.",
        ),
        "src": attr.label(
            mandatory = True,
            allow_single_file = True,
            doc = "The .l, .ll, or .lex source file.",
        ),
    },
    toolchains = [
        "@rules_flex//flex:toolchain_type",
        "@rules_m4//m4:toolchain_type",
    ],
)

def _flex_lexer_h_impl(ctx):
    """Exposes FlexLexer.h from the Flex toolchain as a CcInfo library."""
    hdr = ctx.toolchains["@rules_flex//flex:toolchain_type"].flex_toolchain.flex_lexer_h
    compilation_context = cc_common.create_compilation_context(
        headers = depset([hdr]),
        system_includes = depset([hdr.dirname]),
    )
    return [
        CcInfo(compilation_context = compilation_context),
        DefaultInfo(files = depset([hdr])),
    ]

flex_lexer_h = rule(
    implementation = _flex_lexer_h_impl,
    doc = "Exposes FlexLexer.h from the Flex toolchain as a cc_library dependency.",
    toolchains = ["@rules_flex//flex:toolchain_type"],
)
