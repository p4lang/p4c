# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0

"""Expose a C++ dependency's source repository root as a system include path."""

load("@rules_cc//cc/common:cc_common.bzl", "cc_common")
load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")

def _cc_system_includes_impl(ctx):
    # Resolve the path through the dependency's label: Bzlmod's canonical
    # repository names can differ between direct and downstream builds.
    return [CcInfo(compilation_context = cc_common.create_compilation_context(
        system_includes = depset([ctx.attr.dep.label.workspace_root or "."]),
    ))]

cc_system_includes = rule(
    implementation = _cc_system_includes_impl,
    doc = """Export a dependency's source repository root as a transitive system include path.

This is intended for libraries such as Abseil whose headers are included
relative to their repository root. The extra path propagates to transitive
dependents, including consumers of p4c's public headers, without .bazelrc flags.
This target only supplies include-path metadata. Consumers must still declare
their ordinary library dependencies for headers and linking information.
""",
    attrs = {
        "dep": attr.label(
            mandatory = True,
            providers = [CcInfo],
            doc = "A C++ target in the repository whose source root should be exported.",
        ),
    },
    provides = [CcInfo],
)
