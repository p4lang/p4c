/*
Copyright 2021 VMware, Inc.

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

#include "frontends/p4/checkCoreMethods.h"

#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/methodInstance.h"

namespace P4 {

void DoCheckCoreMethods::checkEmitType(const IR::Expression *emit, const IR::Type *type) const {
    if (type->is<IR::Type_Header>() || type->is<IR::Type_Stack>() ||
        type->is<IR::Type_HeaderUnion>())
        return;

    if (type->is<IR::Type_Struct>()) {
        for (auto f : type->to<IR::Type_Struct>()->fields) {
            auto ftype = typeMap->getType(f);
            if (ftype == nullptr) continue;
            checkEmitType(emit, ftype);
        }
        return;
    }

    if (auto list = type->to<IR::Type_BaseList>()) {
        for (auto f : list->components) {
            auto ftype = typeMap->getTypeType(f, true);
            if (ftype == nullptr) continue;
            checkEmitType(emit, ftype);
        }
        return;
    }

    typeError("%1%: argument must be a header, stack or union, or a struct or tuple of such types",
              emit);
}

void DoCheckCoreMethods::checkCorelibMethods(const ExternMethod *em) const {
    P4CoreLibrary &corelib = P4CoreLibrary::instance();
    auto et = em->actualExternType;
    auto mce = em->expr;
    unsigned argCount = mce->arguments->size();

    if (et->name == corelib.packetIn.name) {
        if (em->method->name == corelib.packetIn.extract.name) {
            if (argCount == 0) {
                // core.p4 is corrupted.
                typeError("%1%: Expected exactly 1 argument for %2% method", mce,
                          corelib.packetIn.extract.name);
                return;
            }

            auto arg0 = mce->arguments->at(0);
            auto argType = typeMap->getType(arg0, true);
            if (!argType->is<IR::Type_Header>() && !argType->is<IR::Type_Dontcare>()) {
                typeError("%1%: argument must be a header", mce->arguments->at(0));
                return;
            }

            if (argCount == 1) {
                if (hasVarbitsOrUnions(typeMap, argType))
                    // This will never have unions, but may have varbits
                    typeError("%1%: argument cannot contain varbit fields", arg0);
            } else if (argCount == 2) {
                if (!hasVarbitsOrUnions(typeMap, argType))
                    typeError("%1%: argument should contain a varbit field", arg0);
            } else {
                // core.p4 is corrupted.
                typeError("%1%: Expected 1 or 2 arguments for '%2%' method", mce,
                          corelib.packetIn.extract.name);
            }
        } else if (em->method->name == corelib.packetIn.lookahead.name) {
            // this is a call to packet_in.lookahead.
            if (mce->typeArguments->size() != 1) {
                typeError("Expected 1 type parameter for %1%", em->method);
                return;
            }
            auto targ = em->expr->typeArguments->at(0);
            auto typearg = typeMap->getTypeType(targ, true);
            if (hasVarbitsOrUnions(typeMap, typearg)) {
                typeError("%1%: type argument must be a fixed-width type", targ);
                return;
            }
        }
    } else if (et->name == corelib.packetOut.name) {
        if (em->method->name == corelib.packetOut.emit.name) {
            if (argCount == 1) {
                auto arg = mce->arguments->at(0);
                auto argType = typeMap->getType(arg, true);
                checkEmitType(mce, argType);
            } else {
                // core.p4 is corrupted.
                typeError("%1%: Expected 1 argument for '%2%' method", mce,
                          corelib.packetOut.emit.name);
                return;
            }
        }
    }
}

void DoCheckCoreMethods::postorder(const IR::MethodCallExpression *expression) {
    auto mi = MethodInstance::resolve(expression, refMap, typeMap, nullptr, true);
    if (mi->is<ExternMethod>()) checkCorelibMethods(mi->to<ExternMethod>());

    // Check that verify is only invoked from parsers.
    if (auto ef = mi->to<ExternFunction>()) {
        if (ef->method->name == IR::ParserState::verify && !findContext<IR::P4Parser>()) {
            typeError("%1%: may only be invoked in parsers", ef->expr);
        }
    }
}

}  // namespace P4
