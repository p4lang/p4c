#include <gtest/gtest.h>

#include "frontends/common/parseInput.h"
#include "frontends/p4/dontcareArgs.h"
#include "frontends/p4/frontend.h"
#include "helpers.h"
#include "ir/ir.h"

using namespace ::P4;

namespace P4::Test {

struct RemoveDontcareArgsTest : P4CTest {
    const IR::Node *parseAndProcess(std::string program) {
        const auto *pgm = P4::parseP4String(program, CompilerOptions::FrontendVersion::P4_16);
        EXPECT_TRUE(pgm);
        EXPECT_EQ(::P4::errorCount(), 0);
        if (!pgm) {
            return nullptr;
        }

        P4::TypeMap typeMap;
        RemoveDontcareArgs rdca(&typeMap);

        return pgm->apply(rdca);
    }
};

class CollectActionAndControlLocals : public Inspector {
 public:
    unsigned actionDecls = 0;
    unsigned controlDecls = 0;

    bool preorder(const IR::P4Action *action) override {
        for (const auto *c : action->body->components) {
            if (c->is<IR::Declaration_Variable>()) ++actionDecls;
        }
        return true;
    }

    bool preorder(const IR::P4Control *control) override {
        for (const auto *c : control->controlLocals) {
            if (c->is<IR::Declaration_Variable>()) ++controlDecls;
        }
        return true;
    }
};

TEST_F(RemoveDontcareArgsTest, Default) {
    std::string program_source = P4_SOURCE(R"(
struct S {
    bit<64> f;
}

control C(inout S s) {
    @name("d") action d_0(@name("b") out bit<64> b_0) {
        b_0 = 64w4;
    }
    @name("foo") action foo_0() {
        d_0(_);
    }
    @name("t") table t_0 {
        actions = {
            foo_0();
        }
        default_action = foo_0();
    }
    apply {
        t_0.apply();
    }
}

control proto(inout S s);
package top(proto p);
top(C()) main;
    )");

    const auto *program = parseAndProcess(program_source);
    ASSERT_TRUE(program);
    ASSERT_EQ(::P4::errorCount(), 0);

    CollectActionAndControlLocals collect;
    program->apply(collect);

    // Just make sure that the control block has no decls and the action has a single decl.
    // The expected output should look something like this:
    // struct S {
    //     bit<64> f;
    // }
    // control C(inout S s) {
    //     @name("d") action d_0(@name("b") out bit<64> b_0) {
    //         b_0 = 64w4;
    //     }
    //     @name("foo") action foo_0() {
    //         bit<64> arg;
    //         d_0(arg);
    //     }
    //     @name("t") table t_0 {
    //         actions = {
    //             foo_0();
    //         }
    //         default_action = foo_0();
    //     }
    //     apply {
    //         t_0.apply();
    //     }
    // }
    // control proto(inout S s);
    // package top(proto p);
    // top(C()) main;
    ASSERT_EQ(collect.actionDecls, 1);
    ASSERT_EQ(collect.controlDecls, 0);
}

}  // namespace P4::Test
