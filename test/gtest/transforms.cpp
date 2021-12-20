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

#include "gtest/gtest.h"
#include "helpers.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/source_file.h"

#include "frontends/common/options.h"
#include "frontends/p4/createBuiltins.h"

namespace Test {

class P4C_IR : public P4CTest { };

TEST_F(P4C_IR, Transform) {
    struct TestTrans : public Transform {
        explicit TestTrans(IR::Constant* c) : c(c) { }

        IR::Node* postorder(IR::Add* a) override {
            EXPECT_EQ(c, a->left);
            EXPECT_EQ(c, a->right);
            return a;
        }

        IR::Constant* c;
    };

    auto c = new IR::Constant(2);
    IR::Expression* e = new IR::Add(Util::SourceInfo(), c, c);
    auto* n = e->apply(TestTrans(c));
    EXPECT_EQ(e, n);
}

class MidEndForFlatten : public PassManager {
 public:
    P4::ReferenceMap    refMap;
    P4::TypeMap         typeMap;
    IR::ToplevelBlock   *toplevel = nullptr;

    explicit MidEndForFlatten(CompilerOptions options) {
        bool isv1 = options.langVersion == CompilerOptions::FrontendVersion::P4_14;
        refMap.setIsV1(isv1);
        auto evaluator = new P4::EvaluatorPass(&refMap, &typeMap);
        setName("MidEndForFlatten");

        addPasses({
            new P4::FlattenInterfaceStructs(&refMap, &typeMap),
            evaluator,
            new P4::MidEndLast()
        });

        toplevel = evaluator->getToplevelBlock();
    }
    IR::ToplevelBlock* process(const IR::P4Program *&program) {
        program = program->apply(*this);
        return toplevel; }
};

const IR::Expression* getKeyExpression(const IR::P4Program* program, const char* controlName,
                                       const char* tableName, const char* fieldName) {
    CHECK_NULL(program);
    CHECK_NULL(controlName);
    CHECK_NULL(tableName);
    CHECK_NULL(fieldName);
    const auto* control =
        program->getDeclsByName(controlName)->toVector()->at(0)->to<IR::P4Control>();
    const auto* table = control->getDeclByName(tableName)->to<IR::P4Table>();
    for (auto key : table->getKey()->keyElements) {
        std::ostringstream output;
        key->expression->dbprint(output);
        if (output.str().find(fieldName) != std::string::npos) {
            return key->expression;
        }
    }
    return nullptr;
}

TEST_F(P4C_IR, FlattenInterface) {
    AutoCompileContext autoP4TestContext(new P4CContextWithOptions<P4TestOptions>);
    auto& options = P4TestContext::get().options();
    const char* argv = "./gtestp4c";
    options.process(1, (char* const*)&argv);
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;
    const IR::P4Program* load_model(const char* curFile, CompilerOptions& options);
    const IR::P4Program* program = load_model("issue983-bmv2.p4", options);
    P4::FrontEnd frontend;
    program = frontend.run(options, program);
    CHECK_NULL(program);
    MidEndForFlatten flattenMidend(options);
    const auto* newProgram = program->apply(flattenMidend);
    const auto* oldKeyExpression =
        getKeyExpression(program, "ingress", "debug_table_cksum1_0", "exp_etherType");
    const auto* newKeyExpression =
        getKeyExpression(newProgram, "ingress", "debug_table_cksum1_0", "exp_etherType");
    CHECK_NULL(oldKeyExpression);
    CHECK_NULL(newKeyExpression);
    CHECK_NULL(oldKeyExpression->type);
    CHECK_NULL(newKeyExpression->type);
    ASSERT_TRUE(oldKeyExpression->type->equiv(*newKeyExpression->type));
}

}  // namespace Test
