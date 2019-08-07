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

#include "ubpfTable.h"
#include "ubpfType.h"
#include "ubpfParser.h"
#include "ir/ir.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/methodInstance.h"

namespace UBPF {

    namespace {

        class UbpfActionTranslationVisitor : public EBPF::CodeGenInspector {
        protected:
            const UBPFProgram *program;
            const IR::P4Action *action;
            cstring valueName;

        public:
            UbpfActionTranslationVisitor(cstring valueName, const UBPFProgram *program) :
                    EBPF::CodeGenInspector(program->refMap, program->typeMap), program(program),
                    action(nullptr), valueName(valueName) {
                CHECK_NULL(program);
            }

            cstring createTmpVariable() {
                return program->refMap->newName("tmp");
            }

            bool preorder(const IR::PathExpression *expression) {
                auto decl = program->refMap->getDeclaration(expression->path, true);
                if (decl->is<IR::Parameter>()) {
                    auto param = decl->to<IR::Parameter>();
                    bool isParam = action->parameters->getParameter(param->name) == param;
                    if (isParam) {
                        builder->append(valueName);
                        builder->append("->u.");
                        cstring name = EBPF::EBPFObject::externalName(action);
                        builder->append(name);
                        builder->append(".");
                        builder->append(expression->path->toString());  // original name
                        return false;
                    }
                }

                visit(expression->path);
                return false;
            }

            bool preorder(const IR::MethodCallExpression *expression) {
                auto mi = P4::MethodInstance::resolve(expression, refMap,
                                                      typeMap);
                auto ef = mi->to<P4::ExternFunction>();
                if (ef != nullptr) {
                    if (ef->method->name.name == program->model.drop.name) {
                        builder->append("pass = false");
                        return false;
                    } else if (ef->method->name.name ==
                               program->model.ubpf_time_get_ns.name) {
                        builder->emitIndent();
                        builder->append("ubpf_time_get_ns()");
                        return false;
                    }
                }
                CodeGenInspector::preorder(expression);
            }

            void emitPacketModification(const IR::Expression *left,
                                        const IR::Expression *right) {
                builder->newline();
                builder->emitIndent();

                auto ltype = typeMap->getType(left);
                auto ubpfType = UBPFTypeFactory::instance->create(ltype);

                bool isWide = false;
                UBPFScalarType *scalar = nullptr;
                unsigned width = 0, widthToExtract = 0;
                if (ubpfType->is<UBPFScalarType>()) {
                    scalar = ubpfType->to<UBPFScalarType>();
                    width = scalar->implementationWidthInBits();
                    isWide = !UBPFScalarType::generatesScalar(width);
                    widthToExtract = scalar->widthInBits();
                }

                auto header_type = program->parser->headerType->to<UBPFStructType>();

                unsigned packetOffsetInBits = 0;
                unsigned alignment = 0;
                bool finished = false;
                for (auto f : header_type->fields) {
                    if (finished)
                        break;
                    auto ftype = typeMap->getType(f->field);
                    auto etype = UBPFTypeFactory::instance->create(ftype);
                    auto et = dynamic_cast<UBPFStructType *>(etype);
                    for (auto inf : et->fields) {

                        if (left->is<IR::Member>()) {
                            if (inf->field->name.name ==
                                left->to<IR::Member>()->member.name) {
                                finished = true;
                                break;
                            }
                        }

                        auto in_et = dynamic_cast<EBPF::IHasWidth *>(inf->type);
                        alignment += in_et->widthInBits();
                        packetOffsetInBits += in_et->widthInBits();
                        alignment %= 8;
                    }
                }


                if (isWide) {
                    // wide values; read all bytes one by one.
                    unsigned shift;
                    if (alignment == 0)
                        shift = 0;
                    else
                        shift = 8 - alignment;

                    const char *helper;
                    const char *scalarType = "uint8_t";
                    if (shift == 0)
                        helper = "load_byte";
                    else {
                        helper = "load_half"; // TODO: why it is needed?
                        scalarType = "uint16_t";
                    }
                    auto bt = UBPFTypeFactory::instance->create(
                            IR::Type_Bits::get(8));
                    unsigned bytes = ROUNDUP(widthToExtract, 8);

                    bool first = true;
                    for (unsigned i = 0; i < bytes; i++) {
                        if (!first)
                            builder->emitIndent();
                        first = false;

                        cstring var = createTmpVariable();
                        scalar->emit(builder);
                        builder->spc();
                        builder->append(var);
                        builder->append(" = &");
                        builder->appendFormat("%s(%s, BYTES(%sOffset) + %d)",
                                              helper,
                                              program->packetStartVar.c_str(),
                                              left->toString(), i);
                        builder->endOfStatement(true);
                        builder->emitIndent();
                        builder->append(scalarType);
                        cstring tmp = createTmpVariable();
                        builder->appendFormat(" %s = ", tmp);
                        visit(right);
                        builder->appendFormat("[%d]", i);
                        builder->endOfStatement(true);

                        builder->emitIndent();
                        builder->appendFormat("*%s = ((*%s) & ~(BPF_MASK(",
                                              var, var);
                        builder->appendFormat("%s, %d)", scalarType, 8);

                        if (shift != 0)
                            builder->appendFormat(" << %d", shift);
                        builder->appendFormat(")) | (%s", tmp);

                        if (shift != 0)
                            builder->appendFormat(" << %d", shift);
                        builder->append(")");
                        builder->endOfStatement(true);
                    }
                } else {

                    const char *var = left->toString().replace('.', '_');
                    scalar->emit(builder);
                    builder->append("*");
                    builder->spc();
                    builder->append(var);
                    builder->append(" = &");
                    unsigned lastBitIndex = widthToExtract + alignment - 1;
                    unsigned lastWordIndex = lastBitIndex / 8;
                    unsigned wordsToRead = lastWordIndex + 1;
                    unsigned loadSize;

                    const char *helper = nullptr;
                    cstring swapToHost = "";
                    cstring swapToNetwork = "";
                    if (wordsToRead <= 1) {
                        helper = "load_byte";
                        loadSize = 8;
                    } else if (widthToExtract <= 16) {
                        helper = "load_half_ptr";
                        swapToHost = "ntohs";
                        swapToNetwork = "bpf_htons";
                        loadSize = 16;
                    } else if (widthToExtract <= 32) {
                        helper = "load_word_ptr";
                        swapToHost = "ntohl";
                        swapToNetwork = "bpf_htonl";
                        loadSize = 32;
                    } else {
                        if (widthToExtract > 64)
                            BUG("Unexpected width %d", widthToExtract);
                        helper = "load_dword_ptr";
                        swapToHost = "ntohll";
                        swapToNetwork = "bpf_htonll";
                        loadSize = 64;
                    }

                    unsigned shift = loadSize - alignment - widthToExtract;
                    builder->appendFormat("%s(%s, BYTES(%sOffset))",
                                          helper,
                                          program->packetStartVar.c_str(),
                                          left->toString());
                    builder->endOfStatement(true);
                    builder->emitIndent();
                    scalar->emit(builder);
                    cstring tmp = createTmpVariable();
                    builder->appendFormat(" %s = ", tmp);
                    visit(right);
                    builder->endOfStatement(true);

                    builder->emitIndent();
                    builder->appendFormat("*%s = %s((%s(*%s) & ~(BPF_MASK(",
                                          var, swapToNetwork, swapToHost, var);
                    scalar->emit(builder);
                    builder->appendFormat(", %d)", widthToExtract);

                    if (shift != 0)
                        builder->appendFormat(" << %d", shift);
                    builder->appendFormat(")) | (%s", tmp);

                    if (shift != 0)
                        builder->appendFormat(" << %d", shift);
                    builder->append("))");
                    builder->endOfStatement(true);
                }
            }

            void convertActionBody(const IR::Vector<IR::StatOrDecl> *body) {
                for (auto s : *body) {
                    if (!s->is<IR::Statement>()) {
                        continue;
                    } else if (auto block = s->to<IR::BlockStatement>()) {
                        builder->blockStart();
                        bool first = true;
                        for (auto a : block->components) {
                            if (!first) {
                                builder->newline();
                                builder->emitIndent();
                            }
                            first = false;
                            convertActionBody(&block->components);
                            builder->blockEnd(true);
                        }
                        if (!block->components.empty())
                            builder->newline();
                        builder->blockEnd(false);
                        continue;
                    } else if (s->is<IR::ReturnStatement>()) {
                        break;
                    } else if (s->is<IR::AssignmentStatement>()) {

                        auto assignment = s->to<IR::AssignmentStatement>();

                        const IR::Expression *left = assignment->left;
                        const IR::Expression *right = assignment->right;

                        if (left->is<IR::Member>()) { // writing to packet
                            emitPacketModification(left, right);
                        } else { // local variable
                            auto ltype = typeMap->getType(left);
                            auto ubpfType = UBPFTypeFactory::instance->create(
                                    ltype);
                            ubpfType->emit(builder);
                            builder->spc();
                            visit(left);
                            builder->append(" = ");
                            visit(right);
                            builder->endOfStatement(true);
                        }
                        continue;
                    } else {
                        visit(body);
                    }
                }
            }

            void convertAction() {
                convertActionBody(&action->body->components);
            }

            bool preorder(const IR::P4Action *act) {
                action = act;
                convertAction();
                return false;
            }
        };  // UbpfActionTranslationVisitor
    }  // namespace

    void UBPFTableBase::emitInstance(EBPF::CodeBuilder *builder) {
        builder->append("struct ");
        builder->appendFormat("ubpf_map_def %s = ", dataMapName);
        builder->spc();
        builder->blockStart();

        builder->emitIndent();
        builder->append(".type = UBPF_MAP_TYPE_HASHMAP,");
        builder->newline();

        builder->emitIndent();
        if (keyType->is<IR::Type_Bits>()) {
            auto tb = keyType->to<IR::Type_Bits>();
            auto scalar = new UBPFScalarType(tb);
            builder->append(".key_size = sizeof(");
            scalar->emit(builder);
            builder->append("),");
        } else {
            builder->appendFormat(".key_size = sizeof(struct %s),",
                                  keyTypeName.c_str());
        }
        builder->newline();

        builder->emitIndent();
        if (valueType->is<IR::Type_Bits>()) {
            auto tb = valueType->to<IR::Type_Bits>();
            auto scalar = new UBPFScalarType(tb);
            builder->append(".value_size = sizeof(");
            scalar->emit(builder);
            builder->append("),");
        } else {
            builder->appendFormat(".value_size = sizeof(struct %s),",
                                  valueTypeName.c_str());
        }
        builder->newline();

        builder->emitIndent();
        builder->appendFormat(".max_entries = %d,", size);
        builder->newline();

        builder->emitIndent();
        builder->append(".nb_hash_functions = 0,");
        builder->newline();

        builder->blockEnd(false);
        builder->endOfStatement(true);
    }

    UBPFTable::UBPFTable(const UBPFProgram *program,
                         const IR::TableBlock *table,
                         EBPF::CodeGenInspector *codeGen) :
            UBPFTableBase(program, EBPFObject::externalName(table->container),
                          codeGen), table(table) {

        cstring base = instanceName + "_defaultAction";
        defaultActionMapName = program->refMap->newName(base);

        base = table->container->name.name + "_actions";
        actionEnumName = program->refMap->newName(base);

        keyGenerator = table->container->getKey();
        actionList = table->container->getActionList();
    }

    void UBPFTable::emitKeyType(EBPF::CodeBuilder *builder) {
        builder->emitIndent();
        builder->appendFormat("struct %s ", keyTypeName.c_str());
        builder->blockStart();

        EBPF::CodeGenInspector commentGen(program->refMap, program->typeMap);
        commentGen.setBuilder(builder);

        if (keyGenerator != nullptr) {
            // Use this to order elements by size
            std::multimap<size_t, const IR::KeyElement *> ordered;
            unsigned fieldNumber = 0;
            for (auto c : keyGenerator->keyElements) {
                auto type = program->typeMap->getType(c->expression);
                auto ebpfType = UBPFTypeFactory::instance->create(type);
                cstring fieldName = c->expression->toString().replace('.', '_');
                if (!ebpfType->is<EBPF::IHasWidth>()) {
                    ::error("%1%: illegal type %2% for key field", c, type);
                    return;
                }
                unsigned width = ebpfType->to<EBPF::IHasWidth>()->widthInBits();
                ordered.emplace(width, c);
                keyTypes.emplace(c, ebpfType);
                keyFieldNames.emplace(c, fieldName);
                fieldNumber++;
            }

            // Emit key in decreasing order size - this way there will be no gaps
            for (auto it = ordered.rbegin(); it != ordered.rend(); ++it) {
                auto c = it->second;

                auto ebpfType = ::get(keyTypes, c);
                builder->emitIndent();
                cstring fieldName = ::get(keyFieldNames, c);
                ebpfType->declare(builder, fieldName, false);
                builder->append("; /* ");
                c->expression->apply(commentGen);
                builder->append(" */");
                builder->newline();

                auto mtdecl = program->refMap->getDeclaration(
                        c->matchType->path, true);
                auto matchType = mtdecl->getNode()->to<IR::Declaration_ID>();
                if (matchType->name.name !=
                    P4::P4CoreLibrary::instance.exactMatch.name)
                    ::error("Match of type %1% not supported", c->matchType);
            }
        }

        builder->blockEnd(false);
        builder->endOfStatement(true);
    }

    void UBPFTable::emitActionArguments(EBPF::CodeBuilder *builder,
                                        const IR::P4Action *action,
                                        cstring name) {
        builder->emitIndent();
        builder->append("struct ");
        builder->blockStart();

        for (auto p : *action->parameters->getEnumerator()) {
            builder->emitIndent();
            auto type = UBPFTypeFactory::instance->create(p->type);
            type->declare(builder, p->name.name, false);
            builder->endOfStatement(true);
        }

        builder->blockEnd(false);
        builder->spc();
        builder->append(name);
        builder->endOfStatement(true);
    }

    void UBPFTable::emitValueType(EBPF::CodeBuilder *builder) {
        // create an enum with tags for all actions
        builder->emitIndent();
        builder->append("enum ");
        builder->append(actionEnumName);
        builder->spc();
        builder->blockStart();

        for (auto a : actionList->actionList) {
            auto adecl = program->refMap->getDeclaration(a->getPath(), true);
            auto action = adecl->getNode()->to<IR::P4Action>();
            cstring name = EBPFObject::externalName(action);
            builder->emitIndent();
            builder->append(name);
            builder->append(",");
            builder->newline();
        }

        builder->blockEnd(false);
        builder->endOfStatement(true);

        // a type-safe union: a struct with a tag and an union
        builder->emitIndent();
        builder->appendFormat("struct %s ", valueTypeName.c_str());
        builder->blockStart();

        builder->emitIndent();
        builder->appendFormat("enum %s action;", actionEnumName.c_str());
        builder->newline();

        builder->emitIndent();
        builder->append("union ");
        builder->blockStart();

        for (auto a : actionList->actionList) {
            auto adecl = program->refMap->getDeclaration(a->getPath(), true);
            auto action = adecl->getNode()->to<IR::P4Action>();
            cstring name = EBPFObject::externalName(action);
            emitActionArguments(builder, action, name);
        }

        builder->blockEnd(false);
        builder->spc();
        builder->appendLine("u;");


        builder->blockEnd(false);
        builder->endOfStatement(true);
    }

    void UBPFTable::emitTypes(EBPF::CodeBuilder *builder) {
        emitKeyType(builder);
        emitValueType(builder);
    }

    void UBPFTable::emitKey(EBPF::CodeBuilder *builder, cstring keyName) {
        if (keyGenerator == nullptr)
            return;
        for (auto c : keyGenerator->keyElements) {
            auto ebpfType = ::get(keyTypes, c);
            cstring fieldName = ::get(keyFieldNames, c);
            CHECK_NULL(fieldName);
            bool memcpy = false;
            EBPF::EBPFScalarType *scalar = nullptr;
            unsigned width = 0;
            if (ebpfType->is<EBPF::EBPFScalarType>()) {
                scalar = ebpfType->to<EBPF::EBPFScalarType>();
                width = scalar->implementationWidthInBits();
                memcpy = !EBPF::EBPFScalarType::generatesScalar(width);
            }

            builder->emitIndent();
            if (memcpy) {
                builder->appendFormat("memcpy(&%s.%s, &", keyName.c_str(),
                                      fieldName.c_str());
                codeGen->visit(c->expression);
                builder->appendFormat(", %d)", scalar->bytesRequired());
            } else {
                builder->appendFormat("%s.%s = ", keyName.c_str(),
                                      fieldName.c_str());
                codeGen->visit(c->expression);
            }
            builder->endOfStatement(true);
        }
    }

    void UBPFTable::emitAction(EBPF::CodeBuilder *builder, cstring valueName) {
        builder->emitIndent();
        builder->appendFormat("switch (%s->action) ", valueName.c_str());
        builder->blockStart();

        for (auto a : actionList->actionList) {
            auto adecl = program->refMap->getDeclaration(a->getPath(), true);
            auto action = adecl->getNode()->to<IR::P4Action>();
            builder->emitIndent();
            cstring name = EBPFObject::externalName(action);
            builder->appendFormat("case %s: ", name.c_str());
            builder->newline();
            builder->emitIndent();
            builder->blockStart();
            builder->emitIndent();

            UbpfActionTranslationVisitor visitor(valueName, program);
            visitor.setBuilder(builder);
            visitor.copySubstitutions(codeGen);

            action->apply(visitor);
            builder->newline();
            builder->blockEnd(true);
            builder->emitIndent();
            builder->appendLine("break;");
        }

        builder->emitIndent();
        builder->appendFormat("default: return %s",
                              builder->target->abortReturnCode().c_str());
        builder->endOfStatement(true);

        builder->blockEnd(true);
    }

    void UBPFTable::emitInitializer(EBPF::CodeBuilder *builder) {
        // emit code to initialize the default action
        const IR::P4Table *t = table->container;
        const IR::Expression *defaultAction = t->getDefaultAction();
        BUG_CHECK(defaultAction->is<IR::MethodCallExpression>(),
                  "%1%: expected an action call", defaultAction);
        auto mce = defaultAction->to<IR::MethodCallExpression>();
        auto mi = P4::MethodInstance::resolve(mce, program->refMap,
                                              program->typeMap);

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
        builder->appendFormat("int %s = BPF_OBJ_GET(MAP_PATH \"/%s\")",
                              fd.c_str(), defaultTable.c_str());
        builder->endOfStatement(true);
        builder->emitIndent();
        builder->appendFormat(
                "if (%s < 0) { fprintf(stderr, \"map %s not loaded\\n\"); exit(1); }",
                fd.c_str(), defaultTable.c_str());
        builder->newline();

        builder->emitIndent();
        builder->appendFormat("struct %s %s = ", valueTypeName.c_str(),
                              value.c_str());
        builder->blockStart();
        builder->emitIndent();
        builder->appendFormat(".action = %s,", name.c_str());
        builder->newline();

        EBPF::CodeGenInspector cg(program->refMap, program->typeMap);
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
        builder->target->emitUserTableUpdate(builder, fd, program->zeroKey,
                                             value);
        builder->newline();

        builder->emitIndent();
        builder->appendFormat("if (ok != 0) { "
                              "perror(\"Could not write in %s\"); exit(1); }",
                              defaultTable.c_str());
        builder->newline();
        builder->blockEnd(true);

        // Emit code for table initializer
        auto entries = t->getEntries();
        if (entries == nullptr)
            return;

        builder->emitIndent();
        builder->blockStart();
        builder->emitIndent();
        builder->appendFormat("int %s = BPF_OBJ_GET(MAP_PATH \"/%s\")",
                              fd.c_str(), dataMapName.c_str());
        builder->endOfStatement(true);
        builder->emitIndent();
        builder->appendFormat(
                "if (%s < 0) { fprintf(stderr, \"map %s not loaded\\n\"); exit(1); }",
                fd.c_str(), dataMapName.c_str());
        builder->newline();

        for (auto e : entries->entries) {
            builder->emitIndent();
            builder->blockStart();

            auto entryAction = e->getAction();
            builder->emitIndent();
            builder->appendFormat("struct %s %s = {", keyTypeName.c_str(),
                                  key.c_str());
            e->getKeys()->apply(cg);
            builder->append("}");
            builder->endOfStatement(true);

            BUG_CHECK(entryAction->is<IR::MethodCallExpression>(),
                      "%1%: expected an action call", defaultAction);
            auto mce = entryAction->to<IR::MethodCallExpression>();
            auto mi = P4::MethodInstance::resolve(mce, program->refMap,
                                                  program->typeMap);

            auto ac = mi->to<P4::ActionCall>();
            BUG_CHECK(ac != nullptr, "%1%: expected an action call", mce);
            auto action = ac->action;
            cstring name = EBPFObject::externalName(action);

            builder->emitIndent();
            builder->appendFormat("struct %s %s = ",
                                  valueTypeName.c_str(), value.c_str());
            builder->blockStart();
            builder->emitIndent();
            builder->appendFormat(".action = %s,", name.c_str());
            builder->newline();

            EBPF::CodeGenInspector cg(program->refMap, program->typeMap);
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
            builder->appendFormat("if (ok != 0) { "
                                  "perror(\"Could not write in %s\"); exit(1); }",
                                  t->name.name.c_str());
            builder->newline();
            builder->blockEnd(true);
        }
        builder->blockEnd(true);
    }

}

