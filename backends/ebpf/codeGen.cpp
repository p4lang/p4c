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
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/enumInstance.h"
#include "frontends/p4/methodInstance.h"

namespace P4::EBPF {

void CodeGenInspector::substitute(const IR::Parameter *p, const IR::Parameter *with) {
    substitution.emplace(p, with);
}

bool CodeGenInspector::preorder(const IR::Constant *expression) {
    unsigned width = EBPFInitializerUtils::ebpfTypeWidth(typeMap, expression);

    if (EBPFScalarType::generatesScalar(width)) {
        builder->append(Util::toString(expression->value, 0, false, expression->base));
        return true;
    }

    cstring str = EBPFInitializerUtils::genHexStr(expression->value, width, expression);
    builder->append("{ ");
    for (size_t i = 0; i < str.size() / 2; ++i)
        builder->appendFormat("0x%v, ", str.substr(2 * i, 2));
    builder->append("}");

    return false;
}

bool CodeGenInspector::preorder(const IR::StringLiteral *expression) {
    builder->appendFormat("\"%v\"", expression->toString());
    return true;
}

bool CodeGenInspector::preorder(const IR::Declaration_Variable *decl) {
    auto type = EBPFTypeFactory::instance->create(decl->type);
    type->declare(builder, decl->name.name, false);
    if (decl->initializer != nullptr) {
        builder->append(" = ");
        visit(decl->initializer);
    }
    builder->endOfStatement();
    return false;
}

// We count on the optimizer to get rid of 64-bit arithmetic when it's unnecessary
static cstring getMask(P4::TypeMap *typeMap, const IR::Node *node) {
    auto type = typeMap->getType(node, true);
    cstring mask = cstring::empty;
    if (auto tb = type->to<IR::Type_Bits>()) {
        if (tb->size != 8 && tb->size != 16 && tb->size != 32 && tb->size != 64)
            mask = " & ((1ULL << " + Util::toString(tb->size) + ") - 1)";
    }
    return mask;
}

bool CodeGenInspector::preorder(const IR::Operation_Binary *b) {
    if (!b->is<IR::BOr>() && !b->is<IR::BAnd>() && !b->is<IR::BXor>() && !b->is<IR::Equ>() &&
        !b->is<IR::Neq>())
        widthCheck(b);
    cstring mask = getMask(typeMap, b);
    int prec = expressionPrecedence;
    bool useParens = getParent<IR::IfStatement>() == nullptr || mask != "";
    if (mask != "") builder->append("(");
    if (useParens) builder->append("(");
    if (builder->target->name == "P4TC") {
        emitTCBinaryOperation(b, true);
    } else {
        visit(b->left);
        builder->spc();
        builder->append(b->getStringOp());
        builder->spc();
        expressionPrecedence = b->getPrecedence() + 1;
        visit(b->right);
    }
    if (useParens) builder->append(")");
    builder->append(mask);
    expressionPrecedence = prec;
    if (mask != "") builder->append(")");
    return false;
}

bool CodeGenInspector::comparison(const IR::Operation_Relation *b) {
    auto type = typeMap->getType(b->left);
    auto et = EBPFTypeFactory::instance->create(type);

    bool scalar = (et->is<EBPFScalarType>() &&
                   EBPFScalarType::generatesScalar(et->to<EBPFScalarType>()->widthInBits())) ||
                  et->is<EBPFBoolType>();
    if (scalar) {
        int prec = expressionPrecedence;
        bool useParens = prec > b->getPrecedence();
        if (useParens) builder->append("(");
        if (builder->target->name == "P4TC") {
            emitTCBinaryOperation(b, scalar);
        } else {
            visit(b->left);
            builder->spc();
            builder->append(b->getStringOp());
            builder->spc();
            visit(b->right);
        }
        if (useParens) builder->append(")");
    } else {
        if (!et->is<IHasWidth>())
            BUG("%1%: Comparisons for type %2% not yet implemented", b->left, type);
        unsigned width = et->to<IHasWidth>()->implementationWidthInBits();
        builder->append("memcmp(&");
        if (builder->target->name == "P4TC") {
            emitTCBinaryOperation(b, scalar);
        } else {
            visit(b->left);
            builder->append(", &");
            visit(b->right);
        }
        builder->appendFormat(", %d)", width / 8);
    }
    return false;
}

bool CodeGenInspector::preorder(const IR::Mux *b) {
    int prec = expressionPrecedence;
    bool useParens = prec >= b->getPrecedence();
    if (useParens) builder->append("(");
    expressionPrecedence = b->getPrecedence();
    visit(b->e0);
    builder->append(" ? ");
    expressionPrecedence = DBPrint::Prec_Low;
    visit(b->e1);
    builder->append(" : ");
    expressionPrecedence = b->getPrecedence();
    visit(b->e2);
    expressionPrecedence = prec;
    if (useParens) builder->append(")");
    return false;
}

bool CodeGenInspector::preorder(const IR::Operation_Unary *u) {
    widthCheck(u);
    int prec = expressionPrecedence;
    cstring mask = getMask(typeMap, u);
    bool useParens = prec > u->getPrecedence() || mask != "";
    if (mask != "") builder->append("(");
    if (useParens) builder->append("(");
    builder->append(u->getStringOp());
    expressionPrecedence = u->getPrecedence();
    visit(u->expr);
    expressionPrecedence = prec;
    if (useParens) builder->append(")");
    builder->append(mask);
    if (mask != "") builder->append(")");
    return false;
}

bool CodeGenInspector::preorder(const IR::ArrayIndex *a) {
    int prec = expressionPrecedence;
    bool useParens = prec > a->getPrecedence();
    if (useParens) builder->append("(");
    expressionPrecedence = a->getPrecedence();
    visit(a->left);
    builder->append("[");
    expressionPrecedence = DBPrint::Prec_Low;
    visit(a->right);
    builder->append("]");
    if (useParens) builder->append(")");
    expressionPrecedence = prec;
    return false;
}

bool CodeGenInspector::preorder(const IR::Cast *c) {
    widthCheck(c);
    int prec = expressionPrecedence;
    bool useParens = prec > c->getPrecedence();
    if (useParens) builder->append("(");
    builder->append("(");
    auto et = EBPFTypeFactory::instance->create(c->destType);
    et->emit(builder);
    builder->append(")");
    expressionPrecedence = c->getPrecedence();
    visit(c->expr);
    if (useParens) builder->append(")");
    expressionPrecedence = prec;
    return false;
}

bool CodeGenInspector::preorder(const IR::Member *expression) {
    bool isErrorAccess = false;
    if (auto tne = expression->expr->to<IR::TypeNameExpression>()) {
        if (auto tn = tne->typeName->to<IR::Type_Name>()) {
            if (tn->path->name.name == IR::Type_Error::error) isErrorAccess = true;
        }
    }

    int prec = expressionPrecedence;
    expressionPrecedence = expression->getPrecedence();
    auto ei = P4::EnumInstance::resolve(expression, typeMap);
    if (ei == nullptr && !isErrorAccess) {
        visit(expression->expr);
        auto pe = expression->expr->to<IR::PathExpression>();
        if (pe != nullptr && isPointerVariable(pe->path->name.name)) {
            builder->append("->");
        } else {
            builder->append(".");
        }
    }
    builder->append(expression->member);
    expressionPrecedence = prec;
    return false;
}

bool CodeGenInspector::preorder(const IR::PathExpression *expression) {
    visit(expression->path);
    return false;
}

bool CodeGenInspector::preorder(const IR::Path *p) {
    if (p->absolute) ::P4::error(ErrorType::ERR_EXPECTED, "%1%: Unexpected absolute path", p);
    builder->append(p->name);
    return false;
}

bool CodeGenInspector::preorder(const IR::StructExpression *expr) {
    builder->append("(struct ");
    CHECK_NULL(expr->structType);
    visit(expr->structType);
    builder->append(")");
    builder->append("{");
    bool first = true;
    for (auto c : expr->components) {
        if (!first) builder->append(", ");
        builder->append(".");
        builder->append(c->name);
        builder->append(" = ");
        visit(c->expression);
        first = false;
    }
    builder->append("}");

    return false;
}

bool CodeGenInspector::preorder(const IR::BoolLiteral *b) {
    builder->append(b->toString());
    return false;
}

bool CodeGenInspector::preorder(const IR::ListExpression *expression) {
    bool first = true;
    int prec = expressionPrecedence;
    expressionPrecedence = DBPrint::Prec_Low;
    for (auto e : expression->components) {
        if (!first) builder->append(", ");
        first = false;
        visit(e);
    }
    expressionPrecedence = prec;
    return false;
}

bool CodeGenInspector::preorder(const IR::MethodCallExpression *expression) {
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

    int prec = expressionPrecedence;
    expressionPrecedence = DBPrint::Prec_Postfix;
    visit(expression->method);
    builder->append("(");
    bool first = true;
    for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
        if (!first) builder->append(", ");
        expressionPrecedence = DBPrint::Prec_Low;
        first = false;

        if (p->direction == IR::Direction::Out || p->direction == IR::Direction::InOut)
            builder->append("&");
        auto arg = mi->substitution.lookup(p);
        visit(arg);
    }
    builder->append(")");
    expressionPrecedence = prec;
    return false;
}

/////////////////////////////////////////

bool CodeGenInspector::preorder(const IR::Type_Typedef *type) {
    auto et = EBPFTypeFactory::instance->create(type->type);
    builder->append("typedef ");
    et->emit(builder);
    builder->spc();
    builder->append(type->name);
    builder->endOfStatement();
    return false;
}

bool CodeGenInspector::preorder(const IR::Type_Enum *type) {
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

void CodeGenInspector::emitAssignStatement(const IR::Type *ltype, const IR::Expression *lexpr,
                                           cstring lpath, const IR::Expression *rexpr) {
    auto ebpfType = EBPFTypeFactory::instance->create(ltype);
    bool memcpy = false;
    EBPFScalarType *scalar = nullptr;
    unsigned width = 0;
    if (ebpfType->is<EBPFScalarType>()) {
        scalar = ebpfType->to<EBPFScalarType>();
        width = scalar->implementationWidthInBits();
        memcpy = !EBPFScalarType::generatesScalar(width);
    }
    builder->emitIndent();
    if (memcpy) {
        builder->append("__builtin_memcpy(&");
        if (lexpr != nullptr) {
            visit(lexpr);
        } else {
            builder->append(lpath);
        }
        builder->append(", &");
        if (rexpr->is<IR::Constant>()) {
            builder->appendFormat("(u8[%u])", scalar->bytesRequired());
        }
        visit(rexpr);
        builder->appendFormat(", %d)", scalar->bytesRequired());
    } else {
        if (builder->target->name == "P4TC") {
            emitTCAssignmentEndianessConversion(ltype, lexpr, rexpr, lpath);
        } else {
            if (lexpr != nullptr) {
                visit(lexpr);
            } else {
                builder->append(lpath);
            }
            builder->append(" = ");
            visit(rexpr);
        }
    }
    builder->endOfStatement();
}

bool CodeGenInspector::preorder(const IR::AssignmentStatement *a) {
    auto ltype = typeMap->getType(a->left);
    emitAssignStatement(ltype, a->left, nullptr, a->right);
    return false;
}

bool CodeGenInspector::preorder(const IR::BlockStatement *s) {
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
    if (!s->components.empty()) builder->newline();
    builder->blockEnd(false);
    return false;
}

/// This is correct only after inlining.
bool CodeGenInspector::preorder(const IR::ExitStatement *) {
    builder->append("return");
    builder->endOfStatement();
    return false;
}

bool CodeGenInspector::preorder(const IR::ReturnStatement *) {
    builder->append("return");
    builder->endOfStatement();
    return false;
}

bool CodeGenInspector::preorder(const IR::EmptyStatement *) {
    builder->endOfStatement();
    return false;
}

bool CodeGenInspector::preorder(const IR::IfStatement *s) {
    builder->append("if (");
    visit(s->condition);
    builder->append(") ");
    if (!s->ifTrue->is<IR::BlockStatement>()) {
        builder->increaseIndent();
        builder->newline();
        builder->emitIndent();
    }
    visit(s->ifTrue);
    if (!s->ifTrue->is<IR::BlockStatement>()) builder->decreaseIndent();
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
        if (!s->ifFalse->is<IR::BlockStatement>()) builder->decreaseIndent();
    }
    return false;
}

bool CodeGenInspector::preorder(const IR::MethodCallStatement *s) {
    visit(s->methodCall);
    builder->endOfStatement();
    return false;
}

void CodeGenInspector::widthCheck(const IR::Node *node) const {
    // This is a temporary solution.
    // Rather than generate incorrect results, we reject programs that
    // do not perform arithmetic on machine-supported widths.
    // In the future we will support a wider range of widths
    CHECK_NULL(node);
    auto type = typeMap->getType(node, true);
    auto tb = type->to<IR::Type_Bits>();
    if (tb == nullptr) return;
    if (tb->size % 8 == 0 && EBPFScalarType::generatesScalar(tb->size)) return;

    if (tb->size <= 64) {
        if (!tb->isSigned) return;
        ::P4::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                    "%1%: Computations on signed %2% bits not yet supported", node, tb->size);
    }
    ::P4::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "%1%: Computations on %2% bits not supported",
                node, tb->size);
}

void CodeGenInspector::emitAndConvertByteOrder(const IR::Expression *expr, cstring byte_order) {
    auto ftype = typeMap->getType(expr);
    auto et = EBPFTypeFactory::instance->create(ftype);
    unsigned widthToEmit = dynamic_cast<IHasWidth *>(et)->widthInBits();
    cstring emit = cstring::empty;
    unsigned loadSize = 64;
    if (widthToEmit <= 16) {
        emit = byte_order == "HOST" ? "bpf_ntohs"_cs : "bpf_htons"_cs;
        loadSize = 16;
    } else if (widthToEmit <= 32) {
        emit = byte_order == "HOST" ? "bpf_ntohl"_cs : "bpf_htonl"_cs;
        loadSize = 32;
    } else if (widthToEmit <= 64) {
        emit = byte_order == "HOST" ? "ntohll"_cs : "bpf_cpu_to_be64"_cs;
        loadSize = 64;
    }
    unsigned shift = loadSize - widthToEmit;
    builder->appendFormat("%v(", emit);
    getBitAlignment(expr);
    if (shift != 0 && byte_order == "HOST") builder->appendFormat(" << %d", shift);
    builder->append(")");
}

void CodeGenInspector::emitTCBinaryOperation(const IR::Operation_Binary *b, bool isScalar) {
    const IR::Expression *lexpr = b->left;
    const IR::Expression *rexpr = b->right;
    cstring stringop = b->getStringOp();

    auto action = findContext<IR::P4Action>();
    auto tcTarget = dynamic_cast<const P4TCTarget *>(builder->target);
    cstring lByteOrder = "HOST"_cs, rByteOrder = "HOST"_cs;
    if (lexpr) {
        lByteOrder = tcTarget->getByteOrder(typeMap, action, lexpr);
    }
    if (rexpr) {
        rByteOrder = tcTarget->getByteOrder(typeMap, action, rexpr);
    }
    if (lByteOrder == rByteOrder) {
        getBitAlignment(lexpr);
        if (isScalar) {
            builder->spc();
            builder->append(stringop);
            builder->spc();
        } else {
            builder->append(", &");
        }
        if (!b->is<IR::Operation_Relation>()) expressionPrecedence = b->getPrecedence() + 1;
        getBitAlignment(rexpr);
        return;
    }
    if (lByteOrder == "NETWORK") {
        // ConvertLeft
        auto ftype = typeMap->getType(lexpr);
        auto et = EBPFTypeFactory::instance->create(ftype);
        unsigned width = dynamic_cast<IHasWidth *>(et)->widthInBits();
        if (width <= 8) {
            visit(lexpr);
        } else {
            emitAndConvertByteOrder(lexpr, "NETWORK"_cs);
        }
        if (isScalar) {
            builder->spc();
            builder->append(stringop);
            builder->spc();
        } else {
            builder->append(", &");
        }
        if (!b->is<IR::Operation_Relation>()) expressionPrecedence = b->getPrecedence() + 1;
        getBitAlignment(rexpr);
        return;
    } else if (rByteOrder == "NETWORK") {
        // ConvertRight
        auto ftype = typeMap->getType(rexpr);
        auto et = EBPFTypeFactory::instance->create(ftype);
        unsigned width = dynamic_cast<IHasWidth *>(et)->widthInBits();
        getBitAlignment(rexpr);
        if (isScalar) {
            builder->spc();
            builder->append(stringop);
            builder->spc();
        } else {
            builder->append(", &");
        }
        if (!b->is<IR::Operation_Relation>()) expressionPrecedence = b->getPrecedence() + 1;
        if (width <= 8) {
            visit(rexpr);
        } else {
            emitAndConvertByteOrder(rexpr, "NETWORK"_cs);
        }
    }
    return;
}

void CodeGenInspector::emitTCAssignmentEndianessConversion(const IR::Type *ltype,
                                                           const IR::Expression *lexpr,
                                                           const IR::Expression *rexpr,
                                                           cstring lpath) {
    bool needsBitAlignment = storeBitAlignment(ltype, lexpr, lpath);
    if (!needsBitAlignment) {
        if (lexpr != nullptr) {
            visit(lexpr);
        } else {
            builder->append(lpath);
        }
        builder->append(" = ");
    }
    auto action = findContext<IR::P4Action>();
    auto b = dynamic_cast<const P4TCTarget *>(builder->target);
    cstring lByteOrder = "HOST"_cs, rByteOrder = "HOST"_cs;
    if (lexpr) {
        lByteOrder = b->getByteOrder(typeMap, action, lexpr);
    }
    if (rexpr) {
        rByteOrder = b->getByteOrder(typeMap, action, rexpr);
    }
    auto ftype = typeMap->getType(rexpr);
    auto et = EBPFTypeFactory::instance->create(ftype);
    unsigned width = dynamic_cast<IHasWidth *>(et)->widthInBits();
    if (width <= 8) {
        visit(rexpr);
    } else if (lByteOrder == rByteOrder) {
        getBitAlignment(rexpr);
    } else if (rByteOrder == "NETWORK") {
        // If left side of assignment is not annotated field i.e host endian and right expression
        // is annotated field i.e network endian, we need to convert rexp to host order.
        // Example -
        //     select_0 = hdr.ipv4.diffserv
        //     select_0 = bntoh(hdr.ipv4.diffserv)
        //
        emitAndConvertByteOrder(rexpr, "HOST"_cs);
    } else if (lByteOrder == "NETWORK") {
        // If left side of assignment is annotated field i.e network endian, we need to convert
        // right expression to network order.
        // Example -
        //     hdr.opv4.diffserv = 0x1;
        //     hdr.opv4.diffserv = bhton(0x1)
        //
        emitAndConvertByteOrder(rexpr, "NETWORK"_cs);
    }
    if (needsBitAlignment) {
        builder->append("))");
    }
}

bool CodeGenInspector::storeBitAlignment(const IR::Type *ltype, const IR::Expression *lexpr,
                                         cstring lpath) {
    auto tcTarget = dynamic_cast<const P4TCTarget *>(builder->target);
    if (lexpr != nullptr) {
        if (!(lexpr->is<IR::Member>() || lexpr->is<IR::PathExpression>())) {
            return false;
        }
    }
    auto ebpfType = EBPFTypeFactory::instance->create(ltype);
    EBPFScalarType *scalar = nullptr;
    if (ebpfType->is<EBPFScalarType>()) {
        scalar = ebpfType->to<EBPFScalarType>();
        bool primitive = tcTarget->isPrimitiveByteAligned(scalar->implementationWidthInBits());
        if (primitive) {
            return false;
        } else {
            cstring storePrimitive = scalar->implementationWidthInBits() < 32
                                         ? "storePrimitive32"_cs
                                         : "storePrimitive64"_cs;
            builder->appendFormat("%v((u8 *)&", storePrimitive);
            if (lexpr != nullptr) {
                visit(lexpr);
            } else {
                builder->append(lpath);
            }
            builder->appendFormat(", %d, (", scalar->implementationWidthInBits());
            return true;
        }
    }
    return false;
}

void CodeGenInspector::getBitAlignment(const IR::Expression *expression) {
    if (expression->is<IR::Member>() || expression->is<IR::PathExpression>()) {
        auto ftype = typeMap->getType(expression);
        if (!ftype) {
            visit(expression);
            return;
        }
        auto tcTarget = dynamic_cast<const P4TCTarget *>(builder->target);
        auto ebpfType = EBPFTypeFactory::instance->create(ftype);
        EBPFScalarType *scalar = nullptr;
        if (ebpfType->is<EBPFScalarType>()) {
            scalar = ebpfType->to<EBPFScalarType>();
            bool isPrimitive =
                tcTarget->isPrimitiveByteAligned(scalar->implementationWidthInBits());
            if (!isPrimitive) {
                cstring getPrimitive = scalar->implementationWidthInBits() < 32
                                           ? "getPrimitive32"_cs
                                           : "getPrimitive64"_cs;
                builder->appendFormat("%v((u8 *)", getPrimitive);
                visit(expression);
                builder->appendFormat(", %d)", scalar->implementationWidthInBits());
                return;
            }
        }
    }
    visit(expression);
}

unsigned EBPFInitializerUtils::ebpfTypeWidth(P4::TypeMap *typeMap, const IR::Expression *expr) {
    auto type = typeMap->getType(expr);
    if (type == nullptr) type = expr->type;
    if (type->is<IR::Type_InfInt>()) return 32;  // let's assume 32 bit for int type
    if (type->is<IR::Type_Set>()) type = type->to<IR::Type_Set>()->elementType;
    // FIXME: When a select expression from the parser supports more than one field then condition
    //        below takes into account only the first component, remaining fields will be ignored.
    if (type->is<IR::Type_List>()) type = type->to<IR::Type_List>()->components.front();

    auto ebpfType = EBPFTypeFactory::instance->create(type);
    if (auto scalar = ebpfType->to<EBPFScalarType>()) {
        unsigned width = scalar->implementationWidthInBits();
        unsigned alignment = scalar->alignment() * 8;
        unsigned units = ROUNDUP(width, alignment);
        return units * alignment;
    }
    return 8;  // assume 1 byte if not available such information
}

cstring EBPFInitializerUtils::genHexStr(const big_int &value, unsigned width,
                                        const IR::Expression *expr) {
    // the required length of hex string, must be an even number
    unsigned nibbles = 2 * ROUNDUP(width, 8);
    auto str = value.str(0, std::ios_base::hex);
    if (str.size() < nibbles) str = std::string(nibbles - str.size(), '0') + str;
    BUG_CHECK(str.size() == nibbles, "%1%: value size does not match %2% bits", expr, width);
    return str;
}

}  // namespace P4::EBPF
