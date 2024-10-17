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

#ifndef EXTENSIONS_BF_P4C_PARDE_PARSER_DOMINATOR_BUILDER_H_
#define EXTENSIONS_BF_P4C_PARDE_PARSER_DOMINATOR_BUILDER_H_

#include <optional>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dominator_tree.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/reverse_graph.hpp>
#include "bf-p4c/logging/event_logger.h"
#include "bf-p4c/parde/parser_info.h"
#include "ir/ir.h"
#include "ir/visitor.h"

/**
 * @ingroup parde
 * @brief Builds parser graphs and determines the dominators and post-dominators of all parser
 * states. Using this information, you can infer many things. For example:
 *
 * Determining which other headers have also surely been encountered if a given header has been
 * encountered and which other headers have not been encountered if a given header has not been
 * encountered (see "bf-p4c/phv/analysis/header_mutex.h/cpp").
 */
class ParserDominatorBuilder : public PardeInspector {
 public:
    using IndexImmediateDominatorMap = ordered_map<int, int>;
    using StateImmediateDominatorMap =
        ordered_map<const IR::BFN::ParserState*, const IR::BFN::ParserState*>;
    using Graph = ReversibleParserGraph::Graph;
    using Vertex = boost::graph_traits<Graph>::vertex_descriptor;
    using IndexMap = boost::property_map<Graph, boost::vertex_index_t>::type;
    using PredMap = boost::iterator_property_map<std::vector<Vertex>::iterator, IndexMap>;

 protected:
    ordered_map<gress_t, ReversibleParserGraph> parser_graphs;
    ordered_map<gress_t, StateImmediateDominatorMap> immediate_dominators;
    ordered_map<gress_t, StateImmediateDominatorMap> immediate_post_dominators;
    std::set<const IR::BFN::ParserState*> states;

 private:
    /**
     * @brief Associate graph data structure to its parser.
     */
    bool preorder(const IR::BFN::Parser* parser) override;

    /**
     * @brief Build ReversibleParserGraph for use with Boost Graph Library algorithms.
     */
    bool preorder(const IR::BFN::ParserState* parser_state) override;

    /**
     * @brief Using the Boost Graph Library, get a vertex index (int) to vertex index (int)
     * immediate dominator map of a parser graph (key is immediately dominated by value).
     *
     * @param graph Template to the underlying Boost Graph representing the parser graph
     * @param entry Vertex representing the entry point (root) vertex of the graph, usually
     * $entry_point
     * @param get_post_dominators Whether logging prints "post-dominator" or "dominator"
     * @return IndexImmediateDominatorMap Vertex index to vertex index immediate dominator
     * map
     */
    template <typename G>
    IndexImmediateDominatorMap get_immediate_dominators(G graph, Vertex entry,
                                                        bool get_post_dominators) {
        std::vector<Vertex> dom_tree_pred_vector;
        IndexMap index_map(boost::get(boost::vertex_index, graph));
        dom_tree_pred_vector =
            std::vector<Vertex>(boost::num_vertices(graph), boost::graph_traits<G>::null_vertex());
        PredMap dom_tree_pred_map =
            boost::make_iterator_property_map(dom_tree_pred_vector.begin(), index_map);
        boost::lengauer_tarjan_dominator_tree(graph, entry, dom_tree_pred_map);

        typename boost::graph_traits<G>::vertex_iterator it, end;
        IndexImmediateDominatorMap idom;
        std::string out = get_post_dominators ? "post-dominator" : "dominator";
        LOG7("Building " << out << " int map");
        for (std::tie(it, end) = boost::vertices(graph); it != end; ++it) {
            if (boost::get(dom_tree_pred_map, *it) != boost::graph_traits<G>::null_vertex()) {
                idom[boost::get(index_map, *it)] =
                    boost::get(index_map, boost::get(dom_tree_pred_map, *it));
                LOG7(TAB1 << "Setting " << out << " for " << boost::get(index_map, *it) << " to "
                          << boost::get(index_map, boost::get(dom_tree_pred_map, *it)));
            } else {
                idom[boost::get(index_map, *it)] = -1;
                LOG7(TAB1 << "Setting " << out << " for " << boost::get(index_map, *it)
                          << " to -1");
            }
        }
        return idom;
    }

    /**
     * @brief Using the Boost Graph Library, get a vertex index (int) to vertex index (int)
     * immediate dominator map of a parser graph (key is immediately dominated by value).
     *
     * @param graph The underlying Boost Graph representing the parser graph
     * @param entry Vertex representing the entry point (root) vertex of the graph, usually
     * $entry_point
     * @return IndexImmediateDominatorMap Vertex index to vertex index immediate dominator
     * map
     */
    IndexImmediateDominatorMap get_immediate_dominators(Graph graph, Vertex entry);

    /**
     * @brief Using the Boost Graph Library, get a vertex index (int) to vertex index (int)
     * immediate dominator map of a **reversed** parser graph (key is immediately dominated by
     * value). These are the immediate post-dominators of the original parser graph.
     *
     * @param graph The **reversed** underlying Boost Graph representing the parser graph
     * @param entry Vertex representing the entry point (root) vertex of the graph, usually nullptr
     * @return IndexImmediateDominatorMap Vertex index to vertex index immediate
     * post-dominator map
     */
    IndexImmediateDominatorMap get_immediate_post_dominators(boost::reverse_graph<Graph> graph,
                                                             Vertex entry);

    /**
     * @brief Converts a vertex index to vertex index immediate dominator map to an const
     * IR::BFN::ParserState* to const IR::BFN::ParserState* immediate dominator map.
     *
     * @param idom Vertex index to vertex index immediate dominator map
     * @param rpg Class wrapping the underlying Boost Graph that contains a vertex index to parser
     * state map
     * @return ParserDominatorBuilder::StateImmediateDominatorMap
     */
    StateImmediateDominatorMap index_map_to_state_map(IndexImmediateDominatorMap& idom,
                                                      ReversibleParserGraph& rpg);

    /**
     * @brief Get all parser states that are dominated by @p state.
     *
     * @param state Parser state on which to perform search
     * @param gress Gress on which to perform the search
     * @param get_post_dominators Whether to search this->immediate_post_dominators or
     * this->immediate_dominators
     * @return std::set<const IR::BFN::ParserState*> A set of all dominatees of @p state
     */
    std::set<const IR::BFN::ParserState*> get_all_dominatees(const IR::BFN::ParserState* state,
                                                             gress_t gress,
                                                             bool get_post_dominatees);

    /**
     * @brief Get all parser states that dominate @p state.
     *
     * @param state Parser state on which to perform search
     * @param gress Gress on which to perform the search
     * @param get_post_dominators Whether to search this->immediate_post_dominators or
     * this->immediate_dominators
     * @return std::set<const IR::BFN::ParserState*> A set of all dominators of @p state
     */
    std::set<const IR::BFN::ParserState*> get_all_dominators(const IR::BFN::ParserState* state,
                                                             gress_t gress,
                                                             bool get_post_dominators);

    /**
     * @brief Build immediate dominator and post-dominator maps for all parser graphs.
     */
    void build_dominator_maps();

 protected:
    /**
     * @brief Get all parser states that are dominated by @p state.
     *
     * @param state Parser state on which to perform search
     * @param gress Optional. Only used if @p state equals nullptr (End of Parser), as which gress
     * the End of Parser belongs to cannot be deduced without it
     * @return std::set<const IR::BFN::ParserState*> A set of all dominatees of @p state
     */
    std::set<const IR::BFN::ParserState*> get_all_dominatees(const IR::BFN::ParserState* state,
                                                             gress_t gress = INGRESS);

    /**
     * @brief Get all parser states that are post-dominated by @p state.
     *
     * @param state Parser state on which to perform search
     * @param gress Optional. Only used if @p state equals nullptr (End of Parser), as which gress
     * the End of Parser belongs to cannot be deduced without it
     * @return std::set<const IR::BFN::ParserState*> A set of all post-dominatees of @p state
     */
    std::set<const IR::BFN::ParserState*> get_all_post_dominatees(const IR::BFN::ParserState* state,
                                                                  gress_t gress = INGRESS);

    /**
     * @brief Get all parser states that dominate @p state.
     *
     * @param state Parser state on which to perform search
     * @param gress Optional. Only used if @p state equals nullptr (End of Parser), as which gress
     * the End of Parser belongs to cannot be deduced without it
     * @return std::set<const IR::BFN::ParserState*> A set of all dominators of @p state
     */
    std::set<const IR::BFN::ParserState*> get_all_dominators(const IR::BFN::ParserState* state,
                                                             gress_t gress = INGRESS);

    /**
     * @brief Get all post-dominators of a given parser @p state.
     *
     * @param state Parser state on which to perform search
     * @param gress Optional. Only used if @p state equals nullptr (End of Parser), as which gress
     * the End of Parser belongs to cannot be deduced without it
     * @return std::set<const IR::BFN::ParserState*> A set of all post-dominators of @p state
     */
    std::set<const IR::BFN::ParserState*> get_all_post_dominators(const IR::BFN::ParserState* state,
                                                                  gress_t gress = INGRESS);

    profile_t init_apply(const IR::Node* root) override;
    void end_apply() override;

 public:
    ParserDominatorBuilder() {}
};

#endif /* EXTENSIONS_BF_P4C_PARDE_PARSER_DOMINATOR_BUILDER_H_ */
