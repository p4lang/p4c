/*
Copyright 2022-present Orange
Copyright 2022-present Open Networking Foundation

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

#include "ebpfPsaTableImplementation.h"

#include "backends/ebpf/psa/ebpfPsaControl.h"
#include "ebpfPsaHashAlgorithm.h"

namespace EBPF {

EBPFTableImplementationPSA::EBPFTableImplementationPSA(const EBPFProgram *program,
                                                       CodeGenInspector *codeGen,
                                                       const IR::Declaration_Instance *decl)
    : EBPFTablePSA(program, codeGen, externalName(decl)), declaration(decl) {
    referenceName = instanceName + "_key";
}

void EBPFTableImplementationPSA::emitTypes(CodeBuilder *builder) {
    if (table == nullptr) return;
    // key is u32, so do not emit type for it
    emitValueType(builder);
    emitCacheTypes(builder);
}

void EBPFTableImplementationPSA::emitInitializer(CodeBuilder *builder) { (void)builder; }

void EBPFTableImplementationPSA::emitReferenceEntry(CodeBuilder *builder) {
    builder->emitIndent();
    builder->appendFormat("u32 %s", referenceName.c_str());
    builder->endOfStatement(true);
}

void EBPFTableImplementationPSA::registerTable(const EBPFTablePSA *instance) {
    // verify table instance
    verifyTableNoEntries(instance);
    verifyTableNoDefaultAction(instance);
    verifyTableNoDirectObjects(instance);

    if (table == nullptr) {
        // no other tables at the moment, take it as a reference
        table = instance->table;
        actionList = instance->actionList;
    } else {
        // another table, check that new instance has the same action list
        verifyTableActionList(instance);
    }
}

void EBPFTableImplementationPSA::verifyTableActionList(const EBPFTablePSA *instance) {
    bool printError = false;
    if (actionList == nullptr) return;

    auto getActionName = [this](const IR::ActionList *al, size_t id) -> cstring {
        auto pe = this->getActionNameExpression(al->actionList.at(id)->expression);
        CHECK_NULL(pe);
        return pe->path->name.originalName;
    };

    if (instance->actionList->size() == actionList->size()) {
        for (size_t i = 0; i < actionList->size(); ++i) {
            auto left = getActionName(instance->actionList, i);
            auto right = getActionName(actionList, i);
            if (left != right) printError = true;
        }
    } else {
        printError = true;
    }

    if (printError) {
        ::error(ErrorType::ERR_EXPECTED,
                "%1%: Action list differs from previous %2% "
                "(tables use the same implementation %3%)",
                instance->table->container->getActionList(), table->container->getActionList(),
                declaration);
    }
}

void EBPFTableImplementationPSA::verifyTableNoDefaultAction(const EBPFTablePSA *instance) {
    auto defaultAction = instance->table->container->getDefaultAction();
    BUG_CHECK(defaultAction->is<IR::MethodCallExpression>(), "%1%: expected an action call",
              defaultAction);

    auto mi = P4::MethodInstance::resolve(defaultAction->to<IR::MethodCallExpression>(),
                                          program->refMap, program->typeMap);
    auto ac = mi->to<P4::ActionCall>();
    BUG_CHECK(ac != nullptr, "%1%: expected an action call", mi);

    if (ac->action->name.originalName != P4::P4CoreLibrary::instance().noAction.name) {
        ::error(ErrorType::ERR_UNSUPPORTED,
                "%1%: Default action cannot be defined for table %2% with implementation %3%",
                defaultAction, instance->table->container->name, declaration);
    }
}

void EBPFTableImplementationPSA::verifyTableNoDirectObjects(const EBPFTablePSA *instance) {
    if (!instance->counters.empty() || !instance->meters.empty()) {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                "%1%: DirectCounter and DirectMeter externs are not supported "
                "with table implementation %2%",
                instance->table->container->name, declaration->type->toString());
    }
}

void EBPFTableImplementationPSA::verifyTableNoEntries(const EBPFTablePSA *instance) {
    // PSA documentation v1.1 says: "Directly specifying the action as part of the table
    //    entry is not allowed for tables with an action profile implementation."
    // I believe that this sentence forbids (const) entries in a table in P4 code at all.
    auto entries = instance->table->container->getEntries();
    if (entries != nullptr) {
        ::error(ErrorType::ERR_UNSUPPORTED,
                "%1%: entries directly specified in a table %2% "
                "with implementation %3% are not supported",
                entries, instance->table->container->name, declaration);
    }
}

unsigned EBPFTableImplementationPSA::getUintFromExpression(const IR::Expression *expr,
                                                           unsigned defaultValue) {
    if (!expr->is<IR::Constant>()) {
        ::error(ErrorType::ERR_UNSUPPORTED, "Must be constant value: %1%", expr);
        return defaultValue;
    }
    auto c = expr->to<IR::Constant>();
    if (!c->fitsUint()) {
        ::error(ErrorType::ERR_OVERLIMIT, "%1%: size too large", c);
        return defaultValue;
    }
    return c->asUnsigned();
}

// ===============================ActionProfile===============================

EBPFActionProfilePSA::EBPFActionProfilePSA(const EBPFProgram *program, CodeGenInspector *codeGen,
                                           const IR::Declaration_Instance *decl)
    : EBPFTableImplementationPSA(program, codeGen, decl) {
    size = getUintFromExpression(decl->arguments->at(0)->expression, 1);
}

void EBPFActionProfilePSA::emitInstance(CodeBuilder *builder) {
    if (table == nullptr)  // no table(s)
        return;

    // Control plane must have ability to know if given reference exists or is used.
    // Problem with TableArray: id of NoAction is 0 and default value of map entry is also 0.
    //   If user change action for given reference to NoAction, it will be hard to
    //   distinguish it from non-existing entry using only key value.
    auto tableKind = TableHash;
    builder->target->emitTableDecl(builder, instanceName, tableKind, "u32",
                                   cstring("struct ") + valueTypeName, size);
}

void EBPFActionProfilePSA::applyImplementation(CodeBuilder *builder, cstring tableValueName,
                                               cstring actionRunVariable) {
    cstring msg = Util::printf_format("ActionProfile: applying %s", instanceName.c_str());
    builder->target->emitTraceMessage(builder, msg.c_str());

    cstring apValueName = program->refMap->newName("ap_value");
    cstring apKeyName =
        Util::printf_format("%s->%s", tableValueName.c_str(), referenceName.c_str());

    builder->target->emitTraceMessage(builder, "ActionProfile: entry id %u", 1, apKeyName.c_str());

    builder->emitIndent();
    builder->appendFormat("struct %s *%s = NULL", valueTypeName.c_str(), apValueName.c_str());
    builder->endOfStatement(true);
    builder->emitIndent();
    emitLookup(builder, apKeyName, apValueName);

    builder->emitIndent();
    builder->appendFormat("if (%s != NULL) ", apValueName.c_str());
    builder->blockStart();

    // Here there is no need to set hit variable, because it is set to 1
    // during map lookup in table that owns this implementation.

    emitAction(builder, apValueName, actionRunVariable);

    builder->blockEnd(false);
    builder->append(" else ");

    builder->blockStart();
    builder->target->emitTraceMessage(
        builder, "ActionProfile: entry not found, executing implicit NoAction");
    builder->emitIndent();
    builder->appendFormat("%s = 0", program->control->hitVariable.c_str());
    builder->endOfStatement(true);
    builder->blockEnd(true);

    msg = Util::printf_format("ActionProfile: %s applied", instanceName.c_str());
    builder->target->emitTraceMessage(builder, msg.c_str());
}

// ===============================ActionSelector===============================

EBPFActionSelectorPSA::EBPFActionSelectorPSA(const EBPFProgram *program, CodeGenInspector *codeGen,
                                             const IR::Declaration_Instance *decl)
    : EBPFTableImplementationPSA(program, codeGen, decl), emptyGroupAction(nullptr) {
    hashEngine = EBPFHashAlgorithmTypeFactoryPSA::instance()->create(
        getUintFromExpression(decl->arguments->at(0)->expression, 0), program,
        instanceName + "_hash");
    if (hashEngine != nullptr) {
        hashEngine->setVisitor(codeGen);
    } else {
        ::error(ErrorType::ERR_UNSUPPORTED, "Algorithm not yet implemented: %1%",
                decl->arguments->at(0));
    }

    size = getUintFromExpression(decl->arguments->at(1)->expression, 1);

    unsigned outputHashWidth = getUintFromExpression(decl->arguments->at(2)->expression, 0);
    if (hashEngine != nullptr && outputHashWidth > hashEngine->getOutputWidth()) {
        ::error(ErrorType::ERR_INVALID, "%1%: more bits requested than hash provides (%2%)",
                decl->arguments->at(2)->expression, hashEngine->getOutputWidth());
    }
    if (outputHashWidth > 64) {
        ::error(ErrorType::ERR_UNSUPPORTED, "%1%: supported up to 64 bits",
                decl->arguments->at(2)->expression);
    }
    if (outputHashWidth < 1) {
        ::error(ErrorType::ERR_INVALID,
                "%1%: invalid output width used for checksum "
                "value truncation, must be at least 1 bit",
                decl->arguments->at(2)->expression);
    }
    outputHashMask = Util::printf_format("0x%llx", (1ull << outputHashWidth) - 1);

    // map names
    actionsMapName = instanceName + "_actions";
    groupsMapName = instanceName + "_groups";
    emptyGroupActionMapName = instanceName + "_defaultActionGroup";

    // fields added to a table
    referenceName = instanceName + "_ref";
    isGroupEntryName = instanceName + "_is_group_ref";

    groupsMapSize = 0;

    if (program->options.enableTableCache) {
        tableCacheEnabled = true;
        createCacheTypeNames(true, false);
    }
}

void EBPFActionSelectorPSA::emitInitializer(CodeBuilder *builder) {
    if (emptyGroupAction == nullptr) return;  // no entry to initialize

    auto ev = emptyGroupAction->value->to<IR::ExpressionValue>()->expression;
    cstring value = program->refMap->newName("value");

    if (auto pe = ev->to<IR::PathExpression>()) {
        auto decl = program->refMap->getDeclaration(pe->path, true);
        auto action = decl->to<IR::P4Action>();
        BUG_CHECK(action != nullptr, "%1%: not an action", ev);

        if (!action->getParameters()->empty()) {
            ::error(ErrorType::ERR_UNINITIALIZED, "%1%: missing value for action parameters: %2%",
                    ev, action->getParameters());
            return;
        }

        builder->emitIndent();
        builder->appendFormat("struct %s %s = ", valueTypeName.c_str(), value.c_str());
        builder->blockStart();
        builder->emitIndent();
        if (action->name.originalName == P4::P4CoreLibrary::instance().noAction.name) {
            builder->append(".action = 0,");
        } else {
            builder->appendFormat(".action = %s,", p4ActionToActionIDName(action));
        }
        builder->newline();
        builder->blockEnd(false);
        builder->endOfStatement(true);
    } else if (auto mce = ev->to<IR::MethodCallExpression>()) {
        emitTableValue(builder, mce, value);
    }

    cstring ret = program->refMap->newName("ret");
    builder->emitIndent();
    builder->appendFormat("int %s = ", ret.c_str());
    builder->target->emitTableUpdate(builder, emptyGroupActionMapName, program->zeroKey, value);
    builder->newline();

    emitMapUpdateTraceMsg(builder, emptyGroupActionMapName, ret);
}

void EBPFActionSelectorPSA::emitInstance(CodeBuilder *builder) {
    if (table == nullptr)  // no table(s)
        return;

    // group map (group ref -> {action refs})
    // TODO: group size (inner size) is assumed to be 128. Make more logic for this.
    //  One additional entry is for group size.
    builder->target->emitMapInMapDecl(builder, groupsMapName + "_inner", TableArray, "u32", "u32",
                                      128 + 1, groupsMapName, TableHash, "u32", groupsMapSize);

    // default empty group action (0 -> action)
    builder->target->emitTableDecl(builder, emptyGroupActionMapName, TableArray,
                                   program->arrayIndexType, cstring("struct ") + valueTypeName, 1);

    // action map (ref -> action)
    builder->target->emitTableDecl(builder, actionsMapName, TableHash, "u32",
                                   cstring("struct ") + valueTypeName, size);

    emitCacheInstance(builder);
}

void EBPFActionSelectorPSA::emitReferenceEntry(CodeBuilder *builder) {
    EBPFTableImplementationPSA::emitReferenceEntry(builder);
    builder->emitIndent();
    builder->appendFormat("u32 %s", isGroupEntryName.c_str());
    builder->endOfStatement(true);
}

void EBPFActionSelectorPSA::applyImplementation(CodeBuilder *builder, cstring tableValueName,
                                                cstring actionRunVariable) {
    if (hashEngine == nullptr) return;

    cstring msg = Util::printf_format("ActionSelector: applying %s", instanceName.c_str());
    builder->target->emitTraceMessage(builder, msg.c_str());

    // 1. Declare variables.

    cstring asValueName = program->refMap->newName("as_value");
    cstring effectiveActionRefName = program->refMap->newName("as_action_ref");
    cstring innerGroupName = program->refMap->newName("as_group_map");
    groupStateVarName = program->refMap->newName("as_group_state");
    // these can be hardcoded because they are declared inside of a block
    cstring checksumValName = "as_checksum_val";
    cstring mapEntryName = "as_map_entry";

    emitCacheVariables(builder);

    builder->emitIndent();
    builder->appendFormat("struct %s * %s = NULL", valueTypeName.c_str(), asValueName.c_str());
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("u32 %s = %s->%s", effectiveActionRefName.c_str(), tableValueName.c_str(),
                          referenceName.c_str());
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("u8 %s = 0", groupStateVarName.c_str());
    builder->endOfStatement(true);

    // 2. Check if we have got group reference.

    builder->emitIndent();
    builder->appendFormat("if (%s->%s != 0) ", tableValueName.c_str(), isGroupEntryName.c_str());
    builder->blockStart();
    builder->target->emitTraceMessage(builder, "ActionSelector: group reference %u", 1,
                                      effectiveActionRefName.c_str());

    emitCacheLookup(builder, tableValueName, asValueName);

    builder->emitIndent();
    builder->append("void * ");
    builder->target->emitTableLookup(builder, groupsMapName, effectiveActionRefName,
                                     innerGroupName);
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("if (%s != NULL) ", innerGroupName.c_str());
    builder->blockStart();

    // 3. Find member reference.
    // 3.1. First entry in inner map contains number of valid elements in the map

    builder->emitIndent();
    // innerGroupName is a pointer to a map, so can't use emitTableLookup() - it expects a map
    builder->appendFormat("u32 * %s = bpf_map_lookup_elem(%s, &%s)", mapEntryName.c_str(),
                          innerGroupName.c_str(), program->zeroKey.c_str());
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("if (%s != NULL) ", mapEntryName.c_str());
    builder->blockStart();

    builder->emitIndent();
    builder->appendFormat("if (*%s != 0) ", mapEntryName.c_str());
    builder->blockStart();

    // 3.2. Calculate hash of selector keys and use some least significant bits.

    hashEngine->emitVariables(builder, nullptr);
    hashEngine->emitAddData(builder, unpackSelectors());

    builder->emitIndent();
    builder->appendFormat("u64 %s = ", checksumValName.c_str());
    hashEngine->emitGet(builder);
    builder->appendFormat(" & %sull", outputHashMask.c_str());
    builder->endOfStatement(true);

    // 3.3. Continue finding member reference

    builder->emitIndent();
    builder->appendFormat("%s = 1 + (%s %% (*%s))", effectiveActionRefName.c_str(),
                          checksumValName.c_str(), mapEntryName.c_str());
    builder->endOfStatement(true);
    builder->target->emitTraceMessage(builder, "ActionSelector: selected action %u from group", 1,
                                      effectiveActionRefName.c_str());
    builder->emitIndent();
    // innerGroupName is a pointer to a map, so can't use emitTableLookup() - it expects a map
    builder->appendFormat("%s = bpf_map_lookup_elem(%s, &%s)", mapEntryName.c_str(),
                          innerGroupName.c_str(), effectiveActionRefName.c_str());
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->appendFormat("if (%s != NULL) ", mapEntryName.c_str());
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("%s = *%s", effectiveActionRefName.c_str(), mapEntryName.c_str());
    builder->endOfStatement(true);
    builder->blockEnd(false);
    builder->append(" else ");
    builder->blockStart();
    builder->emitIndent();
    builder->appendLine("/* Not found, probably bug. Skip further execution of the extern. */");
    builder->target->emitTraceMessage(
        builder,
        "ActionSelector: Entry with action reference was not found, dropping packet. Bug?");
    builder->emitIndent();
    builder->appendFormat("return %s", builder->target->abortReturnCode().c_str());
    builder->endOfStatement(true);
    builder->blockEnd(true);

    builder->blockEnd(false);  // elements in group != 0
    builder->append(" else ");
    builder->blockStart();
    builder->target->emitTraceMessage(builder,
                                      "ActionSelector: empty group, going to default action");
    builder->emitIndent();
    builder->appendFormat("%s = 1", groupStateVarName.c_str());
    builder->endOfStatement(true);
    builder->blockEnd(true);

    builder->blockEnd(false);  // found number of elements
    builder->append(" else ");
    builder->blockStart();
    builder->target->emitTraceMessage(
        builder, "ActionSelector: entry with number of elements not found, dropping packet. Bug?");
    builder->emitIndent();
    builder->appendFormat("return %s", builder->target->abortReturnCode().c_str());
    builder->endOfStatement(true);
    builder->blockEnd(true);

    builder->blockEnd(false);  // group found
    builder->append(" else ");
    builder->blockStart();
    builder->target->emitTraceMessage(
        builder, "ActionSelector: group map was not found, dropping packet. Bug?");
    builder->emitIndent();
    builder->appendFormat("return %s", builder->target->abortReturnCode().c_str());
    builder->endOfStatement(true);
    builder->blockEnd(true);

    builder->blockEnd(true);  // is group reference

    if (tableCacheEnabled) {
        builder->blockEnd(true);
    }

    // 4. Use group state and action ref to get an action data.

    builder->emitIndent();
    builder->appendFormat("if (%s == 0) ", groupStateVarName.c_str());
    builder->blockStart();
    builder->target->emitTraceMessage(builder, "ActionSelector: member reference %u", 1,
                                      effectiveActionRefName.c_str());
    builder->emitIndent();
    builder->target->emitTableLookup(builder, actionsMapName, effectiveActionRefName, asValueName);
    builder->endOfStatement(true);
    builder->blockEnd(false);
    builder->appendFormat(" else if (%s == 1) ", groupStateVarName.c_str());
    builder->blockStart();
    builder->target->emitTraceMessage(
        builder, "ActionSelector: empty group, executing default group action");
    builder->emitIndent();
    builder->target->emitTableLookup(builder, emptyGroupActionMapName, program->zeroKey,
                                     asValueName);
    builder->endOfStatement(true);
    builder->blockEnd(true);

    emitCacheUpdate(builder, cacheKeyVar, asValueName);

    // 5. Execute action.

    builder->emitIndent();
    builder->appendFormat("if (%s != NULL) ", asValueName.c_str());
    builder->blockStart();

    emitAction(builder, asValueName, actionRunVariable);

    builder->blockEnd(false);
    builder->append(" else ");

    builder->blockStart();
    builder->target->emitTraceMessage(
        builder, "ActionSelector: member not found, executing implicit NoAction");
    builder->emitIndent();
    builder->appendFormat("%s = 0", program->control->hitVariable.c_str());
    builder->endOfStatement(true);
    if (!actionRunVariable.isNullOrEmpty()) {
        builder->emitIndent();
        builder->appendFormat("%s = 0", actionRunVariable.c_str());  // set to NoAction
        builder->endOfStatement(true);
    }
    builder->blockEnd(true);

    msg = Util::printf_format("ActionSelector: %s applied", instanceName.c_str());
    builder->target->emitTraceMessage(builder, msg.c_str());
}

EBPFHashAlgorithmPSA::ArgumentsList EBPFActionSelectorPSA::unpackSelectors() {
    EBPFHashAlgorithmPSA::ArgumentsList result;
    for (auto s : selectors) {
        result.emplace_back(s->expression);
    }
    return result;
}

EBPFActionSelectorPSA::SelectorsListType EBPFActionSelectorPSA::getSelectorsFromTable(
    const EBPFTablePSA *instance) {
    SelectorsListType ret;

    for (auto k : instance->keyGenerator->keyElements) {
        auto mkdecl = program->refMap->getDeclaration(k->matchType->path, true);
        auto matchType = mkdecl->getNode()->to<IR::Declaration_ID>();

        if (matchType->name.name == "selector") ret.emplace_back(k);
    }

    return ret;
}

void EBPFActionSelectorPSA::registerTable(const EBPFTablePSA *instance) {
    if (table == nullptr) {
        selectors = getSelectorsFromTable(instance);
        emptyGroupAction =
            instance->table->container->properties->getProperty("psa_empty_group_action");
        groupsMapSize = instance->size;
    } else {
        verifyTableSelectorKeySet(instance);
        verifyTableEmptyGroupAction(instance);

        // Documentation says: The number of groups may be at most the size of the table
        //     that is implemented by the selector.
        // So, when we take the max from both tables, this value will not be conformant
        // with specification because for one table there will be available more groups
        // than possible entries. We have to use min.
        groupsMapSize = std::min(groupsMapSize, instance->size);
    }

    EBPFTableImplementationPSA::registerTable(instance);
}

void EBPFActionSelectorPSA::verifyTableSelectorKeySet(const EBPFTablePSA *instance) {
    bool printError = false;
    auto foreignSelectors = getSelectorsFromTable(instance);

    if (selectors.size() == foreignSelectors.size()) {
        for (size_t i = 0; i < selectors.size(); ++i) {
            auto left = foreignSelectors.at(i)->expression->toString();
            auto right = selectors.at(i)->expression->toString();
            if (left != right) printError = true;
        }
    } else {
        printError = true;
    }

    if (printError) {
        ::error(ErrorType::ERR_EXPECTED,
                "%1%: selector type keys list differs from previous %2% "
                "(tables use the same implementation %3%)",
                instance->table->container, table->container, declaration);
    }
}

void EBPFActionSelectorPSA::verifyTableEmptyGroupAction(const EBPFTablePSA *instance) {
    auto iega = instance->table->container->properties->getProperty("psa_empty_group_action");

    if (emptyGroupAction == nullptr && iega == nullptr) return;  // nothing to do here
    if (emptyGroupAction == nullptr && iega != nullptr) {
        ::error(ErrorType::ERR_UNEXPECTED,
                "%1%: property not specified in previous table %2% "
                "(tables use the same implementation %3%)",
                iega, table->container, declaration);
        return;
    }
    if (emptyGroupAction != nullptr && iega == nullptr) {
        ::error(ErrorType::ERR_EXPECTED,
                "%1%: missing property %2%, defined in previous table %3% "
                "(tables use the same implementation %4%)",
                instance->table->container, emptyGroupAction, table->container->toString(),
                declaration);
        return;
    }

    bool same = true;
    cstring additionalNote;

    if (emptyGroupAction->isConstant != iega->isConstant) {
        same = false;
        additionalNote = "; note: const qualifiers also must be the same";
    }

    // compare action and arguments
    auto rev = iega->value->to<IR::ExpressionValue>()->expression;
    auto lev = emptyGroupAction->value->to<IR::ExpressionValue>()->expression;
    auto rpe = rev->to<IR::PathExpression>();
    auto lpe = lev->to<IR::PathExpression>();
    auto rmce = rev->to<IR::MethodCallExpression>();
    auto lmce = lev->to<IR::MethodCallExpression>();

    if (lpe != nullptr && rpe != nullptr) {
        if (lpe->toString() != rpe->toString()) same = false;
    } else if (lmce != nullptr && rmce != nullptr) {
        if (lmce->method->to<IR::PathExpression>()->path->name.originalName !=
            rmce->method->to<IR::PathExpression>()->path->name.originalName) {
            same = false;
        } else if (lmce->arguments->size() == rmce->arguments->size()) {
            for (size_t i = 0; i < lmce->arguments->size(); ++i) {
                if (lmce->arguments->at(i)->expression->toString() !=
                    rmce->arguments->at(i)->expression->toString()) {
                    same = false;
                    additionalNote = "; note: action arguments must be the same for both tables";
                }
            }
        } else {
            same = false;
        }
    } else {
        same = false;
        additionalNote =
            "; note: action name can\'t be mixed with "
            "action call expression (compiler backend limitation)";
    }

    if (!same) {
        ::error(ErrorType::ERR_EXPECTED,
                "%1%: defined property value is different from %2%, defined in "
                "previous table %3% (tables use the same implementation %4%)%5%",
                rev, lev, table->container->toString(), declaration, additionalNote);
    }
}

void EBPFActionSelectorPSA::emitCacheTypes(CodeBuilder *builder) {
    if (!tableCacheEnabled) return;

    CodeGenInspector commentGen(program->refMap, program->typeMap);
    commentGen.setBuilder(builder);

    // construct cache key type - we need group reference and selector keys
    builder->emitIndent();
    builder->appendFormat("struct %s ", cacheKeyTypeName.c_str());
    builder->blockStart();

    builder->emitIndent();
    builder->appendFormat("u32 group_ref");
    builder->endOfStatement(true);

    unsigned int fieldNumber = 0;
    for (auto s : selectors) {
        auto type = program->typeMap->getType(s->expression);
        auto ebpfType = EBPFTypeFactory::instance->create(type);
        cstring fieldName = Util::printf_format("field%u", fieldNumber++);

        builder->emitIndent();
        ebpfType->declare(builder, fieldName, false);
        builder->endOfStatement(false);
        builder->append(" /* ");
        s->expression->apply(commentGen);
        builder->append(" */");
        builder->newline();
    }

    builder->blockEnd(false);
    builder->append(" __attribute__((aligned(4)))");
    builder->endOfStatement(true);

    // value can be used from Action Selector because there is no need to cache hit variable
}

void EBPFActionSelectorPSA::emitCacheVariables(CodeBuilder *builder) {
    if (!tableCacheEnabled) return;

    cacheKeyVar = program->refMap->newName("key_cache");
    cacheDoUpdateVar = program->refMap->newName("do_update_cache");

    builder->emitIndent();
    builder->appendFormat("struct %s %s = {0}", cacheKeyTypeName.c_str(), cacheKeyVar.c_str());
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->appendFormat("u8 %s = 0", cacheDoUpdateVar.c_str());
    builder->endOfStatement(true);
}

void EBPFActionSelectorPSA::emitCacheLookup(CodeBuilder *builder, cstring key, cstring value) {
    if (!tableCacheEnabled) return;

    builder->emitIndent();
    builder->appendFormat("%s.group_ref = %s->%s", cacheKeyVar.c_str(), key.c_str(),
                          referenceName.c_str());
    builder->endOfStatement(true);

    unsigned int fieldNumber = 0;
    for (auto s : selectors) {
        auto type = program->typeMap->getType(s->expression);
        auto ebpfType = EBPFTypeFactory::instance->create(type);
        cstring fieldName = Util::printf_format("field%u", fieldNumber++);

        bool memcpy = false;
        auto scalar = ebpfType->to<EBPFScalarType>();
        unsigned width = 0;
        if (scalar != nullptr) {
            width = scalar->implementationWidthInBits();
            memcpy = !EBPFScalarType::generatesScalar(width);
        }

        builder->emitIndent();
        if (memcpy) {
            builder->appendFormat("__builtin_memcpy(&%s.%s, &", cacheKeyVar.c_str(),
                                  fieldName.c_str());
            codeGen->visit(s->expression);
            builder->appendFormat(", %d)", scalar->bytesRequired());
        } else {
            builder->appendFormat("%s.%s = ", cacheKeyVar.c_str(), fieldName.c_str());
            codeGen->visit(s->expression);
        }
        builder->endOfStatement(true);
    }

    builder->target->emitTraceMessage(builder, "ActionSelector: trying cache...");

    builder->emitIndent();
    builder->target->emitTableLookup(builder, cacheTableName, cacheKeyVar, value);
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->appendFormat("if (%s != NULL) ", value.c_str());
    builder->blockStart();

    builder->target->emitTraceMessage(builder,
                                      "ActionSelector: cache hit, skipping later lookup(s)");

    builder->emitIndent();
    builder->appendFormat("%s = 2", groupStateVarName.c_str());
    builder->endOfStatement(true);

    builder->blockEnd(false);
    builder->append(" else ");
    builder->blockStart();

    builder->target->emitTraceMessage(builder, "ActionSelector: cache miss, nevermind");

    builder->emitIndent();
    builder->appendFormat("%s = 1", cacheDoUpdateVar.c_str());
    builder->endOfStatement(true);

    // do normal lookup at this indent level and then end block
}

void EBPFActionSelectorPSA::emitCacheUpdate(CodeBuilder *builder, cstring key, cstring value) {
    if (!tableCacheEnabled) return;

    builder->emitIndent();
    builder->appendFormat("if (%s != NULL && %s != 0) ", value.c_str(), cacheDoUpdateVar.c_str());
    builder->blockStart();

    builder->emitIndent();
    builder->appendFormat("BPF_MAP_UPDATE_ELEM(%s, &%s, %s, BPF_ANY);", cacheTableName.c_str(),
                          key.c_str(), value.c_str());
    builder->newline();

    builder->target->emitTraceMessage(builder, "ActionSelector: cache updated");

    builder->blockEnd(true);
}

}  // namespace EBPF
