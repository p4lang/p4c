#include "midend/flattenLogMsg.h"
#include "flattenLogMsg.h"

namespace P4 {

const IR::Type_StructLike* FindTypeInLogMsgForReplace::hasStructInParameter(
        const IR::MethodCallStatement* methodCallStatement) {
    if (auto* path = methodCallStatement->methodCall->method->to<IR::PathExpression>()) {
        if (path->path->name.name == "log_msg") {
            auto* param1 = methodCallStatement->methodCall->arguments->at(0)->expression;
            auto* param2 = methodCallStatement->methodCall->arguments->at(1)->expression;
            if (!param1->is<IR::StringLiteral>() || !param2->is<IR::StructExpression>()) {
                return nullptr;
            }
            auto* type =
                typeMap->getTypeType(methodCallStatement->methodCall->typeArguments->at(0), true);
            if (auto* typeStruct = type->to<IR::Type_StructLike>()) {
                for (auto field : typeStruct->fields) {
                    if (field->type->is<IR::Type_StructLike>()) {
                        return typeStruct;
                    }
                }
            }
        }
    }
    return nullptr;
}

const IR::MethodCallStatement* FindTypeInLogMsgForReplace::prepareLogMsgStatement(
        const IR::MethodCallStatement* methodCallStatement) {
    if (auto* newMethod = getReplacementMethodCall(methodCallStatement->id)) {
        return newMethod;
    }
    // Create new statement
    auto* param1 = methodCallStatement->methodCall->arguments->at(0)->expression;
    std::string strParam1 = param1->to<IR::StringLiteral>()->value.c_str();
    auto* argType =
        typeMap->getTypeType(methodCallStatement->methodCall->typeArguments->at(0), true);
    auto* structType = argType->to<IR::Type_StructLike>();
    index = 0;
    auto exprVector =
        unfoldStruct(methodCallStatement->methodCall->arguments->at(1)->expression, strParam1);
    structType = generateNewStructType(structType, exprVector.first);
    auto* newMethodCall = methodCallStatement->methodCall->clone();
    auto* newTypeArguments = newMethodCall->typeArguments->clone();
    newTypeArguments->clear();
    newTypeArguments->push_back(structType);
    newMethodCall->typeArguments = newTypeArguments;
    auto* newArguments = newMethodCall->arguments->clone();
    auto* newArgument = newMethodCall->arguments->at(0)->clone();
    auto* newString = newArgument->expression->to<IR::StringLiteral>()->clone();
    newString->value = exprVector.second;
    newArgument->expression = newString;
    newArguments->at(0) = newArgument;
    newArgument = newMethodCall->arguments->at(1)->clone();
    auto* oldType = newArgument->expression->to<IR::StructExpression>()->structType;
    auto newStructExpression =new IR::StructExpression(oldType->srcInfo, structType,
        structType->getP4Type()->template to<IR::Type_Name>(), exprVector.first);
    newStructExpression->components = exprVector.first;
    typeMap->setType(structType, structType);
    typeMap->setType(newStructExpression, structType);
    newArgument->expression = newStructExpression;
    newArguments->at(1) = newArgument;
    newMethodCall->arguments = newArguments;
    auto* newMethodCallStatement = methodCallStatement->clone();
    newMethodCallStatement->methodCall = newMethodCall;
    logMsgReplacament.emplace(methodCallStatement->clone_id, newMethodCallStatement);
    return newMethodCallStatement;
}


IR::ID FindTypeInLogMsgForReplace::newName() {
    std::ostringstream ostr;
    ostr << "f" << index++;
    return IR::ID(ostr.str());
}

const IR::Type_StructLike* FindTypeInLogMsgForReplace::generateNewStructType(
    const IR::Type_StructLike* structType, IR::IndexedVector<IR::NamedExpression> &v) {
    IR::IndexedVector<IR::StructField> fields;
    for (auto namedExpr : v) {
        auto* structField = new IR::StructField(namedExpr->srcInfo, namedExpr->name,
                                                namedExpr->expression->type);
        typeMap->setType(structField, structField->type);
        fields.push_back(structField);
    }
    auto* newType = structType->clone();
    newType->fields = fields;
    return newType;
}

TypeLogMsgParams FindTypeInLogMsgForReplace::unfoldStruct(const IR::Expression* expr,
        std::string strParam, std::string curName) {
    TypeLogMsgParams result;
    auto* exprType = typeMap->getType(expr, true);
    if (!expr->is<IR::StructExpression>()) {
        if (auto structType = exprType->to<IR::Type_StructLike>()) {
            result.second += "(";
            for (auto field : structType->fields) {
                std::string nm = field->name.name + std::string(":");
                auto* newMember = new IR::Member(field->type, expr, field->name);
                if (field->type->is<IR::Type_StructLike>()) {
                    auto curResult = unfoldStruct(newMember, strParam, field->name.name.c_str());
                    nm += curResult.second;
                    result.first.insert(result.first.end(), curResult.first.begin(),
                                        curResult.first.end());
                } else {
                    result.first.push_back(new IR::NamedExpression(expr->srcInfo, newName(),
                                                                   newMember));
                    nm += std::string("{}");
                }
                if (result.second.length() > 1)
                    result.second += ",";
                result.second += nm;
            }
            result.second += std::string(")");
            return result;
        }
        result.first.push_back(new IR::NamedExpression(expr->srcInfo, newName(), expr));
        result.second = curName + std::string(" : {}");
        return result;
    }
    auto* structExpr = expr->to<IR::StructExpression>();
    std::string strResult;
    for (auto namedExpr : structExpr->components) {
        size_t n = strParam.find("{}");
        if (n != std::string::npos) {
            strResult += strParam.substr(0, n);
            strParam = strParam.substr(n + 2);
        }
        exprType = typeMap->getType(namedExpr->expression, true);
        if (exprType->is<IR::Type_StructLike>()) {
            std::string nm;
            if (curName.length()) {
                nm = curName + std::string(".");
            }
            nm += namedExpr->name.name.c_str();
            auto innerFields = unfoldStruct(namedExpr->expression, "", nm);
            result.first.insert(result.first.end(), innerFields.first.begin(),
                                innerFields.first.end());
            strResult += innerFields.second;
        } else {
            strResult += "{}";
            result.first.push_back(
                new IR::NamedExpression(namedExpr->srcInfo, newName(), namedExpr->expression));
        }
    }
    result.second = strResult + strParam;
    return result;
}

bool FindTypeInLogMsgForReplace::preorder(const IR::MethodCallStatement* methodCallStatement) {
    if (hasStructInParameter(methodCallStatement)) {
        auto* newMethodCall = prepareLogMsgStatement(methodCallStatement);
        createReplacement(newMethodCall->methodCall->typeArguments->at(0)->
                          to<IR::Type_Struct>());
    }
    return false;
}

void FindTypeInLogMsgForReplace::createReplacement(const IR::Type_StructLike* type) {
    if (replacement.count(type->name)) {
        return;
    }
    replacement.emplace(type->name, type);
}

/////////////////////////////////

const IR::Node* ReplaceLogMsg::preorder(IR::P4Program* program) {
    if (findTypeInLogMsgForReplace->empty()) {
        // nothing to do
        prune();
    }
    return program;
}

const IR::Node* ReplaceLogMsg::postorder(IR::Type_Struct* typeStruct) {
    auto canon = typeMap->getTypeType(getOriginal(), true);
    auto name = canon->to<IR::Type_Struct>()->name;
    auto repl = findTypeInLogMsgForReplace->getReplacement(name);
    if (repl != nullptr) {
        LOG3("Replace " << typeStruct << " with " << repl);
        BUG_CHECK(repl->is<IR::Type_Struct>(), "%1% not a struct", typeStruct);
        return repl;
    }
    return typeStruct;
}

const IR::Node* ReplaceLogMsg::postorder(IR::MethodCallStatement* methodCallStatement) {
    if (auto* newMethod =
            findTypeInLogMsgForReplace->getReplacementMethodCall(methodCallStatement->clone_id)) {
        return newMethod;
    }
    return methodCallStatement;
}

}  // namespace P4
