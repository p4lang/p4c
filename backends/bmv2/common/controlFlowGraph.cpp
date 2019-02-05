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

#include "controlFlowGraph.h"

#include "ir/ir.h"
#include "frontends/p4/fromv1.0/v1model.h"
#include "frontends/p4/typeMap.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/tableApply.h"

#include "lib/ordered_map.h"
#include "lib/ordered_set.h"

namespace BMV2 {

unsigned CFG::Node::crtId = 0;

void CFG::EdgeSet::dbprint(std::ostream& out) const {
    for (auto s : edges)
        out << " " << s;
}

void CFG::Edge::dbprint(std::ostream& out) const {
    out << endpoint->name;
    switch (type) {
        case EdgeType::True:
            out << "(true)";
            break;
        case EdgeType::False:
            out << "(false)";
            break;
        case EdgeType::Label:
            out << "(" << label << ")";
            break;
        default:
            break;
    }
}

void CFG::Node::dbprint(std::ostream& out) const
{ out << name << " =>" << successors; }

void CFG::dbprint(std::ostream& out, CFG::Node* node, std::set<CFG::Node*> &done) const {
    if (done.find(node) != done.end())
        return;
    for (auto p : node->predecessors.edges)
        dbprint(out, p->endpoint, done);
    out << std::endl << node;
    done.emplace(node);
}

void CFG::Node::addPredecessors(const EdgeSet* set) {
    LOG2("Add to predecessors of " << name << ":" << set);
    if (set != nullptr)
        predecessors.mergeWith(set);
}


void CFG::dbprint(std::ostream& out) const {
    out << "CFG for " << dbp(container);
    std::set<CFG::Node*> done;
    for (auto n : allNodes)
        dbprint(out, n, done);
}

void CFG::Node::computeSuccessors() {
    for (auto e : predecessors.edges)
        e->getNode()->successors.emplace(e->clone(this));
}

bool CFG::dfs(Node* node, std::set<Node*> &visited,
              std::set<const IR::P4Table*> &stack) const {
    const IR::P4Table* table = nullptr;
    if (node->is<TableNode>()) {
        table = node->to<TableNode>()->table;
        if (stack.find(table) != stack.end()) {
            ::error(ErrorType::ERR_INVALID,
                    "Program can not be implemented on this taret since it contains a path from "
                    "table %1% back to itself", table);
            return false;
        }
    }
    if (visited.find(node) != visited.end())
        return true;
    if (table != nullptr)
        stack.emplace(table);
    for (auto e : node->successors.edges) {
        bool success = dfs(e->endpoint, visited, stack);
        if (!success) return false;
    }
    if (table != nullptr)
        stack.erase(table);
    visited.emplace(node);
    return true;
}

bool CFG::EdgeSet::isDestination(const CFG::Node* node) const {
    for (auto e : edges) {
        auto dest = e->endpoint;
        if (dest == node) return true;
        auto td = dest->to<TableNode>();
        auto tn = node->to<TableNode>();
        if (td != nullptr && tn != nullptr &&
            td->table == tn->table)
            return true;
    }

    return false;
}

bool CFG::EdgeSet::checkSame(const CFG::EdgeSet& other) const {
    if (size() != other.size())
        return false;
    // This algorithm is quadratic, but we expect these graphs to be very small
    for (auto e : edges) {
        if (!other.isDestination(e->endpoint))
            return false;
    }
    for (auto e : other.edges) {
        if (!isDestination(e->endpoint))
            return false;
    }
    return true;
}

// We check whether a table always jumps to the same destination,
// even if it appears multiple times in the CFG.
bool CFG::checkMergeable(std::set<TableNode*> nodes) const {
    TableNode* first = nullptr;
    for (auto tn : nodes) {
        if (first == nullptr) {
            first = tn;
            continue;
        }
        bool same = first->successors.checkSame(tn->successors);
        if (!same) {
            ::error(ErrorType::ERR_INVALID, "Program is not supported by this target, because "
                    "table %1% has multiple successors", tn->table);
            return false;
        }
    }
    return true;
}

bool CFG::checkImplementable() const {
    std::set<Node*> visited;
    std::set<const IR::P4Table*> stack;
    for (auto n : allNodes) {
        bool success = dfs(n, visited, stack);
        if (!success) return false;
    }

    std::map<const IR::P4Table*, std::set<TableNode*>> tableNodes;
    for (auto n : allNodes) {
        if (auto tn = n->to<TableNode>())
            tableNodes[tn->table].emplace(tn);
    }
    for (auto it : tableNodes) {
        if (it.second.size() == 1)
            continue;
        bool success = checkMergeable(it.second);
        if (!success)
            return false;
    }

    return true;
}

namespace {
class CFGBuilder : public Inspector {
    CFG*                    cfg;
    /// predecessors of current CFG node
    const CFG::EdgeSet*     live;
    P4::ReferenceMap*       refMap;
    P4::TypeMap*            typeMap;

    bool preorder(const IR::ReturnStatement*) override {
        cfg->exitPoint->addPredecessors(live);
        live = new CFG::EdgeSet();
        return false;
    }
    bool preorder(const IR::ExitStatement*) override {
        cfg->exitPoint->addPredecessors(live);
        live = new CFG::EdgeSet();
        return false;
    }
    bool preorder(const IR::EmptyStatement*) override {
        // unchanged 'live'
        return false;
    }
    bool preorder(const IR::MethodCallStatement* statement) override {
        auto instance = P4::MethodInstance::resolve(statement->methodCall, refMap, typeMap);
        if (!instance->is<P4::ApplyMethod>())
            return false;
        auto am = instance->to<P4::ApplyMethod>();
        if (!am->object->is<IR::P4Table>()) {
            ::error(ErrorType::ERR_INVALID, "apply method must be on a table", statement);
            return false;
        }
        auto tc = am->object->to<IR::P4Table>();
        auto node = cfg->makeNode(tc, statement->methodCall);
        node->addPredecessors(live);
        live = new CFG::EdgeSet(new CFG::Edge(node));
        return false;
    }
    bool preorder(const IR::IfStatement* statement) override {
        // We only allow expressions of the form t.apply().hit lively.
        // If the expression is more complex it should have been
        // simplified by prior passes.
        auto tc = P4::TableApplySolver::isHit(statement->condition, refMap, typeMap);
        CFG::Node* node;
        if (tc != nullptr) {
            // hit-miss case.
            node = cfg->makeNode(tc, statement->condition);
        } else {
            node = cfg->makeNode(statement);
        }

        node->addPredecessors(live);
        // If branch
        live = new CFG::EdgeSet(new CFG::Edge(node, true));
        visit(statement->ifTrue);
        auto afterTrue = live;
        if (afterTrue == nullptr)
            // error
            return false;
        auto result = new CFG::EdgeSet(afterTrue);
        // Else branch
        if (statement->ifFalse != nullptr) {
            live = new CFG::EdgeSet(new CFG::Edge(node, false));
            visit(statement->ifFalse);
            result->mergeWith(live);
        } else {
            // no else branch
            result->mergeWith(new CFG::EdgeSet(new CFG::Edge(node, false)));
        }
        live = result;
        return false;
    }
    bool preorder(const IR::BlockStatement* statement) override {
        for (auto s : statement->components) {
            auto stat = s->to<IR::Statement>();
            if (stat == nullptr) continue;
            visit(stat);
        }
        // live is unchanged
        return false;
    }
    bool preorder(const IR::SwitchStatement* statement) override {
        auto tc = P4::TableApplySolver::isActionRun(statement->expression, refMap, typeMap);
        BUG_CHECK(tc != nullptr, "%1%: unexpected switch statement expression",
                  statement->expression);
        auto node = cfg->makeNode(tc, statement->expression);
        node->addPredecessors(live);
        auto result = new CFG::EdgeSet(new CFG::Edge(node));  // In case no label matches
        auto labels = new CFG::EdgeSet();
        for (auto sw : statement->cases) {
            cstring label;
            if (sw->label->is<IR::DefaultExpression>()) {
                label = "default";
            } else {
                auto pe = sw->label->to<IR::PathExpression>();
                CHECK_NULL(pe);
                label = pe->path->name.name;
            }
            labels->mergeWith(new CFG::EdgeSet(new CFG::Edge(node, label)));
            if (sw->statement != nullptr) {
                live = labels;
                visit(sw->statement);
                labels = new CFG::EdgeSet();
            }  // else we accumulate edges
            result->mergeWith(live);
        }
        live = result;
        return false;
    }

 public:
    CFGBuilder(CFG* cfg, P4::ReferenceMap* refMap, P4::TypeMap* typeMap) :
            cfg(cfg), live(nullptr), refMap(refMap), typeMap(typeMap) {}
    const CFG::EdgeSet* run(const IR::Statement* body, const CFG::EdgeSet* predecessors) {
        CHECK_NULL(body); CHECK_NULL(predecessors);
        live = predecessors;
        body->apply(*this);
        return live;
    }
};
}  // end anonymous namespace

void CFG::build(const IR::P4Control* cc, P4::ReferenceMap* refMap, P4::TypeMap* typeMap) {
    container = cc;
    entryPoint = makeNode(cc->name + ".entry");
    exitPoint = makeNode("");  // the only node with an empty name

    CFGBuilder builder(this, refMap, typeMap);
    auto startValue = new CFG::EdgeSet(new CFG::Edge(entryPoint));
    auto last = builder.run(cc->body, startValue);
    LOG2("Before exit " << last);
    if (last != nullptr) {
        // nullptr can happen when there is an error
        exitPoint->addPredecessors(last);
        computeSuccessors();
    }
    LOG2(this);
}

}  // namespace BMV2
