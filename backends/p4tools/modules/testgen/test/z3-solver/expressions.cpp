#include <gtest/gtest.h>

#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "backends/p4tools/common/core/z3_solver.h"
#include "backends/p4tools/common/lib/model.h"
#include "ir/declaration.h"
#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "lib/enumerator.h"
#include "test/gtest/helpers.h"

#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/test/gtest_utils.h"

namespace Test {

using P4Tools::Model;
using P4Tools::Z3Solver;
using P4Tools::P4Testgen::TestgenTarget;
using Value = IR::Literal;

class Z3SolverTests : public ::testing::Test {
 protected:
    Z3SolverTests(const char *condition, const char *equation)
        : condition(condition), equation(equation) {}
    virtual void SetUp() {
        expression = nullptr;
        variableValue = nullptr;
        std::stringstream streamTest;
        streamTest << R"(
    header H {
      bit<4> a; 
      bit<4> b;
      bit<8> c1;
      bit<32> c2;
      bit<32> c3;
      bit<1> c4;
      bool b1;
      bool b2;
    }
    struct Headers {
      H h;
    }
  
    struct Metadata {  }
    parser parse(packet_in pkt,
                out Headers hdr,
                inout Metadata metadata,
                inout standard_metadata_t sm) {
      state start {
          pkt.extract(hdr.h);
          transition accept;
      }
    }
    control mau(inout Headers hdr, inout Metadata meta, inout standard_metadata_t sm) {
      apply {)";
        streamTest << "\n if (" << condition << ")\n " << equation << ";\n";
        streamTest << R"(
      }
    }
    control deparse(packet_out pkt, in Headers hdr) {
      apply {
        pkt.emit(hdr.h);
      }
    }
    control verifyChecksum(inout Headers hdr, inout Metadata meta) {
      apply {}
    }
    control computeChecksum(inout Headers hdr, inout Metadata meta) {
      apply {}
    }
    V1Switch(parse(), verifyChecksum(), mau(), mau(), computeChecksum(), deparse()) main;)";

        auto source = P4_SOURCE(P4Headers::V1MODEL, streamTest.str().c_str());
        const auto test = P4ToolsTestCase::create_16("bmv2", "v1model", source);

        if (!test) {
            return;
        }

        // Produce a ProgramInfo, which is needed to create a SmallStepEvaluator.
        const auto *progInfo = TestgenTarget::initProgram(test->program);
        if (progInfo == nullptr) {
            return;
        }

        // Extract the binary operation from the P4Program
        auto *const declVector = test->program->getDeclsByName("mau")->toVector();
        const auto *decl = (*declVector)[0];
        const auto *control = decl->to<IR::P4Control>();
        for (const auto *st : control->body->components) {
            if (const auto *as = st->to<IR::IfStatement>()) {
                expression = as->condition;
                if (const auto *op = as->ifTrue->to<IR::AssignmentStatement>()) {
                    variableValue = op;
                }
            }
        }
    }
    const IR::Expression *expression = nullptr;
    const IR::AssignmentStatement *variableValue = nullptr;
    std::string condition;
    std::string equation;
};

namespace Z3Test {

void test(const IR::Expression *expression, const IR::AssignmentStatement *variableValue) {
    // checking initial data
    ASSERT_TRUE(expression);
    ASSERT_TRUE(variableValue);

    // Convert members to symbolic variables.
    SymbolicConverter symbolicConverter;
    expression = expression->apply(symbolicConverter);
    variableValue = variableValue->apply(symbolicConverter)->checkedTo<IR::AssignmentStatement>();

    // adding assertion
    Z3Solver solver;

    // checking satisfiability
    ASSERT_EQ(solver.checkSat({expression}), true);

    // getting model
    auto symbolMap = solver.getSymbolicMapping();

    ASSERT_EQ(symbolMap.size(), 2U);

    ASSERT_EQ(symbolMap.count(variableValue->left->checkedTo<IR::SymbolicVariable>()), 1U);

    const auto *value = symbolMap.at(variableValue->left->checkedTo<IR::SymbolicVariable>());

    if (variableValue->right->is<IR::BoolLiteral>()) {
        ASSERT_TRUE(variableValue->right->is<IR::BoolLiteral>());
        const auto *valueP4bool = variableValue->right->to<IR::BoolLiteral>();
        ASSERT_TRUE(value->is<IR::BoolLiteral>());
        const auto *valueZ3 = value->to<IR::BoolLiteral>();
        ASSERT_EQ(valueZ3->value, valueP4bool->value);
    }
    if (variableValue->right->is<IR::Constant>()) {
        ASSERT_TRUE(variableValue->right->is<IR::Constant>());
        const auto *valueP4const = variableValue->right->to<IR::Constant>();
        ASSERT_TRUE(value->is<IR::Constant>());
        const auto *valueZ3 = value->to<IR::Constant>();
        ASSERT_EQ(valueZ3->value, valueP4const->value);
    }
}

class Z3SolverAdd : public Z3SolverTests {
 public:
    Z3SolverAdd()
        : Z3SolverTests("hdr.h.a == 4w15 && hdr.h.b == 4w15 + hdr.h.a", "hdr.h.b = 4w15+4w15") {}
};

TEST_F(Z3SolverAdd, Add) { test(expression, variableValue); }

class Z3SolverSub : public Z3SolverTests {
 public:
    Z3SolverSub()
        : Z3SolverTests("hdr.h.a == 4w1 && hdr.h.b == hdr.h.a - 4w2", "hdr.h.b = 4w1-4w2") {}
};

TEST_F(Z3SolverSub, Sub) { test(expression, variableValue); }

class Z3SolverMult : public Z3SolverTests {
 public:
    Z3SolverMult()
        : Z3SolverTests("hdr.h.a == 4w7 && hdr.h.b == hdr.h.a * 4w7", "hdr.h.b = 4w7 * 4w7") {}
};

TEST_F(Z3SolverMult, Mult) { test(expression, variableValue); }

class Z3SolverBAnd : public Z3SolverTests {
 public:
    Z3SolverBAnd()
        : Z3SolverTests("hdr.h.a == 4w2 && hdr.h.b == hdr.h.a & 4w4", "hdr.h.b = 4w2 & 4w4") {}
};

TEST_F(Z3SolverBAnd, BAnd) { test(expression, variableValue); }

class Z3SolverBOR : public Z3SolverTests {
 public:
    Z3SolverBOR()
        : Z3SolverTests("hdr.h.a == 4w2 && hdr.h.b == hdr.h.a | 4w4", "hdr.h.b = 4w2 | 4w4") {}
};

TEST_F(Z3SolverBOR, BOr) { test(expression, variableValue); }

class Z3SolverBXor : public Z3SolverTests {
 public:
    Z3SolverBXor()
        : Z3SolverTests("hdr.h.a == 4w2 && hdr.h.b == hdr.h.a ^ 4w10", "hdr.h.b = 4w2 ^ 4w10") {}
};

TEST_F(Z3SolverBXor, BXor) { test(expression, variableValue); }

class Z3SolverComplement : public Z3SolverTests {
 public:
    Z3SolverComplement()
        : Z3SolverTests("hdr.h.a == 4w4 && hdr.h.b == ~(hdr.h.a)", "hdr.h.b = ~(4w4)") {}
};

TEST_F(Z3SolverComplement, Complement) { test(expression, variableValue); }

class Z3SolverShl : public Z3SolverTests {
 public:
    Z3SolverShl()
        : Z3SolverTests("hdr.h.a == 4w4 && hdr.h.b == hdr.h.a << 1", "hdr.h.b = 4w4 << 1") {}
};

TEST_F(Z3SolverShl, LeftShift) { test(expression, variableValue); }

class Z3SolverShrU : public Z3SolverTests {
 public:
    Z3SolverShrU()
        : Z3SolverTests("hdr.h.a == 4w15 && hdr.h.b == hdr.h.a >> 1", "hdr.h.b = 4w15 >> 1") {}
};

TEST_F(Z3SolverShrU, RightShiftUnsigned) { test(expression, variableValue); }

class Z3SolverShrS : public Z3SolverTests {
 public:
    Z3SolverShrS()
        : Z3SolverTests("hdr.h.a == -4w1 && hdr.h.b == hdr.h.a >> 1", "hdr.h.b = (-4w1) >> 1") {}
};

TEST_F(Z3SolverShrS, RightShiftSigned) { test(expression, variableValue); }

class Z3SolverMod : public Z3SolverTests {
 public:
    Z3SolverMod()
        : Z3SolverTests("hdr.h.a == 4w4 && hdr.h.b == hdr.h.a % 2", "hdr.h.b = 4w4 % 2") {}
};

TEST_F(Z3SolverMod, Mod) { test(expression, variableValue); }

class Z3SolverConc : public Z3SolverTests {
 public:
    Z3SolverConc()
        : Z3SolverTests("hdr.h.b == 4w15 && hdr.h.a == (hdr.h.b ++ 2w3)[4:1]",
                        "hdr.h.a = 2w3 ++ 2w3") {}
};

TEST_F(Z3SolverConc, Concatenation) { test(expression, variableValue); }

class Z3SolverITE : public Z3SolverTests {
 public:
    Z3SolverITE()
        : Z3SolverTests("hdr.h.a == 4w1 && hdr.h.b == (hdr.h.a>0?4w2:4w0)",
                        "hdr.h.b = (4w1>0?4w2:4w0)") {}
};

TEST_F(Z3SolverITE, ITE) { test(expression, variableValue); }

class Z3SolverCastInAssignment : public Z3SolverTests {
 public:
    Z3SolverCastInAssignment()
        : Z3SolverTests("hdr.h.c1 == 8w1 && hdr.h.c2 == (bit<32>)(hdr.h.c1)", "hdr.h.c2 = 32w1") {}
};

TEST_F(Z3SolverCastInAssignment, Cast) { test(expression, variableValue); }

class Z3SolverCastBit2LessBit : public Z3SolverTests {
 public:
    Z3SolverCastBit2LessBit()
        : Z3SolverTests("hdr.h.c2 == 32w1 && hdr.h.c1 == (bit<8>)(hdr.h.c2)",
                        "hdr.h.c1 = (bit<8>)(32w1)") {}
};

TEST_F(Z3SolverCastBit2LessBit, Cast) { test(expression, variableValue); }

class Z3SolverCastBit2LargerBit : public Z3SolverTests {
 public:
    Z3SolverCastBit2LargerBit()
        : Z3SolverTests("hdr.h.c1 == 8w1 && hdr.h.c2 == (bit<32>)(hdr.h.c1)",
                        "hdr.h.c2 = (bit<32>)(8w1)") {}
};

TEST_F(Z3SolverCastBit2LargerBit, Cast) { test(expression, variableValue); }

class Z3SolverCastBit2EqualBit : public Z3SolverTests {
 public:
    Z3SolverCastBit2EqualBit()
        : Z3SolverTests("hdr.h.c3 == 32w1 && hdr.h.c2 == (bit<32>)(hdr.h.c3)",
                        "hdr.h.c2 = (bit<32>)(32w1)") {}
};

TEST_F(Z3SolverCastBit2EqualBit, Cast) { test(expression, variableValue); }

class Z3SolverCastBool2Bit : public Z3SolverTests {
 public:
    Z3SolverCastBool2Bit()
        : Z3SolverTests("hdr.h.b1 == false && hdr.h.c4 == (bit<1>)(hdr.h.b1)",
                        "hdr.h.c4 = (bit<1>)(false)") {}
};

TEST_F(Z3SolverCastBool2Bit, Cast) { test(expression, variableValue); }

class Z3SolverCastBit2Bool : public Z3SolverTests {
 public:
    Z3SolverCastBit2Bool()
        : Z3SolverTests("hdr.h.c4 == 1w0 && hdr.h.b1 == (bool)(hdr.h.c4)",
                        "hdr.h.b1 = (bool)(1w0)") {}
};

TEST_F(Z3SolverCastBit2Bool, Cast) { test(expression, variableValue); }

}  // namespace Z3Test

}  // namespace Test
