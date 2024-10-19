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

#include "allocate_parser_checksum.h"

#include <map>

#include "bf-p4c/common/utils.h"
#include "bf-p4c/parde/clot/clot_info.h"
#include "bf-p4c/parde/dump_parser.h"
#include "device.h"
#include "lib/bitrange.h"
#include "lib/cstring.h"

/** @addtogroup AllocateParserChecksums
 *  @{
 */

static ordered_set<const IR::BFN::ParserState *> compute_end_states(
    const IR::BFN::Parser *parser, const CollectParserInfo &parser_info,
    const ordered_set<const IR::BFN::ParserState *> &calc_states) {
    ordered_set<const IR::BFN::ParserState *> end_states;
    for (auto a : calc_states) {
        bool is_end = true;
        for (auto b : calc_states) {
            if (parser_info.graph(parser).is_ancestor(a, b)) {
                is_end = false;
                break;
            }
        }
        if (is_end) end_states.insert(a);
    }
    return end_states;
}

// Eliminate adds that do not have a terminal verify()
// Eliminate subtracts that do not have a terminal get()
class ComputeDeadParserChecksums : public ParserInspector {
 public:
    ordered_set<const IR::BFN::ParserPrimitive *> to_elim;

    ComputeDeadParserChecksums(const CollectParserInfo &parser_info,
                               const CollectParserChecksums &checksum_info)
        : parser_info(parser_info), checksum_info(checksum_info) {}

 private:
    bool preorder(const IR::BFN::ParserState *state) override {
        auto parser = findContext<IR::BFN::Parser>();

        auto descendants = parser_info.graph(parser).get_all_descendants(state);
        state_to_descendants[state] = descendants;

        return true;
    }

    // Does cp have its terminal primitive in this state?
    // For ChecksumAdd, the terminal type is ChecksumVerify.
    // For ChecksumSub, the terminal type is ChecksumResidualDeposit.
    bool has_terminal(const IR::BFN::ParserState *state, cstring decl,
                      const IR::BFN::ParserChecksumPrimitive *cp, bool aftercp = false) {
        bool skipped = !aftercp;

        if (!checksum_info.state_to_prims.count(state)) return false;

        for (auto p : checksum_info.state_to_prims.at(state)) {
            if (p->declName != decl) continue;

            if (p == cp) skipped = true;

            if (!skipped) continue;

            if (cp->is<IR::BFN::ChecksumAdd>() && p->is<IR::BFN::ChecksumVerify>()) return true;

            if (cp->is<IR::BFN::ChecksumSubtract>() && p->is<IR::BFN::ChecksumResidualDeposit>())
                return true;
        }
        return false;
    }

    bool is_dead(cstring decl, const IR::BFN::ParserChecksumPrimitive *p) {
        if (p->is<IR::BFN::ChecksumResidualDeposit>() || p->is<IR::BFN::ChecksumVerify>())
            return false;

        // Look for terminal primitive of p in its state and all descendant states.
        // p is dead if no terminal is found.

        auto state = checksum_info.prim_to_state.at(p);

        if (has_terminal(state, decl, p)) return false;

        auto descendants = state_to_descendants.at(state);

        for (auto desc : descendants)
            if (has_terminal(desc, decl, p)) return false;

        return true;
    }

    void end_apply() override {
        for (auto kv : checksum_info.decl_name_to_prims) {
            for (auto dp : kv.second) {
                for (auto csum : dp.second) {
                    if (is_dead(dp.first, csum)) {
                        to_elim.insert(csum);

                        LOG3("eliminate dead parser checksum primitive " << csum << " in "
                                                                         << csum->declName);
                    }
                }
            }
        }
    }

    ordered_map<const IR::BFN::ParserState *, ordered_set<const IR::BFN::ParserState *>>
        state_to_descendants;

    const CollectParserInfo &parser_info;
    const CollectParserChecksums &checksum_info;
};

class ElimDeadParserChecksums : public ParserModifier {
    const ordered_set<const IR::BFN::ParserPrimitive *> &to_elim;

 public:
    explicit ElimDeadParserChecksums(const ordered_set<const IR::BFN::ParserPrimitive *> &to_elim)
        : to_elim(to_elim) {}

    bool preorder(IR::BFN::ParserState *state) override {
        IR::Vector<IR::BFN::ParserPrimitive> newStatements;

        for (auto csum : state->statements)
            if (!to_elim.count(csum)) newStatements.push_back(csum);

        state->statements = newStatements;
        return true;
    }
};

struct ParserChecksumAllocator : public Visitor {
    ParserChecksumAllocator(AllocateParserChecksums &s, const CollectParserInfo &pi,
                            const CollectParserChecksums &ci)
        : self(s), parser_info(pi), checksum_info(ci) {}

    AllocateParserChecksums &self;
    const CollectParserInfo &parser_info;
    const CollectParserChecksums &checksum_info;

    const IR::Node *apply_visitor(const IR::Node *n, const char *) override {
        for (auto &kv : checksum_info.parser_to_decl_names) allocate(kv.first);

        return n;
    }

#define COMPARE_SRC(a, b, T)                                         \
    {                                                                \
        auto at = a->to<T>();                                        \
        auto bt = b->to<T>();                                        \
        if (at && bt) return at->source->range == bt->source->range; \
    }

#define COMPARE_DST(a, b, T)                             \
    {                                                    \
        auto at = a->to<T>();                            \
        auto bt = b->to<T>();                            \
        if (at && bt) return at->dest->equiv(*bt->dest); \
    }

#define OF_TYPE(a, b, T) (a->is<T>() && b->is<T>())

    // TODO boilerplates due to IR's lack of support of "deep" comparision

    bool is_equiv_src(const IR::BFN::ParserChecksumPrimitive *a,
                      const IR::BFN::ParserChecksumPrimitive *b) {
        COMPARE_SRC(a, b, IR::BFN::ChecksumAdd);
        COMPARE_SRC(a, b, IR::BFN::ChecksumSubtract);
        return false;
    }

    bool is_equiv_dest(const IR::BFN::ParserChecksumPrimitive *a,
                       const IR::BFN::ParserChecksumPrimitive *b) {
        COMPARE_DST(a, b, IR::BFN::ChecksumResidualDeposit);
        COMPARE_DST(a, b, IR::BFN::ChecksumVerify);
        return false;
    }

    bool is_equiv(const IR::BFN::ParserState *state, cstring declA, cstring declB) {
        std::vector<const IR::BFN::ParserChecksumPrimitive *> primsA, primsB;

        for (auto prim : checksum_info.state_to_prims.at(state)) {
            if (prim->declName == declA)
                primsA.push_back(prim);
            else if (prim->declName == declB)
                primsB.push_back(prim);
        }

        if (primsA.size() != primsB.size()) return false;

        int size = primsA.size();

        for (int i = 0; i < size; i++) {
            auto a = primsA[i];
            auto b = primsB[i];

            if (OF_TYPE(a, b, IR::BFN::ChecksumResidualDeposit) ||
                OF_TYPE(a, b, IR::BFN::ChecksumVerify)) {
                if (!is_equiv_dest(a, b)) return false;
            } else if (OF_TYPE(a, b, IR::BFN::ChecksumAdd) ||
                       OF_TYPE(a, b, IR::BFN::ChecksumSubtract)) {
                if (!is_equiv_src(a, b)) return false;
            } else {
                return false;
            }
        }

        return true;
    }

#undef COMPARE_SRC
#undef COMPARE_DST
#undef OF_TYPE

    std::string print_calc_states(const IR::BFN::Parser *parser, cstring declName) {
        std::stringstream ss;

        auto &calc_states = checksum_info.decl_name_to_states.at(parser).at(declName);
        auto type = checksum_info.get_type(parser, declName);

        ss << declName << ": (" << type << ")" << std::endl;
        for (auto s : calc_states) ss << "  " << s->name << std::endl;

        return ss.str();
    }

    bool can_share_hw_unit(const IR::BFN::Parser *parser, cstring declA, cstring declB) {
        // Two residual checksum calculations can share HW unit if
        //   1) no start state is ancestor of the other's start state
        //   2) overlapping states must have identical calculation

        // Two verification/CLOT checksum calculations can share HW unit if
        // their calculation states don't overlap

        auto &calc_states_A = checksum_info.decl_name_to_states.at(parser).at(declA);
        auto &calc_states_B = checksum_info.decl_name_to_states.at(parser).at(declB);

        if (is_residual(parser, declA) || is_residual(parser, declB)) {
            auto startA = get_start_states(parser, declA);
            auto startB = get_start_states(parser, declB);

            for (auto a : calc_states_A) {
                for (auto b : calc_states_B) {
                    if (a == b) {
                        if (is_residual(parser, declA) && is_residual(parser, declB)) {
                            if (!is_equiv(a, declA, declB)) {
                                return false;
                            }
                        } else {
                            return false;
                        }
                    }
                }
            }

            if (is_residual(parser, declA)) {
                for (auto a : startA)
                    for (auto b : startB)
                        if (parser_info.graph(parser).is_ancestor(a, b)) return false;
            }

            if (is_residual(parser, declB)) {
                for (auto b : startB)
                    for (auto a : startA)
                        if (parser_info.graph(parser).is_ancestor(b, a)) return false;
            }
        } else {
            for (auto a : calc_states_A) {
                for (auto b : calc_states_B) {
                    if (a == b) {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    std::vector<ordered_set<cstring>> compute_disjoint_sets(const IR::BFN::Parser *parser) {
        std::vector<ordered_set<cstring>> disjoint_sets;

        if (checksum_info.parser_to_residuals.count(parser)) {
            compute_disjoint_sets(parser, checksum_info.parser_to_residuals.at(parser),
                                  disjoint_sets);
        }

        if (checksum_info.parser_to_verifies.count(parser)) {
            compute_disjoint_sets(parser, checksum_info.parser_to_verifies.at(parser),
                                  disjoint_sets);
        }

        if (checksum_info.parser_to_clots.count(parser)) {
            compute_disjoint_sets(parser, checksum_info.parser_to_clots.at(parser), disjoint_sets);
        }

        return disjoint_sets;
    }

    void compute_disjoint_sets(const IR::BFN::Parser *parser, const ordered_set<cstring> &decls,
                               std::vector<ordered_set<cstring>> &disjoint_sets) {
        for (auto decl : decls) {
            bool found_union = false;
            for (auto &ds : disjoint_sets) {
                BUG_CHECK(!ds.empty(), "empty disjoint set?");
                for (auto other : ds) {
                    if (!can_share_hw_unit(parser, decl, other)) {
                        found_union = false;
                        LOG3(decl << " cannot share with " << other);
                        break;
                    } else {
                        found_union = true;
                        LOG3(decl << " can share with " << other);
                    }
                }
                if (found_union) {
                    ds.insert(decl);
                    break;
                }
            }
            if (!found_union) {
                ordered_set<cstring> newset;
                newset.insert(decl);
                disjoint_sets.push_back(newset);
                LOG5("Creating new disjoint set for decl:" << decl);
            }
        }
    }

    bool is_verification(const IR::BFN::Parser *parser, cstring decl) {
        if (checksum_info.parser_to_verifies.count(parser)) {
            return checksum_info.parser_to_verifies.at(parser).count(decl);
        }
        return false;
    }

    bool is_residual(const IR::BFN::Parser *parser, cstring decl) {
        if (checksum_info.parser_to_residuals.count(parser)) {
            return checksum_info.parser_to_residuals.at(parser).count(decl);
        }
        return false;
    }

    bool is_clot(const IR::BFN::Parser *parser, cstring decl) {
        if (checksum_info.parser_to_clots.count(parser)) {
            return checksum_info.parser_to_clots.at(parser).count(decl);
        }
        return false;
    }

    /// Returns the checksum calculation start state for declaration. A calculation
    /// may have mutiple start states, e.g. branches that extract same header. A state
    /// is a start state is no other state is its ancestor.
    ordered_set<const IR::BFN::ParserState *> get_start_states(const IR::BFN::Parser *parser,
                                                               cstring decl) {
        ordered_set<const IR::BFN::ParserState *> start_states;
        auto &calc_states = checksum_info.decl_name_to_states.at(parser).at(decl);
        for (auto a : calc_states) {
            bool is_start = true;
            for (auto b : calc_states) {
                if (parser_info.graph(parser).is_ancestor(b, a)) {
                    is_start = false;
                    break;
                }
            }
            if (is_start) {
                start_states.insert(a);
            }
        }
        return start_states;
    }

    /// Returns the checksum calculation end states for declaration. A calculation
    /// may have multiple end states, e.g. IPv4 with variable length options where
    /// each option length is its own parser state. A state is an end state is no
    /// other state is its descendant.
    ordered_set<const IR::BFN::ParserState *> get_end_states(const IR::BFN::Parser *parser,
                                                             cstring decl) {
        ordered_set<const IR::BFN::ParserState *> end_states;

        auto &calc_states = checksum_info.decl_name_to_states.at(parser).at(decl);
        return compute_end_states(parser, parser_info, calc_states);
    }

    bool contains_clot_checksum(const IR::BFN::Parser *parser, const ordered_set<cstring> &decls) {
        for (auto decl : decls) {
            if (is_clot(parser, decl)) return true;
        }

        return false;
    }

    void bind(const IR::BFN::Parser *parser, cstring csum, unsigned id) {
        self.decl_to_checksum_id[parser][csum] = id;
        self.checksum_id_to_decl[parser][id].insert(csum);
        self.decl_to_start_states[parser][csum] = get_start_states(parser, csum);
        self.decl_to_end_states[parser][csum] = get_end_states(parser, csum);
        LOG1("allocated parser checksum unit " << id << " to " << csum);
    }

    void fail(gress_t gress) {
        std::stringstream msg;

        unsigned avail = 0;

        // FIXME -- should be a Device or PardeSpec feature
        if (Device::currentDevice() == Device::TOFINO)
            avail = 2;
        else if (Device::currentDevice() == Device::JBAY)
            avail = 5;

        msg << "Ran out of checksum units on " << ::toString(gress) << " parser (" << avail
            << " available).";

        if (Device::numClots() > 0) msg << " CLOT checksums can only use unit 2-4.";

        ::fatal_error("%1%", msg.str());
    }

    void allocate(const IR::BFN::Parser *parser) {
        LOG1("need to allocate " << checksum_info.parser_to_decl_names.at(parser).size()
                                 << " logical checksum calculations to "
                                 << ::toString(parser->gress) << " parser");

        if (LOGGING(2)) {
            for (auto decl : checksum_info.parser_to_decl_names.at(parser))
                LOG2(print_calc_states(parser, decl));
        }

        // Tofino: 2 checksum units available per parser
        // Modes: Verification/Residual

        // JBay: 5 checksum units available per parser
        // Modes: Verification/Residual/CLOT
        // CLOT checksum can only use units 2-4

        // Residual checksum once used need to reserved to end of parsing
        // Verification/CLOT checksum can be reused when done

        // each disjoint set can be assigned to a HW unit
        auto disjoint_sets = compute_disjoint_sets(parser);

        LOG3("disjoint parser checksum sets: " << disjoint_sets.size());

        // now bind each set to a checksum unit

        if (Device::currentDevice() == Device::TOFINO) {
            unsigned id = 0;

            for (auto &ds : disjoint_sets) {
                if (id == 2) fail(parser->gress);
                for (auto s : ds) bind(parser, s, id);
                id++;
            }
        } else if (Device::numClots() > 0) {
            ordered_set<unsigned> clots;

            unsigned id = 2;  // allocate clot checksums first
            for (auto &ds : disjoint_sets) {
                if (id == 5) fail(parser->gress);

                if (contains_clot_checksum(parser, ds)) {
                    for (auto s : ds) bind(parser, s, id);
                    clots.insert(id);
                    id++;
                }
            }

            id = 0;  // now allocate the rest
            for (auto &ds : disjoint_sets) {
                if (!contains_clot_checksum(parser, ds)) {
                    while (clots.count(id)) id++;
                    if (id == 5) fail(parser->gress);
                    for (auto s : ds) bind(parser, s, id);
                    id++;
                }
            }

        } else {
            BUG("Unknown device");
        }
    }
};

/**
 * For CLOTs that contribute to checksum update at the deparser,
 * we need to compute the CLOT's portion of checksum in the parser
 * when we issue the CLOTs. So after CLOT allocation, we know which
 * fields are allocated to CLOTs, and of those fields, which contribute
 * to the checksum update. We then insert the checksum calculation
 * primitives for these fields into the parser state where they are
 * extracted.
 */
struct InsertParserClotChecksums : public PassManager {
    struct CollectClotChecksumFields : DeparserInspector {
        CollectClotChecksumFields(const PhvInfo &phv, const ClotInfo &clotInfo)
            : phv(phv), clotInfo(clotInfo) {}

        bool preorder(const IR::BFN::EmitChecksum *emitChecksum) override {
            // look for clot fields in deparser checksums

            for (auto source : emitChecksum->sources) {
                le_bitrange field_range;
                auto field = phv.field(source->field->field, &field_range);
                assoc::map<const PHV::FieldSlice *, Clot *, PHV::FieldSlice::Greater> *clot_map =
                    nullptr;
                if (clotInfo.is_readonly(field)) {
                    clot_map = clotInfo.slice_clots(field);
                } else {
                    clot_map = clotInfo.allocated_slices(field);
                }
                if (!clot_map) continue;
                for (auto slice_clot : *clot_map) {
                    auto clot = slice_clot.second;
                    checksum_field_slice_to_clot[field][slice_clot.first->range()] = clot;
                    checksum_field_slice_to_offset[field][slice_clot.first->range()] =
                        source->offset + field_range.hi - slice_clot.first->range().hi;
                }
            }

            return false;
        }
        // Maps checksum fields used in clot to range of each field slice allocated to clots
        // Each range of field slice maps to its corresponding clot
        ordered_map<const PHV::Field *, ordered_map<le_bitrange, const Clot *>>
            checksum_field_slice_to_clot;
        // Maps checksum field used in CLOT to range of each field slice allocated to clots
        // Each range of field size maps to their offset in deparser checksum fieldlist
        ordered_map<const PHV::Field *, ordered_map<le_bitrange, int>>
            checksum_field_slice_to_offset;
        const PhvInfo &phv;
        const ClotInfo &clotInfo;
    };

    struct CreateParserPrimitives : ParserInspector {
        const PhvInfo &phv;
        const CollectParserInfo &parser_info;
        const ClotInfo &clotInfo;
        const CollectClotChecksumFields &clot_checksum_fields;
        ordered_map<
            const IR::BFN::ParserState *,
            ordered_map<const Clot *, std::vector<const IR::BFN::ParserChecksumPrimitive *>>>
            state_to_clot_primitives;
        ordered_map<const IR::BFN::Parser *,
                    ordered_map<const Clot *, ordered_set<const IR::BFN::ParserState *>>>
            clot_to_states;

        CreateParserPrimitives(const PhvInfo &phv, const CollectParserInfo &parser_info,
                               const ClotInfo &clotInfo,
                               const CollectClotChecksumFields &clot_checksum_fields)
            : phv(phv),
              parser_info(parser_info),
              clotInfo(clotInfo),
              clot_checksum_fields(clot_checksum_fields) {}

        IR::BFN::ChecksumAdd *create_checksum_add(const Clot *clot, const IR::BFN::PacketRVal *rval,
                                                  bool swap) {
            cstring name = "$clot_"_cs + cstring::to_cstring(clot->tag) + "_csum"_cs;
            auto add = new IR::BFN::ChecksumAdd(name, rval, swap, false);
            return add;
        }

        IR::BFN::ChecksumDepositToClot *create_checksum_deposit(const Clot *clot) {
            cstring name = "$clot_"_cs + cstring::to_cstring(clot->tag) + "_csum"_cs;
            auto deposit = new IR::BFN::ChecksumDepositToClot(name);
            deposit->clot = clot;
            deposit->declName = name;
            return deposit;
        }

        bool preorder(const IR::BFN::ParserState *state) override {
            auto parser = findContext<IR::BFN::Parser>();
            auto &checksum_field_slice_to_clot = clot_checksum_fields.checksum_field_slice_to_clot;
            auto &checksum_field_slice_to_offset =
                clot_checksum_fields.checksum_field_slice_to_offset;
            for (auto stmt : state->statements) {
                if (auto extract = stmt->to<IR::BFN::ExtractClot>()) {
                    le_bitrange field_range;
                    auto dest = phv.field(extract->dest->field, &field_range);
                    auto rval = extract->source->to<IR::BFN::PacketRVal>();
                    if (!checksum_field_slice_to_clot.count(dest)) continue;
                    for (auto &range_clot : checksum_field_slice_to_clot.at(dest)) {
                        // check if the field slice is extracted in this stmt
                        if (field_range.lo > range_clot.first.hi ||
                            field_range.hi < range_clot.first.lo) {
                            continue;
                        }
                        auto clot = range_clot.second;
                        bool swap = false;
                        // If a field is on an even byte in the checksum operation field list
                        // but on an odd byte in the input buffer and vice-versa then swap is true.
                        if ((checksum_field_slice_to_offset.at(dest).at(range_clot.first) / 8) %
                                2 !=
                            (rval->range.lo / 8) % 2) {
                            swap = true;
                        }
                        auto add = create_checksum_add(clot, rval, swap);
                        state_to_clot_primitives[state][clot].push_back(add);
                        clot_to_states[parser][clot].insert(state);
                    }
                }
            }

            return true;
        }

        void end_apply() override {
            // insert terminal call at last state
            for (auto &parser_cs : clot_to_states) {
                for (auto &cs : parser_cs.second) {
                    auto clot = cs.first;
                    auto end_states = compute_end_states(parser_cs.first, parser_info, cs.second);
                    for (auto end : end_states) {
                        auto deposit = create_checksum_deposit(clot);
                        state_to_clot_primitives[end][clot].push_back(deposit);
                    }
                }
            }
        }
    };

    struct InsertParserPrimitives : ParserModifier {
        const CreateParserPrimitives &create_parser_prims;

        explicit InsertParserPrimitives(const CreateParserPrimitives &create_parser_prims)
            : create_parser_prims(create_parser_prims) {}

        bool preorder(IR::BFN::ParserState *state) override {
            auto orig = getOriginal<IR::BFN::ParserState>();

            if (create_parser_prims.state_to_clot_primitives.count(orig)) {
                for (auto &kv : create_parser_prims.state_to_clot_primitives.at(orig)) {
                    for (auto prim : kv.second) {
                        state->statements.push_back(prim);
                        LOG3("added " << prim << " to " << state->name);
                    }
                }
            }

            return true;
        }
    };

    InsertParserClotChecksums(const PhvInfo &phv, const CollectParserInfo &parser_info,
                              const ClotInfo &clot) {
        auto clot_checksum_fields = new CollectClotChecksumFields(phv, clot);
        auto create_checksum_prims =
            new CreateParserPrimitives(phv, parser_info, clot, *clot_checksum_fields);
        auto insert_checksum_prims = new InsertParserPrimitives(*create_checksum_prims);

        addPasses({clot_checksum_fields, create_checksum_prims, insert_checksum_prims});
    }
};

// There can be path in a parser which can lead to a situation where a checksum extraction
// happen but a start bit was never set. Such path when taken consumes an extractor which was
// unaccounted and can potentially cause parser lock ups. So the solution is to find such paths
// and duplicate it with the unaccounted checksum extraction removed.
//
// Consider the following parse graph. Assume a checksum calculation starts at state a and
// ends at d. If a path is taken from state b, then the checksum calculation
// would be end at state d, without any start.
//           a        b
//            \      /
//               c
//               |
//               d
// The following pass will create new states c' and d'. The only difference is
// that the new states no longer have the offending checksum calculations.
//          a            b
//          |            |
//          c            c'
//          |            |
//          d            d'
struct DuplicateStates : public ParserTransform {
    ordered_map<cstring, ordered_map<cstring, ordered_set<const IR::BFN::ParserState *>>>
        duplicate_path;
    std::map<std::pair<const IR::BFN::ParserState *, cstring>, IR::BFN::ParserState *>
        duplicate_states;
    AllocateParserChecksums &allocator;
    const CollectParserInfo &parser_info;
    const CollectParserChecksums &checksum_info;
    DuplicateStates(AllocateParserChecksums &allocator, const CollectParserInfo &parser_info,
                    const CollectParserChecksums &checksum_info)
        : allocator(allocator), parser_info(parser_info), checksum_info(checksum_info) {}

    // Finds the path that needs to be duplicated.
    bool add_state_to_duplicate(const IR::BFN::ParserState *state,
                                const ordered_set<const IR::BFN::ParserState *> &start_states,
                                const IR::BFN::Parser *parser,
                                ordered_set<const IR::BFN::ParserState *> &path) {
        if (start_states.count(state)) {
            return false;
        }
        if (!has_ancestor(parser, start_states, state)) {
            path.insert(state);
            return true;
        }
        auto predecessor = parser_info.graph(parser).predecessors();
        bool result = false;
        for (auto pred : predecessor.at(state)) {
            result |= add_state_to_duplicate(pred, start_states, parser, path);
            if (result) {
                path.insert(state);
            }
        }
        return result;
    }

    /// Is any state in "calc_states_" an ancestor of "dst"?
    bool has_ancestor(const IR::BFN::Parser *parser,
                      const ordered_set<const IR::BFN::ParserState *> &calc_states,
                      const IR::BFN::ParserState *dst) {
        for (auto s : calc_states)
            if (parser_info.graph(parser).is_ancestor(s, dst)) return true;
        return false;
    }

    void find_state_to_duplicate(const IR::BFN::Parser *parser, cstring decl) {
        if (!(checksum_info.is_residual(parser, decl))) return;
        ordered_set<const IR::BFN::ParserState *> start_states;
        auto end_states = allocator.decl_to_end_states[parser][decl];
        // get start state of all the residual checksums that uses same checksum engine
        // as decl
        unsigned id = allocator.decl_to_checksum_id[parser][decl];
        for (auto declName : allocator.checksum_id_to_decl[parser][id]) {
            if (!(checksum_info.is_residual(parser, declName))) return;
            auto s = allocator.decl_to_start_states[parser][declName];
            start_states.insert(s.begin(), s.end());
        }

        auto predecessor = parser_info.graph(parser).predecessors();
        ordered_set<const IR::BFN::ParserState *> path_to_duplicate;
        for (auto a : end_states) {
            if (start_states.count(a)) continue;
            add_state_to_duplicate(a, start_states, parser, path_to_duplicate);
            if (path_to_duplicate.size()) {
                auto first = *path_to_duplicate.begin();
                path_to_duplicate.erase(first);
                duplicate_path[first->name][decl].insert(path_to_duplicate.begin(),
                                                         path_to_duplicate.end());
            }
            // Clearing this set is required or else the pass does not work properly when
            // 1 checksum has 2 or more paths to duplicate.
            path_to_duplicate.clear();
        }
        return;
    }

    profile_t init_apply(const IR::Node *root) override {
        duplicate_states.clear();
        for (auto &kv : checksum_info.parser_to_decl_names) {
            for (auto decl : kv.second) {
                find_state_to_duplicate(kv.first, decl);
            }
        }
        return Transform::init_apply(root);
    }

    IR::BFN::ParserState *get_new_state(const IR::BFN::ParserState *state, cstring decl) {
        auto state_decl = std::make_pair(state, decl);
        if (duplicate_states.count(state_decl)) return duplicate_states.at(state_decl);
        auto new_state = new IR::BFN::ParserState(
            state->p4States, state->name + ".$duplicate_" + decl, state->gress);
        auto new_statements = new IR::Vector<IR::BFN::ParserPrimitive>();
        for (auto statement : state->statements) {
            if (auto pc = statement->to<IR::BFN::ParserChecksumPrimitive>()) {
                if (pc->declName == decl) continue;
            }
            new_statements->push_back(statement);
        }
        new_state->statements = *new_statements;
        new_state->transitions = state->transitions;
        new_state->selects = state->selects;
        duplicate_states.emplace(state_decl, new_state);
        return new_state;
    }

    IR::BFN::ParserState *make_path(const IR::BFN::ParserState *state, cstring decl,
                                    ordered_set<const IR::BFN::ParserState *> path) {
        auto new_state = get_new_state(state, decl);
        auto new_transitions = new IR::Vector<IR::BFN::Transition>();
        for (auto transition : new_state->transitions) {
            bool new_trans_added = false;
            for (auto path_state : path) {
                if (transition->next && path_state->equiv(*transition->next)) {
                    auto new_next_state = make_path(path_state, decl, path);
                    auto new_transition = new IR::BFN::Transition(
                        transition->value, transition->shift, new_next_state);
                    LOG3("Adding a new state: " << new_next_state->name);
                    new_transitions->push_back(new_transition);
                    new_trans_added = true;
                    break;
                }
            }
            if (!new_trans_added) {
                new_transitions->push_back(transition);
            }
        }
        new_state->transitions = *new_transitions;
        return new_state;
    }

    IR::Node *preorder(IR::BFN::Transition *transition) override {
        auto state = findContext<IR::BFN::ParserState>();
        if (!transition->next) return transition;
        if (duplicate_path.count(state->name)) {
            for (auto decl_state : duplicate_path.at(state->name)) {
                for (auto &next_state : decl_state.second) {
                    if (transition->next->equiv(*next_state)) {
                        transition->next =
                            make_path(next_state, decl_state.first, decl_state.second);
                    }
                }
            }
        }
        return transition;
    }
};

/** @} */  // end of group AllocateParserChecksums

AllocateParserChecksums::AllocateParserChecksums(const PhvInfo &phv, const ClotInfo &clot)
    : Logging::PassManager("parser"_cs, Logging::Mode::AUTO), checksum_info(parser_info) {
    auto collect_dead_checksums = new ComputeDeadParserChecksums(parser_info, checksum_info);
    auto elim_dead_checksums = new ElimDeadParserChecksums(collect_dead_checksums->to_elim);
    auto insert_clot_checksums = new InsertParserClotChecksums(phv, parser_info, clot);
    auto allocator = new ParserChecksumAllocator(*this, parser_info, checksum_info);

    addPasses({LOGGING(4) ? new DumpParser("before_parser_csum_alloc") : nullptr, &parser_info,
               &checksum_info, collect_dead_checksums, elim_dead_checksums, &parser_info,
               Device::numClots() > 0 ? insert_clot_checksums : nullptr,
               LOGGING(5) ? new DumpParser("after_insert_clot_csum") : nullptr, &parser_info,
               &checksum_info, allocator, new DuplicateStates(*this, parser_info, checksum_info),
               LOGGING(5) ? new DumpParser("after_adding_duplicate_states") : nullptr, &parser_info,
               &checksum_info, allocator,
               LOGGING(4) ? new DumpParser("after_parser_csum_alloc") : nullptr});
}
