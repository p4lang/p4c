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

#include "config.h"

#ifdef HAVE_LIBBOOST_GRAPH

#include <boost/graph/graphviz.hpp>

#include <iostream>

#include "frontends/p4/methodInstance.h"
#include "frontends/p4/tableApply.h"
#include "lib/log.h"
#include "lib/nullstream.h"
#include "lib/path.h"

#include "controls.h"

namespace graphs {

class EdgeUnconditional : public EdgeTypeIface {
 public:
    EdgeUnconditional() = default;
    std::string label() const override { return ""; };
};

class EdgeIf : public EdgeTypeIface {
 public:
    enum class Branch { TRUE, FALSE };
    explicit EdgeIf(Branch branch) : branch(branch) { }
    std::string label() const override {
        switch (branch) {
          case Branch::TRUE:
              return "TRUE";
          case Branch::FALSE:
              return "FALSE";
        }
        assert(0 && "unreachable");
        return "";
    };

 private:
    Branch branch;
};

class EdgeSwitch : public EdgeTypeIface {
 public:
    explicit EdgeSwitch(const IR::Expression *labelExpr)
        : labelExpr(labelExpr) { }
    std::string label() const override {
        std::stringstream sstream;
        labelExpr->dbprint(sstream);
        return sstream.str();
    };

 private:
    const IR::Expression *labelExpr;
};

using vertex_t = ControlGraphs::vertex_t;

ControlGraphs::ControlGraphs(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                             const cstring &graphsDir)
    : refMap(refMap), typeMap(typeMap), graphsDir(graphsDir) {
    visitDagOnce = false;
}

boost::optional<vertex_t> ControlGraphs::merge_other_statements_into_vertex() {
    if (statements_stack.empty()) return boost::none;
    std::stringstream sstream;
    if (statements_stack.size() == 1) {
        statements_stack[0]->dbprint(sstream);
    } else if (statements_stack.size() == 2) {
        statements_stack[0]->dbprint(sstream);
        sstream << "\n";
        statements_stack[1]->dbprint(sstream);
    } else {
        statements_stack[0]->dbprint(sstream);
        sstream << "\n...\n";
        statements_stack.back()->dbprint(sstream);
    }
    auto v = boost::add_vertex(sstream.str(), *g);
    for (auto parent : parents) {
        boost::add_edge(parent.first, v, parent.second->label(), *g);
    }
    parents = {{v, new EdgeUnconditional()}};
    statements_stack.clear();
    return v;
}

vertex_t ControlGraphs::add_vertex(const std::string &name) {
    merge_other_statements_into_vertex();
    auto v = boost::add_vertex(name, *g);
    for (auto parent : parents) {
        boost::add_edge(parent.first, v, parent.second->label(), *g);
    }
    return v;
}

vertex_t ControlGraphs::add_vertex(const cstring &name) {
    return add_vertex(std::string(name));
}

void ControlGraphs::writeGraphToFile(const Graph &g, const cstring &name) {
    auto path = Util::PathName(graphsDir).join(name + ".dot");
    auto out = openFile(path.toString(), false);
    if (out == nullptr) {
        ::error("Failed to open file %1%", path.toString());
        return;
    }
    boost::write_graphviz(
        *out, g,
        boost::make_label_writer(boost::get(boost::vertex_name, g)),
        boost::make_label_writer(boost::get(boost::edge_name, g)));
}

bool ControlGraphs::preorder(const IR::PackageBlock *block) {
    for (auto it : block->constantValue) {
        if (it.second->is<IR::ControlBlock>()) {
            auto name = it.second->to<IR::ControlBlock>()->container->name;
            LOG1("Generating graph for top-level control " << name);
            Graph g_;
            g = &g_;
            root = boost::add_vertex(std::string("root"), g_);
            exit_v = boost::add_vertex(std::string("__EXIT__"), g_);
            parents = {{root, new EdgeUnconditional()}};
            visit(it.second->getNode());
            for (auto parent : parents)
                boost::add_edge(parent.first, exit_v, parent.second->label(), *g);
            writeGraphToFile(g_, name);
        }
    }
    return false;
}

bool ControlGraphs::preorder(const IR::ControlBlock *block) {
    visit(block->container);
    return false;
}

bool ControlGraphs::preorder(const IR::P4Control *cont) {
    std::string name;
    bool doPop = false;
    if (instance_name != boost::none) {
        name = name_stack.get(instance_name.get());
        name_stack.push_back(instance_name.get());
        doPop = true;
    } else {
        name = name_stack.get(cont->controlPlaneName());
    }
    auto v = add_vertex(name);
    return_parents.clear();
    parents = {{v, new EdgeUnconditional()}};
    visit(cont->body);
    merge_other_statements_into_vertex();
    parents.insert(parents.end(), return_parents.begin(), return_parents.end());
    return_parents.clear();
    if (doPop) name_stack.pop_back();
    return false;
}

bool ControlGraphs::preorder(const IR::BlockStatement *statement) {
    for (const auto component : statement->components)
        visit(component);
    merge_other_statements_into_vertex();
    return false;
}

bool ControlGraphs::preorder(const IR::IfStatement *statement) {
    std::stringstream sstream;
    statement->condition->dbprint(sstream);
    auto v = add_vertex(sstream.str());
    Parents new_parents;
    parents = {{v, new EdgeIf(EdgeIf::Branch::TRUE)}};
    visit(statement->ifTrue);
    merge_other_statements_into_vertex();
    new_parents.insert(new_parents.end(), parents.begin(), parents.end());
    parents = {{v, new EdgeIf(EdgeIf::Branch::FALSE)}};
    if (statement->ifFalse != nullptr) {
        visit(statement->ifFalse);
        merge_other_statements_into_vertex();
    }
    new_parents.insert(new_parents.end(), parents.begin(), parents.end());
    parents = new_parents;
    return false;
}

bool ControlGraphs::preorder(const IR::SwitchStatement *statement) {
    auto tbl = P4::TableApplySolver::isActionRun(statement->expression, refMap, typeMap);
    vertex_t v;
    // special case for action_run
    if (tbl == nullptr) {
        std::stringstream sstream;
        statement->expression->dbprint(sstream);
        v = add_vertex(sstream.str());
    } else {
        visit(tbl);
        BUG_CHECK(parents.size() == 1, "Unexpected parents size");
        auto parent = parents.front();
        BUG_CHECK(dynamic_cast<EdgeUnconditional *>(parent.second) != nullptr,
                  "Unexpected eged type");
        v = parent.first;
    }
    Parents new_parents;
    parents = {};
    bool hasDefault{false};
    for (auto scase : statement->cases) {
        parents.emplace_back(v, new EdgeSwitch(scase->label));
        if (scase->statement != nullptr) {
            visit(scase->statement);
            merge_other_statements_into_vertex();
            new_parents.insert(new_parents.end(), parents.begin(), parents.end());
            parents.clear();
        }
        if (scase->label->is<IR::DefaultExpression>()) {
            hasDefault = true;
            break;
        }
    }
    // TODO(antonin): do not add default statement for action_run if all actions
    // are present
    if (!hasDefault)
        new_parents.emplace_back(v, new EdgeSwitch(new IR::DefaultExpression()));
    else
        new_parents.insert(new_parents.end(), parents.begin(), parents.end());
    parents = new_parents;
    return false;
}

bool ControlGraphs::preorder(const IR::MethodCallStatement *statement) {
    auto instance = P4::MethodInstance::resolve(statement->methodCall, refMap, typeMap);
    if (instance->is<P4::ApplyMethod>()) {
        auto am = instance->to<P4::ApplyMethod>();
        if (am->object->is<IR::P4Table>()) {
            visit(am->object->to<IR::P4Table>());
        } else if (am->applyObject->is<IR::Type_Control>()) {
            if (am->object->is<IR::Parameter>()) {
                ::error("%1%: control parameters are not supported by this target",
                        am->object);
              return false;
            }
            BUG_CHECK(am->object->is<IR::Declaration_Instance>(),
                      "Unsupported control invocation: %1%", am->object);
            auto instantiation = am->object->to<IR::Declaration_Instance>();
            instance_name = instantiation->controlPlaneName();
            auto type = instantiation->type;
            if (type->is<IR::Type_Name>()) {
                auto tn = type->to<IR::Type_Name>();
                auto decl = refMap->getDeclaration(tn->path, true);
                visit(decl->to<IR::P4Control>());
            }
        } else {
            BUG("Unsupported apply method: %1%", instance);
        }
    } else {
        statements_stack.push_back(statement);
    }
    return false;
}

bool ControlGraphs::preorder(const IR::AssignmentStatement *statement) {
    statements_stack.push_back(statement);
    return false;
}

bool ControlGraphs::preorder(const IR::ReturnStatement *) {
    merge_other_statements_into_vertex();
    return_parents.insert(return_parents.end(), parents.begin(), parents.end());
    parents.clear();
    return false;
}

bool ControlGraphs::preorder(const IR::ExitStatement *) {
    merge_other_statements_into_vertex();
    for (auto parent : parents)
        boost::add_edge(parent.first, exit_v, parent.second->label(), *g);
    parents.clear();
    return false;
}

bool ControlGraphs::preorder(const IR::P4Table *table) {
    auto name = name_stack.get(table->controlPlaneName());
    auto v = add_vertex(name);
    parents = {{v, new EdgeUnconditional()}};
    return false;
}

}  // namespace graphs

#endif  // HAVE_LIBBOOST_GRAPH
