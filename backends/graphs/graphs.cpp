/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "ir/ir.h"
#include "ir/json_loader.h"
#include "lib/log.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/gc.h"
#include "lib/crash.h"
#include "lib/nullstream.h"
#include "frontends/common/parseInput.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/frontend.h"

#include "graphs.h"

namespace graphs {

class MidEnd : public PassManager {
 public:
    P4::ReferenceMap    refMap;
    P4::TypeMap         typeMap;
    IR::ToplevelBlock   *toplevel = nullptr;

    explicit MidEnd(CompilerOptions& options);
    IR::ToplevelBlock* process(const IR::P4Program *&program) {
        program = program->apply(*this);
        return toplevel;
    }
};

MidEnd::MidEnd(CompilerOptions& options) {
    bool isv1 = options.langVersion == CompilerOptions::FrontendVersion::P4_14;
    refMap.setIsV1(isv1);
    auto evaluator = new P4::EvaluatorPass(&refMap, &typeMap);
    setName("MidEnd");

    addPasses({
        evaluator,
        new VisitFunctor([this, evaluator]() { toplevel = evaluator->getToplevelBlock(); }),
    });
}

class Graphs::GraphAttributeSetter {
 public:
  void operator()(Graph &g) const {
      auto vertices = boost::vertices(g);
      for (auto vit = vertices.first; vit != vertices.second; ++vit) {
          const auto &vinfo = g[*vit];
          auto attrs = boost::get(boost::vertex_attribute, g);
          attrs[*vit]["label"] = vinfo.name;
          attrs[*vit]["style"] = vertexTypeGetStyle(vinfo.type);
          attrs[*vit]["shape"] = vertexTypeGetShape(vinfo.type);
          attrs[*vit]["margin"] = vertexTypeGetMargin(vinfo.type);
      }
      auto edges = boost::edges(g);
      for (auto eit = edges.first; eit != edges.second; ++eit) {
          auto attrs = boost::get(boost::edge_attribute, g);
          attrs[*eit]["label"] = boost::get(boost::edge_name, g, *eit);
      }
  }

 private:
    static cstring vertexTypeGetShape(VertexType type) {
        switch (type) {
          case VertexType::TABLE:
              return "ellipse";
          case VertexType::HEADER:
              return "box";
          case VertexType::START:
              return "circle";
          case VertexType::DEFAULT:
              return "doublecircle";
          default:
              return "rectangle";
        }
        BUG("unreachable");
        return "";
    }

    static cstring vertexTypeGetStyle(VertexType type) {
        switch (type) {
          case VertexType::CONTROL:
              return "dashed";
          case VertexType::HEADER:
              return "solid";
          case VertexType::START:
              return "solid";
          case VertexType::DEFAULT:
              return "solid";
          default:
              return "solid";
        }
        BUG("unreachable");
        return "";
    }

    static cstring vertexTypeGetMargin(VertexType type) {
        switch (type) {
          case VertexType::HEADER:
              return "0.05,0";
          case VertexType::START:
              return "0,0";
          case VertexType::DEFAULT:
              return "0,0";
          default:
              return "";
        }
    }
};

Graphs::vertex_t Graphs::add_vertex(const cstring &name, VertexType type) {
    auto v = boost::add_vertex(*g);
    boost::put(&Vertex::name, *g, v, name);
    boost::put(&Vertex::type, *g, v, type);
    return g->local_to_global(v);
}

void Graphs::add_edge(const vertex_t &from, const vertex_t &to, const cstring &name) {
    auto ep = boost::add_edge(from, to, g->root());
    boost::put(boost::edge_name, g->root(), ep.first, name);
}

boost::optional<Graphs::vertex_t> Graphs::merge_other_statements_into_vertex() {
    if (statementsStack.empty()) return boost::none;
    std::stringstream sstream;
    if (statementsStack.size() == 1) {
        statementsStack[0]->dbprint(sstream);
    } else if (statementsStack.size() == 2) {
        statementsStack[0]->dbprint(sstream);
        sstream << "\n";
        statementsStack[1]->dbprint(sstream);
    } else {
        statementsStack[0]->dbprint(sstream);
        sstream << "\n...\n";
        statementsStack.back()->dbprint(sstream);
    }
    auto v = add_vertex(cstring(sstream), VertexType::STATEMENTS);
    for (auto parent : parents)
        add_edge(parent.first, v, parent.second->label());
    parents = {{v, new EdgeUnconditional()}};
    statementsStack.clear();
    return v;
}

Graphs::vertex_t Graphs::add_and_connect_vertex(const cstring &name, VertexType type) {
    merge_other_statements_into_vertex();
    auto v = add_vertex(name, type);
    for (auto parent : parents)
        add_edge(parent.first, v, parent.second->label());
    return v;
}

class Options : public CompilerOptions {
 public:
    cstring graphsDir{"."};
    Options() {
        registerOption("--graphs-dir", "dir",
                       [this](const char* arg) { graphsDir = arg; return true; },
                       "Use this directory to dump graphs in dot format "
                       "(default is current working directory)\n");
     }
};


}  // namespace graphs

int main(int argc, char *const argv[]) {
    setup_gc_logging();
    setup_signals();

    graphs::Options options;
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    options.compilerVersion = "0.0.5";

    if (options.process(argc, argv) != nullptr)
        options.setInputFile();
    if (::errorCount() > 0)
        return 1;

    auto hook = options.getDebugHook();

    auto program = P4::parseP4File(options);
    if (program == nullptr || ::errorCount() > 0)
        return 1;

    try {
        P4::FrontEnd fe;
        fe.addDebugHook(hook);
        program = fe.run(options, program);
    } catch (const Util::P4CExceptionBase &bug) {
        std::cerr << bug.what() << std::endl;
        return 1;
    }
    if (program == nullptr || ::errorCount() > 0)
        return 1;

    graphs::MidEnd midEnd(options);
    midEnd.addDebugHook(hook);
    const IR::ToplevelBlock *top = nullptr;
    try {
        top = midEnd.process(program);
        if (options.dumpJsonFile)
            JSONGenerator(*openFile(options.dumpJsonFile, true)) << program << std::endl;
    } catch (const Util::P4CExceptionBase &bug) {
        std::cerr << bug.what() << std::endl;
        return 1;
    }
    if (::errorCount() > 0)
        return 1;

    LOG1("Generating graphs under " << options.graphsDir);
    LOG1("Generating control graphs");
    graphs::ControlGraphs cgen(&midEnd.refMap, &midEnd.typeMap, options.graphsDir);
    top->getMain()->apply(cgen);

    LOG1("Generating parser graphs");
    graphs::ParserGraphs pgen(&midEnd.refMap, &midEnd.typeMap, options.graphsDir);
    top->getMain()->apply(pgen);

    return ::errorCount() > 0;
}
