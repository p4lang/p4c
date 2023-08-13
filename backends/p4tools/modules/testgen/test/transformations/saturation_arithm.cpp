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
#include "ir/node.h"
#include "ir/visitor.h"
#include "lib/enumerator.h"
#include "midend/saturationElim.h"
#include "test/gtest/helpers.h"

#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/test/gtest_utils.h"

namespace Test {

using P4::SaturationElim;
using P4Tools::Model;
using P4Tools::Z3Solver;
using P4Tools::P4Testgen::TestgenTarget;

class Z3SolverSatTests : public ::testing::Test {
 protected:
    Z3SolverSatTests(const char *condition, const char *equation)
        : condition(condition), equation(equation) {}
    void SetUp() override {
        expression = nullptr;
        variableValue = nullptr;
        std::stringstream streamTest;
        streamTest << R"(
      header H {
        int<4> ai;
        int<4> bi;
        bit<4> a;
        bit<4> b;
      }
      struct Headers {
        H h;
      }
      struct Metadata { }
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

namespace ArithmTest {

/// The main class for saturation transformation.
class SaturationTransform : public Transform {
 public:
    /// transforms saturation adding
    const IR::Node *postorder(IR::AddSat *add) override { return SaturationElim::eliminate(add); }

    /// transforms saturation substraction
    const IR::Node *postorder(IR::SubSat *sub) override { return SaturationElim::eliminate(sub); }
};

void test(const IR::Expression *expression, const IR::AssignmentStatement *variableValue) {
    // checking initial data
    ASSERT_TRUE(expression);
    ASSERT_TRUE(variableValue);

    // eliminate saturation arithmetics
    SymbolicConverter symbolicConverter;
    expression = expression->apply(symbolicConverter);
    variableValue = variableValue->apply(symbolicConverter)->checkedTo<IR::AssignmentStatement>();

    SaturationTransform transform;
    expression = expression->apply(transform);

    // adding assertion
    Z3Solver solver;

    // checking satisfiability
    ASSERT_EQ(solver.checkSat({expression}), true);

    // getting model
    auto symbolMap = solver.getSymbolicMapping();
    ASSERT_EQ(symbolMap.size(), 2U);

    ASSERT_EQ(symbolMap.count(variableValue->left->checkedTo<IR::SymbolicVariable>()), 1U);

    const auto *value = symbolMap.at(variableValue->left->checkedTo<IR::SymbolicVariable>());

    ASSERT_TRUE(variableValue->right->is<IR::Constant>());
    ASSERT_TRUE(value->is<IR::Constant>());

    const auto *valueP4 = variableValue->right->to<IR::Constant>();
    const auto *valueZ3 = value->to<IR::Constant>();

    ASSERT_EQ(valueZ3->value, valueP4->value);
}

class Z3SolverAddSat01 : public Z3SolverSatTests {
 public:
    Z3SolverAddSat01()
        : Z3SolverSatTests("hdr.h.a == 4w4 && hdr.h.b == 4w14 |+| hdr.h.a",
                           "hdr.h.b = 4w4 |+| 4w14") {}
};

TEST_F(Z3SolverAddSat01, AddSat01) { test(expression, variableValue); }

class Z3SolverAddSat02 : public Z3SolverSatTests {
 public:
    Z3SolverAddSat02()
        : Z3SolverSatTests("hdr.h.a == (bit<4>)4s4 && hdr.h.b == 4w3 |+| hdr.h.a",
                           "hdr.h.b = 4w3 |+| (bit<4>)4s4") {}
};

TEST_F(Z3SolverAddSat02, AddSat02) { test(expression, variableValue); }

class Z3SolverAddSat03 : public Z3SolverSatTests {
 public:
    Z3SolverAddSat03()
        : Z3SolverSatTests("hdr.h.ai == 4s4 && hdr.h.bi == 4s6 |+| hdr.h.ai",
                           "hdr.h.bi = 4s6 |+| 4s4") {}
};

TEST_F(Z3SolverAddSat03, AddSat03) { test(expression, variableValue); }

class Z3SolverAddSat04 : public Z3SolverSatTests {
 public:
    Z3SolverAddSat04()
        : Z3SolverSatTests("hdr.h.ai == -4 && hdr.h.bi == -5 |+| hdr.h.ai",
                           "hdr.h.bi = (int<4>)-5 |+| (int<4>)-4") {}
};

TEST_F(Z3SolverAddSat04, AddSat04) { test(expression, variableValue); }

class Z3SolverAddSat05 : public Z3SolverSatTests {
 public:
    Z3SolverAddSat05()
        : Z3SolverSatTests("hdr.h.ai == -5 && hdr.h.bi == 4 |+| hdr.h.ai",
                           "hdr.h.bi = 4 |+| (int<4>)-5") {}
};

TEST_F(Z3SolverAddSat05, AddSat05) { test(expression, variableValue); }

class Z3SolverSubSat01 : public Z3SolverSatTests {
 public:
    Z3SolverSubSat01()
        : Z3SolverSatTests("hdr.h.a == 4 && hdr.h.b == hdr.h.a |-| 5",
                           "hdr.h.b = (bit<4>)4 |-| 5") {}
};

TEST_F(Z3SolverSubSat01, SubSat01) { test(expression, variableValue); }

class Z3SolverSubSat02 : public Z3SolverSatTests {
 public:
    Z3SolverSubSat02()
        : Z3SolverSatTests("hdr.h.ai == 4 && hdr.h.bi == hdr.h.ai |-| 1",
                           "hdr.h.bi =(int<4>) 4 |-| 1") {}
};

TEST_F(Z3SolverSubSat02, SubSat02) { test(expression, variableValue); }

class Z3SolverSubSat03 : public Z3SolverSatTests {
 public:
    Z3SolverSubSat03()
        : Z3SolverSatTests("hdr.h.ai == 2 && hdr.h.bi == hdr.h.ai |-| 4s3",
                           "hdr.h.bi = (int<4>)2 |-| 4s3") {}
};

TEST_F(Z3SolverSubSat03, SubSat03) { test(expression, variableValue); }

class Z3SolverSubSat04 : public Z3SolverSatTests {
 public:
    Z3SolverSubSat04()
        : Z3SolverSatTests("hdr.h.ai == -4 && hdr.h.bi == hdr.h.ai |-| 5",
                           "hdr.h.bi = (int<4>)-4 |-| 5") {}
};

TEST_F(Z3SolverSubSat04, SubSat04) { test(expression, variableValue); }

class Z3SolverSubSat05 : public Z3SolverSatTests {
 public:
    Z3SolverSubSat05()
        : Z3SolverSatTests("hdr.h.ai == 2 && hdr.h.bi == hdr.h.ai |-| (-7)",
                           "hdr.h.bi = (int<4>)2 |-| (-7)") {}
};

TEST_F(Z3SolverSubSat05, SubSat05) { test(expression, variableValue); }

}  // namespace ArithmTest

}  // namespace Test
