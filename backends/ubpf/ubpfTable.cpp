//
// Created by mateusz on 19.04.19.
//

#include "ubpfTable.h"
#include "ubpfType.h"
#include "ir/ir.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/methodInstance.h"
//#include "ubpfProgram.h"

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
                    action(nullptr), valueName(valueName) { CHECK_NULL(program); }

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
                printf("Visit expression w path expression: %s \n", expression->path->name.name);
                visit(expression->path);
                return false;
            }

            bool preorder(const IR::MethodCallExpression *expression) {
                auto mi = P4::MethodInstance::resolve(expression, refMap, typeMap);
                auto ef = mi->to<P4::ExternFunction>();
                if (ef != nullptr) {
                    if (ef->method->name.name == program->model.drop.name) {
                        builder->emitIndent();
                        builder->append("pass = false");
                        return false;
                    }
                }
                CodeGenInspector::preorder(expression);
            }

            bool preorder(const IR::P4Action *act) {
                action = act;
                visit(action->body);
                return false;
            }
        };  // UbpfActionTranslationVisitor
    }  // namespace

    UBPFTable::UBPFTable(const UBPFProgram *program, const IR::TableBlock *table,
                         EBPF::CodeGenInspector *codeGen) :
            UBPFTableBase(program, EBPFObject::externalName(table->container), codeGen), table(table) {

        printf("Konstruktor UBPFTable");

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
                cstring fieldName = cstring("field") + Util::toString(fieldNumber);
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

                auto mtdecl = program->refMap->getDeclaration(c->matchType->path, true);
                auto matchType = mtdecl->getNode()->to<IR::Declaration_ID>();
                if (matchType->name.name != P4::P4CoreLibrary::instance.exactMatch.name)
                    ::error("Match of type %1% not supported", c->matchType);
            }
        }

        builder->blockEnd(false);
        builder->endOfStatement(true);
    }

    void UBPFTable::emitActionArguments(EBPF::CodeBuilder *builder,
                                        const IR::P4Action *action, cstring name) {
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

//        builder->emitIndent();
//        builder->append("union ");
//        builder->blockStart();
//
//        for (auto a : actionList->actionList) {
//            auto adecl = program->refMap->getDeclaration(a->getPath(), true);
//            auto action = adecl->getNode()->to<IR::P4Action>();
//            cstring name = EBPFObject::externalName(action);
//            emitActionArguments(builder, action, name);
//        }
//
//        builder->blockEnd(false);
//        builder->spc();
//        builder->appendLine("u;");
        builder->blockEnd(false);
        builder->endOfStatement(true);
    }

    void UBPFTable::emitTableDefinition(EBPF::CodeBuilder *builder) {

        //ubpf maps types
        builder->append("enum ");
        builder->append("ubpf_map_type");
        builder->spc();
        builder->blockStart();

        builder->emitIndent();
        builder->append("UBPF_MAP_TYPE_HASHMAP = 1,");
        builder->newline();

//        builder->emitIndent();
//        builder->append("UBPF_MAP_TYPE_ARRAY = 2,");
//        builder->newline();

        builder->blockEnd(false);
        builder->endOfStatement(true);

        // definition if ubpf map
        builder->append("struct ");
        builder->append("ubpf_map_def");
        builder->spc();
        builder->blockStart();

        builder->emitIndent();
        builder->append("enum ubpf_map_type type;");
        builder->newline();

        builder->emitIndent();
        builder->append("unsigned int key_size;");
        builder->newline();

        builder->emitIndent();
        builder->append("unsigned int value_size;");
        builder->newline();

        builder->emitIndent();
        builder->append("unsigned int max_entries;");
        builder->newline();

        builder->emitIndent();
        builder->append("unsigned int nb_hash_functions;");
        builder->newline();

        builder->blockEnd(false);
        builder->endOfStatement(true);

        builder->append("struct ");
        builder->append("ubpf_map_def map_definition = ");
        builder->spc();
        builder->blockStart();

        builder->emitIndent();
        builder->append(".type = UBPF_MAP_TYPE_HASHMAP,");
        builder->newline();

        builder->emitIndent();
        builder->appendFormat(".key_size = sizeof(struct %s),", keyTypeName.c_str());
        builder->newline();

        builder->emitIndent();
        builder->appendFormat(".value_size = sizeof(struct %s),", valueTypeName.c_str());
        builder->newline();

        builder->emitIndent();
        builder->append(".max_entries = 1024,");
        builder->newline();

        builder->emitIndent();
        builder->append(".nb_hash_functions = 0,");
        builder->newline();

        builder->blockEnd(false);
        builder->endOfStatement(true);
    }

    void UBPFTable::emitTypes(EBPF::CodeBuilder *builder) {
        emitKeyType(builder);
        emitValueType(builder);
        emitTableDefinition(builder);
    }

    void UBPFTable::emitInstance(EBPF::CodeBuilder *builder) {
        if (keyGenerator != nullptr) {
            auto impl = table->container->properties->getProperty(
                    program->model.tableImplProperty.name);
            if (impl == nullptr) {
                ::error("Table %1% does not have an %2% property",
                        table->container, program->model.tableImplProperty.name);
                return;
            }

            // Some type checking...
            if (!impl->value->is<IR::ExpressionValue>()) {
                ::error("%1%: Expected property to be an `extern` block", impl);
                return;
            }

            auto expr = impl->value->to<IR::ExpressionValue>()->expression;
            if (!expr->is<IR::ConstructorCallExpression>()) {
                ::error("%1%: Expected property to be an `extern` block", impl);
                return;
            }

            auto block = table->getValue(expr);
            if (block == nullptr || !block->is<IR::ExternBlock>()) {
                ::error("%1%: Expected property to be an `extern` block", impl);
                return;
            }

            bool isHash;
            auto extBlock = block->to<IR::ExternBlock>();
            if (extBlock->type->name.name == program->model.hash_table.name) {
                isHash = true;
            } else {
                ::error("%1%: implementation must be one of %2%",
                        impl, program->model.hash_table.name);
                return;
            }

            auto sz = extBlock->getParameterValue(program->model.hash_table.size.name);
            if (sz == nullptr || !sz->is<IR::Constant>()) {
                ::error("Expected an integer argument for %1%; is the model corrupted?", expr);
                return;
            }
            auto cst = sz->to<IR::Constant>();
            if (!cst->fitsInt()) {
                ::error("%1%: size too large", cst);
                return;
            }
            int size = cst->asInt();
            if (size <= 0) {
                ::error("%1%: negative size", cst);
                return;
            }

            cstring name = EBPFObject::externalName(table->container);
            builder->target->emitTableDecl(builder, name, isHash,
                                           cstring("struct ") + keyTypeName,
                                           cstring("struct ") + valueTypeName, size);
        }
        builder->target->emitTableDecl(builder, defaultActionMapName, false,
                                       program->arrayIndexType,
                                       cstring("struct ") + valueTypeName, 1);
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
                builder->appendFormat("memcpy(&%s.%s, &", keyName.c_str(), fieldName.c_str());
                codeGen->visit(c->expression);
                builder->appendFormat(", %d)", scalar->bytesRequired());
            } else {
                builder->appendFormat("%s.%s = ", keyName.c_str(), fieldName.c_str());
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

            UbpfActionTranslationVisitor visitor(valueName, program);
            visitor.setBuilder(builder);
            visitor.copySubstitutions(codeGen);

            action->apply(visitor);
            builder->newline();
            builder->emitIndent();
            builder->appendLine("break;");
        }

        builder->emitIndent();
        builder->appendFormat("default: return %s", builder->target->abortReturnCode().c_str());
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
        builder->appendFormat("int %s = BPF_OBJ_GET(MAP_PATH \"/%s\")",
                              fd.c_str(), defaultTable.c_str());
        builder->endOfStatement(true);
        builder->emitIndent();
        builder->appendFormat("if (%s < 0) { fprintf(stderr, \"map %s not loaded\\n\"); exit(1); }",
                              fd.c_str(), defaultTable.c_str());
        builder->newline();

        builder->emitIndent();
        builder->appendFormat("struct %s %s = ", valueTypeName.c_str(), value.c_str());
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
        builder->target->emitUserTableUpdate(builder, fd, program->zeroKey, value);
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

            BUG_CHECK(entryAction->is<IR::MethodCallExpression>(),
                      "%1%: expected an action call", defaultAction);
            auto mce = entryAction->to<IR::MethodCallExpression>();
            auto mi = P4::MethodInstance::resolve(mce, program->refMap, program->typeMap);

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

