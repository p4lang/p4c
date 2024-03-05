#include "backends/p4tools/modules/testgen/targets/bmv2/expr_stepper.h"

#include <cstddef>
#include <functional>
#include <optional>
#include <ostream>
#include <variant>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/number.hpp>

#include "backends/p4tools/common/lib/arch_spec.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/taint.h"
#include "backends/p4tools/common/lib/trace_event_types.h"
#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/common/lib/variables.h"
#include "ir/declaration.h"
#include "ir/indexed_vector.h"
#include "ir/ir-generated.h"
#include "ir/irutils.h"
#include "ir/node.h"
#include "ir/solver.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/ordered_map.h"

#include "backends/p4tools/modules/testgen/core/externs.h"
#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/expr_stepper.h"
#include "backends/p4tools/modules/testgen/core/small_step/small_step.h"
#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/exceptions.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/packet_vars.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/constants.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/program_info.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/table_stepper.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/test_spec.h"

namespace P4Tools::P4Testgen::Bmv2 {

std::string Bmv2V1ModelExprStepper::getClassName() { return "Bmv2V1ModelExprStepper"; }

bool Bmv2V1ModelExprStepper::isPartOfFieldList(const IR::StructField *field,
                                               uint64_t recirculateIndex) {
    // Check whether the field has a "field_list" annotation associated with it.
    const auto *annotation = field->getAnnotation("field_list");
    if (annotation != nullptr) {
        // Grab the index of the annotation.
        auto annoExprs = annotation->expr;
        auto annoExprSize = annoExprs.size();
        BUG_CHECK(annoExprSize == 1,
                  "The field list annotation should only have one member. Has %1%.", annoExprSize);
        auto annoVal = annoExprs.at(0)->checkedTo<IR::Constant>()->asUint64();
        // If the indices match of this particular annotation, skip resetting.
        if (annoVal == recirculateIndex) {
            return true;
        }
    }
    return false;
}

void Bmv2V1ModelExprStepper::resetPreservingFieldList(ExecutionState &nextState,
                                                      const IR::PathExpression *ref,
                                                      uint64_t recirculateIndex) const {
    const auto *ts = ref->type->checkedTo<IR::Type_StructLike>();
    for (const auto *field : ts->fields) {
        // Check whether the field has a "field_list" annotation associated with it.
        if (isPartOfFieldList(field, recirculateIndex)) {
            continue;
        }
        // If there is no annotation, reset the user metadata.
        const auto *fieldType = nextState.resolveType(field->type);
        auto *fieldLabel = new IR::Member(fieldType, ref, field->name);
        // Reset the variable.
        setTargetUninitialized(nextState, fieldLabel, false);
    }
}

void Bmv2V1ModelExprStepper::processClone(const ExecutionState &state,
                                          SmallStepEvaluator::Result &result) {
    const auto *progInfo = getProgramInfo().checkedTo<Bmv2V1ModelProgramInfo>();
    // Pick a clone port var. It must adhere to the constraints of the target.
    const auto *cloneInfo = state.getTestObject<Bmv2V1ModelCloneInfo>("clone_infos", "clone_info");
    const auto *sessionIdExpr = cloneInfo->getSessionId();
    const auto &preserveIndex = cloneInfo->getPreserveIndex();
    const auto &egressPortVar = programInfo.getTargetOutputPortVar();
    const auto &clonePortVar =
        ToolsVariables::getSymbolicVariable(egressPortVar->type, "clone_port_var");

    uint64_t recirculateCount = 0;
    if (state.hasProperty("recirculate_count")) {
        recirculateCount = state.getProperty<uint64_t>("recirculate_count");
    }

    // Pick a clone port var. It must adhere to the constraints of the target.
    // Pick a value that is not equal to the normal egress port.
    // TODO: Is this a reasonable restriction? Do we want to test this case?
    // TODO: What about programs where we can not satisfy this restriction? Do we not generate
    // tests?
    // TODO: Circle back to this once we have modifiable state in extern steppers.
    const Constraint *cond = new IR::Neq(egressPortVar, clonePortVar);
    // PTF has a couple more restrictions because it uses P4Runtime. The session ID must be within
    // a specific range. Otherwise no clone call can be produced.
    if (TestgenOptions::get().testBackend == "PTF") {
        cond = new IR::LAnd(
            new IR::LAnd(
                new IR::Leq(sessionIdExpr, IR::getConstant(sessionIdExpr->type,
                                                           BMv2Constants::CLONE_SESSION_ID_MAX)),
                new IR::Geq(sessionIdExpr, IR::getConstant(sessionIdExpr->type,
                                                           BMv2Constants::CLONE_SESSION_ID_MIN))),
            new IR::LAnd(cond, new IR::Lss(clonePortVar, IR::getConstant(clonePortVar->type, 8))));
    }
    // clone methods have a default state where the packet continues as is.
    {
        auto &defaultState = state.clone();
        // Delete the stale clone info to free up some space and reset the clone_active flag.
        defaultState.deleteTestObjectCategory("clone_infos");
        defaultState.setProperty("clone_active", false);
        // Increment the recirculation count.
        defaultState.setProperty("recirculate_count", ++recirculateCount);
        // Attach the clone specification for test generation.
        const auto *defaultCloneInfo = new Bmv2V1ModelCloneSpec(sessionIdExpr, clonePortVar, false);
        defaultState.addTestObject("clone_specs", "clone_spec", defaultCloneInfo);
        defaultState.popBody();
        result->emplace_back(cond, state, defaultState);
    }

    // Use a nullptr state to avoid some code duplication...
    ExecutionState *cloneState = nullptr;
    std::vector<Continuation::Command> cmds;

    // We need to set the instance type once we recirculate.
    const auto *instanceBitType = IR::getBitType(32);
    const auto *instanceTypeVar = new IR::Member(
        instanceBitType, new IR::PathExpression("*standard_metadata"), "instance_type");

    auto cloneType = cloneInfo->getCloneType();
    if (cloneType == BMv2Constants::CloneType::I2E) {
        cloneState = &cloneInfo->getClonedState().clone();
        // Set the metadata instance type.
        // It is important to do this here at the beginning because we use copyIn later.
        // This value will be copied into the local metadata first, then copied back out with the
        // exit statement.
        cloneState->set(
            instanceTypeVar,
            IR::getConstant(instanceBitType, BMv2Constants::PKT_INSTANCE_TYPE_INGRESS_CLONE));
        const auto *progInfo = getProgramInfo().checkedTo<Bmv2V1ModelProgramInfo>();
        // Reset the packet buffer, which corresponds to the output packet.
        // We need to reset everything to the state before the ingress call. We use a
        // trick by calling copyIn on the entire state again. We need a little bit of
        // information for that, including the exact parameter names of the ingress
        // block we are in. Just grab the ingress from the programmable blocks.
        const auto *programmableBlocks = progInfo->getProgrammableBlocks();
        const auto *typeDecl = programmableBlocks->at("Ingress");
        const auto *applyBlock = typeDecl->checkedTo<IR::P4Control>();
        const auto *params = applyBlock->getApplyParameters();
        auto blockIndex = 2;
        const auto *archMember = progInfo->getArchSpec().getArchMember(blockIndex);
        for (size_t paramIdx = 0; paramIdx < params->size(); ++paramIdx) {
            const auto *param = params->getParameter(paramIdx);
            const auto &archRef = archMember->blockParams.at(paramIdx);
            // If there is a preservation index present skip the second parameter (metadata) and do
            // not reset it using copyIn.
            if (preserveIndex.has_value() && paramIdx == 1) {
                // This program segment resets the user metadata of the v1model
                // program to 0. However, fields in the user metadata that have the
                // field_list annotation and the appropriate index will not be
                // reset. The user metadata is the second parameter of the ingress
                // control.
                const auto *paramType = param->type;
                if (const auto *tn = paramType->to<IR::Type_Name>()) {
                    paramType = cloneState->resolveType(tn);
                }
                const auto *paramRef = new IR::PathExpression(paramType, new IR::Path(param->name));
                resetPreservingFieldList(*cloneState, paramRef, preserveIndex.value());
                continue;
            }
            cloneState->copyIn(TestgenTarget::get(), param, archRef);
        }
        // We then exit, which will copy out all the state that we have just reset.
        cmds.emplace_back(new IR::ExitStatement());
    } else if (cloneType == BMv2Constants::CloneType::E2E) {
        cloneState = &state.clone();
        // Set the metadata instance type.
        cloneState->set(
            instanceTypeVar,
            IR::getConstant(instanceBitType, BMv2Constants::PKT_INSTANCE_TYPE_EGRESS_CLONE));
        if (preserveIndex.has_value()) {
            // This program segment resets the user metadata of the v1model program to
            // 0. However, fields in the user metadata that have the field_list
            // annotation and the appropriate index will not be reset. The user
            // metadata is the third parameter of the parser control.
            const auto *paramPath = progInfo->getBlockParam("Parser", 2);
            resetPreservingFieldList(*cloneState, paramPath, preserveIndex.value());
        }

        // In the other state, we start processing from the egress.
        const auto *topLevelBlocks = progInfo->getPipelineSequence();
        size_t egressDelim = 0;
        for (; egressDelim < topLevelBlocks->size(); ++egressDelim) {
            auto block = topLevelBlocks->at(egressDelim);
            if (!std::holds_alternative<const IR::Node *>(block)) {
                continue;
            }
            const auto *p4Node = std::get<const IR::Node *>(block);
            if (const auto *ctrl = p4Node->to<IR::P4Control>()) {
                if (progInfo->getGress(ctrl) == BMV2_EGRESS) {
                    break;
                }
            }
        }
        cmds.insert(cmds.begin(), topLevelBlocks->begin() + egressDelim - 2, topLevelBlocks->end());
    } else {
        TESTGEN_UNIMPLEMENTED("Unsupported clone type %1%.", cloneType);
    }
    // Attach the clone specification for test generation.
    cloneState->addTestObject("clone_specs", "clone_spec",
                              new Bmv2V1ModelCloneSpec(sessionIdExpr, clonePortVar, true));
    // Delete the stale clone info to free up some space and reset the clone_active flag.
    cloneState->setProperty("clone_active", false);
    cloneState->deleteTestObjectCategory("clone_infos");
    // Increment the recirculation count.
    cloneState->setProperty("recirculate_count", ++recirculateCount);
    /// Reset the packet buffer for recirculation.
    cloneState->resetPacketBuffer();
    cloneState->replaceTopBody(&cmds);
    result->emplace_back(cond, state, *cloneState);
}

void Bmv2V1ModelExprStepper::processRecirculate(const ExecutionState &state,
                                                SmallStepEvaluator::Result &result) {
    auto instanceType = state.getProperty<uint64_t>("recirculate_instance_type");
    const auto *progInfo = getProgramInfo().checkedTo<Bmv2V1ModelProgramInfo>();
    auto &recState = state.clone();

    // Check whether the packet needs to be reset.
    // If that is the case, reset the packet buffer to the calculated input packet.
    auto recirculateReset = state.hasProperty("recirculate_reset_pkt");
    if (recirculateReset) {
        // Reset the packet buffer, which corresponds to the output packet.
        recState.resetPacketBuffer();
        // Set the packet buffer to the current calculated program packet for consistency.
        recState.appendToPacketBuffer(recState.getInputPacket());
    }

    // We need to update the size of the packet when recirculating. Do not forget to divide
    // by 8.
    const auto *packetSizeVar =
        new IR::Member(&PacketVars::PACKET_SIZE_VAR_TYPE,
                       new IR::PathExpression("*standard_metadata"), "packet_length");
    const auto *packetSizeConst =
        IR::getConstant(&PacketVars::PACKET_SIZE_VAR_TYPE, recState.getPacketBufferSize() / 8);
    recState.set(packetSizeVar, packetSizeConst);

    if (recState.hasProperty("recirculate_index")) {
        // Get the index set by the recirculate/resubmit function. Will fail if no index is
        // set.
        auto recirculateIndex = recState.getProperty<uint64_t>("recirculate_index");
        // This program segment resets the user metadata of the v1model program to 0.
        // However, fields in the user metadata that have the field_list annotation and the
        // appropriate index will not be reset.
        // The user metadata is the third parameter of the parser control.
        const auto *paramPath = progInfo->getBlockParam("Parser", 2);
        resetPreservingFieldList(recState, paramPath, recirculateIndex);
    }

    // Update the metadata variable to the correct instance type as provided by recirculation.
    const auto *bitType = IR::getBitType(32);
    const auto *instanceTypeVar =
        new IR::Member(bitType, new IR::PathExpression("*standard_metadata"), "instance_type");
    recState.set(instanceTypeVar, IR::getConstant(bitType, instanceType));

    // Set recirculate to false to avoid infinite loops.
    recState.setProperty("recirculate_active", false);

    // "Recirculate" by attaching the sequence again.
    // Does NOT initialize state or adds new conditions.
    const auto *topLevelBlocks = progInfo->getPipelineSequence();
    recState.replaceTopBody(topLevelBlocks);
    result->emplace_back(recState);
}

Bmv2V1ModelExprStepper::Bmv2V1ModelExprStepper(ExecutionState &state, AbstractSolver &solver,
                                               const ProgramInfo &programInfo)
    : ExprStepper(state, solver, programInfo) {}

void Bmv2V1ModelExprStepper::evalExternMethodCall(const IR::MethodCallExpression *call,
                                                  const IR::Expression *receiver, IR::ID name,
                                                  const IR::Vector<IR::Argument> *args,
                                                  ExecutionState &state) {
    const ExternMethodImpls::MethodImpl assertAssumeExecute =
        [](const IR::MethodCallExpression * /*call*/, const IR::Expression * /*receiver*/,
           IR::ID &methodName, const IR::Vector<IR::Argument> *args, const ExecutionState &state,
           SmallStepEvaluator::Result &result) {
            const auto *cond = args->at(0)->expression;

            // If the assert/assume condition is tainted, we do not know whether we abort.
            if (Taint::hasTaint(cond)) {
                TESTGEN_UNIMPLEMENTED(
                    "Assert/assume can not be executed under a tainted condition.");
            }
            auto upCasename = methodName.name.toUpper();
            // Record the condition we evaluate as string.
            std::stringstream condStream;
            condStream << upCasename << " Condition: ";
            cond->dbprint(condStream);
            // Handle the case where the condition is true.
            {
                auto &nextState = state.clone();
                nextState.popBody();
                nextState.add(*new TraceEvents::Generic(upCasename + ": true condition "));
                nextState.add(*new TraceEvents::Generic(condStream.str()));
                result->emplace_back(cond, state, nextState);
            }
            // Handle the case where the condition is false.
            {
                auto &falseState = state.clone();
                falseState.add(*new TraceEvents::Generic(upCasename + ": false condition"));
                falseState.add(*new TraceEvents::Generic(condStream.str()));
                falseState.replaceTopBody(Continuation::Exception::Abort);
                result->emplace_back(new IR::LNot(IR::Type::Boolean::get(), cond), state,
                                     falseState);
            }
        };

    // Provides implementations of BMv2 externs.
    static const ExternMethodImpls EXTERN_METHOD_IMPLS({
        /* ======================================================================================
         *  mark_to_drop
         *  Mark to drop sets the BMv2 internal drop variable to true.
         * ======================================================================================
         */
        // TODO: Implement extern path expression calls.
        {"*method.mark_to_drop",
         {"standard_metadata"},
         [](const IR::MethodCallExpression * /*call*/, const IR::Expression * /*receiver*/,
            IR::ID & /*methodName*/, const IR::Vector<IR::Argument> *args,
            const ExecutionState &state, SmallStepEvaluator::Result &result) {
             auto &nextState = state.clone();
             const auto *nineBitType = IR::getBitType(BMv2Constants::PORT_BIT_WIDTH);
             const auto *metadataLabel = args->at(0)->expression->checkedTo<IR::InOutReference>();
             // Use an assignment to set egress_spec to true.
             // This variable will be processed in the deparser.
             const auto *portVar = new IR::Member(nineBitType, metadataLabel->ref, "egress_spec");
             nextState.set(portVar, IR::getConstant(nineBitType, BMv2Constants::DROP_PORT));
             nextState.add(*new TraceEvents::Generic("mark_to_drop executed."));
             nextState.popBody();
             result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  random
         *  Generate a random number in the range lo..hi, inclusive, and write
         *  it to the result parameter.
         * ======================================================================================
         */
        {"*method.random",
         {"result", "lo", "hi"},
         [this](const IR::MethodCallExpression * /*call*/, const IR::Expression * /*receiver*/,
                IR::ID & /*methodName*/, const IR::Vector<IR::Argument> *args,
                const ExecutionState &state, SmallStepEvaluator::Result &result) {
             BUG_CHECK(args->at(1)->expression->to<IR::Constant>(), "Expected a constant.");
             BUG_CHECK(args->at(2)->expression->to<IR::Constant>(), "Expected a constant.");
             const auto *lo = args->at(1)->expression->to<IR::Constant>();
             const auto *hi = args->at(2)->expression->to<IR::Constant>();
             BUG_CHECK(lo->value <= hi->value,
                       "Low value ( %1% ) must be less than high value ( %2% ).", lo, hi);
             auto &nextState = state.clone();
             const auto *resultField = args->at(0)->expression;
             const auto &fieldRef = ToolsVariables::convertReference(resultField);

             // If the range is limited to only one value, return that value.
             if (lo->value == hi->value) {
                 nextState.set(fieldRef, hi);
                 nextState.popBody();
                 result->emplace_back(nextState);
                 return false;
             }
             // Otherwise, we will have to return taint,
             // as we do not control the random generator of the device under test.
             if (resultField->type->is<IR::Type_Bits>()) {
                 nextState.set(fieldRef,
                               programInfo.createTargetUninitialized(resultField->type, true));
                 nextState.popBody();
                 result->emplace_back(nextState);
                 return false;
             }
             BUG("Not a Type_Bits: %1%", resultField->type);
         }},
        /* ======================================================================================
         *  assume
         *  For the purposes of compiling and executing P4 programs on a target
         *  device, assert and assume are identical, including the use of the
         *  --ndebug p4c option to elide them.  See documentation for assert.
         *  The reason that assume exists as a separate function from assert is
         *  because they are expected to be used differently by formal
         *  verification tools.  For some formal tools, the goal is to try to
         *  find example packets and sets of installed table entries that cause
         *  an assert statement condition to be false.
         *  Suppose you run such a tool on your program, and the example packet
         *  given is an MPLS packet, i.e. hdr.ethernet.etherType == 0x8847.
         *  You look at the example, and indeed it does cause an assert
         *  condition to be false.  However, your plan is to deploy your P4
         *  program in a network in places where no MPLS packets can occur.
         *  You could add extra conditions to your P4 program to handle the
         *  processing of such a packet cleanly, without assertions failing,
         *  but you would prefer to tell the tool "such example packets are not
         *  applicable in my scenario -- never show them to me".  By adding a
         *  statement:
         *      assume(hdr.ethernet.etherType != 0x8847);
         *  at an appropriate place in your program, the formal tool should
         *  never show you such examples -- only ones that make all such assume
         *  conditions true.
         *  The reason that assume statements behave the same as assert
         *  statements when compiled to a target device is that if the
         *  condition ever evaluates to false when operating in a network, it
         *  is likely that your assumption was wrong, and should be reexamined.
         * ======================================================================================
         */
        {"*method.assume", {"check"}, assertAssumeExecute},
        /* ======================================================================================
         *  assert
         *  Calling assert when the argument is true has no effect, except any
         *  effect that might occur due to evaluation of the argument (but see
         *  below).  If the argument is false, the precise behavior is
         *  target-specific, but the intent is to record or log which assert
         *  statement failed, and optionally other information about the
         *  failure.
         *  For example, on the simple_switch target, executing an assert
         *  statement with a false argument causes a log message with the file
         *  name and line number of the assert statement to be printed, and
         *  then the simple_switch process exits.
         *  If you provide the --ndebug command line option to p4c when
         *  compiling, the compiled program behaves as if all assert statements
         *  were not present in the source code.
         *  We strongly recommend that you avoid using expressions as an
         *  argument to an assert call that can have side effects, e.g. an
         *  extern method or function call that has side effects.  p4c will
         *  allow you to do this with no warning given.  We recommend this
         *  because, if you follow this advice, your program will behave the
         *  same way when assert statements are removed.
         * ======================================================================================
         */
        {"*method.assert", {"check"}, assertAssumeExecute},
        /* ======================================================================================
         *  log_msg
         *  Log user defined messages
         *  Example: log_msg("User defined message");
         *  or log_msg("Value1 = {}, Value2 = {}",{value1, value2});
         * ======================================================================================
         */
        {"*method.log_msg",
         {"msg", "args"},
         [](const IR::MethodCallExpression * /*call*/, const IR::Expression * /*receiver*/,
            IR::ID & /*methodName*/, const IR::Vector<IR::Argument> *args,
            const ExecutionState &state, SmallStepEvaluator::Result &result) {
             auto msg = args->at(0)->expression->checkedTo<IR::StringLiteral>()->value;
             auto value = args->at(1)->expression;
             std::stringstream assignStream;
             assignStream << msg << ": ";

             // Strip any newlines in the value we want to record.
             value->dbprint(assignStream);
             auto assignString = assignStream.str();
             assignString.erase(std::remove(assignString.begin(), assignString.end(), '\n'),
                                assignString.cend());

             auto &nextState = state.clone();
             nextState.add(*new TraceEvents::Generic(assignString.c_str()));
             nextState.popBody();
             result->emplace_back(nextState);
         }},
        {"*method.log_msg",
         {"msg"},
         [](const IR::MethodCallExpression * /*call*/, const IR::Expression * /*receiver*/,
            IR::ID & /*methodName*/, const IR::Vector<IR::Argument> *args,
            const ExecutionState &state, SmallStepEvaluator::Result &result) {
             auto msg = args->at(0)->expression->checkedTo<IR::StringLiteral>()->value;
             auto &nextState = state.clone();
             nextState.add(*new TraceEvents::Generic(msg));
             nextState.popBody();
             result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  hash
         *  Calculate a hash function of the value specified by the data
         *  parameter.  The value written to the out parameter named result
         *  will always be in the range [base, base+max-1] inclusive, if max >=
         *  1.  If max=0, the value written to result will always be base.
         *  Note that the types of all of the parameters may be the same as, or
         *  different from, each other, and thus their bit widths are allowed
         *  to be different.
         *  @param O          Must be a type bit<W>
         *  @param D          Must be a tuple type where all the fields are bit-fields (type
         * bit<W> or int<W>) or varbits.
         *  @param T          Must be a type bit<W>
         *  @param M          Must be a type bit<W>
         * ======================================================================================
         */
        {"*method.hash",
         {"result", "algo", "base", "data", "max"},
         [this](const IR::MethodCallExpression *call, const IR::Expression *receiver, IR::ID &name,
                const IR::Vector<IR::Argument> *args, const ExecutionState &state,
                SmallStepEvaluator::Result &result) {
             bool argsAreTainted = false;
             // If any of the input arguments is tainted, the entire extern is unreliable.
             for (size_t idx = 1; idx < args->size(); ++idx) {
                 const auto *arg = args->at(idx);
                 argsAreTainted = argsAreTainted || Taint::hasTaint(arg->expression);
             }
             const auto *hashOutput = args->at(0)->expression;

             const auto *declInstance = state.findDecl(new IR::PathExpression(name));
             IR::IndexedVector<IR::Node> decls({declInstance->checkedTo<IR::Declaration>()});

             // TODO: Find a better way to classify identifiers.
             // We should be using a new IR type instead.
             // This type is defined in an ir.def file.
             auto externName = receiver->toString() + "_" + declInstance->controlPlaneName();
             auto &nextState = state.clone();
             if (hashOutput->type->is<IR::Type_Bits>()) {
                 const auto &fieldRef = ToolsVariables::convertReference(hashOutput);
                 if (argsAreTainted) {
                     nextState.set(fieldRef,
                                   programInfo.createTargetUninitialized(fieldRef->type, false));
                 } else {
                     const auto *concolicVar = new IR::ConcolicVariable(
                         fieldRef->type, externName, args, call->clone_id, 0, decls);
                     nextState.set(fieldRef, concolicVar);
                 }
             } else {
                 TESTGEN_UNIMPLEMENTED("Hash output %1% of type %2% not supported", hashOutput,
                                       hashOutput->type);
             }

             nextState.popBody();
             result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  register.read
         *
         *  read() reads the state of the register array stored at the
         *  specified index, and returns it as the value written to the
         *  result parameter.
         *
         *  @param index The index of the register array element to be
         *               read, normally a value in the range [0, size-1].
         *  @param result Only types T that are bit<W> are currently
         *               supported.  When index is in range, the value of
         *               result becomes the value read from the register
         *               array element.  When index >= size, the final
         *               value of result is not specified, and should be
         *               ignored by the caller.
         *
         * ======================================================================================
         */
        {"register.read",
         {"result", "index"},
         [this](const IR::MethodCallExpression * /*call*/, const IR::Expression *receiver,
                IR::ID & /*methodName*/, const IR::Vector<IR::Argument> *args,
                const ExecutionState &state, SmallStepEvaluator::Result &result) {
             const auto *readOutput = args->at(0)->expression;
             const auto *index = args->at(1)->expression;
             auto &nextState = state.clone();

             std::vector<Continuation::Command> replacements;
             const auto &externRef = receiver->checkedTo<IR::PathExpression>();
             const auto &externInstance = state.findDecl(externRef);
             // Retrieve the register state from the object store. If it is already present, just
             // cast the object to the correct class and retrieve the current value according to the
             // index. If the register has not been added yet, create a new register object.
             const auto *registerState =
                 state.getTestObject("registervalues", externInstance->controlPlaneName(), false);
             const Bmv2V1ModelRegisterValue *registerValue = nullptr;
             if (registerState != nullptr) {
                 registerValue = registerState->checkedTo<Bmv2V1ModelRegisterValue>();
             } else {
                 const auto *inputValue =
                     programInfo.createTargetUninitialized(readOutput->type, false);
                 registerValue = new Bmv2V1ModelRegisterValue(inputValue);
                 nextState.addTestObject("registervalues", externInstance->controlPlaneName(),
                                         registerValue);
             }
             const IR::Expression *baseExpr = registerValue->getValueAtIndex(index);

             if (readOutput->type->is<IR::Type_Bits>()) {
                 // We need an assignment statement (and the inefficient copy) here because we
                 // need to immediately resolve the generated mux into multiple branches. This
                 // is only possible because registers do not return a value.
                 replacements.emplace_back(new IR::AssignmentStatement(readOutput, baseExpr));

             } else {
                 TESTGEN_UNIMPLEMENTED("Read extern output %1% of type %2% not supported",
                                       readOutput, readOutput->type);
             }
             // TODO: Find a better way to model a trace of this event.
             std::stringstream registerStream;
             registerStream << "RegisterRead: Index ";
             index->dbprint(registerStream);
             registerStream << " into field ";
             readOutput->dbprint(registerStream);
             nextState.add(*new TraceEvents::Generic(registerStream.str()));
             nextState.replaceTopBody(&replacements);
             result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  register.write
         *
         *  write() writes the state of the register array at the specified
         *  index, with the value provided by the value parameter.
         *
         *  If you wish to perform a read() followed later by a write() to
         *  the same register array element, and you wish the
         *  read-modify-write sequence to be atomic relative to other
         *  processed packets, then there may be parallel implementations
         *  of the v1model architecture for which you must execute them in
         *  a P4_16 block annotated with an @atomic annotation.  See the
         *  P4_16 language specification description of the @atomic
         *  annotation for more details.
         *
         *  @param index The index of the register array element to be
         *               written, normally a value in the range [0,
         *               size-1].  If index >= size, no register state will
         *               be updated.
         *  @param value Only types T that are bit<W> are currently
         *               supported.  When index is in range, this
         *               parameter's value is written into the register
         *               array element specified by index.
         *
         * ======================================================================================
         */
        {"register.write",
         {"index", "value"},
         [this](const IR::MethodCallExpression * /*call*/, const IR::Expression *receiver,
                IR::ID & /*methodName*/, const IR::Vector<IR::Argument> *args,
                const ExecutionState &state, SmallStepEvaluator::Result &result) {
             const auto *index = args->at(0)->expression;
             const auto *inputValue = args->at(1)->expression;
             if (!(inputValue->type->is<IR::Type_InfInt>() ||
                   inputValue->type->is<IR::Type_Bits>())) {
                 TESTGEN_UNIMPLEMENTED(
                     "Only registers with bit or int types are currently supported for "
                     "v1model.");
             }

             const auto *receiverPath = receiver->checkedTo<IR::PathExpression>();
             const auto &externInstance = state.findDecl(receiverPath);
             auto &nextState = state.clone();
             // TODO: Find a better way to model a trace of this event.
             std::stringstream registerStream;
             registerStream << "RegisterWrite: Value ";
             inputValue->dbprint(registerStream);
             registerStream << " into index ";
             index->dbprint(registerStream);
             nextState.add(*new TraceEvents::Generic(registerStream.str()));

             // "Write" to the register by update the internal test object state. If the register
             // did not exist previously, update it with the value to write as initial value.
             const auto *registerState = nextState.getTestObject(
                 "registervalues", externInstance->controlPlaneName(), false);
             Bmv2V1ModelRegisterValue *registerValue = nullptr;
             if (registerState != nullptr) {
                 registerValue = new Bmv2V1ModelRegisterValue(
                     *registerState->checkedTo<Bmv2V1ModelRegisterValue>());
                 registerValue->writeToIndex(index, inputValue);
             } else {
                 const auto &writeValue =
                     programInfo.createTargetUninitialized(inputValue->type, false);
                 registerValue = new Bmv2V1ModelRegisterValue(writeValue);
                 registerValue->writeToIndex(index, inputValue);
             }
             nextState.addTestObject("registervalues", externInstance->controlPlaneName(),
                                     registerValue);
             nextState.popBody();
             result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  counter.count
         *  A counter object is created by calling its constructor.  This
         *  creates an array of counter states, with the number of counter
         *  states specified by the size parameter.  The array indices are
         *  in the range [0, size-1].
         *
         *  You must provide a choice of whether to maintain only a packet
         *  count (CounterType.packets), only a byte count
         *  (CounterType.bytes), or both (CounterType.packets_and_bytes).
         *
         *  Counters can be updated from your P4 program, but can only be
         *  read from the control plane.  If you need something that can be
         *  both read and written from the P4 program, consider using a
         *  register.
         *  count() causes the counter state with the specified index to be
         *  read, modified, and written back, atomically relative to the
         *  processing of other packets, updating the packet count, byte
         *  count, or both, depending upon the CounterType of the counter
         *  instance used when it was constructed.
         *
         *  @param index The index of the counter state in the array to be
         *               updated, normally a value in the range [0,
         *               size-1].  If index >= size, no counter state will be
         *               updated.
         * ======================================================================================
         */
        // TODO: Count currently has no effect in the symbolic interpreter.
        {"counter.count",
         {"index"},
         [](const IR::MethodCallExpression * /*call*/, const IR::Expression * /*receiver*/,
            IR::ID & /*methodName*/, const IR::Vector<IR::Argument> * /*args*/,
            const ExecutionState &state, SmallStepEvaluator::Result &result) {
             auto &nextState = state.clone();
             nextState.popBody();
             result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  direct_counter.count
         *  A direct_counter object is created by calling its constructor.
         *  You must provide a choice of whether to maintain only a packet
         *  count (CounterType.packets), only a byte count
         *  (CounterType.bytes), or both (CounterType.packets_and_bytes).
         *  After constructing the object, you can associate it with at
         *  most one table, by adding the following table property to the
         *  definition of that table:
         *
         *      counters = <object_name>;
         *
         *  Counters can be updated from your P4 program, but can only be
         *  read from the control plane.  If you need something that can be
         *  both read and written from the P4 program, consider using a
         *  register.
         *  The count() method is actually unnecessary in the v1model
         *  architecture.  This is because after a direct_counter object
         *  has been associated with a table as described in the
         *  documentation for the direct_counter constructor, every time
         *  the table is applied and a table entry is matched, the counter
         *  state associated with the matching entry is read, modified, and
         *  written back, atomically relative to the processing of other
         *  packets, regardless of whether the count() method is called in
         *  the body of that action.
         * ======================================================================================
         */
        // TODO: Count currently has no effect in the symbolic interpreter.
        {"direct_counter.count",
         {},
         [](const IR::MethodCallExpression * /*call*/, const IR::Expression * /*receiver*/,
            IR::ID & /*methodName*/, const IR::Vector<IR::Argument> * /*args*/,
            const ExecutionState &state, SmallStepEvaluator::Result &result) {
             auto &nextState = state.clone();
             nextState.popBody();
             result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  meter.execute_meter
         *  A meter object is created by calling its constructor.  This
         *  creates an array of meter states, with the number of meter
         *  states specified by the size parameter.  The array indices are
         *  in the range [0, size-1].  For example, if in your system you
         *  have 128 different "flows" numbered from 0 up to 127, and you
         *  want to meter each of those flows independently of each other,
         *  you could do so by creating a meter object with size=128.
         *
         *  You must provide a choice of whether to meter based on the
         *  number of packets, regardless of their size
         *  (MeterType.packets), or based upon the number of bytes the
         *  packets contain (MeterType.bytes).
         *  execute_meter() causes the meter state with the specified index
         *  to be read, modified, and written back, atomically relative to
         *  the processing of other packets, and an integer encoding of one
         *  of the colors green, yellow, or red to be written to the result
         *  out parameter.
         *  @param index The index of the meter state in the array to be
         *               updated, normally a value in the range [0,
         *               size-1].  If index >= size, no meter state will be
         *               updated.
         *  @param result Type T must be bit<W> with W >= 2.  When index is
         *               in range, the value of result will be assigned 0
         *               for color GREEN, 1 for color YELLOW, and 2 for
         *               color RED (see RFC 2697 and RFC 2698 for the
         *               meaning of these colors).  When index is out of
         *               range, the final value of result is not specified,
         *  and should be ignored by the caller.
         * ====================================================================================== */
        {"meter.execute_meter",
         {"index", "result"},
         [](const IR::MethodCallExpression *call, const IR::Expression *receiver,
            IR::ID & /*methodName*/, const IR::Vector<IR::Argument> *args,
            const ExecutionState &state, SmallStepEvaluator::Result &result) {
             auto testBackend = TestgenOptions::get().testBackend;
             const IR::StateVariable &meterResult =
                 ToolsVariables::convertReference(args->at(1)->expression);
             if (testBackend != "PTF") {
                 ::warning(
                     "meter.execute_meter not implemented for %1%. Choosing default value (GREEN).",
                     testBackend);
                 auto &nextState = state.clone();
                 nextState.set(
                     meterResult,
                     IR::getConstant(meterResult->type,
                                     static_cast<big_int>(BMv2Constants::METER_COLOR::GREEN)));
                 nextState.popBody();
                 result->emplace_back(nextState);
                 return;
             }

             auto &nextState = state.clone();
             std::vector<Continuation::Command> replacements;

             const auto *index = args->at(0)->expression;
             const auto *receiverPath = receiver->checkedTo<IR::PathExpression>();
             const auto &externInstance = nextState.findDecl(receiverPath);

             // Retrieve the meter state from the object store. If it is already present, just
             // cast the object to the correct class and retrieve the current value according to the
             // index. If the meter has not been added had, create a new meter object.
             const auto *meterState =
                 state.getTestObject("meter_values", externInstance->controlPlaneName(), false);
             Bmv2V1ModelMeterValue *meterValue = nullptr;
             const auto &inputValue = ToolsVariables::getSymbolicVariable(
                 meterResult->type, "meter_value" + std::to_string(call->clone_id));
             // Make sure we do not accidentally get "3" as enum assignment...
             auto *cond = new IR::Lss(inputValue, IR::getConstant(meterResult->type, 3));
             if (meterState != nullptr) {
                 meterValue =
                     new Bmv2V1ModelMeterValue(*meterState->checkedTo<Bmv2V1ModelMeterValue>());
             } else {
                 meterValue = new Bmv2V1ModelMeterValue(inputValue, false);
             }
             meterValue->writeToIndex(index, inputValue);
             nextState.addTestObject("meter_values", externInstance->controlPlaneName(),
                                     meterValue);

             if (meterResult->type->is<IR::Type_Bits>()) {
                 // We need an assignment statement (and the inefficient copy) here because we need
                 // to immediately resolve the generated mux into multiple branches.
                 // This is only possible because meters do not return a value.
                 replacements.emplace_back(new IR::AssignmentStatement(meterResult, inputValue));
             } else {
                 TESTGEN_UNIMPLEMENTED("Read extern output %1% of type %2% not supported",
                                       meterResult, meterResult->type);
             }
             // TODO: Find a better way to model a trace of this event.
             std::stringstream meterStream;
             meterStream << "MeterExecute: Index ";
             index->dbprint(meterStream);
             meterStream << " into field ";
             meterResult->dbprint(meterStream);
             nextState.add(*new TraceEvents::Generic(meterStream.str()));
             nextState.replaceTopBody(&replacements);
             result->emplace_back(cond, state, nextState);
         }},
        /* ======================================================================================
         *  direct_meter.read
         *  A direct_meter object is created by calling its constructor.
         *  You must provide a choice of whether to meter based on the
         *  number of packets, regardless of their size
         *  (MeterType.packets), or based upon the number of bytes the
         *  packets contain (MeterType.bytes).  After constructing the
         *  object, you can associate it with at most one table, by adding
         *  the following table property to the definition of that table:
         *
         *      meters = <object_name>;
         *  After a direct_meter object has been associated with a table as
         *  described in the documentation for the direct_meter
         *  constructor, every time the table is applied and a table entry
         *  is matched, the meter state associated with the matching entry
         *  is read, modified, and written back, atomically relative to the
         *  processing of other packets, regardless of whether the read()
         *  method is called in the body of that action.
         *
         *  read() may only be called within an action executed as a result
         *  of matching a table entry, of a table that has a direct_meter
         *  associated with it.  Calling read() causes an integer encoding
         *  of one of the colors green, yellow, or red to be written to the
         *  result out parameter.
         *
         *  @param result Type T must be bit<W> with W >= 2.  The value of
         *               result will be assigned 0 for color GREEN, 1 for
         *               color YELLOW, and 2 for color RED (see RFC 2697
         *               and RFC 2698 for the meaning of these colors).
         * ====================================================================================== */
        {"direct_meter.read",
         {"result"},
         [this](const IR::MethodCallExpression *call, const IR::Expression *receiver,
                IR::ID & /*methodName*/, const IR::Vector<IR::Argument> *args,
                const ExecutionState &state, SmallStepEvaluator::Result &result) {
             // Check whether table that the extern is attached to has an entry.
             const auto *progInfo = getProgramInfo().checkedTo<Bmv2V1ModelProgramInfo>();
             const auto *receiverPath = receiver->checkedTo<IR::PathExpression>();
             const auto *externInstance = state.findDecl(receiverPath);
             const auto *table = progInfo->getTableofDirectExtern(externInstance);
             const auto *tableEntry =
                 state.getTestObject("tableconfigs", table->controlPlaneName(), false);

             auto &nextState = state.clone();
             std::vector<Continuation::Command> replacements;
             const IR::StateVariable &meterResult =
                 ToolsVariables::convertReference(args->at(0)->expression);
             // If we do not have right back end and no associated table entry, do not bother
             // configuring the extern. We just set the default value (green).
             auto testBackend = TestgenOptions::get().testBackend;
             if (testBackend != "PTF" || tableEntry == nullptr) {
                 ::warning(
                     "direct_meter.read configuration not possible for %1%. Choosing default value "
                     "(GREEN).",
                     testBackend);
                 nextState.set(
                     meterResult,
                     IR::getConstant(meterResult->type,
                                     static_cast<big_int>(BMv2Constants::METER_COLOR::GREEN)));
                 nextState.popBody();
                 result->emplace_back(nextState);
                 return;
             }

             // Retrieve the meter state from the object store. If it is already present, just
             // cast the object to the correct class and retrieve the current value according to the
             // index. If the meter has not been added had, create a new meter object.
             const auto *meterState =
                 state.getTestObject("meter_values", externInstance->controlPlaneName(), false);
             Bmv2V1ModelMeterValue *meterValue = nullptr;
             const auto &inputValue = ToolsVariables::getSymbolicVariable(
                 meterResult->type, "meter_value" + std::to_string(call->clone_id));
             // Make sure we do not accidentally get "3" as enum assignment...
             auto *cond = new IR::Lss(inputValue, IR::getConstant(meterResult->type, 3));
             if (meterState != nullptr) {
                 meterValue =
                     new Bmv2V1ModelMeterValue(*meterState->checkedTo<Bmv2V1ModelMeterValue>());
             } else {
                 meterValue = new Bmv2V1ModelMeterValue(inputValue, true);
             }
             meterValue->writeToIndex(IR::getConstant(IR::getBitType(1), 0), inputValue);
             nextState.addTestObject("meter_values", externInstance->controlPlaneName(),
                                     meterValue);

             if (meterResult->type->is<IR::Type_Bits>()) {
                 // We need an assignment statement (and the inefficient copy) here because we need
                 // to immediately resolve the generated mux into multiple branches.
                 // This is only possible because meters do not return a value.
                 replacements.emplace_back(new IR::AssignmentStatement(meterResult, inputValue));
             } else {
                 TESTGEN_UNIMPLEMENTED("Read extern output %1% of type %2% not supported",
                                       meterResult, meterResult->type);
             }
             // TODO: Find a better way to model a trace of this event.
             std::stringstream meterStream;
             meterStream << "MeterRead: into field ";
             meterResult->dbprint(meterStream);
             nextState.add(*new TraceEvents::Generic(meterStream.str()));
             nextState.replaceTopBody(&replacements);
             result->emplace_back(cond, state, nextState);
             return;
         }},
        /* ======================================================================================
         *  digest
         *  Calling digest causes a message containing the values specified in
         *  the data parameter to be sent to the control plane software.  It is
         *  similar to sending a clone of the packet to the control plane
         *  software, except that it can be more efficient because the messages
         *  are typically smaller than packets, and many such small digest
         *  messages are typically coalesced together into a larger "batch"
         *  which the control plane software processes all at once.
         *
         *  The value of the fields that are sent in the message to the control
         *  plane is the value they have at the time the digest call occurs,
         *  even if those field values are changed by later ingress control
         *  code.  See Note 3.
         *
         *  Calling digest is only supported in the ingress control.  There is
         *  no way to undo its effects once it has been called.
         *
         *  If the type T is a named struct, the name is used to generate the
         *  control plane API.
         *
         *  The BMv2 implementation of the v1model architecture ignores the
         *  value of the receiver parameter.
         * ======================================================================================
         */
        {"*method.digest",
         {"receiver", "data"},
         [](const IR::MethodCallExpression * /*call*/, const IR::Expression * /*receiver*/,
            IR::ID & /*methodName*/, const IR::Vector<IR::Argument> * /*args*/,
            const ExecutionState &state, SmallStepEvaluator::Result &result) {
             ::warning("digest not fully implemented.");
             auto &nextState = state.clone();
             nextState.popBody();
             result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  clone_preserving_field_list
         *  Calling clone_preserving_field_list during execution of the ingress
         *  or egress control will cause the packet to be cloned, sometimes
         *  also called mirroring, i.e. zero or more copies of the packet are
         *  made, and each will later begin egress processing as an independent
         *  packet from the original packet.  The original packet continues
         *  with its normal next steps independent of the clone(s).
         *
         *  The session parameter is an integer identifying a clone session id
         *  (sometimes called a mirror session id).  The control plane software
         *  must configure each session you wish to use, or else no clones will
         *  be made using that session.  Typically this will involve the
         *  control plane software specifying one output port to which the
         *  cloned packet should be sent, or a list of (port, egress_rid) pairs
         *  to which a separate clone should be created for each, similar to
         *  multicast packets.
         *
         *  Cloned packets can be distinguished from others by the value of the
         *  standard_metadata instance_type field.
         *
         *  The user metadata fields that are tagged with @field_list(index) will be
         *  sent to the parser together with a clone of the packet.
         *
         *  If clone_preserving_field_list is called during ingress processing,
         *  the first parameter must be CloneType.I2E.  If
         *  clone_preserving_field_list is called during egress processing, the
         *  first parameter must be CloneType.E2E.
         *
         *  There is no way to undo its effects once it has been called.  If
         *  there are multiple calls to clone_preserving_field_list and/or
         *  clone during a single execution of the same ingress (or egress)
         *  control, only the last clone session and index are used.  See the
         *  v1model architecture documentation (Note 1) for more details.
         * ======================================================================================
         */
        {"*method.clone_preserving_field_list",
         {"type", "session", "data"},
         [](const IR::MethodCallExpression * /*call*/, const IR::Expression * /*receiver*/,
            IR::ID & /*methodName*/, const IR::Vector<IR::Argument> *args,
            const ExecutionState &state, SmallStepEvaluator::Result &result) {
             // Grab the recirculate count. Stop after more than 1 circulation loop to avoid
             // infinite recirculation loops.
             // TODO: Determine the exact count.
             if (state.hasProperty("recirculate_count")) {
                 if (state.getProperty<uint64_t>("recirculate_count") > 0) {
                     auto &nextState = state.clone();
                     ::warning("Only single recirculation supported for now. Dropping packet.");
                     auto *dropStmt = new IR::MethodCallStatement(
                         Utils::generateInternalMethodCall("drop_and_exit", {}));
                     nextState.replaceTopBody(dropStmt);
                     result->emplace_back(nextState);
                     return;
                 }
             }
             bool argsAreTainted = false;
             for (size_t idx = 0; idx < args->size(); ++idx) {
                 const auto *arg = args->at(idx);
                 argsAreTainted = argsAreTainted || Taint::hasTaint(arg->expression);
             }
             // If any of the input arguments is tainted, the entire extern is unreliable.
             if (argsAreTainted) {
                 ::warning("clone args are tainted and not predictable. Skipping clone execution.");
                 auto &nextState = state.clone();
                 nextState.popBody();
                 result->emplace_back(nextState);
                 return;
             }

             auto cloneType = args->at(0)->expression->checkedTo<IR::Constant>()->asUint64();
             const auto *sessionIdExpr = args->at(1)->expression;
             auto preserveIndex = args->at(2)->expression->checkedTo<IR::Constant>()->asInt();
             // This is the clone state. Clone the state and save it.
             // TODO: Do we really need to clone twice?
             auto *cloneInfo = new Bmv2V1ModelCloneInfo(
                 sessionIdExpr, static_cast<BMv2Constants::CloneType>(cloneType), state.clone(),
                 preserveIndex);
             auto &nextState = state.clone();
             nextState.addTestObject("clone_infos", "clone_info", cloneInfo);
             // Also set clone as active, which will trigger "processClone" in the deparser.
             nextState.setProperty("clone_active", true);
             nextState.popBody();
             result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  resubmit_preserving_field_list
         *  Calling resubmit_preserving_field_list during execution of the
         *  ingress control will cause the packet to be resubmitted, i.e. it
         *  will begin processing again with the parser, with the contents of
         *  the packet exactly as they were when it last began parsing.  The
         *  only difference is in the value of the standard_metadata
         *  instance_type field, and any user-defined metadata fields that the
         *  resubmit_preserving_field_list operation causes to be preserved.
         *
         *  The user metadata fields that are tagged with @field_list(index) will
         *  be sent to the parser together with the packet.
         *
         *  Calling resubmit_preserving_field_list is only supported in the
         *  ingress control.  There is no way to undo its effects once it has
         *  been called.  If resubmit_preserving_field_list is called multiple
         *  times during a single execution of the ingress control, only one
         *  packet is resubmitted, and only the user-defined metadata fields
         *  specified by the field list index from the last such call are
         *  preserved.  See the v1model architecture documentation (Note 1) for
         *  more details.
         *
         *  For example, the user metadata fields can be annotated as follows:
         *  struct UM {
         *     @field_list(1)
         *     bit<32> x;
         *     @field_list(1, 2)
         *     bit<32> y;
         *     bit<32> z;
         *  }
         *
         *  Calling resubmit_preserving_field_list(1) will resubmit the packet
         *  and preserve fields x and y of the user metadata.  Calling
         *  resubmit_preserving_field_list(2) will only preserve field y.
         * ======================================================================================
         */
        {"*method.resubmit_preserving_field_list",
         {"data"},
         [](const IR::MethodCallExpression * /*call*/, const IR::Expression * /*receiver*/,
            IR::ID & /*methodName*/, const IR::Vector<IR::Argument> *args,
            const ExecutionState &state, SmallStepEvaluator::Result &result) {
             uint64_t recirculateCount = 0;
             auto &nextState = state.clone();
             // Grab the recirculate count. Stop after more than 1 circulation loop to avoid
             // infinite recirculation loops.
             // TODO: Determine the exact count.
             if (state.hasProperty("recirculate_count")) {
                 recirculateCount = state.getProperty<uint64_t>("recirculate_count");
                 if (recirculateCount > 0) {
                     ::warning("Only single resubmit supported for now. Dropping packet.");
                     auto *dropStmt = new IR::MethodCallStatement(
                         Utils::generateInternalMethodCall("drop_and_exit", {}));
                     nextState.replaceTopBody(dropStmt);
                     result->emplace_back(nextState);
                     return;
                 }
             }
             // Increment the recirculation count.
             nextState.setProperty("recirculate_count", ++recirculateCount);
             // Recirculate is now active and "processRecirculate" will be triggered in the
             // deparser.
             nextState.setProperty("recirculate_active", true);
             // Grab the index and save it to the execution state.
             auto index = args->at(0)->expression->checkedTo<IR::Constant>()->asUint64();
             nextState.setProperty("recirculate_index", index);
             // Resubmit actually uses the original input packet, not the deparsed packet.
             // We have to reset the packet content to the input packet in "processRecirculate".
             nextState.setProperty("recirculate_reset_pkt", true);
             // Set the appropriate instance type, which will be processed by
             // "processRecirculate".
             nextState.setProperty("recirculate_instance_type",
                                   BMv2Constants::PKT_INSTANCE_TYPE_RESUBMIT);
             nextState.popBody();
             result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  recirculate_preserving_field_list
         * Calling recirculate_preserving_field_list during execution of the
         * egress control will cause the packet to be recirculated, i.e. it
         * will begin processing again with the parser, with the contents of
         * the packet as they are created by the deparser.  Recirculated
         * packets can be distinguished from new packets in ingress processing
         * by the value of the standard_metadata instance_type field.  The
         * caller may request that some user-defined metadata fields be
         * preserved with the recirculated packet.
         * The user metadata fields that are tagged with @field_list(index) will be
         * sent to the parser together with the packet.
         * Calling recirculate_preserving_field_list is only supported in the
         * egress control.  There is no way to undo its effects once it has
         * been called.  If recirculate_preserving_field_list is called
         * multiple times during a single execution of the egress control,
         * only one packet is recirculated, and only the user-defined metadata
         * fields specified by the field list index from the last such call
         * are preserved.  See the v1model architecture documentation (Note 1)
         * for more details.
         * ======================================================================================
         */
        {"*method.recirculate_preserving_field_list",
         {"index"},
         [](const IR::MethodCallExpression * /*call*/, const IR::Expression * /*receiver*/,
            IR::ID & /*methodName*/, const IR::Vector<IR::Argument> *args,
            const ExecutionState &state, SmallStepEvaluator::Result &result) {
             uint64_t recirculateCount = 0;
             auto &nextState = state.clone();
             // Grab the recirculate count. Stop after more than 1 circulation loop to avoid
             // infinite recirculation loops.
             // TODO: Determine the exact count.
             if (state.hasProperty("recirculate_count")) {
                 recirculateCount = state.getProperty<uint64_t>("recirculate_count");
                 if (recirculateCount > 0) {
                     ::warning("Only single recirculation supported for now. Dropping packet.");
                     auto *dropStmt = new IR::MethodCallStatement(
                         Utils::generateInternalMethodCall("drop_and_exit", {}));
                     nextState.replaceTopBody(dropStmt);
                     result->emplace_back(nextState);
                     return;
                 }
             }
             // Increment the recirculation count.
             nextState.setProperty("recirculate_count", ++recirculateCount);
             // Recirculate is now active and "processRecirculate" will be triggered in the
             // deparser.
             nextState.setProperty("recirculate_active", true);
             // Grab the index and save it to the execution state.
             auto index = args->at(0)->expression->checkedTo<IR::Constant>()->asUint64();
             nextState.setProperty("recirculate_index", index);
             // Set the appropriate instance type, which will be processed by
             // "processRecirculate".
             nextState.setProperty("recirculate_instance_type",
                                   BMv2Constants::PKT_INSTANCE_TYPE_RECIRC);
             nextState.popBody();
             result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  clone
         *  clone is in most ways identical to the clone_preserving_field_list
         *  operation, with the only difference being that it never preserves
         *  any user-defined metadata fields with the cloned packet.  It is
         *  equivalent to calling clone_preserving_field_list with the same
         *  type and session parameter values, with empty data.
         * ======================================================================================
         */
        {"*method.clone",
         {"type", "session"},
         [](const IR::MethodCallExpression * /*call*/, const IR::Expression * /*receiver*/,
            IR::ID & /*methodName*/, const IR::Vector<IR::Argument> *args,
            const ExecutionState &state, SmallStepEvaluator::Result &result) {
             // Grab the recirculate count. Stop after more than 1 circulation loop to avoid
             // infinite recirculation loops.
             // TODO: Determine the exact count.
             if (state.hasProperty("recirculate_count")) {
                 if (state.getProperty<uint64_t>("recirculate_count") > 0) {
                     auto &nextState = state.clone();
                     ::warning("Only single recirculation supported for now. Dropping packet.");
                     auto *dropStmt = new IR::MethodCallStatement(
                         Utils::generateInternalMethodCall("drop_and_exit", {}));
                     nextState.replaceTopBody(dropStmt);
                     result->emplace_back(nextState);
                     return;
                 }
             }
             bool argsAreTainted = false;
             for (size_t idx = 0; idx < args->size(); ++idx) {
                 const auto *arg = args->at(idx);
                 argsAreTainted = argsAreTainted || Taint::hasTaint(arg->expression);
             }
             // If any of the input arguments is tainted, the entire extern is unreliable.
             if (argsAreTainted) {
                 ::warning("clone args are tainted and not predictable. Skipping clone execution.");
                 auto &nextState = state.clone();
                 nextState.popBody();
                 result->emplace_back(nextState);
                 return;
             }

             auto cloneType = args->at(0)->expression->checkedTo<IR::Constant>()->asUint64();
             const auto *sessionIdExpr = args->at(1)->expression;
             auto &nextState = state.clone();
             // This is the clone state. Clone the state and save it.
             // TODO: Do we really need to clone twice?
             auto *cloneInfo = new Bmv2V1ModelCloneInfo(
                 sessionIdExpr, static_cast<BMv2Constants::CloneType>(cloneType), state.clone(),
                 std::nullopt);
             nextState.addTestObject("clone_infos", "clone_info", cloneInfo);
             // Also set clone as active, which will trigger "processClone" in the deparser.
             nextState.setProperty("clone_active", true);
             nextState.popBody();
             result->emplace_back(nextState);
         }},
        /* ======================================================================================
         *  *invoke_traffic_manager
         * ======================================================================================
         */
        /// Helper extern that processes the parameters set by the recirculate, clone and resubmit
        /// externs. This extern assume the TM is executed at the end of the deparser.
        {"*.invoke_traffic_manager",
         {},
         [this](const IR::MethodCallExpression * /*call*/, const IR::Expression * /*receiver*/,
                IR::ID & /*methodName*/, const IR::Vector<IR::Argument> * /*args*/,
                const ExecutionState &state, SmallStepEvaluator::Result &result) {
             // Check whether the clone variant is  active.
             // Clone triggers a branch and slightly different processing.
             if (state.hasProperty("clone_active") && state.getProperty<bool>("clone_active")) {
                 processClone(state, result);
                 return;
             }

             // Check whether recirculate is active.
             if (state.hasProperty("recirculate_active") &&
                 state.getProperty<bool>("recirculate_active")) {
                 processRecirculate(state, result);
                 return;
             }
             // Neither clone nor recirculate are active, so skip.
             auto &nextState = state.clone();
             nextState.popBody();
             result->emplace_back(nextState);
         }},
        /* ======================================================================================
         * Checksum16.get
         * ======================================================================================
         */
        {"Checksum16.get",
         {"data"},
         [](const IR::MethodCallExpression * /*call*/, const IR::Expression * /*receiver*/,
            IR::ID & /*methodName*/, const IR::Vector<IR::Argument> * /*args*/,
            const ExecutionState & /*state*/, SmallStepEvaluator::Result & /*result*/) {
             P4C_UNIMPLEMENTED("Checksum16.get is deprecated and not supported.");
         }},
        /* ======================================================================================
         * verify_checksum
         *  Verifies the checksum of the supplied data.  If this method detects
         *  that a checksum of the data is not correct, then the value of the
         *  standard_metadata checksum_error field will be equal to 1 when the
         *  packet begins ingress processing.
         *
         *  Calling verify_checksum is only supported in the VerifyChecksum
         *  control.
         *
         *  @param T          Must be a tuple type where all the tuple elements
         *                    are of type bit<W>, int<W>, or varbit<W>.  The
         *                    total length of the fields must be a multiple of
         *                    the output size.
         *  @param O          Checksum type; must be bit<X> type.
         *  @param condition  If 'false' the verification always succeeds.
         *  @param data       Data whose checksum is verified.
         *  @param checksum   Expected checksum of the data; note that it must
         *                    be a left-value.
         *  @param algo       Algorithm to use for checksum (not all algorithms
         *                    may be supported).  Must be a compile-time
         *                    constant.
         * ======================================================================================
         */
        {"*method.verify_checksum",
         {"condition", "data", "checksum", "algo"},
         [this](const IR::MethodCallExpression *call, const IR::Expression * /*receiver*/,
                IR::ID & /*methodName*/, const IR::Vector<IR::Argument> *args,
                const ExecutionState &state, SmallStepEvaluator::Result &result) {
             bool argsAreTainted = false;
             // If any of the input arguments is tainted, the entire extern is unreliable.
             for (size_t idx = 0; idx < args->size(); ++idx) {
                 const auto *arg = args->at(idx);
                 argsAreTainted = argsAreTainted || Taint::hasTaint(arg->expression);
             }

             const auto *verifyCond = args->at(0)->expression;
             const auto *data = args->at(1)->expression;
             const auto *checksumValue = args->at(2)->expression;
             const auto *checksumValueType = checksumValue->type;
             const auto *algo = args->at(3)->expression;
             const auto *oneBitType = IR::getBitType(1);

             // In some cases the condition is false already. No need to do complex processing then.
             if (const auto *boolVal = verifyCond->to<IR::BoolLiteral>()) {
                 if (!boolVal->value) {
                     auto &taintedState = state.clone();
                     taintedState.popBody();
                     result->emplace_back(taintedState);
                     return;
                 }
             }

             // Handle the case where the condition might be true.
             // If the condition is tainted or the input data is tainted, the checksum error
             // will not be reliable.
             if (argsAreTainted) {
                 auto &taintedState = state.clone();
                 const auto *checksumErr = new IR::Member(
                     oneBitType, new IR::PathExpression("*standard_metadata"), "checksum_error");
                 taintedState.set(checksumErr,
                                  programInfo.createTargetUninitialized(checksumErr->type, true));
                 taintedState.popBody();
                 result->emplace_back(taintedState);
                 return;
             }

             // Generate the checksum arguments.
             auto *checksumArgs = new IR::Vector<IR::Argument>();
             checksumArgs->push_back(new IR::Argument(checksumValue));
             checksumArgs->push_back(new IR::Argument(algo));
             checksumArgs->push_back(new IR::Argument(data));

             // The condition is true and the checksum matches.
             {
                 // Try to force the checksum expression to be equal to the result.
                 auto &nextState = state.clone();
                 const auto *concolicVar = new IR::ConcolicVariable(
                     checksumValueType, "*method_checksum", checksumArgs, call->clone_id, 0);
                 std::vector<Continuation::Command> replacements;
                 // We use a guard to enforce that the match condition after the call is true.
                 auto *checksumMatchCond = new IR::Equ(concolicVar, checksumValue);
                 nextState.popBody();
                 result->emplace_back(new IR::LAnd(checksumMatchCond, verifyCond), state,
                                      nextState);
             }

             // The condition is true and the checksum does not match.
             {
                 auto &nextState = state.clone();
                 auto *concolicVar = new IR::ConcolicVariable(checksumValueType, "*method_checksum",
                                                              checksumArgs, call->clone_id, 0);
                 std::vector<Continuation::Command> replacements;
                 auto *checksumMatchCond = new IR::Neq(concolicVar, checksumValue);

                 const auto *checksumErr = new IR::Member(
                     oneBitType, new IR::PathExpression("*standard_metadata"), "checksum_error");
                 const auto *assign =
                     new IR::AssignmentStatement(checksumErr, IR::getConstant(oneBitType, 1));
                 auto *errorCond = new IR::LAnd(verifyCond, checksumMatchCond);
                 replacements.emplace_back(assign);
                 nextState.replaceTopBody(&replacements);
                 result->emplace_back(errorCond, state, nextState);
             }

             // Handle the case where the condition is false.
             {
                 auto &nextState = state.clone();
                 nextState.popBody();
                 result->emplace_back(new IR::LNot(IR::Type::Boolean::get(), verifyCond), state,
                                      nextState);
             }
         }},
        /* ======================================================================================
         * update_checksum
         *  Computes the checksum of the supplied data and writes it to the
         *  checksum parameter.
         *  Calling update_checksum is only supported in the ComputeChecksum
         *  control.
         *  @param T          Must be a tuple type where all the tuple elements
         *                    are of type bit<W>, int<W>, or varbit<W>.  The
         *                    total length of the fields must be a multiple of
         *                    the output size.
         *  @param O          Output type; must be bit<X> type.
         *  @param condition  If 'false' the checksum parameter is not changed
         *  @param data       Data whose checksum is computed.
         *  @param checksum   Checksum of the data.
         *  @param algo       Algorithm to use for checksum (not all algorithms
         *                    may be supported).  Must be a compile-time
         *                    constant.
         * ======================================================================================
         */
        {"*method.update_checksum",
         {"condition", "data", "checksum", "algo"},
         [this](const IR::MethodCallExpression *call, const IR::Expression * /*receiver*/,
                IR::ID & /*methodName*/, const IR::Vector<IR::Argument> *args,
                const ExecutionState &state, SmallStepEvaluator::Result &result) {
             bool argsAreTainted = false;
             // If any of the input arguments is tainted, the entire extern is unreliable.
             for (size_t idx = 0; idx < args->size() - 2; ++idx) {
                 const auto *arg = args->at(idx);
                 argsAreTainted = argsAreTainted || Taint::hasTaint(arg->expression);
             }

             const auto &checksumVar =
                 args->at(2)->expression->checkedTo<IR::InOutReference>()->ref;
             const auto *updateCond = args->at(0)->expression;
             const auto *checksumVarType = checksumVar->type;
             const auto *data = args->at(1)->expression;
             const auto *algo = args->at(3)->expression;

             // In some cases the condition is false already. No need to do complex processing then.
             if (const auto *boolVal = updateCond->to<IR::BoolLiteral>()) {
                 if (!boolVal->value) {
                     auto &taintedState = state.clone();
                     taintedState.popBody();
                     result->emplace_back(taintedState);
                     return;
                 }
             }

             // Handle the case where the condition might be true.
             // If the condition is tainted or the input data is tainted, the checksum error
             // will not be reliable.
             // If the condition is tainted or the input data is tainted.
             // The checksum will also be tainted.
             if (argsAreTainted) {
                 auto &taintedState = state.clone();
                 taintedState.set(checksumVar,
                                  programInfo.createTargetUninitialized(checksumVarType, true));
                 taintedState.popBody();
                 result->emplace_back(taintedState);
                 return;
             }

             // Handle the case where the condition is true.
             {
                 // Generate the checksum arguments.
                 auto *checksumArgs = new IR::Vector<IR::Argument>();
                 checksumArgs->push_back(new IR::Argument(checksumVar));
                 checksumArgs->push_back(new IR::Argument(algo));
                 checksumArgs->push_back(new IR::Argument(data));

                 auto &nextState = state.clone();
                 const auto *concolicVar = new IR::ConcolicVariable(
                     checksumVarType, "*method_checksum", checksumArgs, call->clone_id, 0);
                 nextState.set(checksumVar, concolicVar);
                 nextState.popBody();
                 result->emplace_back(updateCond, state, nextState);
             }
             // Handle the case where the condition is false. No change here.
             {
                 auto &nextState = state.clone();
                 nextState.popBody();
                 result->emplace_back(new IR::LNot(IR::Type::Boolean::get(), updateCond), state,
                                      nextState);
             }
         }},
        /* ======================================================================================
         * update_checksum_with_payload
         *  update_checksum_with_payload is identical in all ways to
         *  update_checksum, except that it includes the payload of the packet
         *  in the checksum calculation.  The payload is defined as "all bytes
         *  of the packet which were not parsed by the parser".
         *  Calling update_checksum_with_payload is only supported in the
         *  ComputeChecksum control.
         * ======================================================================================
         */
        {"*method.update_checksum_with_payload",
         {"condition", "data", "checksum", "algo"},
         [this](const IR::MethodCallExpression *call, const IR::Expression * /*receiver*/,
                IR::ID & /*methodName*/, const IR::Vector<IR::Argument> *args,
                const ExecutionState &state, SmallStepEvaluator::Result &result) {
             bool argsAreTainted = false;
             // If any of the input arguments is tainted, the entire extern is unreliable.
             for (size_t idx = 0; idx < args->size() - 2; ++idx) {
                 const auto *arg = args->at(idx);
                 argsAreTainted = argsAreTainted || Taint::hasTaint(arg->expression);
             }

             const auto &checksumVar =
                 args->at(2)->expression->checkedTo<IR::InOutReference>()->ref;
             const auto *updateCond = args->at(0)->expression;
             const auto *checksumVarType = checksumVar->type;
             const auto *data = args->at(1)->expression;
             const auto *algo = args->at(3)->expression;
             // If the condition is tainted or the input data is tainted.
             // The checksum will also be tainted.
             if (argsAreTainted) {
                 auto &taintedState = state.clone();
                 taintedState.set(checksumVar,
                                  programInfo.createTargetUninitialized(checksumVarType, true));
                 taintedState.popBody();
                 result->emplace_back(taintedState);
                 return;
             }

             // Handle the case where the condition is true.
             {
                 // Generate the checksum arguments.
                 auto *checksumArgs = new IR::Vector<IR::Argument>();
                 checksumArgs->push_back(new IR::Argument(checksumVar));
                 checksumArgs->push_back(new IR::Argument(algo));
                 checksumArgs->push_back(new IR::Argument(data));

                 auto &nextState = state.clone();
                 const auto *concolicVar =
                     new IR::ConcolicVariable(checksumVarType, "*method_checksum_with_payload",
                                              checksumArgs, call->clone_id, 0);
                 nextState.set(checksumVar, concolicVar);
                 nextState.popBody();
                 result->emplace_back(updateCond, state, nextState);
             }
             // Handle the case where the condition is false. No change here.
             {
                 auto &nextState = state.clone();
                 nextState.popBody();
                 result->emplace_back(new IR::LNot(IR::Type::Boolean::get(), updateCond), state,
                                      nextState);
             }
         }},
        /* ======================================================================================
         * verify_checksum_with_payload
         *  verify_checksum_with_payload is identical in all ways to
         *  verify_checksum, except that it includes the payload of the packet
         *  in the checksum calculation.  The payload is defined as "all bytes
         *  of the packet which were not parsed by the parser".
         *  Calling verify_checksum_with_payload is only supported in the
         *  VerifyChecksum control.
         * ======================================================================================
         */
        {"*method.verify_checksum_with_payload",
         {"condition", "data", "checksum", "algo"},
         [this](const IR::MethodCallExpression *call, const IR::Expression * /*receiver*/,
                IR::ID & /*methodName*/, const IR::Vector<IR::Argument> *args,
                const ExecutionState &state, SmallStepEvaluator::Result &result) {
             bool argsAreTainted = false;
             // If any of the input arguments is tainted, the entire extern is unreliable.
             for (size_t idx = 0; idx < args->size(); ++idx) {
                 const auto *arg = args->at(idx);
                 argsAreTainted = argsAreTainted || Taint::hasTaint(arg->expression);
             }

             const auto *verifyCond = args->at(0)->expression;
             const auto *data = args->at(1)->expression;
             const auto *checksumValue = args->at(2)->expression;
             const auto *checksumValueType = checksumValue->type;
             const auto *algo = args->at(3)->expression;
             const auto *oneBitType = IR::getBitType(1);
             // If the condition is tainted or the input data is tainted, the checksum error
             // will not be reliable.
             if (argsAreTainted) {
                 auto &taintedState = state.clone();
                 const auto *checksumErr = new IR::Member(
                     oneBitType, new IR::PathExpression("*standard_metadata"), "checksum_error");
                 taintedState.set(checksumErr,
                                  programInfo.createTargetUninitialized(checksumErr->type, true));
                 taintedState.popBody();
                 result->emplace_back(taintedState);
                 return;
             }

             // Generate the checksum arguments.
             auto *checksumArgs = new IR::Vector<IR::Argument>();
             checksumArgs->push_back(new IR::Argument(checksumValue));
             checksumArgs->push_back(new IR::Argument(algo));
             checksumArgs->push_back(new IR::Argument(data));

             // The condition is true and the checksum matches.
             {
                 // Try to force the checksum expression to be equal to the result.
                 auto &nextState = state.clone();
                 const auto *concolicVar =
                     new IR::ConcolicVariable(checksumValueType, "*method_checksum_with_payload",
                                              checksumArgs, call->clone_id, 0);
                 // We use a guard to enforce that the match condition after the call is true.
                 auto *checksumMatchCond = new IR::Equ(concolicVar, checksumValue);
                 nextState.popBody();
                 result->emplace_back(new IR::LAnd(checksumMatchCond, verifyCond), state,
                                      nextState);
             }

             // The condition is true and the checksum does not match.
             {
                 auto &nextState = state.clone();
                 auto *concolicVar =
                     new IR::ConcolicVariable(checksumValueType, "*method_checksum_with_payload",
                                              checksumArgs, call->clone_id, 0);
                 std::vector<Continuation::Command> replacements;
                 auto *checksumMatchCond = new IR::Neq(concolicVar, checksumValue);

                 const auto *checksumErr = new IR::Member(
                     oneBitType, new IR::PathExpression("*standard_metadata"), "checksum_error");
                 const auto *assign =
                     new IR::AssignmentStatement(checksumErr, IR::getConstant(oneBitType, 1));
                 auto *errorCond = new IR::LAnd(verifyCond, checksumMatchCond);
                 replacements.emplace_back(assign);
                 nextState.replaceTopBody(&replacements);
                 result->emplace_back(errorCond, state, nextState);
             }
             // Handle the case where the condition is false. No change here.
             {
                 auto &nextState = state.clone();
                 nextState.popBody();
                 result->emplace_back(new IR::LNot(IR::Type::Boolean::get(), verifyCond), state,
                                      nextState);
             }
         }},
    });

    if (!EXTERN_METHOD_IMPLS.exec(call, receiver, name, args, state, result)) {
        ExprStepper::evalExternMethodCall(call, receiver, name, args, state);
    }
}  // NOLINT

bool Bmv2V1ModelExprStepper::preorder(const IR::P4Table *table) {
    // Delegate to the tableStepper.
    Bmv2V1ModelTableStepper tableStepper(this, table);

    return tableStepper.eval();
}

}  // namespace P4Tools::P4Testgen::Bmv2
