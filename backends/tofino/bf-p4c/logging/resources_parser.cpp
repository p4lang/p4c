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

/* clang-format off */

#include "bf-p4c/device.h"
#include "resources_parser.h"

namespace BFN {

bool ParserResourcesLogging::preorder(const IR::BFN::LoweredParser *parser) {
    LOG1("Parser: " << parser->name);

    const auto gress = ::toString(parser->gress).c_str();
    const auto parserId = parser->portmap.size() > 0 ? parser->portmap[0] : 0;
    const auto nStates = Device::pardeSpec().numTcamRows();

    BUG_CHECK(parsers[parser->name].phase0 == nullptr,
            "phase0 for parser: %1% is already set unexpectedly!", parser->name);

    if (parser->gress == INGRESS) {
        if (parser->phase0) {
            const std::string usedBy = parser->phase0->tableName.c_str();
            const std::string usedFor = parser->phase0->actionName.c_str();
            parsers[parser->name].phase0 = new Phase0ResourceUsage(usedBy, usedFor);
        }
    }

    parsers[parser->name].usage =
        new ParserResourceUsage(gress, nStates, parserId, parsers[parser->name].phase0);

    return true;
}

bool ParserResourcesLogging::preorder(const IR::BFN::LoweredParserState *state) {
    auto parserIR = findContext<IR::BFN::LoweredParser>();
    BUG_CHECK(parserIR, "State does not belong to a parser? %1%", state);
    BUG_CHECK(parsers.count(parserIR->name) > 0, "Parser %1% not added to parsers map",
              parserIR->name);
    ParserLogData &p = parsers[parserIR->name];

    for (const auto *match : state->transitions) {
        LOG1("State Match: " << match);
        std::string nextStateName =
            (match->next ? match->next->name : (match->loop ? match->loop : "END"));
        nextStateName = stripThreadPrefix(nextStateName);
        auto states = logStateTransitionsByMatch(nextStateName, state, match);
        for (auto state : states) {
            p.usage->append(state);
        }
    }

    return true;
}

std::vector<ParserResourcesLogging::ParserStateTransition *>
ParserResourcesLogging::logStateTransitionsByMatch(const std::string &nextStateName,
                                                   const IR::BFN::LoweredParserState *prevState,
                                                   const IR::BFN::LoweredParserMatch *match) {
    auto tcamRow = getTcamId(match, prevState->gress);
    const auto shifts = match->shift;
    const auto hasCounter = !match->counters.empty();
    const auto nextStateId = getStateId(nextStateName);
    const auto prevStateId = getStateId(prevState->name.c_str());
    const auto prevStateName = prevState->name.c_str();

    std::vector<ParserResourcesLogging::ParserStateTransition *> result;

    auto addStateTransition = [&]() {
        ParserStateTransition *const parser_state_transition = new ParserStateTransition(
            hasCounter, nextStateId, nextStateName, prevStateId, shifts, tcamRow, prevStateName);

        CHECK_NULL(match);
        for (auto *stmt : match->extracts) {
          CHECK_NULL(stmt);

            if (auto *extract = stmt->to<IR::BFN::LoweredExtractClot>()) {
                if (extract->dest) {
                    parser_state_transition->append_clot_extracts(new ClotExtracts(
                            extract->source->to<IR::BFN::LoweredPacketRVal>()->range.lo,
                            extract->source->to<IR::BFN::LoweredPacketRVal>()->range.size(),
                        extract->dest->tag));
                }
            }
        }

        result.push_back(parser_state_transition);

        logStateExtracts(match, result.back()->get_extracts());
        logStateMatches(prevState, match, result.back()->get_matchesOn());
        logStateSaves(match, result.back()->get_savesTo());
    };

    // Add always present state
    addStateTransition();

    // Duplicate the node iff we are working with parser set values;
    //   this is important due to the right count of the used TCAM values.
    auto pvs = match->value->to<IR::BFN::ParserPvsMatchValue>();
    if (pvs) {
        const int duplicateCount = pvs->size - 1;
        for (int count = 0; count < duplicateCount; ++count) {
            --tcamRow;
            addStateTransition();
        }
    }

    return result;
}

/// Assign a tcam id for this match, higher the number, higher the priority.
int ParserResourcesLogging::getTcamId(const IR::BFN::LoweredParserMatch *match, gress_t gress) {
    auto *pvs = match->value->to<IR::BFN::ParserPvsMatchValue>();
    if (!tcamIds[gress].count(match)) {
        tcamIds[gress][match] = nextTcamId[gress];
        nextTcamId[gress] -= pvs ? pvs->size : 1;  // pvs uses multiple TCAM rows
    }

    return tcamIds[gress].at(match);
}

void ParserResourcesLogging::logStateExtracts(const IR::BFN::LoweredParserMatch *match,
                                              std::vector<StateExtracts *> &result) {
    std::map<size_t, int> extractorIds;
    for (const auto *prim : match->extracts) {
        auto extractIR = prim->to<IR::BFN::LoweredExtractPhv>();
        if (extractIR) {
            const size_t bitWidth = extractIR->dest->container.size();
            const auto &phvSpec = Device::phvSpec();
            const auto cid = phvSpec.containerToId(extractIR->dest->container);
            const auto destContainer = phvSpec.physicalAddress(cid, PhvSpec::MAU);
            const auto extractorId = extractorIds[bitWidth]++;
            auto buffer = extractIR->source->to<IR::BFN::LoweredInputBufferRVal>();
            auto constVal = extractIR->source->to<IR::BFN::LoweredConstantRVal>();

            BUG_CHECK(buffer || constVal, "Unknown extract primitive: %1%", prim);

            auto ex = new StateExtracts(bitWidth, destContainer, extractorId,
                buffer ? new int(buffer->range.loByte()) : nullptr,
                constVal ? new int(constVal->constant) : nullptr);
            result.push_back(ex);
        }
    }
}

void ParserResourcesLogging::logStateMatches(const IR::BFN::LoweredParserState *prevState,
                                             const IR::BFN::LoweredParserMatch *match,
                                             std::vector<StateMatchesOn *> &result) {
    if (!prevState) return;

    auto getConstVal = [](const IR::BFN::ParserConstMatchValue *val, unsigned bitw,
                          unsigned &shift) {
        std::stringstream v;
        auto w0 = val->value.word0 >> shift;
        auto w1 = val->value.word1 >> shift;
        auto mask = (1U << bitw) - 1;
        match_t m = {w0 & mask, w1 & mask};
        v << m;

        shift += bitw;
        return v.str();
    };

    unsigned shift = 0;
    auto &regs = prevState->select->regs;
    for (auto rItr = regs.rbegin(); rItr != regs.rend(); rItr++) {
        auto reg = *rItr;
        const auto bitWidth = reg.size * 8;
        const auto hardwareId = reg.id;
        const auto constVal = match->value->to<IR::BFN::ParserConstMatchValue>();
        const auto pvs = match->value->to<IR::BFN::ParserPvsMatchValue>();

        BUG_CHECK(constVal || pvs, "Unknown parser match value type: %1%", match->value);

        auto smo = new StateMatchesOn(bitWidth, hardwareId,
            constVal ? getConstVal(constVal, bitWidth, shift) : "",
            pvs ? pvs->name.c_str() : "");
        result.push_back(smo);
    }
}

void ParserResourcesLogging::logStateSaves(const IR::BFN::LoweredParserMatch *match,
                                           std::vector<StateSavesTo *> &result) {
    for (const auto &save : match->saves) {
        const auto hardwareId = save->dest.id;
        const auto buffer = save->source->to<IR::BFN::LoweredInputBufferRVal>();
        BUG_CHECK(buffer, "Unknown match save source : %1%", save);

        auto ss = new StateSavesTo(buffer->range.loByte(), hardwareId);
        result.push_back(ss);
    }
}

int ParserResourcesLogging::getStateId(const std::string &state) {
    if (!stateIds.count(state)) {
        int id = stateIds.size();
        stateIds[state] = id;
    }
    return stateIds.at(state);
}

const ParserResourcesLogging::ParserResources *ParserResourcesLogging::getLogger() {
    BUG_CHECK(collected, "Trying to get parser log without applying inspector to pipe node.");

    parserLogger = new ParserResources(Device::numParsersPerPipe());
    for (auto &kv : parsers) {
        parserLogger->append(kv.second.usage);
    }

    return parserLogger;
}

}  // namespace BFN

/* clang-format on */
