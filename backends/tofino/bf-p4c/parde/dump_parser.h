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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_DUMP_PARSER_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_DUMP_PARSER_H_

#include <sys/stat.h>

#include <fstream>

#include <boost/algorithm/string.hpp>

#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/ir/gress.h"
#include "bf-p4c/parde/parser_info.h"
#include "ir/dump.h"
#include "lib/log.h"

class DotDumper {
 protected:
    std::string filename;

    // use this to override the default color for nodes and edges
    // each group will be assign an unique color
    std::vector<assoc::set<void *>> *color_groups = nullptr;

    bool detail = true;

    std::stringstream out;

    std::vector<std::string> colors = {"brown", "coral", "crimson", "deeppink", "gold",
                                       "red",   "blue",  "green",   "cyan",     "orchid"};

 public:
    explicit DotDumper(std::string filename, bool detail) : filename(filename), detail(detail) {}

    DotDumper(std::string filename, std::vector<assoc::set<void *>> &color_groups, bool detail)
        : filename(filename), color_groups(&color_groups), detail(detail) {}

 protected:
    static std::string escape(std::string s) {
        boost::replace_all(s, "ingress::", "");
        boost::replace_all(s, "egress::", "");
        boost::replace_all(s, "$", "\\$");
        boost::replace_all(s, ".", "\\.");
        boost::replace_all(s, "->", "-~");
        boost::replace_all(s, "<", "\\<");
        boost::replace_all(s, ">", "\\>");
        boost::replace_all(s, "-~", "->");
        return s;
    }

    std::string cluster_name;

    std::string to_label(std::string label, const void *what) {
        std::stringstream ss;
        ss << cluster_name << label << what;
        return ss.str();
    }

    std::string to_label(std::string label) {
        std::stringstream ss;
        ss << cluster_name << label;
        return ss.str();
    }

    std::string lookup_color(void *obj) {
        if (color_groups) {
            for (auto it = color_groups->begin(); it != color_groups->end(); it++) {
                if ((*it).count(obj)) {
                    auto cid = std::distance(color_groups->begin(), it) % colors.size();
                    return colors.at(cid);
                }
            }
        }
        return "undef";
    }

    void dump(const IR::BFN::Transition *transition) {
        auto c = lookup_color((void *)transition);  // NOLINT
        if (c != "undef") out << "color=" << c << " ";

        if (detail) out << "label=\"" << transition << "\"";
    }

    std::string get_color(const IR::BFN::ParserState *state) {
        auto c = lookup_color((void *)state);  // NOLINT
        if (c != "undef") return c;

        return "cornsilk";
    }

    void dump(const IR::BFN::ParserState *state) {
        out << to_label("State", state);
        out << " [shape=record, style=\"filled,rounded\", ";
        out << "fillcolor=" << get_color(state);
        out << ", label=\"{";
        out << state->name;

        if (detail) {
            out << ":\\l\\l";

            for (auto stmt : state->statements) out << "    " << stmt << "\\l";

            if (state->statements.size()) out << "\\l";

            for (auto select : state->selects) out << "    " << select << "\\l";
        }

        out << "}\"";
        out << "];" << std::endl;
    }

    void dump(const IR::BFN::ParserGraph &graph, gress_t gress) {
        for (auto s : graph.states()) dump(s);

        for (auto succ : graph.successors()) {
            for (auto dst : succ.second) {
                for (auto t : graph.transitions(succ.first, dst)) {
                    out << to_label("State", succ.first) << " -> " << to_label("State", dst)
                        << " [ ";

                    dump(t);

                    out << " ]" << std::endl;
                }
            }
        }

        for (auto &kv : graph.to_pipe()) {
            for (auto t : graph.to_pipe(kv.first)) {
                out << to_label("State", kv.first) << " -> "
                    << to_label(::toString(gress) + "_pipe") << " [ ";

                dump(t);

                out << " ]" << std::endl;
            }
        }

        for (auto &kv : graph.loopbacks()) {
            auto next = graph.get_state(kv.first.second);

            for (auto t : kv.second) {
                out << to_label("State", kv.first.first) << " -> " << to_label("State", next)
                    << " [ color=\"red\" ";

                dump(t);

                out << " ]" << std::endl;
            }
        }
    }

    void dump(const IR::BFN::LoweredParserState *state) {
        out << to_label("State", state) << " [style=filled, fillcolor=lightskyblue1, shape=record";
        out << ", label=\"{";
        out << state->name << ":\\l\\l";

        if (detail) {
            if (!state->select->regs.empty()) out << "    " << state->select << "\\l";
        }

        out << "}\"";
        out << "];" << std::endl;
    }

    void dump(const IR::BFN::LoweredParserMatch *match) {
        out << to_label("Match", match) << " [style=filled, fillcolor=aliceblue, shape=record";
        out << ", label=\"{";
        out << "match " << match->value << ": \\l\\l";

        if (detail) {
            for (auto stmt : match->extracts) out << "    " << stmt << "\\l";

            if (match->extracts.size()) out << "\\l";

            for (auto save : match->saves) out << "    " << save << "\\l";

            if (match->saves.size()) out << "\\l";

            for (auto csum : match->checksums) out << "    " << csum << "\\l";

            if (match->checksums.size()) out << "\\l";

            for (auto cntr : match->counters) out << "    " << cntr << "\\l";

            if (match->counters.size()) out << "\\l";

            out << "shift: " << match->shift;
        }

        out << "}\"";
        out << "];" << std::endl;
    }

    void dump(const IR::BFN::LoweredParserGraph &graph, gress_t gress) {
        unsigned id = 0;
        for (auto s : graph.states()) {
            out << "subgraph cluster_" << id++ << " {";
            out << "style=invis;" << std::endl;

            dump(s);

            for (auto m : s->transitions) dump(m);

            out << "}" << std::endl;
        }

        for (auto s : graph.states()) {
            for (auto m : s->transitions) {
                out << to_label("State", s) << " -> " << to_label("Match", m) << std::endl;

                if (m->next) {
                    out << to_label("Match", m) << " -> " << to_label("State", m->next)
                        << std::endl;
                } else if (m->loop) {
                    // handled below
                } else {
                    out << to_label("Match", m) << " -> " << to_label(::toString(gress) + "_pipe")
                        << std::endl;
                }
            }
        }

        // loopback edges
        for (auto s : graph.states()) {
            for (auto m : s->transitions) {
                if (m->loop) {
                    auto next = graph.get_state(stripThreadPrefix(m->loop));
                    out << to_label("Match", m) << " -> " << to_label("State", next)
                        << " [color=\"red\"] " << std::endl;
                }
            }
        }
    }

    template <typename ParserGraphType>
    void dump_graph(const ParserGraphType &graph, gress_t gress, int pipe_id) {
        out.str(std::string());

        out << "digraph parser {" << std::endl;
        out << "size=\"8,5\"" << std::endl;

        dump(graph, gress);

        out << "}" << std::endl;

        if (auto fs = open_file(gress, pipe_id)) write_to_file(fs);
    }

    std::ofstream *open_file(gress_t gress, int pipe_id, cstring directory = "graphs"_cs) {
        auto outdir = BFNContext::get().getOutputDirectory(directory, pipe_id);
        if (!outdir) return nullptr;

        static int fid = 0;
        auto filepath = outdir + "/" + std::to_string(fid++) + "_" + filename + "_" +
                        ::toString(gress) + ".dot";

        return new std::ofstream(filepath);
    }

    void write_to_file(std::ofstream *fs) {
        *fs << escape(out.str());
        fs->close();
    }
};

/**
 * @ingroup parde
 * @brief Dumps the entire parser graphs (can be used before and also after parser lowering).
 */
class DumpParser : public Visitor, public DotDumper {
 public:
    explicit DumpParser(std::string filename, bool detail = false, bool to_log = false)
        : DotDumper(filename, detail || LOGGING(4)), log(to_log) {}

    DumpParser(std::string filename, std::vector<assoc::set<void *>> &color_groups,
               bool detail = false, bool to_log = false)
        : DotDumper(filename, color_groups, detail || LOGGING(4)), log(to_log) {}

 private:
    const IR::Node *apply_visitor(const IR::Node *n, const char *) override { return n; }

    Visitor::profile_t init_apply(const IR::Node *root) override {
        auto rv = Visitor::init_apply(root);

        CollectParserInfo cg;
        root->apply(cg);

        CollectLoweredParserInfo cgl;
        root->apply(cgl);

        if (!cg.graphs().empty() && !cgl.graphs().empty()) BUG("IR is in an incoherent state");

        for (auto g : cg.graphs())
            dump_graph(*(g.second), (g.first)->gress, root->to<IR::BFN::Pipe>()->canon_id());

        for (auto g : cgl.graphs())
            dump_graph(*(g.second), (g.first)->gress, root->to<IR::BFN::Pipe>()->canon_id());

        if (log) {
            // Use LOG level 1 as to let the caller decide on whether
            // loogging should be done or not.
            for (auto g : cg.graphs())
                LOG1("Parser IR [" << filename << "] : " << std::endl
                                   << dumpToString(g.first) << std::endl);

            for (auto g : cgl.graphs())
                LOG1("Lowered Parser IR [" << filename << "] : " << std::endl
                                           << dumpToString(g.first) << std::endl);
        }

        return rv;
    }

    bool log;
};

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_DUMP_PARSER_H_ */
