/*
Copyright 2019 Orange

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

#include <p4/enumInstance.h>
#include "ubpfType.h"
#include "ubpfControl.h"
#include "lib/error.h"
#include "frontends/p4/tableApply.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/parameterSubstitution.h"

namespace UBPF {


    UBPFControlBodyTranslator::UBPFControlBodyTranslator(
            const UBPFControl *control) :
            CodeGenInspector(control->program->refMap,
                             control->program->typeMap), control(control),
            p4lib(P4::P4CoreLibrary::instance) {
        setName("UBPFControlBodyTranslator");
    }

    void UBPFControlBodyTranslator::processFunction(
            const P4::ExternFunction *function) {
        if (function->method->name.name == control->program->model.drop.name) {
            builder->append("pass = false");
            return;
        } else if (function->method->name.name == control->program->model.pass.name) {
            builder->append("pass = true");
            return;
        } else if (function->method->name.name ==
                   control->program->model.ubpf_time_get_ns.name) {
            builder->append("ubpf_time_get_ns()");
            return;
        } else if (function->method->name.name == control->program->model.hash.name) {
            cstring hashKeyInstanceName = createHashKeyInstance(function);

            auto algorithmTypeArgument = function->expr->arguments->at(1)->expression->to<IR::Member>();
            auto algorithmType = algorithmTypeArgument->member.name;

            if (algorithmType == control->program->model.hashAlgorithm.lookup3.name) {
                builder->appendFormat(" = ubpf_hash(&%s, sizeof(%s))", hashKeyInstanceName, hashKeyInstanceName);
            } else {
                ::error("%1%: Not supported hash algorithm type", algorithmType);
            }

            return;
        }
        ::error("%1%: Not supported", function->method);
    }

    cstring UBPFControlBodyTranslator::createHashKeyInstance(const P4::ExternFunction *function) {
        auto dataArgument = function->expr->arguments->at(2)->expression->to<IR::ListExpression>();
        auto hashKey = control->program->refMap->newName("hash_key");
        builder->appendFormat("struct %s ", hashKey);
        builder->blockStart();

        unsigned int struct_size = 0;
        for (auto component : dataArgument->components) {
            auto etype = UBPFTypeFactory::instance->create(component->type);
            auto widthType = etype->to<EBPF::IHasWidth>();
            struct_size += widthType->implementationWidthInBits();
        }
        unsigned int overallPadding = 32 - struct_size % 32;

        std::vector<cstring> paddingInitializers(dataArgument->components.size());
        int paddingIndex = 0;
        for (auto component : dataArgument->components) {
            builder->emitIndent();
            auto etype = UBPFTypeFactory::instance->create(component->type);
            auto fieldName = component->toString().replace(".", "_");
            etype->declare(builder, fieldName, false);
            builder->endOfStatement(true);

            auto widthType = etype->to<EBPF::IHasWidth>();
            auto remainingBits = std::min(32 - widthType->implementationWidthInBits() % 32, overallPadding);
            if (remainingBits < 32 && remainingBits != 0 && widthType->implementationWidthInBits() % 32 != 0) {
                if (remainingBits == 8) {
                    addPadding(paddingInitializers, remainingBits, paddingIndex);
                    overallPadding -= remainingBits;
                } else if (remainingBits == 16) {
                    addPadding(paddingInitializers, remainingBits, paddingIndex);
                    overallPadding -= remainingBits;
                } else if (remainingBits == 24) {
                    addPadding(paddingInitializers, remainingBits, paddingIndex);
                    overallPadding -= remainingBits;
                } else {
                    error("Not supported bitwidth");
                }
            }
            paddingIndex++;
        }
        builder->blockEnd(false);
        builder->endOfStatement(true);
        builder->emitIndent();
        auto hashKeyInstance = control->program->refMap->newName("hash_key_instance");
        builder->appendFormat("struct %s %s = ", hashKey, hashKeyInstance);
        builder->blockStart();
        unsigned int i = 0;
        for (auto component : dataArgument->components) {
            builder->emitIndent();
            builder->appendFormat(".%s = %s", component->toString().replace(".", "_"), component->toString());
            auto paddingInitializer = paddingInitializers.at(i);
            if (paddingInitializer) {
                builder->append(",");
                builder->newline();
                builder->emitIndent();
                builder->append(".");
                builder->append(paddingInitializer);
            }
            if (i < dataArgument->components.size() - 1) {
                builder->append(",");
            }
            i++;
            builder->newline();
        }
        builder->blockEnd(false);
        builder->endOfStatement(true);

        auto destination = function->expr->arguments->at(0);
        builder->emitIndent();
        visit(destination);
        return hashKeyInstance;
    }

    void
    UBPFControlBodyTranslator::addPadding(std::vector<cstring> &paddingInitializers, unsigned int remainingBits,
                                          int paddingIndex) const {
        builder->emitIndent();
        auto padding = control->program->refMap->newName("padding");
        auto paddingInitializer = padding + " = {0}";
        paddingInitializers[paddingIndex] = paddingInitializer;
        builder->appendFormat("uint8_t %s[%u]", padding, remainingBits / 8);
        builder->endOfStatement(true);
    }

    void UBPFControlBodyTranslator::compileEmitField(const IR::Expression *expr,
                                                     cstring field,
                                                     unsigned alignment,
                                                     EBPF::EBPFType *type) {

        unsigned widthToEmit = dynamic_cast<EBPF::IHasWidth *>(type)->widthInBits();
        cstring swap = "";
        if (widthToEmit == 16)
            swap = "htons";
        else if (widthToEmit == 32)
            swap = "htonl";
        if (!swap.isNullOrEmpty()) {
            builder->emitIndent();
            visit(expr);
            builder->appendFormat(".%s = %s(", field.c_str(), swap);
            visit(expr);
            builder->appendFormat(".%s)", field.c_str());
            builder->endOfStatement(true);
        }

        auto program = control->program;
        unsigned bitsInFirstByte = widthToEmit % 8;
        if (bitsInFirstByte == 0) bitsInFirstByte = 8;
        unsigned bitsInCurrentByte = bitsInFirstByte;
        unsigned left = widthToEmit;

        for (unsigned i = 0; i < (widthToEmit + 7) / 8; i++) {
            builder->emitIndent();
            builder->appendFormat("%s = ((char*)(&", program->byteVar.c_str());
            visit(expr);
            builder->appendFormat(".%s))[%d]", field.c_str(), i);
            builder->endOfStatement(true);

            unsigned freeBits = alignment == 0 ? (8 - alignment) : 8;
            unsigned bitsToWrite =
                    bitsInCurrentByte > freeBits ? freeBits : bitsInCurrentByte;

            BUG_CHECK((bitsToWrite > 0) && (bitsToWrite <= 8),
                      "invalid bitsToWrite %d", bitsToWrite);
            builder->emitIndent();
            if (alignment == 0)
                builder->appendFormat(
                        "write_byte(%s, BYTES(%s) + %d, (%s) << %d)",
                        program->packetStartVar.c_str(),
                        program->offsetVar.c_str(), i,
                        program->byteVar.c_str(), 8 - bitsToWrite);
            else
                builder->appendFormat(
                        "write_partial(%s + BYTES(%s) + %d, %d, (%s) << %d)",
                        program->packetStartVar.c_str(),
                        program->offsetVar.c_str(), i,
                        alignment,
                        program->byteVar.c_str(), 8 - bitsToWrite);
            builder->endOfStatement(true);
            left -= bitsToWrite;
            bitsInCurrentByte -= bitsToWrite;

            if (bitsInCurrentByte > 0) {
                builder->emitIndent();
                builder->appendFormat(
                        "write_byte(%s, BYTES(%s) + %d + 1, (%s << %d))",
                        program->packetStartVar.c_str(),
                        program->offsetVar.c_str(), i, program->byteVar.c_str(),
                        8 - alignment % 8);
                builder->endOfStatement(true);
                left -= bitsInCurrentByte;
            }

            alignment = (alignment + bitsToWrite) % 8;
            bitsInCurrentByte = left >= 8 ? 8 : left;
        }

        builder->emitIndent();
        builder->appendFormat("%s += %d", program->offsetVar.c_str(),
                              widthToEmit);
        builder->endOfStatement(true);
    }

    void UBPFControlBodyTranslator::compileEmit(
            const IR::Vector<IR::Argument> *args) {
        BUG_CHECK(args->size() == 1, "%1%: expected 1 argument for emit", args);

        auto expr = args->at(0)->expression;
        auto type = typeMap->getType(expr);
        auto ht = type->to<IR::Type_Header>();
        if (ht == nullptr) {
            ::error("Cannot emit a non-header type %1%", expr);
            return;
        }

        auto program = control->program;
        builder->emitIndent();
        builder->append("if (");
        visit(expr);
        builder->append(".ebpf_valid) ");
        builder->blockStart();

        unsigned width = ht->width_bits();
        builder->emitIndent();
        builder->appendFormat("if (%s < %s + BYTES(%s + %d)) ",
                              program->packetEndVar.c_str(),
                              program->packetStartVar.c_str(),
                              program->offsetVar.c_str(), width);
        builder->blockStart();

        builder->emitIndent();
        builder->appendFormat("%s = %s;", program->errorVar.c_str(),
                              p4lib.packetTooShort.str());
        builder->newline();

        builder->emitIndent();
        builder->appendFormat("return %s;",
                              builder->target->abortReturnCode().c_str());
        builder->newline();
        builder->blockEnd(true);

        unsigned alignment = 0;
        for (auto f : ht->fields) {
            auto ftype = typeMap->getType(f);
            auto etype = UBPFTypeFactory::instance->create(ftype);
            auto et = dynamic_cast<EBPF::IHasWidth *>(etype);
            if (et == nullptr) {
                ::error("Only headers with fixed widths supported %1%", f);
                return;
            }
            compileEmitField(expr, f->name, alignment, etype);
            alignment += et->widthInBits();
            alignment %= 8;
        }

        builder->blockEnd(true);
    }

    void
    UBPFControlBodyTranslator::processMethod(const P4::ExternMethod *method) {
        auto decl = method->object;
        auto declType = method->originalExternType;

        if (declType->name.name == p4lib.packetOut.name) {
            if (method->method->name.name == p4lib.packetOut.emit.name) {
                compileEmit(method->expr->arguments);
                return;
            }
        } else if (declType->name.name ==
                   UBPFModel::instance.registerModel.name) {
            if (method->method->name.name ==
                UBPFModel::instance.registerModel.write.name) {
                auto arg_key = method->expr->arguments->at(0);
                auto type = arg_key->expression->type;
                if (type->is<IR::Type_Bits>()) {
                    auto keyType = type->to<IR::Type_Bits>();
                    auto scalar = new UBPFScalarType(keyType);
                    auto scalarType = UBPFTypeFactory::instance->create(
                            scalar->type);
                    auto scalarName = control->program->refMap->newName(
                            "const_value");
                    scalarType->declare(builder, scalarName, false);
                    builder->append(" = ");
                    control->codeGen->visit(arg_key->expression);
                    builder->endOfStatement(true);
                    builder->emitIndent();
                }
            }

            cstring name = decl->getName().name;
            auto registerMap = control->getRegister(name);
            registerMap->emitMethodInvocation(builder, method, pointerVariables);
            return;
        }
        ::error("%1%: Unexpected method call", method->expr);
    }

    void
    UBPFControlBodyTranslator::processApply(const P4::ApplyMethod *method) {
        auto table = (UBPFTable *) control->getTable(
                method->object->getName().name);
        BUG_CHECK(table != nullptr, "No table for %1%", method->expr);

        P4::ParameterSubstitution binding;
        cstring actionVariableName;
        if (!saveAction.empty()) {
            actionVariableName = saveAction.at(saveAction.size() - 1);
            if (!actionVariableName.isNullOrEmpty()) {
                builder->appendFormat("enum %s %s;\n",
                                      table->actionEnumName.c_str(),
                                      actionVariableName.c_str());
                builder->emitIndent();
            }
        }
        builder->blockStart();

        BUG_CHECK(method->expr->arguments->size() == 0,
                  "%1%: table apply with arguments", method);
        cstring keyname = "key";
        if (table->keyGenerator != nullptr) {
            builder->emitIndent();
            builder->appendLine("/* construct key */");
            builder->emitIndent();
            builder->appendFormat("struct %s %s = {}",
                                  table->keyTypeName.c_str(), keyname.c_str());
            builder->endOfStatement(true);
            table->emitKey(builder, keyname);
        }
        builder->emitIndent();
        builder->appendLine("/* value */");
        builder->emitIndent();
        cstring valueName = "value";
        builder->appendFormat("struct %s *%s = NULL",
                              table->valueTypeName.c_str(), valueName.c_str());
        builder->endOfStatement(true);

        if (table->keyGenerator != nullptr) {
            builder->emitIndent();
            builder->appendLine("/* perform lookup */");
            builder->emitIndent();
            builder->appendFormat("%s = ", valueName.c_str());
            builder->target->emitTableLookup(builder, table->dataMapName,
                                             keyname, valueName);
            builder->endOfStatement(true);
        }

        builder->newline();
        builder->emitIndent();
        builder->appendFormat("if (%s != NULL) ", valueName.c_str());
        builder->blockStart();
        builder->emitIndent();
        builder->appendLine("/* run action */");
        table->emitAction(builder, valueName);
        if (!actionVariableName.isNullOrEmpty()) {
            builder->emitIndent();
            builder->appendFormat("%s = %s->action",
                                  actionVariableName.c_str(),
                                  valueName.c_str());
            builder->endOfStatement(true);
        }
        toDereference.clear();

        builder->blockEnd(true);
        builder->emitIndent();
        builder->appendFormat("else return %s",
                              builder->target->abortReturnCode().c_str());
        builder->endOfStatement(true);

        builder->blockEnd(true);
    }

    bool
    UBPFControlBodyTranslator::preorder(const IR::PathExpression *expression) {
        auto decl = control->program->refMap->getDeclaration(expression->path,
                                                             true);
        auto param = decl->getNode()->to<IR::Parameter>();
        if (param != nullptr) {
            if (toDereference.count(param) > 0)
                builder->append("*");
            auto subst = ::get(substitution, param);
            if (subst != nullptr) {
                builder->append(subst->name);
                return false;
            }
        }
        builder->append(
                expression->path->name);  // each identifier should be unique
        return false;
    }

    bool UBPFControlBodyTranslator::preorder(const IR::MethodCallStatement *s) {
        visit(s->methodCall);
        builder->endOfStatement(true);
        return false;
    }

    bool UBPFControlBodyTranslator::preorder(
            const IR::MethodCallExpression *expression) {
        auto mi = P4::MethodInstance::resolve(expression,
                                              control->program->refMap,
                                              control->program->typeMap);
        auto apply = mi->to<P4::ApplyMethod>();
        if (apply != nullptr) {
            processApply(apply);
            return false;
        }
        auto ef = mi->to<P4::ExternFunction>();
        if (ef != nullptr) {
            processFunction(ef);
            return false;
        }
        auto ext = mi->to<P4::ExternMethod>();
        if (ext != nullptr) {
            processMethod(ext);
            return false;
        }
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
        auto ac = mi->to<P4::ActionCall>();
        if (ac != nullptr) {
            // Action arguments have been eliminated by the mid-end.
            BUG_CHECK(expression->arguments->size() == 0,
                      "%1%: unexpected arguments for action call", expression);
            visit(ac->action->body);
            return false;
        }

        ::error("Unsupported method invocation %1%", expression);
        return false;
    }

    bool UBPFControlBodyTranslator::preorder(const IR::AssignmentStatement *a) {
        auto ltype = typeMap->getType(a->left);
        auto ebpfType = UBPFTypeFactory::instance->create(ltype);
        bool memcpy = false;
        UBPFScalarType *scalar = nullptr;
        unsigned width = 0;
        if (ebpfType->is<UBPFScalarType>()) {
            scalar = ebpfType->to<UBPFScalarType>();
            width = scalar->implementationWidthInBits();
            memcpy = !UBPFScalarType::generatesScalar(width);
        }

        cstring keyName = "";
        if (a->right->is<IR::MethodCallExpression>()) {
            auto method = a->right->to<IR::MethodCallExpression>();
            if (method->method->is<IR::Member>()) {
                auto arg_key = method->arguments->at(0);
                keyName = arg_key->name.name;

                auto methodName = method->method->to<IR::Member>()->member.name;

                if (methodName == UBPFModel::instance.registerModel.read.name ||
                    methodName == UBPFModel::instance.registerModel.write.name) {
                    auto type = arg_key->expression->type;
                    if (type->is<IR::Type_Bits>()) {
                        auto keyType = type->to<IR::Type_Bits>();
                        auto tb = keyType->to<IR::Type_Bits>();
                        auto scalarType = new UBPFScalarType(tb);
                        auto scalarInstance = UBPFTypeFactory::instance->create(
                                scalarType->type);
                        auto scalarName = control->program->refMap->newName(
                                "const_value");
                        keyName = scalarName;
                        scalarInstance->declare(builder, scalarName, false);
                        builder->append(" = ");
                        control->codeGen->visit(arg_key->expression);
                        builder->endOfStatement(true);
                        builder->emitIndent();
                    } else if (type->is<IR::Type_StructLike>()) {
                        keyName = arg_key->expression->to<IR::PathExpression>()->path->name.name;
                    }
                }
            }
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
            if (a->right->is<IR::PathExpression>()) {
                auto name = a->right->to<IR::PathExpression>()->path->name.name;
                if (std::find(pointerVariables.begin(), pointerVariables.end(), name) !=
                    pointerVariables.end()) {
                    builder->append("*");
                }
            }
            visit(a->right);
        }
        builder->endOfStatement();


        if (a->right->is<IR::MethodCallExpression>()) {
            auto method = a->right->to<IR::MethodCallExpression>();
            if (method->method->is<IR::Member>()) {
                auto methodName = method->method->to<IR::Member>()->member.name;
                if (methodName == UBPFModel::instance.registerModel.read.name) {
                    builder->newline();

                    auto registerName = method->method->to<IR::Member>()->expr->to<IR::PathExpression>()->path->name.name;
                    auto pRegister = control->registers.find(
                            registerName)->second;

                    builder->emitIndent();
                    auto valueName = a->left->to<IR::PathExpression>()->path->name.name;
                    builder->appendFormat("if (%s != NULL) ", valueName);
                    builder->blockStart();

                    registersLookups.push_back(pRegister);
                    registerKeys.push_back(keyName);
                }
            }
        }

        return false;
    }

    bool UBPFControlBodyTranslator::preorder(const IR::BlockStatement *s) {
        builder->blockStart();
        for (auto a : s->components) {
            builder->newline();
            builder->emitIndent();
            visit(a);
        }
        if (!s->components.empty())
            builder->newline();
        builder->blockEnd(false);
        return false;
    }

    bool UBPFControlBodyTranslator::preorder(const IR::ExitStatement *) {
        builder->appendFormat("goto %s;", control->program->endLabel.c_str());
        return false;
    }

    bool UBPFControlBodyTranslator::preorder(const IR::ReturnStatement *) {
        builder->appendFormat("goto %s;", control->program->endLabel.c_str());
        return false;
    }

    bool UBPFControlBodyTranslator::preorder(const IR::IfStatement *statement) {
        builder->append("if (");
        visit(statement->condition);
        builder->append(") ");
        if (!statement->ifTrue->is<IR::BlockStatement>()) {
            builder->blockStart();
            builder->emitIndent();
        }
        visit(statement->ifTrue);
        if (!statement->ifTrue->is<IR::BlockStatement>()) {
            builder->blockEnd(statement->ifFalse == nullptr);
        }
        if (statement->ifFalse != nullptr) {
            builder->append(" else ");
            if (!statement->ifFalse->is<IR::BlockStatement>()) {
                builder->blockStart();
                builder->emitIndent();
            }
            visit(statement->ifFalse);
            if (!statement->ifFalse->is<IR::BlockStatement>()) {
                builder->blockEnd(true);
            }
        }
        return false;
    }

    bool
    UBPFControlBodyTranslator::preorder(const IR::SwitchStatement *statement) {
        cstring newName = control->program->refMap->newName("action_run");
        saveAction.push_back(newName);
        // This must be a table.apply().action_run
        auto mem = statement->expression->to<IR::Member>();
        BUG_CHECK(mem != nullptr,
                  "%1%: Unexpected expression in switch statement",
                  statement->expression);
        visit(mem->expr);
        saveAction.pop_back();
        saveAction.push_back(nullptr);
        builder->emitIndent();
        builder->append("switch (");
        builder->append(newName);
        builder->append(") ");
        builder->blockStart();
        builder->appendLine("Switch statement start block \n");
        for (auto c : statement->cases) {
            builder->emitIndent();
            builder->appendLine("Statement case \n");
            if (c->label->is<IR::DefaultExpression>()) {
                builder->append("default");
            } else {
                builder->append("case ");
                auto pe = c->label->to<IR::PathExpression>();
                auto decl = control->program->refMap->getDeclaration(pe->path,
                                                                     true);
                BUG_CHECK(decl->is<IR::P4Action>(), "%1%: expected an action",
                          pe);
                auto act = decl->to<IR::P4Action>();
                cstring name = EBPF::EBPFObject::externalName(act);
                builder->append(name);
            }
            builder->append(":");
            builder->newline();
            builder->emitIndent();
            visit(c->statement);
            builder->newline();
            builder->emitIndent();
            builder->appendLine("break;");
        }
        builder->blockEnd(false);
        saveAction.pop_back();
        return false;
    }

    bool
    UBPFControlBodyTranslator::preorder(const IR::Operation_Binary *b) {
        widthCheck(b);
        builder->append("(");
        if (b->left->is<IR::PathExpression>()) {
            auto name = b->left->to<IR::PathExpression>()->path->name.name;
            if (std::find(pointerVariables.begin(), pointerVariables.end(), name) !=
                pointerVariables.end()) {
                builder->append("*");
            }
        }
        visit(b->left);
        builder->spc();
        builder->append(b->getStringOp());
        builder->spc();
        if (b->right->is<IR::PathExpression>()) {
            auto name = b->right->to<IR::PathExpression>()->path->name.name;
            if (std::find(pointerVariables.begin(), pointerVariables.end(), name) != pointerVariables.end()) {
                builder->append("*");
            }
        }
        visit(b->right);
        builder->append(")");
        return false;
    }

    bool
    UBPFControlBodyTranslator::comparison(const IR::Operation_Relation* b) {
        auto type = typeMap->getType(b->left);
        auto et = UBPFTypeFactory::instance->create(type);

        bool scalar = (et->is<UBPFScalarType>() &&
                       UBPFScalarType::generatesScalar(et->to<UBPFScalarType>()->widthInBits()))
                      || et->is<UBPFBoolType>();
        if (scalar) {
            builder->append("(");
            if (b->left->is<IR::PathExpression>()) {
                auto name = b->left->to<IR::PathExpression>()->path->name.name;
                if (std::find(pointerVariables.begin(), pointerVariables.end(), name) !=
                    pointerVariables.end()) {
                    builder->append("*");
                }
            }
            visit(b->left);
            builder->spc();
            builder->append(b->getStringOp());
            builder->spc();
            visit(b->right);
            if (b->right->is<IR::PathExpression>()) {
                auto name = b->right->to<IR::PathExpression>()->path->name.name;
                if (std::find(pointerVariables.begin(), pointerVariables.end(), name) != pointerVariables.end()) {
                    builder->append("*");
                }
            }
            builder->append(")");
        } else {
            if (!et->is<EBPF::IHasWidth>())
                BUG("%1%: Comparisons for type %2% not yet implemented", type);
            unsigned width = et->to<EBPF::IHasWidth>()->implementationWidthInBits();
            builder->append("memcmp(&");
            visit(b->left);
            builder->append(", &");
            visit(b->right);
            builder->appendFormat(", %d)", width / 8);
        }
        return false;
    }

    bool
    UBPFControlBodyTranslator::preorder(const IR::Member *expression) {
        cstring name = "";
        if (expression->expr->is<IR::PathExpression>()) {
            name = expression->expr->to<IR::PathExpression>()->path->name.name;
        }
        auto ei = P4::EnumInstance::resolve(expression, typeMap);
        if (std::find(pointerVariables.begin(), pointerVariables.end(), name) != pointerVariables.end()) {
            visit(expression->expr);
            builder->append("->");
        } else if (ei == nullptr) {
            visit(expression->expr);
            builder->append(".");
        }
        builder->append(expression->member);
        return false;
    }

    UBPFControl::UBPFControl(const UBPFProgram *program,
                             const IR::ControlBlock *block,
                             const IR::Parameter *parserHeaders) :
            program(program), controlBlock(block), headers(nullptr),
            parserHeaders(parserHeaders), codeGen(nullptr) {}

    void UBPFControl::scanConstants() {
        for (auto c : controlBlock->constantValue) {
            auto b = c.second;
            if (b->is<IR::TableBlock>()) {
                auto tblblk = b->to<IR::TableBlock>();
                auto tbl = new UBPFTable(program, tblblk, codeGen);
                tables.emplace(tblblk->container->name, tbl);
            } else if (b->is<IR::ExternBlock>()) {
                auto ctrblk = b->to<IR::ExternBlock>();
                auto node = ctrblk->node;
                if (node->is<IR::Declaration_Instance>()) {
                    auto di = node->to<IR::Declaration_Instance>();
                    cstring name = di->name.name;
                    auto ctr = new UBPFRegister(program, ctrblk, name, codeGen);
                    registers.emplace(name, ctr);
                }
            } else if (!b->is<IR::Block>()) {
                continue;
            } else {
                ::error("Unexpected block %s nested within control",
                        b->toString());
            }
        }
    }

    void UBPFControl::emit(EBPF::CodeBuilder *builder) {
        for (auto a : controlBlock->container->controlLocals)
            emitDeclaration(builder, a);
        codeGen->setBuilder(builder);
        builder->emitIndent();
        controlBlock->container->body->apply(*codeGen);

        for (auto reg : codeGen->registersLookups) {
            builder->newline();
            builder->blockEnd(true);
        }
    }

    const IR::Statement *UBPFControl::findFirstStatementWhereVariableIsUsedAsPointer(
            const IR::Statement *statement,
            const IR::Declaration_Variable *vd) {
        if (statement->is<IR::IfStatement>()) {
            auto ifStat = statement->to<IR::IfStatement>();
            auto result = findFirstStatementWhereVariableIsUsedAsPointer(
                    ifStat->ifTrue, vd);
            if (result == nullptr) {
                return findFirstStatementWhereVariableIsUsedAsPointer(
                        ifStat->ifFalse, vd);
            } else {
                return result;
            }
        }
        if (statement->is<IR::BlockStatement>()) {
            auto block = statement->to<IR::BlockStatement>();
            for (auto cmpnt : block->components) {
                if (cmpnt->is<IR::Statement>()) {
                    auto result = findFirstStatementWhereVariableIsUsedAsPointer(
                            cmpnt->to<IR::Statement>(), vd);
                    if (result != nullptr) {
                        return result;
                    }
                }
            }
        }
        if (statement->is<IR::AssignmentStatement>() &&
            variableIsUsedAsPointer(vd,
                                    statement->to<IR::AssignmentStatement>())) {
            return statement;
        }

        return nullptr;
    }

    void UBPFControl::emitDeclaration(EBPF::CodeBuilder *builder,
                                      const IR::Declaration *decl) {
        if (decl->is<IR::Declaration_Variable>()) {
            auto vd = decl->to<IR::Declaration_Variable>();
            auto etype = UBPFTypeFactory::instance->create(vd->type);
            builder->emitIndent();
            bool pointerType = true;

            auto comps = controlBlock->container->body->components;
            for (auto comp : comps) {
                if (comp->is<IR::AssignmentStatement>()) {
                    auto assiStat = comp->to<IR::AssignmentStatement>();
                    pointerType = variableIsUsedAsPointer(vd, assiStat);
                    if (pointerType) {
                        break;
                    }
                } else {
                    if (comp->is<IR::Statement>()) {
                        auto result = findFirstStatementWhereVariableIsUsedAsPointer(
                                comp->to<IR::Statement>(), vd);
                        if (result == nullptr) {
                            pointerType = false;
                        } else {
                            pointerType = true;
                            break;
                        }
                    }
                }
            }
            for (auto it : registers) {
                if (etype->type->is<IR::Type_Name>() &&
                    it.second->keyTypeName ==
                    etype->type->to<IR::Type_Name>()->path->name.name) {
                    pointerType = false;
                }
            }

            if (pointerType) {
                codeGen->pointerVariables.push_back(vd->name.name);
            }

            etype->declare(builder, vd->name, pointerType);
            builder->endOfStatement(true);
            BUG_CHECK(vd->initializer == nullptr,
                      "%1%: declarations with initializers not supported",
                      decl);
            return;
        } else if (decl->is<IR::P4Table>() ||
                   decl->is<IR::P4Action>() ||
                   decl->is<IR::Declaration_Instance>()) {
            return;
        }
        BUG("%1%: not yet handled", decl);
    }

    bool
    UBPFControl::variableIsUsedAsPointer(const IR::Declaration_Variable *vd,
                                         const IR::AssignmentStatement *assiStat) const {
        bool pointerType = false;
        if (assiStat->right->is<IR::MethodCallExpression>()) {
            auto methCall = assiStat->right->to<IR::MethodCallExpression>();
            if (methCall->method->is<IR::Member>() &&
                methCall->method->to<IR::Member>()->member.name == UBPFModel::instance.registerModel.read.name &&
                assiStat->left->is<IR::PathExpression>()) {
                auto variable = assiStat->left->to<IR::PathExpression>();
                if (variable->path->name.name == vd->name.name) {
                    pointerType = true;
                }
            }
        }
        return pointerType;
    }

    void UBPFControl::emitTableTypes(EBPF::CodeBuilder *builder) {
        for (auto it : tables)
            it.second->emitTypes(builder);
    }

    void UBPFControl::emitTableInstances(EBPF::CodeBuilder *builder) {
        for (auto it : tables)
            it.second->emitInstance(builder);
        for (auto it : registers)
            it.second->emitInstance(builder);
    }

    bool UBPFControl::build() {
        passVariable = program->refMap->newName("pass");
        auto pl = controlBlock->container->type->applyParams;
        if (pl->size() != 2) {
            ::error("Expected control block to have exactly 2 parameter");
            return false;
        }

        auto it = pl->parameters.begin();
        headers = *it;

        codeGen = new UBPFControlBodyTranslator(this);
        codeGen->substitute(headers, parserHeaders);

        scanConstants();
        return ::errorCount() == 0;
    }

} // UBPF