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

#ifndef EXTENSIONS_BF_P4C_MAU_ACTION_ANALYSIS_H_
#define EXTENSIONS_BF_P4C_MAU_ACTION_ANALYSIS_H_

#include "bf-p4c/ir/bitrange.h"
#include "bf-p4c/ir/tofino_write_context.h"
#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/mau/reduction_or.h"
#include "lib/safe_vector.h"

using namespace P4;

class PhvInfo;
struct TableResourceAlloc;

/** The purpose of this class is to look through the actions specified by the table and
 *  determine if they are Tofino compliant.  The analysis can be run before and after
 *  phv allocation, as well as before and after action data formats are decided.
 *
 *  Tofino compliant is defined in the following way
 *      - One write / up to 2 reads per action on a container
 *      - Only one source from action data, other can be from PHV
 *      - Alignment restrictions on these sources
 *      - Only one action per container
 *
 *  The pass also constructs a field_actions/container_actions structure, depending on before
 *  or after PHV allocation.  The container actions is especially useful, as it breaks the
 *  instruction into a container by container basis.  The instructions after instruction
 *  selection are done on a field by field basis, and thus having a container view of the
 *  actions is necessary for having a correct view of the action data requirements, as well
 *  as reshaping the instructions from field by field to container by container
 *
 *  This pass can run before and after PHV allocation, and before and after table placement.
 *  The pass is also designed to run when action format allocation is done before PHV
 *  allocation, but a PHV allocation has been completed
 */
class ActionAnalysis : public MauInspector, TofinoWriteContext {
 public:
    enum op_type_t { NONE=0, DST, SRC1, SRC2, SRC3 };
    static constexpr int LOADCONST_MAX = 21;
    static constexpr int CONST_SRC_MAX = 3;
    static constexpr int JBAY_CONST_SRC_MIN = 2;
    static constexpr int MAX_PHV_SOURCES = 2;
    /** A way to encapsulate the information contained within a single operand of an instruction,
     *  whether the instruction is read from or written to.  Also contains the information on
     *  what particular bits of the mask are encapsulated.
     */
    struct ActionParam {
        enum type_t { PHV, ACTIONDATA, CONSTANT, TOTAL_TYPES } type;
        const IR::Expression *expr;
        enum speciality_t { NO_SPECIAL, HASH_DIST, METER_COLOR, RANDOM, METER_ALU, STFUL_COUNTER }
             speciality = NO_SPECIAL;
        bool is_conditional = false;

        ActionParam() : type(PHV), expr(nullptr) {}
        ActionParam(type_t t, const IR::Expression *e)
            : type(t), expr(e) {}

        ActionParam(type_t t, const IR::Expression *e, speciality_t s)
            : type(t), expr(e), speciality(s) {}

        int size() const {
            BUG_CHECK(expr->type, "Untyped expression in backend action");
            return expr->type->width_bits();
        }

        le_bitrange range() const {
            if (auto sl = expr->to<IR::Slice>())
                return { static_cast<int>(sl->getL()), static_cast<int>(sl->getH()) };
            return {0, size() - 1};
        }

        friend std::ostream &operator<<(std::ostream &out, const ActionParam &);
        std::string to_string() const;

        // void dbprint(std::ostream &out) const;
        const IR::Expression *unsliced_expr() const;

        cstring get_type_string() const {
            if (type == PHV)             return "PHV"_cs;
            else if (type == ACTIONDATA) return "ACTIONDATA"_cs;
            else if (type == CONSTANT)   return "CONSTANT"_cs;
            return "INVALID_TYPE"_cs;
        }

        cstring get_speciality_string() const {
            if (speciality == NO_SPECIAL)         return "NO_SPECIAL"_cs;
            else if (speciality == HASH_DIST)     return "HASH_DIST"_cs;
            else if (speciality == METER_COLOR)   return "METER_COLOR"_cs;
            else if (speciality == RANDOM)        return "RANDOM"_cs;
            else if (speciality == METER_ALU)     return "METER_ALU"_cs;
            else if (speciality == STFUL_COUNTER) return "STFUL_COUNTER"_cs;
            return "INVALID_SPECIALITY"_cs;
        }
    };

    /** Information on the entire instruction, essentially what field is written and from which
     *  fields the field is written from.  These can be broken down and analyzed on a container
     *  by container basis.
     */
    struct FieldAction {
        bool write_found = false;
        cstring name;
        const IR::MAU::Instruction *instruction;
        ActionParam write;
        safe_vector<ActionParam> reads;
        void clear() {
            write_found = false;
            reads.clear();
        }

        void setWrite(ActionParam w) {
            write_found = true;
            write = w;
        }

        bool requires_split = false;
        bool constant_to_ad = false;

        // These instructions can be used on partially-overwritten containers in form
        // `X = X op const`, because inserting all 0s or all 1s in parts of the constant source
        // corresponding to the overwritten fields preserves their contents.
        bool is_bitwise_overwritable() const {
            return name == "and" || name == "or" || name == "xor" || name == "xnor";
        }

        bool is_single_shift() const {
            return name == "shru" || name == "shrs" || name == "shl";
        }

        bool is_funnel_shift() const {
            return name == "funnel-shift";
        }

        bool is_shift() const {
            return is_single_shift() || is_funnel_shift();
        }

        enum container_overwrite_t {
            // only bits in dst field slice are overwritten
            DST_ONLY,
            // only bits above dst field slice might get overwritten
            HIGHER_BITS,
            // operation might overwrite all bits in the dst container
            ALL_BITS
        };

        enum source_type_t { CONSTANT, ACTION_DATA_CONSTANT, OTHER };

        std::pair<container_overwrite_t, source_type_t> container_write_type() const;

        enum error_code_t {
            NO_PROBLEM = 0,
            READ_AFTER_WRITES = 1 << 0,
            REPEATED_WRITES = 1 << 1,
            MULTIPLE_ACTION_DATA = 1 << 2,
            DIFFERENT_OP_SIZE = 1 << 3,
            BAD_CONDITIONAL_SET = 1 << 4
        };
        unsigned error_code = 0;

        friend std::ostream &operator<<(std::ostream &out, const FieldAction &);
        std::string to_string() const;
        static std::set<unsigned> codesForErrorCases;
    };

    /** Information on the PHV fields and action data that are read by an individual
     *  field instruction.  write_bits correspond to the bits in the container being written
     *  and read_bits are the bits of one of the sources.  This information is used
     *  for verification and combining the PHV fields into MultiOperands.
     */
    struct Alignment {
        le_bitrange write_bits;
        le_bitrange read_bits;
        op_type_t read_src;
        Alignment(le_bitrange wb, le_bitrange rb, op_type_t rs = NONE)
            : write_bits(wb), read_bits(rb), read_src(rs) {}

        int right_shift(PHV::Container container) const;
        friend std::ostream &operator<<(std::ostream &out, const Alignment&);
    };

    /** Information on all PHV reads affecting a single container.  Again used for verification
     *  and combining fields into MultiOperand.  Understood to be the bits written and read by
     *  the entire container, rather than in individual instruction in the container.  From
     *  these total alignment structures, we can determine if deposit-fields or bitmasked-sets
     *  are possible/necessary
     */
    struct TotalAlignment {
        bool verbose = false;
        safe_vector<Alignment> indiv_alignments;
        bitvec direct_write_bits;
        bitvec direct_read_bits;
        bitvec unused_container_bits;

        // Determined during verify_alignment.  Only used for deposit-field instructions
        bitvec implicit_write_bits;
        bitvec implicit_read_bits;


        bitvec write_bits() const { return direct_write_bits | implicit_write_bits; }
        bitvec read_bits() const { return direct_read_bits | implicit_read_bits; }

        ///> The amount to bits to rotate right the write_bits in order to get the read_bits.
        ///> The deposit-field write shift is the opposite, i.e. the amount to rotate right the
        ///> read bits in order to get the write bits
        int right_shift = 0;
        bool is_src1 = false;

        void add_alignment(le_bitrange wb, le_bitrange rb) {
            indiv_alignments.emplace_back(wb, rb);
            direct_write_bits.setrange(wb.lo, wb.size());
            direct_read_bits.setrange(rb.lo, rb.size());
        }

        bool equiv_bit_totals() const {
            return direct_write_bits.popcount() == direct_read_bits.popcount();
        }

        bool aligned() const {
            return right_shift == 0;
        }

        bitvec df_src1_mask() const;
        bitvec df_src2_mask(PHV::Container container) const;
        bitvec brm_src_mask(PHV::Container container) const;
        bitvec byte_rotate_merge_byte_mask(PHV::Container container) const;
        bool contiguous() const;

        bool deposit_field_src1() const;
        bool deposit_field_src2(PHV::Container container) const;
        bool is_byte_rotate_merge_src(PHV::Container container) const;

        bool verify_individual_alignments(PHV::Container &container);
        bool is_wrapped_shift(PHV::Container container, int *lo = nullptr, int *hi = nullptr) const;
        void set_implicit_bits_from_mask(bitvec mask, PHV::Container container);
        void determine_df_implicit_bits(PHV::Container container);
        void determine_brm_implicit_bits(PHV::Container container, bitvec src1_mask);
        void implicit_bits_full(PHV::Container container);

        /// Returns a size that fits all the non-zero bits of writes. The writes need not be
        /// contiguous.
        int bitrange_cover_size() const {
            return write_bits().max().index() - write_bits().min().index() + 1;
        }

        bool bitrange_contiguous() const { return write_bits().is_contiguous(); }

        /// A size that fits all the write bits provided they are continuous, or -1 if they are
        /// not. Value -1 therefore indicates that deposit-field instruction cannot be used.
        int bitrange_contiguous_size() const {
            return bitrange_contiguous() ? bitrange_cover_size() : -1;
        }

        TotalAlignment operator |(const TotalAlignment &ta) {
            TotalAlignment rv;
            rv.indiv_alignments.insert(rv.indiv_alignments.end(), indiv_alignments.begin(),
                                       indiv_alignments.end());
            rv.indiv_alignments.insert(rv.indiv_alignments.begin(), ta.indiv_alignments.begin(),
                                       ta.indiv_alignments.end());

            rv.direct_write_bits = direct_write_bits | ta.direct_write_bits;
            rv.direct_read_bits = direct_read_bits | ta.direct_read_bits;
            rv.unused_container_bits = unused_container_bits | ta.unused_container_bits;
            rv.verbose = verbose | ta.verbose;
            return rv;
        }
        friend std::ostream &operator<<(std::ostream &out, const TotalAlignment&);
    };

    /** Information on the action data field contained within the instruction.  The action data
     *  could be affected by multiple individual fields.  Assumes that only one action data field
     *  appears in each instruction, as the ALU can only use one action data field.
     */
    struct ActionDataInfo {
        bool initialized = false;
        TotalAlignment alignment;

        cstring action_data_name;
        bool immediate = false;
        int start = -1;
        int total_field_affects = 0;
        int field_affects = 0;
        int size  = -1;
        bitvec specialities;

        void initialize(cstring adn, bool imm, int s, int tfa) {
            initialized = true;
            action_data_name = adn;
            immediate = imm;
            start = s;
            total_field_affects = tfa;
            field_affects = 1;
        }
    };

    /** Information on where a particular constant is used within a container instruction
     */
    struct ConstantPosition {
        unsigned value;
        le_bitrange range;
        ConstantPosition(unsigned v, le_bitrange r) : value(v), range(r) { }
    };

    /** Information on all constants within a single container instruction
     */
    struct ConstantInfo {
        bool initialized = false;
        TotalAlignment alignment;
        safe_vector<ConstantPosition> positions;
        int container_size = -1;
        unsigned build_constant();
        unsigned build_shiftable_constant();
        // built as part of the check_constant_to_actiondata function
        unsigned constant_value;
        bool signExtend = false;  // Only true if requires rotation in a deposit-field instruction
        unsigned valid_instruction_constant(int container_size) const;
    };


    /** Information on all of the individual reads and writes within a single PHV container
     *  in an action function. Essentially coordinate to all the action that can happen
     *  within a single VLIW ALU
     */
    struct ContainerAction {
        bool verbose = false;
        bool convert_instr_to_deposit_field = false;  ///> determined by tofino_compliance check
        bool convert_instr_to_bitmasked_set = false;  ///> determined by tofino_compliance_check
        bool convert_instr_to_byte_rotate_merge = false;
        bool total_overwrite_possible = false;

        bool is_deposit_field_variant = false;
        // If the src1 = dest, but isn't directly specified by the parameters.  Only necessary
        // when to_deposit_field = true
        bool implicit_src1 = false;  ///> determined by tofino_compliance_check
        // If the src2 = dest, but isn't directly specified by the parameters.  Only necessary
        // when to_deposit_field == true
        bool implicit_src2 = false;  ///> determined by tofino_compliance_check
        bool impossible = false;
        bool unhandled_action = false;
        bool constant_to_ad = false;

        ///> Eventually we want to craft correct backtrack/error messages from all
        ///> of these error messages.  In the meantime, used for tracking action
        ///> data issues
        enum error_code_t {
            NO_PROBLEM                          = 0,
            MULTIPLE_CONTAINER_ACTIONS          = (1 << 0),
            READ_PHV_MISMATCH                   = (1 << 1),
            ACTION_DATA_MISMATCH                = (1 << 2),
            CONSTANT_MISMATCH                   = (1 << 3),
            TOO_MANY_PHV_SOURCES                = (1 << 4),
            IMPOSSIBLE_ALIGNMENT                = (1 << 5),
            CONSTANT_TO_ACTION_DATA             = (1 << 6),
            MULTIPLE_ACTION_DATA                = (1 << 7),
            ILLEGAL_OVERWRITE                   = (1 << 8),
            BIT_COLLISION                       = (1 << 9),
            OPERAND_MISMATCH                    = (1 << 10),
            UNHANDLED_ACTION_DATA               = (1 << 11),
            DIFFERENT_READ_SIZE                 = (1 << 12),
            MAU_GROUP_MISMATCH                  = (1 << 13),
            PHV_AND_ACTION_DATA                 = (1 << 14),
            PARTIAL_OVERWRITE                   = (1 << 15),
            MULTIPLE_SHIFTS                     = (1 << 16),
            ILLEGAL_ACTION_DATA                 = (1 << 17),
            REFORMAT_CONSTANT                   = (1 << 18),
            UNRESOLVED_REPEATED_ACTION_DATA     = (1 << 19),
            ATTACHED_OUTPUT_ILLEGAL_ALIGNMENT   = (1 << 20),
            CONSTANT_TO_HASH                    = (1 << 21),
            ILLEGAL_MOCHA_OR_DARK_WRITE         = (1 << 22),
            BIT_COLLISION_SET                   = (1 << 23),
            MULTIPLE_SPECIALITIES               = (1 << 24)
        };
        unsigned error_code = NO_PROBLEM;
        static const std::vector<cstring> error_code_string_t;

        cstring name;
        const IR::MAU::Table *table_context = nullptr;
        ActionDataInfo adi;
        ConstantInfo ci;
        bitvec invalidate_write_bits;
        // This is for keeping information about extra 0 bits when one of the reads
        // is resized by "0 ++ <PHV>"
        TotalAlignment extra_resize_reads;


        ordered_map<PHV::Container, safe_vector<Alignment>> initialization_phv_alignment;
        // A container can be sourced multiple times, and thus this has become a multimap
        std::multimap<PHV::Container, TotalAlignment> phv_alignment;
        bool is_background_source = false;

        // Set of error codes for which we report an error in compilation.
        static std::set<unsigned> codesForErrorCases;

        int counts[ActionParam::TOTAL_TYPES] = {0, 0, 0};
        safe_vector<FieldAction> field_actions;

        ContainerAction() {}  // used to by std::map to build an default object
        ContainerAction(cstring n, const IR::MAU::Table *tbl) : name(n), table_context(tbl) {}

        int total_types() {
            return counts[ActionParam::PHV]
                 + counts[ActionParam::ACTIONDATA]
                 + counts[ActionParam::CONSTANT];
        }

        bool is_single_shift() const {
            return name == "shru" || name == "shrs" || name == "shl";
        }

        bool is_funnel_shift() const {
            return name == "funnel-shift";
        }

        bool is_shift() const {
            return is_single_shift() || is_funnel_shift();
        }

        /**
         * Each alignment comes from an individual manipulation of alloc_slice or single
         * contiguous slice of action format
         */
        int alignment_counts() const {
            int rv = adi.alignment.indiv_alignments.size() + ci.alignment.indiv_alignments.size();
            for (auto pa : phv_alignment)
                rv += pa.second.indiv_alignments.size();
            return rv;
        }

        int operands() const {
            if (name == "to-bitmasked-set" || is_single_shift())
                return 1;
            else if (is_funnel_shift())
                return 2;

            if (field_actions.size() == 0)
                BUG("Cannot call operands function on empty container process");
            return field_actions[0].reads.size();
        }

        int ad_sources() const {
            return std::min(1, counts[ActionParam::ACTIONDATA] + counts[ActionParam::CONSTANT]);
        }

        int read_sources() const {
            return ad_sources() + counts[ActionParam::PHV];
        }


        bool is_from_set() const {
            return name == "set" || name == "to-bitmasked-set";
        }

        bool has_ad_or_constant() const {
            return ad_sources() > 0;
        }

        bool partial_overwrite() const {
            return ((error_code & PARTIAL_OVERWRITE) != 0 && !convert_instr_to_deposit_field)
                    || convert_instr_to_bitmasked_set;
        }

        bool unresolved_ad() const {
            return (error_code & UNRESOLVED_REPEATED_ACTION_DATA) != 0;
        }

        bool ad_renamed() const {
            return adi.field_affects > 1 || unresolved_ad();
        }

        bool no_sources() const {
            return name == "invalidate";
        }

        void set_mismatch(ActionParam::type_t type);

        bool action_data_isolated() const {
            return !is_from_set();
        }

        bool set_invalidate_write_bits(le_bitrange write) {
            if (name != "invalidate") return false;
            invalidate_write_bits.setrange(write.lo, write.size());
            return true;
        }

        bool is_total_overwrite_possible() {
            return total_overwrite_possible && !is_shift();
        }

        // FIXME: Can potentially use rotational shifts at some point
        // bool is_contig_rotate(bitvec check, int &shift, int size);
        // bitvec rotate_contig(bitvec orig, int shift, int size);

        bool verify_mocha_and_dark(cstring &error_message, PHV::Container container);
        bool verify_speciality(cstring &error_message, PHV::Container container,
            cstring action_name);
        bool verify_shift(cstring &error_message, PHV::Container container, const PhvInfo &phv);
        bool verify_phv_mau_group(PHV::Container container);
        bool verify_one_alignment(TotalAlignment &tot_alignment, int size, int &unaligned_count,
            int &non_contiguous_count);
        void move_source_to_bit(safe_vector<int> &bit_uses, TotalAlignment &ta);
        bool verify_source_to_bit(int operands, PHV::Container container);

        bool verify_overwritten(PHV::Container container, const PhvInfo &phv);
        bool verify_only_read(const PhvInfo &phv, int num_source);
        bool verify_possible(cstring &error_message, PHV::Container container,
                             cstring action_name, const PhvInfo &phv);
        bool is_byte_rotate_merge(PHV::Container container, TotalAlignment &ad_alignment);
        bool verify_deposit_field_variant(PHV::Container container, TotalAlignment &ad_alignment);
        bool verify_set_alignment(PHV::Container, TotalAlignment &ad_alignment);

        void determine_src1();
        void determine_implicit_bits(PHV::Container container, TotalAlignment &ad_alignment);
        bool verify_alignment(PHV::Container &container);
        bitvec specialities() const;

        bitvec total_write() const;
        bool convert_constant_to_actiondata() const {
            return (error_code & CONSTANT_TO_ACTION_DATA) != 0;
        }

        bool convert_constant_to_hash() const {
            return (error_code & CONSTANT_TO_HASH) != 0;
        }

        bool is_commutative() const {
            return (name == "add")
               ||  (name == "addc")
               ||  (name == "saddu")
               ||  (name == "sadds")
               ||  (name == "minu")
               ||  (name == "mins")
               ||  (name == "maxu")
               ||  (name == "maxs")
               ||  (name == "setz")
               ||  (name == "nor")
               ||  (name == "xor")
               ||  (name == "nand")
               ||  (name == "and")
               ||  (name == "xnor")
               ||  (name == "or")
               ||  (name == "sethi");
        }

        bool verify_multiple_action_data() const;

        friend std::ostream &operator<<(std::ostream &out, const ContainerAction&);
        std::string to_string() const;
    };

    typedef ordered_map<const IR::MAU::Instruction *, FieldAction> FieldActionsMap;
    typedef std::map<PHV::Container, ContainerAction> ContainerActionsMap;

 private:
    const PhvInfo &phv;
    bool phv_alloc = false;
    bool ad_alloc = false;
    bool has_warning = false;
    bool has_error = false;
    bool allow_unalloc = false;
    bool sequential = true;  // Prior to BackendCopyPropagation actions are sequential.

    bool action_data_misaligned = false;
    bool verbose = false;
    bool error_verbose = false;

    FieldActionsMap *field_actions_map = nullptr;
    ContainerActionsMap *container_actions_map = nullptr;
    const IR::MAU::Table *tbl;
    const ReductionOrInfo &red_info;
    FieldAction field_action;
    ordered_set<std::pair<cstring, le_bitrange>> single_ad_params;
    ordered_set<std::pair<cstring, le_bitrange>> multiple_ad_params;

    void initialize_phv_field(const IR::Expression *expr);
    void initialize_action_data(const IR::Expression *expr);
    ActionParam::speciality_t classify_attached_output(const IR::MAU::AttachedOutput *);
    bool preorder(const IR::MAU::Action *) override { visitOnce(); return true; }
    bool preorder(const IR::MAU::Table *) override { visitOnce(); return true; }
    bool preorder(const IR::MAU::TableSeq *) override { visitOnce(); return true; }
    bool preorder(const IR::Annotation *) override { return false; }

    bool preorder(const IR::Slice *) override;
    bool preorder(const IR::MAU::ActionArg *) override;
    bool preorder(const IR::MAU::ConditionalArg *) override;
    bool preorder(const IR::Cast *) override;
    bool preorder(const IR::Expression *) override;
    bool preorder(const IR::Mux *) override;
    bool preorder(const IR::Member *) override;
    bool preorder(const IR::MAU::ActionDataConstant *) override;
    bool preorder(const IR::Constant *) override;
    bool preorder(const IR::MAU::AttachedOutput *) override;
    bool preorder(const IR::MAU::HashDist *) override;
    bool preorder(const IR::MAU::IXBarExpression *) override;
    bool preorder(const IR::MAU::RandomNumber *) override;
    bool preorder(const IR::MAU::StatefulAlu *) override;
    bool preorder(const IR::MAU::Instruction *) override;
    bool preorder(const IR::MAU::StatefulCall *) override;
    bool preorder(const IR::MAU::StatefulCounter *sc) override;
    bool preorder(const IR::MAU::Primitive *) override;
    void postorder(const IR::MAU::Instruction *) override;
    void postorder(const IR::MAU::Action *) override;
    bool preorder(const IR::BFN::ReinterpretCast* cast) override;

    bool initialize_invalidate_alignment(const ActionParam &write, ContainerAction &cont_action);
    bool initialize_alignment(const ActionParam &write, const ActionParam &read,
        const op_type_t read_src, ContainerAction &cont_action, cstring &error_message,
        PHV::Container container, cstring action_name);
    bool init_phv_alignment(const ActionParam &read, const op_type_t read_src,
                            ContainerAction &cont_action, le_bitrange write_bits,
                            PHV::Container container, cstring &error_message);
    bool init_special_alignment(const ActionParam &read, ContainerAction &cont_action,
        le_bitrange write_bits, cstring action_name, PHV::Container container);
    bool init_ad_alloc_alignment(const ActionParam &read, ContainerAction &cont_action,
        le_bitrange write_bits, cstring action_name, PHV::Container container);
    bool init_hash_constant_alignment(const ActionParam &read, ContainerAction &cont_action,
        le_bitrange write_bits, cstring action_name, PHV::Container container);
    bool init_constant_alignment(const ActionParam &read, ContainerAction &cont_action,
        le_bitrange write_bits, cstring action_name, PHV::Container container);
    bool init_simple_alignment(const ActionParam &read, ContainerAction &cont_action,
        le_bitrange write_bits);
    void initialize_constant(const ActionParam &read, ContainerAction &cont_action,
        le_bitrange write_bits, safe_vector<le_bitrange> &read_bits_brs);
    void build_phv_alignment(PHV::Container container, ContainerAction &cont_action);
    void determine_unused_bits(PHV::Container container, ContainerAction &cont_action);

    void check_constant_to_actiondata(ContainerAction &cont_action, PHV::Container container);
    void add_to_single_ad_params(ContainerAction &cont_action);
    void check_single_ad_params(ContainerAction &cont_action);

    void verify_conditional_set_without_phv(cstring action_name, FieldAction &fa);

    void verify_P4_action_for_tofino(cstring action_name);
    void verify_P4_action_without_phv(cstring action_name);
    void verify_P4_action_with_phv(cstring action_name);

    bool is_allowed_unalloc(const IR::Expression *e) {
        if (!allow_unalloc) return false;
        while (auto *sl = e->to<IR::Slice>()) e = sl->e0;
        return e->is<IR::TempVar>(); }

 public:
    const IR::Expression *isActionParam(const IR::Expression *expr,
        le_bitrange *bits_out = nullptr, ActionParam::type_t *type = nullptr);
    const IR::Expression *isStrengthReducible(const IR::Expression *expr);
    const IR::MAU::ActionArg *isActionArg(const IR::Expression *expr,
        le_bitrange *bits_out = nullptr);
    bool isReductionOr(ContainerAction &cont_action) const;

    bool misaligned_actiondata() {
        return action_data_misaligned;
    }

    void set_field_actions_map(FieldActionsMap *fam) {
        if (ad_alloc == true)
            return;
        field_actions_map = fam;
    }

    void set_container_actions_map(ContainerActionsMap *cam) {
        if (phv_alloc == false)
            return;
        container_actions_map = cam;
    }

    void set_verbose() { verbose = true; }
    void set_error_verbose() { error_verbose = true; }
    bool warning_found() const { return has_warning; }
    bool error_found() const { return has_error; }
    bool get_phv_alloc() const { return phv_alloc; }
    bool get_ad_alloc() const { return ad_alloc; }
    bool get_allow_unalloc() const { return allow_unalloc; }
    bool get_sequential() const { return sequential; }
    bool get_action_data_misaligned() const { return action_data_misaligned; }
    bool get_verbose() const { return verbose; }
    bool get_error_verbose() const { return error_verbose; }
    const IR::MAU::Table* get_table() const { return tbl; }
    const ContainerActionsMap* get_container_actions_map() const { return container_actions_map; }

    ActionAnalysis(const PhvInfo &p, bool pa, bool aa, const IR::MAU::Table *t,
            const ReductionOrInfo &ri, bool au = false, bool seq = true)
        : phv(p), phv_alloc(pa), ad_alloc(aa), allow_unalloc(au), sequential(seq), tbl(t),
          red_info(ri) {visitDagOnce = false;}
};

std::ostream &operator<<(std::ostream &out, const ActionAnalysis&);

#endif /* EXTENSIONS_BF_P4C_MAU_ACTION_ANALYSIS_H_ */
