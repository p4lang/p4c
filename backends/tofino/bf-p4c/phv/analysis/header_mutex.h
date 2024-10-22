/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_TOFINO_BF_P4C_PHV_ANALYSIS_HEADER_MUTEX_H_
#define BACKENDS_TOFINO_BF_P4C_PHV_ANALYSIS_HEADER_MUTEX_H_

#include "bf-p4c/ir/gateway_control_flow.h"
#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/parde/parser_dominator_builder.h"
#include "bf-p4c/parde/parser_info.h"
#include "bf-p4c/phv/analysis/build_mutex.h"
#include "ir/ir.h"
#include "ir/visitor.h"

enum HeaderState : uintmax_t {
    UNKNOWN = 0,
    INACTIVE = 1,
    // ACTIVE has to be larger than INACTIVE because later we want to sort header name and header
    // state pairs by header state value so that INACTIVE headers are ordered first.
    ACTIVE = 2
};
const uint state_size = 2;
const std::map<HeaderState, cstring> header_state_to_cstring = {
    {UNKNOWN, "UNKNOWN"_cs}, {INACTIVE, "INACTIVE"_cs}, {ACTIVE, "ACTIVE"_cs}};

/**
 * @brief Data structure containing information about which headers are present in the parser and
 * MAU, as well as additional properties for each header and mutual exclusivity information. The
 * passes in HeaderMutex fill this data structure in a specific order, thus the order of these
 * passes matter (e.g. FindConstEntryTable needs to run after passes which fill @p all_headers,
 * @p mau_headers, and @p parser_headers).
 */
struct HeaderInfo {
    ordered_set<cstring> parser_headers;
    ordered_set<cstring> mau_headers;
    ordered_set<cstring> all_headers;
    bitvec headers_always_encountered;
    ordered_map<cstring, bitvec> headers_always_encountered_before;
    ordered_map<cstring, bitvec> headers_always_encountered_after;
    ordered_map<cstring, bitvec> headers_always_not_encountered_if_header_not_encountered;
    ordered_map<const IR::MAU::Table *,
                ordered_map<cstring, std::vector<std::pair<cstring, HeaderState>>>>
        headers_live_during_action;
    SymBitMatrix mutually_exclusive_headers;

    size_t get_header_index(cstring header_name);
    cstring get_header_name(size_t header_index);
    void print_mutually_exclusive_headers();
    void pretty_print_bitvec(bitvec bv);
    void print_header_encounter_info();
    void print_headers_live_during_action();
    void clear();
};

/**
 * @brief Collect every header encountered in the parser and insert them into HeaderInfo.
 * Addtionally, produce header level SymBitMatrix, where keys are header index (position of header
 * in HeaderInfo.all_headers) and values are whether or not two headers are mutually exclusive.
 *
 * This pass is functionally identical to BuildParserOverlay, which produces a PHV::Field level
 * matrix, but also computes the header level matrix.
 */
class AddParserHeadersToHeaderMutexMatrix : public BuildMutex {
    HeaderInfo &header_info;
    bitvec headers_encountered;
    SymBitMatrix mutually_inclusive_headers;

    /// Ignore non-header fields.
    static bool ignore_field(const PHV::Field *field) {
        return !field || field->pov || field->metadata;
    }

    void mark(const PHV::Field *field);

    profile_t init_apply(const IR::Node *root) override;
    bool preorder(const IR::MAU::TableSeq *) override { return false; }
    bool preorder(const IR::BFN::Deparser *) override { return false; }
    void flow_merge(Visitor &other_) override;
    void flow_copy(::ControlFlowVisitor &other_) override;
    void end_apply() override;

 public:
    AddParserHeadersToHeaderMutexMatrix(PhvInfo &phv, const bitvec &neverOverlay,
                                        const PragmaNoOverlay &pragma, HeaderInfo &header_info)
        : BuildMutex(phv, neverOverlay, pragma, ignore_field), header_info(header_info) {}
    AddParserHeadersToHeaderMutexMatrix *clone() const override {
        return new AddParserHeadersToHeaderMutexMatrix(*this);
    }
};

/**
 * @brief MauInspector that can get the header name of the PHV field being visited in the member.
 */
class HeaderNameMauInspector : public MauInspector, public TofinoWriteContext {
 protected:
    PhvInfo &phv;
    cstring header_name(const IR::Member *member);
    const PHV::Field *get_phv_field(const IR::Expression *expression);

 public:
    explicit HeaderNameMauInspector(PhvInfo &phv) : phv(phv) {}
};

/**
 * @brief Find if any header $valid bits are read/written to and if parser errors are handled in
 * MAU. Additionally, if a field is found that belongs to a header not encountered in the parser,
 * insert that header into HeaderInfo.
 */
class FindPovAndParserErrorInMau : public HeaderNameMauInspector {
    HeaderInfo &header_info;
    bool &mau_contains_pov_read_write;
    bool &mau_handles_parser_error;

    ordered_set<const PHV::Field *> parser_err_fields;

    profile_t init_apply(const IR::Node *root) override;
    /**
     * @brief Returns true if field name contains "parser_err" (standard name of a field found in
     * the intrinsic metadata from the parser); otherwise returns false.
     */
    bool is_original_parser_err_field(const PHV::Field *field);
    /**
     * @brief Returns destination field of an assignment if one is being visited.
     */
    const PHV::Field *get_assigned_field();
    /**
     * @brief Returns true if this member is a field which is either the original parser error field
     * found in intrinsic metadata from the parser or a another field it was assigned to; otherwise
     * returns false.
     */
    bool is_parser_err(const PHV::Field *field);
    /**
     * @brief From @p expression, get a PHV field and use it to find "parser_err", read/writes to
     * POV bits, and new MAU headers (headers that are not parsed and that are added in MAU by using
     * setValid()).
     */
    bool preorder(const IR::Expression *expression) override;
    /**
     * @brief Output log messages if pass found "parser_err" or read/writes to POV bits.
     */
    void end_apply() override;

 public:
    FindPovAndParserErrorInMau(PhvInfo &phv, HeaderInfo &headers, bool &mau_contains_pov_read_write,
                               bool &mau_handles_parser_error)
        : HeaderNameMauInspector(phv),
          header_info(headers),
          mau_contains_pov_read_write(mau_contains_pov_read_write),
          mau_handles_parser_error(mau_handles_parser_error) {}
};

/**
 * @brief Add the headers added in MAU (not parsed) found by FindPovAndParserErrorInMau into the
 * header mutual exclusivity matrix.
 */
class AddMauHeadersToHeaderMutexMatrix : public Inspector {
    HeaderInfo &header_info;

    void add_mau_headers_to_mutual_exclusive_headers();
    profile_t init_apply(const IR::Node *root) override;

 public:
    explicit AddMauHeadersToHeaderMutexMatrix(HeaderInfo &header_info) : header_info(header_info) {}
};

/**
 * @brief Many PHV field mutexes have already been removed due to other constraints prior to this
 * pass. Therefore, though in theory two headers might be mutually exclusive, in practice they no
 * longer are as all of their respectives fields are not mutually exclusive with each other. If none
 * of the fields between two headers have a field mutex, clear the header level mutex from the
 * header mutual exclusivity matrix.
 */
class RemoveHeaderMutexesIfAllFieldsNotMutex : public Inspector {
    PhvInfo &phv;
    HeaderInfo &header_info;

    bool have_mutually_exclusive_fields(cstring header1, cstring header2);
    void remove_header_mutexes_if_all_fields_not_mutex();
    profile_t init_apply(const IR::Node *root) override;

 public:
    RemoveHeaderMutexesIfAllFieldsNotMutex(PhvInfo &phv, HeaderInfo &header_info)
        : phv(phv), header_info(header_info) {}
};

/**
 * @ingroup parde
 * @brief Based on dominators and post-dominators of parser states, determines which other headers
 * have also surely been encountered if a given header has been encountered and which other headers
 * have not been encountered if a given header has not been encountered.
 */
class FindParserHeaderEncounterInfo : public ParserDominatorBuilder {
    PhvInfo &phv;
    HeaderInfo &header_info;

    ordered_map<gress_t, ordered_map<const IR::BFN::ParserState *, ordered_set<cstring>>>
        state_to_headers;
    ordered_map<gress_t, ordered_map<cstring, std::set<const IR::BFN::ParserState *>>>
        header_to_states;

    static bool ignore_field(const PHV::Field *field) {
        return !field || field->pov || field->metadata;
    }
    bitvec get_headers_extracted_in_states(std::set<const IR::BFN::ParserState *> states);
    bitvec get_surely_not_extracted(std::set<const IR::BFN::ParserState *> not_visited_states);
    bitvec get_always_extracted();
    SymBitMatrix get_flipped_bit_matrix(SymBitMatrix matrix);

    profile_t init_apply(const IR::Node *root) override;
    bool preorder(const IR::BFN::Extract *extract) override;

    void build_header_encounter_maps();
    void end_apply() override;

 public:
    FindParserHeaderEncounterInfo(PhvInfo &phv, HeaderInfo &header_info)
        : phv(phv), header_info(header_info) {}
};

/**
 * @brief Find all tables which have a $valid or $stkvalid table key. If that table has const
 * entries, determine which headers are active during each of the table's actions and store this
 * information in HeaderInfo.
 */
class FindConstEntryTables : public HeaderNameMauInspector {
    HeaderInfo &header_info;

    bool has_const_entries(const IR::MAU::Table *table);
    bool preorder(const IR::Member *member) override;

    void end_apply() override;

 public:
    FindConstEntryTables(PhvInfo &phv, HeaderInfo &header_info)
        : HeaderNameMauInspector(phv), header_info(header_info) {}
};

/**
 * @brief Using transitive closure of table flow graph, determine which PHV fields are referenced
 * after a given table. This can be used to optimise ExcludeMAUNotMutexHeaders, as if a
 * header mutex is invalidated at a specific location
 */
class FieldLevelOptimisation : public Inspector {
    PhvInfo &phv;

    ordered_map<const IR::MAU::Table *, bitvec> next_table_bitvecs;
    ordered_map<const IR::MAU::Table *, bitvec> fields_referenced;
    ordered_map<const IR::MAU::Table *, int> table_to_index;
    ordered_map<int, const IR::MAU::Table *> index_to_table;

    bool preorder(const IR::MAU::Table *table);
    bool preorder(const IR::Expression *expression);

    ordered_set<const IR::MAU::Table *> get_tables(bitvec tables_bv);
    ordered_set<const PHV::Field *> get_fields(bitvec fields_bv);

    bitvec get_all_tables_after(const IR::MAU::Table *table);
    bitvec get_all_fields_referenced_after(const IR::MAU::Table *table);

    ordered_set<const IR::MAU::Table *> get_next_tables(const IR::MAU::Table *table);
    void build_next_table_bitvecs();
    void print_next_table_bitvecs();
    void print_fields_referenced();
    void end_apply() override;

 public:
    bool is_deparsed_or_referenced_later(const IR::MAU::Table *table, const PHV::Field *field);

    explicit FieldLevelOptimisation(PhvInfo &phv) : phv(phv) {}
};

/**
 * @brief Walk through MAU in control flow order and find header level mutually exclusivity in the
 * parser that could be invalidated during an action by keeping a record of all headers potentially
 * active at any given point using @p active_headers. If a read/write to $valid or $stkvalid occurs,
 * @p active_headers is updated conservatively.
 */
class ExcludeMAUNotMutexHeaders : public MauInspector,
                                  public BFN::GatewayControlFlow,
                                  public TofinoWriteContext {
    PhvInfo &phv;
    HeaderInfo &header_info;
    bool &mau_handles_parser_error;
    const PragmaMutuallyExclusive &pa_mutex;
    ordered_map<std::pair<int, int>, ordered_set<const IR::MAU::Table *>> &modified_where;
    FieldLevelOptimisation &field_level_optimisation;

    /* Data structure indicating which headers are active at a given point in MAU. Each header in
     * the program is represented by 2 bits in the vector and is stored in the same order than in
     * header_info.all_headers
     */
    bitvec active_headers;
    ordered_map<cstring, bitvec> extracted_headers;
    ordered_map<cstring, bitvec> not_extracted_headers;
    SymBitMatrix parser_mutex_headers;
    SymBitMatrix mutex_headers_modified;
    cstring visiting_gateway_row_tag;
    std::map<cstring, std::set<const IR::Expression *>> visiting_gateway_rows;
    const IR::MAU::Table *visiting = nullptr;
    cstring gress;

    const PHV::Field *get_phv_field(const IR::Expression *expression);
    HeaderState get_header_state(size_t header_index);
    HeaderState get_header_state(cstring header_name);
    void print_active_headers();
    std::string get_active_headers();
    void set_header_state(size_t header_index, HeaderState value);
    void set_header_state(cstring header_name, HeaderState value);

    bool preorder(const IR::BFN::Parser *parser) override {
        gress = toString(parser->gress);
        return false;
    }

    void pre_visit_table_next(const IR::MAU::Table *tbl, cstring tag) override;

    ExcludeMAUNotMutexHeaders *clone() const override {
        return new ExcludeMAUNotMutexHeaders(*this);
    }

    bool is_header(const PHV::Field *field) { return field && field->header() && !field->metadata; }
    bool is_set(const IR::MAU::Primitive *primitive);
    bool is_set_header_pov(const IR::MAU::Primitive *primitive);
    bool is_set_header_valid(const IR::MAU::Primitive *primitive);
    bool is_set_header_invalid(const IR::MAU::Primitive *primitive);
    bool is_set_header_pov_to_other_header_pov(const IR::MAU::Primitive *primitive);
    void init_active_headers();
    void init_extracted_headers();
    void init_not_extracted_headers();
    profile_t init_apply(const IR::Node *root) override;

    bool is_header_pov_lhs_constant_rhs_operation_relation(const IR::Operation_Relation *op_rel);
    std::optional<std::pair<cstring, HeaderState>> get_header_to_state_pair_from_operation_relation(
        const IR::Operation_Relation *op_rel);
    std::optional<std::pair<const IR::Expression *, cstring>> get_gateway_row();
    std::vector<std::pair<cstring, HeaderState>>
    get_all_header_to_state_pairs_from_gateway_row_expression(const IR::Expression *gre);
    bool preorder(const IR::Expression *) override;

    void clear_row(cstring header, ordered_map<cstring, bitvec> &bit_matrix);
    void clear_column(cstring header, ordered_map<cstring, bitvec> &bit_matrix);
    void process_set_invalid(cstring header);
    void record_modified_where(int i, int j);
    ordered_set<std::pair<cstring, cstring>> process_set_valid(cstring header,
                                                               HeaderState state = ACTIVE);
    void process_is_invalid(cstring header);
    void process_is_valid(cstring header);

    ordered_set<std::pair<cstring, cstring>> process_set_header_povs(const IR::MAU::Action *action);
    std::string get_active_headers_change_table(
        bitvec begin, bitvec mid, bitvec end, bool const_entries,
        ordered_set<std::pair<cstring, cstring>> mutexes_removed);
    bool preorder(const IR::MAU::Action *action) override;

    void bitwise_and_bit_matrices(ordered_map<cstring, bitvec> &bit_matrix,
                                  const ordered_map<cstring, bitvec> &other_bit_matrix);
    void merge_active_headers(bitvec other_active_headers);
    void flow_merge(Visitor &other_) override;
    void flow_copy(::ControlFlowVisitor &other_) override;

    bool can_preserve_field_mutex(
        const PHV::Field *field1, const PHV::Field *field2,
        const ordered_set<const IR::MAU::Table *> tables_affecting_header_mutex);
    void end_apply() override;

 public:
    ExcludeMAUNotMutexHeaders(
        PhvInfo &phv, HeaderInfo &header_info, bool &mau_handles_parser_error,
        const PragmaMutuallyExclusive &pa_mutex,
        ordered_map<std::pair<int, int>, ordered_set<const IR::MAU::Table *>> &modified_where,
        FieldLevelOptimisation &field_level_optimisation)
        : phv(phv),
          header_info(header_info),
          mau_handles_parser_error(mau_handles_parser_error),
          pa_mutex(pa_mutex),
          modified_where(modified_where),
          field_level_optimisation(field_level_optimisation) {}
};

/**
 * @brief After identifying which headers in the P4 program are mutually exclusive in the parser,
 * remove field mutexes between all fields of two headers if that mutual exclusivity is invalidated
 * later during MAU control flow.
 */
class HeaderMutex : public PassManager {
 private:
    HeaderInfo header_info;
    bool mau_contains_pov_read_write;
    bool mau_handles_parser_error;

    FieldLevelOptimisation field_level_optimisation;
    ordered_map<std::pair<int, int>, ordered_set<const IR::MAU::Table *>> modified_where;

 public:
    HeaderMutex(PhvInfo &phv, const bitvec &never_overlay, const PHV::Pragmas &pragmas)
        : field_level_optimisation(phv) {
        addPasses({new AddParserHeadersToHeaderMutexMatrix(phv, never_overlay,
                                                           pragmas.pa_no_overlay(), header_info),
                   new FindPovAndParserErrorInMau(phv, header_info, mau_contains_pov_read_write,
                                                  mau_handles_parser_error),
                   new AddMauHeadersToHeaderMutexMatrix(header_info),
                   new RemoveHeaderMutexesIfAllFieldsNotMutex(phv, header_info),
                   new PassIf(
                       [this]() {
                           // If there are no field mutexes to remove, the following
                           // passes would be redundant as they would do nothing.
                           return mau_contains_pov_read_write &&
                                  !header_info.mutually_exclusive_headers.empty();
                       },
                       {new FindParserHeaderEncounterInfo(phv, header_info),
                        new FindConstEntryTables(phv, header_info), &field_level_optimisation,
                        new ExcludeMAUNotMutexHeaders(phv, header_info, mau_handles_parser_error,
                                                      pragmas.pa_mutually_exclusive(),
                                                      modified_where, field_level_optimisation)})});
    }
};

#endif /* BACKENDS_TOFINO_BF_P4C_PHV_ANALYSIS_HEADER_MUTEX_H_  */
