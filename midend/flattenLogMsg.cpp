#include "midend/flattenLogMsg.h"
#include "flattenLogMsg.h"

namespace P4 {

FlattenLogMsg::FlattenLogMsg(P4::ReferenceMap* refMap, P4::TypeMap* typeMap) :
    refMap(refMap), typeMap(typeMap) {
}

const IR::Node* FlattenLogMsg::preorder(IR::BlockStatement* blockStatement) {
    IR::IndexedVector<IR::StatOrDecl> components;
    for (auto i : blockStatement->components) {
        FlattenLogMsg flattenLogMsg(refMap, typeMap);
        auto* newStatement = i->apply(flattenLogMsg);
        if (auto* block = newStatement->to<IR::BlockStatement>()) {
            components.insert(components.end(), block->components.begin(),
                              block->components.end());
        } else {
            components.push_back(newStatement->to<IR::StatOrDecl>());
        }
    }
    auto* block = blockStatement->clone();
    block->components = components;
    return block;
}

std::pair<IR::Vector<IR::Expression>, std::string> FlattenLogMsg::unfoldStruct(
    const IR::Expression* expr, const IR::Type_StructLike* typeStruct, std::string& strParam) {
    IR::Vector<IR::Expression> result;
    std::string strResult;
    for (auto field : typeStruct->fields) {
        size_t n = strParam.find("{}");
        if (n != std::string::npos) {
            strResult += strParam.substr(0, n);
            strParam = strParam.substr(n + 2); 
        }
        auto* newMember = new IR::Member(expr->srcInfo, field->type, expr, field->name);
        std::cout << "member : " << newMember << "\n type : " << newMember->type << std::endl;
        if (auto *fieldTypeStruct = field->type->to<IR::Type_StructLike>()) {
            std::string curRes;
            auto innerFields = unfoldStruct(newMember, fieldTypeStruct, curRes);
            result.insert(result.end(), innerFields.first.begin(), innerFields.first.end());
            strResult += "(";
            strResult += innerFields.second;
            strResult += ")";
        } else {
            if (expr->is<IR::Member>()) {
                strResult += field->name;
                strResult += "=";
            }
            strResult += "{}";
            if (expr->is<IR::Member>()) {
                strResult += " ";
            }
            result.push_back(newMember);
        }
    }
    return std::make_pair(result, strResult);
}

const IR::Node* FlattenLogMsg::postorder(IR::MethodCallStatement* methodCallStatement) {
    if (auto* path = methodCallStatement->methodCall->method->to<IR::PathExpression>()) {
        if (path->path->name.name == "log_msg") {
            auto* param1 = methodCallStatement->methodCall->arguments->at(0)->expression;
            auto* param2 = methodCallStatement->methodCall->arguments->at(1)->expression;
            if (!param1->is<IR::StringLiteral>() || !param2->is<IR::StructExpression>()) {
                return methodCallStatement;
            }
            std::string strParam1 = param1->to<IR::StringLiteral>()->value.c_str();
            auto* argType =
                typeMap->getTypeType(methodCallStatement->methodCall->typeArguments->at(0), true);
            std::cout << argType << " : " << argType->node_type_name() << std::endl;
            if (argType->is<IR::Type_StructLike>()) {
                std::cout << methodCallStatement->methodCall->arguments->at(0)->expression->node_type_name() << " : " << methodCallStatement->methodCall->arguments->at(0)->expression->type << std::endl;
            std::cout << methodCallStatement->methodCall->arguments->at(1)->expression->node_type_name() << " : " << methodCallStatement->methodCall->arguments->at(1)->expression->type << std::endl;
              auto exprVector = 
                unfoldStruct(methodCallStatement->methodCall->arguments->at(1)->expression,
                             argType->to<IR::Type_StructLike>(), strParam1);
              auto* newBlock = new IR::BlockStatement();
              for (auto newExpr : exprVector.first) {
                  std::cout << "expr : " << newExpr << "\ntype : " << newExpr->type << std::endl;
                  auto* newMethodCall = methodCallStatement->methodCall->clone();
                  auto* newTypeArguments = newMethodCall->typeArguments->clone();
                  newTypeArguments->clear();
                  newTypeArguments->push_back(newExpr->type);
                  newMethodCall->typeArguments = newTypeArguments;
                  auto* newArguments = newMethodCall->arguments->clone(); 
                  auto* newArgument = newMethodCall->arguments->at(1)->clone();
                  newArgument->expression = newExpr;
                  newArguments->at(1) = newArgument;
                  newMethodCall->arguments = newArguments;
                  auto* newMethodCallStatement = methodCallStatement->clone();
                  newMethodCallStatement->methodCall = newMethodCall;
                  std::cout << "new st : " << newMethodCallStatement << std::endl;
                  newBlock->components.push_back(newMethodCallStatement);
              }
              return newBlock;
            }
        }
    }
    return methodCallStatement;
}

}  // namespace P4
