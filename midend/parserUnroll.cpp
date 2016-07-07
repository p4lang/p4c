#include "parserUnroll.h"

namespace P4 {

void EvaluateParsers::postorder(const IR::P4Parser* parser) {
    LOG1("Evaluating " << parser);
    auto structure = ::get(parsers->parsers, getOriginal<IR::P4Parser>());
    CHECK_NULL(structure);
    auto pi = structure->evaluate(refMap, typeMap, unroll);
    parsers->evaluation.emplace(parser, pi);
}

namespace {
class ParserSymbolicInterpreter {
    ParserStructure*    structure;
    const IR::P4Parser* parser;
    ReferenceMap*       refMap;
    TypeMap*            typeMap;
    ValueFactory*       factory;
    ParserInfo*         synthesizedParser;  // output produced

    ValueMap* initializeVariables() {
        ValueMap* result = new ValueMap();
        for (auto p : *parser->type->applyParams->parameters) {
            auto type = typeMap->getType(p);
            bool initialized = p->direction == IR::Direction::In ||
                    p->direction == IR::Direction::InOut;
            auto value = factory->create(type, !initialized);
            result->set(p, value);
        }
        for (auto d : *parser->stateful) {
            auto type = typeMap->getType(d);
            auto value = factory->create(type, true);
            result->set(d, value);
        }
        return result;
    }

    ParserStateInfo* getState(cstring state, bool unroll) {
        if (state == IR::ParserState::accept.name ||
            state == IR::ParserState::reject.name)
            return nullptr;
        auto s = structure->get(state);
        if (unroll) {
            auto pi = new ParserStateInfo(state, parser, s);
            synthesizedParser->add(pi);
            return pi;
        } else {
            auto pi = synthesizedParser->getSingle(state);
            if (pi == nullptr) {
                cstring newName = refMap->newName(state);
                pi = new ParserStateInfo(newName, parser, s);
                synthesizedParser->add(pi);
            }
            return pi;
        }
    }

    bool reportIfError(AbstractValue* value) const {
        if (!value->is<ErrorValue>())
            return true;
        auto ev = value->to<ErrorValue>();
        ::error("%1%: %2%", ev->errorPosition, ev->message());
        return false;
    }

    // Executes symbolically the specified statement.
    // Returns 'true' if execution completes successfully,
    // and 'false' if an error occurred.
    bool executeStatement(const IR::StatOrDecl* sord, ValueMap* valueMap) const {
        ExpressionEvaluator ev(refMap, typeMap, valueMap);

        bool success = true;
        if (sord->is<IR::AssignmentStatement>()) {
            auto ass = sord->to<IR::AssignmentStatement>();
            auto left = ev.evaluate(ass->left);
            success = reportIfError(left);
            if (success) {
                auto right = ev.evaluate(ass->right);
                success = reportIfError(right);
                if (success)
                    left->assign(right);
            }
        } else if (sord->is<IR::MethodCallStatement>()) {
            // can have side-effects
            auto mc = sord->to<IR::MethodCallStatement>();
            auto e = ev.evaluate(mc->methodCall);
            success = reportIfError(e);
        } else {
            BUG("%1%: unexpected declaration or statement", sord);
        }
        LOG2("After " << sord << " state is " << valueMap);
        return success;
    }

    std::vector<ParserStateInfo*>* evaluateSelect(ParserStateInfo* state, bool unroll) {
        // TODO: update next state map
        auto select = state->state->selectExpression;
        if (select == nullptr)
            return nullptr;
        auto result = new std::vector<ParserStateInfo*>();
        if (select->is<IR::PathExpression>()) {
            auto path = select->to<IR::PathExpression>()->path;
            auto next = refMap->getDeclaration(path);
            BUG_CHECK(next->is<IR::ParserState>(), "%1%: expected a state", path);
            auto nextInfo = getState(next->getName(), unroll);
            if (nextInfo != nullptr)
                result->push_back(nextInfo);
        } else if (select->is<IR::SelectExpression>()) {
            // TODO: really try to match cases; today we are conservative
            auto se = select->to<IR::SelectExpression>();
            for (auto c : se->selectCases) {
                auto path = c->state->path;
                auto next = refMap->getDeclaration(path);
                BUG_CHECK(next->is<IR::ParserState>(), "%1%: expected a state", path);
                auto nextInfo = getState(next->getName(), unroll);
                if (nextInfo != nullptr)
                    result->push_back(nextInfo);
            }
        } else {
            BUG("%1%: unexpected expression", select);
        }
        return result;
    }

    std::vector<ParserStateInfo*>* evaluate(ParserStateInfo* state, bool unroll) {
        LOG1("Analyzing " << state->state);
        auto valueMap = state->before->clone();
        for (auto s : *state->state->components) {
            bool success = executeStatement(s, valueMap);
            if (!success)
                return nullptr;
        }
        state->after = valueMap;
        auto result = evaluateSelect(state, unroll);
        return result;
    }

    struct StateInput {
        ParserStateInfo* state;
        ValueMap* values;
        StateInput(ParserStateInfo* state, ValueMap* values) : state(state), values(values)
        { CHECK_NULL(state); CHECK_NULL(values); }
    };

 public:
    ParserSymbolicInterpreter(ParserStructure* structure, ReferenceMap* refMap,
                              TypeMap* typeMap)
            : structure(structure), refMap(refMap), typeMap(typeMap), synthesizedParser(nullptr) {
        CHECK_NULL(structure); CHECK_NULL(refMap); CHECK_NULL(typeMap);
        factory = new ValueFactory(typeMap);
        parser = structure->parser;
    }

    ParserInfo* run(bool unroll) {
        synthesizedParser = new ParserInfo();
        auto initMap = initializeVariables();
        auto startInfo = getState(structure->start->name.name, unroll);
        StateInput si(startInfo, initMap);
        std::vector<StateInput> toRun;
        toRun.push_back(si);

        while (!toRun.empty()) {
            auto work = toRun.back();
            toRun.pop_back();
            work.state->mergeValues(work.values);
            auto nextStates = evaluate(work.state, unroll);
            if (nextStates == nullptr)
                continue;
            for (auto a : *nextStates) {
                StateInput si(a, work.state->after->clone());
                toRun.push_back(si);
            }
        }

        return synthesizedParser;
    }
};
}  // namespace

ParserInfo* ParserStructure::evaluate(ReferenceMap* refMap, TypeMap* typeMap, bool unroll) {
    ParserSymbolicInterpreter psi(this, refMap, typeMap);
    auto result = psi.run(unroll);
    return result;
}

}  // namespace P4
