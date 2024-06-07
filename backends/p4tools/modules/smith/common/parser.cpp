#include "backends/p4tools/modules/smith/common/parser.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

#include <boost/multiprecision/detail/min_max.hpp>

#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/modules/smith/common/declarations.h"
#include "backends/p4tools/modules/smith/common/expressions.h"
#include "backends/p4tools/modules/smith/common/probabilities.h"
#include "backends/p4tools/modules/smith/common/scope.h"
#include "backends/p4tools/modules/smith/common/statements.h"
#include "backends/p4tools/modules/smith/core/target.h"
#include "backends/p4tools/modules/smith/util/util.h"
#include "ir/indexed_vector.h"
#include "ir/vector.h"
#include "lib/big_int_util.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

namespace P4Tools::P4Smith {

IR::MethodCallStatement *ParserGenerator::genHdrExtract(IR::Member *pkt_call, IR::Expression *mem) {
    auto *args = new IR::Vector<IR::Argument>();
    auto *arg = new IR::Argument(mem);

    args->push_back(arg);
    auto *mce = new IR::MethodCallExpression(pkt_call, args);
    return new IR::MethodCallStatement(mce);
}

void ParserGenerator::genHdrUnionExtract(IR::IndexedVector<IR::StatOrDecl> &components,
                                         const IR::Type_HeaderUnion *hdru, IR::ArrayIndex *arr_ind,
                                         IR::Member *pkt_call) {
    const auto *sf = hdru->fields.at(0);
    // for (auto sf : hdru->fields) {
    // auto mem = new IR::Member(arr_ind,
    // sf->type->to<IR::Type_Name>()->path->name);
    auto *mem = new IR::Member(arr_ind, sf->name);

    components.push_back(genHdrExtract(pkt_call, mem));
    // }
}

IR::ParserState *ParserGenerator::genStartState() {
    IR::IndexedVector<IR::StatOrDecl> components;
    IR::Expression *transition = new IR::PathExpression("parse_hdrs");
    auto *ret = new IR::ParserState("start", components, transition);

    P4Scope::addToScope(ret);
    return ret;
}

IR::ParserState *ParserGenerator::genHdrStates() {
    IR::Expression *transition = nullptr;
    IR::IndexedVector<IR::StatOrDecl> components;
    std::vector<cstring> hdrFieldsNames;
    std::map<const cstring, const IR::Type *> hdrFieldsTypes;

    const auto *sysHdrType = P4Scope::getTypeByName(SYS_HDR_NAME);
    const auto *sysHdr = sysHdrType->to<IR::Type_Struct>();
    if (sysHdr == nullptr) {
        BUG("Unexpected system header %s", sysHdrType->static_type_name());
    }
    for (const auto *sf : sysHdr->fields) {
        hdrFieldsNames.push_back(sf->name.name);
        hdrFieldsTypes.emplace(sf->name.name, sf->type);
    }

    auto *pktCall = new IR::Member(new IR::PathExpression("pkt"), "extract");
    for (auto sfName : hdrFieldsNames) {
        const auto *sfType = hdrFieldsTypes[sfName];
        if (const auto *sfTpS = sfType->to<IR::Type_Stack>()) {
            auto *mem = new IR::Member(new IR::PathExpression("hdr"), sfName);
            size_t size = sfTpS->getSize();
            const auto *eleTpName = sfTpS->elementType;
            const auto *eleTp =
                P4Scope::getTypeByName(eleTpName->to<IR::Type_Name>()->path->name.name);
            if (eleTp->is<IR::Type_Header>()) {
                for (size_t j = 0; j < size; j++) {
                    auto *nextMem = new IR::Member(mem, "next");
                    components.push_back(genHdrExtract(pktCall, nextMem));
                }
            } else if (const auto *hdruTp = eleTp->to<IR::Type_HeaderUnion>()) {
                for (size_t j = 0; j < size; j++) {
                    auto *arrInd =
                        new IR::ArrayIndex(mem, new IR::Constant(IR::Type_InfInt::get(), j));
                    genHdrUnionExtract(components, hdruTp, arrInd, pktCall);
                }
            } else {
                BUG("wtf here %s", sfType->node_type_name());
            }
        } else if (sfType->is<IR::Type_Name>()) {
            auto *mem = new IR::Member(new IR::PathExpression("hdr"), sfName);
            const auto *hdrFieldTp =
                P4Scope::getTypeByName(sfType->to<IR::Type_Name>()->path->name.name);
            if (hdrFieldTp->is<IR::Type_HeaderUnion>()) {
                const auto *hdruTp = hdrFieldTp->to<IR::Type_HeaderUnion>();
                const auto *sf = hdruTp->fields.at(0);
                auto *hdrMem = new IR::Member(mem, sf->name);
                components.push_back(genHdrExtract(pktCall, hdrMem));
            } else {
                components.push_back(genHdrExtract(pktCall, mem));
            }
        } else {
            BUG("wtf here %s", sfType->node_type_name());
        }
    }

    // transition part
    // transition = new IR::PathExpression(new IR::Path(IR::ID("state_0")));
    /*    cstring next_state = getRandomString(6);
        genState(next_state);
        transition = new IR::PathExpression(next_state);*/

    transition = new IR::PathExpression("accept");

    auto *ret = new IR::ParserState("parse_hdrs", components, transition);
    P4Scope::addToScope(ret);
    return ret;
}

IR::ListExpression *ParserGenerator::buildMatchExpr(IR::Vector<IR::Type> types) {
    IR::Vector<IR::Expression> components;
    for (const auto *tb : types) {
        IR::Expression *expr = nullptr;
        switch (Utils::getRandInt(0, 2)) {
            case 0: {
                // TODO(fruffy): Figure out allowed expressions
                // if (P4Scope::checkLval(tb)) {
                // cstring lval_name = P4Scope::pickLval(tb);
                // expr = new IR::PathExpression(lval_name);
                // } else {
                // expr = target().expressionGenerator().genBitLiteral(tb);
                // }
                expr = target().expressionGenerator().genBitLiteral(tb);
                break;
            }
            case 1: {
                // Range
                big_int maxRange = big_int(1U) << tb->width_bits();
                // FIXME: disable large ranges for now
                maxRange = min(big_int(1U) << 32, maxRange);
                big_int lower = Utils::getRandBigInt(0, maxRange - 1);
                big_int higher = Utils::getRandBigInt(lower, maxRange - 1);
                auto *lowerExpr = new IR::Constant(lower);
                auto *higherExpr = new IR::Constant(higher);
                expr = new IR::Range(tb, lowerExpr, higherExpr);
                break;
            }
            case 2: {
                // Mask
                auto *left = target().expressionGenerator().genBitLiteral(tb);
                auto *right = target().expressionGenerator().genBitLiteral(tb);
                expr = new IR::Mask(tb, left, right);
                break;
            }
        }
        components.push_back(expr);
    }
    return new IR::ListExpression(components);
}

void ParserGenerator::genState(cstring name) {
    IR::IndexedVector<IR::StatOrDecl> components;

    P4Scope::startLocalScope();

    // variable decls
    for (int i = 0; i < 5; i++) {
        auto *varDecl = target().declarationGenerator().genVariableDeclaration();
        components.push_back(varDecl);
    }
    // statements
    for (int i = 0; i < 5; i++) {
        auto *ass = target().statementGenerator().genAssignmentStatement();
        if (ass != nullptr) {
            components.push_back(ass);
        }
        break;
    }

    // expression
    IR::Expression *transition = nullptr;

    std::vector<int64_t> percent = {PCT.P4STATE_TRANSITION_ACCEPT, PCT.P4STATE_TRANSITION_REJECT,
                                    PCT.P4STATE_TRANSITION_STATE, PCT.P4STATE_TRANSITION_SELECT};

    P4Scope::endLocalScope();
    switch (Utils::getRandInt(percent)) {
        case 0: {
            transition = new IR::PathExpression("accept");
            break;
        }
        case 1: {
            transition = new IR::PathExpression("reject");
            break;
        }
        case 2: {
            cstring nextState = getRandomString(6);
            genState(nextState);
            transition = new IR::PathExpression(nextState);
            break;
        }
        case 3: {
            IR::Vector<IR::Expression> exprs;
            IR::Vector<IR::SelectCase> cases;
            size_t numTransitions = Utils::getRandInt(1, 3);
            size_t keySetLen = Utils::getRandInt(1, 4);

            IR::Vector<IR::Type> types;
            for (size_t i = 0; i <= keySetLen; i++) {
                auto *tb = ExpressionGenerator::genBitType(false);
                types.push_back(tb);
            }

            for (size_t i = 0; i < numTransitions; i++) {
                IR::Expression *matchSet = nullptr;
                // TODO(fruffy): Do !always have a default
                if (i == (numTransitions - 1)) {
                    P4Scope::req.compile_time_known = true;
                    matchSet = buildMatchExpr(types);
                    P4Scope::req.compile_time_known = false;
                } else {
                    matchSet = new IR::DefaultExpression();
                }
                switch (Utils::getRandInt(0, 2)) {
                    case 0: {
                        cases.push_back(
                            new IR::SelectCase(matchSet, new IR::PathExpression("accept")));
                        break;
                    }
                    case 1: {
                        cases.push_back(
                            new IR::SelectCase(matchSet, new IR::PathExpression("reject")));
                        break;
                    }
                    case 2: {
                        cstring nextState = getRandomString(6);
                        genState(nextState);
                        auto *swCase =
                            new IR::SelectCase(matchSet, new IR::PathExpression(nextState));
                        cases.push_back(swCase);
                        break;
                    }
                }
            }
            P4Scope::req.require_scalar = true;
            IR::ListExpression *keySet =
                target().expressionGenerator().genExpressionList(types, false);
            P4Scope::req.require_scalar = false;
            transition = new IR::SelectExpression(keySet, cases);
            break;
        }
    }

    // add to scope
    auto *ret = new IR::ParserState(name, components, transition);
    P4Scope::addToScope(ret);
    state_list.push_back(ret);
}

void ParserGenerator::buildParserTree() {
    state_list.push_back(ParserGenerator::genStartState());
    state_list.push_back(ParserGenerator::genHdrStates());
}

}  // namespace P4Tools::P4Smith
