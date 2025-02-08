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

#include "backends/tofino/bf-asm/target.h"

#include "asm-types.h"
#include "backends/tofino/bf-asm/config.h"
#include "backends/tofino/bf-asm/tables.h"
#include "bson.h"
#include "parser.h"
#include "ubits.h"

void declare_registers(const Target::Tofino::top_level_regs *regs) {
    declare_registers(&regs->mem_top, sizeof(regs->mem_top),
                      [=](std::ostream &out, const char *addr, const void *end) {
                          out << "memories.top";
                          regs->mem_top.emit_fieldname(out, addr, end);
                      });
    declare_registers(&regs->mem_pipe, sizeof(regs->mem_pipe),
                      [=](std::ostream &out, const char *addr, const void *end) {
                          out << "memories.pipe";
                          regs->mem_pipe.emit_fieldname(out, addr, end);
                      });
    declare_registers(&regs->reg_top, sizeof(regs->reg_top),
                      [=](std::ostream &out, const char *addr, const void *end) {
                          out << "registers.top";
                          regs->reg_top.emit_fieldname(out, addr, end);
                      });
    declare_registers(&regs->reg_pipe, sizeof(regs->reg_pipe),
                      [=](std::ostream &out, const char *addr, const void *end) {
                          out << "registers.pipe";
                          regs->reg_pipe.emit_fieldname(out, addr, end);
                      });
}
void undeclare_registers(const Target::Tofino::top_level_regs *regs) {
    undeclare_registers(&regs->mem_top);
    undeclare_registers(&regs->mem_pipe);
    undeclare_registers(&regs->reg_top);
    undeclare_registers(&regs->reg_pipe);
}

void declare_registers(const Target::Tofino::parser_regs *regs) {
    declare_registers(&regs->memory[INGRESS], sizeof regs->memory[INGRESS],
                      [=](std::ostream &out, const char *addr, const void *end) {
                          out << "parser.mem[INGRESS]";
                          regs->memory[INGRESS].emit_fieldname(out, addr, end);
                      });
    declare_registers(&regs->memory[EGRESS], sizeof regs->memory[EGRESS],
                      [=](std::ostream &out, const char *addr, const void *end) {
                          out << "parser.mem[EGRESS]";
                          regs->memory[EGRESS].emit_fieldname(out, addr, end);
                      });
    declare_registers(&regs->ingress, sizeof regs->ingress,
                      [=](std::ostream &out, const char *addr, const void *end) {
                          out << "parser.ibp_reg";
                          regs->ingress.emit_fieldname(out, addr, end);
                      });
    declare_registers(&regs->egress, sizeof regs->egress,
                      [=](std::ostream &out, const char *addr, const void *end) {
                          out << "parser.ebp_reg";
                          regs->egress.emit_fieldname(out, addr, end);
                      });
    declare_registers(&regs->merge, sizeof regs->merge,
                      [=](std::ostream &out, const char *addr, const void *end) {
                          out << "parser.merge";
                          regs->merge.emit_fieldname(out, addr, end);
                      });
}
void undeclare_registers(const Target::Tofino::parser_regs *regs) {
    undeclare_registers(&regs->memory[INGRESS]);
    undeclare_registers(&regs->memory[EGRESS]);
    undeclare_registers(&regs->ingress);
    undeclare_registers(&regs->egress);
    undeclare_registers(&regs->merge);
}
void declare_registers(const Target::Tofino::mau_regs *regs, bool, int stage) {
    declare_registers(regs, sizeof *regs,
                      [=](std::ostream &out, const char *addr, const void *end) {
                          out << "mau[" << stage << "]";
                          regs->emit_fieldname(out, addr, end);
                      });
}
void declare_registers(const Target::Tofino::deparser_regs *regs) {
    declare_registers(&regs->input, sizeof(regs->input),
                      [=](std::ostream &out, const char *addr, const void *end) {
                          out << "deparser.input_phase";
                          regs->input.emit_fieldname(out, addr, end);
                      });
    declare_registers(&regs->header, sizeof(regs->header),
                      [=](std::ostream &out, const char *addr, const void *end) {
                          out << "deparser.header_phase";
                          regs->header.emit_fieldname(out, addr, end);
                      });
}
void undeclare_registers(const Target::Tofino::deparser_regs *regs) {
    undeclare_registers(&regs->input);
    undeclare_registers(&regs->header);
}

void emit_parser_registers(const Target::Tofino::top_level_regs *regs, std::ostream &out) {
    std::set<int> emitted_parsers;
    // The driver can reprogram parser blocks at runtime. We output parser
    // blocks in the binary with the same base address. The driver uses the
    // parser handle at the start of each block to associate the parser block
    // with its respective parser node in context.json.
    // In a p4 program, the user can associate multiple parsers to a
    // multi-parser configuration but only map a few ports. The unmapped
    // parser(s) will be output in context.json node and binary but not have an
    // associated port map in context.json. The driver will not initialize any
    // parsers with these unmapped parser(s) but use them to reconfigure at
    // runtime if required.
    uint64_t pipe_mem_base_addr = 0x200000000000;
    uint64_t prsr_mem_base_addr = (pipe_mem_base_addr + 0x1C800000000) >> 4;
    uint64_t pipe_regs_base_addr = 0x2000000;
    uint64_t prsr_regs_base_addr = pipe_regs_base_addr + 0x700000;
    for (auto ig : regs->parser_ingress) {
        out << binout::tag('P') << binout::byte4(ig.first);
        ig.second->emit_binary(out, prsr_regs_base_addr);
    }
    for (auto ig : regs->parser_memory[INGRESS]) {
        out << binout::tag('P') << binout::byte4(ig.first);
        ig.second->emit_binary(out, prsr_mem_base_addr);
    }
    prsr_regs_base_addr = pipe_regs_base_addr + 0x740000;
    for (auto eg : regs->parser_egress) {
        out << binout::tag('P') << binout::byte4(eg.first);
        eg.second->emit_binary(out, prsr_regs_base_addr);
    }
    prsr_mem_base_addr = (pipe_mem_base_addr + 0x1C800400000) >> 4;
    for (auto eg : regs->parser_memory[EGRESS]) {
        out << binout::tag('P') << binout::byte4(eg.first);
        eg.second->emit_binary(out, prsr_mem_base_addr);
    }
}

void declare_registers(const Target::JBay::top_level_regs *regs) {
    declare_registers(&regs->mem_top, sizeof(regs->mem_top),
                      [=](std::ostream &out, const char *addr, const void *end) {
                          out << "memories.top";
                          regs->mem_top.emit_fieldname(out, addr, end);
                      });
    declare_registers(&regs->mem_pipe, sizeof(regs->mem_pipe),
                      [=](std::ostream &out, const char *addr, const void *end) {
                          out << "memories.pipe";
                          regs->mem_pipe.emit_fieldname(out, addr, end);
                      });
    declare_registers(&regs->reg_top, sizeof(regs->reg_top),
                      [=](std::ostream &out, const char *addr, const void *end) {
                          out << "registers.top";
                          regs->reg_top.emit_fieldname(out, addr, end);
                      });
    declare_registers(&regs->reg_pipe, sizeof(regs->reg_pipe),
                      [=](std::ostream &out, const char *addr, const void *end) {
                          out << "registers.pipe";
                          regs->reg_pipe.emit_fieldname(out, addr, end);
                      });
}
void undeclare_registers(const Target::JBay::top_level_regs *regs) {
    undeclare_registers(&regs->mem_top);
    undeclare_registers(&regs->mem_pipe);
    undeclare_registers(&regs->reg_top);
    undeclare_registers(&regs->reg_pipe);
}
void declare_registers(const Target::JBay::parser_regs *regs) {
    declare_registers(&regs->memory[INGRESS], sizeof regs->memory[INGRESS],
                      [=](std::ostream &out, const char *addr, const void *end) {
                          out << "parser.mem[INGRESS]";
                          regs->memory[INGRESS].emit_fieldname(out, addr, end);
                      });
    declare_registers(&regs->memory[EGRESS], sizeof regs->memory[EGRESS],
                      [=](std::ostream &out, const char *addr, const void *end) {
                          out << "parser.mem[EGRESS]";
                          regs->memory[EGRESS].emit_fieldname(out, addr, end);
                      });
    declare_registers(&regs->ingress, sizeof regs->ingress,
                      [=](std::ostream &out, const char *addr, const void *end) {
                          out << "parser.ipb_reg";
                          regs->ingress.emit_fieldname(out, addr, end);
                      });
    declare_registers(&regs->egress, sizeof regs->egress,
                      [=](std::ostream &out, const char *addr, const void *end) {
                          out << "parser.epb_reg";
                          regs->egress.emit_fieldname(out, addr, end);
                      });
    declare_registers(&regs->main[INGRESS], sizeof regs->main[INGRESS],
                      [=](std::ostream &out, const char *addr, const void *end) {
                          out << "parser.ingress.main";
                          regs->main[INGRESS].emit_fieldname(out, addr, end);
                      });
    declare_registers(&regs->main[EGRESS], sizeof regs->main[EGRESS],
                      [=](std::ostream &out, const char *addr, const void *end) {
                          out << "parser.egress.main";
                          regs->main[EGRESS].emit_fieldname(out, addr, end);
                      });
    declare_registers(&regs->merge, sizeof regs->merge,
                      [=](std::ostream &out, const char *addr, const void *end) {
                          out << "parser.merge";
                          regs->merge.emit_fieldname(out, addr, end);
                      });
}
void undeclare_registers(const Target::JBay::parser_regs *regs) {
    undeclare_registers(&regs->memory[INGRESS]);
    undeclare_registers(&regs->memory[EGRESS]);
    undeclare_registers(&regs->ingress);
    undeclare_registers(&regs->egress);
    undeclare_registers(&regs->main[INGRESS]);
    undeclare_registers(&regs->main[EGRESS]);
    undeclare_registers(&regs->merge);
}
void declare_registers(const Target::JBay::mau_regs *regs, bool, int stage) {
    declare_registers(regs, sizeof *regs,
                      [=](std::ostream &out, const char *addr, const void *end) {
                          out << "mau[" << stage << "]";
                          regs->emit_fieldname(out, addr, end);
                      });
}
void declare_registers(const Target::JBay::deparser_regs *regs) {
    declare_registers(regs, sizeof *regs,
                      [=](std::ostream &out, const char *addr, const void *end) {
                          out << "deparser.regs";
                          regs->emit_fieldname(out, addr, end);
                      });
}

void emit_parser_registers(const Target::JBay::top_level_regs *regs, std::ostream &out) {
    std::set<int> emitted_parsers;
    for (auto ig : regs->parser_ingress) {
        json::map header;
        header["handle"] = ig.first;
        out << binout::tag('P') << json::binary(header);
        ig.second->emit_binary(out, 0);
    }
    for (auto eg : regs->parser_egress) {
        json::map header;
        header["handle"] = eg.first;
        out << binout::tag('P') << json::binary(header);
        eg.second->emit_binary(out, 0);
    }
    for (auto ig : regs->parser_main[INGRESS]) {
        json::map header;
        header["handle"] = ig.first;
        out << binout::tag('P') << json::binary(header);
        ig.second->emit_binary(out, 0);
    }
    for (auto eg : regs->parser_main[EGRESS]) {
        json::map header;
        header["handle"] = eg.first;
        out << binout::tag('P') << json::binary(header);
        eg.second->emit_binary(out, 0);
    }
    for (auto ig : regs->parser_memory[INGRESS]) {
        json::map header;
        header["handle"] = ig.first;
        out << binout::tag('P') << json::binary(header);
        ig.second->emit_binary(out, 0);
    }
    for (auto eg : regs->parser_memory[EGRESS]) {
        json::map header;
        header["handle"] = eg.first;
        out << binout::tag('P') << json::binary(header);
        eg.second->emit_binary(out, 0);
    }
}

int Target::numMauStagesOverride = 0;

int Target::encodeConst(int src) {
    SWITCH_FOREACH_TARGET(options.target, return TARGET::encodeConst(src););
    BUG();
    return 0;
}

void Target::OVERRIDE_NUM_MAU_STAGES(int num) {
    int allowed = NUM_MAU_STAGES_PRIVATE();
    BUG_CHECK(num > 0 && num <= allowed,
              "Invalid override for NUM_MAU_STAGES. Allowed range is <1, %d>, got %d.", allowed,
              num);

    numMauStagesOverride = num;
    return;
}

int Target::NUM_BUS_OF_TYPE_v(int bus_type) const {
    // default values for Tofino1/2
    switch (static_cast<Table::Layout::bus_type_t>(bus_type)) {
        case Table::Layout::SEARCH_BUS:
        case Table::Layout::RESULT_BUS:
        case Table::Layout::TIND_BUS:
            return 2;
        case Table::Layout::IDLE_BUS:
            return 20;
        default:
            return 0;
    }
}

int Target::NUM_BUS_OF_TYPE(int bus_type) {
    SWITCH_FOREACH_TARGET(options.target, return TARGET().NUM_BUS_OF_TYPE_v(bus_type);)
}

// should these be inline in the header file?
#define DEFINE_PER_TARGET_CONSTANT(TYPE, NAME)                      \
    TYPE Target::NAME() {                                           \
        SWITCH_FOREACH_TARGET(options.target, return TARGET::NAME;) \
        return (TYPE){}; /* NOLINT(readability/braces) */           \
    }
PER_TARGET_CONSTANTS(DEFINE_PER_TARGET_CONSTANT)
