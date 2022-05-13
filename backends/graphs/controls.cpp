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

#include <iostream>

#include <boost/graph/graphviz.hpp>

#include "graphs.h"

#include "frontends/p4/methodInstance.h"
#include "frontends/p4/tableApply.h"
#include "lib/log.h"
#include "lib/nullstream.h"
#include "controls.h"

namespace graphs {

using Graph = ControlGraphs::Graph;

Graph *ControlGraphs::ControlStack::pushBack(Graph &currentSubgraph, const cstring &name) {
    auto &newSubgraph = currentSubgraph.create_subgraph();
    auto fullName = getName(name);
    boost::get_property(newSubgraph, boost::graph_name) = "cluster" + fullName;
    boost::get_property(newSubgraph, boost::graph_graph_attribute)["label"] =
                    boost::get_property(currentSubgraph, boost::graph_name) +
                    (fullName != "" ? "." + fullName : fullName);
    boost::get_property(newSubgraph, boost::graph_graph_attribute)["fontsize"] = "22pt";
    boost::get_property(newSubgraph, boost::graph_graph_attribute)["style"] = "bold";
    names.push_back(name);
    subgraphs.push_back(&newSubgraph);
    return getSubgraph();
}

Graph *ControlGraphs::ControlStack::popBack() {
    names.pop_back();
    subgraphs.pop_back();
    return getSubgraph();
}

Graph *ControlGraphs::ControlStack::getSubgraph() const {
    return subgraphs.empty() ? nullptr : subgraphs.back();
}

cstring ControlGraphs::ControlStack::getName(const cstring &name) const {
    std::stringstream sstream;
    for (auto &n : names) {
      if (n != "") sstream << n << ".";
    }
    sstream << name;
    return cstring(sstream);
}

bool ControlGraphs::ControlStack::isEmpty() const {
    return subgraphs.empty();
}

using vertex_t = ControlGraphs::vertex_t;

ControlGraphs::ControlGraphs(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                             const cstring &graphsDir)
    : refMap(refMap), typeMap(typeMap), graphsDir(graphsDir) {
    visitDagOnce = false;
}

bool ControlGraphs::preorder(const IR::PackageBlock *block) {
    for (auto it : block->constantValue) {
        if (!it.second) continue;
        if (it.second->is<IR::ControlBlock>()) {
            auto name = it.second->to<IR::ControlBlock>()->container->name;
            LOG1("Generating graph for top-level control " << name);

            Graph *g_ = new Graph();
            g = g_;
            instanceName = boost::none;
            boost::get_property(*g_, boost::graph_name) = name;
            BUG_CHECK(controlStack.isEmpty(), "Invalid control stack state");
            g = controlStack.pushBack(*g_, "");
            start_v = add_vertex("__START__", VertexType::OTHER);
            exit_v = add_vertex("__EXIT__", VertexType::OTHER);
            parents = {{start_v, new EdgeUnconditional()}};
            visit(it.second->getNode());

            for (auto parent : parents) {
                add_edge(parent.first, exit_v, parent.second->label());
            }
            BUG_CHECK((*g_).is_root(), "Invalid graph");
            controlStack.popBack();
            controlGraphsArray.push_back(g_);
        } else if (it.second->is<IR::PackageBlock>()) {
            visit(it.second->getNode());
        }
    }
    return false;
}

bool ControlGraphs::preorder(const IR::ControlBlock *block) {
    visit(block->container);
    return false;
}

bool ControlGraphs::preorder(const IR::P4Control *cont) {
    bool doPop = false;
    // instanceName == boost::none <=> top level
    if (instanceName != boost::none) {
        g = controlStack.pushBack(*g, instanceName.get());
        doPop = true;
    }
    return_parents.clear();
    visit(cont->body);
    merge_other_statements_into_vertex();

    parents.insert(parents.end(), return_parents.begin(), return_parents.end());
    return_parents.clear();
    if (doPop) g = controlStack.popBack();
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
    auto v = add_and_connect_vertex(cstring(sstream), VertexType::CONDITION);

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
    std::stringstream sstream;
    if (tbl == nullptr) {
        statement->expression->dbprint(sstream);
    } else {
        visit(tbl);
        sstream << "switch: action_run";
    }
    v = add_and_connect_vertex(cstring(sstream), VertexType::SWITCH);

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
                ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                        "%1%: control parameters are not supported by this target",
                        am->object);
              return false;
            }
            BUG_CHECK(am->object->is<IR::Declaration_Instance>(),
                      "Unsupported control invocation: %1%", am->object);
            auto instantiation = am->object->to<IR::Declaration_Instance>();
            instanceName = instantiation->controlPlaneName();
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
        statementsStack.push_back(statement);
    }
    return false;
}

bool ControlGraphs::preorder(const IR::AssignmentStatement *statement) {
    statementsStack.push_back(statement);
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
        add_edge(parent.first, exit_v, parent.second->label());
    parents.clear();
    return false;
}

bool ControlGraphs::preorder(const IR::Key *key) {
    std::stringstream sstream;

    // Build key
    for (auto elVec : key->keyElements) {
        sstream << elVec->matchType->path->name.name << ": ";
        sstream << elVec->annotations->annotations.front()->expr.front() << "\\n";
    }

    auto v = add_and_connect_vertex(cstring(sstream), VertexType::KEY);

    parents = {{v, new EdgeUnconditional()}};

    return false;
}

bool ControlGraphs::preorder(const IR::P4Action *action) {
    visit(action->body);
    return false;
}

bool ControlGraphs::preorder(const IR::P4Table *table) {
    auto name = table->getName();

    auto v = add_and_connect_vertex(name, VertexType::TABLE);

    parents = {{v, new EdgeUnconditional()}};

    auto key = table->getKey();
    visit(key);

    Parents keyNode;
    keyNode.emplace_back(parents.back());

    Parents new_parents;

    auto actions = table->getActionList()->actionList;
    for (auto action : actions){
        parents = keyNode;

        auto v = add_and_connect_vertex(action->getName(), VertexType::ACTION);

        parents = {{v, new EdgeUnconditional()}};

        if (action->expression->is<IR::MethodCallExpression>()) {
            auto mce = action->expression->to<IR::MethodCallExpression>();

            // needed for visiting body of P4Action
            auto resolved = P4::MethodInstance::resolve(mce, refMap, typeMap);

            if (resolved->is<P4::ActionCall>()) {
                auto ac = resolved->to<P4::ActionCall>();
                if (ac->action->is<IR::P4Action>()) {
                    visit(ac->action->to<IR::P4Action>());
                }
            }
        }

        merge_other_statements_into_vertex();

        new_parents.insert(new_parents.end(), parents.begin(), parents.end());
        parents.clear();
    }

    parents = new_parents;

    return false;
}

}  // namespace graphs
