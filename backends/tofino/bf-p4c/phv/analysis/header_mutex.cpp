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

#include "bf-p4c/phv/analysis/header_mutex.h"

#include <optional>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dominator_tree.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/reverse_graph.hpp>

#include "bf-p4c/common/table_printer.h"
#include "bf-p4c/logging/event_logger.h"

cstring get_header_state_as_cstring(HeaderState header_state) {
    BUG_CHECK(header_state_to_cstring.count(header_state), "%d is not a valid HeaderState.",
              header_state);
    return header_state_to_cstring.at(header_state);
}

size_t HeaderInfo::get_header_index(cstring header_name) {
    size_t index = std::distance(all_headers.begin(), all_headers.find(header_name));
    BUG_CHECK(index < all_headers.size(), "%s is not the name of a header in the program.",
              header_name);
    return index;
}

cstring HeaderInfo::get_header_name(size_t header_index) {
    BUG_CHECK(header_index < all_headers.size(),
              "Header index %d exceeds number of headers in the program (%d).", header_index,
              all_headers.size());
    auto it = all_headers.begin();
    std::advance(it, header_index);
    return *it;
}

void HeaderInfo::print_mutually_exclusive_headers() {
    std::stringstream out;
    for (size_t i = 0; i < all_headers.size(); i++) {
        for (size_t j = 0; j < all_headers.size(); j++) {
            out << mutually_exclusive_headers(i, j) << " ";
        }
        out << std::endl;
    }
    LOG1(out);
}

void HeaderInfo::pretty_print_bitvec(bitvec bv) {
    for (const auto &i : bv) LOG1(TAB2 << get_header_name(i));
}

void HeaderInfo::print_header_encounter_info() {
    LOG4("Header Encounter Info:");
    for (const auto &header : all_headers) {
        LOG1(header);
        if (headers_always_encountered_before.count(header)) {
            LOG6(TAB1 << "Headers always encountered before " << header << ":");
            pretty_print_bitvec(headers_always_encountered_before.at(header));
        }

        if (headers_always_encountered_after.count(header)) {
            LOG6(TAB1 << "Headers always encountered after " << header << ":");
            pretty_print_bitvec(headers_always_encountered_after.at(header));
        }

        if (headers_always_not_encountered_if_header_not_encountered.count(header)) {
            LOG6(TAB1 << "Headers always not encountered if " << header << " was not encountered:");
            pretty_print_bitvec(
                headers_always_not_encountered_if_header_not_encountered.at(header));
        }
    }
}

void HeaderInfo::print_headers_live_during_action() {
    if (headers_live_during_action.empty())
        LOG3(
            "Found no tables with header.isValid() match keys that have const entries.\n"
            "Therefore, could not infer additional information about which headers are active\n"
            "during some actions.");
    else
        LOG3(
            "Found tables with header.isValid() match keys that have const entries.\n"
            "Inferring additional information about which headers are active in actions in said\n"
            "tables.");
    for (const auto &kv1 : headers_live_during_action) {
        LOG4(TAB1 << "Table: " << kv1.first->name);
        for (const auto &kv2 : kv1.second) {
            LOG4(TAB2 << "Action: " << kv2.first);
            for (const auto &pairs : kv2.second) {
                LOG4(TAB3 << pairs.first << ": " << get_header_state_as_cstring(pairs.second));
            }
        }
    }
}

void HeaderInfo::clear() {
    parser_headers.clear();
    mau_headers.clear();
    all_headers.clear();
    headers_always_encountered.clear();
    headers_always_encountered_before.clear();
    headers_always_encountered_after.clear();
    headers_always_not_encountered_if_header_not_encountered.clear();
    headers_live_during_action.clear();
    mutually_exclusive_headers.clear();
}

void AddParserHeadersToHeaderMutexMatrix::mark(const PHV::Field *field) {
    BuildMutex::mark(field);
    if (!field || IgnoreField(field)) return;

    cstring header = field->header();
    header_info.parser_headers.insert(header);
    header_info.all_headers.insert(header);
    int i = header_info.get_header_index(header);
    headers_encountered[i] = true;
    mutually_inclusive_headers[i] |= headers_encountered;
}

Visitor::profile_t AddParserHeadersToHeaderMutexMatrix::init_apply(const IR::Node *root) {
    auto rv = BuildMutex::init_apply(root);
    header_info.clear();
    headers_encountered.clear();
    mutually_inclusive_headers.clear();
    return rv;
}

void AddParserHeadersToHeaderMutexMatrix::flow_merge(Visitor &other_) {
    AddParserHeadersToHeaderMutexMatrix &other =
        dynamic_cast<AddParserHeadersToHeaderMutexMatrix &>(other_);
    fields_encountered |= other.fields_encountered;
    mutually_inclusive |= other.mutually_inclusive;
    headers_encountered |= other.headers_encountered;
    mutually_inclusive_headers |= other.mutually_inclusive_headers;
}

void AddParserHeadersToHeaderMutexMatrix::flow_copy(::ControlFlowVisitor &other_) {
    AddParserHeadersToHeaderMutexMatrix &other =
        dynamic_cast<AddParserHeadersToHeaderMutexMatrix &>(other_);
    fields_encountered = other.fields_encountered;
    mutually_inclusive = other.mutually_inclusive;
    headers_encountered = other.headers_encountered;
    mutually_inclusive_headers = other.mutually_inclusive_headers;
}

void AddParserHeadersToHeaderMutexMatrix::end_apply() {
    LOG4("mutually exclusive headers:");
    for (auto it1 = headers_encountered.begin(); it1 != headers_encountered.end(); ++it1) {
        for (auto it2 = it1; it2 != headers_encountered.end(); ++it2) {
            if (*it1 == *it2) continue;
            if (mutually_inclusive_headers(*it1, *it2)) continue;

            header_info.mutually_exclusive_headers(*it1, *it2) = true;
            auto header1 = header_info.get_header_name(*it1);
            auto header2 = header_info.get_header_name(*it2);
            LOG4(TAB1 << "(" << header1 << ", " << header2 << ")");
        }
    }
    header_info.print_mutually_exclusive_headers();
}

/**
 * @brief From an IR::Member that is a header POV bit, get that header's name.
 */
cstring HeaderNameMauInspector::header_name(const IR::Member *member) {
    const PHV::Field *field = nullptr;
    if (member->member == "$valid") {
        auto *expression = member->to<IR::Expression>();
        field = phv.field(expression);
    } else if (member->member == "$stkvalid") {
        auto *expression = getParent<IR::Expression>();
        auto *alias_slice = expression->to<IR::BFN::AliasSlice>();
        field = phv.field(alias_slice->source);
    }
    cstring hdr_name = (field) ? field->header() : ""_cs;
    return hdr_name;
}

const PHV::Field *HeaderNameMauInspector::get_phv_field(const IR::Expression *expression) {
    const PHV::Field *field = phv.field(expression);
    // Special case for header stacks
    if (const IR::BFN::AliasSlice *alias_slice = expression->to<IR::BFN::AliasSlice>()) {
        const IR::Expression *source = alias_slice->source;
        field = phv.field(source);
    }
    return field;
}

Visitor::profile_t FindPovAndParserErrorInMau::init_apply(const IR::Node *root) {
    auto rv = Inspector::init_apply(root);
    mau_handles_parser_error = false;
    mau_contains_pov_read_write = false;
    parser_err_fields.clear();
    return rv;
}

bool FindPovAndParserErrorInMau::is_original_parser_err_field(const PHV::Field *field) {
    const char *str = nullptr;
    cstring name = field->name;
    str = name.find("parser_err");
    return str;
}

const PHV::Field *FindPovAndParserErrorInMau::get_assigned_field() {
    const auto *primitive = findContext<IR::MAU::Primitive>();
    if (!primitive) return nullptr;

    const PHV::Field *field = get_phv_field(primitive->operands[0]);
    return field;
}

bool FindPovAndParserErrorInMau::is_parser_err(const PHV::Field *field) {
    if (is_original_parser_err_field(field)) {
        parser_err_fields.insert(field);
        if (const PHV::Field *assigned_field_parser_err = get_assigned_field())
            parser_err_fields.insert(assigned_field_parser_err);
    }

    return parser_err_fields.count(field);
}

bool FindPovAndParserErrorInMau::preorder(const IR::Expression *expression) {
    const PHV::Field *field = get_phv_field(expression);
    if (!field) return true;

    // This is before field->metadata guard clause because of header stacks:
    //      expression = hdr.mpls.$stkvalid[A:A] --> field = hdr.mpls[A].$valid pov
    //      expression = hdr.mpls.$stkvalid[A:B] --> field = hdr.mpls.$stkvalid **meta** pov
    if (field->pov && (isRead() || isWrite())) mau_contains_pov_read_write = true;

    const auto *operation_relation = findContext<IR::Operation_Relation>();
    const auto *table_key = findContext<IR::MAU::TableKey>();
    if (is_parser_err(field) && (table_key || operation_relation)) mau_handles_parser_error = true;

    if (field->metadata) return true;

    if (cstring header = field->header()) {
        const auto &[_, new_header] = header_info.all_headers.insert(header);
        if (new_header) {
            LOG4("Found header added in MAU: " << header);
            header_info.mau_headers.insert(header);
        }
    }

    return true;
}

void FindPovAndParserErrorInMau::end_apply() {
    if (mau_contains_pov_read_write)
        LOG4("MAU contains at least 1 read or write to a $valid or $stkvalid bit.");
    if (mau_handles_parser_error)
        LOG4("Found parser error handled by conditional or table key in MAU.");
}

/**
 * @brief Add headers that were added in MAU (not parsed) to the mutually exclusive header matrix.
 * As they are invalid at the start of MAU, they are mutually exclusive to all headers in their
 * respective gress. Mutexes will be removed based on when the MAU header is setValid() in the MAU
 * control flow.
 */
void AddMauHeadersToHeaderMutexMatrix::add_mau_headers_to_mutual_exclusive_headers() {
    for (const auto &mau_header : header_info.mau_headers) {
        auto gress1 = mau_header.before(mau_header.find("::"));
        auto i = header_info.get_header_index(mau_header);
        for (const auto &header : header_info.all_headers) {
            auto gress2 = header.before(header.find("::"));
            auto j = header_info.get_header_index(header);

            if (gress1 != gress2 || mau_header == header) continue;
            header_info.mutually_exclusive_headers(i, j) = true;
        }
    }
}

Visitor::profile_t AddMauHeadersToHeaderMutexMatrix::init_apply(const IR::Node *root) {
    auto rv = Inspector::init_apply(root);
    add_mau_headers_to_mutual_exclusive_headers();
    return rv;
}

/**
 * @brief Check if two headers have any mutually exclusive PHV fields.
 */
bool RemoveHeaderMutexesIfAllFieldsNotMutex::have_mutually_exclusive_fields(cstring header1,
                                                                            cstring header2) {
    ordered_set<const PHV::Field *> header1_fields;
    ordered_set<const PHV::Field *> header2_fields;
    phv.get_hdr_fields(header1, header1_fields);
    phv.get_hdr_fields(header2, header2_fields);
    for (const auto &field1 : header1_fields) {
        for (const auto &field2 : header2_fields) {
            if (phv.isFieldMutex(field1, field2)) return true;
        }
    }

    return false;
}

/**
 * @brief If two headers have no mutually exclusive fields, remove the header level mutex from the
 * header mutual exclusivity matrix.
 */
void RemoveHeaderMutexesIfAllFieldsNotMutex::remove_header_mutexes_if_all_fields_not_mutex() {
    for (auto it1 = header_info.all_headers.begin(); it1 != header_info.all_headers.end(); ++it1) {
        for (auto it2 = std::next(it1); it2 != header_info.all_headers.end(); ++it2) {
            auto i = header_info.get_header_index(*it1);
            auto j = header_info.get_header_index(*it2);
            if (!header_info.mutually_exclusive_headers(i, j)) continue;
            if (have_mutually_exclusive_fields(*it1, *it2)) continue;
            header_info.mutually_exclusive_headers(i, j) = false;
        }
    }
}

Visitor::profile_t RemoveHeaderMutexesIfAllFieldsNotMutex::init_apply(const IR::Node *root) {
    auto rv = Inspector::init_apply(root);
    remove_header_mutexes_if_all_fields_not_mutex();
    return rv;
}

Visitor::profile_t FindParserHeaderEncounterInfo::init_apply(const IR::Node *root) {
    auto rv = ParserDominatorBuilder::init_apply(root);
    state_to_headers.clear();
    header_to_states.clear();

    return rv;
}

/**
 * @brief Associate each header to the parser states they are extracted in and vice versa, ignoring
 * fields that were ignored by AddParserHeadersToHeaderMutexMatrix.
 */
bool FindParserHeaderEncounterInfo::preorder(const IR::BFN::Extract *extract) {
    auto lval = extract->dest->to<IR::BFN::FieldLVal>();
    if (!lval) return true;

    auto *field = phv.field(lval->field);
    if (!field || ignore_field(field)) return true;

    if (field && field->header() && !field->metadata) {
        auto state = findContext<IR::BFN::ParserState>();
        auto header = field->header();
        if (state_to_headers[state->gress][state].insert(header).second)
            LOG1("State " << state->name << " extracts header " << header);
        if (header_to_states[state->gress][header].insert(state).second)
            LOG1("Header " << header << " extracted in state " << state->name);
        state_to_headers[state->gress][state].insert(header);
        header_to_states[state->gress][header].insert(state);
    }
    return false;
}

/**
 * @brief Get all headers extracted in a set of parser states as a bitvec.
 *
 * @param states A set of parser states
 * @return bitvec where headers that were extracted are set to 1, with others set to 0
 */
bitvec FindParserHeaderEncounterInfo::get_headers_extracted_in_states(
    std::set<const IR::BFN::ParserState *> states) {
    bitvec headers_extracted;

    for (const auto &state : states) {
        for (const auto &header : state_to_headers[state->gress][state]) {
            auto i = header_info.get_header_index(header);
            headers_extracted.setbit(i);
        }
    }
    return headers_extracted;
}

/**
 * @brief Get all headers that are **not** extracted in a set of parser states as a bitvec.
 *
 * @param not_visited_states A set of parser states
 * @return bitvec where headers that were **not** extracted are set to 1, with others set to 0
 */
bitvec FindParserHeaderEncounterInfo::get_surely_not_extracted(
    std::set<const IR::BFN::ParserState *> not_visited_states) {
    bitvec always_not_encountered = get_headers_extracted_in_states(not_visited_states);

    std::set<const IR::BFN::ParserState *> maybe_visited_states;
    std::set_difference(states.begin(), states.end(), not_visited_states.begin(),
                        not_visited_states.end(),
                        std::inserter(maybe_visited_states, maybe_visited_states.end()));
    for (const auto &state : maybe_visited_states) {
        for (const auto &header : state_to_headers[state->gress][state]) {
            auto i = header_info.get_header_index(header);
            always_not_encountered.clrbit(i);
        }
    }
    return always_not_encountered;
}

/**
 * @brief Get headers that are always extracted for all paths in a parser graph as a bitvec.
 *
 * @return bitvec where headers that were extracted are set to 1, with others set to 0
 */
bitvec FindParserHeaderEncounterInfo::get_always_extracted() {
    bitvec always_extracted;

    for (const auto &kv : parser_graphs) {
        auto end = kv.second.vertex_to_state.at(*kv.second.end);
        auto end_dominators = get_all_dominators(end, kv.first);
        auto entry_point = kv.second.parser->start;
        auto entry_point_post_dominators = get_all_post_dominators(entry_point);
        entry_point_post_dominators.insert(entry_point);
        BUG_CHECK(end_dominators == entry_point_post_dominators,
                  "Could not determine which parser states are always visited.");
        for (const auto &state : end_dominators) {
            for (const auto &header : state_to_headers[state->gress][state]) {
                auto i = header_info.get_header_index(header);
                always_extracted.setbit(i);
            }
        }
    }

    return always_extracted;
}

/**
 * @brief For every header, build 3 bitvectors corresponding to which other headers are encountered
 * when the header is encountered (before and after) and those that are surely not encountered when
 * that header is not encountered.
 */
void FindParserHeaderEncounterInfo::build_header_encounter_maps() {
    LOG3("Building header encounter maps.");
    bitvec always_encountered_before;
    bitvec always_encountered_after;
    bitvec always_not_encountered;
    header_info.headers_always_encountered = get_always_extracted();
    for (const auto &kv0 : header_to_states) {
        auto header_to_states_in_gress = kv0.second;
        for (const auto &kv1 : header_to_states_in_gress) {
            std::set<const IR::BFN::ParserState *> not_visited_states;
            auto header = kv1.first;
            auto states = kv1.second;
            always_encountered_before.clear();
            always_encountered_after.clear();
            always_not_encountered.clear();
            for (const auto &state : states) {
                // Encountered before
                auto before_states = get_all_dominators(state);
                before_states.insert(state);
                if (always_encountered_before.empty())
                    always_encountered_before = get_headers_extracted_in_states(before_states);
                else
                    always_encountered_before &= get_headers_extracted_in_states(before_states);
                // Encountered after
                auto after_states = get_all_post_dominators(state);
                after_states.insert(state);
                if (always_encountered_after.empty())
                    always_encountered_after = get_headers_extracted_in_states(after_states);
                else
                    always_encountered_after &= get_headers_extracted_in_states(after_states);
                // Not encountered
                auto dominatees = get_all_dominatees(state);
                auto post_dominatees = get_all_post_dominatees(state);
                not_visited_states.insert(state);
                not_visited_states.insert(dominatees.begin(), dominatees.end());
                not_visited_states.insert(post_dominatees.begin(), post_dominatees.end());
            }
            always_not_encountered = get_surely_not_extracted(not_visited_states);

            header_info.headers_always_encountered_before[header] = always_encountered_before;
            header_info.headers_always_encountered_after[header] = always_encountered_after;
            header_info.headers_always_not_encountered_if_header_not_encountered[header] =
                always_not_encountered;
        }
    }
    for (const auto &mau_header : header_info.mau_headers) {
        auto i = header_info.get_header_index(mau_header);
        bitvec none;
        header_info.headers_always_encountered_before.emplace(
            mau_header, header_info.headers_always_encountered);
        header_info.headers_always_encountered_before.at(mau_header).setbit(i);
        header_info.headers_always_encountered_after.emplace(mau_header, none);
        header_info.headers_always_encountered_after.at(mau_header).setbit(i);
        header_info.headers_always_not_encountered_if_header_not_encountered.emplace(mau_header,
                                                                                     none);
    }
}

void FindParserHeaderEncounterInfo::end_apply() {
    ParserDominatorBuilder::end_apply();
    build_header_encounter_maps();
    header_info.print_header_encounter_info();
}

bool FindConstEntryTables::has_const_entries(const IR::MAU::Table *table) {
    auto *p4table = (table->match_table) ? table->match_table : nullptr;
    auto *properties = (p4table) ? p4table->properties : nullptr;
    auto *entries_property =
        (properties) ? properties->getProperty(properties->entriesPropertyName) : nullptr;
    return (entries_property) ? entries_property->isConstant : false;
}

bool FindConstEntryTables::preorder(const IR::Member *member) {
    if (member->member != "$valid" && member->member != "$stkvalid") return false;

    const auto *table = findContext<IR::MAU::Table>();
    const auto *table_key = findContext<IR::MAU::TableKey>();
    if (!isRead() || !table_key || !has_const_entries(table)) return false;

    cstring header = header_name(member);
    auto it = std::find(table->match_key.begin(), table->match_key.end(), table_key);
    int i = it - table->match_key.begin();

    auto entries = table->entries_list->entries;
    for (const auto &entry : entries) {
        HeaderState new_state = UNKNOWN;
        if (auto *bool_literal = entry->getKeys()->components[i]->to<IR::BoolLiteral>())
            new_state = (bool_literal->value) ? ACTIVE : INACTIVE;
        cstring action = entry->getAction()->to<IR::MethodCallExpression>()->method->toString();
        auto &pairs = header_info.headers_live_during_action[table][action];
        auto is_same_header = [&header](const std::pair<cstring, HeaderState> &a) {
            return a.first == header;
        };
        auto existing_pair = std::find_if(pairs.begin(), pairs.end(), is_same_header);
        if (existing_pair != pairs.end()) {
            (*existing_pair).second =
                ((*existing_pair).second == new_state) ? (*existing_pair).second : UNKNOWN;
        } else {
            pairs.push_back(std::make_pair(header, new_state));
        }
    }
    return false;
}

void FindConstEntryTables::end_apply() {
    if (LOGGING(3)) header_info.print_headers_live_during_action();
}

bool FieldLevelOptimisation::preorder(const IR::MAU::Table *table) {
    auto index = table_to_index.size();
    table_to_index.emplace(table, index);
    index_to_table.emplace(index, table);
    fields_referenced.emplace(table, bitvec());
    return true;
}

bool FieldLevelOptimisation::preorder(const IR::Expression *expression) {
    auto *table = findContext<IR::MAU::Table>();
    auto *field = phv.field(expression);

    if (!table || !field) return true;
    fields_referenced[table][field->id] = true;

    return true;
}

/**
 * @brief From a bitvec of table indices, get a set of corresponding IR::MAU::Table objects.
 */
ordered_set<const IR::MAU::Table *> FieldLevelOptimisation::get_tables(bitvec tables_bv) {
    ordered_set<const IR::MAU::Table *> tables;
    for (const auto &index : tables_bv) {
        auto *table = index_to_table.at(index);
        tables.insert(table);
    }
    return tables;
}

/**
 * @brief From a bitvec of PHV field IDs, get a set of corresponding PHV::Field objects.
 */
ordered_set<const PHV::Field *> FieldLevelOptimisation::get_fields(bitvec fields_bv) {
    ordered_set<const PHV::Field *> fields;
    for (const auto &id : fields_bv) {
        auto *field = phv.field(id);
        fields.insert(field);
    }
    return fields;
}

/**
 * @brief Get the indices of all tables applied after a given table in the form of a bitvec. If
 * referenced, the bit at the index corresponding table_to_index.at(table) is set to 1.
 */
bitvec FieldLevelOptimisation::get_all_tables_after(const IR::MAU::Table *table) {
    bitvec tables_after;

    for (const auto &index : next_table_bitvecs.at(table)) {
        auto next_table = index_to_table.at(index);
        tables_after |= next_table_bitvecs.at(next_table);
        tables_after |= get_all_tables_after(next_table);
    }

    return tables_after;
}

/**
 * @brief Get the IDs of all PHV fields referenced after a given table in the form of a bitvec. If
 * referenced, the bit at the index corresponding to field->id is set to 1.
 */
bitvec FieldLevelOptimisation::get_all_fields_referenced_after(const IR::MAU::Table *table) {
    bitvec fields_referenced_after;

    bitvec bv = get_all_tables_after(table);
    ordered_set<const IR::MAU::Table *> all_tables_after = get_tables(bv);

    for (const auto &table_after : all_tables_after)
        fields_referenced_after |= fields_referenced.at(table_after);

    return fields_referenced_after;
}

/**
 * @brief For a given table, get all tables for each entry in table->next.
 */
ordered_set<const IR::MAU::Table *> FieldLevelOptimisation::get_next_tables(
    const IR::MAU::Table *table) {
    ordered_set<const IR::MAU::Table *> next_tables;
    for (const auto &kv : table->next) {
        for (const auto &next_table : kv.second->tables) {
            next_tables.insert(next_table);
        }
    }
    return next_tables;
}

/**
 * @brief For each table, build a bitvec representing table->next, where each next table in
 * table->next is set to 1 at the index it was assigned during the preorder(table) function.
 */
void FieldLevelOptimisation::build_next_table_bitvecs() {
    for (const auto &kv : table_to_index) {
        auto table = kv.first;
        auto next_tables = get_next_tables(table);
        next_table_bitvecs.emplace(table, bitvec());
        for (const auto &next_table : next_tables) {
            auto index = table_to_index.at(next_table);
            next_table_bitvecs[table][index] = true;
        }
    }
}

/**
 * @brief Print the content of private member variable next_table_bitvecs to the log.
 */
void FieldLevelOptimisation::print_next_table_bitvecs() {
    LOG_DEBUG7("Tables applied after each table:");
    for (const auto &kv : next_table_bitvecs) {
        auto table = kv.first;
        auto next_tables = get_tables(kv.second);
        LOG_DEBUG7(TAB1 << table->name);
        for (const auto &next_table : next_tables) {
            LOG_DEBUG7(TAB2 << next_table->name);
        }
    }
}

/**
 * @brief Print the content of private member variable fields_referenced to the log.
 */
void FieldLevelOptimisation::print_fields_referenced() {
    LOG_DEBUG7("PHV fields referenced by each table:");
    for (const auto &kv : fields_referenced) {
        auto table = kv.first;
        auto fields = get_fields(kv.second);
        LOG_DEBUG7(TAB1 << table->name);
        if (fields.empty()) LOG_DEBUG7(TAB2 << "NONE");
        for (const auto &field : fields) {
            LOG_DEBUG7(TAB2 << field);
        }
    }
}

void FieldLevelOptimisation::end_apply() {
    build_next_table_bitvecs();
    print_next_table_bitvecs();
    print_fields_referenced();
}

/**
 * @brief If a PHV field is referenced after a given table or it is deparsed, return true.
 */
bool FieldLevelOptimisation::is_deparsed_or_referenced_later(const IR::MAU::Table *table,
                                                             const PHV::Field *field) {
    bitvec referenced_field_ids = get_all_fields_referenced_after(table);
    bool referenced = referenced_field_ids[field->id];

    return field->deparsed() || referenced;
}

void ExcludeMAUNotMutexHeaders::print_active_headers() {
    std::stringstream out;
    for (size_t i = 0; i < header_info.all_headers.size(); i++) out << get_header_state(i) << " ";
    LOG7(out);
}

std::string ExcludeMAUNotMutexHeaders::get_active_headers() {
    std::stringstream out;
    for (const auto &header : header_info.all_headers) {
        auto header_state = get_header_state(header);
        if (header_state != UNKNOWN)
            out << TAB2 << header << ": " << get_header_state_as_cstring(header_state) << "\n";
    }
    out << TAB2 << "All other headers are in an unknown state.";
    return out.str();
}

/**
 * @brief Initialize the active headers bitvec based on whether or not MAU handles Parser Errors.
 */
void ExcludeMAUNotMutexHeaders::init_active_headers() {
    for (const auto &parser_header : header_info.parser_headers)
        set_header_state(parser_header, UNKNOWN);
    if (!mau_handles_parser_error)
        for (const auto &header_index : header_info.headers_always_encountered)
            set_header_state(header_index, ACTIVE);
    for (const auto &mau_header : header_info.mau_headers) set_header_state(mau_header, INACTIVE);
}

/**
 * @brief Store local copies of header parser encounter information that will be modified by this
 * pass.
 */
void ExcludeMAUNotMutexHeaders::init_extracted_headers() {
    extracted_headers = header_info.headers_always_encountered_before;
    for (auto &kv : extracted_headers)
        kv.second |= header_info.headers_always_encountered_after.at(kv.first);
}

/**
 * @brief Store local copies of header parser not encountered information that will be modified by
 * this pass.
 */
void ExcludeMAUNotMutexHeaders::init_not_extracted_headers() {
    not_extracted_headers = header_info.headers_always_not_encountered_if_header_not_encountered;
}

Visitor::profile_t ExcludeMAUNotMutexHeaders::init_apply(const IR::Node *root) {
    auto rv = Inspector::init_apply(root);
    modified_where.clear();
    active_headers.clear();
    init_active_headers();
    init_extracted_headers();
    init_not_extracted_headers();
    parser_mutex_headers = header_info.mutually_exclusive_headers;
    mutex_headers_modified.clear();
    visiting_gateway_row_tag = nullptr;
    visiting_gateway_rows.clear();
    return rv;
}

bool ExcludeMAUNotMutexHeaders::is_set(const IR::MAU::Primitive *primitive) {
    return (primitive->name == "set" && primitive->operands.size() > 1);
}

bool ExcludeMAUNotMutexHeaders::is_set_header_pov(const IR::MAU::Primitive *primitive) {
    if (!is_set(primitive)) return false;
    const PHV::Field *field = get_phv_field(primitive->operands[0]);
    return (field && field->header() && field->pov);
}

bool ExcludeMAUNotMutexHeaders::is_set_header_valid(const IR::MAU::Primitive *primitive) {
    const IR::Constant *constant =
        (is_set_header_pov(primitive)) ? primitive->operands[1]->to<IR::Constant>() : nullptr;
    return (constant && constant->value == 1);
}

bool ExcludeMAUNotMutexHeaders::is_set_header_invalid(const IR::MAU::Primitive *primitive) {
    const IR::Constant *constant =
        (is_set_header_pov(primitive)) ? primitive->operands[1]->to<IR::Constant>() : nullptr;
    return (constant && constant->value == 0);
}

bool ExcludeMAUNotMutexHeaders::is_set_header_pov_to_other_header_pov(
    const IR::MAU::Primitive *primitive) {
    const PHV::Field *field =
        (is_set_header_pov(primitive)) ? get_phv_field(primitive->operands[1]) : nullptr;
    return (field && field->header() && field->pov);
}

/**
 * @brief For a given @p action, find all setValid() and setInvalid() instructions. Finally, process
 * all setInvalids() first, then process setValids() last. The order operations is significant.
 * Given two mutually exclusive headers A and B, processing A.setInvalid() first will prevent
 * processing B.setValid() from removing the mutex between A and B.
 *
 * @param action An IR::MAU::Action on which to perform processing
 * @return ordered_set<std::pair<cstring, cstring>> All mutexes removed by setValids() in this
 * action
 */
ordered_set<std::pair<cstring, cstring>> ExcludeMAUNotMutexHeaders::process_set_header_povs(
    const IR::MAU::Action *action) {
    std::map<cstring, HeaderState> set_valids;
    std::set<cstring> set_invalids;
    for (const auto &primitive : action->action) {
        if (!is_set_header_pov(primitive)) continue;

        cstring lhs_header = get_phv_field(primitive->operands[0])->header();
        if (!header_info.all_headers.count(lhs_header)) continue;

        if (is_set_header_valid(primitive))
            set_valids.emplace(lhs_header, ACTIVE);
        else if (is_set_header_invalid(primitive))
            set_invalids.insert(lhs_header);
        else if (is_set_header_pov_to_other_header_pov(primitive)) {
            cstring rhs_header = get_phv_field(primitive->operands[1])->header();
            if (!header_info.all_headers.count(rhs_header)) continue;

            // Edge case: Header_Stack.(pop|push)_front(); LHS_Header_Stack = RHS_Header_Stack;
            const PHV::Field *lhs_field = get_phv_field(primitive->operands[0]);
            const PHV::Field *rhs_field = get_phv_field(primitive->operands[1]);
            if (lhs_field->metadata && lhs_field->pov && rhs_field->metadata && rhs_field->pov) {
                std::map<int, cstring> lhs_index_to_header_stack_element;
                std::map<int, cstring> rhs_index_to_header_stack_element;
                auto if_header_contains_substring_emplace_in_map = [](const cstring &header,
                                                                      const cstring &substring,
                                                                      std::map<int, cstring> &map) {
                    std::string tmp{header.c_str()};
                    auto begin = tmp.find("[");
                    auto end = tmp.find("]");
                    if (!header.find(substring) || begin == std::string::npos ||
                        end == std::string::npos)
                        return;
                    int index = std::stoi(tmp.substr(begin + 1, end - (begin + 1)));
                    map.emplace(index, header);
                };
                for (const cstring &header : header_info.all_headers) {
                    if_header_contains_substring_emplace_in_map(header, lhs_header,
                                                                lhs_index_to_header_stack_element);
                    if_header_contains_substring_emplace_in_map(header, rhs_header,
                                                                rhs_index_to_header_stack_element);
                }
                auto lhs_slice = primitive->operands[0]->to<IR::Slice>();
                auto rhs_slice = primitive->operands[1]->to<IR::Slice>();
                int shift_amount = rhs_slice->getH() - lhs_slice->getH();
                auto is_push_front = [&shift_amount]() { return shift_amount > 0; };
                auto is_pop_front = [&shift_amount]() { return shift_amount < 0; };
                auto is_assignment = [&shift_amount]() { return shift_amount == 0; };
                if (is_push_front()) {
                    for (const auto &[index, element] : lhs_index_to_header_stack_element) {
                        if (index < shift_amount) {
                            set_valids.emplace(element, ACTIVE);
                        } else if (rhs_index_to_header_stack_element.count(index - shift_amount)) {
                            const cstring &rhs_element =
                                rhs_index_to_header_stack_element.at(index - shift_amount);
                            auto rhs_state = get_header_state(rhs_element);
                            if (rhs_state == INACTIVE)
                                set_invalids.insert(element);
                            else
                                set_valids.emplace(element, rhs_state);
                        }
                    }
                } else if (is_pop_front()) {
                    for (const auto &[index, element] : lhs_index_to_header_stack_element) {
                        int size = lhs_index_to_header_stack_element.size();
                        if (index > (size - 1) + shift_amount) {
                            set_invalids.emplace(element);
                        } else if (rhs_index_to_header_stack_element.count(index - shift_amount)) {
                            const cstring &rhs_element =
                                rhs_index_to_header_stack_element.at(index - shift_amount);
                            auto rhs_state = get_header_state(rhs_element);
                            if (rhs_state == INACTIVE)
                                set_invalids.insert(element);
                            else
                                set_valids.emplace(element, rhs_state);
                        }
                    }
                } else if (is_assignment()) {
                    for (const auto &[index, element] : lhs_index_to_header_stack_element) {
                        if (rhs_index_to_header_stack_element.count(index)) {
                            const cstring &rhs_element =
                                rhs_index_to_header_stack_element.at(index);
                            auto rhs_state = get_header_state(rhs_element);
                            if (rhs_state == INACTIVE)
                                set_invalids.insert(element);
                            else
                                set_valids.emplace(element, rhs_state);
                        }
                    }
                }
                continue;
            }

            auto rhs_state = get_header_state(rhs_header);
            if (rhs_state == INACTIVE)
                set_invalids.insert(lhs_header);
            else
                set_valids.emplace(lhs_header, rhs_state);
        }
    }

    for (const auto &header : set_invalids) process_set_invalid(header);

    ordered_set<std::pair<cstring, cstring>> mutexes_removed;
    for (const auto &[header, state] : set_valids) {
        auto removals = process_set_valid(header, state);
        mutexes_removed.insert(removals.begin(), removals.end());
    }

    return mutexes_removed;
}

std::string ExcludeMAUNotMutexHeaders::get_active_headers_change_table(
    bitvec begin, bitvec mid, bitvec end, bool const_entries,
    ordered_set<std::pair<cstring, cstring>> mutexes_removed) {
    std::stringstream ss;
    TablePrinter table_printer(
        ss, {"Header", "Start of Action", "\"const entries\" Changes", "End of Action"},
        TablePrinter::Align::LEFT);
    for (size_t i = 0; i < header_info.all_headers.size(); i++) {
        auto header_name = header_info.get_header_name(i);
        if (!header_name.find(gress)) continue;

        auto header_index = i * state_size;
        auto begin_state =
            get_header_state_as_cstring(HeaderState(begin.getrange(header_index, state_size)));
        auto mid_state =
            get_header_state_as_cstring(HeaderState(mid.getrange(header_index, state_size)));
        auto end_state =
            get_header_state_as_cstring(HeaderState(end.getrange(header_index, state_size)));

        auto contains_header = [&header_name](const std::pair<cstring, cstring> &pair) {
            return pair.first == header_name || pair.second == header_name;
        };
        auto it = std::find_if(mutexes_removed.begin(), mutexes_removed.end(), contains_header);

        if (begin_state == "UNKNOWN" && begin_state == mid_state && begin_state == end_state &&
            it == mutexes_removed.end())
            continue;

        if (!const_entries || begin_state == mid_state) mid_state = "-"_cs;
        table_printer.addRow({std::string(header_name), std::string(begin_state),
                              std::string(mid_state), std::string(end_state)});
    }
    table_printer.print();
    return ss.str();
}

bool ExcludeMAUNotMutexHeaders::preorder(const IR::MAU::Action *action) {
    auto *table = getParent<IR::MAU::Table>();
    cstring action_name = action->name.toString();
    auto active_headers_begin = bitvec(active_headers);

    bool action_in_const_entry_table =
        header_info.headers_live_during_action.count(table) &&
        header_info.headers_live_during_action.at(table).count(action_name);
    if (action_in_const_entry_table) {
        // Ordering matters here. All INACTIVE headers must be processed before ACTIVE ones, or
        // else active_headers will not end up with the correct values.
        auto pairs = header_info.headers_live_during_action.at(table).at(action_name);
        auto compare_header_states = [](const std::pair<cstring, HeaderState> &a,
                                        const std::pair<cstring, HeaderState> &b) {
            return a.second < b.second;
        };
        std::sort(pairs.begin(), pairs.end(), compare_header_states);
        for (const auto &pair : pairs) {
            if (pair.second == INACTIVE)
                process_is_invalid(pair.first);
            else if (pair.second == ACTIVE)
                process_is_valid(pair.first);
        }
    }
    auto active_headers_middle = bitvec(active_headers);

    ordered_set<std::pair<cstring, cstring>> mutexes_removed = process_set_header_povs(action);
    auto active_headers_end = bitvec(active_headers);

    std::stringstream ar;
    TablePrinter action_report(ar, {"Header 1", "Header 2"}, TablePrinter::Align::LEFT);
    for (const auto &pair : mutexes_removed) {
        action_report.addRow({std::string(pair.first), std::string(pair.second)});
    }

    std::string seperator = std::string(100, '=');
    if (action_report.empty()) {
        LOG5(seperator);
        LOG5(action->name << "\n");
        LOG5("No header mutexes invalidated in this action.");
        LOG5(get_active_headers_change_table(active_headers_begin, active_headers_middle,
                                             active_headers_end, action_in_const_entry_table,
                                             mutexes_removed));
        LOG5(seperator);
    } else {
        LOG3(seperator);
        LOG3(action->srcInfo.toPositionString());
        LOG3(action->name << "\n");
        LOG3("Header mutexes invalidated by hdr.setValid() in this action:");
        action_report.print();
        LOG3(ar);

        if (action_in_const_entry_table) LOG3("This action is in a table with const entries.");
        LOG3(get_active_headers_change_table(active_headers_begin, active_headers_middle,
                                             active_headers_end, action_in_const_entry_table,
                                             mutexes_removed));
        LOG3(seperator);
    }

    return true;
}

/**
 * @brief Return true if @p op_rel's LHS is a header POV bit and the RHS is a constant
 * of value 0 or 1; return false otherwise.
 */
bool ExcludeMAUNotMutexHeaders::is_header_pov_lhs_constant_rhs_operation_relation(
    const IR::Operation_Relation *op_rel) {
    BUG_CHECK(op_rel, "Parameter passed to function was not an operation relation.");

    const PHV::Field *field = get_phv_field(op_rel->left);
    const auto *constant = op_rel->right->to<IR::Constant>();
    return field && field->pov && constant && (constant->value == 1 || constant->value == 0);
}

/**
 * @brief For an Equ or Neq operation relation @p op_rel who's LHS is a header POV bit and RHS
 * is a constant 0 or 1, return a pair containing the header's name and the header state derived
 * from the constant's value; return nothing otherwise.
 */
std::optional<std::pair<cstring, HeaderState>>
ExcludeMAUNotMutexHeaders::get_header_to_state_pair_from_operation_relation(
    const IR::Operation_Relation *op_rel) {
    if (!op_rel || !is_header_pov_lhs_constant_rhs_operation_relation(op_rel))
        return std::optional<std::pair<cstring, HeaderState>>();

    const PHV::Field *field = get_phv_field(op_rel->left);
    const auto *constant = op_rel->right->to<IR::Constant>();
    cstring header = field->header();

    const auto *neq = op_rel->to<IR::Neq>();
    const auto *equ = op_rel->to<IR::Equ>();
    if (neq && constant->value == 0)
        return std::optional<std::pair<cstring, HeaderState>>(std::make_pair(header, ACTIVE));
    else if (neq && constant->value == 1)
        return std::optional<std::pair<cstring, HeaderState>>(std::make_pair(header, INACTIVE));
    else if (equ && constant->value == 0)
        return std::optional<std::pair<cstring, HeaderState>>(std::make_pair(header, INACTIVE));
    else if (equ && constant->value == 1)
        return std::optional<std::pair<cstring, HeaderState>>(std::make_pair(header, ACTIVE));

    return std::optional<std::pair<cstring, HeaderState>>();
}

/**
 * @brief If the visitor is visiting a gateway, return the gateway row being visited.
 */
std::optional<std::pair<const IR::Expression *, cstring>>
ExcludeMAUNotMutexHeaders::get_gateway_row() {
    int index;
    if (const auto *table = gateway_context(index)) {
        if (visiting != table) {
            visiting = table;
            visiting_gateway_rows.clear();
        }

        const auto gateway_row = table->gateway_rows[index];
        LOG_DEBUG9("Currently visiting gateway expression of " << table->name << ":");
        LOG_DEBUG9(TAB1 << gateway_row.first << " " << gateway_row.second);
        return std::optional<std::pair<const IR::Expression *, cstring>>(gateway_row);
    }
    return std::optional<std::pair<const IR::Expression *, cstring>>();
}

/**
 * @brief For a given Boolean expression of format "A_0" or "A_0 && ... && A_n", return a set
 * of pairs sized 0 to N for all operation relations A that contains a Equ or Neq to a header
 * POV bit, where pair.first is the header's name and pair.second is the expected state of that
 * header.
 *
 * For example:
 *
 *     @p gateway_row_expression = ingress::hdr.vlan_tag.$stkvalid[1:1]alias:
 *                                 ingress::hdr.vlan_tag[0].$valid == 1 &&
 *                                 ingress::hdr.vlan_tag.$stkvalid[0:0]alias:
 *                                 ingress::hdr.vlan_tag[1].$valid != 1 &&
 *                                 (bit<1>)ingress::local_md.flags.port_vlan_miss == 0;
 *
 *     Returns { {"ingress::hdr.vlan_tag[0]", ACTIVE}, {ingress::hdr.vlan_tag[1], INACTIVE} }
 *
 * @param gateway_row_expression Boolean expression associated to a gateway's row
 * @return std::vector<std::pair<cstring, HeaderState>> Set header name to header state pairs
 */
std::vector<std::pair<cstring, HeaderState>>
ExcludeMAUNotMutexHeaders::get_all_header_to_state_pairs_from_gateway_row_expression(
    const IR::Expression *gateway_row_expression) {
    std::vector<std::pair<cstring, HeaderState>> header_to_state_pairs;
    if (const auto *l_and = gateway_row_expression->to<IR::LAnd>()) {
        auto left = get_all_header_to_state_pairs_from_gateway_row_expression(l_and->left);
        auto right = get_all_header_to_state_pairs_from_gateway_row_expression(l_and->right);
        auto compare = [](const std::pair<cstring, HeaderState> &a,
                          const std::pair<cstring, HeaderState> &b) { return a.first < b.first; };
        std::sort(left.begin(), left.end(), compare);
        std::sort(right.begin(), right.end(), compare);
        // Symmetric difference will omit headers that appear in both left and right, avoiding
        // situations where we could return a set containing 2 pairs of the same header and
        // conflicting states (both ACTIVE and INACTIVE at once).
        std::set_symmetric_difference(left.begin(), left.end(), right.begin(), right.end(),
                                      std::back_inserter(header_to_state_pairs), compare);
    } else if (const auto *op_rel = gateway_row_expression->to<IR::Operation_Relation>()) {
        auto pair = get_header_to_state_pair_from_operation_relation(op_rel);
        if (pair)
            header_to_state_pairs.push_back(*pair);
        else
            header_to_state_pairs.emplace_back(nullptr, UNKNOWN);
    }
    return header_to_state_pairs;
}

/**
 * @brief Frequently check if visitor has begun visiting a new gateway row. If yes, check if this
 * gateway row's expression contains any reads to header POV bits that could be used to infer those
 * headers' states.
 */
bool ExcludeMAUNotMutexHeaders::preorder(const IR::Expression *) {
    auto gateway_row = get_gateway_row();
    if (!gateway_row) return true;

    visiting_gateway_row_tag = gateway_row->second;
    visiting_gateway_rows[gateway_row->second].insert(gateway_row->first);
    return true;
}

void ExcludeMAUNotMutexHeaders::pre_visit_table_next(const IR::MAU::Table *tbl, cstring tag) {
    if (tbl != visiting) {
        visiting = nullptr;
        visiting_gateway_row_tag = nullptr;
        visiting_gateway_rows.clear();
    } else {
        std::map<const IR::Expression *, std::vector<std::pair<cstring, HeaderState>>>
            gateway_row_expression_to_header_to_state_pairs;
        LOG_DEBUG7("About to visit children of gateway " << tbl->name << " through tag " << tag);
        LOG_DEBUG7("Looking for header POV bits in expression:");
        for (const auto &expression : visiting_gateway_rows[visiting_gateway_row_tag]) {
            LOG_DEBUG7(TAB1 << expression << ": " << visiting_gateway_row_tag);
            auto pairs = get_all_header_to_state_pairs_from_gateway_row_expression(expression);
            gateway_row_expression_to_header_to_state_pairs.emplace(expression, pairs);
        }
        // Ordering matters here. All INACTIVE headers must be processed before ACTIVE ones, or
        // else active_headers will not end up with the correct values.
        auto compare_header_states = [](const std::pair<cstring, HeaderState> &a,
                                        const std::pair<cstring, HeaderState> &b) {
            return a.second < b.second;
        };
        for (auto &[_, pairs] : gateway_row_expression_to_header_to_state_pairs) {
            std::sort(pairs.begin(), pairs.end(), compare_header_states);
            if (visiting_gateway_row_tag != tag) std::reverse(pairs.begin(), pairs.end());
        }
        for (const auto &[_, pairs] : gateway_row_expression_to_header_to_state_pairs) {
            if (visiting_gateway_row_tag == tag) {
                for (const auto &[header, state] : pairs) {
                    if (!header && state == UNKNOWN) continue;
                    if (gateway_row_expression_to_header_to_state_pairs.size() > 1) {
                        set_header_state(header, UNKNOWN);
                        continue;
                    }
                    if (state == INACTIVE) {
                        LOG_DEBUG7(header << ": " << get_header_state_as_cstring(INACTIVE));
                        process_is_invalid(header);
                    } else if (state == ACTIVE) {
                        LOG_DEBUG7(header << ": " << get_header_state_as_cstring(ACTIVE));
                        process_is_valid(header);
                    }
                }
            } else if (visiting_gateway_row_tag != tag) {
                for (const auto &[header, state] : pairs) {
                    if (!header && state == UNKNOWN) continue;
                    if (pairs.size() > 1) {
                        set_header_state(header, UNKNOWN);
                        continue;
                    }
                    if (state == ACTIVE) {
                        LOG_DEBUG7(header << ": " << get_header_state_as_cstring(INACTIVE));
                        process_is_invalid(header);
                    } else if (state == INACTIVE) {
                        LOG_DEBUG7(header << ": " << get_header_state_as_cstring(ACTIVE));
                        process_is_valid(header);
                    }
                }
            }
        }
    }
}

void ExcludeMAUNotMutexHeaders::clear_row(cstring header,
                                          ordered_map<cstring, bitvec> &bit_matrix) {
    bit_matrix[header].clear();
}

void ExcludeMAUNotMutexHeaders::clear_column(cstring header,
                                             ordered_map<cstring, bitvec> &bit_matrix) {
    auto i = header_info.get_header_index(header);
    for (auto &row : bit_matrix) row.second.clrbit(i);
}

void ExcludeMAUNotMutexHeaders::process_set_invalid(cstring header) {
    LOG_DEBUG7("process_set_invalid(" << header << ")");
    clear_row(header, not_extracted_headers);
    clear_column(header, extracted_headers);
    set_header_state(header, INACTIVE);
}

void ExcludeMAUNotMutexHeaders::record_modified_where(int i, int j) {
    auto position1 = std::make_pair(i, j);
    auto position2 = std::make_pair(j, i);
    auto table = findContext<IR::MAU::Table>();
    modified_where[position1].insert(table);
    modified_where[position2].insert(table);
}

ordered_set<std::pair<cstring, cstring>> ExcludeMAUNotMutexHeaders::process_set_valid(
    cstring header, HeaderState state) {
    LOG_DEBUG7("process_set_valid(" << header << ", " << get_header_state_as_cstring(state) << ")");
    auto i = header_info.get_header_index(header);
    bitvec mutex_headers = parser_mutex_headers[i];

    if (pa_mutex.mutex_headers().count(header)) {
        for (const auto &mutex_header : pa_mutex.mutex_headers().at(header)) {
            set_header_state(mutex_header, INACTIVE);
            for (const auto &inactive_header : not_extracted_headers.at(mutex_header)) {
                set_header_state(inactive_header, INACTIVE);
            }
        }
    }

    ordered_set<std::pair<cstring, cstring>> mutexes_removed;
    for (const auto &j : mutex_headers) {
        auto mutex_header_state = get_header_state(j);
        if (mutex_header_state == ACTIVE || mutex_header_state == UNKNOWN) {
            mutex_headers_modified(i, j) = true;
            record_modified_where(i, j);
            cstring mutex_header = header_info.get_header_name(j);
            mutexes_removed.insert(std::make_pair(header, mutex_header));
        }
    }
    clear_row(header, extracted_headers);
    clear_column(header, not_extracted_headers);
    set_header_state(header, state);

    return mutexes_removed;
}

void ExcludeMAUNotMutexHeaders::process_is_invalid(cstring header) {
    LOG_DEBUG7("process_is_invalid(" << header << ")");
    bitvec row = not_extracted_headers[header];
    for (const auto &i : row) {
        set_header_state(i, INACTIVE);
    }
    set_header_state(header, INACTIVE);
}

void ExcludeMAUNotMutexHeaders::process_is_valid(cstring header) {
    LOG_DEBUG7("process_is_valid(" << header << ")");
    bitvec row = extracted_headers[header];
    for (const auto &i : row) {
        set_header_state(i, ACTIVE);
    }
    auto i = header_info.get_header_index(header);
    bitvec mutex = parser_mutex_headers[i];
    for (const auto &j : mutex) {
        if (mutex_headers_modified(i, j)) continue;
        set_header_state(j, INACTIVE);
    }
    set_header_state(header, ACTIVE);
}

const PHV::Field *ExcludeMAUNotMutexHeaders::get_phv_field(const IR::Expression *expression) {
    const PHV::Field *field = phv.field(expression);
    // Special case for header stacks
    if (const IR::BFN::AliasSlice *alias_slice = expression->to<IR::BFN::AliasSlice>()) {
        const IR::Expression *source = alias_slice->source;
        field = phv.field(source);
    }
    return field;
}

HeaderState ExcludeMAUNotMutexHeaders::get_header_state(size_t header_index) {
    return HeaderState(active_headers.getrange(header_index * state_size, state_size));
}

HeaderState ExcludeMAUNotMutexHeaders::get_header_state(cstring header_name) {
    auto header_index = header_info.get_header_index(header_name);
    return get_header_state(header_index);
}

void ExcludeMAUNotMutexHeaders::set_header_state(size_t header_index, HeaderState value) {
    auto header = header_info.get_header_name(header_index);
    LOG_DEBUG8(TAB1 << "set_header_state(" << header << ", " << get_header_state_as_cstring(value)
                    << ")");
    switch (value) {
        case UNKNOWN:
            active_headers.clrrange(header_index * state_size, state_size);
            break;
        case INACTIVE:
            active_headers.setbit(header_index * state_size);
            active_headers.clrbit(header_index * state_size + 1);
            break;
        case ACTIVE:
            active_headers.clrbit(header_index * state_size);
            active_headers.setbit(header_index * state_size + 1);
            break;
        default:
            BUG("%i is not a valid value for HeaderState.", value);
    }
}

void ExcludeMAUNotMutexHeaders::set_header_state(cstring header_name, HeaderState value) {
    auto header_index = header_info.get_header_index(header_name);
    set_header_state(header_index, value);
}

void ExcludeMAUNotMutexHeaders::merge_active_headers(bitvec other_active_headers) {
    active_headers &= other_active_headers;
}

void ExcludeMAUNotMutexHeaders::bitwise_and_bit_matrices(
    ordered_map<cstring, bitvec> &bit_matrix,
    const ordered_map<cstring, bitvec> &other_bit_matrix) {
    for (auto &row : bit_matrix)
        if (other_bit_matrix.count(row.first)) row.second &= other_bit_matrix.at(row.first);
}

void ExcludeMAUNotMutexHeaders::flow_merge(Visitor &other_) {
    ExcludeMAUNotMutexHeaders &other = dynamic_cast<ExcludeMAUNotMutexHeaders &>(other_);
    merge_active_headers(other.active_headers);
    bitwise_and_bit_matrices(extracted_headers, other.extracted_headers);
    bitwise_and_bit_matrices(not_extracted_headers, other.not_extracted_headers);
    parser_mutex_headers |= other.parser_mutex_headers;
    mutex_headers_modified |= other.mutex_headers_modified;
}

void ExcludeMAUNotMutexHeaders::flow_copy(::ControlFlowVisitor &other_) {
    ExcludeMAUNotMutexHeaders &other = dynamic_cast<ExcludeMAUNotMutexHeaders &>(other_);
    active_headers = other.active_headers;
    extracted_headers = other.extracted_headers;
    not_extracted_headers = other.not_extracted_headers;
    parser_mutex_headers = other.parser_mutex_headers;
    mutex_headers_modified = other.mutex_headers_modified;
}

/**
 * @brief Return true if field mutex between @p field1 and @p field2 can be preserved due to being
 * marked as padding/overlayable or if one of the fields is never referenced later in MAU and never
 * deparsed after all @p tables_affecting_header_mutex; false otherwise.
 */
bool ExcludeMAUNotMutexHeaders::can_preserve_field_mutex(
    const PHV::Field *field1, const PHV::Field *field2,
    const ordered_set<const IR::MAU::Table *> tables_affecting_header_mutex) {
    if (field1->padding || field2->padding) return true;
    if (field1->overlayable || field2->overlayable) return true;

    for (const auto &table : tables_affecting_header_mutex) {
        if (field_level_optimisation.is_deparsed_or_referenced_later(table, field1) &&
            field_level_optimisation.is_deparsed_or_referenced_later(table, field2)) {
            return false;
        }
    }
    return true;
}

void ExcludeMAUNotMutexHeaders::end_apply() {
    if (LOGGING(3)) {
        std::stringstream out;
        out << "Header mutual exclusivity matrix from parser analysis:" << std::endl;
        for (size_t i = 0; i < header_info.all_headers.size(); i++) {
            for (size_t j = 0; j < header_info.all_headers.size(); j++) {
                out << parser_mutex_headers(i, j) << " ";
            }
            out << std::endl;
        }
        out << "Header mutual exclusivity invalidated by MAU analysis:" << std::endl;
        for (size_t i = 0; i < header_info.all_headers.size(); i++) {
            for (size_t j = 0; j < header_info.all_headers.size(); j++) {
                out << mutex_headers_modified(i, j) << " ";
            }
            out << std::endl;
        }
        for (size_t i = 0; i < header_info.all_headers.size(); i++) {
            cstring header = header_info.get_header_name(i);
            LOG3(i << " = " << header << " "
                   << (header_info.mau_headers.count(header) ? "(MAU)" : ""));
        }
        LOG3(out);
    }

    std::stringstream parser_to_parser;
    TablePrinter parser_to_parser_removals(parser_to_parser,
                                           {"Header 1 (Parsed)", "Header 2 (Parsed)", "HDR1 Fields",
                                            "HDR2 Fields", "Tables Responsible"},
                                           TablePrinter::Align::LEFT);
    std::stringstream parser_to_mau;
    TablePrinter parser_to_mau_removals(parser_to_mau,
                                        {"Header 1 (Parsed)", "Header 2 (Added)", "HDR1 Fields",
                                         "HDR2 Fields", "Tables Responsible"},
                                        TablePrinter::Align::LEFT);
    std::stringstream mau_to_mau;
    TablePrinter mau_to_mau_removals(mau_to_mau,
                                     {"Header 1 (Added)", "Header 2 (Added)", "HDR1 Fields",
                                      "HDR2 Fields", "Tables Responsible"},
                                     TablePrinter::Align::LEFT);
    std::stringstream no_impact;
    TablePrinter no_impact_removals(no_impact, {"Header 1", "Header 2"}, TablePrinter::Align::LEFT);
    std::stringstream field_to_field;
    TablePrinter field_to_field_removals(field_to_field, {"Field 1", "Field 2"},
                                         TablePrinter::Align::LEFT);

    for (auto it1 = header_info.all_headers.begin(); it1 != header_info.all_headers.end(); ++it1) {
        for (auto it2 = std::next(it1); it2 != header_info.all_headers.end(); ++it2) {
            auto i = header_info.get_header_index(*it1);
            auto j = header_info.get_header_index(*it2);
            if (!mutex_headers_modified(i, j)) continue;
            if (!parser_mutex_headers(i, j)) continue;

            bool mau_header1 = header_info.mau_headers.count(*it1);
            bool mau_header2 = header_info.mau_headers.count(*it2);

            ordered_set<const PHV::Field *> header1_fields;
            ordered_set<const PHV::Field *> header2_fields;
            phv.get_hdr_fields(*it1, header1_fields);
            phv.get_hdr_fields(*it2, header2_fields);

            const auto tables_affecting_header_mutex = modified_where.at(std::make_pair(i, j));
            ordered_map<const PHV::Field *, ordered_set<const PHV::Field *>> header_fields_impacted;
            for (auto &field1 : header1_fields) {
                for (auto &field2 : header2_fields) {
                    if (!phv.isFieldMutex(field1, field2)) continue;

                    if (can_preserve_field_mutex(field1, field2, tables_affecting_header_mutex)) {
                        LOG4("Field mutex preserved: " << field1->name << ", " << field2->name);
                        continue;
                    }

                    phv.removeFieldMutex(field1, field2);
                    field_to_field_removals.addRow(
                        {std::string(field1->name), std::string(field2->name)});
                    header_fields_impacted[field1].insert(field2);
                }
            }

            // Building logging tables
            if (header_fields_impacted.empty()) {
                no_impact_removals.addRow({std::string(*it1), std::string(*it2)});
                continue;
            }
            field_to_field_removals.addSep();

            std::stringstream hdr1_fields_impacted;
            hdr1_fields_impacted << header_fields_impacted.size() << "/" << header1_fields.size();

            std::stringstream hdr2_fields_impacted;
            auto sum = 0;
            for (const auto &kv : header_fields_impacted) sum += kv.second.size();
            auto average = sum / header_fields_impacted.size();
            hdr2_fields_impacted << (average != header2_fields.size() ? "~" : "") << average << "/"
                                 << header2_fields.size();

            for (auto it = tables_affecting_header_mutex.begin();
                 it != tables_affecting_header_mutex.end(); ++it) {
                if (it == tables_affecting_header_mutex.begin()) {
                    if (!mau_header1 && !mau_header2) {
                        parser_to_parser_removals.addRow(
                            {std::string(*it1), std::string(*it2), hdr1_fields_impacted.str(),
                             hdr2_fields_impacted.str(), std::string((*it)->name)});
                    } else if (!mau_header1 && mau_header2) {
                        parser_to_mau_removals.addRow(
                            {std::string(*it1), std::string(*it2), hdr1_fields_impacted.str(),
                             hdr2_fields_impacted.str(), std::string((*it)->name)});
                    } else if (mau_header1 && mau_header2) {
                        mau_to_mau_removals.addRow(
                            {std::string(*it1), std::string(*it2), hdr1_fields_impacted.str(),
                             hdr2_fields_impacted.str(), std::string((*it)->name)});
                    }
                } else {
                    if (!mau_header1 && !mau_header2) {
                        parser_to_parser_removals.addRow(
                            {"", "", "", "", std::string((*it)->name)});
                    } else if (!mau_header1 && mau_header2) {
                        parser_to_mau_removals.addRow({"", "", "", "", std::string((*it)->name)});
                    } else if (mau_header1 && mau_header2) {
                        mau_to_mau_removals.addRow({"", "", "", "", std::string((*it)->name)});
                    }
                }
            }
        }
    }

    if (!LOGGING(3)) return;

    LOG3("Removed mutual exclusivity between all fields of the following headers.\n"
         << "If some fields had already had their mutual exclusivity removed by other "
         << "constraints,\nthose fields will not appear below.\n");
    if (!parser_to_parser_removals.empty()) {
        parser_to_parser_removals.print();
        LOG3("Mutexes invalidated between two parsed headers:");
        LOG3(parser_to_parser);
    }
    if (!parser_to_mau_removals.empty()) {
        parser_to_mau_removals.print();
        LOG3("Mutexes invalidated between a parsed header and a header added in control flow:");
        LOG3(parser_to_mau);
    }
    if (!mau_to_mau_removals.empty()) {
        mau_to_mau_removals.print();
        LOG3("Mutexes invalidated between two headers added in control flow:");
        LOG3(mau_to_mau);
    }
    if (!no_impact_removals.empty()) {
        no_impact_removals.print();
        LOG3("Mutexes invalidated between two headers, but that did not affect any fields:");
        LOG3(no_impact);
    }
    if (!field_to_field_removals.empty()) {
        field_to_field_removals.print();
        LOG3("Mutual exclusivity between fields removed:");
        LOG3(field_to_field);
    }
}
