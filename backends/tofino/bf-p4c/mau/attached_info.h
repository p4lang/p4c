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

#ifndef BF_P4C_MAU_ATTACHED_INFO_H_
#define BF_P4C_MAU_ATTACHED_INFO_H_

#include "bf-p4c/mau/mau_visitor.h"
#include "attached_entries.h"
#include "ir/pass_manager.h"
#include "lib/ordered_map.h"
#include "lib/ordered_set.h"

namespace ActionData {

using namespace P4;

class FormatType_t {
    // An "enum" that tracks the splitting of entries across stages for both the match table
    // and all indirect attached tables.  For each table (match and indirect attached), we
    // need to know if there are entries in this stage, earlier stages, and later stages.
    // That's 3 bits per table, but not all combinations make sense.  We store these packed
    // into a single uint32, with the bottom 3 bits for the match table and additional bits
    // for up to 9 attached tables (in practice there are never more than 2 or 3).
    //
    // We need this to be an easily comparable "enum" as we use it as a key for caches of
    // table layout formats, action data formats, and actions.
    //
    // A given table will end up with a different FormatType_t for each stage that it is
    // split across, which summarizes the view of the table from that stage -- which parts
    // of the table are allocated in the current stage and which other stages have parts that
    // the parts in this stage need to communicate with.  So for example, table with match(M),
    // stateful(S) and counter(C) split across three stages as
    //                        stage1 | stage2 | stage3
    //  FormatType_t            M    |  M+S+C |   S
    //  match    (bits 0..2):  -T-      ET-      E--
    //  stateful (bits 3..5):  --L      -TL      ET-
    //  counter  (bits 6..8):  --L      -T-      E--
    //
    // Many combinations do not make sense, so should not occur.  Attached table entries
    // can never be in a stage before match table entries, so if there are match entries in
    // later stages, all attached entries must also be in later stages.
    // Currently we never set match LATER_STAGE as nothing depends on it -- FormatTypes
    // that differ only in this bit will end up with the exact same possible table and
    // action formats.

    enum stage_t { EARLIER_STAGE = 1, THIS_STAGE = 2, LATER_STAGE = 4, MASK = 7,
                   ALL_ATTACHED = 01111111110, };
    uint32_t value;

 public:
    bool operator<(FormatType_t a) const { return value < a.value; }
    bool operator==(const FormatType_t &a) const { return value == a.value; }
    bool operator!=(const FormatType_t &a) const { return !(*this == a); }
    bool valid() const { return value != 0; }
    void invalidate() { value = 0; }
    void check_valid(const IR::MAU::Table *tbl = nullptr) const;  // sanity check for
                                                                  // insane combinations
    bool normal() const {  // valid format that does not require any action changes
        // either everything is in this stage OR there are no attached tabled
        return (value & ~(ALL_ATTACHED*THIS_STAGE)) == THIS_STAGE || (value > 0 && value <= MASK); }

    bool matchEarlierStage() const { return value & EARLIER_STAGE; }
    bool matchThisStage() const { return value & THIS_STAGE; }
    bool matchLaterStage() const { return value & LATER_STAGE; }

    int num_attached() const { return floor_log2(value)/3; }
    bool attachedEarlierStage(int idx) const { return (value >> 3*(idx+1)) & EARLIER_STAGE; }
    bool attachedThisStage(int idx) const { return (value >> 3*(idx+1)) & THIS_STAGE; }
    bool attachedLaterStage(int idx) const { return (value >> 3*(idx+1)) & LATER_STAGE; }
    bool anyAttachedEarlierStage() const { return (value & (ALL_ATTACHED*EARLIER_STAGE)) != 0; }
    bool anyAttachedThisStage() const { return (value & (ALL_ATTACHED*THIS_STAGE)) != 0; }
    bool anyAttachedLaterStage() const { return (value & (ALL_ATTACHED*LATER_STAGE)) != 0; }

    FormatType_t() : value(0) {}
    void initialize(const IR::MAU::Table *tbl, int entries, bool prev_stages,
                    const attached_entries_t &attached);
    // which attached tables are we tracking in the FormatType_t
    static bool track(const IR::MAU::AttachedMemory *at);
    static std::vector<const IR::MAU::AttachedMemory *> tracking(const IR::MAU::Table *);
    static FormatType_t default_for_table(const IR::MAU::Table *);
    friend std::ostream &operator<<(std::ostream &, FormatType_t);
    std::string toString() const;
};
}  // end namespace ActionData

class ValidateAttachedOfSingleTable : public MauInspector {
 public:
    enum addr_type_t { STATS, METER, ACTIONDATA, TYPES };
    using TypeToAddressMap = std::map<addr_type_t, IR::MAU::Table::IndirectAddress>;

 private:
    const IR::MAU::Table *tbl;
    TypeToAddressMap &ind_addrs;

    std::map<addr_type_t, const IR::MAU::BackendAttached *> users;

    cstring addr_type_name(addr_type_t type) {
        switch (type) {
            case STATS: return "stats"_cs;
            case METER: return "meter"_cs;
            case ACTIONDATA: return "action"_cs;
            default: return ""_cs;
        }
    }

    bool compatible(const IR::MAU::BackendAttached *, const IR::MAU::BackendAttached *);
    void free_address(const IR::MAU::AttachedMemory *am, addr_type_t type);

    bool preorder(const IR::MAU::Counter *cnt) override;
    bool preorder(const IR::MAU::Meter *mtr) override;
    bool preorder(const IR::MAU::StatefulAlu *salu) override;
    bool preorder(const IR::MAU::Selector *as) override;
    bool preorder(const IR::MAU::TernaryIndirect *) override {
        BUG("No ternary indirect should exist before table placement");
        return false; }
    bool preorder(const IR::MAU::ActionData *ad) override;
    bool preorder(const IR::MAU::IdleTime *) override {
        return false;
    }
    bool preorder(const IR::Attached *att) override {
        BUG("Unknown attached table type %s", typeid(*att).name());
    }


 public:
    ValidateAttachedOfSingleTable(TypeToAddressMap &ia, const IR::MAU::Table *t)
        : tbl(t), ind_addrs(ia) {}
};

/**
 * The address is contained in an entry if the address comes from overhead.  This is also
 * when it is necessary to pass it through PHV if the stateful ALU is in a different stage
 * than the match table
 *
 * Per flow enable bit needed on a table when:
 *    - A table has at least two actions that run on hit, where one action runs the stateful
 *      ALU, and another action does not
 *    - When a table is linked with a gateway, the gateway runs a no hit stateful ALU
 *      instruction.  This is only known during table placement.
 *
 * If the stateful ALU is in a different stage than the match table, the enable bit needs to
 * move through PHV if:
 *     - Only the first bullet point, as the by running the noop instruction from the gateway
 *       the enable bit will never run 
 *     - On at least one miss action, the stateful ALU does not run.
 *
 * Meter type is necessary on a table when:
 *     - A table has two actions where the stateful instruction type is different
 *     - A table is linked to a gateway, and the stateful ALU has more than one instruction
 *       across all tables
 *
 * If the stateful ALU is in a different stage than the match table, the type needs to move
 * through PHV if:
 *     - Only the first bullet point, as in the second, the type can just come from payload from
 *       a gateway running a hash action table
 *     - On the miss action, the stateful ALU has a different stateful instruction than
 *       any of the hit actions
 */

class PhvInfo;

/// Search for references to a set of AttachedMemory in instruction -- returns a bitmap of which
/// operands refer to the AttachedMemories
class HasAttachedMemory : public MauInspector {
    ordered_map<const IR::MAU::AttachedMemory *, unsigned> am_match;
    std::vector<unsigned> _found;
    unsigned _found_all = 0U;

    profile_t init_apply(const IR::Node *node) {
        auto rv = MauInspector::init_apply(node);
        _found_all = 0;
        _found.clear();
        _found.resize(am_match.size());
        return rv;
    }

    bool preorder(const IR::MAU::AttachedMemory *am) {
        unsigned mask = 1U;
        const Visitor::Context *ctxt = nullptr;
        if (findContext<IR::MAU::Primitive>(ctxt)) {
            BUG_CHECK(ctxt->child_index >= 0 && ctxt->child_index < 32, "mask overflow");
            mask <<= ctxt->child_index; }
        if (am_match.count(am)) {
            _found[am_match.at(am)] |= mask;
            _found_all |= mask; }
        return false;
    }

 public:
    HasAttachedMemory() = default;
    void add(const IR::MAU::AttachedMemory *am) {
        if (!am_match.count(am)) {
            unsigned idx = am_match.size();
            am_match[am] = idx; } }
    HasAttachedMemory(std::initializer_list<const IR::MAU::AttachedMemory *> am) {
        for (auto *a : am) add(a); }
    unsigned found() const { return _found_all; }
    unsigned found(const IR::MAU::AttachedMemory *am) const { return _found[am_match.at(am)]; }
    auto begin() const -> decltype(Keys(am_match).begin()) { return Keys(am_match).begin(); }
    auto end() const -> decltype(Keys(am_match).end()) { return Keys(am_match).end(); }
};

/**
 * The purpose of this pass is to gather the requirements of what needs to pass through
 * PHV if this stateful ALU portion is split from the match portion
 */
class SplitAttachedInfo : public PassManager {
    typedef ActionData::FormatType_t  FormatType_t;
    PhvInfo     &phv;

    // Can't use IR::Node * as keys in a map, as they change in transforms.  Names
    // are unique and stable, so use them instead.  However, the pointers in the
    // sets may be out-of-date and not refer to the current IR.  Turns out we only
    // ever use this to get a count of the number of tables (to detect shared attached
    // tables) so it doesn't matter for now...
    ordered_map<cstring, ordered_set<const IR::MAU::Table *>> attached_to_table_map;

    struct IndexTemp {
        const IR::TempVar *index = nullptr;
        const IR::TempVar *enable = nullptr;
        const IR::TempVar *type = nullptr;
    };
    std::map<cstring, IndexTemp>  index_tempvars;

    // Not linked via gateway with information
    struct AddressInfo {
        ///> If the ALU is run in every hit action of the table
        bool always_run_on_hit = true;
        ///> If the ALU is run on every miss action of the table
        bool always_run_on_miss = true;
        ///> The number of address bits stored in overhead
        int address_bits = 0;
        ///> The number of bis that are necessary to come through the hash engine
        int hash_bits = 0;
        ///> The number of meter types possible in the table's hit action.  Each action
        ///> currently supports one meter type, (i.e. which stateful instruction to run)
        ///> but different actions can have different types
        bitvec types_on_hit;
        ///> The number of meter types possible in a table's miss actions
        bitvec types_on_miss;
    };

    ordered_map<cstring, ordered_map<cstring, AddressInfo>> address_info_per_table;

    profile_t init_apply(const IR::Node *node) {
        auto rv = PassManager::init_apply(node);
        attached_to_table_map.clear();
        address_info_per_table.clear();
        cache.clear();          // actions might have changed without renaming
        return rv;
    }

    /**
     * Builds maps between tables and attached memories that can be split
     */
    class BuildSplitMaps : public MauInspector {
        SplitAttachedInfo &self;
        bool preorder(const IR::MAU::Table *) override;

     public:
        explicit BuildSplitMaps(SplitAttachedInfo &s) : self(s) {}
    };


    /**
     * For each action in a table, determine if the attached memory is enabled within
     * that action, and if enabled, what meter type each action required.  Used for
     * filling the AddressInfo of a table
     */
    class EnableAndTypesOnActions : public MauInspector {
        SplitAttachedInfo &self;
        bool preorder(const IR::MAU::Action *) override;

     public:
        explicit EnableAndTypesOnActions(SplitAttachedInfo &s) : self(s) {}
    };

    class ValidateAttachedOfAllTables : public MauInspector {
        SplitAttachedInfo &self;
        bool preorder(const IR::MAU::Table *) override;

     public:
        explicit ValidateAttachedOfAllTables(SplitAttachedInfo &s) : self(s) {}
    };

 public:
    explicit SplitAttachedInfo(PhvInfo &phv) : phv(phv) {
        addPasses({
            new BuildSplitMaps(*this),
            new ValidateAttachedOfAllTables(*this),
            new EnableAndTypesOnActions(*this)
        });
    }

    const ordered_set<const IR::MAU::Table *> &
    tables_from_attached(const IR::Attached *att) const {
        return attached_to_table_map.at(att->name); }

 private:
    int addr_bits_to_phv_on_split(const IR::MAU::Table *tbl,
                                  const IR::MAU::AttachedMemory *at) const;
    bool enable_to_phv_on_split(const IR::MAU::Table *tbl,
                                const IR::MAU::AttachedMemory *at) const;
    int type_bits_to_phv_on_split(const IR::MAU::Table *tbl,
                                  const IR::MAU::AttachedMemory *at) const;

    const IR::MAU::Instruction *pre_split_addr_instr(const IR::MAU::Action *act,
        const IR::MAU::Table *tbl, const IR::MAU::AttachedMemory *at);
    const IR::MAU::Instruction *pre_split_enable_instr(const IR::MAU::Action *act,
        const IR::MAU::Table *tbl, const IR::MAU::AttachedMemory *at);
    const IR::MAU::Instruction *pre_split_type_instr(const IR::MAU::Action *act,
        const IR::MAU::Table *tbl, const IR::MAU::AttachedMemory *at);

    // memoization cache for get/create_split_action
    typedef std::tuple<cstring /* table name */, cstring /* action name */, FormatType_t> memo_key;
    std::map<memo_key, const IR::MAU::Action *>  cache;

 public:
    const IR::Expression *split_enable(const IR::MAU::AttachedMemory *, const IR::MAU::Table *);
    const IR::Expression *split_index(const IR::MAU::AttachedMemory *, const IR::MAU::Table *);
    const IR::Expression *split_type(const IR::MAU::AttachedMemory *, const IR::MAU::Table *);

    const IR::MAU::Action *get_split_action(const IR::MAU::Action *act,
        const IR::MAU::Table *tbl, FormatType_t format_type);

 private:
    const IR::MAU::Action *create_split_action(const IR::MAU::Action *act,
        const IR::MAU::Table *tbl, FormatType_t format_type);
};

#endif  /* BF_P4C_MAU_ATTACHED_INFO_H_ */
