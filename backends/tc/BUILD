# tc backend for the P4 compiler.

load("//:bazel/p4_library.bzl", "p4_ir")

cc_library(
    name = "util",
    srcs = [
        "util.cpp",
    ],
    hdrs = [
        "util.h",
    ],
    copts = ["-fexceptions"],
    features = ["-use_header_modules"],
    deps = [
        "//:lib",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "util_test",
    size = "small",
    srcs = ["util_test.cpp"],
    linkopts = ["-lgmp"],
    deps = [
        ":util",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "test_util",
    srcs = [
        "test_util.cpp",
    ],
    hdrs = [
        "test_util.h",
    ],
    copts = ["-fexceptions"],
    features = ["-use_header_modules"],
    deps = [
        ":tcam_program",
        "@com_google_absl//absl/container:btree",
        "@com_google_absl//absl/memory",
    ],
)

cc_library(
    name = "instruction",
    srcs = [
        "instruction.cpp",
    ],
    hdrs = [
        "instruction.h",
    ],
    copts = ["-fexceptions"],
    features = ["-use_header_modules"],
    deps = [
        ":util",
        "//:lib",
        "@com_google_absl//absl/hash",
    ],
)

cc_library(
    name = "tcam_program",
    srcs = [
        "tcam_program.cpp",
    ],
    hdrs = [
        "tcam_program.h",
    ],
    copts = ["-fexceptions"],
    features = ["-use_header_modules"],
    deps = [
        ":instruction",
        ":util",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/hash",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "pass_manager",
    srcs = [
        "pass_manager.cpp",
    ],
    hdrs = [
        "pass.h",
        "pass_manager.h",
    ],
    copts = ["-fexceptions"],
    features = ["-use_header_modules"],
    deps = [
        ":tcam_program",
        "//:lib",
        "@boost//:throw_exception",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:any",
    ],
)

cc_library(
    name = "reachability",
    srcs = [
        "reachability.cpp",
    ],
    hdrs = [
        "reachability.h",
    ],
    copts = ["-fexceptions"],
    features = ["-use_header_modules"],
    deps = [
        ":instruction",
        ":pass_manager",
        "//:ir_frontend_midend_control_plane",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
    ],
)

cc_library(
    name = "dead_state_elimination",
    srcs = [
        "dead_state_elimination.cpp",
    ],
    hdrs = [
        "dead_state_elimination.h",
    ],
    copts = ["-fexceptions"],
    features = ["-use_header_modules"],
    deps = [
        ":pass_manager",
        ":reachability",
        "@boost//:multiprecision",
    ],
)

cc_library(
    name = "inlining",
    srcs = [
        "inlining.cpp",
    ],
    hdrs = [
        "inlining.h",
    ],
    copts = ["-fexceptions"],
    features = ["-use_header_modules"],
    deps = [
        ":instruction",
        ":pass_manager",
        ":reachability",
        ":tcam_program",
        "@com_google_absl//absl/container:flat_hash_map",
    ],
)

cc_library(
    name = "yaml_serializer",
    srcs = [
        "yaml_serializer.cpp",
    ],
    hdrs = [
        "yaml_serializer.h",
    ],
    copts = ["-fexceptions"],
    features = ["-use_header_modules"],
    deps = [
        ":tcam_program",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "yaml_parser",
    srcs = [
        "yaml_parser.cpp",
    ],
    hdrs = [
        "yaml_parser.h",
    ],
    copts = ["-fexceptions"],
    features = ["-use_header_modules"],
    deps = [
        ":instruction",
        ":tcam_program",
        ":util",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "ir_builder",
    srcs = [
        "ir_builder.cpp",
    ],
    hdrs = [
        "ir_builder.h",
    ],
    copts = ["-fexceptions"],
    features = ["-use_header_modules"],
    deps = [
        ":instruction",
        ":tcam_program",
        ":util",
        "//:ir_frontend_midend_control_plane",
        "//:lib",
        "@boost//:multiprecision",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "p4c_interface",
    hdrs = [
        "p4c_interface.h",
    ],
    copts = ["-fexceptions"],
    data = ["//:p4include"],
    features = ["-use_header_modules"],
    deps = [
        "//:ir_frontend_midend_control_plane",
        "//:lib",
    ],
)

p4_ir(
    name = "lookahead_testdata",
    src = "testdata/lookahead.p4",
    ir_out = "testdata/lookahead.json",
)

cc_test(
    name = "lookahead_test",
    size = "small",
    srcs = [
        "lookahead_test.cpp",
    ],
    copts = ["-fexceptions"],
    data = [
        "testdata/lookahead.json",
        "//:p4include",
    ],
    features = ["-use_header_modules"],
    linkopts = ["-lgmp"],
    deps = [
        ":instruction",
        ":ir_builder",
        ":p4c_interface",
        ":tcam_program",
        ":util",
        "@com_google_absl//absl/flags:flag",
        "@com_google_googletest//:gtest_main",
    ],
)

p4_ir(
    name = "tuple_testdata",
    src = "testdata/tuple.p4",
    ir_out = "testdata/tuple.json",
)

cc_test(
    name = "tuple_test",
    size = "small",
    srcs = [
        "tuple_test.cpp",
    ],
    copts = ["-fexceptions"],
    data = [
        "testdata/tuple.json",
        "//:p4include",
    ],
    features = ["-use_header_modules"],
    linkopts = ["-lgmp"],
    deps = [
        ":instruction",
        ":ir_builder",
        ":p4c_interface",
        ":tcam_program",
        ":util",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/flags:flag",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "reachability_test",
    size = "small",
    srcs = [
        "reachability_test.cpp",
    ],
    copts = ["-fexceptions"],
    data = [
    ],
    features = ["-use_header_modules"],
    linkopts = ["-lgmp"],
    deps = [
        ":pass_manager",
        ":reachability",
        ":tcam_program",
        ":test_util",
        ":util",
        "@com_google_absl//absl/container:btree",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "dead_state_elimination_test",
    size = "small",
    srcs = [
        "dead_state_elimination_test.cpp",
    ],
    copts = ["-fexceptions"],
    data = [
    ],
    features = ["-use_header_modules"],
    linkopts = ["-lgmp"],
    deps = [
        ":dead_state_elimination",
        ":pass_manager",
        ":tcam_program",
        ":test_util",
        ":util",
        "@com_google_absl//absl/container:btree",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/memory",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "inlining_test",
    size = "small",
    srcs = [
        "inlining_test.cpp",
    ],
    copts = ["-fexceptions"],
    data = [
    ],
    features = ["-use_header_modules"],
    linkopts = ["-lgmp"],
    deps = [
        ":inlining",
        ":pass_manager",
        ":tcam_program",
        ":test_util",
        ":util",
        "@com_google_absl//absl/memory",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "yaml_test",
    size = "small",
    srcs = [
        "yaml_test.cpp",
    ],
    copts = ["-fexceptions"],
    data = [
    ],
    features = ["-use_header_modules"],
    linkopts = ["-lgmp"],
    deps = [
        ":instruction",
        ":tcam_program",
        ":util",
        ":yaml_parser",
        ":yaml_serializer",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "tcam_program_test",
    size = "small",
    srcs = [
        "tcam_program_test.cpp",
    ],
    copts = ["-fexceptions"],
    features = ["-use_header_modules"],
    linkopts = ["-lgmp"],
    deps = [
        ":tcam_program",
        ":test_util",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_util_test",
    size = "small",
    srcs = [
        "test_util_test.cpp",
    ],
    copts = ["-fexceptions"],
    features = ["-use_header_modules"],
    linkopts = ["-lgmp"],
    deps = [
        ":test_util",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "simulator_test",
    size = "small",
    srcs = [
        "simulator_test.cpp",
    ],
    copts = ["-fexceptions"],
    features = ["-use_header_modules"],
    linkopts = ["-lgmp"],
    deps = [
        ":instruction",
        ":simulator",
        ":test_util",
        ":util",
        ":yaml_parser",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_binary(
    name = "p4c_tc",
    srcs = [
        "p4c_tc.cpp",
    ],
    copts = ["-fexceptions"],
    data = ["//:p4include"],
    features = ["-use_header_modules"],
    linkopts = ["-lgmp"],
    deps = [
        ":dead_state_elimination",
        ":inlining",
        ":ir_builder",
        ":p4c_interface",
        ":pass_manager",
        ":yaml_parser",
        ":yaml_serializer",
        "@com_google_absl//absl/memory",
    ],
)

cc_library(
    name = "simulator",
    srcs = [
        "simulator.cpp",
    ],
    hdrs = [
        "simulator.h",
    ],
    copts = ["-fexceptions"],
    features = ["-use_header_modules"],
    deps = [
        ":instruction",
        ":tcam_program",
        ":util",
        "@com_google_absl//absl/container:flat_hash_map",
    ],
)
