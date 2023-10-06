load("@rules_foreign_cc//foreign_cc:defs.bzl", "configure_make")

filegroup(
    name = "all",
    srcs = glob(["**"]),
)

configure_make(
    name = "api",
    # Bazel redacts certain cc macros such as __DATA__ and __TIMESTAMP__
    # since they will cause the compiled code to have timestamps or other
    # similar information in it, causing the compilation to be
    # non-deterministic.
    # Without such redaction, running the compilation twice with no changes in
    # the code will produce seemingly different binaries.
    # Bazel fixes this by setting the value of these macros to "redacted",
    # which is a valid c++ expression through CFLAGS and CXXFLAGS Toolchain
    # options.
    # See https://github.com/bazelbuild/bazel/issues/5750
    # However, the quotes get dropped because of some bash serialization in
    # rules_foreign_cc when they are passed (as bash environment variables) to
    # "./configure", causing __DATA__ to resolve to the unquoted token redacted,
    # which is usually not a valid c++ expression.
    # This fixes that, it makes redacted (the token not the string) an alias to
    # 0, which is a valid c++ expression.
    # This is a minor improvement on top of:
    # https://github.com/bazelbuild/rules_foreign_cc/issues/239
    configure_env_vars = {
        "CFLAGS": "-Dredacted=0",
        "CXXFLAGS": "-Dredacted=0",
        "PYTHON": "python3",
    },
    configure_in_place = True,
    lib_source = ":all",
    make_commands = [
        "cd build",
        "make -j8",  # Use -j8, hardcoded but otherwise it will be too slow.
        "make install",
    ],
    out_binaries = ["z3"],
    out_shared_libs = ["libz3.so"],
    visibility = ["//visibility:public"],
)
