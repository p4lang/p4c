/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include <algorithm>

#include <boost/optional/optional_io.hpp>

#include "bf-p4c/common/asm_output.h"
#include "bf-p4c/common/autoindent.h"
#include "bf-p4c/common/debug_info.h"
#include "bf-p4c/parde/asm_output.h"
#include "bf-p4c/parde/clot/clot_info.h"
#include "bf-p4c/parde/parde_visitor.h"
#include "bf-p4c/phv/phv_fields.h"
#include "lib/log.h"
#include "lib/match.h"
#include "lib/range.h"
#include "midend/parser_enforce_depth_req.h"

namespace {

static int pvs_handle = 512;
static std::map<cstring, int> pvs_handles;

/**
 * @ingroup parde
 * @brief Generates parser assembly by walking the IR and writes the result to an output stream.
 *
 * The parser IR must be in lowered form - i.e., the root must
 * a IR::BFN::LoweredParser rather than a IR::BFN::Parser.
 *
 * ### PVS Handle Generation
 *
 * TNA instantiates different parsers for ingress and egress and therefore
 * expects a unique PVS handle for each gress.
 * However this is not the case with V1Model as it always instantiates the same parser.
 *
 * In TNA:
 * - P4-14: Share PVS Handle across and within ig/eg
 * - P4-16: Share PVS Handle only within ig/eg, don't share across ig/eg
 */
struct ParserAsmSerializer : public ParserInspector {
    explicit ParserAsmSerializer(std::ostream &out, const PhvInfo &phv, const ClotInfo &clot_info)
        : out(out), phv(phv), clot_info(clot_info) {
        is_v1Model = (BackendOptions().arch == "v1model");
    }

 private:
    bool is_v1Model = false;
    void init_apply() {
        // If not v1Model share pvs handles only when pvs is invoked multiple
        // times within the same parser.
        if (!is_v1Model) pvs_handles.clear();
    }

    int getPvsHandle(cstring pvsName) {
        // For P4_14 parser value set is shared across ingress and egress,
        // Assign the same pvs handle in these cases
        int handle = -1;
        if (pvs_handles.count(pvsName) > 0) {
            handle = pvs_handles[pvsName];
        } else {
            handle = --pvs_handle;
            pvs_handles[pvsName] = handle;
        }
        return handle;
    }

    bool preorder(const IR::BFN::LoweredParser *parser) override {
        AutoIndent indentParser(indent);

        out << "parser " << parser->gress;
        if (parser->portmap.size() != 0) {
            const char *sep = " [ ";
            for (auto port : parser->portmap) {
                out << sep << port;
                sep = ", ";
            }
            out << " ]";
        }
        out << ":" << std::endl;

        if (parser->name && parser->portmap.size() != 0)
            out << indent << "name: " << canon_name(parser->name) << std::endl;

        if (parser->start)
            out << indent << "start: " << canon_name(parser->start->name) << std::endl;

        if (!parser->initZeroContainers.empty()) {
            out << indent << "init_zero: ";
            const char *sep = "[ ";
            for (auto container : parser->initZeroContainers) {
                out << sep << container;
                sep = ", ";
            }
            out << " ]" << std::endl;
        }

        if (!parser->bitwiseOrContainers.empty()) {
            out << indent << "bitwise_or: ";
            const char *sep = "[ ";
            for (auto container : parser->bitwiseOrContainers) {
                out << sep << container;
                sep = ", ";
            }
            out << " ]" << std::endl;
        }

        if (!parser->clearOnWriteContainers.empty()) {
            out << indent << "clear_on_write: ";
            const char *sep = "[ ";
            for (auto container : parser->clearOnWriteContainers) {
                out << sep << container;
                sep = ", ";
            }
            out << " ]" << std::endl;
        }

        out << indent << "hdr_len_adj: " << parser->hdrLenAdj << std::endl;

        if (parser->metaOpt) out << indent << "meta_opt: " << *(parser->metaOpt) << std::endl;

        if (parser->parserError)
            out << indent << "parser_error: " << parser->parserError << std::endl;

        // Parse depth checks disabled (used by driver to disable multi-threading)
        if ((Device::pardeSpec().minParseDepth(parser->gress) > 0 &&
             BackendOptions().disable_parse_min_depth_limit) ||
            (Device::pardeSpec().maxParseDepth(parser->gress) < SIZE_MAX &&
             BackendOptions().disable_parse_max_depth_limit))
            out << indent << "parse_depth_checks_disabled: true" << std::endl;

        // Rate Limiter Setup :
        //
        // The rate limiter is configured in the parser arbiter which controls
        // the rate at which phv arrays are sent to MAU. Based on input pps load
        // we determine the configuration for rate limiter.
        //
        // Tofino - Sets max, inc. dec values on rate limiter
        //  - Decrement credits by dec after sending phv
        //  - Send only if available credits > dec value
        //  - If no phv sent increment credits by inc value
        //  - Saturate credits at max value
        //
        // JBay - Sets max, inc, interval values on rate limiter
        //  - Decrement credits by 1 after sending phv
        //  - Send only if available credit > 0
        //  - After every interval cycles increment credits by inc value
        //  - Saturate credits at max value
        //
        auto pps_load = BackendOptions().traffic_limit;
        if (pps_load > 0 && pps_load < 100) {
            int bubble_load = 100 - pps_load;
            int bubble_gcd = std::__gcd(100, bubble_load);
            int bubble_dec = bubble_load / bubble_gcd;
            int bubble_max = (100 / bubble_gcd) - bubble_dec;
            int bubble_inc = bubble_max;
            out << indent++ << "bubble: " << std::endl;
            out << indent << "max: " << bubble_max << std::endl;
            if (Device::currentDevice() == Device::TOFINO) {
                out << indent << "dec: " << bubble_dec << std::endl;
                LOG3("Bubble Rate Limiter ( load : "
                     << pps_load << "%, bubble: " << bubble_load << "%, max: " << bubble_max
                     << ", inc: " << bubble_inc << ", dec: " << bubble_dec << ")");
            } else {
                int bubble_intvl = 100 / bubble_gcd;
                bubble_inc = pps_load / bubble_gcd;
                out << indent << "interval: " << bubble_intvl << std::endl;
                LOG3("Bubble Rate Limiter ( load : "
                     << pps_load << "%, bubble: " << bubble_load << "%, max: " << bubble_max
                     << ", inc: " << bubble_inc << ", intvl: " << bubble_intvl << ")");
            }
            out << indent << "inc: " << bubble_inc << std::endl;
            indent--;
        }

        out << indent << "states:" << std::endl;

        return true;
    }

    bool preorder(const IR::BFN::LoweredParserState *state) override {
        AutoIndent indentState(indent, 2);

        out << indent << canon_name(state->name) << ':' << std::endl;

        if (!state->select->regs.empty() || !state->select->counters.empty()) {
            AutoIndent indentSelect(indent);

            out << indent << "match: ";
            const char *sep = "[ ";
            for (const auto &c : state->select->counters) {
                if (c->is<IR::BFN::ParserCounterIsZero>())
                    out << sep << "ctr_zero";
                else if (c->is<IR::BFN::ParserCounterIsNegative>())
                    out << sep << "ctr_neg";
                sep = ", ";
            }
            for (const auto &r : state->select->regs) {
                out << sep << r;
                sep = ", ";
            }
            out << " ]" << std::endl;

            // Print human-friendly debug info about what the select offsets
            // mean in terms of the original P4 program.
            for (auto &info : state->select->debug.info) out << "      # - " << info << std::endl;
        }

        if (state->name.startsWith(BFN::ParserEnforceDepthReq::pad_state_name.string())) {
            // FIXME -- what if the user uses this name for their own state?  Should have
            // flag in the state that identifies it as one that is used for min padding
            out << indent << "  option: ignore_max_depth" << std::endl;
        }

        for (auto *match : state->transitions) outputMatch(match, state->thread());

        return true;
    }

    void outputMatch(const IR::BFN::LoweredParserMatch *match, gress_t gress) {
        AutoIndent indentMatchPattern(indent);

        if (auto *const_val = match->value->to<IR::BFN::ParserConstMatchValue>()) {
            out << indent << const_val->value << ':' << std::endl;
        } else if (auto *pvs = match->value->to<IR::BFN::ParserPvsMatchValue>()) {
            out << indent << "value_set " << pvs->name << " " << pvs->size << ":" << std::endl;
            AutoIndent indentMatch(indent);
            out << indent << "handle: " << getPvsHandle(pvs->name) << std::endl;
            out << indent << "field_mapping:" << std::endl;
            AutoIndent indentFieldMap(indent);
            for (const auto &m : pvs->mapping) {
                cstring fieldname = m.first.first;
                cstring reg_name = m.second.first.name;
                out << indent << fieldname << "(" << m.first.second.lo << ".." << m.first.second.hi
                    << ")" << " : " << reg_name << "(" << m.second.second.lo << ".."
                    << m.second.second.hi << ")" << std::endl;
            }
        } else {
            BUG("unknown parser match value %1%", match->value);
        }

        AutoIndent indentMatch(indent);

        if (match->priority) out << indent << "priority: " << match->priority->val << std::endl;

        for (auto *csum : match->checksums) outputChecksum(csum);

        for (auto *cntr : match->counters) outputCounter(cntr);

        int intrinsic_width = 0;
        for (auto *stmt : match->extracts) {
            if (auto *extract = stmt->to<IR::BFN::LoweredExtractPhv>())
                outputExtractPhv(extract, intrinsic_width);
            else if (auto *extract = stmt->to<IR::BFN::LoweredExtractClot>())
                outputExtractClot(extract);
            else
                BUG("unknown lowered parser primitive type");
        }

        if (intrinsic_width > 0 && gress == EGRESS)
            out << indent << "intr_md: " << intrinsic_width << std::endl;

        if (!match->saves.empty()) outputSave(match->saves);

        if (!match->scratches.empty()) outputScratch(match->scratches);

        if (match->shift != 0) out << indent << "shift: " << match->shift << std::endl;

        if (match->hdrLenIncFinalAmt)
            out << indent << "hdr_len_inc_stop: " << *match->hdrLenIncFinalAmt << std::endl;

        if (match->bufferRequired)
            out << indent << "buf_req: " << *match->bufferRequired << std::endl;

        if (match->offsetInc) out << indent << "offset_inc: " << *match->offsetInc << std::endl;

        if (match->partialHdrErrProc) out << indent << "partial_hdr_err_proc: 1" << std::endl;

        out << indent << "next: ";
        if (match->next)
            out << canon_name(match->next->name);
        else if (match->loop)
            out << canon_name(match->loop);
        else
            out << "end";

        out << std::endl;
    }

    void outputSave(const IR::Vector<IR::BFN::LoweredSave> &saves) {
        const char *sep = "load: { ";
        out << indent;
        for (const auto *save : saves) {
            if (auto *source = save->source->to<IR::BFN::LoweredInputBufferRVal>()) {
                auto bytes = source->range;
                out << sep << save->dest << " : " << Range(bytes.lo, bytes.hi);
                sep = ", ";
            }
        }
        out << " }" << std::endl;
    }

    void outputScratch(const std::set<MatchRegister> &scratches) {
        const char *sep = "save: [ ";
        out << indent;
        for (auto reg : scratches) {
            out << sep << reg.name;
            sep = ", ";
        }
        out << " ]" << std::endl;
    }

    void outputExtractPhv(const IR::BFN::LoweredExtractPhv *extract, int &intrinsic_width) {
        // Generate the assembly that actually implements the extract.
        if (auto *source = extract->source->to<IR::BFN::LoweredInputBufferRVal>()) {
            auto bytes = source->range;
            out << indent << Range(bytes.lo, bytes.hi) << ": " << extract->dest;
            for (auto sl : phv.get_slices_in_container(extract->dest->container)) {
                if (sl.field()->is_intrinsic()) intrinsic_width += sl.width();
            }
        } else if (auto *source = extract->source->to<IR::BFN::LoweredConstantRVal>()) {
            out << indent << extract->dest << ": " << source->constant;
        } else {
            BUG("Can't generate assembly for: %1%", extract);
        }

        // Print human-friendly debug info about the extract - what it's writing
        // to, what higher-level extracts it may have been generated from, etc.
        for (auto &info : extract->debug.info) {
            if (extract->debug.info.size() == 1)
                out << "  # ";
            else
                out << std::endl << indent << "    # - ";
            out << info;
        }

        out << std::endl;
    }

    void outputExtractClot(const IR::BFN::LoweredExtractClot *extract) {
        if (extract->is_start) {
            out << indent << "clot " << extract->dest->tag << " :" << std::endl;
            AutoIndent ai(indent, 1);

            auto *source = extract->source->to<IR::BFN::LoweredPacketRVal>();
            cstring name = clot_info.sanitize_state_name(extract->higher_parser_state->name,
                                                         extract->higher_parser_state->gress);

            out << indent << "start: " << source->range.lo << std::endl;
            out << indent << "length: " << extract->dest->length_in_bytes(name) << std::endl;

            if (extract->dest->csum_unit) {
                out << indent << "checksum: " << *extract->dest->csum_unit << std::endl;
            }

            if (extract->dest->stack_depth) {
                out << indent << "stack_depth: " << *extract->dest->stack_depth << std::endl;
            }

            if (extract->dest->stack_inc) {
                out << indent << "stack_inc: " << *extract->dest->stack_inc << std::endl;
            }
        } else {
            out << indent << "# clot " << extract->dest->tag << " (spilled)" << std::endl;
        }
    }

    void outputChecksum(const IR::BFN::LoweredParserChecksum *csum) {
        out << indent << "checksum " << csum->unit_id << ":" << std::endl;
        AutoIndent indentCsum(indent, 1);
        out << indent << "type: " << csum->type << std::endl;

        out << indent << "mask: ";
        out << "[ ";

        unsigned i = 0;
        for (auto r : csum->masked_ranges) {
            out << Range(r.lo, r.hi);

            if (i != csum->masked_ranges.size() - 1) out << ", ";
            i++;
        }

        out << " ]" << std::endl;
        out << indent << "swap: " << csum->swap << std::endl;
        out << indent << "start: " << csum->start << std::endl;
        out << indent << "end: " << csum->end << std::endl;
        if (csum->multiply_2 > 0) {
            out << indent << "mul_2: " << csum->multiply_2 << std::endl;
        }

        if (csum->end) {
            if (csum->type == IR::BFN::ChecksumMode::VERIFY && csum->csum_err)
                out << indent << "dest: " << csum->csum_err << std::endl;
            else if (csum->type == IR::BFN::ChecksumMode::RESIDUAL && csum->phv_dest)
                out << indent << "dest: " << csum->phv_dest << std::endl;
            else if (csum->type == IR::BFN::ChecksumMode::CLOT)
                out << indent << "dest: clot " << csum->clot_dest.tag << std::endl;

            if (csum->type == IR::BFN::ChecksumMode::RESIDUAL)
                out << indent << "end_pos: " << csum->end_pos << std::endl;
        }
    }

    void outputCounter(const IR::BFN::ParserCounterPrimitive *cntr) {
        if (auto *init = cntr->to<IR::BFN::ParserCounterLoadImm>()) {
            out << indent << "counter:" << std::endl;

            indent++;

            out << indent << "imm: " << init->imm << std::endl;

            if (init->push) out << indent << "push: 1" << std::endl;

            if (init->update_with_top) out << indent << "update_with_top: 1" << std::endl;

            indent--;
        } else if (auto *load = cntr->to<IR::BFN::ParserCounterLoadPkt>()) {
            out << indent << "counter:" << std::endl;

            BUG_CHECK(load->source->reg_slices.size() == 1,
                      "counter load allocated to more than 1 registers");

            auto reg = load->source->reg_slices[0].first;
            auto slice = load->source->reg_slices[0].second.toOrder<Endian::Little>(reg.size * 8);

            BUG_CHECK(slice.size() <= 8, "parser counter load more than 8 bits");

            indent++;
            out << indent << "src: " << reg.name;

            if (reg.size == 2) out << (slice.lo ? "_hi" : "_lo");

            out << std::endl;

            if (load->max) out << indent << "max: " << *(load->max) << std::endl;

            if (load->rotate) out << indent << "rotate: " << *(load->rotate) << std::endl;

            if (load->mask) out << indent << "mask: " << *(load->mask) << std::endl;

            if (load->add) out << indent << "add: " << *(load->add) << std::endl;

            if (load->push) out << indent << "push: 1" << std::endl;

            if (load->update_with_top) out << indent << "update_with_top: 1" << std::endl;

            indent--;
        } else if (auto *inc = cntr->to<IR::BFN::ParserCounterIncrement>()) {
            out << indent << "counter: inc " << inc->value << std::endl;
        } else if (auto *inc = cntr->to<IR::BFN::ParserCounterDecrement>()) {
            out << indent << "counter: dec " << inc->value << std::endl;
        } else if (cntr->is<IR::BFN::ParserCounterPop>()) {
            out << indent << "counter: pop" << std::endl;
        }
    }

    std::ostream &out;
    const PhvInfo &phv;
    const ClotInfo &clot_info;
    indent_t indent;
};

}  // namespace

ParserAsmOutput::ParserAsmOutput(const IR::BFN::Pipe *pipe, const PhvInfo &phv,
                                 const ClotInfo &clot_info, gress_t gress)
    : phv(phv), clot_info(clot_info) {
    BUG_CHECK(pipe->thread[gress].parsers.size() != 0, "No parser?");
    for (auto parser : pipe->thread[gress].parsers) {
        auto lowered_parser = parser->to<IR::BFN::BaseLoweredParser>();
        BUG_CHECK(lowered_parser != nullptr, "Writing assembly for a non-lowered parser?");
        parsers.push_back(lowered_parser);
    }
}

std::ostream &operator<<(std::ostream &out, const ParserAsmOutput &parserOut) {
    ParserAsmSerializer serializer(out, parserOut.phv, parserOut.clot_info);
    for (auto p : parserOut.parsers) {
        LOG1("write asm for " << p);
        p->apply(serializer);
    }
    return out;
}
