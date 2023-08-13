/*
Copyright 2022 Intel Corp.

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

#include "dpdkMetadata.h"

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/dpdk/dpdkUtils.h"
#include "ir/id.h"
#include "ir/vector.h"
#include "lib/error.h"
#include "lib/error_catalog.h"

namespace DPDK {

// make sure new decls and fields name are unique
void DirectionToRegRead::uniqueNames(IR::DpdkAsmProgram *p) {
    // "direction" name is used in dpdk for initialzing direction port mask
    // make sure no such decls exist with that name
    registerInstanceName = "direction";
    for (auto decl : p->externDeclarations) {
        usedNames.insert(decl->name);
    }

    if (usedNames.count(registerInstanceName))
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "decl name %s is reserved for dpdk pna",
                registerInstanceName);
}

const IR::Node *DirectionToRegRead::preorder(IR::DpdkAsmProgram *p) {
    bool is_direction_used = false;
    auto IsDirUsed = new IsDirectionMetadataUsed(is_direction_used);
    IsDirUsed->setCalledBy(this);
    p->apply(*IsDirUsed);
    uniqueNames(p);
    p->externDeclarations.push_back(addRegDeclInstance(registerInstanceName));
    if (is_direction_used) {
        IR::IndexedVector<IR::DpdkAsmStatement> stmts;
        stmts.push_back(new IR::DpdkListStatement(
            addRegReadStmtForDirection(p->statements[0]->to<IR::DpdkListStatement>()->statements)));
        p->statements = stmts;
    }
    return p;
}

// create and add register declaration instance to program
IR::DpdkExternDeclaration *DirectionToRegRead::addRegDeclInstance(cstring instanceName) {
    auto typepath = new IR::Path("Register");
    auto type = new IR::Type_Name(typepath);
    auto typeargs = new IR::Vector<IR::Type>({IR::Type::Bits::get(64), IR::Type::Bits::get(32)});
    auto spectype = new IR::Type_Specialized(type, typeargs);
    auto args = new IR::Vector<IR::Argument>();
    args->push_back(new IR::Argument(new IR::Constant(IR::Type::Bits::get(32), 256)));
    auto annot = IR::Annotations::empty;
    annot->addAnnotationIfNew(IR::Annotation::nameAnnotation, new IR::StringLiteral(instanceName));
    auto decl = new IR::DpdkExternDeclaration(instanceName, annot, spectype, args, nullptr);
    return decl;
}

// replace all direction uses with m.pna_main_input_metadata_direction
// it's initilization will be done like istd.direction = direction.read(istd.input_port)
// at start of the pipeline
const IR::Node *DirectionToRegRead::preorder(IR::Member *m) {
    if (isDirection(m))
        return new IR::Member(new IR::PathExpression(IR::ID("m")),
                              IR::ID(dirToDirMapping[m->member.name]));
    else
        return m;
}

IR::IndexedVector<IR::DpdkAsmStatement> DirectionToRegRead::addRegReadStmtForDirection(
    IR::IndexedVector<IR::DpdkAsmStatement> stmts) {
    IR::IndexedVector<IR::DpdkAsmStatement> newStmts;
    newStmts.insert(newStmts.begin(), *stmts.begin());
    // insert direction read after rx statement, we initialize direction so early in
    // pipeline because we sure that it's not going to change after this and this direction
    // is available to all the blocks.
    auto dirMeta = new IR::Member(new IR::PathExpression(IR::ID("m")),
                                  IR::ID("pna_main_input_metadata_direction"));
    auto inputPort = new IR::Member(new IR::PathExpression(IR::ID("m")),
                                    IR::ID("pna_main_input_metadata_input_port"));
    auto reads = new IR::DpdkRegisterReadStatement(dirMeta, registerInstanceName, inputPort);
    newStmts.insert(newStmts.begin() + 1, reads);
    newStmts.insert(newStmts.begin() + 2, stmts.begin() + 1, stmts.end());
    return newStmts;
}

// check member expression using metadata pass field
// "recircid" instruction takes the pass metadata type as argument to fetch the pass_id.
bool PrependPassRecircId::isPass(const IR::Member *m) {
    if (m == nullptr) return false;
    return m->member.name == "pna_main_input_metadata_pass" ||
           m->member.name == "pna_pre_input_metadata_pass" ||
           m->member.name == "pna_main_parser_input_metadata_pass";
}

const IR::Node *PrependPassRecircId::postorder(IR::DpdkListStatement *l) {
    l->statements = prependPassWithRecircid(l->statements);
    newStmts.clear();
    return l;
}

const IR::Node *PrependPassRecircId::postorder(IR::DpdkAction *a) {
    a->statements = prependPassWithRecircid(a->statements);
    newStmts.clear();
    return a;
}

IR::IndexedVector<IR::DpdkAsmStatement> PrependPassRecircId::prependPassWithRecircid(
    IR::IndexedVector<IR::DpdkAsmStatement> stmts) {
    for (auto s : stmts) {
        if (auto jc = s->to<IR::DpdkJmpCondStatement>()) {
            if (isPass(jc->src1->to<IR::Member>()))
                newStmts.push_back(new IR::DpdkRecircidStatement(jc->src1->to<IR::Member>()));
            else if (isPass(jc->src2->to<IR::Member>()))
                newStmts.push_back(new IR::DpdkRecircidStatement(jc->src2->to<IR::Member>()));
        } else if (auto u = s->to<IR::DpdkUnaryStatement>()) {
            if (isPass(u->src->to<IR::Member>())) {
                newStmts.push_back(new IR::DpdkRecircidStatement(u->src->to<IR::Member>()));
            }
        } else if (auto u = s->to<IR::DpdkCastStatement>()) {
            if (isPass(u->src->to<IR::Member>())) {
                newStmts.push_back(new IR::DpdkRecircidStatement(u->src->to<IR::Member>()));
            }
        }
        newStmts.push_back(s);
    }
    return newStmts;
}

}  // namespace DPDK
