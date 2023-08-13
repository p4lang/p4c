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

#include "ebpfTable.h"

#include <algorithm>
#include <limits>
#include <string>

#include "backends/ebpf/ebpfModel.h"
#include "backends/ebpf/ebpfObject.h"
#include "backends/ebpf/ebpfOptions.h"
#include "backends/ebpf/ebpfProgram.h"
#include "ebpfType.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/parameterSubstitution.h"
#include "frontends/p4/typeMap.h"
#include "ir/declaration.h"
#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/vector.h"
#include "lib/enumerator.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/exceptions.h"
#include "lib/map.h"
#include "lib/stringify.h"

namespace EBPF {

bool ActionTranslationVisitor::preorder(const IR::PathExpression *expression) {
    if (isActionParameter(expression)) {
        cstring paramInstanceName = getParamInstanceName(expression);
        builder->append(paramInstanceName.c_str());
        return false;
    }
    visit(expression->path);
    return false;
}

bool ActionTranslationVisitor::isActionParameter(const IR::PathExpression *expression) const {
    auto decl = program->refMap->getDeclaration(expression->path, true);
    if (decl->is<IR::Parameter>()) {
        auto param = decl->to<IR::Parameter>();
        return action->parameters->getParameter(param->name) == param;
    }
    return false;
}

cstring ActionTranslationVisitor::getParamInstanceName(const IR::Expression *expression) const {
    cstring actionName = EBPFObject::externalName(action);
    auto paramStr =
        Util::printf_format("%s->u.%s.%s", valueName, actionName, expression->toString());
    return paramStr;
}

bool ActionTranslationVisitor::preorder(const IR::P4Action *act) {
    action = act;
    visit(action->body);
    return false;
}

////////////////////////////////////////////////////////////////

EBPFTable::EBPFTable(const EBPFProgram *program, const IR::TableBlock *table,
                     CodeGenInspector *codeGen)
    : EBPFTableBase(program, EBPFObject::externalName(table->container), codeGen), table(table) {
    auto sizeProperty =
        table->container->properties->getProperty(IR::TableProperties::sizePropertyName);
    if (sizeProperty != nullptr) {
        auto expr = sizeProperty->value->to<IR::ExpressionValue>()->expression;
        this->size = expr->to<IR::Constant>()->asInt();
    }

    cstring base = instanceName + "_defaultAction";
    defaultActionMapName = base;

    keyGenerator = table->container->getKey();
    actionList = table->container->getActionList();

    initKey();
}

EBPFTable::EBPFTable(const EBPFProgram *program, CodeGenInspector *codeGen, cstring name)
    : EBPFTableBase(program, name, codeGen),
      keyGenerator(nullptr),
      actionList(nullptr),
      table(nullptr) {}

void EBPFTable::initKey() {
    if (keyGenerator != nullptr) {
        unsigned fieldNumber = 0;
        for (auto c : keyGenerator->keyElements) {
            if (c->matchType->path->name.name == "selector")
                continue;  // this match type is intended for ActionSelector, not table itself

            auto type = program->typeMap->getType(c->expression);
            auto ebpfType = EBPFTypeFactory::instance->create(type);
            if (!ebpfType->is<IHasWidth>()) {
                ::error(ErrorType::ERR_TYPE_ERROR, "%1%: illegal type %2% for key field", c, type);
                return;
            }

            cstring fieldName = cstring("field") + Util::toString(fieldNumber);
            keyTypes.emplace(c, ebpfType);
            keyFieldNames.emplace(c, fieldName);
            fieldNumber++;
        }
    }
}

// Performs the following validations:
// - Validates if LPM key is the last one from match keys in an LPM table (ignores selector fields).
// - Validates if match fields in ternary tables are sorted by size
//   in descending order (ignores selector fields).
void EBPFTable::validateKeys() const {
    if (keyGenerator == nullptr) return;

    if (isTernaryTable()) {
        unsigned last_key_size = std::numeric_limits<unsigned>::max();
        for (auto it : keyGenerator->keyElements) {
            if (it->matchType->path->name.name == "selector") continue;

            auto type = program->typeMap->getType(it->expression);
            auto ebpfType = EBPFTypeFactory::instance->create(type);
            if (!ebpfType->is<IHasWidth>()) continue;

            unsigned width = ebpfType->to<IHasWidth>()->widthInBits();
            if (width > last_key_size) {
                ::error(ErrorType::WARN_ORDERING,
                        "%1%: key field larger than previous key, move it before previous key "
                        "to avoid padding between these keys",
                        it->expression);
                return;
            }
            last_key_size = width;
        }
    } else {
        auto lastKey =
            std::find_if(keyGenerator->keyElements.rbegin(), keyGenerator->keyElements.rend(),
                         [](const IR::KeyElement *key) {
                             return key->matchType->path->name.name != "selector";
                         });

        for (auto it : keyGenerator->keyElements) {
            auto mtdecl = program->refMap->getDeclaration(it->matchType->path, true);
            auto matchType = mtdecl->getNode()->to<IR::Declaration_ID>();
            if (matchType->name.name == P4::P4CoreLibrary::instance().lpmMatch.name) {
                if (it != *lastKey) {
                    ::error(ErrorType::ERR_UNSUPPORTED,
                            "%1% field key must be at the end of whole key", it->matchType);
                }
            }
        }
    }
}

void EBPFTable::emitKeyType(CodeBuilder *builder) {
    builder->emitIndent();
    builder->appendFormat("struct %s ", keyTypeName.c_str());
    builder->blockStart();

    CodeGenInspector commentGen(program->refMap, program->typeMap);
    commentGen.setBuilder(builder);

    unsigned int structAlignment = 4;  // 4 by default
    if (keyGenerator != nullptr) {
        if (isLPMTable()) {
            // For LPM kind key we need an additional 32 bit field - prefixlen
            auto prefixType = EBPFTypeFactory::instance->create(IR::Type_Bits::get(32));
            builder->emitIndent();
            prefixType->declare(builder, prefixFieldName, false);
            builder->endOfStatement(true);
        }

        for (auto c : keyGenerator->keyElements) {
            auto mtdecl = program->refMap->getDeclaration(c->matchType->path, true);
            auto matchType = mtdecl->getNode()->to<IR::Declaration_ID>();

            if (!isMatchTypeSupported(matchType)) {
                ::error(ErrorType::ERR_UNSUPPORTED, "Match of type %1% not supported",
                        c->matchType);
            }

            if (matchType->name.name == "selector") {
                builder->emitIndent();
                builder->append("/* ");
                c->expression->apply(commentGen);
                builder->append(" : selector */");
                builder->newline();
                continue;
            }

            auto ebpfType = ::get(keyTypes, c);
            cstring fieldName = ::get(keyFieldNames, c);

            if (ebpfType->is<EBPFScalarType>() &&
                ebpfType->to<EBPFScalarType>()->alignment() > structAlignment) {
                structAlignment = 8;
            }

            builder->emitIndent();
            ebpfType->declare(builder, fieldName, false);

            builder->append("; /* ");
            c->expression->apply(commentGen);
            builder->append(" */");
            builder->newline();
        }
    }

    // Add dummy key if P4 table define table with empty key. This due to that hash map
    // cannot have zero-length key. See function htab_map_alloc_check in kernel/bpf/hashtab.c
    // located in Linux kernel repository
    if (keyFieldNames.empty()) {
        builder->emitIndent();
        builder->appendLine("u8 __dummy_table_key;");
    }

    builder->blockEnd(false);
    builder->appendFormat(" __attribute__((aligned(%d)))", structAlignment);
    builder->endOfStatement(true);

    if (isTernaryTable()) {
        // generate mask key
        builder->emitIndent();
        builder->appendFormat("#define MAX_%s_MASKS %u", keyTypeName.toUpper(),
                              program->options.maxTernaryMasks);
        builder->newline();

        builder->emitIndent();
        builder->appendFormat("struct %s_mask ", keyTypeName.c_str());
        builder->blockStart();

        builder->emitIndent();
        builder->appendFormat("__u8 mask[sizeof(struct %s)];", keyTypeName.c_str());
        builder->newline();

        builder->blockEnd(false);
        builder->appendFormat(" __attribute__((aligned(%d)))", structAlignment);
        builder->endOfStatement(true);
    }
}

void EBPFTable::emitActionArguments(CodeBuilder *builder, const IR::P4Action *action,
                                    cstring name) {
    builder->emitIndent();
    builder->append("struct ");
    builder->blockStart();

    for (auto p : *action->parameters->getEnumerator()) {
        builder->emitIndent();
        auto type = EBPFTypeFactory::instance->create(p->type);
        type->declare(builder, p->externalName(), false);
        builder->endOfStatement(true);
    }

    builder->blockEnd(false);
    builder->spc();
    builder->append(name);
    builder->endOfStatement(true);
}

void EBPFTable::emitValueType(CodeBuilder *builder) {
    emitValueActionIDNames(builder);

    // a type-safe union: a struct with a tag and an union
    builder->emitIndent();
    builder->appendFormat("struct %s ", valueTypeName.c_str());
    builder->blockStart();

    emitValueStructStructure(builder);
    emitDirectValueTypes(builder);

    builder->blockEnd(false);
    builder->endOfStatement(true);

    if (isTernaryTable()) {
        // emit ternary mask value
        builder->emitIndent();
        builder->appendFormat("struct %s_mask ", valueTypeName.c_str());
        builder->blockStart();

        builder->emitIndent();
        builder->appendLine("__u32 tuple_id;");
        builder->emitIndent();
        builder->appendFormat("struct %s_mask next_tuple_mask;", keyTypeName.c_str());
        builder->newline();
        builder->emitIndent();
        builder->appendLine("__u8 has_next;");
        builder->blockEnd(false);
        builder->endOfStatement(true);
    }
}

void EBPFTable::emitValueActionIDNames(CodeBuilder *builder) {
    // create type definition for action
    builder->emitIndent();
    unsigned int action_idx = 1;  // 0 is reserved for NoAction
    for (auto a : actionList->actionList) {
        auto adecl = program->refMap->getDeclaration(a->getPath(), true);
        auto action = adecl->getNode()->to<IR::P4Action>();
        // no need to define a constant for NoAction,
        // "case 0" will be explicitly generated in the action handling switch
        if (action->name.originalName == P4::P4CoreLibrary::instance().noAction.name) {
            continue;
        }
        builder->emitIndent();
        builder->appendFormat("#define %s %d", p4ActionToActionIDName(action), action_idx);
        builder->newline();
        action_idx++;
    }
    builder->emitIndent();
}

void EBPFTable::emitValueStructStructure(CodeBuilder *builder) {
    builder->emitIndent();
    builder->append("unsigned int action;");
    builder->newline();

    if (isTernaryTable()) {
        builder->emitIndent();
        builder->append("__u32 priority;");
        builder->newline();
    }

    builder->emitIndent();
    builder->append("union ");
    builder->blockStart();

    // Declare NoAction data structure at the beginning as it has reserved id 0
    builder->emitIndent();
    builder->appendLine("struct {");
    builder->emitIndent();
    builder->append("} _NoAction");
    builder->endOfStatement(true);

    for (auto a : actionList->actionList) {
        auto adecl = program->refMap->getDeclaration(a->getPath(), true);
        auto action = adecl->getNode()->to<IR::P4Action>();
        if (action->name.originalName == P4::P4CoreLibrary::instance().noAction.name) continue;
        cstring name = EBPFObject::externalName(action);
        emitActionArguments(builder, action, name);
    }

    builder->blockEnd(false);
    builder->spc();
    builder->appendLine("u;");
}

void EBPFTable::emitTypes(CodeBuilder *builder) {
    validateKeys();
    emitKeyType(builder);
    emitValueType(builder);
}

void EBPFTable::emitTernaryInstance(CodeBuilder *builder) {
    builder->target->emitTableDecl(builder, instanceName + "_prefixes", TableHash,
                                   "struct " + keyTypeName + "_mask",
                                   "struct " + valueTypeName + "_mask", size);
    builder->target->emitMapInMapDecl(builder, instanceName + "_tuple", TableHash,
                                      "struct " + keyTypeName, "struct " + valueTypeName, size,
                                      instanceName + "_tuples_map", TableArray, "__u32", size);
}

void EBPFTable::emitInstance(CodeBuilder *builder) {
    if (isTernaryTable()) {
        emitTernaryInstance(builder);
    } else {
        if (keyGenerator != nullptr) {
            auto impl =
                table->container->properties->getProperty(program->model.tableImplProperty.name);
            if (impl == nullptr) {
                ::error(ErrorType::ERR_EXPECTED, "Table %1% does not have an %2% property",
                        table->container, program->model.tableImplProperty.name);
                return;
            }

            // Some type checking...
            if (!impl->value->is<IR::ExpressionValue>()) {
                ::error(ErrorType::ERR_EXPECTED, "%1%: Expected property to be an `extern` block",
                        impl);
                return;
            }

            auto expr = impl->value->to<IR::ExpressionValue>()->expression;
            if (!expr->is<IR::ConstructorCallExpression>()) {
                ::error(ErrorType::ERR_EXPECTED, "%1%: Expected property to be an `extern` block",
                        impl);
                return;
            }

            auto block = table->getValue(expr);
            if (block == nullptr || !block->is<IR::ExternBlock>()) {
                ::error(ErrorType::ERR_EXPECTED, "%1%: Expected property to be an `extern` block",
                        impl);
                return;
            }

            TableKind tableKind;
            auto extBlock = block->to<IR::ExternBlock>();
            if (extBlock->type->name.name == program->model.array_table.name) {
                tableKind = TableArray;
            } else if (extBlock->type->name.name == program->model.hash_table.name) {
                tableKind = TableHash;
            } else {
                ::error(ErrorType::ERR_EXPECTED, "%1%: implementation must be one of %2% or %3%",
                        impl, program->model.array_table.name, program->model.hash_table.name);
                return;
            }

            // If any key field is LPM we will generate an LPM table
            for (auto it : keyGenerator->keyElements) {
                auto mtdecl = program->refMap->getDeclaration(it->matchType->path, true);
                auto matchType = mtdecl->getNode()->to<IR::Declaration_ID>();
                if (matchType->name.name == P4::P4CoreLibrary::instance().lpmMatch.name) {
                    if (tableKind == TableLPMTrie) {
                        ::error(ErrorType::ERR_UNSUPPORTED, "%1%: only one LPM field allowed",
                                it->matchType);
                        return;
                    }
                    tableKind = TableLPMTrie;
                }
            }

            auto sz = extBlock->getParameterValue(program->model.array_table.size.name);
            if (sz == nullptr || !sz->is<IR::Constant>()) {
                ::error(ErrorType::ERR_UNSUPPORTED,
                        "%1%: Expected an integer argument; is the model corrupted?", expr);
                return;
            }
            auto cst = sz->to<IR::Constant>();
            if (!cst->fitsInt()) {
                ::error(ErrorType::ERR_UNSUPPORTED, "%1%: size too large", cst);
                return;
            }
            int size = cst->asInt();
            if (size <= 0) {
                ::error(ErrorType::ERR_INVALID, "%1%: negative size", cst);
                return;
            }

            cstring name = EBPFObject::externalName(table->container);
            builder->target->emitTableDecl(builder, name, tableKind,
                                           cstring("struct ") + keyTypeName,
                                           cstring("struct ") + valueTypeName, size);
        }
    }
    builder->target->emitTableDecl(builder, defaultActionMapName, TableArray,
                                   program->arrayIndexType, cstring("struct ") + valueTypeName, 1);
}

void EBPFTable::emitKey(CodeBuilder *builder, cstring keyName) {
    if (keyGenerator == nullptr) {
        return;
    }

    if (isLPMTable()) {
        builder->emitIndent();
        builder->appendFormat("%s.%s = sizeof(%s)*8 - %d", keyName.c_str(), prefixFieldName,
                              keyName, prefixLenFieldWidth);
        builder->endOfStatement(true);
    }

    for (auto c : keyGenerator->keyElements) {
        auto ebpfType = ::get(keyTypes, c);
        cstring fieldName = ::get(keyFieldNames, c);
        if (fieldName == nullptr || ebpfType == nullptr) continue;
        bool memcpy = false;
        EBPFScalarType *scalar = nullptr;
        cstring swap;
        if (ebpfType->is<EBPFScalarType>()) {
            scalar = ebpfType->to<EBPFScalarType>();
            unsigned width = scalar->implementationWidthInBits();
            memcpy = !EBPFScalarType::generatesScalar(width);

            if (width <= 8) {
                swap = "";  // single byte, nothing to swap
            } else if (width <= 16) {
                swap = "bpf_htons";
            } else if (width <= 32) {
                swap = "bpf_htonl";
            } else if (width <= 64) {
                swap = "bpf_htonll";
            } else {
                // The code works with fields wider than 64 bits for PSA architecture. It is shared
                // with filter model, so should work but has not been tested. Error message is
                // preserved for filter model because existing tests expect it.
                // TODO: handle width > 64 bits for filter model
                if (program->options.arch.isNullOrEmpty() || program->options.arch == "filter") {
                    ::error(ErrorType::ERR_UNSUPPORTED,
                            "%1%: fields wider than 64 bits are not supported yet", fieldName);
                }
            }
        }

        bool isLPMKeyBigEndian = false;
        if (isLPMTable()) {
            if (c->matchType->path->name.name == P4::P4CoreLibrary::instance().lpmMatch.name)
                isLPMKeyBigEndian = true;
        }

        builder->emitIndent();
        if (memcpy) {
            builder->appendFormat("__builtin_memcpy(&(%s.%s[0]), &(", keyName.c_str(),
                                  fieldName.c_str());
            codeGen->visit(c->expression);
            builder->appendFormat("[0]), %d)", scalar->bytesRequired());
        } else {
            builder->appendFormat("%s.%s = ", keyName.c_str(), fieldName.c_str());
            if (isLPMKeyBigEndian) builder->appendFormat("%s(", swap.c_str());
            codeGen->visit(c->expression);
            if (isLPMKeyBigEndian) builder->append(")");
        }
        builder->endOfStatement(true);

        cstring msgStr, varStr;
        if (memcpy) {
            msgStr = Util::printf_format("Control: key %s", c->expression->toString());
            builder->target->emitTraceMessage(builder, msgStr.c_str());
        } else {
            msgStr = Util::printf_format("Control: key %s=0x%%llx", c->expression->toString());
            varStr = Util::printf_format("(unsigned long long) %s.%s", keyName.c_str(),
                                         fieldName.c_str());
            builder->target->emitTraceMessage(builder, msgStr.c_str(), 1, varStr.c_str());
        }
    }
}

void EBPFTable::emitAction(CodeBuilder *builder, cstring valueName, cstring actionRunVariable) {
    builder->emitIndent();
    builder->appendFormat("switch (%s->action) ", valueName.c_str());
    builder->blockStart();

    for (auto a : actionList->actionList) {
        auto adecl = program->refMap->getDeclaration(a->getPath(), true);
        auto action = adecl->getNode()->to<IR::P4Action>();
        cstring name = EBPFObject::externalName(action), msgStr, convStr;
        builder->emitIndent();
        cstring actionName = p4ActionToActionIDName(action);
        builder->appendFormat("case %s: ", actionName);
        builder->newline();
        builder->increaseIndent();

        msgStr = Util::printf_format("Control: executing action %s", name);
        builder->target->emitTraceMessage(builder, msgStr.c_str());
        for (auto param : *(action->parameters)) {
            auto etype = EBPFTypeFactory::instance->create(param->type);
            unsigned width = dynamic_cast<IHasWidth *>(etype)->widthInBits();

            if (width <= 64) {
                convStr = Util::printf_format("(unsigned long long) (%s->u.%s.%s)", valueName, name,
                                              param->toString());
                msgStr = Util::printf_format("Control: param %s=0x%%llx (%d bits)",
                                             param->toString(), width);
                builder->target->emitTraceMessage(builder, msgStr.c_str(), 1, convStr.c_str());
            } else {
                msgStr =
                    Util::printf_format("Control: param %s (%d bits)", param->toString(), width);
                builder->target->emitTraceMessage(builder, msgStr.c_str());
            }
        }

        builder->emitIndent();

        auto visitor = createActionTranslationVisitor(valueName, program);
        visitor->setBuilder(builder);
        visitor->copySubstitutions(codeGen);
        visitor->copyPointerVariables(codeGen);

        action->apply(*visitor);
        builder->newline();
        builder->emitIndent();
        builder->appendLine("break;");
        builder->decreaseIndent();
    }

    builder->emitIndent();
    builder->appendLine("default:");
    builder->increaseIndent();
    builder->target->emitTraceMessage(builder, "Control: Invalid action type, aborting");

    builder->emitIndent();
    builder->appendFormat("return %s", builder->target->abortReturnCode().c_str());
    builder->endOfStatement(true);
    builder->decreaseIndent();

    builder->blockEnd(true);

    if (!actionRunVariable.isNullOrEmpty()) {
        builder->emitIndent();
        builder->appendFormat("%s = %s->action", actionRunVariable.c_str(), valueName.c_str());
        builder->endOfStatement(true);
    }
}

void EBPFTable::emitInitializer(CodeBuilder *builder) {
    // emit code to initialize the default action
    const IR::P4Table *t = table->container;
    const IR::Expression *defaultAction = t->getDefaultAction();
    BUG_CHECK(defaultAction->is<IR::MethodCallExpression>(), "%1%: expected an action call",
              defaultAction);
    auto mce = defaultAction->to<IR::MethodCallExpression>();
    auto mi = P4::MethodInstance::resolve(mce, program->refMap, program->typeMap);

    auto ac = mi->to<P4::ActionCall>();
    BUG_CHECK(ac != nullptr, "%1%: expected an action call", mce);
    auto action = ac->action;
    cstring name = EBPFObject::externalName(action);
    cstring fd = "tableFileDescriptor";
    cstring defaultTable = defaultActionMapName;
    cstring value = "value";
    cstring key = "key";

    builder->emitIndent();
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("int %s = BPF_OBJ_GET(MAP_PATH \"/%s\")", fd.c_str(),
                          defaultTable.c_str());
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("if (%s < 0) { fprintf(stderr, \"map %s not loaded\\n\"); exit(1); }",
                          fd.c_str(), defaultTable.c_str());
    builder->newline();

    builder->emitIndent();
    builder->appendFormat("struct %s %s = ", valueTypeName.c_str(), value.c_str());
    builder->blockStart();
    builder->emitIndent();
    cstring actionName = p4ActionToActionIDName(action);
    builder->appendFormat(".action = %s,", actionName);
    builder->newline();

    CodeGenInspector cg(program->refMap, program->typeMap);
    cg.setBuilder(builder);

    builder->emitIndent();
    builder->appendFormat(".u = {.%s = {", name.c_str());
    for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
        auto arg = mi->substitution.lookup(p);
        arg->apply(cg);
        builder->append(",");
    }
    builder->append("}},\n");

    builder->blockEnd(false);
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->append("int ok = ");
    builder->target->emitUserTableUpdate(builder, fd, program->zeroKey, value);
    builder->newline();

    builder->emitIndent();
    builder->appendFormat(
        "if (ok != 0) { "
        "perror(\"Could not write in %s\"); exit(1); }",
        defaultTable.c_str());
    builder->newline();
    builder->blockEnd(true);

    // Emit code for table initializer
    auto entries = t->getEntries();
    if (entries == nullptr) return;

    builder->emitIndent();
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("int %s = BPF_OBJ_GET(MAP_PATH \"/%s\")", fd.c_str(),
                          dataMapName.c_str());
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("if (%s < 0) { fprintf(stderr, \"map %s not loaded\\n\"); exit(1); }",
                          fd.c_str(), dataMapName.c_str());
    builder->newline();

    for (auto e : entries->entries) {
        builder->emitIndent();
        builder->blockStart();

        auto entryAction = e->getAction();
        builder->emitIndent();
        builder->appendFormat("struct %s %s = {", keyTypeName.c_str(), key.c_str());
        e->getKeys()->apply(cg);
        builder->append("}");
        builder->endOfStatement(true);

        BUG_CHECK(entryAction->is<IR::MethodCallExpression>(), "%1%: expected an action call",
                  defaultAction);
        auto mce = entryAction->to<IR::MethodCallExpression>();
        auto mi = P4::MethodInstance::resolve(mce, program->refMap, program->typeMap);

        auto ac = mi->to<P4::ActionCall>();
        BUG_CHECK(ac != nullptr, "%1%: expected an action call", mce);
        auto action = ac->action;
        cstring name = EBPFObject::externalName(action);

        builder->emitIndent();
        builder->appendFormat("struct %s %s = ", valueTypeName.c_str(), value.c_str());
        builder->blockStart();
        builder->emitIndent();
        cstring actionName = p4ActionToActionIDName(action);
        builder->appendFormat(".action = %s,", actionName);
        builder->newline();

        CodeGenInspector cg(program->refMap, program->typeMap);
        cg.setBuilder(builder);

        builder->emitIndent();
        builder->appendFormat(".u = {.%s = {", name.c_str());
        for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
            auto arg = mi->substitution.lookup(p);
            arg->apply(cg);
            builder->append(",");
        }
        builder->append("}},\n");

        builder->blockEnd(false);
        builder->endOfStatement(true);

        builder->emitIndent();
        builder->append("int ok = ");
        builder->target->emitUserTableUpdate(builder, fd, key, value);
        builder->newline();

        builder->emitIndent();
        builder->appendFormat(
            "if (ok != 0) { "
            "perror(\"Could not write in %s\"); exit(1); }",
            t->name.name.c_str());
        builder->newline();
        builder->blockEnd(true);
    }
    builder->blockEnd(true);
}

void EBPFTable::emitLookup(CodeBuilder *builder, cstring key, cstring value) {
    if (cacheEnabled()) emitCacheLookup(builder, key, value);

    if (!isTernaryTable()) {
        builder->target->emitTableLookup(builder, dataMapName, key, value);
        builder->endOfStatement(true);
        return;
    }

    // for ternary tables
    builder->appendFormat("struct %s_mask head = {0};", keyTypeName);
    builder->newline();
    builder->emitIndent();
    builder->appendFormat("struct %s_mask *", valueTypeName);
    builder->target->emitTableLookup(builder, instanceName + "_prefixes", "head", "val");
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->append("if (val && val->has_next != 0) ");
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("struct %s_mask next = val->next_tuple_mask;", keyTypeName);
    builder->newline();
    builder->emitIndent();
    builder->appendLine("#pragma clang loop unroll(disable)");
    builder->emitIndent();
    builder->appendFormat("for (int i = 0; i < MAX_%s_MASKS; i++) ", keyTypeName.toUpper());
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("struct %s_mask *", valueTypeName);
    builder->target->emitTableLookup(builder, instanceName + "_prefixes", "next", "v");
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->append("if (!v) ");
    builder->blockStart();
    builder->target->emitTraceMessage(builder, "Control: No next element found!");
    builder->emitIndent();
    builder->appendLine("break;");
    builder->blockEnd(true);
    builder->emitIndent();
    cstring new_key = "k";
    builder->appendFormat("struct %s %s = {};", keyTypeName, new_key);
    builder->newline();
    builder->emitIndent();
    builder->appendFormat("__u32 *chunk = ((__u32 *) &%s);", new_key);
    builder->newline();
    builder->emitIndent();
    builder->appendLine("__u32 *mask = ((__u32 *) &next);");
    builder->emitIndent();
    builder->appendLine("#pragma clang loop unroll(disable)");
    builder->emitIndent();
    builder->appendFormat("for (int i = 0; i < sizeof(struct %s_mask) / 4; i++) ", keyTypeName);
    builder->blockStart();
    cstring str = Util::printf_format("*(((__u32 *) &%s) + i)", key);
    builder->target->emitTraceMessage(
        builder, "Control: [Ternary] Masking next 4 bytes of %llx with mask %llx", 2, str,
        "mask[i]");

    builder->emitIndent();
    builder->appendFormat("chunk[i] = ((__u32 *) &%s)[i] & mask[i];", key);
    builder->newline();
    builder->blockEnd(true);

    builder->emitIndent();
    builder->appendLine("__u32 tuple_id = v->tuple_id;");
    builder->emitIndent();
    builder->append("next = v->next_tuple_mask;");
    builder->newline();
    builder->emitIndent();
    builder->append("struct bpf_elf_map *");
    builder->target->emitTableLookup(builder, instanceName + "_tuples_map", "tuple_id", "tuple");
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->append("if (!tuple) ");
    builder->blockStart();
    builder->target->emitTraceMessage(
        builder, Util::printf_format("Control: Tuples map %s not found during ternary lookup. Bug?",
                                     instanceName));
    builder->emitIndent();
    builder->append("break;");
    builder->newline();
    builder->blockEnd(true);

    builder->emitIndent();
    builder->appendFormat(
        "struct %s *tuple_entry = "
        "bpf_map_lookup_elem(%s, &%s)",
        valueTypeName, "tuple", new_key);
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->append("if (!tuple_entry) ");
    builder->blockStart();
    builder->emitIndent();
    builder->append("if (v->has_next == 0) ");
    builder->blockStart();
    builder->emitIndent();
    builder->appendLine("break;");
    builder->blockEnd(true);
    builder->emitIndent();
    builder->append("continue;");
    builder->newline();
    builder->blockEnd(true);
    builder->target->emitTraceMessage(builder, "Control: Ternary match found, priority=%d.", 1,
                                      "tuple_entry->priority");

    builder->emitIndent();
    builder->appendFormat("if (%s == NULL || tuple_entry->priority > %s->priority) ", value, value);
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("%s = tuple_entry;", value);
    builder->newline();
    builder->blockEnd(true);

    builder->emitIndent();
    builder->append("if (v->has_next == 0) ");
    builder->blockStart();
    builder->emitIndent();
    builder->appendLine("break;");
    builder->blockEnd(true);
    builder->blockEnd(true);
    builder->blockEnd(true);
}

cstring EBPFTable::p4ActionToActionIDName(const IR::P4Action *action) const {
    if (action->name.originalName == P4::P4CoreLibrary::instance().noAction.name) {
        // NoAction always gets ID=0.
        return "0";
    }

    cstring actionName = EBPFObject::externalName(action);
    cstring tableInstance = dataMapName;
    return Util::printf_format("%s_ACT_%s", tableInstance.toUpper(), actionName.toUpper());
}

// As ternary has precedence over lpm, this function checks if any
// field is key field is lpm and none of key fields is of type ternary.
bool EBPFTable::isLPMTable() const {
    bool isLPM = false;
    if (keyGenerator != nullptr) {
        // If any key field is LPM we will generate an LPM table
        for (auto it : keyGenerator->keyElements) {
            auto mtdecl = program->refMap->getDeclaration(it->matchType->path, true);
            auto matchType = mtdecl->getNode()->to<IR::Declaration_ID>();
            if (matchType->name.name == P4::P4CoreLibrary::instance().ternaryMatch.name) {
                // if there is a ternary field, we are sure, it is not an LPM table.
                return false;
            } else if (matchType->name.name == P4::P4CoreLibrary::instance().lpmMatch.name) {
                isLPM = true;
            }
        }
    }

    return isLPM;
}

bool EBPFTable::isTernaryTable() const {
    if (keyGenerator != nullptr) {
        // If any key field is a ternary field we will generate a ternary table
        for (auto it : keyGenerator->keyElements) {
            auto mtdecl = program->refMap->getDeclaration(it->matchType->path, true);
            auto matchType = mtdecl->getNode()->to<IR::Declaration_ID>();
            if (matchType->name.name == P4::P4CoreLibrary::instance().ternaryMatch.name) {
                return true;
            }
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////

EBPFCounterTable::EBPFCounterTable(const EBPFProgram *program, const IR::ExternBlock *block,
                                   cstring name, CodeGenInspector *codeGen)
    : EBPFTableBase(program, name, codeGen) {
    auto sz = block->getParameterValue(program->model.counterArray.max_index.name);
    if (sz == nullptr || !sz->is<IR::Constant>()) {
        ::error(ErrorType::ERR_INVALID,
                "%1% (%2%): expected an integer argument; is the model corrupted?",
                program->model.counterArray.max_index, name);
        return;
    }
    auto cst = sz->to<IR::Constant>();
    if (!cst->fitsInt()) {
        ::error(ErrorType::ERR_OVERLIMIT, "%1%: size too large", cst);
        return;
    }
    size = cst->asInt();
    if (size <= 0) {
        ::error(ErrorType::ERR_OVERLIMIT, "%1%: negative size", cst);
        return;
    }

    auto sprs = block->getParameterValue(program->model.counterArray.sparse.name);
    if (sprs == nullptr || !sprs->is<IR::BoolLiteral>()) {
        ::error(ErrorType::ERR_INVALID,
                "%1% (%2%): Expected an integer argument; is the model corrupted?",
                program->model.counterArray.sparse, name);
        return;
    }

    isHash = sprs->to<IR::BoolLiteral>()->value;
}

void EBPFCounterTable::emitInstance(CodeBuilder *builder) {
    TableKind kind = isHash ? TableHash : TableArray;
    builder->target->emitTableDecl(builder, dataMapName, kind, keyTypeName, valueTypeName, size);
}

void EBPFCounterTable::emitCounterIncrement(CodeBuilder *builder,
                                            const IR::MethodCallExpression *expression) {
    cstring keyName = program->refMap->newName("key");
    cstring valueName = program->refMap->newName("value");

    builder->emitIndent();
    builder->append(valueTypeName);
    builder->spc();
    builder->append("*");
    builder->append(valueName);
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->append(valueTypeName);
    builder->spc();
    builder->appendLine("init_val = 1;");

    builder->emitIndent();
    builder->append(keyTypeName);
    builder->spc();
    builder->append(keyName);
    builder->append(" = ");

    BUG_CHECK(expression->arguments->size() == 1, "Expected just 1 argument for %1%", expression);
    auto arg = expression->arguments->at(0);

    codeGen->visit(arg);
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->target->emitTableLookup(builder, dataMapName, keyName, valueName);
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->appendFormat("if (%s != NULL)", valueName.c_str());
    builder->newline();
    builder->increaseIndent();
    builder->emitIndent();
    builder->appendFormat("__sync_fetch_and_add(%s, 1);", valueName.c_str());
    builder->newline();
    builder->decreaseIndent();

    builder->emitIndent();
    builder->appendLine("else");
    builder->increaseIndent();
    builder->emitIndent();
    builder->target->emitTableUpdate(builder, dataMapName, keyName, "init_val");
    builder->newline();
    builder->decreaseIndent();
}

void EBPFCounterTable::emitCounterAdd(CodeBuilder *builder,
                                      const IR::MethodCallExpression *expression) {
    cstring keyName = program->refMap->newName("key");
    cstring valueName = program->refMap->newName("value");
    cstring incName = program->refMap->newName("inc");

    builder->emitIndent();
    builder->append(valueTypeName);
    builder->spc();
    builder->append("*");
    builder->append(valueName);
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->append(valueTypeName);
    builder->spc();
    builder->appendLine("init_val = 1;");

    builder->emitIndent();
    builder->append(keyTypeName);
    builder->spc();
    builder->append(keyName);
    builder->append(" = ");

    BUG_CHECK(expression->arguments->size() == 2, "Expected just 2 arguments for %1%", expression);
    auto index = expression->arguments->at(0);

    codeGen->visit(index);
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->append(valueTypeName);
    builder->spc();
    builder->append(incName);
    builder->append(" = ");

    auto inc = expression->arguments->at(1);

    codeGen->visit(inc);
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->target->emitTableLookup(builder, dataMapName, keyName, valueName);
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->appendFormat("if (%s != NULL)", valueName.c_str());
    builder->newline();
    builder->increaseIndent();
    builder->emitIndent();
    builder->appendFormat("__sync_fetch_and_add(%s, %s);", valueName.c_str(), incName.c_str());
    builder->newline();
    builder->decreaseIndent();

    builder->emitIndent();
    builder->appendLine("else");
    builder->increaseIndent();
    builder->emitIndent();
    builder->target->emitTableUpdate(builder, dataMapName, keyName, "init_val");
    builder->newline();
    builder->decreaseIndent();
}

void EBPFCounterTable::emitMethodInvocation(CodeBuilder *builder, const P4::ExternMethod *method) {
    if (method->method->name.name == program->model.counterArray.increment.name) {
        emitCounterIncrement(builder, method->expr);
        return;
    }
    if (method->method->name.name == program->model.counterArray.add.name) {
        emitCounterAdd(builder, method->expr);
        return;
    }
    ::error(ErrorType::ERR_UNSUPPORTED, "Unexpected method %1% for %2%", method->expr,
            program->model.counterArray.name);
}

void EBPFCounterTable::emitTypes(CodeBuilder *builder) {
    builder->emitIndent();
    builder->appendFormat("typedef %s %s", EBPFModel::instance.counterIndexType.c_str(),
                          keyTypeName.c_str());
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("typedef %s %s", EBPFModel::instance.counterValueType.c_str(),
                          valueTypeName.c_str());
    builder->endOfStatement(true);
}

////////////////////////////////////////////////////////////////

EBPFValueSet::EBPFValueSet(const EBPFProgram *program, const IR::P4ValueSet *p4vs,
                           cstring instanceName, CodeGenInspector *codeGen)
    : EBPFTableBase(program, instanceName, codeGen), size(0), pvs(p4vs) {
    CHECK_NULL(pvs);
    valueTypeName = "u32";  // map value is not used, so its type can be anything

    // validate size
    if (pvs->size->is<IR::Constant>()) {
        auto sc = pvs->size->to<IR::Constant>();
        if (sc->fitsUint()) size = sc->asUnsigned();
        if (size == 0)
            ::error(ErrorType::ERR_OVERLIMIT,
                    "Size must be a positive value less than 2^32, got %1% entries", pvs->size);
    } else {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                "Size of value_set must be know at compilation time: %1%", pvs->size);
    }

    // validate type
    auto elemType = program->typeMap->getTypeType(pvs->elementType, true);
    if (elemType->is<IR::Type_Bits>() || elemType->is<IR::Type_Tuple>()) {
        // no restrictions
    } else if (elemType->is<IR::Type_Struct>()) {
        keyTypeName = elemType->to<IR::Type_Struct>()->name.name;
    } else if (auto h = elemType->to<IR::Type_Header>()) {
        keyTypeName = h->name.name;

        ::warning("Header type may contain additional shadow data: %1%", pvs->elementType);
        ::warning("Header defined here: %1%", h);
    } else {
        ::error(ErrorType::ERR_UNSUPPORTED, "Unsupported type with value_set: %1%",
                pvs->elementType);
    }

    keyTypeName = "struct " + keyTypeName;
}

void EBPFValueSet::emitTypes(CodeBuilder *builder) {
    auto elemType = program->typeMap->getTypeType(pvs->elementType, true);

    if (auto tsl = elemType->to<IR::Type_StructLike>()) {
        for (auto field : tsl->fields) {
            fieldNames.emplace_back(std::make_pair(field->name.name, field->type));
        }
        // Do not re-declare this type
        return;
    }

    builder->emitIndent();
    builder->appendFormat("%s ", keyTypeName.c_str());
    builder->blockStart();

    auto fieldEmitter = [builder](const IR::Type *type, cstring name) {
        auto etype = EBPFTypeFactory::instance->create(type);
        builder->emitIndent();
        etype->declare(builder, name, false);
        builder->endOfStatement(true);
    };

    if (auto tb = elemType->to<IR::Type_Bits>()) {
        cstring name = "field0";
        fieldEmitter(tb, name);
        fieldNames.emplace_back(std::make_pair(name, tb));
    } else if (auto tuple = elemType->to<IR::Type_Tuple>()) {
        int i = 0;
        for (auto field : tuple->components) {
            cstring name = Util::printf_format("field%d", i++);
            fieldEmitter(field, name);
            fieldNames.emplace_back(std::make_pair(name, field));
        }
    } else {
        BUG("Type for value_set not implemented %1%", pvs->elementType);
    }

    builder->blockEnd(false);
    builder->endOfStatement(true);
}

void EBPFValueSet::emitInstance(CodeBuilder *builder) {
    builder->target->emitTableDecl(builder, instanceName, TableKind::TableHash, keyTypeName,
                                   valueTypeName, size);
}

void EBPFValueSet::emitKeyInitializer(CodeBuilder *builder, const IR::SelectExpression *expression,
                                      cstring varName) {
    if (fieldNames.size() != expression->select->components.size()) {
        ::error(ErrorType::ERR_EXPECTED,
                "Fields number of value_set do not match number of arguments: %1%", expression);
        return;
    }
    keyVarName = varName;
    builder->emitIndent();
    builder->appendFormat("%s %s = {0}", keyTypeName.c_str(), keyVarName.c_str());
    builder->endOfStatement(true);

    for (unsigned int i = 0; i < fieldNames.size(); i++) {
        bool useMemcpy = true;
        if (fieldNames.at(i).second->is<IR::Type_Bits>()) {
            if (fieldNames.at(i).second->to<IR::Type_Bits>()->width_bits() <= 64) useMemcpy = false;
        }
        builder->emitIndent();

        auto keyExpr = expression->select->components.at(i);
        if (useMemcpy) {
            if (keyExpr->is<IR::Mask>()) {
                ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                        "%1%: mask not supported for fields larger than 64 bits within value_set",
                        keyExpr);
                continue;
            }

            cstring dst =
                Util::printf_format("%s.%s", keyVarName.c_str(), fieldNames.at(i).first.c_str());
            builder->appendFormat("__builtin_memcpy(&%s, &(", dst.c_str());
            codeGen->visit(keyExpr);
            builder->appendFormat("[0]), sizeof(%s))", dst.c_str());
        } else {
            builder->appendFormat("%s.%s = ", keyVarName.c_str(), fieldNames.at(i).first.c_str());
            if (auto mask = keyExpr->to<IR::Mask>()) {
                builder->append("((");
                codeGen->visit(mask->left);
                builder->append(") & (");
                codeGen->visit(mask->right);
                builder->append("))");
            } else {
                codeGen->visit(keyExpr);
            }
        }

        builder->endOfStatement(true);
    }
}

void EBPFValueSet::emitLookup(CodeBuilder *builder) {
    builder->target->emitTableLookup(builder, instanceName, keyVarName, "");
}

}  // namespace EBPF
