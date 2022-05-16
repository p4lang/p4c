/**
 * @author Timotej Ponek, FIT VUT Brno
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "graphs.h"

#include <boost/graph/copy.hpp>
#include <boost/graph/adjacency_list.hpp>

#ifndef _BACKENDS_GRAPHS_GRAPH_VISITOR_H_
#define _BACKENDS_GRAPHS_GRAPH_VISITOR_H_

namespace graphs{

/**
 * "process" function of class is dependent on previous aplication of visitor 
 * classes Controls and Parsers onto IR, and can only be run after them
 */
class Graph_visitor : public Graphs {
 private:
    /** Enum used to create correct connection between subgraphs */
    enum class PrevType{
        Control,
        Parser
    };

    /** Stores variables and fullgraph */
    struct fullGraphOpts{
        Graph fg;  // fullGraph

        /* variables needed for subgraph connection*/
        unsigned long node_i = 0;  // node indexer
        unsigned cluster_i = 0;  // cluster indexer
    };

    /** Copies only edge name */
    struct edge_name_copier {
        /**
         * @param from graph that will be copied
         * @param to graph to which "from" will be copied
         */
        edge_name_copier(const Graph& from, Graph& to)
        : from(from), to(to){}
        const Graph &from;
        Graph &to;

        void operator()(Graph::edge_descriptor input, Graph::edge_descriptor output) const {
            auto label = boost::get(boost::edge_name, from, input);

            boost::put(boost::edge_name, to, output, label);
        }
    };

 public:
    /**
     * @param graphsDir directory where graphs will be stored
     * @param graphs option to output graph for each function block
     * @param fullGraph option to create fullGraph
     * @param jsonOut option to create json fullGraph
     */
    Graph_visitor(const cstring &graphsDir, const bool graphs,
                  const bool fullGraph, const bool jsonOut,
                  const cstring &filename) :
            graphsDir(graphsDir), graphs(graphs),
            fullGraph(fullGraph), jsonOut(jsonOut),
            filename(filename) {}
    /** 
     * @brief Maps VertexType to string
     * @param v_type VertexType to map
     * @return string representation of v_type
     */
    const char* getType(const VertexType &v_type);
    /** 
     * @brief Maps PrevType to string
     * @param prev_type PrevType to map
     * @return string representation of prev_type
     */
    const char* getPrevType(const PrevType &prev_type);
    /**
     * @brief main function
     * 
     * @details based on the value of boolean class variables "graphs", "fullGraph", "jsonOut"
     * executes:
     * "graphs" - outputs boost graphs to files
     * "fullGraph" - merges boost graphs into one CFG, and outputs to file
     * "jsonOut" - iterates over boost graphs, and creates json representation of these graphs
     * 
     * @param controlGraphsArray vector with boost graphs of control blocks
     * @param parserGraphsArray vector with boost graphs of control parsers
     */
    void process(std::vector<Graph *> &controlGraphsArray, std::vector<Graph *> &parserGraphsArray);
    /**
     * @brief Writes boost graph "g" in dot format to file given by "name"
     * @param g boost graph
     * @param name file name
     */
    void writeGraphToFile(const Graph &g, const cstring &name);

 private:
    /**
     * @brief Loops over vector graphsArray with boost graphs, creating json representation of CFG
     * @param graphsArray vector containing boost graphs
     * @param prev_type represents whether graphs in graphsArray are of type control or parser
     * 
     */
    void forLoopJson(std::vector<Graph *> &graphsArray, PrevType prev_type);
    /**
     * @brief Loops over vector graphsArray with boost graphs, creating fullgraph
     * It basically merges all graphs in graphsArray into one CFG
     * @param graphsArray vector containing boost graphs
     * @param[in,out] opts stores fullgraph and needed variables used for indexing
     * @param prev_type used to correctly connect subgraphs of parser and control type
     */
    void forLoopFullGraph(std::vector<Graph *> &graphsArray, fullGraphOpts* opts,
                          PrevType prev_type);

    Util::JsonObject* json;  // stores json that will be outputted
    Util::JsonArray* programBlocks;  // stores objects in top level array "nodes"
    const cstring graphsDir;
    // options
    const bool graphs;      // output boost graphs to files
    const bool fullGraph;   // merge boost graphs into one CFG, and output to file
    const bool jsonOut;     // iterate over boost graphs, and create json representation of these
                            // graphs
    const cstring filename;
};

}  // namespace graphs

#endif  /* _BACKENDS_GRAPHS_GRAPH_VISITOR_H_ */
