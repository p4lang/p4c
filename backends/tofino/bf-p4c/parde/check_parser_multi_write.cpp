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

#include "check_parser_multi_write.h"

#include "lib/error_reporter.h"
#include "bf-p4c/common/utils.h"
#include "bf-p4c/device.h"
#include "bf-p4c/parde/dump_parser.h"
#include "bf-p4c/parde/parser_info.h"
#include "bf-p4c/parde/parser_query.h"
#include "bf-p4c/parde/parde_utils.h"
#include "bf-p4c/parde/parde_visitor.h"
#include "bf-p4c/phv/phv_fields.h"

const char *CHECKSUM_VERIFY_OR_SUGGESTION = "\n"
        "If your intention was to accumulate checksum verification errors over multiple checksums, "
        "write OR explicitly: either like `err = checksum.verify() || err` if `err` is `bool`, or "
        "like `err = err | (bit<1>)checksum.verify()` if it is an integer (in this case, `err` "
        "can be a slice).";

inline Util::SourceInfo clear_if_same(Util::SourceInfo info, Util::SourceInfo other) {
    return info == other ? Util::SourceInfo() : info;
}

inline Util::SourceInfo first_valid(Util::SourceInfo a, Util::SourceInfo b) {
    return a.isValid() ? a : b;
}

inline bool is_constant_extract(const IR::BFN::ParserPrimitive* p) {
    if (auto e = p->to<IR::BFN::Extract>())
        return e->source->is<IR::BFN::ConstantRVal>();
    return false;
}

inline const nw_bitrange* get_extract_range(const IR::BFN::ParserPrimitive *p) {
    auto e = p->to<IR::BFN::Extract>();
    if (!e)
        return nullptr;
    auto rval = e->source->to<IR::BFN::InputBufferRVal>();
    if (!rval)
        return nullptr;

    return &rval->range;
}

using WrMode = IR::BFN::ParserWriteMode;

struct InferWriteMode : public ParserTransform {
    const PhvInfo& phv;
    const CollectParserInfo& parser_info;
    const MapFieldToParserStates& field_to_states;
    const ParserQuery pq;

    InferWriteMode(const PhvInfo& ph, const CollectParserInfo& pi,
                   const MapFieldToParserStates& fs) :
        phv(ph), parser_info(pi), field_to_states(fs), pq(pi, fs) { }

    std::map<const IR::BFN::ParserPrimitive*, IR::BFN::ParserWriteMode> write_to_write_mode;

    ordered_set<const IR::BFN::ParserPrimitive*> zero_inits;
    ordered_set<const IR::BFN::ParserPrimitive*> dead_writes;

    // Find the first writes, i.e. inits, given the set of writes. The set of writes
    // belong to the same field. An init is a write that has no prior write in the parser IR.
    ordered_set<const IR::BFN::ParserPrimitive*>
    find_inits(const ordered_set<const IR::BFN::ParserPrimitive*>& writes) {
        ordered_set<const IR::BFN::ParserPrimitive*> inits;

        for (auto p : writes) {
            bool has_predecessor = false;

            auto ps = field_to_states.write_to_state.at(p);
            auto parser = field_to_states.state_to_parser.at(ps);

            for (auto q : writes) {
                if ((has_predecessor =
                         pq.is_before(writes, parser, q, nullptr, p, ps)))
                    break;
            }

            if (!has_predecessor)
               inits.insert(p);
        }

        return inits;
    }

    ordered_set<const IR::BFN::ParserPrimitive*>
    exclude_zero_inits(const ordered_set<const IR::BFN::ParserPrimitive*>& writes) {
        auto inits = find_inits(writes);
        ordered_set<const IR::BFN::ParserPrimitive*> rv;

        for (auto e : writes) {
            if (is_zero_extract(e) && inits.count(e)) {
                zero_inits.insert(e);
                continue;
            }

            rv.insert(e);
        }

        return rv;
    }

    bool is_zero_extract(const IR::BFN::ParserPrimitive* p) {
        auto *e = p->to<IR::BFN::Extract>();
        if (!e)
            return false;
        if (auto src = e->source->to<IR::BFN::ConstantRVal>()) {
            if (src->constant->value == 0)
                return true;
        }

        return false;
    }

    void mark_write_mode(IR::BFN::ParserWriteMode mode,
                         const PHV::Field* dest,
                         const ordered_set<const IR::BFN::ParserPrimitive*>& writes) {
        LOG3("mark " << dest->name << " as " << mode);

        for (auto e : writes)
            write_to_write_mode[e] = mode;
    }

    struct CounterExample {
        ordered_set<const IR::BFN::ParserPrimitive*> prev;
        const IR::BFN::ParserPrimitive* curr = nullptr;
        const bool is_rewritten_always = false;

        CounterExample(ordered_set<const IR::BFN::ParserPrimitive*> prev,
                       const IR::BFN::ParserPrimitive* curr,
                       const bool is_rewritten_always = false)
            : prev(prev), curr(curr), is_rewritten_always(is_rewritten_always) {
            BUG_CHECK(curr && prev.size(), "Attempted to create counterexample without specifying "
                      "rewritten fields");
        }
    };

    void print(CounterExample* example) {
        auto c_field = example->curr->getWriteDest()->field;
        bool on_loop = std::any_of(example->prev.begin(), example->prev.end(),
                          [=](const IR::BFN::ParserPrimitive* p) { return p == example->curr; });
        // On Tofino 1, we emit only warning unless the field is rewritten on all paths.
        // FIXME: we probably need a better check and pragma-triggered supression
        // FIXME(vstill): use ErrorType directly once the appropriate overload is added to p4c
        if (Device::currentDevice() != Device::TOFINO || example->is_rewritten_always) {
            error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                "%1%%2% is assigned in state %3% but has also previous assignment%4%%5%%6%. %7%"
                "This re-assignment is not supported by Tofino.%8%%9%",
                /* 1 */ first_valid(c_field->getSourceInfo(), example->curr->getSourceInfo()),
                /* 2 */ c_field->toString(),
                /* 3 */ field_to_states.write_to_state.at(example->curr),
                /* 4 */ example->prev.size() > 1 ? "s"_cs : ""_cs,
                /* 5 */ on_loop && example->prev.size() > 1 ? " including assignment"_cs : ""_cs,
                /* 6 */ on_loop ? " in the same state due to a loop"_cs : ""_cs,
                /* 7 */ example->prev.size() - int(on_loop) > 0 ?
                        "See the following errors for the list of previous assignments. "_cs :
                        ""_cs,
                /* 8 */ example->is_rewritten_always ?
                        "\nThe field will either always be assigned multiple times or there "_cs +
                        "is a loopback in the parser that always reassigns the field."_cs : ""_cs,
                /* 9 */ example->curr->is<IR::BFN::ChecksumVerify>() ?
                        CHECKSUM_VERIFY_OR_SUGGESTION : ""_cs);
        } else {
            warning(ErrorType::WARN_UNSUPPORTED,
                "%1%%2% is assigned in state %3% but has also previous assignment%4%%5%%6%. %7%"
                "This re-assignment is not supported by Tofino.%8%%9%",
                /* 1 */ first_valid(c_field->getSourceInfo(), example->curr->getSourceInfo()),
                /* 2 */ c_field->toString(),
                /* 3 */ field_to_states.write_to_state.at(example->curr),
                /* 4 */ example->prev.size() > 1 ? "s"_cs : ""_cs,
                /* 5 */ on_loop && example->prev.size() > 1 ? " including assignment"_cs : ""_cs,
                /* 6 */ on_loop ? " in the same state due to a loop"_cs : ""_cs,
                /* 7 */ example->prev.size() - int(on_loop) > 0 ?
                        "See the following errors for the list of previous assignments. "_cs :
                        ""_cs,
                /* 8 */ example->is_rewritten_always ?
                        "\nThe field will either always be assigned multiple times or there "_cs +
                        "is a loopback in the parser that always reassigns the field."_cs : ""_cs,
                /* 9 */ example->curr->is<IR::BFN::ChecksumVerify>() ?
                        CHECKSUM_VERIFY_OR_SUGGESTION : ""_cs);
        }
        for (auto p : example->prev) {
            if (p == example->curr)
                continue;  // skip printing the looped assignment again
            auto ps = field_to_states.write_to_state.at(p);
            // FIXME(vstill) we can't use the error type here now as it would
            // then be possibly ignored if we have reported another error with
            // the same source location.
            auto p_field = p->getWriteDest()->field;
            if (Device::currentDevice() != Device::TOFINO || example->is_rewritten_always) {
                error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                    "%1%%2% previously assigned in state %3%.",
                    first_valid(p_field->getSourceInfo(), p->getSourceInfo()),
                    p_field->toString(), ps);
            } else {
                warning(ErrorType::WARN_UNSUPPORTED,
                    "%1%%2% previously assigned in state %3%.",
                    first_valid(p_field->getSourceInfo(), p->getSourceInfo()),
                    p_field->toString(), ps);
            }
        }
    }

    /// Check if the current write @p curr_p is safe to be applied BITWISE_OR on the @p prev_p
    /// write. This can be done if we can guarantee that no bit will need to be set to 0.
    /// - if both @p prev_p and @p curr_p are constants and their bitwise or is same as @p curr_p
    /// - if @p curr_p is a constant and sets all bits in the left-hand-side of the assignment to 1
    /// - if @p prev_p is constant 0
    bool can_absorb(const IR::BFN::ParserPrimitive* prev_p,
                    const IR::BFN::ParserPrimitive* curr_p) {
        auto prev = prev_p->to<IR::BFN::Extract>();
        auto curr = curr_p->to<IR::BFN::Extract>();
        // non-extracts cannot absorb each other
        if (!prev || !curr)
            return false;
        auto prev_c = prev->source->to<IR::BFN::ConstantRVal>();
        auto curr_c = curr->source->to<IR::BFN::ConstantRVal>();

        // for constants, check absorbtion exactly
        if (prev_c && curr_c) {
            // the equality has to be checked on the big-int value, otherwise we could have false
            // negatives due to constant base mismatch
            big_int or_prev = prev_c->constant->value | curr_c->constant->value;
            return or_prev == curr_c->constant->value;
        }

        // for non-constants, a all-one value can cover anything, so allow it (this mainly covers
        // cases when bool true is written over a non-constant value)
        if (curr_c) {
            auto bit_size = curr->dest->size();
            BUG_CHECK(bit_size > 0, "Invalid bit size %1% in %2%", bit_size, curr);
            big_int mask = (big_int(1) << bit_size) - 1;
            return (curr_c->constant->value & mask) == mask;
        }

        // also accept or-ing anything to 0
        return is_zero_extract(prev);
    }

    /// Determines whether extract @p write is postdominated by extract of the same
    /// field (from @p writes).
    /// Essentially we want to check 2 separate things:
    ///   1. A state/extract is postdominated by extracts of a same field as is (without loopbacks)
    ///      Example: A->B
    ///                ->C
    ///      if A, B, C extract the same field x
    ///         then is_postdominated_by_extract = true
    ///      if A, B extract the same field x, but C does not
    ///         then is_postdominated_by_extract = false
    ///   2. There is a loopback in which the same field is always extracted
    ///      (loop start is postdominate by extracts within that loop).
    ///      In this case we also have to take into account the fact that
    ///      the "loop from" state needs to be dominated by the set, otherwise
    ///      the loop might make sense when we reach this state without extracting
    ///      the field and then take the loopback once.
    ///      Example: A->B->D->A    (loopback from D to A)
    ///                ->C->
    ///      if B, C extract the same field x
    ///         then is_postdominated_by_extract = true
    ///      if A or D extract the field x
    ///         then is_postdominated_by_extract = true
    ///      if A, D does not extract x and only one of B, C extract the field x
    ///         then is_postdominated_by_extract = false
    ///
    /// This is mainly used to detect the cases where we allow unsafe rewrites on Tofino 1
    bool is_postdominated_by_extract(const IR::BFN::ParserPrimitive* write,
                                     const ordered_set<const IR::BFN::ParserPrimitive*>& writes) {
        // Get states, parser, graph
        auto state = field_to_states.write_to_state.at(write);
        auto parser = field_to_states.state_to_parser.at(state);
        auto graph = parser_info.graph(parser);
        ordered_set<const IR::BFN::ParserState*> states;
        for (auto w : writes) {
            auto ws = field_to_states.write_to_state.at(w);
            states.insert(ws);
        }

        // Check if the state is directly postdominated by the extracts
        if (graph.is_postdominated_by_set(state, states)) {
            // For Tofino 1 produce a warning as this is not translated correctly, but is still
            // allowed
            if (Device::currentDevice() == Device::TOFINO)
                warning(ErrorType::WARN_UNSUPPORTED,
                          "Extract %1% in state %2% is postdominated by extracts of the same "
                          "field.", write->getWriteDest()->field, state->name);
            return true;
        }
        // Check all of the loops if they have a postdomination inside
        // and also the "loop from" state is dominated by the same set
        auto lb = graph.is_loopback_reassignment(state, states);
        if (lb.first && lb.second) {
            // For Tofino 1 produce a warning as this is not translated correctly, but is still
            // allowed
            if (Device::currentDevice() == Device::TOFINO)
                warning(ErrorType::WARN_UNSUPPORTED,
                          "Extract %1% in state %2% is postdominated by extracts of the same field "
                          "within a loopback from %3% to %4%.",
                          write->getWriteDest()->field, state->name, lb.second->name,
                          lb.first->name);
            return true;
        }
        return false;
    }

    /// @p strict indicates that all ovewrites should be treated as "always overwrites". This
    /// specifically means that they will always cause error even on Tofino 1.
    CounterExample* is_bitwise_or(const PHV::Field* dest,
                                  const ordered_set<const IR::BFN::ParserPrimitive*>& writes,
                                  bool strict = false) {
        if (dest->name.endsWith("$stkvalid"))
            return nullptr;

        CounterExample* counter_example = nullptr;

        for (auto e : writes) {
            auto prev = pq.get_previous_writes(e, writes);
            auto pwm = e->getWriteMode();

            for (auto p : prev) {
                if (pwm != WrMode::BITWISE_OR && !can_absorb(p, e)) {
                    auto ce = new CounterExample(prev, e,
                                      strict || is_postdominated_by_extract(p, writes));
                    // Always prefer "always rewritten" counterexamples.
                    // This is especially important on Tofino 1 where other counterexamples
                    // are not currently considered to be errors, but it also gives better error
                    // mesage to the user.
                    if (counter_example == nullptr || ce->is_rewritten_always)
                        counter_example = ce;
                }
            }
        }

        return counter_example;
    }

    CounterExample* is_clear_on_write(const ordered_set<const IR::BFN::ParserPrimitive*>& writes) {
        auto inits = find_inits(writes);

        const IR::BFN::ParserPrimitive* prev = nullptr;

        for (auto e : writes) {
            if (!inits.count(e) && e->getWriteMode() == IR::BFN::ParserWriteMode::BITWISE_OR)
                return new CounterExample({prev}, e);

            prev = e;
        }

        return nullptr;
    }

    /// Check for all pairs of checksum residual deposits that can conflict with each other.
    /// This can be a problem as these instructions have affect at the end of the parser, after
    /// they were called in some previous state.
    void validate_checksum_residual_deposits(const PHV::Field* field,
            const ordered_set<const IR::BFN::ParserPrimitive*>& writes) {
        for (auto p : writes) {
            auto rp = p->to<IR::BFN::ChecksumResidualDeposit>();
            if (!rp)
                continue;

            for (auto q : writes) {
                if (p == q)
                    break;
                auto rq = q->to<IR::BFN::ChecksumResidualDeposit>();
                if (!rq)
                    continue;

                auto ps = field_to_states.write_to_state.at(p);
                auto qs = field_to_states.write_to_state.at(q);
                auto parser = field_to_states.state_to_parser.at(ps);
                // if one of the deposits can reach the other, we have a problem
                if (ps == qs || parser_info.graph(parser).is_reachable(ps, qs)
                    || parser_info.graph(parser).is_reachable(qs, ps))
                    error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                            "%1%%2%It is not possible to deposit multiple checksum "
                            "residuals into field %3% in states %4% and %5%. "
                            "Either make sure the states in which you deposit cannot reach one "
                            "another, or save the checksum residuals into separate fields",
                            p->getSourceInfo(), q->getSourceInfo(),
                            field->name, ps, qs);
            }
        }
    }

    // At this time, a field's write mode is either SINGLE_WRITE or BITWISE_OR (from program
    // directly). We will try to infer the fields with multiple extracts as SINGLE_WRITE,
    // BITWISE_OR or CLEAR_ON_WRITE.
    void infer_write_mode(const PHV::Field* dest,
                          const ordered_set<const IR::BFN::ParserPrimitive*>& prim_writes) {
        for (auto e : prim_writes) {
            BUG_CHECK(e->getWriteMode() != IR::BFN::ParserWriteMode::CLEAR_ON_WRITE,
                      "field already as clear-on-write mode?");
        }
        validate_checksum_residual_deposits(dest, prim_writes);

        auto writes = mark_and_exclude_dead_writes(exclude_zero_inits(prim_writes));

        if (pq.is_single_write(writes)) {
            // As a special case, we set checksum verify deposits as BITWISE_OR even if it is
            // single write so that they can be safely packed together into one PHV.
            if (std::all_of(writes.begin(), writes.end(),
                    [](const IR::BFN::ParserPrimitive *pr) {
                        return pr->is<IR::BFN::ChecksumVerify>(); })
                    && is_bitwise_or(dest, writes, true) == nullptr)
                mark_write_mode(IR::BFN::ParserWriteMode::BITWISE_OR, dest, prim_writes);
            else
                mark_write_mode(IR::BFN::ParserWriteMode::SINGLE_WRITE, dest, prim_writes);
            return;
        }

        auto or_counter_example = is_bitwise_or(dest, writes);

        if (!or_counter_example) {
            mark_write_mode(IR::BFN::ParserWriteMode::BITWISE_OR, dest, prim_writes);
            return;
        }
        auto clow_counter_example = is_clear_on_write(writes);

        if (!clow_counter_example) {
            if (Device::currentDevice() != Device::TOFINO) {
                mark_write_mode(IR::BFN::ParserWriteMode::CLEAR_ON_WRITE, dest, prim_writes);
            } else {
                auto ps = field_to_states.write_to_state.at(or_counter_example->curr);

                warning(ErrorType::WARN_UNSUPPORTED,
                        "Tofino does not support clear-on-write semantic on re-assignment to "
                        "field %1% in parser state %2%. Try to use advance() to skip over the"
                        "re-assigned values and only extract them once.",
                        dest->name, ps->name);
                print(or_counter_example);
                return;
            }
        } else {
            auto ps = field_to_states.write_to_state.at(clow_counter_example->curr);

            error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                    "Inconsistent parser write semantic for field %1% in parser state %2%.",
                    dest->name, ps->name);
            print(clow_counter_example);
        }
    }

    // For consecutive writes to the same field in the same state, the last write wins.
    // In example below, a.x receives two writes, with only the last write taking effect,
    // and the first extract can be dead code eliminated. The extracts are writes to the
    // field.
    //
    //    packet.extract(hdr.a);
    //    packet.extract(hdr.b);
    //
    //    hdr.a.x = hdr.b.y;
    //
    ordered_set<const IR::BFN::ParserPrimitive*>
    mark_and_exclude_dead_writes(const ordered_set<const IR::BFN::ParserPrimitive*>& writes) {
        // some writes can require bitwise or (for assignments like `x = x | y`), if so, we
        // can't eliminate dead writes
        // also, checksum.verify() is always or-ed even when not using `x = x | ch.verify()`
        bool forced_or = std::any_of(writes.begin(), writes.end(),
            [this](const IR::BFN::ParserPrimitive* pr) {
                auto dest = phv.field(pr->getWriteDest()->field);
                return pr->getWriteMode() == WrMode::BITWISE_OR
                    || (dest && dest->name.endsWith("$stkvalid"));
            });
        // we can't eliminate extracts that extract part of a byte, the extractor has byte
        // granularity
        // FIXME: this can be done if all extracts to the given byte are eliminated
        bool partial = std::any_of(writes.begin(), writes.end(),
            [this](const IR::BFN::ParserPrimitive* pr) {
                auto range = get_extract_range(pr);
                return range && (!range->isLoAligned() || !range->isHiAligned());
            });
        if (forced_or || partial)
            return writes;

        ordered_set<const IR::BFN::ParserPrimitive*> non_dead;
        std::map<const IR::BFN::ParserState*,
                 ordered_set<const IR::BFN::ParserPrimitive*>> state_to_writes;

        for (auto e : writes) {
            auto s = field_to_states.write_to_state.at(e);
            state_to_writes[s].insert(e);
        }

        for (auto& [state, extracts] : state_to_writes) {
            // If more than one writes exist, the last write wins.
            // Mark all other overlapping writes as dead to be elim'd later.
            auto it = extracts.rbegin();
            auto last = it++;
            le_bitrange last_bitrange;
            BUG_CHECK(phv.field((*last)->getWriteDest()->field, &last_bitrange) != nullptr,
                "No PHV field for the extract");
            while (it != extracts.rend()) {
                le_bitrange it_bitrange;
                BUG_CHECK(phv.field((*it)->getWriteDest()->field, &it_bitrange) != nullptr,
                    "No PHV field for extract");
                // parser checksums can't be dead-write eliminated because they change also
                // settings of the checksum unit
                if (last_bitrange.contains(it_bitrange)
                        && !(*it)->is<IR::BFN::ParserChecksumWritePrimitive>())
                    dead_writes.insert(*it);
                it++;
            }
        }

        for (auto write : writes) {
            if (dead_writes.count(write) == 0)
                non_dead.insert(write);
        }
        return non_dead;
    }

    IR::Node* preorder(IR::BFN::Extract* extract) override {
        auto orig = getOriginal<IR::BFN::Extract>();

        if (zero_inits.count(orig)) {
            LOG3("removed zero init " << extract);
            // return a new ParserZeroInit IR node instead of removing it. And also mark
            // ParserZeroInit as write in TofinoContextWrite, so that extract->dest->field will be
            // recognized as write in field_defuse. The reason to do this is because if this
            // instruction is removed here, then in field_defuse, all of uses whose def is parser
            // zero initialization will disappear and result in incorrection defuse generation.
            // In addition, in lower_parser.cpp, ParserZeroInit IR node will be ignored, so that
            // this node is removed in this way.
            if (auto member = extract->dest->field->to<IR::Member>()) {
                if (member->member.name.endsWith("$valid")) {
                    return nullptr;
                }
            }
            return new IR::BFN::ParserZeroInit(new IR::BFN::FieldLVal(extract->dest->field));
        }

        if (dead_writes.count(orig)) {
            LOG3("removed dead extract " << extract);
            return nullptr;
        }

        if (auto it = write_to_write_mode.find(orig); it != write_to_write_mode.end()) {
            LOG3("Setting write mode of " << extract << " to " << it->second);
            extract->write_mode = it->second;
        }

        auto dest = phv.field(extract->dest->field);

        if (dest && dest->name.endsWith("$stkvalid"))
            extract->write_mode = IR::BFN::ParserWriteMode::BITWISE_OR;

        return extract;
    }

    IR::Node* preorder(IR::BFN::ParserChecksumWritePrimitive* checksum_write) override {
        // NOTE: no checksum primitives can be removed, even if they are dead as they change state
        // of the checksum unit(s).
        auto it = write_to_write_mode.find(getOriginal<IR::BFN::ParserChecksumWritePrimitive>());
        if (it != write_to_write_mode.end()) {
            LOG3("Setting write mode of " << checksum_write << " to " << it->second);
            auto verify = checksum_write->to<IR::BFN::ChecksumVerify>();
            if (verify && it->second == WrMode::CLEAR_ON_WRITE) {
                error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                        "%1%Using checksum verify in direct assignment to set %2% is not "
                        "supported when the left-hand side of the assignment can be written "
                        "multiple times for one packet. "
                        "To avoid silent change of behavior, the result of checksum.verify() can "
                        "only be written as logical or bitwise OR to accumulate multiple errors."
                        "%3%",
                        checksum_write->getSourceInfo(),
                        cstring::to_cstring(checksum_write->getWriteDest()),
                        CHECKSUM_VERIFY_OR_SUGGESTION);
            }
            checksum_write->write_mode = it->second;
        }
        return checksum_write;
    }

    profile_t init_apply(const IR::Node* root) override {
        LOG9("Inferring write modes on\n" << root);
        for (auto& [field, writes] : field_to_states.field_to_writes) {
            // if state is strided, it will use offset_inc in the BFA and that means it will use
            // parser counter to place the later writes into following stack position/following
            // PHV fields
            bool stride = false;

            for (auto s : field_to_states.field_to_parser_states.at(field)) {
                if (s->stride) {
                    stride = true;
                    break;
                }
            }

            if (!stride)
                infer_write_mode(field, writes);
        }

        return ParserTransform::init_apply(root);
    }
};

bool CheckWriteModeConsistency::check(const std::vector<const IR::BFN::Extract*> extracts) const {
    if (extracts.size() <= 1) return true;

    WrMode first_mode = extracts[0]->write_mode;
    bool consistent = std::all_of(extracts.begin(), extracts.end(),
            [=](const IR::BFN::Extract* e) {
                LOG9("      extract " << e << " " << e->write_mode);
                return e->write_mode == first_mode;
            });


    if (consistent) return true;

    // not consistent, let's look if this can be fixed
    // * padding fields don't matter, calculate what mode can be used without them
    bool bitwise_or = false;
    bool clear_on_write = false;
    bool has_non_padding = false;
    consistent = true;

    for (auto &e : extracts) {
        auto dest = phv.field(e->dest->field);
        if (dest->padding)
            continue;
        // it may happen that the first one is a padding
        if (!has_non_padding) {
            first_mode = e->write_mode;
            has_non_padding = true;
        }

        if (consistent && e->write_mode != first_mode) {
            consistent = false;
        }
        bitwise_or |= e->write_mode == WrMode::BITWISE_OR;
        clear_on_write |= e->write_mode == WrMode::CLEAR_ON_WRITE;
    }

    // and set padding mode to match the rest of fields, clearly it is not SINGLE_WRITE,
    // otherwise we would have no problem
    BUG_CHECK(has_non_padding, "non non-padding extracts found");

    if (consistent)  // with padding
        return true;

    // * if we have no CLEAR_ON_WRITE for non-padding fields, we can maybe set all
    //   fields to BITWISE_OR mode if
    //   - all SINGLE_WRITE fields are actually set only once (therefore we are not
    //     hiding a runtime error)
    //   - all other writes of the BITWISE_OR fields are either constant, or result of
    //     checksum.verify() and therefore they can guarantee bytes other than ones
    //     corresponding to the field in question will not be set
    if (!clear_on_write) {
        auto writes_safe = true;
        for (auto e : extracts) {
            auto dest = phv.field(e->dest->field);
            if (dest->padding)
                continue;
            // all writes to the same field as extract `e`
            auto e_writes = field_to_states.field_to_writes.at(dest);
            if (e->write_mode == WrMode::BITWISE_OR) {
                // it is safe if all other writes are constant or checksum.verify()
                writes_safe &= std::all_of(
                    e_writes.begin(), e_writes.end(),
                    [e](const IR::BFN::ParserPrimitive* wr) {
                        return wr == e || wr->is<IR::BFN::ChecksumVerify>()
                            || is_constant_extract(wr);
                    });
            } else {  // SINGLE_WRITE
                // check that there is actually only one write
                writes_safe &= e_writes.size() == 1 || pq.is_single_write(e_writes);
            }
            if (!writes_safe)
                break;
        }
        if (writes_safe)
            return true;  // no error
    }

    return false;
}

void CheckWriteModeConsistency::check_and_adjust(
    const std::vector<const IR::BFN::Extract *> extracts) {
    if (extracts.size() <= 1) return;

    WrMode first_mode = extracts[0]->write_mode;
    bool consistent = std::all_of(extracts.begin(), extracts.end(),
            [=](const IR::BFN::Extract* e) {
                LOG9("      extract " << e << " " << e->write_mode);
                return e->write_mode == first_mode;
            });


    if (consistent) return;

    // not consistent, let's look if this can be fixed
    // * padding fields don't matter, calculate what mode can be used without them
    bool bitwise_or = false;
    bool clear_on_write = false;
    bool has_non_padding = false;
    consistent = true;
    int inconsistent_index = -1;
    int first_non_padding_index = -1;

    for (auto &e : extracts) {
        auto dest = phv.field(e->dest->field);
        if (dest->padding)
            continue;
        // it may happen that the first one is a padding
        if (!has_non_padding) {
            first_mode = e->write_mode;
            has_non_padding = true;
            first_non_padding_index = &e - extracts.data();
        }

        if (consistent && e->write_mode != first_mode) {
            inconsistent_index = &e - extracts.data();
            consistent = false;
        }
        bitwise_or |= e->write_mode == WrMode::BITWISE_OR;
        clear_on_write |= e->write_mode == WrMode::CLEAR_ON_WRITE;
    }

    // and set padding mode to match the rest of fields, clearly it is not SINGLE_WRITE,
    // otherwise we would have no problem
    BUG_CHECK(has_non_padding, "non non-padding extracts found");
    LOG3("CWMC: Setting write mode of padding fields extracted together with "
         << extracts[first_non_padding_index] << ".");
    for (auto e : extracts) {
        auto dest = phv.field(e->dest->field);
        LOG9("    - " << e << " -> " << dest);
        if (dest->padding)
            extract_to_write_mode[e] = clear_on_write ? WrMode::CLEAR_ON_WRITE
                                       : bitwise_or   ? WrMode::BITWISE_OR
                                                      : WrMode::SINGLE_WRITE;
    }

    if (consistent)  // with padding
        return;

    // * if we have no CLEAR_ON_WRITE for non-padding fields, we can maybe set all
    //   fields to BITWISE_OR mode if
    //   - all SINGLE_WRITE fields are actually set only once (therefore we are not
    //     hiding a runtime error)
    //   - all other writes of the BITWISE_OR fields are either constant, or result of
    //     checksum.verify() and therefore they can guarantee bytes other than ones
    //     corresponding to the field in question will not be set
    if (!clear_on_write) {
        auto writes_safe = true;
        for (auto e : extracts) {
            auto dest = phv.field(e->dest->field);
            if (dest->padding)
                continue;
            // all writes to the same field as extract `e`
            auto e_writes = field_to_states.field_to_writes.at(dest);
            if (e->write_mode == WrMode::BITWISE_OR) {
                // it is safe if all other writes are constant or checksum.verify()
                writes_safe &= std::all_of(
                    e_writes.begin(), e_writes.end(),
                    [e](const IR::BFN::ParserPrimitive* wr) {
                        return wr == e || wr->is<IR::BFN::ChecksumVerify>()
                            || is_constant_extract(wr);
                    });
            } else {  // SINGLE_WRITE
                // check that there is actually only one write
                writes_safe &= e_writes.size() == 1 || pq.is_single_write(e_writes);
            }
            if (!writes_safe)
                break;
        }
        if (writes_safe) {
            LOG3("CWMC: Setting all fields extracted together with "
                 << extracts[first_non_padding_index] << " as BITWISE_OR.");
            for (auto e : extracts) {
                auto dest = phv.field(e->dest->field);
                LOG9("    - " << e << " -> " << dest);
                extract_to_write_mode[e] = WrMode::BITWISE_OR;
            }
            return;  // no error
        }
    }

    std::stringstream modes;
    modes << "The following fields are packed into a single byte: ";
    const char *sep = "";
    for (auto e : extracts) {
        auto dest = phv.field(e->dest->field);
        modes << sep << dest->name << " (" << cstring::to_cstring(e->write_mode)
              << (dest->padding ? ", padding" : "") << ")";
        sep = ", ";
    }
    modes << ".";

    BUG_CHECK(inconsistent_index >= 1, "Inconsistent index should never be < 1");
    error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
            "%3%%4%%1% and %2% share the same byte on the wire but"
            " have conflicting parser write semantics.\n    %5%",
            phv.field(extracts[inconsistent_index - 1]->dest->field)->name,
            phv.field(extracts[inconsistent_index]->dest->field)->name,
            extracts[inconsistent_index - 1]->getSourceInfo(),
            clear_if_same(extracts[inconsistent_index]->getSourceInfo(),
                          extracts[inconsistent_index - 1]->getSourceInfo()),
            modes.str());
}

void CheckWriteModeConsistency::check_pre_alloc(
    const ordered_set<const IR::BFN::ParserPrimitive *> &state_writes) {
    std::map<int, std::map<int, std::vector<const IR::BFN::Extract *>>> byte_to_extract;
    int last_bit = -1;
    int generation = 0;

    for (auto curr_w : state_writes) {
        auto range = get_extract_range(curr_w);
        if (!range)
            continue;
        if (range->lo <= last_bit)
            ++generation;
        last_bit = range->hi;

        auto curr = curr_w->to<IR::BFN::Extract>();
        BUG_CHECK(curr, "Somehow we got range for non-extract");
        if (!range->isLoAligned())
            byte_to_extract[range->loByte()][generation].push_back(curr);
        if (!range->isHiAligned())
            byte_to_extract[range->hiByte()][generation].push_back(curr);
    }

    LOG9("CWMC: checking state:");
    for (auto &by_byte : byte_to_extract) {
        LOG9("  bit " << by_byte.first);
        for (auto &by_gen : by_byte.second) {
            LOG9("    generation " << by_gen.first << ": "
                 << by_gen.second.size() << " extracts");
            auto extracts = by_gen.second;
            check_and_adjust(extracts);
        }
    }
}

void CheckWriteModeConsistency::check_post_alloc() {
    for (auto id : Device::phvSpec().physicalContainers()) {
        auto c = Device::phvSpec().idToContainer(id);
        if (c.is(PHV::Kind::dark) || c.is(PHV::Kind::tagalong)) continue;

        // Get the slices that could be valid out of parser
        auto write = PHV::FieldUse(PHV::FieldUse::WRITE);
        ordered_set<const PHV::Field*> fields;
        for (auto& slice : phv.get_slices_in_container(c, PHV::AllocContext::PARSER, &write)) {
            fields.emplace(slice.field());
        }
        if (!fields.size()) continue;

        ordered_map<const IR::BFN::ParserState *, ordered_set<const IR::BFN::Extract *>>
            extracts_by_state;
        for (auto *f : fields) {
            if (!field_to_states.field_to_writes.count(f)) continue;
            auto& writes = field_to_states.field_to_writes.at(f);
            for (auto* write : writes) {
                if (auto* ext = write->to<IR::BFN::Extract>()) {
                    // Ignore constant extracts
                    if (!get_extract_range(ext)) continue;

                    auto* state = field_to_states.write_to_state.at(write);

                    PHV::FieldUse use(PHV::FieldUse::WRITE);
                    auto slices =
                        phv.get_alloc(ext->dest->field, PHV::AllocContext::PARSER, &use);
                    for (auto& slice : slices) {
                        if (slice.container() == c) {
                            LOG3("  Extract in " << state->name << ": " << ext);
                            extracts_by_state[state].emplace(ext);
                        }
                    }
                }
            }
        }

        for (auto& [_, extracts] : extracts_by_state) {
            std::vector<const IR::BFN::Extract*> extracts_vec;
            extracts_vec.reserve(extracts.size());
            extracts_vec.insert(extracts_vec.begin(), extracts.begin(), extracts.end());
            check_and_adjust(extracts_vec);
        }
    }
}

Visitor::profile_t CheckWriteModeConsistency::init_apply(const IR::Node* root) {
    if (phv.alloc_done()) {
        check_post_alloc();
    } else {
        for (auto& [state, writes] : field_to_states.state_to_writes) {
            check_pre_alloc(writes);
        }
    }

    return ParserTransform::init_apply(root);
}

IR::Node* CheckWriteModeConsistency::preorder(IR::BFN::Extract* extract) {
    return set_write_mode(extract);
}

IR::Node* CheckWriteModeConsistency::preorder(IR::BFN::ParserChecksumWritePrimitive* pcw) {
    return set_write_mode(pcw);
}

template<typename T>
IR::Node* CheckWriteModeConsistency::set_write_mode(T *write) {
    auto it = extract_to_write_mode.find(getOriginal<T>());
    if (it != extract_to_write_mode.end()) {
        LOG4("CWMC: Setting write mode of " << write << " to " << it->second);
        write->write_mode = it->second;
    }

    return write;
}

bool CheckWriteModeConsistency::check_compatability(const PHV::FieldSlice& slice_a,
                                                    const PHV::FieldSlice& slice_b) {
    // Cheap checks to establish compatability
    const auto *f_a = slice_a.field();
    const auto* f_b = slice_b.field();
    if (f_a->padding || f_b->padding) return true;
    if (phv.field_mutex()(f_a->id, f_b->id)) return true;
    if (!field_to_states.field_to_writes.count(f_a) || !field_to_states.field_to_writes.count(f_b))
        return true;

    // Have we seen this pair of slices before?
    auto slices_a_b = std::make_pair(slice_a, slice_b);
    if (compatability.count(slices_a_b))
        return compatability.at(slices_a_b);
    else {
        auto slices_b_a = std::make_pair(slice_b, slice_a);
        if (compatability.count(slices_b_a))
            return compatability.at(slices_b_a);
    }

    // Identify the extracts of the two fields
    ordered_map<const IR::BFN::ParserState *, ordered_set<const IR::BFN::Extract *>>
        extracts_by_state;
    for (auto& slice : {slice_a, slice_b}) {
        le_bitrange f_range;
        const auto* f = slice.field();
        auto& writes = field_to_states.field_to_writes.at(f);
        for (auto* write : writes) {
            if (auto* ext = write->to<IR::BFN::Extract>()) {
                // Ignore constant extracts
                if (!get_extract_range(ext)) continue;

                le_bitrange range;
                phv.field(ext->dest->field, &range);
                if (slice.range().overlaps(range)) {
                    auto *state = field_to_states.write_to_state.at(write);
                    extracts_by_state[state].emplace(ext);
                }
            }
        }
    }

    // Check the compatibility of extracts in each state
    for (auto& [_, extracts] : extracts_by_state) {
        std::vector<const IR::BFN::Extract*> extracts_vec;
        extracts_vec.reserve(extracts.size());
        extracts_vec.insert(extracts_vec.begin(), extracts.begin(), extracts.end());
        if (!check(extracts_vec)) {
            compatability.emplace(slices_a_b, false);
            return false;
        }
    }

    compatability.emplace(slices_a_b, true);
    return true;
}

// If any egress intrinsic metadata, e.g. "eg_intr_md.egress_port" is mirrored,
// the mirrored version will overwrite the original version in the EPB. Because Tofino
// parser only supports bitwise-or semantic on rewrite, we need to transform the
// egress intrinsic metadata parsing to avoid the bitwise-or cloberring in such case.
// See COMPILER-319 and test
// glass/testsuite/p4_tests/parde/COMPILER-319/mirror-eg_intr_md.p4
//
struct FixupMirroredIntrinsicMetadata : public PassManager {
    struct FindMirroredIntrinsicMetadata : public ParserInspector {
        const PhvInfo& phv;
        std::set<const PHV::Field*> mirrored_intrinsics;

        const IR::BFN::ParserState* check_mirrored = nullptr;
        const IR::BFN::ParserState* egress_entry_point = nullptr;
        const IR::BFN::ParserState* bridged = nullptr;
        const IR::BFN::ParserState* mirrored = nullptr;

        explicit FindMirroredIntrinsicMetadata(const PhvInfo& p) : phv(p) { }

        bool preorder(const IR::BFN::ParserState* state) override {
            if (state->name.startsWith("egress::$mirror_field_list_egress")) {
                for (auto stmt : state->statements) {
                    if (auto extract = stmt->to<IR::BFN::Extract>()) {
                        auto dest = phv.field(extract->dest->field);
                        if (dest->name.startsWith("egress::eg_intr_md")) {
                            mirrored_intrinsics.insert(dest);
                        }
                    }
                }
            } else if (state->gress == EGRESS && state->name == "$entry_point") {
                egress_entry_point = state;
            } else if (state->name == "egress::$check_mirrored") {
                check_mirrored = state;
            } else if (state->name == "egress::$mirrored") {
                mirrored = state;
            } else if (state->name == "egress::$bridged_metadata") {
                bridged = state;
            }

            return true;
        }
    };

    struct RemoveExtracts : public ParserTransform {
        const PhvInfo& phv;
        const FindMirroredIntrinsicMetadata& mim;

        std::vector<const IR::BFN::Extract*> extracts;

        RemoveExtracts(const PhvInfo& p, const FindMirroredIntrinsicMetadata& m) :
            phv(p), mim(m) { }

        IR::BFN::Extract* preorder(IR::BFN::Extract* extract) override {
            auto state = findContext<IR::BFN::ParserState>();

            if (state->name == "$mirror_entry_point") {
                auto dest = phv.field(extract->dest->field);
                if (mim.mirrored_intrinsics.count(dest)) {
                    extracts.push_back(extract);
                    return nullptr;
                }
            }

            return extract;
        }
    };

    struct ReimplementEgressEntryPoint : public ParserTransform {
        const PhvInfo& phv;
        const FindMirroredIntrinsicMetadata& mim;

        ReimplementEgressEntryPoint(const PhvInfo& p,
                                    const FindMirroredIntrinsicMetadata& m) : phv(p), mim(m) { }

        IR::BFN::ParserState* new_start = nullptr;
        IR::BFN::ParserState* mirror_entry = nullptr;
        IR::BFN::ParserState* bridge_entry = nullptr;

        struct InstallEntryStates : public ParserTransform {
            const IR::BFN::ParserState* mirror_entry;
            const IR::BFN::ParserState* bridge_entry;

            InstallEntryStates(const IR::BFN::ParserState* m, const IR::BFN::ParserState* b) :
                mirror_entry(m), bridge_entry(b) { }

            IR::BFN::Transition* postorder(IR::BFN::Transition* transition) override {
                auto next = transition->next;

                if (next) {
                    if (next->name == "egress::$bridged_metadata") {
                        transition->next = bridge_entry;
                    } else if (next->name == "egress::$mirrored") {
                        transition->next = mirror_entry;
                    }
                }

                return transition;
            }
        };

        void create_new_start_state() {
            new_start = mim.check_mirrored->clone();

            auto shift = get_state_shift(mim.egress_entry_point);
            new_start->statements = *(new_start->statements.apply(ShiftPacketRVal(-shift * 8)));
            new_start->selects = *(new_start->selects.apply(ShiftPacketRVal(-shift * 8)));

            bridge_entry = mim.egress_entry_point->clone();
            bridge_entry->name = "$bridge_entry_point"_cs;

            mirror_entry = mim.egress_entry_point->clone();
            mirror_entry->name = "$mirror_entry_point"_cs;

            RemoveExtracts re(phv, mim);
            mirror_entry = mirror_entry->apply(re)->to<IR::BFN::ParserState>()->clone();

            InstallEntryStates ies(mirror_entry, bridge_entry);
            new_start = new_start->apply(ies)->to<IR::BFN::ParserState>()->clone();

            auto to_bridge = new IR::BFN::Transition(match_t(), shift, mim.bridged);
            bridge_entry->transitions = { to_bridge };

            auto to_mirror = new IR::BFN::Transition(match_t(), shift, mim.mirrored);
            mirror_entry->transitions = { to_mirror };
        }

        profile_t init_apply(const IR::Node* root) override {
            if (!mim.mirrored_intrinsics.empty())
                create_new_start_state();

            return ParserTransform::init_apply(root);
        }

        IR::BFN::Parser* preorder(IR::BFN::Parser* parser) override {
            if (parser->gress == EGRESS && new_start) {
                parser->start = new_start;
            }

            return parser;
        }
    };

    explicit FixupMirroredIntrinsicMetadata(const PhvInfo& phv) {
        auto findMirroredIntrinsicMetadata = new FindMirroredIntrinsicMetadata(phv);
        auto reimplementEgressEntry =
            new ReimplementEgressEntryPoint(phv, *findMirroredIntrinsicMetadata);

        addPasses({
            findMirroredIntrinsicMetadata,
            reimplementEgressEntry
        });
    }
};

CheckParserMultiWrite::CheckParserMultiWrite(const PhvInfo& phv) : phv(phv) {
    auto parser_info = new CollectParserInfo;
    auto field_to_states = new MapFieldToParserStates(phv);
    auto infer_write_mode = new InferWriteMode(phv, *parser_info, *field_to_states);
    auto check_write_mode_consistency =
        new CheckWriteModeConsistency(phv, *field_to_states, *parser_info);
    auto fixup_mirrored_intrinsic = new FixupMirroredIntrinsicMetadata(phv);

    bool needs_fixup = Device::currentDevice() == Device::TOFINO &&
                       BackendOptions().arch == "v1model";

    addPasses({
        LOGGING(4) ? new DumpParser("before_check_parser_multi_write") : nullptr,
        needs_fixup ? fixup_mirrored_intrinsic : nullptr,
        LOGGING(4) ? new DumpParser("after_fixup_mirrorred_intrinsic_metadata") : nullptr,
        parser_info,
        field_to_states,
        infer_write_mode,
        field_to_states,
        check_write_mode_consistency,
        LOGGING(4) ? new DumpParser("after_check_parser_multi_write") : nullptr,
    });
}
