# Copyright 2013-2024 Intel Corporation.
#
# This software and the related documents are Intel copyrighted materials,
# and your use of them is governed by the express license under which they
# were provided to you ("License"). Unless the License provides otherwise,
# you may not use, modify, copy, publish, distribute, disclose or transmit this
# software or the related documents without Intel's prior written permission.
#
# This software and the related documents are provided as is, with no
# express or implied warranties, other than those that are expressly stated
# in the License.

####### Barefoot back-end

MESSAGE("-- Adding p4c-barefoot")

# Try to find Z3 in the system, only use the static Z3 library.
set(Z3_MIN_VERSION "4.8.8")
# TODO: Move this option into the Z3 CMake module.
set(_z3_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
find_package(Z3 ${Z3_MIN_VERSION} REQUIRED)
set(CMAKE_FIND_LIBRARY_SUFFIXES ${_z3_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})

# Set dynamic hash include directory
set(LIBDYNHASH_INCLUDE_DIR "${BFN_P4C_SOURCE_DIR}/bf-utils/include")
include_directories(${LIBDYNHASH_INCLUDE_DIR})

# An option to configure the chunk size we use for unified builds in bf-p4c. By
# default, this is tuned to use smaller chunks than the p4c repo uses. The goal
# is to gain most of the benefit of unified builds when many files change, while
# keeping the impact on recompilation time for individual files to a minimum.
set(BF_P4C_UNIFIED_SOURCE_CHUNK_SIZE "5" CACHE STRING
    "Target unified compilation chunk size for bf-p4c (an integer or ALL)")

find_package (Boost REQUIRED COMPONENTS graph)
add_definitions("-DBOOST_NO_ARGUMENT_DEPENDENT_LOOKUP")
if (ENABLE_BAREFOOT_INTERNAL)
  add_definitions("-DBAREFOOT_INTERNAL=1")
endif()

if (ENABLE_MULTITHREAD)
  add_definitions("-DGC_THREADS=1")
  add_definitions("-DGC_NO_THREAD_REDIRECTS=1")
endif()

# We need to abstract away the targets enumerated in the schema files,
# so that we do not tip our hand on future targets.
# The design is to create a file compiler-interfaces/schemas/targets.py
# that defines the list of targets (and p4 architectures supported). If
# the file is present, use it. Otherwise, use a predefined version of the
# list with only the targets that are exposed
# Note that we need to do this early, before we process any of the rules
# using schemas (context, manifest, phv, etc.) and packaging the compiler
# interfaces tools.
set (INTERFACES_DIR ${BFN_P4C_SOURCE_DIR}/compiler_interfaces)
set(COMPILER_SCHEMA_TARGETS "${INTERFACES_DIR}/schemas/targets.py")
  set(DEFAULT_SCHEMA_TARGETS "['tofino', 'tofino2', 'tofino2m', 'tofino2u', 'tofino2a0']")
  set(DEFAULT_SCHEMA_P4ARCH "['tna', 't2na', 'psa', 'PISA', 'v1model']")
file(WRITE ${COMPILER_SCHEMA_TARGETS} "TARGETS=${DEFAULT_SCHEMA_TARGETS}\n")
file(APPEND ${COMPILER_SCHEMA_TARGETS} "P4ARCHITECTURES=${DEFAULT_SCHEMA_P4ARCH}\n")

get_schema_version(manifest_schema MANIFEST_SCHEMA_VERSION)
MESSAGE(STATUS "Found manifest schema version ${MANIFEST_SCHEMA_VERSION}")
set_source_files_properties(logging/manifest.cpp PROPERTIES
  COMPILE_DEFINITIONS MANIFEST_SCHEMA_VERSION=\"${MANIFEST_SCHEMA_VERSION}\")

get_schema_version(event_log_schema EVENT_LOG_SCHEMA_VERSION)
MESSAGE(STATUS "Found event log schema version ${EVENT_LOG_SCHEMA_VERSION}")
add_definitions("-DEVENT_LOG_SCHEMA_VERSION=\"${EVENT_LOG_SCHEMA_VERSION}\"")

get_schema_version(phv_schema PHV_SCHEMA_VERSION)
MESSAGE(STATUS "Found phv schema version ${PHV_SCHEMA_VERSION}")
add_definitions("-DPHV_SCHEMA_VERSION=\"${PHV_SCHEMA_VERSION}\"")

get_schema_version(power_schema POWER_SCHEMA_VERSION)
MESSAGE(STATUS "Found power schema version ${POWER_SCHEMA_VERSION}")
add_definitions("-DPOWER_SCHEMA_VERSION=\"${POWER_SCHEMA_VERSION}\"")

get_schema_version(context_schema CONTEXT_SCHEMA_VERSION)
MESSAGE(STATUS "Found context schema version ${CONTEXT_SCHEMA_VERSION}")
if (CMAKE_UNITY_BUILD)
  add_definitions("-DCONTEXT_SCHEMA_VERSION=\"${CONTEXT_SCHEMA_VERSION}\"")
else()
  set_source_files_properties(p4c-barefoot.cpp PROPERTIES
    COMPILE_DEFINITIONS CONTEXT_SCHEMA_VERSION=\"${CONTEXT_SCHEMA_VERSION}\")
endif()

get_schema_version(source_info_schema SOURCE_INFO_SCHEMA_VERSION)
MESSAGE(STATUS "Found source info schema version ${SOURCE_INFO_SCHEMA_VERSION}")
add_definitions("-DSOURCE_INFO_SCHEMA_VERSION=\"${SOURCE_INFO_SCHEMA_VERSION}\"")

get_schema_version(resources_schema RESOURCES_SCHEMA_VERSION)
MESSAGE(STATUS "Found source info schema version ${RESOURCES_SCHEMA_VERSION}")
add_definitions("-DRESOURCES_SCHEMA_VERSION=\"${RESOURCES_SCHEMA_VERSION}\"")

include_directories(${BFN_P4C_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS} ${LIBDYNHASH_INCLUDE_DIR} ${Z3_INCLUDE_DIR} )
# The generated code for protobuf has an excessive number of warnings so we
# include the build directory as a system directory
include_directories(SYSTEM ${P4C_BINARY_DIR}/control-plane)
set (HAVE_LIBBOOST_GRAPH 1)
set (P4C_LIB_DEPS "${P4C_LIB_DEPS};${Boost_GRAPH_LIBRARY};${LIBDYNHASH_LIBRARY};${Z3_LIBRARY}")
set (P4C_LIB_DEPS ${P4C_LIB_DEPS} PARENT_SCOPE)

add_subdirectory(logging)
include_directories(logging)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/logging)

set_source_files_properties(${BF_P4C_IR_SRCS} PROPERTIES
  COMPILE_FLAGS "-I${Z3_INCLUDE_DIR}")

set (BF_P4C_MIDEND_SRCS
  midend/annotate_with_in_hash.cpp
  midend/check_design_pattern.cpp
  midend/check_header_alignment.cpp
  midend/check_register_actions.cpp
  midend/check_unsupported.cpp
  midend/copy_block_pragmas.cpp
  midend/copy_header.cpp
  midend/defuse.cpp
  midend/drop_packet_with_mirror_engine.cpp
  midend/elim_cast.cpp
  midend/eliminate_tuples.cpp
  midend/fold_constant_hashes.cpp
  midend/desugar_varbit_extract.cpp
  midend/initialize_mirror_io_select.cpp
  midend/move_to_egress.cpp
  midend/normalize_hash_list.cpp
  midend/normalize_params.cpp
  midend/param_binding.cpp
  midend/parser_enforce_depth_req.cpp
  midend/parser_graph.cpp
  midend/detect_multiple_pipelines.cpp
  midend/collect_pipelines.cpp
  midend/ping_pong_generation.cpp
  midend/register_read_write.cpp
  midend/simplify_args.cpp
  midend/simplifyIfStatement.cpp
  midend/simplify_nested_if.cpp
  midend/simplify_references.cpp
  midend/alpm.cpp
  midend/table_mutex.cpp
  midend/type_categories.cpp
  midend/type_checker.cpp
  )
add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${BF_P4C_MIDEND_SRCS}")

set (BF_P4C_MIDEND_HDRS
  midend/action_synthesis_policy.h
  midend/annotate_with_in_hash.h
  midend/blockmap.h
  midend/check_design_pattern.h
  midend/check_header_alignment.h
  midend/check_register_actions.h
  midend/check_unsupported.h
  midend/copy_block_pragmas.h
  midend/copy_header.h
  midend/defuse.h
  midend/drop_packet_with_mirror_engine.h
  midend/elim_cast.h
  midend/eliminate_tuples.h
  midend/fold_constant_hashes.h
  midend/desugar_varbit_extract.h
  midend/initialize_mirror_io_select.h
  midend/move_to_egress.h
  midend/normalize_hash_list.h
  midend/normalize_params.h
  midend/param_binding.h
  midend/parser_enforce_depth_req.h
  midend/parser_graph.h
  midend/register_read_write.h
  midend/detect_multiple_pipelines.h
  midend/ping_pong_generation.h
  midend/rewrite_egress_intrinsic_metadata_header.h
  midend/simplify_args.h
  midend/simplifyIfStatement.h
  midend/simplify_key_policy.h
  midend/simplify_nested_if.h
  midend/simplify_references.h
  midend/alpm.h
  midend/table_mutex.h
  midend/type_categories.h
  midend/type_checker.h
  )
add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${BF_P4C_MIDEND_HDRS}")

set (BF_P4C_BACKEND_CONV_SRCS
  arch/fromv1.0/field_list.cpp
  arch/fromv1.0/lpf.cpp
  arch/fromv1.0/meter.cpp
  arch/fromv1.0/primitives.cpp
  arch/fromv1.0/programStructure.cpp
  arch/fromv1.0/stateful_alu.cpp
  arch/fromv1.0/wred.cpp
  arch/tna/primitives.cpp
  )
add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${BF_P4C_BACKEND_CONV_SRCS}")

set (BF_P4C_BACKEND_CONV_HDRS
  arch/fromv1.0/field_list.h
  arch/fromv1.0/lpf.h
  arch/fromv1.0/meter.h
  arch/fromv1.0/programStructure.h
  arch/fromv1.0/stateful_alu.h
  arch/fromv1.0/wred.h
  )
add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${BF_P4C_BACKEND_CONV_HDRS}")

# IR sources that need to go into the frontend library
set(QUAL_BF_P4C_IR_SRCS)
foreach(src IN LISTS BF_P4C_IR_SRCS)
  set(QUAL_BF_P4C_IR_SRCS ${QUAL_BF_P4C_IR_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/${src})
endforeach()
set(EXTENSION_IR_SOURCES ${EXTENSION_IR_SOURCES} ${QUAL_BF_P4C_IR_SRCS} PARENT_SCOPE)

set (BF_P4C_BACKEND_COMMON_SRCS
  common/alias.cpp
  common/asm_output.cpp
  common/check_field_corruption.cpp
  common/check_for_unimplemented_features.cpp
  common/check_uninitialized_read.cpp
  common/dump_pipe.cpp
  common/pragma/collect_global_pragma.cpp
  common/elim_unused.cpp
  common/empty_tableseq.cpp
  common/extract_maupipe.cpp
  common/field_defuse.cpp

  common/bridged_packing.cpp
  common/header_stack.cpp
  common/ir_utils.cpp
  common/map_tables_to_actions.cpp
  common/merge_pov_bits.cpp
  common/multiple_apply.cpp
  common/pragma/pragmas.cpp
  common/run_id.cpp
  common/scc_toposort.cpp
  common/slice.cpp
  common/size_of.cpp
  common/utils.cpp
  )
add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${BF_P4C_BACKEND_COMMON_SRCS}")

set (BF_P4C_BACKEND_COMMON_HDRS
  common/alias.h
  common/autoindent.h
  common/check_field_corruption.h
  common/check_for_unimplemented_features.h
  common/check_header_refs.h
  common/check_uninitialized_read.h
  common/pragma/collect_global_pragma.h
  common/debug_info.h
  common/elim_unused.h
  common/empty_tableseq.h
  common/extract_maupipe.h
  common/field_defuse.h

  common/front_end_policy.h
  common/bridged_packing.h
  common/header_stack.h
  common/ir_utils.h
  common/map_tables_to_actions.h
  common/merge_pov_bits.h
  common/multiple_apply.h
  common/parse_annotations.h
  common/pragma/all_pragmas.h
  common/run_id.h
  common/scc_toposort.h
  common/slice.h
  common/size_of.h
  common/table_printer.h
  common/utils.h
  )
add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${BF_P4C_BACKEND_COMMON_HDRS}")

set (BF_P4C_BACKEND_CONTROL_PLANE_SRCS
  control-plane/bfruntime_ext.cpp
  control-plane/p4runtime_force_std.cpp
  control-plane/bfruntime.cpp
  control-plane/runtime.cpp
  )
add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${BF_P4C_BACKEND_CONTROL_PLANE_SRCS}")

set (BF_P4C_BACKEND_CONTROL_PLANE_HDRS
  control-plane/bfruntime_arch_handler.h
  control-plane/bfruntime_ext.h
  control-plane/bfruntime.h
  control-plane/p4runtime_force_std.h
  control-plane/runtime.h
  )
add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${BF_P4C_BACKEND_CONTROL_PLANE_HDRS}")

set (BF_P4C_BACKEND_LIB_SRCS
  lib/error_type.cpp
)
add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${BF_P4C_BACKEND_LIB_SRCS}")

set (BF_P4C_BACKEND_LIB_HDRS
  lib/autoclone.h
  lib/boost_graph.h
  lib/cmp.h
  lib/dyn_vector.h
  lib/error_macros.h
  lib/error_type.h
  lib/log_fixup.h
  lib/pad_alignment.h
  lib/pointer_wrapper.h
  lib/safe_width.h
  lib/union_find.hpp
  )
add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${BF_P4C_BACKEND_LIB_HDRS}")

set (BF_P4C_BACKEND_MAU_SRCS
  mau/action_analysis.cpp
  mau/action_data_bus.cpp
  mau/action_format.cpp
  mau/action_mutex.cpp
  mau/adjust_byte_count.cpp
  mau/add_always_run.cpp
  mau/asm_output.cpp
  mau/asm_hash_output.cpp
  mau/attached_info.cpp
  mau/attached_output.cpp
  mau/build_power_graph.cpp
  mau/determine_power_usage.cpp
  mau/dynamic_dep_metrics.cpp
  mau/dynhash.cpp
  mau/field_use.cpp
  mau/finalize_mau_pred_deps_power.cpp
  mau/db-gateway.cpp
  mau/dump_json_graph.cpp
  mau/gateway.cpp
  mau/gen_prim_json.cpp
  mau/handle_assign.cpp
  mau/input_xbar.cpp
  mau/instruction_adjustment.cpp
  mau/instruction_memory.cpp
  mau/instruction_selection.cpp
  mau/ixbar_expr.cpp
  mau/ixbar_info.cpp
  mau/ixbar_realign.cpp
  mau/jbay_next_table.cpp
  mau/mau_alloc.cpp
  mau/mau_power.cpp
  mau/mau_spec.cpp
  mau/memories.cpp
  mau/payload_gateway.cpp
  mau/reduction_or.cpp
  mau/resource_estimate.cpp
  mau/resource.cpp
  mau/selector_update.cpp
  mau/simple_power_graph.cpp
  mau/split_gateways.cpp
  mau/stateful_alu.cpp
  mau/table_control_deps.cpp
  mau/table_dependency_graph.cpp
  mau/table_format.cpp
  mau/table_layout.cpp
  mau/table_mutex.cpp
  mau/table_placement.cpp
  mau/table_seqdeps.cpp
  mau/table_summary.cpp
  mau/table_injected_deps.cpp
  mau/table_flow_graph.cpp
  mau/validate_actions.cpp
  mau/walk_power_graph.cpp
  )
add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${BF_P4C_BACKEND_MAU_SRCS}")

set (BF_P4C_BACKEND_MAU_TOFINO_SRCS
  mau/tofino/action_data_bus.cpp
  mau/tofino/asm_output.cpp
  mau/tofino/input_xbar.cpp
  mau/tofino/mau_spec.cpp
  mau/tofino/memories.cpp
  )
add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${BF_P4C_BACKEND_MAU_TOFINO_SRCS}")


set (BF_P4C_BACKEND_MAU_HDRS
  mau/action_analysis.h
  mau/action_data_bus.h
  mau/action_format.h
  mau/action_mutex.h
  mau/adjust_byte_count.h
  mau/add_always_run.h
  mau/asm_output.h
  mau/asm_hash_output.h
  mau/attached_entries.h
  mau/attached_info.h
  mau/attached_output.h
  mau/build_power_graph.h
  mau/default_next.h
  mau/determine_power_usage.h
  mau/dump_json_graph.h
  mau/dynamic_dep_metrics.h
  mau/dynhash.h
  mau/field_use.h
  mau/finalize_mau_pred_deps_power.h
  mau/gateway.h
  mau/gen_prim_json.h
  mau/handle_assign.h
  mau/input_xbar.h
  mau/instruction_adjustment.h
  mau/instruction_memory.h
  mau/instruction_selection.h
  mau/ixbar_expr.h
  mau/ixbar_info.h
  mau/ixbar_realign.h
  mau/jbay_next_table.h
  mau/mau_alloc.h
  mau/mau_power.h
  mau/memories.h
  mau/next_table.h
  mau/payload_gateway.h
  mau/push_pop.h
  mau/reduction_or.h
  mau/remove_noop_gateway.h
  mau/resource_estimate.h
  mau/resource.h
  mau/selector_update.h
  mau/simple_power_graph.h
  mau/split_gateways.h
  mau/stateful_alu.h
  mau/static_entries_const_prop.h
  mau/table_control_deps.h
  mau/table_dependency_graph.h
  mau/table_flow_graph.h
  mau/table_format.h
  mau/table_layout.h
  mau/table_mutex.h
  mau/table_placement.h
  mau/table_seqdeps.h
  mau/table_summary.h
  mau/table_injected_deps.h
  mau/validate_actions.h
  mau/walk_power_graph.h
  )
add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${BF_P4C_BACKEND_MAU_HDRS}")

set (BF_P4C_BACKEND_MAU_TOFINO_HDRS
  mau/tofino/action_data_bus.h
  mau/tofino/asm_output.h
  mau/tofino/input_xbar.h
  mau/tofino/memories.h
  )
add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${BF_P4C_BACKEND_MAU_TOFINO_HDRS}")


set (BF_P4C_BACKEND_PARDE_SRCS
  parde/add_metadata_pov.cpp
  parde/add_parde_metadata.cpp
  parde/adjust_extract.cpp
  parde/allocate_parser_checksum.cpp
  parde/allocate_parser_match_register.cpp
  parde/check_parser_multi_write.cpp
  parde/coalesce_learning.cpp
  parde/count_strided_header_refs.cpp
  parde/create_pov_encoder.cpp
  parde/decaf.cpp
  parde/deparser_checksum_update.cpp
  parde/deparser_output.cpp
  parde/extract_parser.cpp
  parde/extract_deparser.cpp
  parde/field_packing.cpp
  parde/header_output.cpp
  parde/infer_payload_offset.cpp
  parde/lower_parser.cpp
  parde/merge_parser_state.cpp
  parde/parser_dominator_builder.cpp
  parde/parser_header_sequences.cpp
  parde/parser_loops_info.cpp
  parde/parser_output.cpp
  parde/parser_query.cpp
  parde/phase0.cpp
  parde/rewrite_parser_locals.cpp
  parde/split_parser_state.cpp
  parde/clot/allocate_clot.cpp
  parde/clot/check_clot_groups.cpp
  parde/clot/clot_candidate.cpp
  parde/clot/clot_info.cpp
  parde/clot/deparse_graph.cpp
  parde/clot/field_slice_extract_info.cpp
  parde/clot/field_pov_analysis.cpp
  parde/clot/header_validity_analysis.cpp
  parde/clot/merge_desugared_varbit_valids.cpp
  parde/clot/pseudoheader.cpp
  parde/clot/pragma/do_not_use_clot.cpp
  parde/lowered/compute_buffer_requirements.cpp
  parde/lowered/compute_init_zero_containers.cpp
  parde/lowered/compute_lowered_deparser_ir.cpp
  parde/lowered/compute_lowered_parser_ir.cpp
  parde/lowered/compute_multi_write_containers.cpp
  parde/lowered/eliminate_empty_states.cpp
  parde/lowered/extract_simplifier.cpp
  parde/lowered/helpers.cpp
  parde/lowered/hoist_common_match_operations.cpp
  parde/lowered/merge_lowered_parser_states.cpp
  parde/lowered/rewrite_emit_clot.cpp
  parde/lowered/split_greedy_parser_states.cpp
  parde/mirror/const_mirror_session_opt.cpp
  )
add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${BF_P4C_BACKEND_PARDE_SRCS}")

set (BF_P4C_BACKEND_PARDE_HDRS
  parde/add_metadata_pov.h
  parde/add_parde_metadata.h
  parde/allocate_parser_checksum.h
  parde/allocate_parser_match_register.h
  parde/adjust_extract.h
  parde/asm_output.h
  parde/characterize_parser.h
  parde/check_parser_multi_write.h
  parde/coalesce_learning.h
  parde/collect_parser_usedef.h
  parde/count_strided_header_refs.h
  parde/create_pov_encoder.h
  parde/decaf.h
  parde/deparser_checksum_update.h
  parde/dump_parser.h
  parde/extract_parser.h
  parde/extract_deparser.h
  parde/field_packing.h

  parde/infer_payload_offset.h
  parde/lower_parser.h
  parde/marshal.h
  parde/match_register.h
  parde/merge_parser_state.h
  parde/parde_spec.h
  parde/parser_dominator_builder.h
  parde/parser_header_sequences.h
  parde/parser_loops_info.h
  parde/phase0.h
  parde/parde_utils.h
  parde/parser_info.h
  parde/parser_query.h
  parde/reset_invalidated_checksum_headers.h
  parde/resolve_negative_extract.h
  parde/rewrite_parser_locals.h
  parde/split_parser_state.h
  parde/update_parser_write_mode.h
  parde/clot/allocate_clot.h
  parde/clot/check_clot_groups.h
  parde/clot/clot_candidate.h
  parde/clot/clot_info.h
  parde/clot/deparse_graph.h
  parde/clot/field_slice_extract_info.h
  parde/clot/field_slice_set.h
  parde/clot/field_pov_analysis.h
  parde/clot/header_validity_analysis.h
  parde/clot/merge_desugared_varbit_valids.h
  parde/clot/pseudoheader.h
  parde/clot/pragma/do_not_use_clot.h
  parde/common/allocators.h
  parde/common/match_reducer.h
  parde/lowered/compute_buffer_requirements.h
  parde/lowered/compute_init_zero_containers.h
  parde/lowered/compute_lowered_deparser_ir.h
  parde/lowered/compute_lowered_parser_ir.h
  parde/lowered/compute_multi_write_containers.h
  parde/lowered/eliminate_empty_states.h
  parde/lowered/extract_simplifier.h
  parde/lowered/helpers.h
  parde/lowered/hoist_common_match_operations.h

  parde/lowered/merge_lowered_parser_states.h
  parde/lowered/rewrite_emit_clot.h
  parde/lowered/split_greedy_parser_states.h
  parde/mirror/const_mirror_session_opt.h
  )
add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${BF_P4C_BACKEND_PARDE_HDRS}")

set (BF_P4C_BACKEND_PHV_SRCS
  phv/check_unallocated.cpp
  phv/constraints/constraints.cpp
  phv/action_phv_constraints.cpp
  phv/add_initialization.cpp
  phv/add_special_constraints.cpp
  phv/allocate_phv.cpp
  phv/allocate_temps_and_finalize_liverange.cpp
  phv/asm_output.cpp
  phv/auto_init_metadata.cpp
  phv/cluster_phv_operations.cpp
  phv/create_thread_local_instances.cpp
  phv/collect_table_keys.cpp
  phv/finalize_physical_liverange.cpp
  phv/finalize_stage_allocation.cpp
  phv/live_range_split.cpp
  phv/make_clusters.cpp
  phv/mau_backtracker.cpp
  phv/optimize_phv.cpp
  phv/legacy_action_packing_validator.cpp
  phv/parde_phv_constraints.cpp
  phv/add_alias_allocation.cpp
  phv/phv_analysis.cpp
  phv/phv_fields.cpp
  phv/phv_parde_mau_use.cpp
  phv/phv_spec.cpp
  phv/split_padding.cpp
  phv/table_phv_constraints.cpp
  phv/validate_allocation.cpp
  phv/analysis/build_mutex.cpp
  phv/analysis/critical_path_clusters.cpp
  phv/analysis/dark_live_range.cpp
  phv/analysis/deparser_zero.cpp
  phv/analysis/dominator_tree.cpp
  phv/analysis/header_mutex.cpp
  phv/analysis/memoize_min_stage.cpp
  phv/analysis/meta_live_range.cpp
  phv/analysis/mutex_overlay.cpp
  phv/analysis/non_mocha_dark_fields.cpp
  phv/analysis/live_range_shrinking.cpp
  phv/analysis/pack_conflicts.cpp
  phv/analysis/parser_critical_path.cpp
  phv/analysis/dark.cpp
  phv/analysis/jbay_phv_analysis.cpp
  phv/analysis/mocha.cpp
  phv/pragma/phv_pragmas.cpp
  phv/pragma/pa_alias.cpp
  phv/pragma/pa_atomic.cpp
  phv/pragma/pa_byte_pack.cpp
  phv/pragma/pa_container_size.cpp
  phv/pragma/pa_container_type.cpp
  phv/pragma/pa_deparser_zero.cpp
  phv/pragma/pa_mutually_exclusive.cpp
  phv/pragma/pa_no_init.cpp
  phv/pragma/pa_no_overlay.cpp
  phv/pragma/pa_no_pack.cpp
  phv/pragma/pa_solitary.cpp
  phv/action_source_tracker.cpp
  phv/slicing/phv_slicing_split.cpp
  phv/slicing/phv_slicing_iterator.cpp
  phv/slicing/phv_slicing_dfs_iterator.cpp
  phv/solver/action_constraint_solver.cpp
  phv/solver/symbolic_bitvec.cpp
  phv/transforms/auto_alias.cpp
  phv/utils/live_range_report.cpp
  phv/utils/report.cpp
  phv/utils/slice_alloc.cpp
  phv/utils/utils.cpp
  phv/utils/liverange_opti_utils.cpp
  phv/utils/container_equivalence.cpp
  phv/fieldslice_live_range.cpp
  phv/v2/phv_kit.cpp
  phv/v2/allocator_base.cpp
  phv/v2/copacker.cpp
  phv/v2/phv_allocation_v2.cpp
  phv/v2/parser_packing_validator.cpp
  phv/v2/action_packing_validator.cpp
  phv/v2/greedy_allocator.cpp
  phv/v2/kind_size_indexed_map.cpp
  phv/v2/table_replay_friendly_constraints.cpp
  phv/v2/trivial_allocator.cpp
  phv/v2/tx_score.cpp
  phv/v2/greedy_tx_score.cpp
  phv/v2/utils_v2.cpp
  phv/v2/metadata_initialization.cpp
  phv/v2/smart_packing.cpp
  phv/v2/types.cpp
  )

add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${BF_P4C_BACKEND_PHV_SRCS}")

set (BF_P4C_BACKEND_PHV_HDRS
  phv/dump_table_flow_graph.h
  phv/constraints/constraints.h
  phv/action_phv_constraints.h
  phv/add_initialization.h
  phv/add_special_constraints.h
  phv/init_in_mau.h
  phv/allocate_phv.h
  phv/allocate_temps_and_finalize_liverange.h
  phv/error.h
  phv/asm_output.h
  phv/auto_init_metadata.h
  phv/check_unallocated.h
  phv/cluster_phv_operations.h
  phv/collect_strided_headers.h
  phv/collect_table_keys.h
  phv/create_thread_local_instances.h
  phv/parser_extract_balance_score.h
  phv/finalize_physical_liverange.h
  phv/finalize_stage_allocation.h
  phv/live_range_split.h
  phv/make_clusters.h
  phv/mau_backtracker.h
  phv/optimize_phv.h
  phv/action_packing_validator_interface.h
  phv/parser_packing_validator_interface.h
  phv/legacy_action_packing_validator.h
  phv/parde_phv_constraints.h
  phv/add_alias_allocation.h
  phv/phv_analysis.h
  phv/phv_fields.h
  phv/phv_parde_mau_use.h
  phv/phv_spec.h
  phv/split_padding.h
  phv/table_phv_constraints.h
  phv/validate_allocation.h
  phv/analysis/build_mutex.h
  phv/analysis/critical_path_clusters.h
  phv/analysis/dark_live_range.h
  phv/analysis/deparser_zero.h
  phv/analysis/dominator_tree.h
  phv/analysis/header_mutex.h
  phv/analysis/memoize_min_stage.h
  phv/analysis/meta_live_range.h
  phv/analysis/mutex_overlay.h
  phv/analysis/non_mocha_dark_fields.h
  phv/analysis/live_range_shrinking.h
  phv/analysis/pack_conflicts.h
  phv/analysis/parser_critical_path.h
  phv/analysis/dark.h
  phv/analysis/jbay_phv_analysis.h
  phv/analysis/mocha.h
  phv/pragma/pa_alias.h
  phv/pragma/pa_atomic.h
  phv/pragma/pa_byte_pack.h
  phv/pragma/pa_container_size.h
  phv/pragma/pa_container_type.h
  phv/pragma/pa_deparser_zero.h
  phv/pragma/pa_mutually_exclusive.h
  phv/pragma/pa_no_init.h
  phv/pragma/pa_no_overlay.h
  phv/pragma/pa_no_pack.h
  phv/pragma/pa_solitary.h
  phv/pragma/phv_pragmas.h
  phv/pragma/pretty_print.h
  phv/action_source_tracker.h
  phv/slicing/types.h
  phv/slicing/phv_slicing_split.h
  phv/slicing/phv_slicing_iterator.h
  phv/slicing/phv_slicing_dfs_iterator.h
  phv/solver/symbolic_bitvec.h
  phv/solver/action_constraint_solver.h
  phv/transforms/auto_alias.h
  phv/utils/live_range_report.h
  phv/utils/report.h
  phv/utils/slice_alloc.h
  phv/utils/tables_to_ids.h
  phv/utils/utils.h
  phv/utils/liverange_opti_utils.h
  phv/fieldslice_live_range.h
  phv/alloc_setting.h
  phv/v2/phv_kit.h
  phv/v2/allocator_base.h
  phv/v2/allocator_metrics.h
  phv/v2/copacker.h
  phv/v2/phv_allocation_v2.h
  phv/v2/parser_packing_validator.h
  phv/v2/action_packing_validator.h
  phv/v2/greedy_allocator.h
  phv/v2/kind_size_indexed_map.h
  phv/v2/table_replay_friendly_constraints.cpp
  phv/v2/trivial_allocator.h
  phv/v2/tx_score.h
  phv/v2/greedy_tx_score.h
  phv/v2/utils_v2.h
  phv/v2/sort_macros.h
  phv/v2/metadata_initialization.h
  phv/v2/smart_packing.h
  phv/v2/types.h
  )
add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${BF_P4C_BACKEND_PHV_HDRS}")

set (BF_P4C_BACKEND_ARCH_SRCS
  arch/arch.cpp
  arch/arch_spec.cpp
  arch/add_t2na_meta.cpp
  arch/intrinsic_metadata.cpp
  arch/bridge_metadata.cpp
  arch/check_extern_invocation.cpp
  arch/collect_bridged_fields.cpp
  arch/collect_hardware_constrained_fields.cpp
  arch/helpers.cpp
  arch/program_structure.cpp
  arch/psa/psa.cpp
  arch/psa/psa_converters.cpp
  arch/psa/psa_model.cpp
  arch/psa/programStructure.cpp
  arch/psa/rewrite_packet_path.cpp
  arch/psa/rewrite_bridge_metadata.cpp
  arch/remove_set_metadata.cpp
  arch/rewrite_action_selector.cpp
  arch/tna.cpp
  arch/t2na.cpp
  arch/v1model.cpp
  arch/fromv1.0/add_metadata_parser_states.cpp
  arch/fromv1.0/egress_packet_length.cpp
  arch/fromv1.0/checksum.cpp
  arch/fromv1.0/mirror.cpp
  arch/fromv1.0/parser_counter.cpp
  arch/fromv1.0/phase0.cpp
  arch/fromv1.0/resubmit.cpp
  arch/fromv1.0/v1_converters.cpp
  arch/fromv1.0/v1_program_structure.cpp
  )
add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${BF_P4C_BACKEND_ARCH_SRCS}")
add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "arch/t2na.cpp")


set (BF_P4C_BACKEND_ARCH_HDRS
  arch/arch.h
  arch/arch_spec.h
  arch/add_t2na_meta.h
  arch/intrinsic_metadata.h
  arch/bridge_metadata.h
  arch/check_extern_invocation.h
  arch/collect_bridged_fields.h
  arch/collect_hardware_constrained_fields.h
  arch/helpers.h
  arch/program_structure.h
  arch/psa/psa.h
  arch/psa/psa_converters.h
  arch/psa/psa_model.h
  arch/psa/programStructure.h
  arch/psa/rewrite_packet_path.h
  arch/psa/rewrite_bridge_metadata.h
  arch/remove_set_metadata.h
  arch/rewrite_action_selector.h
  arch/tna.h
  arch/t2na.h


  arch/v1model.h
  arch/fromv1.0/add_metadata_parser_states.h
  arch/fromv1.0/egress_packet_length.h
  arch/fromv1.0/checksum.h
  arch/fromv1.0/mirror.h
  arch/fromv1.0/parser_counter.h
  arch/fromv1.0/phase0.h
  arch/fromv1.0/resubmit.h
  arch/fromv1.0/v1_converters.h
  arch/fromv1.0/v1_program_structure.h
  )
add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${BF_P4C_BACKEND_ARCH_HDRS}")

set (BF_P4C_BACKEND_LOGGING_HDRS
  logging/bf_error_reporter.h
  logging/collect_diagnostic_checks.h
  logging/constrained_fields.h
  logging/container_size_extractor.h
  logging/event_logger.h
  logging/filelog.h
  logging/logging.h
  logging/manifest.h
  logging/group_constraint_extractor.h
  logging/pass_manager.h
  logging/phv_logging.h
  logging/resources.h
  logging/resources_parser.h
  logging/resources_clot.h
  logging/source_info_logging.h
  )
add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${BF_P4C_BACKEND_LOGGING_HDRS}")
set (BF_P4C_BACKEND_LOGGING_SRCS
  logging/bf_error_reporter.cpp
  logging/collect_diagnostic_checks.cpp
  logging/constrained_fields.cpp
  logging/container_size_extractor.cpp
  logging/event_logger.cpp
  logging/filelog.cpp
  logging/manifest.cpp
  logging/group_constraint_extractor.cpp
  logging/pass_manager.cpp
  logging/phv_logging.cpp
  logging/resources.cpp
  logging/resources_parser.cpp
  logging/resources_clot.cpp
  logging/source_info_logging.cpp
  )
add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${BF_P4C_BACKEND_LOGGING_SRCS}")

set (BF_P4C_BACKEND_MAIN_SRCS
  asm.cpp
  backend.cpp
  bf-p4c-options.cpp
  device.cpp
  midend.cpp
  )
add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${BF_P4C_BACKEND_MAIN_SRCS}")

set (BF_P4C_BACKEND_MAIN_HDRS
  asm.h
  backend.h
  bf-p4c-options.h
  device.h
  midend.h
  )
add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${BF_P4C_BACKEND_MAIN_HDRS}")

set (BF_P4C_IR_DEF_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/bf-p4c/ir/tofino.def
  ${CMAKE_CURRENT_SOURCE_DIR}/bf-p4c/ir/arch.def
  ${CMAKE_CURRENT_SOURCE_DIR}/bf-p4c/ir/mau.def
  ${CMAKE_CURRENT_SOURCE_DIR}/bf-p4c/ir/parde.def
  ${CMAKE_CURRENT_SOURCE_DIR}/bf-p4c/ir/parde-lowered.def

  )
# publish IR_DEF_FILES upstream
set (IR_DEF_FILES ${IR_DEF_FILES} ${BF_P4C_IR_DEF_FILES} PARENT_SCOPE)

# do not add to cpp lint. They are added as part of the p4c-graphs backend
set (BF_P4C_GRAPHS_HDRS
  ${P4C_SOURCE_DIR}/backends/graphs/controls.h
  ${P4C_SOURCE_DIR}/backends/graphs/graphs.h
  ${P4C_SOURCE_DIR}/backends/graphs/parsers.h
  ${P4C_SOURCE_DIR}/backends/graphs/graph_visitor.h
  )
set (BF_P4C_GRAPHS_SRCS
  ${P4C_SOURCE_DIR}/backends/graphs/controls.cpp
  ${P4C_SOURCE_DIR}/backends/graphs/graphs.cpp
  ${P4C_SOURCE_DIR}/backends/graphs/parsers.cpp
  ${P4C_SOURCE_DIR}/backends/graphs/graph_visitor.cpp
  )

set (p4include_HEADERS
  p4include/tofino1_specs.p4
  p4include/tofino1_base.p4
  p4include/tofino1_arch.p4
  p4include/tna.p4
  p4include/tofino/lpf.p4
  p4include/tofino/meter.p4
  p4include/tofino/stratum.p4
  p4include/tofino/p4_14_prim.p4
  p4include/tofino/stateful_alu.p4
  p4include/tofino/wred.p4
  p4include/tofino2_specs.p4
  p4include/tofino2_base.p4
  p4include/tofino2_arch.p4
  p4include/t2na.p4
  p4include/psa.p4
  p4include/bmv2/psa.p4
  p4include/dpdk/psa.p4
  )



set (p4_14include_HEADERS
  p4_14include/tofino/constants.p4
  p4_14include/tofino/intrinsic_metadata.p4
  p4_14include/tofino/lpf_blackbox.p4
  p4_14include/tofino/meter_blackbox.p4
  p4_14include/tofino/pktgen_headers.p4
  p4_14include/tofino/primitives.p4
  p4_14include/tofino/stateful_alu_blackbox.p4
  p4_14include/tofino/wred_blackbox.p4
  )

# hack to support jbay clone metadata in p4-14.
# used by mirror_test.p4 which should be rewritten to p4-16.
set (hack_p4_14include_HEADERS
  p4_14include/tofino2/intrinsic_metadata.p4
  )


set (BF_P4C_SOURCES
  ${BF_P4C_MIDEND_SRCS}
  ${BF_P4C_BACKEND_COMMON_SRCS}
  ${BF_P4C_BACKEND_CONTROL_PLANE_SRCS}
  ${BF_P4C_BACKEND_LIB_SRCS}
  ${BF_P4C_BACKEND_LOGGING_SRCS}
  ${BF_P4C_BACKEND_MAU_SRCS}
  ${BF_P4C_BACKEND_MAU_TOFINO_SRCS}
  ${BF_P4C_BACKEND_PARDE_SRCS}
  ${BF_P4C_BACKEND_PHV_SRCS}
  ${BF_P4C_BACKEND_ARCH_SRCS}
  ${BF_P4C_BACKEND_MIDEND_SRCS}
  ${BF_P4C_BACKEND_MAIN_SRCS}
  )

set (P4C_BAREFOOT_SRCS
  frontend.cpp
  ${BF_P4C_BACKEND_CONV_SRCS}
  ${BF_P4C_GRAPHS_SRCS}
  )
add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${P4C_BAREFOOT_SRCS}")

if(BUILD_STATIC_BFP4C_LIBS)
    add_library(tofinobackend STATIC ${BF_P4C_SOURCES})
else()
    add_library(tofinobackend OBJECT ${BF_P4C_SOURCES})
endif()
target_link_libraries(tofinobackend PRIVATE absl::prefetch)
add_dependencies(tofinobackend ir-generated frontend genLogging bfn_p4runtime)
target_include_directories(tofinobackend PRIVATE "${BFN_P4C_SOURCE_DIR}/third_party/spdlog/include")
foreach(dir ${Protobuf_INCLUDE_DIRS})
    target_include_directories(tofinobackend SYSTEM BEFORE PUBLIC ${dir})
endforeach()

if(BUILD_STATIC_BFP4C_LIBS)
    add_library(bfp4c STATIC ${P4C_BAREFOOT_SRCS})
else()
    # For objlib there is no way (as far as I know) to encompass smaller libraries (namely adding
    # $<TARGET_OBJECTS:tofinobackend> $<TARGET_OBJECTS:bfn_p4runtime> to sources does not work)
    add_library(bfp4c OBJECT ${P4C_BAREFOOT_SRCS})
endif()
target_link_libraries (bfp4c tofinobackend bfn_p4runtime
  ${P4C_LIBRARIES} ${P4C_LIB_DEPS} ${CMAKE_THREAD_LIBS_INIT})

# XXX These compile definitions and include directories should be inherited
# XXX from bfp4c's dependencies, but these CMake files aren't very modular.
target_compile_definitions(bfp4c
  INTERFACE "-DBOOST_NO_ARGUMENT_DEPENDENT_LOOKUP"
  INTERFACE "-DMAX_LOGGING_LEVEL=${MAX_LOGGING_LEVEL}"
  INTERFACE "-DEVENT_LOG_SCHEMA_VERSION=\"${EVENT_LOG_SCHEMA_VERSION}\""
  INTERFACE "-DPHV_SCHEMA_VERSION=\"${PHV_SCHEMA_VERSION}\""
  INTERFACE "-DCONFIG_PREFIX=\"${CMAKE_INSTALL_PREFIX}\""
  INTERFACE "-DCONFIG_PKGDATADIR=\"${CMAKE_INSTALL_PREFIX}/${P4C_ARTIFACTS_OUTPUT_DIRECTORY}\""
)
target_compile_definitions(bfp4c
  INTERFACE "-DHAVE_JBAY=1"
)
if (ENABLE_BAREFOOT_INTERNAL)
  target_compile_definitions(bfp4c
    INTERFACE "-DBAREFOOT_INTERNAL=1"
  )
endif()
target_include_directories(bfp4c
  INTERFACE ${LIBDYNHASH_INCLUDE_DIR}
  INTERFACE ${PROTOBUF_INCLUDE_DIRS}
  INTERFACE ${P4C_SOURCE_DIR}/frontends
  INTERFACE ${P4C_SOURCE_DIR}/backends
  INTERFACE ${P4C_SOURCE_DIR}/extensions
  INTERFACE ${P4C_SOURCE_DIR}
  INTERFACE ${P4C_BINARY_DIR}
  INTERFACE ${BFN_P4C_SOURCE_DIR}
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(bfp4c
  SYSTEM INTERFACE ${P4C_BINARY_DIR}/control-plane
)
target_include_directories(bfp4c
  INTERFACE logging
  INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/logging
  INTERFACE ${CMAKE_SOURCE_DIR}/third_party/spdlog/include
)

if(BUILD_STATIC_BFP4C_LIBS)
    add_executable(p4c-barefoot p4c-barefoot.cpp)
    # --whole-archive prevents the linker from removing unused symbols
    #
    # This prevents an issue with non-unified builds and P4 primitives being
    # undefined. The file "bf-p4c/arch/fromv1.0/primitives.cpp" defines a number
    # of primitives (e.g., bypass_egress). This is done by subclassing
    # PrimitiveConverter and creating a singleton instance, which registers
    # itself with the PrimitiveConverter base class. These singleton instances are
    # omitted during linking of the non-unified builds, and so the primitives are
    # not registered.
    #
    # FIXME: Understand why the symbols are present in unified builds but not
    # non-unified builds and remove the --whole-archive flag if possible.
    target_link_libraries(p4c-barefoot
            -Wl,--whole-archive
            bfp4c
            -Wl,--no-whole-archive)
else()
    # No transitive library dependencies for objlibs :-/, bring tofinobackend & bfn_p4runtime
    # here explictly as well as their dependencies
    add_executable(p4c-barefoot p4c-barefoot.cpp $<TARGET_OBJECTS:bfp4c>
                                $<TARGET_OBJECTS:tofinobackend> $<TARGET_OBJECTS:bfn_p4runtime>)
    target_link_libraries(p4c-barefoot ${P4C_LIBRARIES} ${P4C_LIB_DEPS} ${CMAKE_THREAD_LIBS_INIT})
endif()



install (TARGETS p4c-barefoot
  RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})
if (NOT DEFINED EXCLUDE_HEADERS)
  install (DIRECTORY ${P4C_BINARY_DIR}/p4include
    DESTINATION ${P4C_ARTIFACTS_OUTPUT_DIRECTORY})
  install (DIRECTORY ${P4C_BINARY_DIR}/p4_14include
    DESTINATION ${P4C_ARTIFACTS_OUTPUT_DIRECTORY})
else()
  install (DIRECTORY ${P4C_BINARY_DIR}/p4include
    DESTINATION ${P4C_ARTIFACTS_OUTPUT_DIRECTORY}
    REGEX ${EXCLUDE_HEADERS} EXCLUDE)
  install (DIRECTORY ${P4C_BINARY_DIR}/p4_14include
    DESTINATION ${P4C_ARTIFACTS_OUTPUT_DIRECTORY}
    REGEX ${EXCLUDE_HEADERS} EXCLUDE)
endif (NOT DEFINED EXCLUDE_HEADERS)
install (PROGRAMS ${BFN_P4C_SOURCE_DIR}/scripts/p4_14-to-p4_16
  DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})

# 9.1.1 hack, we still need to release primitives.json to keep p4-graphs tool
# usable. To be removed when we stop shipping p4-hlir in release
install (FILES ${BFN_P4C_SOURCE_DIR}/scripts/primitives.json
    DESTINATION ${P4C_ARTIFACTS_OUTPUT_DIRECTORY}/p4_14include/tofino/)

set (MANIFEST_CONFIG ${BFN_P4C_SOURCE_DIR}/scripts/p4c-manifest-config
  CACHE FILEPATH "Path to p4c-manifest-config")
set (GEN_DRIVER_CONF ${BFN_P4C_SOURCE_DIR}/scripts/p4c-gen-conf
  CACHE FILEPATH "Path to p4c-gen-conf")
set (GEN_BFRT_CONF ${BFN_P4C_SOURCE_DIR}/scripts/p4c-gen-bfrt-conf
  CACHE FILEPATH "Path to p4c-gen-bfrt-conf")
set (GEN_BFRT_SCHEMA ${CMAKE_CURRENT_SOURCE_DIR}/../compiler_interfaces/schemas/bfrt_schema.py
  CACHE FILEPATH "Path to bfrt-schema")

install (PROGRAMS ${GEN_BFRT_SCHEMA} ${GEN_BFRT_CONF} ${MANIFEST_CONFIG} ${GEN_DRIVER_CONF}
  DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})

set (P4C_DRIVER_MACROS ${P4C_BINARY_DIR}/p4c_src/bfn_version.py)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/driver/bfn_version.in
  ${P4C_DRIVER_MACROS} @ONLY)

# Tofino targets configuration
set (TOFINO_TARGET_CFG_FILE ${P4C_BINARY_DIR}/p4c_src/p4c.tofino.cfg)
set_source_files_properties (${TOFINO_TARGET_CFG_FILE} PROPERTIES GENERATED TRUE)
if (ENABLE_BAREFOOT_INTERNAL)
  # Keep PSA support for internal
  add_custom_command(OUTPUT ${TOFINO_TARGET_CFG_FILE}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/driver/p4c.tofino.cfg ${TOFINO_TARGET_CFG_FILE}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/driver/p4c.tofino.cfg
    COMMENT "Copying Tofino config")
else()
  # Remove support for production distributions
  add_custom_command(OUTPUT ${TOFINO_TARGET_CFG_FILE}
    COMMAND sed -e '/psa_target/d' ${CMAKE_CURRENT_SOURCE_DIR}/driver/p4c.tofino.cfg > ${TOFINO_TARGET_CFG_FILE}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/driver/p4c.tofino.cfg
    COMMENT "Removing PSA support for Tofino")
endif()
add_custom_target(remove_psa_for_tofino ALL DEPENDS ${TOFINO_TARGET_CFG_FILE})
add_dependencies(p4c_driver remove_psa_for_tofino)
set (DRIVER_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/driver/barefoot.py
  ${TOFINO_TARGET_CFG_FILE}
  )

set (JBAY_TARGET_CFG_FILE ${P4C_BINARY_DIR}/p4c_src/p4c.tofino2.cfg)
set_source_files_properties (${JBAY_TARGET_CFG_FILE} PROPERTIES GENERATED TRUE)
# hack used to compile p4-14 program to JBay, we should not develop anything
# significant with P4-14, it is only used to support existing programs.
add_custom_target(jbay_hack
  COMMAND ${CMAKE_COMMAND} -E make_directory ${P4C_BINARY_DIR}/p4_14include/tofino2
  COMMAND for h in ${hack_p4_14include_HEADERS} \; do
    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/\$$h ${P4C_BINARY_DIR}/\$$h \;
  done
  )
add_dependencies(p4c_driver jbay_hack)
if (NOT ENABLE_BAREFOOT_INTERNAL)
  # Remove the P4-14 support for JBay
  add_custom_command(OUTPUT ${JBAY_TARGET_CFG_FILE}
    COMMAND sed -e '/v1model/d' -e '/tofino2h/d' -e '/psa_target/d' ${CMAKE_CURRENT_SOURCE_DIR}/driver/p4c.tofino2.cfg > ${JBAY_TARGET_CFG_FILE}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/driver/p4c.tofino2.cfg
    COMMENT "Removing P4-14, Tofino2h, and PSA support for Tofino2"
    )
else()
  add_custom_command(OUTPUT ${JBAY_TARGET_CFG_FILE}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/driver/p4c.tofino2.cfg ${JBAY_TARGET_CFG_FILE}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/driver/p4c.tofino2.cfg
    COMMENT "Copying Tofino2 config"
    )
endif()
add_custom_target(remove_p4_14_for_jbay ALL DEPENDS ${JBAY_TARGET_CFG_FILE})
add_dependencies(p4c_driver remove_p4_14_for_jbay)



install (FILES ${DRIVER_FILES} ${JBAY_TARGET_CFG_FILE} ${P4C_DRIVER_MACROS}
  DESTINATION ${P4C_ARTIFACTS_OUTPUT_DIRECTORY}/p4c_src)



# install the tools that generate logging
set (LOGGING_FILES
  ${INTERFACES_DIR}/p4c-build-logs
  ${INTERFACES_DIR}/tools/create_mau_json.py
  ${INTERFACES_DIR}/tools/create_phv_json.py
  ${INTERFACES_DIR}/tools/create_mau_characterize.py
  ${INTERFACES_DIR}/tools/create_mau_resources.py
  ${INTERFACES_DIR}/tools/create_pa_characterize.py
  ${INTERFACES_DIR}/tools/create_pa_results.py
  ${INTERFACES_DIR}/tools/utils.py
  ${INTERFACES_DIR}/tools/__init__.py
  )
set (SCHEMA_FILES
  ${INTERFACES_DIR}/schemas/bfrt_schema.py
  ${INTERFACES_DIR}/schemas/context_schema.py
  ${INTERFACES_DIR}/schemas/event_log_schema.py
  ${INTERFACES_DIR}/schemas/jgf_schema.py
  ${INTERFACES_DIR}/schemas/manifest_schema.py
  ${INTERFACES_DIR}/schemas/mau_schema.py
  ${INTERFACES_DIR}/schemas/metrics_schema.py
  ${INTERFACES_DIR}/schemas/phv_schema.py
  ${INTERFACES_DIR}/schemas/power_schema.py
  ${INTERFACES_DIR}/schemas/resources_schema.py
  ${INTERFACES_DIR}/schemas/schema_enum_values.py
  ${INTERFACES_DIR}/schemas/schema_keys.py
  ${INTERFACES_DIR}/schemas/table_graph_schema.py
  ${INTERFACES_DIR}/schemas/__init__.py
  )
if (EXISTS ${COMPILER_SCHEMA_TARGETS})
  list(APPEND SCHEMA_FILES ${COMPILER_SCHEMA_TARGETS})
endif()

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dist/p4c-build-logs
  POST_BUILD
  COMMAND PYTHONPATH=${INTERFACES_DIR}:$ENV{PYTHONPATH} pyinstaller --workpath=${CMAKE_CURRENT_BINARY_DIR}/build --distpath=${CMAKE_CURRENT_BINARY_DIR}/dist ${INTERFACES_DIR}/p4c-build-logs.spec
  DEPENDS ${LOGGING_FILES} ${SCHEMA_FILES} ${INTERFACES_DIR}/p4c-build-logs.spec
  WORKING_DIRECTORY ${INTERFACES_DIR}
  COMMENT "Package logging tools")

add_custom_target(p4c-build-logs
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dist/p4c-build-logs)
add_dependencies(p4c_driver p4c-build-logs)

install (PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/dist/p4c-build-logs
  DESTINATION bin)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/p4c_src/schema_versions
  COMMAND ${BFN_P4C_SOURCE_DIR}/scripts/extract_schema_versions.sh ${CMAKE_BINARY_DIR}/p4c_src/schema_versions
  DEPENDS ${SCHEMA_FILES}
  COMMENT "Extract all used schema versions")

add_custom_target(schema-versions
  DEPENDS ${CMAKE_BINARY_DIR}/p4c_src/schema_versions)
add_dependencies(p4c_driver schema-versions)

install (FILES ${CMAKE_BINARY_DIR}/p4c_src/schema_versions
  DESTINATION share/p4c/p4c_src)

set(SED_RELEASE_FILTERS "-e ''")

# hack to get around the fact that the test scripts expect the backend
# binary to be in the top level directory. This should go away when we
# fix the scripts.
add_custom_target(linkp4cbarefoot
  COMMAND ${CMAKE_COMMAND} -E create_symlink `realpath --relative-to=${P4C_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/p4c-barefoot` ${P4C_BINARY_DIR}/p4c-barefoot
  COMMAND for h in ${p4include_HEADERS} ${p4_14include_HEADERS} \; do
    ${CMAKE_COMMAND} -E make_directory `dirname ${P4C_BINARY_DIR}/\$$h` \;
    sed ${SED_RELEASE_FILTERS} ${CMAKE_CURRENT_SOURCE_DIR}/\$$h >${P4C_BINARY_DIR}/\$$h \;
  done
  COMMAND ${CMAKE_COMMAND} -E make_directory ${P4C_BINARY_DIR}/p4c_src
  COMMAND ${CMAKE_COMMAND} -E copy ${DRIVER_FILES} ${P4C_BINARY_DIR}/p4c_src
  COMMAND ${CMAKE_COMMAND} -E copy ${P4C_DRIVER_MACROS} ${P4C_BINARY_DIR}/p4c_src
  COMMAND ${CMAKE_COMMAND} -E create_symlink `realpath --relative-to=${P4C_BINARY_DIR} ${MANIFEST_CONFIG}` ${P4C_BINARY_DIR}/p4c-manifest-config
  COMMAND ${CMAKE_COMMAND} -E create_symlink `realpath --relative-to=${P4C_BINARY_DIR} ${GEN_BFRT_CONF}` ${P4C_BINARY_DIR}/p4c-gen-bfrt-conf
  COMMAND ${CMAKE_COMMAND} -E create_symlink `realpath --relative-to=${P4C_BINARY_DIR} ${GEN_DRIVER_CONF}` ${P4C_BINARY_DIR}/p4c-gen-conf
  COMMAND ${CMAKE_COMMAND} -E create_symlink `realpath --relative-to=${P4C_BINARY_DIR} ${GEN_BFRT_SCHEMA}` ${P4C_BINARY_DIR}/bfrt_schema.py
  COMMAND ${CMAKE_COMMAND} -E create_symlink `realpath --relative-to=${CMAKE_CURRENT_BINARY_DIR} ${P4C_BINARY_DIR}/p4include` ${CMAKE_CURRENT_BINARY_DIR}/p4include
  COMMAND ${CMAKE_COMMAND} -E create_symlink `realpath --relative-to=${CMAKE_CURRENT_BINARY_DIR} ${P4C_BINARY_DIR}/p4_14include` ${CMAKE_CURRENT_BINARY_DIR}/p4_14include
  )
add_dependencies(p4c_driver linkp4cbarefoot)
add_dependencies(linkp4cbarefoot remove_psa_for_tofino)

# Set default target and architecture to tofino-tna
add_custom_target(set_p4c_default_target ALL
  # should cythonify bf-p4c
  COMMAND sed -i'' -e 's/bmv2/tofino/g' ${P4C_BINARY_DIR}/p4c_src/main.py
  COMMAND sed -i'' -e 's/v1model/default/g' ${P4C_BINARY_DIR}/p4c_src/main.py
  COMMENT "Set default target and architecture to tofino-default"
  WORKING_DIRECTORY ${P4C_BINARY_DIR}
  DEPENDS p4c_driver ${P4C_BINARY_DIR}/p4c_src/main.py
)

# On internal builds, symlink bf-p4c to p4c.
if (ENABLE_BAREFOOT_INTERNAL)
  add_custom_command(OUTPUT ${P4C_BINARY_DIR}/p4c
    COMMAND ${CMAKE_COMMAND} -E create_symlink bf-p4c ${P4C_BINARY_DIR}/p4c
    DEPENDS p4c_driver
  )
  add_custom_target(bf-p4c_symlink ALL DEPENDS ${P4C_BINARY_DIR}/p4c)
  install (PROGRAMS ${P4C_BINARY_DIR}/p4c DESTINATION bin)
endif()

install (FILES ${P4C_BINARY_DIR}/p4c_src/main.py
  DESTINATION ${P4C_ARTIFACTS_OUTPUT_DIRECTORY}/p4c_src)

################ Proto
set (BFN_P4RUNTIME_DIR ${CMAKE_CURRENT_SOURCE_DIR}/control-plane/proto)
set (BFN_P4RUNTIME_INFO_PROTO ${BFN_P4RUNTIME_DIR}/barefoot/p4info.proto)
set (BFN_P4RUNTIME_INFO_GEN_SRCS ${P4C_BINARY_DIR}/control-plane/barefoot/p4info.pb.cc)
set (BFN_P4RUNTIME_INFO_GEN_HDRS ${P4C_BINARY_DIR}/control-plane/barefoot/p4info.pb.h)
# The generated code for protobuf has an excessive number of warnings
set_source_files_properties(${BFN_P4RUNTIME_INFO_GEN_SRCS} PROPERTIES
  GENERATED TRUE
  COMPILE_OPTIONS "-Wno-unused-parameter;-Wno-array-bounds")

add_custom_target (barefootdir
  ${CMAKE_COMMAND} -E make_directory ${P4C_BINARY_DIR}/control-plane/barefoot)
# Generate source code from .proto using protoc. The output is
# placed in the build directory inside `control-plane` directory
add_custom_command(
  OUTPUT ${BFN_P4RUNTIME_INFO_GEN_SRCS} ${BFN_P4RUNTIME_INFO_GEN_HDRS}
  COMMAND ${Protobuf_PROTOC_EXECUTABLE}
          --proto_path ${BFN_P4RUNTIME_DIR}
          --proto_path ${P4RUNTIME_STD_DIR}
          --cpp_out ${P4C_BINARY_DIR}/control-plane
          --python_out ${P4C_BINARY_DIR}/control-plane
          ${BFN_P4RUNTIME_INFO_PROTO}
  DEPENDS ${BFN_P4RUNTIME_INFO_PROTO}
  COMMENT "Generating protobuf files for p4info."
  )
if(BUILD_STATIC_BFP4C_LIBS)
    add_library(bfn_p4runtime STATIC ${BFN_P4RUNTIME_INFO_GEN_SRCS})
else()
    add_library(bfn_p4runtime OBJECT ${BFN_P4RUNTIME_INFO_GEN_SRCS})
endif()
target_link_libraries(bfn_p4runtime PRIVATE absl::prefetch)
set_source_files_properties(${BFN_P4RUNTIME_INFO_GEN_SRCS} PROPERTIES GENERATED TRUE)
add_dependencies(bfn_p4runtime barefootdir controlplane)
foreach(dir ${Protobuf_INCLUDE_DIRS})
    target_include_directories(bfn_p4runtime SYSTEM BEFORE PUBLIC ${dir})
endforeach()

################ Testing

if (ENABLE_GTESTS)
  # # Tofino-specific GTests.

  # bf_gtest_support library
  #
  # The bf_gtest_helpers.h encapsulates stringified tofino p4 header files.
  set (P4HEADERS_H        "${P4C_BINARY_DIR}/p4headers.h")
  set (P4HEADERS_CPP      "${P4C_BINARY_DIR}/p4headers.cpp")
  set (BFN_P4INCLUDE_DIR  "${BFN_P4C_SOURCE_DIR}/bf-p4c/p4include")
  set (I_P4INCLUDE        -I "${BFN_P4INCLUDE_DIR}" -I "${P4C_SOURCE_DIR}/p4include")
  file(READ   "${BFN_P4INCLUDE_DIR}/tofino1_specs.p4"     TOFINO1_SPECS)
  file(READ   "${BFN_P4INCLUDE_DIR}/tofino1_arch.p4"      TOFINO1_ARCH)
  file(GENERATE OUTPUT "${P4C_BINARY_DIR}/p4headers_tofino1.p4"
      CONTENT "${TOFINO1_SPECS}${TOFINO1_ARCH}")
  file(READ   "${BFN_P4INCLUDE_DIR}/tofino2_specs.p4"     TOFINO2_SPECS)
  file(READ   "${BFN_P4INCLUDE_DIR}/tofino2_arch.p4"      TOFINO2_ARCH)
  file(GENERATE OUTPUT "${P4C_BINARY_DIR}/p4headers_tofino2.p4"
      CONTENT "${TOFINO2_SPECS}${TOFINO2_ARCH}")
  set (HEADERS_H_EXTRA_COMMANDS "")
  set (HEADERS_CPP_EXTRA_COMMANDS "")
  set (HEADERS_CPP_EXTRA_DEPENDS "")
  add_custom_command (OUTPUT "${P4HEADERS_H}"
      COMMAND ${CMAKE_COMMAND} -E echo "// auto generated file, do not edit" >  "${P4HEADERS_H}"
      COMMAND ${CMAKE_COMMAND} -E echo "#ifndef P4HEADERS_H_"                >> "${P4HEADERS_H}"
      COMMAND ${CMAKE_COMMAND} -E echo "#define P4HEADERS_H_"                >> "${P4HEADERS_H}"
      COMMAND ${CMAKE_COMMAND} -E echo "#include <string>"                   >> "${P4HEADERS_H}"
      COMMAND ${CMAKE_COMMAND} -E echo "namespace P4 {"                      >> "${P4HEADERS_H}"
      COMMAND ${CMAKE_COMMAND} -E echo "namespace Test {"                    >> "${P4HEADERS_H}"
      COMMAND ${CMAKE_COMMAND} -E echo "struct P4Headers {"                  >> "${P4HEADERS_H}"
      COMMAND ${CMAKE_COMMAND} -E echo "    std::string tofino1arch_p4;"     >> "${P4HEADERS_H}"
      COMMAND ${CMAKE_COMMAND} -E echo "    std::string tofino2arch_p4;"     >> "${P4HEADERS_H}"
      ${HEADERS_H_EXTRA_COMMANDS} # Expands to some subset of:
      COMMAND ${CMAKE_COMMAND} -E echo "    std::string v1model_2018_p4;"    >> "${P4HEADERS_H}"
      COMMAND ${CMAKE_COMMAND} -E echo "    std::string v1model_2020_p4;"    >> "${P4HEADERS_H}"
      COMMAND ${CMAKE_COMMAND} -E echo "};"                                  >> "${P4HEADERS_H}"
      COMMAND ${CMAKE_COMMAND} -E echo "const P4Headers& p4headers();"       >> "${P4HEADERS_H}"
      COMMAND ${CMAKE_COMMAND} -E echo "}  // namespace Test"                >> "${P4HEADERS_H}"
      COMMAND ${CMAKE_COMMAND} -E echo "}  // namespace P4"                  >> "${P4HEADERS_H}"
      COMMAND ${CMAKE_COMMAND} -E echo "#endif"                              >> "${P4HEADERS_H}"
      VERBATIM)
  add_custom_target(p4headers_h DEPENDS "${P4HEADERS_H}")
  add_custom_command (OUTPUT "${P4HEADERS_CPP}"
      COMMAND ${CMAKE_COMMAND} -E echo "// auto generated file, do not edit"                      >  "${P4HEADERS_CPP}"
      COMMAND ${CMAKE_COMMAND} -E echo "#include \"${P4HEADERS_H}\""                              >> "${P4HEADERS_CPP}"
      COMMAND ${CMAKE_COMMAND} -E echo "namespace P4 {"                                           >> "${P4HEADERS_CPP}"
      COMMAND ${CMAKE_COMMAND} -E echo "namespace Test {"                                         >> "${P4HEADERS_CPP}"
      COMMAND ${CMAKE_COMMAND} -E echo "namespace {"                                              >> "${P4HEADERS_CPP}"
      COMMAND ${CMAKE_COMMAND} -E echo "const P4Headers p4h = {"                                  >> "${P4HEADERS_CPP}"
      COMMAND ${CMAKE_COMMAND} -E echo "    std::string{R\"("                                     >> "${P4HEADERS_CPP}"
      COMMAND ${CMAKE_C_COMPILER} -E -x c "${P4C_BINARY_DIR}/p4headers_tofino1.p4" ${I_P4INCLUDE} >> "${P4HEADERS_CPP}"
      COMMAND ${CMAKE_COMMAND} -E echo "    )\"}, std::string{R\"("                               >> "${P4HEADERS_CPP}"
      COMMAND ${CMAKE_C_COMPILER} -E -x c "${P4C_BINARY_DIR}/p4headers_tofino2.p4" ${I_P4INCLUDE} >> "${P4HEADERS_CPP}"
      COMMAND ${CMAKE_COMMAND} -E echo "    )\"}, std::string{R\"("                               >> "${P4HEADERS_CPP}"
      ${HEADERS_CPP_EXTRA_COMMANDS} # Expands to some subset of:
      COMMAND ${CMAKE_C_COMPILER} -E -x c -DV1MODEL_VERSION=20180101
                                       "${P4C_SOURCE_DIR}/p4include/v1model.p4" ${I_P4INCLUDE}    >> "${P4HEADERS_CPP}"
      COMMAND ${CMAKE_COMMAND} -E echo "    )\"}, std::string{R\"("                               >> "${P4HEADERS_CPP}"
      COMMAND ${CMAKE_C_COMPILER} -E -x c -DV1MODEL_VERSION=20200408
                                       "${P4C_SOURCE_DIR}/p4include/v1model.p4" ${I_P4INCLUDE}    >> "${P4HEADERS_CPP}"
      COMMAND ${CMAKE_COMMAND} -E echo "    )\"}};"                                               >> "${P4HEADERS_CPP}"
      COMMAND ${CMAKE_COMMAND} -E echo "}  // namespace"                                          >> "${P4HEADERS_CPP}"
      COMMAND ${CMAKE_COMMAND} -E echo "const P4Headers& p4headers() {return p4h;}"               >> "${P4HEADERS_CPP}"
      COMMAND ${CMAKE_COMMAND} -E echo "}  // namespace Test"                                     >> "${P4HEADERS_CPP}"
      COMMAND ${CMAKE_COMMAND} -E echo "}  // namespace P4"                                       >> "${P4HEADERS_CPP}"
      DEPENDS "${P4C_BINARY_DIR}/p4headers_tofino1.p4"
              "${P4C_BINARY_DIR}/p4headers_tofino2.p4"
              ${HEADERS_CPP_EXTRA_DEPENDS}
              "${P4C_SOURCE_DIR}/p4include/v1model.p4"
      COMMENT "Stringify P4 headers -> ${P4HEADERS_CPP}"
      VERBATIM)
  add_custom_target(p4headers_cpp DEPENDS "${P4HEADERS_CPP}")
  add_library (bf_gtest_support "${P4HEADERS_CPP}"
                                "${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/bf_gtest_helpers.cpp"
                                "${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/tofino_gtest_utils.cpp")
                                # test_bf_gtest_helpers should be in its own testlib - & requies 'whole_archive' hack!
                                # Leave it in GTEST_BF_P4C_SOURCES for now.
  target_link_libraries (bf_gtest_support PRIVATE absl::flat_hash_map PRIVATE absl::flat_hash_set)
  target_include_directories (bf_gtest_support PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/test/gtest}" PRIVATE "${P4C_BINARY_DIR}")
  add_dependencies (bf_gtest_support p4headers_h p4headers_cpp ir-generated gtest frontend-parser-gen)  # Indirectly we #include "ir/gen-tree-macro.h" and "p4parser.hpp"

  set (GTEST_BF_P4C_SOURCES
    # Currently all gtests need to be built directly into the gtestp4c binary.
    # TODO To use libraries, the whole_archive hack is needed.
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/test_bf_gtest_helpers.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/action_analysis.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/action_format_helper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/action_mutex.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/action_phv.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/add_always_run.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/annotate_with_in_hash.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/attached_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/bfrt_pvs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/bitrange.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/build_super_cluster.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/checksum.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/clot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/constrained_field_map_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/container_action.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/container_size_extractor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/copy_header.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/critical_path_clusters.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/custom_header_stack_name.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/dominator_tree.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/dynamic_dep_metrics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/elim_cast.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/equiv_align_extractor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/error_reporter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/event_logger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/field_alignment.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/field_defuse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/field_packing.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/fold_constant_hashes.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/gateway_control_flow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/normalize_hash_list.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/hidden_merged.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/input_xbar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/ir_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/initialize_mirror_io_select.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/jbay_next_table.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/mask_exact_match_hash_bits.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/mau_control_flow_visitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/mau_group_extractor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/mau_indirect_externs_single_action.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/meta_live_range.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/mocha_dark.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/multipipe_bf_runtime.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/multiple_apply.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/no_co_pack.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/pa_atomic.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/pa_container_size.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/pa_container_type.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/pa_byte_pack.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/pa_mutually_exclusive_pragma.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/pa_no_overlay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/parser_container_valid.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/parser_constant_extract.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/parser_critical_path.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/parser_elim_lowered_zero_extracts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/parser_enforce_depth_req.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/parser_extract_overlay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/parser_extract_reorder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/parser_state_merge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/pa_solitary.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/path_linearizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/payload_gateway.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/phv/action_source_tracker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/phv/fieldslice_live_range.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/phv/greedy_tx_score.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/phv/solver/action_constraint_solver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/phv/solver/symbolic_bitvec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/phv_crush.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/phv_field.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/phv_jbay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/phv_tofino.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/post_midend_constant_folding.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/power_schema_dot_prefix.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/pragmas_backward_quick_phv_bandwidth.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/pragma_no_gateway_conversion.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/pragma_eg_intr_md_opt.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/pragma_sep_gat.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/pragma_stage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/register_actions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/register_read_write.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/scc_toposort.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/simplify_key_elim_casts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/slice_comparison.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/slice.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/small_set.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/static_entries_const_prop.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/table_dependency_graph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/table_flow_graph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/table_mutex.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/tofino_write_context.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/tphv_slice.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/type_categories.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/union_find.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/gtest/v1model_translate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test/utils/super_cluster_builder.cpp
    ${BF_P4C_GRAPHS_SRCS}
    )
  if (NOT BUILD_STATIC_BFP4C_LIBS)
      # objlibs cannot be linked, need to add objects here explicitly
      set (GTEST_BF_P4C_SOURCES ${GTEST_BF_P4C_SOURCES}
                                $<TARGET_OBJECTS:tofinobackend> $<TARGET_OBJECTS:bfn_p4runtime>)
  endif()

  # Do not use a unity build for bf-p4c gtest (for now).
  # set_source_files_properties (${GTEST_BF_P4C_SOURCES} TARGET_DIRECTORY gtestp4c PROPERTIES SKIP_UNITY_BUILD_INCLUSION TRUE)

  set (GTEST_SOURCES ${GTEST_SOURCES} ${GTEST_BF_P4C_SOURCES} PARENT_SCOPE)
  set (GTEST_LDADD ${GTEST_LDADD}
      "bf_gtest_support;tofinobackend;${LIBDYNHASH_LIBRARY};bfn_p4runtime" PARENT_SCOPE)

  add_test(NAME test_p4c_driver
    COMMAND ${BFN_P4C_SOURCE_DIR}/scripts/test_p4c_driver.py -j 4 --print-on-failure --compiler ${P4C_BINARY_DIR}/bf-p4c
    WORKING_DIRECTORY ${P4C_BINARY_DIR})
  set_tests_properties(test_p4c_driver PROPERTIES LABELS lint)

endif(ENABLE_GTESTS)


set (EVEN_MORE_FILES_TO_LINT
  p4c-barefoot.cpp
  )
add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "${EVEN_MORE_FILES_TO_LINT}")
