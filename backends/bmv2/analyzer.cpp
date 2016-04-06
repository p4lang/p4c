#include "analyzer.h"

#include "ir/ir.h"
#include "frontends/p4/fromv1.0/v1model.h"
#include "frontends/common/typeMap.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/tableApply.h"

namespace BMV2 {

cstring nameFromAnnotation(const IR::Annotations* annotations,
                           cstring defaultValue) {
    CHECK_NULL(annotations); CHECK_NULL(defaultValue);
    auto anno = annotations->getSingle(IR::Annotation::nameAnnotation);
    if (anno != nullptr) {
        CHECK_NULL(anno->expr);
        auto str = anno->expr->to<IR::StringLiteral>();
        CHECK_NULL(str);
        return str->value;
    }
    return defaultValue;
}

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

void CFG::dbprint(std::ostream& out) const {
    out << "CFG for " << container;
    std::set<CFG::Node*> done;
    for (auto n : allNodes)
        dbprint(out, n, done);
}

void CFG::Node::computeSuccessors() {
    for (auto e : predecessors.edges)
        e->getNode()->successors.emplace(e->clone(this));
}

// This visitor "converts" IR::Node* into EdgeSets
// Since we cannot return EdgeSets, we place them in the 'after' map.
class CFGBuilder : public Inspector {
    CFG*                                           cfg;
    const CFG::EdgeSet*                            current;  // predecessors of current node
    std::map<const IR::Statement*, const CFG::EdgeSet*> after;  // successors of each statement
    const P4::ReferenceMap*                     refMap;
    const P4::TypeMap*                          typeMap;

    void setAfter(const IR::Statement* statement, const CFG::EdgeSet* value) {
        LOG1("After " << statement << " " << value);
        CHECK_NULL(statement);
        if (value == nullptr)
            // This can happen when an error is signaled
            return;
        after.emplace(statement, value);
        current = value;
    }

    const CFG::EdgeSet* get(const IR::Statement* statement)
    { return ::get(after, statement); }
    bool preorder(const IR::Statement* statement) override {
        ::error("%1%: not supported in control block on this architecture", statement);
        return false;
    }
    bool preorder(const IR::ReturnStatement* statement) override {
        cfg->exitPoint->addPredecessors(current);
        setAfter(statement, new CFG::EdgeSet());  // empty successor set
        return false;
    }
    bool preorder(const IR::EmptyStatement* statement) override {
        // unchanged 'current'
        setAfter(statement, current);
        return false;
    }
    bool preorder(const IR::MethodCallStatement* statement) override {
        auto instance = P4::MethodInstance::resolve(statement->methodCall, refMap, typeMap);
        if (!instance->is<P4::ApplyMethod>())
            return false;
        auto am = instance->to<P4::ApplyMethod>();
        if (!am->object->is<IR::TableContainer>()) {
            ::error("%1%: apply method must be on a table", statement);
            return false;
        }
        auto tc = am->object->to<IR::TableContainer>();
        auto node = cfg->makeNode(tc, statement->methodCall);
        node->addPredecessors(current);
        setAfter(statement, new CFG::EdgeSet(new CFG::Edge(node)));
        return false;
    }
    bool preorder(const IR::IfStatement* statement) override {
        // We only allow expressions of the form t.apply().hit currently.
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

        node->addPredecessors(current);
        // If branch
        current = new CFG::EdgeSet(new CFG::Edge(node, true));
        visit(statement->ifTrue);
        auto ifTrue = get(statement->ifTrue);
        if (ifTrue == nullptr)
            return false;
        auto result = new CFG::EdgeSet(ifTrue);
        // Else branch
        if (statement->ifFalse != nullptr) {
            current = new CFG::EdgeSet(new CFG::Edge(node, false));
            visit(statement->ifFalse);
            auto ifFalse = get(statement->ifFalse);
            result->mergeWith(ifFalse);
        } else {
            // no else branch
            result->mergeWith(new CFG::EdgeSet(new CFG::Edge(node, false)));
        }
        setAfter(statement, result);
        return false;
    }
    bool preorder(const IR::BlockStatement* statement) override {
        for (auto s : *statement->components) {
            auto stat = s->to<IR::Statement>();
            if (stat == nullptr) continue;
            visit(stat);
            current = get(stat);
        }
        setAfter(statement, current);
        return false;
    }
    bool preorder(const IR::SwitchStatement* statement) override {
        auto tc = P4::TableApplySolver::isActionRun(statement->expression, refMap, typeMap);
        BUG_CHECK(tc != nullptr, "%1%: unexpected switch statement expression",
                  statement->expression);
        auto node = cfg->makeNode(tc, statement->expression);
        node->addPredecessors(current);
        auto result = new CFG::EdgeSet(new CFG::Edge(node));  // In case no label matches
        auto labels = new CFG::EdgeSet();
        for (auto sw : *statement->cases) {
            labels->mergeWith(new CFG::EdgeSet(new CFG::Edge(node, sw->label)));
            if (sw->statement != nullptr) {
                current = labels;
                visit(sw->statement);
                labels = new CFG::EdgeSet();
            }  // else we accumulate edges
            result->mergeWith(current);
        }
        setAfter(statement, result);
        return false;
    }

 public:
    CFGBuilder(CFG* cfg, const P4::ReferenceMap* refMap, const P4::TypeMap* typeMap) :
            cfg(cfg), current(nullptr), refMap(refMap), typeMap(typeMap) {}
    const CFG::EdgeSet* run(const IR::Statement* startNode, const CFG::EdgeSet* predecessors) {
        CHECK_NULL(startNode); CHECK_NULL(predecessors);
        current = predecessors;
        startNode->apply(*this);
        return current;
    }
};

void CFG::build(const IR::ControlContainer* cc,
                const P4::ReferenceMap* refMap, const P4::TypeMap* typeMap) {
    container = cc;
    entryPoint = makeNode(cc->name + ".entry");
    exitPoint = makeNode(cc->name + ".exit");

    CFGBuilder builder(this, refMap, typeMap);
    auto startValue = new CFG::EdgeSet(new CFG::Edge(entryPoint));
    auto last = builder.run(cc->body, startValue);
    LOG1("Before exit " << last);
    if (last != nullptr) {
        // nullptr can happen when there is an error
        exitPoint->addPredecessors(last);
        computeSuccessors();
    }
}

class DiscoverStructure : public Inspector {
    ProgramParts*           structure;

 public:
    explicit DiscoverStructure(ProgramParts* structure) : structure(structure) {}

    void postorder(const IR::ParameterList* paramList) override {
        bool inAction = findContext<IR::ActionContainer>() != nullptr;
        unsigned index = 0;
        for (auto p : *paramList->getEnumerator()) {
            structure->index.emplace(p, index);
            if (!inAction)
                structure->nonActionParameters.emplace(p);
            index++;
        }
    }
    void postorder(const IR::ActionContainer* action) override {
        auto control = findContext<IR::ControlContainer>();
        structure->actions.emplace(action, control);
    }
};

void ProgramParts::analyze(P4::BlockMap* blockMap) {
    DiscoverStructure disc(this);
    blockMap->program->apply(disc);
}

}  // namespace BMV2
