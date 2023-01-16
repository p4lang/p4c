/*
Copyright 2021 Intel Corp.

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

#ifndef BACKENDS_DPDK_DPDKCHECKEXTERNINVOCATION_H_
#define BACKENDS_DPDK_DPDKCHECKEXTERNINVOCATION_H_

#include "frontends/p4/methodInstance.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "midend/checkExternInvocationCommon.h"

namespace P4 {
class ReferenceMap;
class TypeMap;
}  // namespace P4

namespace DPDK {

/**
 * @brief Class for checking constraints for invocations of PNA architecture extern
 *        methods and functions.
 */
class CheckPNAExternInvocation : public P4::CheckExternInvocationCommon {
    DpdkProgramStructure *structure;

    enum block_t {
        MAIN_PARSER = 0,
        PRE_CONTROL,
        MAIN_CONTROL,
        MAIN_DEPARSER,
        BLOCK_COUNT,
    };

    void initPipeConstraints() override {
        bitvec validInMainControl;
        validInMainControl.setbit(MAIN_CONTROL);
        setPipeConstraints("send_to_port", validInMainControl);
        setPipeConstraints("mirror_packet", validInMainControl);
        // Add new constraints here
    }

    cstring getBlockName(int bit) override {
        static const char *lookup[] = {"main parser", "pre control", "main control",
                                       "main deparser"};
        BUG_CHECK(sizeof(lookup) / sizeof(lookup[0]) == BLOCK_COUNT, "Bad lookup table");
        return lookup[bit % BLOCK_COUNT];
    }

    const IR::P4Parser *getParser(const cstring parserName) {
        if (auto p = findContext<IR::P4Parser>()) {
            if (structure->parsers.count(parserName) != 0 &&
                structure->parsers.at(parserName)->name == p->name) {
                return p;
            }
        }
        return nullptr;
    }

    const IR::P4Control *getControl(const cstring controlName) {
        if (auto c = findContext<IR::P4Control>()) {
            if (structure->pipelines.count(controlName) != 0 &&
                structure->pipelines.at(controlName)->name == c->name) {
                return c;
            }
        }
        return nullptr;
    }

    const IR::P4Control *getDeparser(const cstring deparserName) {
        if (auto d = findContext<IR::P4Control>()) {
            if (structure->deparsers.count(deparserName) != 0 &&
                structure->deparsers.at(deparserName)->name == d->name) {
                return d;
            }
        }
        return nullptr;
    }

    void checkBlock(const IR::MethodCallExpression *expr, const cstring externType,
                    const cstring externName) {
        bitvec pos;

        LOG4("externType: " << externType << ", externName: " << externName);

        if (pipeConstraints.count(externType)) {
            if (auto block = getParser("MainParserT")) {
                LOG4("MainParser: " << (void *)block << " " << dbp(block));
                pos.setbit(MAIN_PARSER);
                checkPipeConstraints(externType, pos, expr, externName, block->name);
            } else if (auto block = getControl("PreControlT")) {
                LOG4("PreControl: " << (void *)block << " " << dbp(block));
                pos.setbit(PRE_CONTROL);
                checkPipeConstraints(externType, pos, expr, externName, block->name);
            } else if (auto block = getControl("MainControlT")) {
                LOG4("MainControl: " << (void *)block << " " << dbp(block));
                pos.setbit(MAIN_CONTROL);
                checkPipeConstraints(externType, pos, expr, externName, block->name);
            } else if (auto block = getDeparser("MainDeparserT")) {
                LOG4("MainDeparser: " << (void *)block << " " << dbp(block));
                pos.setbit(MAIN_DEPARSER);
                checkPipeConstraints(externType, pos, expr, externName, block->name);
            } else {
                LOG4("Other");
            }
        } else {
            LOG1("Pipe constraints not defined for: " << externType);
        }
    }

    void checkExtern(const P4::ExternMethod *extMethod,
                     const IR::MethodCallExpression *expr) override {
        LOG3("ExternMethod: " << extMethod << ", MethodCallExpression: " << expr);
        checkBlock(expr, extMethod->originalExternType->name, extMethod->object->getName().name);
    }

    void checkExtern(const P4::ExternFunction *extFunction,
                     const IR::MethodCallExpression *expr) override {
        LOG3("ExternFunction: " << extFunction << ", MethodCallExpression: " << expr);
        checkBlock(expr, expr->method->toString(), "");
    }

 public:
    CheckPNAExternInvocation(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                             DpdkProgramStructure *structure)
        : P4::CheckExternInvocationCommon(refMap, typeMap), structure(structure) {
        initPipeConstraints();
    }
};

/**
 * @brief Class which chooses the correct class for checking the constraints for invocations
 *        of extern methods and functions depending on the architecture.
 */
class CheckExternInvocation : public Inspector {
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    DpdkProgramStructure *structure;

 public:
    CheckExternInvocation(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                          DpdkProgramStructure *structure)
        : refMap(refMap), typeMap(typeMap), structure(structure) {}

    bool preorder(const IR::P4Program *program) {
        if (structure->isPNA()) {
            LOG1("Checking extern invocations for PNA architecture.");
            auto checker = new CheckPNAExternInvocation(refMap, typeMap, structure);
            program->apply(*checker);
        } else if (structure->isPSA()) {
            LOG1("Checking extern invocations for PSA architecture.");
            // Add class checking PSA constraints here.
        } else {
            LOG1("Unknown architecture: " << structure->p4arch << ", not checking constraints.");
        }
        return false;
    }
};

}  // namespace DPDK

#endif /* BACKENDS_DPDK_DPDKCHECKEXTERNINVOCATION_H_ */
