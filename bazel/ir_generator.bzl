# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0

"""Generate IR declarations and implementations separately for each .def input."""

def _ir_generated_files_impl(ctx):
    headers = []
    sources = []
    inputs = []
    names = {}
    input_root = None
    output_root = None
    for source in ctx.files.core_defs + ctx.files.extra_defs:
        if source.owner.workspace_name == ctx.label.workspace_name:
            name = source.short_path
            if name.startswith("../"):
                name = "/".join(name.split("/")[2:])
        else:
            # The extension repository already requires unique basenames.
            name = "extensions/" + source.basename
        if name in names:
            fail("Duplicate IR input path: " + name)
        names[name] = True

        # Stage external definitions under the same source root as core inputs.
        # This also makes generated include paths independent of repository names.
        staged = ctx.actions.declare_file(ctx.label.name + ".inputs/" + name)
        ctx.actions.symlink(output = staged, target_file = source)
        inputs.append(staged)
        input_root = staged.path[:-len(name)].rstrip("/")
        header = ctx.actions.declare_file(name + ".h")
        headers.append(header)
        sources.append(ctx.actions.declare_file(name + ".cpp"))
        output_root = header.path[:-len(name + ".h")].rstrip("/")
    for name in ["ir/ir-generated.h", "ir/ir-generated-common.h", "ir/gen-tree-macro.h"]:
        headers.append(ctx.actions.declare_file(name))
    sources.append(ctx.actions.declare_file("ir/ir-generated.cpp"))
    args = ctx.actions.args()
    args.add_all(["-r", input_root, "-d", output_root])
    args.add_all(inputs)
    ctx.actions.run(
        executable = ctx.executable.generator,
        arguments = [args],
        inputs = inputs,
        outputs = headers + sources,
        mnemonic = "GenerateIR",
    )
    return [
        DefaultInfo(files = depset(headers + sources)),
        OutputGroupInfo(headers = depset(headers), sources = depset(sources)),
    ]

ir_generated_files = rule(
    implementation = _ir_generated_files_impl,
    attrs = {
        "core_defs": attr.label_list(allow_files = [".def"], mandatory = True),
        "extra_defs": attr.label_list(allow_files = [".def"]),
        "generator": attr.label(executable = True, cfg = "exec", mandatory = True),
    },
)
