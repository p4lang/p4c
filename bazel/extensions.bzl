"""Module extension for p4c IR extensions.

This extension allows users to register custom IR definitions by providing a list
of files containing the extensions.

Example usage in MODULE.bazel:
    p4c_ir_extensions = use_extension("@p4c//bazel:extensions.bzl", "p4c_ir_extensions")
    p4c_ir_extensions.ir_extensions(srcs = ["//path/to:ir_extensions_files"])
    use_repo(p4c_ir_extensions, "p4c_ir_extensions")
"""

def _p4c_extension_repo_impl(ctx):
    """Creates the extension repo.
    It symlinks all provided files to the root of this repo and creates a filegroup.
    """
    srcs = []
    for src in ctx.attr.srcs:
        # Symlink the file to the root of the repo.
        # We use the basename of the file to avoid directory structure issues.
        # This assumes all extension files have unique names, which is a reasonable constraint.
        ctx.symlink(src, src.name)
        srcs.append('"{}"'.format(src.name))

    ctx.file("BUILD", """
package(default_visibility = ["//visibility:public"])
filegroup(
    name = "ir_extension",
    srcs = [
        {}
    ],
)
""".format(",\n        ".join(srcs)))

p4c_extension_repo = repository_rule(
    implementation = _p4c_extension_repo_impl,
    attrs = {
        "srcs": attr.label_list(allow_files = True, doc = "List of IR extension files"),
    },
)

def _p4c_ir_extensions_impl(mctx):
    """Module extension to discover if a user has registered custom IR extensions."""
    all_srcs = []

    # Iterate over all modules that use this extension to find tags.
    for mod in mctx.modules:
        for tag in mod.tags.ir_extensions:
            all_srcs.extend(tag.srcs)

    # Create the repository, passing all collected sources.
    p4c_extension_repo(
        name = "p4c_ir_extensions",
        srcs = all_srcs,
    )

p4c_ir_extensions = module_extension(
    implementation = _p4c_ir_extensions_impl,
    tag_classes = {
        "ir_extensions": tag_class(attrs = {"srcs": attr.label_list(allow_files = True)}),
    },
)
