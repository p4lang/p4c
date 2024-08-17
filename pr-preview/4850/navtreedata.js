/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "P4 Compiler Documentation (P4C)", "index.html", [
    [ "P4C Repository Organization", "repository_structure.html", [
      [ "Features of P4C", "index.html#features-of-p4c", null ],
      [ "Compiler source code organization", "repository_structure.html#compiler-source-code-organization", null ],
      [ "Additional documentation", "repository_structure.html#additional-documentation", null ],
      [ "Writing documentation", "repository_structure.html#writing-documentation", [
        [ "Documentation Comments Style Guide", "repository_structure.html#documentation-comments-style-guide", null ],
        [ "Git usage", "repository_structure.html#git-usage", null ],
        [ "Debugging", "repository_structure.html#debugging", null ],
        [ "Testing", "repository_structure.html#testing", [
          [ "Adding new test data", "repository_structure.html#adding-new-test-data", null ]
        ] ],
        [ "Coding conventions", "repository_structure.html#coding-conventions", null ],
        [ "Compiler Driver", "repository_structure.html#compiler-driver", null ]
      ] ]
    ] ],
    [ "Getting Started", "getting_started.html", [
      [ "Overview", "getting_started.html#overview", [
        [ "Additional documentation", "getting_started.html#additional-documentation-1", null ],
        [ "Sample Backends in P4C", "getting_started.html#sample-backends-in-p4c", null ],
        [ "Getting started", "getting_started.html#getting-started", [
          [ "Installing packaged versions of P4C", "getting_started.html#installing-packaged-versions-of-p4c", [
            [ "Ubuntu", "getting_started.html#ubuntu", null ],
            [ "Debian", "getting_started.html#debian", null ]
          ] ],
          [ "Installing P4C from source", "getting_started.html#installing-p4c-from-source", null ]
        ] ],
        [ "Dependencies", "getting_started.html#dependencies", [
          [ "Ubuntu dependencies", "getting_started.html#ubuntu-dependencies", [
            [ "CMake", "getting_started.html#cmake", null ]
          ] ],
          [ "Fedora dependencies", "getting_started.html#fedora-dependencies", null ],
          [ "macOS dependencies", "getting_started.html#macos-dependencies", null ],
          [ "Garbage collector", "getting_started.html#garbage-collector", null ],
          [ "Crash dumps", "getting_started.html#crash-dumps", null ]
        ] ],
        [ "Development tools", "getting_started.html#development-tools", [
          [ "Git setup", "getting_started.html#git-setup", null ]
        ] ],
        [ "Docker", "getting_started.html#docker", null ],
        [ "Bazel", "getting_started.html#bazel", null ],
        [ "Build system", "getting_started.html#build-system", [
          [ "Defining new CMake targets", "getting_started.html#defining-new-cmake-targets", [
            [ "IR definition files", "getting_started.html#ir-definition-files", null ],
            [ "Source files", "getting_started.html#source-files", null ],
            [ "Target", "getting_started.html#target", null ],
            [ "Tests", "getting_started.html#tests", null ],
            [ "Installation", "getting_started.html#installation", null ]
          ] ]
        ] ]
      ] ],
      [ "Common P4C utility functions", "getting_started.html#common-p4c-utility-functions", [
        [ "Known issues", "getting_started.html#known-issues", [
          [ "Frontend", "getting_started.html#frontend", [
            [ "P4_14 features not supported in P4_16", "getting_started.html#p4_14-features-not-supported-in-p4_16", null ]
          ] ],
          [ "Backends", "getting_started.html#backends", [
            [ "Bmv2 Backend", "getting_started.html#bmv2-backend", null ]
          ] ]
        ] ],
        [ "How to Contribute", "getting_started.html#how-to-contribute", null ],
        [ "P4 Compiler Onboarding", "getting_started.html#p4-compiler-onboarding", null ],
        [ "Contact", "getting_started.html#contact", null ]
      ] ]
    ] ],
    [ "P4C Intermediate Representation (IR)", "intermediate_representation_ir.html", [
      [ "P4C Intermediate Representation (IR) Classes", "intermediate_representation_ir.html#p4c-intermediate-representation-ir-classes", null ]
    ] ],
    [ "Behavioral Model Backend", "behavioral_model_backend.html", [
      [ "Dependencies", "behavioral_model_backend.html#dependencies-1", null ],
      [ "Unsupported P4_16 language features", "behavioral_model_backend.html#unsupported-p4_16-language-features", null ],
      [ "BMv2 \"pna_nic\" Backend", "behavioral_model_backend.html#bmv2-pna_nic-backend", null ],
      [ "portable_common", "behavioral_model_backend.html#portable_common", null ]
    ] ],
    [ "DPDK Backend", "dpdk_backend.html", null ],
    [ "eBPF Backend", "ebpf_backend.html", [
      [ "How to run the generated eBPF program", "ebpf_backend.html#how-to-run-the-generated-ebpf-program", null ],
      [ "How to inject custom extern function to the generated eBPF program?", "ebpf_backend.html#how-to-inject-custom-extern-function-to-the-generated-ebpf-program", [
        [ "Target architectures", "ebpf_backend.html#target-architectures", null ],
        [ "Background", "ebpf_backend.html#background", [
          [ "P4", "ebpf_backend.html#p4", null ],
          [ "eBPF", "ebpf_backend.html#ebpf", [
            [ "Safe code", "ebpf_backend.html#safe-code", null ],
            [ "Kernel hooks", "ebpf_backend.html#kernel-hooks", null ],
            [ "eBPF Tables", "ebpf_backend.html#ebpf-tables", null ],
            [ "Concurrency", "ebpf_backend.html#concurrency", null ]
          ] ]
        ] ],
        [ "Compiling P4 to eBPF", "ebpf_backend.html#compiling-p4-to-ebpf", [
          [ "Dependencies", "ebpf_backend.html#dependencies-2", null ],
          [ "Supported capabilities", "ebpf_backend.html#supported-capabilities", null ],
          [ "Translating P4 to C", "ebpf_backend.html#translating-p4-to-c", [
            [ "Translating parsers", "ebpf_backend.html#translating-parsers", null ],
            [ "Translating match-action pipelines", "ebpf_backend.html#translating-match-action-pipelines", null ]
          ] ]
        ] ],
        [ "autotoc_md0", "ebpf_backend.html#autotoc_md0", null ],
        [ "Basic principles", "ebpf_backend.html#basic-principles", null ],
        [ "Definition", "ebpf_backend.html#definition", null ],
        [ "Compilation", "ebpf_backend.html#compilation", null ],
        [ "Calling convention", "ebpf_backend.html#calling-convention", null ]
      ] ],
      [ "PSA implementation for eBPF backend", "ebpf_backend.html#psa-implementation-for-ebpf-backend", null ],
      [ "Prerequisites", "ebpf_backend.html#prerequisites", null ],
      [ "Design", "ebpf_backend.html#design", [
        [ "TC-based design (default)", "ebpf_backend.html#tc-based-design-default", null ],
        [ "XDP-based design", "ebpf_backend.html#xdp-based-design", null ],
        [ "Packet paths", "ebpf_backend.html#packet-paths", [
          [ "NTK (Normal Packet To Kernel)", "ebpf_backend.html#ntk-normal-packet-to-kernel", null ],
          [ "NFP (Normal Packet From Port)", "ebpf_backend.html#nfp-normal-packet-from-port", null ],
          [ "RESUBMIT", "ebpf_backend.html#resubmit", null ],
          [ "NU (Normal Unicast), NM (Normal Multicast), CI2E (Clone Ingress to Egress)", "ebpf_backend.html#nu-normal-unicast-nm-normal-multicast-ci2e-clone-ingress-to-egress", null ],
          [ "CE2E (Clone Egress to Egress)", "ebpf_backend.html#ce2e-clone-egress-to-egress", null ],
          [ "Sending packet to CPU", "ebpf_backend.html#sending-packet-to-cpu", null ],
          [ "NTP (Normal packet to port)", "ebpf_backend.html#ntp-normal-packet-to-port", null ],
          [ "RECIRCULATE", "ebpf_backend.html#recirculate", null ]
        ] ],
        [ "Metadata", "ebpf_backend.html#metadata", null ],
        [ "XDP2TC mode", "ebpf_backend.html#xdp2tc-mode", null ],
        [ "Control-plane API", "ebpf_backend.html#control-plane-api", null ],
        [ "P4 match kinds", "ebpf_backend.html#p4-match-kinds", [
          [ "exact", "ebpf_backend.html#exact", null ],
          [ "lpm", "ebpf_backend.html#lpm", null ],
          [ "ternary", "ebpf_backend.html#ternary", null ]
        ] ],
        [ "PSA externs", "ebpf_backend.html#psa-externs", [
          [ "ActionProfile", "ebpf_backend.html#actionprofile", null ],
          [ "ActionSelector", "ebpf_backend.html#actionselector", null ],
          [ "Digest", "ebpf_backend.html#digest", null ],
          [ "Meters", "ebpf_backend.html#meters", [
            [ "Direct Meter", "ebpf_backend.html#direct-meter", null ]
          ] ],
          [ "value_set", "ebpf_backend.html#value_set", null ],
          [ "Random", "ebpf_backend.html#random", null ]
        ] ]
      ] ],
      [ "Getting started", "ebpf_backend.html#getting-started-1", [
        [ "Installation", "ebpf_backend.html#installation-1", null ],
        [ "Using PSA-eBPF", "ebpf_backend.html#using-psa-ebpf", [
          [ "Prerequisites", "ebpf_backend.html#prerequisites-1", null ],
          [ "Compilation", "ebpf_backend.html#compilation-1", [
            [ "Optional flags", "ebpf_backend.html#optional-flags", null ]
          ] ],
          [ "NIKSS API and nikss-ctl", "ebpf_backend.html#nikss-api-and-nikss-ctl", null ]
        ] ],
        [ "Running PTF tests", "ebpf_backend.html#running-ptf-tests", null ],
        [ "Troubleshooting", "ebpf_backend.html#troubleshooting", null ]
      ] ],
      [ "Performance optimizations", "ebpf_backend.html#performance-optimizations", [
        [ "Table caching", "ebpf_backend.html#table-caching", null ]
      ] ],
      [ "TODO / Limitations", "ebpf_backend.html#todo--limitations", null ],
      [ "Roadmap", "ebpf_backend.html#roadmap", [
        [ "Planned features", "ebpf_backend.html#planned-features", null ],
        [ "Long-term goals", "ebpf_backend.html#long-term-goals", null ],
        [ "Support", "ebpf_backend.html#support", null ]
      ] ]
    ] ],
    [ "TC Backend", "tc_backend.html", null ],
    [ "uBPF Backend", "ubpf_backend.html", [
      [ "uBPF Backend test programs", "ubpf_backend.html#ubpf-backend-test-programs", null ],
      [ "Examples", "ubpf_backend.html#examples", [
        [ "Background", "ubpf_backend.html#background-1", [
          [ "P4", "ubpf_backend.html#p4-1", null ],
          [ "uBPF", "ubpf_backend.html#ubpf", null ]
        ] ],
        [ "Compiling P4 to uBPF", "ubpf_backend.html#compiling-p4-to-ubpf", [
          [ "Translation between P4 and C", "ubpf_backend.html#translation-between-p4-and-c", null ],
          [ "How to use?", "ubpf_backend.html#how-to-use", null ]
        ] ],
        [ "Packet modification", "ubpf_backend.html#packet-modification", [
          [ "IPv4 + MPLS (simple-actions.p4)", "ubpf_backend.html#ipv4--mpls-simple-actionsp4", null ],
          [ "IPv6 (ipv6-actions.p4)", "ubpf_backend.html#ipv6-ipv6-actionsp4", null ]
        ] ],
        [ "Registers", "ubpf_backend.html#registers", [
          [ "Rate limiter (rate-limiter.p4)", "ubpf_backend.html#rate-limiter-rate-limiterp4", null ],
          [ "Rate limiter (rate-limiter-structs.p4)", "ubpf_backend.html#rate-limiter-rate-limiter-structsp4", null ],
          [ "Packet counter (packet-counter.p4)", "ubpf_backend.html#packet-counter-packet-counterp4", null ],
          [ "Simple firewall (simple-firewall.p4)", "ubpf_backend.html#simple-firewall-simple-firewallp4", null ]
        ] ],
        [ "Tunneling", "ubpf_backend.html#tunneling", [
          [ "VXLAN", "ubpf_backend.html#vxlan", null ],
          [ "GPRS Tunneling Protocol (GTP)", "ubpf_backend.html#gprs-tunneling-protocol-gtp", null ]
        ] ]
      ] ],
      [ "uBPF Backend testing", "ubpf_backend.html#ubpf-backend-testing", [
        [ "Steps to Run Tests:", "ubpf_backend.html#steps-to-run-tests", [
          [ "Known limitations", "ubpf_backend.html#known-limitations", null ],
          [ "Contact", "ubpf_backend.html#contact-1", null ]
        ] ]
      ] ]
    ] ],
    [ "P4test Backend", "p4test_backend.html", null ],
    [ "Graphs Backend", "graphs_backend.html", null ],
    [ "p4fmt (P4 Formatter)", "p4fmt.html", null ],
    [ "P4Tools", "p4tools.html", [
      [ "P4Tools Contributors", "p4tools.html#p4tools-contributors", null ],
      [ "Core Developers", "p4tools.html#core-developers", null ],
      [ "History", "p4tools.html#history", null ]
    ] ],
    [ "P4Smith", "p4smith.html", null ],
    [ "P4Testgen", "p4testgen.html", [
      [ "P4Testgen Benchmarks", "p4testgen.html#p4testgen-benchmarks", null ],
      [ "P4Testgen BMv2 target tests", "p4testgen.html#p4testgen-bmv2-target-tests", [
        [ "Features", "p4testgen.html#features", null ],
        [ "Installation", "p4testgen.html#installation-3", [
          [ "Dependencies", "p4testgen.html#dependencies-5", null ]
        ] ],
        [ "Extensions", "p4testgen.html#extensions-1", [
          [ "v1model.p4 on BMv2", "p4testgen.html#v1modelp4-on-bmv2", null ],
          [ "pna.p4 on the DPDK SoftNIC", "p4testgen.html#pnap4-on-the-dpdk-softnic-1", null ],
          [ "ebpf_model.p4 on the eBPF kernel target", "p4testgen.html#ebpf_modelp4-on-the-ebpf-kernel-target", null ]
        ] ],
        [ "Definitions", "p4testgen.html#definitions", null ],
        [ "Usage", "p4testgen.html#usage-3", [
          [ "Coverage", "p4testgen.html#coverage", null ],
          [ "Generating Specific Tests", "p4testgen.html#generating-specific-tests", [
            [ "Restricted Tests", "p4testgen.html#restricted-tests", null ],
            [ "Finding Assertion Violations", "p4testgen.html#finding-assertion-violations", null ]
          ] ],
          [ "Interacting with Test Frameworks", "p4testgen.html#interacting-with-test-frameworks", null ],
          [ "Detecting P4 Program Flaws", "p4testgen.html#detecting-p4-program-flaws", null ]
        ] ],
        [ "Limitations", "p4testgen.html#limitations", null ],
        [ "Further Reading", "p4testgen.html#further-reading-1", null ],
        [ "CMake Files", "p4testgen.html#cmake-files", null ],
        [ "How to Run tests", "p4testgen.html#how-to-run-tests", null ],
        [ "Contributing", "p4testgen.html#contributing-1", null ],
        [ "License", "p4testgen.html#license-1", null ]
      ] ]
    ] ],
    [ "Contribute to the P4 Compiler Project", "contribute.html", [
      [ "Coding Standard", "contribute.html#coding-standard", [
        [ "Contributing License", "contribute.html#contributing-license", null ],
        [ "Coding Standard Philosophy", "contribute.html#coding-standard-philosophy", null ],
        [ "How to Contribute", "contribute.html#how-to-contribute-1", [
          [ "Guidelines", "contribute.html#guidelines", null ],
          [ "Finding a Task", "contribute.html#finding-a-task", null ]
        ] ],
        [ "Reporting Issues", "contribute.html#reporting-issues", null ],
        [ "Feature Requests", "contribute.html#feature-requests", null ],
        [ "Commenting the code", "contribute.html#commenting-the-code", null ],
        [ "Handling errors", "contribute.html#handling-errors", null ],
        [ "Git commits and pull requests", "contribute.html#git-commits-and-pull-requests", null ]
      ] ]
    ] ],
    [ "CHANGELOG", "changelog.html", null ],
    [ "Topics", "topics.html", "topics" ],
    [ "Namespaces", "namespaces.html", [
      [ "Namespace List", "namespaces.html", "namespaces_dup" ],
      [ "Namespace Members", "namespacemembers.html", [
        [ "All", "namespacemembers.html", null ],
        [ "Functions", "namespacemembers_func.html", null ],
        [ "Variables", "namespacemembers_vars.html", null ],
        [ "Typedefs", "namespacemembers_type.html", null ],
        [ "Enumerations", "namespacemembers_enum.html", null ]
      ] ]
    ] ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Hierarchy", "hierarchy.html", "hierarchy" ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", "functions_dup" ],
        [ "Functions", "functions_func.html", "functions_func" ],
        [ "Variables", "functions_vars.html", "functions_vars" ],
        [ "Typedefs", "functions_type.html", null ],
        [ "Enumerations", "functions_enum.html", null ],
        [ "Enumerator", "functions_eval.html", null ],
        [ "Related Symbols", "functions_rela.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"annotated.html",
"class_p4_1_1_b_m_v2_1_1_parser_converter.html#ab74ae012876e276af5f4446bf4ca9e28",
"class_p4_1_1_control_plane_a_p_i_1_1_p4_info_maps.html#a75be747c5b0f32b7f77ebd127bcc5adb",
"class_p4_1_1_d_p_d_k_1_1_def_action_value.html#a68d36375f46be44ca07e3293d84af609",
"class_p4_1_1_do_convert_enums.html",
"class_p4_1_1_e_b_p_f_1_1_e_b_p_f_hash_algorithm_type_factory_p_s_a.html",
"class_p4_1_1_e_b_p_f_1_1_target.html#afc4be4383c4d809a1a4a1c630a9a5b76",
"class_p4_1_1_i_r_1_1_node_map.html",
"class_p4_1_1_p4_tools_1_1_abstract_p4c_tool.html",
"class_p4_1_1_p4_tools_1_1_p4_testgen_1_1_bmv2_1_1_bmv2_test_backend.html",
"class_p4_1_1_p4_tools_1_1_p4_testgen_1_1_bmv2_1_1_token.html#a519b9785610d0879025fcb2ed47c34c9",
"class_p4_1_1_p4_tools_1_1_p4_testgen_1_1_expr_stepper.html",
"class_p4_1_1_p4_tools_1_1_p4_testgen_1_1_pna_1_1_shared_pna_program_info.html#a3c824c6b0ab46286db304f0c3e6efa76",
"class_p4_1_1_p4_tools_1_1_p4_testgen_1_1_test_framework.html",
"class_p4_1_1_p4_tools_1_1_trace_events_1_1_method_call.html",
"class_p4_1_1_parsers_unroll.html",
"class_p4_1_1_simplify_def_use.html",
"class_p4_1_1_test_1_1_symbolic_converter.html",
"class_p4_1_1_v1_1_1_v1_parser_driver.html#a9ac7e4ecd7b9eef00855f5b018ec936f",
"ebpf_backend.html#lpm",
"namespace_p4_1_1_control_plane_a_p_i.html",
"struct_p4_1_1_closed_range.html#a6077cc9781ca281322fc4e19fd0504bd",
"struct_p4_1_1_half_open_range.html#a7536d29555335b4d5641f1e838bf22ae",
"struct_p4_1_1_util_1_1_hasher_3_01unsigned_01char_01_4.html"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';