/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "simplify_args.h"

#include <queue>

#include "bf-p4c/common/utils.h"
#include "bf-p4c/midend/path_linearizer.h"

namespace BFN {

/**
 * @brief Detects if IR::Expression passed as an argument is a structure nested inside a header.
 *
 * @param expr Expression which we want to check.
 * @return true If an expression passed as an argument is a structure nested inside a header.
 * @return false If not.
 */
bool InjectTmpVar::DoInject::isNestedStruct(const IR::Expression *expr) {
    bool foundStruct = false;
    while (expr->is<IR::Member>() && expr->type->is<IR::Type_Struct>()) {
        LOG4("Nested struct member: " << expr << ", type: " << expr->type);
        foundStruct = true;
        expr = expr->to<IR::Member>()->expr;
    }

    if (foundStruct && (expr->is<IR::PathExpression>() || expr->is<IR::Member>()) &&
        expr->type->is<IR::Type_Header>()) {
        LOG3("Found nested struct in header: " << expr << ", type: " << expr->type);
        return true;
    }
    return false;
}

const IR::Node *InjectTmpVar::DoInject::postorder(IR::AssignmentStatement *as) {
    auto mce = as->right->to<IR::MethodCallExpression>();
    if (!mce) return as;

    auto retTypeStruct = mce->type->to<IR::Type_Struct>();
    if (!retTypeStruct) return as;

    auto mi = P4::MethodInstance::resolve(mce, refMap, typeMap);
    if (!mi->is<P4::ExternMethod>()) return as;

    if (!isNestedStruct(as->left)) return as;

    IR::IndexedVector<IR::StatOrDecl> statements;
    IR::ID tmpVarName(refMap->newName("tmp"));
    auto tmpVarType = new IR::Type_Name(retTypeStruct->name);

    statements.push_back(new IR::Declaration_Variable(tmpVarName, tmpVarType, mce));
    statements.push_back(new IR::AssignmentStatement(as->getSourceInfo(), as->left,
                                                     new IR::PathExpression(tmpVarName)));

    LOG3("Replacing AssignmentStatement:" << std::endl
                                          << "  " << as << std::endl
                                          << "by statements:");
    for (auto st : statements) {
        LOG3("  " << st);
    }
    return new IR::BlockStatement(statements);
}

const IR::Node *InjectTmpVar::DoInject::postorder(IR::MethodCallStatement *mcs) {
    auto mce = mcs->methodCall;
    if (!mce) return mcs;

    auto mi = P4::MethodInstance::resolve(mce, refMap, typeMap);
    if (!mi->is<P4::ExternMethod>()) return mcs;
    auto em = mi->to<P4::ExternMethod>();

    P4::ParameterSubstitution paramSub;
    paramSub.populate(em->method->getParameters(), mce->arguments);

    IR::IndexedVector<IR::StatOrDecl> before;
    IR::IndexedVector<IR::StatOrDecl> after;
    auto newArgs = new IR::Vector<IR::Argument>();

    for (auto param : em->method->getParameters()->parameters) {
        auto arg = paramSub.lookup(param);
        if (param->type->is<IR::Type_Struct>() && isNestedStruct(arg->expression)) {
            LOG3("Processing argument: " << arg << " for parameter: " << param);
            cstring newName = refMap->newName(param->name.string_view());
            newArgs->push_back(
                new IR::Argument(arg->srcInfo, arg->name, new IR::PathExpression(newName)));
            // Save arguments of in and inout parameters into local variables
            if (param->direction == IR::Direction::In || param->direction == IR::Direction::InOut) {
                auto vardecl = new IR::Declaration_Variable(newName, param->annotations,
                                                            param->type, arg->expression);
                before.push_back(vardecl);
            } else if (param->direction == IR::Direction::Out) {
                auto vardecl =
                    new IR::Declaration_Variable(newName, param->annotations, param->type);
                before.push_back(vardecl);
            }
            // Copy out and inout parameters
            if (param->direction == IR::Direction::InOut ||
                param->direction == IR::Direction::Out) {
                auto right = new IR::PathExpression(newName);
                auto copyout =
                    new IR::AssignmentStatement(mcs->getSourceInfo(), arg->expression, right);
                after.push_back(copyout);
            }
        } else {
            newArgs->push_back(arg);
        }
    }

    /**
     * If this is empty it means that we don't need any new temporary variables so
     * we don't modify the original statement.
     */
    if (before.empty()) return mcs;

    auto mceClone = mce->clone();
    mceClone->arguments = newArgs;
    mcs->methodCall = mceClone;

    IR::IndexedVector<IR::StatOrDecl> statements;
    statements.append(before);
    statements.push_back(mcs);
    statements.append(after);

    LOG3("Replacing MethodCallStatement:" << std::endl
                                          << "  " << mcs << std::endl
                                          << "by statements:");
    for (auto st : statements) {
        LOG3("  " << st);
    }
    return new IR::BlockStatement(statements);
}

void FlattenHeader::flattenType(const IR::Type *type) {
    if (auto st = type->to<IR::Type_StructLike>()) {
        allAnnotations.push_back(st->annotations);
        for (auto f : st->fields) {
            nameSegments.push_back(f->name);
            allAnnotations.push_back(f->annotations);
            srcInfos.push_back(f->srcInfo);
            flattenType(typeMap->getType(f, true));
            srcInfos.pop_back();
            allAnnotations.pop_back();
            nameSegments.pop_back();
        }
        allAnnotations.pop_back();
    } else {  // primitive field types
        auto &newFields = flattenedHeader->fields;
        auto newName = makeName("_");
        auto origName = makeName(".");
        if (origName != newName) {
            LOG2("map " << origName << " to " << newName);
            fieldNameMap.emplace(origName, newName);
        }
        // preserve the original name using an annotation
        auto annotations = mergeAnnotations();
        newFields.push_back(
            new IR::StructField(srcInfos.back(), IR::ID(newName), annotations, type));
    }
}

cstring FlattenHeader::makeName(std::string_view sep) const {
    std::string name;
    for (auto n : nameSegments) {
        name += sep;
        name += n;
    }
    /// removing leading separator
    name = name.substr(1);
    return name;
}

/**
 * Merge all the annotation vectors in allAnnotations into a single
 * one. Duplicates are resolved, with preference given to the ones towards the
 * end of allAnnotations, which correspond to the most "nested" ones.
 */
const IR::Annotations *FlattenHeader::mergeAnnotations() const {
    auto mergedAnnotations = new IR::Annotations();
    for (auto annosIt = allAnnotations.rbegin(); annosIt != allAnnotations.rend(); annosIt++) {
        for (auto anno : (*annosIt)->annotations) {
            // if an annotation with the same name was added previously, skip
            if (mergedAnnotations->getSingle(anno->name)) continue;
            mergedAnnotations->add(anno);
        }
    }
    return mergedAnnotations;
}

bool FlattenHeader::preorder(IR::Type_Header *headerType) {
    if (policy(getChildContext(), headerType)) {
        LOG3("\t skip flattening " << headerType->name << " due to policy");
        return false;
    }
    LOG3("visiting header " << headerType);
    flattenedHeader = headerType->clone();
    flattenedHeader->fields.clear();
    flattenType(headerType);
    headerType->fields = flattenedHeader->fields;
    return false;
}

cstring FlattenHeader::makeMember(std::string_view sep) const {
    std::string name;
    for (auto n = memberSegments.rbegin(); n != memberSegments.rend(); n++) {
        name += sep;
        name += *n;
    }
    /// removing leading separator
    name = name.substr(1);
    return name;
}

/**
 * The IR for member is structured as:
 *
 *     IR::Member
 *         IR::Member
 *             IR::Member
 *                 IR::PathExpression
 *
 * Suppose we would like to transform a path from hdr.meta.nested.field to
 * hdr.meta.nested_field.
 * The IR tree should be transformed to:
 *
 *      IR::Member
 *          IR::Member
 *              IR::PathExpression
 */
void FlattenHeader::flattenMember(const IR::Member *member) {
    if (memberSegments.size() == 0) {
        flattenedMember = member;
    }
    memberSegments.push_back(member->member);
    if (auto mem = member->expr->to<IR::Member>()) {
        flattenMember(mem);
    }
    auto origName = makeMember(".");
    bool isHeader = member->expr->type->is<IR::Type_Header>();
    LOG3("   check " << (isHeader ? "header " : "struct ") << member->expr << " " << origName);
    if (!isHeader) {
        memberSegments.pop_back();
        return;
    }
    if (fieldNameMap.count(origName)) {
        const IR::Expression *header;
        if (member->expr->is<IR::PathExpression>())
            header = member->expr->to<IR::PathExpression>();
        else if (member->expr->is<IR::Member>())
            header = member->expr;
        else
            BUG("Unexpected member expression %1%", member->expr);
        auto field = fieldNameMap.at(origName);
        LOG3("     map " << flattenedMember->toString() << " to " << "(" << header << ", " << field
                         << ")");
        replacementMap.emplace(flattenedMember->toString(), std::make_tuple(header, field));
    }
    memberSegments.pop_back();
}

bool FlattenHeader::preorder(IR::Member *member) {
    LOG2("preorder " << member);
    flattenMember(member);
    memberSegments.clear();
    return true;
}

int FlattenHeader::memberDepth(const IR::Member *m) {
    int depth = 1;
    while (const auto *child = m->expr->to<IR::Member>()) {
        m = child;
        depth++;
    }
    return depth;
}

const IR::Member *FlattenHeader::getTailMembers(const IR::Member *m, int depth) {
    std::queue<const IR::Member *> members;

    members.push(m);
    while (const auto *child = m->expr->to<IR::Member>()) {
        m = child;
        if (members.size() == size_t(depth)) members.pop();
        members.push(child);
    }

    BUG_CHECK(members.size() == size_t(depth), "Insufficient nested members; expected %1%", depth);

    return members.front();
}

const IR::PathExpression *FlattenHeader::replaceSrcInfo(const IR::PathExpression *tgt,
                                                        const IR::PathExpression *src) {
    const auto *tgtPath = tgt->path;
    const auto *srcPath = src->path;

    IR::Path *newPath = new IR::Path(
        srcPath->srcInfo, IR::ID(srcPath->name.srcInfo, tgtPath->name.name), tgtPath->absolute);
    IR::PathExpression *newPathExpr = new IR::PathExpression(src->srcInfo, tgt->type, newPath);

    return newPathExpr;
}

const IR::Member *FlattenHeader::replaceSrcInfo(const IR::Member *tgt, const IR::Member *src) {
    const auto *tgtChildExpr = tgt->expr;
    const auto *srcChildExpr = src->expr;

    const auto *tgtChildMember = tgtChildExpr->to<IR::Member>();
    const auto *srcChildMember = srcChildExpr->to<IR::Member>();
    if (tgtChildMember && srcChildMember) {
        tgtChildExpr = replaceSrcInfo(tgtChildMember, srcChildMember);
    } else {
        const auto *tgtChildPathExpr = tgtChildExpr->to<IR::PathExpression>();
        const auto *srcChildPathExpr = srcChildExpr->to<IR::PathExpression>();
        if (tgtChildPathExpr && srcChildPathExpr) {
            tgtChildExpr = replaceSrcInfo(tgtChildPathExpr, srcChildPathExpr);
        }
    }

    IR::Member *newMember = new IR::Member(src->srcInfo, tgt->type, tgtChildExpr,
                                           IR::ID(src->member.srcInfo, tgt->member.name));

    return newMember;
}

const IR::Expression *FlattenHeader::balancedReplaceSrcInfo(const IR::Expression *tgt,
                                                            const IR::Member *src) {
    if (const auto *tgt_member = tgt->to<IR::Member>()) {
        int depth = memberDepth(tgt_member);
        src = getTailMembers(src, depth);
        return replaceSrcInfo(tgt_member, src);
    } else {
        const auto *tgt_pe = tgt->to<IR::PathExpression>();
        CHECK_NULL(tgt_pe);

        while (const auto *child = src->expr->to<IR::Member>()) {
            src = child;
        }
        const auto *src_pe = src->expr->to<IR::PathExpression>();
        CHECK_NULL(src_pe);

        return replaceSrcInfo(tgt_pe, src_pe);
    }
}

void FlattenHeader::postorder(IR::Member *member) {
    auto mem = member->toString();
    LOG2("postorder " << mem);
    if (replacementMap.count(mem)) {
        LOG2("    replace " << member->expr << "." << member->member << " with "
                            << std::get<0>(replacementMap.at(mem)) << "."
                            << std::get<1>(replacementMap.at(mem)));
        // Ideally, we would just do:
        //
        //   member->expr = std::get<0>(replacementMap.at(mem))->apply(cloner);
        //   member->member = std::get<1>(replacementMap.at(mem));
        //
        // The problem with this is that if we have mutliple instances of a single flattened field,
        // then all instances will have the same srcInfo. This potentially breaks ResolutionContext
        // because ResolutionContext verifies that a name being looked up appears after the
        // corresponding declaration, but now all instances of the field name will use the srcInfo
        // of the first instance.
        //
        // Fix by copying the srcInfo from the original instance to the cloned flattened instance.

        // The flattened expression should be a Member or a PathExpression
        member->expr = balancedReplaceSrcInfo(std::get<0>(replacementMap.at(mem)), member);

        member->member = std::get<1>(replacementMap.at(mem));
    }
}

bool FlattenHeader::preorder(IR::MethodCallExpression *mc) {
    auto method = mc->method->to<IR::Member>();
    if (!method || method->member != "emit") return true;

    auto dname = method->expr->to<IR::PathExpression>();
    if (!dname) return false;

    auto type = dname->type->to<IR::Type_Extern>();
    if (!type) return false;

    if (type->name != "Mirror" && type->name != "Resubmit" && type->name != "Digest") return false;

    // HACK(Han): after the P4-14 to TNA refactor, we should
    // derive these indices from the TNA model.
    // Assume the following syntax
    //   mirror.emit(session_id, field_list);
    //   mirror.emit(); - mirror without parameters
    //   resubmit.emit(field_list);
    //   resubmit.emit(); - resubmit without parameters
    //   digest.emit(field_list);
    //   digest.emit(); - digest without parameters
    int field_list_index = (type->name == "Mirror") ? 1 : 0;
    if (mc->arguments->size() == 0) return false;

    if (type->name == "Mirror" && mc->arguments->size() != 2) {
        return false;
    }

    auto *arg = mc->arguments->at(field_list_index);
    auto *aexpr = arg->expression;
    if (auto *liste = aexpr->to<IR::ListExpression>()) {
        LOG4("Flattening arguments: " << liste);
        auto *flattened_args = new IR::Vector<IR::Argument>();
        if (type->name == "Mirror") flattened_args->push_back(mc->arguments->at(0));
        flattened_args->push_back(new IR::Argument(flatten_list(liste)));
        mc->arguments = flattened_args;
        LOG4("Flattened arguments: " << mc);
    } else if (auto *liste = aexpr->to<IR::StructExpression>()) {
        LOG4("Flattening arguments: " << liste);
        auto *flattened_args = new IR::Vector<IR::Argument>();
        if (type->name == "Mirror") flattened_args->push_back(mc->arguments->at(0));
        auto flattenedList = doFlattenStructInitializer(liste);
        flattened_args->push_back(new IR::Argument(flattenedList));
        mc->arguments = flattened_args;
        LOG4("Flattened arguments: " << mc);
    } else {
        // do not try to simplify reference to header
    }
    return true;
}

void FlattenHeader::explode(const IR::Expression *expression, IR::Vector<IR::Expression> *output) {
    if (auto st = expression->type->to<IR::Type_Header>()) {
        for (auto f : st->fields) {
            auto e = new IR::Member(expression, f->name);
            LOG1("visit " << f);
            explode(e, output);
        }
    } else {
        BUG_CHECK(
            !expression->type->is<IR::Type_StructLike>() && !expression->type->is<IR::Type_Stack>(),
            "%1%: unexpected type", expression->type);
        output->push_back(expression);
    }
}

/**
 * The name scheme of flattened struct initializer expression is by
 * concatenating the parent name + field name + global count.
 *
 * We should clean up the frontend flattenHeaders pass to not
 * append a digit after each field.
 */
cstring FlattenHeader::makePath(std::string_view sep) const {
    std::string name;
    for (auto n : pathSegments) {
        name += sep;
        name += n;
    }
    name = name.substr(1);
    return name;
}

void FlattenHeader::flattenStructInitializer(const IR::StructExpression *expr,
                                             IR::IndexedVector<IR::NamedExpression> *components) {
    for (auto e : expr->components) {
        pathSegments.push_back(e->name);
        if (const auto *c = e->expression->to<IR::StructExpression>()) {
            flattenStructInitializer(c, components);
        } else if (auto *member = e->expression->to<IR::Member>()) {
            if (auto type = member->type->to<IR::Type_Struct>()) {
                for (auto f : type->fields) {
                    pathSegments.push_back(f->name);
                    auto newName = makePath("_");
                    auto newField = new IR::Member(member, f->name);
                    auto namedExpression = new IR::NamedExpression(IR::ID(newName), newField);
                    pathSegments.pop_back();
                    components->push_back(namedExpression);
                }
            } else {
                auto newName = makePath("_");
                auto namedExpression = new IR::NamedExpression(IR::ID(newName), e->expression);
                components->push_back(namedExpression);
            }
        } else {
            auto newName = makePath("_");
            auto namedExpression = new IR::NamedExpression(IR::ID(newName), e->expression);
            components->push_back(namedExpression);
        }
        pathSegments.pop_back();
    }
}

IR::StructExpression *FlattenHeader::doFlattenStructInitializer(const IR::StructExpression *e) {
    // removing nested StructInitailzierExpression
    auto no_nested_struct = new IR::IndexedVector<IR::NamedExpression>();
    flattenStructInitializer(e, no_nested_struct);
    return new IR::StructExpression(e->srcInfo, e->structType, *no_nested_struct);
}

IR::ListExpression *FlattenHeader::flatten_list(const IR::ListExpression *args) {
    IR::Vector<IR::Expression> components;
    for (const auto *expr : args->components) {
        if (const auto *list_arg = expr->to<IR::ListExpression>()) {
            auto *flattened = flatten_list(list_arg);
            for (const auto *comp : flattened->components) components.push_back(comp);
        } else {
            components.push_back(expr);
        }
    }
    return new IR::ListExpression(components);
}

const IR::Node *RewriteTypeArguments::preorder(IR::Type_Struct *typeStruct) {
    for (auto &t : eeh->rewriteTupleType) {
        if (typeStruct->name == t.first) {
            typeStruct->fields = {};
            for (auto comp : t.second) {
                auto field = new IR::StructField(comp->name, comp->expression->type);
                typeStruct->fields.push_back(field);
            }
        }
    }
    return typeStruct;
}

const IR::Node *RewriteTypeArguments::preorder(IR::MethodCallExpression *mc) {
    if (eeh->rewriteOtherType.empty()) return mc;
    for (auto &type : eeh->rewriteOtherType) {
        if (type.first->equiv(*mc)) {
            auto *typeArguments = new IR::Vector<IR::Type>();
            typeArguments->push_back(type.second);
            mc->typeArguments = typeArguments;
        }
    }
    return mc;
}

const IR::Node *EliminateHeaders::preorder(IR::Argument *arg) {
    auto mc = findContext<IR::MethodCallExpression>();
    if (!mc) return arg;
    auto method = mc->method->to<IR::Member>();
    if (!method || method->member == "subtract_all_and_deposit") return arg;
    auto mi = P4::MethodInstance::resolve(mc, refMap, typeMap, true);
    auto em = mi->to<P4::ExternMethod>();
    if (!em) return arg;
    cstring extName = em->actualExternType->name;
    if (extName == "Checksum") {
        auto fieldVectorList = IR::IndexedVector<IR::NamedExpression>();
        std::map<cstring, int> namesUsed;
        auto origlist = IR::IndexedVector<IR::NamedExpression>();
        const IR::Type *structType = nullptr;
        if (arg->expression->is<IR::StructExpression>()) {
            origlist = arg->expression->to<IR::StructExpression>()->components;
            structType = arg->expression->to<IR::StructExpression>()->structType;
        } else if (auto c = arg->expression->to<IR::Member>()) {
            if (c->type->is<IR::Type_Header>()) {
                origlist.push_back(new IR::NamedExpression(c->member, c));
                structType = new IR::Type_Name(c->type->to<IR::Type_Header>()->name);
            }
        }
        for (auto expr : origlist) {
            if (auto header = expr->expression->type->to<IR::Type_Header>()) {
                for (auto f : header->fields) {
                    IR::ID name = f->name;
                    if (namesUsed.count(name.name)) {
                        name = IR::ID(name + "_renamed_" +
                                      cstring::to_cstring(namesUsed[name.name]++));
                    } else {
                        namesUsed[name.name] = 1;
                    }
                    fieldVectorList.push_back(new IR::NamedExpression(
                        name, new IR::Member(f->type, expr->expression, f->name)));
                }
            } else if (auto st = expr->expression->type->to<IR::Type_Struct>()) {
                for (auto f : st->fields) {
                    IR::ID name = f->name;
                    if (namesUsed.count(name.name)) {
                        name = IR::ID(name + "_renamed_" +
                                      cstring::to_cstring(namesUsed[name.name]++));
                    } else {
                        namesUsed[name.name] = 1;
                    }
                    fieldVectorList.push_back(new IR::NamedExpression(
                        name, new IR::Member(f->type, expr->expression, f->name)));
                }
            } else if (auto concat = expr->expression->to<IR::Concat>()) {
                IR::IndexedVector<IR::NamedExpression> concatList;
                elimConcat(concatList, concat);
                fieldVectorList.append(concatList);
            } else if (expr->expression->is<IR::Member>() || expr->expression->is<IR::Constant>() ||
                       expr->expression->is<IR::PathExpression>()) {
                namesUsed[expr->name.name]++;
                fieldVectorList.push_back(expr);
            } else {
                error(ErrorType::ERR_UNEXPECTED, " unexpected type of parameter %2% in %1%",
                      extName, expr->expression);
            }
        }
        if (fieldVectorList.size()) {
            IR::StructExpression *list;
            list = new IR::StructExpression(structType, fieldVectorList);
            if (mc->typeArguments->size()) {
                if (auto type = mc->typeArguments->at(0)->to<IR::Type_Name>()) {
                    rewriteTupleType[type->path->name] = list->components;
                } else {
                    rewriteOtherType[mc] = list->structType;
                }
            }
            return (new IR::Argument(arg->srcInfo, list));
        }
    } else if (extName == "Digest" || extName == "Mirror" || extName == "Resubmit" ||
               extName == "Hash") {
        const IR::Type_StructLike *type = nullptr;
        if (auto path = arg->expression->to<IR::PathExpression>()) {
            if (path->type->is<IR::Type_StructLike>()) {
                type = path->type->to<IR::Type_StructLike>();
            } else {
                return arg;
            }
        } else if (auto mem = arg->expression->to<IR::Member>()) {
            if (mem->type->is<IR::Type_StructLike>()) {
                type = mem->type->to<IR::Type_StructLike>();
            } else {
                return arg;
            }
        }

        if (!type) return arg;

        if (policy(getChildContext(), type)) return arg;

        cstring type_name;
        auto fieldList = IR::IndexedVector<IR::NamedExpression>();
        if (auto header = type->to<IR::Type_Header>()) {
            for (auto f : header->fields) {
                auto mem = new IR::Member(arg->expression, f->name);
                fieldList.push_back(new IR::NamedExpression(f->name, mem));
            }
            type_name = header->name;
        } else if (auto st = type->to<IR::Type_Struct>()) {
            for (auto f : st->fields) {
                auto mem = new IR::Member(arg->expression, f->name);
                fieldList.push_back(new IR::NamedExpression(f->name, mem));
            }
            type_name = st->name;
        } else {
            error(ErrorType::ERR_UNEXPECTED, " unexpected type of parameter %2% in %1%", extName,
                  arg);
        }

        if (fieldList.size() > 0)
            return new IR::Argument(
                arg->srcInfo, new IR::StructExpression(new IR::Type_Name(type_name), fieldList));
    }
    return arg;
}

void EliminateHeaders::elimConcat(IR::IndexedVector<IR::NamedExpression> &output,
                                  const IR::Concat *expr) {
    static int i = 0;
    if (!expr) return;
    if (expr->left->is<IR::Concat>())
        elimConcat(output, expr->left->to<IR::Concat>());
    else
        output.push_back(
            new IR::NamedExpression(IR::ID("concat_" + cstring::to_cstring(i++)), expr->left));

    if (expr->right->is<IR::Concat>())
        elimConcat(output, expr->right->to<IR::Concat>());
    else
        output.push_back(
            new IR::NamedExpression(IR::ID("concat_" + cstring::to_cstring(i++)), expr->right));
}
}  // namespace BFN
