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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_CHARACTERIZE_PARSER_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_CHARACTERIZE_PARSER_H_

#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/common/table_printer.h"
#include "bf-p4c/logging/filelog.h"
#include "bf-p4c/parde/parser_info.h"

/**
 * @ingroup LowerParser
 * @brief Prints various info about parser to the log file.
 */
class CharacterizeParser : public Inspector {
    // Configurable parameters

    unsigned line_rate = 100;  //  In gigabits

    double clock_rate = 1.22;  // In GHz

    // Inter-packet-gap, in bytes
    std::map<unsigned, unsigned> ipg = {{1, 8}, {25, 5}, {40, 1}, {50, 1}, {100, 1}};

    // preamble + min frame + CRC + gap
    unsigned target_min_packet_size = 8 + 60 + 4 + ipg[line_rate];

    // Average consumption rates, in bytes per cycle
    double target_min_avg_rate = line_rate * 1e9 / (8.0 * clock_rate * 1e9);

    CollectLoweredParserInfo cgl;

    ordered_map<const IR::BFN::LoweredParser *, ordered_set<const IR::BFN::LoweredParserState *>>
        parser_to_states;

    ordered_map<const IR::BFN::LoweredParserState *,
                ordered_set<const IR::BFN::LoweredParserMatch *>>
        state_to_matches;

    ordered_map<const IR::BFN::LoweredParserMatch *, const IR::BFN::LoweredParserState *>
        match_to_state;

    struct ExtractorUsage : public ordered_map<PHV::Type, unsigned> {
        unsigned get(PHV::Type type) const {
            unsigned rv = 0;

            if (count(type)) rv = (*this).at(type);

            return rv;
        }

        unsigned get(PHV::Size size) const {
            unsigned rv = 0;

            for (auto use : *this) {
                if (use.first.size() == size) rv += use.second;
            }

            return rv;
        }

        void add(PHV::Container c) { (*this)[c.type()]++; }
    };

    ordered_map<const IR::BFN::LoweredParserMatch *, const ExtractorUsage *>
        match_to_extractor_usage;

    ordered_map<const IR::BFN::LoweredParserMatch *, std::set<unsigned>> match_to_checksum_usage;

    ordered_map<const IR::BFN::LoweredParserMatch *, std::set<unsigned>> match_to_clot_usage;

 public:
    Visitor::profile_t init_apply(const IR::Node *root) override {
        auto rv = Inspector::init_apply(root);
        root->apply(cgl);

        BUG_CHECK(!cgl.graphs().empty(), "Parser IR hasn't been lowered yet?");
        return rv;
    }

    bool preorder(const IR::BFN::LoweredParserState *state) override {
        auto parser = findContext<IR::BFN::LoweredParser>();
        parser_to_states[parser].insert(state);
        return true;
    }

    void get_extractor_usage(const IR::BFN::LoweredParserMatch *match) {
        auto rv = new ExtractorUsage;

        for (auto stmt : match->extracts) {
            if (auto ep = stmt->to<IR::BFN::LoweredExtractPhv>()) rv->add(ep->dest->container);
        }

        match_to_extractor_usage[match] = rv;
    }

    void get_clot_usage(const IR::BFN::LoweredParserMatch *match) {
        for (auto stmt : match->extracts) {
            if (auto ec = stmt->to<IR::BFN::LoweredExtractClot>())
                match_to_clot_usage[match].insert(ec->dest->tag);
        }
    }

    void get_checksum_usage(const IR::BFN::LoweredParserMatch *match) {
        for (auto csum : match->checksums) match_to_checksum_usage[match].insert(csum->unit_id);
    }

    bool preorder(const IR::BFN::LoweredParserMatch *match) override {
        auto state = findContext<IR::BFN::LoweredParserState>();
        state_to_matches[state].insert(match);
        match_to_state[match] = state;

        get_extractor_usage(match);
        get_clot_usage(match);
        get_checksum_usage(match);

        return true;
    }

    void print_state_usage(const IR::BFN::LoweredParser *parser) {
        if (!parser_to_states.count(parser)) return;

        std::string total_extract_label = "Total Extractors";
        if (Device::currentDevice() != Device::TOFINO) total_extract_label += " (16-bit)";

        TablePrinter tp(
            std::clog,
            {"State", "Match", "8-bit", "16-bit", "32-bit", total_extract_label, "Other"},
            TablePrinter::Align::LEFT);

        auto sorted = cgl.graphs().at(parser)->topological_sort();

        for (auto s : sorted) {
            if (state_to_matches.count(s)) {
                std::string state_label;

                for (auto m : state_to_matches.at(s)) {
                    auto &usage = match_to_extractor_usage.at(m);
                    auto state = match_to_state.at(m);

                    auto usage_8b = usage->get(PHV::Size::b8);
                    auto usage_16b = usage->get(PHV::Size::b16);
                    auto usage_32b = usage->get(PHV::Size::b32);

                    unsigned sausage = 0;  // state aggreated usage

                    if (Device::currentDevice() == Device::TOFINO)
                        sausage = usage_8b + usage_16b + usage_32b;
                    else if (Device::currentDevice() != Device::TOFINO)
                        sausage = usage_8b + usage_16b + 2 * usage_32b;

                    if (state_label.empty())
                        state_label = std::string(state->name);
                    else
                        state_label = "-";

                    std::stringstream other_usage;

                    if (match_to_clot_usage.count(m)) {
                        other_usage << "clot ";
                        for (auto c : match_to_clot_usage.at(m)) other_usage << c << " ";
                    }

                    if (match_to_checksum_usage.count(m)) {
                        other_usage << "csum ";
                        for (auto c : match_to_checksum_usage.at(m)) other_usage << c << " ";
                    }

                    tp.addRow({state_label, std::string(m->value->toString()),
                               std::to_string(usage_8b), std::to_string(usage_16b),
                               std::to_string(usage_32b), std::to_string(sausage),
                               other_usage.str()});
                }
            }
        }

        tp.print();
    }

    static double round(double val, int decimal_point) {
        unsigned round_by = pow(10, decimal_point);
        return (val * round_by) / round_by;
    }

    void print_timing_report(const IR::BFN::LoweredParser *parser,
                             const std::vector<const IR::BFN::LoweredParserState *> &path) {
        unsigned total_user_header_bits = 0;

        for (auto it = path.begin(); it != path.end(); it++) {
            // need a better way of delineating metadata states vs. user header states
            if ((*it)->name.startsWith("$")) continue;

            auto next = std::next(it);

            if (next != path.end()) {
                auto matches = cgl.graphs().at(parser)->transitions(*it, *next);
                if (!matches.empty()) total_user_header_bits += (*matches.begin())->shift;
            } else {
                auto matches = cgl.graphs().at(parser)->to_pipe(*it);
                if (!matches.empty()) total_user_header_bits += (*matches.begin())->shift;
            }
        }

        if (total_user_header_bits < target_min_packet_size * 8)
            total_user_header_bits = target_min_packet_size * 8;

        double avg_rate = total_user_header_bits / (8.0 * path.size());

        if (avg_rate < target_min_avg_rate) {
            unsigned min_packet_size = std::max(
                target_min_packet_size, unsigned(ceil(target_min_avg_rate * (path.size()))));

            unsigned min_payload_size = min_packet_size - total_user_header_bits / 8;
            double max_data_rate = clock_rate * (total_user_header_bits / path.size());
            double max_packet_rate = (1e3 * max_data_rate) / total_user_header_bits;

            max_data_rate = round(max_data_rate, 2);
            max_packet_rate = round(max_packet_rate, 2);

            std::clog << "Average rate: " << avg_rate << " Bps" << std::endl;

            std::clog << "Min packet size at " << line_rate << " Gbps: " << min_packet_size << " B"
                      << " (" << min_payload_size << " B payload)" << std::endl;

            std::clog << "Max data rate for min-sized packets: " << max_data_rate << " Gbps / "
                      << max_packet_rate << " MPps" << std::endl;
        } else {
            std::clog << "Timing is met for min-sized packet (" << target_min_packet_size << " B)"
                      << " running at " << line_rate << " Gbps" << std::endl;
        }
    }

    void print_parser_summary(const IR::BFN::LoweredParser *parser) {
        unsigned num_states = 0;
        unsigned num_matches = 0;

        if (parser_to_states.count(parser)) {
            num_states = parser_to_states.at(parser).size();

            for (auto s : parser_to_states.at(parser)) {
                if (state_to_matches.count(s)) num_matches += state_to_matches.at(s).size();
            }
        }

        auto gress = parser->gress;

        std::clog << "Number of states on " << gress << ": " << num_states << std::endl;

        std::clog << std::endl;

        std::clog << "Number of matches on " << gress << ": " << num_matches << std::endl;

        std::clog << std::endl;

        auto longest_path = cgl.graphs().at(parser)->longest_path_states(parser->start);

        std::clog << "Longest path (" << longest_path.size() << " states) on " << gress << ":"
                  << std::endl;

        for (auto s : longest_path) std::clog << "    " << s->name << std::endl;

        std::clog << std::endl;

        auto shortest_path = cgl.graphs().at(parser)->shortest_path_states(parser->start);

        std::clog << "Shortest path (" << shortest_path.size() << " states) on " << gress << ":"
                  << std::endl;

        for (auto s : shortest_path) std::clog << "    " << s->name << std::endl;

        std::clog << std::endl;

        print_timing_report(parser, longest_path);

        std::clog << std::endl;

        std::clog << "Extractor usage:" << std::endl;

        print_state_usage(parser);
    }

    void end_apply(const IR::Node *root) override {
        const IR::BFN::Pipe *pipe = root->to<IR::BFN::Pipe>();
        if (BackendOptions().verbose > 0) {
            Logging::FileLog parserLog(pipe->canon_id(), cstring("parser.characterize.log"));

            std::clog << "Parser Characterization Report:" << std::endl;

            for (auto ps : parser_to_states) print_parser_summary(ps.first);
        }
    }
};

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_CHARACTERIZE_PARSER_H_ */
