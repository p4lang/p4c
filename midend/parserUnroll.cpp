#include "parserUnroll.h"

#include "interpreter.h"
#include "ir/ir.h"
#include "lib/hash.h"
#include "lib/stringify.h"

namespace P4 {

StackVariable::StackVariable(const IR::Expression *expr) : variable(expr) {
    CHECK_NULL(expr);
    BUG_CHECK(repOk(expr), "Invalid stack variable %1%", expr);
    variable = expr;
}

bool StackVariable::repOk(const IR::Expression *expr) {
    // Only members and path expression can be stack variables.
    const auto *member = expr->to<IR::Member>();
    if (member == nullptr) {
        return expr->is<IR::PathExpression>();
    }

    // A member is a stack variable if it is qualified by a PathExpression or if its qualifier is a
    // stack variable.
    return member->expr->is<IR::PathExpression>() || repOk(member->expr);
}

bool StackVariable::operator==(const StackVariable &other) const {
    // Delegate to IR's notion of equality.
    return variable->equiv(*other.variable);
}

size_t StackVariableHash::operator()(const StackVariable &var) const {
    // hash for path expression.
    if (const auto *path = var.variable->to<IR::PathExpression>()) {
        return Util::Hash::fnv1a<const cstring>(path->path->name.name);
    }
    const IR::Member *curMember = var.variable->to<IR::Member>();
    std::vector<size_t> h;
    while (curMember) {
        h.push_back(Util::Hash::fnv1a<const cstring>(curMember->member.name));
        if (auto *path = curMember->expr->to<IR::PathExpression>()) {
            h.push_back(Util::Hash::fnv1a<const cstring>(path->path->name));
            break;
        }
        curMember = curMember->expr->checkedTo<IR::Member>();
    }
    return Util::Hash::fnv1a(h.data(), sizeof(size_t) * h.size());
}

/// The main class for parsers' states key for visited checking.
struct VisitedKey {
    cstring name;              // name of a state.
    StackVariableMap indexes;  // indexes of header stacks.

    VisitedKey(cstring name, StackVariableMap &indexes) : name(name), indexes(indexes) {}

    explicit VisitedKey(const ParserStateInfo *stateInfo) {
        CHECK_NULL(stateInfo);
        name = stateInfo->state->name.name;
        indexes = stateInfo->statesIndexes;
    }

    /// Checks of two states to be less. If @a name < @a e.name then it returns @a true.
    /// if @a name > @a e.name then it returns false.
    /// If @a name is equal @a e.name then it checks the values of the headers stack indexes.
    /// For both map of header stacks' indexes it creates map of pairs of values for each
    /// header stack index.
    /// If some indexes are missing then it cosiders them as -1.
    /// Next, it continues checking each pair in a intermidiate map with the same approach
    /// as for @a name and @a e.name.
    bool operator<(const VisitedKey &e) const {
        if (name < e.name) return true;
        if (name > e.name) return false;
        std::unordered_map<StackVariable, std::pair<size_t, size_t>, StackVariableHash> mp;
        for (auto &i1 : indexes) {
            mp.emplace(i1.first, std::make_pair(i1.second, -1));
        }
        for (auto &i2 : e.indexes) {
            auto ref = mp.find(i2.first);
            if (ref == mp.end())
                mp.emplace(i2.first, std::make_pair(-1, i2.second));
            else
                ref->second.second = i2.second;
        }
        for (auto j : mp) {
            if (j.second.first < j.second.second) return true;
            if (j.second.first > j.second.second) return false;
        }
        return false;
    }
};

/**
 * Checks for terminal state.
 */
bool isTerminalState(IR::ID id) {
    return (id.name == IR::ParserState::reject || id.name == IR::ParserState::accept);
}

bool AnalyzeParser::preorder(const IR::ParserState *state) {
    LOG1("Found state " << dbp(state) << " of " << current->parser->name);
    if (state->name.name == IR::ParserState::start) current->start = state;
    current->addState(state);
    currentState = state;
    return true;
}

void AnalyzeParser::postorder(const IR::ParserState *) { currentState = nullptr; }

void AnalyzeParser::postorder(const IR::ArrayIndex *array) {
    // ignore arrays with concrete arguments
    if (array->right->is<IR::Constant>()) return;
    // tries to collect the name of a header stack for current state of a parser.
    current->addStateHSUsage(currentState, array->left);
}

void AnalyzeParser::postorder(const IR::Member *member) {
    // tries to collect the name of a header stack for current state of a parser.
    current->addStateHSUsage(currentState, member->expr);
}

void AnalyzeParser::postorder(const IR::PathExpression *expression) {
    auto state = findContext<IR::ParserState>();
    if (state == nullptr) return;
    auto decl = refMap->getDeclaration(expression->path);
    if (decl->is<IR::ParserState>()) current->calls(state, decl->to<IR::ParserState>());
}

namespace ParserStructureImpl {

/// Visited map of pairs :
/// 1) name of the parser state and values of the header stack indexes.
/// 2) value of index which is used for generation of the new names of the parsers' states.
using StatesVisitedMap = std::map<VisitedKey, size_t>;

// Makes transformation of the statements of a parser state.
// It updates indexes of a header stack and generates correct name of the next transition.
class ParserStateRewriter : public Transform {
 public:
    /// Default constructor.
    ParserStateRewriter(ParserStructure *parserStructure, ParserStateInfo *state,
                        ValueMap *valueMap, ReferenceMap *refMap, TypeMap *typeMap,
                        ExpressionEvaluator *afterExec, StatesVisitedMap &visitedStates)
        : parserStructure(parserStructure),
          state(state),
          valueMap(valueMap),
          refMap(refMap),
          typeMap(typeMap),
          afterExec(afterExec),
          visitedStates(visitedStates),
          wasOutOfBound(false) {
        CHECK_NULL(parserStructure);
        CHECK_NULL(state);
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
        CHECK_NULL(parserStructure);
        CHECK_NULL(state);
        currentIndex = 0;
    }

    /// Updates indexes of a header stack.
    IR::Node *preorder(IR::ArrayIndex *expression) {
        if (expression->right->is<IR::Constant>()) {
            return expression;
        }
        ParserStateRewriter rewriter(parserStructure, state, valueMap, refMap, typeMap, afterExec,
                                     visitedStates);
        auto basetype = getTypeArray(expression->left);
        if (!basetype->is<IR::Type_Stack>()) return expression;
        IR::ArrayIndex *newExpression = expression->clone();
        ExpressionEvaluator ev(refMap, typeMap, valueMap);
        auto *value = ev.evaluate(expression->right, false);
        if (!value->is<SymbolicInteger>()) return expression;
        auto *res = value->to<SymbolicInteger>()->constant->clone();
        newExpression->right = res;
        if (!res->fitsInt64()) {
            // we need to leave expression as is.
            ::warning(ErrorType::ERR_EXPRESSION, "Index can't be concretized : %1%", expression);
            return expression;
        }
        const auto *arrayType = basetype->to<IR::Type_Stack>();
        if (res->asUnsigned() >= arrayType->getSize()) {
            wasOutOfBound = true;
            return expression;
        }
        state->substitutedIndexes[newExpression->left] = res;
        return newExpression;
    }

    /// Eliminates header stack acces next, last operations.
    IR::Node *postorder(IR::Member *expression) {
        if (!afterExec) return expression;
        auto basetype = getTypeArray(getOriginal<IR::Member>()->expr);
        if (basetype->is<IR::Type_Stack>()) {
            auto l = afterExec->get(expression->expr);
            BUG_CHECK(l->is<SymbolicArray>(), "%1%: expected an array", l);
            auto array = l->to<SymbolicArray>();
            unsigned idx = 0;
            unsigned offset = 0;
            if (state->statesIndexes.count(expression->expr)) {
                idx = state->statesIndexes.at(expression->expr);
                if (expression->member.name != IR::Type_Stack::last) {
                    offset = 1;
                }
            }
            if (expression->member.name == IR::Type_Stack::lastIndex) {
                return new IR::Constant(IR::Type_Bits::get(32), idx);
            } else {
                if (idx + offset >= array->size) {
                    wasOutOfBound = true;
                    return expression;
                }
                state->statesIndexes[expression->expr] = idx + offset;
                return new IR::ArrayIndex(expression->expr->clone(),
                                          new IR::Constant(IR::Type_Bits::get(32), idx + offset));
            }
        }
        return expression;
    }

    /// Adds a new index for transition if it is required by algorithm.
    IR::Node *postorder(IR::PathExpression *expression) {
        if (!expression->type->is<IR::Type_State>()) return expression;
        IR::ID newName = genNewName(expression->path->name);
        if (newName.name == expression->path->name.name)  // first call
            return expression;
        // need to change name
        return new IR::PathExpression(expression->type, new IR::Path(newName, false));
    }
    inline size_t getIndex() { return currentIndex; }
    bool isOutOfBound() { return wasOutOfBound; }

 protected:
    const IR::Type *getTypeArray(const IR::Node *element) {
        if (element->is<IR::ArrayIndex>()) {
            const IR::Expression *left = element->to<IR::ArrayIndex>()->left;
            if (left->type->is<IR::Type_Stack>())
                return left->type->to<IR::Type_Stack>()->elementType;
        }
        return typeMap->getType(element, true);
    }

    /// Checks if this state was called previously with the same state of header stack indexes.
    /// If it was called then it returns true and generates a new name with the stored index.
    bool was_called(cstring nm, IR::ID &id) {
        if (state->scenarioStates.count(id.name) && state->statesIndexes.size() == 0) return false;
        auto i = visitedStates.find(VisitedKey(nm, state->statesIndexes));
        if (i == visitedStates.end()) return false;
        if (i->second > 0) id = IR::ID(id.name + std::to_string(i->second));
        currentIndex = i->second;
        return true;
    }

    /// Checks values of the headers stacks which were evaluated.
    bool checkIndexes(const StackVariableIndexMap &prev, const StackVariableIndexMap &cur) {
        if (prev.size() != cur.size()) {
            return false;
        }
        for (auto &i : prev) {
            auto j = cur.find(i.first);
            if (j == cur.end()) {
                return false;
            }
            if (!i.second->equiv(*j->second)) {
                return false;
            }
        }
        return true;
    }

    /// Returns true for current id if indexes of the headers stack
    /// are the same before previous call of the same parser state.
    bool calledWithNoChanges(IR::ID id, const ParserStateInfo *state) {
        CHECK_NULL(state);
        const auto *prevState = state;
        while (prevState->state->name.name != id.name) {
            prevState = prevState->predecessor;
            if (prevState == nullptr) {
                return false;
            }
        }
        if (prevState->predecessor != nullptr) {
            prevState = prevState->predecessor;
        } else if (prevState == state && state->predecessor == nullptr) {
            // The map should be empty if no next operators in a start.
            return state->statesIndexes.empty();
        }
        return prevState->statesIndexes == state->statesIndexes &&
               checkIndexes(prevState->substitutedIndexes, state->substitutedIndexes);
    }

    /// Generated new state name
    IR::ID genNewName(IR::ID id) {
        if (isTerminalState(id)) return id;
        size_t index = 0;
        cstring name = id.name;
        if (parserStructure->callsIndexes.count(id.name) &&
            (state->scenarioStates.count(id.name) ||
             parserStructure->reachableHSUsage(id, state))) {
            // or self called with no extraction...
            // in this case we need to use the same name as it was in previous call
            if (was_called(name, id)) {
                return id;
            }
            if (calledWithNoChanges(id, state)) {
                index = 0;
                if (parserStructure->callsIndexes.count(id.name)) {
                    index = parserStructure->callsIndexes[id.name];
                }
                if (index > 0) {
                    id = IR::ID(id.name + std::to_string(index));
                }
            } else {
                index = parserStructure->callsIndexes[id.name];
                parserStructure->callsIndexes[id.name] = index + 1;
                if (index + 1 > 0) {
                    index++;
                    id = IR::ID(id.name + std::to_string(index));
                }
            }
        } else if (!parserStructure->callsIndexes.count(id.name)) {
            index = 0;
            parserStructure->callsIndexes[id.name] = 0;
        }
        currentIndex = index;
        visitedStates.emplace(VisitedKey(name, state->statesIndexes), index);
        return id;
    }

 private:
    ParserStructure *parserStructure;
    ParserStateInfo *state;
    ValueMap *valueMap;
    ReferenceMap *refMap;
    TypeMap *typeMap;
    ExpressionEvaluator *afterExec;
    StatesVisitedMap &visitedStates;
    size_t currentIndex;
    bool wasOutOfBound;
};

class ParserSymbolicInterpreter {
    friend class ParserStateRewriter;

 protected:
    ParserStructure *structure;
    const IR::P4Parser *parser;
    ReferenceMap *refMap;
    TypeMap *typeMap;
    SymbolicValueFactory *factory;
    ParserInfo *synthesizedParser;  // output produced
    bool unroll;
    StatesVisitedMap visitedStates;
    bool &wasError;

    ValueMap *initializeVariables() {
        wasError = false;
        ValueMap *result = new ValueMap();
        ExpressionEvaluator ev(refMap, typeMap, result);

        for (auto p : parser->getApplyParameters()->parameters) {
            auto type = typeMap->getType(p);
            bool initialized =
                p->direction == IR::Direction::In || p->direction == IR::Direction::InOut;
            auto value = factory->create(type, !initialized);
            result->set(p, value);
        }
        for (auto d : parser->parserLocals) {
            auto type = typeMap->getType(d);
            SymbolicValue *value = nullptr;
            if (d->is<IR::Declaration_Constant>()) {
                auto dc = d->to<IR::Declaration_Constant>();
                value = ev.evaluate(dc->initializer, false);
            } else if (d->is<IR::Declaration_Variable>()) {
                auto dv = d->to<IR::Declaration_Variable>();
                if (dv->initializer != nullptr) value = ev.evaluate(dv->initializer, false);
            } else if (d->is<IR::P4ValueSet>()) {
                // The midend symbolic interpreter does not have
                // a representation for value_set.
                continue;
            }

            if (value == nullptr) value = factory->create(type, true);
            if (value->is<SymbolicError>()) {
                ::warning(ErrorType::ERR_EXPRESSION, "%1%: %2%", d,
                          value->to<SymbolicError>()->message());
                return nullptr;
            }
            if (value != nullptr) result->set(d, value);
        }
        return result;
    }

    ParserStateInfo *newStateInfo(const ParserStateInfo *predecessor, cstring stateName,
                                  ValueMap *values, size_t index) {
        if (stateName == IR::ParserState::accept || stateName == IR::ParserState::reject)
            return nullptr;
        auto state = structure->get(stateName);
        auto pi =
            new ParserStateInfo(stateName, parser, state, predecessor, values->clone(), index);
        synthesizedParser->add(pi);
        return pi;
    }

    static void stateChain(const ParserStateInfo *state, std::stringstream &stream) {
        if (state->predecessor != nullptr) {
            stateChain(state->predecessor, stream);
            stream << ", ";
        }
        stream << state->state->externalName();
    }

    static cstring stateChain(const ParserStateInfo *state) {
        CHECK_NULL(state);
        std::stringstream result;
        result << "Parser " << state->parser->externalName() << " state chain: ";
        stateChain(state, result);
        return result.str();
    }

    /// Return false if an error can be detected statically
    bool reportIfError(const ParserStateInfo *state, SymbolicValue *value) const {
        if (value->is<SymbolicException>()) {
            auto exc = value->to<SymbolicException>();

            bool stateClone = false;
            auto orig = state->state;
            auto crt = state;
            while (crt->predecessor != nullptr) {
                crt = crt->predecessor;
                if (crt->state == orig) {
                    stateClone = true;
                    break;
                }
            }

            if (!stateClone)
                // errors in the original state are signalled
                ::warning(ErrorType::ERR_EXPRESSION, "%1%: error %2% will be triggered\n%3%",
                          exc->errorPosition, exc->message(), stateChain(state));
            // else this error will occur in a clone of the state produced
            // by unrolling - if the state is reached.  So we don't give an error.
            return false;
        }
        if (!value->is<SymbolicStaticError>()) return true;
        return false;
    }

    /// Executes symbolically the specified statement.
    /// Returns pointer to generated statement if execution completes successfully,
    /// and 'nullptr' if an error occurred.
    const IR::StatOrDecl *executeStatement(ParserStateInfo *state, const IR::StatOrDecl *sord,
                                           ValueMap *valueMap) {
        const IR::StatOrDecl *newSord = nullptr;
        ExpressionEvaluator ev(refMap, typeMap, valueMap);

        SymbolicValue *errorValue = nullptr;
        bool success = true;
        if (sord->is<IR::AssignmentStatement>()) {
            auto ass = sord->to<IR::AssignmentStatement>();
            auto left = ev.evaluate(ass->left, true);
            errorValue = left;
            success = reportIfError(state, left);
            if (success) {
                auto right = ev.evaluate(ass->right, false);
                errorValue = right;
                success = reportIfError(state, right);
                if (success) left->assign(right);
            }
        } else if (sord->is<IR::MethodCallStatement>()) {
            // can have side-effects
            auto mc = sord->to<IR::MethodCallStatement>();
            auto e = ev.evaluate(mc->methodCall, false);
            errorValue = e;
            success = reportIfError(state, e);
        } else if (auto bs = sord->to<IR::BlockStatement>()) {
            IR::IndexedVector<IR::StatOrDecl> newComponents;
            for (auto *component : bs->components) {
                auto newComponent = executeStatement(state, component, valueMap);
                if (!newComponent)
                    success = false;
                else
                    newComponents.push_back(newComponent);
            }
            sord = new IR::BlockStatement(newComponents);
        } else {
            BUG("%1%: unexpected declaration or statement", sord);
        }
        if (!success) {
            if (errorValue->is<SymbolicException>()) {
                auto *exc = errorValue->to<SymbolicException>();
                if (exc->exc == P4::StandardExceptions::StackOutOfBounds) {
                    return newSord;
                }
            }
            std::stringstream errorStr;
            errorStr << errorValue;
            ::warning(ErrorType::WARN_IGNORE_PROPERTY, "Result of %1% is not defined: %2%", sord,
                      errorStr.str());
        }
        ParserStateRewriter rewriter(structure, state, valueMap, refMap, typeMap, &ev,
                                     visitedStates);
        const IR::Node *node = sord->apply(rewriter);
        if (rewriter.isOutOfBound()) {
            return nullptr;
        }
        newSord = node->to<IR::StatOrDecl>();
        LOG2("After " << sord << " state is\n" << valueMap);
        return newSord;
    }

    using EvaluationSelectResult =
        std::pair<std::vector<ParserStateInfo *> *, const IR::Expression *>;

    EvaluationSelectResult evaluateSelect(ParserStateInfo *state, ValueMap *valueMap) {
        const IR::Expression *newSelect = nullptr;
        auto select = state->state->selectExpression;
        if (select == nullptr) return EvaluationSelectResult(nullptr, nullptr);

        auto result = new std::vector<ParserStateInfo *>();
        if (select->is<IR::PathExpression>()) {
            auto path = select->to<IR::PathExpression>()->path;
            auto next = refMap->getDeclaration(path);
            BUG_CHECK(next->is<IR::ParserState>(), "%1%: expected a state", path);
            // update call indexes
            ParserStateRewriter rewriter(structure, state, valueMap, refMap, typeMap, nullptr,
                                         visitedStates);
            const IR::Expression *node = select->apply(rewriter);
            if (rewriter.isOutOfBound()) {
                return EvaluationSelectResult(nullptr, nullptr);
            }
            CHECK_NULL(node);
            newSelect = node->to<IR::Expression>();
            CHECK_NULL(newSelect);
            auto nextInfo = newStateInfo(state, next->getName(), state->after, rewriter.getIndex());
            if (nextInfo != nullptr) {
                nextInfo->scenarioStates = state->scenarioStates;
                result->push_back(nextInfo);
            }
        } else if (select->is<IR::SelectExpression>()) {
            // TODO: really try to match cases; today we are conservative
            auto se = select->to<IR::SelectExpression>();
            IR::Vector<IR::SelectCase> newSelectCases;
            ExpressionEvaluator ev(refMap, typeMap, valueMap);
            try {
                ev.evaluate(se->select, true);
            } catch (...) {
                // Ignore throws from evaluator.
                // If an index of a header stack is not substituted then
                // we should leave a state as is.
            }
            ParserStateRewriter rewriter(structure, state, valueMap, refMap, typeMap, &ev,
                                         visitedStates);
            const IR::Node *node = se->select->apply(rewriter);
            if (rewriter.isOutOfBound()) {
                return EvaluationSelectResult(nullptr, nullptr);
            }
            const IR::ListExpression *newListSelect = node->to<IR::ListExpression>();
            auto etalonStateIndexes = state->statesIndexes;
            for (auto c : se->selectCases) {
                auto currentStateIndexes = etalonStateIndexes;
                auto path = c->state->path;
                auto next = refMap->getDeclaration(path);
                BUG_CHECK(next->is<IR::ParserState>(), "%1%: expected a state", path);

                // update call indexes
                ParserStateRewriter rewriter(structure, state, valueMap, refMap, typeMap, nullptr,
                                             visitedStates);
                const IR::Node *node = c->apply(rewriter);
                if (rewriter.isOutOfBound()) {
                    return EvaluationSelectResult(nullptr, nullptr);
                }
                CHECK_NULL(node);
                auto newC = node->to<IR::SelectCase>();
                CHECK_NULL(newC);
                newSelectCases.push_back(newC);

                auto nextInfo =
                    newStateInfo(state, next->getName(), state->after, rewriter.getIndex());
                if (nextInfo != nullptr) {
                    nextInfo->scenarioStates = state->scenarioStates;
                    nextInfo->statesIndexes = currentStateIndexes;
                    result->push_back(nextInfo);
                }
            }
            newSelect = new IR::SelectExpression(newListSelect, newSelectCases);
        } else {
            BUG("%1%: unexpected expression", select);
        }
        return EvaluationSelectResult(result, newSelect);
    }

    static bool headerValidityChanged(const SymbolicValue *first, const SymbolicValue *second) {
        CHECK_NULL(first);
        CHECK_NULL(second);
        if (first->is<SymbolicHeader>()) {
            auto fhdr = first->to<SymbolicHeader>();
            auto shdr = second->to<SymbolicHeader>();
            CHECK_NULL(shdr);
            return !fhdr->valid->equals(shdr->valid);
        } else if (first->is<SymbolicArray>()) {
            auto farray = first->to<SymbolicArray>();
            auto sarray = second->to<SymbolicArray>();
            CHECK_NULL(sarray);
            for (size_t i = 0; i < farray->size; i++) {
                auto hdr = farray->get(nullptr, i);
                auto newHdr = sarray->get(nullptr, i);
                if (headerValidityChanged(hdr, newHdr)) return true;
            }
        } else if (first->is<SymbolicStruct>()) {
            auto fstruct = first->to<SymbolicStruct>();
            auto sstruct = second->to<SymbolicStruct>();
            CHECK_NULL(sstruct);
            for (auto f : fstruct->fieldValue) {
                auto ffield = fstruct->get(nullptr, f.first);
                auto sfield = sstruct->get(nullptr, f.first);
                if (headerValidityChanged(ffield, sfield)) return true;
            }
        }
        return false;
    }

    /// True if any header has changed its "validity" bit
    static bool headerValidityChange(const ValueMap *before, const ValueMap *after) {
        for (auto v : before->map) {
            auto value = v.second;
            if (headerValidityChanged(value, after->get(v.first))) return true;
        }
        return false;
    }

    /// True if both structures are equal.
    bool equStackVariableMap(const StackVariableMap &l, const StackVariableMap &r) const {
        if (l.empty()) {
            return r.empty();
        }
        for (const auto &i : l) {
            const auto j = r.find(i.first);
            if (j == r.end() || i.second != j->second) {
                return false;
            }
        }
        return true;
    }

    /// Return true if we have detected a loop we cannot unroll
    bool checkLoops(ParserStateInfo *state) const {
        const ParserStateInfo *crt = state;
        while (true) {
            crt = crt->predecessor;
            if (crt == nullptr) break;
            if (crt->state == state->state) {
                // Loop detected.
                // Check if any packet in the valueMap has changed
                auto filter = [](const IR::IDeclaration *, const SymbolicValue *value) {
                    return value->is<SymbolicPacketIn>();
                };
                auto packets = state->before->filter(filter);
                auto prevPackets = crt->before->filter(filter);
                if (packets->equals(prevPackets)) {
                    for (auto p : state->before->map) {
                        if (p.second->is<SymbolicPacketIn>()) {
                            auto pkt = p.second->to<SymbolicPacketIn>();
                            if (pkt->isConservative()) {
                                break;
                            }
                        }
                    }
                    if (equStackVariableMap(crt->statesIndexes, state->statesIndexes)) {
                        ::warning(ErrorType::ERR_INVALID,
                                  "Parser cycle can't be unrolled, because ParserUnroll can't "
                                  "detect the number of loop iterations:\n%1%",
                                  stateChain(state));
                        wasError = true;
                    }
                    return true;
                }

                // If no header validity has changed we can't really unroll
                if (!headerValidityChange(crt->before, state->before)) {
                    if (equStackVariableMap(crt->statesIndexes, state->statesIndexes)) {
                        ::warning(ErrorType::ERR_INVALID,
                                  "Parser cycle can't be unrolled, because ParserUnroll can't "
                                  "detect the number of loop iterations:\n%1%",
                                  stateChain(state));
                        wasError = true;
                    }
                    return true;
                }
                break;
            }
        }
        return false;
    }

    /// Gets new name for a state
    IR::ID getNewName(ParserStateInfo *state) {
        if (state->currentIndex == 0) {
            return state->state->name;
        }
        return IR::ID(state->state->name + std::to_string(state->currentIndex));
    }

    using EvaluationStateResult =
        std::tuple<std::vector<ParserStateInfo *> *, bool, IR::IndexedVector<IR::StatOrDecl>>;

    /// Generates new state with the help of symbolic execution.
    /// If corresponded state was generated previously then it returns @a nullptr and false.
    /// @param newStates is a set of parsers' names which were genereted.
    EvaluationStateResult evaluateState(ParserStateInfo *state,
                                        std::unordered_set<cstring> &newStates) {
        LOG1("Analyzing " << dbp(state->state));
        auto valueMap = state->before->clone();
        IR::IndexedVector<IR::StatOrDecl> components;
        IR::ID newName;
        if (unroll) {
            newName = getNewName(state);
            if (newStates.count(newName)) {
                return EvaluationStateResult(nullptr, false, components);
            }
            newStates.insert(newName);
        }
        for (auto s : state->state->components) {
            auto *newComponent = executeStatement(state, s, valueMap);
            if (!newComponent) {
                return EvaluationStateResult(nullptr, true, components);
            }
            if (unroll) {
                components.push_back(newComponent);
            }
        }
        state->after = valueMap;
        auto result = evaluateSelect(state, valueMap);
        if (unroll) {
            if (result.second == nullptr) {
                return EvaluationStateResult(nullptr, true, components);
            }
            if (state->name == newName) {
                state->newState =
                    new IR::ParserState(state->state->srcInfo, newName, state->state->annotations,
                                        components, result.second);
            } else {
                state->newState =
                    new IR::ParserState(state->state->srcInfo, newName, components, result.second);
            }
        }
        return EvaluationStateResult(result.first, true, components);
    }

 public:
    bool hasOutOfboundState;
    /// constructor
    ParserSymbolicInterpreter(ParserStructure *structure, ReferenceMap *refMap, TypeMap *typeMap,
                              bool unroll, bool &wasError)
        : structure(structure),
          refMap(refMap),
          typeMap(typeMap),
          synthesizedParser(nullptr),
          unroll(unroll),
          wasError(wasError) {
        CHECK_NULL(structure);
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
        factory = new SymbolicValueFactory(typeMap);
        parser = structure->parser;
        hasOutOfboundState = false;
    }

    using StatOrDeclVector = IR::IndexedVector<IR::StatOrDecl>;

    /// generate call OutOfBound
    void addOutOfBound(ParserStateInfo *stateInfo, std::unordered_set<cstring> &newStates,
                       bool checkBefore = true, StatOrDeclVector components = StatOrDeclVector()) {
        IR::ID newName = getNewName(stateInfo);
        if (checkBefore && newStates.count(newName)) {
            return;
        }
        hasOutOfboundState = true;
        newStates.insert(newName);
        auto *pathExpr =
            new IR::PathExpression(new IR::Type_State(), new IR::Path(outOfBoundsStateName, false));
        stateInfo->newState = new IR::ParserState(newName, components, pathExpr);
    }

    /// running symbolic execution
    ParserInfo *run() {
        synthesizedParser = new ParserInfo();
        auto initMap = initializeVariables();
        if (initMap == nullptr)
            // error during initializer evaluation
            return synthesizedParser;
        auto startInfo = newStateInfo(nullptr, structure->start->name.name, initMap, 0);
        structure->callsIndexes.emplace(structure->start->name.name, 0);
        startInfo->scenarioStates.insert(structure->start->name.name);
        std::vector<ParserStateInfo *> toRun;  // worklist
        toRun.push_back(startInfo);
        std::set<VisitedKey> visited;
        std::unordered_set<cstring> newStates;
        while (!toRun.empty()) {
            auto stateInfo = toRun.back();
            toRun.pop_back();
            LOG1("Symbolic evaluation of " << stateChain(stateInfo));
            // checking visited state, loop state, and the reachable states with needed header stack
            // operators.
            if (visited.count(VisitedKey(stateInfo)) &&
                !stateInfo->scenarioStates.count(stateInfo->name) &&
                !structure->reachableHSUsage(stateInfo->state->name, stateInfo))
                continue;
            auto iHSNames = structure->statesWithHeaderStacks.find(stateInfo->name);
            if (iHSNames != structure->statesWithHeaderStacks.end())
                stateInfo->scenarioHS.insert(iHSNames->second.begin(), iHSNames->second.end());
            visited.insert(VisitedKey(stateInfo));              // add to visited map
            stateInfo->scenarioStates.insert(stateInfo->name);  // add to loops detection
            bool infLoop = checkLoops(stateInfo);
            if (infLoop) {
                // Stop unrolling if it was an error.
                if (wasError) {
                    IR::ID newName = getNewName(stateInfo);
                    if (newStates.count(newName) != 0) {
                        evaluateState(stateInfo, newStates);
                    }
                    wasError = false;
                    continue;
                }
                // don't evaluate successors anymore
                // generate call OutOfBound
                addOutOfBound(stateInfo, newStates);
                continue;
            }
            IR::ID newName = getNewName(stateInfo);
            bool notAdded = newStates.count(newName) == 0;
            auto nextStates = evaluateState(stateInfo, newStates);
            if (get<0>(nextStates) == nullptr) {
                if (get<1>(nextStates) && stateInfo->predecessor &&
                    newName.name != stateInfo->predecessor->newState->name) {
                    // generate call OutOfBound
                    addOutOfBound(stateInfo, newStates, false, get<2>(nextStates));
                } else {
                    // save current state
                    if (notAdded) {
                        stateInfo->newState = stateInfo->state->clone();
                    }
                }
                LOG1("No next states");
                continue;
            }
            toRun.insert(toRun.end(), get<0>(nextStates)->begin(), get<0>(nextStates)->end());
        }

        return synthesizedParser;
    }
};

}  // namespace ParserStructureImpl

bool ParserStructure::analyze(ReferenceMap *refMap, TypeMap *typeMap, bool unroll, bool &wasError) {
    ParserStructureImpl::ParserSymbolicInterpreter psi(this, refMap, typeMap, unroll, wasError);
    result = psi.run();
    return psi.hasOutOfboundState;
}

/// check reachability for usage of header stack
bool ParserStructure::reachableHSUsage(IR::ID id, const ParserStateInfo *state) const {
    if (!state->scenarioHS.size()) return false;
    CHECK_NULL(callGraph);
    const IR::IDeclaration *declaration = parser->states.getDeclaration(id.name);
    BUG_CHECK(declaration && declaration->is<IR::ParserState>(), "Invalid declaration %1%", id);
    std::set<const IR::ParserState *> reachableStates;
    callGraph->reachable(declaration->to<IR::ParserState>(), reachableStates);
    std::set<cstring> reachebleHSoperators;
    for (auto i : reachableStates) {
        auto iHSNames = statesWithHeaderStacks.find(i->name);
        if (iHSNames != statesWithHeaderStacks.end())
            reachebleHSoperators.insert(iHSNames->second.begin(), iHSNames->second.end());
    }
    std::set<cstring> intersectionHSOperators;
    std::set_intersection(state->scenarioHS.begin(), state->scenarioHS.end(),
                          reachebleHSoperators.begin(), reachebleHSoperators.end(),
                          std::inserter(intersectionHSOperators, intersectionHSOperators.begin()));
    return intersectionHSOperators.size() > 0;
}

void ParserStructure::addStateHSUsage(const IR::ParserState *state,
                                      const IR::Expression *expression) {
    if (state == nullptr || expression == nullptr || !expression->type->is<IR::Type_Stack>())
        return;
    auto i = statesWithHeaderStacks.find(state->name.name);
    if (i == statesWithHeaderStacks.end()) {
        std::set<cstring> s;
        s.insert(expression->toString());
        statesWithHeaderStacks.emplace(state->name.name, s);
    } else {
        i->second.insert(expression->toString());
    }
}

}  // namespace P4
