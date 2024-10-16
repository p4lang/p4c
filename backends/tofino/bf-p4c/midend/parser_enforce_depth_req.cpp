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

#include "parser_enforce_depth_req.h"

#include <boost/graph/copy.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/property_map/transform_value_property_map.hpp>

#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/device.h"
#include "bf-p4c/midend/parser_graph.h"
#include "bf-p4c/parde/parser_loops_info.h"

namespace BFN {

const cstring ParserEnforceDepthReq::pad_hdr_name = "min_parse_depth_padding"_cs;
const cstring ParserEnforceDepthReq::pad_hdr_type_name = "min_parse_depth_padding_t"_cs;
const cstring ParserEnforceDepthReq::pad_hdr_field = "packet_payload"_cs;
const cstring ParserEnforceDepthReq::pad_ctr_name = "min_parse_depth_counter"_cs;
const cstring ParserEnforceDepthReq::pad_state_name = "min_parse_depth"_cs;
const cstring ParserEnforceDepthReq::non_struct_pad_suf = "_padded"_cs;

/**
 * \brief Verify parse depth requirements and identify any "pad" required.
 *
 * This pass:
 *   - Verifies that maximum parse depths are not exceeded.
 *   - Identifies the "padding" (additional passing) required by each parser to meet minimum depth
 *     requirements.
 *
 * This is achieved by following these steps:
 *   1. Identify the size of each header.
 *   2. For each parser:
 *      a. Check if there are any minimum/maximum depth limits for the gress.
 *      b. If there are min/max limits:
 *         i. Calculate the amount processed by each state.
 *         ii. Calculate the minimum and maximum parse depths through the parse graph.
 *         iii. Report errors when parse depth exceeds maximum.
 *         iv. Record the amount of padding required when parse depth falls below the minimum.
 *
 * Padding is _not_ added by this pass.
 */
class IdentifyPadRequirements : public Inspector {
 private:
    // Used by P4ParserGraphs
    P4::ReferenceMap *refMap;

    /// Struct names
    std::set<cstring> &structs;

    /// Widths of all types.
    /// Variable-width fields should have been replaced by fixed-width fields before this pass runs.
    std::map<cstring, int> typeWidth;

    /// Size of each state in each parser
    std::map<const IR::P4Parser*, std::map<cstring, int>> &stateSize;

    /// Counters declared in the current parser. These are not necessarily used.
    std::set<cstring> counters;

    /// Have any uses of counters been detected for the current parser?
    bool counterInUse = false;

    /// Are any methods called by the parser? (i.e., does it do anything?)
    bool callsMethod = false;

    /// Accumulated size of the current state. Updated as extract and advance statements are
    /// processed.
    int currStateSize = 0;

    int minDepthAllowed = -1;
    int maxDepthAllowed = -1;

    /// Map of parsers and the pad requirements for each parser
    std::map<const IR::P4Parser *, ParserEnforceDepthReq::ParserPadReq> &padReq;

    /// Map of header names to the amount of padding required.
    /// The padding amount is in bytes for non-struct types and in states for struct types.
    std::map<cstring, int> &headerPadAmt;

    /// Map of all parsers and the gress each belongs to
    const std::map<const IR::P4Parser *, gress_t> &allParsers;

    /// Counter shift amount. Calculated based on port speed and clock rate.
    int ctrShiftAmt;

    bool preorder(const IR::P4Program *) override {
        bool hasPadReq = false;
        for (auto gress : {INGRESS, EGRESS}) {
            hasPadReq |= (Device::pardeSpec().minParseDepth(gress) > 0 &&
                          !BackendOptions().disable_parse_min_depth_limit) ||
                         (Device::pardeSpec().maxParseDepth(gress) < SIZE_MAX &&
                          !BackendOptions().disable_parse_max_depth_limit);
        }
        return hasPadReq;
    }

    bool preorder(const IR::P4Parser *prsr) override {
        for (auto &kv : allParsers) {
            if (prsr == kv.first) {
                if (Device::pardeSpec().minParseDepth(kv.second) > 0 ||
                    Device::pardeSpec().maxParseDepth(kv.second) < SIZE_MAX) {
                    LOG1("Verifying parse depth of parser " << prsr->name);
                    stateSize[prsr].clear();
                    counters.clear();
                    counterInUse = false;
                    callsMethod = false;
                    minDepthAllowed = Device::pardeSpec().minParseDepth(kv.second);
                    maxDepthAllowed = Device::pardeSpec().maxParseDepth(kv.second);
                    return true;
                }
            }
        }
        return false;
    }

    void calcPadReqs(cstring state, std::map<cstring, bool> &padStates,
                     const std::map<cstring, ordered_set<cstring>> &preds,
                     const std::map<cstring, int> &minDepthToState,
                     const std::map<cstring, int> &maxDepthToState) {
        if (preds.count(state)) {
            for (auto &pred : preds.at(state)) {
                // Parse graph may contain unreachable states (eliminated later)
                if (!minDepthToState.count(pred)) continue;
                int maxPad =
                    (minDepthAllowed - minDepthToState.at(pred) + ctrShiftAmt - 1) / ctrShiftAmt;
                int minPad =
                    (minDepthAllowed - maxDepthToState.at(pred) + ctrShiftAmt - 1) / ctrShiftAmt;
                if (maxPad > 0) {
                    minPad = minPad < 0 ? 0 : minPad;
                    LOG2("  Padding states required for " << pred << ": min=" << minPad
                                                          << " max=" << maxPad);
                    padStates[pred] = minPad > 0;
                }
            }
        }
    }

    void postorder(const IR::P4Parser *prsr) override {
        // Function visited only if preorder returned true

        // Apply P4ParserGraphs to get the predecessors/successors
        P4ParserGraphs pg(refMap, false);
        prsr->apply(pg);

        // Apply ParserLoopsInfo to get info about loops
        ParserPragmas parserPragmas;
        ParserLoopsInfo *parserLoopsInfo = nullptr;
        if (const auto* tnaParser = prsr->to<IR::BFN::TnaParser>()) {
            tnaParser->apply(parserPragmas);
            parserLoopsInfo = new ParserLoopsInfo(refMap, tnaParser, parserPragmas);
        }

        // Calculate minimum depth to each state
        std::map<cstring, int> minDepthToState;
        minDepthToState[IR::ParserState::start] = stateSize[prsr][IR::ParserState::start];
        std::set<cstring> statesToProcess = {IR::ParserState::start};
        while (statesToProcess.size() != 0) {
            std::set<cstring> newStatesToProcess;
            for (auto &state : statesToProcess) {
                for (auto &nextState : pg.succs[state]) {
                    int nextSize = minDepthToState[state] + stateSize[prsr][nextState];
                    if (!minDepthToState.count(nextState) ||
                        nextSize < minDepthToState.at(nextState)) {
                        minDepthToState[nextState] = nextSize;
                        newStatesToProcess.emplace(nextState);
                    }
                }
            }
            statesToProcess = newStatesToProcess;
        }

        int minDepth = minDepthToState[IR::ParserState::accept];
        if (minDepthToState.count(IR::ParserState::reject))
            if (minDepthToState[IR::ParserState::reject] < minDepth)
                minDepth = minDepthToState[IR::ParserState::reject];

        // Calculate maximum depth to each state (ignores loops)
        std::map<cstring, int> maxDepthToState;
        maxDepthToState[IR::ParserState::start] = stateSize[prsr][IR::ParserState::start];
        std::map<cstring, ordered_set<cstring>> pathToState;
        pathToState[IR::ParserState::start].emplace(IR::ParserState::start);
        statesToProcess.clear();  // Should already be clear :)
        statesToProcess.emplace(IR::ParserState::start);

        while (statesToProcess.size() != 0) {
            std::set<cstring> newStatesToProcess;
            for (auto &state : statesToProcess) {
                for (auto &nextState : pg.succs[state]) {
                    int loop_depth = 1;
                    if (parserLoopsInfo && parserLoopsInfo->max_loop_depth.count(nextState)) {
                        int max_loop_depth = parserLoopsInfo->max_loop_depth.at(nextState);
                        if (max_loop_depth > 0) loop_depth = max_loop_depth;
                    }
                    int nextSize = maxDepthToState[state] + loop_depth * stateSize[prsr][nextState];
                    if (!maxDepthToState.count(nextState) ||
                        (nextSize > maxDepthToState.at(nextState) &&
                         !pathToState[state].count(nextState))) {
                        maxDepthToState[nextState] = nextSize;
                        pathToState[nextState] = pathToState[state];
                        pathToState[nextState].emplace(nextState);
                        newStatesToProcess.emplace(nextState);
                    }
                }
            }
            statesToProcess = newStatesToProcess;
        }

        int maxDepth = maxDepthToState[IR::ParserState::accept];
        if (maxDepthToState.count(IR::ParserState::reject))
            if (maxDepthToState[IR::ParserState::reject] > maxDepth)
                maxDepth = maxDepthToState[IR::ParserState::reject];

        LOG2("Parser " << prsr->externalName() << ": min parse depth=" << minDepth
                       << "   max parse depth=" << maxDepth);

        // Don't pad parsers where nothing is done: the parser will be bypassed via cut-through
        if (!callsMethod) {
            LOG1("Not padding parser " << prsr->externalName() << ": parser doesn't do anything");
        } else if (maxDepth > maxDepthAllowed && !BackendOptions().disable_parse_max_depth_limit) {
            error(
                "Parser %1%: longest path through parser (%2%B) exceeds maximum parse depth (%3%B)",
                prsr->externalName(), maxDepth, maxDepthAllowed);
        } else if (minDepth < minDepthAllowed) {
            LOG1("Padding parser " << prsr->externalName() << " to meet minimum depth requirements."
                                   << " Shortest path: " << minDepth
                                   << "; minimum required depth: " << minDepthAllowed);

            auto *tp = prsr->type->to<IR::Type_Parser>();
            BUG_CHECK(tp, "Type_Parser not found in parser %s", prsr->externalName());
            BUG_CHECK(tp->getApplyParameters() && tp->getApplyParameters()->parameters.size() > 2,
                      "Couldn't find header paremeter for parser %s", prsr->externalName());
            auto *hdr_param = tp->getApplyParameters()->parameters[1];
            auto *tn = hdr_param->type->to<IR::Type_Name>();
            BUG_CHECK(tn, "Couldn't convert type to Type_Struct");
            cstring header = tn->path->name;
            bool hdrTypeIsStruct = !typeWidth.count(header);

            // Verify that we're not already using the counter
            if (counterInUse && hdrTypeIsStruct)
                error(
                    "Can't enforce minimum parse depth for parser %1% because parser "
                    "counter is already used. You must manually enforce minimum (%2%B) / maximum "
                    "(%3%B) parse depths in the parse graph.",
                    prsr->externalName(), minDepthAllowed, maxDepthAllowed);

            // Calculate the padding requirements.
            // If the parser header type is a:
            //   - struct: this value indicates the number of pad states.
            //   - header: this value represents the number of bytes to pad.
            int padStates = hdrTypeIsStruct
                                ? (minDepthAllowed - minDepth + ctrShiftAmt - 1) / ctrShiftAmt
                                : minDepthAllowed - minDepth;

            if (minDepthToState.count(IR::ParserState::accept))
                calcPadReqs(IR::ParserState::accept, padReq[prsr].padStatesAccept, pg.preds,
                            minDepthToState, maxDepthToState);
            if (minDepthToState.count(IR::ParserState::reject))
                calcPadReqs(IR::ParserState::reject, padReq[prsr].padStatesReject, pg.preds,
                            minDepthToState, maxDepthToState);
            padReq[prsr].maxPadStates = padStates;

            headerPadAmt[header] = std::max(padStates, headerPadAmt[header]);
            if (hdrTypeIsStruct)
                LOG2("  Header " << header << " needs " << headerPadAmt[header]
                                 << " parse states");
            else
                LOG2("  Header " << header << " needs " << headerPadAmt[header] << " bytes pad");
        }
    }

    bool preorder(const IR::Declaration_Instance *di) override {
        // Look for ParserCounter declarations
        if (auto *tn = di->type->to<IR::Type_Name>()) {
            if (tn->path->name == "ParserCounter") {
                LOG2("Found ParserCounter: " << di->name);
                counters.emplace(di->name);
            }
        }
        return true;
    }

    bool preorder(const IR::ParserState *state) override {
        if (state->selectExpression) {
            // Visit the children to look for counter accesses (is_zero/is_negative)
            // The children are not visited by default
            visit(state->selectExpression);
        }
        currStateSize = 0;
        return true;
    }

    void postorder(const IR::ParserState *state) override {
        stateSize[findContext<IR::P4Parser>()][state->name] = currStateSize / 8;
        LOG3("State " << state->externalName() << " size: " << (currStateSize / 8) << "B");
    }

    bool preorder(const IR::Type_Header *th) override {
        typeWidth[th->name] = th->width_bits();
        return true;
    }

    bool preorder(const IR::Type_Struct *ts) override {
        structs.insert(ts->name);
        return true;
    }

    bool preorder(const IR::MethodCallExpression *mce) override {
        auto *state = findContext<IR::ParserState>();
        if (!state) return false;

        callsMethod = true;

        // Identify the following methods:
        //   - extract     (header; called from state)
        //   - advance     (bits; called from state)
        //   - set         (counter; called from state)
        //   - decrement   (counter; called from state)
        //   - is_negative (counter; in select in state)
        //   - is_zero     (counter; in select in state)
        if (auto *member = mce->method->to<IR::Member>()) {
            if (member->member == "extract") {
                auto *type = mce->typeArguments->front();
                if (auto *typeName = type->to<IR::Type_Name>()) {
                    auto name = typeName->path->name;
                    currStateSize += typeWidth[name];
                    LOG3("Extract of " << name << " in state " << state->externalName() << ": "
                                       << typeWidth[name] << "b");
                } else if (auto *typeHeader = type->to<IR::Type_Header>()) {
                    auto name = typeHeader->name;
                    typeWidth[name] = typeHeader->width_bits();
                    currStateSize += typeHeader->width_bits();
                    LOG3("Extract of " << name << " in state " << state->externalName() << ": "
                                       << typeWidth[name] << "b");
                }
            } else if (member->member == "advance") {
                auto *arg = mce->arguments->front();
                BUG_CHECK(arg, "\"advance\" call missing argument in %1%", mce);
                if (auto *c = arg->expression->to<IR::Constant>()) {
                    LOG3("Advance in state " << state->externalName() << ": " << c->asInt() << "b");
                    currStateSize += c->asInt();
                } else {
                    error(
                        "Variable-length advance statement found in state %1% of parser %2%. "
                        "Please manually enforce minimum (%3%B) / maximum (%4%B) parse depths in "
                        "the parse graph.",
                        state->externalName(), findContext<IR::P4Parser>()->externalName(),
                        minDepthAllowed, maxDepthAllowed);
                }
            } else if (member->member == "set" || member->member == "decrement" ||
                       member->member == "is_negative" || member->member == "is_zero") {
                auto &op = member->member;
                auto *pe = member->expr->to<IR::PathExpression>();
                BUG_CHECK(pe, "Excpecting a PathExpression in set statement: %1%", mce);
                if (counters.count(pe->path->name)) {
                    counterInUse = true;
                    LOG3("Counter use in state " << state->externalName() << " via " << op);
                }
            }
        }
        return false;
    }

 public:
    IdentifyPadRequirements(
        P4::ReferenceMap *refMap, std::set<cstring> &structs,
        std::map<const IR::P4Parser *, std::map<cstring, int>> &stateSize,
        std::map<const IR::P4Parser *, ParserEnforceDepthReq::ParserPadReq> &padReq,
        std::map<cstring, int> &headerPadAmt,
        const std::map<const IR::P4Parser *, gress_t> &allParsers, int ctrShiftAmt)
        : refMap(refMap),
          structs(structs),
          stateSize(stateSize),
          padReq(padReq),
          headerPadAmt(headerPadAmt),
          allParsers(allParsers),
          ctrShiftAmt(ctrShiftAmt) {}
};

/**
 * \brief Add "padding" (additional states/extract fields) to parsers to meet minimum parse depth
 * requirements.
 *
 * For parsers with struct headers, padding uses a counter to count the bytes
 * remaining to meet the minimum depth requirement. For non-struct headers, padding adds additional
 * bytes to the header to meet the depth requirement.
 *
 * This pass does not calculate padding requirements. Instead, these are calculated by
 * IdentifyPadRequirements.
 *
 * For programs that don't meet the minimum parse depth requirement, this pass:
 *   - Non-struct headers (i.e., a P4 "header" is passed into the parser):
 *     - Creates a clone of the header with an additional field to meet the minimum parse depth.
 *     - Replaces references to the original header with the padded header when used in a pipe
 *       requiring parser min-depth padding.
 *   - Struct headers (i.e., a P4 "struct" is passed into the parser):
 *     - Adds a blob header to store data parsed in padding states.
 *     - Adds a stack of blob headers to each header struct to meet parse depth requirements.
 *     - For each parser that doesn't meet parse depth requirements:
 *       - Adds a counter.
 *       - Initializes the counter in the start state to the parse depth minus the number of bytes
 *         parsed in the state.
 *       - Decrements the counter in each state by the nubmer of bytes parsed.
 *       - Adds padding states that read data and decrement the counter until the counter is <= 0.
 *       - Replaces accept/reject transitions to transitions to the new padding states.
 *       - Adds emit statements to the deparser for the blob header stack.
 */
class AddParserPad : public Modifier {
 private:
    /// Struct names
    const std::set<cstring> &structs;

    /// Size of all states
    const std::map<const IR::P4Parser*, std::map<cstring, int>> &stateSize;

    /// Map of parsers and the pad requirements for each parser
    const std::map<const IR::P4Parser *, ParserEnforceDepthReq::ParserPadReq> &padReq;

    /// Map of header and the number of pad states required
    const std::map<cstring, int> &headerPadAmt;

    /// Map of all parsers and the gress each belongs to
    const std::map<const IR::P4Parser *, gress_t> &allParsers;

    /// Map of all deparsers and the gress each belongs to
    const std::map<const IR::P4Control *, gress_t> &allDeparsers;

    /// Map of all MAU pipes and the gress each belongs to
    const std::map<const IR::P4Control *, gress_t> &allMauPipes;

    /// Map of deparsers to parsers
    const std::map<const IR::P4Control *, std::set<const IR::P4Parser *>> &deparserParser;

    /// Map of MAU pipes to parsers
    const std::map<const IR::P4Control *, std::set<const IR::P4Parser *>> &mauPipeParser;

    /// Counter shift amount. Calculated based on port speed and clock rate.
    int ctrShiftAmt = -1;

    /// Map from header struct name to pad instance name
    std::map<cstring, cstring> headerToPadName;
    std::map<cstring, const IR::Path *> hdrNameToNewPath;
    std::map<cstring, const IR::Type_Header *> hdrNameToNewTypeHdr;
    std::set<const IR::P4Parser *> prsrNoStruct;

    /// Insert header valid initialization after this assignment statement.
    /// A nullptr indicates that initialization shouldn't occur in this state.
    const IR::AssignmentStatement *insertHdrVldInitAfter = nullptr;

    /// Set of all parser names to pad
    std::set<cstring> padPrsrNames;

    std::map<cstring, std::set<int>> pipeReplacements;

    std::map<cstring, const IR::Type_Parser *> replacedTPrsr;
    std::map<cstring, const IR::Type_Control *> replacedTCtrl;

    /// Minimum depth allowed in the current gress
    int minDepthAllowed = -1;

    // Information about the current parser
    bool prsrHasPadStack = false;
    cstring prsrPktName;
    cstring prsrHdrName;
    cstring prsrHdrType;
    cstring prsrPadInstName;
    int prsrNumPadStates = -1;
    cstring prsrCtrName;

    // Map of destination state (accept/reject) to preceding pad state names
    std::map<cstring, cstring> prsrInitialCheckState;
    std::map<cstring, cstring> prsrCheckLoopState;

    // Type information gathered while walking the IR
    const IR::Type_Extern *pktInExtern = nullptr;
    const IR::Type_Extern *pktOutExtern = nullptr;
    const IR::Type_Extern *prsrCtrExtern = nullptr;
    const IR::Type_Method *prsrCtrSetMethod = nullptr;
    const IR::Type_Method *prsrCtrDecMethod = nullptr;
    const IR::Type_Method *prsrExtractMethod = nullptr;
    const IR::Type_Method *dprsrEmitMethod = nullptr;

    // New IR nodes created by this pass
    IR::StructField *padField = nullptr;
    IR::Type_Header *padHdr = nullptr;
    std::map<cstring, IR::Type_Stack *> padHdrStack;
    std::map<cstring, IR::StructField *> padHdrStackField;
    std::map<cstring, const IR::Type_Struct *> hdrStruct;
    std::vector<const IR::Type_Struct *> addedHdrStruct;

    // Tofino1-like architectures
    std::set<cstring> tofArch = {
        "tna"_cs,
#if HAVE_JBAY
        "t2na"_cs,
#endif /* HAVE_JBAY */
    };

    /**
     * Create Type_Header, Type_Stack, and StructField IR objects to be inserted during
     * traversal
     */
    void createHdrStacks() {
        // Do we have any structs, or is everything a header?
        auto prsrHeaders = Keys(headerPadAmt);
        if (!std::any_of(prsrHeaders.begin(), prsrHeaders.end(),
                         [&](const cstring &hdr) { return structs.count(hdr); })) {
            LOG6("Do not need to create pad header; can add field to existing header(s)");
            return;
        }

        // Pad header creation
        padField = new IR::StructField(ParserEnforceDepthReq::pad_hdr_field,
                                       IR::Type_Bits::get(ctrShiftAmt * 8));
        padHdr = new IR::Type_Header(ParserEnforceDepthReq::pad_hdr_type_name, {padField});
        LOG5("Created header: " << padHdr);

        // Header stacks (different for each header struct based on depth)
        int stackId = 0;
        for (auto &kv : headerPadAmt) {
            auto hdr_name = kv.first;
            int num_states = kv.second;
            if (!structs.count(hdr_name)) {
                LOG6(hdr_name << " is not a struct; not creating pad header stack");
                continue;
            }

            auto *stack =
                new IR::Type_Stack(new IR::Type_Name(ParserEnforceDepthReq::pad_hdr_type_name),
                                   new IR::Constant(num_states));
            std::string field_name =
                ParserEnforceDepthReq::pad_hdr_name + "_" + std::to_string(stackId++);
            auto *field = new IR::StructField(field_name.c_str(), stack);
            headerToPadName.emplace(hdr_name, field_name);
            padHdrStack.emplace(hdr_name, stack);
            padHdrStackField.emplace(hdr_name, field);
            LOG5("Created header stack: " << field);
        }
    }

    /** Identify non-struct parsers that require padding.
     *
     * Create the IR::ID for the padded version of the header.
     */
    void identifyNonStructParsers() {
        for (const auto *prsr : Keys(padReq)) {
            // Gather information from the parser parameters
            auto *tp = prsr->type;
            BUG_CHECK(tp->applyParams->parameters.size() > 2,
                      "Missing header information in parser %1%", prsr->externalName());

            auto *hdrType = tp->applyParams->parameters[1]->type->to<IR::Type_Name>();
            BUG_CHECK(hdrType, "Missing header type information in parser %1%",
                      prsr->externalName());
            auto hdrTypeName = hdrType->path->name;

            // Vefify that the header does not correspond to a struct
            if (!structs.count(hdrTypeName)) {
                auto newID = IR::ID(hdrTypeName + ParserEnforceDepthReq::non_struct_pad_suf);
                hdrNameToNewPath.emplace(hdrTypeName, new IR::Path(newID));
                hdrNameToNewTypeHdr[hdrTypeName] = nullptr;
                prsrNoStruct.insert(prsr);
            }
        }
    }

    profile_t init_apply(const IR::Node *root) override {
        // Generate the state names
        for (auto &dst : {IR::ParserState::accept, IR::ParserState::reject}) {
            prsrInitialCheckState[dst] =
                cstring(std::string(ParserEnforceDepthReq::pad_state_name + "_" + dst + "_initial")
                            .c_str());
            prsrCheckLoopState[dst] = cstring(
                std::string(ParserEnforceDepthReq::pad_state_name + "_" + dst + "_loop").c_str());
        }

        // Record the names of all parsers requiring padding
        for (const auto *prsr : Keys(padReq)) padPrsrNames.insert(prsr->name);

        createHdrStacks();
        identifyNonStructParsers();

        return Modifier::init_apply(root);
    }

    bool preorder(IR::P4Program *) override {
        bool hasPadReq = false;
        for (auto gress : {INGRESS, EGRESS}) {
            // Only check min depth here. Max depth is handled in IdentifyPadRequirements
            hasPadReq |= Device::pardeSpec().minParseDepth(gress) > 0 &&
                         !BackendOptions().disable_parse_min_depth_limit;
        }
        if (hasPadReq && BackendOptions().arch == "psa")
            error(
                "Can't enforce minimum parse depth for PSA programs. You must manually enforce the "
                "minimum parse depth in parse graphs.");
        return hasPadReq;
    }

    void postorder(IR::P4Program *prog) override {
        for (auto *th : Values(hdrNameToNewTypeHdr)) {
            prog->objects.insert(prog->objects.begin(), th);
            LOG5("Added header type " << th->name << " to program");
        }
        if (padHdr) {
            prog->objects.insert(prog->objects.begin(), padHdr);
            LOG5("Added header " << padHdr->name << " to program");
        }
        return;
    }

    bool preorder(IR::Type_Header *th) override {
        // headerPadMap is a map of headers used as parser parameters. If a Type_Header appears in
        // this list, then it needs to have an additional field added to meet min depth
        // requirements.
        if (headerPadAmt.count(th->name)) {
            int padAmt = headerPadAmt.at(th->name);

            auto *new_th = th->clone();
            new_th->name = IR::ID(th->name + ParserEnforceDepthReq::non_struct_pad_suf);
            auto *padField = new IR::StructField(ParserEnforceDepthReq::pad_hdr_field,
                                        IR::Type_Bits::get(padAmt * 8));
            new_th->fields.push_back(padField);

            hdrNameToNewTypeHdr[th->name] = new_th;

            LOG5("Added " << padField->name << " to header " << new_th->name);
        }
        return false;
    }

    void postorder(IR::Type_Struct *ts) override {
        if (padHdrStackField.count(ts->name)) {
            hdrStruct[ts->name] = ts;
            ts->fields.push_back(padHdrStackField.at(ts->name));
            LOG5("Added stack " << padHdrStackField.at(ts->name)->name << " to "
                                << ts->externalName());
        }
    }

    bool preorder(IR::Type_Extern *te) override {
        // Record the extern types that we need when inserting new IR nodes
        if (te->name == "ParserCounter") {
            prsrCtrExtern = getOriginal<IR::Type_Extern>();
            // Identify P4-14 ParserCounter: it has a type parameter
            if (te->typeParameters && te->typeParameters->size())
                error(
                    "Can't enforce minimum parse depth for programs with \"extern "
                    "ParserCounter<W>\". You must manually enforce the minimum parse depth.");
        } else if (te->name == "packet_in") {
            pktInExtern = getOriginal<IR::Type_Extern>();
        } else if (te->name == "packet_out") {
            pktOutExtern = getOriginal<IR::Type_Extern>();
        }
        return true;
    }

    bool preorder(IR::Type_Method *tm) override {
        // Record the methods we need when inserting new IR nodes
        if (tm->name == "set" && tm->parameters->parameters.size() == 1) {
            if (auto *te = findContext<IR::Type_Extern>()) {
                if (te->name == "ParserCounter") prsrCtrSetMethod = getOriginal<IR::Type_Method>();
            }
        } else if (tm->name == "decrement" && tm->parameters->parameters.size() == 1) {
            if (auto *te = findContext<IR::Type_Extern>()) {
                if (te->name == "ParserCounter") prsrCtrDecMethod = getOriginal<IR::Type_Method>();
            }
        } else if (tm->name == "emit" && tm->parameters->parameters.size() == 1) {
            if (auto *te = findContext<IR::Type_Extern>()) {
                if (te->name == "packet_out") dprsrEmitMethod = getOriginal<IR::Type_Method>();
            }
        } else if (tm->name == "extract" && tm->parameters->parameters.size() == 1) {
            if (auto *te = findContext<IR::Type_Extern>()) {
                if (te->name == "packet_in") prsrExtractMethod = getOriginal<IR::Type_Method>();
            }
        }
        return false;
    }

    bool preorder(IR::P4Parser *prsr) override {
        static int ctr_id = 0;

        // Verify whether we need to add padding to this parser
        const IR::P4Parser *orig = getOriginal<IR::P4Parser>();
        if (!padReq.count(orig)) return false;

        // Identify the minimum parse depth
        // Max is verified during the check in IdentifyPadRequirements
        minDepthAllowed = Device::pardeSpec().minParseDepth(allParsers.at(orig));

        // Gather information from the parser parameters
        auto *tp = prsr->type;
        BUG_CHECK(tp->applyParams->parameters.size() > 2,
                  "Missing header information in parser %1%", prsr->externalName());

        prsrPktName = tp->applyParams->parameters[0]->name;
        prsrHdrName = tp->applyParams->parameters[1]->name;
        auto hdrType = tp->applyParams->parameters[1]->type->to<IR::Type_Name>();
        BUG_CHECK(hdrType, "Missing header type information in parser %1%", prsr->externalName());
        prsrHdrType = hdrType->path->name;

        if (structs.count(prsrHdrType)) {
            prsrNumPadStates = padReq.at(orig).maxPadStates;
            prsrPadInstName = headerToPadName.at(prsrHdrType);
            prsrHasPadStack = true;

            // Add the counter that we use to parse to the required depth
            std::string ctr_name = ParserEnforceDepthReq::pad_ctr_name + "_" +
                                   std::to_string(ctr_id++) + "/" +
                                   ParserEnforceDepthReq::pad_ctr_name;
            prsrCtrName = cstring(ctr_name.c_str());
            // Different Declaration_Instances based on the ParserCounter extern
            IR::Declaration_Instance *di;
            if (prsrCtrExtern->typeParameters && prsrCtrExtern->typeParameters->size()) {
                di = new IR::Declaration_Instance(
                    prsrCtrName,
                    new IR::Type_Specialized(new IR::Type_Name("ParserCounter"),
                                            new IR::Vector<IR::Type>({IR::Type_Bits::get(8)})),
                    new IR::Vector<IR::Argument>());

            } else {
                di = new IR::Declaration_Instance(prsrCtrName, new IR::Type_Name("ParserCounter"),
                                                new IR::Vector<IR::Argument>());
            }
            prsr->parserLocals.push_back(di);
            LOG5("Added counter to parser " << prsr->externalName() << ": " << di);
        } else {
            prsrNumPadStates = 0;
            prsrPadInstName = ""_cs;
            prsrHasPadStack = false;
            replacedTPrsr[prsr->name] = nullptr;
        }

        return true;
    }

    const IR::P4Parser *findParser() {
        auto *prsr = findOrigCtxt<IR::P4Parser>();
        if (!prsr) {
            if (auto *ctrl = findOrigCtxt<IR::P4Control>()) {
                if (deparserParser.count(ctrl))
                    prsr = *deparserParser.at(ctrl).begin();
                else if (mauPipeParser.count(ctrl))
                    prsr = *mauPipeParser.at(ctrl).begin();
            }
        }
        return prsr;
    }

    bool preorder(IR::Type_Name *tn) override {
        // Replace the path in a Type_Name when it corresponds to a non-struct header used in a
        // parser that is being padded
        auto *prsr = findParser();
        if (prsr && prsrNoStruct.count(prsr)) {
            if (hdrNameToNewPath.count(tn->path->name)) {
                tn->path = hdrNameToNewPath.at(tn->path->name);
            }
        } else if (const auto *ts = findContext<IR::Type_Specialized>()) {
            if (const auto *di = findContext<IR::Declaration_Instance>()) {
                int child_index = getContext()->child_index;
                if (ts->baseType->path->name == "Pipeline") {
                    cstring pipeName = di->name;
                    if (pipeReplacements[pipeName].count(child_index)) {
                        tn->path = hdrNameToNewPath.at(tn->path->name);
                    }
                } else if (ts->baseType->path->name == "Switch") {
                    if (tofArch.count(BackendOptions().arch)) {
                        unsigned pipeParamCnt = 4;
                        unsigned pipeIdx = child_index / pipeParamCnt;
                        unsigned paramIdx = child_index % pipeParamCnt;
                        if (pipeIdx < di->arguments->size()) {
                            auto *pipe =
                                di->arguments->at(pipeIdx)->expression->to<IR::PathExpression>();
                            cstring pipeName = (pipe ? cstring(pipe->path->name) : ""_cs);
                            if (pipe && pipeReplacements.count(pipeName)) {
                                if (pipeReplacements.at(pipeName).count(paramIdx)) {
                                    tn->path = hdrNameToNewPath.at(tn->path->name);
                                }
                            }
                        }
                    }
                }
            }
        }
        return false;
    }

    bool preorder(IR::PathExpression *pe) override {
        auto *prsr = findParser();
        if (prsr && prsrNoStruct.count(prsr)) {
            auto *th = pe->type->to<IR::Type_Header>();
            if (th && hdrNameToNewTypeHdr.count(th->name)) {
                pe->type = hdrNameToNewTypeHdr.at(th->name);
            return false;
            }
        }
        return true;
    }

    void postorder(IR::Type_Parser *tp) override {
        if (replacedTPrsr.count(tp->name)) replacedTPrsr[tp->name] = tp;
    }

    void postorder(IR::Type_Control *tc) override {
        if (replacedTCtrl.count(tc->name)) replacedTCtrl[tc->name] = tc;
    }

    bool preorder(IR::ConstructorCallExpression *cce) override {
        if (auto *tp = cce->type->to<IR::Type_Parser>()) {
            if (replacedTPrsr.count(tp->name) && replacedTPrsr.at(tp->name)) {
                cce->type = replacedTPrsr.at(tp->name);
            }
        } else if (auto *tc = cce->type->to<IR::Type_Control>()) {
            if (replacedTCtrl.count(tc->name) && replacedTCtrl.at(tc->name)) {
                cce->type = replacedTCtrl.at(tc->name);
            }
        }
        return false;
    }

    bool preorder(IR::Type_Specialized *ts) override {
        if (prsrNoStruct.size() == 0) return true;

        // Identify the arguments to check in the Pipeline object
        // The first element in the pair is the argument number in the Declaration_Instance. The
        // second element is the argument to check in the Type_Specialized.
        std::vector<std::pair<int, int>> argsToCheck;
        if (tofArch.count(BackendOptions().arch))
            argsToCheck = {std::make_pair(0, 0), std::make_pair(3, 2)};
        else
            error("Unsupported architecture \"%1%\" for parser minimum depth enforcement",
                    BackendOptions().arch);

        if (auto *di = getParent<IR::Declaration_Instance>()) {
            if (ts->baseType->path->name == "Pipeline") {
                cstring pipeName = di->name;

                for (auto &p : argsToCheck) {
                    const auto *prsrArg = di->arguments->at(p.first);
                    if (const auto *cce =
                            prsrArg->expression->to<IR::ConstructorCallExpression>()) {
                        const auto *prsr_tn = cce->constructedType->to<IR::Type_Name>();
                        BUG_CHECK(prsr_tn, "Did not find a Type_Name as expected");
                        cstring prsrName = prsr_tn->path->name;
                        if (padPrsrNames.count(prsrName)) {
                            auto *tn = ts->arguments->at(p.second)->to<IR::Type_Name>();
                            if (tn && !structs.count(tn->path->name)) {
                                LOG5("Marking argument " << p.second << " for replacement in pipe "
                                                         << pipeName);
                                pipeReplacements[pipeName].insert(p.second);
                            }
                        }
                    }
                }
            }
        }

        return true;
    }

    /**
     * Add states for parser pad/minimum depth processing
     *
     * We want read additional data until the counter <= 0. Because actions are associated with
     * states and not transitions, we can't have a state that extracts when counter > 0 and ends
     * when counter <= 0. Instead, we have an initial state that checks the counter, followed by a
     * loop state that extracts and decrements the counter.
     *   - initial state:
     *     - ctr > 0: go to loop state
     *     - ctr <= 0: go to accept/reject state
     *   - loop state:
     *     - extract; decrement ctr;
     *       - ctr > 0: go to loop state
     *       - ctr <= 0: go to accept/reject state
     */
    void addPadStates(IR::P4Parser *prsr, cstring dst, int loopStates) {
        // Counter negative/zero method calls
        auto *isNegative =
            new IR::Cast(IR::Type_Bits::get(1),
                         new IR::MethodCallExpression(
                             IR::Type_Boolean::get(),
                             new IR::Member(new IR::PathExpression(prsrCtrName), "is_negative"),
                             new IR::Vector<IR::Type>(), new IR::Vector<IR::Argument>));

        auto *isZero =
            new IR::Cast(IR::Type_Bits::get(1),
                         new IR::MethodCallExpression(
                             IR::Type_Boolean::get(),
                             new IR::Member(new IR::PathExpression(prsrCtrName), "is_zero"),
                             new IR::Vector<IR::Type>(), new IR::Vector<IR::Argument>));

        // Negative/zero select expression
        auto *scZeroZero = new IR::SelectCase(new IR::ListExpression(IR::Vector<IR::Expression>(
                                                  {new IR::Constant(IR::Type_Bits::get(1), 0),
                                                   new IR::Constant(IR::Type_Bits::get(1), 0)})),
                                              new IR::PathExpression(prsrCheckLoopState[dst]));
        auto *scWildcard = new IR::SelectCase(
            new IR::ListExpression(
                IR::Vector<IR::Expression>({new IR::DefaultExpression(IR::Type_Dontcare::get()),
                                            new IR::DefaultExpression(IR::Type_Dontcare::get())})),
            new IR::PathExpression(dst));
        auto *se = new IR::SelectExpression(
            new IR::ListExpression(IR::Vector<IR::Expression>({isNegative, isZero})),
            IR::Vector<IR::SelectCase>({scZeroZero, scWildcard}));

        // Possible select expression that does not loop back to _loop state again
        // in case the stack only has 1 item which would create a data re-assignment
        // Also when the stack has only 1 item we do not need to loop at all
        auto *se_no_loop = new IR::SelectExpression(
            new IR::ListExpression(IR::Vector<IR::Expression>({isNegative, isZero})),
            IR::Vector<IR::SelectCase>({scWildcard}));

        // Operation: decrement counter
        auto *decArgs = new IR::Vector<IR::Argument>(
            {new IR::Argument(new IR::Constant(IR::Type_Bits::get(8), ctrShiftAmt))});
        auto *decOp = new IR::MethodCallStatement(new IR::MethodCallExpression(
            new IR::Member(new IR::PathExpression(prsrCtrName), "decrement"),
            new IR::Vector<IR::Type>(), decArgs));

        // Operation: extract to stack
        auto *extArgs = new IR::Vector<IR::Argument>({new IR::Argument(new IR::Member(
            padHdr,
            new IR::Member(
                padHdrStack[prsrHdrType],
                new IR::PathExpression(hdrStruct[prsrHdrType], new IR::Path(prsrHdrName)),
                prsrPadInstName),
            "next"))});
        auto *extOp = new IR::MethodCallStatement(new IR::MethodCallExpression(
            IR::Type_Void::get(),
            new IR::Member(prsrExtractMethod,
                           new IR::PathExpression(pktInExtern, new IR::Path(prsrPktName)),
                           "extract"),
            new IR::Vector<IR::Type>({new IR::Type_Name(ParserEnforceDepthReq::pad_hdr_type_name)}),
            extArgs));

        // Initial state that decrements the counter but does not extract
        auto *stateInitial = new IR::ParserState(prsrInitialCheckState[dst],
                                                 IR::IndexedVector<IR::StatOrDecl>(), se);

        // Loop state that decrements and extracts (accept only)
        // We only need to loop when we need more than 1 state extracts
        auto *stateLoop = new IR::ParserState(
            prsrCheckLoopState[dst],
            new IR::Annotations(
                IR::Vector<IR::Annotation>({new IR::Annotation("max_loop_depth", loopStates)})),
            IR::IndexedVector<IR::StatOrDecl>({decOp}), (prsrNumPadStates > 1) ? se : se_no_loop);
        if (dst == IR::ParserState::accept) stateLoop->components.push_back(extOp);

        prsr->states.push_back(stateInitial);
        prsr->states.push_back(stateLoop);

        IndentCtl::TempIndent indent;
        LOG5("Added states to parser " << prsr->externalName() << ":" << indent);
        LOG5(stateInitial);
        LOG5(stateLoop);
    }

    void postorder(IR::P4Parser *prsr) override {
        const IR::P4Parser *orig = getOriginal<IR::P4Parser>();
        BUG_CHECK(padReq.count(orig), "Pad requirements not found for parser %1%",
                  prsr->externalName());

        if (prsrHasPadStack) {
            auto &prsrPadReq = padReq.at(orig);
            if (prsrPadReq.padStatesAccept.size())
                addPadStates(prsr, IR::ParserState::accept, padReq.at(orig).maxPadStates);
            if (prsrPadReq.padStatesReject.size())
                addPadStates(prsr, IR::ParserState::reject, padReq.at(orig).maxPadStates);
        }
    }

    bool preorder(IR::P4Control *ctrl) override {
        auto *orig = getOriginal<IR::P4Control>();

        // Identify the parser corresponding to this block
        const IR::P4Parser *prsr = nullptr;
        if (mauPipeParser.count(orig)) prsr = *mauPipeParser.at(orig).begin();
        if (deparserParser.count(orig)) prsr = *deparserParser.at(orig).begin();

        if (!prsrNoStruct.count(prsr)) return true;

        replacedTCtrl[ctrl->name] = nullptr;
        return true;
    }

    void postorder(IR::BlockStatement *bs) override {
        // Add emit statements to the deparser for the pad blob fields
        if (auto *ctrl = findOrigCtxt<IR::P4Control>()) {
            // Don't process if statements
            if (findOrigCtxt<IR::IfStatement>()) return;

            if (deparserParser.count(ctrl) && padReq.count(*deparserParser.at(ctrl).begin())) {
                auto *prsr = *deparserParser.at(ctrl).begin();
                auto *tc = ctrl->type->to<IR::Type_Control>();
                BUG_CHECK(tc, "Missing Type_Control in deparser %1%", ctrl->externalName());
                BUG_CHECK(tc->applyParams->parameters.size() > 2,
                          "Missing header information in deparser %1%", ctrl->externalName());

                auto pktName = tc->applyParams->parameters[0]->name;
                auto hdrName = tc->applyParams->parameters[1]->name;
                auto hdrType = tc->applyParams->parameters[1]->type->to<IR::Type_Name>();
                BUG_CHECK(hdrType, "Missing header type information in deparser %1%",
                          ctrl->externalName());
                auto hdrTypeName = hdrType->path->name;

                if (structs.count(prsrHdrType)) {
                    auto padName = headerToPadName.at(hdrTypeName);
                    int numPadStates = padReq.at(prsr).maxPadStates;

                    for (int i = 0; i < numPadStates; ++i) {
                        auto *args =
                            new IR::Vector<IR::Argument>({new IR::Argument(new IR::ArrayIndex(
                                padHdr,
                                new IR::Member(padHdrStack[prsrHdrType],
                                               new IR::PathExpression(hdrStruct[prsrHdrType],
                                                                      new IR::Path(hdrName)),
                                               padName),
                                new IR::Constant(i)))});

                        auto *emit = new IR::MethodCallStatement(new IR::MethodCallExpression(
                            IR::Type_Void::get(),
                            new IR::Member(
                                dprsrEmitMethod,
                                new IR::PathExpression(pktOutExtern, new IR::Path(pktName)),
                                "emit"),
                            new IR::Vector<IR::Type>(
                                {new IR::Type_Name(ParserEnforceDepthReq::pad_hdr_type_name)}),
                            args));

                        bs->components.push_back(emit);
                        LOG5("Added to deparser " << ctrl->externalName() << ": " << emit);
                    }
                }
            }
        }
    }

    bool preorder(IR::ParserState *) override {
        insertHdrVldInitAfter = nullptr;
        return true;
    }

    void postorder(IR::AssignmentStatement *as) override {
        // Identify header.$valid = 0 assignments
        auto *left = as->left->to<IR::Member>();
        auto *right = as->right->to<IR::Constant>();
        if (!left || !right) return;

        if (left->toString().endsWith("$valid") && right->asInt() == 0) {
            insertHdrVldInitAfter = getOriginal<IR::AssignmentStatement>();
        }
    }

    /**
     * Add a counter operation to a parser state
     *
     * Adds a counter set operation to the start state, and adds a decrement to any other states
     * with non-zero extract/advance.
     */
    void addCounterOp(IR::ParserState *state) {
        int size = stateSize.at(findOrigCtxt<IR::P4Parser>()).at(state->name);

        IR::MethodCallStatement *ctrOp = nullptr;
        if (state->name == IR::ParserState::start) {
            auto *args = new IR::Vector<IR::Argument>({new IR::Argument(
                new IR::Constant(IR::Type_Bits::get(8), minDepthAllowed - size))});
            ctrOp = new IR::MethodCallStatement(new IR::MethodCallExpression(
                IR::Type_Void::get(),
                new IR::Member(prsrCtrSetMethod,
                               new IR::PathExpression(prsrCtrExtern, new IR::Path(prsrCtrName)),
                               "set"),
                new IR::Vector<IR::Type>({IR::Type_Bits::get(8)}), args));
        } else if (size > 0) {
            auto *args = new IR::Vector<IR::Argument>(
                {new IR::Argument(new IR::Constant(IR::Type_Bits::get(8), size))});
            ctrOp = new IR::MethodCallStatement(new IR::MethodCallExpression(
                IR::Type_Void::get(),
                new IR::Member(prsrCtrDecMethod,
                               new IR::PathExpression(prsrCtrExtern, new IR::Path(prsrCtrName)),
                               "decrement"),
                new IR::Vector<IR::Type>({}), args));
        }

        if (ctrOp) {
            state->components.push_back(ctrOp);
            LOG5("Counter op added to state " << state->externalName() << ": " << ctrOp);
        }
    }

    /**
     * Set pad headers invalid in the state
     */
    void addPadHdrSetInvalid(IR::ParserState *state) {
        for (auto it = state->components.begin(); it != state->components.end(); ++it) {
            if (*it == insertHdrVldInitAfter) {
                ++it;
                auto *zero = new IR::Constant(IR::Type_Bits::get(1), 0);
                for (int i = 0; i < prsrNumPadStates; ++i) {
                    auto *as = new IR::AssignmentStatement(
                        new IR::Member(
                            IR::Type_Bits::get(1),
                            new IR::ArrayIndex(
                                padHdr,
                                new IR::Member(padHdrStack[prsrHdrType],
                                               new IR::PathExpression(hdrStruct[prsrHdrType],
                                                                      new IR::Path(prsrHdrName)),
                                               prsrPadInstName),
                                new IR::Constant(i)),
                            "$valid"),
                        zero);
                    it = ++state->components.insert(it, as);
                    LOG5("Set valid added to state " << state->externalName() << ": " << as);
                }
                return;
            }
        }
    }

    void postorder(IR::SelectCase *sc) override {
        auto *state = findContext<IR::ParserState>();
        auto *prsr = findOrigCtxt<IR::P4Parser>();

        if (prsrHasPadStack) {
            if (padReq.at(prsr).padStatesAccept.count(state->name)) {
                if (sc->state->path->name == IR::ParserState::accept) {
                    sc->state = new IR::PathExpression(
                        new IR::Path(prsrInitialCheckState[IR::ParserState::accept]));
                    LOG5("Replaced accept transition in " << state->externalName());
                }
            }
            if (padReq.at(prsr).padStatesReject.count(state->name)) {
                if (sc->state->path->name == IR::ParserState::reject) {
                    sc->state = new IR::PathExpression(
                        new IR::Path(prsrInitialCheckState[IR::ParserState::reject]));
                    LOG5("Replaced reject transition in " << state->externalName());
                }
            }
        }
    }

    void postorder(IR::PathExpression *pe) override {
        if (!prsrHasPadStack) return;
        if (auto *state = getParent<IR::ParserState>()) {
            auto *orig_pe = getOriginal<IR::PathExpression>();
            if (state->selectExpression != orig_pe) return;

            auto *prsr = findOrigCtxt<IR::P4Parser>();

            if (padReq.at(prsr).padStatesAccept.count(state->name)) {
                if (pe->path->name == IR::ParserState::accept) {
                    pe->path = new IR::Path(prsrInitialCheckState[IR::ParserState::accept]);
                    LOG5("Replaced accept transition in " << state->externalName());
                }
            }
            if (padReq.at(prsr).padStatesReject.count(state->name)) {
                if (pe->path->name == IR::ParserState::reject) {
                    pe->path = new IR::Path(prsrInitialCheckState[IR::ParserState::reject]);
                    LOG5("Replaced reject transition in " << state->externalName());
                }
            }
        }
    }

    void postorder(IR::ParserState *state) override {
        if (prsrHasPadStack) {
            addCounterOp(state);
            if (insertHdrVldInitAfter) addPadHdrSetInvalid(state);
        }
    }

 public:
    AddParserPad(
        const std::set<cstring> &structs,
        const std::map<const IR::P4Parser *, std::map<cstring, int>> &stateSize,
        const std::map<const IR::P4Parser *, ParserEnforceDepthReq::ParserPadReq> &padReq,
        const std::map<cstring, int> &headerPadAmt,
        const std::map<const IR::P4Parser *, gress_t> &allParsers,
        const std::map<const IR::P4Control *, gress_t> &allDeparsers,
        const std::map<const IR::P4Control *, gress_t> &allMauPipes,
        const std::map<const IR::P4Control *, std::set<const IR::P4Parser *>> &deparserParser,
        const std::map<const IR::P4Control *, std::set<const IR::P4Parser *>> &mauPipeParser,
        int ctrShiftAmt)
        : structs(structs),
          stateSize(stateSize),
          padReq(padReq),
          headerPadAmt(headerPadAmt),
          allParsers(allParsers),
          allDeparsers(allDeparsers),
          allMauPipes(allMauPipes),
          deparserParser(deparserParser),
          mauPipeParser(mauPipeParser),
          ctrShiftAmt(ctrShiftAmt) {}
};

ParserEnforceDepthReq::ParserEnforceDepthReq(P4::ReferenceMap *rm, BFN::EvaluatorPass *ev)
    : refMap(rm), evaluator(ev) {
    // Per-cycle shift amount should match line rate to avoid falling behind
    double ctrShiftAmtBits = (double)Device::pardeSpec().lineRate() / Device::pardeSpec().clkFreq();
    ctrShiftAmt = ceil(ctrShiftAmtBits / 8);

    addPasses({
        evaluator,
        [this]() {
            auto toplevel = evaluator->getToplevelBlock();
            auto main = toplevel->getMain();
            for (auto &pkg : main->constantValue) {
                LOG2(pkg.first->toString()
                     << " : " << (pkg.second ? pkg.second->toString() : "nullptr"));
                if (auto pb = pkg.second ? pkg.second->to<IR::PackageBlock>() : nullptr) {
                    const IR::P4Parser *ip = nullptr;
                    const IR::P4Parser *ep = nullptr;
                    const IR::P4Control *imp = nullptr;
                    const IR::P4Control *emp = nullptr;
                    const IR::P4Control *id = nullptr;
                    const IR::P4Control *ed = nullptr;
                    for (auto &p : pb->constantValue) {
                        LOG2("  " << p.first->toString() << " : "
                                  << (p.second ? p.second->toString() : "nullptr"));
                        if (p.second) {
                            auto name = p.first->toString();
                            if (name == "ingress_parser") {
                                ip = p.second->to<IR::ParserBlock>()->container;
                                all_parser[ip] = INGRESS;
                            } else if (name == "ingress") {
                                imp = p.second->to<IR::ControlBlock>()->container;
                                all_mau_pipe[imp] = INGRESS;
                            } else if (name == "ingress_deparser") {
                                id = p.second->to<IR::ControlBlock>()->container;
                                all_deparser[id] = INGRESS;
                            } else if (name == "egress_parser") {
                                ep = p.second->to<IR::ParserBlock>()->container;
                                all_parser[ep] = EGRESS;
                            } else if (name == "egress") {
                                emp = p.second->to<IR::ControlBlock>()->container;
                                all_mau_pipe[emp] = EGRESS;
                            } else if (name == "egress_deparser") {
                                ed = p.second->to<IR::ControlBlock>()->container;
                                all_deparser[ed] = EGRESS;
                            }
                        }
                    }
                    if (ip && imp) mau_pipe_parser[imp].insert(ip);
                    if (ep && emp) mau_pipe_parser[emp].insert(ep);
                    if (ip && id) deparser_parser[id].insert(ip);
                    if (ep && ed) deparser_parser[ed].insert(ep);
                }
            }
        },
        new IdentifyPadRequirements(refMap, structs, stateSize, padReq, headerPadAmt,
                                    all_parser, ctrShiftAmt),
        new AddParserPad(structs, stateSize, padReq, headerPadAmt, all_parser, all_mau_pipe,
                         all_deparser, deparser_parser, mau_pipe_parser, ctrShiftAmt),
    });
}

}  // namespace BFN
