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

#include "codeGen.h"
#include "ebpfObject.h"
#include "ebpfType.h"
#include "frontends/p4/enumInstance.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/common/resolveReferences/referenceMap.h"

namespace EBPF {

void CodeGenInspector::substitute(const IR::Parameter* p, const IR::Parameter* with)
{ substitution.emplace(p, with); }

bool CodeGenInspector::preorder(const IR::Constant* expression) {
    builder->append(expression->toString());
    return true;
}

bool CodeGenInspector::preorder(const IR::StringLiteral* expression) {
    builder->appendFormat("\"%s\"", expression->toString());
    return true;
}

bool CodeGenInspector::preorder(const IR::Declaration_Variable* decl) {
    auto type = EBPFTypeFactory::instance->create(decl->type);
    type->declare(builder, decl->name.name, false);
    if (decl->initializer != nullptr) {
        builder->append(" = ");
        visit(decl->initializer);
    }
    builder->endOfStatement();
    return false;
}

bool CodeGenInspector::preorder(const IR::Operation_Binary* b) {
    widthCheck(b);
    builder->append("(");
    visit(b->left);
    builder->spc();
    builder->append(b->getStringOp());
    builder->spc();
    visit(b->right);
    builder->append(")");
    return false;
}

bool CodeGenInspector::comparison(const IR::Operation_Relation* b) {
    auto type = typeMap->getType(b->left);
    auto et = EBPFTypeFactory::instance->create(type);

    bool scalar = (et->is<EBPFScalarType>() &&
                   EBPFScalarType::generatesScalar(et->to<EBPFScalarType>()->widthInBits()))
                  || et->is<EBPFBoolType>();
    if (scalar) {
        builder->append("(");
        visit(b->left);
        builder->spc();
        builder->append(b->getStringOp());
        builder->spc();
        visit(b->right);
        builder->append(")");
    } else {
        if (!et->is<IHasWidth>())
            BUG("%1%: Comparisons for type %2% not yet implemented", type);
        unsigned width = et->to<IHasWidth>()->implementationWidthInBits();
        builder->append("memcmp(&");
        visit(b->left);
        builder->append(", &");
        visit(b->right);
        builder->appendFormat(", %d)", width / 8);
    }
    return false;
}

bool CodeGenInspector::preorder(const IR::Mux* b) {
    widthCheck(b);
    builder->append("(");
    visit(b->e0);
    builder->append(" ? ");
    visit(b->e1);
    builder->append(" : ");
    visit(b->e2);
    builder->append(")");
    return false;
}

bool CodeGenInspector::preorder(const IR::Operation_Unary* u) {
    widthCheck(u);
    builder->append("(");
    builder->append(u->getStringOp());
    visit(u->expr);
    builder->append(")");
    return false;
}

bool CodeGenInspector::preorder(const IR::ArrayIndex* a) {
    builder->append("(");
    visit(a->left);
    builder->append("[");
    visit(a->right);
    builder->append("]");
    builder->append(")");
    return false;
}

bool CodeGenInspector::preorder(const IR::Cast* c) {
    widthCheck(c);
    builder->append("(");
    builder->append("(");
    auto et = EBPFTypeFactory::instance->create(c->destType);
    et->emit(builder);
    builder->append(")");
    visit(c->expr);
    builder->append(")");
    return false;
}

bool CodeGenInspector::preorder(const IR::Member* expression) {
    auto ei = P4::EnumInstance::resolve(expression, typeMap);
    if (ei == nullptr) {
        visit(expression->expr);
        builder->append(".");
    }
    builder->append(expression->member);
    return false;
}

bool CodeGenInspector::preorder(const IR::PathExpression* expression) {
    visit(expression->path);
    return false;
}

bool CodeGenInspector::preorder(const IR::Path* p) {
    if (p->absolute)
        ::error("%1%: Unexpected absolute path", p);
    builder->append(p->name);
    return false;
}

bool CodeGenInspector::preorder(const IR::BoolLiteral* b) {
    builder->append(b->toString());
    return false;
}

bool CodeGenInspector::preorder(const IR::ListExpression* expression) {
    bool first = true;
    for (auto e : expression->components) {
        if (!first)
            builder->append(", ");
        first = false;
        visit(e);
    }
    return false;
}

bool CodeGenInspector::preorder(const IR::MethodCallExpression* expression) {
    auto mi = P4::MethodInstance::resolve(expression, refMap, typeMap);
    auto bim = mi->to<P4::BuiltInMethod>();
    if (bim != nullptr) {
        builder->emitIndent();
        if (bim->name == IR::Type_Header::isValid) {
            visit(bim->appliedTo);
            builder->append(".ebpf_valid");
            return false;
        } else if (bim->name == IR::Type_Header::setValid) {
            visit(bim->appliedTo);
            builder->append(".ebpf_valid = true");
            return false;
        } else if (bim->name == IR::Type_Header::setInvalid) {
            visit(bim->appliedTo);
            builder->append(".ebpf_valid = false");
            return false;
        }
    }

    visit(expression->method);
    builder->append("(");
    bool first = true;
    for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
        if (!first)
            builder->append(", ");
        first = false;

        if (p->direction == IR::Direction::Out ||
            p->direction == IR::Direction::InOut)
            builder->append("&");
        auto arg = mi->substitution.lookup(p);
        visit(arg);
    }
    builder->append(")");
    return false;
}

/////////////////////////////////////////

bool CodeGenInspector::preorder(const IR::Type_Typedef* type) {
    auto et = EBPFTypeFactory::instance->create(type->type);
    builder->append("typedef ");
    et->emit(builder);
    builder->spc();
    builder->append(type->name);
    builder->endOfStatement();
    return false;
}

bool CodeGenInspector::preorder(const IR::Type_Enum* type) {
    builder->append("enum ");
    builder->append(type->name);
    builder->spc();
    builder->blockStart();
    for (auto e : *type->getDeclarations()) {
        builder->emitIndent();
        builder->append(e->getName().name);
        builder->appendLine(",");
    }
    builder->blockEnd(true);
    return false;
}

bool CodeGenInspector::preorder(const IR::AssignmentStatement* a) {
    auto ltype = typeMap->getType(a->left);
    auto ebpfType = EBPFTypeFactory::instance->create(ltype);
    bool memcpy = false;
    EBPFScalarType* scalar = nullptr;
    unsigned width = 0;
    if (ebpfType->is<EBPFScalarType>()) {
        scalar = ebpfType->to<EBPFScalarType>();
        width = scalar->implementationWidthInBits();
        memcpy = !EBPFScalarType::generatesScalar(width);
    }

    if (memcpy) {
        builder->append("memcpy(&");
        visit(a->left);
        builder->append(", &");
        visit(a->right);
        builder->appendFormat(", %d)", scalar->bytesRequired());
    } else {
        visit(a->left);
        builder->append(" = ");
        visit(a->right);
    }
    builder->endOfStatement();
    return false;
}

bool CodeGenInspector::preorder(const IR::BlockStatement* s) {
    builder->blockStart();
    bool first = true;
    for (auto a : s->components) {
        if (!first) {
            builder->newline();
            builder->emitIndent();
        }
        first = false;
        visit(a);
    }
    if (!s->components.empty())
        builder->newline();
    builder->blockEnd(false);
    return false;
}

// This is correct only after inlining
bool CodeGenInspector::preorder(const IR::ExitStatement*) {
    builder->append("return");
    builder->endOfStatement();
    return false;
}

bool CodeGenInspector::preorder(const IR::ReturnStatement*) {
    builder->append("return");
    builder->endOfStatement();
    return false;
}

bool CodeGenInspector::preorder(const IR::EmptyStatement*) {
    builder->endOfStatement();
    return false;
}

bool CodeGenInspector::preorder(const IR::IfStatement* s) {
    builder->append("if (");
    visit(s->condition);
    builder->append(") ");
    if (!s->ifTrue->is<IR::BlockStatement>()) {
        builder->increaseIndent();
        builder->newline();
        builder->emitIndent();
    }
    visit(s->ifTrue);
    if (!s->ifTrue->is<IR::BlockStatement>())
        builder->decreaseIndent();
    if (s->ifFalse != nullptr) {
        builder->newline();
        builder->emitIndent();
        builder->append("else ");
        if (!s->ifFalse->is<IR::BlockStatement>()) {
            builder->increaseIndent();
            builder->newline();
            builder->emitIndent();
        }
        visit(s->ifFalse);
        if (!s->ifFalse->is<IR::BlockStatement>())
            builder->decreaseIndent();
    }
    return false;
}

bool CodeGenInspector::preorder(const IR::MethodCallStatement* s) {
    visit(s->methodCall);
    builder->endOfStatement();
    return false;
}

void CodeGenInspector::widthCheck(const IR::Node* node) const {
    // This is a temporary solution.
    // Rather than generate incorrect results, we reject programs that
    // do not perform arithmetic on machine-supported widths.
    // In the future we will support a wider range of widths
    CHECK_NULL(node);
    auto type = typeMap->getType(node, true);
    auto tb = type->to<IR::Type_Bits>();
    if (tb == nullptr) return;
    if (tb->size % 8 == 0 && EBPFScalarType::generatesScalar(tb->size))
        return;

    if (tb->size <= 64)
        // This is a bug which we can probably fix
        BUG("%1%: Computations on %2% bits not yet supported", node, tb->size);
    // We could argue that this may not be supported ever
    ::error("%1%: Computations on %2% bits not supported", node, tb->size);
}

}  // namespace EBPF
