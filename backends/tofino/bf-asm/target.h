/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_TOFINO_BF_ASM_TARGET_H_
#define BACKENDS_TOFINO_BF_ASM_TARGET_H_

#include "asm-types.h"
#include "backends/tofino/bf-asm/config.h"
#include "bfas.h"
#include "lib/exceptions.h"
#include "map.h"

struct MemUnit;

/** FOR_ALL_TARGETS -- metamacro that expands a macro for each defined target
 *  FOR_ALL_REGISTER_SETS -- metamacro that expands for each distinct register set;
 *              basically a subset of targets with one per distinct register set
 *  FOR_ALL_TARGET_CLASSES -- metamacro that expands for each distinct target class
 *              a subset of the register sets
 */
// TODO: clang-format adds space in __VA_OPT__
#define FOR_ALL_TARGETS(M, ...)           \
    M(Tofino __VA_OPT__(,) __VA_ARGS__)   \
    M(JBay __VA_OPT__(,) __VA_ARGS__)     \
    M(Tofino2H __VA_OPT__(,) __VA_ARGS__) \
    M(Tofino2M __VA_OPT__(,) __VA_ARGS__) \
    M(Tofino2U __VA_OPT__(,) __VA_ARGS__) \
    M(Tofino2A0 __VA_OPT__(,) __VA_ARGS__)
#define FOR_ALL_REGISTER_SETS(M, ...)   \
    M(Tofino __VA_OPT__(,) __VA_ARGS__) \
    M(JBay __VA_OPT__(,) __VA_ARGS__)
#define FOR_ALL_TARGET_CLASSES(M, ...) M(Tofino __VA_OPT__(,) __VA_ARGS__)

// alias FOR_ALL -> FOR_EACH so the the group name does need to be plural
#define FOR_EACH_TARGET FOR_ALL_TARGETS
#define FOR_EACH_REGISTER_SET FOR_ALL_REGISTER_SETS
#define FOR_EACH_TARGET_CLASS FOR_ALL_TARGET_CLASSES

#define TARGETS_IN_CLASS_Tofino(M, ...)   \
    M(Tofino __VA_OPT__(,) __VA_ARGS__)   \
    M(JBay __VA_OPT__(,) __VA_ARGS__)     \
    M(Tofino2H __VA_OPT__(,) __VA_ARGS__) \
    M(Tofino2M __VA_OPT__(,) __VA_ARGS__) \
    M(Tofino2U __VA_OPT__(,) __VA_ARGS__) \
    M(Tofino2A0 __VA_OPT__(,) __VA_ARGS__)
#define REGSETS_IN_CLASS_Tofino(M, ...) \
    M(Tofino __VA_OPT__(,) __VA_ARGS__) \
    M(JBay __VA_OPT__(,) __VA_ARGS__)

#define TARGETS_USING_REGS_JBay(M, ...)   \
    M(JBay __VA_OPT__(,) __VA_ARGS__)     \
    M(Tofino2H __VA_OPT__(,) __VA_ARGS__) \
    M(Tofino2M __VA_OPT__(,) __VA_ARGS__) \
    M(Tofino2U __VA_OPT__(,) __VA_ARGS__) \
    M(Tofino2A0 __VA_OPT__(,) __VA_ARGS__)
#define TARGETS_USING_REGS_Tofino(M, ...) M(Tofino __VA_OPT__(,) __VA_ARGS__)

#define TARGETS_IN_CLASS(CL, ...) TARGETS_IN_CLASS_##CL(__VA_ARGS__)
#define TARGETS_USING_REGS(CL, ...) TARGETS_USING_REGS_##CL(__VA_ARGS__)
#define REGSETS_IN_CLASS(CL, ...) REGSETS_IN_CLASS_##CL(__VA_ARGS__)

#define INSTANTIATE_TARGET_TEMPLATE(TARGET, FUNC, ...) template FUNC(Target::TARGET::__VA_ARGS__);
#define DECLARE_TARGET_CLASS(TARGET, ...) class TARGET __VA_ARGS__;
#define FRIEND_TARGET_CLASS(TARGET, ...) friend class Target::TARGET __VA_ARGS__;

#define PER_TARGET_CONSTANTS(M)                         \
    M(const char *, name)                               \
    M(target_t, register_set)                           \
    M(int, ARAM_UNITS_PER_STAGE)                        \
    M(int, DEPARSER_CHECKSUM_UNITS)                     \
    M(int, DEPARSER_CONSTANTS)                          \
    M(int, DEPARSER_MAX_FD_ENTRIES)                     \
    M(int, DEPARSER_MAX_POV_BYTES)                      \
    M(int, DEPARSER_MAX_POV_PER_USE)                    \
    M(int, DP_UNITS_PER_STAGE)                          \
    M(int, DYNAMIC_CONFIG)                              \
    M(int, DYNAMIC_CONFIG_INPUT_BITS)                   \
    M(bool, EGRESS_SEPARATE)                            \
    M(int, END_OF_PIPE)                                 \
    M(int, EXACT_HASH_GROUPS)                           \
    M(int, EXACT_HASH_TABLES)                           \
    M(int, EXTEND_ALU_8_SLOTS)                          \
    M(int, EXTEND_ALU_16_SLOTS)                         \
    M(int, EXTEND_ALU_32_SLOTS)                         \
    M(bool, GATEWAY_INHIBIT_INDEX)                      \
    M(int, GATEWAY_MATCH_BITS)                          \
    M(bool, GATEWAY_NEEDS_SEARCH_BUS)                   \
    M(int, GATEWAY_PAYLOAD_GROUPS)                      \
    M(int, GATEWAY_ROWS)                                \
    M(bool, GATEWAY_SINGLE_XBAR_GROUP)                  \
    M(bool, HAS_MPR)                                    \
    M(int, INSTR_SRC2_BITS)                             \
    M(int, IMEM_COLORS)                                 \
    M(int, IXBAR_HASH_GROUPS)                           \
    M(int, IXBAR_HASH_INDEX_MAX)                        \
    M(int, IXBAR_HASH_INDEX_STRIDE)                     \
    M(int, LOCAL_TIND_UNITS)                            \
    M(int, LONG_BRANCH_TAGS)                            \
    M(int, MAX_IMMED_ACTION_DATA)                       \
    M(int, MAX_OVERHEAD_OFFSET)                         \
    M(int, MAX_OVERHEAD_OFFSET_NEXT)                    \
    M(int, MATCH_BYTE_16BIT_PAIRS)                      \
    M(int, MATCH_REQUIRES_PHYSID)                       \
    M(int, MAU_BASE_DELAY)                              \
    M(int, MAU_BASE_PREDICATION_DELAY)                  \
    M(int, MAU_ERROR_DELAY_ADJUST)                      \
    M(int, METER_ALU_GROUP_DATA_DELAY)                  \
    M(int, MINIMUM_INSTR_CONSTANT)                      \
    M(bool, NEXT_TABLE_EXEC_COMBINED)                   \
    M(int, NEXT_TABLE_SUCCESSOR_TABLE_DEPTH)            \
    M(int, NUM_MAU_STAGES_PRIVATE)                      \
    M(int, NUM_EGRESS_STAGES_PRIVATE)                   \
    M(int, NUM_PARSERS)                                 \
    M(int, NUM_PIPES)                                   \
    M(bool, OUTPUT_STAGE_EXTENSION_PRIVATE)             \
    M(int, PARSER_CHECKSUM_UNITS)                       \
    M(bool, PARSER_EXTRACT_BYTES)                       \
    M(int, PARSER_DEPTH_MAX_BYTES_INGRESS)              \
    M(int, PARSER_DEPTH_MAX_BYTES_EGRESS)               \
    M(int, PARSER_DEPTH_MAX_BYTES_MULTITHREADED_EGRESS) \
    M(int, PARSER_DEPTH_MIN_BYTES_INGRESS)              \
    M(int, PARSER_DEPTH_MIN_BYTES_EGRESS)               \
    M(int, PHASE0_FORMAT_WIDTH)                         \
    M(bool, REQUIRE_TCAM_ID)                            \
    M(int, SRAM_EGRESS_ROWS)                            \
    M(bool, SRAM_GLOBAL_ACCESS)                         \
    M(int, SRAM_HBUS_SECTIONS_PER_STAGE)                \
    M(int, SRAM_HBUSSES_PER_ROW)                        \
    M(int, SRAM_INGRESS_ROWS)                           \
    M(int, SRAM_LOGICAL_UNITS_PER_ROW)                  \
    M(int, SRAM_LAMBS_PER_STAGE)                        \
    M(int, SRAM_REMOVED_COLUMNS)                        \
    M(int, SRAM_STRIDE_COLUMN)                          \
    M(int, SRAM_STRIDE_ROW)                             \
    M(int, SRAM_STRIDE_STAGE)                           \
    M(int, SRAM_UNITS_PER_ROW)                          \
    M(int, STATEFUL_ALU_ADDR_WIDTH)                     \
    M(int, STATEFUL_ALU_CONST_MASK)                     \
    M(int, STATEFUL_ALU_CONST_MAX)                      \
    M(int, STATEFUL_ALU_CONST_MIN)                      \
    M(int, STATEFUL_ALU_CONST_WIDTH)                    \
    M(int, STATEFUL_CMP_ADDR_WIDTH)                     \
    M(int, STATEFUL_CMP_CONST_MASK)                     \
    M(int, STATEFUL_CMP_CONST_MAX)                      \
    M(int, STATEFUL_CMP_CONST_MIN)                      \
    M(int, STATEFUL_CMP_CONST_WIDTH)                    \
    M(int, STATEFUL_CMP_UNITS)                          \
    M(int, STATEFUL_OUTPUT_UNITS)                       \
    M(int, STATEFUL_PRED_MASK)                          \
    M(int, STATEFUL_REGFILE_CONST_WIDTH)                \
    M(int, STATEFUL_REGFILE_ROWS)                       \
    M(int, STATEFUL_TMATCH_UNITS)                       \
    M(bool, SUPPORT_ALWAYS_RUN)                         \
    M(bool, SUPPORT_CONCURRENT_STAGE_DEP)               \
    M(bool, SUPPORT_OVERFLOW_BUS)                       \
    M(bool, SUPPORT_SALU_FAST_CLEAR)                    \
    M(bool, SUPPORT_TRUE_EOP)                           \
    M(bool, SYNTH2PORT_NEED_MAPRAMS)                    \
    M(bool, TCAM_EXTRA_NIBBLE)                          \
    M(bool, TCAM_GLOBAL_ACCESS)                         \
    M(int, TCAM_MATCH_BUSSES)                           \
    M(int, TCAM_MEMORY_FULL_WIDTH)                      \
    M(int, TCAM_ROWS)                                   \
    M(int, TCAM_UNITS_PER_ROW)                          \
    M(int, TCAM_XBAR_GROUPS)                            \
    M(bool, TABLES_REQUIRE_ROW)

#define DECLARE_PER_TARGET_CONSTANT(TYPE, NAME) static TYPE NAME();

#define TARGET_CLASS_SPECIFIC_CLASSES \
    class ActionTable;                \
    class CounterTable;               \
    class ExactMatchTable;            \
    class GatewayTable;               \
    class MeterTable;                 \
    class StatefulTable;              \
    class TernaryIndirectTable;       \
    class TernaryMatchTable;
#define REGISTER_SET_SPECIFIC_CLASSES /* none */
#define TARGET_SPECIFIC_CLASSES       /* none */

class Target {
 public:
    class Phv;
    FOR_ALL_TARGETS(DECLARE_TARGET_CLASS)
    PER_TARGET_CONSTANTS(DECLARE_PER_TARGET_CONSTANT)

    static int encodeConst(int src);

    static int NUM_MAU_STAGES() {
        return numMauStagesOverride ? numMauStagesOverride : NUM_MAU_STAGES_PRIVATE();
    }
    static int NUM_EGRESS_STAGES() {
        int egress_stages = NUM_EGRESS_STAGES_PRIVATE();
        return numMauStagesOverride && numMauStagesOverride < egress_stages ? numMauStagesOverride
                                                                            : egress_stages;
    }
    static int NUM_STAGES(gress_t gr) {
        return gr == EGRESS ? NUM_EGRESS_STAGES() : NUM_MAU_STAGES();
    }

    static int OUTPUT_STAGE_EXTENSION() {
        return numMauStagesOverride ? 1 : OUTPUT_STAGE_EXTENSION_PRIVATE();
    }

    static void OVERRIDE_NUM_MAU_STAGES(int num);

    static int SRAM_ROWS(gress_t gr) {
        return gr == EGRESS ? SRAM_EGRESS_ROWS() : SRAM_INGRESS_ROWS();
    }

    // FIXME -- bus_type here is a Table::Layout::bus_type_t, but can't forward
    // declare a nested type.
    virtual int NUM_BUS_OF_TYPE_v(int bus_type) const;
    static int NUM_BUS_OF_TYPE(int bus_type);

 private:
    static int numMauStagesOverride;
};

#include "backends/tofino/bf-asm/gen/tofino/memories.pipe_addrmap.h"
#include "backends/tofino/bf-asm/gen/tofino/memories.pipe_top_level.h"
#include "backends/tofino/bf-asm/gen/tofino/memories.prsr_mem_main_rspec.h"
#include "backends/tofino/bf-asm/gen/tofino/regs.dprsr_hdr.h"
#include "backends/tofino/bf-asm/gen/tofino/regs.dprsr_inp.h"
#include "backends/tofino/bf-asm/gen/tofino/regs.ebp_rspec.h"
#include "backends/tofino/bf-asm/gen/tofino/regs.ibp_rspec.h"
#include "backends/tofino/bf-asm/gen/tofino/regs.mau_addrmap.h"
#include "backends/tofino/bf-asm/gen/tofino/regs.pipe_addrmap.h"
#include "backends/tofino/bf-asm/gen/tofino/regs.prsr_reg_merge_rspec.h"
#include "backends/tofino/bf-asm/gen/tofino/regs.tofino.h"

class Target::Tofino : public Target {
 public:
    static constexpr const char *const name = "tofino";
    static constexpr target_t tag = TOFINO;
    static constexpr target_t register_set = TOFINO;
    typedef Target::Tofino target_type;
    typedef Target::Tofino register_type;
    class Phv;
    struct top_level_regs {
        typedef ::Tofino::memories_top _mem_top;
        typedef ::Tofino::memories_pipe _mem_pipe;
        typedef ::Tofino::regs_top _regs_top;
        typedef ::Tofino::regs_pipe _regs_pipe;

        ::Tofino::memories_top mem_top;
        ::Tofino::memories_pipe mem_pipe;
        ::Tofino::regs_top reg_top;
        ::Tofino::regs_pipe reg_pipe;

        // map from handle to parser regs
        std::map<unsigned, ::Tofino::memories_all_parser_ *> parser_memory[2];
        std::map<unsigned, ::Tofino::regs_all_parser_ingress *> parser_ingress;
        std::map<unsigned, ::Tofino::regs_all_parser_egress *> parser_egress;
        ::Tofino::regs_all_parse_merge parser_merge;
    };
    struct parser_regs : public ParserRegisterSet {
        typedef ::Tofino::memories_all_parser_ _memory;
        typedef ::Tofino::regs_all_parser_ingress _ingress;
        typedef ::Tofino::regs_all_parser_egress _egress;
        typedef ::Tofino::regs_all_parse_merge _merge;

        ::Tofino::memories_all_parser_ memory[2];
        ::Tofino::regs_all_parser_ingress ingress;
        ::Tofino::regs_all_parser_egress egress;
        ::Tofino::regs_all_parse_merge merge;
    };

    typedef ::Tofino::regs_match_action_stage_ mau_regs;
    struct deparser_regs {
        typedef ::Tofino::regs_all_deparser_input_phase _input;
        typedef ::Tofino::regs_all_deparser_header_phase _header;

        ::Tofino::regs_all_deparser_input_phase input;
        ::Tofino::regs_all_deparser_header_phase header;
    };
    enum {
        ARAM_UNITS_PER_STAGE = 0,
        PARSER_CHECKSUM_UNITS = 2,
        PARSER_EXTRACT_BYTES = false,
        PARSER_DEPTH_MAX_BYTES_INGRESS = (((1 << 10) - 1) * 16),
        PARSER_DEPTH_MAX_BYTES_EGRESS = (((1 << 10) - 1) * 16),
        PARSER_DEPTH_MAX_BYTES_MULTITHREADED_EGRESS = 160,
        PARSER_DEPTH_MIN_BYTES_INGRESS = 0,
        PARSER_DEPTH_MIN_BYTES_EGRESS = 65,
        MATCH_BYTE_16BIT_PAIRS = true,
        MATCH_REQUIRES_PHYSID = false,
        MAX_IMMED_ACTION_DATA = 32,
        MAX_OVERHEAD_OFFSET = 64,
        MAX_OVERHEAD_OFFSET_NEXT = 40,
        NUM_MAU_STAGES_PRIVATE = 12,
        NUM_EGRESS_STAGES_PRIVATE = NUM_MAU_STAGES_PRIVATE,
        ACTION_INSTRUCTION_MAP_WIDTH = 7,
        DEPARSER_CHECKSUM_UNITS = 6,
        DEPARSER_CONSTANTS = 0,
        DEPARSER_MAX_POV_BYTES = 32,
        DEPARSER_MAX_POV_PER_USE = 1,
        DEPARSER_MAX_FD_ENTRIES = 192,
        DP_UNITS_PER_STAGE = 0,
        DYNAMIC_CONFIG = 0,
        DYNAMIC_CONFIG_INPUT_BITS = 0,
        EGRESS_SEPARATE = false,
        END_OF_PIPE = 0xff,
        EXACT_HASH_GROUPS = 8,
        EXACT_HASH_TABLES = 16,
        EXTEND_ALU_8_SLOTS = 0,
        EXTEND_ALU_16_SLOTS = 0,
        EXTEND_ALU_32_SLOTS = 0,
        GATEWAY_INHIBIT_INDEX = false,
        GATEWAY_MATCH_BITS = 56,  // includes extra expansion for range match
        GATEWAY_NEEDS_SEARCH_BUS = true,
        GATEWAY_PAYLOAD_GROUPS = 1,
        GATEWAY_ROWS = 8,
        GATEWAY_SINGLE_XBAR_GROUP = true,
        SUPPORT_TRUE_EOP = 0,
        INSTR_SRC2_BITS = 4,
        IMEM_COLORS = 2,
        IXBAR_HASH_GROUPS = 8,
        IXBAR_HASH_INDEX_MAX = 40,
        IXBAR_HASH_INDEX_STRIDE = 10,
        LOCAL_TIND_UNITS = 0,
        LONG_BRANCH_TAGS = 0,
        MAU_BASE_DELAY = 20,
        MAU_BASE_PREDICATION_DELAY = 11,
        MAU_ERROR_DELAY_ADJUST = 2,
        METER_ALU_GROUP_DATA_DELAY = 13,
        // To avoid under run scenarios, there is a minimum egress pipeline latency required
        MINIMUM_REQUIRED_EGRESS_PIPELINE_LATENCY = 160,
        NEXT_TABLE_EXEC_COMBINED = false,  // no next_exec on tofino1 at all
        NEXT_TABLE_SUCCESSOR_TABLE_DEPTH = 8,
        PHASE0_FORMAT_WIDTH = 64,
        REQUIRE_TCAM_ID = false,  // miss-only tables do not need a tcam id
        SRAM_EGRESS_ROWS = 8,
        SRAM_GLOBAL_ACCESS = false,
        SRAM_HBUS_SECTIONS_PER_STAGE = 0,
        SRAM_HBUSSES_PER_ROW = 0,
        SRAM_INGRESS_ROWS = 8,
        SRAM_LAMBS_PER_STAGE = 0,
        SRAM_LOGICAL_UNITS_PER_ROW = 6,
        SRAM_REMOVED_COLUMNS = 2,
        SRAM_STRIDE_COLUMN = 1,
        SRAM_STRIDE_ROW = 12,
        SRAM_STRIDE_STAGE = 0,
        SRAM_UNITS_PER_ROW = 12,
        STATEFUL_CMP_UNITS = 2,
        STATEFUL_CMP_ADDR_WIDTH = 2,
        STATEFUL_CMP_CONST_WIDTH = 4,
        STATEFUL_CMP_CONST_MASK = 0xf,
        STATEFUL_CMP_CONST_MIN = -8,
        STATEFUL_CMP_CONST_MAX = 7,
        STATEFUL_TMATCH_UNITS = 0,
        STATEFUL_OUTPUT_UNITS = 1,
        STATEFUL_PRED_MASK = (1U << (1 << STATEFUL_CMP_UNITS)) - 1,
        STATEFUL_REGFILE_ROWS = 4,
        STATEFUL_REGFILE_CONST_WIDTH = 32,
        SUPPORT_ALWAYS_RUN = 0,
        HAS_MPR = 0,
        SUPPORT_CONCURRENT_STAGE_DEP = 1,
        SUPPORT_OVERFLOW_BUS = 1,
        SUPPORT_SALU_FAST_CLEAR = 0,
        STATEFUL_ALU_ADDR_WIDTH = 2,
        STATEFUL_ALU_CONST_WIDTH = 4,
        STATEFUL_ALU_CONST_MASK = 0xf,
        STATEFUL_ALU_CONST_MIN = -8,  // TODO Is the same as the following one?
        STATEFUL_ALU_CONST_MAX = 7,
        MINIMUM_INSTR_CONSTANT = -8,  // TODO
        NUM_PARSERS = 18,
        NUM_PIPES = 4,
        OUTPUT_STAGE_EXTENSION_PRIVATE = 0,
        SYNTH2PORT_NEED_MAPRAMS = true,
        TCAM_EXTRA_NIBBLE = true,
        TCAM_GLOBAL_ACCESS = false,
        TCAM_MATCH_BUSSES = 2,
        TCAM_MEMORY_FULL_WIDTH = 47,
        TCAM_ROWS = 12,
        TCAM_UNITS_PER_ROW = 2,
        TCAM_XBAR_GROUPS = 12,
        TABLES_REQUIRE_ROW = 1,
    };
    static int encodeConst(int src) { return (src >> 10 << 15) | (0x8 << 10) | (src & 0x3ff); }
    TARGET_SPECIFIC_CLASSES
    REGISTER_SET_SPECIFIC_CLASSES
    TARGET_CLASS_SPECIFIC_CLASSES
};

void declare_registers(const Target::Tofino::top_level_regs *regs);
void undeclare_registers(const Target::Tofino::top_level_regs *regs);
void declare_registers(const Target::Tofino::parser_regs *regs);
void undeclare_registers(const Target::Tofino::parser_regs *regs);
void declare_registers(const Target::Tofino::mau_regs *regs, bool ignore, int stage);
void declare_registers(const Target::Tofino::deparser_regs *regs);
void undeclare_registers(const Target::Tofino::deparser_regs *regs);
void emit_parser_registers(const Target::Tofino::top_level_regs *regs, std::ostream &);

#include "backends/tofino/bf-asm/gen/jbay/memories.jbay_mem.h"
#include "backends/tofino/bf-asm/gen/jbay/memories.pipe_addrmap.h"
#include "backends/tofino/bf-asm/gen/jbay/memories.prsr_mem_main_rspec.h"
#include "backends/tofino/bf-asm/gen/jbay/regs.dprsr_reg.h"
#include "backends/tofino/bf-asm/gen/jbay/regs.epb_prsr4_reg.h"
#include "backends/tofino/bf-asm/gen/jbay/regs.ipb_prsr4_reg.h"
#include "backends/tofino/bf-asm/gen/jbay/regs.jbay_reg.h"
#include "backends/tofino/bf-asm/gen/jbay/regs.mau_addrmap.h"
#include "backends/tofino/bf-asm/gen/jbay/regs.pipe_addrmap.h"
#include "backends/tofino/bf-asm/gen/jbay/regs.pmerge_reg.h"
#include "backends/tofino/bf-asm/gen/jbay/regs.prsr_reg_main_rspec.h"

class Target::JBay : public Target {
 public:
    static constexpr const char *const name = "tofino2";
    static constexpr target_t tag = JBAY;
    static constexpr target_t register_set = JBAY;
    typedef Target::JBay target_type;
    typedef Target::JBay register_type;
    class Phv;
    struct top_level_regs {
        typedef ::JBay::memories_top _mem_top;
        typedef ::JBay::memories_pipe _mem_pipe;
        typedef ::JBay::regs_top _regs_top;
        typedef ::JBay::regs_pipe _regs_pipe;

        ::JBay::memories_top mem_top;
        ::JBay::memories_pipe mem_pipe;
        ::JBay::regs_top reg_top;
        ::JBay::regs_pipe reg_pipe;

        // map from handle to parser regs
        std::map<unsigned, ::JBay::memories_parser_ *> parser_memory[2];
        std::map<unsigned, ::JBay::regs_parser_ingress *> parser_ingress;
        std::map<unsigned, ::JBay::regs_parser_egress *> parser_egress;
        std::map<unsigned, ::JBay::regs_parser_main_ *> parser_main[2];
        ::JBay::regs_parse_merge parser_merge;
    };
    struct parser_regs : public ParserRegisterSet {
        typedef ::JBay::memories_parser_ _memory;
        typedef ::JBay::regs_parser_ingress _ingress;  // [9]
        typedef ::JBay::regs_parser_egress _egress;    // [9]
        typedef ::JBay::regs_parser_main_ _main;       // [9]
        typedef ::JBay::regs_parse_merge _merge;       // [1]

        ::JBay::memories_parser_ memory[2];
        ::JBay::regs_parser_ingress ingress;
        ::JBay::regs_parser_egress egress;
        ::JBay::regs_parser_main_ main[2];
        ::JBay::regs_parse_merge merge;
    };

    typedef ::JBay::regs_match_action_stage_ mau_regs;
    typedef ::JBay::regs_deparser deparser_regs;
    enum : int {
        ARAM_UNITS_PER_STAGE = 0,
        PARSER_CHECKSUM_UNITS = 5,
        PARSER_EXTRACT_BYTES = true,
        PARSER_DEPTH_MAX_BYTES_INGRESS = (((1 << 10) - 1) * 16),
        PARSER_DEPTH_MAX_BYTES_EGRESS = (32 * 16),
        PARSER_DEPTH_MAX_BYTES_MULTITHREADED_EGRESS = (32 * 16),
        PARSER_DEPTH_MIN_BYTES_INGRESS = 0,
        PARSER_DEPTH_MIN_BYTES_EGRESS = 0,
        MATCH_BYTE_16BIT_PAIRS = false,
        MATCH_REQUIRES_PHYSID = false,
        MAX_IMMED_ACTION_DATA = 32,
        MAX_OVERHEAD_OFFSET = 64,
        MAX_OVERHEAD_OFFSET_NEXT = 40,
#ifdef EMU_OVERRIDE_STAGE_COUNT
        NUM_MAU_STAGES_PRIVATE = EMU_OVERRIDE_STAGE_COUNT,
        OUTPUT_STAGE_EXTENSION_PRIVATE = 1,
#else
        NUM_MAU_STAGES_PRIVATE = 20,
        OUTPUT_STAGE_EXTENSION_PRIVATE = 0,
#endif
        NUM_EGRESS_STAGES_PRIVATE = NUM_MAU_STAGES_PRIVATE,
        ACTION_INSTRUCTION_MAP_WIDTH = 8,
        DEPARSER_CHECKSUM_UNITS = 8,
        DEPARSER_CONSTANTS = 8,
        DEPARSER_MAX_POV_BYTES = 16,
        DEPARSER_MAX_POV_PER_USE = 1,
        DEPARSER_CHUNKS_PER_GROUP = 8,
        DEPARSER_CHUNK_SIZE = 8,
        DEPARSER_CHUNK_GROUPS = 16,
        DEPARSER_CLOTS_PER_GROUP = 4,
        DEPARSER_TOTAL_CHUNKS = DEPARSER_CHUNK_GROUPS * DEPARSER_CHUNKS_PER_GROUP,
        DEPARSER_MAX_FD_ENTRIES = DEPARSER_TOTAL_CHUNKS,
        DP_UNITS_PER_STAGE = 0,
        DYNAMIC_CONFIG = 0,
        DYNAMIC_CONFIG_INPUT_BITS = 0,
        EGRESS_SEPARATE = false,
        END_OF_PIPE = 0x1ff,
        EXACT_HASH_GROUPS = 8,
        EXACT_HASH_TABLES = 16,
        EXTEND_ALU_8_SLOTS = 0,
        EXTEND_ALU_16_SLOTS = 0,
        EXTEND_ALU_32_SLOTS = 0,
        GATEWAY_INHIBIT_INDEX = false,
        GATEWAY_MATCH_BITS = 56,  // includes extra expansion for range match
        GATEWAY_NEEDS_SEARCH_BUS = true,
        GATEWAY_PAYLOAD_GROUPS = 5,
        GATEWAY_ROWS = 8,
        GATEWAY_SINGLE_XBAR_GROUP = true,
        SUPPORT_TRUE_EOP = 1,
        INSTR_SRC2_BITS = 5,
        IMEM_COLORS = 2,
        IXBAR_HASH_GROUPS = 8,
        IXBAR_HASH_INDEX_MAX = 40,
        IXBAR_HASH_INDEX_STRIDE = 10,
        LOCAL_TIND_UNITS = 0,
        LONG_BRANCH_TAGS = 8,
        MAU_BASE_DELAY = 23,
        MAU_BASE_PREDICATION_DELAY = 13,
        MAU_ERROR_DELAY_ADJUST = 3,
        METER_ALU_GROUP_DATA_DELAY = 15,
        NEXT_TABLE_EXEC_COMBINED = true,
        NEXT_TABLE_SUCCESSOR_TABLE_DEPTH = 8,
        PHASE0_FORMAT_WIDTH = 128,
        REQUIRE_TCAM_ID = false,  // miss-only tables do not need a tcam id
        SRAM_EGRESS_ROWS = 8,
        SRAM_GLOBAL_ACCESS = false,
        SRAM_HBUS_SECTIONS_PER_STAGE = 0,
        SRAM_HBUSSES_PER_ROW = 0,
        SRAM_INGRESS_ROWS = 8,
        SRAM_LAMBS_PER_STAGE = 0,
        SRAM_LOGICAL_UNITS_PER_ROW = 6,
        SRAM_REMOVED_COLUMNS = 2,
        SRAM_STRIDE_COLUMN = 1,
        SRAM_STRIDE_ROW = 12,
        SRAM_STRIDE_STAGE = 0,
        SRAM_UNITS_PER_ROW = 12,
        STATEFUL_CMP_UNITS = 4,
        STATEFUL_CMP_ADDR_WIDTH = 2,
        STATEFUL_CMP_CONST_WIDTH = 6,
        STATEFUL_CMP_CONST_MASK = 0x3f,
        STATEFUL_CMP_CONST_MIN = -32,
        STATEFUL_CMP_CONST_MAX = 31,
        STATEFUL_TMATCH_UNITS = 2,
        STATEFUL_OUTPUT_UNITS = 4,
        STATEFUL_PRED_MASK = (1U << (1 << STATEFUL_CMP_UNITS)) - 1,
        STATEFUL_REGFILE_ROWS = 4,
        STATEFUL_REGFILE_CONST_WIDTH = 34,
        SUPPORT_ALWAYS_RUN = 1,
        HAS_MPR = 1,
        SUPPORT_CONCURRENT_STAGE_DEP = 0,
        SUPPORT_OVERFLOW_BUS = 0,
        SUPPORT_SALU_FAST_CLEAR = 1,
        STATEFUL_ALU_ADDR_WIDTH = 2,
        STATEFUL_ALU_CONST_WIDTH = 4,
        STATEFUL_ALU_CONST_MASK = 0xf,
        STATEFUL_ALU_CONST_MIN = -8,  // TODO Is the same as the following one?
        STATEFUL_ALU_CONST_MAX = 7,
        MINIMUM_INSTR_CONSTANT = -4,  // TODO
        NUM_PARSERS = 36,
        NUM_PIPES = 4,
        TABLES_REQUIRE_ROW = 1,
        SYNTH2PORT_NEED_MAPRAMS = true,
        TCAM_EXTRA_NIBBLE = true,
        TCAM_GLOBAL_ACCESS = false,
        TCAM_MATCH_BUSSES = 2,
        TCAM_MEMORY_FULL_WIDTH = 47,
        TCAM_ROWS = 12,
        TCAM_UNITS_PER_ROW = 2,
        TCAM_XBAR_GROUPS = 12,
    };
    static int encodeConst(int src) { return (src >> 11 << 16) | (0x8 << 11) | (src & 0x7ff); }
    TARGET_SPECIFIC_CLASSES
    REGISTER_SET_SPECIFIC_CLASSES
};
void declare_registers(const Target::JBay::top_level_regs *regs);
void undeclare_registers(const Target::JBay::top_level_regs *regs);
void declare_registers(const Target::JBay::parser_regs *regs);
void undeclare_registers(const Target::JBay::parser_regs *regs);
void declare_registers(const Target::JBay::mau_regs *regs, bool ignore, int stage);
void declare_registers(const Target::JBay::deparser_regs *regs);

class Target::Tofino2H : public Target::JBay {
 public:
    static constexpr const char *const name = "tofino2h";
    static constexpr target_t tag = TOFINO2H;
    typedef Target::Tofino2H target_type;
    class Phv;
    enum {
        NUM_MAU_STAGES_PRIVATE = 6,
        NUM_EGRESS_STAGES_PRIVATE = NUM_MAU_STAGES_PRIVATE,
        OUTPUT_STAGE_EXTENSION_PRIVATE = 1,
    };
    TARGET_SPECIFIC_CLASSES
};

class Target::Tofino2M : public Target::JBay {
 public:
    static constexpr const char *const name = "tofino2m";
    static constexpr target_t tag = TOFINO2M;
    typedef Target::Tofino2M target_type;
    class Phv;
    enum {
        NUM_MAU_STAGES_PRIVATE = 12,
        NUM_EGRESS_STAGES_PRIVATE = NUM_MAU_STAGES_PRIVATE,
        OUTPUT_STAGE_EXTENSION_PRIVATE = 1,
    };
    TARGET_SPECIFIC_CLASSES
};

class Target::Tofino2U : public Target::JBay {
 public:
    static constexpr const char *const name = "tofino2u";
    static constexpr target_t tag = TOFINO2U;
    typedef Target::Tofino2U target_type;
    class Phv;
    enum {
        NUM_MAU_STAGES_PRIVATE = 20,
        NUM_EGRESS_STAGES_PRIVATE = NUM_MAU_STAGES_PRIVATE,
    };
    TARGET_SPECIFIC_CLASSES
};

class Target::Tofino2A0 : public Target::JBay {
 public:
    static constexpr const char *const name = "tofino2a0";
    static constexpr target_t tag = TOFINO2A0;
    typedef Target::Tofino2A0 target_type;
    class Phv;
    enum {
        NUM_MAU_STAGES_PRIVATE = 20,
        NUM_EGRESS_STAGES_PRIVATE = NUM_MAU_STAGES_PRIVATE,
    };
    TARGET_SPECIFIC_CLASSES
};

void emit_parser_registers(const Target::JBay::top_level_regs *regs, std::ostream &);

/** Macro to buid a switch table switching on a target_t, expanding to the same
 *  code for each target, with TARGET being a typedef for the target type */
#define SWITCH_FOREACH_TARGET(VAR, ...)                        \
    switch (VAR) {                                             \
        FOR_ALL_TARGETS(DO_SWITCH_FOREACH_TARGET, __VA_ARGS__) \
        default:                                               \
            BUG("invalid target");                             \
    }

#define DO_SWITCH_FOREACH_TARGET(TARGET_, ...) \
    case Target::TARGET_::tag: {               \
        typedef Target::TARGET_ TARGET;        \
        __VA_ARGS__                            \
        break;                                 \
    }

#define SWITCH_FOREACH_REGISTER_SET(VAR, ...)                              \
    switch (VAR) {                                                         \
        FOR_ALL_REGISTER_SETS(DO_SWITCH_FOREACH_REGISTER_SET, __VA_ARGS__) \
        default:                                                           \
            BUG("invalid target");                                         \
    }

#define DO_SWITCH_FOREACH_REGISTER_SET(REGS_, ...) \
    TARGETS_USING_REGS(REGS_, CASE_FOR_TARGET) {   \
        typedef Target::REGS_ TARGET;              \
        __VA_ARGS__                                \
        break;                                     \
    }

#define SWITCH_FOREACH_TARGET_CLASS(VAR, ...)                               \
    switch (VAR) {                                                          \
        FOR_ALL_TARGET_CLASSES(DO_SWITCH_FOREACH_TARGET_CLASS, __VA_ARGS__) \
        default:                                                            \
            BUG("invalid target");                                          \
    }

#define DO_SWITCH_FOREACH_TARGET_CLASS(CLASS_, ...) \
    TARGETS_IN_CLASS(CLASS_, CASE_FOR_TARGET) {     \
        typedef Target::CLASS_ TARGET;              \
        __VA_ARGS__                                 \
        break;                                      \
    }

#define CASE_FOR_TARGET(TARGET) case Target::TARGET::tag:

/* macro to define a function that overloads over a GROUP of types -- will declare all the
 * functions that overload on a Target::type argument and a 'generic' overload that calls
 * the right specific overload based on options.target
 * GROUP can be one of
 *    TARGET -- overload on all the different targets
 *    REGISTER_SET -- overload just on the register sets (targets that share a register
 *                    set will only have one overload)
 *    TARGET_CLASS -- overload based on the CLASS
 * RTYPE NAME ARGDECL together make the declaration of the (generic) function, the overloads
 * will all have a Target::type argument prepended.  The final ARGS argument is the argument
 * list that that will be forwarded (basically ARGDECL without the types)
 */
#define EXPAND(...) __VA_ARGS__
#define EXPAND_COMMA(...) __VA_OPT__(,) __VA_ARGS__
#define EXPAND_COMMA_CLOSE(...) __VA_OPT__(,) __VA_ARGS__ )
#define TARGET_OVERLOAD(TARGET, FN, ARGS, ...) FN(Target::TARGET::EXPAND ARGS) __VA_ARGS__;

#define DECL_OVERLOAD_FUNC(TARGET, RTYPE, NAME, ARGDECL, ARGS) \
    RTYPE NAME(Target::TARGET EXPAND_COMMA_CLOSE ARGDECL;
#define OVERLOAD_FUNC_FOREACH(GROUP, RTYPE, NAME, ARGDECL, ARGS, ...)                    \
    FOR_EACH_##GROUP(DECL_OVERLOAD_FUNC, RTYPE, NAME, ARGDECL, ARGS)                     \
        RTYPE NAME ARGDECL __VA_ARGS__ {                                                 \
        SWITCH_FOREACH_##GROUP(options.target, return NAME(TARGET() EXPAND_COMMA ARGS);) \
    }

#endif /* BACKENDS_TOFINO_BF_ASM_TARGET_H_ */
