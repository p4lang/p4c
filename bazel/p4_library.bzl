"""P4 compilation rule."""

load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "CPP_TOOLCHAIN_TYPE", "find_cpp_toolchain", "use_cpp_toolchain")

def _extract_common_p4c_args(ctx):
    """Extract common arguments for p4c build rules."""
    p4file = ctx.file.src
    p4deps = ctx.files._p4include + ctx.files.deps
    args = [
        p4file.path,
        "--std",
        ctx.attr.std,
        "--arch",
        ctx.attr.arch,
    ]

    include_dirs = {d.dirname: 0 for d in p4deps}  # Use dict to express set.
    include_dirs["."] = 0  # Enable include paths relative to workspace root.
    args += [("-I" + dir) for dir in include_dirs.keys()]

    return args

def _extract_p4c_inputs(ctx):
    """Extract input p4 files to give to p4c from the build rule context."""
    return ctx.files._p4include + ctx.files.deps + [ctx.file.src]

def _run_shell_cmd_with_p4c(ctx, command, **run_shell_kwargs):
    """Run given shell command using `run_shell` action after setting up
       the C compiler toolchain.

       This function also sets up the `tools` parameter for `run_shell` to
       set up p4c and the cpp toolchain, and `kwargs` is passed to
       `run_shell`.
    """

    if not hasattr(ctx.executable, "p4c_backend"):
        fail("The build rule does not specify the p4c backend executable via `p4c_backend` attribute.")
    p4c = ctx.executable.p4c_backend

    cpp_toolchain = find_cpp_toolchain(ctx)
    ctx.actions.run_shell(
        command = """
            # p4c invokes cc for preprocessing; we provide it below.
            function cc () {{ "{cc}" "$@"; }}
            export -f cc

            {command}
        """.format(
            cc = cpp_toolchain.compiler_executable,
            command = command,
        ),
        tools = depset(
            direct = [p4c],
            transitive = [cpp_toolchain.all_files],
        ),
        use_default_shell_env = True,
        toolchain = CPP_TOOLCHAIN_TYPE,
        **run_shell_kwargs
    )

def _p4_library_impl(ctx):
    p4c = ctx.executable.p4c_backend
    p4file = ctx.file.src
    target = ctx.attr.target
    args = _extract_common_p4c_args(ctx)
    args += ["--target", (target if target else "bmv2")]

    if ctx.attr.extra_args:
        args.append(ctx.attr.extra_args)

    outputs = []

    if ctx.outputs.bf_rt_schema_out:
        if target != "dpdk":
            fail('Must use `target = "dpdk"` when specifying bf_rt_schema_out.')
        args += ["--bf-rt-schema", ctx.outputs.bf_rt_schema_out.path]
        outputs.append(ctx.outputs.bf_rt_schema_out)

    if ctx.outputs.context_out:
        if target != "dpdk":
            fail('Must use `target = "dpdk"` when specifying context_out.')
        args += ["--context", ctx.outputs.context_out.path]
        outputs.append(ctx.outputs.context_out)

    if ctx.outputs.p4info_out:
        if target != "" and target != "bmv2":
            fail('Must use `target = "bmv2"` when specifying p4info_out.')
        args += ["--p4runtime-files", ctx.outputs.p4info_out.path]
        outputs.append(ctx.outputs.p4info_out)

    if ctx.outputs.target_out:
        if not target:
            fail("Cannot specify target_out without specifying target explicitly.")
        args += ["-o", ctx.outputs.target_out.path]
        outputs.append(ctx.outputs.target_out)

    if not outputs:
        fail("No outputs specified. Must specify p4info_out or target_out or both.")

    _run_shell_cmd_with_p4c(
        ctx,
        command = """
            "{p4c}" {p4c_args}
        """.format(
            p4c = p4c.path,
            p4c_args = " ".join(args),
        ),
        inputs = _extract_p4c_inputs(ctx),
        outputs = outputs,
        progress_message = "Compiling P4 program %s" % p4file.short_path,
    )

p4_library = rule(
    doc = "Compiles P4 program using the p4c compiler.",
    implementation = _p4_library_impl,
    attrs = {
        "src": attr.label(
            doc = "P4 source file to pass to p4c.",
            mandatory = True,
            allow_single_file = [".p4"],
        ),
        "deps": attr.label_list(
            doc = "Additional P4 dependencies (optional). Use for #include-ed files.",
            mandatory = False,
            allow_files = [".p4", ".h"],
            default = [],
        ),
        "bf_rt_schema_out": attr.output(
            mandatory = False,
            doc = "The name of the dpdk bf rt schema output file.",
        ),
        "context_out": attr.output(
            mandatory = False,
            doc = "The name of the dpdk context output file.",
        ),
        "p4info_out": attr.output(
            mandatory = False,
            doc = "The name of the p4info output file.",
        ),
        "target_out": attr.output(
            mandatory = False,
            doc = "The name of the target output file, passed to p4c via -o.",
        ),
        "target": attr.string(
            doc = "The --target argument passed to p4c (default: bmv2).",
            mandatory = False,
            default = "",  # Use "" so we can recognize implicit target.
        ),
        "arch": attr.string(
            doc = "The --arch argument passed to p4c (default: v1model).",
            mandatory = False,
            default = "v1model",
        ),
        "std": attr.string(
            doc = "The --std argument passed to p4c (default: p4-16).",
            mandatory = False,
            default = "p4-16",
        ),
        "extra_args": attr.string(
            doc = "String of additional command line arguments to pass to p4c.",
            mandatory = False,
            default = "",
        ),
        "p4c_backend": attr.label(
            default = Label("@com_github_p4lang_p4c//:p4c_bmv2"),
            executable = True,
            cfg = "exec",
        ),
        "_p4include": attr.label(
            default = Label("@com_github_p4lang_p4c//:p4include"),
            allow_files = [".p4", ".h"],
        ),
        "_cc_toolchain": attr.label(default = Label("@bazel_tools//tools/cpp:current_cc_toolchain")),
    },
    incompatible_use_toolchain_transition = True,
    toolchains = use_cpp_toolchain(),
)

def _p4_graphs_impl(ctx):
    p4c = ctx.executable.p4c_backend
    p4file = ctx.file.src
    output_file = ctx.outputs.out

    if not output_file.path.lower().endswith(".dot"):
        fail("The output graph file must have extension .dot")

    args = _extract_common_p4c_args(ctx)
    graph_dir = output_file.path + "-graphs-dir"
    args += ["--graphs-dir", graph_dir]

    _run_shell_cmd_with_p4c(
        ctx,
        command = """
            # Create the output directory
            mkdir "{graph_dir}"
            # Run the compiler
            "{p4c}" {p4c_args}
            # Merge all output graphs, the * needs to be outside the quotes for globbing
            cat "{graph_dir}"/* > {output_file}
        """.format(
            p4c = p4c.path,
            p4c_args = " ".join(args),
            graph_dir = graph_dir,
            output_file = output_file.path,
        ),
        inputs = _extract_p4c_inputs(ctx),
        outputs = [output_file],
        progress_message = "Generating the graphs for P4 program %s" % p4file.short_path,
    )

p4_graphs = rule(
    doc = "Generates the graphs for a P4 program using p4c-graphs, and merges them in a single GraphViz file. This file can be later processed by gvpack and dot.",
    implementation = _p4_graphs_impl,
    attrs = {
        "src": attr.label(
            doc = "P4 source file to pass to p4c.",
            mandatory = True,
            allow_single_file = [".p4"],
        ),
        "deps": attr.label_list(
            doc = "Additional P4 dependencies (optional). Use for #include-ed files.",
            mandatory = False,
            allow_files = [".p4", ".h"],
            default = [],
        ),
        "out": attr.output(
            doc = "The name of the output DOT file",
            mandatory = True,
        ),
        "arch": attr.string(
            doc = "The --arch argument passed to p4c (default: v1model).",
            mandatory = False,
            default = "v1model",
        ),
        "std": attr.string(
            doc = "The --std argument passed to p4c (default: p4-16).",
            mandatory = False,
            default = "p4-16",
        ),
        "extra_args": attr.string(
            doc = "String of additional command line arguments to pass to p4c.",
            mandatory = False,
            default = "",
        ),
        "p4c_backend": attr.label(
            default = Label("@com_github_p4lang_p4c//:p4c_graphs"),
            executable = True,
            cfg = "exec",
        ),
        "_p4include": attr.label(
            default = Label("@com_github_p4lang_p4c//:p4include"),
            allow_files = [".p4", ".h"],
        ),
        "_cc_toolchain": attr.label(default = Label("@bazel_tools//tools/cpp:current_cc_toolchain")),
    },
    incompatible_use_toolchain_transition = True,
    toolchains = use_cpp_toolchain(),
)
