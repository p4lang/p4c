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

namespace EBPF {

EBPFTableImplementationPSA::EBPFTableImplementationPSA(const EBPFProgram* program,
        CodeGenInspector* codeGen, const IR::Declaration_Instance* decl) :
        EBPFTablePSA(program, codeGen, externalName(decl)), declaration(decl) {
    referenceName = instanceName + "_key";
}

void EBPFTableImplementationPSA::emitTypes(CodeBuilder* builder) {
    if (table == nullptr)
        return;
    // key is u32, so do not emit type for it
    emitValueType(builder);
}

void EBPFTableImplementationPSA::emitInitializer(CodeBuilder *builder) {
    (void) builder;
}

void EBPFTableImplementationPSA::emitReferenceEntry(CodeBuilder *builder) {
    builder->emitIndent();
    builder->appendFormat("u32 %s", referenceName.c_str());
    builder->endOfStatement(true);
}

void EBPFTableImplementationPSA::registerTable(const EBPFTablePSA * instance) {
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

void EBPFTableImplementationPSA::verifyTableActionList(const EBPFTablePSA * instance) {
    bool printError = false;
    if (actionList == nullptr)
        return;

    auto getActionName = [this](const IR::ActionList * al, size_t id)->cstring {
        auto pe = this->getActionNameExpression(al->actionList.at(id)->expression);
        CHECK_NULL(pe);
        return pe->path->name.originalName;
    };

    if (instance->actionList->size() == actionList->size()) {
        for (size_t i = 0; i < actionList->size(); ++i) {
            auto left = getActionName(instance->actionList, i);
            auto right = getActionName(actionList, i);
            if (left != right)
                printError = true;
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

void EBPFTableImplementationPSA::verifyTableNoDefaultAction(const EBPFTablePSA * instance) {
    auto defaultAction = instance->table->container->getDefaultAction();
    BUG_CHECK(defaultAction->is<IR::MethodCallExpression>(),
              "%1%: expected an action call", defaultAction);

    auto mi = P4::MethodInstance::resolve(defaultAction->to<IR::MethodCallExpression>(),
                                          program->refMap, program->typeMap);
    auto ac = mi->to<P4::ActionCall>();
    BUG_CHECK(ac != nullptr, "%1%: expected an action call", mi);

    if (ac->action->name.originalName != P4::P4CoreLibrary::instance.noAction.name) {
        ::error(ErrorType::ERR_UNSUPPORTED,
                "%1%: Default action cannot be defined for table %2% with implementation %3%",
                defaultAction, instance->table->container->name, declaration);
    }
}

void EBPFTableImplementationPSA::verifyTableNoDirectObjects(const EBPFTablePSA * instance) {
    if (!instance->counters.empty() || !instance->meters.empty()) {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                "%1%: DirectCounter and DirectMeter externs are not supported "
                "with table implementation %2%",
                instance->table->container->name, declaration->type->toString());
    }
}

void EBPFTableImplementationPSA::verifyTableNoEntries(const EBPFTablePSA * instance) {
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

unsigned EBPFTableImplementationPSA::getUintFromExpression(const IR::Expression * expr,
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

EBPFActionProfilePSA::EBPFActionProfilePSA(const EBPFProgram* program, CodeGenInspector* codeGen,
                                           const IR::Declaration_Instance* decl) :
        EBPFTableImplementationPSA(program, codeGen, decl) {
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

void EBPFActionProfilePSA::applyImplementation(CodeBuilder* builder, cstring tableValueName,
                                               cstring actionRunVariable) {
    cstring msg = Util::printf_format("ActionProfile: applying %s", instanceName.c_str());
    builder->target->emitTraceMessage(builder, msg.c_str());

    cstring apValueName = program->refMap->newName("ap_value");
    cstring apKeyName = Util::printf_format("%s->%s",
        tableValueName.c_str(), referenceName.c_str());

    builder->target->emitTraceMessage(builder, "ActionProfile: entry id %u",
                                      1, apKeyName.c_str());

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
    builder->target->emitTraceMessage(builder,
        "ActionProfile: entry not found, executing implicit NoAction");
    builder->emitIndent();
    builder->appendFormat("%s = 0", program->control->hitVariable.c_str());
    builder->endOfStatement(true);
    builder->blockEnd(true);

    msg = Util::printf_format("ActionProfile: %s applied", instanceName.c_str());
    builder->target->emitTraceMessage(builder, msg.c_str());
}

}  // namespace EBPF
